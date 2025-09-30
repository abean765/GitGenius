#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "gitclientbackend.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    GitClientBackend backend;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("gitBackend", &backend);
    const QUrl url(u"qrc:/GitGenius/qml/main.qml"_qs);
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreated,
        &app,
        [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl) {
                QCoreApplication::exit(-1);
            }
        },
        Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
