#include <gtest/gtest.h>

#include <UtilsQt/Futures/RetryingFuture.h>
#include <UtilsQt/Futures/Utils.h>
#include <QTimer>
#include <QEventLoop>

using namespace UtilsQt;

TEST(UtilsQt, Futures_RetryingFuture_Basic_Bool)
{
    QObject context;
    int count = 3;

    auto future = createRetryingFuture(&context, [&count]() -> QFuture<bool> {
                                        count--;
                                        return createTimedFuture(50, true);
                                    });

    waitForFuture<QEventLoop>(future);
    ASSERT_TRUE(future.result());
    ASSERT_EQ(count, 2);
}

TEST(UtilsQt, Futures_RetryingFuture_Basic_Int)
{
    QObject context;
    int count = 3;

    auto future = createRetryingFuture(&context, [&count]() -> QFuture<int> {
                                        count--;
                                        return createTimedFuture(100, 52);
                                    });

    waitForFuture<QEventLoop>(future);
    ASSERT_EQ(future.result(), 52);
    ASSERT_EQ(count, 2);
}

TEST(UtilsQt, Futures_RetryingFuture_Retried)
{
    QObject context;
    int count = 2;

    auto future = createRetryingFuture(&context, [&count]() -> QFuture<bool> {
                                        count--;
                                        if (count == 0)
                                            return createTimedFuture(48, true);
                                        return createTimedCanceledFuture<bool>(26);
                                    });

    waitForFuture<QEventLoop>(future);
    ASSERT_TRUE(future.result());
    ASSERT_EQ(count, 0);
}

TEST(UtilsQt, Futures_RetryingFuture_Retried_Canceled)
{
    QObject context;
    int count = 3;

    auto future = createRetryingFuture(&context, [&count]() -> QFuture<bool> {
                                        count--;
                                        if (count == 0)
                                            return createTimedFuture(48, true);
                                        return createTimedCanceledFuture<bool>(26);
                                       }, [](auto){ return ValidatorDecision::Cancel; }, 1);

    waitForFuture<QEventLoop>(future);
    ASSERT_TRUE(future.isCanceled());
    ASSERT_EQ(count, 2);
}

TEST(UtilsQt, Futures_RetryingFuture_Retried_Canceled_2)
{
    QObject context;
    int count = 3;

    auto future = createRetryingFuture(&context, [&count]() -> QFuture<bool> {
                                        count--;
                                        return createTimedCanceledFuture<bool>(14);
                                    });

    waitForFuture<QEventLoop>(future);
    ASSERT_TRUE(future.isCanceled());
    ASSERT_EQ(count, 0);
}

TEST(UtilsQt, Futures_RetryingFuture_Validator)
{
    QObject context;
    int count = 3;

    auto future = createRetryingFuture(&context, [&count]() -> QFuture<int> {
                                        count--;
                                        if (count == 0)
                                            return createTimedFuture(21, 48);
                                        return createTimedFuture(27, 52);
                                    },
                                    [](const std::optional<int>& result) -> ValidatorDecision {
                                        return result.value_or(-1) == 48 ? ValidatorDecision::ResultIsValid :
                                                                           ValidatorDecision::NeedRetry;
                                    }
                                    );

    waitForFuture<QEventLoop>(future);
    ASSERT_EQ(future.result(), 48);
    ASSERT_EQ(count, 0);
}

TEST(UtilsQt, Futures_RetryingFuture_Validator_Instant)
{
    QObject context;
    int count = 3;

    auto future = createRetryingFuture(&context, [&count]() -> QFuture<int> {
                                        count--;
                                        if (count == 0)
                                            return createTimedFuture(21, 48);
                                        return createTimedFuture(27, 52);
                                    },
                                    [](const std::optional<int>& result) -> ValidatorDecision {
                                        return result.value_or(-1) == 48 ? ValidatorDecision::ResultIsValid :
                                                                           ValidatorDecision::NeedRetry;
                                    },
                                    3,  // Calls limit
                                    0); // Interval

    waitForFuture<QEventLoop>(future);
    ASSERT_EQ(future.result(), 48);
    ASSERT_EQ(count, 0);
}

TEST(UtilsQt, Futures_RetryingFuture_Validator_Canceled)
{
    QObject context;
    int count = 3;

    auto future = createRetryingFuture(&context, [&count]() -> QFuture<bool> {
                                        count--;
                                        return createTimedFuture(14, false);
                                    },
                                    [](const std::optional<bool>& result) -> ValidatorDecision {
                                        return result.value_or(false) ? ValidatorDecision::ResultIsValid :
                                                                        ValidatorDecision::NeedRetry;
                                    }
    );

    waitForFuture<QEventLoop>(future);
    ASSERT_TRUE(future.isCanceled());
    ASSERT_EQ(count, 0);
}

TEST(UtilsQt, Futures_RetryingFuture_Cancel_Target)
{
    QObject context;
    int count = 3;

    auto future = createRetryingFuture(&context, [&count]() -> QFuture<int> {
                                        count--;
                                        if (count == 0)
                                            return createTimedFuture(21, 48);
                                        return createTimedFuture(27, 52);
                                    },
                                    [](const std::optional<int>& result) -> ValidatorDecision {
                                        return result.value_or(-1) == 48 ? ValidatorDecision::ResultIsValid :
                                                                           ValidatorDecision::NeedRetry;
                                    });

    ASSERT_FALSE(future.isCanceled());
    future.cancel();
    ASSERT_TRUE(future.isCanceled());

    waitForFuture<QEventLoop>(future);

    ASSERT_EQ(count, 2);
}

TEST(UtilsQt, Futures_RetryingFuture_Context)
{
    QFuture<int> future;
    int count = 3;

    {
        QObject context;


        future = createRetryingFuture(&context, [&count]() -> QFuture<int> {
                                       count--;
                                       if (count == 0)
                                           return createTimedFuture(21, 48);
                                       return createTimedFuture(27, 52);
                                   },
                                   [](const std::optional<int>& result) -> ValidatorDecision {
                                       return result.value_or(-1) == 48 ? ValidatorDecision::ResultIsValid :
                                                                          ValidatorDecision::NeedRetry;
                                   });

        ASSERT_FALSE(future.isCanceled());
    }

    ASSERT_TRUE(future.isCanceled());

    waitForFuture<QEventLoop>(future);

    ASSERT_EQ(count, 2);
}
