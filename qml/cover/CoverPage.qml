import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.cribbage 1.0
import "../cards/cards.js" as Cards

CoverBackground {
    Column {
        anchors.centerIn: parent
        width: parent.width - 2 * Theme.paddingLarge
        spacing: Theme.paddingMedium

        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("Cribbage")
            color: Theme.secondaryColor
            font.pixelSize: Theme.fontSizeSmall
        }

        Repeater {
            model: [
                { name: qsTr("You"), score: game.humanScore, color: Cards.playerColor(0) },
                { name: qsTr("Bot"), score: game.botScore, color: Cards.playerColor(1) }
            ]
            Item {
                width: parent.width
                height: scoreLabel.height
                Label {
                    anchors.verticalCenter: parent.verticalCenter
                    text: modelData.name
                    color: modelData.color
                }
                Label {
                    id: scoreLabel
                    anchors.right: parent.right
                    text: modelData.score
                    color: modelData.color
                    font.pixelSize: Theme.fontSizeHuge
                    font.family: Theme.fontFamilyHeading
                }
            }
        }

        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSizeExtraSmall
            color: Theme.secondaryColor
            text: {
                switch (game.phase) {
                case Cribbage.Cut: return qsTr("Cutting for deal")
                case Cribbage.Discard: return qsTr("Laying away")
                case Cribbage.Play: return qsTr("Count %1").arg(game.count)
                case Cribbage.Show: return qsTr("Counting hands")
                }
                return game.winner === 0 ? qsTr("You won") : qsTr("The bot won")
            }
        }
    }

    CoverActionList {
        enabled: game.phase === Cribbage.GameOver
        CoverAction {
            iconSource: "image://theme/icon-cover-new"
            onTriggered: game.newGame()
        }
    }
}
