#include <QCoreApplication>
#include "scraperapplication.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    ScraperApplication app;
    app.run();

    return a.exec();
}

