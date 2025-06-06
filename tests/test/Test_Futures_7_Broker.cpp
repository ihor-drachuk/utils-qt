/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <gtest/gtest.h>

#include <UtilsQt/Futures/Broker.h>
#include <UtilsQt/Futures/Utils.h>
#include <QCoreApplication>
#include <QEventLoop>
#include <QString>

using namespace UtilsQt;

namespace {

#ifdef UTILS_QT_OS_MACOS
constexpr unsigned UsualDuration = 500;
constexpr unsigned MicroDuration = 50;
#else
constexpr unsigned UsualDuration = 50;
constexpr unsigned MicroDuration = 10;
#endif // UTILS_QT_OS_MACOS

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
    ASSERT_FALSE(broker.hasFuture());
    ASSERT_FALSE(broker.hasRunningFuture());
}

TEST(UtilsQt, Futures_Broker_BasicBinding_Int)
{
    Broker<int> broker;

    ASSERT_FALSE(broker.hasFuture());
    ASSERT_FALSE(broker.hasRunningFuture());

    broker.bind(createReadyFuture(42));

    ASSERT_TRUE(broker.hasFuture());
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

    ASSERT_FALSE(broker.hasFuture());
    ASSERT_FALSE(broker.hasRunningFuture());

    broker.bind(createReadyFuture());

    ASSERT_TRUE(broker.hasFuture());
    ASSERT_FALSE(broker.hasRunningFuture());

    auto destFuture = broker.future();
    waitForFuture<QEventLoop>(destFuture);

    ASSERT_TRUE(destFuture.isFinished());
    ASSERT_FALSE(destFuture.isCanceled());
}

TEST(UtilsQt, Futures_Broker_BasicBinding_String)
{
    Broker<QString> broker;

    broker.bind(createReadyFuture(QString("Hello World")));

    auto destFuture = broker.future();
    waitForFuture<QEventLoop>(destFuture);

    ASSERT_TRUE(destFuture.isFinished());
    ASSERT_FALSE(destFuture.isCanceled());
    ASSERT_EQ(destFuture.result(), QString("Hello World"));
}

TEST(UtilsQt, Futures_Broker_DelayedBinding)
{
    Broker<int> broker;

    broker.bind(createTimedFuture(UsualDuration, 123));

    ASSERT_TRUE(broker.hasFuture());
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
    broker.bind(createReadyFuture(42));
    auto destFuture1 = broker.future();
    waitForFuture<QEventLoop>(destFuture1);

    ASSERT_TRUE(destFuture1.isFinished());
    ASSERT_EQ(destFuture1.result(), 42);

    // Replace with new future
    broker.bindOrReplace(createReadyFuture(84));

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
    auto sourceFuture1 = createTimedFuture(UsualDuration, 42);
    broker.bind(sourceFuture1);

    ASSERT_TRUE(broker.hasRunningFuture());

    // Replace before it finishes
    broker.bindOrReplace(createReadyFuture(84));

    // Original source should be canceled
    waitForFuture<QEventLoop>(createTimedFuture(MicroDuration));
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

    broker.bind(createTimedCanceledFuture<int>(UsualDuration));

    auto destFuture = broker.future();
    waitForFuture<QEventLoop>(destFuture);

    ASSERT_TRUE(destFuture.isFinished());
    ASSERT_TRUE(destFuture.isCanceled());
    ASSERT_EQ(destFuture.resultCount(), 0);
}

TEST(UtilsQt, Futures_Broker_CancellationFromDestination)
{
    Broker<int> broker;

    auto sourceFuture = createTimedFuture(UsualDuration, 42);
    broker.bind(sourceFuture);

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

    broker.bind(createExceptionFuture<int>(MyException()));

    auto destFuture = broker.future();
    waitForFuture<QEventLoop>(destFuture);

    ASSERT_TRUE(destFuture.isFinished());
    ASSERT_TRUE(destFuture.isCanceled()); // Exception futures are always canceled
    ASSERT_EQ(getFutureState(destFuture), FutureState::Exception);
    ASSERT_THROW(destFuture.result(), MyException);

    // Test binding a new future after exception
    broker.bind(createTimedFuture(UsualDuration, 123));
    auto newDestFuture = broker.future();
    waitForFuture<QEventLoop>(newDestFuture);

    ASSERT_TRUE(newDestFuture.isFinished());
    ASSERT_FALSE(newDestFuture.isCanceled());
    ASSERT_EQ(newDestFuture.result(), 123);
}

TEST(UtilsQt, Futures_Broker_TimedExceptionHandling)
{
    Broker<int> broker;

    broker.bind(createTimedExceptionFuture<int>(UsualDuration, MyException()));

    auto destFuture = broker.future();
    waitForFuture<QEventLoop>(destFuture);

    ASSERT_TRUE(destFuture.isFinished());
    ASSERT_TRUE(destFuture.isCanceled()); // Exception futures are always canceled
    ASSERT_EQ(getFutureState(destFuture), FutureState::Exception);
    ASSERT_THROW(destFuture.result(), MyException);

    // Test binding a new future after timed exception
    broker.bind(createTimedFuture(UsualDuration, 456));
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
    broker.bind(createReadyFuture(QString("First")));
    auto future1 = broker.future();
    waitForFuture<QEventLoop>(future1);
    ASSERT_EQ(future1.result(), QString("First"));

    // Second future
    broker.bind(createReadyFuture(QString("Second")));
    auto future2 = broker.future();
    waitForFuture<QEventLoop>(future2);
    ASSERT_EQ(future2.result(), QString("Second"));

    // Third future
    broker.bind(createReadyFuture(QString("Third")));
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
    broker.bind(createTimedFuture(UsualDuration, QString("First")));
    auto future1 = broker.future();
    waitForFuture<QEventLoop>(createTimedFuture(MicroDuration));
    ASSERT_FALSE(future1.isFinished());

    // Second timed future
    broker.bindOrReplace(createTimedFuture(UsualDuration, QString("Second")));
    auto future2 = broker.future();
    waitForFuture<QEventLoop>(createTimedFuture(MicroDuration));
    ASSERT_FALSE(future2.isFinished());

    // Third timed future
    broker.bindOrReplace(createTimedFuture(UsualDuration, QString("Third")));
    auto future3 = broker.future();
    waitForFuture<QEventLoop>(createTimedFuture(MicroDuration));
    ASSERT_FALSE(future3.isFinished());

    waitForFuture<QEventLoop>(future1);

    ASSERT_TRUE(future1.isFinished());
    ASSERT_TRUE(future2.isFinished());
    ASSERT_TRUE(future3.isFinished());

    // All futures should be the same
    ASSERT_TRUE(sameOperation(future1, future2));
    ASSERT_TRUE(sameOperation(future2, future3));
    ASSERT_TRUE(sameOperation(future1, future3));

    ASSERT_EQ(future1.result(), QString("Third"));
    ASSERT_EQ(future2.result(), QString("Third"));
    ASSERT_EQ(future3.result(), QString("Third"));
}


TEST(UtilsQt, Futures_Broker_StateTransitions)
{
    Broker<int> broker;

    // Initial state
    ASSERT_FALSE(broker.hasFuture());
    ASSERT_FALSE(broker.hasRunningFuture());

    // Bind running future
    broker.bind(createTimedFuture(UsualDuration, 42));
    ASSERT_TRUE(broker.hasFuture());
    ASSERT_TRUE(broker.hasRunningFuture());

    // Wait for completion
    auto future1 = broker.future();
    waitForFuture<QEventLoop>(future1);
    ASSERT_EQ(future1.result(), 42);
    ASSERT_TRUE(broker.hasFuture());
    ASSERT_FALSE(broker.hasRunningFuture()); // Finished but still has future

    // Replace finished future
    broker.bind(createTimedFuture(MicroDuration, 84));
    ASSERT_TRUE(broker.hasFuture());
    ASSERT_TRUE(broker.hasRunningFuture()); // New running future

    auto future2 = broker.future();
    waitForFuture<QEventLoop>(future2);
    ASSERT_EQ(future2.result(), 84);
    ASSERT_TRUE(broker.hasFuture());
    ASSERT_FALSE(broker.hasRunningFuture());
}

TEST(UtilsQt, Futures_Broker_CancellationDuringReplacement)
{
    Broker<int> broker;

    // Start with a long-running future
    broker.bind(createTimedFuture(UsualDuration, 42));
    auto destFuture1 = broker.future();

    // Cancel the destination
    destFuture1.cancel(); // Anyway 'createTimedFuture' will finish it with the result
    ASSERT_TRUE(destFuture1.isCanceled());

    // Now replace with a new future
    broker.bind(createReadyFuture(84));

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

        if (i == 0) {
            broker.bind(sourceFuture);
        } else {
            broker.bindOrReplace(sourceFuture);
        }

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
    broker.bind(createReadyFuture(42));
    auto future1 = broker.future();
    waitForFuture<QEventLoop>(future1);
    ASSERT_EQ(future1.result(), 42);

    // Replace with exception future
    broker.bindOrReplace(createExceptionFuture<int>(MyException()));
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

    broker->bind(createTimedFuture(UsualDuration, 42));
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
    broker.bind(createReadyFuture());
    auto future1 = broker.future();
    waitForFuture<QEventLoop>(future1);
    ASSERT_TRUE(future1.isFinished());
    ASSERT_FALSE(future1.isCanceled());

    // Replace with timed void future
    broker.bind(createTimedFuture(UsualDuration));
    auto future2 = broker.future();
    waitForFuture<QEventLoop>(future2);
    ASSERT_TRUE(future2.isFinished());
    ASSERT_FALSE(future2.isCanceled());

    // Test replacement of unfinished future
    auto sourceFuture = createTimedFuture(UsualDuration);
    broker.bind(sourceFuture);
    auto future3 = broker.future();

    // Verify futures are not finished yet
    ASSERT_FALSE(future3.isFinished());
    ASSERT_FALSE(sourceFuture.isFinished());

    // Replace before completion
    broker.bindOrReplace(createTimedFuture(MicroDuration));
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
    broker.bindOrReplace(createTimedCanceledFuture<void>(UsualDuration));
    auto future5 = broker.future();
    waitForFuture<QEventLoop>(future5);
    ASSERT_TRUE(future5.isFinished());
    ASSERT_TRUE(future5.isCanceled());

    // Test exception handling with void futures
    broker.bindOrReplace(createExceptionFuture<void>(MyException()));
    auto future6 = broker.future();
    waitForFuture<QEventLoop>(future6);
    ASSERT_TRUE(future6.isFinished());
    ASSERT_TRUE(future6.isCanceled()); // Exception futures are always canceled
    ASSERT_EQ(getFutureState(future6), FutureState::Exception);
    ASSERT_THROW(future6.waitForFinished(), MyException);

    // Test timed exception handling
    broker.bindOrReplace(createTimedExceptionFuture<void>(UsualDuration, MyException()));
    auto future7 = broker.future();
    waitForFuture<QEventLoop>(future7);
    ASSERT_TRUE(future7.isFinished());
    ASSERT_TRUE(future7.isCanceled());
    ASSERT_EQ(getFutureState(future7), FutureState::Exception);
    ASSERT_THROW(future7.waitForFinished(), MyException);

    // Replace exception future with normal future
    broker.bindOrReplace(createReadyFuture());
    auto future8 = broker.future();
    waitForFuture<QEventLoop>(future8);
    ASSERT_TRUE(future8.isFinished());
    ASSERT_FALSE(future8.isCanceled());
}
