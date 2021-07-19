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

    ListModelTools {
        id: listModelTools
        model: listModel
        allowRoles: true
        allowJsValues: true
        bufferChanges: true
    }

    SignalSpy {
        id: sBeforeRemoved
        target: listModelTools
        signalName: "beforeRemoved"
    }

    SignalSpy {
        id: sRemoved
        target: listModelTools
        signalName: "removed"
    }

    SignalSpy {
        id: sBeforeInserted
        target: listModelTools
        signalName: "beforeInserted"
    }

    SignalSpy {
        id: sInserted
        target: listModelTools
        signalName: "inserted"
    }

    SignalSpy {
        id: sChanged
        target: listModelTools
        signalName: "changed"
    }

    TestCase {
        name: "ListModelToolsTest"

        function test_count() {
            compare(listModelTools.itemsCount, 4);

            var roles = [].concat(listModelTools.roles);
            var rolesModel = ["stringValue", "intValue"];
            compare(roles.sort(), rolesModel.sort());

            var i1s = listModelTools.getDataByRoles(1, ["stringValue"]);
            var i1i = listModelTools.getDataByRoles(1, ["intValue"]);
            var i1   = listModelTools.getDataByRoles(1, ["intValue", "stringValue"]);
            var i1_2 = listModelTools.getData(1);
            compare(i1, i1_2);
            compare(i1s, {"stringValue": "str2"});
            compare(i1i, {"intValue": 22});
            compare(i1, {"intValue": 22, "stringValue": "str2"});
            compare(listModelTools.getData(1, "stringValue"), "str2");
            compare(listModelTools.getData(1, "intValue"), 22);

            listModel.remove(1);
            compare(listModelTools.itemsCount, 3);
            compare(sBeforeRemoved.count, 1);
            compare(sRemoved.count, 1);

            compare(sBeforeRemoved.signalArguments[0][0], 1);
            compare(sBeforeRemoved.signalArguments[0][1], 1);
            compare(sRemoved.signalArguments[0][0], 1);
            compare(sRemoved.signalArguments[0][1], 1);
            var tester = sRemoved.signalArguments[0][2];
            compare(tester(0), false);
            compare(tester(1), true);
            compare(tester(2), false);

            listModel.append({"stringValue": "str-new", "intValue": 100});
            compare(listModelTools.itemsCount, 4);
            compare(sBeforeInserted.count, 1);
            compare(sInserted.count, 1);

            compare(sBeforeInserted.signalArguments[0][0], 3);
            compare(sBeforeInserted.signalArguments[0][1], 3);
            compare(sInserted.signalArguments[0][0], 3);
            compare(sInserted.signalArguments[0][1], 3);

            listModel.set(2, {"stringValue": "s222", "intValue": 222})
            compare(listModelTools.itemsCount, 4);
            compare(sChanged.count, 1);
            compare(sChanged.signalArguments[0][0], 2);
            compare(sChanged.signalArguments[0][1], 2);
            var roles1 = sChanged.signalArguments[0][3];
            var roles2 = ["stringValue", "intValue"];
            compare(roles1.sort(), roles2.sort());
            var data = listModelTools.getData(2);
            compare(data["stringValue"], "s222");
            compare(data["intValue"], 222);

            listModelTools.setData(0, "SSS", "stringValue");
            compare(sChanged.count, 2);
            data = listModelTools.getData(0);
            compare(data["stringValue"], "SSS");
            compare(data["intValue"], 11);
            data = listModel.get(0);
            compare(data["stringValue"], "SSS");
            compare(data["intValue"], 11);

            listModelTools.setDataByRoles(0, {"stringValue": "S", "intValue": 1});
            compare(sChanged.count, 3);
            data = listModelTools.getData(0);
            compare(data["stringValue"], "S");
            compare(data["intValue"], 1);
            data = listModel.get(0);
            compare(data["stringValue"], "S");
            compare(data["intValue"], 1);

            listModelTools.setDataByRoles(0, {"stringValue": "S2", "intValue": 12});
            compare(sChanged.count, 4);
            data = listModelTools.getData(0);
            compare(data["stringValue"], "S2");
            compare(data["intValue"], 12);
            data = listModel.get(0);
            compare(data["stringValue"], "S2");
            compare(data["intValue"], 12);

            listModelTools.model = null;
            compare(listModelTools.itemsCount, 0);
        }
    }
}
