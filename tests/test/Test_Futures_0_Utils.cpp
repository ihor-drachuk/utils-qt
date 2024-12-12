/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <gtest/gtest.h>

#include <QCoreApplication>
#include <QEventLoop>
#include <QTimer>
#include <QVector>

#include <UtilsQt/Futures/Utils.h>

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

    {
        auto f = createExceptionFuture<int>(std::make_exception_ptr(std::runtime_error("Test")));
        ASSERT_TRUE(f.isCanceled());
        ASSERT_TRUE(f.isFinished());
        ASSERT_THROW(f.waitForFinished(), std::runtime_error);
    }

    {
        auto f = createExceptionFuture<int>(std::runtime_error("Test"));
        ASSERT_TRUE(f.isCanceled());
        ASSERT_TRUE(f.isFinished());
        ASSERT_THROW(f.waitForFinished(), std::runtime_error);
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

    {
        auto f = createTimedExceptionFuture<int>(20, std::make_exception_ptr(std::runtime_error("Test")));
        ASSERT_FALSE(f.isFinished());
        ASSERT_FALSE(f.isCanceled());
        waitForFuture<QEventLoop>(f);
        ASSERT_TRUE(f.isCanceled());
        ASSERT_TRUE(f.isFinished());
        ASSERT_THROW(f.waitForFinished(), std::runtime_error);
    }

    {
        auto f = createTimedExceptionFuture<int>(20, std::runtime_error("Test"));
        ASSERT_FALSE(f.isFinished());
        ASSERT_FALSE(f.isCanceled());
        waitForFuture<QEventLoop>(f);
        ASSERT_TRUE(f.isCanceled());
        ASSERT_TRUE(f.isFinished());
        ASSERT_THROW(f.waitForFinished(), std::runtime_error);
    }
}

TEST(UtilsQt, Futures_Utils_Future)
{
    {
        QObject ctx;

        auto f = createPromise<int>(true);
        ASSERT_FALSE(f.isFinished());

        QTimer::singleShot(20, &ctx, [f]() mutable { f.finish(120); });

        waitForFuture<QEventLoop>(f.future());

        ASSERT_TRUE(f.isFinished());
        ASSERT_TRUE(f.future().isFinished());
        ASSERT_FALSE(f.future().isCanceled());
    }

    {
        QObject ctx;

        auto f = createPromise<int>(true);
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
        std::optional<bool> checkpoint1;
        std::optional<bool> checkpoint2;
        std::optional<bool> checkpoint3;
        auto f = createReadyFuture();
        onFinished(f, &ctx, [&checkpoint1](const std::optional<std::monostate>& optResult){ checkpoint1 = optResult.has_value(); });
        onResult(f, &ctx, [&checkpoint2](){ checkpoint2 = true; });
        onCanceled(f, &ctx, [&checkpoint3](){ checkpoint3 = true; });
        EXPECT_EQ(checkpoint1, std::optional(true));
        EXPECT_EQ(checkpoint2, std::optional(true));
        EXPECT_EQ(checkpoint3, std::optional<bool>());
    }

    {
        QObject ctx;
        std::optional<int> result;
        int result2 = -1;
        std::optional<bool> checkpoint3;
        auto f = createReadyFuture(170);
        onFinished(f, &ctx, [&result](const std::optional<int>& value){ result = value; });
        onResult(f, &ctx, [&result2](int value){ result2 = value; });
        onCanceled(f, &ctx, [&checkpoint3](){ checkpoint3 = true; });
        ASSERT_TRUE(result.has_value());
        ASSERT_EQ(result.value(), 170);
        ASSERT_EQ(result2, 170);
        EXPECT_EQ(checkpoint3, std::optional<bool>());
    }

    {
        QObject ctx;
        std::optional<bool> checkpoint1;
        std::optional<bool> checkpoint2;
        std::optional<bool> checkpoint3;
        auto f = createCanceledFuture<void>();
        onFinished(f, &ctx, [&checkpoint1](const std::optional<std::monostate>& optResult){ checkpoint1 = optResult.has_value(); });
        onResult(f, &ctx, [&checkpoint2](){ checkpoint2 = true; });
        onCanceled(f, &ctx, [&checkpoint3](){ checkpoint3 = true; });
        EXPECT_EQ(checkpoint1, std::optional(false));
        EXPECT_EQ(checkpoint2, std::optional<bool>());
        EXPECT_EQ(checkpoint3, std::optional(true));
    }

    {
        QObject ctx;
        std::optional<int> result;
        int result2 = -1;
        std::optional<bool> checkpoint3;
        auto f = createCanceledFuture<int>();
        onFinished(f, &ctx, [&result](const std::optional<int>& value){ result = value; });
        onResult(f, &ctx, [&result2](int value){ result2 = value; });
        onCanceled(f, &ctx, [&checkpoint3](){ checkpoint3 = true; });
        ASSERT_FALSE(result.has_value());
        ASSERT_EQ(result2, -1);
        EXPECT_EQ(checkpoint3, std::optional(true));
    }
}

TEST(UtilsQt, Futures_Utils_Timed_onFinished)
{
    {
        QObject ctx;
        std::optional<bool> checkpoint1;
        std::optional<bool> checkpoint2;
        std::optional<bool> checkpoint3;
        auto f = createTimedFuture(20);
        onFinished(f, &ctx, [&checkpoint1](const std::optional<std::monostate>& optResult){ checkpoint1 = optResult.has_value(); });
        onResult(f, &ctx, [&checkpoint2](){ checkpoint2 = true; });
        onCanceled(f, &ctx, [&checkpoint3](){ checkpoint3 = true; });
        waitForFuture<QEventLoop>(f);
        EXPECT_EQ(checkpoint1, std::optional(true));
        EXPECT_EQ(checkpoint2, std::optional(true));
        EXPECT_EQ(checkpoint3, std::optional<bool>());
    }

    {
        QObject ctx;
        std::optional<int> result;
        int result2 = -1;
        std::optional<bool> checkpoint3;
        auto f = createTimedFuture(20, 170);
        onFinished(f, &ctx, [&result](const std::optional<int>& value){ result = value; });
        onResult(f, &ctx, [&result2](int value){ result2 = value; });
        onCanceled(f, &ctx, [&checkpoint3](){ checkpoint3 = true; });
        waitForFuture<QEventLoop>(f);
        ASSERT_TRUE(result.has_value());
        ASSERT_EQ(result.value(), 170);
        ASSERT_EQ(result2, 170);
        EXPECT_EQ(checkpoint3, std::optional<bool>());
    }

    {
        QObject ctx;
        std::optional<bool> checkpoint1;
        std::optional<bool> checkpoint2;
        std::optional<bool> checkpoint3;
        auto f = createTimedCanceledFuture<void>(20);
        onFinished(f, &ctx, [&checkpoint1](const std::optional<std::monostate>& optResult){ checkpoint1 = optResult.has_value(); });
        onResult(f, &ctx, [&checkpoint2](){ checkpoint2 = true; });
        onCanceled(f, &ctx, [&checkpoint3](){ checkpoint3 = true; });
        waitForFuture<QEventLoop>(f);
        EXPECT_EQ(checkpoint1, std::optional(false));
        EXPECT_EQ(checkpoint2, std::optional<bool>());
        EXPECT_EQ(checkpoint3, std::optional(true));
    }

    {
        QObject ctx;
        std::optional<int> result;
        int result2 = -1;
        std::optional<bool> checkpoint3;
        auto f = createTimedCanceledFuture<int>(20);
        onFinished(f, &ctx, [&result](const std::optional<int>& value){ result = value; });
        onResult(f, &ctx, [&result2](int value){ result2 = value; });
        onCanceled(f, &ctx, [&checkpoint3](){ checkpoint3 = true; });
        waitForFuture<QEventLoop>(f);
        ASSERT_FALSE(result.has_value());
        ASSERT_EQ(result2, -1);
        EXPECT_EQ(checkpoint3, std::optional(true));
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
    onFinished(f, &context, [tracker](const auto&){ (void)tracker; });
    ASSERT_EQ(data.count(), 2);

    waitForFuture<QEventLoop>(f);
    qApp->processEvents();
    qApp->processEvents();

    ASSERT_EQ(data.count(), 1);
}

TEST(UtilsQt, Futures_Utils_getFutureState)
{
    {
        QFutureInterface<int> fi;
        auto f = fi.future();
        ASSERT_EQ(UtilsQt::getFutureState(f), UtilsQt::FutureState::NotStarted);

        fi.reportStarted();
        ASSERT_EQ(UtilsQt::getFutureState(f), UtilsQt::FutureState::Running);

        fi.reportResult(10);
        ASSERT_EQ(UtilsQt::getFutureState(f), UtilsQt::FutureState::Running);

        fi.reportFinished();
        ASSERT_EQ(UtilsQt::getFutureState(f), UtilsQt::FutureState::Completed);

        fi.reportCanceled();
        ASSERT_EQ(UtilsQt::getFutureState(f), UtilsQt::FutureState::Canceled);
    }

    {
        QFutureInterface<int> fi;
        auto f = fi.future();
        fi.reportStarted();
        fi.reportCanceled();
        ASSERT_EQ(UtilsQt::getFutureState(f), UtilsQt::FutureState::Running);

        fi.reportFinished();
        ASSERT_EQ(UtilsQt::getFutureState(f), UtilsQt::FutureState::Canceled);
    }

    {
        QFutureInterface<int> fi;
        auto f = fi.future();
        fi.reportStarted();
        ASSERT_EQ(UtilsQt::getFutureState(f), UtilsQt::FutureState::Running);

        fi.reportFinished();
        ASSERT_EQ(UtilsQt::getFutureState(f), UtilsQt::FutureState::CompletedWrong);
    }

    {
        QFutureInterface<int> fi;
        auto f = fi.future();
        fi.reportStarted();
        fi.reportException(QExceptionPtr(std::make_exception_ptr(std::runtime_error("Test"))));
        ASSERT_EQ(UtilsQt::getFutureState(f), UtilsQt::FutureState::Running);

        fi.reportFinished();
        ASSERT_EQ(UtilsQt::getFutureState(f), UtilsQt::FutureState::Exception);
    }
}

TEST(UtilsQt, Futures_Utils_hasResult)
{
    {
        QFutureInterface<int> fi;
        auto f = fi.future();
        ASSERT_FALSE(UtilsQt::hasResult(f));

        fi.reportStarted();
        ASSERT_FALSE(UtilsQt::hasResult(f));

        fi.reportResult(10);
        ASSERT_FALSE(UtilsQt::hasResult(f));

        fi.reportFinished();
        ASSERT_TRUE(UtilsQt::hasResult(f));

        fi.reportCanceled();
        ASSERT_TRUE(UtilsQt::hasResult(f));
    }

    {
        QFutureInterface<int> fi;
        auto f = fi.future();
        fi.reportStarted();
        ASSERT_FALSE(UtilsQt::hasResult(f));

        fi.reportFinished();
        ASSERT_FALSE(UtilsQt::hasResult(f));
    }

    {
        QFutureInterface<int> fi;
        auto f = fi.future();
        fi.reportStarted();
        fi.reportCanceled();
        ASSERT_FALSE(UtilsQt::hasResult(f));

        fi.reportFinished();
        ASSERT_FALSE(UtilsQt::hasResult(f));
    }

    {
        QFutureInterface<int> fi;
        auto f = fi.future();
        fi.reportStarted();
        fi.reportException(QExceptionPtr(std::make_exception_ptr(std::runtime_error("Test"))));
        ASSERT_FALSE(UtilsQt::hasResult(f));

        fi.reportFinished();
        ASSERT_FALSE(UtilsQt::hasResult(f));
    }
}

TEST(UtilsQt, Futures_Utils_Promise)
{
    {
        Promise<int> p;
        auto f = p.future();
        ASSERT_EQ(UtilsQt::getFutureState(f), UtilsQt::FutureState::NotStarted);

        p.start();
        ASSERT_EQ(UtilsQt::getFutureState(f), UtilsQt::FutureState::Running);

        p.finish(10);
        ASSERT_EQ(UtilsQt::getFutureState(f), UtilsQt::FutureState::Completed);
    }

    {
        Promise<int> p;
        auto f = p.future();
        p.cancel();
        ASSERT_EQ(UtilsQt::getFutureState(f), UtilsQt::FutureState::Canceled);
    }

    {
        Promise<int> p;
        auto f = p.future();
        p.start();
        p.finishWithException(std::runtime_error("Test"));
        ASSERT_EQ(UtilsQt::getFutureState(f), UtilsQt::FutureState::Exception);
        ASSERT_THROW(f.waitForFinished(), std::runtime_error);
    }
}

TEST(UtilsQt, Futures_Utils_onCancelNotified)
{
    QObject obj;
    int calls {};
    auto f = UtilsQt::createTimedCanceledFuture<void>(10);

    UtilsQt::onCancelNotified(f, &obj, [&]() {
        calls++;
    });

    waitForFuture<QEventLoop>(f);
    ASSERT_EQ(calls, 1);

    UtilsQt::onCancelNotified(createCanceledFuture<int>(), &obj, [&]() {
        calls++;
    });

    waitForFuture<QEventLoop>(createTimedFuture(10, 100));

    qApp->processEvents();
    qApp->processEvents();
    ASSERT_EQ(calls, 2);

    UtilsQt::onCancelNotified(UtilsQt::createReadyFuture(), &obj, [&]() {
        calls++;
    });

    f = UtilsQt::createTimedFuture<int>(10, 150);
    UtilsQt::onCancelNotified(f, &obj, [&]() {
        calls++;
    });

    waitForFuture<QEventLoop>(f);
    ASSERT_EQ(calls, 2);

    f = UtilsQt::createTimedCanceledFuture<int>(10);
    auto obj2 = std::make_unique<QObject>();
    UtilsQt::onCancelNotified(f, obj2.get(), [&]() {
        calls++;
    });
    obj2.reset();

    waitForFuture<QEventLoop>(f);
    ASSERT_EQ(calls, 2);

    QFutureInterface<int> fi;
    fi.reportCanceled();

    UtilsQt::onCancelNotified(fi.future(), &obj, [&]() {
        calls++;
    });

    qApp->processEvents();
    qApp->processEvents();
    ASSERT_EQ(calls, 3);
}

TEST(UtilsQt, Futures_Utils_onFinishedNP)
{
    QObject obj;
    int calls {};

    UtilsQt::onFinishedNP(UtilsQt::createCanceledFuture<int>(), &obj, [&]() mutable {
        calls++;
    });

    UtilsQt::onFinishedNP(UtilsQt::createReadyFuture(QString("123")), &obj, [&]() mutable {
        calls++;
    });

    UtilsQt::onFinishedNP(UtilsQt::createExceptionFuture<QString>(std::runtime_error("Test")), &obj, [&]() mutable {
        calls++;
    });

    qApp->processEvents();
    qApp->processEvents();
    ASSERT_EQ(calls, 3);
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
