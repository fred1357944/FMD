import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: root
    objectName: "propertiesBar"
    implicitHeight: column.implicitHeight
    height: implicitHeight
    clip: true

    property bool addingProperty: false

    Column {
        id: column
        width: parent.width
        spacing: 4

        RowLayout {
            width: parent.width
            spacing: 6

            ToolButton {
                objectName: "propertiesToggle"
                text: backend.propertiesExpanded ? "▾" : "▸"
                implicitWidth: win.scaledSize(28)
                implicitHeight: win.scaledSize(24)
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(12)
                onClicked: backend.propertiesExpanded = !backend.propertiesExpanded
            }

            Label {
                Layout.fillWidth: true
                text: {
                    var title = (backend.uiLanguage, backend.t("properties"))
                    var n = backend.previewProperties.length
                    return n > 0 ? title + "  " + n : title
                }
                color: win.mutedColor
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(11)
                font.bold: true
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: backend.propertiesExpanded = !backend.propertiesExpanded
                }
            }
        }

        Column {
            visible: backend.propertiesExpanded
            width: parent.width
            spacing: 4

            Repeater {
                model: backend.previewProperties
                delegate: RowLayout {
                    required property var modelData
                    width: parent.width
                    spacing: 8
                    readonly property var choices: backend.propertyChoices(modelData.key)

                    Label {
                        Layout.preferredWidth: win.scaledSize(110)
                        text: modelData.key
                        elide: Text.ElideRight
                        color: win.mutedColor
                        font.family: "iA Writer Mono S"
                        font.pixelSize: win.scaledSize(11)
                    }

                    ComboBox {
                        visible: parent.choices.length > 0
                        Layout.fillWidth: true
                        implicitHeight: win.scaledSize(28)
                        model: parent.choices
                        editable: true
                        font.family: "iA Writer Mono S"
                        font.pixelSize: win.scaledSize(11)
                        Component.onCompleted: {
                            var i = -1
                            for (var n = 0; n < model.length; ++n) {
                                if (model[n] === modelData.value)
                                    i = n
                            }
                            currentIndex = i
                            editText = modelData.value
                        }
                        onActivated: backend.setCurrentFrontMatterField(modelData.key, currentText)
                        onAccepted: {
                            if (editText !== modelData.value)
                                backend.setCurrentFrontMatterField(modelData.key, editText)
                        }
                    }

                    TextField {
                        visible: parent.choices.length === 0
                        Layout.fillWidth: true
                        implicitHeight: win.scaledSize(28)
                        text: modelData.value
                        font.family: "iA Writer Mono S"
                        font.pixelSize: win.scaledSize(11)
                        onEditingFinished: {
                            if (text !== modelData.value)
                                backend.setCurrentFrontMatterField(modelData.key, text)
                        }
                    }

                    ToolButton {
                        text: "×"
                        implicitWidth: win.scaledSize(24)
                        implicitHeight: win.scaledSize(24)
                        font.pixelSize: win.scaledSize(12)
                        onClicked: backend.removeCurrentFrontMatterField(modelData.key)
                    }
                }
            }

            ToolButton {
                objectName: "addPropertyButton"
                visible: !root.addingProperty
                text: "+  " + (backend.uiLanguage, backend.t("newProperty"))
                implicitHeight: win.scaledSize(28)
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(11)
                onClicked: {
                    root.addingProperty = true
                    newKeyField.forceActiveFocus()
                }
            }

            RowLayout {
                visible: root.addingProperty
                width: parent.width
                spacing: 8

                ComboBox {
                    id: newKeyField
                    objectName: "newPropertyKey"
                    Layout.preferredWidth: win.scaledSize(110)
                    implicitHeight: win.scaledSize(28)
                    editable: true
                    model: backend.knownPropertyKeys()
                    font.family: "iA Writer Mono S"
                    font.pixelSize: win.scaledSize(11)
                    Keys.onEscapePressed: root.cancelNewProperty()
                    onAccepted: newValueField.forceActiveFocus()
                }

                TextField {
                    id: newValueField
                    objectName: "newPropertyValue"
                    Layout.fillWidth: true
                    implicitHeight: win.scaledSize(28)
                    placeholderText: (backend.uiLanguage, backend.t("propertyValue"))
                    font.family: "iA Writer Mono S"
                    font.pixelSize: win.scaledSize(11)
                    Keys.onEscapePressed: root.cancelNewProperty()
                    onAccepted: root.commitNewProperty()
                }

                ToolButton {
                    text: "+"
                    implicitWidth: win.scaledSize(24)
                    implicitHeight: win.scaledSize(24)
                    onClicked: root.commitNewProperty()
                }
            }
        }
    }

    function cancelNewProperty() {
        newKeyField.editText = ""
        newValueField.text = ""
        root.addingProperty = false
    }

    function commitNewProperty() {
        var key = (newKeyField.editText || newKeyField.currentText || "").trim()
        if (!key.length)
            return
        backend.setCurrentFrontMatterField(key, newValueField.text)
        newKeyField.editText = ""
        newValueField.text = ""
        root.addingProperty = false
    }
}
