/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

import QtQuick 2.9
import QtTest 1.1
import UtilsQt 1.0

Item {
    function createSpy(sigName, target) {
        var obj = Qt.createQmlObject("import QtTest 1.1; SignalSpy { signalName: \"%1\" }".arg(sigName), parent);
        obj.target = target ? target : proxyModel;
        return obj;
    }

    function deleteSpy(spy) {
        spy.target = null;
        spy.destroy();
    }

    ListModel {
        id: listModel
        dynamicRoles: false
        ListElement { someId: 1; someValue: "RSGC1_F12" }
        ListElement { someId: 2; someValue: "Monocerotis" }
        ListElement { someId: 3; someValue: "25875" }
    }

    PlusOneProxyModel {
        id: proxyModel
        sourceModel: listModel
    }

    ListModelTools {
        id: listModelTools
        model: proxyModel
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
        name: "PlusOneProxyModel"
        when: !delay.running

        // Test initial state
        function test_00_initial() {
            var x = internal.dumpModelVar();
            compare(x, [
                        {"someId": 1,    "someValue": "RSGC1_F12",   "isArtificial": false, "artificialValue": null},
                        {"someId": 2,    "someValue": "Monocerotis", "isArtificial": false, "artificialValue": null},
                        {"someId": 3,    "someValue": "25875",       "isArtificial": false, "artificialValue": null},
                        {"someId": null, "someValue": null,          "isArtificial": true,  "artificialValue": null},
                    ]);
        }

        // Test 'artificialValue'
        function test_01_artificialValue() {
            proxyModel.artificialValue = "art";

            var x = internal.dumpModelVar();
            compare(x, [
                        {"someId": 1,    "someValue": "RSGC1_F12",   "isArtificial": false, "artificialValue": null},
                        {"someId": 2,    "someValue": "Monocerotis", "isArtificial": false, "artificialValue": null},
                        {"someId": 3,    "someValue": "25875",       "isArtificial": false, "artificialValue": null},
                        {"someId": null, "someValue": null,          "isArtificial": true,  "artificialValue": "art"},
                    ]);

            proxyModel.artificialValue = null;
            x = internal.dumpModelVar();
            compare(x, [
                        {"someId": 1,    "someValue": "RSGC1_F12",   "isArtificial": false, "artificialValue": null},
                        {"someId": 2,    "someValue": "Monocerotis", "isArtificial": false, "artificialValue": null},
                        {"someId": 3,    "someValue": "25875",       "isArtificial": false, "artificialValue": null},
                        {"someId": null, "someValue": null,          "isArtificial": true,  "artificialValue": null},
                    ]);
        }

        // Test 'enabled' switch
        function test_02_enabled() {
            proxyModel.enabled = false;

            var x = internal.dumpModelVar();
            compare(x, [
                        {"someId": 1,    "someValue": "RSGC1_F12",   "isArtificial": false, "artificialValue": null},
                        {"someId": 2,    "someValue": "Monocerotis", "isArtificial": false, "artificialValue": null},
                        {"someId": 3,    "someValue": "25875",       "isArtificial": false, "artificialValue": null},
                    ]);

            proxyModel.enabled = true;
            x = internal.dumpModelVar();
            compare(x, [
                        {"someId": 1,    "someValue": "RSGC1_F12",   "isArtificial": false, "artificialValue": null},
                        {"someId": 2,    "someValue": "Monocerotis", "isArtificial": false, "artificialValue": null},
                        {"someId": 3,    "someValue": "25875",       "isArtificial": false, "artificialValue": null},
                        {"someId": null, "someValue": null,          "isArtificial": true,  "artificialValue": null},
                    ]);
        }

        // Test 'mode' switch
        function test_03_mode() {
            var sRowsMoved = createSpy("rowsMoved");

            proxyModel.mode = PlusOneProxyModel.Prepend;
            var x = internal.dumpModelVar();
            compare(x, [
                        {"someId": null, "someValue": null,          "isArtificial": true,  "artificialValue": null},
                        {"someId": 1,    "someValue": "RSGC1_F12",   "isArtificial": false, "artificialValue": null},
                        {"someId": 2,    "someValue": "Monocerotis", "isArtificial": false, "artificialValue": null},
                        {"someId": 3,    "someValue": "25875",       "isArtificial": false, "artificialValue": null},
                    ]);

            const count = proxyModel.rowCount();

            compare(sRowsMoved.count, 1);
            compare(sRowsMoved.signalArguments[0][1], count - 1);
            compare(sRowsMoved.signalArguments[0][2], count - 1);
            compare(sRowsMoved.signalArguments[0][4], 0);
            sRowsMoved.clear();

            proxyModel.mode = PlusOneProxyModel.Append;
            x = internal.dumpModelVar();
            compare(x, [
                        {"someId": 1,    "someValue": "RSGC1_F12",   "isArtificial": false, "artificialValue": null},
                        {"someId": 2,    "someValue": "Monocerotis", "isArtificial": false, "artificialValue": null},
                        {"someId": 3,    "someValue": "25875",       "isArtificial": false, "artificialValue": null},
                        {"someId": null, "someValue": null,          "isArtificial": true,  "artificialValue": null},
                    ]);

            compare(sRowsMoved.count, 1);
            compare(sRowsMoved.signalArguments[0][1], 0);
            compare(sRowsMoved.signalArguments[0][2], 0);
            compare(sRowsMoved.signalArguments[0][4], count - 1);

            deleteSpy(sRowsMoved);
        }

        // Test combined cases
        function test_04_setData() {
            proxyModel.enabled = false;
            var x = internal.dumpModelVar();
            compare(x, [
                        {"someId": 1,    "someValue": "RSGC1_F12",   "isArtificial": false, "artificialValue": null},
                        {"someId": 2,    "someValue": "Monocerotis", "isArtificial": false, "artificialValue": null},
                        {"someId": 3,    "someValue": "25875",       "isArtificial": false, "artificialValue": null},
                    ]);

            proxyModel.mode = PlusOneProxyModel.Prepend;
            x = internal.dumpModelVar();
            compare(x, [
                        {"someId": 1,    "someValue": "RSGC1_F12",   "isArtificial": false, "artificialValue": null},
                        {"someId": 2,    "someValue": "Monocerotis", "isArtificial": false, "artificialValue": null},
                        {"someId": 3,    "someValue": "25875",       "isArtificial": false, "artificialValue": null},
                    ]);

            proxyModel.artificialValue = 123;
            x = internal.dumpModelVar();
            compare(x, [
                        {"someId": 1,    "someValue": "RSGC1_F12",   "isArtificial": false, "artificialValue": null},
                        {"someId": 2,    "someValue": "Monocerotis", "isArtificial": false, "artificialValue": null},
                        {"someId": 3,    "someValue": "25875",       "isArtificial": false, "artificialValue": null},
                    ]);

            proxyModel.enabled = true;
            x = internal.dumpModelVar();
            compare(x, [
                        {"someId": null, "someValue": null,          "isArtificial": true,  "artificialValue": 123},
                        {"someId": 1,    "someValue": "RSGC1_F12",   "isArtificial": false, "artificialValue": null},
                        {"someId": 2,    "someValue": "Monocerotis", "isArtificial": false, "artificialValue": null},
                        {"someId": 3,    "someValue": "25875",       "isArtificial": false, "artificialValue": null},
                    ]);

            proxyModel.sourceModel = null;
            x = internal.dumpModelVar();
            compare(x, []);

            proxyModel.sourceModel = listModel;
            x = internal.dumpModelVar();
            compare(x, [
                        {"someId": null, "someValue": null,          "isArtificial": true,  "artificialValue": 123},
                        {"someId": 1,    "someValue": "RSGC1_F12",   "isArtificial": false, "artificialValue": null},
                        {"someId": 2,    "someValue": "Monocerotis", "isArtificial": false, "artificialValue": null},
                        {"someId": 3,    "someValue": "25875",       "isArtificial": false, "artificialValue": null},
                    ]);

            proxyModel.mode = PlusOneProxyModel.Append;
            proxyModel.artificialValue = null;
            x = internal.dumpModelVar();
            compare(x, [
                        {"someId": 1,    "someValue": "RSGC1_F12",   "isArtificial": false, "artificialValue": null},
                        {"someId": 2,    "someValue": "Monocerotis", "isArtificial": false, "artificialValue": null},
                        {"someId": 3,    "someValue": "25875",       "isArtificial": false, "artificialValue": null},
                        {"someId": null, "someValue": null,          "isArtificial": true,  "artificialValue": null},
                    ]);
        }

        // Test 'setData'
        function test_05_setData() {
            var sDataChanged = createSpy("dataChanged");

            proxyModel.mode = PlusOneProxyModel.Prepend;
            var x = internal.dumpModelVar();
            compare(x, [
                        {"someId": null, "someValue": null,          "isArtificial": true,  "artificialValue": null},
                        {"someId": 1,    "someValue": "RSGC1_F12",   "isArtificial": false, "artificialValue": null},
                        {"someId": 2,    "someValue": "Monocerotis", "isArtificial": false, "artificialValue": null},
                        {"someId": 3,    "someValue": "25875",       "isArtificial": false, "artificialValue": null},
                    ]);
            compare(sDataChanged.count, 0);

            const index_1_0 = listModelTools.modelIndexByRow(1);
            const role_someId = listModelTools.roleNameToInt("someId");

            proxyModel.setData(index_1_0, 111, role_someId);
            x = internal.dumpModelVar();
            compare(x, [
                        {"someId": null, "someValue": null,          "isArtificial": true,  "artificialValue": null},
                        {"someId": 111,  "someValue": "RSGC1_F12",   "isArtificial": false, "artificialValue": null},
                        {"someId": 2,    "someValue": "Monocerotis", "isArtificial": false, "artificialValue": null},
                        {"someId": 3,    "someValue": "25875",       "isArtificial": false, "artificialValue": null},
                    ]);
            compare(sDataChanged.count, 1);
            compare(sDataChanged.signalArguments[0][0], proxyModel.index(1, 0));
            compare(sDataChanged.signalArguments[0][1], proxyModel.index(1, 0));
            //compare(sDataChanged.signalArguments[2], [role_someId]);

            proxyModel.setData(index_1_0, 1, role_someId);
            compare(sDataChanged.count, 2);

            proxyModel.mode = PlusOneProxyModel.Append;
            deleteSpy(sDataChanged);
        }

        // Test row insertion & deletion
        function test_06_rowInsertion() {
            proxyModel.mode = PlusOneProxyModel.Prepend;

            var insertionSpy = createSpy("rowsInserted");

            listModel.append({"someId": 4, "someValue": "NewElement"});
            compare(insertionSpy.count, 1);
            compare(proxyModel.rowCount(), listModel.count + 1);

            var arguments = insertionSpy.signalArguments[0];
            compare(arguments[1], listModel.count);
            compare(arguments[2], listModel.count);
            deleteSpy(insertionSpy);

            var deletionSpy = createSpy("rowsRemoved");

            listModel.remove(listModel.count-1);
            compare(deletionSpy.count, 1);
            compare(proxyModel.rowCount(), listModel.count + 1);

            arguments = insertionSpy.signalArguments[0];
            compare(arguments[1], listModel.count+1);
            compare(arguments[2], listModel.count+1);

            proxyModel.mode = PlusOneProxyModel.Append;
            deleteSpy(deletionSpy);
        }

        // Test row movement
        function test_07_rowMovement() {
            var srcMoveSpy = createSpy("rowsMoved", listModel);
            var proxyMoveSpy = createSpy("rowsMoved");

            var x = internal.dumpModelVar();
            compare(x, [
                        {"someId": 1,    "someValue": "RSGC1_F12",   "isArtificial": false, "artificialValue": null},
                        {"someId": 2,    "someValue": "Monocerotis", "isArtificial": false, "artificialValue": null},
                        {"someId": 3,    "someValue": "25875",       "isArtificial": false, "artificialValue": null},
                        {"someId": null, "someValue": null,          "isArtificial": true,  "artificialValue": null},
                    ]);

            listModel.move(0, 1, 2);

            x = internal.dumpModelVar();
            compare(x, [
                        {"someId": 3,    "someValue": "25875",       "isArtificial": false, "artificialValue": null},
                        {"someId": 1,    "someValue": "RSGC1_F12",   "isArtificial": false, "artificialValue": null},
                        {"someId": 2,    "someValue": "Monocerotis", "isArtificial": false, "artificialValue": null},
                        {"someId": null, "someValue": null,          "isArtificial": true,  "artificialValue": null},
                    ]);

            compare(srcMoveSpy.count, 1);

            compare(srcMoveSpy.signalArguments[0][1], 0);
            compare(srcMoveSpy.signalArguments[0][2], 1);
            compare(srcMoveSpy.signalArguments[0][4], 3);

            compare(proxyMoveSpy.signalArguments[0][1], 0);
            compare(proxyMoveSpy.signalArguments[0][2], 1);
            compare(proxyMoveSpy.signalArguments[0][4], 3);

            listModel.move(listModel.count - 2, 0, 2);
            x = internal.dumpModelVar();
            compare(x, [
                        {"someId": 1,    "someValue": "RSGC1_F12",   "isArtificial": false, "artificialValue": null},
                        {"someId": 2,    "someValue": "Monocerotis", "isArtificial": false, "artificialValue": null},
                        {"someId": 3,    "someValue": "25875",       "isArtificial": false, "artificialValue": null},
                        {"someId": null, "someValue": null,          "isArtificial": true,  "artificialValue": null},
                    ]);

            proxyModel.mode = PlusOneProxyModel.Prepend;
            srcMoveSpy.clear();
            proxyMoveSpy.clear();

            x = internal.dumpModelVar();
            compare(x, [
                        {"someId": null, "someValue": null,          "isArtificial": true,  "artificialValue": null},
                        {"someId": 1,    "someValue": "RSGC1_F12",   "isArtificial": false, "artificialValue": null},
                        {"someId": 2,    "someValue": "Monocerotis", "isArtificial": false, "artificialValue": null},
                        {"someId": 3,    "someValue": "25875",       "isArtificial": false, "artificialValue": null},
                    ]);

            listModel.move(0, 1, 2);

            x = internal.dumpModelVar();
            compare(x, [
                        {"someId": null, "someValue": null,          "isArtificial": true,  "artificialValue": null},
                        {"someId": 3,    "someValue": "25875",       "isArtificial": false, "artificialValue": null},
                        {"someId": 1,    "someValue": "RSGC1_F12",   "isArtificial": false, "artificialValue": null},
                        {"someId": 2,    "someValue": "Monocerotis", "isArtificial": false, "artificialValue": null},
                    ]);

            compare(srcMoveSpy.count, 1);

            compare(srcMoveSpy.signalArguments[0][1], 0);
            compare(srcMoveSpy.signalArguments[0][2], 1);
            compare(srcMoveSpy.signalArguments[0][4], 3);

            compare(proxyMoveSpy.signalArguments[0][1], 1);
            compare(proxyMoveSpy.signalArguments[0][2], 2);
            compare(proxyMoveSpy.signalArguments[0][4], 4);

            listModel.move(listModel.count - 2, 0, 2);
            proxyModel.mode = PlusOneProxyModel.Append;
            x = internal.dumpModelVar();
            compare(x, [
                        {"someId": 1,    "someValue": "RSGC1_F12",   "isArtificial": false, "artificialValue": null},
                        {"someId": 2,    "someValue": "Monocerotis", "isArtificial": false, "artificialValue": null},
                        {"someId": 3,    "someValue": "25875",       "isArtificial": false, "artificialValue": null},
                        {"someId": null, "someValue": null,          "isArtificial": true,  "artificialValue": null},
                    ]);

            deleteSpy(proxyMoveSpy);
            deleteSpy(srcMoveSpy);
        }

        // Test cascaded mode
        function test_08_cascaded() {
            var anotherPlusOne = Qt.createQmlObject('import UtilsQt 1.0; PlusOneProxyModel { sourceModel: proxyModel; artificialValue: 123 }', parent);
            var anotherTools =  Qt.createQmlObject('import UtilsQt 1.0; ListModelTools { allowRoles: true; allowJsValues: false }', parent);
            anotherPlusOne.mode = PlusOneProxyModel.Prepend;
            anotherTools.model = anotherPlusOne;

            // Check count
            compare(anotherPlusOne.rowCount(), listModel.count+2);

            // Check cascaded values
            compare(anotherTools.getData(0, "isArtificial"), true);
            compare(anotherTools.getData(0, "artificialValue"), 123);

            compare(anotherTools.getData(1, "isArtificial"), false);
            compare(anotherTools.getData(1, "artificialValue"), null);

            compare(anotherTools.getData(anotherTools.itemsCount - 1, "isArtificial"), true);
            compare(anotherTools.getData(anotherTools.itemsCount - 1, "artificialValue"), null);

            // Change and check cascaded values again
            proxyModel.artificialValue = "Test";

            compare(anotherTools.getData(0, "isArtificial"), true);
            compare(anotherTools.getData(0, "artificialValue"), 123);

            compare(anotherTools.getData(1, "isArtificial"), false);
            compare(anotherTools.getData(1, "artificialValue"), null);

            compare(anotherTools.getData(anotherTools.itemsCount - 1, "isArtificial"), true);
            compare(anotherTools.getData(anotherTools.itemsCount - 1, "artificialValue"), "Test");

            // Return to initial state
            proxyModel.artificialValue = null;

            // Delete temp objects
            anotherTools.destroy();
            anotherPlusOne.destroy();
        }
    }
}
