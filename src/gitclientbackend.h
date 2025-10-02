#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QUrl>

struct git_repository;

struct GitCommandResult
{
    bool success = false;
    int exitCode = -1;
    QStringList stdOut;
    QStringList stdErr;
};

class GitClientBackend : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString repositoryPath READ repositoryPath NOTIFY repositoryPathChanged FINAL)
    Q_PROPERTY(QVariantList status READ status NOTIFY statusChanged FINAL)
    Q_PROPERTY(QVariantList submodules READ submodules NOTIFY submodulesChanged FINAL)

public:
    explicit GitClientBackend(QObject *parent = nullptr);
    ~GitClientBackend() override;

    QString repositoryPath() const;
    QVariantList status() const;
    QVariantList submodules() const;

    Q_INVOKABLE bool openRepository(const QUrl &url);
    Q_INVOKABLE void refreshRepository();
    Q_INVOKABLE QVariantMap runCustomCommand(const QStringList &arguments);
    Q_INVOKABLE bool stageFiles(const QStringList &files);
    Q_INVOKABLE bool commit(const QString &message);

signals:
    void repositoryPathChanged();
    void statusChanged();
    void submodulesChanged();
    void commandExecuted(const QVariantMap &result);

private:
    GitCommandResult runGit(const QStringList &arguments, const QByteArray &input = QByteArray()) const;
    void updateStatus();
    void updateSubmodules();

    QString m_repositoryPath;
    QVariantList m_status;
    QVariantList m_submodules;
    git_repository *m_repository = nullptr;
};
