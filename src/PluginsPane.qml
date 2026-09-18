import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Column {
    id: root
    spacing: 10
    width: parent ? parent.width : implicitWidth

    readonly property var allPlugins: (typeof pluginHost !== "undefined" && pluginHost)
                                      ? pluginHost.loadedPlugins : []
    readonly property var corePlugins: pluginsWhere(true)
    readonly property var communityPlugins: pluginsWhere(false)
    readonly property bool communityOn: (typeof pluginHost !== "undefined" && pluginHost)
                                        ? pluginHost.communityPluginsEnabled : false

    function matchesQuery(plugin, q) {
        if (q.length === 0)
            return true
        return String(plugin.name).toLowerCase().indexOf(q) >= 0
            || String(plugin.id).toLowerCase().indexOf(q) >= 0
            || String(plugin.description).toLowerCase().indexOf(q) >= 0
            || String(plugin.author).toLowerCase().indexOf(q) >= 0
    }

    function pluginsWhere(builtin) {
        var q = searchField.text.trim().toLowerCase()
        var out = []
        for (var i = 0; i < allPlugins.length; ++i) {
            var p = allPlugins[i]
            if (!!p.builtin !== builtin)
                continue
            if (matchesQuery(p, q))
                out.push(p)
        }
        return out
    }

    Label {
        width: parent.width
        text: (backend.uiLanguage, backend.t("installedPlugins"))
        color: win.strongTextColor
        font.family: "iA Writer Mono S"
        font.pixelSize: win.scaledSize(13)
        font.bold: true
    }

    Label {
        width: parent.width
        wrapMode: Text.Wrap
        text: (backend.uiLanguage, backend.t("pluginsHelp"))
        color: win.mutedColor
        font.family: "iA Writer Mono S"
        font.pixelSize: win.scaledSize(11)
    }

    RowLayout {
        width: parent.width
        spacing: 8
        Label {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            text: (backend.uiLanguage, backend.t("restrictedMode"))
            color: win.strongTextColor
            font.family: "iA Writer Mono S"
            font.pixelSize: win.scaledSize(12)
        }
        Switch {
            id: communitySwitch
            objectName: "communityPluginsSwitch"
            checked: root.communityOn
            onToggled: if (pluginHost) pluginHost.communityPluginsEnabled = checked
        }
    }

    Label {
        width: parent.width
        wrapMode: Text.Wrap
        text: (backend.uiLanguage, backend.t("restrictedModeHelp"))
        color: win.mutedColor
        font.family: "iA Writer Mono S"
        font.pixelSize: win.scaledSize(11)
    }

    Label {
        width: parent.width
        text: (backend.uiLanguage, backend.t("pluginCount")).arg(allPlugins.length)
        color: win.mutedColor
        font.family: "iA Writer Mono S"
        font.pixelSize: win.scaledSize(11)
    }

    RowLayout {
        width: parent.width
        spacing: 8
        PaneButton {
            objectName: "reloadPluginsButton"
            text: (backend.uiLanguage, backend.t("reloadPlugins"))
            onClicked: if (pluginHost) pluginHost.reloadAll()
        }
        PaneButton {
            text: (backend.uiLanguage, backend.t("openPluginsFolder"))
            onClicked: if (pluginHost) pluginHost.revealUserPluginsFolder()
        }
    }

    TextField {
        id: searchField
        objectName: "pluginSearchField"
        width: parent.width
        implicitHeight: win.scaledSize(36)
        leftPadding: 12
        rightPadding: 12
        placeholderText: (backend.uiLanguage, backend.t("searchPlugins"))
        font.family: "iA Writer Mono S"
        font.pixelSize: win.scaledSize(12)
    }

    component PluginList: Column {
        property var plugins: []
        property bool community: false
        width: root.width
        spacing: 8

        Repeater {
            model: parent.plugins
            delegate: Item {
                required property var modelData
                width: root.width
                implicitHeight: Math.max(pluginText.implicitHeight, pluginToggle.implicitHeight) + 24

                Rectangle {
                    anchors.fill: parent
                    radius: 8
                    color: "transparent"
                    border.color: win.panelBorderColor
                    border.width: 1
                    opacity: community && !root.communityOn ? 0.55 : 1
                }

                Column {
                    id: pluginText
                    anchors.left: parent.left
                    anchors.right: pluginToggle.left
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 4

                    Label {
                        width: parent.width
                        text: (modelData.name || modelData.id)
                              + (modelData.version ? "  v" + modelData.version : "")
                        color: win.strongTextColor
                        wrapMode: Text.Wrap
                        font.family: "iA Writer Mono S"
                        font.pixelSize: win.scaledSize(12)
                        font.bold: true
                    }
                    Label {
                        visible: (modelData.author || "").length > 0 || modelData.builtin
                        width: parent.width
                        text: (modelData.builtin ? "core" : "community")
                              + (modelData.author ? " · " + modelData.author : "")
                        color: win.mutedColor
                        font.family: "iA Writer Mono S"
                        font.pixelSize: win.scaledSize(10)
                    }
                    Label {
                        visible: (modelData.description || "").length > 0
                        width: parent.width
                        text: modelData.description || ""
                        color: win.textColor
                        wrapMode: Text.Wrap
                        font.family: "iA Writer Mono S"
                        font.pixelSize: win.scaledSize(11)
                    }
                }

                Switch {
                    id: pluginToggle
                    objectName: "pluginEnabled-" + modelData.id
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.rightMargin: 12
                    checked: modelData.enabled === true
                    enabled: !community || root.communityOn
                    onToggled: pluginHost.setPluginEnabled(modelData.id, checked)
                }
            }
        }
    }

    Label {
        width: parent.width
        text: (backend.uiLanguage, backend.t("corePlugins"))
        color: win.strongTextColor
        font.family: "iA Writer Mono S"
        font.pixelSize: win.scaledSize(12)
        font.bold: true
        topPadding: 8
    }

    PluginList {
        plugins: root.corePlugins
        community: false
    }

    Label {
        width: parent.width
        text: (backend.uiLanguage, backend.t("communityPlugins"))
        color: win.strongTextColor
        font.family: "iA Writer Mono S"
        font.pixelSize: win.scaledSize(12)
        font.bold: true
        topPadding: 8
    }

    Label {
        visible: !root.communityOn
        width: parent.width
        wrapMode: Text.Wrap
        text: (backend.uiLanguage, backend.t("communityDisabledHint"))
        color: win.mutedColor
        font.family: "iA Writer Mono S"
        font.pixelSize: win.scaledSize(11)
    }

    PluginList {
        plugins: root.communityPlugins
        community: true
    }
}
