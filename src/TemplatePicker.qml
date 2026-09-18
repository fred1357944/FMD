import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: root
    objectName: "templatePicker"
    padding: 8
    modal: false
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property string mode: "insert"
    property int replaceStart: 0
    property int replaceEnd: 0
    property int currentIndex: 0
    property string query: ""
    readonly property var filtered: {
        var needle = query.trim().toLowerCase()
        var files = backend.templateFiles || []
        if (needle.length === 0)
            return files
        var out = []
        for (var i = 0; i < files.length; ++i) {
            var item = files[i]
            var name = String(item.name || "")
            var title = String(item.title || "")
            if (name.toLowerCase().indexOf(needle) >= 0
                    || title.toLowerCase().indexOf(needle) >= 0)
                out.push(item)
        }
        return out
    }

    signal chosen(url templateUrl)

    function openAt(point) {
        query = ""
        queryField.text = ""
        currentIndex = 0
        x = Math.max(12, Math.min(point.x, win.width - width - 12))
        y = Math.max(12, Math.min(point.y, win.height - implicitHeight - 12))
        open()
        queryField.forceActiveFocus()
    }

    function runCurrent() {
        if (filtered.length === 0)
            return
        var item = filtered[Math.max(0, Math.min(currentIndex, filtered.length - 1))]
        close()
        if (item && item.url)
            chosen(item.url)
    }

    width: win.scaledSize(280)
    background: Rectangle {
        color: win.darkMode ? "#1a1a1a" : "#ffffff"
        border.color: win.panelBorderColor
        radius: 10
    }

    contentItem: ColumnLayout {
        spacing: 6

        Label {
            Layout.fillWidth: true
            text: root.mode === "create"
                  ? (backend.uiLanguage, backend.t("newFromTemplate"))
                  : (backend.uiLanguage, backend.t("insertTemplate"))
            color: win.mutedColor
            font.family: "iA Writer Mono S"
            font.pixelSize: win.scaledSize(11)
        }

        TextField {
            id: queryField
            objectName: "templatePickerQuery"
            Layout.fillWidth: true
            placeholderText: (backend.uiLanguage, backend.t("template"))
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
            objectName: "templatePickerResults"
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(win.scaledSize(240),
                                             Math.max(win.scaledSize(36), count * win.scaledSize(36)))
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
                contentItem: Label {
                    text: modelData.title || modelData.name
                    color: win.strongTextColor
                    font.family: "iA Writer Mono S"
                    font.pixelSize: win.scaledSize(12)
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideMiddle
                }
            }
        }

        Label {
            visible: results.count === 0
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: (backend.uiLanguage, backend.t("noTemplates"))
            color: win.mutedColor
            font.family: "iA Writer Mono S"
            font.pixelSize: win.scaledSize(12)
        }
    }
}
