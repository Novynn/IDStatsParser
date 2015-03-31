#ifndef APPLICATION_H
#define APPLICATION_H

#include <QObject>
#include <QDebug>
#include "entconnection.h"
#include "statsdatabase.h"
#include "w3idplayer.h"

class Application : public QObject
{
    Q_OBJECT
public:
    Application();

    void run() {
        std::cout << "IDStatsParser v1.0.0 started...\n";
        _database = new StatsDatabase();

//        if (!_database->connect()) {
//            qDebug() << "Fatal Error: Failed to connect to the database.";
//            exit(1);
//        }

//        qDebug() << "Database connection established.";

        _ent = new ENTConnection();
        connect(_ent, &ENTConnection::onNewENTIDGame, this, &Application::onNewENTIDGame);
        _ent->run();
    }
    int totalDesynced = 0;
    int totalPlayed = 0;

    QHash<QString, int> playersSeen;
public slots:
    void onNewENTIDGame(ENTGame* game) {
        W3IDReplay* replay = static_cast<W3IDReplay*>(game->replay());
        QString name = game->name();
        std::cout << qPrintable(QString("Parsed %1 (%2): ").arg(name, 24).arg(game->id(), 7));

        //QString winner = game->findWinner(false);
        //std::cout << qPrintable("Winner: " + winner + "\n");
        //delete game;
        //return;

        for (W3Player* player : game->players()) {
            QString name = player->name().toLower();
            if (playersSeen.contains(name)) {
                playersSeen[name] += 1;
            }
            else {
                playersSeen.insert(name, 1);
            }
        }
        totalPlayed += game->players().count();
        std::cout << qPrintable(QString::number(game->players().count()) + " players (total: " + QString::number(playersSeen.count()) + "/" + QString::number(totalPlayed) + ") ");
        QList<W3IDPlayer*> players = game->getDesyncedPlayers();
        if (players.count() > 0) {
            totalDesynced += players.count();
            std::cout << qPrintable(QString::number(players.count()) + " desynced players (total: " + QString::number(totalDesynced) + ")\n");
            for (W3IDPlayer* player : players) {
                std::cout << qPrintable("\t" + player->name() + " " + QTime::fromMSecsSinceStartOfDay(player->leftAt()).toString("HH:mm:ss.zzz") + "\n");
            }
        }

        QList<W3Player*> normPlayers = game->players();
        for (W3Player* player : normPlayers) {
            if (player->leftAt() > 4000 &&
                player->leftAt() < 8000) {
                std::cout << qPrintable("\n\tSuspected maphacker: " + player->name() + "\n");
            }
        }


        static QStringList words = {"neco", "editor", "bug", "glitch", "98", "version", "9d"};
        bool messages = false;
        for (W3ChatMessage* message : game->replay()->messages()) {
            bool ok = false;
            for (QString w : words) {
                if (message->message().toLower().contains(w)) {
                    ok = true;
                    break;
                }
            }

            if (ok) {
                messages = true;
                std::cout << qPrintable("\n\t" + message->sender()->name() + ": " + message->message());
            }
        }

        if (messages) {
            getchar();
        }
        else {
            std::cout << std::endl;
        }

        delete game;
    }

private:
    StatsDatabase* _database;
    ENTConnection* _ent;
};

#endif // APPLICATION_H
