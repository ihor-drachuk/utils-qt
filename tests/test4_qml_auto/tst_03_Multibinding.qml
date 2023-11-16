/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

import QtQuick 2.9
import QtTest 1.1
import UtilsQt 1.0

Item {
    QtObject {
        id: object1
        property string value: "value1"
    }

    QtObject {
        id: object2
        property string value: "value2"
    }

    Component {
        id: mbSettings

        QtObject {
            property bool enableR: true
            property bool enableW: true
        }
    }

    Loader { id: settings1; sourceComponent: mbSettings }
    Loader { id: settings2; sourceComponent: mbSettings }

    Loader {
        id: mbLoader

        active: false

        sourceComponent: Item {
            Multibinding {
                MultibindingItem { object: object1; propertyName: "value";
                    enableR: settings1.item.enableR
                    enableW: settings1.item.enableW
                }

                MultibindingItem { object: object2; propertyName: "value";
                    enableR: settings2.item.enableR
                    enableW: settings2.item.enableW
                }
            }
        }
    }

    TestCase {
        name: "MultibindingTest"

        function test_00_basic() {
            compare(object1.value, "value1");
            compare(object2.value, "value2");
            mbLoader.active = true;
            compare(object1.value, "value1");
            compare(object2.value, "value1");
            object1.value = "123";
            compare(object2.value, "123");
            object2.value = "11100";
            compare(object1.value, "11100");

            //---
            mbLoader.active = false;
            wait(1);
            object1.value = "value1";
            object2.value = "value2";
            settings2.item.enableR = false;
            //---

            compare(object1.value, "value1");
            compare(object2.value, "value2");
            mbLoader.active = true;
            compare(object1.value, "value1");
            compare(object2.value, "value1");
            object2.value = "123";
            compare(object1.value, "value1");

            //---
            mbLoader.active = false;
            wait(1);
            object1.value = "value1";
            object2.value = "value2";
            settings2.item.enableW = false;
            //---

            compare(object1.value, "value1");
            compare(object2.value, "value2");
            mbLoader.active = true;
            compare(object1.value, "value1");
            compare(object2.value, "value2");
            object1.value = "123";
            compare(object2.value, "value2");
        }
    }
}
