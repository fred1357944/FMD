import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    objectName: "mindmapView"
    focus: visible

    property string editingId: ""
    property string dragId: ""
    property real dragX: 0
    property real dragY: 0
    property bool dragMoved: false
    property real ghostX: 0
    property real ghostY: 0
    property real ghostW: 120
    property real ghostH: 36
    property real dragOriginX: 0
    property real dragOriginY: 0
    property real lockContentX: 0
    property real lockContentY: 0
    property string dragParentId: ""
    property string dropTargetId: ""
    property bool ignoreNextClick: false
    property var activeEditor: null
    property var linkHits: []
    property int linkIndex: 0
    readonly property bool linkPopupOpen: linkHits.length > 0 && editingId.length > 0

    readonly property var layout: {
        var _ = backend.mindmapStamp
        return backend.mindmapLayout
    }
    readonly property var outline: {
        var _ = backend.mindmapStamp
        return backend.mindmapOutline
    }
    readonly property var nodes: layout.nodes || []
    readonly property var edges: layout.edges || []

    function addChildAndEdit() {
        backend.mindmapAddChild(backend.mindmapSelectedId)
        beginEdit(backend.mindmapSelectedId)
    }

    function addSiblingAndEdit() {
        backend.mindmapAddSibling(backend.mindmapSelectedId)
        beginEdit(backend.mindmapSelectedId)
    }

    function nodeById(id) {
        for (var i = 0; i < nodes.length; ++i) {
            if (nodes[i].id === id)
                return nodes[i]
        }
        return null
    }

    function isDescendantTarget(drag, target) {
        if (!drag || !target)
            return false
        return target === drag || String(target).indexOf(String(drag) + ".") === 0
    }

    function inDragBranch(id) {
        return root.dragMoved && root.dragId.length > 0
               && root.isDescendantTarget(root.dragId, id)
    }

    function dragOffsetX() {
        return root.dragMoved ? (root.ghostX - root.dragOriginX) : 0
    }

    function dragOffsetY() {
        return root.dragMoved ? (root.ghostY - root.dragOriginY) : 0
    }

    function visualX(node) {
        if (!node)
            return 0
        return node.x + (root.inDragBranch(node.id) ? root.dragOffsetX() : 0)
    }

    function visualY(node) {
        if (!node)
            return 0
        return node.y + (root.inDragBranch(node.id) ? root.dragOffsetY() : 0)
    }

    function nodeAt(px, py, skipId) {
        skipId = skipId || ""
        for (var i = nodes.length - 1; i >= 0; --i) {
            var n = nodes[i]
            if (skipId.length > 0 && root.isDescendantTarget(skipId, n.id))
                continue
            var nx = root.visualX(n)
            var ny = root.visualY(n)
            if (px >= nx && px <= nx + n.w && py >= ny && py <= ny + n.h)
                return n.id
        }
        return ""
    }

    function dropTargetAt(px, py) {
        var target = nodeAt(px, py, root.dragId)
        if (target.length === 0 || root.isDescendantTarget(root.dragId, target))
            return ""
        return target
    }

    function beginDrag(node, localX, localY) {
        root.finishEdit()
        root.forceActiveFocus()
        backend.mindmapSelectedId = node.id
        root.lockContentX = canvasFlick.contentX
        root.lockContentY = canvasFlick.contentY
        canvasFlick.cancelFlick()
        root.dragId = node.id
        root.dragParentId = node.parentId || ""
        root.dragX = localX
        root.dragY = localY
        root.dragMoved = false
        root.dropTargetId = ""
        root.dragOriginX = node.x
        root.dragOriginY = node.y
        root.ghostX = node.x
        root.ghostY = node.y
        root.ghostW = node.w
        root.ghostH = node.h
    }

    function updateDrag(boardX, boardY) {
        if (root.dragId.length === 0)
            return
        if (Math.hypot(boardX - (root.dragOriginX + root.dragX),
                       boardY - (root.dragOriginY + root.dragY)) > 4)
            root.dragMoved = true
        if (!root.dragMoved)
            return
        root.ghostX = boardX - root.dragX
        root.ghostY = boardY - root.dragY
        root.dropTargetId = root.dropTargetAt(boardX, boardY)
        edgeCanvas.requestPaint()
    }

    function finishDrag(boardX, boardY) {
        if (root.dragId.length === 0)
            return
        var target = root.dropTargetAt(boardX, boardY)
        var moving = root.dragId
        var moved = root.dragMoved
        root.ignoreNextClick = moved
        root.clearDrag()
        if (moved && moving.length > 0 && target.length > 0)
            backend.mindmapReparent(moving, target, -1)
        edgeCanvas.requestPaint()
    }

    function nodeFill(node) {
        if (!node)
            return win.darkMode ? "#2a2a2a" : "#f4f4f4"
        if (node.color && String(node.color).length > 0)
            return node.color
        if (node.id === backend.mindmapSelectedId)
            return backend.themeAccent
        return win.darkMode ? "#2a2a2a" : "#f4f4f4"
    }

    function usesLightText(node) {
        if (!node)
            return false
        if (node.color && String(node.color).length > 0)
            return true
        return node.id === backend.mindmapSelectedId
    }

    function clearDrag() {
        root.dragId = ""
        root.dragMoved = false
        root.dropTargetId = ""
        root.dragParentId = ""
    }

    function beginEdit(id) {
        backend.mindmapSelectedId = id
        root.editingId = id
    }

    function scrollToNode(id) {
        var n = root.nodeById(id)
        if (!n || !canvasFlick)
            return
        var targetX = n.x + n.w / 2 - canvasFlick.width / 2
        var targetY = n.y + n.h / 2 - canvasFlick.height / 2
        var maxX = Math.max(0, canvasFlick.contentWidth - canvasFlick.width)
        var maxY = Math.max(0, canvasFlick.contentHeight - canvasFlick.height)
        canvasFlick.contentX = Math.max(0, Math.min(targetX, maxX))
        canvasFlick.contentY = Math.max(0, Math.min(targetY, maxY))
    }

    function selectAndReveal(id) {
        root.finishEdit()
        backend.mindmapSelectedId = id
        Qt.callLater(function() { root.scrollToNode(id) })
    }

    function commitEdit(id, text, restoreCanvasFocus) {
        if (root.editingId !== id)
            return
        root.editingId = ""
        root.linkHits = []
        root.activeEditor = null
        backend.mindmapRename(id, text)
        if (restoreCanvasFocus)
            root.forceActiveFocus()
    }

    function finishEdit() {
        if (root.editingId.length === 0)
            return
        var text = root.activeEditor ? root.activeEditor.text : ""
        root.commitEdit(root.editingId, text, true)
    }

    function isFollowable(node) {
        if (!node)
            return false
        var kind = node.kind || ""
        if (kind === "plain" || kind.length === 0)
            return false
        var url = node.resolved && node.resolved.length > 0
                  ? node.resolved
                  : backend.resolveNodeTarget(node.target || "")
        return !!(url && url.toString().length > 0)
    }

    function updateLinkPopup(editor) {
        if (!editor || root.editingId.length === 0) {
            root.linkHits = []
            return
        }
        var query = backend.linkQueryAt(editor.text, editor.cursorPosition)
        if (!query.active) {
            root.linkHits = []
            return
        }
        root.linkHits = backend.linkSuggestions(query.query, query.imagesPreferred)
        root.linkIndex = 0
    }

    function applyLinkSuggestion(item) {
        var editor = root.activeEditor
        if (!editor || !item || !item.insert)
            return false
        var query = backend.linkQueryAt(editor.text, editor.cursorPosition)
        if (!query.active)
            return false
        var text = editor.text
        editor.text = text.slice(0, query.start) + item.insert + text.slice(query.end)
        editor.cursorPosition = query.start + item.insert.length
        root.linkHits = []
        return true
    }

    function openNodeLink(node) {
        if (!node)
            return
        var url = node.resolved && node.resolved.length > 0
                  ? node.resolved
                  : backend.resolveNodeTarget(node.target || "")
        if (!url || url.toString().length === 0)
            return
        if (node.isImage)
            backend.openExternalUrl(url)
        else
            win.requestOpen(url, node.fragmentKind || "", node.fragment || "")
    }

    Keys.onPressed: function(event) {
        if (root.editingId.length > 0)
            return
        if (event.key === Qt.Key_Backspace || event.key === Qt.Key_Delete) {
            backend.mindmapRemove(backend.mindmapSelectedId)
            event.accepted = true
            return
        }
        if (event.key === Qt.Key_Left) {
            backend.mindmapSetCollapsed(backend.mindmapSelectedId, true)
            event.accepted = true
            return
        }
        if (event.key === Qt.Key_Right) {
            backend.mindmapSetCollapsed(backend.mindmapSelectedId, false)
            event.accepted = true
        }
    }

    Timer {
        id: openDelay
        interval: 280
        repeat: false
        property string nodeId: ""
        onTriggered: {
            var node = root.nodeById(nodeId)
            if (node)
                root.openNodeLink(node)
        }
    }

    Shortcut {
        sequence: "Tab"
        enabled: root.visible && root.editingId.length === 0
        context: Qt.WindowShortcut
        onActivated: root.addChildAndEdit()
    }
    Shortcut {
        sequence: "Return"
        enabled: root.visible && root.editingId.length === 0
        context: Qt.WindowShortcut
        onActivated: root.addSiblingAndEdit()
    }
    Shortcut {
        sequence: "Enter"
        enabled: root.visible && root.editingId.length === 0
        context: Qt.WindowShortcut
        onActivated: root.addSiblingAndEdit()
    }
    Shortcut {
        sequence: "Shift+Tab"
        enabled: root.visible && root.editingId.length === 0
        context: Qt.WindowShortcut
        onActivated: backend.mindmapOutdent(backend.mindmapSelectedId)
    }
    Shortcut {
        sequence: "F2"
        enabled: root.visible && root.editingId.length === 0
        context: Qt.WindowShortcut
        onActivated: root.beginEdit(backend.mindmapSelectedId)
    }
    Shortcut {
        sequences: ["Backspace", "Delete"]
        enabled: root.visible && root.editingId.length === 0
        context: Qt.WindowShortcut
        onActivated: backend.mindmapRemove(backend.mindmapSelectedId)
    }
    Shortcut {
        sequence: "Left"
        enabled: root.visible && root.editingId.length === 0
        context: Qt.WindowShortcut
        onActivated: backend.mindmapSetCollapsed(backend.mindmapSelectedId, true)
    }
    Shortcut {
        sequence: "Right"
        enabled: root.visible && root.editingId.length === 0
        context: Qt.WindowShortcut
        onActivated: backend.mindmapSetCollapsed(backend.mindmapSelectedId, false)
    }

    SplitView {
        anchors.fill: parent
        anchors.margins: 8
        orientation: Qt.Horizontal

        Rectangle {
            SplitView.preferredWidth: win.scaledSize(260)
            SplitView.minimumWidth: win.scaledSize(180)
            color: "transparent"

            ColumnLayout {
                anchors.fill: parent
                spacing: 6

                Label {
                    Layout.fillWidth: true
                    wrapMode: Text.Wrap
                    color: win.mutedColor
                    font.family: "iA Writer Mono S"
                    font.pixelSize: win.scaledSize(11)
                    text: (backend.uiLanguage, backend.t("mindmapHelp"))
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    ToolButton {
                        objectName: "mindmapAddChildButton"
                        activeFocusOnTab: false
                        text: (backend.uiLanguage, backend.t("mindmapAddChild"))
                        font.family: "iA Writer Mono S"
                        font.pixelSize: win.scaledSize(11)
                        onClicked: root.addChildAndEdit()
                    }
                    ToolButton {
                        activeFocusOnTab: false
                        text: (backend.uiLanguage, backend.t("mindmapAddSibling"))
                        font.family: "iA Writer Mono S"
                        font.pixelSize: win.scaledSize(11)
                        onClicked: root.addSiblingAndEdit()
                    }
                    ToolButton {
                        activeFocusOnTab: false
                        text: (backend.uiLanguage, backend.t("mindmapDelete"))
                        font.family: "iA Writer Mono S"
                        font.pixelSize: win.scaledSize(11)
                        onClicked: backend.mindmapRemove(backend.mindmapSelectedId)
                    }
                }

                Row {
                    id: colorPalette
                    objectName: "mindmapColorPalette"
                    Layout.fillWidth: true
                    spacing: 6
                    Repeater {
                        model: backend.mindmapPalette()
                        delegate: Rectangle {
                            required property var modelData
                            width: win.scaledSize(16)
                            height: win.scaledSize(16)
                            radius: width / 2
                            color: modelData && String(modelData).length > 0
                                   ? modelData
                                   : (win.darkMode ? "#2a2a2a" : "#f4f4f4")
                            border.color: {
                                var node = root.nodeById(backend.mindmapSelectedId)
                                var current = node && node.color ? String(node.color) : ""
                                var swatch = modelData ? String(modelData) : ""
                                return current.toLowerCase() === swatch.toLowerCase()
                                       ? backend.themeAccent
                                       : win.panelBorderColor
                            }
                            border.width: 2

                            Text {
                                anchors.centerIn: parent
                                visible: !modelData || String(modelData).length === 0
                                text: "×"
                                color: win.mutedColor
                                font.pixelSize: win.scaledSize(10)
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: backend.mindmapSetColor(
                                               backend.mindmapSelectedId,
                                               modelData ? String(modelData) : "")
                            }
                        }
                    }
                }

                ListView {
                    id: outlineList
                    objectName: "mindmapOutline"
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: root.outline
                    spacing: 1
                    boundsBehavior: Flickable.StopAtBounds
                    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                    delegate: Item {
                        required property var modelData
                        width: ListView.view ? ListView.view.width : parent.width
                        height: win.scaledSize(28)

                        readonly property bool current: modelData.id === backend.mindmapSelectedId

                        Rectangle {
                            anchors.fill: parent
                            radius: 6
                            color: current ? win.currentFileColor
                                           : (hover.hovered ? Qt.rgba(1, 1, 1, 0.04) : "transparent")
                        }

                        Rectangle {
                            visible: modelData.color && String(modelData.color).length > 0
                            width: 6
                            height: parent.height - 10
                            radius: 3
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.left: parent.left
                            anchors.leftMargin: 4
                            color: modelData.color || "transparent"
                        }

                        Label {
                            id: outlineChevron
                            anchors.verticalCenter: parent.verticalCenter
                            x: 10 + modelData.depth * 12
                            width: win.scaledSize(14)
                            text: modelData.collapsible
                                  ? (modelData.collapsed
                                     ? "▸" + (modelData.childCount > 0 ? " " + modelData.childCount : "")
                                     : "▾")
                                  : ""
                            color: win.mutedColor
                            font.family: "iA Writer Mono S"
                            font.pixelSize: win.scaledSize(11)

                            MouseArea {
                                anchors.fill: parent
                                enabled: modelData.collapsible
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    root.finishEdit()
                                    backend.mindmapSelectedId = modelData.id
                                    backend.mindmapToggleCollapse(modelData.id)
                                }
                            }
                        }

                        Label {
                            anchors.fill: parent
                            anchors.leftMargin: 26 + modelData.depth * 12
                            anchors.rightMargin: 8
                            text: String(modelData.display || modelData.title || "").replace(/\n/g, " · ")
                            elide: Text.ElideRight
                            verticalAlignment: Text.AlignVCenter
                            color: win.strongTextColor
                            font.family: "iA Writer Mono S"
                            font.pixelSize: win.scaledSize(12)
                            font.bold: modelData.depth === 0
                        }

                        HoverHandler { id: hover }
                        TapHandler {
                            onTapped: root.selectAndReveal(modelData.id)
                            onDoubleTapped: {
                                openDelay.stop()
                                root.beginEdit(modelData.id)
                            }
                        }
                    }
                }
            }
        }

        Item {
            SplitView.fillWidth: true
            objectName: "mindmapCanvasHost"

            Flickable {
                id: canvasFlick
                objectName: "mindmapCanvas"
                anchors.fill: parent
                clip: true
                interactive: root.dragId.length === 0
                contentWidth: Math.max(width, (layout.width || 0) + 80)
                contentHeight: Math.max(height, (layout.height || 0) + 80)
                boundsBehavior: Flickable.StopAtBounds
                maximumFlickVelocity: root.dragId.length === 0 ? 2500 : 0
                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                    interactive: root.dragId.length === 0
                }
                ScrollBar.horizontal: ScrollBar {
                    policy: ScrollBar.AsNeeded
                    interactive: root.dragId.length === 0
                }

                Item {
                    id: board
                    width: canvasFlick.contentWidth
                    height: canvasFlick.contentHeight

                    MouseArea {
                        anchors.fill: parent
                        z: 0
                        acceptedButtons: Qt.LeftButton
                        onPressed: function(mouse) {
                            if (root.nodeAt(mouse.x, mouse.y).length === 0)
                                root.finishEdit()
                            mouse.accepted = false
                        }
                    }

                    DropArea {
                        anchors.fill: parent
                        keys: ["text/uri-list"]
                        onDropped: function(drop) {
                            if (!drop.hasUrls || drop.urls.length === 0)
                                return
                            backend.mindmapAttachUrl(root.nodeAt(drop.x, drop.y), drop.urls[0])
                            drop.acceptProposedAction()
                        }
                    }

                    Canvas {
                        id: edgeCanvas
                        anchors.fill: parent
                        onPaint: {
                            var ctx = getContext("2d")
                            ctx.clearRect(0, 0, width, height)
                            ctx.lineCap = "round"
                            ctx.lineJoin = "round"

                            function curve(x1, y1, x2, y2, color, width, dashed) {
                                ctx.strokeStyle = color
                                ctx.lineWidth = width
                                ctx.setLineDash(dashed ? [8, 6] : [])
                                var span = Math.max(36, Math.abs(x2 - x1) * 0.45)
                                ctx.beginPath()
                                ctx.moveTo(x1, y1)
                                ctx.bezierCurveTo(x1 + span, y1, x2 - span, y2, x2, y2)
                                ctx.stroke()
                            }

                            var ox = root.dragOffsetX()
                            var oy = root.dragOffsetY()
                            for (var e = 0; e < root.edges.length; ++e) {
                                var edge = root.edges[e]
                                if (root.dragMoved && edge.to === root.dragId)
                                    continue
                                var x1 = Number(edge.x1)
                                var y1 = Number(edge.y1)
                                var x2 = Number(edge.x2)
                                var y2 = Number(edge.y2)
                                if (root.inDragBranch(edge.from)) {
                                    x1 += ox
                                    y1 += oy
                                }
                                if (root.inDragBranch(edge.to)) {
                                    x2 += ox
                                    y2 += oy
                                }
                                curve(x1, y1, x2, y2, win.panelBorderColor, 1.5, false)
                            }

                            if (root.dragMoved && root.dragId.length > 0) {
                                var fromId = root.dropTargetId.length > 0
                                             ? root.dropTargetId : root.dragParentId
                                var fromNode = root.nodeById(fromId)
                                if (fromNode) {
                                    var fx = root.visualX(fromNode) + fromNode.w
                                    var fy = root.visualY(fromNode) + fromNode.h / 2
                                    var tx = root.ghostX
                                    var ty = root.ghostY + root.ghostH / 2
                                    curve(fx, fy, tx, ty, backend.themeAccent, 2.2, true)
                                }
                            }
                            ctx.setLineDash([])
                        }
                    }

                    Repeater {
                        model: root.nodes
                        delegate: Rectangle {
                            required property var modelData
                            readonly property bool dragging: root.inDragBranch(modelData.id)
                            z: dragging ? 20 : (modelData.id === root.dropTargetId ? 8 : 1)
                            x: modelData.x + (dragging ? root.dragOffsetX() : 0)
                            y: modelData.y + (dragging ? root.dragOffsetY() : 0)
                            width: modelData.w
                            height: {
                                if (root.editingId === modelData.id && titleEdit.contentHeight > 0)
                                    return Math.max(modelData.h, titleEdit.contentHeight + 16)
                                return modelData.h
                            }
                            radius: Math.min(18, height / 2)
                            opacity: dragging ? 0.92 : 1
                            scale: dragging && modelData.id === root.dragId ? 1.03 : 1
                            color: root.nodeFill(modelData)
                            border.color: modelData.id === root.dropTargetId
                                          ? backend.themeAccent
                                          : (modelData.id === backend.mindmapSelectedId
                                             ? backend.themeAccent
                                             : win.panelBorderColor)
                            border.width: modelData.id === root.dropTargetId ? 2 : 1

                            Rectangle {
                                visible: modelData.id === root.dropTargetId
                                anchors.fill: parent
                                anchors.margins: -5
                                radius: parent.radius + 5
                                color: "transparent"
                                border.color: backend.themeAccent
                                border.width: 2
                                z: -1
                            }

                            TextEdit {
                                id: titleEdit
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: modelData.collapsible ? 22 : 12
                                anchors.topMargin: 8
                                anchors.bottomMargin: 8
                                visible: root.editingId === modelData.id
                                activeFocusOnTab: false
                                text: modelData.display || modelData.title
                                color: root.usesLightText(modelData) ? "#ffffff" : win.strongTextColor
                                selectedTextColor: "#ffffff"
                                selectionColor: Qt.darker(backend.themeAccent, 1.35)
                                font.family: "iA Writer Mono S"
                                font.pixelSize: win.scaledSize(12)
                                wrapMode: TextEdit.Wrap
                                activeFocusOnPress: visible
                                clip: true
                                onVisibleChanged: if (visible) {
                                    text = modelData.display || modelData.title
                                    root.activeEditor = titleEdit
                                    forceActiveFocus()
                                    selectAll()
                                    root.updateLinkPopup(titleEdit)
                                } else if (root.activeEditor === titleEdit) {
                                    root.activeEditor = null
                                    root.linkHits = []
                                }
                                onTextChanged: root.updateLinkPopup(titleEdit)
                                onCursorPositionChanged: root.updateLinkPopup(titleEdit)
                                onActiveFocusChanged: {
                                    if (!activeFocus && root.editingId === modelData.id)
                                        root.commitEdit(modelData.id, text, false)
                                }
                                Keys.onShortcutOverride: function(event) {
                                    if (event.key === Qt.Key_Tab || event.key === Qt.Key_Backtab
                                            || event.key === Qt.Key_Return
                                            || event.key === Qt.Key_Enter
                                            || event.key === Qt.Key_Escape)
                                        event.accepted = true
                                }
                                Keys.onPressed: function(event) {
                                    if (root.linkPopupOpen) {
                                        if (event.key === Qt.Key_Down) {
                                            root.linkIndex = Math.min(root.linkIndex + 1, root.linkHits.length - 1)
                                            event.accepted = true
                                            return
                                        }
                                        if (event.key === Qt.Key_Up) {
                                            root.linkIndex = Math.max(root.linkIndex - 1, 0)
                                            event.accepted = true
                                            return
                                        }
                                        if (event.key === Qt.Key_Tab || event.key === Qt.Key_Return
                                                || event.key === Qt.Key_Enter) {
                                            if (!(event.modifiers & Qt.ShiftModifier)) {
                                                root.applyLinkSuggestion(root.linkHits[root.linkIndex])
                                                event.accepted = true
                                                return
                                            }
                                        }
                                        if (event.key === Qt.Key_Escape) {
                                            root.linkHits = []
                                            event.accepted = true
                                            return
                                        }
                                    }
                                    if (event.key === Qt.Key_Tab) {
                                        root.commitEdit(modelData.id, text, false)
                                        root.addChildAndEdit()
                                        event.accepted = true
                                        return
                                    }
                                    if (event.key === Qt.Key_Backtab) {
                                        root.commitEdit(modelData.id, text, true)
                                        backend.mindmapOutdent(backend.mindmapSelectedId)
                                        event.accepted = true
                                        return
                                    }
                                    if (event.key !== Qt.Key_Return && event.key !== Qt.Key_Enter)
                                        return
                                    if (event.modifiers & Qt.ShiftModifier) {
                                        insert(cursorPosition, "\n")
                                        event.accepted = true
                                        return
                                    }
                                    root.commitEdit(modelData.id, text, true)
                                    event.accepted = true
                                }
                                Keys.onEscapePressed: {
                                    if (root.linkPopupOpen) {
                                        root.linkHits = []
                                        return
                                    }
                                    root.editingId = ""
                                    root.forceActiveFocus()
                                }
                            }

                            Image {
                                anchors.fill: parent
                                anchors.margins: 6
                                visible: root.editingId !== modelData.id && modelData.isImage
                                         && modelData.resolved
                                source: modelData.resolved || ""
                                fillMode: Image.PreserveAspectCrop
                                asynchronous: true
                                cache: true
                            }

                            Label {
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: modelData.collapsible ? 22 : 12
                                visible: root.editingId !== modelData.id && !modelData.isImage
                                text: String(modelData.display || modelData.title || "")
                                wrapMode: Text.Wrap
                                elide: Text.ElideNone
                                color: root.usesLightText(modelData) ? "#ffffff" : win.strongTextColor
                                font.family: "iA Writer Mono S"
                                font.pixelSize: win.scaledSize(12)
                                font.bold: modelData.depth === 0
                                font.underline: modelData.kind === "wiki"
                                                || modelData.kind === "link"
                                                || modelData.kind === "embed"
                                verticalAlignment: Text.AlignVCenter
                                horizontalAlignment: Text.AlignHCenter
                            }

                            MouseArea {
                                anchors.fill: parent
                                enabled: root.editingId !== modelData.id
                                acceptedButtons: Qt.LeftButton
                                preventStealing: true
                                cursorShape: root.dragMoved
                                             ? Qt.ClosedHandCursor
                                             : (root.isFollowable(modelData)
                                                ? Qt.PointingHandCursor : Qt.OpenHandCursor)
                                onPressed: {
                                    if (modelData.depth === 0) {
                                        root.finishEdit()
                                        backend.mindmapSelectedId = modelData.id
                                        return
                                    }
                                    root.ghostW = width
                                    root.ghostH = height
                                    root.beginDrag(modelData, mouse.x, mouse.y)
                                }
                                onPositionChanged: {
                                    var boardPos = mapToItem(board, mouse.x, mouse.y)
                                    root.updateDrag(boardPos.x, boardPos.y)
                                }
                                onReleased: {
                                    var boardPos = mapToItem(board, mouse.x, mouse.y)
                                    root.finishDrag(boardPos.x, boardPos.y)
                                }
                                onCanceled: root.clearDrag()
                                onClicked: function(mouse) {
                                    if (root.ignoreNextClick) {
                                        root.ignoreNextClick = false
                                        return
                                    }
                                    if (mouse.modifiers & (Qt.ControlModifier | Qt.MetaModifier)) {
                                        openDelay.stop()
                                        root.openNodeLink(modelData)
                                        return
                                    }
                                    if (root.isFollowable(modelData)) {
                                        openDelay.nodeId = modelData.id
                                        openDelay.restart()
                                    }
                                }
                                onDoubleClicked: {
                                    openDelay.stop()
                                    root.beginEdit(modelData.id)
                                }
                            }

                            Label {
                                visible: modelData.collapsible && root.editingId !== modelData.id
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                anchors.rightMargin: 6
                                z: 3
                                text: modelData.collapsed
                                      ? "▸" + (modelData.childCount > 0 ? modelData.childCount : "")
                                      : "▾"
                                color: root.usesLightText(modelData) ? "#ffffff" : win.mutedColor
                                font.family: "iA Writer Mono S"
                                font.pixelSize: win.scaledSize(11)

                                MouseArea {
                                    anchors.fill: parent
                                    anchors.margins: -6
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: {
                                        root.finishEdit()
                                        backend.mindmapSelectedId = modelData.id
                                        backend.mindmapToggleCollapse(modelData.id)
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        visible: root.dragMoved && root.dragId.length > 0
                        x: root.ghostX + 5
                        y: root.ghostY + 7
                        width: root.ghostW
                        height: root.ghostH
                        z: 19
                        radius: Math.min(18, height / 2)
                        color: Qt.rgba(0, 0, 0, 0.22)
                    }
                }
            }

            Binding {
                target: canvasFlick
                property: "contentX"
                value: root.lockContentX
                when: root.dragId.length > 0
                restoreMode: Binding.RestoreBindingOrValue
            }
            Binding {
                target: canvasFlick
                property: "contentY"
                value: root.lockContentY
                when: root.dragId.length > 0
                restoreMode: Binding.RestoreBindingOrValue
            }

            WheelHandler {
                target: canvasFlick
                enabled: root.dragId.length > 0
                onWheel: function(event) { event.accepted = true }
            }
        }
    }

    Connections {
        target: backend
        function onMindmapChanged() {
            edgeCanvas.requestPaint()
        }
    }

    onVisibleChanged: if (visible) {
        forceActiveFocus()
        edgeCanvas.requestPaint()
    }
    Component.onCompleted: edgeCanvas.requestPaint()

    Rectangle {
        id: linkPopup
        visible: root.linkPopupOpen
        z: 30
        width: win.scaledSize(260)
        height: Math.min(win.scaledSize(220), 8 + root.linkHits.length * win.scaledSize(32))
        radius: 8
        color: win.darkMode ? "#1a1a1a" : "#ffffff"
        border.color: win.panelBorderColor
        x: {
            var n = root.nodeById(backend.mindmapSelectedId)
            if (!n)
                return 24
            return Math.max(8, Math.min(board.mapToItem(root, n.x, n.y).x, root.width - width - 8))
        }
        y: {
            var n = root.nodeById(backend.mindmapSelectedId)
            if (!n)
                return 24
            var mapped = board.mapToItem(root, n.x, n.y + n.h + 8)
            if (mapped.y + height > root.height - 8)
                return Math.max(8, mapped.y - n.h - height - 16)
            return mapped.y
        }

        ListView {
            anchors.fill: parent
            anchors.margins: 4
            clip: true
            model: root.linkHits
            currentIndex: root.linkIndex
            delegate: ItemDelegate {
                required property var modelData
                required property int index
                width: ListView.view.width
                height: win.scaledSize(32)
                highlighted: index === root.linkIndex
                text: (modelData.kind === "image" ? "🖼 "
                       : (modelData.kind === "heading" ? "# "
                       : (modelData.kind === "block" ? "^ " : "↗ ")))
                      + modelData.title
                      + (modelData.subtitle ? "  " + modelData.subtitle : "")
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(11)
                onClicked: root.applyLinkSuggestion(modelData)
            }
        }
    }
}
