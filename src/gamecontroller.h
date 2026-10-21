#pragma once

#include "core/bot.h"
#include "core/game.h"
#include "cards/gamelog.h"
#include "online/session.h"

#include <QObject>
#include <QSettings>
#include <QTimer>
#include <QVariantList>

class GameController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int phase READ phase NOTIFY changed)
    Q_PROPERTY(int dealer READ dealer NOTIFY changed)
    Q_PROPERTY(int humanScore READ humanScore NOTIFY changed)
    Q_PROPERTY(int botScore READ botScore NOTIFY changed)
    Q_PROPERTY(int humanBackPeg READ humanBackPeg NOTIFY changed)
    Q_PROPERTY(int botBackPeg READ botBackPeg NOTIFY changed)
    Q_PROPERTY(QVariantList hand READ hand NOTIFY changed)
    Q_PROPERTY(QVariantList cutDeck READ cutDeck NOTIFY changed)
    Q_PROPERTY(int humanCut READ humanCut NOTIFY changed)
    Q_PROPERTY(int botCut READ botCut NOTIFY changed)
    Q_PROPERTY(bool cutTied READ cutTied NOTIFY changed)
    Q_PROPERTY(int cutDealer READ cutDealer NOTIFY changed)
    Q_PROPERTY(QVariantList botHand READ botHand NOTIFY changed)
    Q_PROPERTY(QVariantList humanKept READ humanKept NOTIFY changed)
    Q_PROPERTY(QVariantList botKept READ botKept NOTIFY changed)
    Q_PROPERTY(QVariantList crib READ crib NOTIFY changed)
    Q_PROPERTY(int starter READ starter NOTIFY changed)
    Q_PROPERTY(QVariantList played READ played NOTIFY changed)
    Q_PROPERTY(int count READ count NOTIFY changed)
    Q_PROPERTY(bool humanTurn READ humanTurn NOTIFY changed)
    Q_PROPERTY(bool botTurn READ botTurn NOTIFY changed)
    Q_PROPERTY(bool mustSayGo READ mustSayGo NOTIFY changed)
    Q_PROPERTY(bool humanDiscarded READ humanDiscarded NOTIFY changed)
    Q_PROPERTY(int showStep READ showStep NOTIFY changed)
    Q_PROPERTY(int showOwner READ showOwner NOTIFY changed)
    Q_PROPERTY(QVariantList showCards READ showCards NOTIFY changed)
    Q_PROPERTY(QVariantList showItems READ showItems NOTIFY changed)
    Q_PROPERTY(QVariantList lastScoring READ lastScoring NOTIFY changed)
    Q_PROPERTY(int showTotal READ showTotal NOTIFY changed)
    Q_PROPERTY(int winner READ winner NOTIFY changed)
    Q_PROPERTY(int skunk READ skunk NOTIFY changed)
    Q_PROPERTY(int round READ round NOTIFY changed)
    Q_PROPERTY(bool endedInShow READ endedInShow NOTIFY changed)
    Q_PROPERTY(QString message READ message NOTIFY changed)
    Q_PROPERTY(bool started READ started NOTIFY changed)
    Q_PROPERTY(QString opponent READ opponent NOTIFY changed)
    Q_PROPERTY(int gameDifficulty READ gameDifficulty NOTIFY changed)
    Q_PROPERTY(int difficulty READ difficulty WRITE setDifficulty NOTIFY difficultyChanged)
    Q_PROPERTY(QVariantList stats READ stats NOTIFY statsChanged)
    Q_PROPERTY(int scoringSpeed READ scoringSpeed WRITE setScoringSpeed NOTIFY scoringSpeedChanged)
    Q_PROPERTY(bool tapTwice READ tapTwice WRITE setTapTwice NOTIFY tapTwiceChanged)
    Q_PROPERTY(int revealMs READ revealMs NOTIFY scoringSpeedChanged)

public:
    enum Phase { Discard, Play, Show, GameOver, Cut };
    Q_ENUM(Phase)

    explicit GameController(Session *online, QObject *parent = nullptr);

    int phase() const;
    int dealer() const { return m_game.dealer(); }
    int humanScore() const { return m_game.score(Human); }
    int botScore() const { return m_game.score(Computer); }
    int humanBackPeg() const { return m_game.previousScore(Human); }
    int botBackPeg() const { return m_game.previousScore(Computer); }
    QVariantList hand() const;
    // The pack spread for the cut for deal, and the cards each player drew.
    QVariantList cutDeck() const;
    int humanCut() const;
    int botCut() const;
    bool cutTied() const { return m_game.phase() == cribbage::Phase::Cut && m_game.cutTied(); }
    int cutDealer() const { return m_game.phase() == cribbage::Phase::Cut ? m_game.cutDealer() : -1; }
    // Face-down cards; ids let the table animate each card from where it lies.
    QVariantList botHand() const;
    QVariantList humanKept() const;
    QVariantList botKept() const;
    QVariantList crib() const;
    int starter() const { return m_game.hasStarter() ? m_game.starter().id() : -1; }
    QVariantList played() const;
    int count() const { return m_game.count(); }
    bool humanTurn() const;
    bool botTurn() const;
    bool mustSayGo() const { return m_game.mustSayGo(Human); }
    bool humanDiscarded() const { return m_game.hasDiscarded(Human); }
    int showStep() const { return m_game.showStep(); }
    int showOwner() const { return m_game.showOwner(); }
    QVariantList showCards() const;
    QVariantList showItems() const;
    // Points pegged by the latest action, one entry per score with its cards.
    QVariantList lastScoring() const { return m_lastScoring; }
    int showTotal() const;
    int winner() const { return m_game.winner(); }
    int skunk() const { return m_game.skunk(); }
    int round() const { return m_game.round(); }
    bool endedInShow() const;
    QString message() const { return m_message; }
    bool started() const { return m_started; }
    // The other player's name online; empty against the bot.
    QString opponent() const;
    int gameDifficulty() const { return static_cast<int>(m_bot.difficulty()); }
    int difficulty() const { return m_difficulty; }
    void setDifficulty(int difficulty);
    QVariantList stats() const;
    // 0 = slow ... 3 = very fast; how long each score is shown.
    int scoringSpeed() const { return m_scoringSpeed; }
    void setScoringSpeed(int speed);
    // A card is played by a second tap; the first raises it.
    bool tapTwice() const { return m_tapTwice; }
    void setTapTwice(bool on);
    int revealMs() const;

    // Carries on the game against the bot from when the app last closed,
    // or deals a new one.
    Q_INVOKABLE void start();
    Q_INVOKABLE void newGame();
    Q_INVOKABLE bool cut(int index);
    Q_INVOKABLE bool discard(const QVariantList &cardIds);
    Q_INVOKABLE bool play(int cardId);
    Q_INVOKABLE bool sayGo();
    Q_INVOKABLE bool nextShow();
    Q_INVOKABLE bool canPlay(int cardId) const;
    Q_INVOKABLE QVariantList hint();
    Q_INVOKABLE void resetStats();

    Q_INVOKABLE QString categoryName(int category, int points) const;

signals:
    void changed();
    void difficultyChanged();
    void statsChanged();
    void scoringSpeedChanged();
    void tapTwiceChanged();

private:
    static constexpr int Human = 0;
    static constexpr int Computer = 1;

    void afterAction(int actor, const QString &announcement = QString());
    // Runs the bot's moves, and the cut, while it is this app's to do so.
    void schedule();
    void botStep();
    bool isOnline() const { return m_online->active(); }
    // Whether this app plays the bot's moves: always offline, online only for
    // a bot's seat and only on the table's driver.
    bool runsBot() const;
    void sendNew();
    // Makes a move from the table's log, if it is valid now.
    // Makes a move: online it goes to the table's log and comes back; against
    // the bot it is made at once and written down.
    bool perform(const QVariantMap &action);
    bool applyAction(const QVariantMap &action, bool live);
    void replay();
    void onTableChanged();
    void recordResult();
    QString describeEvents(size_t from) const;

    Session *m_online;
    // The moves of the game against the bot.
    cards::GameLog m_log;
    cribbage::Game m_game;
    cribbage::Bot m_bot;
    QSettings m_settings;
    QTimer m_botTimer;
    QString m_message;
    QVariantList m_lastScoring;
    size_t m_seenEvents = 0;
    int m_difficulty = 1;
    int m_scoringSpeed = 1;
    bool m_tapTwice = false;
    bool m_started = false;
    bool m_recorded = false;
    bool m_wasOnline = false;
    bool m_wasDriver = false;
};
