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

// What a card counts towards fifteen and 31: court cards are ten.
inline int value(const Card &c) { return c.rank > 10 ? 10 : c.rank; }

} // namespace cribbage
