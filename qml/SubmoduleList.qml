import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Frame {
    id: root
    property var submoduleModel: []
    property var repositoryModel: []
    property string repositoryPath: ""
    property string repositoryRootPath: ""
    signal repositoryActivated(string path)

    readonly property int cardSize: 140
    readonly property bool showingRepositories: repositoryPath.length === 0
    readonly property bool hasRepositoryRoot: repositoryRootPath.length > 0

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

    function leafName(value) {
        if (!value)
            return "";

        var normalized = value.replace(/\\/g, "/");
        normalized = normalized.replace(/\/+$/, "");
        if (!normalized.length)
            return "";

        var segments = normalized.split("/");
        for (var i = segments.length - 1; i >= 0; --i) {
            if (segments[i].length)
                return segments[i];
        }

        return value;
    }

    function textColorForBackground(color, subtle) {
        var luminance = 0.299 * color.r + 0.587 * color.g + 0.114 * color.b;
        if (subtle)
            return luminance > 0.6 ? Qt.rgba(0, 0, 0, 0.65) : Qt.rgba(1, 1, 1, 0.7);
        return luminance > 0.6 ? "#202020" : "#ffffff";
    }

    function abbreviationForName(name, length) {
        length = Math.max(1, length || 5);

        if (!name)
            return Array(length + 1).join("?").slice(0, length);

        var normalized = name.replace(/[^A-Za-z0-9]+/g, " ").trim();
        if (!normalized.length)
            normalized = name.replace(/[^A-Za-z0-9]/g, "");

        var tokens = normalized.split(/\s+/).filter(function(token) { return token.length > 0; });
        if (tokens.length === 0) {
            var fallback = name.replace(/[^A-Za-z0-9]/g, "");
            if (fallback.length)
                tokens = [fallback];
        }

        if (tokens.length === 0)
            return Array(length + 1).join("?").slice(0, length);

        var combined = tokens.join("");
        if (!combined.length)
            return Array(length + 1).join("?").slice(0, length);

        var letters = [];
        var used = [];
        var lastIndex = -1;
        var vowels = "AEIOUY";

        function addCharAt(index) {
            if (index <= lastIndex || index < 0 || index >= combined.length)
                return false;

            var ch = combined[index];
            if (!(/[A-Za-z0-9]/).test(ch))
                return false;

            var upper = ch.toUpperCase();
            if (used.indexOf(upper) !== -1)
                return false;

            letters.push(upper);
            used.push(upper);
            lastIndex = index;
            return true;
        }

        var offsets = [];
        var offset = 0;
        for (var t = 0; t < tokens.length; ++t) {
            offsets.push(offset);
            offset += tokens[t].length;
        }

        for (var i = 0; i < tokens.length && letters.length < length; ++i) {
            var token = tokens[i];
            var baseIndex = offsets[i];
            for (var j = 0; j < token.length; ++j) {
                if (addCharAt(baseIndex + j))
                    break;
            }
        }

        while (letters.length < length) {
            var preferVowel = letters.length === 1 && vowels.indexOf(letters[0]) === -1;
            var chosenIndex = -1;
            var fallbackIndex = -1;

            for (var idx = lastIndex + 1; idx < combined.length; ++idx) {
                var candidate = combined[idx];
                if (!(/[A-Za-z0-9]/).test(candidate))
                    continue;

                var upperCandidate = candidate.toUpperCase();
                if (used.indexOf(upperCandidate) !== -1)
                    continue;

                if (!preferVowel) {
                    chosenIndex = idx;
                    break;
                }

                if (vowels.indexOf(upperCandidate) !== -1) {
                    chosenIndex = idx;
                    break;
                }

                if (fallbackIndex === -1)
                    fallbackIndex = idx;
            }

            if (chosenIndex === -1)
                chosenIndex = fallbackIndex;

            if (chosenIndex === -1)
                break;

            addCharAt(chosenIndex);
        }

        while (letters.length < length)
            letters.push("?");

        return letters.slice(0, length).join("");
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        Label {
            text: showingRepositories
                  ? (hasRepositoryRoot
                     ? qsTr("Repositories in %1").arg(repositoryRootPath)
                     : qsTr("Repositories"))
                  : qsTr("Subrepositories")
            font.bold: true
            Layout.fillWidth: true
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: showingRepositories ? repositoryModel.length > 0 : submoduleModel.length > 0
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

                    Repeater {
                        model: showingRepositories ? repositoryModel : submoduleModel

                        delegate: Item {
                            width: root.cardSize
                            height: root.cardSize

                            readonly property string repoName: modelData.name || modelData.path || modelData.raw || ""
                            readonly property string displayName: root.leafName(repoName)
                            readonly property color baseColor: root.colorFromString(displayName || repoName)
                            readonly property string abbrev: root.abbreviationForName(displayName || repoName, 5)

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
                                        font.pixelSize: 28
                                        color: root.textColorForBackground(baseColor, false)
                                        horizontalAlignment: Text.AlignHCenter
                                        verticalAlignment: Text.AlignVCenter
                                        width: parent.width
                                    }

                                    Label {
                                        text: displayName
                                        font.pixelSize: 12
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
                                    acceptedButtons: root.showingRepositories ? Qt.LeftButton : Qt.NoButton
                                    id: hoverArea
                                    onClicked: {
                                        if (!root.showingRepositories)
                                            return
                                        var candidate = modelData.path || modelData.raw || ""
                                        if (!candidate || candidate.length === 0)
                                            return
                                        root.repositoryActivated(candidate)
                                    }
                                }

                                ToolTip.visible: hoverArea.containsMouse
                                ToolTip.text: {
                                    var parts = [];
                                    if (displayName)
                                        parts.push(displayName);
                                    if (repoName && repoName !== displayName)
                                        parts.push(repoName);
                                    if (modelData.details)
                                        parts.push(modelData.details);
                                    if (modelData.commit)
                                        parts.push(qsTr("Commit %1").arg(modelData.commit));
                                    if (root.showingRepositories)
                                        parts.push(qsTr("Click to open repository"));
                                    return parts.join("\n");
                                }
                            }
                        }
                    }
                }
            }
        }

        Label {
            visible: showingRepositories ? repositoryModel.length === 0 : submoduleModel.length === 0
            text: showingRepositories
                  ? (hasRepositoryRoot
                     ? qsTr("No repositories found in %1").arg(repositoryRootPath)
                     : qsTr("Choose a workspace folder to list repositories"))
                  : qsTr("No submodules detected")
            horizontalAlignment: Text.AlignHCenter
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            opacity: 0.6
            wrapMode: Text.Wrap
        }
    }
}
