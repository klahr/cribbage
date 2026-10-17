import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    allowedOrientations: Orientation.Portrait

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width
            spacing: Theme.paddingMedium

            PageHeader { title: qsTr("Cribbage") }

            Repeater {
                model: [
                    { title: qsTr("The game"), text: qsTr("Two players race to 121 points. The game ends the moment either player reaches 121, even in the middle of a hand. Losing with fewer than 91 points is a skunk, fewer than 61 a double skunk.") },
                    { title: qsTr("The deal"), text: qsTr("Both players cut the pack and the lower card deals first, ace low; a tie is cut again. Each player gets six cards and lays away two to the crib, which belongs to the dealer. The deal alternates. The starter is then cut; if it is a jack the dealer pegs two for his heels.") },
                    { title: qsTr("The play"), text: qsTr("Starting with the non-dealer, players lay cards in turn, calling the running count, which may not pass 31. Fifteen and 31 score two. A pair scores two, three of a kind six and four of a kind twelve. A run of three or more cards, in any order, scores one per card. A player who cannot play says go; the other plays on as far as possible and pegs one for go, or two for exactly 31. Then the count starts again. The last card of the play scores one.") },
                    { title: qsTr("The show"), text: qsTr("The non-dealer counts first, then the dealer, then the crib, each with the starter. Every combination adding up to fifteen scores two, each pair two, each run one per card. Four cards of one suit in the hand score four, five with the starter; the crib only scores a flush of five. The jack of the starter's suit scores one for his nobs.") },
                    { title: qsTr("Playing here"), text: qsTr("To cut for deal, tap any card in the spread pack. Tap two cards to lay them away and then tap the button. During the play, tap a card to lay it down; cards that would pass 31 are dimmed. Your points are shown in blue and the bot's in red. The board shows each player's front and back peg; the taller marks are the skunk lines.") }
                ]
                Column {
                    width: column.width
                    SectionHeader { text: modelData.title }
                    Label {
                        x: Theme.horizontalPageMargin
                        width: parent.width - 2 * Theme.horizontalPageMargin
                        wrapMode: Text.Wrap
                        text: modelData.text
                        color: Theme.highlightColor
                        font.pixelSize: Theme.fontSizeSmall
                    }
                }
            }

            SectionHeader { text: qsTr("About") }
            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                wrapMode: Text.Wrap
                text: qsTr("Cribbage %1 for Sailfish OS.").arg(Qt.application.version)
                color: Theme.secondaryHighlightColor
                font.pixelSize: Theme.fontSizeSmall
            }
        }
        VerticalScrollDecorator { }
    }
}
