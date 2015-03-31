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
        db.setHostName("");
        db.setDatabaseName("");
        db.setUserName("");
        db.setPassword("");
        bool ok = db.open();

        if (ok) {
            _driver = db.driver();
        }

        return ok;
    }

    bool uploadENTIDGame(ENTGame* game) {
        QSqlDatabase::database().transaction();
        {
            // game: int id, string name, date time, duration float, string winner (Defenders/Titan)
            QSqlQuery q(QSqlDatabase::database());
            q.prepare("INSERT INTO `idstats`.`game` (`id`, `name`, `date`, `duration`, `winner`)"
                      "VALUES (:id, :name, :date, :duration, :winner)");
            q.bindValue(0, game->id());
            q.bindValue(1, game->name());
            q.exec();
        }
        return QSqlDatabase::database().commit();
    }
private:

    QSqlDriver* _driver = 0;
};

#endif // STATSDATABASE_H
