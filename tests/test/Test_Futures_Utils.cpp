#include <gtest/gtest.h>
#include <UtilsQt/Futures/Utils.h>
#include <QCoreApplication>
#include <QEventLoop>
#include <QTimer>
#include <QVector>
#include "internal/LifetimeTracker.h"

using namespace UtilsQt;

namespace {

struct DataPack_AnalyzeFutures
{
    QVector<QFuture<void>> futures;
    UtilsQt::FuturesSetProperties properties;
};

class UtilsQt_Futures_Utils_AnalyzeFutures : public testing::TestWithParam<DataPack_AnalyzeFutures>
{ };

struct DataPack_FuturesToResults
{
    QVector<QFuture<int>> futures;
    QVector<std::optional<int>> result;
};

class UtilsQt_Futures_Utils_FuturesToResults : public testing::TestWithParam<DataPack_FuturesToResults>
{ };

struct DataPack_FuturesToResultsTuple
{
    std::tuple<QFuture<int>, QFuture<std::string>> futures;
    std::tuple<std::optional<int>, std::optional<std::string>> result;
};

class UtilsQt_Futures_Utils_FuturesToResultsTuple : public testing::TestWithParam<DataPack_FuturesToResultsTuple>
{ };

QFuture<void> createStarted() {
    QFutureInterface<void> fi;
    fi.reportStarted();
    return fi.future();
}

template<typename T>
QFuture<T> createStarted() {
    QFutureInterface<T> fi;
    fi.reportStarted();
    return fi.future();
}

} // namespace

TEST(UtilsQt, Futures_Utils_createFinished)
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

TEST(UtilsQt, Futures_Utils_Timed)
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
        auto f = createTimedFuture(20, false);
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

TEST(UtilsQt, Futures_Utils_Future)
{
    {
        QObject ctx;

        auto f = createPromise<int>();
        ASSERT_FALSE(f.isFinished());

        QTimer::singleShot(20, &ctx, [f]() mutable { f.finish(120); });

        waitForFuture<QEventLoop>(f.future());

        ASSERT_TRUE(f.isFinished());
        ASSERT_TRUE(f.future().isFinished());
        ASSERT_FALSE(f.future().isCanceled());
    }

    {
        QObject ctx;

        auto f = createPromise<int>();
        ASSERT_FALSE(f.isFinished());

        QTimer::singleShot(20, &ctx, [f]() mutable { f.cancel(); });

        waitForFuture<QEventLoop>(f.future());

        ASSERT_TRUE(f.isFinished());
        ASSERT_TRUE(f.future().isFinished());
        ASSERT_TRUE(f.future().isCanceled());
    }
}


TEST(UtilsQt, Futures_Utils_onFinished)
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
        ASSERT_TRUE(ok);
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

TEST(UtilsQt, Futures_Utils_Timed_onFinished)
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
        auto f = createTimedFuture(20, 170);
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
        ASSERT_TRUE(ok);
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


TEST(UtilsQt, Futures_Utils_context)
{
    auto obj = new QObject();
    auto f = createTimedFuture(20, 170, obj);

    qApp->processEvents();
    qApp->processEvents();

    delete obj; obj = nullptr;

    qApp->processEvents();
    qApp->processEvents();

    ASSERT_TRUE(f.isFinished());
    ASSERT_TRUE(f.isCanceled());
}

TEST(UtilsQt, Futures_Utils_reference)
{
    {
        QObject obj;
        int value = 170;
        auto f = createTimedFutureRef(20, value, &obj);
        qApp->processEvents();
        qApp->processEvents();
        value = 210;
        waitForFuture<QEventLoop>(f);
        ASSERT_TRUE(f.isFinished());
        ASSERT_FALSE(f.isCanceled());
        ASSERT_EQ(f.result(), 210);
    }

    {
        auto obj = new QObject();
        int value = 170;
        auto f = createTimedFutureRef(20, value, obj);
        qApp->processEvents();
        qApp->processEvents();
        value = 210;

        delete obj; obj = nullptr;

        qApp->processEvents();
        qApp->processEvents();

        ASSERT_TRUE(f.isFinished());
        ASSERT_TRUE(f.isCanceled());
    }
}

TEST(UtilsQt, Futures_Utils_Lifetime)
{
    LifetimeTracker tracker;
    const auto data = tracker.getData();
    ASSERT_EQ(data.count(), 1);
    ASSERT_EQ(data.count(), 1);

    QObject context;
    auto f = createTimedFuture(10, &context);

    ASSERT_EQ(data.count(), 1);
    onFinished(f, &context, [tracker](){ (void)tracker; });
    ASSERT_EQ(data.count(), 2);

    waitForFuture<QEventLoop>(f);
    qApp->processEvents();
    qApp->processEvents();

    ASSERT_EQ(data.count(), 1);
}

TEST_P(UtilsQt_Futures_Utils_AnalyzeFutures, Test)
{
    const auto dataPack = GetParam();
    const auto result = UtilsQt::analyzeFutures(dataPack.futures);
    ASSERT_EQ(result, dataPack.properties);
}

INSTANTIATE_TEST_SUITE_P(
    Test,
    UtilsQt_Futures_Utils_AnalyzeFutures,
    testing::Values(
            DataPack_AnalyzeFutures{{createStarted(), createStarted(), createStarted()},
                                    {false, false, true, false, false, true, false, false}},
            DataPack_AnalyzeFutures{{createStarted(), createStarted(), UtilsQt::createReadyFuture()},
                                    {false, true, false, false, false, true, false, true}},
            DataPack_AnalyzeFutures{{UtilsQt::createReadyFuture(), UtilsQt::createReadyFuture(), UtilsQt::createReadyFuture()},
                                    {true, true, false, false, false, true, true, true}},

            DataPack_AnalyzeFutures{{createStarted(), UtilsQt::createCanceledFuture<void>()},
                                    {false, true, false, false, true, false, false, false}},
            DataPack_AnalyzeFutures{{UtilsQt::createCanceledFuture<void>(), UtilsQt::createCanceledFuture<void>()},
                                    {true, true, false, true, true, false, false, false}},
            DataPack_AnalyzeFutures{{createReadyFuture(), UtilsQt::createCanceledFuture<void>()},
                                    {true, true, false, false, true, false, false, true}},

            DataPack_AnalyzeFutures{{},
                                    {true, true, false, false, false, true, true, true}}
    )
);

TEST_P(UtilsQt_Futures_Utils_FuturesToResults, Test)
{
    const auto dataPack = GetParam();
    const auto result = UtilsQt::futuresToOptResults(dataPack.futures);
    ASSERT_EQ(result, dataPack.result);
}

INSTANTIATE_TEST_SUITE_P(
    Test,
    UtilsQt_Futures_Utils_FuturesToResults,
    testing::Values(
            DataPack_FuturesToResults{{createReadyFuture(10), createReadyFuture(15)}, {10, 15}},
            DataPack_FuturesToResults{{createStarted<int>(), createReadyFuture(15)}, {{}, 15}},
            DataPack_FuturesToResults{{createCanceledFuture<int>(), createReadyFuture(15)}, {{}, 15}}
    )
);

TEST_P(UtilsQt_Futures_Utils_FuturesToResultsTuple, Test)
{
    const auto dataPack = GetParam();
    const auto result = UtilsQt::futuresToOptResults(dataPack.futures);
    ASSERT_EQ(result, dataPack.result);
}

INSTANTIATE_TEST_SUITE_P(
    Test,
    UtilsQt_Futures_Utils_FuturesToResultsTuple,
    testing::Values(
            DataPack_FuturesToResultsTuple{std::make_tuple(createReadyFuture(10), createReadyFuture<std::string>("abc")),
                                           std::make_tuple(std::optional<int>(10), std::optional<std::string>("abc"))},

            DataPack_FuturesToResultsTuple{std::make_tuple(createStarted<int>(), createReadyFuture<std::string>("abc")),
                                           std::make_tuple(std::optional<int>(), std::optional<std::string>("abc"))},

            DataPack_FuturesToResultsTuple{std::make_tuple(createCanceledFuture<int>(), createReadyFuture<std::string>("abc")),
                                           std::make_tuple(std::optional<int>(), std::optional<std::string>("abc"))}
    )
);
