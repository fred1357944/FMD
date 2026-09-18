import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    objectName: "slidePreview"
    focus: true

    readonly property var current: {
        var slides = backend.previewSlides
        if (!slides || slides.length === 0)
            return ({ title: "", body: "", index: 0 })
        var i = Math.max(0, Math.min(backend.previewSlideIndex, slides.length - 1))
        return slides[i]
    }

    Keys.onLeftPressed: backend.stepPreviewSlide(-1)
    Keys.onRightPressed: backend.stepPreviewSlide(1)
    Keys.onUpPressed: backend.stepPreviewSlide(-1)
    Keys.onDownPressed: backend.stepPreviewSlide(1)

    Rectangle {
        anchors.fill: parent
        color: backend.previewCanvas
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 12

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Rectangle {
                id: stage
                width: Math.min(parent.width, parent.height * 16 / 9)
                height: width * 9 / 16
                anchors.centerIn: parent
                radius: 10
                color: backend.previewBackground
                border.color: backend.previewTheme === "night" ? "#2a2723" : "#efeae2"
                border.width: 1

                Column {
                    anchors.fill: parent
                    anchors.margins: 28
                    spacing: 14

                    Label {
                        width: parent.width
                        text: root.current.title || ""
                        color: backend.previewForeground
                        wrapMode: Text.Wrap
                        font.family: "PingFang TC"
                        font.pixelSize: win.scaledSize(26)
                        font.weight: Font.DemiBold
                    }

                    Text {
                        width: parent.width
                        text: root.current.body || ""
                        color: backend.previewForeground
                        wrapMode: Text.Wrap
                        textFormat: Text.MarkdownText
                        font.family: "PingFang TC"
                        font.pixelSize: win.scaledSize(16)
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            ToolButton {
                text: "←"
                enabled: backend.previewSlideIndex > 0
                onClicked: backend.stepPreviewSlide(-1)
            }

            Label {
                text: (backend.previewSlideCount > 0)
                      ? (backend.previewSlideIndex + 1) + " / " + backend.previewSlideCount
                      : "0 / 0"
                color: win.mutedColor
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(11)
            }

            ToolButton {
                text: "→"
                enabled: backend.previewSlideIndex < backend.previewSlideCount - 1
                onClicked: backend.stepPreviewSlide(1)
            }

            Item { Layout.fillWidth: true }

            ToolButton {
                text: (backend.uiLanguage, backend.t("publishSlidevNow"))
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(11)
                onClicked: win.requestPublishSlidev(false)
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        z: -1
        onClicked: root.forceActiveFocus()
    }
}
