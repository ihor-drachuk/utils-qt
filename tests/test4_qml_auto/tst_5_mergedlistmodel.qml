import QtQuick 2.9
import QtTest 1.1
import UtilsQt 1.0

Item {
    ListModel {
        id: listModel1
        dynamicRoles: false
        ListElement { uid: 1; value1: "Ihor" }
        ListElement { uid: 2; value1: "Alex" }
        ListElement { uid: 3; value1: "Oleg" }
    }

    ListModel {
        id: listModel2
        dynamicRoles: false
        ListElement { uid: 2; value2: "Alex22"; value3: "Alex23" }
        ListElement { uid: 4; value2: "Bob22"; value3: "Bob23" }
    }

    MergedListModel {
        id: mergedListModel
        model1: listModel1
        model2: listModel2
        joinRole1: "uid"
        joinRole2: "uid"
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
        name: "MergedListModelTest_1"
        when: !delay.running

        function test_0_basic() {
            // Test initial state
            var x = internal.dumpModelVar();
            compare(x, [
                        {"source":1,"uid":1,"value1":"Ihor","value2":null,"value3":null},
                        {"source":3,"uid":2,"value1":"Alex","value2":"Alex22","value3":"Alex23"},
                        {"source":1,"uid":3,"value1":"Oleg","value2":null,"value3":null},
                        {"source":2,"uid":4,"value1":null,"value2":"Bob22","value3":"Bob23"}]);

            // Test onDataChanged
            listModel1.set(0, {uid:1, value1:"IHOR"})
            x = internal.dumpModelVar();
            compare(x, [
                        {"source":1,"uid":1,"value1":"IHOR","value2":null,"value3":null},
                        {"source":3,"uid":2,"value1":"Alex","value2":"Alex22","value3":"Alex23"},
                        {"source":1,"uid":3,"value1":"Oleg","value2":null,"value3":null},
                        {"source":2,"uid":4,"value1":null,"value2":"Bob22","value3":"Bob23"}]);

            listModel1.set(0, {uid:1, value1:"Ihor"})
            listModel1.set(1, {uid:2, value1:"ALEX"})
            x = internal.dumpModelVar();
            compare(x, [
                        {"source":1,"uid":1,"value1":"Ihor","value2":null,"value3":null},
                        {"source":3,"uid":2,"value1":"ALEX","value2":"Alex22","value3":"Alex23"},
                        {"source":1,"uid":3,"value1":"Oleg","value2":null,"value3":null},
                        {"source":2,"uid":4,"value1":null,"value2":"Bob22","value3":"Bob23"}]);

            listModel1.set(1, {uid:2, value1:"Alex"})

            // Test onDataChanged with changed joinRole
            listModel2.set(0, {uid:0, value2:"ALEX", value3:"ALEX!"}) // TODO: Better to use 0 -> null. But ListModel can't...

            x = internal.dumpModelVar();
            compare(x, [
                        {"source":1,"uid":1,"value1":"Ihor","value2":null,"value3":null},
                        {"source":1,"uid":2,"value1":"Alex","value2":null,"value3":null},
                        {"source":1,"uid":3,"value1":"Oleg","value2":null,"value3":null},
                        {"source":2,"uid":4,"value1":null,"value2":"Bob22","value3":"Bob23"},
                        {"source":2,"uid":0,"value1":null,"value2":"ALEX","value3":"ALEX!"}]);

            listModel2.set(0, {uid:2, value2:"ALEX", value3:"ALEX!"})

            x = internal.dumpModelVar();
            compare(x, [
                        {"source":1,"uid":1,"value1":"Ihor","value2":null,"value3":null},
                        {"source":3,"uid":2,"value1":"Alex","value2":"ALEX","value3":"ALEX!"},
                        {"source":1,"uid":3,"value1":"Oleg","value2":null,"value3":null},
                        {"source":2,"uid":4,"value1":null,"value2":"Bob22","value3":"Bob23"}]);

            listModel1.set(1, {uid:10, value1:"Alex"})

            x = internal.dumpModelVar();
            compare(x, [
                        {"source":1,"uid":1,"value1":"Ihor","value2":null,"value3":null},
                        {"source":2,"uid":2,"value1":null,"value2":"ALEX","value3":"ALEX!"},
                        {"source":1,"uid":3,"value1":"Oleg","value2":null,"value3":null},
                        {"source":2,"uid":4,"value1":null,"value2":"Bob22","value3":"Bob23"},
                        {"source":1,"uid":10,"value1":"Alex","value2":null,"value3":null}]);

            listModel1.set(1, {uid:4, value1:"Alex"})

            x = internal.dumpModelVar();
            compare(x, [
                        {"source":1,"uid":1,"value1":"Ihor","value2":null,"value3":null},
                        {"source":2,"uid":2,"value1":null,"value2":"ALEX","value3":"ALEX!"},
                        {"source":1,"uid":3,"value1":"Oleg","value2":null,"value3":null},
                        {"source":3,"uid":4,"value1":"Alex","value2":"Bob22","value3":"Bob23"}]);

            // Test insertion
            listModel1.append({"uid":10,"value1":"AAA"});
            listModel1.append({"uid":2,"value1":"BBB"});
            x = internal.dumpModelVar();
            compare(x, [
                        {"source":1,"uid":1,"value1":"Ihor","value2":null,"value3":null},
                        {"source":3,"uid":2,"value1":"BBB","value2":"ALEX","value3":"ALEX!"},
                        {"source":1,"uid":3,"value1":"Oleg","value2":null,"value3":null},
                        {"source":3,"uid":4,"value1":"Alex","value2":"Bob22","value3":"Bob23"},
                        {"source":1,"uid":10,"value1":"AAA","value2":null,"value3":null}]);

            // Test removal
            listModel1.remove(listModel1.count - 2, 1)

            x = internal.dumpModelVar();
            compare(x, [
                        {"source":1,"uid":1,"value1":"Ihor","value2":null,"value3":null},
                        {"source":3,"uid":2,"value1":"BBB","value2":"ALEX","value3":"ALEX!"},
                        {"source":1,"uid":3,"value1":"Oleg","value2":null,"value3":null},
                        {"source":3,"uid":4,"value1":"Alex","value2":"Bob22","value3":"Bob23"}]);

            listModel1.remove(listModel1.count - 1, 1)

            x = internal.dumpModelVar();
            compare(x, [
                        {"source":1,"uid":1,"value1":"Ihor","value2":null,"value3":null},
                        {"source":2,"uid":2,"value1":null,"value2":"ALEX","value3":"ALEX!"},
                        {"source":1,"uid":3,"value1":"Oleg","value2":null,"value3":null},
                        {"source":3,"uid":4,"value1":"Alex","value2":"Bob22","value3":"Bob23"}]);

            // Test setData
            mergedListModel.setData(listModelTools.modelIndexByRow(0), "Ihor2", listModelTools.roleNameToInt("value1"));
            mergedListModel.setData(listModelTools.modelIndexByRow(1), "ALEX2", listModelTools.roleNameToInt("value2"));
            mergedListModel.setData(listModelTools.modelIndexByRow(3), "Alex2", listModelTools.roleNameToInt("value1"));

            x = internal.dumpModelVar();
            compare(x, [
                        {"source":1,"uid":1,"value1":"Ihor2","value2":null,"value3":null},
                        {"source":2,"uid":2,"value1":null,"value2":"ALEX2","value3":"ALEX!"},
                        {"source":1,"uid":3,"value1":"Oleg","value2":null,"value3":null},
                        {"source":3,"uid":4,"value1":"Alex2","value2":"Bob22","value3":"Bob23"}]);
        }
    }
}
