import QtQuick 2.6
import Sailfish.Silica 1.0
import harbour.cribbage 1.0
import "../components"
import "../cards.js" as Cards

Page {
    id: page

    property var selected: []
    // One card size everywhere: six overlapping cards fill the width.
    readonly property real cardWidth: Math.floor((width - 2 * Theme.horizontalPageMargin) / (1 + 5 * 0.76))
    readonly property real cardHeight: Math.round(cardWidth * 1.42)
    readonly property real margin: Theme.horizontalPageMargin
    readonly property bool humanCrib: game.dealer === 0
    readonly property bool showing: game.phase === Cribbage.Show
                                    || (game.phase === Cribbage.GameOver && game.endedInShow)
    // Cards are laid out for the show once the play's last points are shown.
    readonly property bool tableShowing: showing && heldPlayed === null
    // The bot's hand is turned up once its turn in the show has come.
    readonly property bool botHandShown: tableShowing && game.showStep >= (game.dealer === 1 ? 1 : 0)

    // Scores are shown one at a time: the cards that made each one rise and
    // glow, its name pops up above them and the peg moves.
    property var revealQueue: []
    property var current: null
    property int revealedCount: 0     // show items revealed so far
    property var scoreBase: [0, 0]    // scores before the action being shown
    property var revealedPoints: [0, 0]
    property var backPegs: [0, 0]
    property var lastScores: [0, 0]
    property var heldPlayed: null     // the count kept on the table while it is scored
    property var previousPlayed: []
    property int shownStepKey: -1
    readonly property bool revealing: current !== null || revealQueue.length > 0
    readonly property string panel: (game.phase === Cribbage.Show
                                     || (game.phase === Cribbage.GameOver && game.endedInShow && revealing))
                                    && heldPlayed === null
                                    ? "show"
                                    : game.phase === Cribbage.GameOver && !revealing ? "over" : "message"

    readonly property var difficultyNames: [qsTr("Easy"), qsTr("Medium"), qsTr("Hard")]
    readonly property var places: computePlaces()

    allowedOrientations: Orientation.Portrait

    function toggle(id) {
        var s = selected.slice()
        var i = s.indexOf(id)
        if (i >= 0)
            s.splice(i, 1)
        else if (s.length < 2)
            s.push(id)
        else
            s = [s[1], id]
        selected = s
    }

    function startNewGame() {
        if (game.phase === Cribbage.GameOver || game.phase === Cribbage.Cut
                || game.round <= 1 && game.phase === Cribbage.Discard && !game.humanDiscarded)
            game.newGame()
        else
            remorse.execute(qsTr("Starting a new game"), function() { game.newGame() })
    }

    function shownScore(player) {
        var real = player === 0 ? game.humanScore : game.botScore
        return revealing ? Math.min(real, scoreBase[player] + revealedPoints[player]) : real
    }

    function plainList(list) {
        var out = []
        for (var i = 0; i < list.length; ++i)
            out.push(list[i])
        return out
    }

    function scoreItem(entry, player, show) {
        return { player: player, name: entry.name, points: entry.points, cards: plainList(entry.cards), show: show }
    }

    // Queues the points of the latest action, and of a hand that has just
    // come up for counting.
    function updateReveal() {
        var i
        var played = game.played
        var items = []
        var scoring = game.lastScoring
        for (i = 0; i < scoring.length; ++i)
            items.push(scoreItem(scoring[i], scoring[i].player, false))

        var key = game.round * 10 + game.showStep
        var newShow = showing && key !== shownStepKey
        if (showing)
            shownStepKey = key
        if (newShow) {
            var shown = game.showItems
            for (i = 0; i < shown.length; ++i)
                items.push(scoreItem(shown[i], game.showOwner, true))
        }

        // Keep a finished count on the table until its points are shown.
        var hold = null
        if (scoring.length > 0) {
            var wasCurrent = {}
            var seen = {}
            var reset = false
            for (i = 0; i < previousPlayed.length; ++i) {
                seen[previousPlayed[i].card] = true
                if (previousPlayed[i].current)
                    wasCurrent[previousPlayed[i].card] = true
            }
            for (i = 0; i < played.length; ++i)
                if (wasCurrent[played[i].card] && !played[i].current)
                    reset = true
            if (reset || showing) {
                hold = []
                for (i = 0; i < played.length; ++i)
                    hold.push({ card: played[i].card, player: played[i].player,
                                current: wasCurrent[played[i].card] === true || seen[played[i].card] !== true })
            }
        }
        previousPlayed = plainList(played)

        var pegs = backPegs.slice()
        for (i = 0; i < items.length; ++i)
            pegs[items[i].player] = lastScores[items[i].player]
        if (game.round <= 1 && game.humanScore === 0 && game.botScore === 0)
            pegs = [0, 0]
        backPegs = pegs

        // Queue first, so the board never shows the new score before its reveal.
        revealTimer.stop()
        heldPlayed = hold
        revealQueue = items
        current = null
        scoreBase = lastScores
        lastScores = [game.humanScore, game.botScore]
        revealedPoints = [0, 0]
        revealedCount = newShow ? 0 : game.showItems.length
        if (items.length === 0)
            return
        if (items[0].show) {
            revealTimer.interval = 450   // let the cards reach the counting row
            revealTimer.restart()
        } else {
            advanceReveal()
        }
    }

    function advanceReveal() {
        if (revealQueue.length === 0) {
            current = null
            heldPlayed = null
            return
        }
        var next = revealQueue[0]
        if (next.show && heldPlayed !== null) {
            // The play's points are done: lay the hands out for the show first.
            heldPlayed = null
            current = null
            revealTimer.interval = 450
            revealTimer.restart()
            return
        }
        revealQueue = revealQueue.slice(1)
        var points = revealedPoints.slice()
        points[next.player] += next.points
        revealedPoints = points
        if (next.show)
            revealedCount += 1
        current = next
        revealTimer.interval = game.revealMs
        revealTimer.restart()
    }

    function groupedItems(items) {
        var groups = []
        var byName = {}
        for (var i = 0; i < items.length; ++i) {
            var it = items[i]
            if (byName[it.name] === undefined) {
                byName[it.name] = groups.length
                groups.push({ name: it.name, times: 0, points: 0 })
            }
            var g = groups[byName[it.name]]
            g.times += 1
            g.points += it.points
        }
        return groups
    }

    // Where every one of the 52 cards lies, in content coordinates.
    function computePlaces() {
        var cw = cardWidth
        var w = page.width
        var showing = tableShowing
        var row1 = cluster.y + topRow.y
        var row2 = cluster.y + tableRow.y
        var handY = cluster.y + handRow.y + Theme.paddingLarge
        var deckX = margin
        // The deck and starter stay on the play's row; played and counted
        // cards lie to the right of them.
        var deckY = row2 + Theme.paddingLarge
        var pileX = margin + cw + Theme.paddingLarge
        var p = []
        var i, n, off, x0, ids

        if (game.phase === Cribbage.Cut) {
            // The pack spread face down in one line across the table; the
            // cards drawn turn up, yours towards you and the bot's towards it.
            ids = game.cutDeck
            off = (w - 2 * margin - cw) / Math.max(1, ids.length - 1)
            for (i = 0; i < 52; ++i)
                p.push({ x: deckX, y: row2 + Theme.paddingLarge, z: 1, down: true, role: "deck", delay: 0 })
            for (i = 0; i < ids.length; ++i)
                p[ids[i]] = { x: margin + i * off, y: row2 + Theme.paddingLarge, z: 10 + i, down: true,
                              role: "cut", line: i, delay: i * 8 }
            if (game.humanCut >= 0)
                p[game.humanCut] = { x: (w - cw) / 2, y: handY, z: 200, down: false, role: "drawn", owner: 0, delay: 0 }
            if (game.botCut >= 0)
                p[game.botCut] = { x: (w - cw) / 2, y: row1, z: 200, down: false, role: "drawn", owner: 1, delay: 0 }
            return p
        }

        for (i = 0; i < 52; ++i)
            p.push({ x: deckX + Theme.paddingSmall, y: deckY + Theme.paddingSmall, z: 1, down: true, role: "deck", delay: 0 })
        if (game.starter >= 0)
            p[game.starter] = { x: deckX, y: deckY, z: 2, down: false, role: "starter", delay: 0 }

        // The crib waits face down with the deck until it is counted; the
        // board shows whose it is.
        ids = game.crib
        for (i = 0; i < ids.length; ++i)
            p[ids[i]] = { x: deckX + Theme.paddingSmall, y: deckY + Theme.paddingSmall, z: 1, down: true, role: "crib", delay: i * 60 }

        // The bot's cards, fanned against the right edge.
        ids = showing ? game.botKept : game.botHand
        n = ids.length
        off = n > 1 ? Math.min(cw * 0.5, (w - margin - (deckX + 2 * cw + Theme.paddingLarge)) / (n - 1)) : 0
        for (i = 0; i < n; ++i)
            p[ids[i]] = { x: w - margin - cw - (n - 1 - i) * off, y: row1, z: 30 + i,
                          down: !botHandShown, role: "bot", delay: game.phase === Cribbage.Discard ? i * 60 : 0 }

        // Your cards, centred.
        ids = showing ? game.humanKept : game.hand
        n = ids.length
        off = n > 1 ? Math.min(cw * 0.76, (w - 2 * margin - cw) / (n - 1)) : 0
        x0 = (w - cw - (n - 1) * off) / 2
        for (i = 0; i < n; ++i)
            p[ids[i]] = { x: x0 + i * off, y: handY, z: 50 + i, down: false, role: showing ? "kept" : "hand",
                          delay: game.phase === Cribbage.Discard ? i * 60 : 0 }

        if (showing) {
            // The hand being counted, next to the starter.
            ids = game.showCards
            off = Math.min(cw * 0.78, (w - margin - pileX - cw) / 3)
            for (i = 0; i < ids.length; ++i)
                p[ids[i]] = { x: pileX + i * off, y: row2 + Theme.paddingLarge, z: 100 + i,
                              down: false, role: "show", delay: i * 50 }
        } else {
            // Finished counts are turned over and set aside, up beside the
            // bot's hand; the current count lies on the table next to the
            // starter, the bot's cards a little higher.
            var played = heldPlayed !== null ? heldPlayed : game.played
            var current = []
            var aside = 0
            for (i = 0; i < played.length; ++i) {
                if (played[i].current) {
                    current.push(played[i])
                } else {
                    p[played[i].card] = { x: margin + aside * Theme.paddingSmall / 2, y: row1 + aside * Theme.paddingSmall / 2,
                                          z: 20 + aside, down: true, role: "aside", delay: aside * 30 }
                    ++aside
                }
            }
            n = current.length
            off = n > 1 ? Math.min(cw * 0.45, (w - margin - pileX - cw) / (n - 1)) : 0
            for (i = 0; i < n; ++i)
                p[current[i].card] = { x: pileX + i * off, y: row2 + (current[i].player === 1 ? 0 : 2 * Theme.paddingLarge),
                                       z: 100 + i, down: false, role: "pile", delay: 0 }
        }
        return p
    }

    Connections {
        target: game
        onChanged: {
            if (game.phase !== Cribbage.Discard || game.humanDiscarded)
                page.selected = []
            page.updateReveal()
        }
    }

    Timer {
        id: revealTimer
        onTriggered: page.advanceReveal()
    }

    RemorsePopup { id: remorse }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: Math.max(page.height, cluster.y + cluster.height + Theme.paddingLarge)

        PullDownMenu {
            MenuItem {
                text: qsTr("About and rules")
                onClicked: pageStack.push(Qt.resolvedUrl("RulesPage.qml"))
            }
            MenuItem {
                text: qsTr("Settings")
                onClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"))
            }
            MenuItem {
                text: qsTr("New game")
                onClicked: page.startNewGame()
            }
        }

        Item {
            id: content
            width: parent.width
            height: parent.contentHeight

            // ---------------------------------------------------- board
            Column {
                id: board
                x: margin
                y: Theme.paddingLarge
                width: parent.width - 2 * margin

                ScoreTrack {
                    name: qsTr("You")
                    score: page.shownScore(0)
                    backPeg: page.backPegs[0]
                    hasCrib: game.phase !== Cribbage.Cut && game.dealer === 0
                    pegColor: Cards.playerColor(0)
                }
                ScoreTrack {
                    name: qsTr("Bot")
                    detail: page.difficultyNames[game.gameDifficulty]
                    score: page.shownScore(1)
                    backPeg: page.backPegs[1]
                    hasCrib: game.phase !== Cribbage.Cut && game.dealer === 1
                    pegColor: Cards.playerColor(1)
                }
            }

            // Everything else sits together in the middle of the screen, so
            // your hand is within easy reach of the thumb. The space is set
            // by the play layout, so the table does not move between phases.
            Column {
                id: cluster
                width: parent.width
                y: board.y + board.height
                   + Math.max(Theme.paddingLarge,
                              (page.height - board.y - board.height
                               - (height - info.height + messageBlock.height)) * 0.4)
                spacing: Theme.paddingMedium

                // Cards set aside and the bot's hand (drawn by the card layer).
                Item {
                    id: topRow
                    width: parent.width
                    height: cardHeight + Theme.paddingLarge
                }

                // The starter and the play, or the hand being counted.
                Item {
                    id: tableRow
                    width: parent.width
                    height: cardHeight + 2 * Theme.paddingLarge + countLabel.height

                    Label {
                        id: deckLabel
                        x: margin
                        y: Theme.paddingLarge + cardHeight + Theme.paddingSmall * 2
                        visible: game.phase !== Cribbage.Cut
                        text: game.starter >= 0 ? qsTr("Starter") : qsTr("Deck")
                        color: Theme.secondaryColor
                        font.pixelSize: Theme.fontSizeExtraSmall
                    }

                    Label {
                        anchors.centerIn: parent
                        anchors.horizontalCenterOffset: (cardWidth + Theme.paddingLarge) / 2
                        width: parent.width - 2 * (margin + cardWidth)
                        opacity: game.phase === Cribbage.Discard ? 1 : 0
                        visible: opacity > 0
                        Behavior on opacity { FadeAnimation { } }
                        wrapMode: Text.Wrap
                        horizontalAlignment: Text.AlignHCenter
                        text: game.humanDiscarded ? qsTr("Waiting for the cut") : qsTr("Lay away two cards")
                        color: Theme.secondaryHighlightColor
                        font.pixelSize: Theme.fontSizeLarge
                        font.family: Theme.fontFamilyHeading
                    }

                    Label {
                        id: countLabel
                        anchors { right: parent.right; rightMargin: margin; bottom: parent.bottom }
                        opacity: game.phase === Cribbage.Play ? 1 : 0
                        Behavior on opacity { FadeAnimation { } }
                        text: qsTr("Count %1").arg(game.count)
                        color: Theme.highlightColor
                        font.pixelSize: Theme.fontSizeExtraLarge
                        font.family: Theme.fontFamilyHeading
                        transformOrigin: Item.Right

                        property int shown: game.count
                        onShownChanged: if (shown > 0) pulse.restart()
                        SequentialAnimation {
                            id: pulse
                            NumberAnimation { target: countLabel; property: "scale"; to: 1.15; duration: 110; easing.type: Easing.OutQuad }
                            NumberAnimation { target: countLabel; property: "scale"; to: 1; duration: 180; easing.type: Easing.InOutQuad }
                        }
                    }
                }

                // Messages, the score of the counted hand, or the result.
                Item {
                    id: info
                    width: parent.width
                    height: page.panel === "show" ? showPanel.height
                                                   : page.panel === "over" ? overPanel.height : messageBlock.height

                    Column {
                        id: messageBlock
                        x: margin
                        width: parent.width - 2 * margin
                        opacity: page.panel === "message" ? 1 : 0
                        visible: opacity > 0
                        Behavior on opacity { FadeAnimation { } }

                        Label {
                            width: parent.width
                            height: fontMetrics.height
                            truncationMode: TruncationMode.Fade
                            color: Theme.secondaryColor
                            font.pixelSize: Theme.fontSizeSmall
                            text: {
                                switch (game.phase) {
                                case Cribbage.Cut:
                                    if (game.humanCut < 0)
                                        return qsTr("Cut for deal: tap a card. Lowest card deals.")
                                    if (game.botCut < 0)
                                        return qsTr("The bot is cutting…")
                                    if (game.cutTied)
                                        return qsTr("A tie. Cut again.")
                                    return game.cutDealer === 0 ? qsTr("Your card is lower. You deal.")
                                                                : qsTr("The bot's card is lower. The bot deals.")
                                case Cribbage.Discard:
                                    if (game.humanDiscarded)
                                        return ""
                                    return page.humanCrib ? qsTr("Choose two cards for your crib.")
                                                          : qsTr("Choose two cards for the bot's crib.")
                                case Cribbage.Play:
                                    if (game.mustSayGo)
                                        return qsTr("No card fits under 31.")
                                    if (game.humanTurn)
                                        return qsTr("Your turn. Tap a card to play it.")
                                    if (game.botTurn)
                                        return qsTr("The bot is thinking…")
                                }
                                return ""
                            }
                        }

                        FontMetrics {
                            id: fontMetrics
                            font.pixelSize: Theme.fontSizeSmall
                        }
                    }

                    Column {
                        id: showPanel
                        x: margin
                        width: parent.width - 2 * margin
                        opacity: page.panel === "show" ? 1 : 0
                        visible: opacity > 0
                        Behavior on opacity { FadeAnimation { } }

                        Item {
                            width: parent.width
                            height: Theme.itemSizeExtraSmall
                            Label {
                                anchors.verticalCenter: parent.verticalCenter
                                text: game.showStep === 2
                                      ? (game.showOwner === 0 ? qsTr("Your crib") : qsTr("Bot's crib"))
                                      : (game.showOwner === 0 ? qsTr("Your hand") : qsTr("Bot's hand"))
                                color: Cards.playerColor(game.showOwner)
                                font.pixelSize: Theme.fontSizeLarge
                                font.family: Theme.fontFamilyHeading
                            }
                            Label {
                                anchors { right: parent.right; verticalCenter: parent.verticalCenter }
                                text: {
                                    var total = 0
                                    var items = game.showItems
                                    for (var i = 0; i < page.revealedCount && i < items.length; ++i)
                                        total += items[i].points
                                    return total
                                }
                                color: Cards.playerColor(game.showOwner)
                                font.pixelSize: Theme.fontSizeExtraLarge
                                font.family: Theme.fontFamilyHeading
                            }
                        }

                        Repeater {
                            model: page.groupedItems(page.plainList(game.showItems).slice(0, page.revealedCount))
                            Item {
                                width: showPanel.width
                                height: Theme.itemSizeExtraSmall * 0.7
                                Label {
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: modelData.times > 1 ? qsTr("%1 × %2").arg(modelData.name).arg(modelData.times) : modelData.name
                                    font.pixelSize: Theme.fontSizeSmall
                                }
                                Label {
                                    anchors { right: parent.right; verticalCenter: parent.verticalCenter }
                                    text: modelData.points
                                    color: Cards.playerColor(game.showOwner)
                                    font.pixelSize: Theme.fontSizeSmall
                                }
                            }
                        }

                        Label {
                            visible: game.showTotal === 0
                            text: qsTr("Nineteen — nothing to count")
                            color: Theme.secondaryHighlightColor
                            font.pixelSize: Theme.fontSizeSmall
                        }
                    }

                    Column {
                        id: overPanel
                        x: margin
                        width: parent.width - 2 * margin
                        opacity: page.panel === "over" ? 1 : 0
                        visible: opacity > 0
                        Behavior on opacity { FadeAnimation { duration: 400 } }

                        Label {
                            width: parent.width
                            horizontalAlignment: Text.AlignHCenter
                            text: game.winner === 0 ? qsTr("You won") : qsTr("The bot won")
                            color: Cards.playerColor(game.winner)
                            font.pixelSize: Theme.fontSizeHuge
                            font.family: Theme.fontFamilyHeading
                        }
                        Label {
                            width: parent.width
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.Wrap
                            text: {
                                var loser = game.winner === 0 ? game.botScore : game.humanScore
                                var line = qsTr("%1 to %2").arg(121).arg(loser)
                                if (game.skunk === 2)
                                    line += " · " + qsTr("Double skunk!")
                                else if (game.skunk === 1)
                                    line += " · " + qsTr("Skunk!")
                                var how = game.endedInShow
                                        ? (game.winner === 0 ? qsTr("You counted out.") : qsTr("The bot counted out."))
                                        : (game.winner === 0 ? qsTr("You pegged out.") : qsTr("The bot pegged out."))
                                return line + "\n" + how
                            }
                            color: Theme.secondaryHighlightColor
                            font.pixelSize: Theme.fontSizeMedium
                        }
                    }
                }

                // Space for your hand (cards are drawn by the card layer).
                Item {
                    id: handRow
                    width: parent.width
                    height: cardHeight + Theme.paddingLarge
                }

                Item {
                    width: parent.width
                    height: Theme.itemSizeSmall

                    Row {
                        anchors.centerIn: parent
                        spacing: Theme.paddingLarge

                        Button {
                            visible: game.phase === Cribbage.Discard && !game.humanDiscarded
                            enabled: page.selected.length === 2
                            text: page.humanCrib ? qsTr("To your crib") : qsTr("To bot's crib")
                            onClicked: game.discard(page.selected)
                        }
                        Button {
                            visible: game.mustSayGo
                            text: qsTr("Go")
                            onClicked: game.sayGo()
                        }
                        Button {
                            visible: game.phase === Cribbage.Show
                            text: game.showStep === 2 ? qsTr("Next deal") : qsTr("Continue")
                            onClicked: game.nextShow()
                        }
                        Button {
                            visible: game.phase === Cribbage.GameOver
                            text: qsTr("New game")
                            onClicked: game.newGame()
                        }
                    }
                }
            }

            // ---------------------------------------------------- cards
            // All 52 cards live here and glide to wherever the game puts them.
            Repeater {
                model: 52
                PlayingCard {
                    id: card

                    readonly property var place: page.places[index]
                    readonly property bool inHand: place.role === "hand"
                    property int moveDelay
                    property bool ready

                    function moveToPlace() {
                        moveDelay = place.delay
                        x = place.x
                        y = place.y
                    }

                    width: cardWidth
                    cardId: index
                    faceDown: place.down
                    z: place.z
                    selected: inHand && page.selected.indexOf(index) >= 0
                    glowing: place.role === "drawn"
                             || (page.current !== null && page.current.cards.indexOf(index) >= 0)
                    // Kept by the label while the glow fades out.
                    glowColor: Cards.playerColor(place.role === "drawn" ? place.owner : flash.shownByBot ? 1 : 0)
                    // game.count makes these follow the play.
                    dimmed: inHand && game.humanTurn && game.count >= 0 && !game.canPlay(index)
                    enabled: place.role === "cut" ? game.humanCut < 0
                             : inHand && (game.phase === Cribbage.Discard ? !game.humanDiscarded
                                                                         : game.humanTurn && game.count >= 0 && game.canPlay(index))

                    onPlaceChanged: moveToPlace()
                    Component.onCompleted: {
                        moveToPlace()
                        ready = true
                    }
                    onClicked: {
                        if (place.role === "cut")
                            game.cut(place.line)
                        else if (game.phase === Cribbage.Discard)
                            page.toggle(index)
                        else
                            game.play(index)
                    }

                    Behavior on x {
                        enabled: card.ready
                        SequentialAnimation {
                            PauseAnimation { duration: card.moveDelay }
                            NumberAnimation { duration: 320; easing.type: Easing.OutCubic }
                        }
                    }
                    Behavior on y {
                        enabled: card.ready
                        SequentialAnimation {
                            PauseAnimation { duration: card.moveDelay }
                            NumberAnimation { duration: 320; easing.type: Easing.OutCubic }
                        }
                    }
                }
            }

            // The name and points of the score being shown, above its cards.
            Rectangle {
                id: flash

                readonly property var bounds: {
                    if (page.current === null || page.current.cards.length === 0)
                        return null
                    var left = 1e9, right = -1e9, top = 1e9
                    for (var i = 0; i < page.current.cards.length; ++i) {
                        var p = page.places[page.current.cards[i]]
                        left = Math.min(left, p.x)
                        right = Math.max(right, p.x + cardWidth)
                        top = Math.min(top, p.y)
                    }
                    return { centre: (left + right) / 2, top: top - Theme.paddingLarge }
                }

                // Last shown values, kept while the label fades out.
                property var shownBounds: ({ centre: 0, top: 0 })
                property string shownText
                property bool shownByBot
                onBoundsChanged: {
                    if (bounds === null)
                        return
                    shownBounds = bounds
                    shownText = page.current.name + "  " + page.current.points
                    shownByBot = page.current.player === 1
                }

                z: 500
                visible: opacity > 0
                opacity: bounds !== null ? 1 : 0
                Behavior on opacity { FadeAnimation { duration: 150 } }
                width: flashLabel.width + 2 * Theme.paddingLarge
                height: flashLabel.height + Theme.paddingMedium
                radius: height / 2
                color: Theme.rgba(Theme.highlightDimmerColor, 0.92)
                border.color: flashLabel.color
                border.width: Math.max(1, Math.round(Theme.pixelRatio))
                x: Math.max(margin, Math.min(page.width - margin - width, shownBounds.centre - width / 2))
                y: shownBounds.top - height - Theme.paddingSmall

                Label {
                    id: flashLabel
                    anchors.centerIn: parent
                    text: flash.shownText
                    color: Cards.playerColor(flash.shownByBot ? 1 : 0)
                    font.pixelSize: Theme.fontSizeLarge
                    font.family: Theme.fontFamilyHeading
                }

                Connections {
                    target: page
                    onCurrentChanged: if (page.current !== null) pop.restart()
                }
                NumberAnimation {
                    id: pop
                    target: flash; property: "scale"
                    from: 0.6; to: 1; duration: 260; easing.type: Easing.OutBack
                }
            }
        }
    }
}
