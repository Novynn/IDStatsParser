#include "w3replay.h"
#include "shared/qbytearraybuilder.h"
#include <iostream>
#include <QDebug>
#include "zlib/include/zlib.h"

W3Replay::W3Replay(const QByteArray &data)
{
    _data = data;
}


QByteArray gUncompress(const QByteArray &data)
{
    if (data.size() <= 4) {
        qWarning("gUncompress: Input data is truncated");
        return QByteArray();
    }

    QByteArray result;

    int ret;
    z_stream strm;
    static const int CHUNK_SIZE = 1024;
    char out[CHUNK_SIZE];

    // allocate inflate state
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = data.size();
    strm.next_in = (Bytef*)(data.data());

    ret = inflateInit(&strm); // gzip decoding
    if (ret != Z_OK)
        return QByteArray();

    // run inflate()
    do {
        strm.avail_out = CHUNK_SIZE;
        strm.next_out = (Bytef*)(out);

        ret = inflate(&strm, Z_SYNC_FLUSH);
        Q_ASSERT(ret != Z_STREAM_ERROR);  // state not clobbered

        switch (ret) {
        case Z_NEED_DICT:
            ret = Z_DATA_ERROR;     // and fall through
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
            (void)inflateEnd(&strm);
            qDebug() << "error?" << ret;
            return QByteArray();
        }

        result.append(out, CHUNK_SIZE - strm.avail_out);
    } while (strm.avail_out == 0);

    // clean up and return
    inflateEnd(&strm);
    return result;
}


bool W3Replay::parse(bool keepData) {
    QByteArrayBuilder contents = parseReplay();
    if (contents.size() == 0) {
        _data.clear();
        return false;
    }
    parseHeader(&contents);
    parseBlocks(&contents);

    // Clear from memory
    if (!keepData)
        _data.clear();
    return true;
}

QByteArrayBuilder W3Replay::parseReplay() {
    QByteArrayBuilder input(_data);
    QByteArrayBuilder contents;

    QString header = input.getString();

    if (header != "Warcraft III recorded game\x1a") {
        return contents;
    }

    dword offset = input.getDWord();
    dword csize = input.getDWord();
    dword version = input.getDWord();
    dword dsize = input.getDWord();
    dword cblocks = input.getDWord();

    if(csize != (uint)_data.size()) {
        qDebug() << "replay data is corrupted or missing";
        return contents;
    }
    if(version > 1) {
        qDebug() << "replay version check failed";
        return contents;
    }

    QByteArray identifier = "";
    word versionNum = 0;
    word build = 0;
    word flags = 0;
    dword length = 0;
    dword checksum = 0;

    if (version == 0) {
        // 0x40
        input.getWord();
        versionNum = input.getWord();
        build = input.getWord();
        flags = input.getWord();
        length = input.getDWord();
        checksum = input.getDWord();
    }
    else if (version == 1) {
        // 0x44
        identifier = input.getVoid(4);
        versionNum = input.getWord();
        build = input.getWord();
        flags = input.getWord();
        length = input.getDWord();
        checksum = input.getDWord();
    }

    Q_UNUSED(versionNum)
    Q_UNUSED(build)
    Q_UNUSED(flags)
    Q_UNUSED(length)
    Q_UNUSED(checksum)

    input.reset();
    input.getVoid(offset);


    for (uint i = 0; i < cblocks; i++) {
        word size = input.getWord(); // Excluding header
        QByteArrayBuilder dsize = input.getVoid(2); // 8912
        input.getDWord(); // Checksum?
        QByteArray data = input.getVoid(size); // Compressed data
        QByteArray ddata = gUncompress(data);
        contents.append(ddata);
    }
    contents = contents.left(dsize);

    return contents;
}

W3ReplayAction* W3Replay::parseAction(quint8 pid, QByteArrayBuilder* contents) {
    byte id = contents->peekByte(); // Action ID
    //qDebug() << "Parsing 0x" + QString::number(id, 16);
    W3ReplayAction* action = new W3ReplayAction(_players.value(pid), id, _time);
    if (!action->parse(contents)) {
        qDebug() << "FAILED TO PARSE " << id;
        return 0;
    }
    addAction(action);
    return action;
}

void W3Replay::parseTimeSlotBlock(QByteArrayBuilder* contents) {
    word blocksize = contents->getWord();
    word time = contents->getWord();
    _time += time;
    if (blocksize != 2) {
        //contents->getVoid(blocksize - 2);
        while (blocksize - 2 > 0) {
            byte pid = contents->getByte(); // Player
            word actionlen = contents->getWord(); // Action Block Len
            QByteArrayBuilder actionData = contents->getVoid(actionlen);
            blocksize -= (3 + actionlen);
            bool failedToParse = false;
            while (actionlen > 0 && !failedToParse){
                quint64 pointer = actionData.getPointer();
                if (parseAction(pid, &actionData) == 0) {
                    failedToParse = true;
                }
                else {
                    actionlen -= (actionData.getPointer() - pointer); // Change in pointer
                }
            }

            contents->getVoid(actionlen); // Get remaining?
        }
    }
}

void W3Replay::parseChatMessageBlock(QByteArrayBuilder* contents) {
    byte sender = contents->getByte();
    contents->getWord(); // Block Size

    quint32 mode = 0x800; // "Delayed" message custom enum value
    if (contents->getByte() == 0x20) {
        mode = contents->getDWord();
    }

    QString message = contents->getString();
    addChatMessage(_players.value(sender), message, (W3ChatMessage::Mode)mode, _time);
}

void W3Replay::parseLeaveGame(QByteArrayBuilder* contents) {
    quint32 reason = contents->getDWord();
    quint8 pid = contents->getByte();
    quint32 result = contents->getDWord();
    quint32 unknown = contents->getDWord();

    _players.value(pid)->setLeftAt(_time);
}

void W3Replay::parseStartGame(byte id, QByteArrayBuilder* contents) {
    Q_UNUSED(id)
    contents->getDWord();
}

void W3Replay::parseSeedForNextFrame(QByteArrayBuilder* contents) {
    byte size = contents->getByte();
    QByteArray data = contents->getVoid(size);
}

void W3Replay::parseForcedGameEnd(QByteArrayBuilder* contents) {
    contents->getDWord();
    contents->getDWord();
}

void W3Replay::parseUnknown(QByteArrayBuilder* contents) {
    contents->getDWord();
    contents->getByte();
    contents->getDWord();
    contents->getByte();
}


void W3Replay::parseBlocks(QByteArrayBuilder* contents) {
    // Contents begin
    int blocks = 0;
    while (contents->peekByte() != 0) {
        byte bid = contents->getByte();

        switch (bid) {
        case 0x17:
            parseLeaveGame(contents);
            break;
        case 0x1A:
        case 0x1B:
        case 0x1C:
            parseStartGame(bid, contents);
            break;
        case 0x1E:
        case 0x1F:
            parseTimeSlotBlock(contents);
            break;
        case 0x20:
            parseChatMessageBlock(contents);
            break;
        case 0x22:
            parseSeedForNextFrame(contents);
            break;
        case 0x23:
            parseUnknown(contents);
            break;
        case 0x2F:
            parseForcedGameEnd(contents);
            break;
        }

        blocks++;
    }
}

W3Player *W3Replay::newPlayer(quint8 pid, QString name)
{
    W3Player* player = new W3Player(pid, name);
    _players.insert(pid, player);
    return player;
}

W3Player* W3Replay::parsePlayerRecord(QByteArrayBuilder* contents) {
    contents->getByte();
    byte pid = contents->getByte();
    QString name = contents->getString();
    byte additional = contents->getByte();

    if (additional == 0x01) {
        contents->getByte();
    }
    else if (additional == 0x08) {
        contents->getDWord();
        contents->getDWord();
    }

    return newPlayer(pid, name);
}

void W3Replay::parseHeader(QByteArrayBuilder* contents) {
    contents->getVoid(4);
    // Saver PlayerRecord
    parsePlayerRecord(contents)->setSaver();

    _gameName = contents->getString();
    contents->getByte();

    // TODO: Parse encoded string
    QString encodedString = contents->getString();
    contents->getDWord(); // slotsCount

    contents->getDWord(); // type
    contents->getDWord(); // lang

    // Player Records
    while (contents->peekByte() == 0x16) {
        parsePlayerRecord(contents);
        contents->getDWord();
    }

    // Game Start Record
    contents->getByte(); // 0x19
    contents->getWord(); // Length of following data
    byte nslots = contents->getByte();

    // Slots
    for (int i = 0; i < nslots; i++) {
        byte pid = contents->getByte();
        contents->getByte(); // download
        contents->getByte(); // slotStatus
        contents->getByte(); // comp
        byte team = contents->getByte();
        byte color = contents->getByte();
        byte raceFlags = contents->getByte();
        contents->getByte(); // ai
        contents->getByte(); // handicap

        if (_players.contains(pid)) {
            W3Player* player = _players.value(pid);
            player->setSlotInfo((W3Player::Color) color, team, raceFlags);
        }
    }

    _seed = contents->getDWord();
    contents->getByte(); // Selection Mode (locked teams etc)
    contents->getByte(); // Start Positions

    // Header Over
}


bool W3ReplayAction::parse(QByteArrayBuilder *contents) {
    contents->getByte();

    switch((Action)_id) {
    case PauseGame:
        break;
    case ResumeGame:
        break;
    case SetGameSpeed:
        _data.insert("game_speed", contents->getByte());
        break;
    case IncreaseGameSpeed:
        break;
    case DecreaseGameSpeed:
        break;
    case SaveGame:
        _data.insert("save_file", contents->getString());
        break;
    case SaveGameFinished:
        _data.insert("data", contents->getDWord());
        break;
    case UnitAbility:
        contents->getWord(); // Ability Flags
        _data.insert("unitid", W3ReplayAction::toItemID(contents->getDWord())); // ItemID
        contents->getDWord(); // Unk
        contents->getDWord(); // Unk
        break;
    case UnitAbilityTargetPosition:
        contents->getWord();
        contents->getDWord();
        contents->getDWord();
        contents->getDWord();
        contents->getDWord();
        contents->getDWord();
        break;
    case UnitAbilityTargetPositionObject:
        contents->getWord();
        contents->getDWord();
        contents->getDWord();
        contents->getDWord();
        contents->getDWord();
        contents->getDWord();
        contents->getDWord();
        contents->getDWord();
        break;
    case UnitManipulateItem:
        contents->getWord();
        contents->getDWord();
        contents->getDWord();
        contents->getDWord();
        contents->getDWord();
        contents->getDWord();
        contents->getDWord();
        contents->getDWord();
        contents->getDWord();
        contents->getDWord();
        break;
    case UnitAbilityTwoTargetTwoObjects:
        contents->getWord();

        contents->getDWord();

        contents->getDWord();
        contents->getDWord();

        contents->getDWord();
        contents->getDWord();

        contents->getDWord();

        contents->getVoid(9);

        contents->getDWord();
        contents->getDWord();
        break;
    case ChangeSelection:
        {
            // 0x01 =
            quint8 type = contents->getByte();
            quint16 n = contents->getWord();

            for (quint16 i = 0; i < n; i++) {
                contents->getDWord();
                contents->getDWord();
            }
        }
        break;
    case AssignGroupHotkey:
        {
            contents->getByte();
            quint16 n = contents->getWord();

            for (quint16 i = 0; i < n; i++) {
                contents->getDWord();
                contents->getDWord();
            }
        }
        break;
    case SelectGroupHotkey:
        contents->getByte();
        contents->getByte();
        break;
    case SelectSubgroup:
        _data.insert("unitid", W3ReplayAction::toItemID(contents->getDWord()));
        contents->getDWord();
        contents->getDWord();
        break;
    case PreSubSelection:
        break;
    case TriggerSelectionEvent:
        contents->getByte();
        contents->getDWord();
        contents->getDWord();
        break;
    case SelectItem:
        contents->getByte();
        contents->getDWord();
        contents->getDWord();
        break;
    case CancelHeroRevival:
        contents->getDWord();
        contents->getDWord();
        break;
    case RemoveUnitFromBuildingQueue:
        contents->getByte();
        contents->getDWord();
        break;
    case Unknown:
        contents->getDWord();
        contents->getDWord();
        break;
    case CheatTheDudeAbides:
    case CheatSomebodySetUpUsTheBomb:
    case CheatWarpTen:
    case CheatIocainePowder:
    case CheatPointBreak:
    case CheatWhosYourDaddy:
    case CheatThereIsNoSpoon:
    case CheatStrengthAndHonor:
    case Cheatitvexesme:
    case CheatWhoIsJohnGalt:
    case CheatISeeDeadPeople:
    case CheatSynergy:
    case CheatSharpAndShiny:
    case CheatAllYourBaseAreBelongToUs:
        break;
    case CheatKeyserSoze:
    case CheatLeafitToMe:
    case CheatGreedIsGood:
        contents->getByte();
        contents->getDWord();
        break;
    case CheatDayLightSavings:
        contents->getFloat();
        break;
    case ChangeAllyOptions:
        contents->getByte();
        contents->getDWord();
        break;
    case TransferResources:
        contents->getByte();
        contents->getDWord();
        contents->getDWord();
        break;
    case MapTriggerChatCommand:
        contents->getDWord();
        contents->getDWord();
        _data.insert("message", contents->getString());
        break;
    case EscPressed:
        break;
    case TriggerSleepOrSyncFinished:
        contents->getDWord();
        contents->getDWord();
        contents->getDWord();
        break;
    case TriggerSyncReady:
        contents->getDWord();
        contents->getDWord();
        break;
    case TriggerMouseClickedTrackable:
    case TriggerMouseTouchedTrackable:
        contents->getDWord();
        contents->getDWord();
        break;
    case EnterChooseHeroSkillSubMenu:
    case EnterChooseBuildingSubMenu:
        break;
    case MinimapSignal:
        contents->getDWord();
        contents->getDWord();
        contents->getDWord();
        break;
    case DialogButtonClicked:
        contents->getDWord();
        contents->getDWord();
        contents->getDWord();
        contents->getDWord();
        break;
    case DialogAnyButtonClicked:
        contents->getDWord();
        contents->getDWord();
        contents->getDWord();
        contents->getDWord();
        break;
    case GameCacheSyncInteger:
    case GameCacheSyncBoolean:
        _data.insert("file_name", contents->getString());
        _data.insert("mission_key", contents->getString());
        _data.insert("key", contents->getString());
        _data.insert("value", contents->getDWord());
        break;
    case GameCacheSyncReal:
        _data.insert("file_name", contents->getString());
        _data.insert("mission_key", contents->getString());
        _data.insert("key", contents->getString());
        _data.insert("value", contents->getFloat());
        break;
    case GameCacheSyncEmptyInteger:
    case GameCacheSyncEmptyBoolean:
    case GameCacheSyncEmptyUnit:
    case GameCacheSyncEmptyReal:
        _data.insert("file_name", contents->getString());
        _data.insert("mission_key", contents->getString());
        _data.insert("key", contents->getString());
        break;
    case TriggerArrowKeyEvent:
        /*  PressedLeftArrow = 0
            ReleasedLeftArrow = 1
            PressedRightArrow = 2
            ReleasedRightArrow = 3
            PressedDownArrow = 4
            ReleasedDownArrow = 5
            PressedUpArrow = 6
            ReleasedUpArrow = 7
        */
        _data.insert("key", contents->getByte());
        break;
    default:
        qDebug() << "Unknown Action: 0x" + QString::number(_id, 16);
        return false;
    }

    return true;
}





void W3Replay::addAction(W3ReplayAction *action) {
    // Insert preserving order by time.
//    bool inserted = false;
//    for (int i = 0; i < _actions.size(); i++) {
//        if (_actions.at(i)->time() > action->time()) {
//            inserted = true;
//            _actions.insert(i, action);
//            break;
//        }
//    }

//    if (!inserted) {
        _actions.append(action);
//    }
}
