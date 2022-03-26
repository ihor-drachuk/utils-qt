#include <gtest/gtest.h>

#include <QEventLoop>
#include <utils-qt/futureutils.h>
#include <utils-qt/futuresunifier.h>
#include <string>

#ifdef __APPLE__
constexpr int time1 = 50;
constexpr int time2 = 350;
constexpr int time3 = 900;
#else
constexpr int time1 = 50;
constexpr int time2 = 100;
constexpr int time3 = 150;
#endif

TEST(UtilsQt, FuturesUnifier_basic)
{
    { // All. Timed
        auto f1 = createTimedFuture(time1, true);
        auto f2 = createTimedFuture(time2, std::string("Test"));
        auto f3 = createTimedFuture(time3, 17);

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

    { // Any. Timed
        auto f1 = createTimedFuture(time1, true);
        auto f2 = createTimedFuture(time2, std::string("Test"));
        auto f3 = createTimedFuture(time3, 17);

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

    { // All. Ready, timed canceled, timed
        auto f1 = createReadyFuture(true);
        auto f2 = createTimedCanceledFuture<std::string>(time2);
        auto f3 = createTimedFuture(time3, 17);

        auto unifier = createFuturesUnifier(f1, f2, f3);
        auto f = unifier->mergeFuturesAll();

        ASSERT_TRUE(f1.isFinished());
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

    { // Any. Timed canceled, timed
        auto f1 = createTimedCanceledFuture<bool>(time1);
        auto f2 = createTimedFuture(time2, std::string("Test"));
        auto f3 = createTimedFuture(time3, 17);

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

    { // All. Canceled, timed canceled, timed
        auto f1 = createCanceledFuture<bool>();
        auto f2 = createTimedCanceledFuture<int>(time2);
        auto f3 = createTimedFuture(time3, 17);

        auto unifier = createFuturesUnifier(f1, f2, f3);
        auto f = unifier->mergeFuturesAll();

        ASSERT_TRUE(f1.isFinished());
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
        ASSERT_EQ(std::get<0>(result), std::nullopt);
        ASSERT_EQ(std::get<1>(result), std::nullopt);
        ASSERT_EQ(std::get<2>(result), 17);
    }
}
