import QtQuick 2.9
import QtTest 1.1
import UtilsQt 1.0

Item {
    ListModel {
        id: listModel1
        dynamicRoles: false
        ListElement { uid: 1; value1: "Ihor"; value1_mlm1: 1 }
        ListElement { uid: 2; value1: "Alex"; value1_mlm1: 2 }
        ListElement { uid: 3; value1: "Oleg"; value1_mlm1: 3 }
    }

    ListModel {
        id: listModel2
        dynamicRoles: false
        ListElement { uidd: 2; value1: "Alex22"; value2: "Alex23" }
        ListElement { uidd: 4; value1: "Bob22"; value2: "Bob23" }
    }

    MergedListModel {
        id: mergedListModel
        model1: listModel1
        model2: listModel2
        joinRole1: "uid"
        joinRole2: "uidd"
    }

    ListModelTools {
        id: listModelTools
        model: mergedListModel
        allowRoles: true
        allowJsValues: false
    }

    Timer {
        id: delay
        running: true
        interval: 250
        repeat: false
    }

    QtObject {
        id: internal

        function dumpModel() {
            console.debug("Dump:");
            var count = listModelTools.itemsCount;
            for (var i = 0; i < count; i++) {
                var x = listModelTools.getData(i);
                console.debug(JSON.stringify(x));
            }
        }

        function dumpModelVar() {
            var list = [];
            var count = listModelTools.itemsCount;
            for (var i = 0; i < count; i++) {
                var x = listModelTools.getData(i);
                list.push(x);
            }
            return list;
        }
    }

    TestCase {
        name: "MergedListModelTest_2"
        when: !delay.running

        function test_0_basic() {
            // Test initial state
            var x = internal.dumpModelVar();
            compare(x, [
                        {"source":1,"uid":1,"value1":"Ihor","value1_mlm1":1,"value1_mlm2":null,"value2":null},
                        {"source":3,"uid":2,"value1":"Alex","value1_mlm1":2,"value1_mlm2":"Alex22","value2":"Alex23"},
                        {"source":1,"uid":3,"value1":"Oleg","value1_mlm1":3,"value1_mlm2":null,"value2":null},
                        {"source":2,"uid":4,"value1":null,"value1_mlm1":null,"value1_mlm2":"Bob22","value2":"Bob23"}]);
        }
    }
}
