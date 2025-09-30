import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Frame {
    id: root
    property var submoduleModel: []

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        Label {
            text: qsTr("Subrepositories")
            font.bold: true
            Layout.fillWidth: true
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true

            ListView {
                model: submoduleModel
                clip: true
                spacing: 4

                delegate: ItemDelegate {
                    width: ListView.view.width
                    contentItem: RowLayout {
                        spacing: 12
                        Label {
                            text: modelData.path || modelData.raw
                            Layout.fillWidth: true
                            elide: Label.ElideRight
                        }
                        Label {
                            text: modelData.status || qsTr("Unknown")
                            color: "#ff9800"
                        }
                        Label {
                            text: modelData.details ? modelData.details : ""
                            color: palette.placeholderText
                            elide: Label.ElideRight
                            Layout.preferredWidth: 160
                        }
                    }
                    ToolTip.visible: hovered
                    ToolTip.text: modelData.commit ? qsTr("Commit %1").arg(modelData.commit) : (modelData.raw || "")
                }

                footer: Label {
                    visible: submoduleModel.length === 0
                    text: qsTr("No submodules detected")
                    horizontalAlignment: Text.AlignHCenter
                    width: ListView.view ? ListView.view.width : implicitWidth
                    padding: 12
                    opacity: 0.6
                }
            }
        }
    }
}
