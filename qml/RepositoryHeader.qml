import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ToolBar {
    id: toolbar
    property string repositoryPath: ""
    property string repositoryRootFolder: ""
    signal openRequested()
    signal refreshRequested()
    signal rootSelectionRequested()

    RowLayout {
        anchors.fill: parent
        spacing: 12
        //padding: 8

        RowLayout {
            spacing: 6

            ToolButton {
                icon.source: "qrc:/GitGenius/assets/icons/repository.svg"
                text: qsTr("Open")
                display: AbstractButton.IconOnly
                onClicked: toolbar.openRequested()
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Open repository")
            }

            ToolButton {
                text: qsTr("Folder")
                display: AbstractButton.TextOnly
                onClicked: toolbar.rootSelectionRequested()
                ToolTip.visible: hovered
                ToolTip.text: qsTr("Choose a folder that contains Git repositories")
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            Label {
                text: repositoryPath.length > 0 ? repositoryPath : (repositoryRootFolder.length > 0 ? repositoryRootFolder : qsTr("No repository selected"))
                font.bold: repositoryPath.length > 0
                elide: Label.ElideRight
                Layout.fillWidth: true
            }

            Label {
                text: repositoryPath.length > 0
                      ? qsTr("Status and submodules are displayed below")
                      : (repositoryRootFolder.length > 0
                         ? qsTr("Select a repository from the section below")
                         : qsTr("Use the folder button to choose a Git repository"))
                color: palette.placeholderText
                font.pixelSize: 12
                elide: Label.ElideRight
                Layout.fillWidth: true
            }
        }

        ToolButton {
            icon.source: "qrc:/GitGenius/assets/icons/branch.svg"
            text: qsTr("Refresh")
            display: AbstractButton.IconOnly
            enabled: repositoryPath.length > 0
            onClicked: toolbar.refreshRequested()
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Refresh status and submodules")
        }
    }
}
