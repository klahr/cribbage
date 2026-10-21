#pragma once

#include "cards/card.h"

namespace cribbage {

using cards::Card;
using cards::Cards;
using cards::Suit;
using cards::Clubs;
using cards::Diamonds;
using cards::Hearts;
using cards::Spades;
using cards::parseCard;
using cards::parseCards;
using cards::fullDeck;
using cards::contains;
using cards::removeCard;

// What a card counts in the play and towards fifteens: court cards ten.
inline int countValue(const Card &c) { return c.rank > 10 ? 10 : c.rank; }

} // namespace cribbage
