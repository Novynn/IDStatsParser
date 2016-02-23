#ifndef W3IDREPLAY_H
#define W3IDREPLAY_H

#include "w3idplayer.h"
#include "w3replay.h"
#include "shared/qbytearraybuilder.h"

class W3IDReplay : public W3Replay
{
public:
    W3IDReplay(const QByteArray &data)
        : W3Replay(data) {
    }

    W3IDPlayer* getIDPlayer(quint8 pid) {
        return static_cast<W3IDPlayer*>(W3Replay::_players.value(pid));
    }

    W3IDPlayer* getIDPlayerByName(const QString &name) {
        W3IDPlayer* player = 0;
        for (W3Player* p : _players.values()) {
            if (p->name().toLower() == name.toLower()) {
                player = static_cast<W3IDPlayer*>(p);
                break;
            }
        }
        return player;
    }

    W3IDPlayer* getIDPlayerByColor(W3Player::Color color) {
        W3IDPlayer* player = 0;
        for (W3Player* p : _players.values()) {
            if (p->color() == color) {
                player = static_cast<W3IDPlayer*>(p);
                break;
            }
        }
        return player;
    }

protected:
    W3Player* newPlayer(quint8 pid, QString name) final {
        W3IDPlayer* player = new W3IDPlayer(pid, name);
        _players.insert(pid, player);
        return player;
    }

    void addAction(W3ReplayAction* action) final {
        W3Replay::addAction(action);

        //qDebug() << action->time() << action->action() << action->id();

        if (action->id() == W3ReplayAction::GameCacheSyncInteger) {
            QString file = action->data().value("file_name").toString();
            QString mkey = action->data().value("mission_key").toString();
            QString key = action->data().value("key").toString();
            quint32 val = action->data().value("value").toUInt();
            Q_UNUSED(file)
            Q_UNUSED(val)

            //qDebug() << file << mkey << key << val;

            if (mkey.split(":").first() == "chk") {
                return;
            }

            QStringList parts = key.split(" ");
            if (parts.first() == "init") {
                if (parts.at(1) == "version") {
                    mmdVersionMajor = parts.at(2).toUInt(); // Major
                    mmdVersionMinor = parts.at(3).toUInt(); // Minor
                }
                else if (parts.at(1) == "pid") {
                    quint8 pid = parts.at(2).toUInt(); // color
                    QString name = parts.at(3); // name

                    W3IDPlayer* player = getIDPlayerByColor((W3Player::Color)pid);
                    player->setMMDInitialized();
                }
            }
            else if (parts.first() == "FlagP") {
                quint8 pid = parts.at(1).toUInt(); // color
                QString flag = parts.at(2); // flag
                W3IDPlayer* player = getIDPlayerByColor((W3Player::Color)pid);

                if (flag == "winner") {
                    player->setFlag(W3IDPlayer::Winner);
                }
                else if (flag == "loser") {
                    player->setFlag(W3IDPlayer::Loser);
                }
                else if (flag == "drawer") {
                    player->setFlag(W3IDPlayer::Drawer);
                }
                else if (flag == "leaver") {
                    player->setFlag(W3IDPlayer::Leaver);
                }
                else if (flag == "practicing") {
                    player->setFlag(W3IDPlayer::Practicing);
                }
                else {
                    player->setFlag(W3IDPlayer::Unknown);
                }
            }
        }
        else if (action->id() == W3ReplayAction::MapTriggerChatCommand) {
            QString message = action->data().value("message").toString();
            W3Player* p = action->player();
            if (p) {
                W3IDPlayer* player = getIDPlayer(p->id());

                if (player && !player->picked() && message.trimmed() == "-") {
                    // Lazy Random!
                    player->setPicked(true);
                }
            }
            else {
                qDebug() << "Unknown player! " << p->name();
            }
        }
        else if (action->id() == W3ReplayAction::UnitAbility) {
            // First ImmediateOrder (0x10) is the "Item" that they choose
            // I006 being 'random' etc.
            auto id = action->data().value("unitid").toString();
            W3IDPlayer* player = getIDPlayerByColor(action->player()->color());

            if (!player || !player->race().isEmpty()) return;

            if (id == "I04X") {
                player->setPicked(false);
            }

            // If Random, and Player's race is empty! :O
            if ((id == "I006" || id == "I02F")) {
                // Random
                player->setPicked(true);
            }
            else if (id == "I02E" ||
                     id == "I02X" ||
                     id == "I03T" ||
                     id == "I01J" ||
                     id == "I00S" ||
                     id == "I039" ||
                     id == "I028" ||
                     id == "I02C" ||
                     id == "I012" ||
                     id == "I066" ||
                     id == "I02B" ||
                     id == "I03Y" ||
                     id == "I033" ||
                     id == "I02W" ||
                     id == "I03D" ||
                     id == "I04C" ||
                     id == "I006" ||
                     id == "I029" ||
                     id == "I02D" ||
                     id == "I02A"){
                // Picked a Defender Race
                player->setPicked(false);
            }
            else if (id == "I00R" ||
                     id == "I02H" ||
                     id == "I02G" ||
                     id == "I02M" ||
                     id == "I02L" ||
                     id == "I02K" ||
                     id == "I02U" ||
                     id == "I03X" ||
                     id == "I02F" ||
                     id == "I02I" ||
                     id == "I02J"){
                // Picked a Titan Race
                player->setPicked(false);
            }
        }
        else if (action->id() == W3ReplayAction::SelectSubgroup) {
            auto id = action->data().value("unitid").toString();
            W3IDPlayer* player = getIDPlayerByColor(action->player()->color());

            // Ignore selection shops!
            if (QStringList({"n01G", "n00X", "n00W", "n00M", "n00Z", "n00G", "h001"}).contains(id)) return;
            // Use these to find out which race the player is (and to see if it matches selection from items).

            if (player == 0 || !player->race().isEmpty()) return;
            QString race = "";

            const QHash<QString, QString> raceMap{
                {"h01T", "Dryad"},
                {"h021", "Morphling"},
                {"u009", "Radioactive"},
                {"h009", "Gnoll"},
                {"u00W", "Draenei"},
                {"h035", "Satyr"},
                {"h00X", "Goblin"},
                {"h01B", "Magnataur"},
                {"h037", "Ogre"},
                {"O01Q", "Tauren"},
                {"h043", "Pirate"},
                {"h00Q", "Nature"},
                {"h016", "Furbolg"},
                {"h023", "Morphling Warrior"},
                {"h008", "Makrura"},
                {"h01N", "Ogre"},
                {"u00I", "Demonologist"},
                {"h04I", "Murloc"},
                {"h007", "Troll"},
                {"h02S", "Faerie"},
                {"E00B", "Sypherious"},
                {"E00E", "Demonicus"},
                {"E01D", "Lucidious"},
                {"E00C", "Moltenious"},
            };

            race = raceMap.value(id, id);

            player->setRace(race);
        }
    }
private:

    quint8 mmdVersionMajor = 0;
    quint8 mmdVersionMinor = 0;
};

#endif // W3IDREPLAY_H
