import QtQuick 2.6
import Sailfish.Silica 1.0

Page {
    id: page

    allowedOrientations: Orientation.Portrait

    RemorsePopup { id: remorse }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: column.height + Theme.paddingLarge

        Column {
            id: column
            width: parent.width

            PageHeader { title: qsTr("Settings") }

            ComboBox {
                label: qsTr("Bot")
                currentIndex: game.difficulty
                description: [
                    qsTr("Lays away and plays at random."),
                    qsTr("Keeps the best four cards and grabs points in the play."),
                    qsTr("Weighs every possible starter and the crib, and plays to deny you points.")
                ][currentIndex] + " " + qsTr("Takes effect in the next game.")
                menu: ContextMenu {
                    MenuItem { text: qsTr("Easy") }
                    MenuItem { text: qsTr("Medium") }
                    MenuItem { text: qsTr("Hard") }
                }
                onCurrentIndexChanged: game.difficulty = currentIndex
            }

            ComboBox {
                label: qsTr("Scoring speed")
                currentIndex: game.scoringSpeed
                description: qsTr("How long each score stays on the table before the next.")
                menu: ContextMenu {
                    MenuItem { text: qsTr("Slow") }
                    MenuItem { text: qsTr("Normal") }
                    MenuItem { text: qsTr("Fast") }
                    MenuItem { text: qsTr("Very fast") }
                }
                onCurrentIndexChanged: game.scoringSpeed = currentIndex
            }

            SectionHeader { text: qsTr("Statistics") }

            Repeater {
                model: game.stats
                DetailItem {
                    label: [qsTr("Easy"), qsTr("Medium"), qsTr("Hard")][index]
                    value: qsTr("%1 won, %2 lost").arg(modelData.won).arg(modelData.lost)
                           + (modelData.skunks > 0 ? " · " + qsTr("%n skunk(s)", "", modelData.skunks) : "")
                }
            }

            Item { width: 1; height: Theme.paddingLarge }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Reset statistics")
                onClicked: remorse.execute(qsTr("Resetting statistics"), function() { game.resetStats() })
            }
        }
        VerticalScrollDecorator { }
    }
}
