import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    objectName: "outlinePane"

    property var headings: []
    property int cursorPosition: 0
    property int maxLevel: 6

    signal jumpTo(int position)
    signal maxLevelChosen(int level)

    property string query: ""
    property var collapsedKeys: ({})
    property int collapseRev: 0

    function headingKey(heading) {
        return String(heading.level) + ":" + heading.title
    }

    function toggleCollapsed(heading) {
        var key = headingKey(heading)
        var next = Object.assign({}, collapsedKeys)
        if (next[key])
            delete next[key]
        else
            next[key] = true
        collapsedKeys = next
        collapseRev += 1
    }

    function focusSearch() {
        searchField.forceActiveFocus()
        searchField.selectAll()
    }

    readonly property int activePosition: {
        var best = -1
        for (var i = 0; i < headings.length; ++i) {
            if (headings[i].position <= cursorPosition)
                best = headings[i].position
        }
        return best
    }

    readonly property var visibleHeadings: {
        var _ = collapseRev
        var src = headings
        var needle = query.trim().toLowerCase()
        var searching = needle.length > 0
        var out = []
        var hideBelow = 0
        for (var i = 0; i < src.length; ++i) {
            var heading = src[i]
            if (hideBelow > 0) {
                if (heading.level > hideBelow)
                    continue
                hideBelow = 0
            }
            if (!searching && heading.level > maxLevel)
                continue
            if (searching && heading.title.toLowerCase().indexOf(needle) < 0)
                continue
            var hasChild = false
            for (var j = i + 1; j < src.length; ++j) {
                if (src[j].level > heading.level) {
                    hasChild = true
                    break
                }
                if (src[j].level <= heading.level)
                    break
            }
            var collapsed = !!collapsedKeys[headingKey(heading)]
            out.push({
                "level": heading.level,
                "title": heading.title,
                "position": heading.position,
                "hasChild": hasChild,
                "collapsed": collapsed
            })
            if (!searching && collapsed && hasChild)
                hideBelow = heading.level
        }
        return out
    }

    onActivePositionChanged: {
        for (var i = 0; i < visibleHeadings.length; ++i) {
            if (visibleHeadings[i].position === activePosition) {
                headingList.positionViewAtIndex(i, ListView.Contain)
                break
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 6

        TextField {
            id: searchField
            objectName: "outlineSearchField"
            Layout.fillWidth: true
            placeholderText: "Search headings"
            font.family: "iA Writer Mono S"
            font.pixelSize: win.scaledSize(11)
            color: win.strongTextColor
            onTextChanged: root.query = text
            Keys.onEscapePressed: {
                text = ""
                root.query = ""
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 2
            Repeater {
                model: 6
                ToolButton {
                    required property int index
                    Layout.fillWidth: true
                    implicitHeight: win.scaledSize(26)
                    text: String(index + 1)
                    checkable: true
                    checked: root.maxLevel === index + 1
                    font.family: "iA Writer Mono S"
                    font.pixelSize: win.scaledSize(11)
                    font.bold: checked
                    ToolTip.visible: hovered
                    ToolTip.text: "Show headings through H" + (index + 1)
                    onClicked: root.maxLevelChosen(index + 1)
                }
            }
        }

        ListView {
            id: headingList
            objectName: "outlineSidebar"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            visible: count > 0
            model: root.visibleHeadings
            spacing: 1
            currentIndex: -1
            highlightFollowsCurrentItem: false
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            delegate: Item {
                id: row
                required property var modelData
                width: ListView.view ? ListView.view.width : parent.width
                height: win.scaledSize(28)

                readonly property bool current: modelData.position === root.activePosition

                Rectangle {
                    anchors.fill: parent
                    radius: 6
                    color: row.current ? win.currentFileColor
                          : (hover.hovered ? Qt.rgba(1, 1, 1, 0.04) : "transparent")
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 4 + (modelData.level - 1) * 10
                    anchors.rightMargin: 6
                    spacing: 2

                    ToolButton {
                        visible: modelData.hasChild
                        text: modelData.collapsed ? "▸" : "▾"
                        implicitWidth: win.scaledSize(22)
                        implicitHeight: win.scaledSize(22)
                        font.pixelSize: win.scaledSize(10)
                        onClicked: root.toggleCollapsed(modelData)
                    }
                    Item {
                        visible: !modelData.hasChild
                        implicitWidth: win.scaledSize(22)
                    }

                    Label {
                        Layout.fillWidth: true
                        text: modelData.title
                        elide: Text.ElideRight
                        color: win.strongTextColor
                        font.family: "iA Writer Mono S"
                        font.pixelSize: win.scaledSize(12)
                        font.bold: modelData.level <= 2
                        TapHandler {
                            onTapped: root.jumpTo(modelData.position)
                        }
                    }
                }

                HoverHandler { id: hover }
            }
        }

        Label {
            Layout.fillWidth: true
            Layout.fillHeight: true
            wrapMode: Text.Wrap
            visible: headingList.count === 0
            color: win.mutedColor
            font.family: "iA Writer Mono S"
            font.pixelSize: win.scaledSize(12)
            text: headings.length === 0
                  ? "Headings in the current note appear here. Click one to jump."
                  : (query.length > 0 ? "No headings match that search."
                                      : "No headings at this level. Raise the level with 1–6.")
        }
    }
}
