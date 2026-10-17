import QtQuick 2.6
import Sailfish.Silica 1.0

// One player's row on the board: name, a 121-hole street with front and
// back peg, and the score.
Item {
    id: root

    property string name
    property string detail
    property int score
    property int backPeg
    property bool hasCrib
    property color pegColor: Theme.highlightColor

    width: parent.width
    height: Theme.itemSizeSmall

    property int _shown: 0
    onScoreChanged: {
        var gained = score - _shown
        _shown = score
        if (gained > 0) {
            gain.text = "+" + gained
            gainAnimation.restart()
        }
    }

    Column {
        id: nameColumn
        width: Theme.itemSizeExtraLarge
        anchors.verticalCenter: parent.verticalCenter

        Label {
            width: parent.width
            text: root.name
            color: root.pegColor
            font.pixelSize: Theme.fontSizeMedium
            truncationMode: TruncationMode.Fade
        }
        Label {
            width: parent.width
            text: root.hasCrib ? qsTr("Crib") + (root.detail ? " · " + root.detail : "") : root.detail
            color: Theme.secondaryColor
            font.pixelSize: Theme.fontSizeExtraSmall
            truncationMode: TruncationMode.Fade
        }
    }

    Item {
        id: track
        anchors {
            left: nameColumn.right
            leftMargin: Theme.paddingMedium
            right: scoreLabel.left
            rightMargin: Theme.paddingMedium
            verticalCenter: parent.verticalCenter
        }
        height: Theme.paddingLarge

        readonly property real step: width / 121

        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width
            height: Math.max(2, Math.round(Theme.pixelRatio * 2))
            radius: height / 2
            color: Theme.rgba(Theme.primaryColor, 0.15)
        }
        Rectangle {
            anchors.verticalCenter: parent.verticalCenter
            width: track.step * root.score
            height: Math.max(2, Math.round(Theme.pixelRatio * 2))
            radius: height / 2
            color: root.pegColor
            Behavior on width { NumberAnimation { duration: 400; easing.type: Easing.InOutQuad } }
        }

        // Every fifth hole, with the skunk lines at 61 and 91 drawn taller.
        Repeater {
            model: 24
            Rectangle {
                readonly property int hole: (index + 1) * 5
                readonly property bool skunkLine: hole === 60 || hole === 90
                x: track.step * (hole + (skunkLine ? 1 : 0)) - width / 2
                anchors.verticalCenter: parent.verticalCenter
                width: Math.max(1, Math.round(Theme.pixelRatio))
                height: skunkLine ? track.height : track.height / 3
                color: Theme.rgba(Theme.primaryColor, skunkLine ? 0.5 : 0.3)
            }
        }

        Rectangle {
            id: back
            width: Theme.paddingMedium
            height: width
            radius: width / 2
            anchors.verticalCenter: parent.verticalCenter
            x: track.step * root.backPeg - width / 2
            color: "transparent"
            border.color: root.pegColor
            border.width: Math.max(1, Math.round(Theme.pixelRatio * 1.5))
            visible: root.backPeg !== root.score
            Behavior on x { NumberAnimation { duration: 400; easing.type: Easing.InOutQuad } }
        }
        Rectangle {
            width: Theme.paddingMedium + Theme.paddingSmall
            height: width
            radius: width / 2
            anchors.verticalCenter: parent.verticalCenter
            x: track.step * root.score - width / 2
            color: root.pegColor
            Behavior on x { NumberAnimation { duration: 400; easing.type: Easing.InOutQuad } }
        }
    }

    Label {
        id: gain
        anchors.right: scoreLabel.left
        anchors.rightMargin: Theme.paddingSmall
        y: (root.height - height) / 2
        opacity: 0
        color: root.pegColor
        font.pixelSize: Theme.fontSizeMedium
        font.bold: true
    }

    ParallelAnimation {
        id: gainAnimation
        NumberAnimation { target: gain; property: "opacity"; from: 1; to: 0; duration: 1400; easing.type: Easing.InQuad }
        NumberAnimation {
            target: gain; property: "y"
            from: (root.height - gain.height) / 2; to: (root.height - gain.height) / 2 - Theme.paddingLarge
            duration: 1400; easing.type: Easing.OutQuad
        }
    }

    Label {
        id: scoreLabel
        anchors { right: parent.right; verticalCenter: parent.verticalCenter }
        width: Theme.itemSizeSmall
        horizontalAlignment: Text.AlignRight
        text: root.score
        color: root.pegColor
        font.pixelSize: Theme.fontSizeLarge
        font.family: Theme.fontFamilyHeading
    }
}
