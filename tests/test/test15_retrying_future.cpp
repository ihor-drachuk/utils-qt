#include <gtest/gtest.h>

#include <utils-qt/retrying_future.h>
#include <utils-qt/futureutils.h>
#include <QTimer>
#include <QEventLoop>

TEST(UtilsQt, RetryingFuture_Basic)
{
    QObject context;
    int count = 3;

    auto future = createRetryFuture(&context, [&count]() ->QFuture<bool> {
                                        count--;
                                        return createTimedFuture(50, true);
                                    });

    waitForFuture<QEventLoop>(future);
    ASSERT_TRUE(future.result());
    ASSERT_TRUE(count == 2);
}

TEST(UtilsQt, RetryingFuture_BasicInt)
{
    QObject context;
    int count = 3;

    auto future = createRetryFuture(&context, [&count]() ->QFuture<int> {
                                        count--;
                                        return createTimedFuture(100, 52);
                                    });

    waitForFuture<QEventLoop>(future);
    ASSERT_TRUE(future.result() == 52);
    ASSERT_TRUE(count == 2);
}

TEST(UtilsQt, RetryingFuture_Retryed)
{
    QObject context;
    int count = 3;

    auto future = createRetryFuture(&context, [&count]() ->QFuture<bool> {
                                        count--;
                                        if (count == 0)
                                            return createTimedFuture(48, true);
                                        return createTimedCanceledFuture<bool>(26);
                                    });

    waitForFuture<QEventLoop>(future);
    ASSERT_TRUE(future.result());
    ASSERT_TRUE(count == 0);
}

TEST(UtilsQt, RetryingFuture_Cancel)
{
    QObject context;
    int count = 3;

    auto future = createRetryFuture(&context, [&count]() ->QFuture<bool> {
                                        count--;
                                        return createTimedCanceledFuture<bool>(14);
                                    });

    waitForFuture<QEventLoop>(future);
    ASSERT_TRUE(future.isCanceled());
    ASSERT_TRUE(count == 0);
}

TEST(UtilsQt, RetryingFuture_BasicInt_with_checker)
{
    QObject context;
    int count = 3;

    auto future = createRetryFuture(&context, [&count]() ->QFuture<int> {
                                        count--;
                                        if (count == 0)
                                            return createTimedFuture(21, 48);
                                        return createTimedFuture(27, 52);
                                    },
                                    [](int result)->bool{ return  result == 48; }
                                    );

    waitForFuture<QEventLoop>(future);
    ASSERT_TRUE(future.result() == 48);
    ASSERT_TRUE(count == 0);
}

TEST(UtilsQt, RetryingFuture_Cancel_with_checker)
{
    QObject context;
    int count = 3;

    auto future = createRetryFuture(&context, [&count]() ->QFuture<bool> {
                                        count--;
                                        return createTimedFuture(14, false);
                                    },
                                    [](bool result)->bool { return result; });

    waitForFuture<QEventLoop>(future);
    ASSERT_TRUE(future.isCanceled());
    ASSERT_TRUE(count == 0);
}
