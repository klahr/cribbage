#include "scoring.h"

#include <algorithm>

namespace cribbage {

HandScore scoreCards(const Cards &all)
{
    HandScore score;
    const int n = static_cast<int>(all.size());

    for (int mask = 1; mask < (1 << n); ++mask) {
        int sum = 0;
        Cards used;
        for (int i = 0; i < n; ++i) {
            if (mask & (1 << i)) {
                sum += countValue(all[i]);
                used.push_back(all[i]);
            }
        }
        if (sum == 15) {
            score.fifteens += 2;
            score.items.push_back({Category::Fifteen, 2, used});
        }
    }

    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            if (all[i].rank == all[j].rank) {
                score.pairs += 2;
                score.items.push_back({Category::Pair, 2, {all[i], all[j]}});
            }
        }
    }

    // A run is every combination of distinct consecutive ranks of maximal
    // length; duplicates multiply the number of runs.
    int count[15] = {};
    for (const Card &c : all)
        ++count[c.rank];
    for (int start = 1; start <= 13;) {
        if (!count[start]) {
            ++start;
            continue;
        }
        int end = start;
        while (end + 1 <= 13 && count[end + 1])
            ++end;
        const int length = end - start + 1;
        if (length >= 3) {
            // Enumerate each run so the UI can show which cards make it.
            std::vector<Cards> runs(1);
            for (int r = start; r <= end; ++r) {
                std::vector<Cards> next;
                for (const Cards &partial : runs) {
                    for (const Card &c : all) {
                        if (c.rank == r) {
                            Cards extended = partial;
                            extended.push_back(c);
                            next.push_back(extended);
                        }
                    }
                }
                runs = next;
            }
            for (const Cards &run : runs) {
                score.runs += length;
                score.items.push_back({Category::Run, length, run});
            }
        }
        start = end + 1;
    }
    return score;
}

HandScore scoreHand(const Cards &hand, const Card &starter, bool isCrib)
{
    Cards all = hand;
    all.push_back(starter);
    HandScore score = scoreCards(all);

    if (hand.size() == 4) {
        const bool handFlush = std::all_of(hand.begin(), hand.end(),
                                           [&](const Card &c) { return c.suit == hand[0].suit; });
        if (handFlush) {
            const bool starterMatches = starter.suit == hand[0].suit;
            if (starterMatches) {
                score.flush = 5;
                score.items.push_back({Category::Flush, 5, all});
            } else if (!isCrib) {
                score.flush = 4;
                score.items.push_back({Category::Flush, 4, hand});
            }
        }
    }

    for (const Card &c : hand) {
        if (c.rank == 11 && c.suit == starter.suit) {
            score.nobs = 1;
            score.items.push_back({Category::Nobs, 1, {c, starter}});
        }
    }

    return score;
}

int pileCount(const Cards &pile)
{
    int sum = 0;
    for (const Card &c : pile)
        sum += countValue(c);
    return sum;
}

std::vector<ScoreItem> scorePegging(const Cards &pile)
{
    std::vector<ScoreItem> items;
    if (pile.empty())
        return items;

    const int count = pileCount(pile);
    if (count == 15)
        items.push_back({Category::Fifteen, 2, pile});
    if (count == 31)
        items.push_back({Category::ThirtyOne, 2, pile});

    const int n = static_cast<int>(pile.size());
    int same = 1;
    while (same < n && pile[n - 1 - same].rank == pile[n - 1].rank)
        ++same;
    if (same >= 2) {
        const int points = same * (same - 1); // 2, 6, 12
        items.push_back({Category::Pair, points, Cards(pile.end() - same, pile.end())});
    }

    for (int length = n; length >= 3; --length) {
        std::vector<int> ranks;
        for (int i = n - length; i < n; ++i)
            ranks.push_back(pile[i].rank);
        std::sort(ranks.begin(), ranks.end());
        bool run = true;
        for (int i = 1; i < length; ++i) {
            if (ranks[i] != ranks[i - 1] + 1) {
                run = false;
                break;
            }
        }
        if (run) {
            items.push_back({Category::Run, length, Cards(pile.end() - length, pile.end())});
            break;
        }
    }
    return items;
}

int peggingPoints(const Cards &pile)
{
    int total = 0;
    for (const ScoreItem &item : scorePegging(pile))
        total += item.points;
    return total;
}

} // namespace cribbage
