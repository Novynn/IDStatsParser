#include "scraperapplication.h"

ScraperApplication::ScraperApplication(QObject *parent)
    : QObject(parent)
    , _database(new StatsDatabase())
{

}

ScraperApplication::~ScraperApplication(){
    _parseThread->quit();
    _parseThread->wait();
}

void ScraperApplication::run() {
    std::cout << "IDReplayScraper started..." << std::endl;

    if (!_database->connect()) {
        std::cout << "Failed to connect to the database." << std::endl;
        return;
    }

    _ent = new ENTConnection();
    _parseThread = new QThread();
    _ent->moveToThread(_parseThread);
    connect(this, &ScraperApplication::beginParsing, _ent, &ENTConnection::run);
    connect(_ent, &ENTConnection::onNewENTIDGame, this, &ScraperApplication::onNewENTIDGame);
    _parseThread->start();



    emit beginParsing();
}

void ScraperApplication::onNewENTIDGame(ENTGame *game) {
    if (_database->uploadENTIDGame(game)) {
        std::cout << "Uploaded replay #" << game->id() << std::endl;
    }
    game->unloadReplay();
    delete game;
}
