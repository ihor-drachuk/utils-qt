/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once

#include <atomic>
#include <functional>

#include <QElapsedTimer>
#include <QEventLoop>

#include <UtilsQt/Futures/Utils.h>

namespace TestHelpers {

constexpr int SafetyTimeoutMs = 10000;
constexpr int PollIntervalMs = 10;

// Generic wait helper: waits until predicate returns true or timeout expires.
// Processes Qt events during wait to allow signal/slot delivery.
// Returns true if predicate became true, false on timeout.
template<typename Predicate>
bool waitUntil(Predicate&& pred, int timeoutMs = SafetyTimeoutMs)
{
    QElapsedTimer elapsed;
    elapsed.start();
    while (!pred() && elapsed.elapsed() < timeoutMs) {
        UtilsQt::waitForFuture<QEventLoop>(UtilsQt::createTimedFuture(PollIntervalMs));
    }
    return pred();
}

// Convenience: wait for atomic bool flag to become true
inline bool waitForFlag(const std::atomic<bool>& flag, int timeoutMs = SafetyTimeoutMs)
{
    return waitUntil([&flag]() {
        return flag.load(std::memory_order_acquire);
    }, timeoutMs);
}

// Convenience: wait for atomic counter to reach or exceed expected value
inline bool waitForCounter(const std::atomic<int>& counter, int expected, int timeoutMs = SafetyTimeoutMs)
{
    return waitUntil([&counter, expected]() {
        return counter.load(std::memory_order_acquire) >= expected;
    }, timeoutMs);
}

// Simple fixed-time wait with Qt event processing (non-timing-critical only)
inline void waitMs(int ms)
{
    UtilsQt::waitForFuture<QEventLoop>(UtilsQt::createTimedFuture(ms));
}

} // namespace TestHelpers
