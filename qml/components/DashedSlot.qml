import QtQuick 2.6
import Sailfish.Silica 1.0

// An empty place on the table, outlined with dashes, where cards will go.
Item {
    id: root

    property color color: Theme.highlightColor
    property real dash: Theme.paddingMedium
    property real gap: Theme.paddingSmall
    property real thickness: Math.max(2, Math.round(Theme.pixelRatio * 2))

    // Dashes along a side, spread so both ends get one.
    function dashCount(length) { return Math.max(2, Math.floor((length + gap) / (dash + gap))) }

    Rectangle {
        anchors.fill: parent
        color: Theme.rgba(root.color, 0.1)
    }

    // Top and bottom.
    Repeater {
        model: root.dashCount(root.width) * 2
        Rectangle {
            readonly property int n: root.dashCount(root.width)
            x: (index % n) * (root.width - root.dash) / (n - 1)
            y: index < n ? 0 : root.height - root.thickness
            width: root.dash
            height: root.thickness
            color: root.color
        }
    }

    // Left and right.
    Repeater {
        model: root.dashCount(root.height) * 2
        Rectangle {
            readonly property int n: root.dashCount(root.height)
            x: index < n ? 0 : root.width - root.thickness
            y: (index % n) * (root.height - root.dash) / (n - 1)
            width: root.thickness
            height: root.dash
            color: root.color
        }
    }
}
