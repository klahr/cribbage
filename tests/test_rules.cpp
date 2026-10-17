// Rule tests for the cribbage engine. Plain C++ so they build anywhere:
//   make -C tests

#include "../src/core/bot.h"
#include "../src/core/game.h"
#include "../src/core/scoring.h"

#include <algorithm>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

using namespace cribbage;

namespace {

int g_failures = 0;
int g_checks = 0;
const char *g_test = "";

#define CHECK(cond)                                                                      \
    do {                                                                                 \
        ++g_checks;                                                                      \
        if (!(cond)) {                                                                   \
            ++g_failures;                                                                \
            std::printf("FAIL %s (%s:%d): %s\n", g_test, __FILE__, __LINE__, #cond);     \
        }                                                                                \
    } while (0)

#define CHECK_EQ(a, b)                                                                   \
    do {                                                                                 \
        ++g_checks;                                                                      \
        const auto va = (a);                                                             \
        const auto vb = (b);                                                             \
        if (!(va == vb)) {                                                               \
            ++g_failures;                                                                \
            std::printf("FAIL %s (%s:%d): %s == %s (%d vs %d)\n", g_test, __FILE__,      \
                        __LINE__, #a, #b, int(va), int(vb));                             \
        }                                                                                \
    } while (0)

struct Test {
    const char *name;
    std::function<void()> body;
};

std::vector<Test> &tests()
{
    static std::vector<Test> all;
    return all;
}

struct Register {
    Register(const char *name, std::function<void()> body) { tests().push_back({name, body}); }
};

#define TEST(name)                                     \
    static void name();                                \
    static Register register_##name(#name, name);      \
    static void name()

int hand(const std::string &cards, const std::string &starter, bool crib = false)
{
    return scoreHand(parseCards(cards), parseCard(starter), crib).total();
}

int peg(const std::string &pile)
{
    return peggingPoints(parseCards(pile));
}

// Builds a deck that deals `pone` and `dealer` their six cards and turns up
// `starter`; the rest of the pack follows in order.
Cards stackedDeck(const std::string &pone, const std::string &dealer, const std::string &starter)
{
    const Cards p = parseCards(pone);
    const Cards d = parseCards(dealer);
    Cards deck;
    for (int i = 0; i < 6; ++i) {
        deck.push_back(p[i]);
        deck.push_back(d[i]);
    }
    deck.push_back(parseCard(starter));
    for (const Card &c : fullDeck())
        if (std::find(deck.begin(), deck.end(), c) == deck.end())
            deck.push_back(c);
    return deck;
}

// Player 0 is the pone (plays first), player 1 deals.
Game stackedGame(const std::string &pone, const std::string &dealer, const std::string &starter)
{
    Game g(1);
    g.newGame(1, stackedDeck(pone, dealer, starter));
    return g;
}

void discardLastTwo(Game &g)
{
    for (int p = 0; p < 2; ++p) {
        const Cards h = g.hand(p);
        g.discard(p, {h[4], h[5]});
    }
}

bool playCard(Game &g, int player, const std::string &card)
{
    return g.play(player, parseCard(card));
}

int sumCategory(const HandScore &s, Category c)
{
    int total = 0;
    for (const ScoreItem &i : s.items)
        if (i.category == c)
            total += i.points;
    return total;
}

// ---------------------------------------------------------------- cards

TEST(card_values)
{
    CHECK_EQ(parseCard("AS").value(), 1);
    CHECK_EQ(parseCard("9H").value(), 9);
    CHECK_EQ(parseCard("10D").value(), 10);
    CHECK_EQ(parseCard("JC").value(), 10);
    CHECK_EQ(parseCard("QC").value(), 10);
    CHECK_EQ(parseCard("KC").value(), 10);
}

TEST(card_ids_roundtrip_and_deck_is_complete)
{
    const Cards deck = fullDeck();
    CHECK_EQ(deck.size(), size_t(52));
    for (const Card &c : deck)
        CHECK(Card::fromId(c.id()) == c);
    for (size_t i = 0; i < deck.size(); ++i)
        for (size_t j = i + 1; j < deck.size(); ++j)
            CHECK(deck[i] != deck[j]);
}

// ---------------------------------------------------------------- the show

TEST(fifteens_score_two_each)
{
    const HandScore s = scoreHand(parseCards("10S 5H 2C 3D"), parseCard("KD"), false);
    // 10+5, K+5, 10+2+3, K+2+3
    CHECK_EQ(s.fifteens, 8);
}

TEST(pair_scores_two)
{
    const HandScore s = scoreHand(parseCards("9S 9H 2C 4D"), parseCard("KD"), false);
    CHECK_EQ(s.pairs, 2);
}

TEST(pair_royal_scores_six)
{
    const HandScore s = scoreHand(parseCards("7C 7D 7H AS"), parseCard("2C"), false);
    CHECK_EQ(s.pairs, 6);
    CHECK_EQ(s.fifteens, 6); // 7+7+A three ways
    CHECK_EQ(s.total(), 12);
}

TEST(double_pair_royal_scores_twelve)
{
    const HandScore s = scoreHand(parseCards("4C 4D 4H 4S"), parseCard("KC"), false);
    CHECK_EQ(s.pairs, 12);
}

TEST(face_cards_of_different_rank_do_not_pair)
{
    const HandScore s = scoreHand(parseCards("JC QD KH 2S"), parseCard("8C"), false);
    CHECK_EQ(s.pairs, 0);
    CHECK_EQ(s.runs, 3); // J Q K is a run
}

TEST(runs_of_three_four_five)
{
    CHECK_EQ(scoreHand(parseCards("2C 3D 4H 9S"), parseCard("KC"), false).runs, 3);
    CHECK_EQ(scoreHand(parseCards("9C 10D JH QS"), parseCard("2C"), false).runs, 4);
    CHECK_EQ(scoreHand(parseCards("AC 2D 3H 4S"), parseCard("5C"), false).runs, 5);
    CHECK_EQ(hand("AC 2D 3H 4S", "5C"), 7); // run of five and 1+2+3+4+5
}

TEST(ace_is_low_and_runs_do_not_wrap)
{
    CHECK_EQ(scoreHand(parseCards("QC KD AH 7S"), parseCard("4C"), false).runs, 0);
    CHECK_EQ(scoreHand(parseCards("AC 2D 3H 7S"), parseCard("9C"), false).runs, 3);
}

TEST(double_run)
{
    const HandScore s = scoreHand(parseCards("3C 3D 4H 5S"), parseCard("KH"), false);
    CHECK_EQ(s.runs, 6);
    CHECK_EQ(s.pairs, 2);
    CHECK_EQ(s.fifteens, 4); // 3+3+4+5, K+5
    CHECK_EQ(s.total(), 12);
}

TEST(triple_run)
{
    const HandScore s = scoreHand(parseCards("3C 3D 3H 4S"), parseCard("5S"), false);
    CHECK_EQ(s.runs, 9);
    CHECK_EQ(s.pairs, 6);
    CHECK_EQ(s.total(), 21);
}

TEST(double_double_run)
{
    const HandScore s = scoreHand(parseCards("3C 3D 4H 4S"), parseCard("5S"), false);
    CHECK_EQ(s.runs, 12);
    CHECK_EQ(s.pairs, 4);
    CHECK_EQ(s.total(), 20);
}

TEST(double_run_of_four)
{
    const HandScore s = scoreHand(parseCards("6C 7D 8H 8S"), parseCard("9S"), false);
    CHECK_EQ(s.runs, 8);
    CHECK_EQ(s.pairs, 2);
    CHECK_EQ(s.fifteens, 6); // 6+9, 7+8 twice
    CHECK_EQ(s.total(), 16);
}

TEST(four_card_flush_in_hand)
{
    const HandScore s = scoreHand(parseCards("2H 4H 6H 8H"), parseCard("KC"), false);
    CHECK_EQ(s.flush, 4);
    CHECK_EQ(s.total(), 4);
}

TEST(five_card_flush_in_hand)
{
    CHECK_EQ(scoreHand(parseCards("2H 4H 6H 8H"), parseCard("KH"), false).flush, 5);
}

TEST(starter_alone_cannot_make_a_flush)
{
    CHECK_EQ(scoreHand(parseCards("2H 4H 6H 8C"), parseCard("KH"), false).flush, 0);
}

TEST(crib_needs_five_cards_for_flush)
{
    CHECK_EQ(scoreHand(parseCards("2H 4H 6H 8H"), parseCard("KC"), true).flush, 0);
    CHECK_EQ(scoreHand(parseCards("2H 4H 6H 8H"), parseCard("KH"), true).flush, 5);
}

TEST(nobs_jack_of_starter_suit)
{
    CHECK_EQ(scoreHand(parseCards("JH 2C 4D 6S"), parseCard("8H"), false).nobs, 1);
    CHECK_EQ(hand("JH 2C 4D 6S", "8H"), 1);
    CHECK_EQ(scoreHand(parseCards("JH 2C 4D 6S"), parseCard("8C"), false).nobs, 0);
    // Nobs counts in the crib too.
    CHECK_EQ(scoreHand(parseCards("JH 2C 4D 6S"), parseCard("8H"), true).nobs, 1);
}

TEST(jack_starter_is_not_nobs)
{
    CHECK_EQ(scoreHand(parseCards("2C 4D 6S 8C"), parseCard("JC"), false).nobs, 0);
}

TEST(perfect_twenty_nine)
{
    const HandScore s = scoreHand(parseCards("5H 5D 5S JC"), parseCard("5C"), false);
    CHECK_EQ(s.fifteens, 16);
    CHECK_EQ(s.pairs, 12);
    CHECK_EQ(s.nobs, 1);
    CHECK_EQ(s.total(), 29);
}

TEST(twenty_eight)
{
    CHECK_EQ(hand("5C 5D 5H 5S", "JC"), 28);
}

TEST(twenty_four_with_double_runs)
{
    CHECK_EQ(hand("7C 7D 8H 8S", "9S"), 24);
}

TEST(zero_hand)
{
    CHECK_EQ(hand("2C 4D 6H 8S", "KC"), 0);
    CHECK_EQ(hand("AC 3D 9H JS", "KC"), 0);
}

TEST(breakdown_items_add_up)
{
    const HandScore s = scoreHand(parseCards("3C 3D 4H 5S"), parseCard("KH"), false);
    CHECK_EQ(sumCategory(s, Category::Fifteen), s.fifteens);
    CHECK_EQ(sumCategory(s, Category::Pair), s.pairs);
    CHECK_EQ(sumCategory(s, Category::Run), s.runs);
    int total = 0;
    for (const ScoreItem &i : s.items)
        total += i.points;
    CHECK_EQ(total, s.total());
}

TEST(score_cards_without_starter)
{
    CHECK_EQ(scoreCards(parseCards("5C 5D 5H JS")).total(), 14);
    CHECK_EQ(scoreCards(parseCards("2C 4D 6H 8S")).total(), 0);
}

// ---------------------------------------------------------------- pegging scoring

TEST(pegging_fifteen)
{
    CHECK_EQ(peg("10S 5H"), 2);
    CHECK_EQ(peg("7S 8H"), 2);
    CHECK_EQ(peg("7S 7H"), 2); // a pair, not a fifteen
    CHECK_EQ(peg("4S 4H 7C"), 2);
}

TEST(pegging_thirty_one)
{
    CHECK_EQ(peg("10S KH QD AC"), 2);
    CHECK_EQ(peg("10S KH 9D"), 0);
}

TEST(pegging_pairs)
{
    CHECK_EQ(peg("9S 9H"), 2);
    CHECK_EQ(peg("9S 9H 9C"), 6);
    CHECK_EQ(peg("2S 2H 2C 2D"), 12);
}

TEST(pegging_pair_must_be_adjacent)
{
    CHECK_EQ(peg("9S 3H 9C"), 0);
    CHECK_EQ(peg("9S 9H 3C 9D"), 0);
}

TEST(pegging_jack_and_queen_do_not_pair)
{
    CHECK_EQ(peg("JS QH"), 0);
}

TEST(pegging_runs_in_any_order)
{
    CHECK_EQ(peg("7S 9H 8C"), 3);
    CHECK_EQ(peg("4S 6H 5C 3D"), 4);
    CHECK_EQ(peg("4S 6H 5C 3D 7C"), 5);
    CHECK_EQ(peg("AS 3H 2C"), 3);
}

TEST(pegging_run_broken_by_extra_card)
{
    CHECK_EQ(peg("4S 5H 5C 6D"), 0);
    CHECK_EQ(peg("4S 5H 9C 6D"), 0);
}

TEST(pegging_run_after_pair)
{
    CHECK_EQ(peg("5S 5H 4C 6D"), 3); // last three are 5 4 6
}

TEST(pegging_combined_fifteen_and_run)
{
    CHECK_EQ(peg("7S 9H 8C"), 3);
    CHECK_EQ(peg("6S 4H 5C"), 5); // run of 3 and fifteen
}

TEST(pegging_combined_fifteen_and_pair)
{
    CHECK_EQ(peg("5S 5H 5C"), 6 + 2);
}

TEST(pegging_items_and_count)
{
    const std::vector<ScoreItem> items = scorePegging(parseCards("6S 4H 5C"));
    CHECK_EQ(items.size(), size_t(2));
    CHECK_EQ(pileCount(parseCards("KS QH 9C 2C")), 31);
}

// ---------------------------------------------------------------- dealing and discarding

TEST(deal_six_cards_each_starting_with_pone)
{
    Game g(1);
    const Cards deck = fullDeck();
    g.newGame(1, deck);
    CHECK(g.phase() == Phase::Discard);
    CHECK_EQ(g.dealer(), 1);
    CHECK_EQ(g.pone(), 0);
    CHECK_EQ(g.hand(0).size(), size_t(6));
    CHECK_EQ(g.hand(1).size(), size_t(6));
    CHECK(g.hand(0)[0] == deck[0]);
    CHECK(g.hand(1)[0] == deck[1]);
    CHECK(!g.hasStarter());
}

int indexOfRank(const Game &g, int rank, int skip = -1)
{
    for (int i = 0; i < static_cast<int>(g.cutDeck().size()); ++i)
        if (g.cutDeck()[i].rank == rank && i != skip)
            return i;
    return -1;
}

TEST(new_game_spreads_the_pack_for_the_cut)
{
    Game g(5);
    g.newGame();
    CHECK(g.phase() == Phase::Cut);
    CHECK_EQ(g.cutDeck().size(), size_t(52));
    CHECK(g.hand(0).empty());
    CHECK(g.hand(1).empty());
    CHECK(!g.cutComplete());
    CHECK_EQ(g.cutDealer(), -1);
    CHECK(!g.discard(0, Cards(g.cutDeck().begin(), g.cutDeck().begin() + 2)));
    for (const Card &c : fullDeck())
        CHECK(std::find(g.cutDeck().begin(), g.cutDeck().end(), c) != g.cutDeck().end());
}

TEST(cut_rejects_bad_picks)
{
    Game g(6);
    g.newGame();
    CHECK(!g.cut(0, -1));
    CHECK(!g.cut(0, 52));
    CHECK(!g.finishCut());        // nobody has cut
    CHECK(g.cut(0, 10));
    CHECK(!g.cut(0, 11));         // only once
    CHECK(!g.cut(1, 10));         // the same card
    CHECK(!g.finishCut());        // the bot has not cut
    CHECK(g.cut(1, 11));
    CHECK(g.cutComplete());
    CHECK(g.firstCut(0) == g.cutDeck()[10]);
    CHECK(g.firstCut(1) == g.cutDeck()[11]);
}

TEST(lower_cut_deals_first)
{
    Game g(7);
    g.newGame();
    CHECK(g.cut(0, indexOfRank(g, 2)));
    CHECK(g.cut(1, indexOfRank(g, 9)));
    CHECK_EQ(g.cutDealer(), 0);
    CHECK(g.finishCut());
    CHECK(g.phase() == Phase::Discard);
    CHECK_EQ(g.dealer(), 0);
    CHECK_EQ(g.round(), 1);
    CHECK_EQ(g.hand(0).size(), size_t(6));
    CHECK_EQ(g.hand(1).size(), size_t(6));
    CHECK_EQ(g.score(0), 0);
    CHECK_EQ(g.score(1), 0);

    Game h(8);
    h.newGame();
    CHECK(h.cut(0, indexOfRank(h, 13)));
    CHECK(h.cut(1, indexOfRank(h, 1)));  // ace is low
    CHECK_EQ(h.cutDealer(), 1);
    CHECK(h.finishCut());
    CHECK_EQ(h.dealer(), 1);
}

TEST(face_cards_cut_by_rank_not_value)
{
    Game g(9);
    g.newGame();
    CHECK(g.cut(0, indexOfRank(g, 12)));  // queen
    CHECK(g.cut(1, indexOfRank(g, 13)));  // king
    CHECK_EQ(g.cutDealer(), 0);
}

TEST(tied_cut_is_cut_again)
{
    Game g(10);
    g.newGame();
    const int first = indexOfRank(g, 7);
    CHECK(g.cut(0, first));
    CHECK(g.cut(1, indexOfRank(g, 7, first)));
    CHECK(g.cutTied());
    CHECK_EQ(g.cutDealer(), -1);
    CHECK(g.finishCut());
    CHECK(g.phase() == Phase::Cut);
    CHECK(!g.cutComplete());
    CHECK_EQ(g.cutIndex(0), -1);
    CHECK_EQ(g.cutDeck().size(), size_t(52));
    CHECK(g.cut(0, 0));
}

TEST(bot_cuts_a_free_card)
{
    for (unsigned seed = 1; seed < 30; ++seed) {
        Game g(seed);
        g.newGame();
        CHECK(g.cut(0, static_cast<int>(seed)));
        Bot bot(Difficulty::Easy, seed);
        const int pick = bot.chooseCut(g, 1);
        CHECK(pick != static_cast<int>(seed));
        CHECK(g.cut(1, pick));
    }
}

TEST(discard_two_cards_to_the_crib)
{
    Game g = stackedGame("AS 2S 3S 4S 5S 6S", "AH 2H 3H 4H 5H 6H", "9C");
    CHECK(!g.discard(0, parseCards("AS")));            // one card
    CHECK(!g.discard(0, parseCards("AS 2S 3S")));      // three cards
    CHECK(!g.discard(0, parseCards("AS AH")));         // not in hand
    CHECK(!g.discard(0, parseCards("AS AS")));         // same card twice
    CHECK(g.discard(0, parseCards("AS 2S")));
    CHECK(!g.discard(0, parseCards("3S 4S")));         // only once
    CHECK(g.hasDiscarded(0));
    CHECK(g.phase() == Phase::Discard);                // waiting for the dealer
    CHECK_EQ(g.hand(0).size(), size_t(4));
    CHECK(g.discard(1, parseCards("5H 6H")));
    CHECK_EQ(g.crib().size(), size_t(4));
    CHECK(g.phase() == Phase::Play);
}

TEST(starter_is_cut_after_discards)
{
    Game g = stackedGame("AS 2S 3S 4S 5S 6S", "AH 2H 3H 4H 5H 6H", "9C");
    discardLastTwo(g);
    CHECK(g.hasStarter());
    CHECK(g.starter() == parseCard("9C"));
}

TEST(nibs_gives_dealer_two)
{
    Game g = stackedGame("AS 2S 3S 4S 5S 6S", "AH 2H 3H 4H 5H 6H", "JC");
    discardLastTwo(g);
    CHECK_EQ(g.score(1), 2);
    CHECK_EQ(g.score(0), 0);
    CHECK(g.events().back().category == Category::Nibs);
}

TEST(nibs_can_win_the_game)
{
    Game g = stackedGame("AS 2S 3S 4S 5S 6S", "AH 2H 3H 4H 5H 6H", "JC");
    g.setScores(50, 120);
    discardLastTwo(g);
    CHECK(g.phase() == Phase::GameOver);
    CHECK_EQ(g.winner(), 1);
}

// ---------------------------------------------------------------- the play

TEST(pone_leads_and_turns_alternate)
{
    Game g = stackedGame("AS 2S 3S 4S 5S 6S", "AH 2H 3H 4H 5H 6H", "9C");
    discardLastTwo(g);
    CHECK_EQ(g.turn(), 0);
    CHECK(!playCard(g, 1, "AH"));  // out of turn
    CHECK(playCard(g, 0, "AS"));
    CHECK_EQ(g.turn(), 1);
    CHECK(!playCard(g, 0, "2S"));
    CHECK(playCard(g, 1, "4H"));
    CHECK_EQ(g.turn(), 0);
    CHECK_EQ(g.count(), 5);
}

TEST(cannot_play_card_not_in_hand)
{
    Game g = stackedGame("AS 2S 3S 4S 5S 6S", "AH 2H 3H 4H 5H 6H", "9C");
    discardLastTwo(g);
    CHECK(!playCard(g, 0, "AH"));
    CHECK(!playCard(g, 0, "5S")); // laid away in the crib
}

TEST(count_cannot_exceed_thirty_one)
{
    Game g = stackedGame("KS QS JS 9H AD 4D", "KH QH 2C 3C AC 4C", "7C");
    discardLastTwo(g);
    CHECK(playCard(g, 0, "KS"));
    CHECK(playCard(g, 1, "KH"));
    CHECK(playCard(g, 0, "QS"));     // 30
    CHECK(!playCard(g, 1, "QH"));    // 40
    CHECK(!playCard(g, 1, "2C"));    // 32
    CHECK(!g.canPlay(1));
    CHECK(g.mustSayGo(1));
}

TEST(cannot_say_go_while_able_to_play)
{
    Game g = stackedGame("KS QS JS 9H AD 4D", "KH QH 2C 3C AC 4C", "7C");
    discardLastTwo(g);
    CHECK(!g.sayGo(0));
    CHECK(!g.sayGo(1)); // not their turn
}

TEST(go_scores_one_and_resets_count)
{
    // Pone keeps K Q J 9, dealer keeps K Q 2 3.
    Game g = stackedGame("KS QS JS 9H AD 4D", "KH QH 2C 3C AC 4C", "7C");
    discardLastTwo(g);
    CHECK(playCard(g, 0, "KS"));
    CHECK(playCard(g, 1, "KH"));     // pair
    CHECK_EQ(g.score(1), 2);
    CHECK(playCard(g, 0, "QS"));     // 30
    CHECK(g.sayGo(1));
    // Pone cannot play either, so pone pegs one for go.
    CHECK_EQ(g.score(0), 1);
    CHECK(g.events().back().category == Category::Go);
    CHECK_EQ(g.count(), 0);
    CHECK(g.pile().empty());
    // The player after the one who pegged the go leads.
    CHECK_EQ(g.turn(), 1);
    CHECK(playCard(g, 1, "QH"));
    CHECK(playCard(g, 0, "JS"));
    CHECK(playCard(g, 1, "2C"));
    CHECK(playCard(g, 0, "9H"));     // 31 for two
    CHECK_EQ(g.score(0), 3);
    CHECK(g.events().back().category == Category::ThirtyOne);
    CHECK_EQ(g.count(), 0);
    CHECK_EQ(g.turn(), 1);
    const size_t before = g.events().size();
    CHECK(playCard(g, 1, "3C"));     // last card
    CHECK(g.events()[before].category == Category::LastCard);
    CHECK_EQ(g.events()[before].player, 1);
    CHECK_EQ(g.events()[before].points, 1);
    CHECK(g.phase() == Phase::Show);
}

TEST(player_continues_after_opponent_says_go)
{
    // Pone keeps 10 10 5 A, dealer keeps K K 4 4.
    Game g = stackedGame("10S 10H 5S AS 2D 3D", "KS KH 4C 4D 2C 3C", "7C");
    discardLastTwo(g);
    CHECK(playCard(g, 0, "10S"));
    CHECK(playCard(g, 1, "KS"));
    CHECK(playCard(g, 0, "10H"));    // 30
    CHECK(g.sayGo(1));
    CHECK_EQ(g.turn(), 0);           // pone may still play
    CHECK_EQ(g.score(0), 0);         // no go point yet
    CHECK(playCard(g, 0, "AS"));     // 31: two points, not three
    CHECK_EQ(g.score(0), 2);
    CHECK_EQ(g.count(), 0);
    CHECK_EQ(g.turn(), 1);
    CHECK(playCard(g, 1, "KH"));
    CHECK(playCard(g, 0, "5S"));     // fifteen
    CHECK_EQ(g.score(0), 4);
    // Pone is out of cards, so the dealer plays on alone.
    CHECK_EQ(g.turn(), 1);
    CHECK(playCard(g, 1, "4C"));
    CHECK_EQ(g.turn(), 1);
    CHECK(playCard(g, 1, "4D"));     // pair, then last card
    CHECK_EQ(g.score(1), 3);
    CHECK(g.phase() == Phase::Show);
}

TEST(thirty_one_resets_and_play_continues)
{
    // Pone K Q 9 8, dealer K 5 3 2.
    Game g = stackedGame("KS QS 9H 8H AD 4D", "KH 5C 3C 2C AC 4C", "7D");
    discardLastTwo(g);
    CHECK(playCard(g, 0, "KS"));
    CHECK(playCard(g, 1, "KH"));     // 20, pair
    CHECK_EQ(g.score(1), 2);
    CHECK(playCard(g, 0, "9H"));     // 29
    CHECK(playCard(g, 1, "2C"));     // 31
    CHECK_EQ(g.score(1), 4);
    CHECK_EQ(g.turn(), 0);
    CHECK(playCard(g, 0, "QS"));     // 10
    CHECK(playCard(g, 1, "5C"));     // 15
    CHECK_EQ(g.score(1), 6);
    CHECK(playCard(g, 0, "8H"));     // 23
    CHECK(playCard(g, 1, "3C"));     // 26, both out of cards
    CHECK_EQ(g.score(1), 7);         // last card
    CHECK(g.phase() == Phase::Show);
}

TEST(stuck_player_after_go_takes_point_without_saying_go)
{
    // Pone K Q 9 A; dealer K Q 8 7.
    Game g = stackedGame("KS QS 9H AS 2D 3D", "KH QH 8C 7C 2C 3C", "4D");
    discardLastTwo(g);
    CHECK(playCard(g, 0, "KS"));     // 10
    CHECK(playCard(g, 1, "8C"));     // 18
    CHECK(playCard(g, 0, "9H"));     // 27
    CHECK(g.sayGo(1));               // dealer holds K Q 7
    CHECK(playCard(g, 0, "AS"));     // 28, pone still holds Q
    // Pone can't play the queen; the dealer has already said go, so pone
    // takes one for go without another announcement.
    CHECK_EQ(g.score(0), 1);
    CHECK_EQ(g.count(), 0);
    CHECK_EQ(g.turn(), 1);
}

TEST(last_card_scores_one)
{
    Game g = stackedGame("KS QS 2D 3D AD 4D", "KH 9C 2C 3C AC 4C", "7D");
    discardLastTwo(g);
    // Pone K Q 2 3, dealer K 9 2 3.
    CHECK(playCard(g, 0, "2D"));
    CHECK(playCard(g, 1, "2C"));     // pair
    CHECK(playCard(g, 0, "3D"));
    CHECK(playCard(g, 1, "3C"));     // pair, count 10
    CHECK(playCard(g, 0, "KS"));     // 20
    CHECK(playCard(g, 1, "KH"));     // 30, pair
    CHECK(g.sayGo(0));
    CHECK_EQ(g.score(1), 7);         // three pairs and go
    CHECK_EQ(g.turn(), 0);
    CHECK(playCard(g, 0, "QS"));     // 10
    CHECK(playCard(g, 1, "9C"));     // 19, last card
    CHECK_EQ(g.score(1), 8);
    CHECK(g.phase() == Phase::Show);
}

TEST(last_card_making_thirty_one_scores_only_two)
{
    // Pone A A 2 2, dealer K K 4 A: all eight cards add up to 31.
    Game g = stackedGame("AS AH 2S 2H 7D 8D", "KS KH 4C AC 7C 8C", "9D");
    discardLastTwo(g);
    for (const char *c : {"AS", "KS", "AH", "KH", "2S", "4C", "2H"})
        CHECK(playCard(g, g.turn(), c));
    CHECK_EQ(g.score(1), 0);
    const size_t before = g.events().size();
    CHECK(playCard(g, 1, "AC"));
    CHECK(g.events()[before].category == Category::ThirtyOne);
    // Everything after the 31 belongs to the show, not to a last card.
    for (size_t i = before + 1; i < g.events().size(); ++i)
        CHECK(g.events()[i].category != Category::LastCard);
    CHECK_EQ(g.score(1), 2); // the dealer's hand is counted after the pone's
    CHECK(g.phase() == Phase::Show);
}

TEST(runs_do_not_carry_past_a_reset)
{
    Game g = stackedGame("KS QS 2D 3D AD 4D", "10H 9C AC 4C 5C 6C", "7D");
    discardLastTwo(g);
    // Pone K Q 2 3, dealer 10 9 A 4.
    CHECK(playCard(g, 0, "KS"));
    CHECK(playCard(g, 1, "10H"));    // 20
    CHECK(playCard(g, 0, "QS"));     // 30
    CHECK(playCard(g, 1, "AC"));     // 31
    CHECK_EQ(g.score(1), 2);
    CHECK_EQ(g.score(0), 0);
    CHECK(playCard(g, 0, "2D"));     // new count: 2
    CHECK(playCard(g, 1, "4C"));     // 6
    CHECK(playCard(g, 0, "3D"));     // 9: run 2-4-3, not A-2-3-4
    CHECK_EQ(g.score(0), 3);
}

// ---------------------------------------------------------------- scoring cards

Game showGame();

bool sameCards(const Cards &a, const std::string &b)
{
    Cards x = a;
    Cards y = parseCards(b);
    auto order = [](const Card &l, const Card &r) { return l.id() < r.id(); };
    std::sort(x.begin(), x.end(), order);
    std::sort(y.begin(), y.end(), order);
    return x == y;
}

TEST(pegging_events_name_the_cards_that_scored)
{
    Game g = stackedGame("KS QS 2D 3D AD 4D", "KH 9C 2C 3C AC 4C", "7D");
    discardLastTwo(g);
    CHECK(playCard(g, 0, "2D"));
    CHECK(playCard(g, 1, "2C"));
    CHECK(g.events().back().category == Category::Pair);
    CHECK(sameCards(g.events().back().cards, "2D 2C"));
    CHECK(playCard(g, 0, "3D"));
    CHECK(playCard(g, 1, "3C"));     // pair of threes only
    CHECK(sameCards(g.events().back().cards, "3D 3C"));
    CHECK(playCard(g, 0, "KS"));
    CHECK(playCard(g, 1, "KH"));     // 30
    CHECK(g.sayGo(0));
    CHECK(g.events().back().category == Category::Go);
    CHECK(sameCards(g.events().back().cards, "KH"));
}

TEST(fifteen_and_run_in_play_name_their_cards)
{
    const std::vector<ScoreItem> run = scorePegging(parseCards("9S 6S 4H 5C"));
    CHECK_EQ(run.size(), size_t(1));
    CHECK(run[0].category == Category::Run);
    CHECK(sameCards(run[0].cards, "6S 4H 5C")); // the nine is not part of it

    const std::vector<ScoreItem> both = scorePegging(parseCards("4S 6H 5C"));
    CHECK_EQ(both.size(), size_t(2));
    CHECK(both[0].category == Category::Fifteen);
    CHECK(sameCards(both[0].cards, "4S 6H 5C"));
}

TEST(last_card_and_nibs_name_their_cards)
{
    Game g = stackedGame("AS 2S 3S 4S 5S 6S", "AH 2H 3H 4H 5H 6H", "JC");
    discardLastTwo(g);
    CHECK(g.events().back().category == Category::Nibs);
    CHECK(sameCards(g.events().back().cards, "JC"));

    Game h = showGame();
    const size_t before = h.events().size();
    CHECK(playCard(h, 1, "8D"));
    CHECK(h.events()[before].category == Category::LastCard);
    CHECK(sameCards(h.events()[before].cards, "8D"));
}

TEST(show_events_carry_each_combination)
{
    Game g = showGame();
    CHECK(playCard(g, 1, "8D"));
    // Pone's 5 5 10 10 with a 3: four fifteens and two pairs.
    int fifteens = 0;
    int pairs = 0;
    for (const Event &e : g.events()) {
        if (!e.show)
            continue;
        CHECK_EQ(e.player, 0);
        if (e.category == Category::Fifteen) {
            ++fifteens;
            CHECK_EQ(e.cards.size(), size_t(2));
            CHECK_EQ(pileCount(e.cards), 15);
        }
        if (e.category == Category::Pair) {
            ++pairs;
            CHECK_EQ(e.cards.size(), size_t(2));
            CHECK_EQ(e.cards[0].rank, e.cards[1].rank);
        }
    }
    CHECK_EQ(fifteens, 4);
    CHECK_EQ(pairs, 2);
}

// ---------------------------------------------------------------- the show

// Pone keeps 5 5 10 10 (lays away K Q), dealer keeps 2 4 6 8 (lays away
// 9 A), starter 3. The play pegs pone 2 (31) and dealer 1 (last card).
Game showGame()
{
    Game g = stackedGame("5S 5H 10S 10H KD QD", "2C 4C 6D 8D 9C AC", "3S");
    discardLastTwo(g);
    for (const char *c : {"10S", "2C", "10H", "4C", "5S", "6D", "5H"})
        playCard(g, g.turn(), c);
    return g;
}

TEST(show_counts_pone_then_dealer_then_crib)
{
    Game g = showGame();
    CHECK(playCard(g, 1, "8D"));
    CHECK(g.phase() == Phase::Show);
    const int poneHand = hand("5S 5H 10S 10H", "3S");
    const int dealerHand = hand("2C 4C 6D 8D", "3S");
    const int crib = hand("KD QD 9C AC", "3S", true);
    CHECK_EQ(poneHand, 12);

    // Entering the show counts the pone's hand.
    CHECK_EQ(g.showStep(), 0);
    CHECK_EQ(g.showOwner(), 0);
    CHECK_EQ(g.showScore().total(), poneHand);
    CHECK_EQ(g.score(0), 2 + poneHand);
    CHECK_EQ(g.score(1), 1);

    CHECK(g.nextShow());
    CHECK_EQ(g.showStep(), 1);
    CHECK_EQ(g.showOwner(), 1);
    CHECK_EQ(g.showScore().total(), dealerHand);
    CHECK_EQ(g.score(1), 1 + dealerHand);

    CHECK(g.nextShow());
    CHECK_EQ(g.showStep(), 2);
    CHECK_EQ(g.showOwner(), 1);
    CHECK_EQ(g.showCards().size(), size_t(4));
    CHECK_EQ(g.showScore().total(), crib);
    CHECK_EQ(g.score(1), 1 + dealerHand + crib);
    CHECK_EQ(g.score(0), 2 + poneHand);
}

TEST(pone_counts_first_and_wins_before_dealer)
{
    Game g = showGame();
    g.setScores(115, 115);
    CHECK(playCard(g, 1, "8D"));     // last card: dealer 116
    CHECK_EQ(g.score(1), 116);
    // The dealer's hand would also be enough, but the pone counts first.
    CHECK(hand("2C 4C 6D 8D", "3S") >= 5);
    CHECK(g.phase() == Phase::GameOver);
    CHECK_EQ(g.winner(), 0);
    CHECK_EQ(g.score(0), 121);
    CHECK_EQ(g.score(1), 116);
    CHECK(!g.nextShow());
}

TEST(dealer_hand_wins_before_crib_is_counted)
{
    Game g = showGame();
    CHECK(playCard(g, 1, "8D"));
    g.setScores(g.score(0), 121 - hand("2C 4C 6D 8D", "3S"));
    CHECK(g.nextShow());
    CHECK(g.phase() == Phase::GameOver);
    CHECK_EQ(g.winner(), 1);
    CHECK(!g.nextShow());
}

TEST(pegging_to_121_wins_immediately)
{
    Game g = stackedGame("10S 5H 2D 3D AD 4D", "KH QH 2C 3C AC 4C", "7C");
    g.setScores(0, 119);
    discardLastTwo(g);
    CHECK(playCard(g, 0, "5H"));
    CHECK(playCard(g, 1, "QH"));     // fifteen for the dealer: 121
    CHECK(g.phase() == Phase::GameOver);
    CHECK_EQ(g.winner(), 1);
    CHECK_EQ(g.score(1), 121);
    CHECK(!playCard(g, 0, "10S"));
}

TEST(score_is_capped_at_121)
{
    Game g = stackedGame("10S 5H 2D 3D AD 4D", "KH QH 2C 3C AC 4C", "7C");
    g.setScores(0, 120);
    discardLastTwo(g);
    CHECK(playCard(g, 0, "5H"));
    CHECK(playCard(g, 1, "QH"));
    CHECK_EQ(g.score(1), 121);
}

TEST(dealer_alternates_each_round)
{
    Game g = showGame();
    CHECK(playCard(g, 1, "8D"));
    CHECK_EQ(g.round(), 1);
    g.nextShow();
    g.nextShow();
    g.nextShow();
    CHECK(g.phase() == Phase::Discard);
    CHECK_EQ(g.round(), 2);
    CHECK_EQ(g.dealer(), 0);
    CHECK_EQ(g.hand(0).size(), size_t(6));
    CHECK_EQ(g.hand(1).size(), size_t(6));
    CHECK(g.crib().empty());
    CHECK(!g.hasStarter());
}

TEST(skunks)
{
    Game g = stackedGame("10S 5H 2D 3D AD 4D", "KH QH 2C 3C AC 4C", "7C");
    g.setScores(60, 119);
    discardLastTwo(g);
    playCard(g, 0, "5H");
    playCard(g, 1, "QH");
    CHECK_EQ(g.winner(), 1);
    CHECK_EQ(g.skunk(), 2);

    Game h = stackedGame("10S 5H 2D 3D AD 4D", "KH QH 2C 3C AC 4C", "7C");
    h.setScores(61, 119);
    discardLastTwo(h);
    playCard(h, 0, "5H");
    playCard(h, 1, "QH");
    CHECK_EQ(h.skunk(), 1);

    Game k = stackedGame("10S 5H 2D 3D AD 4D", "KH QH 2C 3C AC 4C", "7C");
    k.setScores(91, 119);
    discardLastTwo(k);
    playCard(k, 0, "5H");
    playCard(k, 1, "QH");
    CHECK_EQ(k.skunk(), 0);
}

// ---------------------------------------------------------------- bot

void playOut(Game &g, Bot &a, Bot &b, int maxRounds = 60)
{
    Bot *bots[2] = {&a, &b};
    while (g.phase() != Phase::GameOver && g.round() <= maxRounds) {
        switch (g.phase()) {
        case Phase::Cut:
            for (int p = 0; p < 2; ++p)
                if (g.cutIndex(p) < 0)
                    CHECK(g.cut(p, bots[p]->chooseCut(g, p)));
            CHECK(g.finishCut());
            break;
        case Phase::Discard:
            for (int p = 0; p < 2; ++p) {
                if (g.hasDiscarded(p))
                    continue;
                const Cards hand = g.hand(p);
                const Cards d = bots[p]->chooseDiscard(hand, g.dealer() == p);
                CHECK_EQ(d.size(), size_t(2));
                CHECK(d[0] != d[1]);
                CHECK(g.discard(p, d));
            }
            break;
        case Phase::Play: {
            const int p = g.turn();
            CHECK(g.count() <= 31);
            if (g.mustSayGo(p)) {
                CHECK(g.sayGo(p));
            } else {
                const int before = g.score(p);
                CHECK(g.play(p, bots[p]->choosePlay(g, p)));
                CHECK(g.score(p) >= before);
            }
            break;
        }
        case Phase::Show:
            g.nextShow();
            break;
        case Phase::GameOver:
            break;
        }
    }
}

TEST(bots_of_every_level_play_legal_games)
{
    for (int level = 0; level < 3; ++level) {
        for (unsigned seed = 1; seed <= 5; ++seed) {
            Game g(seed);
            g.newGame();
            Bot a(static_cast<Difficulty>(level), seed);
            Bot b(static_cast<Difficulty>(level), seed + 100);
            playOut(g, a, b);
            CHECK(g.phase() == Phase::GameOver);
            CHECK(g.score(g.winner()) == 121);
            CHECK(g.score(1 - g.winner()) < 121);
        }
    }
}

TEST(medium_bot_keeps_best_hand)
{
    Bot bot(Difficulty::Medium, 3);
    const Cards d = bot.chooseDiscard(parseCards("5C 5D 5H JS 9D 2C"), false);
    CHECK(std::find(d.begin(), d.end(), parseCard("9D")) != d.end());
    CHECK(std::find(d.begin(), d.end(), parseCard("2C")) != d.end());
}

TEST(medium_and_hard_bots_take_fifteen)
{
    Game g = stackedGame("10S 2D 3D 4D AD 6D", "5H 9C 8C 7C AC 4C", "QC");
    discardLastTwo(g);
    CHECK(playCard(g, 0, "10S"));
    for (Difficulty d : {Difficulty::Medium, Difficulty::Hard}) {
        Bot bot(d, 5);
        CHECK(bot.choosePlay(g, 1) == parseCard("5H"));
    }
}

TEST(hard_bot_takes_thirty_one)
{
    Game g = stackedGame("KS QS 2D 3D AD 4D", "KH AC 9C 7C 5C 6C", "8D");
    discardLastTwo(g);
    // Pone K Q 2 3, dealer K A 9 7.
    CHECK(playCard(g, 0, "KS"));
    CHECK(playCard(g, 1, "9C"));
    CHECK(playCard(g, 0, "QS"));     // 29
    Bot bot(Difficulty::Hard, 7);
    // Only the ace fits.
    CHECK(bot.choosePlay(g, 1) == parseCard("AC"));
}

TEST(hard_bot_does_not_lead_a_five)
{
    Game g = stackedGame("5S 4H 9D KC AD 2D", "KH QH 2C 3C AC 4C", "8D");
    discardLastTwo(g);
    Bot bot(Difficulty::Hard, 11);
    CHECK(bot.choosePlay(g, 0) != parseCard("5S"));
}

TEST(hard_bot_values_fives_more_in_own_crib)
{
    Bot bot(Difficulty::Hard, 13);
    const Cards kept = parseCards("9S 10H KC QD");
    const Cards fives = parseCards("5C 5D");
    CHECK(bot.expectedDiscardValue(kept, fives, true) > bot.expectedDiscardValue(kept, fives, false));
}

TEST(hard_bot_keeps_twenty_nine_material)
{
    Bot bot(Difficulty::Hard, 17);
    const Cards d = bot.chooseDiscard(parseCards("5C 5D 5H JS 2D 9S"), true);
    CHECK(std::find(d.begin(), d.end(), parseCard("5C")) == d.end());
    CHECK(std::find(d.begin(), d.end(), parseCard("JS")) == d.end());
}

int winsOf(Difficulty strong, Difficulty weak, int games, unsigned seed)
{
    int wins = 0;
    for (int i = 0; i < games; ++i) {
        Game g(seed + i);
        g.newGame();
        Bot a(weak, seed + 1000 + i);
        Bot b(strong, seed + 2000 + i);
        playOut(g, a, b);
        if (g.winner() == 1)
            ++wins;
    }
    return wins;
}

TEST(stronger_levels_win_more)
{
    const int games = 40;
    const int hardVsEasy = winsOf(Difficulty::Hard, Difficulty::Easy, games, 100);
    const int mediumVsEasy = winsOf(Difficulty::Medium, Difficulty::Easy, games, 200);
    const int hardVsMedium = winsOf(Difficulty::Hard, Difficulty::Medium, games, 300);
    std::printf("  of %d: hard-easy %d, medium-easy %d, hard-medium %d\n", games, hardVsEasy,
                mediumVsEasy, hardVsMedium);
    CHECK(hardVsEasy > games * 3 / 4);
    CHECK(mediumVsEasy > games * 3 / 5);
    CHECK(hardVsMedium > games / 2);
}

} // namespace

int main()
{
    for (const Test &t : tests()) {
        g_test = t.name;
        const int before = g_failures;
        t.body();
        std::printf("%s %s\n", g_failures == before ? "ok  " : "FAIL", t.name);
    }
    std::printf("\n%zu tests, %d checks, %d failures\n", tests().size(), g_checks, g_failures);
    return g_failures ? 1 : 0;
}
