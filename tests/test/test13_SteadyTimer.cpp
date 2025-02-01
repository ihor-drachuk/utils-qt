/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <gtest/gtest.h>
#include <QEventLoop>
#include <UtilsQt/Qml-Cpp/SteadyTimer.h>
#include <UtilsQt/Futures/Utils.h>
#include <UtilsQt/Futures/SignalToFuture.h>

namespace {

#ifdef UTILS_QT_LONG_INTERVALS
constexpr auto TimeFactor = 7;
#else
constexpr auto TimeFactor = 1;
#endif // UTILS_QT_LONG_INTERVALS

} // namespace

TEST(UtilsQt, SteadyTimer_Basic)
{
    SteadyTimer timer;
    ASSERT_GT(timer.interval(), 0);
    ASSERT_GT(timer.resolution(), 0);
    ASSERT_FALSE(timer.active());

    bool set = false;
    QObject::connect(&timer, &SteadyTimer::timeout, [&]{ set = true; });
    timer.setResolution(100);
    timer.start(300 * TimeFactor);
    ASSERT_TRUE(timer.active());
    ASSERT_FALSE(set);
    UtilsQt::waitForFuture<QEventLoop>(UtilsQt::createTimedFuture(250 * TimeFactor));
    ASSERT_FALSE(set);
    UtilsQt::waitForFuture<QEventLoop>(UtilsQt::createTimedFuture((50 +50) * TimeFactor));
    ASSERT_TRUE(set);

    ASSERT_FALSE(timer.active());
}

TEST(UtilsQt, SteadyTimer_LowAutoResolution)
{
    bool set = false;

    SteadyTimer timer;
    QObject::connect(&timer, &SteadyTimer::timeout, [&]{ set = true; });
    timer.start(100 * TimeFactor);

    ASSERT_FALSE(set);
    UtilsQt::waitForFuture<QEventLoop>(UtilsQt::createTimedFuture(70 * TimeFactor));
    ASSERT_FALSE(set);

    UtilsQt::waitForFuture<QEventLoop>(UtilsQt::createTimedFuture((30 +30) * TimeFactor));
    ASSERT_TRUE(set);

    ASSERT_FALSE(timer.active());
}

TEST(UtilsQt, SteadyTimer_Threshold)
{
    bool set = false;

    SteadyTimer timer;
    QObject::connect(&timer, &SteadyTimer::timeout, [&]{ set = true; });
    timer.setResolution(100);
    timer.setThresholdFactor(20 * TimeFactor); // means resolution x20 (+- 2 sec)
    timer.start(1000 * TimeFactor);

    ASSERT_FALSE(set);
    UtilsQt::waitForFuture<QEventLoop>(UtilsQt::createTimedFuture(150 * TimeFactor));
    ASSERT_TRUE(set);

    ASSERT_FALSE(timer.active());
}

TEST(UtilsQt, SteadyTimer_StartStop)
{
    bool set = false;

    SteadyTimer timer;
    QObject::connect(&timer, &SteadyTimer::timeout, [&]{ set = true; });
    timer.start(200 * TimeFactor);

    UtilsQt::waitForFuture<QEventLoop>(UtilsQt::createTimedFuture(170 * TimeFactor));
    ASSERT_FALSE(set);
    timer.stop();
    ASSERT_FALSE(set);

    UtilsQt::waitForFuture<QEventLoop>(UtilsQt::createTimedFuture(100 * TimeFactor));
    ASSERT_FALSE(set);
}

TEST(UtilsQt, SteadyTimer_Repeat)
{
    int counter = 0;
    SteadyTimer timer;
    QObject::connect(&timer, &SteadyTimer::timeout, [&]{ counter++; });
    timer.start(100 * TimeFactor, true);

    UtilsQt::waitForFuture<QEventLoop>(UtilsQt::createTimedFuture(350 * TimeFactor));
    ASSERT_EQ(counter, 3);
    timer.stop();

    UtilsQt::waitForFuture<QEventLoop>(UtilsQt::createTimedFuture(200 * TimeFactor));
    ASSERT_EQ(counter, 3);
}

TEST(UtilsQt, SteadyTimer_ZeroInterval)
{
    bool set = false;

    SteadyTimer timer;
    QObject::connect(&timer, &SteadyTimer::timeout, [&]{ set = true; });
    auto f = UtilsQt::signalToFuture(&timer, &SteadyTimer::timeout, nullptr, 200);
    timer.start(0);

    ASSERT_FALSE(timer.active());  // Should immediately become inactive

    UtilsQt::waitForFuture<QEventLoop>(f);
    ASSERT_TRUE(set);
}

TEST(UtilsQt, SteadyTimer_AutoResolutionIncrease)
{
    SteadyTimer timer;

    // First set a small interval to force resolution down
    timer.start(50);
    ASSERT_EQ(timer.resolution(), 50);
    timer.stop();

    // Test increase to 150ms
    timer.start(150);
    ASSERT_EQ(timer.resolution(), 150);  // Should increase to match new interval
    timer.stop();

    // Test increase to 250ms (default max)
    timer.start(500);
    ASSERT_EQ(timer.resolution(), 250);  // Should increase to default max
    timer.stop();

    // Verify timer still works with new resolution
    bool set = false;
    QObject::connect(&timer, &SteadyTimer::timeout, [&]{ set = true; });
    timer.start(500 * TimeFactor);

    ASSERT_FALSE(set);
    UtilsQt::waitForFuture<QEventLoop>(UtilsQt::createTimedFuture(450 * TimeFactor));
    ASSERT_FALSE(set);
    UtilsQt::waitForFuture<QEventLoop>(UtilsQt::createTimedFuture(100 * TimeFactor));
    ASSERT_TRUE(set);

    ASSERT_FALSE(timer.active());
}
