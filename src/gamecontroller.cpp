#include "gamecontroller.h"

#include "cards/settings.h"
#include "cards/variant.h"

#include <QVariantMap>

#include <algorithm>

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

QVariantList shuffledDeck()
{
    static std::mt19937 rng{std::random_device{}()};
    Cards deck = fullDeck();
    std::shuffle(deck.begin(), deck.end(), rng);
    return ids(deck);
}

} // namespace

GameController::GameController(Session *online, QObject *parent)
    : QObject(parent)
    , m_online(online)
    , m_settings(cards::settingsPath(), QSettings::IniFormat)
{
    m_difficulty = qBound(0, m_settings.value(QStringLiteral("difficulty"), 1).toInt(), 2);
    m_scoringSpeed = qBound(0, m_settings.value(QStringLiteral("scoringSpeed"), 1).toInt(), 3);
    m_tapTwice = m_settings.value(QStringLiteral("tapTwice"), false).toBool();
    m_botTimer.setSingleShot(true);
    m_botTimer.setInterval(BotDelayMs);
    connect(&m_botTimer, &QTimer::timeout, this, &GameController::botStep);

    connect(m_online, &Session::reset, this, [this]() {
        if (isOnline())
            replay();
    });
    connect(m_online, &Session::action, this, [this](const QVariantMap &action) {
        if (isOnline())
            applyAction(action, true);
    });
    connect(m_online, &Session::tableChanged, this, &GameController::onTableChanged);
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

QString GameController::opponent() const
{
    return isOnline() ? m_online->playerName(m_online->toTable(Computer)) : QString();
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

void GameController::setTapTwice(bool on)
{
    if (on == m_tapTwice)
        return;
    m_tapTwice = on;
    m_settings.setValue(QStringLiteral("tapTwice"), on);
    emit tapTwiceChanged();
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

void GameController::start()
{
    if (isOnline())
        return;
    m_botTimer.stop();
    m_game = Game();
    m_started = false;
    bool ok = true;
    for (const QVariant &action : m_log.load())
        ok = ok && applyAction(action.toMap(), false);
    if (!ok || !m_started) {
        newGame();
        return;
    }
    // A finished game was counted before the app closed.
    m_recorded = m_game.phase() == cribbage::Phase::GameOver;
    m_seenEvents = m_game.events().size();
    m_message.clear();
    afterAction(-1);
}

void GameController::newGame()
{
    if (isOnline()) {
        if (m_game.phase() == cribbage::Phase::GameOver)
            sendNew();
        return;
    }
    m_botTimer.stop();
    m_log.clear();
    m_started = false;
    m_recorded = false;
    perform({{QStringLiteral("t"), QStringLiteral("new")},
             {QStringLiteral("difficulty"), m_difficulty},
             {QStringLiteral("cut"), shuffledDeck()}});
}

bool GameController::cut(int index)
{
    Game trial = m_game;
    if (!m_started || !trial.cut(Human, index))
        return false;
    return perform({{QStringLiteral("t"), QStringLiteral("cut")},
                    {QStringLiteral("seat"), m_online->toTable(Human)},
                    {QStringLiteral("index"), index}});
}

bool GameController::discard(const QVariantList &cardIds)
{
    Game trial = m_game;
    Cards cards;
    if (!m_started || !cardsFrom(cardIds, &cards) || !trial.discard(Human, cards))
        return false;
    return perform({{QStringLiteral("t"), QStringLiteral("discard")},
                    {QStringLiteral("seat"), m_online->toTable(Human)},
                    {QStringLiteral("cards"), cardIds}});
}

bool GameController::play(int cardId)
{
    if (!canPlay(cardId))
        return false;
    return perform({{QStringLiteral("t"), QStringLiteral("play")},
                    {QStringLiteral("seat"), m_online->toTable(Human)},
                    {QStringLiteral("card"), cardId}});
}

bool GameController::sayGo()
{
    if (!m_game.mustSayGo(Human))
        return false;
    return perform({{QStringLiteral("t"), QStringLiteral("go")}, {QStringLiteral("seat"), m_online->toTable(Human)}});
}

bool GameController::nextShow()
{
    // Either player may move the show on; the step stops a second press
    // from skipping one.
    if (m_game.phase() != cribbage::Phase::Show)
        return false;
    return perform({{QStringLiteral("t"), QStringLiteral("next")},
                    {QStringLiteral("round"), m_game.round()},
                    {QStringLiteral("step"), m_game.showStep()},
                    {QStringLiteral("deck"), shuffledDeck()}});
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
        return ids(Cards{advisor.choosePlay(m_game, Human)});
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
        parts << tr("%1: %2").arg(opponent().isEmpty() ? tr("Bot") : opponent(), bot.join(QStringLiteral(", ")));
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
        const QString name = opponent();
        if (endedInShow())
            parts << (human ? tr("You counted out.") : name.isEmpty() ? tr("The bot counted out.") : tr("%1 counted out.").arg(name));
        else
            parts << (human ? tr("You pegged out.") : name.isEmpty() ? tr("The bot pegged out.") : tr("%1 pegged out.").arg(name));
    }
    if (!scored.isEmpty())
        parts << scored;
    if (!parts.isEmpty() || actor >= 0)
        m_message = parts.join(QStringLiteral(" "));

    if (m_game.phase() == cribbage::Phase::GameOver)
        recordResult();

    schedule();
    emit changed();
}

void GameController::schedule()
{
    // Online only the driver moves the game on, and plays a bot's seat.
    if (!m_started || (isOnline() && !m_online->driver())) {
        m_botTimer.stop();
        return;
    }
    const bool bot = runsBot();

    // The bot lays away as soon as the cards are dealt.
    if (bot && m_game.phase() == cribbage::Phase::Discard && !m_game.hasDiscarded(Computer)) {
        const QVariantMap action = {{QStringLiteral("t"), QStringLiteral("discard")},
                                    {QStringLiteral("seat"), m_online->toTable(Computer)},
                                    {QStringLiteral("cards"), ids(m_bot.chooseDiscard(m_game.hand(Computer), m_game.dealer() == Computer))}};
        if (isOnline())
            m_online->send(action);
        else if (applyAction(action, false))
            m_log.append(action);
    }

    if (m_game.phase() == cribbage::Phase::Cut && (m_game.cutComplete() || (bot && m_game.cutIndex(Human) >= 0))) {
        // The bot cuts after you, then both cards stay up for a moment.
        m_botTimer.setInterval(m_game.cutComplete() ? CutResultMs : BotDelayMs);
        m_botTimer.start();
    } else if (bot && botTurn()) {
        // Let the table finish showing the points before the bot plays on.
        m_botTimer.setInterval(BotDelayMs + revealMs() * m_lastScoring.size());
        m_botTimer.start();
    }
}

void GameController::botStep()
{
    if (isOnline() && !m_online->driver())
        return;
    if (m_game.phase() == cribbage::Phase::Cut) {
        if (m_game.cutComplete())
            perform({{QStringLiteral("t"), QStringLiteral("finish")}, {QStringLiteral("deck"), shuffledDeck()}});
        else if (runsBot() && m_game.cutIndex(Computer) < 0)
            perform({{QStringLiteral("t"), QStringLiteral("cut")},
                     {QStringLiteral("seat"), m_online->toTable(Computer)},
                     {QStringLiteral("index"), m_bot.chooseCut(m_game, Computer)}});
        return;
    }
    if (!runsBot() || !botTurn())
        return;
    if (m_game.mustSayGo(Computer))
        perform({{QStringLiteral("t"), QStringLiteral("go")}, {QStringLiteral("seat"), m_online->toTable(Computer)}});
    else
        perform({{QStringLiteral("t"), QStringLiteral("play")},
                 {QStringLiteral("seat"), m_online->toTable(Computer)},
                 {QStringLiteral("card"), m_bot.choosePlay(m_game, Computer).id()}});
}

bool GameController::perform(const QVariantMap &action)
{
    if (isOnline()) {
        m_online->send(action);
        return true;
    }
    return applyAction(action, true);
}

void GameController::recordResult()
{
    // Statistics are for games against the bot.
    if (m_recorded || isOnline())
        return;
    m_recorded = true;
    const QString key = QStringLiteral("stats/%1/").arg(gameDifficulty());
    const QString result = m_game.winner() == Human ? QStringLiteral("won") : QStringLiteral("lost");
    m_settings.setValue(key + result, m_settings.value(key + result, 0).toInt() + 1);
    if (m_game.winner() == Human && m_game.skunk() > 0)
        m_settings.setValue(key + QStringLiteral("skunks"), m_settings.value(key + QStringLiteral("skunks"), 0).toInt() + 1);
    emit statsChanged();
}

bool GameController::runsBot() const
{
    return !isOnline() || (m_online->driver() && m_online->isBot(m_online->toTable(Computer)));
}

void GameController::sendNew()
{
    m_online->send({{QStringLiteral("t"), QStringLiteral("new")}, {QStringLiteral("cut"), shuffledDeck()}});
}

bool GameController::applyAction(const QVariantMap &action, bool live)
{
    const QString t = action.value(QStringLiteral("t")).toString();
    const int seat = action.value(QStringLiteral("seat"), -1).toInt();
    const int player = m_online->toLocal(seat);
    int actor = player;
    QString announcement;
    Cards cards;
    bool ok = false;
    if (t == QLatin1String("new")) {
        // Both players may ask for the next game; the first one starts it.
        ok = (!m_started || m_game.phase() == cribbage::Phase::GameOver)
             && deckFrom(action.value(QStringLiteral("cut")), &cards);
        if (ok) {
            m_bot.setDifficulty(static_cast<Difficulty>(qBound(0, action.value(QStringLiteral("difficulty"), m_difficulty).toInt(), 2)));
            m_game.newGame(cards);
            m_started = true;
            m_seenEvents = 0;
            m_message.clear();
            actor = -1;
        }
    } else if (t == QLatin1String("finish")) {
        ok = deckFrom(action.value(QStringLiteral("deck")), &cards) && m_game.finishCut(cards);
        actor = Computer;
    } else if (t == QLatin1String("next")) {
        ok = action.value(QStringLiteral("round")).toInt() == m_game.round()
             && action.value(QStringLiteral("step")).toInt() == m_game.showStep()
             && deckFrom(action.value(QStringLiteral("deck")), &cards) && m_game.nextShow(cards);
        actor = -1;
    } else if (player < 0) {
        return false;
    } else if (t == QLatin1String("cut")) {
        ok = m_game.cut(player, action.value(QStringLiteral("index"), -1).toInt());
    } else if (t == QLatin1String("discard")) {
        ok = cardsFrom(action.value(QStringLiteral("cards")), &cards) && m_game.discard(player, cards);
    } else if (t == QLatin1String("play")) {
        const int id = action.value(QStringLiteral("card"), -1).toInt();
        ok = id >= 0 && id < 52 && m_game.play(player, Card::fromId(id));
        if (ok && player != Human)
            announcement = opponent().isEmpty() ? tr("The bot plays %1.").arg(cardText(Card::fromId(id)))
                                                : tr("%1 plays %2.").arg(opponent(), cardText(Card::fromId(id)));
    } else if (t == QLatin1String("go")) {
        ok = m_game.sayGo(player);
        announcement = player == Human ? tr("You say go.")
                       : opponent().isEmpty() ? tr("The bot says go.") : tr("%1 says go.").arg(opponent());
    }
    // Against the bot, each move is written down as it is made.
    if (ok && live && !isOnline())
        m_log.append(action);
    if (ok && live)
        afterAction(actor, announcement);
    return ok;
}

void GameController::replay()
{
    m_botTimer.stop();
    m_game = Game();
    m_started = false;
    m_recorded = false;
    for (const QVariant &action : m_online->actions())
        applyAction(action.toMap(), false);
    m_seenEvents = m_game.events().size();
    m_message.clear();
    m_wasDriver = m_online->driver();
    if (m_online->synced() && m_online->actions().isEmpty() && m_online->driver())
        sendNew();
    afterAction(-1);
}

void GameController::onTableChanged()
{
    const bool online = isOnline();
    if (online != m_wasOnline) {
        m_wasOnline = online;
        m_wasDriver = false;
        // Off the table, your game against the bot is back; on it, the game
        // waits for the log.
        if (online)
            replay();
        else
            start();
        return;
    }
    if (online) {
        // The driver may have changed, or a bot taken over an absent player.
        if (m_online->driver() && !m_wasDriver && m_online->synced() && m_online->actions().isEmpty())
            sendNew();
        m_wasDriver = m_online->driver();
        schedule();
    }
    emit changed();
}
