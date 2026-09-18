import QtQuick

Item {
    id: root
    property string kind: "file"
    property color ink: "#6b5d4d"
    width: 16
    height: 16

    Canvas {
        id: canvas
        anchors.fill: parent
        antialiasing: true
        onPaint: {
            const ctx = getContext("2d")
            const w = width
            const h = height
            ctx.clearRect(0, 0, w, h)
            ctx.strokeStyle = root.ink
            ctx.fillStyle = root.ink
            ctx.lineWidth = Math.max(1.1, w / 12)
            ctx.lineCap = "round"
            ctx.lineJoin = "round"
            const k = root.kind
            if (k === "folder") {
                ctx.beginPath()
                ctx.moveTo(w * 0.12, h * 0.38)
                ctx.lineTo(w * 0.12, h * 0.82)
                ctx.quadraticCurveTo(w * 0.12, h * 0.9, w * 0.22, h * 0.9)
                ctx.lineTo(w * 0.78, h * 0.9)
                ctx.quadraticCurveTo(w * 0.88, h * 0.9, w * 0.88, h * 0.82)
                ctx.lineTo(w * 0.88, h * 0.42)
                ctx.quadraticCurveTo(w * 0.88, h * 0.34, w * 0.78, h * 0.34)
                ctx.lineTo(w * 0.46, h * 0.34)
                ctx.lineTo(w * 0.38, h * 0.22)
                ctx.lineTo(w * 0.22, h * 0.22)
                ctx.quadraticCurveTo(w * 0.12, h * 0.22, w * 0.12, h * 0.3)
                ctx.closePath()
                ctx.globalAlpha = 0.18
                ctx.fill()
                ctx.globalAlpha = 1
                ctx.stroke()
            } else if (k === "markdown") {
                roundedPage(ctx, w, h)
                ctx.beginPath()
                ctx.moveTo(w * 0.32, h * 0.42)
                ctx.lineTo(w * 0.68, h * 0.42)
                ctx.moveTo(w * 0.32, h * 0.56)
                ctx.lineTo(w * 0.62, h * 0.56)
                ctx.moveTo(w * 0.32, h * 0.7)
                ctx.lineTo(w * 0.52, h * 0.7)
                ctx.stroke()
            } else if (k === "pdf") {
                roundedPage(ctx, w, h)
                ctx.font = "bold " + Math.floor(h * 0.34) + "px 'iA Writer Mono S', sans-serif"
                ctx.textAlign = "center"
                ctx.textBaseline = "middle"
                ctx.fillText("P", w * 0.5, h * 0.58)
            } else if (k === "image") {
                ctx.beginPath()
                ctx.moveTo(w * 0.18, h * 0.28)
                ctx.lineTo(w * 0.82, h * 0.28)
                ctx.lineTo(w * 0.82, h * 0.78)
                ctx.lineTo(w * 0.18, h * 0.78)
                ctx.closePath()
                ctx.stroke()
                ctx.beginPath()
                ctx.arc(w * 0.34, h * 0.44, w * 0.07, 0, Math.PI * 2)
                ctx.fill()
                ctx.beginPath()
                ctx.moveTo(w * 0.22, h * 0.78)
                ctx.lineTo(w * 0.42, h * 0.52)
                ctx.lineTo(w * 0.56, h * 0.66)
                ctx.lineTo(w * 0.7, h * 0.48)
                ctx.lineTo(w * 0.82, h * 0.78)
                ctx.stroke()
            } else if (k === "code") {
                ctx.beginPath()
                ctx.moveTo(w * 0.38, h * 0.28)
                ctx.lineTo(w * 0.22, h * 0.5)
                ctx.lineTo(w * 0.38, h * 0.72)
                ctx.moveTo(w * 0.62, h * 0.28)
                ctx.lineTo(w * 0.78, h * 0.5)
                ctx.lineTo(w * 0.62, h * 0.72)
                ctx.stroke()
            } else {
                roundedPage(ctx, w, h)
            }
        }

        function roundedPage(ctx, w, h) {
            ctx.beginPath()
            ctx.moveTo(w * 0.28, h * 0.16)
            ctx.lineTo(w * 0.62, h * 0.16)
            ctx.lineTo(w * 0.78, h * 0.32)
            ctx.lineTo(w * 0.78, h * 0.84)
            ctx.lineTo(w * 0.22, h * 0.84)
            ctx.lineTo(w * 0.22, h * 0.16)
            ctx.closePath()
            ctx.stroke()
            ctx.beginPath()
            ctx.moveTo(w * 0.62, h * 0.16)
            ctx.lineTo(w * 0.62, h * 0.32)
            ctx.lineTo(w * 0.78, h * 0.32)
            ctx.stroke()
        }
    }

    onKindChanged: canvas.requestPaint()
    onInkChanged: canvas.requestPaint()
    onWidthChanged: canvas.requestPaint()
    onHeightChanged: canvas.requestPaint()
}
