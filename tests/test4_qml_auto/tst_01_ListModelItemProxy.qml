/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

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

    ListModelItemProxy {
        id: proxy
        model: listModel
        index: 1
    }

    TestCase {
        name: "ListModelItemProxyTest"

        function test_accessItemRoles() {
            compare(proxy.propertyMap.stringValue, "str2");
            compare(proxy.propertyMap.intValue, 22);
            proxy.propertyMap.intValue = 222;
            compare(listModel.get(1).intValue, 222);
        }
    }
}
