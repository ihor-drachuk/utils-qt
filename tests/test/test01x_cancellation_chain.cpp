#include <gtest/gtest.h>

#include <utils-qt/futureutils.h>
#include <utils-qt/futureutils_combine.h>
#include <utils-qt/futureutils_combine_container.h>
#include <utils-qt/futureutils_sequential.h>
#include <utils-qt/futurebridge.h>
#include <utils-qt/futuresunifier.h>
#include <utils-qt/retrying_future.h>
#include <QEventLoop>

TEST(UtilsQt, CancellationChain_convert)
{
    auto promise = createPromise<int>();
    auto f = convertFuture<int, int>(promise->future(), [](const std::optional<int>& value) -> std::optional<int> {
        if (!value.has_value())
            return {};

        return *value + 1;
    });

    f.future.cancel();
    waitForFuture<QEventLoop>(createTimedFuture(1));

    ASSERT_TRUE(promise->isCanceled());

    promise->cancel(); // need to call reportFinished()
    waitForFuture<QEventLoop>(f.future);

    ASSERT_TRUE(f.future.isFinished());
}


TEST(UtilsQt, CancellationChain_mergeFuturesAll)
{
    QObject ctx;
    auto p1 = createPromise<int>();
    auto p2 = createPromise<int>();
    auto p3 = createPromise<int>();
    auto f = mergeFuturesAll(&ctx, p1->future(), p2->future(), p3->future());

    f.cancel();
    waitForFuture<QEventLoop>(createTimedFuture(1));

    ASSERT_TRUE(p1->isCanceled());
    ASSERT_TRUE(p2->isCanceled());
    ASSERT_TRUE(p3->isCanceled());

    p1->cancel(); // need to call reportFinished()
    p2->cancel();
    p3->cancel();
    waitForFuture<QEventLoop>(f);
    ASSERT_TRUE(f.isFinished());
}


TEST(UtilsQt, CancellationChain_combineFutures)
{
    QObject ctx;
    auto p1 = createPromise<int>();
    auto p2 = createPromise<int>();
    auto p3 = createPromise<int>();
    QVector<QFuture<int>> futures {p1->future(), p2->future(), p3->future()};

    auto f = UtilsQt::combineFutures(futures, &ctx);

    f.cancel();
    waitForFuture<QEventLoop>(createTimedFuture(1));

    ASSERT_TRUE(p1->isCanceled());
    ASSERT_TRUE(p2->isCanceled());
    ASSERT_TRUE(p3->isCanceled());

    p1->cancel(); // need to call reportFinished()
    p2->cancel();
    p3->cancel();
    waitForFuture<QEventLoop>(f);
    ASSERT_TRUE(f.isFinished());
}
