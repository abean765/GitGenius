import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Frame {
    id: root
    property var statusModel: []
    signal stageRequested(string path)

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        Label {
            text: qsTr("Working Tree Status")
            font.bold: true
            Layout.fillWidth: true
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                id: listView
                model: statusModel
                clip: true
                spacing: 4

                delegate: ItemDelegate {
                    width: ListView.view.width
                    text: modelData.file
                    icon.source: modelData.rawIndex === "" && modelData.rawWorktree === "" ? "" : "qrc:/GitGenius/assets/icons/branch.svg"
                    contentItem: RowLayout {
                        spacing: 12
                        Label {
                            text: modelData.file
                            Layout.fillWidth: true
                            elide: Label.ElideRight
                        }
                        Label {
                            text: modelData.indexStatus
                            color: "#1976d2"
                        }
                        Label {
                            text: modelData.worktreeStatus
                            color: "#d32f2f"
                        }
                        Button {
                            visible: modelData.worktreeStatus !== qsTr("Clean")
                            text: qsTr("Stage")
                            onClicked: root.stageRequested(modelData.file)
                        }
                    }
                    ToolTip.visible: hovered && modelData.target && modelData.target.length > 0
                    ToolTip.text: modelData.target && modelData.target.length > 0 ? qsTr("Renamed to %1").arg(modelData.target) : ""
                }

                footer: Label {
                    visible: statusModel.length === 0
                    text: qsTr("Working tree is clean")
                    horizontalAlignment: Text.AlignHCenter
                    width: ListView.view ? ListView.view.width : implicitWidth
                    padding: 12
                    opacity: 0.6
                }
            }
        }
    }
}
