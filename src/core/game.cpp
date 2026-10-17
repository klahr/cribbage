#include "game.h"

#include <algorithm>

namespace cribbage {

namespace {

bool contains(const Cards &cards, const Card &card)
{
    return std::find(cards.begin(), cards.end(), card) != cards.end();
}

void removeCard(Cards &cards, const Card &card)
{
    cards.erase(std::find(cards.begin(), cards.end(), card));
}

} // namespace

Game::Game(unsigned seed)
    : m_rng(seed)
{
}

void Game::newGame()
{
    m_score[0] = m_score[1] = 0;
    m_previous[0] = m_previous[1] = 0;
    m_winner = -1;
    m_round = 0;
    m_events.clear();
    for (int p = 0; p < Players; ++p) {
        m_hand[p].clear();
        m_kept[p].clear();
    }
    m_crib.clear();
    m_starter = Card();
    m_pile.clear();
    m_played.clear();
    m_playedBy.clear();
    m_phase = Phase::Cut;
    m_cutDeck = fullDeck();
    std::shuffle(m_cutDeck.begin(), m_cutDeck.end(), m_rng);
    m_cutIndex[0] = m_cutIndex[1] = -1;
}

bool Game::cut(int player, int index)
{
    if (m_phase != Phase::Cut || m_cutIndex[player] >= 0 || index < 0
        || index >= static_cast<int>(m_cutDeck.size()) || index == m_cutIndex[1 - player])
        return false;
    m_cutIndex[player] = index;
    m_firstCut[player] = m_cutDeck[index];
    return true;
}

int Game::cutDealer() const
{
    if (!cutComplete() || cutTied())
        return -1;
    return m_firstCut[0].rank < m_firstCut[1].rank ? 0 : 1;
}

bool Game::finishCut()
{
    if (m_phase != Phase::Cut || !cutComplete())
        return false;
    if (cutTied()) {
        std::shuffle(m_cutDeck.begin(), m_cutDeck.end(), m_rng);
        m_cutIndex[0] = m_cutIndex[1] = -1;
        return true;
    }
    Cards deck = fullDeck();
    std::shuffle(deck.begin(), deck.end(), m_rng);
    newGame(cutDealer(), deck);
    return true;
}

void Game::newGame(int firstDealer, const Cards &deck)
{
    m_score[0] = m_score[1] = 0;
    m_previous[0] = m_previous[1] = 0;
    m_winner = -1;
    m_round = 0;
    m_events.clear();
    m_dealer = firstDealer;
    dealFrom(deck);
}

void Game::dealFrom(const Cards &deck)
{
    m_deck = deck;
    ++m_round;
    m_phase = Phase::Discard;
    for (int p = 0; p < Players; ++p) {
        m_hand[p].clear();
        m_kept[p].clear();
        m_discarded[p] = false;
        m_go[p] = false;
    }
    m_crib.clear();
    m_starter = Card();
    m_pile.clear();
    m_played.clear();
    m_playedBy.clear();
    m_lastPlayer = -1;
    m_showStep = 0;
    for (int i = 0; i < 12; ++i)
        m_hand[i % 2 == 0 ? pone() : m_dealer].push_back(m_deck[i]);
}

void Game::dealShuffled()
{
    Cards deck = fullDeck();
    std::shuffle(deck.begin(), deck.end(), m_rng);
    dealFrom(deck);
}

void Game::setScores(int first, int second)
{
    m_score[0] = m_previous[0] = first;
    m_score[1] = m_previous[1] = second;
}

bool Game::discard(int player, const Cards &cards)
{
    if (m_phase != Phase::Discard || m_discarded[player] || cards.size() != 2 || cards[0] == cards[1])
        return false;
    for (const Card &c : cards)
        if (!contains(m_hand[player], c))
            return false;
    for (const Card &c : cards) {
        removeCard(m_hand[player], c);
        m_crib.push_back(c);
    }
    m_kept[player] = m_hand[player];
    m_discarded[player] = true;
    if (m_discarded[0] && m_discarded[1])
        startPlay();
    return true;
}

void Game::startPlay()
{
    // The pone cuts; the dealer turns up the top card of the lower packet.
    m_starter = m_deck[12];
    m_phase = Phase::Play;
    if (m_starter.rank == 11)
        award(m_dealer, Category::Nibs, 2, {m_starter});
    if (m_phase == Phase::GameOver)
        return;
    m_turn = pone();
    settle();
}

bool Game::canPlayCard(int player, const Card &card) const
{
    return m_phase == Phase::Play && contains(m_hand[player], card) && count() + card.value() <= 31;
}

bool Game::canPlay(int player) const
{
    for (const Card &c : m_hand[player])
        if (count() + c.value() <= 31)
            return true;
    return false;
}

bool Game::mustSayGo(int player) const
{
    return m_phase == Phase::Play && m_turn == player && !m_hand[player].empty() && !canPlay(player)
           && !m_go[player];
}

bool Game::play(int player, const Card &card)
{
    if (m_phase != Phase::Play || m_turn != player || !canPlayCard(player, card))
        return false;
    removeCard(m_hand[player], card);
    m_pile.push_back(card);
    m_played.push_back(card);
    m_playedBy.push_back(player);
    m_lastPlayer = player;
    award(player, scorePegging(m_pile));
    if (m_phase == Phase::GameOver)
        return true;
    if (count() == 31) {
        resetCount();
        m_turn = 1 - player;
    } else {
        // After the opponent has said go, the same player keeps playing.
        m_turn = m_go[1 - player] ? player : 1 - player;
    }
    settle();
    return true;
}

bool Game::sayGo(int player)
{
    if (!mustSayGo(player))
        return false;
    m_go[player] = true;
    m_turn = 1 - player;
    settle();
    return true;
}

void Game::resetCount()
{
    m_pile.clear();
    m_go[0] = m_go[1] = false;
}

void Game::settle()
{
    while (m_phase == Phase::Play) {
        if (m_hand[0].empty() && m_hand[1].empty()) {
            if (!m_pile.empty())
                award(m_lastPlayer, Category::LastCard, 1, {m_played.back()});
            if (m_phase == Phase::Play)
                startShow();
            return;
        }
        const int p = m_turn;
        const int o = 1 - p;
        if (canPlay(p))
            return;
        // A player holding cards announces go themselves, unless the
        // opponent is already out, in which case the go is the player's.
        if (!m_go[p] && !m_hand[p].empty() && !m_go[o])
            return;
        m_go[p] = true;
        if (canPlay(o)) {
            m_turn = o;
            return;
        }
        // Nobody can play: the last player to lay a card pegs one for go.
        award(m_lastPlayer, Category::Go, 1, {m_played.back()});
        if (m_phase != Phase::Play)
            return;
        resetCount();
        m_turn = 1 - m_lastPlayer;
    }
}

void Game::startShow()
{
    m_phase = Phase::Show;
    m_showScores[0] = scoreHand(m_kept[pone()], m_starter, false);
    m_showScores[1] = scoreHand(m_kept[m_dealer], m_starter, false);
    m_showScores[2] = scoreHand(m_crib, m_starter, true);
    m_showStep = 0;
    applyShow();
}

void Game::applyShow()
{
    award(showOwner(), m_showScores[m_showStep].items);
}

bool Game::nextShow()
{
    if (m_phase != Phase::Show)
        return false;
    if (m_showStep < 2) {
        ++m_showStep;
        applyShow();
    } else {
        m_dealer = pone();
        dealShuffled();
    }
    return true;
}

const Cards &Game::showCards() const
{
    if (m_showStep == 2)
        return m_crib;
    return m_kept[showOwner()];
}

void Game::award(int player, Category category, int points, const Cards &cards)
{
    award(player, {{category, points, cards}});
}

void Game::award(int player, const std::vector<ScoreItem> &items)
{
    if (m_phase == Phase::GameOver || items.empty())
        return;
    m_previous[player] = m_score[player];
    for (const ScoreItem &item : items) {
        m_score[player] += item.points;
        m_events.push_back({player, item.category, item.points, m_phase == Phase::Show, item.cards});
    }
    m_score[player] = std::min(WinningScore, m_score[player]);
    if (m_score[player] >= WinningScore) {
        m_winner = player;
        m_phase = Phase::GameOver;
    }
}

int Game::skunk() const
{
    if (m_winner < 0)
        return 0;
    const int loser = m_score[1 - m_winner];
    if (loser < 61)
        return 2;
    if (loser < 91)
        return 1;
    return 0;
}

} // namespace cribbage
