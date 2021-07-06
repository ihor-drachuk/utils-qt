import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import UtilsQt 1.0

ApplicationWindow {
    width: 800
    height: 600
    visible: true

    ColumnLayout {
        Repeater2 {
            count: 5
            delegate: Label { text: index }
        }
    }
}
