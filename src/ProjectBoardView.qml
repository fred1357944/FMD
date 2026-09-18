import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    objectName: "projectBoardView"

    function laneTargets() {
        var items = []
        for (var i = 0; i < laneRepeater.count; ++i) {
            var item = laneRepeater.itemAt(i)
            if (item)
                items.push(item)
        }
        return items
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        RowLayout {
            visible: backend.workspaceFolderPath.length > 0
            Layout.fillWidth: true
            Label {
                text: "Columns are the status values in this folder. Add a column for a drop target; it stays with this folder only. Press × to delete a column."
                color: win.mutedColor
                wrapMode: Text.Wrap
                Layout.fillWidth: true
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(11)
            }
            TextField {
                id: newStatusField
                placeholderText: "Add status"
                Layout.preferredWidth: win.scaledSize(140)
                Layout.fillWidth: false
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(11)
                color: win.strongTextColor
                onAccepted: {
                    backend.addStatusChoice(text)
                    text = ""
                }
            }
            PaneButton {
                objectName: "addStatusButton"
                text: "Add"
                Layout.fillWidth: false
                onClicked: {
                    backend.addStatusChoice(newStatusField.text)
                    newStatusField.text = ""
                }
            }
        }

        Label {
            visible: backend.workspaceFolderPath.length === 0
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            color: win.mutedColor
            font.family: "iA Writer Mono S"
            font.pixelSize: win.scaledSize(13)
            text: (backend.uiLanguage, backend.t("boardNeedsFolder"))
        }

        Flickable {
            id: hFlick
            visible: backend.workspaceFolderPath.length > 0
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            interactive: !dragLayer.active
            flickableDirection: Flickable.HorizontalFlick
            boundsBehavior: Flickable.StopAtBounds
            contentWidth: laneRow.implicitWidth
            contentHeight: height
            ScrollBar.horizontal: ScrollBar { policy: ScrollBar.AsNeeded }

            Row {
                id: laneRow
                spacing: 10
                height: hFlick.height

                Repeater {
                    id: laneRepeater
                    model: backend.boardColumns

                    Rectangle {
                        id: lane
                        required property var modelData
                        property string dropValue: modelData.value
                        property bool dropHot: dragLayer.hoverTarget === lane
                        width: win.scaledSize(220)
                        height: hFlick.height
                        radius: 8
                        color: dropHot ? win.currentFileColor : win.pageColor
                        border.color: dropHot ? backend.themeAccent : win.panelBorderColor
                        border.width: dropHot ? 2 : 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 8

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                TextField {
                                    Layout.fillWidth: true
                                    text: lane.modelData.label
                                    readOnly: lane.modelData.value === ""
                                    color: win.strongTextColor
                                    font.family: "iA Writer Mono S"
                                    font.pixelSize: win.scaledSize(12)
                                    font.bold: true
                                    background: Item {}
                                    onEditingFinished: {
                                        if (lane.modelData.value !== "" && text !== lane.modelData.label)
                                            backend.renameStatus(lane.modelData.value, text)
                                    }
                                }
                                Label {
                                    text: String(lane.modelData.records.length)
                                    color: win.mutedColor
                                    font.family: "iA Writer Mono S"
                                    font.pixelSize: win.scaledSize(11)
                                }
                                ToolButton {
                                    text: "+"
                                    implicitWidth: win.scaledSize(28)
                                    implicitHeight: win.scaledSize(28)
                                    ToolTip.visible: hovered
                                    ToolTip.text: backend.t("boardAddNote")
                                    onClicked: backend.createBoardNote(lane.modelData.value)
                                }

                                ToolButton {
                                    objectName: "deleteLaneButton"
                                    visible: lane.modelData.value !== ""
                                    text: "×"
                                    implicitWidth: win.scaledSize(28)
                                    implicitHeight: win.scaledSize(28)
                                    font.pixelSize: win.scaledSize(16)
                                    ToolTip.visible: hovered
                                    ToolTip.text: "Remove this column"
                                    onClicked: {
                                        var count = lane.modelData.records.length
                                        if (count === 0) {
                                            backend.removeStatusChoice(lane.modelData.value)
                                            return
                                        }
                                        deleteLaneDialog.laneValue = lane.modelData.value
                                        deleteLaneDialog.noteCount = count
                                        deleteLaneDialog.open()
                                    }
                                }
                            }

                            Flickable {
                                id: vFlick
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                clip: true
                                interactive: !dragLayer.active
                                boundsBehavior: Flickable.StopAtBounds
                                contentWidth: width
                                contentHeight: cardColumn.implicitHeight
                                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                                Column {
                                    id: cardColumn
                                    width: vFlick.width
                                    spacing: 6

                                    Repeater {
                                        model: lane.modelData.records
                                        Rectangle {
                                            id: card
                                            required property var modelData
                                            width: cardColumn.width
                                            height: Math.max(win.scaledSize(56), cardBody.implicitHeight + 16)
                                            radius: 6
                                            color: win.panelColor
                                            border.color: win.panelBorderColor
                                            opacity: dragLayer.active && dragLayer.noteUrl === modelData.url.toString() ? 0.35 : 1

                                            Column {
                                                id: cardBody
                                                anchors.left: parent.left
                                                anchors.right: parent.right
                                                anchors.top: parent.top
                                                anchors.margins: 8
                                                spacing: 4
                                                Label {
                                                    id: titleLabel
                                                    width: parent.width
                                                    text: card.modelData.title
                                                    color: win.strongTextColor
                                                    wrapMode: Text.Wrap
                                                    font.family: "iA Writer Mono S"
                                                    font.pixelSize: win.scaledSize(12)
                                                }
                                                Label {
                                                    width: parent.width
                                                    text: card.modelData.name
                                                    color: win.mutedColor
                                                    elide: Text.ElideMiddle
                                                    font.family: "iA Writer Mono S"
                                                    font.pixelSize: win.scaledSize(10)
                                                }
                                                Label {
                                                    width: parent.width
                                                    visible: {
                                                        var fields = card.modelData.fields || {}
                                                        return !!(fields.date || fields.scheduled || fields.due)
                                                    }
                                                    text: {
                                                        var fields = card.modelData.fields || {}
                                                        return fields.date || fields.scheduled || fields.due || ""
                                                    }
                                                    color: win.mutedColor
                                                    font.family: "iA Writer Mono S"
                                                    font.pixelSize: win.scaledSize(10)
                                                }
                                                Label {
                                                    width: parent.width
                                                    visible: (card.modelData.openTasks + card.modelData.doneTasks) > 0
                                                    text: card.modelData.doneTasks + "/"
                                                          + (card.modelData.openTasks + card.modelData.doneTasks)
                                                          + " tasks"
                                                    color: win.mutedColor
                                                    font.family: "iA Writer Mono S"
                                                    font.pixelSize: win.scaledSize(10)
                                                }
                                            }

                                            MouseArea {
                                                anchors.fill: parent
                                                hoverEnabled: false
                                                cursorShape: pressed || dragLayer.active ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                                                preventStealing: dragLayer.active || pressed
                                                property point pressPos
                                                property bool dragging: false

                                                onPressed: function(mouse) {
                                                    pressPos = Qt.point(mouse.x, mouse.y)
                                                    dragging = false
                                                }
                                                onPositionChanged: function(mouse) {
                                                    if (!pressed)
                                                        return
                                                    var layerPos = mapToItem(dragLayer, mouse.x, mouse.y)
                                                    if (!dragging) {
                                                        if (Math.hypot(mouse.x - pressPos.x, mouse.y - pressPos.y) < dragLayer.activationDistance)
                                                            return
                                                        dragging = true
                                                        dragLayer.edgeScroller = hFlick
                                                        dragLayer.start(layerPos, pressPos, {
                                                                            url: card.modelData.url.toString(),
                                                                            title: card.modelData.title,
                                                                            width: card.width,
                                                                            height: card.height
                                                                        }, root.laneTargets())
                                                    } else {
                                                        dragLayer.dragTo(layerPos)
                                                    }
                                                }
                                                onReleased: function() {
                                                    if (dragging)
                                                        dragLayer.finish()
                                                    else
                                                        win.requestOpen(card.modelData.url)
                                                    dragging = false
                                                }
                                                onCanceled: {
                                                    if (dragging)
                                                        dragLayer.finish()
                                                    dragging = false
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
        }
    }

    CardDragLayer {
        id: dragLayer
        onDropped: function(url, target) {
            if (!target)
                return
            Qt.callLater(function() {
                backend.setRecordField(url, backend.boardField, target.dropValue)
            })
        }
    }

    Dialog {
        id: deleteLaneDialog
        property string laneValue: ""
        property int noteCount: 0
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape
        width: Math.min(win.scaledSize(420), root.width - 48)
        x: Math.round((root.width - width) / 2)
        y: Math.round((root.height - height) / 2)
        padding: 20

        background: Rectangle {
            color: win.darkMode ? "#1a1a1a" : "#ffffff"
            border.color: win.panelBorderColor
            radius: 8
        }

        contentItem: Column {
            spacing: 12
            Label {
                text: "Remove column"
                color: win.strongTextColor
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(16)
                font.bold: true
            }
            Label {
                width: deleteLaneDialog.availableWidth
                wrapMode: Text.Wrap
                color: win.textColor
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(13)
                text: "Delete “" + deleteLaneDialog.laneValue + "”? "
                      + deleteLaneDialog.noteCount
                      + (deleteLaneDialog.noteCount === 1 ? " note" : " notes")
                      + " will move to None and lose this status."
            }
        }

        footer: Item {
            implicitHeight: win.scaledSize(52)
            Row {
                anchors.right: parent.right
                anchors.rightMargin: 20
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8
                SquareDialogButton {
                    text: "Cancel"
                    darkMode: win.darkMode
                    textScale: win.textScale
                    labelColor: win.textColor
                    onClicked: deleteLaneDialog.close()
                }
                SquareDialogButton {
                    text: "Delete column"
                    primary: true
                    darkMode: win.darkMode
                    textScale: win.textScale
                    activeColor: backend.themeAccent
                    onClicked: {
                        backend.removeStatusChoice(deleteLaneDialog.laneValue)
                        deleteLaneDialog.close()
                    }
                }
            }
        }
    }
}
