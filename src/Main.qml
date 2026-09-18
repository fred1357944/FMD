import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Dialogs as Dialogs
import QtQuick.Layouts
import QtQuick.Window
import "EditorMutations.js" as EditorMutations

ApplicationWindow {
    id: win
    width: 1280
    height: 820
    minimumWidth: 1000
    minimumHeight: 520
    visible: true
    title: (backend.modified ? "* " : "") + backend.fileName + " - FMD"

    readonly property bool darkMode: backend.darkMode
    readonly property color pageColor: backend.themeBackground
    readonly property color panelColor: darkMode ? "#161616" : "#fbfbfb"
    readonly property color panelBorderColor: darkMode ? "#2b2b2b" : "#dedede"
    readonly property color currentFileColor: darkMode ? "#223442" : "#deebf5"
    readonly property color textColor: backend.themeForeground
    readonly property color strongTextColor: backend.themeForeground
    readonly property color mutedColor: darkMode ? "#909191" : "#aeb1b5"
    readonly property color selectionFill: backend.themeSelection
    // The desktop's text size knob (GNOME's text-scaling-factor, which
    // `omarchy display text size` drives) anchored so its 12px default leaves
    // the app at the sizes it was designed around.
    readonly property real textScale: backend.textScale
    readonly property int statusBarHeight: scaledSize(38)
    readonly property int editorFontPixelSize: scaledSize(20)
    property bool closeConfirmed: false
    property bool searchOpen: false
    property bool searchUpdating: false
    property var searchMatches: []
    property int searchMatchIndex: -1
    property url pendingOpenUrl
    property string pendingAction: ""
    property string pendingFragmentKind: ""
    property string pendingFragment: ""
    property bool replaceOpen: false
    property bool awaitingPendingSave: false
    property string sidebarSection: "files"
    readonly property bool showSidebar: backend.sidebarVisible && !backend.zenMode
    readonly property bool showPreview: backend.previewVisible && !backend.zenMode
    property url pendingDeleteUrl
    property url renamingUrl
    property bool pendingPublishAfterSave: false
    property bool pendingPublishDryRun: false
    property var threadsPublishReport: ({})
    property bool pendingSlidevAfterSave: false
    property bool pendingSlidevDryRun: false
    property var slidevPublishReport: ({})
    property string pendingGardenKind: "note"
    property bool pendingGardenDryRun: false
    property var gardenPublishReport: ({})

    Material.theme: darkMode ? Material.Dark : Material.Light
    Material.accent: backend.themeAccent
    color: pageColor

    onClosing: function(close) {
        if (closeConfirmed || !backend.shouldPromptForUnsaved()) {
            if (!closeConfirmed)
                prepareToLeaveDocument();
            return;
        }

        close.accepted = false;
        pendingAction = "close";
        if (!unsavedChangesDialog.opened)
            unsavedChangesDialog.open();
    }

    function prepareToLeaveDocument() {
        if (!backend.modified)
            return;
        if (backend.isBlankDocument() && backend.isUntitledDocument()) {
            backend.discardBlankUntitled();
            return;
        }
        if (!backend.confirmUnsavedChanges && backend.fileUrl.toString().length > 0)
            backend.save();
    }

    property var tableEdit: ({ "active": false })

    function refreshTableEdit() {
        tableEdit = backend.tableAt(editor.text, editor.cursorPosition)
    }

    function tableCursor() {
        var cursor = editor.cursorPosition
        if (tableEdit && tableEdit.active
                && (cursor < tableEdit.start || cursor > tableEdit.end))
            return tableEdit.start
        return cursor
    }

    function applyTableMutation(mutation) {
        if (!mutation || !mutation.active)
            return
        var caret = mutation.caret !== undefined ? mutation.caret : mutation.start
        EditorMutations.replaceRange(editor, mutation.start, mutation.end, mutation.text,
                                     caret - mutation.start, caret - mutation.start)
        refreshTableEdit()
    }

    function placeLiveCaret() {
        if (backend.editorMode !== "live")
            return
        var end = backend.frontMatterEnd
        if (end > 0 && editor.cursorPosition < end)
            editor.cursorPosition = Math.min(editor.text.length, end)
    }

    function schedulePreviewFollow() {
        if (!backend.previewVisible || backend.previewMode === "threads")
            return
        previewFollowTimer.restart()
    }

    function followPreviewToCaret() {
        if (!previewFlick.visible || previewFlick.height <= 0)
            return
        var maxY = Math.max(0, previewFlick.contentHeight - previewFlick.height)
        if (maxY <= 0)
            return
        var caret = editor.cursorPosition
        var textLen = editor.text.length
        if (textLen === 0) {
            previewFlick.contentY = 0
            return
        }
        if (caret >= textLen - 1) {
            previewFlick.contentY = maxY
            return
        }
        var index = backend.previewBlockIndexAt(caret)
        var kids = previewDocument.children
        var delegates = []
        for (var i = 0; i < kids.length; ++i) {
            if (kids[i].index !== undefined)
                delegates.push(kids[i])
        }
        if (delegates.length === 0)
            return
        if (index < 0 || index >= delegates.length)
            index = delegates.length - 1
        var item = delegates[index]
        if (!item || item.height <= 0)
            return
        var block = backend.previewBlockAt(index)
        var from = Number(block.from || 0)
        var to = Number(block.to || from + 1)
        var span = Math.max(1, to - from)
        var frac = Math.max(0, Math.min(1, (caret - from) / span))
        var localY = item.height * frac
        var mapped = item.mapToItem(previewFlick.contentItem, 0, localY)
        var y = mapped.y
        var margin = 56
        var viewTop = previewFlick.contentY
        var viewBottom = viewTop + previewFlick.height
        if (y + margin > viewBottom)
            previewFlick.contentY = Math.min(maxY, y + margin - previewFlick.height)
        else if (y - margin < viewTop)
            previewFlick.contentY = Math.max(0, y - margin)
    }

    Timer {
        id: previewFollowTimer
        interval: 40
        repeat: false
        onTriggered: win.followPreviewToCaret()
    }

    function requestOpen(url, fragmentKind, fragment) {
        pendingFragmentKind = fragmentKind || "";
        pendingFragment = fragment || "";
        backend.projectView = "editor";
        if (url && backend.fileUrl.toString() === url.toString() && pendingFragment.length > 0) {
            applyPendingFragment();
            return;
        }
        if (!backend.shouldPromptForUnsaved()) {
            prepareToLeaveDocument();
            backend.open(url);
            applyPendingFragment();
            return;
        }
        pendingOpenUrl = url;
        pendingAction = "open";
        unsavedChangesDialog.open();
    }

    function applyPendingFragment() {
        if (pendingFragment.length === 0)
            return;
        var pos = backend.fragmentPosition(pendingFragmentKind, pendingFragment);
        pendingFragmentKind = "";
        pendingFragment = "";
        if (pos >= 0)
            jumpToOutline(pos);
    }

    function completePendingAction() {
        var action = pendingAction;
        pendingAction = "";
        if (action === "close") {
            closeConfirmed = true;
            close();
        } else if (action === "open") {
            backend.open(pendingOpenUrl);
            applyPendingFragment();
        }
    }

    menuBar: MenuBar {
        Menu {
            title: "File"
            Action {
                text: (backend.uiLanguage, backend.t("newPost"))
                onTriggered: win.createThreadsPost()
            }
            Action {
                text: (backend.uiLanguage, backend.t("newGarden"))
                onTriggered: win.createGardenNote()
            }
            Action {
                text: (backend.uiLanguage, backend.t("newSlides"))
                onTriggered: win.createSlidevNote()
            }
            Action {
                text: "Edit Threads template"
                onTriggered: win.openThreadsTemplate()
            }
            Action {
                text: (backend.uiLanguage, backend.t("editGardenTemplate"))
                onTriggered: win.openGardenTemplate()
            }
            Action {
                text: (backend.uiLanguage, backend.t("editSlidesTemplate"))
                onTriggered: win.openSlidevTemplate()
            }
            Action {
                text: "New Window"
                onTriggered: backend.newWindow()
            }
            Action {
                text: "New Markdown File"
                onTriggered: win.createMarkdownNote()
            }
            MenuSeparator {}
            Action {
                text: "Open…"
                onTriggered: backend.openDialog()
            }
            Action {
                text: "Open Folder…"
                onTriggered: backend.openFolderDialog()
            }
            Menu {
                id: fileRecentProjectsMenu
                title: (backend.uiLanguage, backend.t("recentProjects"))
                enabled: backend.recentWorkspaces.length > 0
                Instantiator {
                    model: backend.recentWorkspaces
                    delegate: Action {
                        required property var modelData
                        text: modelData.name
                        onTriggered: backend.openFolder(modelData.url)
                    }
                    onObjectAdded: function(index, object) {
                        fileRecentProjectsMenu.insertAction(index, object)
                    }
                    onObjectRemoved: function(index, object) {
                        fileRecentProjectsMenu.removeAction(object)
                    }
                }
            }
            MenuSeparator {}
            Action {
                text: "Save"
                onTriggered: backend.save()
            }
            Action {
                text: "Save As…"
                onTriggered: backend.saveAsDialog()
            }
            MenuSeparator {}
            Action {
                text: (backend.uiLanguage, backend.t("revealCurrentFile"))
                onTriggered: win.revealCurrentFileInSidebar()
            }
            Action {
                text: "Move Note to Trash…"
                onTriggered: win.requestDelete(backend.fileUrl)
            }
            Action {
                text: "Print…"
                onTriggered: backend.printDocument()
            }
            MenuSeparator {}
            Action {
                text: "立刻發 Threads"
                onTriggered: win.requestPublishThreads(false)
            }
            Action {
                text: "Dry-run publish Threads"
                onTriggered: win.requestPublishThreads(true)
            }
            Action {
                text: (backend.uiLanguage, backend.t("publishSlidevNow"))
                onTriggered: win.requestPublishSlidev(false)
            }
            Action {
                text: (backend.uiLanguage, backend.t("publishSlidevDry"))
                onTriggered: win.requestPublishSlidev(true)
            }
            Action {
                text: (backend.uiLanguage, backend.t("publishGardenNow"))
                onTriggered: win.requestPublishGarden("note", false)
            }
            Action {
                text: (backend.uiLanguage, backend.t("publishFolderGardenNow"))
                onTriggered: win.requestPublishGarden("folder", false)
            }
        }
    }

    FontMetrics {
        id: writerFontMetrics
        font.family: "iA Writer Mono S"
        font.pixelSize: win.editorFontPixelSize
    }

    // Every hardcoded size in the interface is expressed at text scale 1.
    function scaledSize(pixels) {
        return Math.max(1, Math.round(pixels * win.textScale));
    }

    function documentColumnWidth(availableWidth) {
        return Math.min(
            Math.round(writerFontMetrics.averageCharacterWidth * 65),
            Math.max(320, availableWidth - Math.round(writerFontMetrics.averageCharacterWidth * 8)));
    }

    function toggleFullScreen() {
        win.visibility = win.visibility === Window.FullScreen
            ? Window.Windowed
            : Window.FullScreen;
    }

    function openPreferencesDialog() {
        preferencesDialog.open();
    }

    function tr(key) {
        return backend.t(key);
    }

    function createThreadsPost() {
        var url = backend.createThreadsDraft();
        if (url && url.toString().length > 0)
            requestOpen(url);
    }

    function createGardenNote() {
        var url = backend.createGardenNote();
        if (url && url.toString().length > 0)
            requestOpen(url);
    }

    function createSlidevNote() {
        var url = backend.createSlidevNote();
        if (url && url.toString().length > 0)
            requestOpen(url);
    }

    function openGardenTemplate() {
        var url = backend.ensureGardenTemplate();
        if (url && url.toString().length > 0)
            requestOpen(url);
    }

    function openSlidevTemplate() {
        var url = backend.ensureSlidevTemplate();
        if (url && url.toString().length > 0)
            requestOpen(url);
    }

    function createMarkdownNote() {
        var url = backend.createMarkdownNote();
        if (url && url.toString().length > 0)
            requestOpen(url);
    }

    function createFolder() {
        var url = backend.createFolder()
        if (url && url.toString().length > 0)
            startRename(url)
    }

    function openDefaultTemplate() {
        var url = backend.ensureDefaultTemplate();
        if (url && url.toString().length > 0)
            requestOpen(url);
    }

    function createNoteFromTemplate(url) {
        var created = backend.createNoteFromTemplate(url);
        if (created && created.toString().length > 0)
            requestOpen(created);
    }

    function insertTemplate(url) {
        var text = backend.renderTemplate(url);
        if (!text)
            return;
        EditorMutations.replaceRange(editor, templatePicker.replaceStart,
                                     templatePicker.replaceEnd, text);
    }

    function openTemplatePicker(mode, start, end) {
        if (!backend.templateFiles || backend.templateFiles.length === 0)
            backend.ensureDefaultTemplate();
        templatePicker.mode = mode || "insert";
        var caret = editor.cursorPosition;
        templatePicker.replaceStart = (start !== undefined) ? start : caret;
        templatePicker.replaceEnd = (end !== undefined) ? end : caret;
        templatePicker.openAt(editorCaretPoint());
    }

    function openThreadsTemplate() {
        var url = backend.ensureThreadsTemplate();
        if (url && url.toString().length > 0)
            requestOpen(url);
    }

    function requestDelete(url) {
        if (!url || url.toString().length === 0)
            return;
        pendingDeleteUrl = url;
        deleteNoteDialog.open();
    }

    function noteStem(name) {
        if (!name)
            return "";
        var base = String(name).split("/").pop();
        return base.replace(/\.markdown$/i, "").replace(/\.md$/i, "");
    }

    function startRename(url) {
        if (!url || url.toString().length === 0)
            return;
        renamingUrl = url;
    }

    function tryFollowLink(text, pos) {
        var hit = backend.followableLinkAt(text, pos);
        if (!hit || !hit.kind)
            return false;
        if (hit.kind === "external") {
            backend.openExternalUrl(hit.url);
            return true;
        }
        if (hit.kind === "note" && hit.url && hit.url.toString().length > 0) {
            requestOpen(hit.url, hit.fragmentKind || "", hit.fragment || "");
            return true;
        }
        return false;
    }

    function fileNameFromUrl(url) {
        if (!url || url.toString().length === 0)
            return "this note";
        var path = url.toString();
        var slash = Math.max(path.lastIndexOf("/"), path.lastIndexOf("\\"));
        var name = slash >= 0 ? path.slice(slash + 1) : path;
        try {
            return decodeURIComponent(name);
        } catch (err) {
            return name;
        }
    }

    function commandCatalog() {
        var items = [
            { "id": "new-threads", "title": backend.t("newPost"), "shortcut": "⌘⇧T", "kind": "command" },
            { "id": "new-garden", "title": backend.t("newGarden"), "kind": "command" },
            { "id": "new-slides", "title": backend.t("newSlides"), "kind": "command" },
            { "id": "edit-template", "title": "Edit Threads template", "kind": "command" },
            { "id": "edit-garden-template", "title": backend.t("editGardenTemplate"), "kind": "command" },
            { "id": "edit-slides-template", "title": backend.t("editSlidesTemplate"), "kind": "command" },
            { "id": "save", "title": "Save", "shortcut": "⌘S", "kind": "command" },
            { "id": "save-as", "title": "Save As", "shortcut": "⌘⇧S", "kind": "command" },
            { "id": "open-file", "title": "Open file", "shortcut": "⌘O", "kind": "command" },
            { "id": "open-folder", "title": "Open folder", "shortcut": "⌘⇧O", "kind": "command" },
            { "id": "delete-note", "title": "Move current note to Trash", "shortcut": "⌘⌫", "kind": "command" },
            { "id": "print", "title": "Print", "kind": "command" },
            { "id": "preferences", "title": "Preferences", "shortcut": "⌘,", "kind": "command" },
            { "id": "reveal-file", "title": backend.t("revealCurrentFile"), "shortcut": "⌘⌥E", "kind": "command" },
            { "id": "toggle-sidebar", "title": "Toggle sidebar", "shortcut": "⌘⇧L", "kind": "command" },
            { "id": "toggle-preview", "title": "Toggle preview", "shortcut": "⌘⇧P", "kind": "command" },
            { "id": "toggle-zen", "title": backend.t("zen"), "shortcut": "⌘⇧U", "kind": "command" },
            { "id": "toggle-editor-mode", "title": backend.t("liveSourceHelp"), "shortcut": "⌘⇧E", "kind": "command" },
            { "id": "new-inbox-note", "title": backend.t("newInboxNote"), "shortcut": "⌘⇧I", "kind": "command" },
            { "id": "view-editor", "title": "Editor view", "shortcut": "⌘1", "kind": "command" },
            { "id": "view-table", "title": "Table view", "shortcut": "⌘2", "kind": "command" },
            { "id": "view-board", "title": "Board view", "shortcut": "⌘3", "kind": "command" },
            { "id": "view-calendar", "title": "Calendar view", "shortcut": "⌘4", "kind": "command" },
            { "id": "view-mindmap", "title": "Mindmap view", "shortcut": "⌘5", "kind": "command" },
            { "id": "view-cards", "title": backend.t("cards"), "shortcut": "⌘6", "kind": "command" },
            { "id": "new-window", "title": "New window", "shortcut": "⌘N", "kind": "command" },
            { "id": "new-note", "title": "New Markdown file", "shortcut": "⌘⇧N", "kind": "command" },
            { "id": "new-from-template", "title": backend.t("newFromTemplate"), "kind": "command" },
            { "id": "insert-template", "title": backend.t("insertTemplate"), "kind": "command" },
            { "id": "edit-default-template", "title": backend.t("editDefaultTemplate"), "kind": "command" },
            { "id": "insert-code", "title": "Insert code block", "shortcut": "/", "kind": "command" },
            { "id": "search-outline", "title": "Search outline", "kind": "command" },
            { "id": "next-heading", "title": "Next heading", "shortcut": "⌘⇧]", "kind": "command" },
            { "id": "prev-heading", "title": "Previous heading", "shortcut": "⌘⇧[", "kind": "command" },
            { "id": "copy-outline", "title": "Copy outline headings", "kind": "command" },
            { "id": "outline-deeper", "title": "Show deeper outline level", "kind": "command" },
            { "id": "outline-shallower", "title": "Show shallower outline level", "kind": "command" },
            { "id": "publish-threads", "title": "Publish Threads now", "shortcut": "⌘⇧↩", "kind": "command" },
            { "id": "publish-threads-dry", "title": "Dry-run publish Threads", "kind": "command" },
            { "id": "publish-slidev", "title": backend.t("publishSlidevNow"), "kind": "command" },
            { "id": "publish-slidev-dry", "title": backend.t("publishSlidevDry"), "kind": "command" },
            { "id": "publish-site", "title": backend.t("publishGardenNow"), "kind": "command" },
            { "id": "publish-folder-site", "title": backend.t("publishFolderGardenNow"), "kind": "command" },
            { "id": "publish-changelog-site", "title": backend.t("publishChangelogNow"), "kind": "command" },
            { "id": "reload-plugins", "title": "Reload plugins", "kind": "command" }
        ];
        var pluginCommands = (typeof pluginHost !== "undefined" && pluginHost)
            ? pluginHost.commands : [];
        for (var p = 0; p < pluginCommands.length; ++p) {
            items.push({
                "id": "plugin-command",
                "title": pluginCommands[p].title,
                "kind": "plugin",
                "pluginCommand": pluginCommands[p].id
            });
        }
        var files = backend.workspaceFiles;
        for (var i = 0; i < files.length; ++i)
            items.push({ "id": "open", "title": "Open " + files[i].name, "kind": "file", "url": files[i].url });
        var projects = backend.recentWorkspaces;
        for (var w = 0; w < projects.length; ++w)
            items.push({
                "id": "open-project",
                "title": "Project " + projects[w].name,
                "kind": "folder",
                "url": projects[w].url
            });
        var recent = backend.recentFiles;
        for (var j = 0; j < recent.length; ++j)
            items.push({ "id": "open", "title": "Recent " + recent[j].name, "kind": "file", "url": recent[j].url });
        return items;
    }

    function runCommand(item) {
        if (!item)
            return;
        if (item.id === "new-threads")
            createThreadsPost();
        else if (item.id === "new-garden")
            createGardenNote();
        else if (item.id === "new-slides")
            createSlidevNote();
        else if (item.id === "edit-template")
            openThreadsTemplate();
        else if (item.id === "edit-garden-template")
            openGardenTemplate();
        else if (item.id === "edit-slides-template")
            openSlidevTemplate();
        else if (item.id === "save")
            backend.save();
        else if (item.id === "save-as")
            backend.saveAsDialog();
        else if (item.id === "open-file")
            backend.openDialog();
        else if (item.id === "open-folder")
            backend.openFolderDialog();
        else if (item.id === "open-project")
            backend.openFolder(item.url);
        else if (item.id === "delete-note")
            requestDelete(backend.fileUrl);
        else if (item.id === "print")
            backend.printDocument();
        else if (item.id === "preferences")
            openPreferencesDialog();
        else if (item.id === "reveal-file")
            revealCurrentFileInSidebar();
        else if (item.id === "toggle-sidebar")
            toggleSidebarVisibility();
        else if (item.id === "toggle-preview")
            togglePreviewVisibility();
        else if (item.id === "toggle-zen")
            backend.zenMode = !backend.zenMode;
        else if (item.id === "toggle-editor-mode")
            backend.editorMode = backend.editorMode === "source" ? "live" : "source";
        else if (item.id === "new-inbox-note")
            openInboxNote(backend.createInboxNote());
        else if (item.id === "view-editor")
            backend.projectView = "editor";
        else if (item.id === "view-table")
            backend.projectView = "table";
        else if (item.id === "view-board")
            backend.projectView = "board";
        else if (item.id === "view-calendar")
            backend.projectView = "calendar";
        else if (item.id === "view-mindmap")
            backend.projectView = "mindmap";
        else if (item.id === "view-cards")
            backend.projectView = "cards";
        else if (item.id === "new-window")
            backend.newWindow();
        else if (item.id === "new-note")
            createMarkdownNote();
        else if (item.id === "new-from-template")
            openTemplatePicker("create");
        else if (item.id === "insert-template")
            openTemplatePicker("insert");
        else if (item.id === "edit-default-template")
            openDefaultTemplate();
        else if (item.id === "insert-code")
            beginInsertCodeBlock();
        else if (item.id === "open")
            requestOpen(item.url);
        else if (item.id === "search-outline") {
            backend.sidebarVisible = true;
            sidebarSection = "outline";
            outlinePane.focusSearch();
        } else if (item.id === "next-heading")
            jumpOutlineDelta(1);
        else if (item.id === "prev-heading")
            jumpOutlineDelta(-1);
        else if (item.id === "copy-outline")
            backend.copyOutlineHeadings();
        else if (item.id === "outline-deeper")
            backend.outlineMaxLevel = Math.min(6, backend.outlineMaxLevel + 1);
        else if (item.id === "outline-shallower")
            backend.outlineMaxLevel = Math.max(1, backend.outlineMaxLevel - 1);
        else if (item.id === "publish-threads")
            requestPublishThreads(false);
        else if (item.id === "publish-threads-dry")
            requestPublishThreads(true);
        else if (item.id === "publish-slidev")
            requestPublishSlidev(false);
        else if (item.id === "publish-slidev-dry")
            requestPublishSlidev(true);
        else if (item.id === "publish-site")
            requestPublishGarden("note", false);
        else if (item.id === "publish-folder-site")
            requestPublishGarden("folder", false);
        else if (item.id === "publish-changelog-site")
            requestPublishGarden("changelog", false);
        else if (item.id === "reload-plugins") {
            if (typeof pluginHost !== "undefined" && pluginHost)
                pluginHost.reloadAll();
        } else if (item.kind === "plugin" && item.pluginCommand) {
            if (typeof pluginHost !== "undefined" && pluginHost)
                pluginHost.invokeCommand(item.pluginCommand);
        }
    }

    function requestPublishThreads(dryRun) {
        pendingPublishDryRun = !!dryRun;
        if (!backend.fileUrl || backend.fileUrl.toString().length === 0 || backend.modified) {
            pendingPublishAfterSave = true;
            backend.save();
            return;
        }
        runThreadsValidateThenConfirm(dryRun);
    }

    function requestPublishSlidev(dryRun) {
        pendingSlidevDryRun = !!dryRun;
        if (!backend.fileUrl || backend.fileUrl.toString().length === 0 || backend.modified) {
            pendingSlidevAfterSave = true;
            backend.save();
            return;
        }
        runSlidevValidateThenConfirm(dryRun);
    }

    function runSlidevValidateThenConfirm(dryRun) {
        var report = backend.validateSlidevDraft();
        slidevPublishReport = report;
        if (!report || !report.ok) {
            slidevPublishErrorDialog.open();
            return;
        }
        pendingSlidevDryRun = !!dryRun;
        slidevPublishConfirmDialog.open();
    }

    function requestPublishGarden(kind, dryRun) {
        pendingGardenKind = kind || "note";
        pendingGardenDryRun = !!dryRun;
        gardenPublishReport = ({});
        gardenPublishConfirmDialog.open();
    }

    function confirmPublishGarden() {
        var report;
        if (win.pendingGardenKind === "folder")
            report = backend.publishFolderGardenNow(win.pendingGardenDryRun);
        else if (win.pendingGardenKind === "changelog")
            report = backend.publishChangelogNow(win.pendingGardenDryRun);
        else
            report = backend.publishGardenNow(win.pendingGardenDryRun);
        gardenPublishReport = report;
        if (!report || !report.ok) {
            gardenPublishConfirmDialog.close();
            gardenPublishErrorDialog.open();
            return;
        }
        gardenPublishConfirmDialog.close();
    }

    function confirmPublishSlidev() {
        var report = backend.publishSlidevNow(pendingSlidevDryRun);
        slidevPublishReport = report;
        if (!report || !report.ok) {
            slidevPublishConfirmDialog.close();
            slidevPublishErrorDialog.open();
        }
    }

    function runThreadsValidateThenConfirm(dryRun) {
        var report = backend.validateThreadsDraft();
        threadsPublishReport = report;
        if (!report || !report.ok) {
            threadsPublishErrorDialog.open();
            return;
        }
        pendingPublishDryRun = !!dryRun;
        threadsPublishConfirmDialog.open();
    }

    function confirmPublishThreads() {
        var report = backend.publishThreadsNow(pendingPublishDryRun);
        threadsPublishReport = report;
        if (!report || !report.ok) {
            threadsPublishConfirmDialog.close();
            threadsPublishErrorDialog.open();
        }
    }

    function revealCurrentFileInSidebar() {
        backend.sidebarVisible = true
        sidebarSection = "files"
        var index = backend.expandAncestorsOfCurrentFile()
        Qt.callLater(function() {
            var i = backend.workspaceIndexOfCurrentFile()
            if (i < 0)
                i = index
            if (i >= 0 && workspaceNavPane)
                workspaceNavPane.positionCurrentFile(i)
        })
    }

    function toggleSidebarVisibility() {
        backend.sidebarVisible = !backend.sidebarVisible;
    }

    function togglePreviewVisibility() {
        backend.previewVisible = !backend.previewVisible;
    }

    function openInboxNote(url) {
        if (!url || url.toString().length === 0)
            return
        requestOpen(url)
    }

    function jumpToOutline(position) {
        backend.projectView = "editor";
        var pos = Math.max(0, Math.min(editor.text.length, position));
        editor.cursorPosition = pos;
        editor.forceActiveFocus();
        editorFlick.ensureCursorVisible();
    }

    function headingIndexAtCursor() {
        var headings = backend.documentOutline;
        var best = -1;
        for (var i = 0; i < headings.length; ++i) {
            if (headings[i].position <= editor.cursorPosition)
                best = i;
        }
        return best;
    }

    function jumpOutlineDelta(delta) {
        var headings = backend.documentOutline;
        if (!headings || headings.length === 0)
            return;
        var index = headingIndexAtCursor();
        if (index < 0)
            index = delta > 0 ? -1 : 0;
        index = Math.max(0, Math.min(headings.length - 1, index + delta));
        jumpToOutline(headings[index].position);
        sidebarSection = "outline";
    }

    function updateSearch() {
        var matches = [];
        var query = searchField.text;
        if (query.length > 0) {
            var haystack = editor.text.toLocaleLowerCase();
            var needle = query.toLocaleLowerCase();
            var position = 0;
            while ((position = haystack.indexOf(needle, position)) !== -1) {
                matches.push(position);
                position += Math.max(1, needle.length);
            }
        }
        searchMatches = matches;
        searchMatchIndex = matches.length > 0 ? 0 : -1;
        showSearchMatch();
    }

    function showSearchMatch() {
        var start = searchMatchIndex >= 0 ? searchMatches[searchMatchIndex] : -1;
        searchUpdating = true;
        backend.setSearchHighlight(searchField.text, start);
        if (start >= 0) {
            editor.select(start, start + searchField.text.length);
            editorFlick.ensureCursorVisible();
        }
        searchUpdating = false;
    }

    function moveSearch(direction) {
        if (searchMatches.length === 0)
            return;
        searchMatchIndex = (searchMatchIndex + direction + searchMatches.length)
                           % searchMatches.length;
        showSearchMatch();
    }

    function closeSearch() {
        searchOpen = false;
        searchUpdating = true;
        backend.setSearchHighlight("", -1);
        editor.deselect();
        searchUpdating = false;
        replaceOpen = false;
        editor.forceActiveFocus();
    }

    Shortcut {
        sequences: backend.hotkeys["save"] ? [backend.hotkeys["save"]] : []
        enabled: !!(backend.hotkeys["save"])
        context: Qt.ApplicationShortcut
        onActivated: backend.save()
    }

    Shortcut {
        sequences: backend.hotkeys["find-replace"] ? [backend.hotkeys["find-replace"]] : []
        enabled: !!(backend.hotkeys["find-replace"])
        context: Qt.ApplicationShortcut
        onActivated: {
            searchOpen = true;
            replaceOpen = true;
            searchField.forceActiveFocus();
            searchField.selectAll();
        }
    }

    Shortcut {
        sequences: backend.hotkeys["bold"] ? [backend.hotkeys["bold"]] : []
        enabled: !!(backend.hotkeys["bold"])
        context: Qt.WindowShortcut
        onActivated: editor.wrapSelection("**", "**")
    }

    Shortcut {
        sequences: backend.hotkeys["italic"] ? [backend.hotkeys["italic"]] : []
        enabled: !!(backend.hotkeys["italic"])
        context: Qt.WindowShortcut
        onActivated: editor.wrapSelection("*", "*")
    }

    Shortcut {
        sequences: backend.hotkeys["link"] ? [backend.hotkeys["link"]] : []
        enabled: !!(backend.hotkeys["link"])
        context: Qt.WindowShortcut
        onActivated: editor.insertLink()
    }

    Shortcut {
        sequences: ["Ctrl+?", "Meta+?"]
        context: Qt.ApplicationShortcut
        onActivated: openPreferencesDialog()
    }

    Shortcut {
        objectName: "preferencesShortcut"
        sequences: backend.hotkeys["preferences"] ? [backend.hotkeys["preferences"]] : []
        enabled: !!(backend.hotkeys["preferences"])
        context: Qt.ApplicationShortcut
        onActivated: openPreferencesDialog()
    }

    Shortcut {
        sequences: backend.hotkeys["open"] ? [backend.hotkeys["open"]] : []
        enabled: !!(backend.hotkeys["open"])
        context: Qt.ApplicationShortcut
        onActivated: backend.openDialog()
    }

    Shortcut {
        sequences: backend.hotkeys["open-folder"] ? [backend.hotkeys["open-folder"]] : []
        enabled: !!(backend.hotkeys["open-folder"])
        context: Qt.ApplicationShortcut
        onActivated: backend.openFolderDialog()
    }

    Shortcut {
        sequences: backend.hotkeys["new-window"] ? [backend.hotkeys["new-window"]] : []
        enabled: !!(backend.hotkeys["new-window"])
        context: Qt.ApplicationShortcut
        onActivated: backend.newWindow()
    }

    Shortcut {
        objectName: "newMarkdownNoteShortcut"
        sequences: backend.hotkeys["new-note"] ? [backend.hotkeys["new-note"]] : []
        enabled: !!(backend.hotkeys["new-note"])
        context: Qt.ApplicationShortcut
        onActivated: win.createMarkdownNote()
    }

    Shortcut {
        objectName: "newThreadsPostShortcut"
        sequences: backend.hotkeys["new-post"] ? [backend.hotkeys["new-post"]] : []
        enabled: !!(backend.hotkeys["new-post"])
        context: Qt.ApplicationShortcut
        onActivated: win.createThreadsPost()
    }

    Shortcut {
        objectName: "publishThreadsShortcut"
        sequences: backend.hotkeys["publish-now"] ? [backend.hotkeys["publish-now"]] : []
        enabled: !!(backend.hotkeys["publish-now"])
        context: Qt.ApplicationShortcut
        onActivated: win.requestPublishThreads(false)
    }

    Shortcut {
        sequences: backend.hotkeys["save-as"] ? [backend.hotkeys["save-as"]] : []
        enabled: !!(backend.hotkeys["save-as"])
        context: Qt.ApplicationShortcut
        onActivated: backend.saveAsDialog()
    }

    Shortcut {
        objectName: "commandPaletteShortcut"
        sequences: backend.hotkeys["command-palette"] ? [backend.hotkeys["command-palette"]] : []
        enabled: !!(backend.hotkeys["command-palette"])
        context: Qt.ApplicationShortcut
        onActivated: {
            commandPalette.catalog = commandCatalog();
            commandPalette.openPalette();
        }
    }

    Shortcut {
        objectName: "deleteNoteShortcut"
        sequences: backend.hotkeys["delete-note"] ? [backend.hotkeys["delete-note"]] : []
        enabled: !!(backend.hotkeys["delete-note"])
        context: Qt.ApplicationShortcut
        onActivated: win.requestDelete(backend.fileUrl)
    }

    Shortcut {
        objectName: "renameNoteShortcut"
        sequence: "F2"
        enabled: win.sidebarSection === "files" && backend.projectView !== "mindmap"
        context: Qt.ApplicationShortcut
        onActivated: win.startRename(backend.fileUrl)
    }

    Shortcut {
        objectName: "nextHeadingShortcut"
        sequences: backend.hotkeys["next-heading"] ? [backend.hotkeys["next-heading"]] : []
        enabled: !!(backend.hotkeys["next-heading"])
        context: Qt.ApplicationShortcut
        onActivated: win.jumpOutlineDelta(1)
    }

    Shortcut {
        objectName: "prevHeadingShortcut"
        sequences: backend.hotkeys["prev-heading"] ? [backend.hotkeys["prev-heading"]] : []
        enabled: !!(backend.hotkeys["prev-heading"])
        context: Qt.ApplicationShortcut
        onActivated: win.jumpOutlineDelta(-1)
    }

    Shortcut {
        objectName: "toggleSidebarShortcut"
        sequences: backend.hotkeys["toggle-sidebar"] ? [backend.hotkeys["toggle-sidebar"]] : []
        enabled: !!(backend.hotkeys["toggle-sidebar"])
        context: Qt.ApplicationShortcut
        onActivated: toggleSidebarVisibility()
    }

    Shortcut {
        objectName: "togglePreviewShortcut"
        sequences: backend.hotkeys["toggle-preview"] ? [backend.hotkeys["toggle-preview"]] : []
        enabled: !!(backend.hotkeys["toggle-preview"])
        context: Qt.ApplicationShortcut
        onActivated: togglePreviewVisibility()
    }

    Shortcut {
        objectName: "toggleZenShortcut"
        sequences: backend.hotkeys["toggle-zen"] ? [backend.hotkeys["toggle-zen"]] : []
        enabled: !!(backend.hotkeys["toggle-zen"])
        context: Qt.ApplicationShortcut
        onActivated: backend.zenMode = !backend.zenMode
    }

    Shortcut {
        objectName: "toggleEditorModeShortcut"
        sequences: backend.hotkeys["toggle-editor-mode"] ? [backend.hotkeys["toggle-editor-mode"]] : []
        enabled: !!(backend.hotkeys["toggle-editor-mode"])
        context: Qt.ApplicationShortcut
        onActivated: backend.editorMode = backend.editorMode === "source" ? "live" : "source"
    }

    Shortcut {
        objectName: "newInboxNoteShortcut"
        sequences: backend.hotkeys["new-inbox-note"] ? [backend.hotkeys["new-inbox-note"]] : []
        enabled: !!(backend.hotkeys["new-inbox-note"])
        context: Qt.ApplicationShortcut
        onActivated: openInboxNote(backend.createInboxNote())
    }

    Shortcut {
        enabled: backend.zenMode
        sequence: "Escape"
        context: Qt.ApplicationShortcut
        onActivated: backend.zenMode = false
    }

    Shortcut {
        sequences: backend.hotkeys["fullscreen"] ? [backend.hotkeys["fullscreen"]] : []
        enabled: !!(backend.hotkeys["fullscreen"])
        context: Qt.ApplicationShortcut
        onActivated: toggleFullScreen()
    }

    Shortcut {
        sequences: backend.hotkeys["undo"] ? [backend.hotkeys["undo"]] : []
        enabled: !!(backend.hotkeys["undo"])
                 && !(backend.projectView === "mindmap" && mindmapView.editingId.length > 0)
        context: Qt.WindowShortcut
        onActivated: editor.undo()
    }

    Shortcut {
        sequences: backend.hotkeys["redo"] ? [backend.hotkeys["redo"]] : []
        enabled: !!(backend.hotkeys["redo"])
                 && !(backend.projectView === "mindmap" && mindmapView.editingId.length > 0)
        context: Qt.WindowShortcut
        onActivated: editor.redo()
    }

    Shortcut {
        sequences: backend.hotkeys["find"] ? [backend.hotkeys["find"]] : []
        enabled: !!(backend.hotkeys["find"])
        context: Qt.ApplicationShortcut
        onActivated: {
            searchOpen = true;
            searchField.forceActiveFocus();
            searchField.selectAll();
        }
    }

    Shortcut {
        sequences: backend.hotkeys["find-next"] ? [backend.hotkeys["find-next"]] : []
        enabled: win.searchOpen && !!(backend.hotkeys["find-next"])
        context: Qt.ApplicationShortcut
        onActivated: win.moveSearch(1)
    }

    Shortcut {
        objectName: "viewEditorShortcut"
        sequences: backend.hotkeys["view-editor"] ? [backend.hotkeys["view-editor"]] : []
        enabled: !!(backend.hotkeys["view-editor"])
        context: Qt.ApplicationShortcut
        onActivated: backend.projectView = "editor"
    }
    Shortcut {
        objectName: "viewTableShortcut"
        sequences: backend.hotkeys["view-table"] ? [backend.hotkeys["view-table"]] : []
        enabled: !!(backend.hotkeys["view-table"])
        context: Qt.ApplicationShortcut
        onActivated: backend.projectView = "table"
    }
    Shortcut {
        objectName: "viewBoardShortcut"
        sequences: backend.hotkeys["view-board"] ? [backend.hotkeys["view-board"]] : []
        enabled: !!(backend.hotkeys["view-board"])
        context: Qt.ApplicationShortcut
        onActivated: backend.projectView = "board"
    }
    Shortcut {
        objectName: "viewCalendarShortcut"
        sequences: backend.hotkeys["view-calendar"] ? [backend.hotkeys["view-calendar"]] : []
        enabled: !!(backend.hotkeys["view-calendar"])
        context: Qt.ApplicationShortcut
        onActivated: backend.projectView = "calendar"
    }
    Shortcut {
        objectName: "viewMindmapShortcut"
        sequences: backend.hotkeys["view-mindmap"] ? [backend.hotkeys["view-mindmap"]] : []
        enabled: !!(backend.hotkeys["view-mindmap"])
        context: Qt.ApplicationShortcut
        onActivated: backend.projectView = "mindmap"
    }
    Shortcut {
        objectName: "viewCardsShortcut"
        sequences: backend.hotkeys["view-cards"] ? [backend.hotkeys["view-cards"]] : []
        enabled: !!(backend.hotkeys["view-cards"])
        context: Qt.ApplicationShortcut
        onActivated: backend.projectView = "cards"
    }
    Shortcut {
        objectName: "revealFileShortcut"
        sequences: backend.hotkeys["reveal-file"] ? [backend.hotkeys["reveal-file"]] : []
        enabled: !!(backend.hotkeys["reveal-file"])
        context: Qt.ApplicationShortcut
        onActivated: win.revealCurrentFileInSidebar()
    }

    Connections {
        target: backend

        function onFileUrlChanged() {
            Qt.callLater(function() {
                backend.refreshLiveFolding()
                win.placeLiveCaret()
                win.revealCurrentFileInSidebar()
            })
        }

        function onEditorModeChanged() {
            Qt.callLater(function() {
                backend.refreshLiveFolding()
                win.placeLiveCaret()
            })
        }

        function onOpenDialogRequested() {
            openFileDialog.currentFolder = backend.preferredDialogFolderUrl();
            openFileDialog.open();
        }

        function onOpenFolderDialogRequested() {
            openFolderDialog.currentFolder = backend.preferredDialogFolderUrl();
            openFolderDialog.open();
        }

        function onSaveDialogRequested(suggestedUrl) {
            saveFileDialog.selectedFile = suggestedUrl;
            saveFileDialog.open();
        }

        function onCloseAfterSave() {
            win.closeConfirmed = true;
            win.close();
        }

        function onSaveSucceeded() {
            win.awaitingPendingSave = false;
            if (win.pendingAction !== "")
                win.completePendingAction();
            if (win.pendingPublishAfterSave) {
                win.pendingPublishAfterSave = false;
                win.runThreadsValidateThenConfirm(win.pendingPublishDryRun);
            }
            if (win.pendingSlidevAfterSave) {
                win.pendingSlidevAfterSave = false;
                win.runSlidevValidateThenConfirm(win.pendingSlidevDryRun);
            }
        }

        function onThreadsPublishFinished(report) {
            win.threadsPublishReport = report;
            threadsPublishConfirmDialog.close();
            if (!report || !report.ok)
                threadsPublishErrorDialog.open();
        }

        function onSlidevPublishFinished(report) {
            win.slidevPublishReport = report;
            slidevPublishConfirmDialog.close();
            if (!report || !report.ok)
                slidevPublishErrorDialog.open();
        }

        function onExternalChangeDetected(deleted, locallyModified) {
            externalChangeDialog.deleted = deleted;
            externalChangeDialog.locallyModified = locallyModified;
            externalChangeDialog.open();
        }
    }

    Dialogs.FileDialog {
        id: openFileDialog
        title: "Open File"
        fileMode: Dialogs.FileDialog.OpenFile
        nameFilters: ["Markdown files (*.md *.markdown)", "All files (*)"]
        onAccepted: win.requestOpen(selectedFile)
    }

    Dialogs.FolderDialog {
        id: openFolderDialog
        title: "Open Folder"
        onAccepted: backend.openFolder(selectedFolder)
    }

    Dialogs.FolderDialog {
        id: threadsFolderDialog
        title: "Threads drafts folder"
        onAccepted: backend.threadsDraftsFolder = selectedFolder.toLocalFile()
    }

    Dialogs.FolderDialog {
        id: notesSiteFolderDialog
        title: "Notes site folder"
        onAccepted: backend.notesSiteFolder = selectedFolder.toLocalFile()
    }

    Dialogs.FolderDialog {
        id: changelogSiteFolderDialog
        title: "Changelog site folder"
        onAccepted: backend.changelogSiteFolder = selectedFolder.toLocalFile()
    }

    Dialogs.FolderDialog {
        id: slidesSiteFolderDialog
        title: "Slidev folder"
        onAccepted: backend.slidesSiteFolder = selectedFolder.toLocalFile()
    }

    Dialogs.FileDialog {
        id: saveFileDialog
        title: "Save File"
        fileMode: Dialogs.FileDialog.SaveFile
        nameFilters: ["Markdown files (*.md *.markdown)", "All files (*)"]
        onAccepted: backend.saveAs(selectedFile)
        onRejected: {
            backend.fileDialogCanceled();
            win.awaitingPendingSave = false;
            win.pendingAction = "";
            win.pendingFragment = "";
            win.pendingFragmentKind = "";
            win.pendingPublishAfterSave = false;
        }
    }

    UnsavedChangesDialog {
        id: unsavedChangesDialog
        fileName: backend.fileName
        darkMode: win.darkMode
        textScale: win.textScale
        textColor: win.textColor
        strongTextColor: win.strongTextColor
        activeButtonColor: backend.themeAccent
        containerWidth: win.width
        containerHeight: win.height

        onDiscardRequested: {
            backend.discardRecovery();
            win.completePendingAction();
        }

        onSaveRequested: {
            win.awaitingPendingSave = true;
            backend.save();
        }
        onCancelRequested: {
            win.pendingAction = "";
            win.pendingFragment = "";
            win.pendingFragmentKind = "";
        }
    }

    ExternalChangeDialog {
        id: externalChangeDialog
        darkMode: win.darkMode
        textScale: win.textScale
        textColor: win.textColor
        strongTextColor: win.strongTextColor
        containerWidth: win.width
        containerHeight: win.height

        onKeepRequested: backend.keepExternalVersion()
        onReloadRequested: backend.reloadFromDisk()
    }

    CommandPalette {
        id: commandPalette
        parent: Overlay.overlay
        catalog: []
        onActivated: function(item) { win.runCommand(item) }
    }

    property bool slashIgnore: false
    property bool backtickIgnore: false

    function editorCaretPoint() {
        var rect = editor.cursorRectangle
        return editor.mapToItem(win.contentItem, rect.x, rect.y + rect.height + 6)
    }

    function beginInsertCodeBlock() {
        backend.projectView = "editor"
        editor.forceActiveFocus()
        languagePicker.wrapSelected = editor.selectionStart !== editor.selectionEnd
        languagePicker.replaceStart = Math.min(editor.selectionStart, editor.selectionEnd)
        languagePicker.replaceEnd = Math.max(editor.selectionStart, editor.selectionEnd)
        languagePicker.openAt(editorCaretPoint())
    }

    function insertCodeLanguage(language, start, end, wrapSelected) {
        var inner = wrapSelected ? editor.text.slice(start, end) : ""
        var fence = backend.codeFenceText(language, inner)
        var caret = backend.codeFenceCaretOffset(language, inner)
        backend.lastCodeLanguage = language
        EditorMutations.replaceRange(editor, start, end, fence, caret, caret)
    }

    function applySlashItem(item) {
        var start = slashMenu.replaceStart
        var end = slashMenu.replaceEnd
        if (!item || !item.id)
            return
        if (item.id === "code") {
            EditorMutations.replaceRange(editor, start, end, "")
            languagePicker.wrapSelected = false
            languagePicker.replaceStart = start
            languagePicker.replaceEnd = start
            languagePicker.openAt(editorCaretPoint())
            return
        }
        if (item.id === "mermaid") {
            insertCodeLanguage("mermaid", start, end, false)
            return
        }
        if (item.id === "template") {
            if (!backend.templateFiles || backend.templateFiles.length === 0)
                backend.ensureDefaultTemplate()
            var files = backend.templateFiles
            if (files.length === 1) {
                templatePicker.replaceStart = start
                templatePicker.replaceEnd = end
                insertTemplate(files[0].url)
                return
            }
            openTemplatePicker("insert", start, end)
            return
        }
        if (item.id === "table") {
            var table = "| Column 1 | Column 2 | Column 3 |\n| -------- | -------- | -------- |\n|          |          |          |\n|          |          |          |\n"
            EditorMutations.replaceRange(editor, start, end, table, 2, 10)
            refreshTableEdit()
            return
        }
        if (item.id === "tablecol") {
            EditorMutations.replaceRange(editor, start, end, "")
            applyTableMutation(backend.insertTableColumn(editor.text, tableCursor()))
            return
        }
        if (item.id === "tablerow") {
            EditorMutations.replaceRange(editor, start, end, "")
            applyTableMutation(backend.insertTableRow(editor.text, tableCursor()))
            return
        }
        if (item.id === "toc") {
            var toc = "```toc\nstyle: bullet\nmin_depth: 2\nmax_depth: 6\n```\n"
            EditorMutations.replaceRange(editor, start, end, toc)
            return
        }
        if (item.id === "highlight") {
            EditorMutations.replaceRange(editor, start, end, "====", 2, 2)
            return
        }
        if (item.id === "color") {
            var span = "<span style=\"color:#e11d48;background:#fef3c7\"></span>"
            EditorMutations.replaceRange(editor, start, end, span, 47, 47)
            return
        }
        var replacements = {
            "heading1": "# ",
            "heading2": "## ",
            "heading3": "### ",
            "quote": "> ",
            "bullet": "- ",
            "todo": "- [ ] ",
            "divider": "---\n\n"
        }
        var text = replacements[item.id] || ""
        if (text.length > 0)
            EditorMutations.replaceRange(editor, start, end, text)
    }

    function refreshEditorBlockMenus() {
        var slash = backend.slashQueryAt(editor.text, editor.cursorPosition)
        if (slash.active) {
            slashMenu.query = slash.query
            slashMenu.replaceStart = slash.replaceStart
            slashMenu.replaceEnd = slash.replaceEnd
            if (!slashMenu.visible && !win.slashIgnore)
                slashMenu.openAt(editorCaretPoint())
        } else {
            win.slashIgnore = false
            if (slashMenu.visible)
                slashMenu.close()
        }

        var tick = backend.backtickTriggerAt(editor.text, editor.cursorPosition)
        if (tick.active) {
            languagePicker.wrapSelected = false
            languagePicker.replaceStart = tick.replaceStart
            languagePicker.replaceEnd = tick.replaceEnd
            if (!languagePicker.visible && !win.backtickIgnore)
                languagePicker.openAt(editorCaretPoint())
        } else {
            win.backtickIgnore = false
        }
    }

    SlashMenu {
        id: slashMenu
        parent: Overlay.overlay
        onActivated: function(item) { win.applySlashItem(item) }
        onClosed: win.slashIgnore = true
    }

    TemplatePicker {
        id: templatePicker
        parent: Overlay.overlay
        onChosen: function(url) {
            if (mode === "create")
                win.createNoteFromTemplate(url)
            else
                win.insertTemplate(url)
        }
    }

    LanguagePicker {
        id: languagePicker
        parent: Overlay.overlay
        onPicked: function(language) {
            win.insertCodeLanguage(language, replaceStart, replaceEnd, wrapSelected)
        }
        onClosed: win.backtickIgnore = true
    }

    Dialog {
        id: deleteNoteDialog
        objectName: "deleteNoteDialog"
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape
        width: Math.min(win.scaledSize(420), win.width - 48)
        x: Math.round((win.width - width) / 2)
        y: Math.round((win.height - height) / 2)
        padding: 20

        background: Rectangle {
            color: win.darkMode ? "#1a1a1a" : "#ffffff"
            border.color: win.panelBorderColor
            radius: 8
        }

        contentItem: Column {
            spacing: 12
            Label {
                text: "Move to Trash"
                color: win.strongTextColor
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(16)
                font.bold: true
            }
            Label {
                width: deleteNoteDialog.availableWidth
                wrapMode: Text.Wrap
                color: win.textColor
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(13)
                text: "Move “" + win.fileNameFromUrl(win.pendingDeleteUrl) + "” to Trash?"
                      + (backend.fileUrl.toString() === win.pendingDeleteUrl.toString() && backend.modified
                         ? " Unsaved changes will be lost." : "")
            }
        }

        footer: Item {
            implicitHeight: win.scaledSize(52)
            Row {
                anchors.right: parent.right
                anchors.rightMargin: 20
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8
                SquareDialogButton {
                    text: "Cancel"
                    darkMode: win.darkMode
                    textScale: win.textScale
                    labelColor: win.textColor
                    onClicked: deleteNoteDialog.close()
                }
                SquareDialogButton {
                    text: "Move to Trash"
                    primary: true
                    darkMode: win.darkMode
                    textScale: win.textScale
                    activeColor: backend.themeAccent
                    onClicked: {
                        backend.moveNoteToTrash(win.pendingDeleteUrl)
                        deleteNoteDialog.close()
                    }
                }
            }
        }
    }

    Dialog {
        id: threadsPublishConfirmDialog
        objectName: "threadsPublishConfirmDialog"
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape
        width: Math.min(win.scaledSize(460), win.width - 48)
        x: Math.round((win.width - width) / 2)
        y: Math.round((win.height - height) / 2)
        padding: 20

        background: Rectangle {
            color: win.darkMode ? "#1a1a1a" : "#ffffff"
            border.color: win.panelBorderColor
            radius: 8
        }

        contentItem: Column {
            spacing: 12
            Label {
                text: win.pendingPublishDryRun ? "Dry-run publish" : "立刻發 Threads"
                color: win.strongTextColor
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(16)
                font.bold: true
            }
            Label {
                width: threadsPublishConfirmDialog.availableWidth
                wrapMode: Text.Wrap
                color: win.textColor
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(13)
                text: backend.threadsBusy
                      ? "正在發布，請稍候。FMD 在等 threads-schedule，不會自己連 Threads。取消可中止。"
                      : (win.fileNameFromUrl(backend.fileUrl)
                         + "\nmain " + (win.threadsPublishReport.mainChars || 0)
                         + " chars · " + (win.threadsPublishReport.replies || 0) + " replies"
                         + (win.pendingPublishDryRun
                            ? "\n\nDry-run only — threads-schedule will not post."
                            : "\n\nFMD will call threads-schedule publish-now. It does not talk to Threads itself."))
            }
        }

        footer: Item {
            implicitHeight: win.scaledSize(52)
            Row {
                anchors.right: parent.right
                anchors.rightMargin: 20
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8
                SquareDialogButton {
                    text: "Cancel"
                    darkMode: win.darkMode
                    textScale: win.textScale
                    labelColor: win.textColor
                    onClicked: {
                        if (backend.threadsBusy)
                            backend.abortThreadsPublish()
                        threadsPublishConfirmDialog.close()
                    }
                }
                SquareDialogButton {
                    text: backend.threadsBusy
                          ? "Publishing…"
                          : (win.pendingPublishDryRun ? "Dry-run" : "Publish now")
                    primary: true
                    enabled: !backend.threadsBusy
                    darkMode: win.darkMode
                    textScale: win.textScale
                    activeColor: backend.themeAccent
                    onClicked: win.confirmPublishThreads()
                }
            }
        }
    }

    Dialog {
        id: threadsPublishErrorDialog
        objectName: "threadsPublishErrorDialog"
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape
        width: Math.min(win.scaledSize(480), win.width - 48)
        x: Math.round((win.width - width) / 2)
        y: Math.round((win.height - height) / 2)
        padding: 20

        background: Rectangle {
            color: win.darkMode ? "#1a1a1a" : "#ffffff"
            border.color: win.panelBorderColor
            radius: 8
        }

        contentItem: Column {
            spacing: 12
            Label {
                text: "Threads check failed"
                color: win.strongTextColor
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(16)
                font.bold: true
            }
            Label {
                width: threadsPublishErrorDialog.availableWidth
                wrapMode: Text.Wrap
                color: win.textColor
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(12)
                text: (win.threadsPublishReport && win.threadsPublishReport.output)
                      ? win.threadsPublishReport.output
                      : "threads-schedule returned an error."
            }
        }

        footer: Item {
            implicitHeight: win.scaledSize(52)
            Row {
                anchors.right: parent.right
                anchors.rightMargin: 20
                anchors.verticalCenter: parent.verticalCenter
                SquareDialogButton {
                    text: "OK"
                    primary: true
                    darkMode: win.darkMode
                    textScale: win.textScale
                    activeColor: backend.themeAccent
                    onClicked: threadsPublishErrorDialog.close()
                }
            }
        }
    }

    Dialog {
        id: slidevPublishConfirmDialog
        objectName: "slidevPublishConfirmDialog"
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape
        width: Math.min(win.scaledSize(460), win.width - 48)
        x: Math.round((win.width - width) / 2)
        y: Math.round((win.height - height) / 2)
        padding: 20

        background: Rectangle {
            color: win.darkMode ? "#1a1a1a" : "#ffffff"
            border.color: win.panelBorderColor
            radius: 8
        }

        contentItem: Column {
            spacing: 12
            Label {
                text: win.pendingSlidevDryRun ? "Dry-run Slidev" : (backend.uiLanguage, backend.t("publishSlidevNow"))
                color: win.strongTextColor
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(16)
                font.bold: true
            }
            Label {
                width: slidevPublishConfirmDialog.availableWidth
                wrapMode: Text.Wrap
                color: win.textColor
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(13)
                text: backend.slidevBusy
                      ? (backend.uiLanguage, backend.t("slidevBusyHelp"))
                      : ((win.slidevPublishReport && win.slidevPublishReport.message)
                         ? win.slidevPublishReport.message
                         : "")
                         + (win.pendingSlidevDryRun
                            ? "\n\nDry-run only — will not start Vite."
                            : "\n\nFMD copies the deck then calls slidev-present. Vite stays outside the app.")
            }
        }

        footer: Item {
            implicitHeight: win.scaledSize(52)
            Row {
                anchors.right: parent.right
                anchors.rightMargin: 20
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8
                SquareDialogButton {
                    text: "Cancel"
                    darkMode: win.darkMode
                    textScale: win.textScale
                    labelColor: win.textColor
                    onClicked: {
                        if (backend.slidevBusy)
                            backend.abortSlidevPublish()
                        slidevPublishConfirmDialog.close()
                    }
                }
                SquareDialogButton {
                    text: backend.slidevBusy
                          ? "Starting…"
                          : (win.pendingSlidevDryRun ? "Dry-run" : "Present")
                    primary: true
                    enabled: !backend.slidevBusy
                    darkMode: win.darkMode
                    textScale: win.textScale
                    activeColor: backend.themeAccent
                    onClicked: win.confirmPublishSlidev()
                }
            }
        }
    }

    Dialog {
        id: slidevPublishErrorDialog
        objectName: "slidevPublishErrorDialog"
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape
        width: Math.min(win.scaledSize(480), win.width - 48)
        x: Math.round((win.width - width) / 2)
        y: Math.round((win.height - height) / 2)
        padding: 20

        background: Rectangle {
            color: win.darkMode ? "#1a1a1a" : "#ffffff"
            border.color: win.panelBorderColor
            radius: 8
        }

        contentItem: Column {
            spacing: 12
            Label {
                text: (backend.uiLanguage, backend.t("slidevCheckFailed"))
                color: win.strongTextColor
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(16)
                font.bold: true
            }
            Label {
                width: slidevPublishErrorDialog.availableWidth
                wrapMode: Text.Wrap
                color: win.textColor
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(12)
                text: (win.slidevPublishReport && (win.slidevPublishReport.output || win.slidevPublishReport.message))
                      ? (win.slidevPublishReport.output || win.slidevPublishReport.message)
                      : "slidev-present returned an error."
            }
        }

        footer: Item {
            implicitHeight: win.scaledSize(52)
            Row {
                anchors.right: parent.right
                anchors.rightMargin: 20
                SquareDialogButton {
                    text: "OK"
                    primary: true
                    darkMode: win.darkMode
                    textScale: win.textScale
                    activeColor: backend.themeAccent
                    onClicked: slidevPublishErrorDialog.close()
                }
            }
        }
    }

    Dialog {
        id: gardenPublishConfirmDialog
        objectName: "gardenPublishConfirmDialog"
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape
        width: Math.min(win.scaledSize(460), win.width - 48)
        x: Math.round((win.width - width) / 2)
        y: Math.round((win.height - height) / 2)
        padding: 20

        background: Rectangle {
            color: win.darkMode ? "#1a1a1a" : "#ffffff"
            border.color: win.panelBorderColor
            radius: 8
        }

        contentItem: Column {
            spacing: 12
            Label {
                text: (backend.uiLanguage, backend.t("publishGardenNow"))
                color: win.strongTextColor
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(16)
                font.bold: true
            }
            Label {
                width: gardenPublishConfirmDialog.availableWidth
                wrapMode: Text.Wrap
                color: win.textColor
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(13)
                text: (backend.uiLanguage, backend.t("publishGardenConfirmHelp"))
            }
        }

        footer: Item {
            implicitHeight: win.scaledSize(52)
            Row {
                anchors.right: parent.right
                anchors.rightMargin: 20
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8
                SquareDialogButton {
                    text: "Cancel"
                    darkMode: win.darkMode
                    textScale: win.textScale
                    labelColor: win.textColor
                    onClicked: gardenPublishConfirmDialog.close()
                }
                SquareDialogButton {
                    text: backend.sitePushBusy ? "Pushing…" : (backend.uiLanguage, backend.t("publishGardenNow"))
                    primary: true
                    enabled: !backend.sitePushBusy
                    darkMode: win.darkMode
                    textScale: win.textScale
                    activeColor: backend.themeAccent
                    onClicked: win.confirmPublishGarden()
                }
            }
        }
    }

    Dialog {
        id: gardenPublishErrorDialog
        objectName: "gardenPublishErrorDialog"
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape
        width: Math.min(win.scaledSize(480), win.width - 48)
        x: Math.round((win.width - width) / 2)
        y: Math.round((win.height - height) / 2)
        padding: 20

        background: Rectangle {
            color: win.darkMode ? "#1a1a1a" : "#ffffff"
            border.color: win.panelBorderColor
            radius: 8
        }

        contentItem: Column {
            spacing: 12
            Label {
                text: (backend.uiLanguage, backend.t("gardenPushFailed"))
                color: win.strongTextColor
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(16)
                font.bold: true
            }
            Label {
                width: gardenPublishErrorDialog.availableWidth
                wrapMode: Text.Wrap
                color: win.textColor
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(12)
                text: (win.gardenPublishReport && (win.gardenPublishReport.output || win.gardenPublishReport.message))
                      ? (win.gardenPublishReport.output || win.gardenPublishReport.message)
                      : "site-push returned an error."
            }
        }

        footer: Item {
            implicitHeight: win.scaledSize(52)
            Row {
                anchors.right: parent.right
                SquareDialogButton {
                    text: "OK"
                    primary: true
                    darkMode: win.darkMode
                    textScale: win.textScale
                    activeColor: backend.themeAccent
                    onClicked: gardenPublishErrorDialog.close()
                }
            }
        }
    }

    Dialog {
        id: preferencesDialog
        objectName: "preferencesDialog"
        modal: true
        title: (backend.uiLanguage, backend.t("preferences"))
        standardButtons: Dialog.Close
        padding: 16
        bottomPadding: 8
        width: Math.min(win.width - 48, win.scaledSize(620))
        height: Math.min(win.height - 72, win.scaledSize(620))
        x: Math.round((win.width - width) / 2)
        y: Math.round((win.height - height) / 2)
        onOpened: languageBox.forceActiveFocus()
        contentItem: Item {
            implicitWidth: win.scaledSize(560)
            implicitHeight: win.scaledSize(420)

            Flickable {
                id: prefsFlick
                objectName: "preferencesFlick"
                anchors.fill: parent
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                contentWidth: width
                contentHeight: prefsColumn.implicitHeight + 28
                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AlwaysOn
                }

                Column {
                    id: prefsColumn
                    width: Math.max(100, prefsFlick.width - 18)
                    spacing: 14

            Label {
                width: parent ? parent.width : implicitWidth
                text: (backend.uiLanguage, backend.t("interfaceLanguage"))
                color: win.strongTextColor
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(13)
                font.bold: true
            }

            Label {
                width: parent ? parent.width : implicitWidth
                text: (backend.uiLanguage, backend.t("languageHelp"))
                color: win.mutedColor
                wrapMode: Text.Wrap
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(11)
            }

            ComboBox {
                id: languageBox
                objectName: "uiLanguageBox"
                width: Math.min(win.scaledSize(240), parent ? parent.width : win.scaledSize(240))
                textRole: "label"
                valueRole: "code"
                model: [
                    { "code": "zh-TW", "label": "繁體中文" },
                    { "code": "en", "label": "English" }
                ]
                Component.onCompleted: currentIndex = backend.uiLanguage === "en" ? 1 : 0
                onActivated: backend.uiLanguage = currentValue
            }

            Rectangle {
                width: parent ? parent.width : implicitWidth
                height: 1
                color: win.panelBorderColor
            }

            Label {
                width: parent ? parent.width : implicitWidth
                text: "Workspace panels"
                color: win.strongTextColor
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(13)
                font.bold: true
            }

            CheckBox {
                id: sidebarPreferenceCheckbox
                objectName: "sidebarPreferenceCheckbox"
                text: "Show article sidebar"
                checked: backend.sidebarVisible
                onToggled: backend.sidebarVisible = checked
            }

            CheckBox {
                objectName: "previewPreferenceCheckbox"
                text: "Show live preview"
                checked: backend.previewVisible
                onToggled: backend.previewVisible = checked
            }

            CheckBox {
                objectName: "typewriterScrollCheckbox"
                text: (backend.uiLanguage, backend.t("typewriterScroll"))
                checked: backend.typewriterScroll
                onToggled: backend.typewriterScroll = checked
            }

            CheckBox {
                objectName: "confirmUnsavedCheckbox"
                text: (backend.uiLanguage, backend.t("confirmUnsaved"))
                checked: backend.confirmUnsavedChanges
                onToggled: backend.confirmUnsavedChanges = checked
            }

            Label {
                width: parent ? parent.width : implicitWidth
                wrapMode: Text.Wrap
                text: (backend.uiLanguage, backend.t("confirmUnsavedHelp"))
                color: win.mutedColor
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(11)
            }

            Rectangle {
                width: parent ? parent.width : implicitWidth
                height: 1
                color: win.panelBorderColor
            }

            Label {
                width: parent ? parent.width : implicitWidth
                text: (backend.uiLanguage, backend.t("threadsFolder"))
                color: win.strongTextColor
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(13)
                font.bold: true
            }

            Label {
                width: parent ? parent.width : implicitWidth
                text: (backend.uiLanguage, backend.t("threadsFolderHelp"))
                color: win.mutedColor
                wrapMode: Text.Wrap
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(11)
            }

            RowLayout {
                width: parent ? parent.width : implicitWidth
                spacing: 8

                TextField {
                    id: threadsFolderField
                    objectName: "threadsFolderField"
                    Layout.fillWidth: true
                    text: backend.threadsDraftsFolder
                    placeholderText: backend.defaultThreadsDraftsFolder
                    font.family: "iA Writer Mono S"
                    font.pixelSize: win.scaledSize(12)
                    onEditingFinished: backend.threadsDraftsFolder = text
                }

                Button {
                    text: (backend.uiLanguage, backend.t("browse"))
                    font.family: "iA Writer Mono S"
                    font.pixelSize: win.scaledSize(12)
                    onClicked: {
                        threadsFolderDialog.currentFolder = Qt.resolvedUrl(backend.resolvedThreadsDraftsFolder())
                        threadsFolderDialog.open()
                    }
                }
            }

            Label {
                width: parent ? parent.width : implicitWidth
                text: (backend.uiLanguage, backend.t("notesSiteFolder"))
                color: win.strongTextColor
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(13)
                font.bold: true
            }
            Label {
                width: parent ? parent.width : implicitWidth
                text: (backend.uiLanguage, backend.t("notesSiteFolderHelp"))
                color: win.mutedColor
                wrapMode: Text.Wrap
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(11)
            }
            RowLayout {
                width: parent ? parent.width : implicitWidth
                spacing: 8
                TextField {
                    Layout.fillWidth: true
                    text: backend.notesSiteFolder
                    placeholderText: backend.defaultNotesSiteFolder
                    font.family: "iA Writer Mono S"
                    font.pixelSize: win.scaledSize(12)
                    onEditingFinished: backend.notesSiteFolder = text
                }
                Button {
                    text: (backend.uiLanguage, backend.t("browse"))
                    onClicked: {
                        notesSiteFolderDialog.currentFolder =
                                Qt.resolvedUrl(backend.resolvedNotesSiteFolder)
                        notesSiteFolderDialog.open()
                    }
                }
            }
            Label {
                width: parent ? parent.width : implicitWidth
                text: (backend.uiLanguage, backend.t("changelogSiteFolder"))
                color: win.strongTextColor
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(13)
                font.bold: true
            }
            RowLayout {
                width: parent ? parent.width : implicitWidth
                spacing: 8
                TextField {
                    Layout.fillWidth: true
                    text: backend.changelogSiteFolder
                    placeholderText: backend.defaultChangelogSiteFolder
                    font.family: "iA Writer Mono S"
                    font.pixelSize: win.scaledSize(12)
                    onEditingFinished: backend.changelogSiteFolder = text
                }
                Button {
                    text: (backend.uiLanguage, backend.t("browse"))
                    onClicked: {
                        changelogSiteFolderDialog.currentFolder =
                                Qt.resolvedUrl(backend.resolvedChangelogSiteFolder)
                        changelogSiteFolderDialog.open()
                    }
                }
            }

            Label {
                width: parent ? parent.width : implicitWidth
                text: (backend.uiLanguage, backend.t("slidesSiteFolder"))
                color: win.strongTextColor
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(13)
                font.bold: true
            }
            Label {
                width: parent ? parent.width : implicitWidth
                text: (backend.uiLanguage, backend.t("slidesSiteFolderHelp"))
                color: win.mutedColor
                wrapMode: Text.Wrap
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(11)
            }
            RowLayout {
                width: parent ? parent.width : implicitWidth
                spacing: 8
                TextField {
                    Layout.fillWidth: true
                    text: backend.slidesSiteFolder
                    placeholderText: backend.defaultSlidesSiteFolder
                    font.family: "iA Writer Mono S"
                    font.pixelSize: win.scaledSize(12)
                    onEditingFinished: backend.slidesSiteFolder = text
                }
                Button {
                    text: (backend.uiLanguage, backend.t("browse"))
                    onClicked: {
                        slidesSiteFolderDialog.currentFolder = Qt.resolvedUrl(backend.resolvedSlidesSiteFolder())
                        slidesSiteFolderDialog.open()
                    }
                }
            }

            Label {
                width: parent ? parent.width : implicitWidth
                text: "Threads preview shows this name and handle. It never posts."
                color: win.mutedColor
                wrapMode: Text.Wrap
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(11)
            }

            RowLayout {
                width: parent ? parent.width : implicitWidth
                spacing: 8
                TextField {
                    objectName: "threadsDisplayNameField"
                    Layout.fillWidth: true
                    text: backend.threadsDisplayName
                    placeholderText: "Display name"
                    font.family: "iA Writer Mono S"
                    font.pixelSize: win.scaledSize(12)
                    onEditingFinished: backend.threadsDisplayName = text
                }
                TextField {
                    objectName: "threadsHandleField"
                    Layout.fillWidth: true
                    text: backend.threadsHandle
                    placeholderText: "handle"
                    font.family: "iA Writer Mono S"
                    font.pixelSize: win.scaledSize(12)
                    onEditingFinished: backend.threadsHandle = text
                }
            }

            Rectangle {
                width: parent ? parent.width : implicitWidth
                height: 1
                color: win.panelBorderColor
            }

            HotkeysPane {
                width: parent ? parent.width : implicitWidth
            }

            Label {
                id: shortcutText
                objectName: "shortcutText"
                visible: false
                width: parent ? parent.width : implicitWidth
                text: "Ctrl/Cmd+P  Command palette\nCtrl/Cmd+S  Save\nCtrl/Cmd+Shift+S  Save As\nCtrl/Cmd+O  Open File\nCtrl/Cmd+Shift+O  Open Folder\nCtrl/Cmd+F  Find\nCtrl+H / Cmd+Option+F  Find and Replace\nCtrl/Cmd+N  New Window\nCtrl/Cmd+Shift+N  New Markdown file\nCtrl/Cmd+Shift+T  New Threads post\nCtrl/Cmd+Shift+Enter  Publish Threads now\nCtrl/Cmd+Backspace  Move note to Trash\nCtrl/Cmd+Shift+[  Previous heading\nCtrl/Cmd+Shift+]  Next heading\nCtrl/Cmd+,  Preferences\nCtrl/Cmd+Shift+L  Toggle Sidebar\nCtrl/Cmd+Shift+P  Toggle Preview\nCtrl/Cmd+1  Editor\nCtrl/Cmd+2  Table\nCtrl/Cmd+3  Board\nCtrl/Cmd+4  Calendar\nCtrl/Cmd+5  Mindmap\nCtrl/Cmd+B  Bold\nCtrl/Cmd+I  Italic\nCtrl/Cmd+K  Link\n/ then Code  Insert code block\n```  Code block language picker\nShift+Enter  Leave code block\nF11 / Super+F  Fullscreen\nCtrl/Cmd+?  Preferences and Shortcuts"
            }

            Label {
                width: parent ? parent.width : implicitWidth
                text: "Pasting a screenshot or image file copies it into a {filename}.assets folder beside the Markdown file and inserts a relative image link. Save the document first. Drag the gaps between sidebar, editor, and preview to resize them."
                color: win.mutedColor
                wrapMode: Text.Wrap
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(11)
            }

            Label {
                width: parent ? parent.width : implicitWidth
                text: "Preview themes Night / Newsprint / Gothic follow Craft-like measure and spacing, with color cues from Typora's open-source default themes."
                color: win.mutedColor
                wrapMode: Text.Wrap
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(11)
            }

            PluginsPane {
                width: parent ? parent.width : implicitWidth
            }

            Item {
                width: 1
                height: 20
            }
                }
            }
        }
    }

    Item {
        id: chromeRoot
        anchors.fill: parent

        Pane {
            id: sidebarPane
            visible: win.showSidebar
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: statusBar.top
            anchors.margins: 8
            width: backend.sidebarSplitWidth
            padding: 0

            background: Rectangle {
                color: win.panelColor
                border.color: win.panelBorderColor
                radius: 10
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                RowLayout {
                    Layout.fillWidth: true
                    Layout.topMargin: 10
                    Layout.leftMargin: 14
                    Layout.rightMargin: 6
                    Layout.bottomMargin: 4
                    spacing: 8

                    Label {
                        Layout.fillWidth: true
                        text: backend.workspaceFolderName.length > 0
                            ? backend.workspaceFolderName
                            : (backend.uiLanguage, backend.t("files"))
                        color: win.strongTextColor
                        font.family: "iA Writer Mono S"
                        font.pixelSize: win.scaledSize(13)
                        font.bold: true
                        elide: Text.ElideMiddle
                    }

                    ToolButton {
                        objectName: "hideSidebarButton"
                        text: (backend.uiLanguage, backend.t("hide"))
                        implicitWidth: win.scaledSize(44)
                        implicitHeight: win.scaledSize(28)
                        padding: 4
                        font.family: "iA Writer Mono S"
                        font.pixelSize: win.scaledSize(11)
                        ToolTip.visible: hovered
                        ToolTip.text: "Hide sidebar (Ctrl/Cmd+Shift+L)"
                        onClicked: toggleSidebarVisibility()
                    }
                }

                Label {
                    Layout.fillWidth: true
                    Layout.leftMargin: 14
                    Layout.rightMargin: 14
                    Layout.bottomMargin: 8
                    text: backend.workspaceFolderPath
                    visible: text.length > 0
                    color: win.mutedColor
                    font.family: "iA Writer Mono S"
                    font.pixelSize: win.scaledSize(11)
                    elide: Text.ElideMiddle
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 10
                    Layout.rightMargin: 10
                    Layout.bottomMargin: 10
                    spacing: 6

                    Button {
                        id: openFolderButton
                        objectName: "openFolderButton"
                        Layout.fillWidth: true
                        implicitHeight: win.scaledSize(32)
                        flat: true
                        text: backend.workspaceFolderPath.length > 0
                              ? (backend.uiLanguage, backend.t("changeFolder"))
                              : (backend.uiLanguage, backend.t("openFolder"))
                        font.family: "iA Writer Mono S"
                        font.pixelSize: win.scaledSize(12)
                        onClicked: backend.openFolderDialog()
                    }

                    ToolButton {
                        objectName: "recentWorkspacesButton"
                        visible: backend.recentWorkspaces.length > 0
                        text: "▾"
                        implicitWidth: win.scaledSize(28)
                        implicitHeight: win.scaledSize(32)
                        font.family: "iA Writer Mono S"
                        font.pixelSize: win.scaledSize(12)
                        ToolTip.visible: hovered
                        ToolTip.text: (backend.uiLanguage, backend.t("recentProjects"))
                        onClicked: recentProjectsMenu.popup()
                    }

                    Menu {
                        id: recentProjectsMenu
                        Instantiator {
                            model: backend.recentWorkspaces
                            delegate: MenuItem {
                                required property var modelData
                                text: modelData.name
                                onTriggered: backend.openFolder(modelData.url)
                            }
                            onObjectAdded: function(index, object) {
                                recentProjectsMenu.insertItem(index, object)
                            }
                            onObjectRemoved: function(index, object) {
                                recentProjectsMenu.removeItem(object)
                            }
                        }
                    }

                    ToolButton {
                        objectName: "newMarkdownNoteButton"
                        text: "+"
                        implicitWidth: win.scaledSize(32)
                        implicitHeight: win.scaledSize(32)
                        font.family: "iA Writer Mono S"
                        font.pixelSize: win.scaledSize(16)
                        ToolTip.visible: hovered
                        ToolTip.text: "New Markdown file (⌘⇧N)"
                        onClicked: win.createMarkdownNote()
                    }

                    ToolButton {
                        objectName: "newFolderButton"
                        text: "▸+"
                        implicitWidth: win.scaledSize(36)
                        implicitHeight: win.scaledSize(32)
                        font.family: "iA Writer Mono S"
                        font.pixelSize: win.scaledSize(13)
                        ToolTip.visible: hovered
                        ToolTip.text: (backend.uiLanguage, backend.t("newFolder"))
                        onClicked: win.createFolder()
                    }

                    ToolButton {
                        objectName: "collapseAllFoldersButton"
                        enabled: backend.workspaceFolderPath.length > 0
                        text: backend.workspaceFoldersCollapsed ? "▸▸" : "▾▾"
                        implicitWidth: win.scaledSize(32)
                        implicitHeight: win.scaledSize(32)
                        font.family: "iA Writer Mono S"
                        font.pixelSize: win.scaledSize(12)
                        ToolTip.visible: hovered
                        ToolTip.text: backend.workspaceFoldersCollapsed
                                      ? (backend.uiLanguage, backend.t("expandAllFolders"))
                                      : (backend.uiLanguage, backend.t("collapseAllFolders"))
                        onClicked: backend.toggleAllWorkspaceFolders()
                    }

                    ToolButton {
                        objectName: "fileSortButton"
                        text: "↕"
                        implicitWidth: win.scaledSize(28)
                        implicitHeight: win.scaledSize(32)
                        font.family: "iA Writer Mono S"
                        font.pixelSize: win.scaledSize(13)
                        ToolTip.visible: hovered
                        ToolTip.text: (backend.uiLanguage, backend.t("sortFiles"))
                        onClicked: fileSortMenu.popup()
                    }

                    Menu {
                        id: fileSortMenu
                        MenuItem {
                            text: (backend.uiLanguage, backend.t("sortNameAsc"))
                            checked: backend.fileSort === "name-asc"
                            checkable: true
                            onTriggered: backend.fileSort = "name-asc"
                        }
                        MenuItem {
                            text: (backend.uiLanguage, backend.t("sortNameDesc"))
                            checked: backend.fileSort === "name-desc"
                            checkable: true
                            onTriggered: backend.fileSort = "name-desc"
                        }
                        MenuItem {
                            text: (backend.uiLanguage, backend.t("sortMtimeDesc"))
                            checked: backend.fileSort === "mtime-desc"
                            checkable: true
                            onTriggered: backend.fileSort = "mtime-desc"
                        }
                        MenuItem {
                            text: (backend.uiLanguage, backend.t("sortMtimeAsc"))
                            checked: backend.fileSort === "mtime-asc"
                            checkable: true
                            onTriggered: backend.fileSort = "mtime-asc"
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 1
                    color: win.panelBorderColor
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: win.scaledSize(32)
                    Layout.leftMargin: 6
                    Layout.rightMargin: 6
                    Layout.topMargin: 4
                    Layout.bottomMargin: 4
                    spacing: 2

                    Repeater {
                        model: ["files", "outline", "tags", "recent", "links"]
                        ToolButton {
                            required property string modelData
                            Layout.fillWidth: true
                            implicitHeight: win.scaledSize(28)
                            text: (backend.uiLanguage, backend.t(modelData))
                            checkable: true
                            checked: win.sidebarSection === modelData
                            font.family: "iA Writer Mono S"
                            font.pixelSize: win.scaledSize(10)
                            font.bold: checked
                            onClicked: win.sidebarSection = modelData
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 1
                    color: win.panelBorderColor
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 6
                        visible: win.sidebarSection === "files"

                        RowLayout {
                            Layout.fillWidth: true
                            Label {
                                Layout.fillWidth: true
                                text: (backend.uiLanguage, backend.t("inbox"))
                                color: win.strongTextColor
                                font.family: "iA Writer Mono S"
                                font.pixelSize: win.scaledSize(11)
                                font.bold: true
                            }
                            ToolButton {
                                objectName: "newInboxNoteButton"
                                text: "+"
                                implicitWidth: win.scaledSize(28)
                                implicitHeight: win.scaledSize(24)
                                font.family: "iA Writer Mono S"
                                ToolTip.visible: hovered
                                ToolTip.text: (backend.uiLanguage, backend.t("newInboxNote"))
                                onClicked: openInboxNote(backend.createInboxNote())
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            visible: backend.inboxFiles.length === 0
                            wrapMode: Text.Wrap
                            text: (backend.uiLanguage, backend.t("inboxHelp"))
                            color: win.mutedColor
                            font.family: "iA Writer Mono S"
                            font.pixelSize: win.scaledSize(10)
                        }

                        Repeater {
                            model: backend.inboxFiles
                            delegate: ItemDelegate {
                                required property var modelData
                                Layout.fillWidth: true
                                implicitHeight: win.scaledSize(28)
                                highlighted: backend.fileUrl.toString() === modelData.url.toString()
                                onClicked: openInboxNote(modelData.url)
                                contentItem: Label {
                                    text: modelData.fileName || modelData.name
                                    elide: Text.ElideMiddle
                                    color: parent.highlighted ? win.strongTextColor : win.textColor
                                    font.family: "iA Writer Mono S"
                                    font.pixelSize: win.scaledSize(11)
                                }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            implicitHeight: 1
                            color: win.panelBorderColor
                        }

                        Label {
                            Layout.fillWidth: true
                            visible: backend.tagFilter.length > 0
                            padding: 8
                            wrapMode: Text.Wrap
                            text: "Notes tagged #" + backend.tagFilter
                                  + ". Click a file to open. Click this banner to clear."
                            color: win.mutedColor
                            font.family: "iA Writer Mono S"
                            font.pixelSize: win.scaledSize(11)
                            background: Rectangle {
                                radius: 6
                                color: win.currentFileColor
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: backend.tagFilter = ""
                            }
                        }

                        WorkspaceNavPane {
                            id: workspaceNavPane
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }
                    }

                    OutlinePane {
                        id: outlinePane
                        anchors.fill: parent
                        visible: win.sidebarSection === "outline"
                        headings: backend.documentOutline
                        cursorPosition: editor.cursorPosition
                        maxLevel: backend.outlineMaxLevel
                        onJumpTo: function(position) { win.jumpToOutline(position) }
                        onMaxLevelChosen: function(level) { backend.outlineMaxLevel = level }
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 8
                        visible: win.sidebarSection === "tags"

                        Label {
                            Layout.fillWidth: true
                            wrapMode: Text.Wrap
                            color: win.mutedColor
                            font.family: "iA Writer Mono S"
                            font.pixelSize: win.scaledSize(11)
                            text: "Click a tag to list matching notes — it does not open a file. Then click a note. Rename merges that tag across this folder."
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            TextField {
                                id: newTagField
                                Layout.fillWidth: true
                                placeholderText: "Add #tag to this note"
                                font.family: "iA Writer Mono S"
                                font.pixelSize: win.scaledSize(11)
                                onAccepted: {
                                    backend.insertTag(text)
                                    text = ""
                                }
                            }
                            PaneButton {
                                text: "Add"
                                onClicked: {
                                    backend.insertTag(newTagField.text)
                                    newTagField.text = ""
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            TextField {
                                id: renameFromField
                                Layout.fillWidth: true
                                placeholderText: "Rename from"
                                font.family: "iA Writer Mono S"
                                font.pixelSize: win.scaledSize(11)
                            }
                            TextField {
                                id: renameToField
                                Layout.fillWidth: true
                                placeholderText: "to"
                                font.family: "iA Writer Mono S"
                                font.pixelSize: win.scaledSize(11)
                                onAccepted: {
                                    backend.renameTag(renameFromField.text, text)
                                    renameFromField.text = ""
                                    text = ""
                                }
                            }
                            PaneButton {
                                text: "Rename"
                                onClicked: {
                                    backend.renameTag(renameFromField.text, renameToField.text)
                                    renameFromField.text = ""
                                    renameToField.text = ""
                                }
                            }
                        }

                        ListView {
                            id: tagSidebar
                            objectName: "tagSidebar"
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            model: backend.workspaceTags
                            spacing: 2
                            boundsBehavior: Flickable.StopAtBounds
                            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                            delegate: ItemDelegate {
                                required property var modelData
                                width: ListView.view.width
                                leftPadding: 10
                                rightPadding: 10
                                topPadding: 8
                                bottomPadding: 8
                                highlighted: backend.tagFilter.toLowerCase() === modelData.name.toLowerCase()
                                text: "#" + modelData.name + "  " + modelData.count
                                font.family: "iA Writer Mono S"
                                font.pixelSize: win.scaledSize(12)
                                onClicked: {
                                    renameFromField.text = modelData.name
                                    backend.tagFilter = modelData.name
                                    win.sidebarSection = "files"
                                }
                                ToolTip.visible: hovered
                                ToolTip.text: "List notes with #" + modelData.name + " — does not open a file"
                                background: Rectangle {
                                    radius: 6
                                    color: parent.highlighted ? win.currentFileColor : "transparent"
                                }
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            wrapMode: Text.Wrap
                            color: win.mutedColor
                            font.family: "iA Writer Mono S"
                            font.pixelSize: win.scaledSize(12)
                            visible: tagSidebar.count === 0
                            text: backend.workspaceFolderPath.length > 0
                                  ? "No tags in this folder yet. Add YAML tags: [one, two] or type #one in a note."
                                  : "Open a folder to collect tags from its notes."
                        }
                    }

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 6
                        visible: win.sidebarSection === "recent"

                        Label {
                            Layout.fillWidth: true
                            text: (backend.uiLanguage, backend.t("recentProjects"))
                            color: win.mutedColor
                            font.family: "iA Writer Mono S"
                            font.pixelSize: win.scaledSize(11)
                            visible: recentWorkspaceList.count > 0
                        }

                        ListView {
                            id: recentWorkspaceList
                            objectName: "recentWorkspacesList"
                            Layout.fillWidth: true
                            Layout.preferredHeight: Math.min(count * win.scaledSize(36),
                                                             win.scaledSize(160))
                            visible: count > 0
                            clip: true
                            model: backend.recentWorkspaces
                            spacing: 2
                            boundsBehavior: Flickable.StopAtBounds
                            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                            delegate: ItemDelegate {
                                required property var modelData
                                width: ListView.view.width
                                text: modelData.name
                                font.family: "iA Writer Mono S"
                                font.pixelSize: win.scaledSize(12)
                                leftPadding: 12
                                rightPadding: 12
                                topPadding: 8
                                bottomPadding: 8
                                highlighted: backend.workspaceFolderPath === modelData.path
                                onClicked: backend.openFolder(modelData.url)
                                ToolTip.visible: hovered
                                ToolTip.text: modelData.path || ""
                                background: Rectangle {
                                    radius: 8
                                    color: parent.highlighted ? win.currentFileColor : "transparent"
                                }
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            text: (backend.uiLanguage, backend.t("recentNotes"))
                            color: win.mutedColor
                            font.family: "iA Writer Mono S"
                            font.pixelSize: win.scaledSize(11)
                            visible: recentSidebar.count > 0
                        }

                        ListView {
                            id: recentSidebar
                            objectName: "recentSidebar"
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            visible: count > 0
                            model: backend.recentFiles
                            spacing: 4
                            boundsBehavior: Flickable.StopAtBounds
                            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                            delegate: ItemDelegate {
                                required property var modelData
                                width: ListView.view.width
                                text: modelData.name
                                font.family: "iA Writer Mono S"
                                font.pixelSize: win.scaledSize(13)
                                leftPadding: 14
                                rightPadding: 14
                                topPadding: 12
                                bottomPadding: 12
                                highlighted: backend.fileUrl.toString() === modelData.url.toString()
                                onClicked: win.requestOpen(modelData.url)
                                background: Rectangle {
                                    radius: 8
                                    color: parent.highlighted ? win.currentFileColor : "transparent"
                                }
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            wrapMode: Text.Wrap
                            color: win.mutedColor
                            font.family: "iA Writer Mono S"
                            font.pixelSize: win.scaledSize(12)
                            visible: recentWorkspaceList.count === 0 && recentSidebar.count === 0
                            text: (backend.uiLanguage, backend.t("noRecentProjects"))
                        }
                    }

                    ColumnLayout {
                        id: linksSidebar
                        objectName: "linksSidebar"
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 8
                        visible: win.sidebarSection === "links"

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4
                            PaneButton {
                                Layout.fillWidth: true
                                text: (backend.uiLanguage, backend.t("insertTemplate"))
                                onClicked: win.openTemplatePicker("insert")
                            }
                            PaneButton {
                                Layout.fillWidth: true
                                text: (backend.uiLanguage, backend.t("newFromTemplate"))
                                onClicked: win.openTemplatePicker("create")
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            text: (backend.uiLanguage, backend.t("backlinks"))
                            color: win.strongTextColor
                            font.family: "iA Writer Mono S"
                            font.pixelSize: win.scaledSize(11)
                            font.bold: true
                        }

                        ListView {
                            id: backlinkList
                            objectName: "backlinkList"
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            visible: count > 0
                            model: backend.backlinks
                            spacing: 2
                            boundsBehavior: Flickable.StopAtBounds
                            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                            delegate: ItemDelegate {
                                required property var modelData
                                width: ListView.view.width
                                leftPadding: 10
                                rightPadding: 10
                                topPadding: 8
                                bottomPadding: 8
                                text: modelData.title && modelData.title.length > 0
                                      ? modelData.title : modelData.name
                                font.family: "iA Writer Mono S"
                                font.pixelSize: win.scaledSize(12)
                                onClicked: if (modelData.url)
                                    win.requestOpen(modelData.url)
                                ToolTip.visible: hovered
                                ToolTip.text: modelData.name || ""
                                background: Rectangle {
                                    radius: 6
                                    color: parent.highlighted ? win.currentFileColor : "transparent"
                                }
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            wrapMode: Text.Wrap
                            visible: backlinkList.count === 0
                            text: (backend.uiLanguage, backend.t("noBacklinks"))
                            color: win.mutedColor
                            font.family: "iA Writer Mono S"
                            font.pixelSize: win.scaledSize(12)
                        }

                        Label {
                            Layout.fillWidth: true
                            text: (backend.uiLanguage, backend.t("outgoingLinks"))
                            color: win.strongTextColor
                            font.family: "iA Writer Mono S"
                            font.pixelSize: win.scaledSize(11)
                            font.bold: true
                        }

                        ListView {
                            id: outgoingList
                            objectName: "outgoingList"
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            clip: true
                            visible: count > 0
                            model: backend.outgoingLinks
                            spacing: 2
                            boundsBehavior: Flickable.StopAtBounds
                            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                            delegate: ItemDelegate {
                                required property var modelData
                                width: ListView.view.width
                                leftPadding: 10
                                rightPadding: 10
                                topPadding: 8
                                bottomPadding: 8
                                text: modelData.resolved
                                      ? (modelData.title || modelData.name)
                                      : ((modelData.target || modelData.name) + "  · missing")
                                font.family: "iA Writer Mono S"
                                font.pixelSize: win.scaledSize(12)
                                onClicked: {
                                    if (modelData.resolved && modelData.url)
                                        win.requestOpen(modelData.url)
                                }
                                ToolTip.visible: hovered
                                ToolTip.text: modelData.name || modelData.target || ""
                                background: Rectangle {
                                    radius: 6
                                    color: parent.highlighted ? win.currentFileColor : "transparent"
                                }
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            wrapMode: Text.Wrap
                            visible: outgoingList.count === 0
                            text: (backend.uiLanguage, backend.t("noOutgoing"))
                            color: win.mutedColor
                            font.family: "iA Writer Mono S"
                            font.pixelSize: win.scaledSize(12)
                        }
                    }
                }
            }
        }

        Pane {
            id: sidebarRevealRail
            visible: !win.showSidebar && !backend.zenMode
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.bottom: statusBar.top
            anchors.margins: 8
            width: Math.max(win.scaledSize(64), showSidebarButton.implicitWidth + 16)
            padding: 0

            background: Rectangle {
                color: win.panelColor
                border.color: win.panelBorderColor
                radius: 10
            }

            Button {
                id: showSidebarButton
                objectName: "showSidebarButton"
                anchors.centerIn: parent
                text: "Show Sidebar"
                onClicked: backend.sidebarVisible = true
            }
        }

        Pane {
            id: previewPane
            visible: win.showPreview
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.bottom: statusBar.top
            anchors.margins: 8
            width: backend.previewSplitWidth
            padding: 0

            background: Rectangle {
                color: win.panelColor
                border.color: win.panelBorderColor
                radius: 10
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 14
                    Layout.rightMargin: 10
                    Layout.topMargin: 8
                    Layout.bottomMargin: 8
                    spacing: 8

                    Label {
                        text: (backend.uiLanguage, backend.t("preview"))
                        color: win.strongTextColor
                        font.family: "iA Writer Mono S"
                        font.pixelSize: win.scaledSize(13)
                        font.bold: true
                    }

                    ComboBox {
                        objectName: "previewThemeBox"
                        model: ["night", "newsprint", "gothic", "Threads", "Slides"]
                        currentIndex: backend.previewMode === "threads"
                                      ? 3
                                      : (backend.previewMode === "slides"
                                         ? 4
                                         : Math.max(0, ["night", "newsprint", "gothic"].indexOf(backend.previewTheme)))
                        implicitWidth: win.scaledSize(120)
                        implicitHeight: win.scaledSize(28)
                        font.family: "iA Writer Mono S"
                        font.pixelSize: win.scaledSize(11)
                        onActivated: function(index) {
                            var choices = ["night", "newsprint", "gothic", "Threads", "Slides"]
                            if (choices[index] === "Threads")
                                backend.previewMode = "threads"
                            else if (choices[index] === "Slides")
                                backend.previewMode = "slides"
                            else {
                                backend.previewMode = "markdown"
                                backend.previewTheme = choices[index]
                            }
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    ToolButton {
                        objectName: "hidePreviewButton"
                        text: (backend.uiLanguage, backend.t("hidePreview"))
                        implicitWidth: win.scaledSize(44)
                        implicitHeight: win.scaledSize(28)
                        padding: 4
                        font.family: "iA Writer Mono S"
                        font.pixelSize: win.scaledSize(11)
                        ToolTip.visible: hovered
                        ToolTip.text: "Hide preview (Ctrl/Cmd+Shift+P)"
                        onClicked: togglePreviewVisibility()
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: 1
                    color: win.panelBorderColor
                }

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    Rectangle {
                        anchors.fill: parent
                        color: backend.previewMode === "threads" ? "#000000" : backend.previewCanvas
                    }

                    ThreadsPreview {
                        anchors.fill: parent
                        visible: backend.previewMode === "threads"
                    }

                    SlidePreview {
                        anchors.fill: parent
                        visible: backend.previewMode === "slides"
                    }

                    Flickable {
                        id: previewFlick
                        anchors.fill: parent
                        visible: backend.previewMode !== "threads" && backend.previewMode !== "slides"
                        clip: true
                        contentWidth: width
                        contentHeight: Math.max(height, previewPaper.y + previewPaper.height + 48)
                        boundsBehavior: Flickable.StopAtBounds
                        onContentHeightChanged: win.schedulePreviewFollow()
                        ScrollBar.vertical: ScrollBar {
                            policy: ScrollBar.AsNeeded
                        }

                        Rectangle {
                            id: previewPaperShadow
                            x: previewPaper.x + 0
                            y: previewPaper.y + 8
                            width: previewPaper.width
                            height: previewPaper.height
                            radius: 16
                            color: backend.previewTheme === "night" ? "#000000" : "#3d3428"
                            opacity: backend.previewTheme === "night" ? 0.35 : 0.12
                        }

                        Rectangle {
                            id: previewPaper
                            readonly property int measure: Math.max(win.scaledSize(280),
                                                                    Math.min(previewFlick.width - 48,
                                                                             win.scaledSize(720)))
                            x: Math.max(24, Math.round((previewFlick.width - measure) / 2))
                            y: 28
                            width: measure
                            height: previewColumn.implicitHeight + 80
                            radius: 14
                            color: backend.previewBackground
                            border.color: backend.previewTheme === "night" ? "#2a2723" : "#efeae2"
                            border.width: 1

                            Column {
                                id: previewColumn
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.top: parent.top
                                anchors.leftMargin: 40
                                anchors.rightMargin: 40
                                anchors.topMargin: 40
                                spacing: 22
                                readonly property string pageTitle: {
                                    var fields = backend.previewProperties
                                    for (var i = 0; i < fields.length; ++i) {
                                        if (fields[i].key === "title")
                                            return fields[i].value
                                    }
                                    return ""
                                }

                                Label {
                                    visible: previewColumn.pageTitle.length > 0
                                    width: parent.width
                                    text: previewColumn.pageTitle
                                    color: backend.previewForeground
                                    wrapMode: Text.Wrap
                                    font.family: "PingFang TC"
                                    font.pixelSize: win.scaledSize(28)
                                    font.weight: Font.DemiBold
                                    lineHeight: 1.25
                                }

                                Flow {
                                    width: parent.width
                                    spacing: 18
                                    visible: backend.previewProperties.length > 0

                                    Repeater {
                                        model: backend.previewProperties
                                        Column {
                                            visible: modelData.key !== "title"
                                            spacing: 2
                                            Label {
                                                text: modelData.key
                                                color: backend.previewMuted
                                                font.family: "PingFang TC"
                                                font.pixelSize: win.scaledSize(11)
                                            }
                                            Label {
                                                text: modelData.value
                                                color: backend.previewForeground
                                                font.family: "PingFang TC"
                                                font.pixelSize: win.scaledSize(14)
                                            }
                                        }
                                    }
                                }

                                Rectangle {
                                    visible: backend.previewProperties.length > 0
                                    width: parent.width
                                    height: 1
                                    color: backend.previewTheme === "night" ? "#2f2c28" : "#ece6dc"
                                }

                                Column {
                                    id: previewDocument
                                    objectName: "renderedPreview"
                                    width: parent.width
                                    spacing: 16

                                    Repeater {
                                        model: backend.previewBlockCount
                                        delegate: Item {
                                            required property int index
                                            property int rev: backend.previewRevision
                                            readonly property var modelData: {
                                                var _ = rev
                                                return backend.previewBlockAt(index)
                                            }
                                            width: previewDocument.width
                                            height: markdownView.visible
                                                    ? markdownView.implicitHeight
                                                    : codeView.implicitHeight

                                            function refreshPreview() {
                                                var block = backend.previewBlockAt(index)
                                                codeView.block = block
                                                if (block && block.kind === "markdown")
                                                    backend.renderPreviewFragment(
                                                        markdownView.textDocument,
                                                        (block && block.text) ? block.text : "")
                                            }

                                            Connections {
                                                target: backend
                                                function onPreviewRevisionChanged() {
                                                    refreshPreview()
                                                    win.schedulePreviewFollow()
                                                }
                                            }
                                            Component.onCompleted: refreshPreview()

                                            TextEdit {
                                                id: markdownView
                                                visible: modelData.kind === "markdown"
                                                width: parent.width
                                                readOnly: true
                                                activeFocusOnPress: false
                                                selectByMouse: true
                                                wrapMode: TextEdit.Wrap
                                                textFormat: TextEdit.RichText
                                                color: backend.previewForeground
                                                selectedTextColor: win.strongTextColor
                                                selectionColor: win.selectionFill
                                                font.family: "PingFang TC"
                                                font.pixelSize: win.scaledSize(17)
                                                renderType: Screen.devicePixelRatio % 1 === 0
                                                    ? TextEdit.NativeRendering
                                                    : TextEdit.QtRendering
                                                onLinkActivated: function(link) {
                                                    var href = backend.normalizedLinkUrl(link)
                                                    if (href.length > 0)
                                                        backend.openExternalUrl(href)
                                                }
                                            }

                                            CodePreviewCard {
                                                id: codeView
                                                visible: modelData.kind !== "markdown"
                                                width: parent.width
                                                block: modelData
                                            }
                                        }
                                    }
                                }
                            }
                        }

                        Label {
                            anchors.centerIn: parent
                            width: parent.width - 32
                            horizontalAlignment: Text.AlignHCenter
                            wrapMode: Text.Wrap
                            text: "Markdown preview updates as you write."
                            color: win.mutedColor
                            font.family: "iA Writer Mono S"
                            font.pixelSize: win.scaledSize(12)
                            visible: editor.text.length === 0
                        }
                    }
                }
            }
        }

        Pane {
            id: previewRevealRail
            visible: !win.showPreview && !backend.zenMode
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.bottom: statusBar.top
            anchors.margins: 8
            width: Math.max(win.scaledSize(64), showPreviewButton.implicitWidth + 16)
            padding: 0

            background: Rectangle {
                color: win.panelColor
                border.color: win.panelBorderColor
                radius: 10
            }

            Button {
                id: showPreviewButton
                objectName: "showPreviewButton"
                anchors.centerIn: parent
                text: "Show Preview"
                onClicked: backend.previewVisible = true
            }
        }

        Item {
            id: sidebarSplitter
            visible: win.showSidebar
            z: 8
            width: 10
            anchors.left: sidebarPane.right
            anchors.top: sidebarPane.top
            anchors.bottom: sidebarPane.bottom

            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                width: sidebarSplitMouse.containsMouse || sidebarSplitMouse.pressed ? 3 : 1
                height: parent.height - 28
                radius: 1
                color: sidebarSplitMouse.pressed ? backend.themeAccent
                      : (sidebarSplitMouse.containsMouse ? win.mutedColor : win.panelBorderColor)
            }

            MouseArea {
                id: sidebarSplitMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.SplitHCursor
                property real originX: 0
                property int originW: 0
                onPressed: function(mouse) {
                    originX = mapToItem(chromeRoot, mouse.x, 0).x
                    originW = backend.sidebarSplitWidth
                }
                onPositionChanged: function(mouse) {
                    if (!pressed)
                        return
                    backend.sidebarSplitWidth = originW
                            + (mapToItem(chromeRoot, mouse.x, 0).x - originX)
                }
            }
        }

        Item {
            id: previewSplitter
            visible: win.showPreview
            z: 8
            width: 10
            anchors.right: previewPane.left
            anchors.top: previewPane.top
            anchors.bottom: previewPane.bottom

            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                width: previewSplitMouse.containsMouse || previewSplitMouse.pressed ? 3 : 1
                height: parent.height - 28
                radius: 1
                color: previewSplitMouse.pressed ? backend.themeAccent
                      : (previewSplitMouse.containsMouse ? win.mutedColor : win.panelBorderColor)
            }

            MouseArea {
                id: previewSplitMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.SplitHCursor
                property real originX: 0
                property int originW: 0
                onPressed: function(mouse) {
                    originX = mapToItem(chromeRoot, mouse.x, 0).x
                    originW = backend.previewSplitWidth
                }
                onPositionChanged: function(mouse) {
                    if (!pressed)
                        return
                    backend.previewSplitWidth = originW
                            + (originX - mapToItem(chromeRoot, mouse.x, 0).x)
                }
            }
        }

        Rectangle {
            id: editorPaneFrame
            anchors.left: backend.zenMode ? parent.left : (win.showSidebar ? sidebarSplitter.right : sidebarRevealRail.right)
            anchors.right: backend.zenMode ? parent.right : (win.showPreview ? previewSplitter.left : previewRevealRail.left)
            anchors.top: parent.top
            anchors.bottom: backend.zenMode ? parent.bottom : statusBar.top
            anchors.leftMargin: 0
            anchors.rightMargin: 0
            anchors.topMargin: 8
            anchors.bottomMargin: 8
            color: win.panelColor
            border.color: win.panelBorderColor
            radius: 10
        }

        ProjectViewBar {
            id: projectViewBar
            parent: editorPaneFrame
            visible: !backend.zenMode
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 10
            anchors.rightMargin: 10
            anchors.topMargin: 6
            z: 3
        }

        ToolButton {
            parent: editorPaneFrame
            visible: backend.zenMode
            objectName: "zenExitButton"
            z: 4
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.margins: 10
            text: (backend.uiLanguage, backend.t("zenExit"))
            font.family: "iA Writer Mono S"
            font.pixelSize: win.scaledSize(11)
            onClicked: backend.zenMode = false
        }

        RowLayout {
            id: tableEditBar
            objectName: "tableEditBar"
            parent: editorPaneFrame
            visible: backend.projectView === "editor" && win.tableEdit.active && !backend.zenMode
            z: 6
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.leftMargin: 14
            anchors.rightMargin: 14
            anchors.bottomMargin: 8
            spacing: 8

            ToolButton {
                objectName: "tableAddColumnButton"
                text: (backend.uiLanguage, backend.t("tableAddColumn"))
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(11)
                focusPolicy: Qt.NoFocus
                onClicked: win.applyTableMutation(backend.insertTableColumn(editor.text, win.tableCursor()))
            }
            ToolButton {
                objectName: "tableAddRowButton"
                text: (backend.uiLanguage, backend.t("tableAddRow"))
                font.family: "iA Writer Mono S"
                font.pixelSize: win.scaledSize(11)
                focusPolicy: Qt.NoFocus
                onClicked: win.applyTableMutation(backend.insertTableRow(editor.text, win.tableCursor()))
            }
            Item { Layout.fillWidth: true }
        }

        PropertiesBar {
            id: propertiesBar
            parent: editorPaneFrame
            visible: backend.projectView === "editor" && backend.editorMode === "live"
                     && !backend.zenMode
            anchors.top: projectViewBar.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 14
            anchors.rightMargin: 14
            anchors.topMargin: 2
            z: 3
        }

        Item {
            id: projectViewsHost
            objectName: "projectViewsHost"
            parent: editorPaneFrame
            anchors.fill: parent
            anchors.topMargin: backend.zenMode ? 10 : projectViewBar.height + 10
            visible: backend.projectView !== "editor"
            z: 2

            ProjectTableView {
                anchors.fill: parent
                visible: backend.projectView === "table"
            }
            ProjectBoardView {
                anchors.fill: parent
                visible: backend.projectView === "board"
            }
            ProjectCalendarView {
                anchors.fill: parent
                visible: backend.projectView === "calendar"
            }
            ProjectCardsView {
                anchors.fill: parent
                visible: backend.projectView === "cards"
            }
            MindmapView {
                id: mindmapView
                anchors.fill: parent
                visible: backend.projectView === "mindmap"
            }
        }

        Flickable {
            id: editorFlick
            anchors.fill: editorPaneFrame
            anchors.topMargin: backend.zenMode ? 6
                : (projectViewBar.height + 6
                   + (propertiesBar.visible ? propertiesBar.height + 4 : 0))
            // Leave the table bar uncovered. This Flickable is a sibling of
            // editorPaneFrame, so without a bottom margin it eats the clicks.
            anchors.bottomMargin: tableEditBar.visible ? tableEditBar.implicitHeight + 10 : 0
            visible: backend.projectView === "editor"
            clip: true
            contentWidth: width
            contentHeight: Math.max(height, editor.y + editor.implicitHeight + 220)
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar {
                policy: ScrollBar.AsNeeded
                // Wheel scrolling moves contentY directly rather than
                // flicking the Flickable, so the bar has to be told about
                // that activity; linger briefly after the last event.
                active: hovered || pressed || wheelScroll.running || scrollLinger.running
            }

            Timer {
                id: scrollLinger
                interval: 600
            }

            // Flickable turns a wheel notch into a flick sized by the small
            // application font, which crawls next to a browser. Reproduce
            // Chromium's wheel physics instead (cc::ScrollOffsetAnimationCurve):
            // each notch moves 3 lines of 40px towards a running target, the
            // animation gets shorter as the outstanding distance grows, and a
            // notch landing mid-animation carries the current velocity into
            // the new curve, so sustained spinning keeps picking up speed.
            readonly property real wheelStep: win.scaledSize(120)

            FrameAnimation {
                id: wheelScroll
                running: false

                property real startY: 0
                property real targetY: 0
                property real duration: 0.2
                // Cubic bezier easing; ease-in-out (0.42, 0, 0.58, 1) for a
                // fresh scroll, with y1 tilted on retarget so the curve's
                // initial slope matches the velocity it inherits.
                property real cx1: 0.42
                property real cy1: 0
                readonly property real cx2: 0.58
                readonly property real cy2: 1

                onTriggered: {
                    var x = elapsedTime / duration;
                    if (x >= 1) {
                        editorFlick.contentY = editorFlick.snapToPixel(targetY);
                        stop();
                        return;
                    }
                    editorFlick.contentY = editorFlick.snapToPixel(
                        startY + (targetY - startY) * curveY(solveCurve(x)));
                }

                function begin(from, to, dur, slope) {
                    startY = from;
                    targetY = to;
                    duration = dur;
                    cx1 = 0.42;
                    cy1 = 0.42 * Math.max(-1000, Math.min(1000, slope));
                    restart();
                }

                function retarget(newTarget) {
                    var s = solveCurve(Math.min(1, elapsedTime / duration));
                    var pos = startY + (targetY - startY) * curveY(s);
                    var delta = newTarget - pos;
                    if (Math.abs(delta) < 0.5) {
                        editorFlick.contentY = newTarget;
                        stop();
                        return;
                    }

                    var velocity = curveDY(s) / Math.max(1e-6, curveDX(s))
                        * (targetY - startY) / duration;
                    var dur = editorFlick.wheelDuration(delta);
                    // When already moving faster than the eased curve would,
                    // bound the duration by the time to target at the current
                    // velocity; the 2.5x covers the ease-out tail.
                    if (velocity !== 0 && delta / velocity > 0)
                        dur = Math.min(dur, delta / velocity * 2.5);
                    begin(pos, newTarget, dur, velocity * dur / delta);
                }

                // Cubic bezier through (0,0), (cx1,cy1), (cx2,cy2), (1,1),
                // evaluated by Newton-solving the curve parameter from x.
                function curveX(s) { return 3 * s * (1 - s) * ((1 - s) * cx1 + s * cx2) + s * s * s; }
                function curveY(s) { return 3 * s * (1 - s) * ((1 - s) * cy1 + s * cy2) + s * s * s; }
                function curveDX(s) { return 3 * (1 - s) * (1 - s) * cx1 + 6 * (1 - s) * s * (cx2 - cx1) + 3 * s * s * (1 - cx2); }
                function curveDY(s) { return 3 * (1 - s) * (1 - s) * cy1 + 6 * (1 - s) * s * (cy2 - cy1) + 3 * s * s * (1 - cy2); }

                function solveCurve(x) {
                    var s = x;
                    for (var i = 0; i < 8; ++i) {
                        var error = curveX(s) - x;
                        if (Math.abs(error) < 0.001)
                            break;
                        var d = curveDX(s);
                        if (Math.abs(d) < 1e-6)
                            break;
                        s = Math.max(0, Math.min(1, s - error / d));
                    }
                    return s;
                }
            }

            WheelHandler {
                // Wayland compositors route every pointer's scroll through
                // one seat device that Qt classifies as a touchpad, so the
                // device type cannot tell a mouse wheel from two-finger
                // scrolling. Distinguish by event shape instead: discrete
                // wheel notches arrive with only angleDelta set, while
                // finger scrolling carries pixel-precise pixelDelta.
                acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
                onWheel: function(wheel) {
                    scrollLinger.restart();
                    if (wheel.pixelDelta.y !== 0)
                        editorFlick.scrollTo(editorFlick.clampContentY(editorFlick.contentY - wheel.pixelDelta.y));
                    else
                        editorFlick.scrollByWheel(wheel);
                    wheel.accepted = true;
                }
            }

            onMovementStarted: wheelScroll.stop()

            function scrollByWheel(wheel) {
                // High-resolution wheels report fractional notches; feed
                // those through the same animated path, like Chromium does
                // for every wheel-source event.
                var notches = wheel.angleDelta.y / 120;
                if (notches === 0)
                    return;

                if (wheelScroll.running) {
                    wheelScroll.retarget(clampContentY(wheelScroll.targetY - notches * wheelStep));
                    return;
                }

                var target = clampContentY(contentY - notches * wheelStep);
                if (target !== contentY)
                    wheelScroll.begin(contentY, target, wheelDuration(target - contentY), 0);
            }

            // Chromium's inverse-delta duration: 200ms for a single notch,
            // ramping down to 100ms once 480px are outstanding.
            function wheelDuration(delta) {
                var pixels = Math.abs(delta) / win.textScale;
                return Math.max(6, Math.min(12, 14 - pixels / 60)) / 60;
            }

            function clampContentY(y) {
                return Math.max(0, Math.min(Math.max(0, contentHeight - height), y));
            }

            // Whole device pixels keep natively hinted glyphs from
            // re-rasterizing mid-animation, which reads as shimmer.
            function snapToPixel(y) {
                return Math.round(y * Screen.devicePixelRatio) / Screen.devicePixelRatio;
            }

            // Jump to a position, abandoning any wheel animation still running.
            function scrollTo(y) {
                wheelScroll.stop();
                contentY = snapToPixel(y);
            }

            // Keep the editing caret within the viewport so writing past the
            // bottom edge scrolls the page along with the text.
            function ensureCursorVisible() {
                var cursorTop = editor.y + editor.cursorRectangle.y;
                var cursorBottom = cursorTop + editor.cursorRectangle.height;
                var maxContentY = Math.max(0, contentHeight - height);
                if (backend.typewriterScroll) {
                    // Short notes must stay at the top. Mid-screen scrolling
                    // on a few paragraphs looks like the page ran away.
                    var textBottom = editor.y + editor.contentHeight;
                    if (textBottom <= height + 8) {
                        scrollTo(0);
                        return;
                    }
                    scrollTo(clampContentY(cursorTop - height * 0.38));
                    return;
                }
                var margin = win.editorFontPixelSize * 2;
                if (cursorBottom + margin > contentY + height)
                    scrollTo(Math.min(maxContentY, cursorBottom + margin - height));
                else if (cursorTop - margin < contentY)
                    scrollTo(Math.max(0, cursorTop - margin));
            }

            TextEdit {
                id: editor
                objectName: "sourceEditor"
                x: Math.round((editorFlick.width - width) / 2)
                y: Math.max(42, Math.round(editorFlick.height * 0.05))
                width: win.documentColumnWidth(editorFlick.width)
                height: Math.max(editorFlick.height - y - 48, implicitHeight + 20)
                text: ""
                textFormat: TextEdit.PlainText
                wrapMode: TextEdit.Wrap
                selectByMouse: true
                persistentSelection: true
                activeFocusOnPress: true
                color: win.textColor
                selectedTextColor: win.strongTextColor
                selectionColor: win.selectionFill
                font.family: "iA Writer Mono S"
                font.pixelSize: win.editorFontPixelSize
                font.weight: Font.Normal
                // Native rendering hints glyphs to the pixel grid, which is
                // crispest at whole scale factors but misplaces and unevenly
                // rasterizes glyphs at fractional ones (and goes stale when
                // the compositor delivers the fractional scale after the
                // first frame). Fall back to Qt's scalable renderer there.
                renderType: Screen.devicePixelRatio % 1 === 0 ? TextEdit.NativeRendering : TextEdit.QtRendering
                cursorDelegate: Rectangle {
                    width: 1
                    color: win.strongTextColor
                }
                onCursorRectangleChanged: editorFlick.ensureCursorVisible()

                MouseArea {
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    propagateComposedEvents: true
                    onPressed: function(mouse) {
                        if (!(mouse.modifiers & (Qt.MetaModifier | Qt.ControlModifier))) {
                            mouse.accepted = false
                            return
                        }
                        var pos = editor.positionAt(mouse.x, mouse.y)
                        mouse.accepted = win.tryFollowLink(editor.text, pos)
                    }
                }

                function replaceSelectionWith(replacement) {
                    var start = Math.min(selectionStart, selectionEnd);
                    var end = Math.max(selectionStart, selectionEnd);
                    EditorMutations.replaceRange(editor, start, end, replacement);
                }

                function wrapSelection(before, after) {
                    forceActiveFocus();
                    var start = Math.min(selectionStart, selectionEnd);
                    var end = Math.max(selectionStart, selectionEnd);
                    var selected = text.slice(start, end);
                    EditorMutations.replaceRange(editor, start, end,
                                                 before + selected + after,
                                                 before.length,
                                                 before.length + selected.length);
                }

                function insertLink() {
                    var start = Math.min(selectionStart, selectionEnd);
                    var end = Math.max(selectionStart, selectionEnd);
                    var selected = text.slice(start, end);
                    var url = backend.clipboardUrl();
                    var label = selected.length > 0 ? selected : "link text";
                    var destination = url.length > 0 ? url : "https://";
                    var escapedLabel = escapeMarkdownLinkText(label);
                    var markdown = "[" + escapedLabel + "](" + escapeMarkdownLinkDestination(destination) + ")";
                    if (selected.length === 0) {
                        EditorMutations.replaceRange(editor, start, end, markdown,
                                                     1, 1 + escapedLabel.length);
                    } else if (url.length === 0) {
                        EditorMutations.replaceRange(editor, start, end, markdown,
                                                     escapedLabel.length + 3,
                                                     markdown.length - 1);
                    } else {
                        EditorMutations.replaceRange(editor, start, end, markdown);
                    }
                }

                function smartReturn(softBreak) {
                    if (softBreak) {
                        replaceSelectionWith("\n");
                        return;
                    }
                    var lineStart = text.lastIndexOf("\n", cursorPosition - 1) + 1;
                    var line = text.slice(lineStart, cursorPosition);
                    var before = text.slice(0, cursorPosition);
                    var fences = (before.match(/^\s*```/gm) || []).length;
                    if ((fences % 2) === 1) {
                        replaceSelectionWith("\n");
                        return;
                    }
                    var match = line.match(/^(\s*)([-+*]|\d+[.)]|>+)\s+(.*)$/);
                    if (match) {
                        if (match[3].length === 0) {
                            EditorMutations.replaceRange(editor, lineStart,
                                                         cursorPosition, "\n");
                        } else {
                            var marker = match[2];
                            if (/^\d/.test(marker))
                                marker = (parseInt(marker) + 1) + marker.slice(-1);
                            replaceSelectionWith("\n" + match[1] + marker + " ");
                        }
                        return;
                    }
                    replaceSelectionWith("\n\n");
                }

                function indentCurrentList(outdent) {
                    var before = text.slice(0, cursorPosition);
                    var fences = (before.match(/^\s*```/gm) || []).length;
                    if ((fences % 2) === 1) {
                        if (!outdent)
                            replaceSelectionWith("  ");
                        return;
                    }
                    var lineStart = text.lastIndexOf("\n", cursorPosition - 1) + 1;
                    var lineEnd = text.indexOf("\n", cursorPosition);
                    if (lineEnd < 0)
                        lineEnd = text.length;
                    var line = text.slice(lineStart, lineEnd);
                    var match = line.match(/^(\s*)([-+*]|\d+[.)])(\s+)/);
                    if (!match) {
                        if (!outdent)
                            replaceSelectionWith("  ");
                        return;
                    }
                    var indent = match[1];
                    if (outdent) {
                        if (indent.length === 0)
                            return;
                        var cut = indent.charAt(0) === "\t" ? 1 : Math.min(2, indent.length);
                        var caret = cursorPosition;
                        EditorMutations.replaceRange(editor, lineStart, lineStart + cut, "");
                        cursorPosition = Math.max(lineStart, caret - cut);
                    } else {
                        var caretIn = cursorPosition;
                        EditorMutations.replaceRange(editor, lineStart, lineStart, "  ");
                        cursorPosition = caretIn + 2;
                    }
                }

                function escapeMarkdownLinkText(linkText) {
                    return linkText.replace(/\\/g, "\\\\")
                                   .replace(/\[/g, "\\[")
                                   .replace(/\]/g, "\\]");
                }

                function escapeMarkdownLinkDestination(linkUrl) {
                    return linkUrl.replace(/\\/g, "\\\\")
                                  .replace(/\(/g, "\\(")
                                  .replace(/\)/g, "\\)");
                }

                function pasteClipboardUrlAsMarkdownLink() {
                    var start = Math.min(selectionStart, selectionEnd);
                    var end = Math.max(selectionStart, selectionEnd);
                    if (start === end)
                        return false;

                    var url = backend.clipboardUrl();
                    if (url === "")
                        return false;

                    var selected = text.slice(start, end);
                    var leading = selected.match(/^\s*/)[0];
                    var trailing = selected.match(/\s*$/)[0];
                    var linkText = selected.slice(leading.length,
                                                  selected.length - trailing.length);
                    if (linkText === "")
                        return false;

                    replaceSelectionWith(leading + "[" + escapeMarkdownLinkText(linkText) + "]("
                                         + escapeMarkdownLinkDestination(url) + ")" + trailing);
                    return true;
                }

                function pasteClipboardImageMarkdown() {
                    if (!backend.clipboardContainsImportableImage())
                        return false;

                    var markdown = backend.clipboardImageMarkdown();
                    if (markdown.length > 0)
                        replaceSelectionWith(markdown);
                    return true;
                }

                function pasteClipboardAsPlainText() {
                    var pastedText = backend.clipboardText();
                    if (pastedText.length === 0)
                        return false;

                    replaceSelectionWith(pastedText);
                    return true;
                }

                function skipHiddenForward(position) {
                    var pos = position;
                    var ranges = backend.hiddenRangesAt(pos);
                    for (var i = 0; i < ranges.length; i++) {
                        if (pos >= ranges[i].start && pos < ranges[i].end) {
                            pos = ranges[i].end;
                            i = -1;
                        }
                    }
                    return pos;
                }

                function skipHiddenBackward(position) {
                    var pos = position;
                    var ranges = backend.hiddenRangesAt(pos);
                    for (var i = ranges.length - 1; i >= 0; i--) {
                        if (pos > ranges[i].start && pos <= ranges[i].end) {
                            pos = ranges[i].start;
                            i = ranges.length;
                        }
                    }
                    return pos;
                }

                function moveCursorVisibly(direction) {
                    if (selectionStart !== selectionEnd) {
                        cursorPosition = direction > 0
                            ? Math.max(selectionStart, selectionEnd)
                            : Math.min(selectionStart, selectionEnd);
                        return;
                    }

                    var pos = Math.max(0, Math.min(text.length, cursorPosition + direction));
                    cursorPosition = direction > 0
                        ? skipHiddenForward(pos)
                        : skipHiddenBackward(pos);
                }

                function movePage(direction, extendSelection) {
                    var pageStep = Math.max(win.editorFontPixelSize,
                                            editorFlick.height - win.editorFontPixelSize * 2);
                    var rect = cursorRectangle;
                    var targetY = rect.y + rect.height / 2 + direction * pageStep;
                    var target = positionAt(rect.x, Math.max(0, targetY));
                    if (extendSelection)
                        moveCursorSelection(target, TextEdit.SelectCharacters);
                    else
                        cursorPosition = target;
                }

                function deleteParagraphBreakBehindCursor() {
                    if (selectionStart !== selectionEnd || cursorPosition < 2)
                        return false;

                    if (text.slice(cursorPosition - 2, cursorPosition) !== "\n\n")
                        return false;

                    var start = cursorPosition - 2;
                    remove(start, cursorPosition);
                    cursorPosition = start;
                    return true;
                }

                Keys.priority: Keys.BeforeItem
                Keys.onPressed: function(event) {
                    var pasteKey = (event.key === Qt.Key_V)
                        && (event.modifiers & Qt.ControlModifier)
                        && !(event.modifiers & (Qt.AltModifier | Qt.MetaModifier | Qt.ShiftModifier));
                    var shiftInsert = (event.key === Qt.Key_Insert)
                        && (event.modifiers & Qt.ShiftModifier)
                        && !(event.modifiers & (Qt.ControlModifier | Qt.AltModifier | Qt.MetaModifier));
                    if (pasteKey || shiftInsert) {
                        if (pasteClipboardImageMarkdown()
                                || pasteClipboardUrlAsMarkdownLink()
                                || pasteClipboardAsPlainText())
                            event.accepted = true;
                        return;
                    }

                    var returnKey = event.key === Qt.Key_Return || event.key === Qt.Key_Enter;
                    var commandModifier = event.modifiers & (Qt.ControlModifier | Qt.AltModifier | Qt.MetaModifier);
                    if (commandModifier && !(event.modifiers & Qt.ShiftModifier)
                            && event.key >= Qt.Key_1 && event.key <= Qt.Key_5) {
                        event.accepted = false;
                        return;
                    }
                    if (slashMenu.visible && !commandModifier) {
                        if (event.key === Qt.Key_Down) {
                            slashMenu.move(1)
                            event.accepted = true
                            return
                        }
                        if (event.key === Qt.Key_Up) {
                            slashMenu.move(-1)
                            event.accepted = true
                            return
                        }
                        if (returnKey) {
                            slashMenu.runCurrent()
                            event.accepted = true
                            return
                        }
                        if (event.key === Qt.Key_Escape) {
                            slashMenu.close()
                            win.slashIgnore = true
                            event.accepted = true
                            return
                        }
                    }
                    if (returnKey && !commandModifier && (event.modifiers & Qt.ShiftModifier)) {
                        var leave = backend.leaveFenceAt(text, cursorPosition)
                        if (leave.active) {
                            if (leave.above) {
                                EditorMutations.replaceRange(editor, leave.insertPos, leave.insertPos, "\n")
                                cursorPosition = leave.insertPos
                            } else {
                                var prefix = ""
                                if (leave.needsClose) {
                                    var before = text.slice(0, leave.insertPos)
                                    if (!before.endsWith("\n"))
                                        prefix += "\n"
                                    prefix += "```"
                                }
                                var inserted = prefix + "\n\n"
                                EditorMutations.replaceRange(editor, leave.insertPos, leave.insertPos, inserted)
                                cursorPosition = leave.insertPos + inserted.length
                            }
                            event.accepted = true
                            return
                        }
                    }
                    if (returnKey && !commandModifier) {
                        smartReturn(event.modifiers & Qt.ShiftModifier);
                        event.accepted = true;
                    } else if (!commandModifier && event.key === Qt.Key_Tab) {
                        var tabCell = backend.tableMoveCell(text, cursorPosition, 1)
                        if (tabCell.active) {
                            if (tabCell.replaced)
                                win.applyTableMutation(tabCell)
                            else
                                cursorPosition = tabCell.caret
                        } else {
                            indentCurrentList(false);
                        }
                        event.accepted = true;
                    } else if (!commandModifier && event.key === Qt.Key_Backtab) {
                        var backCell = backend.tableMoveCell(text, cursorPosition, -1)
                        if (backCell.active)
                            cursorPosition = backCell.caret
                        else
                            indentCurrentList(true);
                        event.accepted = true;
                    } else if (!commandModifier && event.key === Qt.Key_Backspace
                               && deleteParagraphBreakBehindCursor()) {
                        event.accepted = true;
                    } else if (!commandModifier && !(event.modifiers & Qt.ShiftModifier)
                               && event.key === Qt.Key_Right) {
                        moveCursorVisibly(1);
                        event.accepted = true;
                    } else if (!commandModifier && !(event.modifiers & Qt.ShiftModifier)
                               && event.key === Qt.Key_Left) {
                        moveCursorVisibly(-1);
                        event.accepted = true;
                    } else if (!commandModifier
                               && (event.key === Qt.Key_PageDown || event.key === Qt.Key_PageUp)) {
                        movePage(event.key === Qt.Key_PageDown ? 1 : -1,
                                 event.modifiers & Qt.ShiftModifier);
                        event.accepted = true;
                    }
                }

                onTextChanged: {
                    if (win.searchUpdating)
                        return;
                    var contentChanged = backend.editorTextChanged();
                    if (win.searchOpen && contentChanged)
                        win.updateSearch();
                    win.refreshEditorBlockMenus();
                }
                onCursorPositionChanged: {
                    if (backend.editorMode === "live" && backend.frontMatterEnd > 0
                            && cursorPosition < backend.frontMatterEnd) {
                        cursorPosition = Math.min(text.length, backend.frontMatterEnd)
                        return
                    }
                    backend.setEditorCaret(cursorPosition)
                    if (typeof pluginHost !== "undefined" && pluginHost)
                        pluginHost.setEditorSelectionRange(
                                    Math.min(selectionStart, selectionEnd),
                                    Math.max(selectionStart, selectionEnd))
                    win.refreshEditorBlockMenus()
                    win.refreshTableEdit()
                    win.schedulePreviewFollow()
                }

                Text {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    text: "# Start writing"
                    visible: editor.text.length === 0 && !editor.activeFocus
                    color: win.mutedColor
                    font.family: editor.font.family
                    font.pixelSize: editor.font.pixelSize
                    font.weight: editor.font.weight
                }

                Component.onCompleted: {
                    backend.attachDocument(textDocument);
                    forceActiveFocus();
                }
            }
        }

        Rectangle {
            id: statusBar
            visible: !backend.zenMode
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: visible ? win.statusBarHeight : 0
            color: "transparent"

            Rectangle {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                height: 1
                color: win.panelBorderColor
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 12

                FooterIconButton {
                    objectName: "saveButton"
                    iconName: "save"
                    iconColor: win.mutedColor
                    tooltip: "Save"
                    onClicked: backend.save()
                }

                FooterIconButton {
                    objectName: "openButton"
                    iconName: "open"
                    iconColor: win.mutedColor
                    tooltip: "Open"
                    onClicked: backend.openDialog()
                }

                ToolButton {
                    objectName: "newThreadsPostButton"
                    text: (backend.uiLanguage, backend.t("newPost"))
                    font.family: "iA Writer Mono S"
                    font.pixelSize: win.scaledSize(11)
                    ToolTip.visible: hovered
                    ToolTip.text: (backend.uiLanguage, backend.t("newPostTip"))
                    onClicked: win.createThreadsPost()
                }

                ToolButton {
                    objectName: "newGardenNoteButton"
                    text: (backend.uiLanguage, backend.t("newGarden"))
                    font.family: "iA Writer Mono S"
                    font.pixelSize: win.scaledSize(11)
                    ToolTip.visible: hovered
                    ToolTip.text: (backend.uiLanguage, backend.t("newGardenTip"))
                    onClicked: win.createGardenNote()
                }

                ToolButton {
                    objectName: "newSlidevNoteButton"
                    text: (backend.uiLanguage, backend.t("newSlides"))
                    font.family: "iA Writer Mono S"
                    font.pixelSize: win.scaledSize(11)
                    ToolTip.visible: hovered
                    ToolTip.text: (backend.uiLanguage, backend.t("newSlidesTip"))
                    onClicked: win.createSlidevNote()
                }

                ToolButton {
                    objectName: "editThreadsTemplateButton"
                    text: (backend.uiLanguage, backend.t("template"))
                    font.family: "iA Writer Mono S"
                    font.pixelSize: win.scaledSize(11)
                    ToolTip.visible: hovered
                    ToolTip.text: (backend.uiLanguage, backend.t("templateTip"))
                    onClicked: win.openThreadsTemplate()
                }

                ToolButton {
                    objectName: "publishThreadsButton"
                    text: (backend.uiLanguage, backend.t("publishNow"))
                    font.family: "iA Writer Mono S"
                    font.pixelSize: win.scaledSize(11)
                    ToolTip.visible: hovered
                    ToolTip.text: (backend.uiLanguage, backend.t("publishTip"))
                    onClicked: win.requestPublishThreads(false)
                }

                ToolButton {
                    objectName: "publishGardenButton"
                    text: (backend.uiLanguage, backend.t("publishGardenNow"))
                    font.family: "iA Writer Mono S"
                    font.pixelSize: win.scaledSize(11)
                    ToolTip.visible: hovered
                    ToolTip.text: (backend.uiLanguage, backend.t("publishGardenTip"))
                    onClicked: win.requestPublishGarden("note", false)
                }

                ToolButton {
                    objectName: "publishSlidevButton"
                    visible: backend.slidevNote
                    text: (backend.uiLanguage, backend.t("publishSlidevNow"))
                    font.family: "iA Writer Mono S"
                    font.pixelSize: win.scaledSize(11)
                    ToolTip.visible: hovered
                    ToolTip.text: (backend.uiLanguage, backend.t("publishSlidevTip"))
                    onClicked: win.requestPublishSlidev(false)
                }

                ToolButton {
                    objectName: "preferencesButton"
                    text: (backend.uiLanguage, backend.t("preferences"))
                    onClicked: openPreferencesDialog()
                }

                Label {
                    Layout.preferredWidth: Math.min(400, win.width / 3)
                    text: backend.status
                    color: win.mutedColor
                    font.family: "iA Writer Mono S"
                    font.pixelSize: win.scaledSize(11)
                    visible: text !== ""
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                }

                Label {
                    objectName: "pluginStatusLabel"
                    visible: typeof pluginHost !== "undefined" && pluginHost
                             && pluginHost.pluginStatusText.length > 0
                    text: (typeof pluginHost !== "undefined" && pluginHost)
                          ? pluginHost.pluginStatusText : ""
                    color: win.mutedColor
                    font.family: "iA Writer Mono S"
                    font.pixelSize: win.scaledSize(11)
                    elide: Text.ElideRight
                    verticalAlignment: Text.AlignVCenter
                }

                Item {
                    Layout.fillWidth: true
                }

                Label {
                    objectName: "taskProgressLabel"
                    visible: (backend.openTaskCount + backend.closedTaskCount) > 0
                    text: backend.closedTaskCount + "/" + (backend.openTaskCount + backend.closedTaskCount)
                          + " " + (backend.uiLanguage, backend.t("taskProgress"))
                    color: win.mutedColor
                    font.family: "iA Writer Mono S"
                    font.pixelSize: win.scaledSize(11)
                    verticalAlignment: Text.AlignVCenter
                }

                Label {
                    text: backend.wordCount + (backend.wordCount === 1 ? " Word" : " Words")
                    color: win.mutedColor
                    opacity: 0.75
                    font.family: "iA Writer Mono S"
                    font.pixelSize: win.scaledSize(11)
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }


        Pane {
            anchors.top: editorFlick.top
            anchors.left: editorFlick.left
            anchors.right: editorFlick.right
            height: win.scaledSize(win.replaceOpen ? 104 : 56)
            visible: win.searchOpen
            z: 10
            leftPadding: 16
            rightPadding: 8
            topPadding: 0
            bottomPadding: 0
            Material.elevation: 8

            background: Rectangle {
                radius: 9
                color: win.darkMode ? "#22221f" : "#fffef2"
            }

            RowLayout {
                anchors.fill: parent
                spacing: 8

                Item {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    TextInput {
                        id: searchField
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        height: win.replaceOpen ? parent.height / 2 : parent.height
                        verticalAlignment: TextInput.AlignVCenter
                        selectByMouse: true
                        color: win.textColor
                        selectionColor: win.selectionFill
                        selectedTextColor: win.strongTextColor
                        font.pixelSize: win.scaledSize(17)
                        clip: true
                        onTextChanged: win.updateSearch()
                        Keys.onReturnPressed: function(event) {
                            win.moveSearch((event.modifiers & Qt.ShiftModifier) ? -1 : 1);
                            event.accepted = true;
                        }
                        Keys.onEscapePressed: function(event) {
                            win.closeSearch();
                            event.accepted = true;
                        }
                    }

                    TextInput {
                        id: replaceField
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.bottom: parent.bottom
                        height: parent.height / 2
                        visible: win.replaceOpen
                        verticalAlignment: TextInput.AlignVCenter
                        color: win.textColor
                        selectionColor: win.selectionFill
                        selectedTextColor: win.strongTextColor
                        font.pixelSize: win.scaledSize(17)
                        Keys.onReturnPressed: replaceCurrentButton.clicked()
                    }

                    Label {
                        anchors.verticalCenter: replaceField.verticalCenter
                        text: "Replace with"
                        visible: win.replaceOpen && replaceField.text.length === 0
                        color: win.mutedColor
                        font.pixelSize: win.scaledSize(17)
                    }

                    Label {
                        anchors.verticalCenter: searchField.verticalCenter
                        text: "Find"
                        visible: searchField.text.length === 0
                        color: win.mutedColor
                        font.pixelSize: win.scaledSize(17)
                    }
                }

                Label {
                    Layout.preferredWidth: win.scaledSize(58)
                    Layout.fillHeight: true
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    text: win.searchMatches.length === 0
                        ? "0/0"
                        : (win.searchMatchIndex + 1) + "/" + win.searchMatches.length
                    color: win.darkMode ? win.textColor : "#62635f"
                    font.pixelSize: win.scaledSize(16)
                }

                Button {
                    id: replaceCurrentButton
                    visible: win.replaceOpen
                    text: "Replace"
                    onClicked: {
                        if (win.searchMatchIndex < 0) return;
                        var start = win.searchMatches[win.searchMatchIndex];
                        EditorMutations.replaceRange(editor, start,
                                                     start + searchField.text.length,
                                                     replaceField.text);
                        win.updateSearch();
                    }
                }

                Button {
                    visible: win.replaceOpen
                    text: "All"
                    onClicked: {
                        if (searchField.text.length === 0) return;
                        for (var i = win.searchMatches.length - 1; i >= 0; --i) {
                            var start = win.searchMatches[i];
                            EditorMutations.replaceRange(editor, start,
                                                         start + searchField.text.length,
                                                         replaceField.text);
                        }
                        win.updateSearch();
                    }
                }

                Rectangle {
                    Layout.preferredWidth: 1
                    Layout.preferredHeight: 34
                    color: win.darkMode ? "#6f6f62" : "#d5d56e"
                }

                SearchIconButton {
                    iconName: "up"
                    iconColor: win.darkMode ? win.textColor : "#62635f"
                    onClicked: win.moveSearch(-1)
                }

                SearchIconButton {
                    iconName: "down"
                    iconColor: win.darkMode ? win.textColor : "#62635f"
                    onClicked: win.moveSearch(1)
                }

                SearchIconButton {
                    iconName: "close"
                    iconColor: win.darkMode ? win.textColor : "#62635f"
                    onClicked: win.closeSearch()
                }
            }
        }
    }

    Component.onCompleted: {
        backend.refreshInboxFiles();
        var geometry = backend.windowGeometry();
        if (geometry.x >= 0) x = geometry.x;
        if (geometry.y >= 0) y = geometry.y;
        width = geometry.width;
        height = geometry.height;
        if (geometry.maximized) showMaximized();
    }

    Component.onDestruction: backend.saveWindowGeometry(x, y, width, height, visibility === Window.Maximized)

}
