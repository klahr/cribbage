#pragma once

#include "card.h"

namespace cribbage {

enum class Category {
    Fifteen,
    Pair,
    Run,
    Flush,
    Nobs,
    Nibs,
    ThirtyOne,
    Go,
    LastCard,
};

struct ScoreItem {
    Category category;
    int points;
    Cards cards;
};

struct HandScore {
    int fifteens = 0;
    int pairs = 0;
    int runs = 0;
    int flush = 0;
    int nobs = 0;
    std::vector<ScoreItem> items;

    int total() const { return fifteens + pairs + runs + flush + nobs; }
};

// Scores four cards plus the starter. A crib only scores a flush when all
// five cards share a suit; a hand scores four for four of a suit.
HandScore scoreHand(const Cards &hand, const Card &starter, bool isCrib);

// Fifteens, pairs and runs of any set of cards.
HandScore scoreCards(const Cards &cards);

// Points for the last card of `pile` during the play. `pile` holds the cards
// played since the count was last reset, in order.
std::vector<ScoreItem> scorePegging(const Cards &pile);
int peggingPoints(const Cards &pile);

int pileCount(const Cards &pile);

} // namespace cribbage
