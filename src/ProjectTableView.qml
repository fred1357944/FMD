import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    objectName: "projectTableView"

    readonly property var columns: backend.projectTableColumns
    readonly property string statusKey: backend.boardField
    readonly property string dateKey: backend.dateField

    function columnWidth(column) {
        if (column === "File")
            return Math.max(win.scaledSize(140), Math.round(root.width * 0.18))
        if (column === "Title")
            return Math.max(win.scaledSize(180), Math.round(root.width * 0.28))
        if (column === root.statusKey || column === root.dateKey)
            return win.scaledSize(140)
        return Math.max(win.scaledSize(110), Math.round(root.width * 0.14))
    }

    function cellText(record, column) {
        if (column === "File")
            return record.name
        if (column === "Title")
            return record.title
        return record.fields[column] || ""
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            color: win.mutedColor
            font.family: "iA Writer Mono S"
            font.pixelSize: win.scaledSize(12)
            text: "Manuscript table. Status and date edit the YAML front matter in each file — not a separate database."
        }

        Label {
            visible: backend.projectRecords.length === 0
            Layout.fillWidth: true
            Layout.fillHeight: true
            wrapMode: Text.Wrap
            color: win.mutedColor
            font.family: "iA Writer Mono S"
            font.pixelSize: win.scaledSize(13)
            text: backend.workspaceFolderPath.length > 0
                  ? "No Markdown notes in this folder.\n\nAdd YAML at the top of a file:\n---\nstatus: writing\ndate: 2026-09-16\n---"
                  : (backend.uiLanguage, backend.t("tableNeedsFolder"))
        }

        Flickable {
            visible: backend.projectRecords.length > 0
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: Math.max(width, tableGrid.implicitWidth)
            contentHeight: tableGrid.implicitHeight
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
            ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }

            Column {
                id: tableGrid
                spacing: 0

                Row {
                    Repeater {
                        model: root.columns
                        Label {
                            required property string modelData
                            width: root.columnWidth(modelData)
                            text: modelData
                            padding: 8
                            color: win.strongTextColor
                            font.family: "iA Writer Mono S"
                            font.pixelSize: win.scaledSize(11)
                            font.bold: true
                            elide: Text.ElideRight
                        }
                    }
                }

                Rectangle {
                    width: tableGrid.implicitWidth
                    height: 1
                    color: win.panelBorderColor
                }

                Repeater {
                    model: backend.projectRecords
                    delegate: Rectangle {
                        id: row
                        required property var modelData
                        readonly property bool current:
                            backend.fileUrl.toString() === modelData.url.toString()
                        width: tableGrid.implicitWidth
                        height: win.scaledSize(40)
                        color: row.current ? win.currentFileColor : "transparent"

                        Row {
                            anchors.fill: parent
                            Repeater {
                                model: root.columns
                                Item {
                                    id: cell
                                    required property string modelData
                                    width: root.columnWidth(modelData)
                                    height: parent.height

                                    readonly property bool isStatus: cell.modelData === root.statusKey
                                    readonly property bool isDate: cell.modelData === root.dateKey
                                    readonly property bool isOpen: cell.modelData === "File" || cell.modelData === "Title"

                                    Label {
                                        anchors.fill: parent
                                        visible: !cell.isStatus && !cell.isDate
                                        text: root.cellText(row.modelData, cell.modelData)
                                        padding: 8
                                        color: win.textColor
                                        font.family: "iA Writer Mono S"
                                        font.pixelSize: win.scaledSize(12)
                                        elide: Text.ElideRight
                                        verticalAlignment: Text.AlignVCenter
                                    }

                                    ComboBox {
                                        id: statusBox
                                        visible: cell.isStatus
                                        anchors.fill: parent
                                        anchors.margins: 4
                                        editable: true
                                        font.family: "iA Writer Mono S"
                                        font.pixelSize: win.scaledSize(11)
                                        model: [""].concat(backend.statusChoices)
                                        displayText: currentText === "" ? "None" : currentText
                                        Component.onCompleted: {
                                            var value = root.cellText(row.modelData, cell.modelData)
                                            var idx = model.indexOf(value)
                                            currentIndex = idx >= 0 ? idx : 0
                                        }
                                        onActivated: function() {
                                            backend.setRecordField(row.modelData.url, root.statusKey, currentText)
                                        }
                                        onAccepted: {
                                            backend.addStatusChoice(editText)
                                            backend.setRecordField(row.modelData.url, root.statusKey, editText)
                                        }
                                    }

                                    TextField {
                                        id: dateField
                                        visible: cell.isDate
                                        anchors.fill: parent
                                        anchors.margins: 4
                                        text: root.cellText(row.modelData, cell.modelData)
                                        placeholderText: "YYYY-MM-DD"
                                        font.family: "iA Writer Mono S"
                                        font.pixelSize: win.scaledSize(11)
                                        onEditingFinished: {
                                            backend.setRecordField(row.modelData.url, root.dateKey, text.trim())
                                        }
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        enabled: cell.isOpen
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: win.requestOpen(row.modelData.url)
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
