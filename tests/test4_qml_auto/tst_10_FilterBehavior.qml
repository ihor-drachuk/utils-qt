/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */


//     Usage examples
// ----------------------
//
// Example 1: Simple debounce
//  /
// |  property int value: rapidlyChangingSource
// |  FilterBehavior on value { delay: 300 }
//  \
//
// Example 2: Bool with delayed true, immediate false
//  /
// |  property bool activated: rawInput
// |  FilterBehavior on activated {
// |    when: rawInput === true
// |    delay: 500
// |  }
//  \


import QtQuick 2.15
import QtTest 1.15
import UtilsQt 1.0

Item {
    id: root

    readonly property int timeMultiplier: Qt.platform.os === "osx" ? 10 : 1;

    QtObject {
        id: testObj
        property bool rawBool: false
        property int rawInt: 0

        property bool filteredBool: rawBool
        FilterBehavior on filteredBool {
            when: testObj.rawBool === true
            delay: 100 * root.timeMultiplier
        }

        property int filteredInt: rawInt
        FilterBehavior on filteredInt {
            when: testObj.rawInt > 10
            delay: 100 * root.timeMultiplier
        }
    }

    QtObject {
        id: zeroDelayTest
        property bool raw: false
        property bool filtered: raw
        FilterBehavior on filtered {
            when: zeroDelayTest.raw === true
            delay: 0
        }
    }

    QtObject {
        id: externalCondTest
        property bool editMode: false
        property int raw: 5
        property int filtered: raw
        FilterBehavior on filtered {
            when: externalCondTest.editMode
            delay: 100 * root.timeMultiplier
        }
    }

    QtObject {
        id: dynamicCondTest
        property bool condition: true
        property int raw: 5
        property int filtered: raw
        FilterBehavior on filtered {
            when: dynamicCondTest.condition
            delay: 100 * root.timeMultiplier
        }
    }

    QtObject {
        id: stringTest
        property string raw: ""
        property string filtered: raw
        FilterBehavior on filtered {
            when: stringTest.raw.length > 3
            delay: 100 * root.timeMultiplier
        }
    }

    QtObject {
        id: multipleTest
        property int raw: 5

        property int filtered1: raw
        FilterBehavior on filtered1 {
            when: multipleTest.raw > 10
            delay: 100 * root.timeMultiplier
        }

        property int filtered2: raw
        FilterBehavior on filtered2 {
            when: multipleTest.raw > 20
            delay: 200 * root.timeMultiplier
        }
    }

    TestCase {
        name: "FilterBehaviorTest"

        function test_00_basic_delay_on_match() {
            testObj.rawBool = false;
            wait(60 * root.timeMultiplier);
            compare(testObj.filteredBool, false);

            testObj.rawBool = true;
            compare(testObj.filteredBool, false); // Delayed
            wait(160 * root.timeMultiplier);
            compare(testObj.filteredBool, true); // Applied
        }

        function test_01_immediate_on_no_match() {
            testObj.rawBool = true;
            wait(160 * root.timeMultiplier);
            compare(testObj.filteredBool, true);

            testObj.rawBool = false;
            wait(1);
            compare(testObj.filteredBool, false); // Immediate
        }

        function test_02_cancel_pending_bool() {
            testObj.rawBool = false;
            wait(60 * root.timeMultiplier);
            compare(testObj.filteredBool, false);

            // Start delay
            testObj.rawBool = true;
            compare(testObj.filteredBool, false);
            wait(60 * root.timeMultiplier); // Half delay

            // Cancel by setting false
            testObj.rawBool = false;
            wait(1);
            compare(testObj.filteredBool, false); // Applied immediately

            wait(160 * root.timeMultiplier); // Wait more than original delay
            compare(testObj.filteredBool, false); // Should stay false
        }

        function test_03_cancel_with_write_int() {
            testObj.rawInt = 5;
            wait(60 * root.timeMultiplier);
            compare(testObj.filteredInt, 5);

            // Start delay (15 > 10)
            testObj.rawInt = 15;
            compare(testObj.filteredInt, 5); // Pending 15
            wait(60 * root.timeMultiplier); // Half delay

            // Cancel by value that doesn't match condition
            testObj.rawInt = 7;
            wait(1);
            compare(testObj.filteredInt, 7); // Applied immediately

            wait(160 * root.timeMultiplier);
            compare(testObj.filteredInt, 7); // Should stay 7
        }

        function test_04_replace_pending_int() {
            testObj.rawInt = 5;
            wait(60 * root.timeMultiplier);
            compare(testObj.filteredInt, 5);

            // First delay (15 > 10)
            testObj.rawInt = 15;
            compare(testObj.filteredInt, 5);
            wait(60 * root.timeMultiplier);

            // Replace pending with another delayed value (20 > 10)
            testObj.rawInt = 20;
            compare(testObj.filteredInt, 5); // Still 5

            wait(160 * root.timeMultiplier); // Wait for delay
            compare(testObj.filteredInt, 20); // Should be 20, not 15
        }

        function test_05_zero_delay() {
            zeroDelayTest.raw = true;
            wait(1);
            compare(zeroDelayTest.filtered, true); // Immediate even with when=true
        }

        function test_06_external_condition() {
            externalCondTest.raw = 10;
            wait(1);
            compare(externalCondTest.filtered, 10); // Immediate (editMode=false)

            externalCondTest.editMode = true;
            externalCondTest.raw = 20;
            compare(externalCondTest.filtered, 10); // Delayed (editMode=true)
            wait(160 * root.timeMultiplier);
            compare(externalCondTest.filtered, 20);
        }

        function test_07_dynamic_when_change() {
            dynamicCondTest.raw = 10;
            compare(dynamicCondTest.filtered, 5); // Delayed
            wait(60 * root.timeMultiplier);

            // Change condition during pending
            dynamicCondTest.condition = false;
            dynamicCondTest.raw = 15;
            wait(1);
            compare(dynamicCondTest.filtered, 15); // Should apply immediately now
        }

        function test_08_string_type() {
            stringTest.raw = "hello";
            compare(stringTest.filtered, ""); // Delayed (length > 3)
            wait(160 * root.timeMultiplier);
            compare(stringTest.filtered, "hello");

            stringTest.raw = "hi";
            wait(1);
            compare(stringTest.filtered, "hi"); // Immediate (length <= 3)
        }

        function test_09_rapid_changes_with_cancel() {
            testObj.rawInt = 5;
            wait(60 * root.timeMultiplier);
            compare(testObj.filteredInt, 5);

            // Rapid changes with delayed values
            testObj.rawInt = 15; // Delayed
            wait(60 * root.timeMultiplier);
            testObj.rawInt = 20; // Replace pending
            wait(60 * root.timeMultiplier);
            testObj.rawInt = 25; // Replace pending again
            wait(60 * root.timeMultiplier);
            testObj.rawInt = 7;  // Cancel all, apply immediately

            wait(1);
            compare(testObj.filteredInt, 7);
            wait(160 * root.timeMultiplier);
            compare(testObj.filteredInt, 7); // Still 7
        }

        function test_10_multiple_instances() {
            // Reset to initial state
            multipleTest.raw = 5;
            wait(60 * root.timeMultiplier);
            compare(multipleTest.filtered1, 5);
            compare(multipleTest.filtered2, 5);

            multipleTest.raw = 15;
            compare(multipleTest.filtered1, 5);  // Delayed (15 > 10)
            wait(1);
            compare(multipleTest.filtered2, 15); // Immediate (15 <= 20)

            wait(160 * root.timeMultiplier);
            compare(multipleTest.filtered1, 15); // Applied
            compare(multipleTest.filtered2, 15);

            multipleTest.raw = 25;
            compare(multipleTest.filtered1, 15); // Delayed (25 > 10)
            compare(multipleTest.filtered2, 15); // Delayed (25 > 20)

            wait(260 * root.timeMultiplier);
            compare(multipleTest.filtered1, 25);
            compare(multipleTest.filtered2, 25);
        }
    }
}
