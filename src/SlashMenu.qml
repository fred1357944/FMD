import QtQuick
import QtQuick.Controls

Popup {
    id: root
    objectName: "slashMenu"
    padding: 6
    modal: false
    focus: false
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property string query: ""
    property int replaceStart: 0
    property int replaceEnd: 0
    property int currentIndex: 0
    readonly property var filtered: backend.slashCommands(query)

    signal activated(var item)

    function openAt(point) {
        if (filtered.length === 0) {
            close()
            return
        }
        currentIndex = 0
        x = Math.max(12, Math.min(point.x, win.width - width - 12))
        y = Math.max(12, Math.min(point.y, win.height - implicitHeight - 12))
        open()
    }

    function move(delta) {
        if (filtered.length === 0)
            return
        currentIndex = Math.max(0, Math.min(currentIndex + delta, filtered.length - 1))
    }

    function runCurrent() {
        if (filtered.length === 0)
            return
        var item = filtered[Math.max(0, Math.min(currentIndex, filtered.length - 1))]
        close()
        activated(item)
    }

    width: win.scaledSize(280)
    background: Rectangle {
        color: win.darkMode ? "#1a1a1a" : "#ffffff"
        border.color: win.panelBorderColor
        radius: 10
    }

    contentItem: Column {
        spacing: 2
        width: root.width - 12

        Label {
            visible: root.query.length > 0
            width: parent.width
            text: "/" + root.query
            color: win.mutedColor
            font.family: "iA Writer Mono S"
            font.pixelSize: win.scaledSize(11)
            leftPadding: 8
            topPadding: 4
        }

        Repeater {
            model: root.filtered
            delegate: ItemDelegate {
                required property var modelData
                required property int index
                width: parent ? parent.width : root.width
                implicitHeight: win.scaledSize(40)
                highlighted: index === root.currentIndex
                onClicked: {
                    root.currentIndex = index
                    root.runCurrent()
                }
                background: Rectangle {
                    radius: 6
                    color: parent.highlighted ? win.currentFileColor : "transparent"
                }
                contentItem: Column {
                    spacing: 1
                    Label {
                        text: modelData.title
                        color: win.strongTextColor
                        font.family: "iA Writer Mono S"
                        font.pixelSize: win.scaledSize(13)
                    }
                    Label {
                        text: modelData.hint || ""
                        color: win.mutedColor
                        font.family: "iA Writer Mono S"
                        font.pixelSize: win.scaledSize(10)
                    }
                }
            }
        }

        Label {
            visible: root.filtered.length === 0
            width: parent.width
            text: "No matching blocks"
            color: win.mutedColor
            font.family: "iA Writer Mono S"
            font.pixelSize: win.scaledSize(12)
            padding: 8
        }
    }
}
