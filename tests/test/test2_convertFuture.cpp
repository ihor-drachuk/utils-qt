#include <gtest/gtest.h>
#include <utils-qt/futurebridge.h>
#include <utils-qt/futureutils.h>
#include <QEventLoop>


TEST(UtilsQt, FutureBridgeTest)
{
    {
        auto f = createReadyFuture(170);

        int input = -1;
        const char* const result = "Test";

        auto conv = convertFuture<int, const char*>(f, [&](int value) -> std::optional<const char*> { input = value; return result; });
        ASSERT_TRUE(conv.future.isFinished());
        ASSERT_FALSE(conv.future.isCanceled());
        ASSERT_EQ(input, 170);
        ASSERT_EQ(conv.future.result(), result);
    }

    {
        auto f = createReadyFuture(170);

        int input = -1;

        auto conv = convertFuture<int, const char*>(f, [&](int value) -> std::optional<const char*> { input = value; return {}; });
        ASSERT_TRUE(conv.future.isFinished());
        ASSERT_TRUE(conv.future.isCanceled());
        ASSERT_EQ(input, 170);
    }

    {
        auto f = createReadyFuture();

        bool ok = false;
        const char* const result = "Test";

        auto conv = convertFuture<void, const char*>(f, [&]() -> std::optional<const char*> { ok = true; return result; });
        ASSERT_FALSE(conv.future.isCanceled());
        ASSERT_TRUE(conv.future.isFinished());
        ASSERT_TRUE(ok);
        ASSERT_EQ(conv.future.result(), result);
    }

    {
        auto f = createReadyFuture();

        bool ok = false;

        auto conv = convertFuture<void, const char*>(f, [&]() -> std::optional<const char*> { ok = true; return {}; });
        ASSERT_TRUE(conv.future.isCanceled());
        ASSERT_TRUE(conv.future.isFinished());
        ASSERT_TRUE(ok);
    }

    {
        auto f = createCanceledFuture<int>();

        bool ok = false;

        auto conv = convertFuture<void, const char*>(f, [&]() -> std::optional<const char*> { ok = true; return {}; });
        ASSERT_TRUE(conv.future.isCanceled());
        ASSERT_TRUE(conv.future.isFinished());
        ASSERT_FALSE(ok);
    }

    {
        auto f = createCanceledFuture<void>();

        bool ok = false;

        auto conv = convertFuture<void, const char*>(f, [&]() -> std::optional<const char*> { ok = true; return {}; });
        ASSERT_TRUE(conv.future.isCanceled());
        ASSERT_TRUE(conv.future.isFinished());
        ASSERT_FALSE(ok);
    }
}

TEST(UtilsQt, FutureBridgeTest_Timed)
{
    {
        auto f = createTimedFuture(170, 20);

        int input = -1;
        const char* const result = "Test";

        auto conv = convertFuture<int, const char*>(f, [&](int value) -> std::optional<const char*> { input = value; return result; });
        ASSERT_FALSE(conv.future.isFinished());
        waitForFuture<QEventLoop>(conv.future);
        ASSERT_TRUE(conv.future.isFinished());
        ASSERT_FALSE(conv.future.isCanceled());
        ASSERT_EQ(input, 170);
        ASSERT_EQ(conv.future.result(), result);
    }

    {
        auto f = createTimedFuture(170, 20);

        int input = -1;

        auto conv = convertFuture<int, const char*>(f, [&](int value) -> std::optional<const char*> { input = value; return {}; });
        ASSERT_FALSE(conv.future.isFinished());
        waitForFuture<QEventLoop>(conv.future);
        ASSERT_TRUE(conv.future.isFinished());
        ASSERT_TRUE(conv.future.isCanceled());
        ASSERT_EQ(input, 170);
    }

    {
        auto f = createTimedFuture(20);

        bool ok = false;
        const char* const result = "Test";

        auto conv = convertFuture<void, const char*>(f, [&]() -> std::optional<const char*> { ok = true; return result; });
        ASSERT_FALSE(conv.future.isFinished());
        waitForFuture<QEventLoop>(conv.future);
        ASSERT_FALSE(conv.future.isCanceled());
        ASSERT_TRUE(conv.future.isFinished());
        ASSERT_TRUE(ok);
        ASSERT_EQ(conv.future.result(), result);
    }

    {
        auto f = createTimedFuture(20);

        bool ok = false;

        auto conv = convertFuture<void, const char*>(f, [&]() -> std::optional<const char*> { ok = true; return {}; });
        ASSERT_FALSE(conv.future.isFinished());
        waitForFuture<QEventLoop>(conv.future);
        ASSERT_TRUE(conv.future.isCanceled());
        ASSERT_TRUE(conv.future.isFinished());
        ASSERT_TRUE(ok);
    }

    {
        auto f = createTimedCanceledFuture<int>(20);

        bool ok = false;

        auto conv = convertFuture<void, const char*>(f, [&]() -> std::optional<const char*> { ok = true; return {}; });
        ASSERT_FALSE(conv.future.isFinished());
        waitForFuture<QEventLoop>(conv.future);
        ASSERT_TRUE(conv.future.isCanceled());
        ASSERT_TRUE(conv.future.isFinished());
        ASSERT_FALSE(ok);
    }

    {
        auto f = createTimedCanceledFuture<void>(20);

        bool ok = false;

        auto conv = convertFuture<void, const char*>(f, [&]() -> std::optional<const char*> { ok = true; return {}; });
        ASSERT_FALSE(conv.future.isFinished());
        waitForFuture<QEventLoop>(conv.future);
        ASSERT_TRUE(conv.future.isCanceled());
        ASSERT_TRUE(conv.future.isFinished());
        ASSERT_FALSE(ok);
    }
}
