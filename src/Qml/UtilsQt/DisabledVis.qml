/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/utils-qt
 * Contact:  ihor-drachuk-libs@pm.me  */

import QtQuick 2.15
import QtGraphicalEffects 1.2

Item {
    id: root

    property real desaturation: 1.0

    Binding {
        target: parent
        property: "opacity"
        value: parent.enabled ? 1 : 0.5
    }

    Binding {
        target: parent.layer
        property: "enabled"
        value: !parent.enabled && parent.width && parent.height
    }

    Component {
        id: desaturate

        Desaturate { desaturation: root.desaturation }
    }

    Component.onCompleted: parent.layer.effect = desaturate;
}
