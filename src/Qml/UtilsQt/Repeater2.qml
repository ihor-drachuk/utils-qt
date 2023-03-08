import QtQuick 2.15
import UtilsQt 1.0

Item {
    id: root

    // Attached properties:
    //  - index
    //  - isValid
    //  - modelData

    property var model: null
    default property Component delegate: null
    property Item target: parent
    property bool instantRemoval: false
    property bool asynchronous: false

    onTargetChanged: internal.ready && internal.allowCallLater ? Qt.callLater(internal.update) : internal.update()
    onDelegateChanged: internal.ready && internal.allowCallLater ? Qt.callLater(internal.update) : internal.update()

    LoggingCategory {
        id: category
        name: "Repeater2" + (root.objectName ? " (" + root.objectName + ")" : "")
    }

    ListModelTools {
        id: modelTools

        model: (typeof root.model) === "number" ? null : root.model
    }

    QtObject {
        id: internal

        readonly property int modelCount: (typeof root.model) === "number" ? root.model : modelTools.itemsCount

        property int createdCount: 0
        property var createdItems: []

        readonly property bool allowCallLater: true
        property bool ready: false
        property int currentCount: 0

        property Component complexComponent: Component {
            Loader {
                id: __repeater2_internal_loader
                readonly property int index: __repeater2_internal_index
                readonly property bool isValid: __repeater2_internal_isValid
                readonly property var modelData: __repeater2_internal_.propertyMap

                property int __repeater2_internal_index: -1
                property bool __repeater2_internal_isValid: true
                property ListModelItemProxy __repeater2_internal_: ListModelItemProxy {
                    index: __repeater2_internal_loader.index
                    model: modelTools.model
                }

                sourceComponent: root.delegate
                asynchronous: root.asynchronous
            }
        }

        function update() {
            if (!internal.ready) return;
            internal.currentCount = 0;
            if (!root.delegate) return;
            internal.currentCount = Qt.binding(function(){ return internal.modelCount; });
        }

        onCurrentCountChanged: {
            if (currentCount > internal.createdCount) {
                for (var i = internal.createdCount; i < currentCount; i++) {
                    console.debug(category, "Creating delegate #" + i + "...");
                    var obj = internal.complexComponent.createObject(root.target, {"__repeater2_internal_index": i, "__repeater2_internal_isValid": true});
                    internal.createdItems.push(obj);
                }

                internal.createdCount = currentCount;

            } else if (currentCount < internal.createdCount) {
                var removeCount = internal.createdCount - currentCount;

                for (i = 0; i < removeCount; i++) {
                    console.debug(category, "Removing delegate #" + (internal.createdItems.length - 1) + "...");

                    obj = internal.createdItems.pop();

                    obj.__repeater2_internal_isValid = false;
                    if (root.instantRemoval) obj.active = false;

                    obj.destroy();
                }

                internal.createdCount = currentCount;
            }
        }
    }

    Component.onCompleted: {
        internal.ready = true;
        if (!root.delegate) return;
        internal.currentCount = Qt.binding(function(){ return internal.modelCount; });
        console.debug(category, "Repeater2 created (%1)".arg(root));
    }

    Component.onDestruction: {
        console.debug(category, "Repeater2 destroying (%1)".arg(root));
    }
}
