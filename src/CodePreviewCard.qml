import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    objectName: "previewCodeCard"
    property var block: ({})
    readonly property string language: (block.language || "plaintext")
    readonly property string code: block.text || ""
    readonly property bool mermaid: block.mermaid === true || language === "mermaid"
    property string mode: mermaid ? "split" : "code"
    readonly property bool night: backend.previewTheme === "night"

    width: parent ? parent.width : implicitWidth
    implicitHeight: cardColumn.implicitHeight
    radius: 10
    color: night ? "#1c1a18" : "#f3efe8"
    border.color: night ? "#2f2c28" : "#e4dfd6"
    border.width: 1
    clip: true

    Column {
        id: cardColumn
        width: parent.width
        spacing: 0

        Item {
            width: parent.width
            height: win.scaledSize(34)

            Label {
                anchors.left: parent.left
                anchors.leftMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                text: root.language
                color: backend.previewMuted
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(11)
            }

            Row {
                visible: root.mermaid
                anchors.centerIn: parent
                spacing: 2

                Repeater {
                    model: [
                        { "id": "split", "title": "Split" },
                        { "id": "code", "title": "Code" },
                        { "id": "preview", "title": "Preview" }
                    ]
                    delegate: Rectangle {
                        required property var modelData
                        width: tabLabel.implicitWidth + 12
                        height: win.scaledSize(22)
                        radius: 5
                        color: root.mode === modelData.id
                               ? (root.night ? "#2a2723" : "#e7e1d6")
                               : "transparent"
                        Label {
                            id: tabLabel
                            anchors.centerIn: parent
                            text: modelData.title
                            color: root.mode === modelData.id
                                   ? backend.previewForeground : backend.previewMuted
                            font.family: "iA Writer Mono S"
                            font.pixelSize: win.scaledSize(10)
                        }
                        MouseArea {
                            anchors.fill: parent
                            onClicked: root.mode = modelData.id
                        }
                    }
                }
            }

            PaneButton {
                objectName: "copyCodeButton"
                anchors.right: parent.right
                anchors.rightMargin: 6
                anchors.verticalCenter: parent.verticalCenter
                text: "Copy"
                implicitHeight: win.scaledSize(24)
                font.pixelSize: win.scaledSize(10)
                onClicked: backend.copyText(root.code)
            }
        }

        Rectangle {
            width: parent.width
            height: 1
            color: root.night ? "#2f2c28" : "#e4dfd6"
        }

        Row {
            width: parent.width
            spacing: 0
            visible: !root.mermaid || root.mode === "split"

            CodePane {
                width: root.mermaid && root.mode === "split" ? parent.width / 2 : parent.width
                visible: !root.mermaid || root.mode === "split" || root.mode === "code"
                code: root.code
            }

            Rectangle {
                visible: root.mermaid && root.mode === "split"
                width: 1
                height: parent.height
                color: root.night ? "#2f2c28" : "#e4dfd6"
            }

            MermaidPane {
                visible: root.mermaid && root.mode === "split"
                width: parent.width / 2
                code: root.code
            }
        }

        CodePane {
            width: parent.width
            visible: root.mermaid && root.mode === "code"
            code: root.code
        }

        MermaidPane {
            width: parent.width
            visible: root.mermaid && root.mode === "preview"
            code: root.code
        }
    }

    component CodePane: TextEdit {
        required property string code
        text: code
        readOnly: true
        selectByMouse: true
        wrapMode: TextEdit.Wrap
        color: backend.previewForeground
        selectedTextColor: win.strongTextColor
        selectionColor: win.selectionFill
        font.family: "iA Writer Mono S"
        font.pixelSize: win.scaledSize(13)
        leftPadding: 12
        rightPadding: 12
        topPadding: 10
        bottomPadding: 12
        height: implicitHeight
    }

    component MermaidPane: Rectangle {
        required property string code
        color: "transparent"
        implicitHeight: mermaidColumn.implicitHeight + 16
        height: implicitHeight

        Column {
            id: mermaidColumn
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 12
            spacing: 8

            Label {
                width: parent.width
                text: "Preview"
                color: backend.previewMuted
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(10)
            }

            TextEdit {
                width: parent.width
                text: code
                readOnly: true
                selectByMouse: true
                wrapMode: TextEdit.Wrap
                color: backend.previewForeground
                selectedTextColor: win.strongTextColor
                selectionColor: win.selectionFill
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(13)
            }
        }
    }
}
