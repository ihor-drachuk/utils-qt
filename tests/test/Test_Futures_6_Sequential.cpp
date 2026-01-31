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

#include "internal/TestWaitHelpers.h"

using TestHelpers::waitForFlag;
using TestHelpers::waitForCounter;

namespace {

// Default work loop duration when thread needs to simulate ongoing work
constexpr auto WorkLoopDuration = std::chrono::milliseconds(200);

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
        std::atomic<bool> h1Started{false};
        std::atomic<bool> h2Started{false};
        bool h1Cancel {};
        bool h1Result {};
        bool h2PrevCancel {};
        bool h2PrevResult {};
        bool h2Cancel {};
        bool h2Result {};

        void reset() {
            h1Started = false;
            h2Started = false;
            h1Cancel = false;
            h1Result = false;
            h2PrevCancel = false;
            h2PrevResult = false;
            h2Cancel = false;
            h2Result = false;
        }

        auto resultTie() const { return std::tie(h1Cancel, h1Result, h2PrevCancel, h2PrevResult, h2Cancel, h2Result); }
        bool resultsEqual(bool _h1Cancel, bool _h1Result, bool _h2PrevCancel, bool _h2PrevResult, bool _h2Cancel, bool _h2Result) const {
            return resultTie() == std::tie(_h1Cancel, _h1Result, _h2PrevCancel, _h2PrevResult, _h2Cancel, _h2Result);
        }
    };

    Status status;

    QObject obj;
    auto factory = [&](){
        UtilsQt::Awaitables awaitables;
        auto f = UtilsQt::Sequential(&obj)
        .start([&status](UtilsQt::SequentialMediator& sm) {
            UtilsQt::Promise<int> promise(true);

            auto fThr = QtConcurrent::run([sm, promise, &status]() mutable {
                status.h1Started.store(true, std::memory_order_release);
                const auto now = Clock::now();

                while (!sm.isCancelRequested() && (Clock::now() - now < WorkLoopDuration))
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
                status.h2Started.store(true, std::memory_order_release);
                const auto now = Clock::now();

                while (!sm.isCancelRequested() && (Clock::now() - now < WorkLoopDuration))
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

    // Test 1: Cancel during first handler
    {
        QFuture<QString> f;

        {
            status.reset();
            auto [localF, localAw] = factory();
            f = std::move(localF);

            // Wait for h1 to start, then cancel
            ASSERT_TRUE(waitForFlag(status.h1Started));
            f.cancel();
            UtilsQt::waitForFuture<QEventLoop>(f);

            localAw.confirmWait();
        }

        ASSERT_TRUE(f.isFinished());
        ASSERT_TRUE(f.isCanceled());
        ASSERT_TRUE(status.resultsEqual(true, false, false, false, false, false));
    }

    // Test 2: Cancel during second handler
    {
        status.reset();
        auto [f, awaitables] = factory();

        // Wait for h2 to start (h1 must have finished with result)
        ASSERT_TRUE(waitForFlag(status.h2Started));
        ASSERT_TRUE(f.isRunning());
        f.cancel();
        UtilsQt::waitForFuture<QEventLoop>(f);
        awaitables.wait();

        ASSERT_TRUE(f.isFinished());
        ASSERT_TRUE(f.isCanceled());
        ASSERT_EQ(f.resultCount(), 0);
        ASSERT_TRUE(status.resultsEqual(false, true, false, true, true, false));
    }

    // Test 3: Let both handlers complete without cancellation
    {
        status.reset();
        auto [f, awaitables] = factory();

        // Just wait for completion
        UtilsQt::waitForFuture<QEventLoop>(f);
        ASSERT_TRUE(f.isFinished());
        ASSERT_FALSE(f.isCanceled());

        // Cancel after completion (should have no effect on result)
        f.cancel();
        awaitables.wait();

        ASSERT_TRUE(f.isFinished());
        ASSERT_TRUE(f.isCanceled()); // cancel() sets canceled flag even after completion
        ASSERT_EQ(f.resultCount(), 1);
        ASSERT_EQ(f.result(), "17");
        ASSERT_TRUE(status.resultsEqual(false, true, false, true, false, true));
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
        std::atomic<bool> threadStarted{false};
        std::atomic<bool> wasCancelled{false};
        auto f = UtilsQt::Sequential(&obj)
                     .start([&threadStarted, &wasCancelled](UtilsQt::SequentialMediator& sm) {
                         UtilsQt::Promise<int> promise(true);

                         auto fThr = QtConcurrent::run([&threadStarted, &wasCancelled, sm, promise]() mutable {
                             volatile bool run = true;
                             auto subscription = sm.onCancellation([&run, &wasCancelled]() mutable {
                                 run = false;
                                 wasCancelled.store(true, std::memory_order_release);
                             });

                             threadStarted.store(true, std::memory_order_release);

                             const auto start = Clock::now();
                             while (run && (Clock::now() - start < WorkLoopDuration))
                                 QThread::currentThread()->msleep(20);

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

        // Wait for thread to start, then cancel
        ASSERT_TRUE(waitForFlag(threadStarted));
        f.cancel();
        UtilsQt::waitForFuture<QEventLoop>(f);
        ASSERT_TRUE(f.isFinished());
        ASSERT_TRUE(f.isCanceled());
        awaitables.wait();
        ASSERT_EQ(f.resultCount(), 0);
        // Verify cancellation callback was triggered
        ASSERT_TRUE(wasCancelled.load());
    }

    // Test (immediate) unsubscription on scope exit - callback should NOT be called
    {
        QObject obj;
        UtilsQt::Awaitables awaitables;
        std::atomic<bool> threadStarted{false};
        std::atomic<bool> callbackCalled{false};
        auto f = UtilsQt::Sequential(&obj)
                     .start([&threadStarted, &callbackCalled](UtilsQt::SequentialMediator& sm) {
                         UtilsQt::Promise<int> promise(true);

                         auto fThr = QtConcurrent::run([&threadStarted, &callbackCalled, sm, promise]() mutable {
                             volatile bool run = true;
                             // Subscription immediately goes out of scope - callback should NOT be called
                             (void)sm.onCancellation([&run, &callbackCalled]() mutable {
                                 run = false;
                                 callbackCalled.store(true, std::memory_order_release);
                             });

                             threadStarted.store(true, std::memory_order_release);

                             const auto start = Clock::now();
                             while (run && (Clock::now() - start < WorkLoopDuration))
                                 QThread::currentThread()->msleep(20);

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

        // Wait for thread to start, then cancel
        ASSERT_TRUE(waitForFlag(threadStarted));
        f.cancel();
        UtilsQt::waitForFuture<QEventLoop>(f);
        ASSERT_TRUE(f.isFinished());
        ASSERT_TRUE(f.isCanceled());
        awaitables.wait();
        ASSERT_EQ(f.resultCount(), 0);
        // Callback should NOT have been called because subscription was dropped
        ASSERT_FALSE(callbackCalled.load());
    }

    // Test idle subscription - no cancellation, thread completes normally
    {
        QObject obj;
        UtilsQt::Awaitables awaitables;
        std::atomic<bool> threadFinished{false};
        auto f = UtilsQt::Sequential(&obj)
                     .start([&threadFinished](UtilsQt::SequentialMediator& sm) {
                         UtilsQt::Promise<int> promise(true);

                         auto fThr = QtConcurrent::run([&threadFinished, sm, promise]() mutable {
                             volatile bool run = true;
                             auto subscription = sm.onCancellation([&run]() mutable { run = false; });

                             const auto start = Clock::now();
                             while (run && (Clock::now() - start < WorkLoopDuration))
                                 QThread::currentThread()->msleep(20);

                             if (run) {
                                 promise.finish(17);
                             } else {
                                 promise.cancel();
                             }
                             threadFinished.store(true, std::memory_order_release);
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
        awaitables.confirmWait();
        // Thread should have finished normally
        ASSERT_TRUE(threadFinished.load());
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

    // Test `wait` - explicit waiting for awaitables
    {
        QObject obj;
        UtilsQt::Awaitables awaitables;
        bool flag = false;
        std::atomic<bool> threadStarted{false};
        auto f = UtilsQt::Sequential(&obj)
                     .start([&flag, &threadStarted](UtilsQt::SequentialMediator& sm) {
                         UtilsQt::Promise<int> promise(true);

                         auto fThr = QtConcurrent::run([&flag, &threadStarted, sm, promise]() mutable {
                             auto sg = CreateScopedGuard([&flag](){ flag = true; });
                             volatile bool run = true;
                             auto subscription = sm.onCancellation([&run]() mutable { run = false; });

                             threadStarted.store(true, std::memory_order_release);

                             const auto start = Clock::now();
                             while (run && (Clock::now() - start < WorkLoopDuration))
                                 QThread::currentThread()->msleep(20);

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

        // Wait for thread to enter work loop
        ASSERT_TRUE(waitForFlag(threadStarted));
        ASSERT_TRUE(awaitables.isRunning());
        f.cancel();
        UtilsQt::waitForFuture<QEventLoop>(f);
        ASSERT_TRUE(f.isFinished());
        ASSERT_TRUE(f.isCanceled());
        ASSERT_FALSE(flag);  // Thread hasn't finished cleanup yet
        awaitables.wait();
        awaitables.wait(); // Just test that it doesn't crash
        awaitables.wait();
        ASSERT_FALSE(awaitables.isRunning());
        ASSERT_TRUE(flag);  // Now thread finished
        ASSERT_EQ(f.resultCount(), 0);
    }

    // Test `confirmWait` - destructor waits for thread completion
    {
        bool flag = false;
        std::atomic<bool> threadStarted{false};
        QFuture<int> f;

        {
            QObject obj;
            UtilsQt::Awaitables awaitables;

            f = UtilsQt::Sequential(&obj)
                         .start([&flag, &threadStarted](UtilsQt::SequentialMediator& sm) {
                             UtilsQt::Promise<int> promise(true);

                             auto fThr = QtConcurrent::run([&flag, &threadStarted, sm, promise]() mutable {
                                 auto sg = CreateScopedGuard([&flag](){ flag = true; });
                                 volatile bool run = true;
                                 auto subscription = sm.onCancellation([&run]() mutable { run = false; });

                                 threadStarted.store(true, std::memory_order_release);

                                 const auto start = Clock::now();
                                 while (run && (Clock::now() - start < WorkLoopDuration))
                                     QThread::currentThread()->msleep(20);

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

            // Wait for thread to enter work loop
            ASSERT_TRUE(waitForFlag(threadStarted));
            ASSERT_TRUE(awaitables.isRunning());
            f.cancel();
            UtilsQt::waitForFuture<QEventLoop>(f);
            ASSERT_TRUE(f.isFinished());
            ASSERT_TRUE(f.isCanceled());
            ASSERT_FALSE(flag);  // Thread hasn't finished cleanup yet
            awaitables.confirmWait();
        }

        ASSERT_TRUE(flag);  // Destructor waited for thread
        ASSERT_EQ(f.resultCount(), 0);
    }
}
