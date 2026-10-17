#pragma once

#include "game.h"

#include <random>

namespace cribbage {

enum class Difficulty { Easy = 0, Medium = 1, Hard = 2 };

class Bot {
public:
    explicit Bot(Difficulty difficulty = Difficulty::Medium, unsigned seed = std::random_device{}());

    void setDifficulty(Difficulty d) { m_difficulty = d; }
    Difficulty difficulty() const { return m_difficulty; }

    // A card to cut from the spread pack, not the one already taken.
    int chooseCut(const Game &game, int player);

    // Two cards to lay away from the six in `hand`.
    Cards chooseDiscard(const Cards &hand, bool ownCrib);
    // A legal card to play; only valid when the player can play.
    Card choosePlay(const Game &game, int player);

    // Expected points of keeping `kept` and laying away `discarded`, averaged
    // over every starter that could still be turned. Positive crib values
    // count for the dealer and against the pone.
    double expectedDiscardValue(const Cards &kept, const Cards &discarded, bool ownCrib);

private:
    Cards discardRandom(const Cards &hand);
    Cards discardBestHand(const Cards &hand);
    Cards discardExpected(const Cards &hand, bool ownCrib);
    Card playRandom(const Cards &legal);
    Card playGreedy(const Game &game, const Cards &legal);
    Card playThoughtful(const Game &game, int player, const Cards &legal);

    Difficulty m_difficulty;
    std::mt19937 m_rng;
};

} // namespace cribbage
