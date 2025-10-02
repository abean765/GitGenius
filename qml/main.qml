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
        onOpenRequested: repositoryDialog.open()
        onRefreshRequested: gitBackend.refreshRepository()
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

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        StatusList {
            Layout.fillWidth: true
            Layout.fillHeight: true
            statusModel: gitBackend.status
            onStageRequested: function(path) { gitBackend.stageFiles([path]) }
        }

        SubmoduleList {
            Layout.fillWidth: true
            Layout.preferredHeight: 280
            submoduleModel: gitBackend.submodules
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
