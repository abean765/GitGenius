#include "gitclientbackend.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QVariantMap>

#include <git2.h>

namespace {
QString interpretStatusCode(const QChar &code)
{
    switch (code.toLatin1()) {
    case 'M':
        return QObject::tr("Modified");
    case 'A':
        return QObject::tr("Added");
    case 'D':
        return QObject::tr("Deleted");
    case 'R':
        return QObject::tr("Renamed");
    case 'C':
        return QObject::tr("Copied");
    case 'U':
        return QObject::tr("Unmerged");
    case 'T':
        return QObject::tr("Type changed");
    case '?':
        return QObject::tr("Untracked");
    case '!':
        return QObject::tr("Ignored");
    case ' ':
        return QObject::tr("Clean");
    default:
        return QObject::tr("Unknown");
    }
}

QString interpretSubmoduleStatus(const QChar &code)
{
    switch (code.toLatin1()) {
    case ' ':
        return QObject::tr("Up to date");
    case '+':
        return QObject::tr("Needs update");
    case '-':
        return QObject::tr("Not initialized");
    case 'U':
        return QObject::tr("Merge conflicts");
    default:
        return QObject::tr("Unknown");
    }
}
}

namespace {
QChar indexStatusFromFlags(unsigned int status)
{
    if (status & GIT_STATUS_INDEX_NEW) {
        return QChar::fromLatin1('A');
    }
    if (status & GIT_STATUS_INDEX_MODIFIED) {
        return QChar::fromLatin1('M');
    }
    if (status & GIT_STATUS_INDEX_DELETED) {
        return QChar::fromLatin1('D');
    }
    if (status & GIT_STATUS_INDEX_RENAMED) {
        return QChar::fromLatin1('R');
    }
    if (status & GIT_STATUS_INDEX_TYPECHANGE) {
        return QChar::fromLatin1('T');
    }
    if (status & GIT_STATUS_CONFLICTED) {
        return QChar::fromLatin1('U');
    }
    return QChar::fromLatin1(' ');
}

QChar worktreeStatusFromFlags(unsigned int status)
{
    if (status & GIT_STATUS_WT_NEW) {
        return QChar::fromLatin1('?');
    }
    if (status & GIT_STATUS_WT_MODIFIED) {
        return QChar::fromLatin1('M');
    }
    if (status & GIT_STATUS_WT_DELETED) {
        return QChar::fromLatin1('D');
    }
    if (status & GIT_STATUS_WT_TYPECHANGE) {
        return QChar::fromLatin1('T');
    }
    if (status & GIT_STATUS_WT_RENAMED) {
        return QChar::fromLatin1('R');
    }
    if (status & GIT_STATUS_WT_UNREADABLE) {
        return QChar::fromLatin1('!');
    }
    if (status & GIT_STATUS_CONFLICTED) {
        return QChar::fromLatin1('U');
    }
    return QChar::fromLatin1(' ');
}
}

GitClientBackend::GitClientBackend(QObject *parent)
    : QObject(parent)
{
    git_libgit2_init();
}

GitClientBackend::~GitClientBackend()
{
    if (m_repository) {
        git_repository_free(m_repository);
        m_repository = nullptr;
    }
    git_libgit2_shutdown();
}

QString GitClientBackend::repositoryPath() const
{
    return m_repositoryPath;
}

QVariantList GitClientBackend::status() const
{
    return m_status;
}

QVariantList GitClientBackend::submodules() const
{
    return m_submodules;
}

bool GitClientBackend::openRepository(const QString &path)
{
    const QFileInfo info(path);
    if (!info.exists()) {
        return false;
    }

    QDir dir(info.isDir() ? info.absoluteFilePath() : info.absolutePath());
    if (info.isDir() && info.fileName() == QStringLiteral(".git")) {
        dir.cdUp();
    }

    const QFileInfo gitEntry(dir.absoluteFilePath(QStringLiteral(".git")));
    const bool hasGitDirectory = dir.exists(QStringLiteral(".git"));
    const bool hasGitFile = gitEntry.exists() && gitEntry.isFile();
    const bool looksBare = QFileInfo(dir.absoluteFilePath(QStringLiteral("HEAD"))).exists();
    if (!hasGitDirectory && !hasGitFile && !looksBare) {
        return false;
    }

    const QString canonical = dir.canonicalPath();
    if (canonical.isEmpty()) {
        return false;
    }

    if (canonical == m_repositoryPath) {
        refreshRepository();
        return true;
    }

    git_repository *repository = nullptr;
    const QByteArray canonicalUtf8 = canonical.toUtf8();
    int error = git_repository_open_ext(&repository, canonicalUtf8.constData(), 0, nullptr);
    if (error != 0) {
        return false;
    }

    if (m_repository) {
        git_repository_free(m_repository);
    }
    m_repository = repository;
    m_repositoryPath = canonical;
    emit repositoryPathChanged();
    refreshRepository();
    return true;
}

void GitClientBackend::refreshRepository()
{
    if (m_repositoryPath.isEmpty() || !m_repository) {
        return;
    }
    updateStatus();
    updateSubmodules();
}

QVariantMap GitClientBackend::runCustomCommand(const QStringList &arguments)
{
    const GitCommandResult result = runGit(arguments);
    QVariantMap payload;
    payload.insert("success", result.success);
    payload.insert("exitCode", result.exitCode);
    payload.insert("stdout", result.stdOut);
    payload.insert("stderr", result.stdErr);
    emit commandExecuted(payload);
    if (result.success) {
        updateStatus();
        updateSubmodules();
    }
    return payload;
}

bool GitClientBackend::stageFiles(const QStringList &files)
{
    if (files.isEmpty()) {
        return true;
    }
    QStringList arguments{"add"};
    arguments.append(files);
    const GitCommandResult result = runGit(arguments);
    if (result.success) {
        updateStatus();
    }
    return result.success;
}

bool GitClientBackend::commit(const QString &message)
{
    if (message.trimmed().isEmpty()) {
        return false;
    }

    const GitCommandResult result = runGit({"commit", "-m", message});
    if (result.success) {
        updateStatus();
        updateSubmodules();
    }
    return result.success;
}

GitCommandResult GitClientBackend::runGit(const QStringList &arguments, const QByteArray &input) const
{
    GitCommandResult result;

    if (m_repositoryPath.isEmpty()) {
        result.stdErr << tr("No repository selected.");
        return result;
    }

    QProcess process;
    process.setProgram(QStringLiteral("git"));
    process.setArguments(arguments);
    process.setWorkingDirectory(m_repositoryPath);
    process.setProcessChannelMode(QProcess::SeparateChannels);

    process.start();
    if (!process.waitForStarted()) {
        result.stdErr << tr("Unable to start git process.");
        return result;
    }

    if (!input.isEmpty()) {
        process.write(input);
    }
    process.closeWriteChannel();

    if (!process.waitForFinished()) {
        result.stdErr << tr("Git command timed out.");
        return result;
    }

    result.exitCode = process.exitCode();
    const QString stdoutText = QString::fromUtf8(process.readAllStandardOutput());
    const QString stderrText = QString::fromUtf8(process.readAllStandardError());
    result.stdOut = stdoutText.split('\n', Qt::SkipEmptyParts);
    result.stdErr = stderrText.split('\n', Qt::SkipEmptyParts);
    result.success = (process.exitStatus() == QProcess::NormalExit && result.exitCode == 0);
    return result;
}

void GitClientBackend::updateStatus()
{
    m_status.clear();

    if (!m_repository) {
        emit statusChanged();
        return;
    }

    git_status_options options;
    git_status_options_init(&options, GIT_STATUS_OPTIONS_VERSION);
    options.show = GIT_STATUS_SHOW_INDEX_AND_WORKDIR;
    options.flags = GIT_STATUS_OPT_INCLUDE_UNTRACKED | GIT_STATUS_OPT_RECURSE_UNTRACKED_DIRS
        | GIT_STATUS_OPT_RENAMES_HEAD_TO_INDEX | GIT_STATUS_OPT_RENAMES_INDEX_TO_WORKDIR;

    git_status_list *statusList = nullptr;
    const int statusError = git_status_list_new(&statusList, m_repository, &options);
    if (statusError != 0 || !statusList) {
        emit statusChanged();
        return;
    }

    const size_t entryCount = git_status_list_entrycount(statusList);
    for (size_t i = 0; i < entryCount; ++i) {
        const git_status_entry *entry = git_status_byindex(statusList, i);
        if (!entry) {
            continue;
        }

        const unsigned int statusFlags = entry->status;
        const QChar indexCode = indexStatusFromFlags(statusFlags);
        const QChar worktreeCode = worktreeStatusFromFlags(statusFlags);

        QString filePath;
        QString renameTarget;

        if (entry->index_to_workdir && entry->index_to_workdir->old_file.path) {
            filePath = QString::fromUtf8(entry->index_to_workdir->old_file.path);
        } else if (entry->head_to_index && entry->head_to_index->old_file.path) {
            filePath = QString::fromUtf8(entry->head_to_index->old_file.path);
        } else if (entry->index_to_workdir && entry->index_to_workdir->new_file.path) {
            filePath = QString::fromUtf8(entry->index_to_workdir->new_file.path);
        } else if (entry->head_to_index && entry->head_to_index->new_file.path) {
            filePath = QString::fromUtf8(entry->head_to_index->new_file.path);
        }

        if (entry->index_to_workdir && entry->index_to_workdir->new_file.path
            && entry->index_to_workdir->old_file.path
            && QString::fromUtf8(entry->index_to_workdir->new_file.path)
                != QString::fromUtf8(entry->index_to_workdir->old_file.path)) {
            renameTarget = QString::fromUtf8(entry->index_to_workdir->new_file.path);
        } else if (entry->head_to_index && entry->head_to_index->new_file.path
            && entry->head_to_index->old_file.path
            && QString::fromUtf8(entry->head_to_index->new_file.path)
                != QString::fromUtf8(entry->head_to_index->old_file.path)) {
            renameTarget = QString::fromUtf8(entry->head_to_index->new_file.path);
        }

        if (filePath.isEmpty()) {
            continue;
        }

        QVariantMap statusEntry;
        statusEntry.insert("file", filePath);
        statusEntry.insert("target", renameTarget);
        statusEntry.insert("indexStatus", interpretStatusCode(indexCode));
        statusEntry.insert("worktreeStatus", interpretStatusCode(worktreeCode));
        statusEntry.insert("rawIndex", indexCode.isSpace() ? QString() : QString(indexCode));
        statusEntry.insert("rawWorktree", worktreeCode.isSpace() ? QString() : QString(worktreeCode));
        m_status.append(statusEntry);
    }

    git_status_list_free(statusList);

    emit statusChanged();
}

void GitClientBackend::updateSubmodules()
{
    m_submodules.clear();

    if (!m_repository) {
        emit submodulesChanged();
        return;
    }

    struct SubmodulePayload {
        QVariantList *list = nullptr;
    } payload{&m_submodules};

    auto callback = [](git_submodule *sm, const char * /*name*/, void *data) -> int {
        auto *payload = static_cast<SubmodulePayload *>(data);
        if (!payload || !payload->list) {
            return 0;
        }

        QVariantMap entry;

        const char *path = git_submodule_path(sm);
        if (path) {
            entry.insert("path", QString::fromUtf8(path));
        }

        const git_oid *oid = git_submodule_head_id(sm);
        if (!oid) {
            oid = git_submodule_index_id(sm);
        }
        if (!oid) {
            oid = git_submodule_wd_id(sm);
        }
        if (oid) {
            char oidStr[GIT_OID_HEXSZ + 1] = {0};
            git_oid_tostr(oidStr, sizeof(oidStr), oid);
            entry.insert("commit", QString::fromUtf8(oidStr));
        }

        unsigned int statusFlags = 0;
        git_repository *repo = git_submodule_owner(sm);
        const char *submoduleName = git_submodule_name(sm);
        if (repo && submoduleName) {
            git_submodule_status(&statusFlags, repo, submoduleName, GIT_SUBMODULE_IGNORE_NONE);
        }

        QChar symbol = QChar::fromLatin1(' ');
        if (statusFlags & GIT_SUBMODULE_STATUS_WD_CONFLICT) {
            symbol = QChar::fromLatin1('U');
        } else if (statusFlags & GIT_SUBMODULE_STATUS_WD_UNINITIALIZED) {
            symbol = QChar::fromLatin1('-');
        } else if (statusFlags & (GIT_SUBMODULE_STATUS_INDEX_MODIFIED | GIT_SUBMODULE_STATUS_WD_MODIFIED
                                   | GIT_SUBMODULE_STATUS_WD_INDEX_MODIFIED
                                   | GIT_SUBMODULE_STATUS_WD_UNTRACKED | GIT_SUBMODULE_STATUS_INDEX_ADDED
                                   | GIT_SUBMODULE_STATUS_INDEX_DELETED)) {
            symbol = QChar::fromLatin1('+');
        }

        const char *url = git_submodule_url(sm);
        if (url) {
            entry.insert("details", QString::fromUtf8(url));
        }

        entry.insert("status", interpretSubmoduleStatus(symbol));
        entry.insert("symbol", symbol.isSpace() ? QString() : QString(symbol));

        payload->list->append(entry);
        return 0;
    };

    git_submodule_foreach(m_repository, callback, &payload);

    emit submodulesChanged();
}
