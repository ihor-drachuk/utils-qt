#include <gtest/gtest.h>
#include <utils-qt/futureutils.h>
#include <QEventLoop>
#include <QTimer>


TEST(UtilsQt, FutureUtilsTest_Finished)
{
    {
        auto f = createReadyFuture();
        ASSERT_FALSE(f.isCanceled());
        ASSERT_TRUE(f.isFinished());
    }

    {
        const char* str = "Hello!";
        auto f = createReadyFuture(str);
        ASSERT_FALSE(f.isCanceled());
        ASSERT_TRUE(f.isFinished());
    }

    {
        auto f = createCanceledFuture<void>();
        ASSERT_TRUE(f.isCanceled());
        ASSERT_TRUE(f.isFinished());
    }

    {
        auto f = createCanceledFuture<int>();
        ASSERT_TRUE(f.isCanceled());
        ASSERT_TRUE(f.isFinished());
    }
}

TEST(UtilsQt, FutureUtilsTest_Timed)
{
    {
        auto f = createTimedFuture(20);
        ASSERT_FALSE(f.isFinished());
        ASSERT_FALSE(f.isCanceled());
        waitForFuture<QEventLoop>(f);
        ASSERT_FALSE(f.isCanceled());
        ASSERT_TRUE(f.isFinished());
    }

    {
        auto f = createTimedFuture2(20, false);
        ASSERT_FALSE(f.isFinished());
        ASSERT_FALSE(f.isCanceled());
        waitForFuture<QEventLoop>(f);
        ASSERT_FALSE(f.isCanceled());
        ASSERT_TRUE(f.isFinished());
        ASSERT_EQ(f.result(), false);
    }

    {
        auto f = createTimedCanceledFuture<void>(20);
        ASSERT_FALSE(f.isFinished());
        ASSERT_FALSE(f.isCanceled());
        waitForFuture<QEventLoop>(f);
        ASSERT_TRUE(f.isCanceled());
        ASSERT_TRUE(f.isFinished());
    }

    {
        auto f = createTimedCanceledFuture<bool>(20);
        ASSERT_FALSE(f.isFinished());
        ASSERT_FALSE(f.isCanceled());
        waitForFuture<QEventLoop>(f);
        ASSERT_TRUE(f.isCanceled());
        ASSERT_TRUE(f.isFinished());
    }
}

TEST(UtilsQt, FutureUtilsTest_Future)
{
    {
        QObject ctx;

        auto f = createFuture<int>();
        ASSERT_FALSE(f->isFinished());

        QTimer::singleShot(20, &ctx, [f](){ f->finish(120); });

        waitForFuture<QEventLoop>(f->future());

        ASSERT_TRUE(f->isFinished());
        ASSERT_TRUE(f->future().isFinished());
        ASSERT_FALSE(f->future().isCanceled());
    }

    {
        QObject ctx;

        auto f = createFuture<int>();
        ASSERT_FALSE(f->isFinished());

        QTimer::singleShot(20, &ctx, [f](){ f->cancel(); });

        waitForFuture<QEventLoop>(f->future());

        ASSERT_TRUE(f->isFinished());
        ASSERT_TRUE(f->future().isFinished());
        ASSERT_TRUE(f->future().isCanceled());
    }
}


TEST(UtilsQt, FutureUtilsTest_connectFuture_Finished)
{
    {
        QObject ctx;
        bool ok = false;
        bool ok2 = false;
        bool ok3 = false;
        auto f = createReadyFuture();
        onFinished(f, &ctx, [&ok](){ ok = true; });
        onResult(f, &ctx, [&ok2](){ ok2 = true; });
        onCanceled(f, &ctx, [&ok3](){ ok3 = true; });
        ASSERT_TRUE(ok);
        ASSERT_TRUE(ok2);
        ASSERT_FALSE(ok3);
    }

    {
        QObject ctx;
        std::optional<int> result;
        int result2 = -1;
        bool ok3 = false;
        auto f = createReadyFuture(170);
        onFinished(f, &ctx, [&result](const std::optional<int>& value){ result = value; });
        onResult(f, &ctx, [&result2](int value){ result2 = value; });
        onCanceled(f, &ctx, [&ok3](){ ok3 = true; });
        ASSERT_TRUE(result.has_value());
        ASSERT_EQ(result.value(), 170);
        ASSERT_EQ(result2, 170);
        ASSERT_FALSE(ok3);
    }

    {
        QObject ctx;
        bool ok = false;
        bool ok2 = false;
        bool ok3 = false;
        auto f = createCanceledFuture<void>();
        onFinished(f, &ctx, [&ok](){ ok = true; });
        onResult(f, &ctx, [&ok2](){ ok2 = true; });
        onCanceled(f, &ctx, [&ok3](){ ok3 = true; });
        ASSERT_FALSE(ok); // connectFuture<void> is NOT triggered on Cancel
        ASSERT_FALSE(ok2);
        ASSERT_TRUE(ok3);
    }

    {
        QObject ctx;
        std::optional<int> result;
        int result2 = -1;
        bool ok3 = false;
        auto f = createCanceledFuture<int>();
        onFinished(f, &ctx, [&result](const std::optional<int>& value){ result = value; });
        onResult(f, &ctx, [&result2](int value){ result2 = value; });
        onCanceled(f, &ctx, [&ok3](){ ok3 = true; });
        ASSERT_FALSE(result.has_value());
        ASSERT_EQ(result2, -1);
        ASSERT_TRUE(ok3);
    }
}

TEST(UtilsQt, FutureUtilsTest_connectFuture_Timed)
{
    {
        QObject ctx;
        bool ok = false;
        bool ok2 = false;
        bool ok3 = false;
        auto f = createTimedFuture(20);
        onFinished(f, &ctx, [&ok](){ ok = true; });
        onResult(f, &ctx, [&ok2](){ ok2 = true; });
        onCanceled(f, &ctx, [&ok3](){ ok3 = true; });
        waitForFuture<QEventLoop>(f);
        ASSERT_TRUE(ok);
        ASSERT_TRUE(ok2);
        ASSERT_FALSE(ok3);
    }

    {
        QObject ctx;
        std::optional<int> result;
        int result2 = -1;
        bool ok3 = false;
        auto f = createTimedFuture2(20, 170);
        onFinished(f, &ctx, [&result](const std::optional<int>& value){ result = value; });
        onResult(f, &ctx, [&result2](int value){ result2 = value; });
        onCanceled(f, &ctx, [&ok3](){ ok3 = true; });
        waitForFuture<QEventLoop>(f);
        ASSERT_TRUE(result.has_value());
        ASSERT_EQ(result.value(), 170);
        ASSERT_EQ(result2, 170);
        ASSERT_FALSE(ok3);
    }

    {
        QObject ctx;
        bool ok = false;
        bool ok2 = false;
        bool ok3 = false;
        auto f = createTimedCanceledFuture<void>(20);
        onFinished(f, &ctx, [&ok](){ ok = true; });
        onResult(f, &ctx, [&ok2](){ ok2 = true; });
        onCanceled(f, &ctx, [&ok3](){ ok3 = true; });
        waitForFuture<QEventLoop>(f);
        ASSERT_FALSE(ok); // connectFuture<void> is NOT triggered on Cancel
        ASSERT_FALSE(ok2);
        ASSERT_TRUE(ok3);
    }

    {
        QObject ctx;
        std::optional<int> result;
        int result2 = -1;
        bool ok3 = false;
        auto f = createTimedCanceledFuture<int>(20);
        onFinished(f, &ctx, [&result](const std::optional<int>& value){ result = value; });
        onResult(f, &ctx, [&result2](int value){ result2 = value; });
        onCanceled(f, &ctx, [&ok3](){ ok3 = true; });
        waitForFuture<QEventLoop>(f);
        ASSERT_FALSE(result.has_value());
        ASSERT_EQ(result2, -1);
        ASSERT_TRUE(ok3);
    }
}
