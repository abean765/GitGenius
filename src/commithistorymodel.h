#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QHash>
#include <QSet>
#include <QStringList>
#include <QVector>

struct git_repository;
struct git_oid;

class CommitHistoryModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QStringList branches READ branches NOTIFY branchesChanged FINAL)
    Q_PROPERTY(QString currentBranch READ currentBranch WRITE setCurrentBranch NOTIFY currentBranchChanged FINAL)
    Q_PROPERTY(int maxLaneOffset READ maxLaneOffset NOTIFY laneSpanChanged FINAL)

public:
    enum Roles {
        OidRole = Qt::UserRole + 1,
        ShortOidRole,
        SummaryRole,
        LeftSummaryRole,
        AuthorRole,
        AuthorEmailRole,
        TimestampRole,
        RelativeTimeRole,
        ParentIdsRole,
        LaneRole,
        LanesBeforeRole,
        LanesAfterRole,
        ConnectionsRole,
        IsMainlineRole,
        GroupKeyRole,
        GroupSizeRole,
        GroupIndexRole
    };

    explicit CommitHistoryModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QStringList branches() const;
    QString currentBranch() const;
    int maxLaneOffset() const;

    void setRepository(git_repository *repository);
    Q_INVOKABLE void setCurrentBranch(const QString &branchName);
    void reload();

signals:
    void branchesChanged();
    void currentBranchChanged();
    void laneSpanChanged();

private:
    struct Connection {
        int fromLane = 0;
        int toLane = 0;
        bool mainline = false;
        bool parentMainline = false;
    };

    struct CommitEntry {
        QString oid;
        QString shortOid;
        QString summary;
        QString leftSummary;
        QString author;
        QString authorEmail;
        qint64 timestamp = 0;
        QString relativeTime;
        QStringList parentIds;
        QVector<int> lanesBefore;
        QVector<int> lanesAfter;
        QVector<Connection> connections;
        int laneValue = 0;
        bool mainline = false;
        QString groupKey;
        int groupSize = 1;
        int groupIndex = 0;
    };

    void updateBranches();
    void collectCommits();
    void computeMainline(const git_oid &headOid, QSet<QString> &outMainline) const;
    QString detectHeadBranch() const;
    int allocateLane(QSet<int> &usedLanes);
    static QString formatRelativeTime(qint64 timestamp);
    static QString buildLeftSummary(const QString &summary);
    void filterRelevantCommits(QVector<CommitEntry> &entries) const;

    git_repository *m_repository = nullptr;
    QStringList m_branches;
    QString m_currentBranch;
    QVector<CommitEntry> m_entries;
    bool m_nextLeft = true;
    int m_minLane = 0;
    int m_maxLane = 0;
};

