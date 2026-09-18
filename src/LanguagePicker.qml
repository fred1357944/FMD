import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: root
    objectName: "languagePicker"
    padding: 8
    modal: false
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property int replaceStart: 0
    property int replaceEnd: 0
    property bool wrapSelected: false
    property int currentIndex: 0
    property string query: ""
    readonly property var filtered: backend.codeLanguages(query)

    signal picked(string language)

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
        var language = query.trim()
        if (filtered.length > 0) {
            var item = filtered[Math.max(0, Math.min(currentIndex, filtered.length - 1))]
            language = item.id || language
        }
        if (language.length === 0)
            language = "plaintext"
        close()
        picked(language)
    }

    width: win.scaledSize(240)
    background: Rectangle {
        color: win.darkMode ? "#1a1a1a" : "#ffffff"
        border.color: win.panelBorderColor
        radius: 10
    }

    contentItem: ColumnLayout {
        spacing: 6

        TextField {
            id: queryField
            objectName: "languagePickerQuery"
            Layout.fillWidth: true
            placeholderText: "Search language"
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
            objectName: "languagePickerResults"
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(win.scaledSize(280),
                                             Math.max(win.scaledSize(36), count * win.scaledSize(32)))
            clip: true
            model: root.filtered
            currentIndex: root.currentIndex
            boundsBehavior: Flickable.StopAtBounds
            delegate: ItemDelegate {
                required property var modelData
                required property int index
                width: ListView.view.width
                implicitHeight: win.scaledSize(32)
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
                    text: modelData.title
                    color: win.strongTextColor
                    font.family: "iA Writer Mono S"
                    font.pixelSize: win.scaledSize(12)
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }

        Label {
            visible: results.count === 0
            Layout.fillWidth: true
            text: query.length > 0 ? "Use “" + query.trim() + "”" : "No languages"
            color: win.mutedColor
            font.family: "iA Writer Mono S"
            font.pixelSize: win.scaledSize(12)
        }
    }
}
