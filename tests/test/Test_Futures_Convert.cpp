#include <gtest/gtest.h>
#include <UtilsQt/Futures/Converter.h>
#include <UtilsQt/Futures/Utils.h>
#include <QCoreApplication>
#include <QEventLoop>

#include "internal/LifetimeTracker.h"

using namespace UtilsQt;


TEST(UtilsQt, Futures_Convert)
{
    {
        auto f = createReadyFuture(170);

        int input = -1;
        const char* const result = "Test";

        auto conv = convertFuture<int, const char*>(nullptr, f, [&](int value) -> std::optional<const char*> { input = value; return result; });
        ASSERT_TRUE(conv.isFinished());
        ASSERT_FALSE(conv.isCanceled());
        ASSERT_EQ(input, 170);
        ASSERT_EQ(conv.result(), result);
    }

    {
        auto f = createReadyFuture(170);

        int input = -1;

        auto conv = convertFuture<int, const char*>(nullptr, f, [&](int value) -> std::optional<const char*> { input = value; return {}; });
        ASSERT_TRUE(conv.isFinished());
        ASSERT_TRUE(conv.isCanceled());
        ASSERT_EQ(input, 170);
    }

    {
        auto f = createReadyFuture();

        bool ok = false;
        const char* const result = "Test";

        auto conv = convertFuture<void, const char*>(nullptr, f, [&]() -> std::optional<const char*> { ok = true; return result; });
        ASSERT_FALSE(conv.isCanceled());
        ASSERT_TRUE(conv.isFinished());
        ASSERT_TRUE(ok);
        ASSERT_EQ(conv.result(), result);
    }

    {
        auto f = createReadyFuture();

        bool ok = false;

        auto conv = convertFuture<void, const char*>(nullptr, f, [&]() -> std::optional<const char*> { ok = true; return {}; });
        ASSERT_TRUE(conv.isCanceled());
        ASSERT_TRUE(conv.isFinished());
        ASSERT_TRUE(ok);
    }

    {
        auto f = createCanceledFuture<int>();

        bool ok = false;

        auto conv = convertFuture<int, const char*>(nullptr, f, [&](int) -> std::optional<const char*> { ok = true; return {}; });
        ASSERT_TRUE(conv.isCanceled());
        ASSERT_TRUE(conv.isFinished());
        ASSERT_FALSE(ok);
    }

    {
        auto f = createCanceledFuture<void>();

        bool ok = false;

        auto conv = convertFuture<void, const char*>(nullptr, f, [&]() -> std::optional<const char*> { ok = true; return {}; });
        ASSERT_TRUE(conv.isCanceled());
        ASSERT_TRUE(conv.isFinished());
        ASSERT_FALSE(ok);
    }
}

TEST(UtilsQt, Futures_Convert_Timed)
{
    {
        auto f = createTimedFuture(20, 170);

        int input = -1;
        const char* const result = "Test";

        auto conv = convertFuture<int, const char*>(nullptr, f, [&](int value) -> std::optional<const char*> { input = value; return result; });
        ASSERT_FALSE(conv.isFinished());
        waitForFuture<QEventLoop>(conv);
        ASSERT_TRUE(conv.isFinished());
        ASSERT_FALSE(conv.isCanceled());
        ASSERT_EQ(input, 170);
        ASSERT_EQ(conv.result(), result);
    }

    {
        auto f = createTimedFuture(20, 170);

        int input = -1;

        auto conv = convertFuture<int, const char*>(nullptr, f, [&](int value) -> std::optional<const char*> { input = value; return {}; });
        ASSERT_FALSE(conv.isFinished());
        waitForFuture<QEventLoop>(conv);
        ASSERT_TRUE(conv.isFinished());
        ASSERT_TRUE(conv.isCanceled());
        ASSERT_EQ(input, 170);
    }

    {
        auto f = createTimedFuture(20);

        bool ok = false;
        const char* const result = "Test";

        auto conv = convertFuture<void, const char*>(nullptr, f, [&]() -> std::optional<const char*> { ok = true; return result; });
        ASSERT_FALSE(conv.isFinished());
        waitForFuture<QEventLoop>(conv);
        ASSERT_FALSE(conv.isCanceled());
        ASSERT_TRUE(conv.isFinished());
        ASSERT_TRUE(ok);
        ASSERT_EQ(conv.result(), result);
    }

    {
        auto f = createTimedFuture(20);

        bool ok = false;

        auto conv = convertFuture<void, const char*>(nullptr, f, [&]() -> std::optional<const char*> { ok = true; return {}; });
        ASSERT_FALSE(conv.isFinished());
        waitForFuture<QEventLoop>(conv);
        ASSERT_TRUE(conv.isCanceled());
        ASSERT_TRUE(conv.isFinished());
        ASSERT_TRUE(ok);
    }

    {
        auto f = createTimedCanceledFuture<int>(20);

        bool ok = false;

        auto conv = convertFuture<int, const char*>(nullptr, f, [&](int) -> std::optional<const char*> { ok = true; return {}; });
        ASSERT_FALSE(conv.isFinished());
        waitForFuture<QEventLoop>(conv);
        ASSERT_TRUE(conv.isCanceled());
        ASSERT_TRUE(conv.isFinished());
        ASSERT_FALSE(ok);
    }

    {
        auto f = createTimedCanceledFuture<void>(20);

        bool ok = false;

        auto conv = convertFuture<void, const char*>(nullptr, f, [&]() -> std::optional<const char*> { ok = true; return {}; });
        ASSERT_FALSE(conv.isFinished());
        waitForFuture<QEventLoop>(conv);
        ASSERT_TRUE(conv.isCanceled());
        ASSERT_TRUE(conv.isFinished());
        ASSERT_FALSE(ok);
    }
}

TEST(UtilsQt, Futures_Convert_Destruction)
{
    {
        bool ok = false;
        QFuture<int> resultFuture;

        {
            auto f = createTimedFuture(100, 20);

            QObject ctx;
            resultFuture = convertFuture<int, int>(&ctx, f, [&](int value) -> std::optional<int> { ok = true; return value + 1; });
            ASSERT_FALSE(resultFuture.isFinished());
        }

        ASSERT_TRUE(resultFuture.isFinished());
        ASSERT_TRUE(resultFuture.isCanceled());
        ASSERT_FALSE(ok);
    }

    {
        bool ok = false;
        QFuture<int> resultFuture;

        {
            auto f = createPromise<int>();

            QObject ctx;
            resultFuture = convertFuture<int, int>(&ctx, f.future(), [&](int value) -> std::optional<int> { ok = true; return value + 1; });
            f.finish(170); // Will not handle in time
            ASSERT_FALSE(resultFuture.isFinished());
        }

        ASSERT_TRUE(resultFuture.isFinished());
        ASSERT_TRUE(resultFuture.isCanceled());
        ASSERT_FALSE(ok);
    }
}



TEST(UtilsQt, Futures_Convert_StepByStep)
{
    {
        auto f = createTimedFuture(50, 1234);

        auto conv = convertFuture<int, std::string>(nullptr, f, [&](int value) -> std::optional<std::string> { return std::to_string(value); });

        ASSERT_TRUE(conv.isStarted());
        ASSERT_FALSE(conv.isCanceled());
        ASSERT_FALSE(conv.isFinished());

        waitForFuture<QEventLoop>(conv);

        ASSERT_FALSE(conv.isCanceled());
        ASSERT_TRUE(conv.isFinished());
        ASSERT_EQ(conv.result(), "1234");
    }

    {
        QFutureInterface<int> fi;

        auto f = fi.future();
        auto conv = convertFuture<int, std::string>(nullptr, f, [&](int value) -> std::optional<std::string> { return std::to_string(value); });

        ASSERT_FALSE(conv.isStarted());
        ASSERT_FALSE(conv.isCanceled());
        ASSERT_FALSE(conv.isFinished());
    }

    {
        QFutureInterface<int> fi;

        auto f = fi.future();
        auto conv = convertFuture<int, std::string>(nullptr, f, [&](int value) -> std::optional<std::string> { return std::to_string(value); });

        ASSERT_FALSE(conv.isStarted());
        ASSERT_FALSE(conv.isCanceled());
        ASSERT_FALSE(conv.isFinished());

        fi.reportStarted();
        ASSERT_TRUE(f.isStarted());

        waitForFuture<QEventLoop>(createTimedCanceledFuture<void>(1));

        ASSERT_TRUE(conv.isStarted());
        ASSERT_FALSE(conv.isCanceled());
        ASSERT_FALSE(conv.isFinished());

        fi.reportCanceled();
        waitForFuture<QEventLoop>(createTimedCanceledFuture<void>(1));

        ASSERT_TRUE(conv.isStarted());
        ASSERT_TRUE(conv.isCanceled());
        ASSERT_FALSE(conv.isFinished());

        fi.reportFinished();
        waitForFuture<QEventLoop>(createTimedCanceledFuture<void>(1));

        ASSERT_TRUE(conv.isStarted());
        ASSERT_TRUE(conv.isCanceled());
        ASSERT_TRUE(conv.isFinished());
    }


    {
        QFutureInterface<int> fi;

        auto f = fi.future();
        auto conv = convertFuture<int, std::string>(nullptr, f, [&](int value) -> std::optional<std::string> { return std::to_string(value); });

        ASSERT_FALSE(conv.isStarted());
        ASSERT_FALSE(conv.isCanceled());
        ASSERT_FALSE(conv.isFinished());

        fi.reportStarted();
        waitForFuture<QEventLoop>(createTimedCanceledFuture<void>(1));

        ASSERT_TRUE(conv.isStarted());
        ASSERT_FALSE(conv.isCanceled());
        ASSERT_FALSE(conv.isFinished());

        fi.reportResult(1234);
        waitForFuture<QEventLoop>(createTimedCanceledFuture<void>(1));

        ASSERT_TRUE(conv.isStarted());
        ASSERT_FALSE(conv.isCanceled());
        ASSERT_FALSE(conv.isFinished());

        fi.reportFinished();
        waitForFuture<QEventLoop>(createTimedCanceledFuture<void>(1));

        ASSERT_TRUE(conv.isStarted());
        ASSERT_FALSE(conv.isCanceled());
        ASSERT_TRUE(conv.isFinished());

        ASSERT_EQ(conv.resultCount(), 1);
        ASSERT_EQ(conv.result(), "1234");
    }
}


TEST(UtilsQt, Futures_Convert_CancelTarget)
{
    {
        QFutureInterface<int> fi;

        auto f = fi.future();
        auto conv = convertFuture<int, std::string>(nullptr, f, [&](int value) -> std::optional<std::string> { return std::to_string(value); });

        ASSERT_FALSE(fi.isStarted());
        ASSERT_FALSE(fi.isCanceled());
        ASSERT_FALSE(fi.isFinished());

        ASSERT_FALSE(conv.isStarted());
        ASSERT_FALSE(conv.isCanceled());
        ASSERT_FALSE(conv.isFinished());

        conv.cancel();
        waitForFuture<QEventLoop>(createTimedCanceledFuture<void>(1));

        ASSERT_FALSE(fi.isStarted());
        ASSERT_TRUE(fi.isCanceled());
        ASSERT_FALSE(fi.isFinished());

        ASSERT_FALSE(conv.isStarted());
        ASSERT_TRUE(conv.isCanceled());
        ASSERT_FALSE(conv.isFinished());
    }
}

TEST(UtilsQt, Futures_Convert_CancelSource)
{
    {
        QFutureInterface<int> fi;

        QEventLoop().processEvents();
        ASSERT_FALSE(fi.isCanceled());

        {
            QObject ctx;
            auto conv = convertFuture<int, std::string>(&ctx, fi.future(), [&](int value) -> std::optional<std::string> { return std::to_string(value); });
        }

        QEventLoop().processEvents();
        ASSERT_TRUE(fi.isCanceled());
    }
}

TEST(UtilsQt, Futures_Convert_NoHint)
{
    struct SomeData {SomeData(int){};};

    auto f1 = convertFuture(nullptr, QFuture<void>(), []() {
        return std::to_string(12);
    });

    auto f2 = convertFuture(nullptr, QFuture<int>(), [](int i) {
        return std::to_string(i);
    });

    auto f3 = convertFuture(nullptr, QFuture<SomeData>(), [](const SomeData&) {
        return 0;
    });

    (void)f1, (void)f2, (void)f3;
}

TEST(UtilsQt, Futures_Convert_Lifetime)
{
    LifetimeTracker tracker;

    ASSERT_EQ(tracker.count(), 1);
    auto f = convertFuture(nullptr, createTimedFuture(10), [tracker]() {
        (void)tracker;
        return QString();
    });
    ASSERT_EQ(tracker.count(), 2);

    waitForFuture<QEventLoop>(f);
    qApp->processEvents();
    qApp->processEvents();
    ASSERT_EQ(tracker.count(), 1);
}
