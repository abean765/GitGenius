#ifndef BACKEND_H
#define BACKEND_H

#include <QObject>
#include <QDebug>

class Backend : public QObject
{
    Q_OBJECT
public:
    Backend();

    Q_INVOKABLE void handleText(const QString& t) {
        qDebug() << "Bekommen von QML:" << t;
    }
};

#endif // BACKEND_H
