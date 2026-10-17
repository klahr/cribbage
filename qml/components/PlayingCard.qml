import QtQuick 2.6
import Sailfish.Silica 1.0
import "../cards.js" as Cards

// A playing card drawn in QML. cardId -1 shows the back.
MouseArea {
    id: root

    property int cardId: -1
    property bool faceDown
    property bool selected
    property bool dimmed
    // Part of the score being shown right now.
    property bool glowing
    property color glowColor: Theme.highlightColor
    // Follows faceDown halfway through the flip.
    property bool _back: cardId < 0 || faceDown
    readonly property bool faceUp: !_back
    readonly property bool red: faceUp && Cards.isRed(cardId)
    readonly property color ink: red ? "#c62828" : "#1b1b1b"

    width: Theme.itemSizeLarge
    height: Math.round(width * 1.42)
    enabled: faceUp

    onFaceDownChanged: flip.restart()

    transform: Scale {
        id: flipScale
        origin.x: root.width / 2
    }

    SequentialAnimation {
        id: flip
        NumberAnimation { target: flipScale; property: "xScale"; to: 0; duration: 110; easing.type: Easing.InQuad }
        ScriptAction { script: root._back = root.cardId < 0 || root.faceDown }
        NumberAnimation { target: flipScale; property: "xScale"; to: 1; duration: 110; easing.type: Easing.OutQuad }
    }

    // Soft halo behind a card that just scored.
    Rectangle {
        anchors { fill: face; margins: -Theme.paddingSmall }
        radius: face.radius + Theme.paddingSmall
        color: Theme.rgba(root.glowColor, 0.5)
        opacity: root.glowing ? 1 : 0
        Behavior on opacity { FadeAnimation { duration: 180 } }
    }

    Rectangle {
        id: face

        width: parent.width
        height: parent.height
        y: root.selected || root.glowing ? -Theme.paddingLarge : 0
        radius: Math.round(root.width * 0.08)
        color: root.faceUp ? "#fbfaf6" : Theme.highlightDimmerColor
        border.width: root.selected || root.glowing ? Math.max(2, Math.round(Theme.pixelRatio * 3)) : 1
        border.color: root.glowing ? root.glowColor
                      : root.selected ? Theme.highlightColor
                      : (root.faceUp ? Qt.rgba(0, 0, 0, 0.25) : Theme.highlightColor)

        Behavior on y { NumberAnimation { duration: 120; easing.type: Easing.OutQuad } }

        // Back: a quiet highlight-tinted lattice in the ambience colours.
        Rectangle {
            visible: !root.faceUp
            anchors { fill: parent; margins: Math.round(root.width * 0.08) }
            radius: Math.round(root.width * 0.04)
            color: "transparent"
            border.color: Theme.rgba(Theme.highlightColor, 0.6)
            border.width: 1
            clip: true

            Grid {
                anchors.centerIn: parent
                columns: 3
                spacing: Math.round(root.width * 0.06)
                Repeater {
                    model: 9
                    Label {
                        text: ["♠", "♥", "♦", "♣"][index % 4]
                        color: Theme.rgba(Theme.highlightColor, 0.55)
                        font.pixelSize: Math.round(root.width * 0.18)
                    }
                }
            }
        }

        Column {
            visible: root.faceUp
            x: Math.round(root.width * 0.07)
            y: Math.round(root.width * 0.04)
            spacing: -Math.round(root.width * 0.06)

            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: root.faceUp ? Cards.rankText(root.cardId) : ""
                color: root.ink
                font.pixelSize: Math.round(root.width * 0.3)
                font.bold: true
                font.family: Theme.fontFamilyHeading
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                text: root.faceUp ? Cards.suitText(root.cardId) : ""
                color: root.ink
                font.pixelSize: Math.round(root.width * 0.24)
            }
        }

        // Court cards get a framed panel instead of pips.
        Rectangle {
            visible: root.faceUp && Cards.isFace(root.cardId)
            anchors {
                fill: parent
                leftMargin: Math.round(root.width * 0.3)
                rightMargin: Math.round(root.width * 0.1)
                topMargin: Math.round(root.height * 0.12)
                bottomMargin: Math.round(root.height * 0.08)
            }
            radius: Math.round(root.width * 0.04)
            color: root.red ? "#fdecea" : "#eceff1"
            border.color: root.ink
            border.width: 1
        }

        Text {
            visible: root.faceUp
            anchors {
                horizontalCenter: parent.horizontalCenter
                horizontalCenterOffset: Math.round(root.width * 0.1)
                verticalCenter: parent.verticalCenter
                verticalCenterOffset: Math.round(root.height * 0.1)
            }
            text: root.faceUp
                  ? (Cards.isFace(root.cardId) ? Cards.rankText(root.cardId) : Cards.suitText(root.cardId))
                  : ""
            color: root.ink
            font.pixelSize: Math.round(root.width * (Cards.isFace(root.cardId) ? 0.5 : 0.6))
            font.family: Theme.fontFamilyHeading
            font.bold: Cards.isFace(root.cardId)
        }
    }

    // Opaque shade, so overlapping dimmed cards don't show through.
    Rectangle {
        anchors.fill: face
        radius: face.radius
        visible: root.dimmed && root.faceUp
        color: Qt.rgba(0, 0, 0, 0.5)
    }

    Rectangle {
        anchors.fill: face
        radius: face.radius
        color: Theme.highlightBackgroundColor
        opacity: root.pressed && root.faceUp ? Theme.highlightBackgroundOpacity : 0
    }
}
