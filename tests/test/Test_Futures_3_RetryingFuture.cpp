/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <gtest/gtest.h>

#include <UtilsQt/Futures/RetryingFuture.h>
#include <UtilsQt/Futures/Utils.h>
#include <QTimer>
#include <QEventLoop>
#include <QCoreApplication>

#include "internal/LifetimeTracker.h"

using namespace UtilsQt;

TEST(UtilsQt, Futures_RetryingFuture_Basic_Bool)
{
    QObject context;
    int count = 3;

    auto future = createRetryingFutureRR(&context, [&count]() -> QFuture<bool> {
                                        count--;
                                        return createTimedFuture(50, true);
                                    });

    waitForFuture<QEventLoop>(future);
    ASSERT_TRUE(future.result().result);
    ASSERT_TRUE(future.result().isOk);
    ASSERT_EQ(count, 2);
}

TEST(UtilsQt, Futures_RetryingFuture_Basic_Int)
{
    QObject context;
    int count = 3;

    auto future = createRetryingFutureRR(&context, [&count]() -> QFuture<int> {
                                        count--;
                                        return createTimedFuture(100, 52);
                                    });

    waitForFuture<QEventLoop>(future);
    ASSERT_EQ(future.result().result, 52);
    ASSERT_TRUE(future.result().isOk);
    ASSERT_EQ(count, 2);
}

TEST(UtilsQt, Futures_RetryingFuture_Retried)
{
    QObject context;
    int count = 2;

    auto future = createRetryingFutureRR(&context, [&count]() -> QFuture<bool> {
                                        count--;
                                        if (count == 0)
                                            return createTimedFuture(48, true);
                                        return createTimedCanceledFuture<bool>(26);
                                    });

    waitForFuture<QEventLoop>(future);
    ASSERT_TRUE(future.isCanceled());
    ASSERT_EQ(count, 1);
}

TEST(UtilsQt, Futures_RetryingFuture_Retried_Canceled)
{
    QObject context;
    int count = 3;

    auto future = createRetryingFutureRR(&context, [&count]() -> QFuture<bool> {
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

    auto future = createRetryingFutureRR(&context, [&count]() -> QFuture<bool> {
                                        count--;
                                        return createTimedCanceledFuture<bool>(14);
                                    });

    waitForFuture<QEventLoop>(future);
    ASSERT_TRUE(future.isCanceled());
    ASSERT_EQ(count, 2);
}

TEST(UtilsQt, Futures_RetryingFuture_Validator)
{
    QObject context;
    int count = 3;

    auto future = createRetryingFutureRR(&context, [&count]() -> QFuture<int> {
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
    ASSERT_EQ(future.result().result, 48);
    ASSERT_TRUE(future.result().isOk);
    ASSERT_EQ(count, 0);
}

TEST(UtilsQt, Futures_RetryingFuture_Validator_Instant)
{
    QObject context;
    int count = 3;

    auto future = createRetryingFutureRR(&context, [&count]() -> QFuture<int> {
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
    ASSERT_EQ(future.result().result, 48);
    ASSERT_TRUE(future.result().isOk);
    ASSERT_EQ(count, 0);
}

TEST(UtilsQt, Futures_RetryingFuture_Validator_Canceled)
{
    QObject context;
    int count = 3;

    auto future = createRetryingFutureRR(&context, [&count]() -> QFuture<bool> {
                                        count--;
                                        return createTimedFuture(14, false);
                                    },
                                    [](const std::optional<bool>& result) -> ValidatorDecision {
                                        return result.value_or(false) ? ValidatorDecision::ResultIsValid :
                                                                        ValidatorDecision::NeedRetry;
                                    }
    );

    waitForFuture<QEventLoop>(future);
    ASSERT_FALSE(future.isCanceled());
    ASSERT_EQ(future.result().result, false);
    ASSERT_EQ(future.result().isOk, false);
    ASSERT_EQ(count, 0);
}

TEST(UtilsQt, Futures_RetryingFuture_Cancel_Target)
{
    QObject context;
    int count = 3;

    auto future = createRetryingFutureRR(&context, [&count]() -> QFuture<int> {
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
    QFuture<RetryingResult<int>> future;
    int count = 3;

    {
        QObject context;


        future = createRetryingFutureRR(&context, [&count]() -> QFuture<int> {
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

TEST(UtilsQt, Futures_RetryingFuture_Lifetime)
{
    LifetimeTracker tracker;

    ASSERT_EQ(tracker.count(), 1);
    auto f = createRetryingFutureRR(nullptr, [tracker](){ (void)tracker; return createTimedFuture(10); });
    ASSERT_EQ(tracker.count(), 2);

    waitForFuture<QEventLoop>(f);
    qApp->processEvents();
    qApp->processEvents();
    ASSERT_EQ(tracker.count(), 1);
}

TEST(UtilsQt, Futures_RetryingFuture_PreciseResultValidation)
{
    const auto validator = [](const std::optional<int> &result) -> ValidatorDecision {
        if (result) {
            return (*result >= 0 && *result <= 100) ? ValidatorDecision::ResultIsValid
                                                    : ValidatorDecision::NeedRetry;
        } else {
            return ValidatorDecision::Cancel;
        }
    };

    {
        auto future = createRetryingFutureRR(nullptr, []() { return createTimedFuture(27, 52); }, validator);

        waitForFuture<QEventLoop>(future);
        ASSERT_EQ(future.result().result, 52);
        ASSERT_TRUE(future.result().isOk);
    }

    {
        auto future = createRetryingFutureRR(nullptr, []() { return createTimedFuture(27, 101); }, validator);

        waitForFuture<QEventLoop>(future);
        ASSERT_EQ(future.result().result, 101);
        ASSERT_FALSE(future.result().isOk);
    }

    {
        int result = 102;
        auto future = createRetryingFutureRR(nullptr, [&result]() { return createTimedFuture(27, result--); }, validator);

        waitForFuture<QEventLoop>(future);
        ASSERT_EQ(future.result().result, 100);
        ASSERT_TRUE(future.result().isOk);
    }
}
