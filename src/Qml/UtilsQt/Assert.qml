/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

import QtQuick 2.15

Item {
    id: root

    required property string name
    required property bool condition

    objectName: name

    Component.onCompleted: {
        if (!condition) {
            var errorMsg = "Failed assert (%1)!".arg(root.objectName);
            throw errorMsg;
        }
    }
}
