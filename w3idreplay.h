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
        for (W3Player* p : W3Replay::_players.values()) {
            if (p->name().toLower() == name.toLower()) {
                player = static_cast<W3IDPlayer*>(p);
                break;
            }
        }
        return player;
    }

    W3IDPlayer* getIDPlayerByColor(W3Player::Color color) {
        W3IDPlayer* player = 0;
        for (W3Player* p : W3Replay::_players.values()) {
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
            W3IDPlayer* player = getIDPlayerByColor(action->player()->color());

            if (player && !player->picked() && message.trimmed() == "-") {
                // Lazy Random!
                player->setPicked(true);
            }
        }
        else if (action->id() == W3ReplayAction::UnitAbility) {
            // First ImmediateOrder (0x10) is the "Item" that they choose
            // I006 being 'random' etc.
            auto id = action->data().value("unitid").toString();
            W3IDPlayer* player = getIDPlayerByColor(action->player()->color());

            if (!player || !player->race().isEmpty()) return;
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
            if (QStringList({"n01G", "n00X", "n00W", "n00M", "n00Z"}).contains(id)) return;
            // Use these to find out which race the player is (and to see if it matches selection from items).

            if (player == 0 || !player->race().isEmpty() || !player->picked()) return;
            QString race = "";
            // Builders
            if (id == "h01T")       race = "Dryad";
            else if (id == "h021")  race = "Morphling";
            else if (id == "u009")  race = "Radioactive";
            else if (id == "h009")  race = "Gnoll";
            else if (id == "u00W")  race = "Draenei";
            else if (id == "h035")  race = "Satyr";
            else if (id == "h00X")  race = "Goblin";
            else if (id == "h01B")  race = "Magnataur";
            else if (id == "h037")  race = "Ogre";
            else if (id == "O01Q")  race = "Tauren";
            else if (id == "h043")  race = "Pirate";
            else if (id == "h00Q")  race = "Nature";
            else if (id == "h016")  race = "Furbolg";
            else if (id == "h023")  race = "Morphling Warrior";
            else if (id == "h008")  race = "Makrura";
            else if (id == "h01N")  race = "Ogre";
            else if (id == "u00I")  race = "Demonologist";
            else if (id == "h04I")  race = "Murloc";
            else if (id == "h007")  race = "Troll";
            else if (id == "h02S")  race = "Faerie";

            // Titans
            if (id == "E00B") race = "Sypherious";
            else if (id == "E00E") race = "Demonicus";
            else if (id == "E01D") race = "Lucidious";
            else if (id == "E00C") race = "Moltenious";


            player->setRace(race);
        }
    }
private:

    quint8 mmdVersionMajor = 0;
    quint8 mmdVersionMinor = 0;
};

#endif // W3IDREPLAY_H
