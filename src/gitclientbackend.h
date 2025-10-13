#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QUrl>
#include <QDir>

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
    Q_PROPERTY(QString repositoryRootFolder READ repositoryRootFolder WRITE setRepositoryRootFolder NOTIFY repositoryRootFolderChanged FINAL)
    Q_PROPERTY(QVariantList availableRepositories READ availableRepositories NOTIFY availableRepositoriesChanged FINAL)

public:
    explicit GitClientBackend(QObject *parent = nullptr);
    ~GitClientBackend() override;

    QString repositoryPath() const;
    QVariantList status() const;
    QVariantList submodules() const;
    QString repositoryRootFolder() const;
    void setRepositoryRootFolder(const QString &path);
    QVariantList availableRepositories() const;

    Q_INVOKABLE bool openRepository(const QUrl &url);
    Q_INVOKABLE bool openRepositoryByPath(const QString &path);
    Q_INVOKABLE void refreshRepository();
    Q_INVOKABLE bool setRepositoryRoot(const QUrl &url);
    Q_INVOKABLE void refreshAvailableRepositories();
    Q_INVOKABLE QVariantMap runCustomCommand(const QStringList &arguments);
    Q_INVOKABLE bool stageFiles(const QStringList &files);
    Q_INVOKABLE bool commit(const QString &message);

signals:
    void repositoryPathChanged();
    void statusChanged();
    void submodulesChanged();
    void repositoryRootFolderChanged();
    void availableRepositoriesChanged();
    void commandExecuted(const QVariantMap &result);

private:
    GitCommandResult runGit(const QStringList &arguments, const QByteArray &input = QByteArray()) const;
    void updateStatus();
    void updateSubmodules();
    void updateAvailableRepositories();
    static bool isGitRepository(const QDir &dir);

    QString m_repositoryPath;
    QVariantList m_status;
    QVariantList m_submodules;
    QString m_repositoryRootFolder;
    QVariantList m_availableRepositories;
    git_repository *m_repository = nullptr;
};
