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
    property int lastCallValue: 0
    property var callTimes: []

    function resetCounters() {
        callCount = 0;
        lastCallValue = 0;
        callTimes = [];
    }

    function testCallback() {
        callCount++;
        callTimes.push(Date.now());
    }

    function testCallbackWithValue(value) {
        callCount++;
        lastCallValue = value;
        callTimes.push(Date.now());
    }

    TestCase {
        name: "callOnceTest"

        function init() {
            resetCounters();
        }

        function test_00_basicCall() {
            QmlUtils.callOnce(testCallback, 100 * root.timeMultiplier);
            compare(callCount, 0);

            wait(200 * root.timeMultiplier);
            compare(callCount, 1);
        }

        function test_01_callWithParameters() {
            QmlUtils.callOnce(function() { testCallbackWithValue(42); }, 100 * root.timeMultiplier);
            compare(callCount, 0);
            compare(lastCallValue, 0);

            wait(200 * root.timeMultiplier);
            compare(callCount, 1);
            compare(lastCallValue, 42);
        }

        function test_02_restartOnCall_mode() {
            var startTime = Date.now();

            QmlUtils.callOnce(testCallback, 200 * root.timeMultiplier, QmlUtils.RestartOnCall);
            wait(100 * root.timeMultiplier);
            compare(callCount, 0);

            QmlUtils.callOnce(testCallback, 200 * root.timeMultiplier, QmlUtils.RestartOnCall);
            wait(150 * root.timeMultiplier);
            compare(callCount, 0);

            wait(100 * root.timeMultiplier);
            compare(callCount, 1);

            verify(callTimes.length === 1);
            var totalTime = callTimes[0] - startTime;
            verify(totalTime >= 250 * root.timeMultiplier && totalTime <= 350 * root.timeMultiplier);
        }

        function test_03_continueOnCall_mode() {
            var startTime = Date.now();

            QmlUtils.callOnce(testCallback, 200 * root.timeMultiplier, QmlUtils.ContinueOnCall);
            wait(100 * root.timeMultiplier);
            compare(callCount, 0);

            QmlUtils.callOnce(testCallback, 200 * root.timeMultiplier, QmlUtils.ContinueOnCall);
            wait(150 * root.timeMultiplier);
            compare(callCount, 1);

            verify(callTimes.length === 1);
            var totalTime = callTimes[0] - startTime;
            verify(totalTime >= 170 * root.timeMultiplier && totalTime <= 250 * root.timeMultiplier);

            wait(500 * root.timeMultiplier);
            compare(callCount, 1);
        }

        function test_04_multipleFunctions() {
            var counter1 = 0, counter2 = 0;

            QmlUtils.callOnce(function() { counter1++; }, 100 * root.timeMultiplier);
            QmlUtils.callOnce(function() { counter2++; }, 200 * root.timeMultiplier);

            compare(counter1, 0);
            compare(counter2, 0);

            wait(150 * root.timeMultiplier);
            compare(counter1, 1);
            compare(counter2, 0);

            wait(150 * root.timeMultiplier);
            compare(counter1, 1);
            compare(counter2, 1);
        }

        function test_05_sameFunctionDifferentClosures() {
            var value1 = "first";
            var value2 = "second";
            var results = [];

            QmlUtils.callOnce(function() { results.push(value1); }, 100 * root.timeMultiplier);
            QmlUtils.callOnce(function() { results.push(value2); }, 200 * root.timeMultiplier);

            wait(300 * root.timeMultiplier);
            compare(results.length, 2);
            verify(results.indexOf("first") !== -1);
            verify(results.indexOf("second") !== -1);
        }

        function test_06_errorHandling() {
            var errorThrown = false;

            QmlUtils.callOnce(function() { throw new Error("Test error"); }, 50 * root.timeMultiplier);
            wait(150 * root.timeMultiplier);
            // Error should be caught and logged, execution continues
        }

        function test_07_veryShortTimeout() {
            QmlUtils.callOnce(testCallback, 1);
            wait(50 * root.timeMultiplier);
            compare(callCount, 1);
        }

        function test_08_sequentialCalls() {
            var sequence = [];

            QmlUtils.callOnce(function() {
                sequence.push(1);
                QmlUtils.callOnce(function() {
                    sequence.push(2);
                }, 100 * root.timeMultiplier);
            }, 100 * root.timeMultiplier);

            wait(150 * root.timeMultiplier);
            compare(sequence.length, 1);
            compare(sequence[0], 1);

            wait(100 * root.timeMultiplier);
            compare(sequence.length, 2);
            compare(sequence[1], 2);
        }

        function test_09_allLambdasAlwaysDiffer() {
            var localCallCount = 0;
            const N = 5;
            const Timeout = 100 * root.timeMultiplier;
            const Delay = 50 * root.timeMultiplier;
            const WaitTime = (Timeout-Delay) + 50 * root.timeMultiplier;
            const MinimalCount = Math.floor(N * Delay / Timeout);

            for (var i = 0; i < N; i++) {
                QmlUtils.callOnce(function() { localCallCount++; }, Timeout, QmlUtils.RestartOnCall);
                wait(Delay);
            }

            verify(localCallCount >= MinimalCount);
            wait(Timeout * N + 50 * root.timeMultiplier);

            // --------

            localCallCount = 0;
            var func = function() { localCallCount++; };

            for (i = 0; i < N; i++) {
                QmlUtils.callOnce(func, Timeout, QmlUtils.RestartOnCall);
                wait(Delay);
            }

            compare(localCallCount, 0);

            wait(WaitTime);
            compare(localCallCount, 1);
            wait(WaitTime);
            compare(localCallCount, 1);

            // --------

            resetCounters();

            for (i = 0; i < N; i++) {
                QmlUtils.callOnce(testCallback, Timeout, QmlUtils.RestartOnCall);
                wait(Delay);
            }

            compare(callCount, 0);

            wait(WaitTime);
            compare(callCount, 1);
            wait(WaitTime);
            compare(callCount, 1);
        }
    }
}
