import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ColumnLayout {
    id: root
    property var nodeData: ({})
    property int depth: 0
    property bool expanded: depth === 0

    Layout.fillWidth: true
    width: parent ? parent.width : implicitWidth
    spacing: 2

    RowLayout {
        id: headerRow
        Layout.fillWidth: true
        spacing: 6
        Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
        Layout.margins: 4

        Item {
            Layout.preferredWidth: depth * 16
            Layout.minimumWidth: depth * 16
            Layout.maximumWidth: depth * 16
            Layout.fillHeight: true
        }

        ToolButton {
            id: toggleButton
            visible: nodeData.children && nodeData.children.length > 0
            text: expanded ? "\u25BC" : "\u25B6"
            onClicked: expanded = !expanded
            Accessible.name: expanded ? qsTr("Collapse") : qsTr("Expand")
        }

        Label {
            text: nodeData.name || qsTr("Repository")
            font.bold: depth === 0
            Layout.fillWidth: true
            elide: Label.ElideRight
        }

        Label {
            visible: !!(nodeData.commit)
            text: nodeData.commit ? nodeData.commit.substr(0, 7) : ""
            font.family: "monospace"
            color: palette.placeholderText
        }

        Label {
            text: nodeData.status || ""
            visible: text.length > 0
            color: (nodeData.symbol && nodeData.symbol !== "") ? "#ff9800" : palette.text
        }

        Label {
            text: nodeData.details || ""
            visible: text.length > 0
            color: palette.placeholderText
            elide: Label.ElideRight
            Layout.preferredWidth: 200
        }
    }
/*
    Column {
        id: childrenContainer
        Layout.fillWidth: true
        Layout.preferredHeight: visible ? implicitHeight : 0
        spacing: 2
        visible: expanded && nodeData.children && nodeData.children.length > 0

        Repeater {
            model: nodeData.children || []
            delegate: RepositoryTreeNode {
                Layout.fillWidth: true
                nodeData: modelData
                depth: root.depth + 1
            }
        }
    }
*/
}
