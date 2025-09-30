import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Frame {
    id: root
    property var treeModel: ({})

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        Label {
            text: qsTr("Repository tree")
            font.bold: true
            Layout.fillWidth: true
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            Column {
                id: treeContent
                width: parent ? parent.width : implicitWidth
                spacing: 4

                RepositoryTreeNode {
                    id: rootNode
                    visible: treeModel && treeModel.name
                    nodeData: treeModel
                    depth: 0
                    Layout.fillWidth: true
                }

                Label {
                    Layout.fillWidth: true
                    visible: !rootNode.visible
                    text: qsTr("Select a repository to view its submodules.")
                    horizontalAlignment: Text.AlignHCenter
                    color: palette.placeholderText
                    padding: 12
                    wrapMode: Text.Wrap
                }
            }
        }
    }
}
