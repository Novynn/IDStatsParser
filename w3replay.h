#ifndef W3REPLAY_H
#define W3REPLAY_H

#include <QByteArray>
#include <QMetaEnum>
#include <QString>
#include <QHash>
#include <QVariantHash>
#include "shared/qbytearraybuilder.h"

class QByteArrayBuilder;
class W3ReplayAction;
class W3Player;

class W3ChatMessage {
public:
    enum Mode {
        All = 0x00,
        Allied = 0x01,
        Observer = 0x02,
        ToRed = 0x03,
        ToBlue,
        ToTeal,
        ToPurple,
        ToYellow,
        ToOrange,
        ToGreen,
        ToPink,
        ToGrey,
        ToLightBlue,
        ToDarkGreen,
        ToBrown,
        Delayed = 0x800
    };

    W3ChatMessage(W3Player* sender, QString message, Mode mode, quint32 time)
        : _message(message)
        , _mode(mode)
        , _time(time)
        , _sender(sender) {
    }

    W3Player* sender() {
        return _sender;
    }

    quint32 time() {
        return _time;
    }

    const QString message() const {
        return _message;
    }

    Mode mode() {
        return _mode;
    }

private:
    QString _message = "";
    Mode _mode = All;
    quint32 _time = 0;
    W3Player* _sender = 0;
};

// Contains information about the player for this specific game.
class W3Player {
public:
    enum Color {
        Red,
        Blue,
        Teal,
        Purple,
        Yellow,
        Orange,
        Green,
        Pink,
        Grey,
        LightBlue,
        DarkGreen,
        Brown
    };

    W3Player(quint8 id, QString name)
        : _id(id)
        , _name(name) {
    }

    void setSlotInfo(Color color, quint8 team, quint8 race) {
        _color = color;
        _team = team;
        _race = race;
    }

    void setSaver() {
        _saver = true;
    }

    void setLeftAt(quint32 left) {
        _left = left;
    }

    quint32 leftAt() const {
        return _left;
    }

    quint8 id() {
        return _id;
    }

    const QString name() const {
        return _name;
    }

    Color color() {
        return _color;
    }

    quint8 team() {
        return _team;
    }

    const QString toString() const {
        return QString("Player {id=%1, name=%2, color=%3, team=%4, race=%5, saver=%6").arg(_id)
                                                                                      .arg(_name)
                                                                                      .arg((quint8)_color)
                                                                                      .arg(_team)
                                                                                      .arg(_race)
                                                                                      .arg(_saver);
    }

protected:
    quint8 _id = 0;
    Color _color = Red;
    quint8 _team = 0;
    QString _name = "";
    quint8 _race = 0;

    bool _saver = false;

    quint32 _left = 0;
    // double _apm;
};

class W3ReplayAction : public QObject {
    Q_OBJECT
public:
    W3ReplayAction(W3Player* player, quint8 id, quint32 time)
        : _player(player)
        , _id(id)
        , _time(time) {
    }

    static QString toItemID(quint32 data) {
        // Split into 4 bytes:
        QByteArrayBuilder b;
        b.insertDWord(data);
        if (b.peekByte(2) == 0x0D &&
            b.peekByte(3) == 0x00) {

            if (b.peekByte(0) == 0x9B && b.peekByte(1) == 0x00) {
                return "mirrorimage";
            }
            return "ALPH";

        }
        else {
            return b.getStringReversed(4);
        }
    }

    enum Action {
        Other = 0x0,
        PauseGame = 0x01,
        ResumeGame = 0x02,
        SetGameSpeed = 0x03,
        IncreaseGameSpeed = 0x04,
        DecreaseGameSpeed = 0x05,
        SaveGame = 0x06,
        SaveGameFinished = 0x07,
        UnitAbility = 0x10,
        UnitAbilityTargetPosition = 0x11,
        UnitAbilityTargetPositionObject = 0x12,
        UnitManipulateItem = 0x13,
        UnitAbilityTwoTargetTwoObjects = 0x14,
        ChangeSelection = 0x16,
        AssignGroupHotkey = 0x17,
        SelectGroupHotkey = 0x18,
        SelectSubgroup = 0x19,
        PreSubSelection = 0x1A,
        TriggerSelectionEvent = 0x1B,
        SelectItem = 0x1C,
        CancelHeroRevival = 0x1D,
        RemoveUnitFromBuildingQueue = 0x1E,
        CheatTheDudeAbides = 0x20,
        Unknown = 0x21,
        CheatSomebodySetUpUsTheBomb = 0x22,
        CheatWarpTen = 0x23,
        CheatIocainePowder = 0x24,
        CheatPointBreak = 0x25,
        CheatWhosYourDaddy = 0x26,
        CheatKeyserSoze = 0x27,
        CheatLeafitToMe = 0x28,
        CheatThereIsNoSpoon = 0x29,
        CheatStrengthAndHonor = 0x2A,
        Cheatitvexesme = 0x2B,
        CheatWhoIsJohnGalt = 0x2C,
        CheatGreedIsGood = 0x2D,
        CheatDayLightSavings = 0x2E,
        CheatISeeDeadPeople = 0x2F,
        CheatSynergy = 0x30,
        CheatSharpAndShiny = 0x31,
        CheatAllYourBaseAreBelongToUs = 0x32,
        ChangeAllyOptions = 0x50,
        TransferResources = 0x51,
        MapTriggerChatCommand = 0x60,
        EscPressed = 0x61,
        TriggerSleepOrSyncFinished  = 0x62,
        TriggerSyncReady = 0x63,
        TriggerMouseClickedTrackable = 0x64,
        TriggerMouseTouchedTrackable = 0x65,
        EnterChooseHeroSkillSubMenu = 0x66,
        EnterChooseBuildingSubMenu = 0x67,
        MinimapSignal = 0x68,
        DialogButtonClicked  = 0x69,
        DialogAnyButtonClicked = 0x6A,
        GameCacheSyncInteger = 0x6B,
        GameCacheSyncReal = 0x6C,
        GameCacheSyncBoolean = 0x6D,
        GameCacheSyncUnit = 0x6E,
        _unseen0x6F = 0x6F, // [probably meant to be GameCacheSyncString, but wc3 seems unable to send or receive it]
        GameCacheSyncEmptyInteger = 0x70,
        _unseen0x71 = 0x71, // [probably meant to be GameCacheSyncEmptyString, but wc3 seems unable to send or receive it]
        GameCacheSyncEmptyBoolean = 0x72,
        GameCacheSyncEmptyUnit = 0x73,
        GameCacheSyncEmptyReal = 0x74,
        TriggerArrowKeyEvent = 0x75
    };

    Q_ENUMS(Action)

    quint8 id() {
        return _id;
    }

    const QString action() const {
        int index = metaObject()->indexOfEnumerator("Action");
        return metaObject()->enumerator(index).valueToKey((Action) _id);
    }

    quint32 time() {
        return _time;
    }

    const QVariantHash data() const {
        return _data;
    }

    bool isAPM() {
        return _id == 0x10 ||
               _id == 0x11 ||
               _id == 0x12 ||
               _id == 0x13 ||
               _id == 0x14 ||
               _id == 0x17 ||
               _id == 0x18 ||
               _id == 0x1C ||
               _id == 0x1D ||
               _id == 0x1E ||
               _id == 0x61 ||
               _id == 0x66;
    }

    W3Player* player() const {
        return _player;
    }

    bool parse(QByteArrayBuilder *contents);
protected:
    W3Player* _player = 0;
    quint8 _id;
    quint32 _time;
    QVariantHash _data;
};

class W3Replay
{
public:
    W3Replay(const QByteArray &data);
    virtual ~W3Replay() {
        qDeleteAll(_players.begin(), _players.end());
        _players.clear();
        qDeleteAll(_messages.begin(), _messages.end());
        _messages.clear();
        qDeleteAll(_actions.begin(), _actions.end());
        _actions.clear();
    }

    bool parseAll(bool keepData = false);

    QList<W3Player*> players() {
        return _players.values();
    }

    QList<W3ChatMessage*> messages() {
        return _messages;
    }

    QList<W3ReplayAction*> actions() {
        return _actions;
    }

    void addChatMessage(W3Player* sender, QString message, W3ChatMessage::Mode mode, quint32 time) {
        W3ChatMessage* chatMessage = new W3ChatMessage(sender, message, mode, time);

//        // Insert preserving order by time.
//        bool inserted = false;
//        for (int i = 0; i < _messages.size(); i++) {
//            if (_messages.at(i)->time() > time) {
//                inserted = true;
//                _messages.insert(i, chatMessage);
//                break;
//            }
//        }

//        if (!inserted) {
            _messages.append(chatMessage);
//        }
    }


    const QString name() const {
        return _gameName;
    }

    const QString mapName() const {
        return _mapName;
    }

    quint32 time() {
        return _time;
    }

    bool parseHeaderData();
protected:
    void parseHeader(QByteArrayBuilder *contents);
    void parseBlocks(QByteArrayBuilder *contents);
    void parseTimeSlotBlock(QByteArrayBuilder *contents);
    void parseChatMessageBlock(QByteArrayBuilder *contents);
    void parseLeaveGame(QByteArrayBuilder *contents);
    void parseStartGame(quint8 id, QByteArrayBuilder *contents);
    void parseSeedForNextFrame(QByteArrayBuilder *contents);
    void parseForcedGameEnd(QByteArrayBuilder *contents);
    void parseUnknown(QByteArrayBuilder *contents);
    virtual W3Player* newPlayer(quint8 pid, QString name);
    W3Player* parsePlayerRecord(QByteArrayBuilder *contents);
    virtual void addAction(W3ReplayAction* action);

    QString _gameName;
    quint32 _seed;
    QHash<quint8, W3Player*> _players;

    QList<W3ChatMessage*> _messages;
    QList<W3ReplayAction*> _actions;

    quint8 _gameSpeed;
    quint8 _gameOptions;
    quint8 _teamsFixed;
    quint8 _teamOptions;
    quint32 _mapChecksum;

    QString _mapName;
    QString _creatorName;

    QByteArray _data;
    QByteArrayBuilder parseReplay();
private:
    quint32 _time = 0;
    W3ReplayAction *parseAction(quint8 pid, QByteArrayBuilder *contents);
};

#endif // W3REPLAY_H
