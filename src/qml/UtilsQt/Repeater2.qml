import QtQuick 2.15

Item {
    id: root

    property int count: 0
    property Component delegate: null
    property Item target: parent
    property bool instantRemoval: false
    property bool asynchronous: false

    onTargetChanged: internal.ready && internal.allowCallLater ? Qt.callLater(internal.update) : internal.update()
    onDelegateChanged: internal.ready && internal.allowCallLater ? Qt.callLater(internal.update) : internal.update()

    QtObject {
        id: internal

        property int createdCount: 0
        property var createdItems: []

        readonly property bool allowCallLater: true
        property bool ready: false
        property int currentCount: 0

        property Component complexComponent: Component {
            Loader {
                readonly property int index: __repeater2_internal_index
                readonly property bool isValid: __repeater2_internal_isValid

                property int __repeater2_internal_index: -1
                property bool __repeater2_internal_isValid: true

                sourceComponent: root.delegate
                asynchronous: root.asynchronous
            }
        }

        function update() {
            if (!internal.ready) return;
            internal.currentCount = 0;
            if (!root.delegate) return;
            internal.currentCount = Qt.binding(function(){ return root.count; });
        }

        onCurrentCountChanged: {
            if (currentCount > internal.createdCount) {
                for (var i = internal.createdCount; i < currentCount; i++) {
                    console.debug("Creating delegate #" + i + "...");
                    var obj = internal.complexComponent.createObject(root.target, {"__repeater2_internal_index": i, "__repeater2_internal_isValid": true});
                    internal.createdItems.push(obj);
                }

                internal.createdCount = currentCount;

            } else if (currentCount < internal.createdCount) {
                var removeCount = internal.createdCount - currentCount;

                for (i = 0; i < removeCount; i++) {
                    console.debug("Removing delegate #" + (internal.createdItems.length - 1) + "...");

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
        internal.currentCount = Qt.binding(function(){ return root.count; });
    }
}
