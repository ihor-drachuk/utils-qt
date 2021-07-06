import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import UtilsQt 1.0
import CppTypes 1.0

ApplicationWindow {
    width: 800
    height: 600
    visible: true

    RowLayout {
        ColumnLayout {
            Layout.minimumWidth: 20

            ColumnLayout {
                Repeater2 {
                    model: 5
                    delegate: Label { text: index }
                }
            }

            Item { Layout.fillHeight: true }
        }

        ColumnLayout {
            ListModel {
                id: listModel

                ListElement {
                    stringValue: "First"
                    intValue: 1
                }

                ListElement {
                    stringValue: "Second"
                    intValue: 2
                }

                ListElement {
                    stringValue: "Third"
                    intValue: 3
                }
            }

            Timer {
                repeat: true
                running: true
                interval: 1000
                onTriggered: {
                    var item = listModel.get(0);
                    item.intValue++;
                    listModel.set(0, item);
                }
            }

            ListView {
                Layout.alignment: Qt.AlignTop
                implicitWidth: 50
                implicitHeight: 50
                model: listModel
                delegate: Label { text: index + " " + stringValue + " " + intValue }
            }

            ListView {
                implicitWidth: 50
                implicitHeight: 50
                model: listModel
                delegate: Label {
                    id: label
                    //text: index + " " + stringValue + " " + intValue

                    readonly property int idx: index

                    text: index + " " + proxy.ready ? (proxy.propertyMap.stringValue + " " + proxy.propertyMap.intValue) : ""

                    SequentialAnimation on color {
                        id: anim
                        ColorAnimation { to: "red"; duration: 100 }
                        ColorAnimation { to: "black"; duration: 100 }
                    }

                    ListModelItemProxy {
                        id: proxy
                        model: listModel
                        index: idx
                        keepIndexTrack: true

                        onChanged: anim.start();
                    }

                    Item {
                        RowLayout {
                            x: 60

                            Button {
                                implicitHeight: label.height
                                text: "Update"
                                onClicked: proxy.propertyMap.intValue++;
                            }

                            Button {
                                implicitHeight: label.height
                                text: "Remove"
                                onClicked: listModel.remove(label.idx);
                            }
                        }
                    }
                }
            }

            Label {
                text: pr.index + " " + pr.ready ? (pr.propertyMap.stringValue + " " + pr.propertyMap.intValue) : ""

                ListModelItemProxy {
                    id: pr
                    model: listModel
                    index: 1
                    onSuggestedNewIndex: index = newIndex;
                }

                Timer {
                    repeat: true
                    running: true
                    interval: 1000
                    onTriggered: pr.propertyMap.intValue++;
                }
            }

            Item { Layout.fillHeight: true }
        }
    }
}
