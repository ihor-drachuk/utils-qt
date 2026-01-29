/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <gtest/gtest.h>
#include <chrono>
#include <QEventLoop>
#include <UtilsQt/Qml-Cpp/SteadyTimer.h>
#include <UtilsQt/Futures/Utils.h>
#include <UtilsQt/Futures/SignalToFuture.h>

namespace {

using Clock = std::chrono::steady_clock;

// Maximum wait time for any timer test (generous for slow CI)
constexpr auto MaxTestTimeout = 10000; // ms

// Helper to measure elapsed time and wait for timer signal
struct TimerTestHelper {
    SteadyTimer timer;
    Clock::time_point startTime;
    int triggerCount = 0;
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
        startTime = Clock::now();
        timer.start(interval, repeat);
    }

    std::chrono::milliseconds elapsed() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - startTime);
    }

    // Wait for signal with timeout, returns true if signal received
    bool waitForTimeout(int timeoutMs = MaxTestTimeout) {
        auto f = UtilsQt::signalToFuture(&timer, &SteadyTimer::timeout, nullptr, timeoutMs);
        UtilsQt::waitForFuture<QEventLoop>(f);
        return !f.isCanceled();
    }

    // Wait for specific number of triggers
    bool waitForTriggers(int count, int timeoutMs = MaxTestTimeout) {
        auto deadline = Clock::now() + std::chrono::milliseconds(timeoutMs);
        while (triggerCount < count && Clock::now() < deadline) {
            // Process events to allow timer callbacks
            UtilsQt::waitForFuture<QEventLoop>(UtilsQt::createTimedFuture(10));
        }
        return triggerCount >= count;
    }
};

} // namespace

TEST(UtilsQt, SteadyTimer_Basic)
{
    using namespace std::chrono_literals;

    TimerTestHelper helper;
    ASSERT_GT(helper.timer.interval(), 0);
    ASSERT_GT(helper.timer.resolution(), 0);
    ASSERT_FALSE(helper.timer.active());

    helper.timer.setResolution(100);
    helper.start(300);
    ASSERT_TRUE(helper.timer.active());
    ASSERT_EQ(helper.triggerCount, 0);

    // Wait for the timeout signal
    ASSERT_TRUE(helper.waitForTimeout());

    // Verify timing: should have taken at least ~300ms (with some tolerance)
    auto elapsedMs = helper.elapsed();
    ASSERT_GE(elapsedMs, 250ms) << "Timer triggered too early";
    ASSERT_LE(elapsedMs, 1000ms) << "Timer took too long";  // Generous upper bound for CI

    ASSERT_EQ(helper.triggerCount, 1);
    ASSERT_FALSE(helper.timer.active());
}

TEST(UtilsQt, SteadyTimer_LowAutoResolution)
{
    using namespace std::chrono_literals;

    TimerTestHelper helper;
    helper.start(100);

    ASSERT_EQ(helper.triggerCount, 0);

    // Wait for the timeout signal
    ASSERT_TRUE(helper.waitForTimeout());

    // Verify timing
    auto elapsedMs = helper.elapsed();
    ASSERT_GE(elapsedMs, 70ms) << "Timer triggered too early";
    ASSERT_LE(elapsedMs, 500ms) << "Timer took too long";

    ASSERT_EQ(helper.triggerCount, 1);
    ASSERT_FALSE(helper.timer.active());
}

TEST(UtilsQt, SteadyTimer_Threshold)
{
    using namespace std::chrono_literals;

    TimerTestHelper helper;
    helper.timer.setResolution(100);
    helper.timer.setThresholdFactor(20); // means resolution x20 (+- 2 sec)
    helper.start(1000);

    ASSERT_EQ(helper.triggerCount, 0);

    // With high threshold, timer can fire early
    ASSERT_TRUE(helper.waitForTimeout());

    // Timer should fire, but might be early due to threshold
    auto elapsedMs = helper.elapsed();
    ASSERT_LE(elapsedMs, 2000ms) << "Timer took too long even with threshold";

    ASSERT_EQ(helper.triggerCount, 1);
    ASSERT_FALSE(helper.timer.active());
}

TEST(UtilsQt, SteadyTimer_StartStop)
{
    using namespace std::chrono_literals;

    TimerTestHelper helper;
    helper.start(200);

    // Wait a bit, but not long enough for timer
    UtilsQt::waitForFuture<QEventLoop>(UtilsQt::createTimedFuture(50));
    ASSERT_EQ(helper.triggerCount, 0);
    ASSERT_TRUE(helper.timer.active());

    // Stop the timer
    helper.timer.stop();
    ASSERT_EQ(helper.triggerCount, 0);
    ASSERT_FALSE(helper.timer.active());

    // Wait more - timer should NOT fire since it's stopped
    UtilsQt::waitForFuture<QEventLoop>(UtilsQt::createTimedFuture(300));
    ASSERT_EQ(helper.triggerCount, 0);
}

TEST(UtilsQt, SteadyTimer_Repeat)
{
    using namespace std::chrono_literals;

    TimerTestHelper helper;
    helper.start(100, true);

    // Wait for 3 triggers
    ASSERT_TRUE(helper.waitForTriggers(3));

    auto elapsedMs = helper.elapsed();
    ASSERT_GE(elapsedMs, 250ms) << "3 triggers happened too fast";
    ASSERT_LE(elapsedMs, 2000ms) << "3 triggers took too long";

    ASSERT_GE(helper.triggerCount, 3);

    // Stop and verify no more triggers
    helper.timer.stop();
    int countAtStop = helper.triggerCount;

    UtilsQt::waitForFuture<QEventLoop>(UtilsQt::createTimedFuture(250));
    ASSERT_EQ(helper.triggerCount, countAtStop) << "Timer triggered after stop";
}

TEST(UtilsQt, SteadyTimer_ZeroInterval)
{
    TimerTestHelper helper;
    auto f = UtilsQt::signalToFuture(&helper.timer, &SteadyTimer::timeout, nullptr, 200);
    helper.timer.start(0);

    ASSERT_FALSE(helper.timer.active());  // Should immediately become inactive

    UtilsQt::waitForFuture<QEventLoop>(f);
    ASSERT_EQ(helper.triggerCount, 1);
}

TEST(UtilsQt, SteadyTimer_AutoResolutionIncrease)
{
    using namespace std::chrono_literals;

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
