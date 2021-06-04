#include <gtest/gtest.h>
#include <utils-qt/futurebridge.h>
#include <utils-qt/futureutils.h>

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
    // ...
}
