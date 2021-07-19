import QtQuick 2.9
import QtTest 1.1
import UtilsQt 1.0

Item {
    ListModel {
        id: listModel
        dynamicRoles: false
        ListElement { stringValue: "str1"; intValue: 11 }
        ListElement { stringValue: "str2"; intValue: 22 }
        ListElement { stringValue: "str3"; intValue: 33 }
        ListElement { stringValue: "str4"; intValue: 44 }
    }

    QtObject {
        id: internal
        property var values: []
        property var modelDatas: []
    }

    Item {
        Repeater2 {
            model: listModel

            delegate: Item {
                Component.onCompleted: {
                    var d = {};
                    d["index"] = index;
                    d["isValid"] = isValid;
                    d["stringValue"] = modelData.stringValue;
                    d["intValue"] = modelData.intValue;
                    internal.values.push(d);
                    internal.modelDatas.push(modelData);
                }
            }
        }
    }

    Timer {
        id: delay
        running: true
        interval: 250
        repeat: false
    }

    TestCase {
        name: "RepeaterTest"
        when: !delay.running

        function test_0_basic() {
            var values2 = [
                {"index": 0, "isValid": true, "stringValue": "str1", "intValue": 11},
                {"index": 1, "isValid": true, "stringValue": "str2", "intValue": 22},
                {"index": 2, "isValid": true, "stringValue": "str3", "intValue": 33},
                {"index": 3, "isValid": true, "stringValue": "str4", "intValue": 44},
            ];

            compare(internal.values, values2);
        }
    }
}
