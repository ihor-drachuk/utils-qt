#include <gtest/gtest.h>
#include <UtilsQt/Futures/Utils.h>
#include <UtilsQt/Futures/Merge.h>
#include <QEventLoop>
#include <vector>
#include <string>

using namespace UtilsQt;


TEST(UtilsQt, Futures_Merge)
{
    {
        auto f1 = createReadyFuture<int>(10);
        auto f2 = createReadyFuture();
        mergeFuturesAll(nullptr, f1, f2);
    }

    {
        auto f1 = createReadyFuture<int>(10);
        auto r = mergeFuturesAll<int>(nullptr, f1);

        ASSERT_TRUE(r.isStarted());
        ASSERT_FALSE(r.isCanceled());
        ASSERT_TRUE(r.isFinished());
    }

    {
        auto f1 = createTimedFuture<int>(10, 123);
        auto f2 = createCanceledFuture<int>();
        auto r1 = mergeFuturesAny(nullptr, f1, f2);
        auto r2 = mergeFuturesAll(nullptr, f1, f2);

        ASSERT_TRUE(r1.isStarted());
        ASSERT_TRUE(r1.isCanceled());
        ASSERT_TRUE(r1.isFinished());
        ASSERT_TRUE(r2.isStarted());
        ASSERT_TRUE(r2.isCanceled());
        ASSERT_TRUE(r2.isFinished());
    }

    {
        auto f1 = createTimedFuture<int>(50, 123);
        auto f2 = createTimedCanceledFuture<int>(5);
        auto r1 = mergeFuturesAny(nullptr, f1, f2);
        auto r2 = mergeFuturesAll(nullptr, f1, f2);

        waitForFuture<QEventLoop>(r1);
        waitForFuture<QEventLoop>(r2);

        ASSERT_TRUE(r1.isCanceled());
        ASSERT_TRUE(r2.isCanceled());
    }

    {
        auto f1 = createTimedFuture<int>(50, 123);
        auto f2 = createTimedFuture<std::string>(5, "Test");
        auto r = mergeFuturesAny(nullptr, f1, f2);

        waitForFuture<QEventLoop>(r);

        ASSERT_TRUE(r.isStarted());
        ASSERT_FALSE(r.isCanceled());
        ASSERT_TRUE(r.isFinished());

        ASSERT_FALSE(f1.isFinished());
        ASSERT_TRUE(f2.isFinished());
    }


    {
        auto f1 = createTimedFuture<int>(50, 123);
        auto f2 = createTimedFuture<std::string>(5, "Test");
        auto r = mergeFuturesAll(nullptr, f1, f2);

        waitForFuture<QEventLoop>(r);

        ASSERT_TRUE(r.isStarted());
        ASSERT_FALSE(r.isCanceled());
        ASSERT_TRUE(r.isFinished());

        ASSERT_TRUE(f1.isFinished());
        ASSERT_TRUE(f2.isFinished());
    }


    { // std::tuple
        auto f1 = createTimedFuture<int>(50, 123);
        auto f2 = createTimedFuture<std::string>(5, "Test");
        const auto futures = std::make_tuple(f1, f2);
        auto r = mergeFuturesAll(nullptr, futures);

        waitForFuture<QEventLoop>(r);

        ASSERT_TRUE(r.isStarted());
        ASSERT_FALSE(r.isCanceled());
        ASSERT_TRUE(r.isFinished());

        ASSERT_TRUE(f1.isFinished());
        ASSERT_TRUE(f2.isFinished());
    }
}

TEST(UtilsQt, Futures_Merge_Container)
{
    {
        auto f1 = createTimedFuture<int>(10, 123);
        auto f2 = createTimedFuture<int>(20, 124);
        auto r = mergeFuturesAll(nullptr, std::vector{f1, f2});

        waitForFuture<QEventLoop>(r);

        ASSERT_TRUE(f1.isFinished());
        ASSERT_TRUE(f2.isFinished());
    }
}

TEST(UtilsQt, Futures_Merge_Context)
{
    QFuture<void> result;

    {
        QObject ctx;
        auto f1 = createTimedFuture<int>(50, 123);
        auto f2 = createTimedFuture<std::string>(100, "Test");
        result = mergeFuturesAny(&ctx, f1, f2);
        ASSERT_FALSE(result.isCanceled());
    }

    ASSERT_TRUE(result.isCanceled());
}

TEST(UtilsQt, Futures_Merge_TargetCancellation)
{
    {
        QFuture<void> result;

        auto f1 = createTimedFuture<int>(50, 123);
        auto f2 = createTimedFuture<std::string>(100, "Test");
        result = mergeFuturesAny(nullptr, f1, f2);

        ASSERT_FALSE(f1.isCanceled());
        ASSERT_FALSE(f2.isCanceled());
        ASSERT_FALSE(result.isCanceled());

        result.cancel();
        QEventLoop().processEvents();

        ASSERT_TRUE(f1.isCanceled());
        ASSERT_TRUE(f2.isCanceled());
        ASSERT_TRUE(result.isCanceled());
    }

    {
        QFuture<void> result;

        auto f1 = createTimedFuture<int>(50, 123);
        auto f2 = createTimedFuture<int>(100, 124);
        result = mergeFuturesAny(nullptr, std::vector{f1, f2});

        ASSERT_FALSE(f1.isCanceled());
        ASSERT_FALSE(f2.isCanceled());
        ASSERT_FALSE(result.isCanceled());

        result.cancel();
        QEventLoop().processEvents();

        ASSERT_TRUE(f1.isCanceled());
        ASSERT_TRUE(f2.isCanceled());
        ASSERT_TRUE(result.isCanceled());
    }
}
