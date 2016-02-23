#ifndef ENTGAME_H
#define ENTGAME_H

#include "w3idreplay.h"
#include "w3replay.h"
#include <QByteArray>
#include <qdatetime.h>
#include "shared/boolinq.h"
#include <qdatetime.h>
#include <qfile.h>

class ENTGame
{
public:
    enum GameMap {
        Unknown,
        IslandDefense
    };

    ENTGame(int id, QString name, GameMap map, const QDateTime date, const QByteArray &data = QByteArray())
        : _id(id)
        , _name(name)
        , _replay(0)
        , _date(date) {
        _gameMap = map;
        loadReplay(data);
    }

    void loadReplayName(const QByteArray &data) {
        if (data.size() > 0) {
            if (is(IslandDefense)) {
                _replay = new W3IDReplay(data);
            }
            else {
                _replay = new W3Replay(data);
            }
            _replay->parseHeaderData();
            _name = _replay->name();
            delete _replay;
            _replay = 0;
        }
    }

    bool loadReplay(const QByteArray &data) {
        bool success = false;
        if (data.size() > 0) {
            if (is(IslandDefense)) {
                _replay = new W3IDReplay(data);
            }
            else {
                _replay = new W3Replay(data);
                Q_ASSERT(false);
            }
            success = _replay->parseAll();
            if (success) {
                _name = _replay->name();
            }
            return success;
        }
        return false;
    }

    void unloadReplay() {
        delete _replay;
        _replay = 0;
    }

    ~ENTGame() {
        unloadReplay();
    }

    const QDateTime date() const {
        return _date;
    }

    bool is(GameMap map) {
        return _gameMap == map;
    }

    bool parse() {
        return replay()->parseAll();
    }

    W3Replay* replay() {
        if (_replay != 0) {
            return _replay;
        }
        loadReplayFromLocalDisk();
        return _replay;
    }

    bool loadReplayFromLocalDisk() {
        // Save a local copy
        QFile file("replays/" + QString::number(_id) + ".w3g");
        QByteArray data;
        if (file.open(QFile::ReadOnly)) {
            data = file.readAll();
        }
        file.close();

        return loadReplay(data);
    }

    QList<W3Player*> players() {
        return replay()->players();
    }

    uint id() {
        return _id;
    }

    const QString name() {
        return _name;
    }

    uint durationAsMS() {
        return replay()->time();
    }

    QString duration() {
        return QTime::fromMSecsSinceStartOfDay(durationAsMS()).toString("HH:mm:ss.zzz");
    }

    QList<W3IDPlayer*> getDesyncedPlayers() {
        QList<W3IDPlayer*> list;
        W3IDReplay* idReplay = static_cast<W3IDReplay*>(replay());
        if (idReplay == 0 || !is(ENTGame::IslandDefense)) return list;


        for(W3ChatMessage* message : idReplay->messages()){
            if (message->message().endsWith("was dropped due to desync.")) {
                QString player = message->message().split(" ").first();
                W3IDPlayer* p = idReplay->getIDPlayerByName(player);
                if (p != 0) {
                    list.append(p);
                }
            }
        }
        return list;
    }

    const QString findWinner(bool output = false) {
        QString winner;
        if (output) std::cout << "\n";
        W3IDReplay* idReplay = static_cast<W3IDReplay*>(replay());
        if (idReplay == 0 || !is(ENTGame::IslandDefense)) return "Unknown";

        // Check if the Titan left early...
        QList<W3IDPlayer::Color> titanColors = { W3IDPlayer::DarkGreen };
        bool titanLeft = false;
        {
            W3IDPlayer* player = idReplay->getIDPlayerByColor(titanColors.last());
            if (player->leftAt() < idReplay->time() - 30000) {
                // Titan left early!
                titanLeft = true;
            }
        }

        if (titanLeft) {
            if (output) {
                //qDebug() << "\nThe Titan left at " << idReplay->getIDPlayerByColor(titanColors.last())->leftAt();
            }
            std::vector<W3ReplayAction*> actions = idReplay->actions().toVector().toStdVector();

            QList<W3ReplayAction*> titanActions = QList<W3ReplayAction*>::fromStdList(boolinq::from(actions)
                                .where([] (W3ReplayAction* action) {
                                    return action->id() == W3ReplayAction::MapTriggerChatCommand &&
                                           action->data().value("message").toString().startsWith("-titan");
            }).distinct([] (W3ReplayAction* action) {
                    return QString::number(action->time()) + "|" + action->data().value("message").toString();
            }).toList());


            for (W3ReplayAction* action : titanActions) {
                W3IDPlayer* player = idReplay->getIDPlayerByColor(titanColors.last());
                // If the Titan leaving and this player doing "-titan" are within 15,000 milliseconds, then valid
                if (output) {
                    //qDebug() << action->player()->name() + " used -titan at" << action->time();
                }
                if (player->leftAt() <= action->time()) {
                    // Within boundary
                    if (output) {
                        //qDebug() << "It was successful.";
                    }

                    titanColors.append(action->player()->color());
                }
            }

            if (titanColors.count() == 1) {
                // The titan left but we have no data on who took over...
                //qDebug() << "No data";
                return "none";
                //output = true;
            }
        }

        winner = "defenders";
        bool unknown = true;
        for (W3IDPlayer::Color color : {W3IDPlayer::Red, W3IDPlayer::Blue, W3IDPlayer::Teal,
                                        W3IDPlayer::Purple, W3IDPlayer::Yellow, W3IDPlayer::Orange,
                                        W3IDPlayer::Green, W3IDPlayer::Pink, W3IDPlayer::Grey,
                                        W3IDPlayer::LightBlue, W3IDPlayer::DarkGreen, W3IDPlayer::Brown}) {
            W3IDPlayer* player = idReplay->getIDPlayerByColor(color);
            if (player != 0) {
                QString role = "defender (" + player->race() + ")";
                if (titanColors.contains(player->color())) {
                    role = "titan (" + player->race() + ")";
                }
                if (player->race().isEmpty()) {
                    role = "obs/leaver";
                }
                if (player->randomed()) role += "*";

                QString flag;
                switch(player->flag()){
                    case W3IDPlayer::Winner:
                        flag = "winner";
                        break;
                    case W3IDPlayer::Loser:
                        flag = "loser";
                        break;
                    case W3IDPlayer::Leaver:
                        flag = "leaver";
                        break;
                    default:
                        flag = "unknown";
                }

                if (unknown && flag != "unknown") {
                    unknown = false;
                }

                if (role.startsWith("titan") && flag == "winner") {
                    // Titan won (apart from minion detection etc).
                    winner = "titan (" + player->name() + ")";
                }

                QTime dur = QTime::fromMSecsSinceStartOfDay(player->leftAt());

                if (output) {
                    std::cout << qPrintable(QString("  %1 %2 %3 %4 %5\n").arg(color, 2)
                                                                          .arg(player->name(), 16)
                                                                          .arg(flag, 6)
                                            .arg(dur.toString("HH:mm:ss.zzz"), 12)
                                            .arg(role, 32));
                }
            }
        }

        if (output)
        {
            getchar();
        }
        if (unknown) winner = "none";
        return winner;
    }

private:
    uint _id;
    QString _name;
    W3Replay* _replay;
    QDateTime _date;

    // Name, Date, Duration, Winner

    GameMap _gameMap;
};

#endif // ENTGAME_H





