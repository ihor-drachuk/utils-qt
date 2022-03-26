#include <gtest/gtest.h>
#include <string>
#include <QEventLoop>
#include <utils-qt/futureutils_combine.h>
#include <utils-qt/futureutils.h>


TEST(UtilsQt, FutureUtilsCombineTest_basic)
{
    { // All
        auto f1 = createTimedFuture(50, true);
        auto f2 = createTimedFuture(100, std::string("Test"));
        auto f3 = createTimedFuture(150, 17);

        auto f = combineFutures(FutureUtils::CombineTrigger::All, nullptr, 0, f1, f2, f3);

        ASSERT_FALSE(f.isFinished());

        waitForFuture<QEventLoop>(f);

        ASSERT_TRUE(f.isFinished());
        auto result = f.result();
        ASSERT_EQ(result->timeout, false);
        ASSERT_EQ(result->lostContext, false);
        ASSERT_EQ(result->finishedCnt, 3);

        ASSERT_TRUE(result->allFinished());
        ASSERT_TRUE(result->anyFinished());

        ASSERT_TRUE(result->allHasResult());
        ASSERT_TRUE(result->anyHasResult());

        ASSERT_FALSE(result->allCanceled());
        ASSERT_FALSE(result->anyCanceled());
        ASSERT_FALSE(result->aborted());

        for (int i = 0; i < 3; i++) {
            ASSERT_TRUE(result->iFinished[i]);
            ASSERT_TRUE(result->iHasResult[i]);
            ASSERT_TRUE(result->iCanceled[i] == false);
        }

        ASSERT_EQ(std::get<0>(result->sourceFutures).result(), true);
        ASSERT_EQ(std::get<1>(result->sourceFutures).result(), "Test");
        ASSERT_EQ(std::get<2>(result->sourceFutures).result(), 17);
    }

    { // Any
        auto f1 = createTimedFuture(50, true);
        auto f2 = createTimedFuture(100, std::string("Test"));
        auto f3 = createTimedFuture(150, 17);

        auto f = combineFutures(FutureUtils::CombineTrigger::Any, nullptr, 0, f1, f2, f3);

        ASSERT_FALSE(f.isFinished());

        waitForFuture<QEventLoop>(f);

        ASSERT_TRUE(f.isFinished());
        auto result = f.result();
        ASSERT_EQ(result->timeout, false);
        ASSERT_EQ(result->lostContext, false);
        ASSERT_EQ(result->finishedCnt, 1);

        ASSERT_FALSE(result->allFinished());
        ASSERT_TRUE (result->anyFinished());

        ASSERT_FALSE(result->allHasResult());
        ASSERT_TRUE (result->anyHasResult());

        ASSERT_FALSE(result->allCanceled());
        ASSERT_FALSE(result->anyCanceled());
        ASSERT_FALSE(result->aborted());

        for (int i = 0; i < 3; i++)
            ASSERT_FALSE(result->iCanceled[i]);

        ASSERT_TRUE(result->iFinished[0]);
        ASSERT_TRUE(result->iHasResult[0]);

        ASSERT_FALSE(result->iFinished[1]);
        ASSERT_FALSE(result->iHasResult[1]);
        ASSERT_FALSE(result->iFinished[2]);
        ASSERT_FALSE(result->iHasResult[2]);

        ASSERT_EQ(result->getFuture<0>().result(), true);
    }

    { // All, 2nd canceled
        auto f1 = createTimedFuture(50, true);
        auto f2 = createTimedCanceledFuture<std::string>(100);
        auto f3 = createTimedFuture(150, 17);

        auto f = combineFutures(FutureUtils::CombineTrigger::All, nullptr, 0, f1, f2, f3);

        ASSERT_FALSE(f.isFinished());

        waitForFuture<QEventLoop>(f);

        ASSERT_TRUE(f.isFinished());
        auto result = f.result();
        ASSERT_EQ(result->timeout, false);
        ASSERT_EQ(result->lostContext, false);
        ASSERT_EQ(result->finishedCnt, 3);

        ASSERT_TRUE(result->allFinished());
        ASSERT_TRUE(result->anyFinished());

        ASSERT_FALSE(result->allHasResult());
        ASSERT_TRUE(result->anyHasResult());

        ASSERT_FALSE(result->allCanceled());
        ASSERT_TRUE(result->anyCanceled());
        ASSERT_FALSE(result->aborted());
    }

    { // All, ready
        auto f1 = createReadyFuture(true);
        auto f2 = createReadyFuture(std::string("Test"));
        auto f3 = createReadyFuture(17);

        auto f = combineFutures(FutureUtils::CombineTrigger::All, nullptr, 0, f1, f2, f3);

        ASSERT_TRUE(f.isFinished());
        auto result = f.result();
        ASSERT_EQ(result->timeout, false);
        ASSERT_EQ(result->lostContext, false);
        ASSERT_EQ(result->finishedCnt, 3);

        ASSERT_TRUE(result->allFinished());
        ASSERT_TRUE(result->anyFinished());

        ASSERT_TRUE(result->allHasResult());
        ASSERT_TRUE(result->anyHasResult());

        ASSERT_FALSE(result->allCanceled());
        ASSERT_FALSE(result->anyCanceled());
        ASSERT_FALSE(result->aborted());

        for (int i = 0; i < 3; i++) {
            ASSERT_TRUE(result->iFinished[i]);
            ASSERT_TRUE(result->iHasResult[i]);
            ASSERT_TRUE(result->iCanceled[i] == false);
        }

        ASSERT_EQ(std::get<0>(result->sourceFutures).result(), true);
        ASSERT_EQ(std::get<1>(result->sourceFutures).result(), "Test");
        ASSERT_EQ(std::get<2>(result->sourceFutures).result(), 17);
    }

    { // Any, ready
        auto f1 = createTimedFuture(50, true);
        auto f2 = createReadyFuture(std::string("Test"));
        auto f3 = createTimedFuture(150, 17);

        auto f = combineFutures(FutureUtils::CombineTrigger::Any, nullptr, 0, f1, f2, f3);

        ASSERT_TRUE(f.isFinished());
        auto result = f.result();
        ASSERT_EQ(result->timeout, false);
        ASSERT_EQ(result->lostContext, false);
        ASSERT_EQ(result->finishedCnt, 1);

        ASSERT_FALSE(result->allFinished());
        ASSERT_TRUE (result->anyFinished());

        ASSERT_FALSE(result->allHasResult());
        ASSERT_TRUE (result->anyHasResult());

        ASSERT_FALSE(result->allCanceled());
        ASSERT_FALSE(result->anyCanceled());
        ASSERT_FALSE(result->aborted());

        for (int i = 0; i < 3; i++)
            ASSERT_FALSE(result->iCanceled[i]);

        ASSERT_FALSE(result->iFinished[0]);
        ASSERT_FALSE(result->iHasResult[0]);
        ASSERT_TRUE(result->iFinished[1]);
        ASSERT_TRUE(result->iHasResult[1]);
        ASSERT_FALSE(result->iFinished[2]);
        ASSERT_FALSE(result->iHasResult[2]);

        ASSERT_EQ(result->getFuture<1>().result(), "Test");
    }
}

TEST(UtilsQt, FutureUtilsCombineTest_timeout)
{
    {
        auto f1 = createTimedFuture(500, true);
        auto f2 = createTimedFuture(1000, std::string("Test"));
        auto f3 = createTimedFuture(1500, 17);

        auto f = combineFutures(FutureUtils::CombineTrigger::All, nullptr, 100, f1, f2, f3);

        ASSERT_FALSE(f.isFinished());

        waitForFuture<QEventLoop>(f);

        ASSERT_TRUE(f.isFinished());
        ASSERT_FALSE(f.isCanceled());

        ASSERT_TRUE(f.result()->aborted());
        ASSERT_FALSE(f.result()->lostContext);
        ASSERT_TRUE(f.result()->timeout);
    }

    {
        auto f1 = createTimedFuture(500, true);
        auto f2 = createTimedFuture(1000, std::string("Test"));
        auto f3 = createTimedFuture(1500, 17);

        auto f = combineFutures(FutureUtils::CombineTrigger::All, nullptr, 100, f1, f2, f3);

        ASSERT_FALSE(f.isFinished());

        waitForFuture<QEventLoop>(f);

        ASSERT_TRUE(f.isFinished());
        ASSERT_FALSE(f.isCanceled());

        ASSERT_TRUE(f.result()->aborted());
        ASSERT_FALSE(f.result()->lostContext);
        ASSERT_TRUE(f.result()->timeout);
    }
}

TEST(UtilsQt, FutureUtilsCombineTest_context)
{
    {
        auto f1 = createTimedFuture(50, true);
        auto f2 = createTimedFuture(100, std::string("Test"));
        auto f3 = createTimedFuture(150, 17);
        QObject ctx;

        auto f = combineFutures(FutureUtils::CombineTrigger::All, &ctx, 0, f1, f2, f3);

        ASSERT_FALSE(f.isFinished());

        waitForFuture<QEventLoop>(f);

        ASSERT_TRUE(f.isFinished());
        ASSERT_FALSE(f.isCanceled());
    }

    {
        auto f1 = createTimedFuture(50, true);
        auto f2 = createTimedFuture(100, std::string("Test"));
        auto f3 = createTimedFuture(150, 17);
        auto ctx = new QObject();

        auto f = combineFutures(FutureUtils::CombineTrigger::All, ctx, 0, f1, f2, f3);

        ASSERT_FALSE(f.isFinished());

        delete ctx;

        ASSERT_TRUE(f.isFinished());
        ASSERT_TRUE(f.isCanceled());
    }
}
