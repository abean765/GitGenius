import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Dialog {
    id: root
    property alias commandText: commandField.text
    signal commandSubmitted(var arguments)

    modal: true
    title: qsTr("Run Custom Git Command")
    standardButtons: Dialog.Ok | Dialog.Cancel
    focus: true

    onAccepted: {
        if (commandField.text.trim().length === 0)
            return
        commandSubmitted(commandField.text.split(/\s+/))
        commandField.text = ""
    }

    contentItem: ColumnLayout {
        spacing: 12
        padding: 12

        Label {
            text: qsTr("Enter arguments as you would after the 'git' command.")
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }

        TextField {
            id: commandField
            placeholderText: qsTr("status --short")
            Layout.fillWidth: true
        }
    }
}
