import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    objectName: "threadsPreview"

    readonly property string displayName: {
        var name = backend.threadsDisplayName
        return name.length > 0 ? name : "You"
    }
    readonly property string handle: backend.threadsHandle
    readonly property string whenText: {
        var fields = backend.previewProperties
        var scheduled = ""
        var date = ""
        var status = ""
        for (var i = 0; i < fields.length; ++i) {
            if (fields[i].key === "scheduled")
                scheduled = fields[i].value
            else if (fields[i].key === "date")
                date = fields[i].value
            else if (fields[i].key === "status")
                status = fields[i].value
        }
        var bits = []
        if (status.length > 0)
            bits.push(status)
        if (scheduled.length > 0)
            bits.push(scheduled)
        else if (date.length > 0)
            bits.push(date)
        return bits.join(" · ")
    }

    Rectangle {
        anchors.fill: parent
        color: "#000000"
    }

    Flickable {
        id: feedFlick
        anchors.fill: parent
        clip: true
        contentWidth: width
        contentHeight: feedColumn.implicitHeight + 32
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        Column {
            id: feedColumn
            x: 16
            width: Math.max(120, feedFlick.width - 32)
            y: 16
            spacing: 0

            Label {
                visible: root.whenText.length > 0
                width: parent.width
                text: root.whenText
                color: "#8a8a8a"
                font.family: "PingFang TC"
                font.pixelSize: win.scaledSize(11)
                bottomPadding: 12
            }

            Repeater {
                model: backend.threadPosts
                Column {
                    required property var modelData
                    width: feedColumn.width
                    spacing: 0

                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#1f1f1f"
                        visible: modelData.index > 0
                    }

                    Row {
                        width: parent.width
                        spacing: 10
                        topPadding: 14
                        bottomPadding: 14

                        Rectangle {
                            width: win.scaledSize(36)
                            height: win.scaledSize(36)
                            radius: width / 2
                            color: "#2a2a2a"
                            Label {
                                anchors.centerIn: parent
                                text: root.displayName.charAt(0)
                                color: "#f2f2f2"
                                font.family: "PingFang TC"
                                font.pixelSize: win.scaledSize(14)
                                font.bold: true
                            }
                        }

                        Column {
                            width: parent.width - win.scaledSize(46)
                            spacing: 4

                            RowLayout {
                                width: parent.width
                                spacing: 6
                                Label {
                                    text: root.displayName
                                    color: "#f2f2f2"
                                    font.family: "PingFang TC"
                                    font.pixelSize: win.scaledSize(13)
                                    font.bold: true
                                    elide: Text.ElideRight
                                }
                                Label {
                                    visible: root.handle.length > 0
                                    text: "@" + root.handle
                                    color: "#8a8a8a"
                                    font.family: "PingFang TC"
                                    font.pixelSize: win.scaledSize(13)
                                    elide: Text.ElideRight
                                }
                                Item { Layout.fillWidth: true }
                                Label {
                                    visible: modelData.total > 1
                                    text: (modelData.index + 1) + "/" + modelData.total
                                    color: "#8a8a8a"
                                    font.family: "iA Writer Mono S"
                                    font.pixelSize: win.scaledSize(11)
                                }
                            }

                            Column {
                                width: parent.width
                                spacing: 10
                                visible: modelData.text.length > 0

                                Repeater {
                                    model: modelData.blocks && modelData.blocks.length
                                           ? modelData.blocks
                                           : [{ "kind": "text", "text": modelData.text }]

                                    Column {
                                        required property var modelData
                                        width: parent.width
                                        spacing: 0

                                        Label {
                                            visible: modelData.kind !== "table"
                                            width: parent.width
                                            text: modelData.text || ""
                                            color: "#f2f2f2"
                                            wrapMode: Text.Wrap
                                            font.family: "PingFang TC"
                                            font.pixelSize: win.scaledSize(15)
                                            lineHeight: 1.35
                                        }

                                        GridLayout {
                                            visible: modelData.kind === "table"
                                            width: parent.width
                                            columns: Math.max(1, modelData.columns || 1)
                                            columnSpacing: 0
                                            rowSpacing: 0

                                            Repeater {
                                                model: modelData.cells || []
                                                Rectangle {
                                                    required property var modelData
                                                    Layout.fillWidth: true
                                                    color: modelData.header ? "#1c1c1c" : "#111111"
                                                    border.color: "#3a3a3a"
                                                    border.width: 1
                                                    implicitHeight: cellLabel.implicitHeight + 16
                                                    Label {
                                                        id: cellLabel
                                                        anchors.fill: parent
                                                        anchors.margins: 8
                                                        text: modelData.text && modelData.text.length
                                                              ? modelData.text : " "
                                                        color: "#f2f2f2"
                                                        wrapMode: Text.Wrap
                                                        font.family: "PingFang TC"
                                                        font.pixelSize: win.scaledSize(13)
                                                        font.bold: !!modelData.header
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }

                            Label {
                                visible: modelData.text.length === 0
                                width: parent.width
                                text: modelData.role === "post" ? "(main post)" : "(reply)"
                                color: "#6a6a6a"
                                font.family: "PingFang TC"
                                font.pixelSize: win.scaledSize(15)
                            }

                            Label {
                                text: modelData.chars + " / 500"
                                color: modelData.overLimit ? "#f87171" : "#6a6a6a"
                                font.family: "iA Writer Mono S"
                                font.pixelSize: win.scaledSize(11)
                            }
                        }
                    }
                }
            }

            Label {
                visible: backend.threadPosts.length === 0
                width: parent.width
                wrapMode: Text.Wrap
                text: "Write the main post above the first ---. Each later --- starts a reply."
                color: "#8a8a8a"
                font.family: "PingFang TC"
                font.pixelSize: win.scaledSize(13)
            }
        }
    }
}
