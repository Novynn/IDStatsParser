#ifndef ENTCONNECTION_H
#define ENTCONNECTION_H

#include "entgame.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QXmlQuery>
#include <QObject>
#include <QDebug>
#include <QFile>
#include <QTimer>
#include <QRegularExpressionMatchIterator>

class ENTConnection : public QObject
{
    Q_OBJECT
public:
    explicit ENTConnection() {
        _network = new QNetworkAccessManager(this);
        _networkDownloader = new QNetworkAccessManager(this);

        connect(_network, &QNetworkAccessManager::finished, this, [this](QNetworkReply* reply) {
            int page = reply->request().attribute(QNetworkRequest::User).toInt();
            QByteArray data = reply->readAll();
            if (data.length() == 0) return;
            onReply(page, data);
            reply->deleteLater();
        });

        connect(_networkDownloader, &QNetworkAccessManager::finished, this, [this](QNetworkReply* reply) {
            int id = reply->request().attribute(QNetworkRequest::User).toInt();
            QByteArray data = reply->readAll();
            if (data.length() == 0) return;
            onReplay(id, data);
            reply->deleteLater();
        });

        _timer = new QTimer(this);
        connect(_timer, &QTimer::timeout, this, [this] {
            checkNewGames();
        });
    }

    void run() {
        // Executes a timer that periodically checks for new games.
        _timer->setInterval(10000); // 10 seconds
        _timer->start();
        checkNewGames();
    }

private:
    QTimer* _timer;
    QHash<int, ENTGame*> _games;
    QHash<int, QDateTime> _dates;
    int lastPage = 5267956;
    int after = 5518784;
    QNetworkAccessManager* _network;
    QNetworkAccessManager* _networkDownloader;

    void checkNewGames(int page = 1) {
        //qDebug() << "Checking for new Island Defense games...";

        QNetworkRequest r;
        r.setUrl(QUrl("https://entgaming.net/customstats/islanddefense/games/" + QString::number(page) + "/"));
        r.setSslConfiguration(QSslConfiguration::defaultConfiguration());
        r.setAttribute(QNetworkRequest::User, page);
        _network->get(r);
    }

    void onNewGameId(int game) {
        if (game < this->after) return;
        if (_games.contains(game)) return;
        //qDebug() << "New Game: " << game;

        // Check local replay:
        QFile file("replays/" + QString::number(game) + ".w3g");
        if (file.exists() && file.open(QFile::ReadOnly)) {
            QByteArray data = file.readAll();
            file.close();
            onReplay(game, data, false);
            return;
        }
        // Download Replay
        QString replayLink = QString("http://storage.entgaming.net/replay/download.php?f=%1.w3g&fc=%1.w3g").arg(game);

        QNetworkRequest r;
        r.setUrl(QUrl(replayLink));
        r.setSslConfiguration(QSslConfiguration::defaultConfiguration());
        r.setAttribute(QNetworkRequest::User, game);
        _networkDownloader->get(r);
    }

    void onReplay(int id, const QByteArray &data, bool save=true, QDateTime date=QDateTime()) {
        if (_games.contains(id)) return;
        //qDebug() << "Have replay: " << id << data.size();

        // Parse replay while in memory
        ENTGame* game = new ENTGame(id, ENTGame::IslandDefense, data, date);
        if (game->parse()) {
            _games.insert(id, game);

            if (save) {
                // Save a local copy
                QFile file("replays/" + QString::number(id) + ".w3g");
                if (file.open(QFile::WriteOnly)) {
                    file.write(data);
                    file.close();
                }
                else {
                    qDebug() << "Failed to save " << QString::number(id) + ".w3g";
                }

            }

            // Emit new game
            emit onNewENTIDGame(game);
        }
        else {
            qDebug() << "Could not load " << id;
        }
    }

    void onReply(int page, const QByteArray &data) {
        QStringList results;
        QRegularExpression expression("<tr class=\"TableRow28\">(.+?)<\/tr>", QRegularExpression::DotMatchesEverythingOption);
        QRegularExpressionMatchIterator i = expression.globalMatch(data);

        while (i.hasNext()) {
            QRegularExpressionMatch match = i.next();
            QString word = match.captured(1);
            results << word;
        }

        if (page == 250) return;
        for (QString result : results) {
            QRegularExpression idex("islanddefense/game/(\\d*)");

            QRegularExpressionMatch match = idex.match(result);
            if (match.hasMatch()) {
                int r = match.captured(1).toInt();
                if (r <= lastPage) {
                    return;
                }

                // New game
                onNewGameId(r);
            }
        }

        page++;
        checkNewGames(page);
    }

signals:
    void onNewENTIDGame(ENTGame* game);
};

#endif // ENTCONNECTION_H
