/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>

#include <QtConcurrent/QtConcurrent>
#include <QEventLoop>
#include <QString>
#include <QThread>

#include <UtilsQt/Futures/Sequential.h>
#include <UtilsQt/Futures/Utils.h>
#include <utils-cpp/threadid.h>
#include <utils-cpp/scoped_guard.h>
#include <utils-cpp/gtest_printers.h>

namespace {

#ifdef UTILS_QT_OS_MACOS
constexpr auto TimeFactor = 10;
#else
constexpr auto TimeFactor = 1;
#endif // UTILS_QT_OS_MACOS

// Helper for reliable synchronization: wait until atomic flag becomes true
// This is non-invasive observation - we just wait for the code to reach a state
template<typename Clock = std::chrono::steady_clock>
bool waitForFlag(const std::atomic<bool>& flag,
                 std::chrono::milliseconds timeout = std::chrono::milliseconds(5000))
{
    const auto deadline = Clock::now() + timeout;
    while (!flag.load(std::memory_order_acquire) && Clock::now() < deadline) {
        QThread::currentThread()->msleep(5);
    }
    return flag.load(std::memory_order_acquire);
}

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

    auto operator()(const UtilsQt::AsyncResult<QString>& r, const UtilsQt::SequentialMediator& sm) const {
        if (isCanceledPtr)
            *isCanceledPtr |= sm.isCancelRequested();

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
                 .start([m1 = std::move(m1), &cf](const UtilsQt::SequentialMediator& sm){ (void)m1; cf |= sm.isCancelRequested(); return UtilsQt::createReadyFuture(15); })
                 .then([m2 = std::move(m2), &cf](const UtilsQt::AsyncResult<int>& r, const UtilsQt::SequentialMediator& sm){ (void)m2; cf |= sm.isCancelRequested(); r.tryRethrow(); return UtilsQt::createReadyFuture(QString::number(r.value())); })
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
                 .start([m1, &cf](const UtilsQt::SequentialMediator& sm){ (void)m1; cf |= sm.isCancelRequested(); return UtilsQt::createReadyFuture(15); })
                 .then([m2, &cf](const UtilsQt::AsyncResult<int>& r, const UtilsQt::SequentialMediator& sm){ (void)m2; cf |= sm.isCancelRequested(); r.tryRethrow(); return UtilsQt::createReadyFuture(QString::number(r.value())); })
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
                 .start([mc1, &cf](const UtilsQt::SequentialMediator& sm){ (void)mc1; cf |= sm.isCancelRequested(); return UtilsQt::createReadyFuture(15); })
                 .then([mc2, &cf](const UtilsQt::AsyncResult<int>& r, const UtilsQt::SequentialMediator& sm){ (void)mc2; cf |= sm.isCancelRequested(); r.tryRethrow(); return UtilsQt::createReadyFuture(QString::number(r.value())); })
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
                 .start([&](const UtilsQt::SequentialMediator& sm){ calls++; if (sm.isCancelRequested()) cancels++; return UtilsQt::createTimedFuture(50, 15); })
                 .then([&](const UtilsQt::AsyncResult<int>& r, const UtilsQt::SequentialMediator& sm){ calls++; if (sm.isCancelRequested()) cancels++; r.tryRethrow(); return UtilsQt::createTimedFuture(50, QString::number(r.value())); })
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
        UtilsQt::Awaitables awaitables;
        auto f = UtilsQt::Sequential(&obj)
        .start([&status](UtilsQt::SequentialMediator& sm) {
            UtilsQt::Promise<int> promise(true);

            auto fThr = QtConcurrent::run([sm, promise, &status]() mutable {
                const auto now = Clock::now();

                while (!sm.isCancelRequested() && (Clock::now() - now < 200ms * TimeFactor))
                    QThread::currentThread()->msleep(20);

                if (sm.isCancelRequested()) {
                    status.h1Cancel = true;
                    promise.cancel();
                } else {
                    status.h1Result = true;
                    promise.finish(17);
                }
            });

            sm.registerAwaitable(fThr);

            return promise.future();
        })
        .then([&](const UtilsQt::AsyncResult<int>& r, UtilsQt::SequentialMediator& sm) {
            status.h2PrevCancel |= r.isCanceled();
            status.h2PrevResult |= r.hasValue();

            if (r.isCanceled())
                return UtilsQt::createCanceledFuture<QString>();

            UtilsQt::Promise<QString> promise(true);

            auto fThr = QtConcurrent::run([r, sm, promise, &status]() mutable {
                const auto now = Clock::now();

                while (!sm.isCancelRequested() && (Clock::now() - now < 200ms * TimeFactor))
                    QThread::currentThread()->msleep(20);

                if (sm.isCancelRequested()) {
                    status.h2Cancel = true;
                    promise.cancel();
                } else {
                    status.h2Result = true;
                    promise.finish(QString::number(r.value()));
                }
            });

            sm.registerAwaitable(fThr);

            return promise.future();
        })
        .execute(awaitables);

        return std::make_pair(f, std::move(awaitables));
    };

    {
        QFuture<QString> f;
        Clock::time_point start;

        {
            status = {};
            start = Clock::now();
            auto [localF, localAw] = factory();
            f = std::move(localF);
            UtilsQt::waitForFuture<QEventLoop>(UtilsQt::createTimedFuture(50 * TimeFactor));
            f.cancel();
            UtilsQt::waitForFuture<QEventLoop>(f);

            // localAw (Awaitables) is destroyed here
            localAw.confirmWait();
        }
        ASSERT_LE(Clock::now() - start, 120ms * TimeFactor); // 50ms delay + 20ms check interval + 50ms as usual time gap
        const auto end = Clock::now();

        ASSERT_TRUE(f.isFinished());
        ASSERT_TRUE(f.isCanceled());
        ASSERT_EQ(status, (Status{true, false, false, false, false, false}));

        ASSERT_LE(end - start, 150ms * TimeFactor);
    }

    {
        status = {};
        const auto start = Clock::now();
        auto [f, awaitables] = factory();
        UtilsQt::waitForFuture<QEventLoop>(UtilsQt::createTimedFuture(250 * TimeFactor));
        f.cancel();
        UtilsQt::waitForFuture<QEventLoop>(f);
        ASSERT_LE(Clock::now() - start, 300ms * TimeFactor);
        awaitables.wait();
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
        auto [f, awaitables] = factory();
        UtilsQt::waitForFuture<QEventLoop>(UtilsQt::createTimedFuture(250 * TimeFactor));
        ASSERT_TRUE(f.isRunning());
        f.cancel();
        UtilsQt::waitForFuture<QEventLoop>(f);
        ASSERT_LE(Clock::now() - start, 300ms * TimeFactor);
        awaitables.wait();
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
        auto [f, awaitables] = factory();
        UtilsQt::waitForFuture<QEventLoop>(UtilsQt::createTimedFuture(500 * TimeFactor));
        ASSERT_TRUE(f.isFinished());
        ASSERT_FALSE(f.isCanceled());
        f.cancel();
        UtilsQt::waitForFuture<QEventLoop>(f);
        ASSERT_LE(Clock::now() - start, 550ms * TimeFactor);
        awaitables.wait();
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
        UtilsQt::Awaitables awaitables;
        std::chrono::milliseconds duration;
        auto f = UtilsQt::Sequential(&obj)
                     .start([&duration](UtilsQt::SequentialMediator& sm) {
                         UtilsQt::Promise<int> promise(true);

                         auto fThr = QtConcurrent::run([&duration, sm, promise]() mutable {
                             volatile bool run = true;
                             auto subscription = sm.onCancellation([&run]() mutable { run = false; });

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

                         sm.registerAwaitable(fThr);

                         return promise.future();
                     })
                     .execute(awaitables);

        UtilsQt::waitForFuture<QEventLoop>(UtilsQt::createTimedFuture(50 * TimeFactor));
        f.cancel();
        UtilsQt::waitForFuture<QEventLoop>(f);
        ASSERT_TRUE(f.isFinished());
        ASSERT_TRUE(f.isCanceled());
        // ASSERT_TRUE(awaitables.isRunning()); -- Likely true, but not guaranteed, as thread could finish earlier.
        awaitables.wait();
        ASSERT_EQ(f.resultCount(), 0);
        ASSERT_LE(duration, 150ms * TimeFactor);
    }

    // Test (immediate) unsubscription on scope exit
    {
        QObject obj;
        UtilsQt::Awaitables awaitables;
        std::chrono::milliseconds duration;
        auto f = UtilsQt::Sequential(&obj)
                     .start([&duration](UtilsQt::SequentialMediator& sm) {
                         UtilsQt::Promise<int> promise(true);

                         auto fThr = QtConcurrent::run([&duration, sm, promise]() mutable {
                             volatile bool run = true;
                             (void)sm.onCancellation([&run]() mutable { run = false; });

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

                         sm.registerAwaitable(fThr);

                         return promise.future();
                     })
                     .execute(awaitables);

        UtilsQt::waitForFuture<QEventLoop>(UtilsQt::createTimedFuture(50 * TimeFactor));
        f.cancel();
        UtilsQt::waitForFuture<QEventLoop>(f);
        ASSERT_TRUE(f.isFinished());
        ASSERT_TRUE(f.isCanceled());
        //ASSERT_TRUE(awaitables.isRunning()); -- Likely true, but not guaranteed, as thread could finish earlier.
        awaitables.wait();
        ASSERT_EQ(f.resultCount(), 0);
        ASSERT_GE(duration, 200ms * TimeFactor);
    }

    // Test idle subscription
    {
        QObject obj;
        UtilsQt::Awaitables awaitables;
        std::chrono::milliseconds duration;
        auto f = UtilsQt::Sequential(&obj)
                     .start([&duration](UtilsQt::SequentialMediator& sm) {
                         UtilsQt::Promise<int> promise(true);

                         auto fThr = QtConcurrent::run([&duration, sm, promise]() mutable {
                             volatile bool run = true;
                             auto subscription = sm.onCancellation([&run]() mutable { run = false; });

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

                         sm.registerAwaitable(fThr);

                         return promise.future();
                     })
                     .execute(awaitables);

        UtilsQt::waitForFuture<QEventLoop>(f);
        ASSERT_TRUE(f.isFinished());
        ASSERT_FALSE(f.isCanceled());
        ASSERT_EQ(f.resultCount(), 1);
        ASSERT_EQ(f.result(), 17);
        //ASSERT_FALSE(awaitables.isRunning());  <---| Likely true, but not guaranteed without `wait`
        ASSERT_GE(duration, 200ms * TimeFactor); //  | as promise.finish() occurs earlier than thread finishes.
        awaitables.confirmWait();
    }
}

TEST(UtilsQt, Futures_Sequential_LoosingContext)
{
    auto obj = std::make_unique<QObject>();
    int calls {};
    int cancels {};

    auto f = UtilsQt::Sequential(obj.get())
                 .start([&](const UtilsQt::SequentialMediator& sm){ calls++; if (sm.isCancelRequested()) cancels++; return UtilsQt::createTimedFuture(50, 15); })
                 .then([&](const UtilsQt::AsyncResult<int>& r, const UtilsQt::SequentialMediator& sm){ calls++; if (sm.isCancelRequested()) cancels++; r.tryRethrow(); return UtilsQt::createTimedFuture(50, QString::number(r.value())); })
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
                 .start([&](const UtilsQt::SequentialMediator& sm) {
                     calls++;

                     if (sm.isCancelRequested())
                         cancels++;

                     return UtilsQt::createTimedCanceledFuture<int>(50);
                 })
                 .then([&](const UtilsQt::AsyncResult<int>& r, const UtilsQt::SequentialMediator& sm) {
                     calls++;

                     if (sm.isCancelRequested())
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
                 .start([&](const UtilsQt::SequentialMediator& sm) {
                     calls++;

                     if (sm.isCancelRequested())
                         cancels++;

                     return UtilsQt::createExceptionFuture<int>(std::make_exception_ptr(MyException()));
                 })
                 .then([&](const UtilsQt::AsyncResult<int>& r, const UtilsQt::SequentialMediator& sm) {
                     calls++;

                     if (sm.isCancelRequested())
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
    auto f = UtilsQt::Sequential(&obj, UtilsQt::SequentialOptions::AutoFinishOnCanceled)
                 .start([&](const UtilsQt::SequentialMediator& sm) {
                     calls++;

                     if (sm.isCancelRequested())
                         cancels++;

                     return UtilsQt::createTimedCanceledFuture<int>(50);
                 })
                 .then([&](const UtilsQt::AsyncResult<int>& r, const UtilsQt::SequentialMediator& sm) {
                     calls++;

                     if (sm.isCancelRequested())
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
    auto f = UtilsQt::Sequential(&obj, UtilsQt::SequentialOptions::AutoFinishOnException)
                 .start([&](const UtilsQt::SequentialMediator& sm) {
                     calls++;

                     if (sm.isCancelRequested())
                         cancels++;

                     return UtilsQt::createExceptionFuture<int>(std::make_exception_ptr(MyException()));
                 })
                 .then([&](const UtilsQt::AsyncResult<int>& r, const UtilsQt::SequentialMediator& sm) {
                     calls++;

                     if (sm.isCancelRequested())
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
    auto f = UtilsQt::Sequential(&obj, UtilsQt::SequentialOptions::AutoFinishOnException)
                 .start([&](const UtilsQt::SequentialMediator& sm) -> QFuture<int> {
                     calls++;

                     if (sm.isCancelRequested())
                         cancels++;

                     throw MyException();
                     //return UtilsQt::createReadyFuture(15);
                 })
                 .then([&](const UtilsQt::AsyncResult<int>& r, const UtilsQt::SequentialMediator& sm) {
                     calls++;

                     if (sm.isCancelRequested())
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
    auto f = UtilsQt::Sequential(&obj, UtilsQt::SequentialOptions::AutoFinishOnException)
                 .start([&](const UtilsQt::SequentialMediator& sm) -> QFuture<int> {
                     calls++;

                     if (sm.isCancelRequested())
                         cancels++;

                     return UtilsQt::createReadyFuture(15);
                 })
                 .then([&](const UtilsQt::AsyncResult<int>& r, const UtilsQt::SequentialMediator& sm) -> QFuture<QString> {
                     calls++;

                     if (sm.isCancelRequested())
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


TEST(UtilsQt, Futures_Sequential_Awaitable)
{
    using namespace std::chrono_literals;
    using Clock = std::chrono::steady_clock;

    // `wait`
    {
        QObject obj;
        UtilsQt::Awaitables awaitables;
        std::chrono::milliseconds duration;
        bool flag = false;
        std::atomic<bool> threadStarted{false};  // Reliable sync: signals when thread enters work loop
        auto f = UtilsQt::Sequential(&obj)
                     .start([&duration, &flag, &threadStarted](UtilsQt::SequentialMediator& sm) {
                         UtilsQt::Promise<int> promise(true);

                         auto fThr = QtConcurrent::run([&duration, &flag, &threadStarted, sm, promise]() mutable {
                             auto sg = CreateScopedGuard([&flag](){ flag = true; });
                             volatile bool run = true;
                             auto subscription = sm.onCancellation([&run]() mutable { run = false; });

                             threadStarted.store(true, std::memory_order_release);  // Signal: entered work loop

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

                         sm.registerAwaitable(fThr);

                         return promise.future();
                     })
                     .execute(awaitables);

        // Reliable sync: wait for thread to enter work loop (non-invasive observation)
        ASSERT_TRUE(waitForFlag(threadStarted));
        ASSERT_TRUE(awaitables.isRunning());  // Now guaranteed: thread is in the loop
        f.cancel();
        UtilsQt::waitForFuture<QEventLoop>(f);
        ASSERT_TRUE(f.isFinished());
        ASSERT_TRUE(f.isCanceled());
        //ASSERT_TRUE(awaitables.isRunning()); -- Not guaranteed after cancel, thread may finish quickly
        ASSERT_FALSE(flag);
        awaitables.wait();
        awaitables.wait(); // Just test that it doesn't crash
        awaitables.wait();
        ASSERT_FALSE(awaitables.isRunning());
        ASSERT_TRUE(flag);
        ASSERT_EQ(f.resultCount(), 0);
        ASSERT_LE(duration, 150ms * TimeFactor);
    }

    // `confirmWait` - tests that destructor waits for thread completion
    {
        bool flag = false;
        std::chrono::milliseconds duration;
        std::atomic<bool> threadStarted{false};  // Reliable sync: signals when thread enters work loop
        QFuture<int> f;

        {
            QObject obj;
            UtilsQt::Awaitables awaitables;

            f = UtilsQt::Sequential(&obj)
                         .start([&duration, &flag, &threadStarted](UtilsQt::SequentialMediator& sm) {
                             UtilsQt::Promise<int> promise(true);

                             auto fThr = QtConcurrent::run([&duration, &flag, &threadStarted, sm, promise]() mutable {
                                 auto sg = CreateScopedGuard([&flag](){ flag = true; });
                                 volatile bool run = true;
                                 auto subscription = sm.onCancellation([&run]() mutable { run = false; });

                                 threadStarted.store(true, std::memory_order_release);  // Signal: entered work loop

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

                             sm.registerAwaitable(fThr);

                             return promise.future();
                         })
                         .execute(awaitables);

            // Reliable sync: wait for thread to enter work loop (non-invasive observation)
            ASSERT_TRUE(waitForFlag(threadStarted));
            ASSERT_TRUE(awaitables.isRunning());  // Now guaranteed: thread is in the loop
            f.cancel();
            UtilsQt::waitForFuture<QEventLoop>(f);
            ASSERT_TRUE(f.isFinished());
            ASSERT_TRUE(f.isCanceled());
            // After cancel, thread may finish quickly, so isRunning() is not guaranteed
            ASSERT_FALSE(flag);
            awaitables.confirmWait();
        }

        ASSERT_TRUE(flag);
        ASSERT_EQ(f.resultCount(), 0);
        ASSERT_LE(duration, 150ms * TimeFactor);
    }
}
