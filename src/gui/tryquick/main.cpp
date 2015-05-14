#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlComponent>

#include "dctpinterface.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QUrl sw((QStringLiteral("qrc:/main.qml")));

    qmlRegisterType<DCTPinterface>("dctpinterface", 1, 0, "DCTP");

    QQmlApplicationEngine engine;
    engine.load(sw);

    return app.exec();
}
