#include <QCoreApplication>
#include "application.h"
#include <QFile>
#include "w3idreplay.h"

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    Application app;
    app.run();

//    QFile file("LastReplay.w3g");
//    if (file.open(QFile::ReadOnly)) {
//        W3IDReplay r(file.readAll());
//        r.parse();
//        qDebug() << "Complete.";
//    }

    return a.exec();
}
