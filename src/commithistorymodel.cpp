#include "commithistorymodel.h"

#include <QRegularExpression>
#include <QtGlobal>

#include <algorithm>
#include <utility>

#include <git2.h>

namespace {
QString oidToString(const git_oid &oid)
{
    char buffer[GIT_OID_HEXSZ + 1] = {0};
    git_oid_tostr(buffer, sizeof(buffer), &oid);
    return QString::fromUtf8(buffer);
}

QString shortOid(const git_oid &oid)
{
    char buffer[8] = {0};
    git_oid_tostr(buffer, sizeof(buffer), &oid);
    return QString::fromUtf8(buffer);
}
}

CommitHistoryModel::CommitHistoryModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int CommitHistoryModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_entries.size();
}

QVariant CommitHistoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size()) {
        return {};
    }

    const CommitEntry &entry = m_entries.at(index.row());
    switch (role) {
    case OidRole:
        return entry.oid;
    case ShortOidRole:
        return entry.shortOid;
    case SummaryRole:
        return entry.summary;
    case LeftSummaryRole:
        return entry.leftSummary;
    case AuthorRole:
        return entry.author;
    case AuthorEmailRole:
        return entry.authorEmail;
    case TimestampRole:
        return entry.timestamp;
    case RelativeTimeRole:
        return entry.relativeTime;
    case ParentIdsRole:
        return entry.parentIds;
    case BranchNamesRole:
        return entry.branchNames;
    case LaneRole:
        return entry.laneValue;
    case LanesBeforeRole: {
        QVariantList values;
        values.reserve(entry.lanesBefore.size());
        for (int lane : entry.lanesBefore) {
            values.append(lane);
        }
        return values;
    }
    case LanesAfterRole: {
        QVariantList values;
        values.reserve(entry.lanesAfter.size());
        for (int lane : entry.lanesAfter) {
            values.append(lane);
        }
        return values;
    }
    case ConnectionsRole: {
        QVariantList values;
        values.reserve(entry.connections.size());
        for (const Connection &connection : entry.connections) {
            QVariantMap map;
            map.insert(QStringLiteral("from"), connection.fromLane);
            map.insert(QStringLiteral("to"), connection.toLane);
            map.insert(QStringLiteral("mainline"), connection.mainline);
            map.insert(QStringLiteral("parentMainline"), connection.parentMainline);
            values.append(map);
        }
        return values;
    }
    case IsMainlineRole:
        return entry.mainline;
    case GroupKeyRole:
        return entry.groupKey;
    case GroupSizeRole:
        return entry.groupSize;
    case GroupIndexRole:
        return entry.groupIndex;
    default:
        return {};
    }
}

QHash<int, QByteArray> CommitHistoryModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles.insert(OidRole, "oid");
    roles.insert(ShortOidRole, "shortOid");
    roles.insert(SummaryRole, "summary");
    roles.insert(LeftSummaryRole, "leftSummary");
    roles.insert(AuthorRole, "author");
    roles.insert(AuthorEmailRole, "authorEmail");
    roles.insert(TimestampRole, "timestamp");
    roles.insert(RelativeTimeRole, "relativeTime");
    roles.insert(ParentIdsRole, "parentIds");
    roles.insert(BranchNamesRole, "branchNames");
    roles.insert(LaneRole, "lane");
    roles.insert(LanesBeforeRole, "lanesBefore");
    roles.insert(LanesAfterRole, "lanesAfter");
    roles.insert(ConnectionsRole, "connections");
    roles.insert(IsMainlineRole, "mainline");
    roles.insert(GroupKeyRole, "groupKey");
    roles.insert(GroupSizeRole, "groupSize");
    roles.insert(GroupIndexRole, "groupIndex");
    return roles;
}

QStringList CommitHistoryModel::branches() const
{
    return m_branches;
}

QString CommitHistoryModel::currentBranch() const
{
    return m_currentBranch;
}

int CommitHistoryModel::maxLaneOffset() const
{
    const int maxMagnitude = std::max(qAbs(m_minLane), qAbs(m_maxLane));
    return maxMagnitude;
}

void CommitHistoryModel::setRepository(git_repository *repository)
{
    if (m_repository == repository) {
        return;
    }
    m_repository = repository;
    reload();
}

void CommitHistoryModel::setCurrentBranch(const QString &branchName)
{
    if (branchName == m_currentBranch) {
        return;
    }

    if (!branchName.isEmpty() && !m_branches.contains(branchName)) {
        return;
    }

    m_currentBranch = branchName;
    emit currentBranchChanged();
    collectCommits();
}

void CommitHistoryModel::reload()
{
    updateBranches();
    collectCommits();
}

void CommitHistoryModel::updateBranches()
{
    QStringList branches;
    if (!m_repository) {
        if (m_branches != branches) {
            m_branches = branches;
            emit branchesChanged();
        }
        if (!m_currentBranch.isEmpty()) {
            m_currentBranch.clear();
            emit currentBranchChanged();
        }
        return;
    }

    git_branch_iterator *iterator = nullptr;
    if (git_branch_iterator_new(&iterator, m_repository, GIT_BRANCH_LOCAL) != 0) {
        return;
    }

    git_reference *ref = nullptr;
    git_branch_t type;
    while (git_branch_next(&ref, &type, iterator) == 0) {
        const char *name = nullptr;
        if (git_branch_name(&name, ref) == 0 && name) {
            branches.append(QString::fromUtf8(name));
        }
        git_reference_free(ref);
    }
    git_branch_iterator_free(iterator);
    branches.sort(Qt::CaseInsensitive);

    if (branches != m_branches) {
        m_branches = branches;
        emit branchesChanged();
    }

    QString preferred = detectHeadBranch();
    if (m_currentBranch.isEmpty()) {
        if (!preferred.isEmpty() && branches.contains(preferred)) {
            m_currentBranch = preferred;
        } else if (branches.contains(QStringLiteral("main"))) {
            m_currentBranch = QStringLiteral("main");
        } else if (branches.contains(QStringLiteral("master"))) {
            m_currentBranch = QStringLiteral("master");
        } else if (!branches.isEmpty()) {
            m_currentBranch = branches.front();
        } else {
            m_currentBranch.clear();
        }
        if (!m_currentBranch.isEmpty()) {
            emit currentBranchChanged();
        }
    } else if (!branches.contains(m_currentBranch)) {
        m_currentBranch.clear();
        if (!preferred.isEmpty() && branches.contains(preferred)) {
            m_currentBranch = preferred;
        } else if (!branches.isEmpty()) {
            m_currentBranch = branches.front();
        }
        emit currentBranchChanged();
    }
}

void CommitHistoryModel::collectCommits()
{
    const int previousMin = m_minLane;
    const int previousMax = m_maxLane;

    beginResetModel();
    m_entries.clear();
    m_minLane = 0;
    m_maxLane = 0;

    auto finish = [&]() {
        endResetModel();
        if (previousMin != m_minLane || previousMax != m_maxLane) {
            emit laneSpanChanged();
        }
    };

    if (!m_repository || m_currentBranch.isEmpty()) {
        finish();
        return;
    }

    const QByteArray refName = QByteArrayLiteral("refs/heads/") + m_currentBranch.toUtf8();
    git_reference *branchRef = nullptr;
    if (git_reference_lookup(&branchRef, m_repository, refName.constData()) != 0) {
        finish();
        return;
    }

    git_reference *resolvedRef = nullptr;
    git_oid headOid = git_oid{};
    if (git_reference_resolve(&resolvedRef, branchRef) == 0 && resolvedRef) {
        const git_oid *target = git_reference_target(resolvedRef);
        if (target) {
            headOid = *target;
        }
    } else {
        const git_oid *target = git_reference_target(branchRef);
        if (target) {
            headOid = *target;
        }
    }
    git_reference_free(resolvedRef);
    git_reference_free(branchRef);

    if (git_oid_is_zero(&headOid)) {
        finish();
        return;
    }

    QSet<QString> mainline;
    computeMainline(headOid, mainline);

    const QHash<QString, QStringList> branchTips = collectBranchTips();

    git_revwalk *walker = nullptr;
    if (git_revwalk_new(&walker, m_repository) != 0) {
        finish();
        return;
    }
    git_revwalk_sorting(walker, GIT_SORT_TOPOLOGICAL | GIT_SORT_TIME);
    if (git_revwalk_push_ref(walker, refName.constData()) != 0) {
        git_revwalk_free(walker);
        finish();
        return;
    }

    const int maxCommits = 2000;
    QVector<CommitEntry> collected;
    collected.reserve(maxCommits);

    git_oid oid;
    int processed = 0;
    while (processed < maxCommits && git_revwalk_next(&oid, walker) == 0) {
        git_commit *commit = nullptr;
        if (git_commit_lookup(&commit, m_repository, &oid) != 0) {
            continue;
        }

        CommitEntry entry;
        entry.oid = oidToString(oid);
        entry.shortOid = shortOid(oid);

        const char *summary = git_commit_summary(commit);
        if (summary) {
            entry.summary = QString::fromUtf8(summary);
        }
        entry.leftSummary = buildLeftSummary(entry.summary);

        const git_signature *author = git_commit_author(commit);
        if (author) {
            if (author->name) {
                entry.author = QString::fromUtf8(author->name);
            }
            if (author->email) {
                entry.authorEmail = QString::fromUtf8(author->email);
            }
            entry.timestamp = author->when.time;
        }
        entry.relativeTime = formatRelativeTime(entry.timestamp);

        const unsigned int parentCount = git_commit_parentcount(commit);
        entry.parentIds.reserve(parentCount);
        for (unsigned int i = 0; i < parentCount; ++i) {
            const git_oid *parentOid = git_commit_parent_id(commit, i);
            if (parentOid) {
                entry.parentIds.append(oidToString(*parentOid));
            }
        }

        entry.mainline = mainline.contains(entry.oid);
        entry.groupKey = entry.summary.trimmed().toLower() + QLatin1Char('|') + entry.author.toLower();

        collected.append(entry);
        ++processed;
        git_commit_free(commit);
    }

    git_revwalk_free(walker);

    filterRelevantCommits(collected);

    m_entries.clear();
    m_entries.reserve(collected.size());

    QSet<QString> pendingIds;
    pendingIds.reserve(collected.size());
    for (const CommitEntry &entry : std::as_const(collected)) {
        pendingIds.insert(entry.oid);
    }

    QVector<int> activeLanes;
    activeLanes.append(0);

    QHash<int, QStringList> activeLaneBranches;
    activeLaneBranches.insert(0, QStringList{m_currentBranch});

    QHash<QString, int> futureLanes;
    QHash<QString, QStringList> futureLaneBranches;
    QHash<QString, int> laneByCommit;
    m_nextLeft = true;

    for (CommitEntry entry : collected) {
        pendingIds.remove(entry.oid);

        entry.lanesBefore.clear();
        entry.lanesAfter.clear();
        entry.connections.clear();

        QVector<int> lanesBefore = activeLanes;
        if (!lanesBefore.contains(0)) {
            lanesBefore.append(0);
        }
        std::sort(lanesBefore.begin(), lanesBefore.end());

        QSet<int> used;
        for (int lane : lanesBefore) {
            used.insert(lane);
        }
        for (auto it = futureLanes.cbegin(); it != futureLanes.cend(); ++it) {
            used.insert(it.value());
        }

        int laneValue = 0;
        if (laneByCommit.contains(entry.oid)) {
            laneValue = laneByCommit.value(entry.oid);
        } else if (entry.mainline) {
            laneValue = 0;
        } else if (futureLanes.contains(entry.oid)) {
            laneValue = futureLanes.value(entry.oid);
        } else {
            laneValue = allocateLane(used);
        }
        futureLanes.remove(entry.oid);
        laneByCommit.insert(entry.oid, laneValue);
        used.insert(laneValue);

        if (!lanesBefore.contains(laneValue)) {
            lanesBefore.append(laneValue);
            std::sort(lanesBefore.begin(), lanesBefore.end());
        }

        entry.laneValue = laneValue;
        entry.lanesBefore = lanesBefore;
        for (int lane : entry.lanesBefore) {
            m_minLane = std::min(m_minLane, lane);
            m_maxLane = std::max(m_maxLane, lane);
        }
        m_minLane = std::min(m_minLane, entry.laneValue);
        m_maxLane = std::max(m_maxLane, entry.laneValue);

        QStringList branchNames;
        if (entry.mainline) {
            branchNames = QStringList{m_currentBranch};
        } else if (futureLaneBranches.contains(entry.oid)) {
            branchNames = futureLaneBranches.take(entry.oid);
        } else if (activeLaneBranches.contains(laneValue)) {
            branchNames = activeLaneBranches.value(laneValue);
        } else if (branchTips.contains(entry.oid)) {
            branchNames = branchTips.value(entry.oid);
        }
        if (branchNames.isEmpty() && entry.mainline) {
            branchNames = QStringList{m_currentBranch};
        }
        if (branchNames.isEmpty() && !entry.mainline) {
            branchNames.append(tr("Unknown branch"));
        }
        entry.branchNames = branchNames;
        if (!branchNames.isEmpty()) {
            activeLaneBranches.insert(laneValue, branchNames);
        }

        for (const QString &parentId : std::as_const(entry.parentIds)) {
            if (!pendingIds.contains(parentId) && !futureLanes.contains(parentId)) {
                continue;
            }

            const bool parentMainline = mainline.contains(parentId);
            int parentLane = 0;
            if (laneByCommit.contains(parentId)) {
                parentLane = laneByCommit.value(parentId);
            } else if (parentMainline) {
                parentLane = 0;
            } else {
                parentLane = allocateLane(used);
            }
            laneByCommit.insert(parentId, parentLane);
            used.insert(parentLane);
            if (pendingIds.contains(parentId)) {
                futureLanes.insert(parentId, parentLane);
            }

            QStringList parentBranchNames;
            if (parentMainline) {
                parentBranchNames = QStringList{m_currentBranch};
            } else if (futureLaneBranches.contains(parentId)) {
                parentBranchNames = futureLaneBranches.value(parentId);
            } else if (activeLaneBranches.contains(parentLane)) {
                parentBranchNames = activeLaneBranches.value(parentLane);
            } else if (!branchNames.isEmpty()) {
                parentBranchNames = branchNames;
            } else if (branchTips.contains(parentId)) {
                parentBranchNames = branchTips.value(parentId);
            }
            if (!parentBranchNames.isEmpty()) {
                futureLaneBranches.insert(parentId, parentBranchNames);
            }

            m_minLane = std::min(m_minLane, parentLane);
            m_maxLane = std::max(m_maxLane, parentLane);

            Connection connection;
            connection.fromLane = laneValue;
            connection.toLane = parentLane;
            connection.mainline = entry.mainline;
            connection.parentMainline = parentMainline;
            entry.connections.append(connection);
        }

        for (auto it = futureLanes.begin(); it != futureLanes.end();) {
            if (!pendingIds.contains(it.key())) {
                futureLaneBranches.remove(it.key());
                it = futureLanes.erase(it);
            } else {
                ++it;
            }
        }

        QSet<int> futureUsed;
        for (auto it = futureLanes.cbegin(); it != futureLanes.cend(); ++it) {
            futureUsed.insert(it.value());
        }
        if (!futureUsed.contains(0)) {
            futureUsed.insert(0);
        }
        entry.lanesAfter = futureUsed.values().toVector();
        std::sort(entry.lanesAfter.begin(), entry.lanesAfter.end());

        QHash<int, QStringList> nextActiveLaneBranches;
        for (auto it = futureLanes.cbegin(); it != futureLanes.cend(); ++it) {
            const QString commitId = it.key();
            const int lane = it.value();
            QStringList names = futureLaneBranches.value(commitId);
            if (names.isEmpty() && activeLaneBranches.contains(lane)) {
                names = activeLaneBranches.value(lane);
            }
            if (lane == 0) {
                names = QStringList{m_currentBranch};
            }
            if (!names.isEmpty()) {
                nextActiveLaneBranches.insert(lane, names);
            }
        }
        if (!nextActiveLaneBranches.contains(0)) {
            nextActiveLaneBranches.insert(0, QStringList{m_currentBranch});
        }
        activeLaneBranches = nextActiveLaneBranches;
        activeLanes = entry.lanesAfter;
        for (int lane : entry.lanesAfter) {
            m_minLane = std::min(m_minLane, lane);
            m_maxLane = std::max(m_maxLane, lane);
        }

        m_entries.append(entry);
    }

    // compute grouping information
    QString lastKey;
    int currentCount = 0;
    QVector<int> indices;
    for (int i = 0; i < m_entries.size(); ++i) {
        CommitEntry &entry = m_entries[i];
        if (entry.groupKey == lastKey) {
            ++currentCount;
        } else {
            for (int index : indices) {
                m_entries[index].groupSize = currentCount;
            }
            indices.clear();
            lastKey = entry.groupKey;
            currentCount = 1;
        }
        entry.groupIndex = currentCount - 1;
        indices.append(i);
    }
    for (int index : indices) {
        m_entries[index].groupSize = currentCount;
    }

    finish();
}

void CommitHistoryModel::computeMainline(const git_oid &headOid, QSet<QString> &outMainline) const
{
    git_oid current = headOid;
    while (!git_oid_is_zero(&current)) {
        git_commit *commit = nullptr;
        if (git_commit_lookup(&commit, m_repository, &current) != 0) {
            break;
        }
        outMainline.insert(oidToString(current));
        if (git_commit_parentcount(commit) == 0) {
            git_commit_free(commit);
            break;
        }
        const git_oid *parent = git_commit_parent_id(commit, 0);
        if (!parent) {
            git_commit_free(commit);
            break;
        }
        current = *parent;
        git_commit_free(commit);
    }
}

QString CommitHistoryModel::detectHeadBranch() const
{
    if (!m_repository) {
        return {};
    }
    git_reference *head = nullptr;
    if (git_repository_head(&head, m_repository) != 0) {
        return {};
    }
    QString result;
    if (git_reference_is_branch(head)) {
        const char *shorthand = git_reference_shorthand(head);
        if (shorthand) {
            result = QString::fromUtf8(shorthand);
        }
    }
    git_reference_free(head);
    return result;
}

int CommitHistoryModel::allocateLane(QSet<int> &usedLanes)
{
    int candidate = 0;
    if (m_nextLeft) {
        candidate = -1;
        while (usedLanes.contains(candidate)) {
            --candidate;
        }
    } else {
        candidate = 1;
        while (usedLanes.contains(candidate)) {
            ++candidate;
        }
    }
    m_nextLeft = !m_nextLeft;
    usedLanes.insert(candidate);
    return candidate;
}

QString CommitHistoryModel::formatRelativeTime(qint64 timestamp)
{
    if (timestamp == 0) {
        return {};
    }
    QDateTime commitTime = QDateTime::fromSecsSinceEpoch(timestamp, Qt::UTC).toLocalTime();
    const QDateTime now = QDateTime::currentDateTime();
    qint64 seconds = commitTime.secsTo(now);
    if (seconds < 0) {
        seconds = 0;
    }
    if (seconds < 60) {
        return QObject::tr("Just now");
    }
    if (seconds < 3600) {
        return QObject::tr("%1 minutes ago").arg(seconds / 60);
    }
    if (seconds < 86400) {
        return QObject::tr("%1 hours ago").arg(seconds / 3600);
    }
    if (seconds < 86400 * 7) {
        return QObject::tr("%1 days ago").arg(seconds / 86400);
    }
    return commitTime.toString(Qt::DefaultLocaleShortDate);
}

QString CommitHistoryModel::buildLeftSummary(const QString &summary)
{
    const QRegularExpression pattern(QStringLiteral("^(?:#?)(\\d+)\\s+(.*)$"), QRegularExpression::DotMatchesEverythingOption);
    const QRegularExpressionMatch match = pattern.match(summary.trimmed());
    if (!match.hasMatch()) {
        return summary;
    }
    const QString number = match.captured(1);
    const QString rest = match.captured(2).trimmed();
    if (rest.isEmpty()) {
        return summary;
    }
    return rest + QStringLiteral(" (#%1)").arg(number);
}

void CommitHistoryModel::filterRelevantCommits(QVector<CommitEntry> &entries) const
{
    if (entries.isEmpty()) {
        return;
    }

    QHash<QString, int> indexByOid;
    indexByOid.reserve(entries.size());
    for (int i = 0; i < entries.size(); ++i) {
        indexByOid.insert(entries.at(i).oid, i);
    }

    QSet<QString> relevant;
    QVector<QString> stack;
    stack.append(entries.first().oid);
    relevant.insert(entries.first().oid);

    while (!stack.isEmpty()) {
        const QString current = stack.takeLast();
        const int index = indexByOid.value(current, -1);
        if (index < 0) {
            continue;
        }
        const CommitEntry &entry = entries.at(index);
        for (const QString &parentId : entry.parentIds) {
            const int parentIndex = indexByOid.value(parentId, -1);
            if (parentIndex < 0) {
                continue;
            }
            if (!relevant.contains(parentId)) {
                relevant.insert(parentId);
                stack.append(parentId);
            }
        }
    }

    QVector<CommitEntry> filtered;
    filtered.reserve(relevant.size());
    for (const CommitEntry &entry : std::as_const(entries)) {
        if (relevant.contains(entry.oid)) {
            filtered.append(entry);
        }
    }

    entries = filtered;
}

QHash<QString, QStringList> CommitHistoryModel::collectBranchTips() const
{
    QHash<QString, QStringList> result;
    if (!m_repository) {
        return result;
    }

    git_branch_iterator *iterator = nullptr;
    if (git_branch_iterator_new(&iterator, m_repository, GIT_BRANCH_LOCAL) != 0) {
        return result;
    }

    git_reference *ref = nullptr;
    git_branch_t type;
    while (git_branch_next(&ref, &type, iterator) == 0) {
        git_reference *resolved = nullptr;
        const git_oid *target = nullptr;
        if (git_reference_resolve(&resolved, ref) == 0 && resolved) {
            target = git_reference_target(resolved);
        }
        if (!target) {
            target = git_reference_target(ref);
        }
        if (target) {
            const QString oid = oidToString(*target);
            const char *name = nullptr;
            if (git_branch_name(&name, ref) == 0 && name) {
                result[oid].append(QString::fromUtf8(name));
            }
        }
        git_reference_free(resolved);
        git_reference_free(ref);
    }

    git_branch_iterator_free(iterator);
    return result;
}

