#include <gtest/gtest.h>
#include <UtilsQt/Futures/Utils.h>
#include <QCoreApplication>
#include <QEventLoop>
#include <QTimer>
#include <QVector>

using namespace UtilsQt;

namespace {

struct DataPack_FuturesSetProperties
{
    QVector<QFuture<void>> futures;
    UtilsQt::FuturesSetProperties properties;
};

class UtilsQt_Futures_Regex : public testing::TestWithParam<DataPack_FuturesSetProperties>
{ };

QFuture<void> createStarted() {
    QFutureInterface<void> fi;
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

TEST_P(UtilsQt_Futures_Regex, Test)
{
    auto dataPack = GetParam();
    auto result = UtilsQt::analyzeFuturesSet(dataPack.futures);
    ASSERT_EQ(result, dataPack.properties);
}

INSTANTIATE_TEST_SUITE_P(
    Test,
    UtilsQt_Futures_Regex,
    testing::Values(
            DataPack_FuturesSetProperties{{createStarted(), createStarted(), createStarted()},
                                          {false, false, true, false, false, true, false, false}},
            DataPack_FuturesSetProperties{{createStarted(), createStarted(), UtilsQt::createReadyFuture()},
                                          {false, true, false, false, false, true, false, true}},
            DataPack_FuturesSetProperties{{UtilsQt::createReadyFuture(), UtilsQt::createReadyFuture(), UtilsQt::createReadyFuture()},
                                          {true, true, false, false, false, true, true, true}},

            DataPack_FuturesSetProperties{{createStarted(), UtilsQt::createCanceledFuture<void>()},
                                          {false, true, false, false, true, false, false, false}},
            DataPack_FuturesSetProperties{{UtilsQt::createCanceledFuture<void>(), UtilsQt::createCanceledFuture<void>()},
                                          {true, true, false, true, true, false, false, false}},
            DataPack_FuturesSetProperties{{createReadyFuture(), UtilsQt::createCanceledFuture<void>()},
                                          {true, true, false, false, true, false, false, true}},

            DataPack_FuturesSetProperties{{},
                                          {true, true, false, false, false, true, true, true}}
    )
);
