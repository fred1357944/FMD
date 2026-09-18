import QtQuick
import QtQuick.Controls

// Pointer-driven drag overlay, matching tabExtend/dnd-kit:
// the source stays put as a placeholder; a floating copy follows the cursor
// from the grab point; drop is hit-tested in our coordinates, not Qt DnD.
Item {
    id: layer
    anchors.fill: parent
    z: 100
    clip: false

    property bool active: false
    property string noteUrl
    property string noteTitle
    property real grabDX: 0
    property real grabDY: 0
    property real pointerX: 0
    property real pointerY: 0
    property real ghostWidth: 200
    property real ghostHeight: 48
    property var dropTargets: []
    property var hoverTarget: null
    property var edgeScroller: null

    readonly property real activationDistance: 8
    readonly property real edgeScrollPx: 40

    signal dropped(string url, var target)
    signal cancelled()

    function start(layerPos, grab, payload, targets) {
        noteUrl = payload.url
        noteTitle = payload.title
        ghostWidth = payload.width
        ghostHeight = payload.height
        grabDX = grab.x
        grabDY = grab.y
        pointerX = layerPos.x
        pointerY = layerPos.y
        dropTargets = targets
        hoverTarget = null
        active = true
        updateHover()
        scrollTimer.start()
    }

    function dragTo(layerPos) {
        if (!active)
            return
        pointerX = layerPos.x
        pointerY = layerPos.y
        updateHover()
    }

    function finish() {
        scrollTimer.stop()
        if (!active)
            return
        const target = hoverTarget
        const url = noteUrl
        active = false
        hoverTarget = null
        if (target)
            dropped(url, target)
        else
            cancelled()
    }

    function updateHover() {
        var hit = null
        for (var i = 0; i < dropTargets.length; ++i) {
            var item = dropTargets[i]
            if (!item || item.width <= 0 || item.height <= 0)
                continue
            var p = item.mapFromItem(layer, pointerX, pointerY)
            if (p.x >= 0 && p.y >= 0 && p.x < item.width && p.y < item.height)
                hit = item
        }
        hoverTarget = hit
    }

    Timer {
        id: scrollTimer
        interval: 16
        repeat: true
        onTriggered: {
            if (!layer.active || !layer.edgeScroller)
                return
            var scroller = layer.edgeScroller
            var local = scroller.mapFromItem(layer, layer.pointerX, layer.pointerY)
            var step = 18
            if (local.x < layer.edgeScrollPx && scroller.contentX > 0)
                scroller.contentX = Math.max(0, scroller.contentX - step)
            else if (local.x > scroller.width - layer.edgeScrollPx)
                scroller.contentX = Math.min(Math.max(0, scroller.contentWidth - scroller.width),
                                             scroller.contentX + step)
            if (scroller.contentHeight > scroller.height) {
                if (local.y < layer.edgeScrollPx && scroller.contentY > 0)
                    scroller.contentY = Math.max(0, scroller.contentY - step)
                else if (local.y > scroller.height - layer.edgeScrollPx)
                    scroller.contentY = Math.min(Math.max(0, scroller.contentHeight - scroller.height),
                                                 scroller.contentY + step)
            }
            layer.updateHover()
        }
    }

    Rectangle {
        id: ghost
        visible: layer.active
        width: layer.ghostWidth
        height: layer.ghostHeight
        x: layer.pointerX - layer.grabDX
        y: layer.pointerY - layer.grabDY
        radius: 8
        color: win.panelColor
        border.color: backend.themeAccent
        border.width: 1
        opacity: 0.94
        scale: 1.03
        z: 2

        Rectangle {
            anchors.fill: parent
            anchors.topMargin: 4
            z: -1
            radius: 8
            color: "#000000"
            opacity: 0.28
        }

        Label {
            anchors.fill: parent
            anchors.margins: 8
            text: layer.noteTitle
            wrapMode: Text.Wrap
            elide: Text.ElideRight
            color: win.strongTextColor
            font.family: "iA Writer Mono S"
            font.pixelSize: win.scaledSize(12)
            verticalAlignment: Text.AlignVCenter
        }
    }
}
