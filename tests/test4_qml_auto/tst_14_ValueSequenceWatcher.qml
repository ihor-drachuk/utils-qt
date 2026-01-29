/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */


//     Usage examples
// ----------------------
//
// Example 1: Detect false->true transition
//  /
// |  ValueSequenceWatcher {
// |      value: someProperty
// |      sequence: [false, true]
// |      onTriggered: console.log("false->true detected!")
// |  }
//  \
//
// Example 2: Detect value held for minimum time
//  /
// |  ValueSequenceWatcher {
// |      value: someProperty
// |      sequence: [VSW.MinDuration(true, 500)]
// |      onTriggered: console.log("true held for 500ms!")
// |  }
//  \
//
// Example 3: Detect sequence with duration constraints
//  /
// |  ValueSequenceWatcher {
// |      value: someProperty
// |      sequence: [false, VSW.MinDuration(true, 500)]
// |      once: true
// |      onTriggered: console.log("Pattern matched once!")
// |  }
//  \


import QtQuick 2.15
import QtTest 1.15
import UtilsQt 1.0

Item {
    id: root

    readonly property int timeMultiplier: Qt.platform.os === "osx" ? 10 : 1

    // Test 1: Simple sequence detection
    QtObject {
        id: simpleSeqTest
        property bool value: false
        property int triggerCount: 0
    }

    ValueSequenceWatcher {
        id: simpleSeqWatcher
        value: simpleSeqTest.value
        sequence: [false, true]
        onTriggered: simpleSeqTest.triggerCount++
    }

    // Test 2: Immediate match on creation
    QtObject {
        id: immediateMatchTest
        property bool value: true
        property int triggerCount: 0
    }

    ValueSequenceWatcher {
        id: immediateMatchWatcher
        value: immediateMatchTest.value
        sequence: [true]
        onTriggered: immediateMatchTest.triggerCount++
    }

    // Test 3: MinDuration test
    QtObject {
        id: minDurationTest
        property bool value: false
        property int triggerCount: 0
    }

    ValueSequenceWatcher {
        id: minDurationWatcher
        value: minDurationTest.value
        sequence: [VSW.MinDuration(true, 100 * root.timeMultiplier)]
        onTriggered: minDurationTest.triggerCount++
    }

    // Test 4: MaxDuration test
    QtObject {
        id: maxDurationTest
        property bool value: false
        property int triggerCount: 0
    }

    ValueSequenceWatcher {
        id: maxDurationWatcher
        value: maxDurationTest.value
        sequence: [VSW.MaxDuration(false, 100 * root.timeMultiplier), true]
        onTriggered: maxDurationTest.triggerCount++
    }

    // Test 5: once=true test
    QtObject {
        id: onceTest
        property bool value: false
        property int triggerCount: 0
    }

    ValueSequenceWatcher {
        id: onceWatcher
        value: onceTest.value
        sequence: [false, true]
        once: true
        onTriggered: onceTest.triggerCount++
    }

    // Test 6: enabled toggle test
    QtObject {
        id: enabledTest
        property bool value: false
        property int triggerCount: 0
    }

    ValueSequenceWatcher {
        id: enabledWatcher
        enabled: false
        value: enabledTest.value
        sequence: [false, true]
        onTriggered: enabledTest.triggerCount++
    }

    // Test 7: resetOnTrigger=false (sliding window) test
    QtObject {
        id: slidingWindowTest
        property int value: 0
        property int triggerCount: 0
    }

    ValueSequenceWatcher {
        id: slidingWindowWatcher
        value: slidingWindowTest.value
        sequence: [1, 2]
        resetOnTrigger: false
        onTriggered: slidingWindowTest.triggerCount++
    }

    // Test 8: triggerCount property test
    QtObject {
        id: triggerCountTest
        property bool value: false
    }

    ValueSequenceWatcher {
        id: triggerCountWatcher
        value: triggerCountTest.value
        sequence: [false, true]
    }

    // Test 9: Duration (min + max) test
    QtObject {
        id: durationTest
        property bool value: false
        property int triggerCount: 0
    }

    ValueSequenceWatcher {
        id: durationWatcher
        value: durationTest.value
        sequence: [VSW.Duration(false, 50 * root.timeMultiplier, 150 * root.timeMultiplier), true]
        onTriggered: durationTest.triggerCount++
    }

    // Test 10: MinDuration in middle of sequence
    QtObject {
        id: middleMinDurationTest
        property int value: 0
        property int triggerCount: 0
    }

    ValueSequenceWatcher {
        id: middleMinDurationWatcher
        value: middleMinDurationTest.value
        sequence: [1, VSW.MinDuration(2, 100 * root.timeMultiplier), 3]
        onTriggered: middleMinDurationTest.triggerCount++
    }

    // Test 11: String values
    QtObject {
        id: stringTest
        property string value: ""
        property int triggerCount: 0
    }

    ValueSequenceWatcher {
        id: stringWatcher
        value: stringTest.value
        sequence: ["hello", "world"]
        onTriggered: stringTest.triggerCount++
    }

    // Test 12: Sequence change during operation
    QtObject {
        id: seqChangeTest
        property bool value: false
        property int triggerCount: 0
    }

    ValueSequenceWatcher {
        id: seqChangeWatcher
        value: seqChangeTest.value
        sequence: [false, true]
        onTriggered: seqChangeTest.triggerCount++
    }

    // Test 13: Float/double values with fuzzy comparison
    QtObject {
        id: floatTest
        property real value: 0.0
        property int triggerCount: 0
    }

    ValueSequenceWatcher {
        id: floatWatcher
        value: floatTest.value
        sequence: [1.0, 2.0]
        onTriggered: floatTest.triggerCount++
    }

    // Test 14: Empty sequence (should never trigger)
    QtObject {
        id: emptySeqTest
        property int value: 0
        property int triggerCount: 0
    }

    ValueSequenceWatcher {
        id: emptySeqWatcher
        value: emptySeqTest.value
        sequence: []
        onTriggered: emptySeqTest.triggerCount++
    }

    TestCase {
        name: "ValueSequenceWatcherTest"

        function test_00_simple_sequence_detection() {
            simpleSeqTest.value = false;
            simpleSeqTest.triggerCount = 0;
            wait(50 * root.timeMultiplier);

            compare(simpleSeqTest.triggerCount, 0, "Should not trigger initially");

            simpleSeqTest.value = true;
            wait(50 * root.timeMultiplier);
            compare(simpleSeqTest.triggerCount, 1, "Should trigger on false->true");

            simpleSeqTest.value = false;
            simpleSeqTest.value = true;
            wait(50 * root.timeMultiplier);
            compare(simpleSeqTest.triggerCount, 2, "Should trigger again on false->true");
        }

        function test_01_immediate_match_on_creation() {
            // The watcher should have triggered immediately since value was true at creation
            compare(immediateMatchTest.triggerCount, 1, "Should trigger immediately when value matches sequence at creation");
        }

        function test_02_minDuration_waits() {
            minDurationTest.value = false;
            minDurationTest.triggerCount = 0;
            wait(50 * root.timeMultiplier);

            minDurationTest.value = true;
            compare(minDurationTest.triggerCount, 0, "Should not trigger immediately");

            wait(30 * root.timeMultiplier); // Well before MinDuration (100ms)
            compare(minDurationTest.triggerCount, 0, "Should not trigger before MinDuration");

            wait(120 * root.timeMultiplier); // Well past MinDuration (total 150ms vs 100ms required)
            compare(minDurationTest.triggerCount, 1, "Should trigger after MinDuration");

            // Wait more to ensure no spurious re-trigger
            wait(200 * root.timeMultiplier);
            compare(minDurationTest.triggerCount, 1, "Should not trigger again after timer fired");
        }

        function test_03_minDuration_cancelled_by_change() {
            minDurationTest.value = false;
            minDurationTest.triggerCount = 0;
            wait(50 * root.timeMultiplier);

            minDurationTest.value = true;
            wait(30 * root.timeMultiplier); // Well before MinDuration (100ms)

            // Change value before MinDuration is reached
            minDurationTest.value = false;
            wait(150 * root.timeMultiplier);
            compare(minDurationTest.triggerCount, 0, "Should not trigger if value changed before MinDuration");
        }

        function test_04_maxDuration_passes() {
            // First set to a different value to reset history timestamp
            maxDurationTest.value = true;
            maxDurationTest.triggerCount = 0;
            wait(10 * root.timeMultiplier);

            // Now set to false with fresh timestamp
            maxDurationTest.value = false;
            wait(30 * root.timeMultiplier); // Well within MaxDuration (100ms)

            // Quickly change to true (within MaxDuration)
            maxDurationTest.value = true;
            wait(50 * root.timeMultiplier);
            compare(maxDurationTest.triggerCount, 1, "Should trigger when previous value was within MaxDuration");
        }

        function test_05_maxDuration_fails() {
            maxDurationTest.value = true;
            maxDurationTest.triggerCount = 0;
            wait(50 * root.timeMultiplier);

            maxDurationTest.value = false;
            wait(180 * root.timeMultiplier); // Well over MaxDuration (100ms)

            maxDurationTest.value = true;
            wait(50 * root.timeMultiplier);
            compare(maxDurationTest.triggerCount, 0, "Should not trigger when previous value exceeded MaxDuration");
        }

        function test_06_once_triggers_only_once() {
            onceTest.value = false;
            onceTest.triggerCount = 0;
            wait(50 * root.timeMultiplier);

            onceTest.value = true;
            wait(50 * root.timeMultiplier);
            compare(onceTest.triggerCount, 1, "Should trigger first time");

            onceTest.value = false;
            onceTest.value = true;
            wait(50 * root.timeMultiplier);
            compare(onceTest.triggerCount, 1, "Should not trigger again with once=true");
        }

        function test_07_enabled_toggle() {
            enabledTest.value = false;
            enabledTest.triggerCount = 0;
            wait(50 * root.timeMultiplier);

            // Watcher is disabled, should not trigger
            enabledTest.value = true;
            wait(50 * root.timeMultiplier);
            compare(enabledTest.triggerCount, 0, "Should not trigger when disabled");

            // Enable the watcher
            enabledWatcher.enabled = true;
            wait(50 * root.timeMultiplier);

            // Now it should start fresh - current value is the first element
            enabledTest.value = false;
            enabledTest.value = true;
            wait(50 * root.timeMultiplier);
            compare(enabledTest.triggerCount, 1, "Should trigger after enabled");

            // Disable again
            enabledWatcher.enabled = false;
        }

        function test_08_sliding_window() {
            slidingWindowTest.value = 0;
            slidingWindowTest.triggerCount = 0;
            wait(50 * root.timeMultiplier);

            slidingWindowTest.value = 1;
            wait(50 * root.timeMultiplier);
            compare(slidingWindowTest.triggerCount, 0, "Should not trigger yet");

            slidingWindowTest.value = 2;
            wait(50 * root.timeMultiplier);
            compare(slidingWindowTest.triggerCount, 1, "Should trigger on 1->2");

            // With resetOnTrigger=false, history keeps [2]
            // Now send 1, 2 again
            slidingWindowTest.value = 1;
            slidingWindowTest.value = 2;
            wait(50 * root.timeMultiplier);
            compare(slidingWindowTest.triggerCount, 2, "Should trigger again (sliding window)");
        }

        function test_09_triggerCount_property() {
            triggerCountTest.value = false;
            wait(50 * root.timeMultiplier);

            compare(triggerCountWatcher.triggerCount, 0, "triggerCount should be 0 initially");

            triggerCountTest.value = true;
            wait(50 * root.timeMultiplier);
            compare(triggerCountWatcher.triggerCount, 1, "triggerCount should be 1 after first trigger");

            triggerCountTest.value = false;
            triggerCountTest.value = true;
            wait(50 * root.timeMultiplier);
            compare(triggerCountWatcher.triggerCount, 2, "triggerCount should be 2 after second trigger");
        }

        function test_10_duration_min_and_max() {
            durationTest.value = true;
            durationTest.triggerCount = 0;
            wait(50 * root.timeMultiplier);

            // Set false, wait within valid range (50-150ms), then change to true
            durationTest.value = false;
            wait(100 * root.timeMultiplier); // Solidly within 50-150ms range
            durationTest.value = true;
            wait(50 * root.timeMultiplier);
            compare(durationTest.triggerCount, 1, "Should trigger when duration is within range");
        }

        function test_11_duration_too_short() {
            durationTest.value = true;
            durationTest.triggerCount = 0;
            wait(50 * root.timeMultiplier);

            // Set false, change too quickly (less than minMs of 50ms)
            durationTest.value = false;
            wait(10 * root.timeMultiplier); // Well under 50ms minimum
            durationTest.value = true;
            wait(50 * root.timeMultiplier);
            compare(durationTest.triggerCount, 0, "Should not trigger when duration is too short");
        }

        function test_12_minDuration_in_middle() {
            middleMinDurationTest.value = 0;
            middleMinDurationTest.triggerCount = 0;
            wait(50 * root.timeMultiplier);

            middleMinDurationTest.value = 1;
            wait(50 * root.timeMultiplier);

            middleMinDurationTest.value = 2;
            wait(180 * root.timeMultiplier); // Hold 2 well over 100ms

            middleMinDurationTest.value = 3;
            wait(50 * root.timeMultiplier);
            compare(middleMinDurationTest.triggerCount, 1, "Should trigger when middle element held for MinDuration");
        }

        function test_13_minDuration_in_middle_too_short() {
            middleMinDurationTest.value = 0;
            middleMinDurationTest.triggerCount = 0;
            wait(50 * root.timeMultiplier);

            middleMinDurationTest.value = 1;
            wait(50 * root.timeMultiplier);

            middleMinDurationTest.value = 2;
            wait(30 * root.timeMultiplier); // Hold 2 well under 100ms

            middleMinDurationTest.value = 3;
            wait(50 * root.timeMultiplier);
            compare(middleMinDurationTest.triggerCount, 0, "Should not trigger when middle element not held long enough");
        }

        function test_14_string_values() {
            stringTest.value = "";
            stringTest.triggerCount = 0;
            wait(50 * root.timeMultiplier);

            stringTest.value = "hello";
            wait(50 * root.timeMultiplier);
            compare(stringTest.triggerCount, 0, "Should not trigger yet");

            stringTest.value = "world";
            wait(50 * root.timeMultiplier);
            compare(stringTest.triggerCount, 1, "Should trigger on hello->world");
        }

        function test_15_sequence_change_resets() {
            seqChangeTest.value = false;
            seqChangeTest.triggerCount = 0;
            seqChangeWatcher.sequence = [false, true];
            wait(50 * root.timeMultiplier);

            seqChangeTest.value = true;
            wait(50 * root.timeMultiplier);
            compare(seqChangeTest.triggerCount, 1, "Should trigger with original sequence");

            // Change the sequence
            seqChangeWatcher.sequence = [true, false];
            seqChangeTest.triggerCount = 0;
            wait(50 * root.timeMultiplier);

            seqChangeTest.value = false;
            wait(50 * root.timeMultiplier);
            compare(seqChangeTest.triggerCount, 1, "Should trigger with new sequence");
        }

        function test_16_float_values() {
            floatTest.value = 0.0;
            floatTest.triggerCount = 0;
            wait(50 * root.timeMultiplier);

            // Use values that might have floating point representation issues
            floatTest.value = 1.0;
            wait(50 * root.timeMultiplier);
            compare(floatTest.triggerCount, 0, "Should not trigger yet");

            floatTest.value = 2.0;
            wait(50 * root.timeMultiplier);
            compare(floatTest.triggerCount, 1, "Should trigger on 1.0->2.0");
        }

        function test_17_float_fuzzy_comparison() {
            floatTest.value = 0.0;
            floatTest.triggerCount = 0;
            floatWatcher.sequence = [0.1 + 0.2, 0.6];  // 0.1 + 0.2 is not exactly 0.3 in floating point
            wait(50 * root.timeMultiplier);

            // Set to a value that equals 0.3 via different calculation
            floatTest.value = 0.3;
            wait(50 * root.timeMultiplier);
            compare(floatTest.triggerCount, 0, "Should match 0.3 with 0.1+0.2 (fuzzy)");

            floatTest.value = 0.6;
            wait(50 * root.timeMultiplier);
            compare(floatTest.triggerCount, 1, "Should trigger on fuzzy float sequence");

            // Restore original sequence
            floatWatcher.sequence = [1.0, 2.0];
        }

        function test_18_empty_sequence() {
            emptySeqTest.value = 0;
            emptySeqTest.triggerCount = 0;
            wait(50 * root.timeMultiplier);

            emptySeqTest.value = 1;
            wait(50 * root.timeMultiplier);
            compare(emptySeqTest.triggerCount, 0, "Empty sequence should never trigger");

            emptySeqTest.value = 2;
            emptySeqTest.value = 3;
            wait(50 * root.timeMultiplier);
            compare(emptySeqTest.triggerCount, 0, "Empty sequence should still never trigger");
        }
    }
}
