#pragma once

#include <string>
#include <vector>

namespace cribbage {

enum Suit { Clubs = 0, Diamonds = 1, Hearts = 2, Spades = 3 };

struct Card {
    int rank = 0; // 1 = ace ... 11 = jack, 12 = queen, 13 = king
    int suit = 0;

    Card() = default;
    Card(int r, int s) : rank(r), suit(s) {}

    int value() const { return rank > 10 ? 10 : rank; }
    int id() const { return suit * 13 + rank - 1; }
    bool valid() const { return rank >= 1 && rank <= 13 && suit >= 0 && suit <= 3; }

    static Card fromId(int id) { return Card(id % 13 + 1, id / 13); }

    bool operator==(const Card &o) const { return rank == o.rank && suit == o.suit; }
    bool operator!=(const Card &o) const { return !(*this == o); }
};

using Cards = std::vector<Card>;

// Parses compact notation used by tests, e.g. "5H", "10S", "JD", "AC".
Card parseCard(const std::string &text);
Cards parseCards(const std::string &text);

Cards fullDeck();

} // namespace cribbage
