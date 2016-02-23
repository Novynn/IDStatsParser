#ifndef SCRAPERAPPLICATION_H
#define SCRAPERAPPLICATION_H

#include <QObject>
#include "entconnection.h"
#include <QThread>
#include "statsdatabase.h"

class ScraperApplication : public QObject
{
    Q_OBJECT
public:
    explicit ScraperApplication(QObject *parent = 0);
    ~ScraperApplication();

    void run();
public slots:
    void onNewENTIDGame(ENTGame* game);
signals:
    void beginParsing();
private:
    ENTConnection* _ent;
    QThread* _parseThread;
    StatsDatabase* _database;
};

#endif // SCRAPERAPPLICATION_H
