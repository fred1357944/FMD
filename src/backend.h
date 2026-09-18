#pragma once

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QByteArray>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QImage>
#include <QString>
#include <QStringList>
#include <QProcess>
#include <QTimer>
#include <QUrl>
#include <QVariantList>
#include <memory>

class MarkdownHighlighter;
class QTextDocument;
class QWindow;
class QLockFile;

class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(QUrl fileUrl READ fileUrl NOTIFY fileUrlChanged)
    Q_PROPERTY(QString fileName READ fileName NOTIFY fileUrlChanged)
    Q_PROPERTY(QUrl workspaceFolderUrl READ workspaceFolderUrl NOTIFY workspaceFolderChanged)
    Q_PROPERTY(QString workspaceFolderName READ workspaceFolderName NOTIFY workspaceFolderChanged)
    Q_PROPERTY(QString workspaceFolderPath READ workspaceFolderPath NOTIFY workspaceFolderChanged)
    Q_PROPERTY(QVariantList workspaceFiles READ workspaceFiles NOTIFY workspaceFilesChanged)
    Q_PROPERTY(QVariantList workspaceTree READ workspaceTree NOTIFY workspaceTreeChanged)
    Q_PROPERTY(QString fileSort READ fileSort WRITE setFileSort NOTIFY fileSortChanged)
    Q_PROPERTY(QString selectedWorkspacePath READ selectedWorkspacePath WRITE setSelectedWorkspacePath NOTIFY selectedWorkspacePathChanged)
    Q_PROPERTY(QString selectedFolderPath READ selectedFolderPath NOTIFY workspaceNavChanged)
    Q_PROPERTY(bool workspaceFoldersCollapsed READ workspaceFoldersCollapsed NOTIFY workspaceTreeChanged)
    Q_PROPERTY(QVariantList documentOutline READ documentOutline NOTIFY outlineChanged)
    Q_PROPERTY(int outlineMaxLevel READ outlineMaxLevel WRITE setOutlineMaxLevel NOTIFY outlineMaxLevelChanged)
    Q_PROPERTY(QVariantList workspaceTags READ workspaceTags NOTIFY workspaceTagsChanged)
    Q_PROPERTY(QString tagFilter READ tagFilter WRITE setTagFilter NOTIFY tagFilterChanged)
    Q_PROPERTY(QVariantList recentFiles READ recentFiles NOTIFY recentFilesChanged)
    Q_PROPERTY(QVariantList recentWorkspaces READ recentWorkspaces NOTIFY recentWorkspacesChanged)
    Q_PROPERTY(QVariantList backlinks READ backlinks NOTIFY noteLinksChanged)
    Q_PROPERTY(QVariantList outgoingLinks READ outgoingLinks NOTIFY noteLinksChanged)
    Q_PROPERTY(QVariantList templateFiles READ templateFiles NOTIFY templateFilesChanged)
    Q_PROPERTY(bool modified READ modified NOTIFY modifiedChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(int wordCount READ wordCount NOTIFY wordCountChanged)
    Q_PROPERTY(bool darkMode READ darkMode WRITE setDarkMode NOTIFY darkModeChanged)
    Q_PROPERTY(qreal textScale READ textScale WRITE setTextScale NOTIFY textScaleChanged)
    Q_PROPERTY(bool sidebarVisible READ sidebarVisible WRITE setSidebarVisible NOTIFY sidebarVisibleChanged)
    Q_PROPERTY(bool previewVisible READ previewVisible WRITE setPreviewVisible NOTIFY previewVisibleChanged)
    Q_PROPERTY(bool zenMode READ zenMode WRITE setZenMode NOTIFY zenModeChanged)
    Q_PROPERTY(QString editorMode READ editorMode WRITE setEditorMode NOTIFY editorModeChanged)
    Q_PROPERTY(bool typewriterScroll READ typewriterScroll WRITE setTypewriterScroll NOTIFY typewriterScrollChanged)
    Q_PROPERTY(int frontMatterEnd READ frontMatterEnd NOTIFY previewPropertiesChanged)
    Q_PROPERTY(bool propertiesExpanded READ propertiesExpanded WRITE setPropertiesExpanded NOTIFY propertiesExpandedChanged)
    Q_PROPERTY(QString projectKeywordFilter READ projectKeywordFilter WRITE setProjectKeywordFilter NOTIFY projectFiltersChanged)
    Q_PROPERTY(QString projectDateFilter READ projectDateFilter WRITE setProjectDateFilter NOTIFY projectFiltersChanged)
    Q_PROPERTY(QString projectPublishFilter READ projectPublishFilter WRITE setProjectPublishFilter NOTIFY projectFiltersChanged)
    Q_PROPERTY(int openTaskCount READ openTaskCount NOTIFY wordCountChanged)
    Q_PROPERTY(int closedTaskCount READ closedTaskCount NOTIFY wordCountChanged)
    Q_PROPERTY(QUrl inboxFolderUrl READ inboxFolderUrl NOTIFY inboxChanged)
    Q_PROPERTY(QString inboxFolderPath READ inboxFolderPath NOTIFY inboxChanged)
    Q_PROPERTY(QVariantList inboxFiles READ inboxFiles NOTIFY inboxChanged)
    Q_PROPERTY(QString projectView READ projectView WRITE setProjectView NOTIFY projectViewChanged)
    Q_PROPERTY(QVariantList cardRecords READ cardRecords NOTIFY projectRecordsChanged)
    Q_PROPERTY(QStringList cardTagFilters READ cardTagFilters NOTIFY cardFiltersChanged)
    Q_PROPERTY(QVariantList activityHeatmap READ activityHeatmap NOTIFY projectRecordsChanged)
    Q_PROPERTY(QVariantList workspaceNavFolders READ workspaceNavFolders NOTIFY workspaceTreeChanged)
    Q_PROPERTY(QVariantList workspaceNavFiles READ workspaceNavFiles NOTIFY workspaceNavChanged)
    Q_PROPERTY(QVariantList workspaceNavCrumbs READ workspaceNavCrumbs NOTIFY workspaceNavChanged)
    Q_PROPERTY(bool workspaceNavCanGoUp READ workspaceNavCanGoUp NOTIFY workspaceNavChanged)
    Q_PROPERTY(QVariantList workspaceShortcuts READ workspaceShortcuts NOTIFY projectRecordsChanged)
    Q_PROPERTY(QVariantList projectRecords READ projectRecords NOTIFY projectRecordsChanged)
    Q_PROPERTY(QStringList projectFieldNames READ projectFieldNames NOTIFY projectRecordsChanged)
    Q_PROPERTY(QStringList projectTableColumns READ projectTableColumns NOTIFY projectRecordsChanged)
    Q_PROPERTY(QStringList statusChoices READ statusChoices NOTIFY projectRecordsChanged)
    Q_PROPERTY(QString boardField READ boardField WRITE setBoardField NOTIFY projectViewChanged)
    Q_PROPERTY(QString dateField READ dateField WRITE setDateField NOTIFY projectViewChanged)
    Q_PROPERTY(QVariantList boardColumns READ boardColumns NOTIFY projectRecordsChanged)
    Q_PROPERTY(int calendarYear READ calendarYear NOTIFY calendarChanged)
    Q_PROPERTY(int calendarMonth READ calendarMonth NOTIFY calendarChanged)
    Q_PROPERTY(QString calendarTitle READ calendarTitle NOTIFY calendarChanged)
    Q_PROPERTY(QVariantList calendarCells READ calendarCells NOTIFY calendarChanged)
    Q_PROPERTY(QVariantList unscheduledRecords READ unscheduledRecords NOTIFY projectRecordsChanged)
    Q_PROPERTY(QVariantList previewProperties READ previewProperties NOTIFY previewPropertiesChanged)
    Q_PROPERTY(QString previewTheme READ previewTheme WRITE setPreviewTheme NOTIFY previewThemeChanged)
    Q_PROPERTY(QString previewMode READ previewMode WRITE setPreviewMode NOTIFY previewModeChanged)
    Q_PROPERTY(QString previewBackground READ previewBackground NOTIFY previewThemeChanged)
    Q_PROPERTY(QString previewForeground READ previewForeground NOTIFY previewThemeChanged)
    Q_PROPERTY(QString previewMuted READ previewMuted NOTIFY previewThemeChanged)
    Q_PROPERTY(QString previewCanvas READ previewCanvas NOTIFY previewThemeChanged)
    Q_PROPERTY(int sidebarSplitWidth READ sidebarSplitWidth WRITE setSidebarSplitWidth NOTIFY splitSizesChanged)
    Q_PROPERTY(int previewSplitWidth READ previewSplitWidth WRITE setPreviewSplitWidth NOTIFY splitSizesChanged)
    Q_PROPERTY(QString threadsDraftsFolder READ threadsDraftsFolder WRITE setThreadsDraftsFolder NOTIFY threadsDraftsFolderChanged)
    Q_PROPERTY(QString notesSiteFolder READ notesSiteFolder WRITE setNotesSiteFolder NOTIFY notesSiteFolderChanged)
    Q_PROPERTY(QString changelogSiteFolder READ changelogSiteFolder WRITE setChangelogSiteFolder NOTIFY changelogSiteFolderChanged)
    Q_PROPERTY(QString slidesSiteFolder READ slidesSiteFolder WRITE setSlidesSiteFolder NOTIFY slidesSiteFolderChanged)
    Q_PROPERTY(QString defaultSlidesSiteFolder READ defaultSlidesSiteFolder CONSTANT)
    Q_PROPERTY(QString defaultGardenNotesFolder READ defaultGardenNotesFolder CONSTANT)
    Q_PROPERTY(QString defaultNotesSiteFolder READ defaultNotesSiteFolder CONSTANT)
    Q_PROPERTY(QString defaultChangelogSiteFolder READ defaultChangelogSiteFolder CONSTANT)
    Q_PROPERTY(QString resolvedNotesSiteFolder READ resolvedNotesSiteFolder NOTIFY notesSiteFolderChanged)
    Q_PROPERTY(QString resolvedChangelogSiteFolder READ resolvedChangelogSiteFolder NOTIFY changelogSiteFolderChanged)
    Q_PROPERTY(bool slidevNote READ slidevNote NOTIFY previewPropertiesChanged)
    Q_PROPERTY(QVariantList previewSlides READ previewSlides NOTIFY previewSlidesChanged)
    Q_PROPERTY(int previewSlideIndex READ previewSlideIndex WRITE setPreviewSlideIndex NOTIFY previewSlidesChanged)
    Q_PROPERTY(int previewSlideCount READ previewSlideCount NOTIFY previewSlidesChanged)
    Q_PROPERTY(bool sitePushBusy READ sitePushBusy NOTIFY sitePushBusyChanged)
    Q_PROPERTY(bool slidevBusy READ slidevBusy NOTIFY slidevBusyChanged)
    Q_PROPERTY(QString defaultThreadsDraftsFolder READ defaultThreadsDraftsFolder CONSTANT)
    Q_PROPERTY(bool threadsPreview READ threadsPreview NOTIFY threadsPreviewChanged)
    Q_PROPERTY(QVariantList threadPosts READ threadPosts NOTIFY threadsPreviewChanged)
    Q_PROPERTY(bool threadsBusy READ threadsBusy NOTIFY threadsBusyChanged)
    Q_PROPERTY(QString threadsDisplayName READ threadsDisplayName WRITE setThreadsDisplayName NOTIFY threadsProfileChanged)
    Q_PROPERTY(QString threadsHandle READ threadsHandle WRITE setThreadsHandle NOTIFY threadsProfileChanged)
    Q_PROPERTY(QString themeBackground READ themeBackground NOTIFY themeColorsChanged)
    Q_PROPERTY(QString themeForeground READ themeForeground NOTIFY themeColorsChanged)
    Q_PROPERTY(QString themeAccent READ themeAccent NOTIFY themeColorsChanged)
    Q_PROPERTY(QString themeSelection READ themeSelection NOTIFY themeColorsChanged)
    Q_PROPERTY(QVariantList previewBlocks READ previewBlocks NOTIFY previewBlocksChanged)
    Q_PROPERTY(int previewBlockCount READ previewBlockCount NOTIFY previewBlocksChanged)
    Q_PROPERTY(int previewRevision READ previewRevision NOTIFY previewRevisionChanged)
    Q_PROPERTY(QString lastCodeLanguage READ lastCodeLanguage WRITE setLastCodeLanguage NOTIFY lastCodeLanguageChanged)
    Q_PROPERTY(QString uiLanguage READ uiLanguage WRITE setUiLanguage NOTIFY uiLanguageChanged)
    Q_PROPERTY(bool confirmUnsavedChanges READ confirmUnsavedChanges WRITE setConfirmUnsavedChanges NOTIFY confirmUnsavedChangesChanged)
    Q_PROPERTY(QVariantMap hotkeys READ hotkeys NOTIFY hotkeysChanged)
    Q_PROPERTY(QVariantList hotkeyRows READ hotkeyRows NOTIFY hotkeysChanged)
    Q_PROPERTY(QVariantMap mindmapLayout READ mindmapLayout NOTIFY mindmapChanged)
    Q_PROPERTY(QVariantList mindmapOutline READ mindmapOutline NOTIFY mindmapChanged)
    Q_PROPERTY(QString mindmapSelectedId READ mindmapSelectedId WRITE setMindmapSelectedId NOTIFY mindmapChanged)
    Q_PROPERTY(int mindmapStamp READ mindmapStamp NOTIFY mindmapChanged)

public:
    explicit Backend(QObject *parent = nullptr);
    ~Backend() override;

    void setParentWindow(QWindow *window);

    QUrl fileUrl() const { return m_fileUrl; }
    QString fileName() const;
    QUrl workspaceFolderUrl() const { return m_workspaceFolderUrl; }
    QString workspaceFolderName() const;
    QString workspaceFolderPath() const;
    QVariantList workspaceFiles() const { return m_workspaceFiles; }
    QVariantList workspaceTree() const { return m_workspaceTree; }
    QString fileSort() const { return m_fileSort; }
    void setFileSort(const QString &sort);
    QString selectedWorkspacePath() const { return m_selectedWorkspacePath; }
    void setSelectedWorkspacePath(const QString &path);
    QString selectedFolderPath() const;
    bool workspaceFoldersCollapsed() const;
    QVariantList documentOutline() const { return m_documentOutline; }
    int outlineMaxLevel() const { return m_outlineMaxLevel; }
    void setOutlineMaxLevel(int level);
    QVariantList workspaceTags() const { return m_workspaceTags; }
    QString tagFilter() const { return m_tagFilter; }
    void setTagFilter(const QString &tag);
    QVariantList recentFiles() const { return m_recentFiles; }
    QVariantList recentWorkspaces() const { return m_recentWorkspaces; }

    bool modified() const { return m_modified; }
    QString status() const { return m_status; }
    int wordCount() const { return m_wordCount; }
    bool darkMode() const { return m_darkMode; }
    void setDarkMode(bool darkMode);
    qreal textScale() const { return m_textScale; }
    void setTextScale(qreal textScale);
    bool sidebarVisible() const { return m_sidebarVisible; }
    void setSidebarVisible(bool visible);
    bool previewVisible() const { return m_previewVisible; }
    void setPreviewVisible(bool visible);
    bool zenMode() const { return m_zenMode; }
    void setZenMode(bool zen);
    QString editorMode() const { return m_editorMode; }
    void setEditorMode(const QString &mode);
    bool typewriterScroll() const { return m_typewriterScroll; }
    void setTypewriterScroll(bool enabled);
    int frontMatterEnd() const { return m_frontMatterEnd; }
    bool propertiesExpanded() const { return m_propertiesExpanded; }
    void setPropertiesExpanded(bool expanded);
    QString projectKeywordFilter() const { return m_projectKeywordFilter; }
    void setProjectKeywordFilter(const QString &query);
    QString projectDateFilter() const { return m_projectDateFilter; }
    void setProjectDateFilter(const QString &filter);
    QString projectPublishFilter() const { return m_projectPublishFilter; }
    void setProjectPublishFilter(const QString &filter);
    int openTaskCount() const { return m_openTaskCount; }
    int closedTaskCount() const { return m_closedTaskCount; }
    QUrl inboxFolderUrl() const;
    QString inboxFolderPath() const;
    QVariantList inboxFiles() const { return m_inboxFiles; }
    QString projectView() const { return m_projectView; }
    void setProjectView(const QString &view);
    QVariantList cardRecords() const;
    QStringList cardTagFilters() const { return m_cardTagFilters; }
    Q_INVOKABLE void toggleCardTag(const QString &tag);
    Q_INVOKABLE void clearCardTags();
    Q_INVOKABLE QUrl randomCardUrl() const;
    Q_INVOKABLE void togglePin(const QUrl &url);
    QVariantList activityHeatmap() const;
    QVariantList workspaceNavFolders() const;
    QVariantList workspaceNavFiles() const;
    QVariantList workspaceNavCrumbs() const;
    bool workspaceNavCanGoUp() const;
    Q_INVOKABLE void selectParentWorkspaceFolder();
    QVariantList workspaceShortcuts() const;
    QVariantList projectRecords() const { return m_projectRecords; }
    QStringList projectFieldNames() const { return m_projectFieldNames; }
    QStringList projectTableColumns() const;
    QStringList statusChoices() const;
    QString boardField() const;
    void setBoardField(const QString &field);
    QString dateField() const;
    void setDateField(const QString &field);
    QVariantList boardColumns() const;
    int calendarYear() const { return m_calendarYear; }
    int calendarMonth() const { return m_calendarMonth; }
    QString calendarTitle() const;
    QVariantList calendarCells() const;
    QVariantList unscheduledRecords() const;
    QVariantList previewProperties() const { return m_previewProperties; }
    QString previewTheme() const { return m_previewTheme; }
    void setPreviewTheme(const QString &theme);
    QString previewMode() const { return m_previewMode; }
    void setPreviewMode(const QString &mode);
    QString previewBackground() const;
    QString previewForeground() const;
    QString previewMuted() const;
    QString previewCanvas() const;
    int sidebarSplitWidth() const { return m_sidebarSplitWidth; }
    void setSidebarSplitWidth(int width);
    int previewSplitWidth() const { return m_previewSplitWidth; }
    void setPreviewSplitWidth(int width);
    QString threadsDraftsFolder() const { return m_threadsDraftsFolder; }
    void setThreadsDraftsFolder(const QString &path);
    QString notesSiteFolder() const { return m_notesSiteFolder; }
    void setNotesSiteFolder(const QString &path);
    QString changelogSiteFolder() const { return m_changelogSiteFolder; }
    void setChangelogSiteFolder(const QString &path);
    QString slidesSiteFolder() const { return m_slidesSiteFolder; }
    void setSlidesSiteFolder(const QString &path);
    QString defaultSlidesSiteFolder() const;
    QString defaultGardenNotesFolder() const;
    QString defaultNotesSiteFolder() const;
    QString defaultChangelogSiteFolder() const;
    void setGardenNotesFolder(const QString &path);
    QString resolvedGardenNotesFolder() const;
    QString resolvedSlidesSiteFolder() const;
    QString resolvedNotesSiteFolder() const;
    QString resolvedChangelogSiteFolder() const;
    bool slidevNote() const;
    QVariantList previewSlides() const { return m_previewSlides; }
    int previewSlideIndex() const { return m_previewSlideIndex; }
    int previewSlideCount() const { return m_previewSlides.size(); }
    void setPreviewSlideIndex(int index);
    Q_INVOKABLE void stepPreviewSlide(int delta);
    bool sitePushBusy() const { return m_sitePushBusy; }
    bool slidevBusy() const { return m_slidevBusy; }
    QString defaultThreadsDraftsFolder() const;
    Q_INVOKABLE QStringList propertyChoices(const QString &key) const;
    Q_INVOKABLE QStringList knownPropertyKeys() const;
    Q_INVOKABLE QVariantMap publishCurrentNoteToSite();
    Q_INVOKABLE QVariantMap publishWorkspaceToSite();
    Q_INVOKABLE QVariantMap publishChangelogToSite();
    Q_INVOKABLE QVariantMap publishGardenNow(bool dryRun = false);
    Q_INVOKABLE QVariantMap publishFolderGardenNow(bool dryRun = false);
    Q_INVOKABLE QVariantMap publishChangelogNow(bool dryRun = false);
    Q_INVOKABLE QVariantMap validateSlidevDraft() const;
    Q_INVOKABLE QVariantMap publishSlidevNow(bool dryRun = false);
    Q_INVOKABLE void abortSlidevPublish();
    Q_INVOKABLE QString resolvedThreadsDraftsFolder() const;
    bool threadsPreview() const { return m_threadsPreview; }
    QVariantList threadPosts() const { return m_threadPosts; }
    QString threadsDisplayName() const { return m_threadsDisplayName; }
    void setThreadsDisplayName(const QString &name);
    QString threadsHandle() const { return m_threadsHandle; }
    void setThreadsHandle(const QString &handle);
    QString themeBackground() const { return m_themeBackground; }
    QString themeForeground() const { return m_themeForeground; }
    QString themeAccent() const { return m_themeAccent; }
    QString themeSelection() const { return m_themeSelection; }
    QVariantList previewBlocks() const { return m_previewBlocks; }
    int previewBlockCount() const { return m_previewBlocks.size(); }
    int previewRevision() const { return m_previewRevision; }
    Q_INVOKABLE QVariantMap previewBlockAt(int index) const;
    Q_INVOKABLE int previewBlockIndexAt(int cursor) const;
    QString lastCodeLanguage() const { return m_lastCodeLanguage; }
    void setLastCodeLanguage(const QString &language);
    static int countWords(const QString &text);
    static QStringList markdownFilesInDirectory(const QString &directoryPath);
    static QStringList imageFilesInDirectory(const QString &directoryPath);
    static QString normalizedLinkUrl(const QString &clipboardText);
    static QUrl localImageUrlFromClipboardText(const QString &clipboardText);
    static QUrl previewBaseUrl(const QUrl &fileUrl, const QUrl &workspaceFolderUrl = {});
    static QString suggestedFileName(const QString &text);
    static QString filenameStemFromDocument(const QString &text);
    static QString previewMarkdownFrom(const QString &text);

    Q_INVOKABLE void attachDocument(QObject *textDocument);
    Q_INVOKABLE void attachPreviewDocument(QObject *textDocument);
    Q_INVOKABLE void renderPreviewFragment(QObject *textDocument, const QString &markdown);
    Q_INVOKABLE QVariantList codeLanguages(const QString &query) const;
    Q_INVOKABLE QVariantList slashCommands(const QString &query) const;
    Q_INVOKABLE QVariantMap slashQueryAt(const QString &text, int cursor) const;
    Q_INVOKABLE QVariantMap backtickTriggerAt(const QString &text, int cursor) const;
    Q_INVOKABLE QVariantMap leaveFenceAt(const QString &text, int cursor) const;
    Q_INVOKABLE QString codeFenceText(const QString &language, const QString &inner) const;
    Q_INVOKABLE int codeFenceCaretOffset(const QString &language, const QString &inner) const;
    Q_INVOKABLE void copyText(const QString &text);
    Q_INVOKABLE QString editorPlainText() const;
    Q_INVOKABLE void setEditorPlainText(const QString &text);
    Q_INVOKABLE void replaceEditorRange(int start, int end, const QString &text);
    Q_INVOKABLE void setEditorCaret(int position);
    Q_INVOKABLE void refreshLiveFolding();
    Q_INVOKABLE void refreshWordCount();
    Q_INVOKABLE QUrl createInboxNote();
    Q_INVOKABLE void refreshInboxFiles();
    Q_INVOKABLE bool isInboxPath(const QString &path) const;
    Q_INVOKABLE QString currentFilePath() const;
    void setStatus(const QString &status);
    Q_INVOKABLE void openDialog();
    Q_INVOKABLE void openFolderDialog();
    Q_INVOKABLE void open(const QUrl &url);
    Q_INVOKABLE void openFolder(const QUrl &url);
    Q_INVOKABLE void restoreLastSession();
    Q_INVOKABLE void save();
    Q_INVOKABLE void saveForClose();
    Q_INVOKABLE void saveAsDialog();
    Q_INVOKABLE void saveAs(const QUrl &url);
    Q_INVOKABLE void fileDialogCanceled();
    Q_INVOKABLE void discardRecovery();
    Q_INVOKABLE void reloadFromDisk();
    Q_INVOKABLE void keepExternalVersion();
    Q_INVOKABLE void printDocument();
    Q_INVOKABLE void newWindow();
    Q_INVOKABLE QString clipboardUrl() const;
    Q_INVOKABLE QString clipboardText() const;
    Q_INVOKABLE bool clipboardContainsImportableImage() const;
    Q_INVOKABLE QString clipboardImageMarkdown();
    Q_INVOKABLE QString importImageFile(const QUrl &sourceUrl);
    Q_INVOKABLE bool editorTextChanged();
    Q_INVOKABLE QVariantList hiddenRangesAt(int position) const;
    Q_INVOKABLE void setSearchHighlight(const QString &query, int currentMatchStart);
    Q_INVOKABLE void openExternalUrl(const QUrl &url);
    Q_INVOKABLE QVariantMap followableLinkAt(const QString &text, int cursor) const;
    Q_INVOKABLE QUrl preferredDialogFolderUrl() const;
    Q_INVOKABLE QVariantMap windowGeometry() const;
    Q_INVOKABLE void saveWindowGeometry(int x, int y, int width, int height, bool maximized);
    Q_INVOKABLE void setRecordField(const QUrl &url, const QString &key, const QString &value);
    Q_INVOKABLE void setCurrentFrontMatterField(const QString &key, const QString &value);
    Q_INVOKABLE void removeCurrentFrontMatterField(const QString &key);
    Q_INVOKABLE void stepCalendar(int monthDelta);
    Q_INVOKABLE void showCalendarMonth(int year, int month);
    Q_INVOKABLE void showCalendarToday();
    Q_INVOKABLE void revealCalendarDate(const QString &iso);
    Q_INVOKABLE QStringList calendarMonthNames() const;
    Q_INVOKABLE QVariantMap tableAt(const QString &text, int cursor) const;
    Q_INVOKABLE QVariantMap insertTableColumn(const QString &text, int cursor) const;
    Q_INVOKABLE QVariantMap insertTableRow(const QString &text, int cursor) const;
    Q_INVOKABLE QVariantMap tableMoveCell(const QString &text, int cursor, int delta) const;
    Q_INVOKABLE QUrl createBoardNote(const QString &status);
    Q_INVOKABLE void addStatusChoice(const QString &status);
    Q_INVOKABLE void renameStatus(const QString &from, const QString &to);
    Q_INVOKABLE void removeStatusChoice(const QString &status);
    Q_INVOKABLE void renameTag(const QString &from, const QString &to);
    Q_INVOKABLE void insertTag(const QString &tag);
    Q_INVOKABLE void copyOutlineHeadings();
    Q_INVOKABLE QUrl createThreadsDraft();
    QString uiLanguage() const { return m_uiLanguage; }
    void setUiLanguage(const QString &language);
    Q_INVOKABLE QString t(const QString &key) const;
    bool confirmUnsavedChanges() const { return m_confirmUnsavedChanges; }
    void setConfirmUnsavedChanges(bool confirm);
    Q_INVOKABLE bool isBlankDocument() const;
    Q_INVOKABLE bool isUntitledDocument() const;
    Q_INVOKABLE bool shouldPromptForUnsaved() const;
    Q_INVOKABLE void discardBlankUntitled();
    QVariantMap hotkeys() const;
    QVariantList hotkeyRows() const;
    Q_INVOKABLE QString hotkey(const QString &id) const;
    Q_INVOKABLE void setHotkey(const QString &id, const QString &sequence);
    Q_INVOKABLE void clearHotkey(const QString &id);
    Q_INVOKABLE void resetHotkey(const QString &id);
    Q_INVOKABLE QString captureHotkey(int key, int modifiers) const;
    Q_INVOKABLE QString hotkeyDisplay(const QString &sequence) const;
    QVariantMap mindmapLayout() const { return m_mindmapLayout; }
    QVariantList mindmapOutline() const { return m_mindmapOutline; }
    QString mindmapSelectedId() const { return m_mindmapSelectedId; }
    int mindmapStamp() const { return m_mindmapStamp; }
    void setMindmapSelectedId(const QString &id);
    Q_INVOKABLE void mindmapRename(const QString &id, const QString &title);
    Q_INVOKABLE void mindmapAddChild(const QString &id);
    Q_INVOKABLE void mindmapAddSibling(const QString &id);
    Q_INVOKABLE void mindmapRemove(const QString &id);
    Q_INVOKABLE void mindmapReparent(const QString &id, const QString &parentId, int index = -1);
    Q_INVOKABLE void mindmapIndent(const QString &id);
    Q_INVOKABLE void mindmapOutdent(const QString &id);
    Q_INVOKABLE void mindmapToggleCollapse(const QString &id);
    Q_INVOKABLE void mindmapSetCollapsed(const QString &id, bool collapsed);
    Q_INVOKABLE void mindmapSetColor(const QString &id, const QString &color);
    Q_INVOKABLE QVariantList mindmapPalette() const;
    Q_INVOKABLE void mindmapAttachUrl(const QString &id, const QUrl &url);
    Q_INVOKABLE QVariantList linkSuggestions(const QString &query, bool imagesPreferred = false) const;
    Q_INVOKABLE QVariantMap linkQueryAt(const QString &text, int cursor) const;
    Q_INVOKABLE QUrl resolveNodeTarget(const QString &target) const;
    Q_INVOKABLE QString markupForDroppedUrl(const QUrl &url);
    Q_INVOKABLE int fragmentPosition(const QString &kind, const QString &fragment) const;
    Q_INVOKABLE QUrl createMarkdownNote();
    Q_INVOKABLE QUrl createFolder(const QString &name = QString());
    Q_INVOKABLE void toggleWorkspaceFolder(const QString &path);
    Q_INVOKABLE void collapseAllWorkspaceFolders();
    Q_INVOKABLE void expandAllWorkspaceFolders();
    Q_INVOKABLE void toggleAllWorkspaceFolders();
    Q_INVOKABLE bool renameFolder(const QUrl &url, const QString &newName);
    Q_INVOKABLE QUrl createNoteFromTemplate(const QUrl &templateUrl = QUrl());
    Q_INVOKABLE QString renderTemplate(const QUrl &templateUrl) const;
    static QString renderTemplateText(const QString &source, const QString &title,
                                      const QString &filename);
    Q_INVOKABLE QUrl ensureDefaultTemplate();
    QVariantList backlinks() const { return m_backlinks; }
    QVariantList outgoingLinks() const { return m_outgoingLinks; }
    QVariantList templateFiles() const { return m_templateFiles; }
    Q_INVOKABLE QUrl ensureThreadsTemplate();
    Q_INVOKABLE QUrl createGardenNote();
    Q_INVOKABLE QUrl createSlidevNote();
    Q_INVOKABLE QUrl ensureGardenTemplate();
    Q_INVOKABLE QUrl ensureSlidevTemplate();
    Q_INVOKABLE bool renameNote(const QUrl &url, const QString &newName);
    Q_INVOKABLE bool moveNoteToTrash(const QUrl &url);
    Q_INVOKABLE void revealInFinder(const QUrl &url);
    Q_INVOKABLE void openLocalFile(const QUrl &url);
    Q_INVOKABLE int expandAncestorsOfCurrentFile();
    Q_INVOKABLE int workspaceIndexOfCurrentFile() const;
    Q_INVOKABLE QVariantMap validateThreadsDraft();
    Q_INVOKABLE QVariantMap publishThreadsNow(bool dryRun);
    Q_INVOKABLE void abortThreadsPublish();
    bool threadsBusy() const { return m_threadsBusy; }
    QString routeThreadsPostAfterSave(const QString &savedPath, const QString &contents);
    QString threadsPublishedFolder() const;
    QString archivePublishedThreadsPost(const QString &savedPath);
    int archiveAlreadyPublishedThreadsDrafts();

signals:
    void fileUrlChanged();
    void workspaceFolderChanged();
    void workspaceFilesChanged();
    void workspaceTreeChanged();
    void fileSortChanged();
    void selectedWorkspacePathChanged();
    void outlineChanged();
    void outlineMaxLevelChanged();
    void workspaceTagsChanged();
    void tagFilterChanged();
    void recentFilesChanged();
    void recentWorkspacesChanged();
    void noteLinksChanged();
    void templateFilesChanged();
    void modifiedChanged();
    void statusChanged();
    void wordCountChanged();
    void darkModeChanged();
    void textScaleChanged();
    void sidebarVisibleChanged();
    void previewVisibleChanged();
    void zenModeChanged();
    void editorModeChanged();
    void typewriterScrollChanged();
    void propertiesExpandedChanged();
    void projectFiltersChanged();
    void inboxChanged();
    void projectViewChanged();
    void cardFiltersChanged();
    void workspaceNavChanged();
    void projectRecordsChanged();
    void calendarChanged();
    void previewPropertiesChanged();
    void previewThemeChanged();
    void previewModeChanged();
    void previewSlidesChanged();
    void sitePushBusyChanged();
    void splitSizesChanged();
    void threadsDraftsFolderChanged();
    void notesSiteFolderChanged();
    void changelogSiteFolderChanged();
    void slidesSiteFolderChanged();
    void threadsPreviewChanged();
    void threadsBusyChanged();
    void slidevBusyChanged();
    void slidevPublishFinished(const QVariantMap &report);
    void threadsPublishFinished(const QVariantMap &report);
    void threadsProfileChanged();
    void themeColorsChanged();
    void previewBlocksChanged();
    void previewRevisionChanged();
    void lastCodeLanguageChanged();
    void uiLanguageChanged();
    void confirmUnsavedChangesChanged();
    void hotkeysChanged();
    void mindmapChanged();
    void fileOpened(const QString &path);
    void fileChanged(const QString &path);
    void fileSaved(const QString &path);
    void closeAfterSave();
    void openDialogRequested();
    void openFolderDialogRequested();
    void saveDialogRequested(const QUrl &suggestedUrl);
    void saveSucceeded();
    void externalChangeDetected(bool deleted, bool locallyModified);

private:
    void loadDocumentText(const QString &text);
    void setFileUrl(const QUrl &url);
    void setModified(bool modified);
    void saveTo(const QUrl &url);
    QUrl suggestedSaveUrl() const;
    QString currentDocumentText() const;
    void updatePreviewDocument();
    void schedulePreviewUpdate();
    void stylePreviewDocument(QTextDocument *document);
    void setWordCount(int words);
    void scheduleWordCount();
    void applyDocumentTypography();
    void reapplyTypographyToChange();
    void scheduleRecovery();
    void writeRecovery();
    void restoreRecovery();
    void clearRecovery();
    QString recoveryPath() const;
    void watchCurrentFile();
    void watchWorkspaceDirectories();
    void setWorkspaceFolderUrl(const QUrl &url);
    void refreshWorkspaceFiles();
    void refreshNoteLinks();
    void refreshOutgoingLinks();
    void refreshTemplateFiles();
    QVariantList buildOutgoingLinks() const;
    QVariantList buildBacklinks() const;
    QStringList templateDirectoryPaths() const;
    QUrl resolveWikiTarget(const QString &target, const QString &fromFile) const;
    void refreshProjectRecords();
    void refreshDocumentOutline();
    void refreshMindmap();
    void enrichMindmapLayout();
    void commitMindmap(const QString &selectedId);
    void commitMindmapText(const QString &text, const QString &selectedId);
    void refreshThreadPreview(const QString &source);
    void applyPreviewModeForDocument();
    void finishThreadsJob(const QVariantMap &result);
    QVariantMap runThreadsSchedule(const QStringList &arguments, int timeoutMs);
    void finishSlidevJob(const QVariantMap &result);
    QVariantMap emptySlidevResult(const QString &message) const;
    QVariantMap parseSlidevPresentOutput(const QString &text, int exitCode) const;
    QString findSlidevPresentBinary() const;
    QString findSitePushBinary() const;
    QVariantMap runSitePush(const QString &repo, const QStringList &paths,
                            const QString &message, bool dryRun);
    void refreshPreviewSlides(const QString &source);
    QVariantMap publishThenPush(const QVariantMap &copied, const QString &repo,
                                const QStringList &paths, const QString &message,
                                bool dryRun);
    bool ensureSlidesProject(const QString &folder, QString *error) const;
    QVariantMap writeSlidevDeck(const QString &markdown, const QString &sourcePath,
                                const QString &folder) const;
    void recordRecentFile(const QUrl &url);
    void recordRecentWorkspace(const QString &directoryPath);
    void applyNoteText(const QUrl &url, const QString &updated, bool refreshLists);
    QStringList tagsFromText(const QString &text) const;
    QString noteTextFor(const QUrl &url) const;
    QString resolvedBoardField() const;
    QString resolvedDateField() const;
    bool currentWorkspaceContains(const QUrl &url) const;
    void syncWorkspaceForFile(const QUrl &url);
    void rememberWorkspaceDirectory(const QString &directoryPath);
    void loadStatusChoicesForWorkspace();
    void saveStatusChoicesForWorkspace();
    QString preferredDialogDirectoryPath() const;
    QString expandUserPath(const QString &path) const;
    QString threadsTemplateText(const QString &folderPath) const;
    QString uniqueThreadsDraftPath(const QString &folderPath) const;
    QString unusedThreadsDraftPath(const QString &folderPath) const;
    QString gardenTemplateText() const;
    QString slidevTemplateText() const;
    QString uniquePrefixedDraftPath(const QString &folder, const QString &prefix) const;
    QString unusedMatchingDraftPath(const QString &folder, const QString &glob,
                                    const QString &templateText) const;
    QUrl createPrefixedDraft(const QString &folder, const QString &prefix,
                             const QString &templateFileName, const QString &fallbackText,
                             const QVariantMap &fields);
    QUrl ensureNamedTemplate(const QString &folder, const QString &fileName,
                             const QString &fallbackText, const QString &openedMessage);
    bool isPathUnderDirectory(const QString &filePath, const QString &directoryPath) const;
    void moveMarkdownAssets(const QFileInfo &from, const QFileInfo &to) const;
    void maybeAutoRenameUntitled();
    void scheduleUntitledRename();
    QString documentAssetsDirectoryPath() const;
    QString markdownForDocumentImage(const QString &imagePath) const;
    QString importClipboardBitmap(const QImage &image);
    void loadOmarchyTheme();
    void watchOmarchyTheme();
    QString createTargetDirectory() const;
    void rebuildWorkspaceTree();
    void persistCollapsedFolders();
    void applyLiveFolding();
    void scheduleLiveFolding();
    bool recordMatchesDateFilter(const QString &isoDate) const;
    bool recordMatchesPublishFilter(const QVariantMap &fields, const QString &path) const;
    void watchInboxFolder();

    QUrl m_fileUrl;
    QUrl m_workspaceFolderUrl;
    bool m_modified = false;
    QString m_status;
    int m_wordCount = 0;
    bool m_darkMode = true;
    qreal m_textScale = 1.0;
    bool m_sidebarVisible = true;
    bool m_previewVisible = true;
    bool m_zenMode = false;
    QString m_editorMode = QStringLiteral("live");
    bool m_typewriterScroll = true;
    int m_frontMatterEnd = 0;
    bool m_propertiesExpanded = true;
    bool m_applyingLiveFolding = false;
    bool m_liveFoldingQueued = false;
    QString m_projectKeywordFilter;
    QString m_projectDateFilter = QStringLiteral("all");
    QString m_projectPublishFilter = QStringLiteral("all");
    int m_openTaskCount = 0;
    int m_closedTaskCount = 0;
    int m_editorCaret = 0;
    QVariantList m_inboxFiles;
    QString m_projectView = QStringLiteral("editor");
    QStringList m_cardTagFilters;
    QString m_boardField;
    QString m_dateField;
    QVariantList m_projectRecords;
    QStringList m_projectFieldNames;
    QStringList m_statusChoices;
    int m_calendarYear = 0;
    int m_calendarMonth = 0;
    QVariantList m_previewProperties;
    QString m_previewTheme = QStringLiteral("night");
    QString m_previewMode = QStringLiteral("markdown");
    QVariantList m_previewSlides;
    int m_previewSlideIndex = 0;
    bool m_sitePushBusy = false;
    int m_sidebarSplitWidth = 280;
    int m_previewSplitWidth = 420;
    QString m_threadsDraftsFolder;
    QString m_notesSiteFolder;
    QString m_changelogSiteFolder;
    QString m_slidesSiteFolder;
    QString m_gardenNotesFolder;
    bool m_threadsPreview = false;
    QVariantList m_threadPosts;
    QString m_threadsDisplayName;
    QString m_threadsHandle;
    bool m_loading = false;
    bool m_closeAfterSave = false;
    bool m_formattingTypography = false;
    int m_formattedBlockCount = 0;
    int m_lastChangePos = 0;
    int m_lastChangeAdded = 0;
    QTimer m_wordCountTimer;
    QTimer m_previewTimer;
    QTimer m_recoveryTimer;
    QTimer m_untitledRenameTimer;
    QProcess m_threadsProcess;
    QTimer m_threadsTimeout;
    bool m_threadsBusy = false;
    bool m_threadsDryRun = false;
    bool m_threadsAborting = false;
    QProcess m_slidevProcess;
    QTimer m_slidevTimeout;
    bool m_slidevBusy = false;
    bool m_slidevDryRun = false;
    bool m_slidevAborting = false;
    bool m_slidevPresentInPlace = false;
    bool m_cachedSlidevNote = false;
    QFileSystemWatcher m_fileWatcher;
    QFileSystemWatcher m_inboxWatcher;
    QFileSystemWatcher m_workspaceWatcher;
    QTimer m_workspaceRefreshTimer;
    bool m_refreshingInbox = false;
    QPointer<QTextDocument> m_document;
    QPointer<QTextDocument> m_previewDocument;
    QPointer<QWindow> m_parentWindow;
    QPointer<MarkdownHighlighter> m_highlighter;
    QString m_lastDocumentText;
    QByteArray m_lastKnownFileContents;
    bool m_hasKnownFileContents = false;
    QString m_recoveryPath;
    std::unique_ptr<QLockFile> m_recoveryLock;
    QVariantList m_workspaceFiles;
    QVariantList m_workspaceTree;
    QString m_fileSort = QStringLiteral("name-asc");
    QString m_selectedWorkspacePath;
    QStringList m_collapsedFolders;
    QStringList m_workspaceFolderPaths;
    QVariantList m_documentOutline;
    int m_outlineMaxLevel = 6;
    QVariantList m_workspaceTags;
    QString m_tagFilter;
    QVariantList m_recentFiles;
    QVariantList m_recentWorkspaces;
    QVariantList m_backlinks;
    QVariantList m_outgoingLinks;
    QVariantList m_templateFiles;

    QString m_themeBackground;
    QString m_themeForeground;
    QString m_themeAccent;
    QString m_themeSelection;
    QFileSystemWatcher m_themeWatcher;
    QVariantList m_previewBlocks;
    int m_previewRevision = 0;
    QString m_lastCodeLanguage;
    QString m_uiLanguage = QStringLiteral("zh-TW");
    bool m_confirmUnsavedChanges = true;
    QHash<QString, QString> m_hotkeyOverrides;
    QVariantMap m_mindmapLayout;
    QVariantList m_mindmapOutline;
    QString m_mindmapSelectedId;
    int m_mindmapStamp = 0;
};
