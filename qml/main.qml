import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs

ApplicationWindow {
    id: window
    width: 1200
    height: 720
    visible: true
    title: qsTr("GitGenius - Git Client with Subrepository Support")

    header: RepositoryHeader {
        repositoryPath: gitBackend.repositoryPath
        repositoryFolder: gitBackend.repositoryRootPath
        onOpenRequested: repositoryDialog.open()
        onRefreshRequested: gitBackend.refreshRepository()
        onChooseFolderRequested: workspaceDialog.open()
    }

    GitCommandDialog {
        id: commandDialog
        onCommandSubmitted: function(args) {
            gitBackend.runCustomCommand(args)
        }
    }

    Dialog {
        id: commandResultDialog
        title: qsTr("Git Command Result")
        modal: true
        standardButtons: Dialog.Ok
        contentItem: ScrollView {
            implicitWidth: 520
            implicitHeight: 240
            TextArea {
                id: commandResultText
                readOnly: true
                wrapMode: TextEdit.Wrap
                text: ""
                anchors.fill: parent
            }
        }
    }

    Connections {
        target: gitBackend
        function onCommandExecuted(result) {
            const stdoutText = result.stdout && result.stdout.length ? result.stdout.join("\n") : qsTr("<no output>")
            const stderrText = result.stderr && result.stderr.length ? "\n\n" + qsTr("Errors:\n%1").arg(result.stderr.join("\n")) : ""
            const summary = qsTr("Exit code: %1").arg(result.exitCode) + "\n" + stdoutText + stderrText
            commandResultText.text = summary
            commandResultDialog.open()
        }
    }

    FolderDialog {
        id: repositoryDialog
        title: qsTr("Open Git Repository")
        onAccepted: {
            gitBackend.openRepository(repositoryDialog.currentFolder)
        }
    }

    FolderDialog {
        id: workspaceDialog
        title: qsTr("Choose Repository Folder")
        onAccepted: {
            gitBackend.setRepositoryRoot(workspaceDialog.currentFolder)
        }
    }

    SplitView {
        anchors.fill: parent
        anchors.margins: 12
        orientation: Qt.Vertical

        CommitHistoryView {
            SplitView.fillWidth: true
            SplitView.fillHeight: true
            model: gitBackend.commitHistoryModel
            branches: gitBackend.branches
            currentBranch: gitBackend.currentBranch
            onBranchSelected: gitBackend.setCurrentBranch(branch)
        }

        ColumnLayout {
            id: repositoriesSection
            SplitView.fillWidth: true
            SplitView.preferredHeight: 280
            SplitView.minimumHeight: 120
            spacing: 12

            SubmoduleList {
                Layout.fillWidth: true
                Layout.fillHeight: true
                repositoryPath: gitBackend.repositoryPath
                repositoryRootPath: gitBackend.repositoryRootPath
                repositoryModel: gitBackend.availableRepositories
                submoduleModel: gitBackend.submodules
                onRepositoryActivated: function(path) {
                    gitBackend.openRepositoryPath(path)
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Button {
                    text: qsTr("Commit")
                    enabled: gitBackend.status && gitBackend.status.length > 0
                    onClicked: commitDialog.open()
                }

                Button {
                    text: qsTr("Run Git Command")
                    onClicked: commandDialog.open()
                }
            }
        }
    }

    Dialog {
        id: commitDialog
        title: qsTr("Commit Changes")
        modal: true
        standardButtons: Dialog.Ok | Dialog.Cancel
        focus: true
        onAccepted: {
            if (commitMessage.text.length > 0) {
                gitBackend.commit(commitMessage.text)
                commitMessage.text = ""
            }
        }
        contentItem: ColumnLayout {
            spacing: 12
            //padding: 12
            TextArea {
                id: commitMessage
                placeholderText: qsTr("Describe your changes")
                wrapMode: TextEdit.Wrap
                Layout.preferredWidth: 480
                Layout.preferredHeight: 160
            }
        }
    }
}
