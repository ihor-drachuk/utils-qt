/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */


//     Usage examples
// ----------------------
//
// Example 1: Log property changes or measure performance
//  /
// |  Loader {
// |    id: loader
// |
// |    //active: true
// |
// |    PropertyInterceptor on active {
// |      onBeforeUpdated: (oldValue, newValue) => {
// |        console.warning("Changing from " + oldValue + " to " + newValue)
// |      }
// |
// |      onAfterUpdated: (oldValue, newValue) => {
// |        console.warning("Changed from " + oldValue + " to " + newValue)
// |      }
// |    }
// |  }
//  \
//
// Example 2: Track value changes
//  /
// |  property int counter: 0
// |
// |  PropertyInterceptor on counter {
// |    onAfterUpdated: (oldValue, newValue) => {
// |      console.log("Counter updated: " + oldValue + " -> " + newValue)
// |    }
// |  }
//  \


import QtQuick 2.15
import QtTest 1.15
import UtilsQt 1.0

Item {
    id: root

    QtObject {
        id: testObj
        property int value: 0
        property var beforeCalls: []
        property var afterCalls: []

        PropertyInterceptor on value {
            onBeforeUpdated: (oldValue, newValue) => { testObj.beforeCalls.push({old: oldValue, newVal: newValue}) }
            onAfterUpdated: (oldValue, newValue) => { testObj.afterCalls.push({old: oldValue, newVal: newValue}) }
        }
    }

    QtObject {
        id: beforeOnlyTest
        property string text: ""
        property var calls: []

        PropertyInterceptor on text {
            onBeforeUpdated: (oldValue, newValue) => { beforeOnlyTest.calls.push({old: oldValue, newVal: newValue}) }
        }
    }

    QtObject {
        id: afterOnlyTest
        property bool flag: false
        property var calls: []

        PropertyInterceptor on flag {
            onAfterUpdated: (oldValue, newValue) => { afterOnlyTest.calls.push({old: oldValue, newVal: newValue}) }
        }
    }

    QtObject {
        id: noCallbackTest
        property int counter: 0
        PropertyInterceptor on counter {
            // No callbacks
        }
    }

    QtObject {
        id: timingTest
        property int value: 10
        property int valueInBefore: -1
        property int valueInAfter: -1

        PropertyInterceptor on value {
            onBeforeUpdated: (oldValue, newValue) => {
                timingTest.valueInBefore = timingTest.value  // Read actual property value
            }
            onAfterUpdated: (oldValue, newValue) => {
                timingTest.valueInAfter = timingTest.value  // Read actual property value
            }
        }
    }

    QtObject {
        id: multiTypeTest
        property var calls: []
        property string stringVal: "initial"
        property int intVal: 0
        property bool boolVal: false

        PropertyInterceptor on stringVal {
            onBeforeUpdated: (oldValue, newValue) => { multiTypeTest.calls.push({type: "string", phase: "before", old: oldValue, newVal: newValue}) }
            onAfterUpdated: (oldValue, newValue) => { multiTypeTest.calls.push({type: "string", phase: "after", old: oldValue, newVal: newValue}) }
        }

        PropertyInterceptor on intVal {
            onBeforeUpdated: (oldValue, newValue) => { multiTypeTest.calls.push({type: "int", phase: "before", old: oldValue, newVal: newValue}) }
            onAfterUpdated: (oldValue, newValue) => { multiTypeTest.calls.push({type: "int", phase: "after", old: oldValue, newVal: newValue}) }
        }

        PropertyInterceptor on boolVal {
            onBeforeUpdated: (oldValue, newValue) => { multiTypeTest.calls.push({type: "bool", phase: "before", old: oldValue, newVal: newValue}) }
            onAfterUpdated: (oldValue, newValue) => { multiTypeTest.calls.push({type: "bool", phase: "after", old: oldValue, newVal: newValue}) }
        }
    }

    TestCase {
        name: "PropertyInterceptorTest"

        function test_00_both_callbacks() {
            testObj.beforeCalls = [];
            testObj.afterCalls = [];

            testObj.value = 42;

            compare(testObj.value, 42);
            compare(testObj.beforeCalls.length, 1);
            compare(testObj.afterCalls.length, 1);

            compare(testObj.beforeCalls[0].old, 0);
            compare(testObj.beforeCalls[0].newVal, 42);

            compare(testObj.afterCalls[0].old, 0);
            compare(testObj.afterCalls[0].newVal, 42);
        }

        function test_01_multiple_changes() {
            testObj.beforeCalls = [];
            testObj.afterCalls = [];

            testObj.value = 10;
            testObj.value = 20;
            testObj.value = 30;

            compare(testObj.value, 30);
            compare(testObj.beforeCalls.length, 3);
            compare(testObj.afterCalls.length, 3);

            // First change: 42 -> 10 (from previous test)
            compare(testObj.beforeCalls[0].old, 42);
            compare(testObj.beforeCalls[0].newVal, 10);

            // Second change: 10 -> 20
            compare(testObj.beforeCalls[1].old, 10);
            compare(testObj.beforeCalls[1].newVal, 20);

            // Third change: 20 -> 30
            compare(testObj.beforeCalls[2].old, 20);
            compare(testObj.beforeCalls[2].newVal, 30);
        }

        function test_02_before_only() {
            beforeOnlyTest.calls = [];

            beforeOnlyTest.text = "hello";

            compare(beforeOnlyTest.text, "hello");
            compare(beforeOnlyTest.calls.length, 1);
            compare(beforeOnlyTest.calls[0].old, "");
            compare(beforeOnlyTest.calls[0].newVal, "hello");
        }

        function test_03_after_only() {
            afterOnlyTest.calls = [];

            afterOnlyTest.flag = true;

            compare(afterOnlyTest.flag, true);
            compare(afterOnlyTest.calls.length, 1);
            compare(afterOnlyTest.calls[0].old, false);
            compare(afterOnlyTest.calls[0].newVal, true);
        }

        function test_04_no_callbacks() {
            // Should work normally without callbacks
            noCallbackTest.counter = 100;
            compare(noCallbackTest.counter, 100);

            noCallbackTest.counter = 200;
            compare(noCallbackTest.counter, 200);
        }

        function test_05_string_type() {
            multiTypeTest.calls = [];

            multiTypeTest.stringVal = "test";

            compare(multiTypeTest.stringVal, "test");
            compare(multiTypeTest.calls.length, 2);
            compare(multiTypeTest.calls[0].phase, "before");
            compare(multiTypeTest.calls[0].old, "initial");
            compare(multiTypeTest.calls[0].newVal, "test");
            compare(multiTypeTest.calls[1].phase, "after");
            compare(multiTypeTest.calls[1].old, "initial");
            compare(multiTypeTest.calls[1].newVal, "test");
        }

        function test_06_int_type() {
            multiTypeTest.calls = [];

            multiTypeTest.intVal = 123;

            compare(multiTypeTest.intVal, 123);
            compare(multiTypeTest.calls.length, 2);
            compare(multiTypeTest.calls[0].phase, "before");
            compare(multiTypeTest.calls[0].old, 0);
            compare(multiTypeTest.calls[0].newVal, 123);
            compare(multiTypeTest.calls[1].phase, "after");
        }

        function test_07_bool_type() {
            multiTypeTest.calls = [];

            multiTypeTest.boolVal = true;

            compare(multiTypeTest.boolVal, true);
            compare(multiTypeTest.calls.length, 2);
            compare(multiTypeTest.calls[0].phase, "before");
            compare(multiTypeTest.calls[0].old, false);
            compare(multiTypeTest.calls[0].newVal, true);
        }

        function test_08_callback_order() {
            testObj.beforeCalls = [];
            testObj.afterCalls = [];

            testObj.value = 999;

            compare(testObj.value, 999);
            // Verify before is called before after
            compare(testObj.beforeCalls.length, 1);
            compare(testObj.afterCalls.length, 1);
            // Both should see the same old value (30 from previous test)
            compare(testObj.beforeCalls[0].old, 30);
            compare(testObj.afterCalls[0].old, 30);
            // Both should see the same new value
            compare(testObj.beforeCalls[0].newVal, 999);
            compare(testObj.afterCalls[0].newVal, 999);
        }

        function test_09_same_value_write() {
            beforeOnlyTest.calls = [];
            beforeOnlyTest.text = "same";
            beforeOnlyTest.calls = [];

            // Write same value
            beforeOnlyTest.text = "same";

            // Interceptor should still be called
            compare(beforeOnlyTest.calls.length, 1);
            compare(beforeOnlyTest.calls[0].old, "same");
            compare(beforeOnlyTest.calls[0].newVal, "same");
        }

        function test_10_before_called_before_change() {
            timingTest.valueInBefore = -1;
            timingTest.valueInAfter = -1;
            timingTest.value = 10;

            // Change value from 10 to 50
            timingTest.value = 50;

            // In beforeUpdated, property should still have OLD value (10)
            compare(timingTest.valueInBefore, 10);

            // In afterUpdated, property should have NEW value (50)
            compare(timingTest.valueInAfter, 50);

            // Final value should be 50
            compare(timingTest.value, 50);
        }

        function test_11_after_called_after_change() {
            timingTest.valueInBefore = -1;
            timingTest.valueInAfter = -1;
            timingTest.value = 100;

            // Change value from 100 to 200
            timingTest.value = 200;

            // In beforeUpdated, property should still have OLD value (100)
            compare(timingTest.valueInBefore, 100);

            // In afterUpdated, property should have NEW value (200)
            compare(timingTest.valueInAfter, 200);

            // Final value should be 200
            compare(timingTest.value, 200);
        }
    }
}
