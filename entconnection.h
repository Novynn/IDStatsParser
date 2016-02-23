#ifndef ENTCONNECTION_H
#define ENTCONNECTION_H

#include "entgame.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QObject>
#include <QDebug>
#include <QFile>
#include <QTimer>
#include <QRegularExpressionMatchIterator>
#include <QThread>
#include <QTimeZone>

class ENTConnection : public QObject
{
    Q_OBJECT
public:
    explicit ENTConnection() {
        _network = new QNetworkAccessManager(this);
        _networkDownloader = new QNetworkAccessManager(this);

        connect(_network, &QNetworkAccessManager::finished, this, [this](QNetworkReply* reply) {
            int page = reply->request().attribute(QNetworkRequest::User).toInt();
            if (!reply->isReadable()) {
                qDebug() << "Failed to read.";
                return;
            }
            QByteArray data = reply->readAll();
            if (data.length() == 0) return;
            onReply(page, data);
            reply->deleteLater();
        });

        connect(_networkDownloader, &QNetworkAccessManager::finished, this, [this](QNetworkReply* reply) {
            int id = reply->request().attribute(QNetworkRequest::User).toInt();
            QDateTime date = reply->request().attribute((QNetworkRequest::Attribute) 1001).toDateTime();
            QByteArray data = reply->readAll();
            if (data.length() == 0) return;
            onReplay(id, "", data, date, true);
            reply->deleteLater();
        });
    }

    void parseGame(int id) {
        onNewGameId(id);
    }

    void start() {
        // For automatic game parsing
        _timer = new QTimer(this);
        connect(_timer, &QTimer::timeout, this, [this] {
            checkNewGames();
        });
    }

private:
    QTimer* _timer;
    QList<int> _games;
    QHash<int, QDateTime> _dates;
    int currPage = 0;
    int lastPage = 6818204;//5267956;
    int after = 0;//5542201;
    int maxPage = 250;
    QNetworkAccessManager* _network;
    QNetworkAccessManager* _networkDownloader;

    void checkNewGames() {
        if (currPage == 0) {
            //qDebug() << "Checking for new games...";
            currPage = 1;
        }
        else {
            //qDebug() << "Checking page: " << currPage;
        }

        QNetworkRequest r;
        r.setUrl(QUrl("https://entgaming.net/customstats/islanddefense/games/" + QString::number(currPage) + "/"));
        r.setSslConfiguration(QSslConfiguration::defaultConfiguration());
        r.setAttribute(QNetworkRequest::User, currPage);
        _network->get(r);
    }

    void onNewGameId(int game, QDateTime date = QDateTime()) {
        if (game < this->after && this->after != 0) return;
        if (_games.contains(game)) return;

        // Check local replay:
        QFile file("replays/" + QString::number(game) + ".w3g");
        if (file.exists() && file.open(QFile::ReadOnly)) {
            QByteArray data = file.readAll();
            file.close();
            onReplay(game, "", data, date, false, true);
            return;
        }
        // Download Replay
        QString replayLink = QString("http://storage.entgaming.net/replay/download.php?f=%1.w3g&fc=%1.w3g").arg(game);

        QNetworkRequest r;
        r.setUrl(QUrl(replayLink));
        r.setSslConfiguration(QSslConfiguration::defaultConfiguration());
        r.setAttribute(QNetworkRequest::User, game);
        r.setAttribute((QNetworkRequest::Attribute) 1001, date);
        _networkDownloader->get(r);
    }

    void onReplay(int id, QString name, const QByteArray &data, QDateTime date=QDateTime(), bool save=false, bool hasLocal=false) {
        if (_games.contains(id)) return;
        if (data.size() == 0) return;

        ENTGame* game = 0;

        if (save && !hasLocal) {
            // Save a local copy
            QFile file("replays/" + QString::number(id) + ".w3g");
            if (file.open(QFile::WriteOnly) &&
                file.write(data) != 0) {
            }
            else {
                qDebug() << "Failed to save " << QString::number(id) + ".w3g";
            }
            file.close();
            game = new ENTGame(id, name, ENTGame::IslandDefense, date);
            game->loadReplay(data);
        }
        else if (hasLocal) {
            game = new ENTGame(id, name, ENTGame::IslandDefense, date);
            game->loadReplay(data);
        }
        else {
            game = new ENTGame(id, name, ENTGame::IslandDefense, date, data);
        }

        _games.append(id);

        // Emit new game
        emit onNewENTIDGame(game);
    }

    void onReply(int page, const QByteArray &data) {
        QStringList results;
        QRegularExpression expression("<tr class=\"TableRow28\">(.+?)<\\/tr>", QRegularExpression::DotMatchesEverythingOption);
        QRegularExpressionMatchIterator i = expression.globalMatch(data);

        while (i.hasNext()) {
            QRegularExpressionMatch match = i.next();
            QString word = match.captured(1);
            results << word;
        }

        if (page > maxPage) {
            currPage = 0;
            std::cout << qPrintable("Reached end of replays.") << std::endl;
            emit finished();
            return;
        }
        for (QString result : results) {
            QRegularExpression rowdex("<td class=\"GamesRow\">(.+?)<\\/td>", QRegularExpression::DotMatchesEverythingOption);
            QRegularExpressionMatchIterator j = rowdex.globalMatch(result);

            QDateTime date;
//            date.setTimeZone(QTimeZone(-18000)); // -0500 UTC (New Orleans, Lousiana, USA)
            int index = 0;
            while (j.hasNext()) {
                QRegularExpressionMatch match = j.next();
                if (index == 2) {
                    // 16/05/2015, 20:13
                    date = QDateTime::fromString(match.captured(1), "dd/MM/yyyy, HH:mm");
                    date.setOffsetFromUtc(-18000);
                    date = date.toUTC();
                }
                index++;
            }

            //date = date.toLocalTime();

            QRegularExpression idex("islanddefense/game/(\\d*)");


            QRegularExpressionMatch match = idex.match(result);
            if (match.hasMatch()) {
                int r = match.captured(1).toInt();
                if (r <= lastPage && r != 0) {
                    currPage = 0;
                    emit finished();
                    return;
                }

                // New game
                onNewGameId(r, date);
            }
        }

        page++;
        currPage = page;
    }

signals:
    void onNewENTIDGame(ENTGame* game);
    void finished();
};

#endif // ENTCONNECTION_H
