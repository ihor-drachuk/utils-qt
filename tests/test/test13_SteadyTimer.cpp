/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <gtest/gtest.h>
#include <chrono>
#include <QEventLoop>
#include <UtilsQt/Qml-Cpp/SteadyTimer.h>
#include <UtilsQt/Futures/Utils.h>
#include <UtilsQt/Futures/SignalToFuture.h>

#include "internal/TestWaitHelpers.h"

using namespace std::chrono_literals;

namespace {

struct TimerTestHelper {
    SteadyTimer timer;
    std::chrono::steady_clock::time_point startTime;
    int triggerCount {};
    QMetaObject::Connection conn;

    TimerTestHelper() {
        conn = QObject::connect(&timer, &SteadyTimer::timeout, [this]() {
            triggerCount++;
        });
    }

    ~TimerTestHelper() {
        QObject::disconnect(conn);
    }

    void start(int interval, bool repeat = false) {
        startTime = std::chrono::steady_clock::now();
        timer.start(interval, repeat);
    }

    std::chrono::milliseconds elapsed() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime);
    }

    bool waitForTimeout(int timeoutMs = TestHelpers::SafetyTimeoutMs) {
        auto f = UtilsQt::signalToFuture(&timer, &SteadyTimer::timeout,
                                          nullptr, std::chrono::milliseconds(timeoutMs));
        UtilsQt::waitForFuture<QEventLoop>(f);
        return !f.isCanceled();
    }

    bool waitForTriggers(int count, int timeoutMs = TestHelpers::SafetyTimeoutMs) {
        return TestHelpers::waitUntil([this, count]() {
            return triggerCount >= count;
        }, timeoutMs);
    }
};

} // namespace

TEST(UtilsQt, SteadyTimer_Basic)
{
    TimerTestHelper helper;
    ASSERT_GT(helper.timer.interval(), 0);
    ASSERT_GT(helper.timer.resolution(), 0);
    ASSERT_FALSE(helper.timer.active());

    helper.timer.setResolution(100);
    helper.start(300);
    ASSERT_TRUE(helper.timer.active());
    ASSERT_EQ(helper.triggerCount, 0);

    ASSERT_TRUE(helper.waitForTimeout());

    auto elapsedMs = helper.elapsed();
    ASSERT_GE(elapsedMs, 250ms) << "Timer triggered too early";
    ASSERT_LE(elapsedMs, 2000ms) << "Timer took too long";

    ASSERT_EQ(helper.triggerCount, 1);
    ASSERT_FALSE(helper.timer.active());
}

TEST(UtilsQt, SteadyTimer_LowAutoResolution)
{
    TimerTestHelper helper;
    helper.start(100);

    ASSERT_EQ(helper.triggerCount, 0);

    ASSERT_TRUE(helper.waitForTimeout());

    auto elapsedMs = helper.elapsed();
    ASSERT_GE(elapsedMs, 70ms) << "Timer triggered too early";
    ASSERT_LE(elapsedMs, 1000ms) << "Timer took too long";

    ASSERT_EQ(helper.triggerCount, 1);
    ASSERT_FALSE(helper.timer.active());
}

TEST(UtilsQt, SteadyTimer_Threshold)
{
    TimerTestHelper helper;
    helper.timer.setResolution(100);
    helper.timer.setThresholdFactor(20); // means resolution x20 (+- 2 sec)
    helper.start(1000);

    ASSERT_EQ(helper.triggerCount, 0);

    ASSERT_TRUE(helper.waitForTimeout());

    auto elapsedMs = helper.elapsed();
    ASSERT_LE(elapsedMs, 2000ms) << "Timer took too long even with threshold";

    ASSERT_EQ(helper.triggerCount, 1);
    ASSERT_FALSE(helper.timer.active());
}

TEST(UtilsQt, SteadyTimer_StartStop)
{
    TimerTestHelper helper;
    helper.start(200);

    // Wait a bit, but not long enough for timer (150ms margin)
    TestHelpers::waitMs(50);
    ASSERT_EQ(helper.triggerCount, 0);
    ASSERT_TRUE(helper.timer.active());

    helper.timer.stop();
    ASSERT_EQ(helper.triggerCount, 0);
    ASSERT_FALSE(helper.timer.active());

    // Wait well past when timer would have fired — should NOT fire since stopped
    TestHelpers::waitMs(300);
    ASSERT_EQ(helper.triggerCount, 0);
}

TEST(UtilsQt, SteadyTimer_Repeat)
{
    TimerTestHelper helper;
    helper.start(100, true);

    ASSERT_TRUE(helper.waitForTriggers(3));
    ASSERT_GE(helper.triggerCount, 3);

    helper.timer.stop();
    int countAtStop = helper.triggerCount;

    TestHelpers::waitMs(250);
    ASSERT_EQ(helper.triggerCount, countAtStop) << "Timer triggered after stop";
}

TEST(UtilsQt, SteadyTimer_ZeroInterval)
{
    TimerTestHelper helper;
    auto f = UtilsQt::signalToFuture(&helper.timer, &SteadyTimer::timeout,
                                      nullptr, std::chrono::milliseconds(TestHelpers::SafetyTimeoutMs));
    helper.timer.start(0);

    ASSERT_FALSE(helper.timer.active());  // Should immediately become inactive

    UtilsQt::waitForFuture<QEventLoop>(f);
    ASSERT_EQ(helper.triggerCount, 1);
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
    TimerTestHelper helper;
    helper.start(500);

    ASSERT_EQ(helper.triggerCount, 0);
    ASSERT_TRUE(helper.waitForTimeout());

    auto elapsedMs = helper.elapsed();
    ASSERT_GE(elapsedMs, 400ms) << "Timer triggered too early";
    ASSERT_LE(elapsedMs, 2000ms) << "Timer took too long";

    ASSERT_EQ(helper.triggerCount, 1);
    ASSERT_FALSE(helper.timer.active());
}
