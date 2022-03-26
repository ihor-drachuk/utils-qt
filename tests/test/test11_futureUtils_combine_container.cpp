#include <gtest/gtest.h>
#include <utils-qt/futureutils.h>
#include <utils-qt/futureutils_combine_container.h>
#include <QEventLoop>

TEST(UtilsQt, FutureUtilsCombineContainer_Basic)
{
    {
        auto f1 = createTimedFuture(50, std::string("Str1"));
        auto f2 = createTimedFuture(100, std::string("Str2"));
        auto f3 = createTimedFuture(150, std::string("Str3"));
        auto f = combineFutures({f1,f2,f3});

        QEventLoop().processEvents();

        ASSERT_TRUE(f.isStarted());
        ASSERT_FALSE(f.isCanceled());
        ASSERT_FALSE(f.isFinished());

        waitForFuture<QEventLoop>(f);

        ASSERT_TRUE(f.isStarted());
        ASSERT_FALSE(f.isCanceled());
        ASSERT_TRUE(f.isFinished());

        ASSERT_EQ(f.results().size(), 3);
        ASSERT_EQ(f.results(), QList<std::string>({"Str1", "Str2", "Str3"}));
    }

    {
        auto f1 = createTimedFuture(50, std::string("Str1"));
        auto f2 = createReadyFuture(std::string("Str2"));
        auto f3 = createTimedFuture(150, std::string("Str3"));
        auto f = combineFutures({f1,f2,f3});

        QEventLoop().processEvents();

        ASSERT_TRUE(f.isStarted());
        ASSERT_FALSE(f.isCanceled());
        ASSERT_FALSE(f.isFinished());

        waitForFuture<QEventLoop>(f);

        ASSERT_TRUE(f.isStarted());
        ASSERT_FALSE(f.isCanceled());
        ASSERT_TRUE(f.isFinished());

        ASSERT_EQ(f.results().size(), 3);
        ASSERT_EQ(f.results(), QList<std::string>({"Str1", "Str2", "Str3"}));
    }
}

TEST(UtilsQt, FutureUtilsCombineContainer_Cancelled)
{
    {
        auto f1 = createTimedFuture(50, std::string("Str1"));
        auto f2 = createTimedCanceledFuture<std::string>(100);
        auto f3 = createTimedFuture(150, std::string("Str3"));
        auto f = combineFutures({f1,f2,f3});

        QEventLoop().processEvents();

        ASSERT_TRUE(f.isStarted());
        ASSERT_FALSE(f.isCanceled());
        ASSERT_FALSE(f.isFinished());

        waitForFuture<QEventLoop>(f);

        ASSERT_TRUE(f.isStarted());
        ASSERT_FALSE(f.isCanceled());
        ASSERT_TRUE(f.isFinished());

        ASSERT_EQ(f.results().size(), 2);
        ASSERT_FALSE(f.isResultReadyAt(1));
        ASSERT_EQ(f.results(), QList<std::string>({"Str1", "Str3"}));
    }

    {
        auto f1 = createTimedFuture(50, std::string("Str1"));
        auto f2 = createCanceledFuture<std::string>();
        auto f3 = createTimedFuture(150, std::string("Str3"));
        auto f = combineFutures({f1,f2,f3});

        QEventLoop().processEvents();

        ASSERT_TRUE(f.isStarted());
        ASSERT_FALSE(f.isCanceled());
        ASSERT_FALSE(f.isFinished());

        waitForFuture<QEventLoop>(f);

        ASSERT_TRUE(f.isStarted());
        ASSERT_FALSE(f.isCanceled());
        ASSERT_TRUE(f.isFinished());

        ASSERT_EQ(f.results().size(), 2);
        ASSERT_FALSE(f.isResultReadyAt(1));
        ASSERT_EQ(f.results(), QList<std::string>({"Str1", "Str3"}));
    }
}

TEST(UtilsQt, FutureUtilsCombineContainer_BasicReady)
{
    {
        auto f1 = createReadyFuture(std::string("Str1"));
        auto f2 = createCanceledFuture<std::string>();
        auto f3 = createReadyFuture(std::string("Str3"));
        auto f = combineFutures({f1,f2,f3});

        QEventLoop().processEvents();

        ASSERT_TRUE(f.isStarted());
        ASSERT_FALSE(f.isCanceled());
        ASSERT_TRUE(f.isFinished());

        ASSERT_EQ(f.results().size(), 2);
        ASSERT_FALSE(f.isResultReadyAt(1));
        ASSERT_EQ(f.results(), QList<std::string>({"Str1", "Str3"}));
    }
}

TEST(UtilsQt, FutureUtilsCombineContainer_StepByStep)
{
    QFutureInterface<int> fi;
    auto f = combineFutures({fi.future()});

    QEventLoop().processEvents();

    ASSERT_FALSE(f.isStarted());
    ASSERT_FALSE(f.isCanceled());
    ASSERT_FALSE(f.isFinished());

    fi.reportStarted();
    QEventLoop().processEvents();

    ASSERT_TRUE(f.isStarted());
    ASSERT_FALSE(f.isCanceled());
    ASSERT_FALSE(f.isFinished());

    fi.reportResult(123);
    QEventLoop().processEvents();

    ASSERT_TRUE(f.isStarted());
    ASSERT_FALSE(f.isCanceled());
    ASSERT_FALSE(f.isFinished());
    ASSERT_EQ(f.resultCount(), 0);

    fi.reportFinished();
    QEventLoop().processEvents();

    ASSERT_TRUE(f.isStarted());
    ASSERT_FALSE(f.isCanceled());
    ASSERT_TRUE(f.isFinished());
    ASSERT_EQ(f.resultCount(), 1);
    ASSERT_EQ(f.results().size(), 1);
    ASSERT_EQ(f.results().first(), 123);
}

TEST(UtilsQt, FutureUtilsCombineContainer_Context)
{
    QFutureInterface<int> fi1, fi2;
    QFuture<int> f;

    {
        QObject ctx;
        f = combineFutures({fi1.future(), fi2.future()}, &ctx);

        QEventLoop().processEvents();

        ASSERT_FALSE(f.isCanceled());
        ASSERT_FALSE(f.isFinished());
    }

    waitForFuture<QEventLoop>(f);

    ASSERT_TRUE(f.isCanceled());
    ASSERT_TRUE(f.isFinished());
}

TEST(UtilsQt, FutureUtilsCombineContainer_CancelTarget)
{
    QFutureInterface<int> fi1, fi2;
    fi1.reportStarted();

    auto f = combineFutures({fi1.future(), fi2.future()});

    QEventLoop().processEvents();

    f.cancel();
    QEventLoop().processEvents();

    ASSERT_TRUE(fi1.isCanceled());
    ASSERT_TRUE(fi2.isCanceled());

    fi1.reportResult(1);
    fi1.reportFinished();

    fi2.reportFinished();

    QEventLoop().processEvents();

    ASSERT_TRUE(f.isStarted());
    ASSERT_TRUE(f.isCanceled());
    ASSERT_TRUE(f.isFinished());
    ASSERT_EQ(f.resultCount(), 0);
    ASSERT_EQ(f.results().size(), 0);
}
