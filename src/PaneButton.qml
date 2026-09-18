import QtQuick
import QtQuick.Controls

Button {
    id: control

    implicitHeight: win.scaledSize(28)
    implicitWidth: Math.max(win.scaledSize(52), label.implicitWidth + leftPadding + rightPadding)
    leftPadding: win.scaledSize(12)
    rightPadding: win.scaledSize(12)
    topPadding: win.scaledSize(4)
    bottomPadding: win.scaledSize(4)
    font.family: "iA Writer Mono S"
    font.pixelSize: win.scaledSize(11)

    contentItem: Text {
        id: label
        text: control.text
        font: control.font
        color: win.strongTextColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        opacity: control.enabled ? 1 : 0.4
    }

    background: Rectangle {
        radius: 6
        color: control.down ? win.currentFileColor
                            : (control.hovered ? win.currentFileColor : win.panelColor)
        border.width: 1
        border.color: control.visualFocus || control.hovered
                      ? backend.themeAccent : win.panelBorderColor
    }
}
