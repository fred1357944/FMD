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
        spacing: 4

        ToolButton {
            objectName: "workspaceNavGoUpButton"
            text: nav.width < win.scaledSize(280)
                  ? "←"
                  : ("← " + (backend.uiLanguage, backend.t("navGoUp")))
            enabled: backend.workspaceNavCanGoUp
            implicitHeight: win.scaledSize(26)
            font.family: "iA Writer Mono S"
            font.pixelSize: win.scaledSize(11)
            ToolTip.visible: hovered
            ToolTip.text: (backend.uiLanguage, backend.t("navGoUp"))
            onClicked: backend.selectParentWorkspaceFolder()
        }

        Flickable {
            id: crumbFlick
            Layout.fillWidth: true
            Layout.preferredHeight: win.scaledSize(26)
            clip: true
            contentWidth: Math.max(width, crumbRow.implicitWidth)
            contentHeight: height
            boundsBehavior: Flickable.StopAtBounds
            function revealEnd() {
                contentX = Math.max(0, contentWidth - width)
            }
            onContentWidthChanged: revealEnd()
            onWidthChanged: revealEnd()

            Row {
                id: crumbRow
                spacing: 0
                height: parent.height
                Repeater {
                    id: crumbRepeater
                    model: backend.workspaceNavCrumbs
                    Row {
                        required property var modelData
                        required property int index
                        spacing: 0
                        height: crumbRow.height
                        Label {
                            visible: index > 0
                            text: " / "
                            height: parent.height
                            color: win.mutedColor
                            font.family: "iA Writer Mono S"
                            font.pixelSize: win.scaledSize(11)
                            verticalAlignment: Text.AlignVCenter
                        }
                        Label {
                            text: modelData && modelData.name ? modelData.name : ""
                            height: parent.height
                            color: index === crumbRepeater.count - 1
                                   ? win.strongTextColor : win.mutedColor
                            font.family: "iA Writer Mono S"
                            font.pixelSize: win.scaledSize(11)
                            font.bold: index === crumbRepeater.count - 1
                            font.underline: index < crumbRepeater.count - 1
                            verticalAlignment: Text.AlignVCenter
                            MouseArea {
                                anchors.fill: parent
                                enabled: index < crumbRepeater.count - 1
                                cursorShape: enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                                onClicked: if (modelData && modelData.path)
                                    backend.selectedWorkspacePath = modelData.path
                            }
                        }
                    }
                }
            }
        }
    }

    SplitView {
        id: navSplit
        objectName: "workspaceNavSplit"
        Layout.fillWidth: true
        Layout.fillHeight: true
        orientation: Qt.Horizontal
        handle: Rectangle {
            implicitWidth: 8
            implicitHeight: 8
            color: "transparent"
            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                width: parent.SplitHandle.pressed || parent.SplitHandle.hovered ? 3 : 1
                height: parent.height - 16
                radius: 1
                color: parent.SplitHandle.pressed
                       ? backend.themeAccent
                       : (parent.SplitHandle.hovered ? win.mutedColor : win.panelBorderColor)
            }
        }

        ColumnLayout {
            id: folderPane
            objectName: "workspaceNavFolderPane"
            SplitView.preferredWidth: backend.workspaceNavSplitWidth
            SplitView.minimumWidth: win.scaledSize(108)
            SplitView.maximumWidth: Math.max(win.scaledSize(108), navSplit.width - win.scaledSize(96))
            spacing: 2
            onWidthChanged: {
                if (width < win.scaledSize(108))
                    return
                if (Math.round(width) !== backend.workspaceNavSplitWidth)
                    backend.workspaceNavSplitWidth = Math.round(width)
            }

            Label {
                Layout.fillWidth: true
                text: (backend.uiLanguage, backend.t("navFolders"))
                color: win.mutedColor
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(10)
                font.bold: true
            }

        ListView {
            id: workspaceFolderSidebar
            objectName: "workspaceFolderSidebar"
            Layout.fillWidth: true
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
                    onDoubleClicked: {
                        if (folderDelegate.fileEntry && folderDelegate.fileEntry.hasChildren)
                            backend.toggleWorkspaceFolder(folderDelegate.fileEntry.path)
                    }
                }
                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 4 + (folderDelegate.fileEntry
                                             ? folderDelegate.fileEntry.depth * 10 : 0)
                    anchors.rightMargin: 4
                    spacing: 4
                    Item {
                        Layout.preferredWidth: win.scaledSize(14)
                        Layout.fillHeight: true
                        Label {
                            anchors.centerIn: parent
                            visible: folderDelegate.fileEntry && folderDelegate.fileEntry.hasChildren
                            text: folderDelegate.fileEntry && folderDelegate.fileEntry.expanded ? "▾" : "▸"
                            color: win.mutedColor
                            font.family: "iA Writer Mono S"
                            font.pixelSize: win.scaledSize(10)
                        }
                        MouseArea {
                            anchors.fill: parent
                            enabled: folderDelegate.fileEntry && folderDelegate.fileEntry.hasChildren
                            onClicked: function(mouse) {
                                mouse.accepted = true
                                backend.toggleWorkspaceFolder(folderDelegate.fileEntry.path)
                            }
                        }
                    }
                    FileKindIcon {
                        Layout.preferredWidth: win.scaledSize(14)
                        Layout.preferredHeight: win.scaledSize(14)
                        kind: "folder"
                        ink: folderDelegate.highlighted ? win.strongTextColor : win.mutedColor
                    }
                    Label {
                        id: folderNameLabel
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        text: folderDelegate.fileEntry ? folderDelegate.fileEntry.name : ""
                        elide: Text.ElideRight
                        color: win.strongTextColor
                        font.family: "iA Writer Mono S"
                        font.pixelSize: win.scaledSize(12)
                        font.bold: folderDelegate.highlighted
                        verticalAlignment: Text.AlignVCenter
                        HoverHandler { id: folderNameHover }
                        ToolTip.visible: folderNameHover.hovered && folderNameLabel.truncated
                        ToolTip.text: folderDelegate.fileEntry ? folderDelegate.fileEntry.name : ""
                        ToolTip.delay: 350
                    }
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
        }

        ColumnLayout {
            SplitView.fillWidth: true
            SplitView.minimumWidth: win.scaledSize(96)
            spacing: 2

            Label {
                Layout.fillWidth: true
                text: (backend.uiLanguage, backend.t("navThisFolder"))
                      + (workspaceSidebar.count > 0
                         ? ("  ·  " + workspaceSidebar.count)
                         : "")
                color: win.mutedColor
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(10)
                font.bold: true
            }

        ListView {
            id: workspaceSidebar
            objectName: "workspaceSidebar"
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: backend.workspaceNavFiles
            spacing: 1
            currentIndex: -1
            highlightFollowsCurrentItem: false
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }
            header: Item {
                width: workspaceSidebar.width
                height: backend.workspaceNavCanGoUp ? win.scaledSize(32) : 0
                visible: backend.workspaceNavCanGoUp
                Rectangle {
                    anchors.fill: parent
                    anchors.bottomMargin: 1
                    radius: 6
                    color: "transparent"
                    border.color: win.panelBorderColor
                }
                MouseArea {
                    anchors.fill: parent
                    enabled: backend.workspaceNavCanGoUp
                    onClicked: backend.selectParentWorkspaceFolder()
                }
                Row {
                    anchors.fill: parent
                    anchors.leftMargin: 6
                    anchors.rightMargin: 6
                    spacing: 6
                    Label {
                        height: parent.height
                        text: "←"
                        color: win.mutedColor
                        font.family: "iA Writer Mono S"
                        font.pixelSize: win.scaledSize(12)
                        verticalAlignment: Text.AlignVCenter
                    }
                    Label {
                        height: parent.height
                        text: (backend.uiLanguage, backend.t("navGoUp"))
                        color: win.strongTextColor
                        font.family: "iA Writer Mono S"
                        font.pixelSize: win.scaledSize(12)
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }

            delegate: ItemDelegate {
                id: fileDelegate
                required property var modelData
                readonly property var fileEntry: modelData
                readonly property bool isFolder:
                    fileEntry && fileEntry.kind === "folder"
                readonly property bool isMarkdown:
                    fileEntry && fileEntry.kind === "markdown"
                readonly property bool currentFile:
                    fileEntry && isMarkdown
                    && backend.fileUrl.toString() === fileEntry.url.toString()
                readonly property bool renaming:
                    fileEntry && win.renamingUrl.toString() === fileEntry.url.toString()
                width: ListView.view ? ListView.view.width : parent.width
                text: ""
                implicitHeight: win.scaledSize(32)
                highlighted: currentFile
                Drag.dragType: Drag.Automatic
                Drag.supportedActions: Qt.CopyAction
                Drag.mimeData: fileEntry
                    ? { "text/uri-list": fileEntry.url.toString() }
                    : {}
                Drag.active: fileDrag.active

                DragHandler {
                    id: fileDrag
                    enabled: !fileDelegate.renaming && !fileDelegate.isFolder
                             && backend.projectView === "mindmap"
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
                        if (fileDelegate.isFolder) {
                            backend.selectedWorkspacePath = fileDelegate.fileEntry.path
                            return
                        }
                        if (fileDelegate.isMarkdown) {
                            if (!fileDelegate.currentFile)
                                win.requestOpen(fileDelegate.fileEntry.url)
                            return
                        }
                        backend.openLocalFile(fileDelegate.fileEntry.url)
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 6
                    anchors.rightMargin: 6
                    spacing: 6
                    visible: !fileDelegate.renaming

                    FileKindIcon {
                        Layout.preferredWidth: win.scaledSize(16)
                        Layout.preferredHeight: win.scaledSize(16)
                        kind: fileDelegate.fileEntry && fileDelegate.fileEntry.kind
                              ? fileDelegate.fileEntry.kind : "file"
                        ink: fileDelegate.highlighted ? win.strongTextColor : win.mutedColor
                    }

                    Label {
                        id: fileNameLabel
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        text: fileDelegate.fileEntry
                              ? ((fileDelegate.fileEntry.pinned ? "★ " : "")
                                 + fileDelegate.fileEntry.name)
                              : ""
                        elide: Text.ElideRight
                        color: win.strongTextColor
                        font.family: "iA Writer Mono S"
                        font.pixelSize: win.scaledSize(12)
                        verticalAlignment: Text.AlignVCenter
                        HoverHandler { id: fileNameHover }
                        ToolTip.visible: fileNameHover.hovered && fileNameLabel.truncated
                        ToolTip.text: fileDelegate.fileEntry ? fileDelegate.fileEntry.name : ""
                        ToolTip.delay: 350
                    }

                    Label {
                        visible: fileDelegate.isFolder
                        Layout.preferredWidth: win.scaledSize(14)
                        Layout.fillHeight: true
                        text: "›"
                        color: win.mutedColor
                        font.family: "iA Writer Mono S"
                        font.pixelSize: win.scaledSize(14)
                        horizontalAlignment: Text.AlignRight
                        verticalAlignment: Text.AlignVCenter
                    }

                    Label {
                        visible: fileDelegate.fileEntry && !fileDelegate.isFolder
                                 && !fileDelegate.isMarkdown
                                 && fileDelegate.fileEntry.suffix
                        Layout.preferredWidth: win.scaledSize(28)
                        Layout.fillHeight: true
                        text: fileDelegate.fileEntry && fileDelegate.fileEntry.suffix
                              ? fileDelegate.fileEntry.suffix : ""
                        color: win.mutedColor
                        font.family: "iA Writer Mono S"
                        font.pixelSize: win.scaledSize(9)
                        horizontalAlignment: Text.AlignRight
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
                        text = fileDelegate.isFolder
                               ? fileDelegate.fileEntry.name
                               : win.noteStem(fileDelegate.fileEntry.name)
                        forceActiveFocus()
                        selectAll()
                    }
                    function commitRename() {
                        if (!fileDelegate.fileEntry)
                            return
                        if (fileDelegate.isFolder)
                            backend.renameFolder(fileDelegate.fileEntry.url, text)
                        else
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
                        visible: !fileDelegate.isFolder
                        text: (backend.uiLanguage, backend.t("openNote"))
                        onTriggered: if (fileDelegate.fileEntry) {
                            if (fileDelegate.isMarkdown)
                                win.requestOpen(fileDelegate.fileEntry.url)
                            else
                                backend.openLocalFile(fileDelegate.fileEntry.url)
                        }
                    }
                    MenuItem {
                        text: fileDelegate.isFolder
                              ? (backend.uiLanguage, backend.t("renameFolder"))
                              : (backend.uiLanguage, backend.t("renameNote"))
                        onTriggered: if (fileDelegate.fileEntry)
                            win.startRename(fileDelegate.fileEntry.url)
                    }
                    MenuItem {
                        visible: fileDelegate.isMarkdown
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

            Label {
                Layout.fillWidth: true
                Layout.fillHeight: workspaceSidebar.count === 0
                wrapMode: Text.Wrap
                text: backend.workspaceFolderPath.length > 0
                    ? (backend.uiLanguage, backend.t("navEmptyFolder"))
                    : (backend.uiLanguage, backend.t("openFolderHint"))
                color: win.mutedColor
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(12)
                visible: workspaceSidebar.count === 0
            }
        }
    }
}
