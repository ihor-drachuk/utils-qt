/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

import QtQuick 2.9
import QtTest 1.1
import UtilsQt 1.0

Item {
    ListModel {
        id: listModel1
        dynamicRoles: false
        ListElement { uid: 1; type: "Pulsar" }
        ListElement { uid: 2; type: "Black Hole" }
        ListElement { uid: 3; type: "Magnetar" }
    }

    ListModel {
        id: listModel2
        dynamicRoles: false
        ListElement { uid: 1; interest: 1; objects: "PSR B1919+21, PSR B0531+21, PSR J1748-2446ad" }
        ListElement { uid: 2; interest: 3; objects: "Sagittarius A, Cygnus X-1" }
        ListElement { uid: 3; interest: 2; objects: "SGR 1806-20, SGR 1900+14, 1E 2259+586" }
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
        name: "MergedListModelTest_3"
        when: !delay.running

        function test_0_basic() {
            // Test initial state
            var x = internal.dumpModelVar();
            compare(x, [
                {"source":3, "uid":1, "interest": 1, "type": "Pulsar",     "objects": "PSR B1919+21, PSR B0531+21, PSR J1748-2446ad"},
                {"source":3, "uid":2, "interest": 3, "type": "Black Hole", "objects": "Sagittarius A, Cygnus X-1"},
                {"source":3, "uid":3, "interest": 2, "type": "Magnetar",   "objects": "SGR 1806-20, SGR 1900+14, 1E 2259+586"}
            ]);

            // Add resetter
            MLMResetter.setCustomResetter(mergedListModel);

            // Try remove some data
            listModel2.remove(2);

            x = internal.dumpModelVar();
            compare(x, [
                {"source":3, "uid":1, "interest": 1,           "type": "Pulsar",     "objects": "PSR B1919+21, PSR B0531+21, PSR J1748-2446ad"},
                {"source":3, "uid":2, "interest": 3,           "type": "Black Hole", "objects": "Sagittarius A, Cygnus X-1"},
                {"source":1, "uid":3, "interest": "I-DELETED", "type": "Magnetar",   "objects": "DELETED"}
            ]);

            // Try to split by changing join-role
            listModel2.set(1, {uid:22});

            x = internal.dumpModelVar();
            compare(x, [
                {"source":3, "uid":1, "interest": 1,           "type": "Pulsar",     "objects": "PSR B1919+21, PSR B0531+21, PSR J1748-2446ad"},
                {"source":1, "uid":2, "interest": "I-DELETED", "type": "Black Hole", "objects": "DELETED"},
                {"source":1, "uid":3, "interest": "I-DELETED", "type": "Magnetar",   "objects": "DELETED"},
                {"source":2, "uid":22,"interest": 3,           "type": null,         "objects": "Sagittarius A, Cygnus X-1"}
            ]);
        }
    }
}
