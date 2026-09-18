import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: root
    objectName: "commandPalette"
    modal: true
    focus: true
    padding: 10
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property var catalog: []
    property int currentIndex: 0
    property string query: ""
    readonly property var filtered: {
        var needle = query.trim().toLowerCase()
        var out = []
        for (var i = 0; i < catalog.length; ++i) {
            var item = catalog[i]
            var title = (item.title || "").toLowerCase()
            if (needle.length === 0) {
                if (item.kind !== "file")
                    out.push(item)
            } else if (title.indexOf(needle) !== -1) {
                out.push(item)
            }
            if (out.length >= 30)
                break
        }
        return out
    }

    signal activated(var item)

    function openPalette() {
        query = ""
        queryField.text = ""
        currentIndex = 0
        open()
        queryField.forceActiveFocus()
    }

    function runCurrent() {
        if (filtered.length === 0)
            return
        var index = Math.max(0, Math.min(currentIndex, filtered.length - 1))
        var item = filtered[index]
        close()
        activated(item)
    }

    width: Math.min(win.scaledSize(520), win.width - 48)
    x: Math.round((win.width - width) / 2)
    y: Math.round(win.height * 0.12)

    background: Rectangle {
        color: win.darkMode ? "#1a1a1a" : "#ffffff"
        border.color: win.panelBorderColor
        radius: 10
    }

    contentItem: ColumnLayout {
        spacing: 8

        TextField {
            id: queryField
            objectName: "commandPaletteQuery"
            Layout.fillWidth: true
            placeholderText: "Search commands and files"
            font.family: "iA Writer Mono S"
            font.pixelSize: win.scaledSize(13)
            color: win.strongTextColor
            onTextChanged: {
                root.query = text
                root.currentIndex = 0
            }
            Keys.onPressed: function(event) {
                if (event.key === Qt.Key_Down) {
                    if (root.filtered.length > 0)
                        root.currentIndex = Math.min(root.currentIndex + 1, root.filtered.length - 1)
                    event.accepted = true
                } else if (event.key === Qt.Key_Up) {
                    root.currentIndex = Math.max(0, root.currentIndex - 1)
                    event.accepted = true
                } else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                    root.runCurrent()
                    event.accepted = true
                }
            }
        }

        ListView {
            id: results
            objectName: "commandPaletteResults"
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(win.scaledSize(280),
                                             Math.max(win.scaledSize(44), count * win.scaledSize(36)))
            clip: true
            model: root.filtered
            currentIndex: root.currentIndex
            boundsBehavior: Flickable.StopAtBounds
            delegate: ItemDelegate {
                required property var modelData
                required property int index
                width: ListView.view.width
                implicitHeight: win.scaledSize(36)
                highlighted: index === root.currentIndex
                onClicked: {
                    root.currentIndex = index
                    root.runCurrent()
                }
                background: Rectangle {
                    radius: 6
                    color: parent.highlighted ? win.currentFileColor : "transparent"
                }
                contentItem: RowLayout {
                    spacing: 8
                    Label {
                        Layout.fillWidth: true
                        text: modelData.title
                        elide: Text.ElideMiddle
                        color: win.strongTextColor
                        font.family: "iA Writer Mono S"
                        font.pixelSize: win.scaledSize(12)
                    }
                    Label {
                        visible: (modelData.shortcut || "").length > 0
                        text: modelData.shortcut || ""
                        color: win.mutedColor
                        font.family: "iA Writer Mono S"
                        font.pixelSize: win.scaledSize(11)
                    }
                }
            }
        }

        Label {
            visible: results.count === 0
            Layout.fillWidth: true
            text: "No matching commands or files."
            color: win.mutedColor
            font.family: "iA Writer Mono S"
            font.pixelSize: win.scaledSize(12)
        }
    }
}
