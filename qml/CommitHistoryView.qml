import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Frame {
    id: root
    property alias model: historyList.model
    property var branches: []
    property string currentBranch: ""
    property color mainlineColor: Qt.rgba(0.17, 0.48, 0.9, 1)
    property color branchColor: Qt.rgba(0.54, 0.59, 0.63, 1)
    property color evenRowColor: Qt.rgba(0.96, 0.97, 0.98, 1)
    property color oddRowColor: Qt.rgba(1, 1, 1, 1)
    property real laneSpacing: 28
    property var expandedGroups: ({})
    property real graphColumnWidth: {
        const model = historyList ? historyList.model : null
        const maxOffset = model && model.maxLaneOffset !== undefined ? model.maxLaneOffset : 0
        const laneCount = Math.max(1, maxOffset * 2 + 1)
        return Math.max(240, laneCount * root.laneSpacing + 40)
    }

    signal branchSelected(string branch)

    padding: 12

    function isGroupExpanded(key) {
        return expandedGroups && expandedGroups[key];
    }

    function toggleGroup(key) {
        if (!expandedGroups) {
            expandedGroups = {};
        }
        const current = expandedGroups[key];
        expandedGroups[key] = !current;
        expandedGroups = Object.assign({}, expandedGroups);
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        RowLayout {
            spacing: 12
            Layout.fillWidth: true

            Label {
                text: qsTr("Branch")
                font.bold: true
            }

            ComboBox {
                id: branchSelector
                Layout.preferredWidth: 220
                model: root.branches
                textRole: ""
                onActivated: (index) => {
                    if (index >= 0 && index < count) {
                        const name = textAt(index)
                        root.currentBranch = name
                        root.branchSelected(name)
                    }
                }

                Component.onCompleted: {
                    const idx = root.branches.indexOf(root.currentBranch)
                    if (idx >= 0) {
                        currentIndex = idx
                    }
                }

                onModelChanged: {
                    const idx = root.branches.indexOf(root.currentBranch)
                    if (idx >= 0) {
                        currentIndex = idx
                    }
                }
            }

            Item {
                Layout.fillWidth: true
            }
        }

        ListView {
            id: historyList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            spacing: 0
            cacheBuffer: 480
            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AlwaysOn
            }

            delegate: Item {
                id: delegateRoot
                width: ListView.view.width
                property bool groupExpanded: root.isGroupExpanded(groupKey)
                property bool collapsible: groupSize > 1
                property bool header: collapsible && groupIndex === 0
                property bool collapsedMember: collapsible && groupIndex > 0 && !groupExpanded
                property var lanesBeforeData: lanesBefore
                property var lanesAfterData: lanesAfter
                property var connectionsData: connections
                property var incomingConnectionsData: incomingConnections
                property int laneValue: lane
                property bool mainlineState: mainline
                property var branchNamesData: branchNames
                property real commitRadius: 6
                property real commitCenterX: graphContainer.width / 2 + laneValue * root.laneSpacing
                property string branchLabel: {
                    if (!branchNamesData || branchNamesData.length === 0)
                        return ""
                    return branchNamesData.join(", ")
                }

                visible: !collapsedMember
                implicitHeight: collapsedMember ? 0 : Math.max(graphContainer.implicitHeight, 48)

                onLanesBeforeDataChanged: graphCanvas.requestPaint()
                onLanesAfterDataChanged: graphCanvas.requestPaint()
                onConnectionsDataChanged: graphCanvas.requestPaint()
                onIncomingConnectionsDataChanged: graphCanvas.requestPaint()
                onLaneValueChanged: graphCanvas.requestPaint()
                onMainlineStateChanged: graphCanvas.requestPaint()

                ToolTip.visible: {
                    if (!commitHover.hovered || delegateRoot.branchLabel.length === 0)
                        return false
                    var pos = commitHover.point.position
                    var dx = pos.x - delegateRoot.commitCenterX
                    var dy = pos.y - graphContainer.height / 2
                    return dx * dx + dy * dy <= delegateRoot.commitRadius * delegateRoot.commitRadius
                }
                ToolTip.text: delegateRoot.branchLabel
                ToolTip.delay: 200

                Rectangle {
                    id: contentItem
                    anchors.fill: parent
                    color: index % 2 === 0 ? root.evenRowColor : root.oddRowColor
                    implicitHeight: graphContainer.implicitHeight

                        Item {
                            anchors.fill: parent
                            anchors.topMargin: 0
                            anchors.bottomMargin: 0
                            anchors.leftMargin: 8
                            anchors.rightMargin: 8

                        Item {
                            id: graphContainer
                            width: root.graphColumnWidth
                            implicitHeight: Math.max(Math.max(textColumn.implicitHeight, leftColumn.implicitHeight), 48)
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.horizontalCenter: parent.horizontalCenter

                            Canvas {
                                id: graphCanvas
                                anchors.fill: parent
                                antialiasing: true
                                onPaint: {
                                    const ctx = getContext("2d")
                                    ctx.reset()
                                    ctx.clearRect(0, 0, width, height)

                                    function laneToX(value) {
                                        return width / 2 + value * root.laneSpacing
                                    }

                                    const top = 0
                                    const bottom = height
                                    const mid = height / 2
                                    const before = lanesBeforeData || []
                                    const after = lanesAfterData || []
                                    const edges = connectionsData || []
                                    const incoming = incomingConnectionsData || []

                                    ctx.lineWidth = 2

                                    for (let i = 0; i < incoming.length; ++i) {
                                        const edge = incoming[i]
                                        const fromX = laneToX(edge.from)
                                        const toX = laneToX(edge.to)
                                        const isMainlineEdge = edge.from === 0 && edge.to === 0
                                        ctx.strokeStyle = isMainlineEdge ? root.mainlineColor : root.branchColor
                                        ctx.lineWidth = isMainlineEdge ? 3 : 2
                                        ctx.beginPath()
                                        if (edge.from === edge.to) {
                                            ctx.moveTo(fromX, top)
                                            ctx.lineTo(toX, mid)
                                        } else {
                                            const controlOffset = Math.min(Math.abs(toX - fromX) * 0.5, root.laneSpacing * 2)
                                            ctx.moveTo(fromX, top)
                                            ctx.bezierCurveTo(fromX, mid - controlOffset, toX, mid - controlOffset, toX, mid)
                                        }
                                        ctx.stroke()
                                    }

                                    for (let i = 0; i < before.length; ++i) {
                                        const laneId = before[i]
                                        const x = laneToX(laneId)
                                        const isMainlineLane = laneId === 0
                                        ctx.strokeStyle = isMainlineLane ? root.mainlineColor : root.branchColor
                                        ctx.lineWidth = isMainlineLane ? 3 : 2
                                        ctx.beginPath()
                                        ctx.moveTo(x, top)
                                        ctx.lineTo(x, mid)
                                        ctx.stroke()
                                    }

                                    for (let i = 0; i < after.length; ++i) {
                                        const laneId = after[i]
                                        const x = laneToX(laneId)
                                        const isMainlineLane = laneId === 0
                                        ctx.strokeStyle = isMainlineLane ? root.mainlineColor : root.branchColor
                                        ctx.lineWidth = isMainlineLane ? 3 : 2
                                        ctx.beginPath()
                                        ctx.moveTo(x, mid)
                                        ctx.lineTo(x, bottom)
                                        ctx.stroke()
                                    }

                                    for (let i = 0; i < edges.length; ++i) {
                                        const edge = edges[i]
                                        const fromX = laneToX(edge.from)
                                        const toX = laneToX(edge.to)
                                        const isMainlineEdge = edge.from === 0 && edge.to === 0
                                        ctx.strokeStyle = isMainlineEdge ? root.mainlineColor : root.branchColor
                                        ctx.lineWidth = isMainlineEdge ? 3 : 2
                                        ctx.beginPath()
                                        if (edge.from === edge.to) {
                                            ctx.moveTo(fromX, mid)
                                            ctx.lineTo(toX, bottom)
                                        } else {
                                            const controlOffset = Math.min(Math.abs(toX - fromX) * 0.5, root.laneSpacing * 2)
                                            ctx.moveTo(fromX, mid)
                                            ctx.bezierCurveTo(fromX, mid + controlOffset, toX, mid + controlOffset, toX, bottom)
                                        }
                                        ctx.stroke()
                                    }

                                    const laneX = laneToX(laneValue)
                                    ctx.fillStyle = delegateRoot.mainlineState ? root.mainlineColor : root.branchColor
                                    ctx.beginPath()
                                    ctx.arc(laneX, mid, 6, 0, Math.PI * 2)
                                    ctx.fill()
                                    ctx.lineWidth = 2
                                    ctx.strokeStyle = "white"
                                    ctx.stroke()
                                }
                                onWidthChanged: requestPaint()
                                onHeightChanged: requestPaint()
                            }

                            HoverHandler {
                                id: commitHover
                                target: graphContainer
                                acceptedDevices: PointerDevice.Mouse | PointerDevice.Stylus
                                cursorShape: {
                                    if (!hovered)
                                        return Qt.ArrowCursor
                                    var pos = point.position
                                    var dx = pos.x - delegateRoot.commitCenterX
                                    var dy = pos.y - graphContainer.height / 2
                                    return (dx * dx + dy * dy) <= delegateRoot.commitRadius * delegateRoot.commitRadius
                                            ? Qt.PointingHandCursor
                                            : Qt.ArrowCursor
                                }
                            }
                    }

                        Item {
                            id: leftContainer
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            anchors.left: parent.left
                            anchors.right: graphContainer.left
                            anchors.rightMargin: 16
                            visible: laneValue < 0
                            implicitHeight: graphCanvas.implicitHeight

                            ColumnLayout {
                                id: leftColumn
                                anchors.fill: parent
                                anchors.right: parent.right
                                spacing: 6

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 6

                                    ToolButton {
                                        visible: header
                                        text: header && groupExpanded ? "−" : "+"
                                        font.bold: true
                                        onClicked: root.toggleGroup(groupKey)
                                    }

                                    Text {
                                        id: leftSummaryLabel
                                        Layout.fillWidth: true
                                        text: laneValue < 0 ? leftSummary : summary
                                        textFormat: Text.PlainText
                                        wrapMode: Text.Wrap
                                        horizontalAlignment: Text.AlignRight
                                        font.pointSize: 11
                                    }
                                }

                                Text {
                                    id: leftMeta
                                    Layout.fillWidth: true
                                    text: laneValue < 0 ? `${author} • ${relativeTime} • ${shortOid}` : ""
                                    font.pointSize: 9
                                    color: Qt.rgba(0.5, 0.55, 0.6, 1)
                                    horizontalAlignment: Text.AlignRight
                                    wrapMode: Text.NoWrap
                                }
                            }
                        }

                        Item {
                            id: rightContainer
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            anchors.left: graphContainer.right
                            anchors.leftMargin: 16
                            anchors.right: parent.right
                            visible: laneValue >= 0
                            implicitHeight: graphCanvas.implicitHeight

                            ColumnLayout {
                                id: textColumn
                                anchors.fill: parent
                                spacing: 6

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 6

                                    ToolButton {
                                        visible: header
                                        text: header && groupExpanded ? "−" : "+"
                                        font.bold: true
                                        onClicked: root.toggleGroup(groupKey)
                                    }

                                    Text {
                                        id: rightSummary
                                        Layout.fillWidth: true
                                        text: summary
                                        textFormat: Text.PlainText
                                        wrapMode: Text.Wrap
                                        font.pointSize: 11
                                    }
                                }

                                Text {
                                    id: rightMeta
                                    Layout.fillWidth: true
                                    text: `${author} • ${relativeTime} • ${shortOid}`
                                    font.pointSize: 9
                                    color: Qt.rgba(0.5, 0.55, 0.6, 1)
                                    wrapMode: Text.NoWrap
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Connections {
        target: historyList.model
        function onModelReset() {
            root.expandedGroups = ({})
        }
    }

    onCurrentBranchChanged: {
        const idx = branches.indexOf(currentBranch)
        if (idx >= 0) {
            branchSelector.currentIndex = idx
        }
        expandedGroups = ({})
    }

    onBranchesChanged: {
        const idx = branches.indexOf(currentBranch)
        if (idx >= 0) {
            branchSelector.currentIndex = idx
        }
    }
}

