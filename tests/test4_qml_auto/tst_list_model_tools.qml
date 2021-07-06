import QtQuick 2.15
import QtTest 1.15
import UtilsQt 1.0

Item {
    ListModel {
        id: listModel
        dynamicRoles: false
        ListElement { stringValue: "str1" }
        ListElement { stringValue: "str2" }
        ListElement { stringValue: "str3" }
        ListElement { stringValue: "str4" }
    }

    ListModelTools {
        id: listModelTools
        model: listModel
    }

    SignalSpy {
        target: listModelTools
        signalName: "count"
    }

    TestCase {
        name: "ListModelToolsTest"

        function test_count() {
            compare(listModelTools.itemsCount, 4);
            listModel.remove(1);
            compare(listModelTools.itemsCount, 3);
            listModel.append({"stringValue": "str-new"});
            compare(listModelTools.itemsCount, 4);
            listModelTools.model = null;
            compare(listModelTools.itemsCount, 0);
        }
    }
}
