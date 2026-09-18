import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: bar
    objectName: "projectViewBar"
    spacing: 6

    readonly property bool libraryView: backend.projectView === "table"
                                        || backend.projectView === "board"
                                        || backend.projectView === "calendar"
                                        || backend.projectView === "cards"

    RowLayout {
        Layout.fillWidth: true
        spacing: 8
        height: win.scaledSize(36)

        Label {
            text: (backend.uiLanguage, backend.t("view"))
            color: win.mutedColor
            font.family: "iA Writer Mono S"
            font.pixelSize: win.scaledSize(11)
        }

        Repeater {
            model: [
                { id: "editor", labelKey: "editor" },
                { id: "table", labelKey: "table" },
                { id: "board", labelKey: "board" },
                { id: "calendar", labelKey: "calendar" },
                { id: "cards", labelKey: "cards" },
                { id: "mindmap", labelKey: "mindmap" }
            ]

            ToolButton {
                required property var modelData
                objectName: "projectViewButton-" + modelData.id
                text: (backend.uiLanguage, backend.t(modelData.labelKey))
                checkable: true
                checked: backend.projectView === modelData.id
                enabled: true
                implicitHeight: win.scaledSize(28)
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(12)
                font.bold: backend.projectView === modelData.id
                onClicked: backend.projectView = modelData.id
            }
        }

        Item { Layout.fillWidth: true }

        ToolButton {
            objectName: "liveSourceToggle"
            visible: backend.projectView === "editor" && !backend.zenMode
            text: backend.editorMode === "source"
                  ? (backend.uiLanguage, backend.t("sourceMode"))
                  : (backend.uiLanguage, backend.t("liveMode"))
            implicitHeight: win.scaledSize(28)
            font.family: "iA Writer Mono S"
            font.pixelSize: win.scaledSize(11)
            ToolTip.visible: hovered
            ToolTip.text: (backend.uiLanguage, backend.t("liveSourceHelp"))
            onClicked: backend.editorMode = backend.editorMode === "source" ? "live" : "source"
        }

        ToolButton {
            objectName: "zenToggleButton"
            visible: !backend.zenMode
            text: (backend.uiLanguage, backend.t("zen"))
            implicitHeight: win.scaledSize(28)
            font.family: "iA Writer Mono S"
            font.pixelSize: win.scaledSize(11)
            onClicked: backend.zenMode = true
        }

        Label {
            visible: bar.libraryView
            text: backend.projectRecords.length + " notes · " + backend.boardField + " / " + backend.dateField
            color: win.mutedColor
            font.family: "iA Writer Mono S"
            font.pixelSize: win.scaledSize(11)
        }
    }

    RowLayout {
        Layout.fillWidth: true
        visible: bar.libraryView
        spacing: 8

        TextField {
            id: keywordField
            objectName: "projectKeywordFilter"
            Layout.fillWidth: true
            implicitHeight: win.scaledSize(32)
            placeholderText: (backend.uiLanguage, backend.t("filterKeyword"))
            font.family: "iA Writer Mono S"
            font.pixelSize: win.scaledSize(11)
            text: backend.projectKeywordFilter
            onTextEdited: backend.projectKeywordFilter = text
        }

        ComboBox {
            id: tagFilterBox
            objectName: "projectTagFilter"
            implicitWidth: win.scaledSize(140)
            implicitHeight: win.scaledSize(32)
            font.family: "iA Writer Mono S"
            font.pixelSize: win.scaledSize(11)
            model: {
                var items = [backend.t("tags")]
                for (var i = 0; i < backend.workspaceTags.length; ++i)
                    items.push(backend.workspaceTags[i].name)
                return items
            }
            onActivated: {
                if (currentIndex <= 0)
                    backend.tagFilter = ""
                else
                    backend.tagFilter = currentText
            }
        }

        ComboBox {
            id: dateFilterBox
            objectName: "projectDateFilter"
            implicitWidth: win.scaledSize(150)
            implicitHeight: win.scaledSize(32)
            font.family: "iA Writer Mono S"
            font.pixelSize: win.scaledSize(11)
            textRole: "label"
            valueRole: "id"
            model: [
                { id: "all", label: backend.t("dateAll") },
                { id: "today", label: backend.t("dateToday") },
                { id: "yesterday", label: backend.t("dateYesterday") },
                { id: "week", label: backend.t("dateWeek") },
                { id: "dated", label: backend.t("dateDated") },
                { id: "undated", label: backend.t("dateUndated") }
            ]
            Component.onCompleted: {
                for (var i = 0; i < model.length; ++i) {
                    if (model[i].id === backend.projectDateFilter)
                        currentIndex = i
                }
            }
            onActivated: backend.projectDateFilter = currentValue
        }

        ComboBox {
            id: publishFilterBox
            objectName: "projectPublishFilter"
            implicitWidth: win.scaledSize(150)
            implicitHeight: win.scaledSize(32)
            font.family: "iA Writer Mono S"
            font.pixelSize: win.scaledSize(11)
            textRole: "label"
            valueRole: "id"
            model: [
                { id: "all", label: backend.t("publishAll") },
                { id: "published", label: backend.t("publishPublished") },
                { id: "drafts", label: backend.t("publishDrafts") }
            ]
            Component.onCompleted: {
                for (var i = 0; i < model.length; ++i) {
                    if (model[i].id === backend.projectPublishFilter)
                        currentIndex = i
                }
            }
            onActivated: backend.projectPublishFilter = currentValue
        }
    }
}
