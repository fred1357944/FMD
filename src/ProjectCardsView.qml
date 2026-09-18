import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    objectName: "projectCardsView"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            spacing: 8
            Label {
                text: (backend.uiLanguage, backend.t("cardsHelp"))
                color: win.mutedColor
                wrapMode: Text.Wrap
                Layout.fillWidth: true
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(11)
            }
            PaneButton {
                objectName: "randomCardButton"
                text: (backend.uiLanguage, backend.t("randomCard"))
                Layout.fillWidth: false
                onClicked: {
                    var url = backend.randomCardUrl()
                    if (url && url.toString().length > 0)
                        win.requestOpen(url)
                }
            }
            PaneButton {
                visible: backend.cardTagFilters.length > 0
                text: (backend.uiLanguage, backend.t("clearCardTags"))
                Layout.fillWidth: false
                onClicked: backend.clearCardTags()
            }
        }

        Flow {
            Layout.fillWidth: true
            spacing: 6
            Repeater {
                model: backend.workspaceTags
                delegate: ToolButton {
                    required property var modelData
                    text: modelData.name + " " + modelData.count
                    checkable: true
                    checked: backend.cardTagFilters.indexOf(modelData.name) >= 0
                    font.family: "iA Writer Mono S"
                    font.pixelSize: win.scaledSize(10)
                    implicitHeight: win.scaledSize(26)
                    onClicked: backend.toggleCardTag(modelData.name)
                }
            }
        }

        Row {
            Layout.fillWidth: true
            spacing: 2
            Repeater {
                model: backend.activityHeatmap
                delegate: Rectangle {
                    required property var modelData
                    width: Math.max(6, Math.floor((root.width - 24) / 112) )
                    height: 10
                    radius: 1
                    color: {
                        var lv = modelData.level
                        if (lv >= 4) return "#1d4ed8"
                        if (lv >= 3) return "#3b82f6"
                        if (lv >= 2) return "#60a5fa"
                        if (lv >= 1) return "#93c5fd"
                        return win.darkMode ? "#2a2a2a" : "#e5e5e5"
                    }
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        ToolTip.visible: containsMouse
                        ToolTip.text: modelData.date + " · " + modelData.count
                        onClicked: backend.projectDateFilter =
                                   (backend.projectDateFilter === modelData.date
                                    ? "all" : modelData.date)
                    }
                }
            }
        }

        Label {
            visible: backend.workspaceFolderPath.length === 0
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            color: win.mutedColor
            text: (backend.uiLanguage, backend.t("boardNeedsFolder"))
            font.family: "iA Writer Mono S"
            font.pixelSize: win.scaledSize(13)
        }

        Flickable {
            visible: backend.workspaceFolderPath.length > 0
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: width
            contentHeight: grid.implicitHeight
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            Grid {
                id: grid
                width: parent.width
                columns: Math.max(1, Math.floor(width / win.scaledSize(220)))
                columnSpacing: 10
                rowSpacing: 10

                Repeater {
                    model: backend.cardRecords
                    delegate: Item {
                        required property var modelData
                        width: Math.floor((grid.width - (grid.columns - 1) * grid.columnSpacing) / grid.columns)
                        height: win.scaledSize(220)

                        Rectangle {
                            anchors.fill: parent
                            radius: 10
                            color: win.pageColor
                            border.color: modelData.pinned ? backend.themeAccent : win.panelBorderColor
                            border.width: modelData.pinned ? 2 : 1

                            Column {
                                anchors.fill: parent
                                anchors.margins: 8
                                spacing: 6

                                Rectangle {
                                    width: parent.width
                                    height: win.scaledSize(96)
                                    radius: 6
                                    color: win.darkMode ? "#1b1b1b" : "#efeae2"
                                    clip: true
                                    Image {
                                        anchors.fill: parent
                                        visible: modelData.cover && modelData.cover.length > 0
                                        source: modelData.cover || ""
                                        fillMode: Image.PreserveAspectCrop
                                        asynchronous: true
                                    }
                                    Label {
                                        anchors.centerIn: parent
                                        visible: !(modelData.cover && modelData.cover.length > 0)
                                        text: (modelData.title || "?").charAt(0)
                                        color: win.mutedColor
                                        font.pixelSize: win.scaledSize(28)
                                        font.family: "PingFang TC"
                                    }
                                }

                                Label {
                                    width: parent.width
                                    text: (modelData.pinned ? "★ " : "") + (modelData.title || modelData.name)
                                    color: win.strongTextColor
                                    elide: Text.ElideRight
                                    font.family: "PingFang TC"
                                    font.pixelSize: win.scaledSize(13)
                                    font.bold: true
                                }

                                Label {
                                    width: parent.width
                                    height: win.scaledSize(36)
                                    text: modelData.snippet || ""
                                    color: win.mutedColor
                                    wrapMode: Text.Wrap
                                    elide: Text.ElideRight
                                    font.family: "PingFang TC"
                                    font.pixelSize: win.scaledSize(11)
                                }

                                RowLayout {
                                    width: parent.width
                                    Label {
                                        Layout.fillWidth: true
                                        text: (modelData.tags || []).join(" · ")
                                        color: win.mutedColor
                                        elide: Text.ElideRight
                                        font.family: "iA Writer Mono S"
                                        font.pixelSize: win.scaledSize(10)
                                    }
                                    ToolButton {
                                        text: modelData.pinned ? "★" : "☆"
                                        implicitWidth: win.scaledSize(28)
                                        implicitHeight: win.scaledSize(24)
                                        onClicked: backend.togglePin(modelData.url)
                                    }
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                anchors.bottomMargin: 28
                                onClicked: win.requestOpen(modelData.url)
                            }
                        }
                    }
                }
            }
        }
    }
}
