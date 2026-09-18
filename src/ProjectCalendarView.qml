import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    objectName: "projectCalendarView"

    function dayTargets() {
        var items = []
        for (var i = 0; i < dayRepeater.count; ++i) {
            var item = dayRepeater.itemAt(i)
            if (item)
                items.push(item)
        }
        return items
    }

    function allDropTargets() {
        var items = dayTargets()
        items.push(unscheduledTray)
        return items
    }

    function beginNoteDrag(mouseArea, mouse, note, item) {
        var layerPos = mouseArea.mapToItem(dragLayer, mouse.x, mouse.y)
        dragLayer.start(layerPos, Qt.point(mouse.x, mouse.y), {
                            url: note.url.toString(),
                            title: note.title,
                            width: Math.max(item.width, win.scaledSize(120)),
                            height: Math.max(item.height, win.scaledSize(28))
                        }, root.allDropTargets())
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        Label {
            visible: backend.workspaceFolderPath.length === 0
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            color: win.mutedColor
            font.family: "iA Writer Mono S"
            font.pixelSize: win.scaledSize(13)
            text: (backend.uiLanguage, backend.t("calendarNeedsFolder"))
        }

        Label {
            visible: backend.workspaceFolderPath.length > 0
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            color: win.mutedColor
            font.family: "iA Writer Mono S"
            font.pixelSize: win.scaledSize(12)
            text: (backend.uiLanguage, backend.t("calendarHeatmapHelp"))
        }

        Item {
            id: heat
            visible: backend.workspaceFolderPath.length > 0
            Layout.fillWidth: true
            Layout.preferredHeight: 7 * (cell + gap) - gap
            clip: true
            readonly property int cell: Math.max(8, win.scaledSize(11))
            readonly property int gap: 3

            Repeater {
                model: backend.activityHeatmap
                delegate: Rectangle {
                    required property int index
                    required property var modelData
                    width: heat.cell
                    height: heat.cell
                    radius: 2
                    x: Math.floor(index / 7) * (heat.cell + heat.gap)
                    y: (index % 7) * (heat.cell + heat.gap)
                    opacity: modelData.future ? 0.18 : 1
                    color: {
                        var lv = modelData.level
                        if (lv >= 4) return "#216e39"
                        if (lv >= 3) return "#30a14e"
                        if (lv >= 2) return "#40c463"
                        if (lv >= 1) return "#9be9a8"
                        return win.darkMode ? "#2a2a2a" : "#ebedf0"
                    }
                    MouseArea {
                        anchors.fill: parent
                        enabled: !modelData.future
                        hoverEnabled: true
                        cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                        ToolTip.visible: containsMouse
                        ToolTip.text: modelData.date + " · " + modelData.count
                        onClicked: backend.revealCalendarDate(modelData.date)
                    }
                }
            }
        }

        RowLayout {
            visible: backend.workspaceFolderPath.length > 0
            Layout.fillWidth: true
            ToolButton {
                text: "‹"
                implicitWidth: win.scaledSize(32)
                implicitHeight: win.scaledSize(28)
                onClicked: backend.stepCalendar(-1)
            }
            Label {
                id: calendarTitleLabel
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                text: backend.calendarTitle
                color: win.strongTextColor
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(14)
                font.bold: true
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: monthPicker.open()
                }
            }
            ToolButton {
                text: "›"
                implicitWidth: win.scaledSize(32)
                implicitHeight: win.scaledSize(28)
                onClicked: backend.stepCalendar(1)
            }

            Popup {
                id: monthPicker
                parent: calendarTitleLabel
                x: Math.round((calendarTitleLabel.width - width) / 2)
                y: calendarTitleLabel.height + 6
                padding: 10
                modal: true
                focus: true
                closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

                Column {
                    spacing: 8
                    RowLayout {
                        width: monthGrid.width
                        ToolButton {
                            text: "‹"
                            onClicked: backend.showCalendarMonth(backend.calendarYear - 1, backend.calendarMonth)
                        }
                        Label {
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignHCenter
                            text: backend.calendarYear
                            color: win.strongTextColor
                            font.family: "iA Writer Mono S"
                            font.pixelSize: win.scaledSize(13)
                            font.bold: true
                        }
                        ToolButton {
                            text: "›"
                            onClicked: backend.showCalendarMonth(backend.calendarYear + 1, backend.calendarMonth)
                        }
                    }
                    Grid {
                        id: monthGrid
                        columns: 4
                        columnSpacing: 4
                        rowSpacing: 4
                        Repeater {
                            model: 12
                            ToolButton {
                                required property int index
                                text: backend.calendarMonthNames()[index]
                                font.bold: backend.calendarMonth === index + 1
                                implicitWidth: win.scaledSize(56)
                                implicitHeight: win.scaledSize(28)
                                onClicked: {
                                    backend.showCalendarMonth(backend.calendarYear, index + 1)
                                    monthPicker.close()
                                }
                            }
                        }
                    }
                    RowLayout {
                        width: monthGrid.width
                        ToolButton {
                            text: backend.t("dateToday")
                            Layout.fillWidth: true
                            onClicked: {
                                backend.showCalendarToday()
                                monthPicker.close()
                            }
                        }
                        ToolButton {
                            text: backend.t("dateYesterday")
                            Layout.fillWidth: true
                            onClicked: {
                                var d = new Date()
                                d.setDate(d.getDate() - 1)
                                backend.showCalendarMonth(d.getFullYear(), d.getMonth() + 1)
                                monthPicker.close()
                            }
                        }
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Repeater {
                model: ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"]
                Label {
                    required property string modelData
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    text: modelData
                    color: win.mutedColor
                    font.family: "iA Writer Mono S"
                    font.pixelSize: win.scaledSize(11)
                }
            }
        }

        GridLayout {
            id: dayGrid
            Layout.fillWidth: true
            Layout.fillHeight: true
            columns: 7
            rowSpacing: 4
            columnSpacing: 4

            Repeater {
                id: dayRepeater
                model: backend.calendarCells
                Rectangle {
                    id: dayCell
                    required property var modelData
                    property string dropValue: modelData.date
                    property bool dropHot: dragLayer.hoverTarget === dayCell
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: 6
                    color: dropHot ? win.currentFileColor
                                   : (modelData.inMonth ? win.pageColor : "transparent")
                    border.color: dropHot ? backend.themeAccent : win.panelBorderColor
                    border.width: dropHot ? 2 : 1
                    opacity: modelData.inMonth ? 1 : 0.45

                    Column {
                        anchors.fill: parent
                        anchors.margins: 6
                        spacing: 4
                        clip: true

                        Label {
                            text: modelData.day
                            color: win.strongTextColor
                            font.family: "iA Writer Mono S"
                            font.pixelSize: win.scaledSize(11)
                            font.bold: true
                        }

                        Repeater {
                            model: modelData.records
                            Rectangle {
                                id: dayNote
                                required property var modelData
                                width: parent.width
                                height: Math.max(win.scaledSize(22), dayNoteLabel.implicitHeight + 6)
                                radius: 4
                                color: win.panelColor
                                border.color: win.panelBorderColor
                                opacity: dragLayer.active && dragLayer.noteUrl === modelData.url.toString() ? 0.35 : 1

                                Label {
                                    id: dayNoteLabel
                                    anchors.fill: parent
                                    anchors.margins: 3
                                    text: dayNote.modelData.title
                                    elide: Text.ElideRight
                                    color: win.textColor
                                    font.family: "iA Writer Mono S"
                                    font.pixelSize: win.scaledSize(10)
                                    verticalAlignment: Text.AlignVCenter
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
                                            root.beginNoteDrag(this, mouse, dayNote.modelData, dayNote)
                                        } else {
                                            dragLayer.dragTo(layerPos)
                                        }
                                    }
                                    onReleased: function() {
                                        if (dragging)
                                            dragLayer.finish()
                                        else
                                            win.requestOpen(dayNote.modelData.url)
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

        Rectangle {
            id: unscheduledTray
            objectName: "unscheduledTray"
            property string dropValue: ""
            property bool dropHot: dragLayer.hoverTarget === unscheduledTray
            Layout.fillWidth: true
            Layout.preferredHeight: win.scaledSize(96)
            Layout.maximumHeight: win.scaledSize(140)
            Layout.minimumHeight: win.scaledSize(72)
            clip: true
            radius: 8
            color: dropHot ? win.currentFileColor : win.pageColor
            border.color: dropHot ? backend.themeAccent : win.panelBorderColor
            border.width: dropHot ? 2 : 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 6

                Label {
                    text: "Unscheduled"
                    color: win.strongTextColor
                    font.family: "iA Writer Mono S"
                    font.pixelSize: win.scaledSize(12)
                    font.bold: true
                }

                Flickable {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    contentWidth: width
                    contentHeight: unscheduledFlow.implicitHeight
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                    Flow {
                        id: unscheduledFlow
                        width: parent.width
                        spacing: 8

                        Repeater {
                            model: backend.unscheduledRecords
                            Rectangle {
                                id: unscheduledCard
                                required property var modelData
                                radius: 6
                                height: win.scaledSize(28)
                                width: Math.max(win.scaledSize(80),
                                                Math.min(win.scaledSize(200), unscheduledFlow.width))
                                clip: true
                                color: win.panelColor
                                border.color: win.panelBorderColor
                                opacity: dragLayer.active && dragLayer.noteUrl === modelData.url.toString() ? 0.35 : 1

                                Label {
                                    anchors.fill: parent
                                    anchors.leftMargin: 8
                                    anchors.rightMargin: 8
                                    text: unscheduledCard.modelData.title
                                    elide: Text.ElideRight
                                    wrapMode: Text.NoWrap
                                    color: win.textColor
                                    font.family: "iA Writer Mono S"
                                    font.pixelSize: win.scaledSize(11)
                                    verticalAlignment: Text.AlignVCenter
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
                                            root.beginNoteDrag(this, mouse, unscheduledCard.modelData, unscheduledCard)
                                        } else {
                                            dragLayer.dragTo(layerPos)
                                        }
                                    }
                                    onReleased: function() {
                                        if (dragging)
                                            dragLayer.finish()
                                        else
                                            win.requestOpen(unscheduledCard.modelData.url)
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

                        Label {
                            visible: backend.unscheduledRecords.length === 0
                            text: "Drop a dated note here to clear its date."
                            color: win.mutedColor
                            font.family: "iA Writer Mono S"
                            font.pixelSize: win.scaledSize(11)
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
                backend.setRecordField(url, backend.dateField, target.dropValue)
            })
        }
    }
}
