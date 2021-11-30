import QtQuick 2.15

Item {
    id: root
    property int radius: 0
    property color color: "#FFFFFF"
    property color borderColor: "#000000"
    property int borderWidth: 1

    property var dash: null

    function repaint() {
        canvas.requestPaint();
    }

    QtObject {
        readonly property var deps: [root.radius, root.color, root.borderColor, root.borderWidth, root.dash]
        onDepsChanged: root.repaint();
    }

    Canvas {
        id: canvas
        anchors.fill: parent

        onPaint: {
            var ctx = getContext("2d");
            ctx.reset();

            // Workaround for https://bugreports.qt.io/browse/QTBUG-75553
            if (!root.dash || (typeof root.dash === "object" && root.dash.length === 0)) {
                ctx.setLineDash([1000, 0]);
            } else {
                ctx.setLineDash(root.dash);
            }

            ctx.lineWidth = root.borderWidth;
            ctx.fillStyle = root.color;
            ctx.strokeStyle = root.borderColor;

            if (root.radius) {
                ctx.roundedRect(0 + root.borderWidth/2, 0 + root.borderWidth/2, width - root.borderWidth, height - root.borderWidth, radius, radius);
            } else {
                ctx.rect(0 + root.borderWidth/2, 0 + root.borderWidth/2, width - root.borderWidth, height - root.borderWidth);
            }

            ctx.fill();
            ctx.stroke();
        }
    }
}
