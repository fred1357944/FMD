import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ColumnLayout {
    id: nav
    spacing: 6

    function positionCurrentFile(index) {
        if (index >= 0)
            workspaceSidebar.positionViewAtIndex(index, ListView.Center)
    }

    Label {
        Layout.fillWidth: true
        visible: backend.workspaceShortcuts.length > 0
        text: (backend.uiLanguage, backend.t("shortcuts"))
        color: win.mutedColor
        font.family: "iA Writer Mono S"
        font.pixelSize: win.scaledSize(10)
        font.bold: true
    }

    Flow {
        Layout.fillWidth: true
        visible: backend.workspaceShortcuts.length > 0
        spacing: 4
        Repeater {
            model: backend.workspaceShortcuts
            delegate: ToolButton {
                required property var modelData
                text: "★ " + (modelData.title || modelData.name)
                implicitHeight: win.scaledSize(24)
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(10)
                onClicked: if (modelData.url)
                    win.requestOpen(modelData.url)
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.fillHeight: true
        spacing: 4

        ListView {
            id: workspaceFolderSidebar
            objectName: "workspaceFolderSidebar"
            Layout.preferredWidth: Math.max(win.scaledSize(96), parent.width * 0.38)
            Layout.fillHeight: true
            clip: true
            model: backend.workspaceNavFolders
            spacing: 1
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            delegate: ItemDelegate {
                id: folderDelegate
                required property var modelData
                readonly property var fileEntry: modelData
                width: ListView.view ? ListView.view.width : parent.width
                implicitHeight: win.scaledSize(28)
                highlighted: fileEntry && fileEntry.path === backend.selectedFolderPath
                background: Rectangle {
                    radius: 6
                    color: folderDelegate.highlighted ? win.currentFileColor : "transparent"
                    border.color: folderDelegate.highlighted ? win.panelBorderColor : "transparent"
                }
                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    onClicked: function(mouse) {
                        if (!folderDelegate.fileEntry)
                            return
                        if (mouse.button === Qt.RightButton) {
                            folderMenu.popup()
                            return
                        }
                        backend.selectedWorkspacePath = folderDelegate.fileEntry.path
                    }
                }
                Label {
                    anchors.fill: parent
                    anchors.leftMargin: 8 + (folderDelegate.fileEntry
                                             ? folderDelegate.fileEntry.depth * 10 : 0)
                    anchors.rightMargin: 6
                    text: folderDelegate.fileEntry ? folderDelegate.fileEntry.name : ""
                    elide: Text.ElideMiddle
                    color: win.strongTextColor
                    font.family: "iA Writer Mono S"
                    font.pixelSize: win.scaledSize(12)
                    font.bold: true
                    verticalAlignment: Text.AlignVCenter
                }
                Menu {
                    id: folderMenu
                    MenuItem {
                        text: (backend.uiLanguage, backend.t("newFolder"))
                        onTriggered: {
                            backend.selectedWorkspacePath = folderDelegate.fileEntry.path
                            win.createFolder()
                        }
                    }
                    MenuItem {
                        text: "New Markdown file"
                        onTriggered: {
                            backend.selectedWorkspacePath = folderDelegate.fileEntry.path
                            win.createMarkdownNote()
                        }
                    }
                    MenuItem {
                        text: (backend.uiLanguage, backend.t("renameFolder"))
                        visible: folderDelegate.fileEntry && !folderDelegate.fileEntry.root
                        onTriggered: if (folderDelegate.fileEntry)
                            win.startRename(folderDelegate.fileEntry.url)
                    }
                    MenuItem {
                        text: (backend.uiLanguage, backend.t("revealInFinder"))
                        onTriggered: if (folderDelegate.fileEntry)
                            backend.revealInFinder(folderDelegate.fileEntry.url)
                    }
                }
            }
        }

        Rectangle {
            Layout.fillHeight: true
            implicitWidth: 1
            color: win.panelBorderColor
        }

        ListView {
            id: workspaceSidebar
            objectName: "workspaceSidebar"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: backend.workspaceNavFiles
            spacing: 1
            visible: count > 0
            currentIndex: -1
            highlightFollowsCurrentItem: false
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            delegate: ItemDelegate {
                id: fileDelegate
                required property var modelData
                readonly property var fileEntry: modelData
                readonly property bool isFolder: false
                readonly property bool currentFile:
                    fileEntry && backend.fileUrl.toString() === fileEntry.url.toString()
                readonly property bool renaming:
                    fileEntry && win.renamingUrl.toString() === fileEntry.url.toString()
                width: ListView.view ? ListView.view.width : parent.width
                text: ""
                implicitHeight: win.scaledSize(fileEntry && fileEntry.cover ? 40 : 32)
                highlighted: currentFile
                Drag.dragType: Drag.Automatic
                Drag.supportedActions: Qt.CopyAction
                Drag.mimeData: fileEntry
                    ? { "text/uri-list": fileEntry.url.toString() }
                    : {}
                Drag.active: fileDrag.active

                DragHandler {
                    id: fileDrag
                    enabled: !fileDelegate.renaming && backend.projectView === "mindmap"
                    target: null
                    acceptedButtons: Qt.LeftButton
                }

                background: Rectangle {
                    radius: 6
                    color: fileDelegate.highlighted ? win.currentFileColor : "transparent"
                    border.color: fileDelegate.highlighted ? win.panelBorderColor : "transparent"
                }

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    onClicked: function(mouse) {
                        if (fileDelegate.renaming || fileDrag.active)
                            return
                        if (mouse.button === Qt.RightButton) {
                            fileMenu.popup()
                            return
                        }
                        if (!fileDelegate.fileEntry)
                            return
                        backend.selectedWorkspacePath = fileDelegate.fileEntry.path
                        if (!fileDelegate.currentFile)
                            win.requestOpen(fileDelegate.fileEntry.url)
                    }
                }

                Row {
                    anchors.fill: parent
                    anchors.leftMargin: 6
                    anchors.rightMargin: 6
                    spacing: 6
                    visible: !fileDelegate.renaming

                    Rectangle {
                        width: win.scaledSize(28)
                        height: win.scaledSize(28)
                        anchors.verticalCenter: parent.verticalCenter
                        radius: 4
                        color: win.darkMode ? "#1b1b1b" : "#efeae2"
                        clip: true
                        Image {
                            anchors.fill: parent
                            visible: fileDelegate.fileEntry && fileDelegate.fileEntry.cover
                            source: fileDelegate.fileEntry && fileDelegate.fileEntry.cover
                                    ? fileDelegate.fileEntry.cover : ""
                            fillMode: Image.PreserveAspectCrop
                            asynchronous: true
                        }
                    }

                    Label {
                        width: parent.width - win.scaledSize(34)
                        height: parent.height
                        text: fileDelegate.fileEntry
                              ? ((fileDelegate.fileEntry.pinned ? "★ " : "")
                                 + (fileDelegate.fileEntry.threads
                                    ? (fileDelegate.fileEntry.name + "  · Threads")
                                    : fileDelegate.fileEntry.name))
                              : ""
                        elide: Text.ElideMiddle
                        color: win.strongTextColor
                        font.family: "iA Writer Mono S"
                        font.pixelSize: win.scaledSize(12)
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                TextInput {
                    id: fileRenameField
                    anchors.fill: parent
                    anchors.leftMargin: 36
                    anchors.rightMargin: 8
                    visible: fileDelegate.renaming
                    activeFocusOnTab: false
                    color: win.strongTextColor
                    font.family: "iA Writer Mono S"
                    font.pixelSize: win.scaledSize(13)
                    verticalAlignment: Text.AlignVCenter
                    clip: true
                    onVisibleChanged: if (visible && fileDelegate.fileEntry) {
                        text = win.noteStem(fileDelegate.fileEntry.name)
                        forceActiveFocus()
                        selectAll()
                    }
                    function commitRename() {
                        if (!fileDelegate.fileEntry)
                            return
                        backend.renameNote(fileDelegate.fileEntry.url, text)
                        win.renamingUrl = ""
                    }
                    onAccepted: commitRename()
                    Keys.onEscapePressed: win.renamingUrl = ""
                    onEditingFinished: {
                        if (!fileDelegate.renaming)
                            return
                        commitRename()
                    }
                }

                Menu {
                    id: fileMenu
                    MenuItem {
                        text: (backend.uiLanguage, backend.t("openNote"))
                        onTriggered: if (fileDelegate.fileEntry)
                            win.requestOpen(fileDelegate.fileEntry.url)
                    }
                    MenuItem {
                        text: (backend.uiLanguage, backend.t("renameNote"))
                        onTriggered: if (fileDelegate.fileEntry)
                            win.startRename(fileDelegate.fileEntry.url)
                    }
                    MenuItem {
                        text: fileDelegate.fileEntry && fileDelegate.fileEntry.pinned
                              ? (backend.uiLanguage, backend.t("unpinNote"))
                              : (backend.uiLanguage, backend.t("pinNote"))
                        onTriggered: if (fileDelegate.fileEntry)
                            backend.togglePin(fileDelegate.fileEntry.url)
                    }
                    MenuItem {
                        text: (backend.uiLanguage, backend.t("revealInFinder"))
                        onTriggered: if (fileDelegate.fileEntry)
                            backend.revealInFinder(fileDelegate.fileEntry.url)
                    }
                    MenuSeparator {}
                    MenuItem {
                        text: (backend.uiLanguage, backend.t("moveToTrash"))
                        onTriggered: if (fileDelegate.fileEntry)
                            win.requestDelete(fileDelegate.fileEntry.url)
                    }
                }
            }
        }
    }

    Label {
        Layout.fillWidth: true
        Layout.fillHeight: workspaceSidebar.count === 0
        wrapMode: Text.Wrap
        text: backend.tagFilter.length > 0
            ? "No notes tagged #" + backend.tagFilter + "."
            : (backend.workspaceFolderPath.length > 0
                ? "No Markdown files in this folder."
                : (backend.uiLanguage, backend.t("openFolderHint")))
        color: win.mutedColor
        font.family: "iA Writer Mono S"
        font.pixelSize: win.scaledSize(12)
        visible: workspaceSidebar.count === 0
    }
}
