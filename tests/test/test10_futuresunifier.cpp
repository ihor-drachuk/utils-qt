#include <gtest/gtest.h>

#include <QEventLoop>
#include <utils-qt/futureutils.h>
#include <utils-qt/futuresunifier.h>
#include <string>

TEST(UtilsQt, FuturesUnifier_basic)
{
    { // All
        auto f1 = createTimedFuture(50, true);
        auto f2 = createTimedFuture(100, std::string("Test"));
        auto f3 = createTimedFuture(150, 17);

        auto unifier = createFuturesUnifier(f1, f2, f3);
        auto f = unifier->mergeFuturesAll();

        ASSERT_FALSE(f1.isFinished());
        ASSERT_FALSE(f2.isFinished());
        ASSERT_FALSE(f3.isFinished());
        ASSERT_FALSE(f.isFinished());

        waitForFuture<QEventLoop>(f);

        ASSERT_TRUE(f1.isFinished());
        ASSERT_TRUE(f2.isFinished());
        ASSERT_TRUE(f3.isFinished());
        ASSERT_TRUE(f.isFinished());
        ASSERT_FALSE(f.isCanceled());

        auto result = f.result();

        ASSERT_EQ(std::tuple_size_v<decltype(result)>, 3);
        ASSERT_EQ(std::get<0>(result), true);
        ASSERT_EQ(std::get<1>(result), "Test");
        ASSERT_EQ(std::get<2>(result), 17);
    }

    { // Any
        auto f1 = createTimedFuture(50, true);
        auto f2 = createTimedFuture(100, std::string("Test"));
        auto f3 = createTimedFuture(150, 17);

        auto unifier = createFuturesUnifier(f1, f2, f3);
        auto f = unifier->mergeFuturesAny();

        ASSERT_FALSE(f1.isFinished());
        ASSERT_FALSE(f2.isFinished());
        ASSERT_FALSE(f3.isFinished());
        ASSERT_FALSE(f.isFinished());

        waitForFuture<QEventLoop>(f);

        ASSERT_TRUE(f1.isFinished());
        ASSERT_FALSE(f2.isFinished());
        ASSERT_FALSE(f3.isFinished());
        ASSERT_TRUE(f.isFinished());
        ASSERT_FALSE(f.isCanceled());
    }

    { // All, but 2nd canceled
        auto f1 = createTimedFuture(50, true);
        auto f2 = createTimedCanceledFuture<std::string>(100);
        auto f3 = createTimedFuture(150, 17);

        auto unifier = createFuturesUnifier(f1, f2, f3);
        auto f = unifier->mergeFuturesAll();

        ASSERT_FALSE(f1.isFinished());
        ASSERT_FALSE(f2.isFinished());
        ASSERT_FALSE(f3.isFinished());
        ASSERT_FALSE(f.isFinished());

        waitForFuture<QEventLoop>(f);

        ASSERT_TRUE(f1.isFinished());
        ASSERT_TRUE(f2.isFinished());
        ASSERT_TRUE(f3.isFinished());
        ASSERT_TRUE(f.isFinished());
        ASSERT_FALSE(f.isCanceled());

        auto result = f.result();

        ASSERT_EQ(std::tuple_size_v<decltype(result)>, 3);
        ASSERT_EQ(std::get<0>(result), true);
        ASSERT_EQ(std::get<1>(result), std::nullopt);
        ASSERT_EQ(std::get<2>(result), 17);
    }

    { // Any, but 1st canceled
        auto f1 = createTimedCanceledFuture<bool>(50);
        auto f2 = createTimedFuture(100, std::string("Test"));
        auto f3 = createTimedFuture(150, 17);

        auto unifier = createFuturesUnifier(f1, f2, f3);
        auto f = unifier->mergeFuturesAny();

        ASSERT_FALSE(f1.isFinished());
        ASSERT_FALSE(f2.isFinished());
        ASSERT_FALSE(f3.isFinished());
        ASSERT_FALSE(f.isFinished());

        waitForFuture<QEventLoop>(f);

        ASSERT_TRUE(f1.isFinished());
        ASSERT_FALSE(f2.isFinished());
        ASSERT_FALSE(f3.isFinished());
        ASSERT_TRUE(f.isFinished());
        ASSERT_TRUE(f.isCanceled());
    }
}
