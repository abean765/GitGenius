import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ToolBar {
    id: toolbar
    property string repositoryPath: ""
    signal openRequested()
    signal refreshRequested()

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
                text: repositoryPath.length > 0 ? qsTr("Status and submodules are displayed below") : qsTr("Use the folder button to choose a Git repository")
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
