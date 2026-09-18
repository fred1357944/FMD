import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Column {
    id: root
    width: parent ? parent.width : implicitWidth
    spacing: 10

    property string filter: ""
    property string capturingId: ""

    readonly property var rows: {
        var _ = backend.hotkeys
        var all = backend.hotkeyRows
        var q = filter.trim().toLowerCase()
        if (q.length === 0)
            return all
        var out = []
        for (var i = 0; i < all.length; ++i) {
            var row = all[i]
            if (String(row.title).toLowerCase().indexOf(q) >= 0
                    || String(row.id).toLowerCase().indexOf(q) >= 0
                    || String(row.display).toLowerCase().indexOf(q) >= 0)
                out.push(row)
        }
        return out
    }

    Label {
        width: parent.width
        text: (backend.uiLanguage, backend.t("hotkeys"))
        color: win.strongTextColor
        font.family: "iA Writer Mono S"
        font.pixelSize: win.scaledSize(13)
        font.bold: true
    }

    Label {
        width: parent.width
        wrapMode: Text.Wrap
        text: (backend.uiLanguage, backend.t("hotkeysHelp"))
        color: win.mutedColor
        font.family: "iA Writer Mono S"
        font.pixelSize: win.scaledSize(11)
    }

    TextField {
        id: hotkeySearch
        objectName: "hotkeySearchField"
        width: parent.width
        placeholderText: (backend.uiLanguage, backend.t("searchHotkeys"))
        font.family: "iA Writer Mono S"
        font.pixelSize: win.scaledSize(12)
        onTextChanged: root.filter = text
    }

    Repeater {
        model: root.rows
        delegate: RowLayout {
            required property var modelData
            width: root.width
            spacing: 8

            Label {
                Layout.fillWidth: true
                text: modelData.title
                color: modelData.conflict ? "#f87171" : win.strongTextColor
                wrapMode: Text.Wrap
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(12)
            }

            Rectangle {
                id: chord
                Layout.preferredHeight: win.scaledSize(26)
                Layout.preferredWidth: Math.max(win.scaledSize(72), chipLabel.implicitWidth + 16)
                radius: 13
                color: root.capturingId === modelData.id
                       ? backend.themeAccent
                       : (win.darkMode ? "#2a2a2a" : "#ececec")
                border.color: modelData.conflict ? "#f87171" : win.panelBorderColor
                Label {
                    id: chipLabel
                    anchors.centerIn: parent
                    text: root.capturingId === modelData.id
                          ? (backend.uiLanguage, backend.t("pressKey"))
                          : modelData.display
                    color: root.capturingId === modelData.id ? "#ffffff" : win.textColor
                    font.family: "iA Writer Mono S"
                    font.pixelSize: win.scaledSize(11)
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        root.capturingId = modelData.id
                        captureFocus.forceActiveFocus()
                    }
                }
            }

            Label {
                visible: modelData.sequence.length > 0
                text: "×"
                color: win.mutedColor
                font.pixelSize: win.scaledSize(14)
                MouseArea {
                    anchors.fill: parent
                    anchors.margins: -6
                    onClicked: backend.clearHotkey(modelData.id)
                }
            }

            Label {
                visible: !modelData.isDefault
                text: "reset"
                color: win.mutedColor
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(10)
                MouseArea {
                    anchors.fill: parent
                    anchors.margins: -6
                    onClicked: backend.resetHotkey(modelData.id)
                }
            }
        }
    }

    Item {
        id: captureFocus
        width: 1
        height: 1
        focus: root.capturingId.length > 0
        Keys.onPressed: function(event) {
            if (root.capturingId.length === 0)
                return
            if (event.key === Qt.Key_Escape) {
                root.capturingId = ""
                event.accepted = true
                return
            }
            var seq = backend.captureHotkey(event.key, event.modifiers)
            if (seq.length === 0) {
                event.accepted = true
                return
            }
            backend.setHotkey(root.capturingId, seq)
            root.capturingId = ""
            event.accepted = true
        }
    }
}
