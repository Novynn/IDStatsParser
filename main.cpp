#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDateTime>
#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include "entconnection.h"
#include "w3replay.h"

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);
    QCommandLineParser parser;
    parser.setApplicationDescription("Downloads and parses Island Defense replays from EntGaming.net");
    parser.addPositionalArgument("ids", "The Game IDs you wish to parse.", "ids...");

    parser.process(a);
    QStringList args = parser.positionalArguments();

    QJsonArray array;

    ENTConnection* connection = new ENTConnection();

    QObject::connect(connection, &ENTConnection::onNewENTIDGame, [&array, &args](ENTGame* game) {
        W3IDReplay* replay = static_cast<W3IDReplay*>(game->replay());
        QJsonObject object;
        object.insert("id", (qint64) game->id());
        object.insert("name", game->name());
        object.insert("map", replay->mapName());
        object.insert("date", game->date().toString());
        object.insert("duration", (qint64) game->durationAsMS());


        QJsonArray players;
        for (W3Player* player : replay->players()) {
            W3IDPlayer* idPlayer = replay->getIDPlayerByColor(player->color());
            QJsonObject playerObj;
            playerObj.insert("name", idPlayer->name());
            playerObj.insert("color", idPlayer->color());
            playerObj.insert("flag", idPlayer->flag());
            playerObj.insert("left", (qint64) idPlayer->leftAt());
            playerObj.insert("team", idPlayer->team());
            playerObj.insert("picked", idPlayer->picked());
            playerObj.insert("randomed", idPlayer->randomed());
            playerObj.insert("race", idPlayer->race());
            players.append(playerObj);
        }
        object.insert("players", players);

        array.append(object);

        game->unloadReplay();
        args.removeOne(QString::number(game->id()));
        if (args.isEmpty()) {
            QJsonDocument doc;
            doc.setArray(array);
            std::cout << qPrintable(doc.toJson()) << std::endl;
            QCoreApplication::exit(0);
        }
    });

    for (const QString id : args) {
        bool ok = false;
        int i = id.toInt(&ok);
        if (!ok) continue;
        connection->parseGame(i);
    }

    return a.exec();
}
