import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ToolBar {
    id: toolbar
    property string repositoryPath: ""
    property string repositoryFolder: ""
    signal openRequested()
    signal refreshRequested()
    signal chooseFolderRequested()

    RowLayout {
        anchors.fill: parent
        spacing: 12
        //padding: 8

        ToolButton {
            icon.source: "qrc:/GitGenius/assets/icons/repository.svg"
            text: qsTr("Open")
            display: AbstractButton.IconOnly
            onClicked: toolbar.openRequested()
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Open repository")
        }

        ToolButton {
            icon.source: "qrc:/GitGenius/assets/icons/submodule.svg"
            text: qsTr("Workspace")
            display: AbstractButton.IconOnly
            onClicked: toolbar.chooseFolderRequested()
            ToolTip.visible: hovered
            ToolTip.text: qsTr("Choose repository folder")
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            Label {
                text: repositoryPath.length > 0 ? repositoryPath : qsTr("No repository selected")
                font.bold: repositoryPath.length > 0
                elide: Label.ElideRight
                Layout.fillWidth: true
            }

            Label {
                text: repositoryPath.length > 0
                      ? qsTr("Status and submodules are displayed below")
                      : (repositoryFolder.length > 0
                         ? qsTr("Repositories from %1 are listed below").arg(repositoryFolder)
                         : qsTr("Use the workspace button to choose a folder with Git repositories"))
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
