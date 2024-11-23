/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <gtest/gtest.h>

#include <UtilsQt/Futures/Utils.h>
#include <UtilsQt/Futures/futureutils_sequential.h>
#include <UtilsQt/Futures/Converter.h>
#include <UtilsQt/Futures/Merge.h>
#include <UtilsQt/Futures/RetryingFuture.h>
#include <QEventLoop>

using namespace UtilsQt;


TEST(UtilsQt, Futures_CancellationChain_Convert)
{
    auto promise = createPromise<int>(true);
    auto f = UtilsQt::convertFuture<int, int>(nullptr, promise.future(), ConverterFlags::IgnoreNullContext, [](int value) -> std::optional<int> {
        return value + 1;
    });

    f.cancel();
    waitForFuture<QEventLoop>(createTimedFuture(1));

    ASSERT_TRUE(promise.isCanceled());

    promise.cancel(); // need to call reportFinished()
    waitForFuture<QEventLoop>(f);

    ASSERT_TRUE(f.isFinished());
}


TEST(UtilsQt, Futures_CancellationChain_Merge)
{
    QObject ctx;
    auto p1 = createPromise<int>(true);
    auto p2 = createPromise<int>(true);
    auto p3 = createPromise<int>(true);
    QVector<QFuture<int>> futures {p1.future(), p2.future(), p3.future()};

    auto f = UtilsQt::mergeFuturesAll(&ctx, futures);

    f.cancel();
    waitForFuture<QEventLoop>(createTimedFuture(1));

    ASSERT_TRUE(p1.isCanceled());
    ASSERT_TRUE(p2.isCanceled());
    ASSERT_TRUE(p3.isCanceled());

    p1.cancel(); // need to call reportFinished()
    p2.cancel();
    p3.cancel();
    waitForFuture<QEventLoop>(f);
    ASSERT_TRUE(f.isFinished());
}


TEST(UtilsQt, Futures_CancellationChain_RetryingFutureRR)
{
    int calls {0};
    auto future = createRetryingFutureRR(
        nullptr,
        [&calls]() -> QFuture<int> {
            calls++;
            return UtilsQt::createTimedFuture(27, 52);
        },
        [](const std::optional<int>& result) -> ValidatorDecision {
            return result.value_or(-1) == 48 ? ValidatorDecision::ResultIsValid : ValidatorDecision::NeedRetry;
        });

    ASSERT_FALSE(future.isCanceled());
    future.cancel();
    ASSERT_TRUE(future.isCanceled());

    waitForFuture<QEventLoop>(future);

    ASSERT_EQ(calls, 1);
}

TEST(UtilsQt, Futures_CancellationChain_RetryingFuture)
{
    int calls {0};
    auto future = createRetryingFuture(
        nullptr,
        [&calls]() -> QFuture<int> {
            calls++;
            return UtilsQt::createTimedFuture(27, 52);
        },
        [](const std::optional<int>& result) -> ValidatorDecision {
            return result.value_or(-1) == 48 ? ValidatorDecision::ResultIsValid : ValidatorDecision::NeedRetry;
        });

    ASSERT_FALSE(future.isCanceled());
    future.cancel();
    ASSERT_TRUE(future.isCanceled());

    waitForFuture<QEventLoop>(future);

    ASSERT_EQ(calls, 1);
}
