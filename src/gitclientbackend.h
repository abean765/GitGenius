#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QUrl>

class CommitHistoryModel;

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
    Q_PROPERTY(QStringList branches READ branches NOTIFY branchesChanged FINAL)
    Q_PROPERTY(QString currentBranch READ currentBranch WRITE setCurrentBranch NOTIFY currentBranchChanged FINAL)
    Q_PROPERTY(QObject *commitHistoryModel READ commitHistoryModel CONSTANT)

public:
    explicit GitClientBackend(QObject *parent = nullptr);
    ~GitClientBackend() override;

    QString repositoryPath() const;
    QVariantList status() const;
    QVariantList submodules() const;
    QStringList branches() const;
    QString currentBranch() const;
    QObject *commitHistoryModel() const;

    Q_INVOKABLE bool openRepository(const QUrl &url);
    Q_INVOKABLE void refreshRepository();
    Q_INVOKABLE QVariantMap runCustomCommand(const QStringList &arguments);
    Q_INVOKABLE bool stageFiles(const QStringList &files);
    Q_INVOKABLE bool commit(const QString &message);
    Q_INVOKABLE void setCurrentBranch(const QString &branchName);

signals:
    void repositoryPathChanged();
    void statusChanged();
    void submodulesChanged();
    void branchesChanged();
    void currentBranchChanged();
    void commandExecuted(const QVariantMap &result);

private:
    GitCommandResult runGit(const QStringList &arguments, const QByteArray &input = QByteArray()) const;
    void updateStatus();
    void updateSubmodules();

    QString m_repositoryPath;
    QVariantList m_status;
    QVariantList m_submodules;
    git_repository *m_repository = nullptr;
    CommitHistoryModel *m_commitHistoryModel = nullptr;
};
