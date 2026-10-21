#include "bot.h"

#include <algorithm>

namespace cribbage {

namespace {

Cards without(const Cards &cards, const Cards &removed)
{
    Cards out;
    for (const Card &c : cards)
        if (std::find(removed.begin(), removed.end(), c) == removed.end())
            out.push_back(c);
    return out;
}

struct Split {
    Cards kept;
    Cards discarded;
};

std::vector<Split> splits(const Cards &hand)
{
    std::vector<Split> out;
    for (size_t i = 0; i < hand.size(); ++i) {
        for (size_t j = i + 1; j < hand.size(); ++j) {
            Split s;
            s.discarded = {hand[i], hand[j]};
            s.kept = without(hand, s.discarded);
            out.push_back(s);
        }
    }
    return out;
}

// Scores the four kept cards on their own, as a hand without a starter.
int bareHandPoints(const Cards &kept)
{
    return scoreCards(kept).total();
}

} // namespace

Bot::Bot(Difficulty difficulty, unsigned seed)
    : m_difficulty(difficulty)
    , m_rng(seed)
{
}

int Bot::chooseCut(const Game &game, int player)
{
    const int taken = game.cutIndex(1 - player);
    std::uniform_int_distribution<int> pick(0, static_cast<int>(game.cutDeck().size()) - 1);
    int index;
    do {
        index = pick(m_rng);
    } while (index == taken);
    return index;
}

Cards Bot::chooseDiscard(const Cards &hand, bool ownCrib)
{
    switch (m_difficulty) {
    case Difficulty::Easy:
        return discardRandom(hand);
    case Difficulty::Medium:
        return discardBestHand(hand);
    case Difficulty::Hard:
        break;
    }
    return discardExpected(hand, ownCrib);
}

Cards Bot::discardRandom(const Cards &hand)
{
    Cards shuffled = hand;
    std::shuffle(shuffled.begin(), shuffled.end(), m_rng);
    return {shuffled[0], shuffled[1]};
}

Cards Bot::discardBestHand(const Cards &hand)
{
    std::vector<Split> options = splits(hand);
    std::shuffle(options.begin(), options.end(), m_rng);
    int best = -1;
    Cards choice;
    for (const Split &s : options) {
        const int points = bareHandPoints(s.kept);
        if (points > best) {
            best = points;
            choice = s.discarded;
        }
    }
    return choice;
}

double Bot::expectedDiscardValue(const Cards &kept, const Cards &discarded, bool ownCrib)
{
    Cards hand = kept;
    hand.insert(hand.end(), discarded.begin(), discarded.end());
    const Cards unseen = without(fullDeck(), hand);

    double handTotal = 0;
    double cribTotal = 0;
    int cribSamples = 0;
    std::uniform_int_distribution<size_t> pick(0, unseen.size() - 1);
    for (const Card &starter : unseen) {
        handTotal += scoreHand(kept, starter, false).total();
        // The other two crib cards are unknown; sample a few pairs.
        for (int k = 0; k < 8; ++k) {
            const Card a = unseen[pick(m_rng)];
            const Card b = unseen[pick(m_rng)];
            if (a == b || a == starter || b == starter)
                continue;
            cribTotal += scoreHand({discarded[0], discarded[1], a, b}, starter, true).total();
            ++cribSamples;
        }
    }
    const double handValue = handTotal / unseen.size();
    const double cribValue = cribSamples ? cribTotal / cribSamples : 0;
    return handValue + (ownCrib ? cribValue : -cribValue);
}

Cards Bot::discardExpected(const Cards &hand, bool ownCrib)
{
    double best = -1e9;
    Cards choice;
    for (const Split &s : splits(hand)) {
        const double value = expectedDiscardValue(s.kept, s.discarded, ownCrib);
        if (value > best) {
            best = value;
            choice = s.discarded;
        }
    }
    return choice;
}

Card Bot::choosePlay(const Game &game, int player)
{
    Cards legal;
    for (const Card &c : game.hand(player))
        if (game.canPlayCard(player, c))
            legal.push_back(c);
    switch (m_difficulty) {
    case Difficulty::Easy:
        return playRandom(legal);
    case Difficulty::Medium:
        return playGreedy(game, legal);
    case Difficulty::Hard:
        break;
    }
    return playThoughtful(game, player, legal);
}

Card Bot::playRandom(const Cards &legal)
{
    std::uniform_int_distribution<size_t> pick(0, legal.size() - 1);
    return legal[pick(m_rng)];
}

Card Bot::playGreedy(const Game &game, const Cards &legal)
{
    Cards options = legal;
    std::shuffle(options.begin(), options.end(), m_rng);
    int best = -1;
    Card choice;
    for (const Card &c : options) {
        Cards pile = game.pile();
        pile.push_back(c);
        const int points = peggingPoints(pile);
        if (points > best) {
            best = points;
            choice = c;
        }
    }
    return choice;
}

Card Bot::playThoughtful(const Game &game, int player, const Cards &legal)
{
    const int opponent = 1 - player;
    Cards known = game.hand(player);
    known.insert(known.end(), game.played().begin(), game.played().end());
    known.push_back(game.starter());
    const Cards unseen = without(fullDeck(), known);
    const int opponentCards = static_cast<int>(game.hand(opponent).size());
    const bool opponentOut = game.saidGo(opponent) || opponentCards == 0;

    double best = -1e9;
    Card choice = legal.front();
    for (const Card &c : legal) {
        Cards pile = game.pile();
        pile.push_back(c);
        const int count = pileCount(pile);
        double value = peggingPoints(pile);

        if (count < 31 && !opponentOut && !unseen.empty()) {
            // What the opponent is likely to peg in reply.
            double reply = 0;
            for (const Card &u : unseen) {
                if (count + countValue(u) > 31)
                    continue;
                Cards next = pile;
                next.push_back(u);
                reply += peggingPoints(next);
            }
            const double chance = std::min(1.0, opponentCards / 4.0);
            value -= 1.6 * chance * reply / unseen.size() * std::min(opponentCards, 3);
        }

        // Keep the cards that can extend the play for later, and lead small.
        if (opponentOut) {
            int follow = 0;
            for (const Card &k : game.hand(player))
                if (k != c && count + countValue(k) <= 31)
                    ++follow;
            value += 0.3 * follow;
        }
        if (game.pile().empty() && countValue(c) == 5)
            value -= 1.0;
        value += 0.01 * countValue(c);

        if (value > best) {
            best = value;
            choice = c;
        }
    }
    return choice;
}

} // namespace cribbage
