/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once

#include <atomic>
#include <chrono>
#include <functional>

#include <QEventLoop>

#include <UtilsQt/Futures/Utils.h>

namespace TestHelpers {

// Maximum timeout for any single wait operation (generous to handle slow CI)
constexpr auto MaxWaitTimeout = std::chrono::milliseconds(10000);

// Poll interval during wait loops
constexpr auto WaitPollInterval = std::chrono::milliseconds(10);

// Generic wait helper: waits until predicate returns true or timeout expires.
// Processes Qt events during wait to allow signal/slot delivery.
// Returns true if predicate became true, false on timeout.
template<typename Predicate, typename Clock = std::chrono::steady_clock>
bool waitUntil(Predicate&& pred, std::chrono::milliseconds timeout = MaxWaitTimeout)
{
    const auto deadline = Clock::now() + timeout;
    while (!pred() && Clock::now() < deadline) {
        // Process Qt events to allow signal/slot delivery
        UtilsQt::waitForFuture<QEventLoop>(UtilsQt::createTimedFuture(
            static_cast<int>(WaitPollInterval.count())));
    }
    return pred();
}

// Convenience: wait for atomic bool flag to become true
template<typename Clock = std::chrono::steady_clock>
bool waitForFlag(const std::atomic<bool>& flag,
                 std::chrono::milliseconds timeout = MaxWaitTimeout)
{
    return waitUntil([&flag]() {
        return flag.load(std::memory_order_acquire);
    }, timeout);
}

// Convenience: wait for atomic counter to reach or exceed expected value
template<typename Clock = std::chrono::steady_clock>
bool waitForCounter(const std::atomic<int>& counter, int expected,
                    std::chrono::milliseconds timeout = MaxWaitTimeout)
{
    return waitUntil([&counter, expected]() {
        return counter.load(std::memory_order_acquire) >= expected;
    }, timeout);
}

} // namespace TestHelpers
