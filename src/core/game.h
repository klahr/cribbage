#pragma once

#include "scoring.h"

#include <random>

namespace cribbage {

enum class Phase {
    Cut,      // cutting for the first deal
    Discard,  // both players lay two cards away to the crib
    Play,     // pegging
    Show,     // counting hands, then the crib
    GameOver,
};

struct Event {
    int player;
    Category category;
    int points;
    bool show; // scored while counting hands rather than during the play
    Cards cards; // the cards that earned the points
};

class Game {
public:
    static constexpr int WinningScore = 121;
    static constexpr int Players = 2;

    explicit Game(unsigned seed = std::random_device{}());

    // Spreads a shuffled pack for the cut for first deal.
    void newGame();
    // Starts a game with a fixed first dealer and no shuffling for the
    // first round; used by tests.
    void newGame(int firstDealer, const Cards &deck);

    // Deals the next round from `deck` (top card first). The first six go
    // to the non-dealer and dealer alternately, as at a real table.
    void dealFrom(const Cards &deck);

    // Restores scores, e.g. when resuming a game.
    void setScores(int first, int second);

    // Picks the card at `index` of the spread pack.
    bool cut(int player, int index);
    // After both have cut: on a tie, spreads a fresh pack to cut again;
    // otherwise the lower card deals the first hand.
    bool finishCut();
    bool cutComplete() const { return m_cutIndex[0] >= 0 && m_cutIndex[1] >= 0; }
    bool cutTied() const { return cutComplete() && m_firstCut[0].rank == m_firstCut[1].rank; }
    // Who deals once the cut is complete and not tied, else -1.
    int cutDealer() const;
    int cutIndex(int player) const { return m_cutIndex[player]; }
    const Cards &cutDeck() const { return m_cutDeck; }

    bool discard(int player, const Cards &cards);
    bool play(int player, const Card &card);
    bool sayGo(int player);
    // Advances to the next counting step; after the crib, deals a new round.
    bool nextShow();

    bool canPlay(int player) const;
    bool canPlayCard(int player, const Card &card) const;
    bool mustSayGo(int player) const;

    Phase phase() const { return m_phase; }
    int dealer() const { return m_dealer; }
    int pone() const { return 1 - m_dealer; }
    int score(int player) const { return m_score[player]; }
    int previousScore(int player) const { return m_previous[player]; }
    int winner() const { return m_winner; }
    // 0 = no skunk, 1 = skunk (loser under 91), 2 = double skunk (under 61).
    int skunk() const;

    const Cards &hand(int player) const { return m_hand[player]; }        // cards held (6 before discard)
    const Cards &keptHand(int player) const { return m_kept[player]; }     // the four for the show
    const Cards &crib() const { return m_crib; }
    bool hasDiscarded(int player) const { return m_discarded[player]; }
    const Card &starter() const { return m_starter; }
    bool hasStarter() const { return m_starter.valid(); }
    const Cards &pile() const { return m_pile; }          // since the count was last reset
    const Cards &played() const { return m_played; }      // all cards played this round
    const std::vector<int> &playedBy() const { return m_playedBy; }
    int count() const { return pileCount(m_pile); }
    int turn() const { return m_turn; }
    bool saidGo(int player) const { return m_go[player]; }

    // Show: 0 = pone's hand, 1 = dealer's hand, 2 = crib.
    int showStep() const { return m_showStep; }
    int showOwner() const { return m_showStep == 0 ? pone() : m_dealer; }
    const Cards &showCards() const;
    const HandScore &showScore() const { return m_showScores[m_showStep]; }
    const HandScore &showScore(int step) const { return m_showScores[step]; }

    const std::vector<Event> &events() const { return m_events; }
    int round() const { return m_round; }
    const Card &firstCut(int player) const { return m_firstCut[player]; }

private:
    void award(int player, Category category, int points, const Cards &cards);
    void award(int player, const std::vector<ScoreItem> &items);
    void settle();
    void resetCount();
    void startPlay();
    void startShow();
    void applyShow();
    void dealShuffled();

    std::mt19937 m_rng;
    Phase m_phase = Phase::Discard;
    int m_dealer = 0;
    int m_score[Players] = {};
    int m_previous[Players] = {};
    int m_winner = -1;
    Cards m_hand[Players];
    Cards m_kept[Players];
    bool m_discarded[Players] = {};
    Cards m_crib;
    Card m_starter;
    Cards m_pile;
    Cards m_played;
    std::vector<int> m_playedBy;
    int m_turn = 0;
    int m_lastPlayer = -1;
    bool m_go[Players] = {};
    int m_showStep = 0;
    HandScore m_showScores[3];
    std::vector<Event> m_events;
    Cards m_deck;
    int m_round = 0;
    Card m_firstCut[Players];
    int m_cutIndex[Players] = {-1, -1};
    Cards m_cutDeck;
};

} // namespace cribbage
