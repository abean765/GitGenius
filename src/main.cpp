#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QCoreApplication>

#include "backend.h"
#include "gitclientbackend.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName(QStringLiteral("GitGenius"));
    QCoreApplication::setApplicationName(QStringLiteral("GitGenius"));

    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

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

    Backend backend;
    GitClientBackend gitBackend;

    engine.rootContext()->setContextProperty("backend", &backend);
    engine.rootContext()->setContextProperty("gitBackend", &gitBackend);

    engine.load(url);

    return app.exec();
}
