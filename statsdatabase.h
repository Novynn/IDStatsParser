#ifndef STATSDATABASE_H
#define STATSDATABASE_H

#include "entgame.h"

#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlQuery>

class StatsDatabase
{
public:
    StatsDatabase();

    bool connect() {
        QSqlDatabase db = QSqlDatabase::addDatabase("QMYSQL");
        db.setHostName("rory.io");
        db.setDatabaseName("idstats");
        db.setUserName("root");
        db.setPassword("lll");
        bool ok = db.open();

        if (ok) {
            _driver = db.driver();
        }

        qDebug() << "Database Error: " << db.lastError().text();


        return ok;
    }

    bool uploadENTIDGame(ENTGame* game) {
        if (game->id() == 0) return false;
        if (game->name().isEmpty()) return false;
        if (game->durationAsMS() == 0) return false;
        QSqlDatabase::database().transaction();
        {
            // game: int id, string name, date time, duration float, string winner (Defenders/Titan)
            QSqlQuery q(QSqlDatabase::database());
            q.prepare("INSERT INTO `idstats`.`game` (`id`, `name`, `date`, `duration`, `winner`)"
                      "VALUES (:id, :name, :date, :duration, :winner)");
            q.bindValue(":id", game->id());
            q.bindValue(":name", game->name());
            q.bindValue(":date", game->date());
            q.bindValue(":duration", game->durationAsMS());
            q.bindValue(":winner", game->findWinner());
            if (!q.exec()) {
                QSqlDatabase::database().rollback();
                qDebug() << q.lastError().text();
                return false;
            }
        }
        return QSqlDatabase::database().commit();
    }
private:

    QSqlDriver* _driver = 0;
};

#endif // STATSDATABASE_H
