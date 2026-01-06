/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

import QtQuick 2.15
import QtTest 1.15
import UtilsQt 1.0

Item {
    id: root

    readonly property int timeMultiplier: Qt.platform.os === "osx" ? 10 : 1;
    property int callCount: 0
    property var callTimes: []

    function resetCounters() {
        callCount = 0;
        callTimes = [];
    }

    function testCallback() {
        callCount++;
        callTimes.push(Date.now());
    }

    // Dynamic component for context destruction tests
    Component {
        id: dynamicItemComponent
        Item { }
    }

    TestCase {
        name: "callDelayedTest"

        function init() {
            resetCounters();
        }

        function test_00_basicDelayedCall() {
            QmlUtils.callDelayed(null, testCallback, 100 * root.timeMultiplier);
            compare(callCount, 0);

            wait(200 * root.timeMultiplier);
            compare(callCount, 1);
        }

        function test_01_multipleDelayedCallsNoDedupe() {
            // Unlike Once_* modes, RegularDelayed should NOT deduplicate
            var localCount = 0;
            var func = function() { localCount++; };

            QmlUtils.callDelayed(null, func, 100 * root.timeMultiplier);
            QmlUtils.callDelayed(null, func, 100 * root.timeMultiplier);
            QmlUtils.callDelayed(null, func, 100 * root.timeMultiplier);

            compare(localCount, 0);

            wait(200 * root.timeMultiplier);

            // All three calls should have executed
            compare(localCount, 3);
        }

        function test_02_contextDestruction() {
            // Create a dynamic item to use as context
            var dynamicItem = dynamicItemComponent.createObject(root);
            verify(dynamicItem !== null);

            var called = false;
            QmlUtils.callDelayed(dynamicItem, function() { called = true; }, 200 * root.timeMultiplier);

            wait(50 * root.timeMultiplier);
            compare(called, false);

            // Destroy the context before delay expires
            dynamicItem.destroy();
            wait(50 * root.timeMultiplier); // Allow destruction to process

            // Wait for the original delay to pass
            wait(200 * root.timeMultiplier);

            // Function should NOT have been called since context was destroyed
            compare(called, false);
        }

        function test_03_contextValidCallExecutes() {
            // Create a dynamic item to use as context
            var dynamicItem = dynamicItemComponent.createObject(root);
            verify(dynamicItem !== null);

            var called = false;
            QmlUtils.callDelayed(dynamicItem, function() { called = true; }, 100 * root.timeMultiplier);

            wait(200 * root.timeMultiplier);

            // Function should have been called since context is still alive
            compare(called, true);

            dynamicItem.destroy();
        }

        function test_04_contextNullWorks() {
            // Verify that null context works (no context tracking)
            var called = false;
            QmlUtils.callDelayed(null, function() { called = true; }, 100 * root.timeMultiplier);

            wait(200 * root.timeMultiplier);
            compare(called, true);
        }

        function test_05_differentDelays() {
            var sequence = [];

            QmlUtils.callDelayed(null, function() { sequence.push(1); }, 100 * root.timeMultiplier);
            QmlUtils.callDelayed(null, function() { sequence.push(2); }, 50 * root.timeMultiplier);
            QmlUtils.callDelayed(null, function() { sequence.push(3); }, 150 * root.timeMultiplier);

            wait(200 * root.timeMultiplier);

            compare(sequence.length, 3);
            compare(sequence[0], 2); // 50ms first
            compare(sequence[1], 1); // 100ms second
            compare(sequence[2], 3); // 150ms third
        }

        function test_06_errorHandling() {
            // Error should be caught and logged, not crash
            QmlUtils.callDelayed(null, function() { throw new Error("Test error"); }, 50 * root.timeMultiplier);
            wait(150 * root.timeMultiplier);
            // If we get here, error was handled properly
        }

        function test_07_multipleContextsOneSurvives() {
            var item1 = dynamicItemComponent.createObject(root);
            var item2 = dynamicItemComponent.createObject(root);
            verify(item1 !== null);
            verify(item2 !== null);

            var count1 = 0, count2 = 0;

            QmlUtils.callDelayed(item1, function() { count1++; }, 150 * root.timeMultiplier);
            QmlUtils.callDelayed(item2, function() { count2++; }, 150 * root.timeMultiplier);

            wait(50 * root.timeMultiplier);

            // Destroy only item1
            item1.destroy();
            wait(50 * root.timeMultiplier);

            wait(150 * root.timeMultiplier);

            // Only item2's callback should have executed
            compare(count1, 0);
            compare(count2, 1);

            item2.destroy();
        }

        function test_08_veryShortDelay() {
            var called = false;
            QmlUtils.callDelayed(null, function() { called = true; }, 1);
            wait(50 * root.timeMultiplier);
            compare(called, true);
        }

        function test_09_deferredDeletionSafety() {
            // Test that deferred deletion doesn't cause issues with rapid create/destroy cycles
            var callCount = 0;

            for (var i = 0; i < 5; i++) {
                var item = dynamicItemComponent.createObject(root);
                QmlUtils.callDelayed(item, function() { callCount++; }, 50 * root.timeMultiplier);
                item.destroy();
            }

            // Final call without context
            QmlUtils.callDelayed(null, function() { callCount++; }, 50 * root.timeMultiplier);

            wait(200 * root.timeMultiplier);

            // Only the final call should have executed (the others had their contexts destroyed)
            compare(callCount, 1);
        }

        function test_10_multipleDelayedCallsSameFunc() {
            // Unlike Once_* modes, RegularDelayed should execute ALL calls even with same function
            var callCount = 0;
            var func = function() { callCount++; };

            QmlUtils.callDelayed(null, func, 50 * root.timeMultiplier);
            QmlUtils.callDelayed(null, func, 50 * root.timeMultiplier);
            QmlUtils.callDelayed(null, func, 50 * root.timeMultiplier);

            wait(150 * root.timeMultiplier);

            // All 3 calls should have executed
            compare(callCount, 3);
        }
    }
}
