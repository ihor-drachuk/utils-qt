/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <gtest/gtest.h>

#include <chrono>

#include <QtConcurrent/QtConcurrent>
#include <QThreadPool>
#include <QEventLoop>
#include <QString>
#include <QThread>

#include <UtilsQt/Futures/Sequential.h>
#include <UtilsQt/Futures/Utils.h>
#include <utils-cpp/threadid.h>

namespace {

#ifdef UTILS_QT_OS_MACOS
constexpr auto TimeFactor = 5;
#else
constexpr auto TimeFactor = 1;
#endif // UTILS_QT_OS_MACOS

struct OpStatistics
{
    int constructions = 0;
    int destructions = 0;
    int moves = 0;
    int copies = 0;
    int assignments = 0;
    int movesAssignments = 0;
};

struct MovableOnly
{
    MovableOnly() = default;
    MovableOnly(const MovableOnly&) = delete;
    MovableOnly(MovableOnly&&) noexcept = default;
    MovableOnly& operator=(const MovableOnly&) = delete;
    MovableOnly& operator=(MovableOnly&&) noexcept = default;
};

struct CopyableOnly
{
    CopyableOnly() = default;
    CopyableOnly(const CopyableOnly&) = default;
    CopyableOnly(CopyableOnly&&) noexcept = delete;
    CopyableOnly& operator=(const CopyableOnly&) = default;
    CopyableOnly& operator=(CopyableOnly&&) noexcept = delete;
};

struct MovableCopyable
{
    MovableCopyable() { stats->constructions++; }
    MovableCopyable(const MovableCopyable& other)
    {
        stats = other.stats;
        stats->copies++;
    }

    MovableCopyable(MovableCopyable&& other) noexcept
    {
        stats = other.stats;
        stats->moves++;
        other.isMoved = true;
    }

    ~MovableCopyable() { stats->destructions++; }

    MovableCopyable& operator=(const MovableCopyable& other)
    {
        if (this == &other)
            return *this;

        stats->assignments++;

        return *this;
    }

    MovableCopyable& operator=(MovableCopyable&& other) noexcept
    {
        if (this == &other)
            return *this;

        stats->movesAssignments++;
        other.isMoved = true;

        return *this;
    }

    std::shared_ptr<OpStatistics> stats { std::make_shared<OpStatistics>() };
    bool isMoved {false};
};

struct MovableCopyableFunctor
{
    bool isMoved {false};
    bool isCopied {false};

    MovableCopyableFunctor() = default;
    MovableCopyableFunctor(const MovableCopyableFunctor& other) { const_cast<MovableCopyableFunctor&>(other).isCopied = true; }
    MovableCopyableFunctor(MovableCopyableFunctor&& other) noexcept { other.isMoved = true; }

    auto operator()(const UtilsQt::AsyncResult<QString>& r) {
        r.tryRethrow();
        return UtilsQt::createReadyFuture(r.value());
    }
};

struct MovableCopyableCancellableFunctor
{
    bool isMoved {false};
    bool isCopied {false};
    bool* isCanceledPtr {nullptr};

    MovableCopyableCancellableFunctor(bool* isCanceled = nullptr): isCanceledPtr(isCanceled) {}
    MovableCopyableCancellableFunctor(const MovableCopyableCancellableFunctor& other) { const_cast<MovableCopyableCancellableFunctor&>(other).isCopied = true; }
    MovableCopyableCancellableFunctor(MovableCopyableCancellableFunctor&& other) noexcept { other.isMoved = true; }

    auto operator()(const UtilsQt::AsyncResult<QString>& r, const UtilsQt::CancelStatus& c) const {
        if (isCanceledPtr)
            *isCanceledPtr |= c.isCancelRequested();

        r.tryRethrow();
        return UtilsQt::createReadyFuture(r.value());
    }
};

class MyException: public std::exception
{
public:
    MyException() = default;
    MyException(const MyException&) = default;
    MyException(MyException&&) = default;
    MyException& operator=(const MyException&) = default;
    MyException& operator=(MyException&&) = default;
};

} // namespace

TEST(UtilsQt, Futures_Sequential_ImmediateResult)
{
    QObject obj;
    auto f = UtilsQt::Sequential(&obj)
                 .start([](){ return UtilsQt::createReadyFuture(15); })
                 .then([](const UtilsQt::AsyncResult<int>& r){ r.tryRethrow(); return UtilsQt::createReadyFuture(QString::number(r.value())); })
                 .execute();

    UtilsQt::waitForFuture<QEventLoop>(f);
    ASSERT_TRUE(f.isFinished());
    ASSERT_FALSE(f.isCanceled());
    ASSERT_EQ(f.result(), QString("15"));
}

TEST(UtilsQt, Futures_Sequential_DelayedResult)
{
    QObject obj;
    auto f = UtilsQt::Sequential(&obj)
                 .start([](){ return UtilsQt::createTimedFuture(10, 123); })
                 .then([](const UtilsQt::AsyncResult<int>& r){ r.tryRethrow(); return UtilsQt::createTimedFuture(20, QString::number(r.value())); })
                 .execute();

    UtilsQt::waitForFuture<QEventLoop>(f);
    ASSERT_TRUE(f.isFinished());
    ASSERT_FALSE(f.isCanceled());
    ASSERT_EQ(f.result(), QString("123"));
}

TEST(UtilsQt, Futures_Sequential_StartWithFuture)
{
    QObject obj;
    auto f = UtilsQt::Sequential(&obj)
                 .start(UtilsQt::createReadyFuture(15))
                 .then([](const UtilsQt::AsyncResult<int>& r){ r.tryRethrow(); return UtilsQt::createReadyFuture(QString::number(r.value())); })
                 .execute();

    UtilsQt::waitForFuture<QEventLoop>(f);
    ASSERT_TRUE(f.isFinished());
    ASSERT_FALSE(f.isCanceled());
    ASSERT_EQ(f.result(), QString("15"));
}

TEST(UtilsQt, Futures_Sequential_Movable)
{
    MovableOnly m1;
    MovableOnly m2;

    QObject obj;
    auto f = UtilsQt::Sequential(&obj)
                 .start([m1 = std::move(m1)](){ (void)m1; return UtilsQt::createReadyFuture(15); })
                 .then([m2 = std::move(m2)](const UtilsQt::AsyncResult<int>& r){ (void)m2; r.tryRethrow(); return UtilsQt::createReadyFuture(QString::number(r.value())); })
                 .execute();

    UtilsQt::waitForFuture<QEventLoop>(f);
    ASSERT_TRUE(f.isFinished());
    ASSERT_FALSE(f.isCanceled());
    ASSERT_EQ(f.result(), QString("15"));
}

TEST(UtilsQt, Futures_Sequential_Copyable)
{
    CopyableOnly m1;
    CopyableOnly m2;

    QObject obj;
    auto f = UtilsQt::Sequential(&obj)
                 .start([m1](){ (void)m1; return UtilsQt::createReadyFuture(15); })
                 .then([m2](const UtilsQt::AsyncResult<int>& r){ (void)m2; r.tryRethrow(); return UtilsQt::createReadyFuture(QString::number(r.value())); })
                 .execute();

    UtilsQt::waitForFuture<QEventLoop>(f);
    ASSERT_TRUE(f.isFinished());
    ASSERT_FALSE(f.isCanceled());
    ASSERT_EQ(f.result(), QString("15"));
}

TEST(UtilsQt, Futures_Sequential_MovableCopyable)
{
    // Object is movable and copyable.
    // As move wasn't requested, it should be copied.
    // But internally it should be moved.

    MovableCopyable mc1;
    MovableCopyable mc2;
    MovableCopyableFunctor mc3;

    QObject obj;
    auto f = UtilsQt::Sequential(&obj)
                 .start([mc1](){ (void)mc1; return UtilsQt::createReadyFuture(15); })
                 .then([mc2](const UtilsQt::AsyncResult<int>& r){ (void)mc2; r.tryRethrow(); return UtilsQt::createReadyFuture(QString::number(r.value())); })
                 .then(mc3)
                 .execute();

    ASSERT_FALSE(mc1.isMoved);
    ASSERT_FALSE(mc2.isMoved);
    ASSERT_FALSE(mc3.isMoved);
    ASSERT_TRUE(mc3.isCopied);

    auto mc3moved = std::move(mc3);
    ASSERT_TRUE(mc3.isMoved); // NOLINT

    UtilsQt::waitForFuture<QEventLoop>(f);
    ASSERT_TRUE(f.isFinished());
    ASSERT_FALSE(f.isCanceled());
    ASSERT_EQ(f.result(), QString("15"));

    ASSERT_EQ(mc1.stats->copies, 1);
    ASSERT_EQ(mc2.stats->copies, 1);
}

TEST(UtilsQt, Futures_Sequential_Cancellable_Movable)
{
    MovableOnly m1;
    MovableOnly m2;

    QObject obj;
    bool cf = false;
    auto f = UtilsQt::Sequential(&obj)
                 .start([m1 = std::move(m1), &cf](const UtilsQt::CancelStatus& c){ (void)m1; cf |= c.isCancelRequested(); return UtilsQt::createReadyFuture(15); })
                 .then([m2 = std::move(m2), &cf](const UtilsQt::AsyncResult<int>& r, const UtilsQt::CancelStatus& c){ (void)m2; cf |= c.isCancelRequested(); r.tryRethrow(); return UtilsQt::createReadyFuture(QString::number(r.value())); })
                 .execute();

    UtilsQt::waitForFuture<QEventLoop>(f);
    ASSERT_TRUE(f.isFinished());
    ASSERT_FALSE(f.isCanceled());
    ASSERT_EQ(f.result(), QString("15"));

    ASSERT_FALSE(cf);
}

TEST(UtilsQt, Futures_Sequential_Cancellable_Copyable)
{
    CopyableOnly m1;
    CopyableOnly m2;

    QObject obj;
    bool cf = false;
    auto f = UtilsQt::Sequential(&obj)
                 .start([m1, &cf](const UtilsQt::CancelStatus& c){ (void)m1; cf |= c.isCancelRequested(); return UtilsQt::createReadyFuture(15); })
                 .then([m2, &cf](const UtilsQt::AsyncResult<int>& r, const UtilsQt::CancelStatus& c){ (void)m2; cf |= c.isCancelRequested(); r.tryRethrow(); return UtilsQt::createReadyFuture(QString::number(r.value())); })
                 .execute();

    UtilsQt::waitForFuture<QEventLoop>(f);
    ASSERT_TRUE(f.isFinished());
    ASSERT_FALSE(f.isCanceled());
    ASSERT_EQ(f.result(), QString("15"));

    ASSERT_FALSE(cf);
}

TEST(UtilsQt, Futures_Sequential_Cancellable_MovableCopyable)
{
    QObject obj;
    bool cf = false;

    MovableCopyable mc1;
    MovableCopyable mc2;
    MovableCopyableCancellableFunctor mc3 (&cf);

    auto f = UtilsQt::Sequential(&obj)
                 .start([mc1, &cf](const UtilsQt::CancelStatus& c){ (void)mc1; cf |= c.isCancelRequested(); return UtilsQt::createReadyFuture(15); })
                 .then([mc2, &cf](const UtilsQt::AsyncResult<int>& r, const UtilsQt::CancelStatus& c){ (void)mc2; cf |= c.isCancelRequested(); r.tryRethrow(); return UtilsQt::createReadyFuture(QString::number(r.value())); })
                 .then(mc3)
                 .execute();

    ASSERT_FALSE(mc1.isMoved);
    ASSERT_FALSE(mc2.isMoved);
    ASSERT_FALSE(mc3.isMoved);
    ASSERT_TRUE(mc3.isCopied);

    auto mc3moved = std::move(mc3);
    ASSERT_TRUE(mc3.isMoved); // NOLINT

    UtilsQt::waitForFuture<QEventLoop>(f);
    ASSERT_TRUE(f.isFinished());
    ASSERT_FALSE(f.isCanceled());
    ASSERT_EQ(f.result(), QString("15"));

    ASSERT_EQ(mc1.stats->copies, 1);
    ASSERT_EQ(mc2.stats->copies, 1);

    ASSERT_FALSE(cf);
}

TEST(UtilsQt, Futures_Sequential_ExplicitMove)
{
    MovableCopyable mc1;
    MovableCopyable mc2;
    MovableCopyableFunctor mc3;

    QObject obj;
    auto f = UtilsQt::Sequential(&obj)
                 .start([mc1 = std::move(mc1)](){ (void)mc1; return UtilsQt::createReadyFuture(15); })
                 .then([mc2 = std::move(mc2)](const UtilsQt::AsyncResult<int>& r){ (void)mc2; r.tryRethrow(); return UtilsQt::createReadyFuture(QString::number(r.value())); })
                 .then(std::move(mc3))
                 .execute();

    ASSERT_TRUE(mc1.isMoved); // NOLINT
    ASSERT_TRUE(mc2.isMoved); // NOLINT
    ASSERT_TRUE(mc3.isMoved); // NOLINT
    ASSERT_FALSE(mc3.isCopied);

    UtilsQt::waitForFuture<QEventLoop>(f);
    ASSERT_TRUE(f.isFinished());
    ASSERT_FALSE(f.isCanceled());
    ASSERT_EQ(f.result(), QString("15"));

    ASSERT_EQ(mc1.stats->copies, 0);
    ASSERT_EQ(mc2.stats->copies, 0);
}

TEST(UtilsQt, Futures_Sequential_ThreadAffinity)
{
    const int mainThread = currentThreadId();
    int h1 = -1;
    int h2 = -1;
    int t1 = -1;
    int t2 = -1;

    ASSERT_NE(mainThread, h2);

    QObject obj;
    auto f = UtilsQt::Sequential(&obj)
                 .start([&h1, &t1]() {
                     h1 = currentThreadId();
                     return QtConcurrent::run([&t1]() {
                         t1 = currentThreadId();
                         QThread::currentThread()->msleep(30);
                         return 17;
                     });
                 })
                 .then([&h2, &t2](const UtilsQt::AsyncResult<int>& r) {
                     h2 = currentThreadId();
                     return QtConcurrent::run([r, &t2]() {
                         t2 = currentThreadId();
                         QThread::currentThread()->msleep(30);
                         return QString::number(r.value());
                     });
                 })
                 .execute();

    UtilsQt::waitForFuture<QEventLoop>(f);
    ASSERT_TRUE(f.isFinished());
    ASSERT_FALSE(f.isCanceled());
    ASSERT_EQ(f.result(), QString("17"));

    ASSERT_EQ(mainThread, h1);
    ASSERT_EQ(mainThread, h2);
    ASSERT_NE(mainThread, t1);
    ASSERT_NE(mainThread, t2);
}

TEST(UtilsQt, Futures_Sequential_ExternalCancellation)
{
    int calls {};
    int cancels {};

    QObject obj;
    auto f = UtilsQt::Sequential(&obj)
                 .start([&](const UtilsQt::CancelStatus& c){ calls++; if (c.isCancelRequested()) cancels++; return UtilsQt::createTimedFuture(50, 15); })
                 .then([&](const UtilsQt::AsyncResult<int>& r, const UtilsQt::CancelStatus& c){ calls++; if (c.isCancelRequested()) cancels++; r.tryRethrow(); return UtilsQt::createTimedFuture(50, QString::number(r.value())); })
                 .execute();

    f.cancel();

    UtilsQt::waitForFuture<QEventLoop>(f);
    ASSERT_TRUE(f.isFinished());
    ASSERT_TRUE(f.isCanceled());
    ASSERT_EQ(f.resultCount(), 0);
    ASSERT_EQ(calls, 1);
    ASSERT_EQ(cancels, 0);
}

TEST(UtilsQt, Futures_Sequential_ThreadedExternalCancellation)
{
    using namespace std::chrono_literals;
    using Clock = std::chrono::steady_clock;

    struct Status
    {
        bool h1Cancel {};
        bool h1Result {};
        bool h2PrevCancel {};
        bool h2PrevResult {};
        bool h2Cancel {};
        bool h2Result {};

        auto tie() const { return std::tie(h1Cancel, h1Result, h2PrevCancel, h2PrevResult, h2Cancel, h2Result); }
        bool operator==(const Status& other) const { return tie() == other.tie(); }
        bool operator!=(const Status& other) const { return tie() != other.tie(); }
    };

    Status status;

    QObject obj;
    auto factory = [&](){
        return UtilsQt::Sequential(&obj)
        .start([&status](const UtilsQt::CancelStatus& c) {
            UtilsQt::Promise<int> promise(true);

            QThreadPool::globalInstance()->start([c, promise, &status]() mutable {
                const auto now = Clock::now();

                while (!c.isCancelRequested() && (Clock::now() - now < 200ms * TimeFactor))
                    QThread::currentThread()->msleep(20);

                if (c.isCancelRequested()) {
                    status.h1Cancel = true;
                    promise.cancel();
                } else {
                    status.h1Result = true;
                    promise.finish(17);
                }
            });

            return promise.future();
        })
        .then([&](const UtilsQt::AsyncResult<int>& r, const UtilsQt::CancelStatus& c) {
            status.h2PrevCancel |= r.isCanceled();
            status.h2PrevResult |= r.hasValue();

            if (r.isCanceled())
                return UtilsQt::createCanceledFuture<QString>();

            UtilsQt::Promise<QString> promise(true);

            QThreadPool::globalInstance()->start([r, c, promise, &status]() mutable {
                const auto now = Clock::now();

                while (!c.isCancelRequested() && (Clock::now() - now < 200ms * TimeFactor))
                    QThread::currentThread()->msleep(20);

                if (c.isCancelRequested()) {
                    status.h2Cancel = true;
                    promise.cancel();
                } else {
                    status.h2Result = true;
                    promise.finish(QString::number(r.value()));
                }
            });

            return promise.future();
        })
        .execute();
    };

    {
        status = {};
        const auto start = Clock::now();
        auto f = factory();
        UtilsQt::waitForFuture<QEventLoop>(UtilsQt::createTimedFuture(50 * TimeFactor));
        f.cancel();
        UtilsQt::waitForFuture<QEventLoop>(f);
        const auto end = Clock::now();

        ASSERT_TRUE(f.isFinished());
        ASSERT_TRUE(f.isCanceled());
        ASSERT_EQ(status, (Status{true, false, false, false, false, false}));

        ASSERT_LE(end - start, 150ms * TimeFactor);
    }

    {
        status = {};
        const auto start = Clock::now();
        auto f = factory();
        UtilsQt::waitForFuture<QEventLoop>(UtilsQt::createTimedFuture(250 * TimeFactor));
        f.cancel();
        UtilsQt::waitForFuture<QEventLoop>(f);
        const auto end = Clock::now();

        ASSERT_TRUE(f.isFinished());
        ASSERT_TRUE(f.isCanceled());
        ASSERT_EQ(f.resultCount(), 0);
        ASSERT_EQ(status, (Status{false, true, false, true, true, false}));

        ASSERT_LE(end - start, 350ms * TimeFactor);
    }

    {
        status = {};
        const auto start = Clock::now();
        auto f = factory();
        UtilsQt::waitForFuture<QEventLoop>(UtilsQt::createTimedFuture(250 * TimeFactor));
        ASSERT_TRUE(f.isRunning());
        f.cancel();
        UtilsQt::waitForFuture<QEventLoop>(f);
        const auto end = Clock::now();

        ASSERT_TRUE(f.isFinished());
        ASSERT_TRUE(f.isCanceled());
        ASSERT_EQ(f.resultCount(), 0);
        ASSERT_EQ(status, (Status{false, true, false, true, true, false}));

        ASSERT_LE(end - start, 350ms * TimeFactor);
    }

    {
        status = {};
        const auto start = Clock::now();
        auto f = factory();
        UtilsQt::waitForFuture<QEventLoop>(UtilsQt::createTimedFuture(500 * TimeFactor));
        ASSERT_TRUE(f.isFinished());
        ASSERT_FALSE(f.isCanceled());
        f.cancel();
        UtilsQt::waitForFuture<QEventLoop>(f);
        const auto end = Clock::now();

        ASSERT_TRUE(f.isFinished());
        ASSERT_TRUE(f.isCanceled());
        ASSERT_EQ(f.resultCount(), 1);
        ASSERT_EQ(f.result(), "17");
        ASSERT_EQ(status, (Status{false, true, false, true, false, true}));

        ASSERT_GE(end - start, 400ms * TimeFactor);
    }
}

TEST(UtilsQt, Futures_Sequential_ThreadedExternalCancellationSubscription)
{
    using namespace std::chrono_literals;
    using Clock = std::chrono::steady_clock;

    // Test for subscription to the cancel status
    {
        QObject obj;
        std::chrono::milliseconds duration;
        auto f = UtilsQt::Sequential(&obj)
                     .start([&duration](const UtilsQt::CancelStatus& c) {
                         UtilsQt::Promise<int> promise(true);

                         QThreadPool::globalInstance()->start([&duration, c, promise]() mutable {
                             volatile bool run = true;
                             auto subscription = c.subscribe([&run]() mutable { run = false; });

                             const auto start = Clock::now();
                             while (run && (Clock::now() - start < 200ms * TimeFactor))
                                 QThread::currentThread()->msleep(20);

                             duration = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start);

                             if (run) {
                                 promise.finish(17);
                             } else {
                                 promise.cancel();
                             }
                         });

                         return promise.future();
                     })
                     .execute();

        UtilsQt::waitForFuture<QEventLoop>(UtilsQt::createTimedFuture(50 * TimeFactor));
        f.cancel();
        UtilsQt::waitForFuture<QEventLoop>(f);
        ASSERT_TRUE(f.isFinished());
        ASSERT_TRUE(f.isCanceled());
        ASSERT_EQ(f.resultCount(), 0);
        ASSERT_LE(duration, 150ms * TimeFactor);
    }

    // Test unsubscription on scope exit
    {
        QObject obj;
        std::chrono::milliseconds duration;
        auto f = UtilsQt::Sequential(&obj)
                     .start([&duration](const UtilsQt::CancelStatus& c) {
                         UtilsQt::Promise<int> promise(true);

                         QThreadPool::globalInstance()->start([&duration, c, promise]() mutable {
                             volatile bool run = true;
                             (void)c.subscribe([&run]() mutable { run = false; });

                             const auto start = Clock::now();
                             while (run && (Clock::now() - start < 200ms * TimeFactor))
                                 QThread::currentThread()->msleep(20);

                             duration = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start);

                             if (run) {
                                 promise.finish(17);
                             } else {
                                 promise.cancel();
                             }
                         });

                         return promise.future();
                     })
                     .execute();

        UtilsQt::waitForFuture<QEventLoop>(UtilsQt::createTimedFuture(50 * TimeFactor));
        f.cancel();
        UtilsQt::waitForFuture<QEventLoop>(f);
        ASSERT_TRUE(f.isFinished());
        ASSERT_TRUE(f.isCanceled());
        ASSERT_EQ(f.resultCount(), 0);
        ASSERT_GE(duration, 200ms * TimeFactor);
    }

    // Test idle subscription
    {
        QObject obj;
        std::chrono::milliseconds duration;
        auto f = UtilsQt::Sequential(&obj)
                     .start([&duration](const UtilsQt::CancelStatus& c) {
                         UtilsQt::Promise<int> promise(true);

                         QThreadPool::globalInstance()->start([&duration, c, promise]() mutable {
                             volatile bool run = true;
                             auto subscription = c.subscribe([&run]() mutable { run = false; });

                             const auto start = Clock::now();
                             while (run && (Clock::now() - start < 200ms * TimeFactor))
                                 QThread::currentThread()->msleep(20);

                             duration = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start);

                             if (run) {
                                 promise.finish(17);
                             } else {
                                 promise.cancel();
                             }
                         });

                         return promise.future();
                     })
                     .execute();

        UtilsQt::waitForFuture<QEventLoop>(f);
        ASSERT_TRUE(f.isFinished());
        ASSERT_FALSE(f.isCanceled());
        ASSERT_EQ(f.resultCount(), 1);
        ASSERT_EQ(f.result(), 17);
    }
}

TEST(UtilsQt, Futures_Sequential_LoosingContext)
{
    auto obj = std::make_unique<QObject>();
    int calls {};
    int cancels {};

    auto f = UtilsQt::Sequential(obj.get())
                 .start([&](const UtilsQt::CancelStatus& c){ calls++; if (c.isCancelRequested()) cancels++; return UtilsQt::createTimedFuture(50, 15); })
                 .then([&](const UtilsQt::AsyncResult<int>& r, const UtilsQt::CancelStatus& c){ calls++; if (c.isCancelRequested()) cancels++; r.tryRethrow(); return UtilsQt::createTimedFuture(50, QString::number(r.value())); })
                 .execute();

    obj.reset();

    UtilsQt::waitForFuture<QEventLoop>(f);
    ASSERT_TRUE(f.isFinished());
    ASSERT_TRUE(f.isCanceled());
    ASSERT_EQ(f.resultCount(), 0);
    ASSERT_EQ(calls, 1);
    ASSERT_EQ(cancels, 0);
}

TEST(UtilsQt, Futures_Sequential_MissingContext)
{
    auto f = UtilsQt::Sequential(nullptr)
                 .start([](){ return UtilsQt::createTimedFuture(10, 123); })
                 .then([](const UtilsQt::AsyncResult<int>& r){ r.tryRethrow(); return UtilsQt::createTimedFuture(20, QString::number(r.value())); })
                 .execute();

    UtilsQt::waitForFuture<QEventLoop>(f);
    ASSERT_TRUE(f.isFinished());
    ASSERT_FALSE(f.isCanceled());
    ASSERT_EQ(f.result(), QString("123"));
}

TEST(UtilsQt, Futures_Sequential_InternalCancellation)
{
    int calls {};
    int cancels {};
    int cancalledResults {};

    QObject obj;
    auto f = UtilsQt::Sequential(&obj)
                 .start([&](const UtilsQt::CancelStatus& c) {
                     calls++;

                     if (c.isCancelRequested())
                         cancels++;

                     return UtilsQt::createTimedCanceledFuture<int>(50);
                 })
                 .then([&](const UtilsQt::AsyncResult<int>& r, const UtilsQt::CancelStatus& c) {
                     calls++;

                     if (c.isCancelRequested())
                         cancels++;

                     if (r.isCanceled()) {
                         cancalledResults++;
                         return UtilsQt::createCanceledFuture<QString>();

                     } else {
                        r.tryRethrow();
                         return UtilsQt::createTimedFuture(50, QString::number(r.value()));
                     }
                 })
                 .execute();

    UtilsQt::waitForFuture<QEventLoop>(f);
    ASSERT_TRUE(f.isFinished());
    ASSERT_TRUE(f.isCanceled());
    ASSERT_EQ(f.resultCount(), 0);
    ASSERT_EQ(calls, 2);
    ASSERT_EQ(cancels, 0);
    ASSERT_EQ(cancalledResults, 1);
}

TEST(UtilsQt, Futures_Sequential_Exception)
{
    int calls {};
    int cancels {};
    int exceptionResults {};

    QObject obj;
    auto f = UtilsQt::Sequential(&obj)
                 .start([&](const UtilsQt::CancelStatus& c) {
                     calls++;

                     if (c.isCancelRequested())
                         cancels++;

                     return UtilsQt::createExceptionFuture<int>(std::make_exception_ptr(MyException()));
                 })
                 .then([&](const UtilsQt::AsyncResult<int>& r, const UtilsQt::CancelStatus& c) {
                     calls++;

                     if (c.isCancelRequested())
                         cancels++;

                     if (r.hasException())
                         exceptionResults++;

                     r.tryRethrow();
                     return UtilsQt::createTimedFuture(50, QString::number(r.value()));
                 })
                 .execute();

    UtilsQt::waitForFuture<QEventLoop>(f);
    ASSERT_EQ(UtilsQt::getFutureState(f), UtilsQt::FutureState::Exception);

    try {
        f.waitForFinished();
    } catch (...) {
        exceptionResults++;
    }

    ASSERT_FALSE(UtilsQt::hasResult(f));
    ASSERT_EQ(calls, 2);
    ASSERT_EQ(cancels, 0);
    ASSERT_EQ(exceptionResults, 2);

    ASSERT_THROW(f.result(), MyException);
}

TEST(UtilsQt, Futures_Sequential_Options_Cancellation)
{
    int calls {};
    int cancels {};
    int cancalledResults {};

    QObject obj;
    auto f = UtilsQt::Sequential(&obj, UtilsQt::SequenceOptions::AutoFinishOnCanceled)
                 .start([&](const UtilsQt::CancelStatus& c) {
                     calls++;

                     if (c.isCancelRequested())
                         cancels++;

                     return UtilsQt::createTimedCanceledFuture<int>(50);
                 })
                 .then([&](const UtilsQt::AsyncResult<int>& r, const UtilsQt::CancelStatus& c) {
                     calls++;

                     if (c.isCancelRequested())
                         cancels++;

                     if (r.isCanceled()) {
                         cancalledResults++;
                         return UtilsQt::createCanceledFuture<QString>();

                     } else {
                         r.tryRethrow();
                         return UtilsQt::createTimedFuture(50, QString::number(r.value()));
                     }
                 })
                 .execute();

    UtilsQt::waitForFuture<QEventLoop>(f);
    ASSERT_TRUE(f.isFinished());
    ASSERT_TRUE(f.isCanceled());
    ASSERT_EQ(f.resultCount(), 0);
    ASSERT_EQ(calls, 1);
    ASSERT_EQ(cancels, 0);
    ASSERT_EQ(cancalledResults, 0);
}

TEST(UtilsQt, Futures_Sequential_Options_Exception_1)
{
    int calls {};
    int cancels {};
    int exceptionResults {};

    QObject obj;
    auto f = UtilsQt::Sequential(&obj, UtilsQt::SequenceOptions::AutoFinishOnException)
                 .start([&](const UtilsQt::CancelStatus& c) {
                     calls++;

                     if (c.isCancelRequested())
                         cancels++;

                     return UtilsQt::createExceptionFuture<int>(std::make_exception_ptr(MyException()));
                 })
                 .then([&](const UtilsQt::AsyncResult<int>& r, const UtilsQt::CancelStatus& c) {
                     calls++;

                     if (c.isCancelRequested())
                         cancels++;

                     if (r.hasException())
                         exceptionResults++;

                     r.tryRethrow();
                     return UtilsQt::createTimedFuture(50, QString::number(r.value()));
                 })
                 .execute();

    UtilsQt::waitForFuture<QEventLoop>(f);
    ASSERT_EQ(UtilsQt::getFutureState(f), UtilsQt::FutureState::Exception);

    try {
        f.waitForFinished();
    } catch (...) {
        exceptionResults++;
    }

    ASSERT_FALSE(UtilsQt::hasResult(f));
    ASSERT_EQ(calls, 1);
    ASSERT_EQ(cancels, 0);
    ASSERT_EQ(exceptionResults, 1);

    ASSERT_THROW(f.result(), MyException);
}

TEST(UtilsQt, Futures_Sequential_Options_Exception_2)
{
    int calls {};
    int cancels {};
    int exceptionResults {};

    QObject obj;
    auto f = UtilsQt::Sequential(&obj, UtilsQt::SequenceOptions::AutoFinishOnException)
                 .start([&](const UtilsQt::CancelStatus& c) -> QFuture<int> {
                     calls++;

                     if (c.isCancelRequested())
                         cancels++;

                     throw MyException();
                     //return UtilsQt::createReadyFuture(15);
                 })
                 .then([&](const UtilsQt::AsyncResult<int>& r, const UtilsQt::CancelStatus& c) {
                     calls++;

                     if (c.isCancelRequested())
                         cancels++;

                     if (r.hasException())
                         exceptionResults++;

                     r.tryRethrow();
                     return UtilsQt::createTimedFuture(50, QString::number(r.value()));
                 })
                 .execute();

    UtilsQt::waitForFuture<QEventLoop>(f);
    ASSERT_EQ(UtilsQt::getFutureState(f), UtilsQt::FutureState::Exception);

    try {
        f.waitForFinished();
    } catch (...) {
        exceptionResults++;
    }

    ASSERT_FALSE(UtilsQt::hasResult(f));
    ASSERT_EQ(calls, 1);
    ASSERT_EQ(cancels, 0);
    ASSERT_EQ(exceptionResults, 1);

    ASSERT_THROW(f.result(), MyException);
}


TEST(UtilsQt, Futures_Sequential_Options_Exception_3)
{
    int calls {};
    int cancels {};
    int exceptionResults {};

    QObject obj;
    auto f = UtilsQt::Sequential(&obj, UtilsQt::SequenceOptions::AutoFinishOnException)
                 .start([&](const UtilsQt::CancelStatus& c) -> QFuture<int> {
                     calls++;

                     if (c.isCancelRequested())
                         cancels++;

                     return UtilsQt::createReadyFuture(15);
                 })
                 .then([&](const UtilsQt::AsyncResult<int>& r, const UtilsQt::CancelStatus& c) -> QFuture<QString> {
                     calls++;

                     if (c.isCancelRequested())
                         cancels++;

                     if (r.hasException())
                         exceptionResults++;

                     r.tryRethrow();

                     throw MyException();
                     //return UtilsQt::createTimedFuture(50, QString::number(r.value()));
                 })
                 .execute();

    UtilsQt::waitForFuture<QEventLoop>(f);
    ASSERT_EQ(UtilsQt::getFutureState(f), UtilsQt::FutureState::Exception);

    try {
        f.waitForFinished();
    } catch (...) {
        exceptionResults++;
    }

    ASSERT_FALSE(UtilsQt::hasResult(f));
    ASSERT_EQ(calls, 2);
    ASSERT_EQ(cancels, 0);
    ASSERT_EQ(exceptionResults, 1);

    ASSERT_THROW(f.result(), MyException);
}
