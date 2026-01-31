/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <gtest/gtest.h>

#include <chrono>

#include <UtilsQt/Futures/Broker.h>
#include <UtilsQt/Futures/Utils.h>
#include <QCoreApplication>
#include <QEventLoop>
#include <QString>

using namespace UtilsQt;
using namespace std::chrono_literals;

namespace {

// Duration for "running" futures - long enough to test state transitions
constexpr auto FutureDuration = 200ms;

class MyException : public std::exception
{
public:
    MyException() = default;
    MyException(const MyException&) = default;
    MyException(MyException&&) = default;
    MyException& operator=(const MyException&) = default;
    MyException& operator=(MyException&&) = default;
};

} // namespace

#if QT_VERSION_MAJOR >= 6
// Qt6 removed operator== and operator!= from QFuture, so we need to implement a function to check
// if two QFuture objects handle the same asynchronous operation.
// This function doesn't compare the results, only the pointers to the internal QFutureInterface.
template<typename T>
static bool sameOperation(const QFuture<T>& lhs, const QFuture<T>& rhs)
{
    struct QFutureLayout {
        void* vtable {};
        void* dptr {};
        bool operator==(const QFutureLayout& other) const { return dptr == other.dptr; }
    };

    const auto& lhsLayout = reinterpret_cast<const QFutureLayout&>(lhs);
    const auto& rhsLayout = reinterpret_cast<const QFutureLayout&>(rhs);
    return lhsLayout == rhsLayout;
}
#else
template<typename T>
static bool sameOperation(const QFuture<T>& lhs, const QFuture<T>& rhs)
{
    return lhs == rhs;
}
#endif // QT_VERSION_MAJOR >= 6

TEST(UtilsQt, Futures_Broker_InitialState)
{
    Broker<int> broker;
    ASSERT_FALSE(broker.hasRunningFuture());
}

TEST(UtilsQt, Futures_Broker_ConstructorWithFuture)
{
    // Test constructor with ready future
    {
        Broker<int> broker(createReadyFuture(42));
        ASSERT_FALSE(broker.hasRunningFuture()); // Ready future is already finished

        auto destFuture = broker.future();
        waitForFuture<QEventLoop>(destFuture);

        ASSERT_TRUE(destFuture.isFinished());
        ASSERT_FALSE(destFuture.isCanceled());
        ASSERT_EQ(destFuture.result(), 42);
    }

    // Test constructor with running future
    {
        Broker<int> broker(createTimedFuture(FutureDuration, 123));
        ASSERT_TRUE(broker.hasRunningFuture());

        auto destFuture = broker.future();
        ASSERT_FALSE(destFuture.isFinished());

        waitForFuture<QEventLoop>(destFuture);

        ASSERT_TRUE(destFuture.isFinished());
        ASSERT_FALSE(destFuture.isCanceled());
        ASSERT_EQ(destFuture.result(), 123);
        ASSERT_FALSE(broker.hasRunningFuture());
    }

    // Test constructor with void future
    {
        Broker<void> broker(createReadyFuture());
        ASSERT_FALSE(broker.hasRunningFuture());

        auto destFuture = broker.future();
        waitForFuture<QEventLoop>(destFuture);

        ASSERT_TRUE(destFuture.isFinished());
        ASSERT_FALSE(destFuture.isCanceled());
    }
}

TEST(UtilsQt, Futures_Broker_DefaultConstructorFutureAvailability)
{
    // Test that default constructor provides a future even without source
    Broker<int> broker;

    // Future should be available immediately
    auto destFuture = broker.future();

    // But it should not be started/finished yet since no source is bound
    ASSERT_FALSE(destFuture.isStarted());
    ASSERT_FALSE(destFuture.isFinished());
    ASSERT_FALSE(broker.hasRunningFuture());

    // Now bind a source and verify the same future gets the result
    broker.rebind(createReadyFuture(84));

    // Should still be the same future object
    auto sameFuture = broker.future();
    ASSERT_TRUE(sameOperation(destFuture, sameFuture));

    waitForFuture<QEventLoop>(destFuture);
    ASSERT_TRUE(destFuture.isFinished());
    ASSERT_FALSE(destFuture.isCanceled());
    ASSERT_EQ(destFuture.result(), 84);
}

TEST(UtilsQt, Futures_Broker_ConstructorWithExceptionFuture)
{
    Broker<int> broker(createExceptionFuture<int>(MyException()));

    auto destFuture = broker.future();
    waitForFuture<QEventLoop>(destFuture);

    ASSERT_TRUE(destFuture.isFinished());
    ASSERT_TRUE(destFuture.isCanceled()); // Exception futures are always canceled
    ASSERT_EQ(getFutureState(destFuture), FutureState::Exception);
    ASSERT_THROW(destFuture.result(), MyException);

    // Test that we can rebind after constructor exception
    broker.rebind(createReadyFuture(456));
    auto newDestFuture = broker.future();
    waitForFuture<QEventLoop>(newDestFuture);

    ASSERT_TRUE(newDestFuture.isFinished());
    ASSERT_FALSE(newDestFuture.isCanceled());
    ASSERT_EQ(newDestFuture.result(), 456);
}

TEST(UtilsQt, Futures_Broker_BasicBinding_Int)
{
    Broker<int> broker;

    ASSERT_FALSE(broker.hasRunningFuture());

    broker.rebind(createReadyFuture(42));

    ASSERT_FALSE(broker.hasRunningFuture()); // Ready future is already finished

    auto destFuture = broker.future();
    waitForFuture<QEventLoop>(destFuture);

    ASSERT_TRUE(destFuture.isFinished());
    ASSERT_FALSE(destFuture.isCanceled());
    ASSERT_EQ(destFuture.result(), 42);
}

TEST(UtilsQt, Futures_Broker_BasicBinding_Void)
{
    Broker<void> broker;

    ASSERT_FALSE(broker.hasRunningFuture());

    broker.rebind(createReadyFuture());

    ASSERT_FALSE(broker.hasRunningFuture());

    auto destFuture = broker.future();
    waitForFuture<QEventLoop>(destFuture);

    ASSERT_TRUE(destFuture.isFinished());
    ASSERT_FALSE(destFuture.isCanceled());
}

TEST(UtilsQt, Futures_Broker_BasicBinding_String)
{
    Broker<QString> broker;

    broker.rebind(createReadyFuture(QString("Hello World")));

    auto destFuture = broker.future();
    waitForFuture<QEventLoop>(destFuture);

    ASSERT_TRUE(destFuture.isFinished());
    ASSERT_FALSE(destFuture.isCanceled());
    ASSERT_EQ(destFuture.result(), QString("Hello World"));
}

TEST(UtilsQt, Futures_Broker_DelayedBinding)
{
    Broker<int> broker;

    broker.rebind(createTimedFuture(FutureDuration, 123));

    ASSERT_TRUE(broker.hasRunningFuture());

    auto destFuture = broker.future();
    ASSERT_FALSE(destFuture.isFinished());

    waitForFuture<QEventLoop>(destFuture);

    ASSERT_TRUE(destFuture.isFinished());
    ASSERT_FALSE(destFuture.isCanceled());
    ASSERT_EQ(destFuture.result(), 123);
    ASSERT_FALSE(broker.hasRunningFuture());
}

TEST(UtilsQt, Futures_Broker_ReplaceFinishedFuture)
{
    Broker<int> broker;

    // First bind and wait for completion
    broker.rebind(createReadyFuture(42));
    auto destFuture1 = broker.future();
    waitForFuture<QEventLoop>(destFuture1);

    ASSERT_TRUE(destFuture1.isFinished());
    ASSERT_EQ(destFuture1.result(), 42);

    // Replace with new future
    broker.rebind(createReadyFuture(84));

    // Should get new future
    auto destFuture2 = broker.future();
    ASSERT_FALSE(sameOperation(destFuture1, destFuture2)); // Different future objects

    waitForFuture<QEventLoop>(destFuture2);
    ASSERT_TRUE(destFuture2.isFinished());
    ASSERT_EQ(destFuture2.result(), 84);
}

TEST(UtilsQt, Futures_Broker_ReplaceRunningFuture)
{
    Broker<int> broker;

    // Bind a long-running future
    auto sourceFuture1 = createTimedFuture(FutureDuration, 42);
    broker.rebind(sourceFuture1);

    ASSERT_TRUE(broker.hasRunningFuture());

    // Replace before it finishes
    broker.rebind(createReadyFuture(84));

    // Process events to allow cancellation to propagate
    qApp->processEvents();
    qApp->processEvents();

    // Original source should be canceled
    ASSERT_TRUE(sourceFuture1.isCanceled());

    // Destination should get new result
    auto destFuture2 = broker.future();
    waitForFuture<QEventLoop>(destFuture2);
    ASSERT_TRUE(destFuture2.isFinished());
    ASSERT_FALSE(destFuture2.isCanceled());
    ASSERT_EQ(destFuture2.result(), 84);
}

TEST(UtilsQt, Futures_Broker_CancellationFromSource)
{
    Broker<int> broker;

    broker.rebind(createTimedCanceledFuture<int>(FutureDuration));

    auto destFuture = broker.future();
    waitForFuture<QEventLoop>(destFuture);

    ASSERT_TRUE(destFuture.isFinished());
    ASSERT_TRUE(destFuture.isCanceled());
    ASSERT_EQ(destFuture.resultCount(), 0);
}

TEST(UtilsQt, Futures_Broker_CancellationFromDestination)
{
    Broker<int> broker;

    auto sourceFuture = createTimedFuture(FutureDuration, 42);
    broker.rebind(sourceFuture);

    auto destFuture = broker.future();
    ASSERT_FALSE(destFuture.isCanceled());
    ASSERT_FALSE(sourceFuture.isCanceled());

    // Cancel destination
    destFuture.cancel();

    // Source should be canceled too
    qApp->processEvents();
    qApp->processEvents();
    ASSERT_TRUE(sourceFuture.isCanceled());

    waitForFuture<QEventLoop>(destFuture);
    ASSERT_TRUE(destFuture.isFinished());
    ASSERT_TRUE(destFuture.isCanceled());
}

TEST(UtilsQt, Futures_Broker_ExceptionHandling)
{
    Broker<int> broker;

    broker.rebind(createExceptionFuture<int>(MyException()));

    auto destFuture = broker.future();
    waitForFuture<QEventLoop>(destFuture);

    ASSERT_TRUE(destFuture.isFinished());
    ASSERT_TRUE(destFuture.isCanceled()); // Exception futures are always canceled
    ASSERT_EQ(getFutureState(destFuture), FutureState::Exception);
    ASSERT_THROW(destFuture.result(), MyException);

    // Test binding a new future after exception
    broker.rebind(createTimedFuture(FutureDuration, 123));
    auto newDestFuture = broker.future();
    waitForFuture<QEventLoop>(newDestFuture);

    ASSERT_TRUE(newDestFuture.isFinished());
    ASSERT_FALSE(newDestFuture.isCanceled());
    ASSERT_EQ(newDestFuture.result(), 123);
}

TEST(UtilsQt, Futures_Broker_TimedExceptionHandling)
{
    Broker<int> broker;

    broker.rebind(createTimedExceptionFuture<int>(FutureDuration, MyException()));

    auto destFuture = broker.future();
    waitForFuture<QEventLoop>(destFuture);

    ASSERT_TRUE(destFuture.isFinished());
    ASSERT_TRUE(destFuture.isCanceled()); // Exception futures are always canceled
    ASSERT_EQ(getFutureState(destFuture), FutureState::Exception);
    ASSERT_THROW(destFuture.result(), MyException);

    // Test binding a new future after timed exception
    broker.rebind(createTimedFuture(FutureDuration, 456));
    auto newDestFuture = broker.future();
    waitForFuture<QEventLoop>(newDestFuture);

    ASSERT_TRUE(newDestFuture.isFinished());
    ASSERT_FALSE(newDestFuture.isCanceled());
    ASSERT_EQ(newDestFuture.result(), 456);
}

TEST(UtilsQt, Futures_Broker_MultipleReplacements)
{
    Broker<QString> broker;

    // First future
    broker.rebind(createReadyFuture(QString("First")));
    auto future1 = broker.future();
    waitForFuture<QEventLoop>(future1);
    ASSERT_EQ(future1.result(), QString("First"));

    // Second future
    broker.rebind(createReadyFuture(QString("Second")));
    auto future2 = broker.future();
    waitForFuture<QEventLoop>(future2);
    ASSERT_EQ(future2.result(), QString("Second"));

    // Third future
    broker.rebind(createReadyFuture(QString("Third")));
    auto future3 = broker.future();
    waitForFuture<QEventLoop>(future3);
    ASSERT_EQ(future3.result(), QString("Third"));

    // All futures should be different objects
    ASSERT_FALSE(sameOperation(future1, future2));
    ASSERT_FALSE(sameOperation(future2, future3));
    ASSERT_FALSE(sameOperation(future1, future3));
}

TEST(UtilsQt, Futures_Broker_TimedMultipleReplacements)
{
    Broker<QString> broker;

    // First timed future
    broker.rebind(createTimedFuture(FutureDuration, QString("First")));
    auto future1 = broker.future();
    ASSERT_FALSE(future1.isFinished());

    // Second timed future (replaces first)
    broker.rebind(createTimedFuture(FutureDuration, QString("Second")));
    auto future2 = broker.future();

    // Third timed future (replaces second)
    broker.rebind(createTimedFuture(FutureDuration, QString("Third")));
    auto future3 = broker.future();

    // Wait for completion
    waitForFuture<QEventLoop>(future1);

    ASSERT_TRUE(future1.isFinished());
    ASSERT_TRUE(future2.isFinished());
    ASSERT_TRUE(future3.isFinished());

    // All futures should be the same (destination future is reused)
    ASSERT_TRUE(sameOperation(future1, future2));
    ASSERT_TRUE(sameOperation(future2, future3));
    ASSERT_TRUE(sameOperation(future1, future3));

    // Should have the result from the last rebind
    ASSERT_EQ(future1.result(), QString("Third"));
    ASSERT_EQ(future2.result(), QString("Third"));
    ASSERT_EQ(future3.result(), QString("Third"));
}


TEST(UtilsQt, Futures_Broker_StateTransitions)
{
    Broker<int> broker;

    // Initial state
    ASSERT_FALSE(broker.hasRunningFuture());

    // Bind running future
    broker.rebind(createTimedFuture(FutureDuration, 42));
    ASSERT_TRUE(broker.hasRunningFuture());

    // Wait for completion
    auto future1 = broker.future();
    waitForFuture<QEventLoop>(future1);
    ASSERT_EQ(future1.result(), 42);
    ASSERT_FALSE(broker.hasRunningFuture()); // Finished but still has future

    // Replace finished future
    broker.rebind(createTimedFuture(50ms, 84));
    ASSERT_TRUE(broker.hasRunningFuture()); // New running future

    auto future2 = broker.future();
    waitForFuture<QEventLoop>(future2);
    ASSERT_EQ(future2.result(), 84);
    ASSERT_FALSE(broker.hasRunningFuture());
}

TEST(UtilsQt, Futures_Broker_CancellationDuringReplacement)
{
    Broker<int> broker;

    // Start with a long-running future
    broker.rebind(createTimedFuture(FutureDuration, 42));
    auto destFuture1 = broker.future();

    // Cancel the destination
    destFuture1.cancel(); // Anyway 'createTimedFuture' will finish it with the result
    ASSERT_TRUE(destFuture1.isCanceled());

    // Now replace with a new future
    broker.rebind(createReadyFuture(84));

    // Should get a new future after replacement, as previous one is canceled
    auto destFuture2 = broker.future();
    ASSERT_FALSE(sameOperation(destFuture1, destFuture2));

    waitForFuture<QEventLoop>(destFuture2);
    ASSERT_TRUE(destFuture2.isFinished());
    ASSERT_FALSE(destFuture2.isCanceled());
    ASSERT_EQ(destFuture2.result(), 84);
}

TEST(UtilsQt, Futures_Broker_RepeatedBindingSameType)
{
    Broker<int> broker;

    // Bind multiple times with finished futures
    for (int i = 0; i < 5; ++i) {
        const auto value = i * 10;
        auto sourceFuture = createReadyFuture(value);

        broker.rebind(sourceFuture);

        auto destFuture = broker.future();
        waitForFuture<QEventLoop>(destFuture);

        ASSERT_TRUE(destFuture.isFinished());
        ASSERT_FALSE(destFuture.isCanceled());
        ASSERT_EQ(destFuture.result(), value);
    }
}

TEST(UtilsQt, Futures_Broker_ExceptionInReplacedFuture)
{
    Broker<int> broker;

    // Start with a normal future
    broker.rebind(createReadyFuture(42));
    auto future1 = broker.future();
    waitForFuture<QEventLoop>(future1);
    ASSERT_EQ(future1.result(), 42);

    // Replace with exception future
    broker.rebind(createExceptionFuture<int>(MyException()));
    auto future2 = broker.future();
    waitForFuture<QEventLoop>(future2);

    ASSERT_TRUE(future2.isFinished());
    ASSERT_TRUE(future2.isCanceled()); // Exception futures are always canceled
    ASSERT_EQ(getFutureState(future2), FutureState::Exception);
    ASSERT_THROW(future2.result(), MyException);
}

TEST(UtilsQt, Futures_Broker_ContextDestruction)
{
    auto broker = std::make_unique<Broker<int>>();

    broker->rebind(createTimedFuture(FutureDuration, 42));
    auto destFuture = broker->future();

    // Destroy context
    broker.reset();

    // Future should be canceled due to context destruction
    waitForFuture<QEventLoop>(destFuture);
    ASSERT_TRUE(destFuture.isFinished());
    ASSERT_TRUE(destFuture.isCanceled());
}

TEST(UtilsQt, Futures_Broker_VoidFutureOperations)
{
    Broker<void> broker;

    // Test with multiple void futures
    broker.rebind(createReadyFuture());
    auto future1 = broker.future();
    waitForFuture<QEventLoop>(future1);
    ASSERT_TRUE(future1.isFinished());
    ASSERT_FALSE(future1.isCanceled());

    // Replace with timed void future
    broker.rebind(createTimedFuture(FutureDuration));
    auto future2 = broker.future();
    waitForFuture<QEventLoop>(future2);
    ASSERT_TRUE(future2.isFinished());
    ASSERT_FALSE(future2.isCanceled());

    // Test replacement of unfinished future
    auto sourceFuture = createTimedFuture(FutureDuration);
    broker.rebind(sourceFuture);
    auto future3 = broker.future();

    // Verify futures are not finished yet
    ASSERT_FALSE(future3.isFinished());
    ASSERT_FALSE(sourceFuture.isFinished());

    // Replace before completion
    broker.rebind(createTimedFuture(50ms));
    auto future4 = broker.future();
    ASSERT_TRUE(sameOperation(future4, future3));
    qApp->processEvents();
    qApp->processEvents();
    ASSERT_TRUE(sourceFuture.isCanceled()); // Original source should be canceled

    // New future should complete normally
    waitForFuture<QEventLoop>(future4);
    ASSERT_TRUE(future4.isFinished());
    ASSERT_FALSE(future4.isCanceled());

    // Replace with canceled void future
    broker.rebind(createTimedCanceledFuture<void>(FutureDuration));
    auto future5 = broker.future();
    waitForFuture<QEventLoop>(future5);
    ASSERT_TRUE(future5.isFinished());
    ASSERT_TRUE(future5.isCanceled());

    // Test exception handling with void futures
    broker.rebind(createExceptionFuture<void>(MyException()));
    auto future6 = broker.future();
    waitForFuture<QEventLoop>(future6);
    ASSERT_TRUE(future6.isFinished());
    ASSERT_TRUE(future6.isCanceled()); // Exception futures are always canceled
    ASSERT_EQ(getFutureState(future6), FutureState::Exception);
    ASSERT_THROW(future6.waitForFinished(), MyException);

    // Test timed exception handling
    broker.rebind(createTimedExceptionFuture<void>(FutureDuration, MyException()));
    auto future7 = broker.future();
    waitForFuture<QEventLoop>(future7);
    ASSERT_TRUE(future7.isFinished());
    ASSERT_TRUE(future7.isCanceled());
    ASSERT_EQ(getFutureState(future7), FutureState::Exception);
    ASSERT_THROW(future7.waitForFinished(), MyException);

    // Replace exception future with normal future
    broker.rebind(createReadyFuture());
    auto future8 = broker.future();
    waitForFuture<QEventLoop>(future8);
    ASSERT_TRUE(future8.isFinished());
    ASSERT_FALSE(future8.isCanceled());
}

TEST(UtilsQt, Futures_Broker_Reset_AfterBinding)
{
    Broker<int> broker;

    // Bind a ready future
    broker.rebind(createReadyFuture(42));
    ASSERT_FALSE(broker.hasRunningFuture()); // Ready future is already finished

    auto destFuture1 = broker.future();
    waitForFuture<QEventLoop>(destFuture1);
    ASSERT_TRUE(destFuture1.isFinished());
    ASSERT_EQ(destFuture1.result(), 42);

    // Reset the broker
    broker.reset();
    ASSERT_FALSE(broker.hasRunningFuture());

    // Future should be available but not started/finished
    auto destFuture2 = broker.future();
    ASSERT_FALSE(destFuture2.isStarted());
    ASSERT_FALSE(destFuture2.isFinished());

    // Should be a different future object
    ASSERT_FALSE(sameOperation(destFuture1, destFuture2));
}

TEST(UtilsQt, Futures_Broker_Reset_DuringRunning)
{
    Broker<int> broker;

    // Bind a long-running future
    auto sourceFuture = createTimedFuture(FutureDuration, 123);
    broker.rebind(sourceFuture);
    ASSERT_TRUE(broker.hasRunningFuture());

    auto destFuture1 = broker.future();
    ASSERT_FALSE(destFuture1.isFinished());

    // Reset while future is running
    broker.reset();
    ASSERT_FALSE(broker.hasRunningFuture());

    // Original source should be canceled
    qApp->processEvents();
    qApp->processEvents();
    ASSERT_TRUE(sourceFuture.isCanceled());

    // Get future after reset - should be the same future object since the destination promise
    // was not finished or canceled before the reset, so it gets reused
    auto destFuture2 = broker.future();
    ASSERT_TRUE(sameOperation(destFuture1, destFuture2));
    ASSERT_FALSE(destFuture2.isStarted());
    ASSERT_FALSE(destFuture2.isFinished());
}

TEST(UtilsQt, Futures_Broker_Reset_VoidFuture)
{
    Broker<void> broker;

    // Bind a void future
    broker.rebind(createTimedFuture(FutureDuration));
    ASSERT_TRUE(broker.hasRunningFuture());

    auto destFuture1 = broker.future();
    ASSERT_FALSE(destFuture1.isFinished());

    // Reset
    broker.reset();
    ASSERT_FALSE(broker.hasRunningFuture());

    // Get future after reset - should be the same future object since the destination promise
    // was not finished or canceled before the reset, so it gets reused
    auto destFuture2 = broker.future();
    ASSERT_TRUE(sameOperation(destFuture1, destFuture2));
    ASSERT_FALSE(destFuture2.isStarted());
    ASSERT_FALSE(destFuture2.isFinished());
}

TEST(UtilsQt, Futures_Broker_Reset_MultipleResets)
{
    Broker<QString> broker;

    // First binding and reset
    broker.rebind(createReadyFuture(QString("First")));
    auto future1 = broker.future();
    waitForFuture<QEventLoop>(future1);
    ASSERT_EQ(future1.result(), QString("First"));

    broker.reset();
    auto future2 = broker.future();
    ASSERT_FALSE(future2.isStarted());
    ASSERT_FALSE(sameOperation(future1, future2));

    // Second binding and reset
    broker.rebind(createReadyFuture(QString("Second")));
    auto future3 = broker.future();
    ASSERT_TRUE(sameOperation(future2, future3)); // Should reuse the same future
    waitForFuture<QEventLoop>(future3);
    ASSERT_EQ(future3.result(), QString("Second"));

    broker.reset();
    auto future4 = broker.future();
    ASSERT_FALSE(future4.isStarted());
    ASSERT_FALSE(sameOperation(future3, future4));

    // Third binding after multiple resets
    broker.rebind(createReadyFuture(QString("Third")));
    auto future5 = broker.future();
    ASSERT_TRUE(sameOperation(future4, future5));
    waitForFuture<QEventLoop>(future5);
    ASSERT_EQ(future5.result(), QString("Third"));
}

TEST(UtilsQt, Futures_Broker_Reset_AfterException)
{
    Broker<int> broker;

    // Bind exception future
    broker.rebind(createExceptionFuture<int>(MyException()));
    auto destFuture1 = broker.future();
    waitForFuture<QEventLoop>(destFuture1);

    ASSERT_TRUE(destFuture1.isFinished());
    ASSERT_TRUE(destFuture1.isCanceled());
    ASSERT_EQ(getFutureState(destFuture1), FutureState::Exception);

    // Reset after exception
    broker.reset();
    ASSERT_FALSE(broker.hasRunningFuture());

    // Should get a new unstarted future
    auto destFuture2 = broker.future();
    ASSERT_FALSE(destFuture2.isStarted());
    ASSERT_FALSE(destFuture2.isFinished());
    ASSERT_FALSE(sameOperation(destFuture1, destFuture2));

    // Should be able to bind normally after reset
    broker.rebind(createReadyFuture(456));
    auto destFuture3 = broker.future();
    ASSERT_TRUE(sameOperation(destFuture2, destFuture3));
    waitForFuture<QEventLoop>(destFuture3);
    ASSERT_TRUE(destFuture3.isFinished());
    ASSERT_FALSE(destFuture3.isCanceled());
    ASSERT_EQ(destFuture3.result(), 456);
}

TEST(UtilsQt, Futures_Broker_Reset_AfterCancellation)
{
    Broker<int> broker;

    // Bind and cancel
    broker.rebind(createTimedFuture(FutureDuration, 42));
    auto destFuture1 = broker.future();
    destFuture1.cancel();

    waitForFuture<QEventLoop>(destFuture1);
    ASSERT_TRUE(destFuture1.isFinished());
    ASSERT_TRUE(destFuture1.isCanceled());

    // Reset after cancellation
    broker.reset();
    ASSERT_FALSE(broker.hasRunningFuture());

    // Should get a new unstarted future
    auto destFuture2 = broker.future();
    ASSERT_FALSE(destFuture2.isStarted());
    ASSERT_FALSE(destFuture2.isFinished());
    ASSERT_FALSE(sameOperation(destFuture1, destFuture2));

    // Should work normally after reset
    broker.rebind(createReadyFuture(789));
    auto destFuture3 = broker.future();
    ASSERT_TRUE(sameOperation(destFuture2, destFuture3));
    waitForFuture<QEventLoop>(destFuture3);
    ASSERT_TRUE(destFuture3.isFinished());
    ASSERT_FALSE(destFuture3.isCanceled());
    ASSERT_EQ(destFuture3.result(), 789);
}

TEST(UtilsQt, Futures_Broker_Reset_InitialState)
{
    Broker<int> broker;

    // Reset on initial state (should be no-op)
    auto destFuture1 = broker.future();
    ASSERT_FALSE(destFuture1.isStarted());
    ASSERT_FALSE(destFuture1.isFinished());

    broker.reset();
    ASSERT_FALSE(broker.hasRunningFuture());

    // Should get the same future after reset since the destination promise
    // was neither finished nor canceled before the reset
    auto destFuture2 = broker.future();
    ASSERT_TRUE(sameOperation(destFuture1, destFuture2));
    ASSERT_FALSE(destFuture2.isStarted());
    ASSERT_FALSE(destFuture2.isFinished());
}

TEST(UtilsQt, Futures_Broker_Reset_BindAfterReset)
{
    Broker<int> broker;

    // Reset without prior binding
    broker.reset();
    auto destFuture1 = broker.future();
    ASSERT_FALSE(destFuture1.isStarted());

    // Bind after reset
    broker.rebind(createReadyFuture(999));
    auto destFuture2 = broker.future();
    ASSERT_TRUE(sameOperation(destFuture1, destFuture2)); // Should reuse the same future

    waitForFuture<QEventLoop>(destFuture2);
    ASSERT_TRUE(destFuture2.isFinished());
    ASSERT_FALSE(destFuture2.isCanceled());
    ASSERT_EQ(destFuture2.result(), 999);

    // Reset again and verify state - since destFuture2 is finished,
    // a new future should be created
    broker.reset();
    auto destFuture3 = broker.future();
    ASSERT_FALSE(destFuture3.isStarted());
    ASSERT_FALSE(destFuture3.isFinished());
    ASSERT_FALSE(sameOperation(destFuture2, destFuture3));
}
