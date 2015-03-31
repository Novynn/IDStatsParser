#ifndef W3IDPLAYER_H
#define W3IDPLAYER_H

#include <QString>
#include "w3replay.h"

class W3IDPlayer : public W3Player
{
public:
    W3IDPlayer(quint8 id, QString name)
        : W3Player(id, name){
    }

    enum Class {
        None = 0,
        Minion = 1,
        Titan = 2,
        Defender = 3,
        Observer = 4
    };

    enum State {
        Idle = 0, //When the map is initialized
        Starting = 1, //When game modes / player races are being chosen
        Started = 2, //Once defenders (and Titan) have spawned
        Paused = 3, //(The actual game state will be shown in "end_state", so you'll never see STATE_PAUSED)
        Finished = 4 //On gameover
    };

    enum Flag {
        Drawer,
        Loser,
        Winner,
        Leaver,
        Practicing,
        Unknown
    };

    void setMMDInitialized() {
        _mmdInitialized = true;
    }

    void setFlag(Flag flag) {
        _flag = flag;
    }

    Flag flag() {
        return _flag;
    }

    bool randomed() {
        return _randomed;
    }

    bool picked() {
        return _picked;
    }

    const QString race() const {
        return _race;
    }

    void setPicked(bool randomed) {
        _picked = true;
        _randomed = randomed;
    }

    void setRace(QString id) {
        _race = id;
    }

private:
    bool _mmdInitialized = false;
    Flag _flag;
    // Class _startClass;
    // Class _endClass;
    // State _endState;
    QString _race = "";
    bool _randomed = false;
    bool _picked = false;
    // quint32 _fed;
};

#endif // W3IDPLAYER_H
