#include "gamecontroller.h"

#include <QVariantMap>

using namespace cribbage;

namespace {

const int BotDelayMs = 900;
// How long both cut cards stay up before the deal (or the cut again).
const int CutResultMs = 2200;
// How long the table shows each score, from slow to very fast.
const int RevealMs[] = {1700, 1000, 600, 300};

QString cardText(const Card &c)
{
    static const char *ranks[] = {"A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K"};
    static const QString suits[] = {QStringLiteral("♣"), QStringLiteral("♦"),
                                    QStringLiteral("♥"), QStringLiteral("♠")};
    return QString::fromLatin1(ranks[c.rank - 1]) + suits[c.suit];
}

QVariantList ids(const Cards &cards)
{
    QVariantList out;
    for (const Card &c : cards)
        out << c.id();
    return out;
}

Cards fromIds(const QVariantList &list)
{
    Cards out;
    for (const QVariant &v : list)
        out.push_back(Card::fromId(v.toInt()));
    return out;
}

} // namespace

GameController::GameController(QObject *parent)
    : QObject(parent)
{
    m_difficulty = qBound(0, m_settings.value(QStringLiteral("difficulty"), 1).toInt(), 2);
    m_scoringSpeed = qBound(0, m_settings.value(QStringLiteral("scoringSpeed"), 1).toInt(), 3);
    m_botTimer.setSingleShot(true);
    m_botTimer.setInterval(BotDelayMs);
    connect(&m_botTimer, &QTimer::timeout, this, &GameController::botStep);
}

int GameController::phase() const
{
    switch (m_game.phase()) {
    case cribbage::Phase::Cut:
        return Cut;
    case cribbage::Phase::Discard:
        return Discard;
    case cribbage::Phase::Play:
        return Play;
    case cribbage::Phase::Show:
        return Show;
    case cribbage::Phase::GameOver:
        break;
    }
    return GameOver;
}

QVariantList GameController::hand() const
{
    return ids(m_game.hand(Human));
}

QVariantList GameController::cutDeck() const
{
    return m_game.phase() == cribbage::Phase::Cut ? ids(m_game.cutDeck()) : QVariantList();
}

int GameController::humanCut() const
{
    const int index = m_game.cutIndex(Human);
    return m_game.phase() == cribbage::Phase::Cut && index >= 0 ? m_game.cutDeck()[index].id() : -1;
}

int GameController::botCut() const
{
    const int index = m_game.cutIndex(Computer);
    return m_game.phase() == cribbage::Phase::Cut && index >= 0 ? m_game.cutDeck()[index].id() : -1;
}

QVariantList GameController::botHand() const
{
    return ids(m_game.hand(Computer));
}

QVariantList GameController::humanKept() const
{
    return ids(m_game.keptHand(Human));
}

QVariantList GameController::botKept() const
{
    return ids(m_game.keptHand(Computer));
}

QVariantList GameController::crib() const
{
    return ids(m_game.crib());
}

QVariantList GameController::played() const
{
    // Every card of the round; those before the last reset are marked old.
    QVariantList out;
    const Cards &all = m_game.played();
    const size_t current = all.size() - m_game.pile().size();
    for (size_t i = 0; i < all.size(); ++i) {
        QVariantMap m;
        m.insert(QStringLiteral("card"), all[i].id());
        m.insert(QStringLiteral("player"), m_game.playedBy()[i]);
        m.insert(QStringLiteral("current"), i >= current);
        out << m;
    }
    return out;
}

bool GameController::humanTurn() const
{
    return m_started && m_game.phase() == cribbage::Phase::Play && m_game.turn() == Human;
}

bool GameController::botTurn() const
{
    return m_started && m_game.phase() == cribbage::Phase::Play && m_game.turn() == Computer;
}

QVariantList GameController::showCards() const
{
    if (m_game.phase() != cribbage::Phase::Show && m_game.phase() != cribbage::Phase::GameOver)
        return QVariantList();
    return ids(m_game.showCards());
}

QVariantList GameController::showItems() const
{
    QVariantList out;
    if (m_game.phase() != cribbage::Phase::Show && !endedInShow())
        return out;
    for (const ScoreItem &item : m_game.showScore().items) {
        QVariantMap m;
        m.insert(QStringLiteral("name"), categoryName(static_cast<int>(item.category), item.points));
        m.insert(QStringLiteral("points"), item.points);
        m.insert(QStringLiteral("cards"), ids(item.cards));
        out << m;
    }
    return out;
}

bool GameController::endedInShow() const
{
    return m_game.phase() == cribbage::Phase::GameOver && !m_game.events().empty() && m_game.events().back().show;
}

int GameController::showTotal() const
{
    return m_game.phase() == cribbage::Phase::Show || endedInShow() ? m_game.showScore().total() : 0;
}

void GameController::setDifficulty(int difficulty)
{
    difficulty = qBound(0, difficulty, 2);
    if (difficulty == m_difficulty)
        return;
    m_difficulty = difficulty;
    m_settings.setValue(QStringLiteral("difficulty"), difficulty);
    emit difficultyChanged();
}

void GameController::setScoringSpeed(int speed)
{
    speed = qBound(0, speed, 3);
    if (speed == m_scoringSpeed)
        return;
    m_scoringSpeed = speed;
    m_settings.setValue(QStringLiteral("scoringSpeed"), speed);
    emit scoringSpeedChanged();
}

int GameController::revealMs() const
{
    return RevealMs[m_scoringSpeed];
}

QVariantList GameController::stats() const
{
    QVariantList out;
    for (int level = 0; level < 3; ++level) {
        const QString key = QStringLiteral("stats/%1/").arg(level);
        QVariantMap m;
        m.insert(QStringLiteral("won"), m_settings.value(key + QStringLiteral("won"), 0).toInt());
        m.insert(QStringLiteral("lost"), m_settings.value(key + QStringLiteral("lost"), 0).toInt());
        m.insert(QStringLiteral("skunks"), m_settings.value(key + QStringLiteral("skunks"), 0).toInt());
        out << m;
    }
    return out;
}

void GameController::resetStats()
{
    m_settings.remove(QStringLiteral("stats"));
    emit statsChanged();
}

void GameController::newGame()
{
    m_botTimer.stop();
    m_bot.setDifficulty(static_cast<Difficulty>(m_difficulty));
    m_game.newGame();
    m_started = true;
    m_recorded = false;
    m_seenEvents = 0;
    m_message.clear();
    afterAction(-1);
}

bool GameController::cut(int index)
{
    if (!m_game.cut(Human, index))
        return false;
    afterAction(Human);
    return true;
}

bool GameController::discard(const QVariantList &cardIds)
{
    if (!m_started || !m_game.discard(Human, fromIds(cardIds)))
        return false;
    afterAction(Human);
    return true;
}

bool GameController::play(int cardId)
{
    if (!humanTurn() || !m_game.play(Human, Card::fromId(cardId)))
        return false;
    afterAction(Human);
    return true;
}

bool GameController::sayGo()
{
    if (!m_game.sayGo(Human))
        return false;
    afterAction(Human, tr("You say go."));
    return true;
}

bool GameController::nextShow()
{
    if (!m_game.nextShow())
        return false;
    afterAction(-1);
    return true;
}

bool GameController::canPlay(int cardId) const
{
    return humanTurn() && m_game.canPlayCard(Human, Card::fromId(cardId));
}

QVariantList GameController::hint()
{
    Bot advisor(Difficulty::Hard);
    if (m_game.phase() == cribbage::Phase::Discard && !m_game.hasDiscarded(Human))
        return ids(advisor.chooseDiscard(m_game.hand(Human), m_game.dealer() == Human));
    if (humanTurn() && m_game.canPlay(Human))
        return ids({advisor.choosePlay(m_game, Human)});
    return QVariantList();
}

QString GameController::categoryName(int category, int points) const
{
    switch (static_cast<Category>(category)) {
    case Category::Fifteen:
        return tr("Fifteen");
    case Category::Pair:
        return points >= 12 ? tr("Double pair royal") : points >= 6 ? tr("Pair royal") : tr("Pair");
    case Category::Run:
        return tr("Run of %1").arg(points);
    case Category::Flush:
        return tr("Flush");
    case Category::Nobs:
        return tr("His nobs");
    case Category::Nibs:
        return tr("His heels");
    case Category::ThirtyOne:
        return tr("Thirty-one");
    case Category::Go:
        return tr("Go");
    case Category::LastCard:
        return tr("Last card");
    }
    return QString();
}

QString GameController::describeEvents(size_t from) const
{
    QStringList human;
    QStringList bot;
    const std::vector<Event> &events = m_game.events();
    for (size_t i = from; i < events.size(); ++i) {
        const Event &e = events[i];
        if (e.show)
            continue;
        const QString text = tr("%1 for %2").arg(categoryName(static_cast<int>(e.category), e.points)).arg(e.points);
        (e.player == Human ? human : bot) << text;
    }
    QStringList parts;
    if (!human.isEmpty())
        parts << tr("You: %1").arg(human.join(QStringLiteral(", ")));
    if (!bot.isEmpty())
        parts << tr("Bot: %1").arg(bot.join(QStringLiteral(", ")));
    return parts.join(QStringLiteral(" · "));
}

void GameController::afterAction(int actor, const QString &announcement)
{
    const QString scored = describeEvents(m_seenEvents);
    m_lastScoring.clear();
    const std::vector<Event> &events = m_game.events();
    for (size_t i = m_seenEvents; i < events.size(); ++i) {
        if (events[i].show)
            continue;
        QVariantMap m;
        m.insert(QStringLiteral("player"), events[i].player);
        m.insert(QStringLiteral("name"), categoryName(static_cast<int>(events[i].category), events[i].points));
        m.insert(QStringLiteral("points"), events[i].points);
        m.insert(QStringLiteral("cards"), ids(events[i].cards));
        m_lastScoring << m;
    }
    m_seenEvents = events.size();
    QStringList parts;
    if (!announcement.isEmpty())
        parts << announcement;
    if (m_game.phase() == cribbage::Phase::GameOver) {
        const bool human = m_game.winner() == Human;
        if (endedInShow())
            parts << (human ? tr("You counted out.") : tr("The bot counted out."));
        else
            parts << (human ? tr("You pegged out.") : tr("The bot pegged out."));
    }
    if (!scored.isEmpty())
        parts << scored;
    if (!parts.isEmpty() || actor >= 0)
        m_message = parts.join(QStringLiteral(" "));

    if (m_game.phase() == cribbage::Phase::GameOver)
        recordResult();

    // The bot lays away as soon as the cards are dealt.
    if (m_game.phase() == cribbage::Phase::Discard && !m_game.hasDiscarded(Computer))
        m_game.discard(Computer, m_bot.chooseDiscard(m_game.hand(Computer), m_game.dealer() == Computer));

    if (m_game.phase() == cribbage::Phase::Cut && m_game.cutIndex(Human) >= 0) {
        // The bot cuts after you, then both cards stay up for a moment.
        m_botTimer.setInterval(m_game.cutComplete() ? CutResultMs : BotDelayMs);
        m_botTimer.start();
    } else if (botTurn()) {
        // Let the table finish showing the points before the bot plays on.
        m_botTimer.setInterval(BotDelayMs + revealMs() * m_lastScoring.size());
        m_botTimer.start();
    }
    emit changed();
}

void GameController::botStep()
{
    if (m_game.phase() == cribbage::Phase::Cut) {
        if (m_game.cutComplete())
            m_game.finishCut();
        else
            m_game.cut(Computer, m_bot.chooseCut(m_game, Computer));
        afterAction(Computer);
        return;
    }
    if (!botTurn())
        return;
    if (m_game.mustSayGo(Computer)) {
        m_game.sayGo(Computer);
        afterAction(Computer, tr("The bot says go."));
        return;
    }
    const Card card = m_bot.choosePlay(m_game, Computer);
    m_game.play(Computer, card);
    afterAction(Computer, tr("The bot plays %1.").arg(cardText(card)));
}

void GameController::recordResult()
{
    if (m_recorded)
        return;
    m_recorded = true;
    const QString key = QStringLiteral("stats/%1/").arg(gameDifficulty());
    const QString result = m_game.winner() == Human ? QStringLiteral("won") : QStringLiteral("lost");
    m_settings.setValue(key + result, m_settings.value(key + result, 0).toInt() + 1);
    if (m_game.winner() == Human && m_game.skunk() > 0)
        m_settings.setValue(key + QStringLiteral("skunks"), m_settings.value(key + QStringLiteral("skunks"), 0).toInt() + 1);
    emit statsChanged();
}
