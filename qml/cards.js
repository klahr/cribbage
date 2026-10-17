.pragma library

var ranks = ["A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K"]
var suits = ["♣", "♦", "♥", "♠"]

function rank(id) { return id % 13 + 1 }
function suit(id) { return Math.floor(id / 13) }
function rankText(id) { return ranks[id % 13] }
function suitText(id) { return suits[suit(id)] }
function isRed(id) { var s = suit(id); return s === 1 || s === 2 }
function isFace(id) { return id >= 0 && rank(id) > 10 }
function name(id) { return rankText(id) + suitText(id) }

function names(ids) {
    var out = []
    for (var i = 0; i < ids.length; ++i)
        out.push(name(ids[i]))
    return out.join(" ")
}

// Player colours: you are blue, the bot is red.
var playerColors = ["#4a9dff", "#ff5252"]
function playerColor(player) { return playerColors[player === 1 ? 1 : 0] }
