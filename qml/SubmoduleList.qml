import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Frame {
    id: root
    property var submoduleModel: []

    readonly property int cardSize: 140

    function _stringToHash(value) {
        var hash = 0;
        if (!value)
            value = "";
        for (var i = 0; i < value.length; ++i) {
            hash = ((hash << 5) - hash) + value.charCodeAt(i);
            hash = hash & 0xffffffff;
        }
        return Math.abs(hash);
    }

    function colorFromString(value) {
        var hash = _stringToHash(value);
        var hue = hash % 360;
        var saturation = 0.45 + ((hash >> 3) % 30) / 100;
        var lightness = 0.55 + ((hash >> 6) % 20) / 100;
        return Qt.hsla(hue / 360.0,
                       Math.min(saturation, 0.8),
                       Math.min(lightness, 0.8),
                       1.0);
    }

    function textColorForBackground(color, subtle) {
        var luminance = 0.299 * color.r + 0.587 * color.g + 0.114 * color.b;
        if (subtle)
            return luminance > 0.6 ? Qt.rgba(0, 0, 0, 0.65) : Qt.rgba(1, 1, 1, 0.7);
        return luminance > 0.6 ? "#202020" : "#ffffff";
    }

    function _nextDistinctLetter(pool, preferVowel, used) {
        if (!pool)
            return "";
        var vowels = "AEIOUY";
        var vowelCandidate = "";
        var consonantCandidate = "";
        used = used || [];
        for (var i = 0; i < pool.length; ++i) {
            var ch = pool[i];
            if (!(/[A-Za-z0-9]/).test(ch))
                continue;
            var upper = ch.toUpperCase();
            if (used.indexOf(upper) !== -1)
                continue;
            if (vowels.indexOf(upper) !== -1) {
                if (!vowelCandidate)
                    vowelCandidate = upper;
                if (preferVowel)
                    return upper;
            } else {
                if (!consonantCandidate)
                    consonantCandidate = upper;
                if (!preferVowel)
                    return upper;
            }
        }
        return preferVowel ? (vowelCandidate || consonantCandidate) : (consonantCandidate || vowelCandidate);
    }

    function abbreviationForName(name) {
        if (!name)
            return "???";

        var normalized = name.replace(/[^A-Za-z0-9]+/g, " ").trim();
        if (!normalized.length)
            normalized = name.replace(/[^A-Za-z0-9]/g, "");

        var tokens = normalized.split(/\s+/).filter(function(token) { return token.length > 0; });
        if (tokens.length === 0)
            tokens = [name.replace(/[^A-Za-z0-9]/g, "")];

        var used = [];
        var letters = [];

        function addLetter(letter) {
            if (!letter)
                return;
            var upper = letter.toUpperCase();
            if (used.indexOf(upper) === -1) {
                letters.push(upper);
                used.push(upper);
            }
        }

        addLetter(tokens[0][0]);

        if (tokens.length > 1)
            addLetter(tokens[tokens.length - 1][0]);

        if (tokens.length > 2) {
            var middleIndex = Math.floor(tokens.length / 2);
            if (middleIndex === tokens.length - 1)
                middleIndex = Math.max(1, tokens.length - 2);
            addLetter(tokens[middleIndex][0]);
        } else {
            var searchPool = tokens[0].slice(1);
            if (tokens.length > 1)
                searchPool += tokens[1].slice(1);
            var preferred = _nextDistinctLetter(searchPool, true, used);
            addLetter(preferred);
        }

        var combined = tokens.join("");
        while (letters.length < 3) {
            var preferVowel = letters.length === 1;
            var next = _nextDistinctLetter(combined, preferVowel, used);
            if (!next)
                break;
            addLetter(next);
        }

        while (letters.length < 3)
            letters.push("?");

        return letters.slice(0, 3).join("");
    }

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
            visible: submoduleModel.length > 0
            clip: true

            contentWidth: availableWidth

            Item {
                width: parent.width
                implicitHeight: submoduleFlow.implicitHeight + 24
                height: implicitHeight

                Flow {
                    id: submoduleFlow
                    anchors.fill: parent
                    anchors.margins: 12
                    spacing: 12
                    implicitHeight: childrenRect.height

                    Repeater {
                        model: submoduleModel

                        delegate: Item {
                        width: root.cardSize
                        height: root.cardSize

                        readonly property string repoLabel: modelData.path || modelData.raw || ""
                        readonly property color baseColor: root.colorFromString(repoLabel)
                        readonly property string abbrev: root.abbreviationForName(repoLabel)

                        Rectangle {
                            anchors.fill: parent
                            radius: 12
                            color: baseColor
                            border.color: Qt.darker(baseColor, 1.2)
                            border.width: 1

                            Column {
                                anchors.centerIn: parent
                                width: parent.width - 24
                                spacing: 6

                                Label {
                                    text: abbrev
                                    font.bold: true
                                    font.pixelSize: 32
                                    color: root.textColorForBackground(baseColor, false)
                                    horizontalAlignment: Text.AlignHCenter
                                    verticalAlignment: Text.AlignVCenter
                                    width: parent.width
                                }

                                Label {
                                    text: repoLabel
                                    font.pixelSize: 12
                                    wrapMode: Label.WordWrap
                                    horizontalAlignment: Text.AlignHCenter
                                    color: root.textColorForBackground(baseColor, true)
                                }

                                Label {
                                    text: modelData.details ? modelData.details : ""
                                    visible: text.length > 0
                                    font.pixelSize: 11
                                    wrapMode: Label.WordWrap
                                    horizontalAlignment: Text.AlignHCenter
                                    color: root.textColorForBackground(baseColor, true)
                                }

                                Label {
                                    text: modelData.status || qsTr("Unknown")
                                    font.pixelSize: 11
                                    horizontalAlignment: Text.AlignHCenter
                                    color: root.textColorForBackground(baseColor, true)
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                hoverEnabled: true
                                acceptedButtons: Qt.NoButton
                                id: hoverArea
                            }

                            ToolTip.visible: hoverArea.containsMouse
                            ToolTip.text: modelData.commit ? qsTr("Commit %1").arg(modelData.commit) : (modelData.raw || repoLabel)
                        }
                    }
                }
            }
        }

        Label {
            visible: submoduleModel.length === 0
            text: qsTr("No submodules detected")
            horizontalAlignment: Text.AlignHCenter
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            opacity: 0.6
        }
    }
}
