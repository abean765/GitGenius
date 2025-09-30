#include "gitclientbackend.h"

#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QRegularExpression>
#include <QVariantMap>
#include <functional>
#include <memory>
#include <vector>

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

GitClientBackend::GitClientBackend(QObject *parent)
    : QObject(parent)
{
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

QVariantMap GitClientBackend::repositoryTree() const
{
    return m_repositoryTree;
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

    m_repositoryPath = canonical;
    emit repositoryPathChanged();
    refreshRepository();
    return true;
}

void GitClientBackend::refreshRepository()
{
    if (m_repositoryPath.isEmpty()) {
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

    const GitCommandResult result = runGit({"status", "--porcelain"});
    if (!result.success && result.exitCode != 0) {
        emit statusChanged();
        return;
    }

    for (const QString &line : result.stdOut) {
        if (line.isEmpty()) {
            continue;
        }
        const QString indexCode = line.left(1);
        const QString worktreeCode = line.mid(1, 1);
        QString filePath = line.mid(3);
        QString renameTarget;
        const int renameIndex = filePath.indexOf(" -> ");
        if (renameIndex != -1) {
            renameTarget = filePath.mid(renameIndex + 4);
            filePath = filePath.left(renameIndex);
        }

        QVariantMap entry;
        entry.insert("file", filePath);
        entry.insert("target", renameTarget);
        entry.insert("indexStatus", interpretStatusCode(indexCode.isEmpty() ? ' ' : indexCode[0]));
        entry.insert("worktreeStatus", interpretStatusCode(worktreeCode.isEmpty() ? ' ' : worktreeCode[0]));
        entry.insert("rawIndex", indexCode.trimmed());
        entry.insert("rawWorktree", worktreeCode.trimmed());
        m_status.append(entry);
    }

    emit statusChanged();
}

void GitClientBackend::updateSubmodules()
{
    m_submodules.clear();

    struct TreeNode
    {
        QString name;
        QString fullPath;
        QString status;
        QString details;
        QString commit;
        QString symbol;
        std::vector<std::unique_ptr<TreeNode>> children;
    };

    TreeNode rootNode;
    const QFileInfo repoInfo(m_repositoryPath);
    const QString repoDisplayName = repoInfo.fileName().isEmpty() ? repoInfo.absoluteFilePath() : repoInfo.fileName();
    rootNode.name = repoDisplayName;
    rootNode.fullPath = m_repositoryPath;
    rootNode.status = tr("Repository root");
    rootNode.details = m_repositoryPath;

    std::function<QVariantMap(const TreeNode &)> toVariant = [&](const TreeNode &node) {
        QVariantMap map;
        map.insert(QStringLiteral("name"), node.name);
        map.insert(QStringLiteral("path"), node.fullPath);
        if (!node.status.isEmpty()) {
            map.insert(QStringLiteral("status"), node.status);
        }
        if (!node.details.isEmpty()) {
            map.insert(QStringLiteral("details"), node.details);
        }
        if (!node.commit.isEmpty()) {
            map.insert(QStringLiteral("commit"), node.commit);
        }
        if (!node.symbol.isEmpty()) {
            map.insert(QStringLiteral("symbol"), node.symbol);
        }

        QVariantList childList;
        childList.reserve(static_cast<int>(node.children.size()));
        for (const auto &child : node.children) {
            childList.append(toVariant(*child));
        }
        map.insert(QStringLiteral("children"), childList);
        return map;
    };

    const GitCommandResult result = runGit({"submodule", "status", "--recursive"});
    if (!result.success && result.exitCode != 0) {
        m_repositoryTree = toVariant(rootNode);
        emit submodulesChanged();
        emit repositoryTreeChanged();
        return;
    }

    QRegularExpression rx(R"(^([ \+\-U])([0-9a-f]+)\s([^\s]+)(?:\s\((.+)\))?$)");
    for (const QString &line : result.stdOut) {
        if (line.trimmed().isEmpty()) {
            continue;
        }
        const QRegularExpressionMatch match = rx.match(line);
        if (!match.hasMatch()) {
            QVariantMap entry;
            entry.insert("raw", line);
            entry.insert("status", tr("Unknown"));
            m_submodules.append(entry);
            continue;
        }

        const QString statusSymbol = match.captured(1);
        const QString commit = match.captured(2);
        const QString path = match.captured(3);
        const QString details = match.captured(4);

        QVariantMap entry;
        entry.insert("path", path);
        entry.insert("commit", commit);
        entry.insert("status", interpretSubmoduleStatus(statusSymbol[0]));
        entry.insert("details", details);
        entry.insert("symbol", statusSymbol.trimmed());
        m_submodules.append(entry);

        TreeNode *currentNode = &rootNode;
        QString accumulatedPath;
        const QStringList segments = path.split('/', Qt::SkipEmptyParts);
        for (const QString &segment : segments) {
            accumulatedPath = accumulatedPath.isEmpty() ? segment : accumulatedPath + QLatin1Char('/') + segment;

            TreeNode *existingChild = nullptr;
            for (const auto &child : currentNode->children) {
                if (child->name == segment) {
                    existingChild = child.get();
                    break;
                }
            }

            if (!existingChild) {
                auto newChild = std::make_unique<TreeNode>();
                newChild->name = segment;
                newChild->fullPath = accumulatedPath;
                existingChild = newChild.get();
                currentNode->children.push_back(std::move(newChild));
            }

            currentNode = existingChild;
        }

        if (currentNode != &rootNode) {
            currentNode->status = interpretSubmoduleStatus(statusSymbol[0]);
            currentNode->details = details;
            currentNode->commit = commit;
            currentNode->symbol = statusSymbol.trimmed();
        }
    }

    emit submodulesChanged();
    m_repositoryTree = toVariant(rootNode);
    emit repositoryTreeChanged();
}
