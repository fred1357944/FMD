#include "backend.h"

#include <QClipboard>
#include <QColor>
#include <QCoreApplication>
#include <QDate>
#include <QDateTime>
#include <QVector>
#include <QLocale>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QTextCharFormat>
#include <QDesktopServices>
#include <QGuiApplication>
#include <QImage>
#include <QKeySequence>
#include <QMimeData>
#include <QProcess>
#include <QPrintDialog>
#include <QPrinter>
#include <QQuickTextDocument>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QSettings>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLockFile>
#include <QMap>
#include <QSaveFile>
#include <QSet>
#include <QTextBlock>
#include <QTextBlockFormat>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextStream>
#include <QTextFormat>
#include <QTextTable>
#include <QUrl>
#include <QVariantMap>
#include <QWindow>

#include <algorithm>

#include "codeblocks.h"
#include "frontmatter.h"
#include "markdownhighlighter.h"
#include "mindmap.h"
#include "uilocale.h"

constexpr qreal typoraLineHeightPercent = 140;
const QString lastSaveDirectorySetting = QStringLiteral("file/lastSaveDirectory");
const QString lastWorkspaceDirectorySetting = QStringLiteral("workspace/lastDirectory");
const QString sidebarVisibleSetting = QStringLiteral("view/sidebarVisible");
const QString previewVisibleSetting = QStringLiteral("view/previewVisible");
const QString projectViewSetting = QStringLiteral("project/view");
const QString boardFieldSetting = QStringLiteral("project/boardField");
const QString dateFieldSetting = QStringLiteral("project/dateField");
const QString statusChoicesSetting = QStringLiteral("project/statusChoices");
const QString statusChoicesFolderSetting = QStringLiteral("project/statusChoicesFolder");

QString statusChoicesKeyForFolder(const QString &folderPath) {
    const QString absolute = QDir::fromNativeSeparators(QDir(folderPath).absolutePath());
    if (absolute.isEmpty())
        return statusChoicesFolderSetting + QStringLiteral("/_none");
    return statusChoicesFolderSetting + QLatin1Char('/') + absolute;
}
const QString previewThemeSetting = QStringLiteral("preview/theme");
const QString sidebarSplitSetting = QStringLiteral("view/sidebarSplitWidth");
const QString previewSplitSetting = QStringLiteral("view/previewSplitWidth");
const QString threadsDraftsFolderSetting = QStringLiteral("threads/draftsFolder");
const QString notesSiteFolderSetting = QStringLiteral("site/notesFolder");
const QString changelogSiteFolderSetting = QStringLiteral("site/changelogFolder");
const QString slidesSiteFolderSetting = QStringLiteral("site/slidesFolder");
const QString threadsDisplayNameSetting = QStringLiteral("threads/displayName");
const QString threadsHandleSetting = QStringLiteral("threads/handle");
const QString recentFilesSetting = QStringLiteral("recent/files");
const QString recentWorkspacesSetting = QStringLiteral("workspace/recent");
const QString lastFileSetting = QStringLiteral("file/lastFile");
const QString fileSortSetting = QStringLiteral("workspace/fileSort");
const QString collapsedFoldersSetting = QStringLiteral("workspace/collapsedFolders");
const QString zenModeSetting = QStringLiteral("view/zenMode");
const QString editorModeSetting = QStringLiteral("editor/mode");
const QString typewriterScrollSetting = QStringLiteral("editor/typewriter");
const QString propertiesExpandedSetting = QStringLiteral("view/propertiesExpanded");
const QString inboxFolderSetting = QStringLiteral("inbox/folder");
constexpr int recentWorkspaceLimit = 12;

QVariantMap workspaceEntry(const QString &directoryPath)
{
    const QFileInfo info(directoryPath);
    const QString path = QDir(directoryPath).absolutePath();
    QString name = info.fileName();
    if (name.isEmpty())
        name = path;
    return {
        {QStringLiteral("name"), name},
        {QStringLiteral("path"), path},
        {QStringLiteral("url"), QUrl::fromLocalFile(path)},
    };
}
const QString lastCodeLanguageSetting = QStringLiteral("editor/lastCodeLanguage");
const QString uiLanguageSetting = QStringLiteral("ui/language");
const QString confirmUnsavedSetting = QStringLiteral("ui/confirmUnsavedChanges");
const QString hotkeysGroup = QStringLiteral("hotkeys");

struct HotkeySpec {
    const char *id;
    const char *title;
    const char *def;
};

const HotkeySpec kHotkeys[] = {
    {"save", "Save", "Ctrl+S"},
    {"save-as", "Save As", "Ctrl+Shift+S"},
    {"open", "Open file", "Ctrl+O"},
    {"open-folder", "Open folder", "Ctrl+Shift+O"},
    {"new-window", "New window", "Ctrl+N"},
    {"new-note", "New Markdown file", "Ctrl+Shift+N"},
    {"new-post", "New Threads post", "Ctrl+Shift+T"},
    {"publish-now", "Publish Threads now", "Ctrl+Shift+Return"},
    {"command-palette", "Command palette", "Ctrl+P"},
    {"preferences", "Preferences", "Ctrl+,"},
    {"find", "Find", "Ctrl+F"},
    {"find-replace", "Find and replace", "Alt+Ctrl+F"},
    {"find-next", "Find next", "Ctrl+G"},
    {"bold", "Bold", "Ctrl+B"},
    {"italic", "Italic", "Ctrl+I"},
    {"link", "Link", "Ctrl+K"},
    {"undo", "Undo", "Ctrl+Z"},
    {"redo", "Redo", "Ctrl+Shift+Z"},
    {"toggle-sidebar", "Toggle sidebar", "Ctrl+Shift+L"},
    {"toggle-preview", "Toggle preview", "Ctrl+Shift+P"},
    {"toggle-zen", "Zen mode", "Ctrl+Shift+U"},
    {"toggle-editor-mode", "Live / Source", "Ctrl+Shift+E"},
    {"reveal-file", "Reveal current file in sidebar", "Ctrl+Alt+E"},
    {"new-inbox-note", "New inbox note", "Ctrl+Shift+I"},
    {"delete-note", "Move note to Trash", "Ctrl+Backspace"},
    {"next-heading", "Next heading", "Ctrl+Shift+]"},
    {"prev-heading", "Previous heading", "Ctrl+Shift+["},
    {"view-editor", "Editor view", "Ctrl+1"},
    {"view-table", "Table view", "Ctrl+2"},
    {"view-board", "Board view", "Ctrl+3"},
    {"view-calendar", "Calendar view", "Ctrl+4"},
    {"view-mindmap", "Mindmap view", "Ctrl+5"},
    {"view-cards", "Cards view", "Ctrl+6"},
    {"fullscreen", "Fullscreen", "F11"},
};

QString defaultHotkey(const QString &id)
{
    if (id == QLatin1String("find-replace")) {
#ifdef Q_OS_MACOS
        return QStringLiteral("Ctrl+Alt+F");
#else
        return QStringLiteral("Ctrl+H");
#endif
    }
    for (const HotkeySpec &spec : kHotkeys) {
        if (id == QLatin1String(spec.id))
            return QString::fromLatin1(spec.def);
    }
    return {};
}
const QString outlineMaxLevelSetting = QStringLiteral("view/outlineMaxLevel");

namespace {

const QRegularExpression lineBreakRe(QStringLiteral("[\\r\\n]+"));

bool isMarkdownFileName(const QString &fileName) {
    return fileName.endsWith(QStringLiteral(".md"), Qt::CaseInsensitive)
        || fileName.endsWith(QStringLiteral(".markdown"), Qt::CaseInsensitive);
}

bool shouldSkipWorkspacePath(const QString &rootPath, const QString &filePath,
                             bool skipAssetFolders = true) {
    const QString relative = QDir::fromNativeSeparators(QDir(rootPath).relativeFilePath(filePath));
    const QStringList parts = relative.split(QLatin1Char('/'));
    for (const QString &part : parts) {
        if (part.startsWith(QLatin1Char('.')))
            return true;
        if (skipAssetFolders && part.endsWith(QStringLiteral(".assets"), Qt::CaseInsensitive))
            return true;
        if (part.compare(QStringLiteral("node_modules"), Qt::CaseInsensitive) == 0)
            return true;
        if (part.compare(QStringLiteral("__pycache__"), Qt::CaseInsensitive) == 0)
            return true;
    }
    return false;
}

QString fileIdentity(const QString &path)
{
    if (path.isEmpty())
        return {};
    const QFileInfo info(path);
    const QString canonical = info.canonicalFilePath();
    return QDir::fromNativeSeparators(canonical.isEmpty() ? info.absoluteFilePath()
                                                          : canonical);
}

QString templaterToQtDateFormat(QString format)
{
    format.replace(QStringLiteral("YYYY"), QStringLiteral("yyyy"));
    format.replace(QStringLiteral("DD"), QStringLiteral("dd"));
    return format;
}

QString wikiInsertTarget(const QString &path, const QHash<QString, int> &stems,
                         const QString &root)
{
    const QFileInfo info(path);
    QString insertTarget = info.completeBaseName();
    if (stems.value(insertTarget.toLower()) > 1 && !root.isEmpty()) {
        QString relative = QDir::fromNativeSeparators(QDir(root).relativeFilePath(path));
        if (relative.endsWith(QStringLiteral(".markdown"), Qt::CaseInsensitive))
            relative.chop(9);
        else if (relative.endsWith(QStringLiteral(".md"), Qt::CaseInsensitive))
            relative.chop(3);
        insertTarget = relative;
    }
    return insertTarget;
}

bool shouldSkipMarkdownFileName(const QString &fileName) {
    if (fileName.compare(QStringLiteral("README.md"), Qt::CaseInsensitive) == 0)
        return true;
    if (fileName.compare(QStringLiteral("SCHEMA.md"), Qt::CaseInsensitive) == 0)
        return true;
    if (fileName.startsWith(QStringLiteral("_TEMPLATE-"), Qt::CaseInsensitive)
        || fileName.startsWith(QStringLiteral("_template-"), Qt::CaseInsensitive))
        return true;
    return false;
}

QString sanitizeNoteStem(const QString &raw) {
    QString name = raw.trimmed();
    if (name.endsWith(QStringLiteral(".markdown"), Qt::CaseInsensitive))
        name.chop(9);
    else if (name.endsWith(QStringLiteral(".md"), Qt::CaseInsensitive))
        name.chop(3);
    name.replace(QRegularExpression(QStringLiteral("[\\\\/:*?\"<>|\\x00-\\x1f\\x7f]")),
                 QStringLiteral("-"));
    name.replace(QRegularExpression(QStringLiteral("\\s+")), QStringLiteral(" "));
    name = name.left(80).trimmed();
    while (name.endsWith(QLatin1Char('.')))
        name.chop(1);
    while (name.startsWith(QLatin1Char('.')))
        name.remove(0, 1);
    name = name.trimmed();
    if (name.isEmpty() || name == QLatin1String(".") || name == QLatin1String(".."))
        return {};
    return name;
}

bool isUntitledStem(const QString &stem) {
    static const QRegularExpression untitledStemRe(
        QStringLiteral("^Untitled(?:-\\d+)?$"), QRegularExpression::CaseInsensitiveOption);
    return untitledStemRe.match(stem).hasMatch();
}

QString uniqueNotePath(const QString &directory, const QString &stem, const QString &extension,
                       const QString &ignorePath) {
    const QDir dir(directory);
    QString fileName = stem + extension;
    int suffix = 2;
    const QString ignoreAbsolute = ignorePath.isEmpty()
        ? QString()
        : QFileInfo(ignorePath).absoluteFilePath();
    while (true) {
        const QString candidate = QFileInfo(dir.filePath(fileName)).absoluteFilePath();
        if (!QFileInfo::exists(candidate) || candidate == ignoreAbsolute)
            return candidate;
        fileName = stem + QLatin1Char('-') + QString::number(suffix++) + extension;
    }
}

QUrl directoryUrl(const QString &directoryPath) {
    if (directoryPath.isEmpty())
        return {};

    const QString absolutePath = QDir(directoryPath).absolutePath() + QDir::separator();
    return QUrl::fromLocalFile(absolutePath);
}

QString firstClipboardItem(const QString &clipboardText) {
    QString candidate = clipboardText.trimmed();
    const int lineBreak = candidate.indexOf(lineBreakRe);
    if (lineBreak >= 0)
        candidate = candidate.left(lineBreak).trimmed();
    return candidate;
}

bool isSupportedImageFilePath(const QString &path) {
    static const QStringList extensions = {
        QStringLiteral("png"),
        QStringLiteral("jpg"),
        QStringLiteral("jpeg"),
        QStringLiteral("gif"),
        QStringLiteral("webp"),
        QStringLiteral("svg"),
    };
    return extensions.contains(QFileInfo(path).suffix().toLower());
}

QString escapeMarkdownLinkText(QString text) {
    text.replace(lineBreakRe, QStringLiteral(" "));
    text = text.simplified();
    return text.replace(QStringLiteral("\\"), QStringLiteral("\\\\"))
        .replace(QStringLiteral("["), QStringLiteral("\\["))
        .replace(QStringLiteral("]"), QStringLiteral("\\]"));
}

QString escapeMarkdownLinkDestination(QString url) {
    return url.replace(QStringLiteral("\\"), QStringLiteral("\\\\"))
        .replace(QStringLiteral("("), QStringLiteral("\\("))
        .replace(QStringLiteral(")"), QStringLiteral("\\)"));
}

QUrl localFileUrlFromClipboardText(const QString &clipboardText) {
    QString candidate = firstClipboardItem(clipboardText);
    if (candidate.isEmpty())
        return {};

    const QUrl url(candidate);
    if (url.isValid() && url.scheme().compare(QStringLiteral("file"), Qt::CaseInsensitive) == 0
            && url.isLocalFile()) {
        const QString localPath = QDir::cleanPath(url.toLocalFile());
        if (!localPath.isEmpty())
            return QUrl::fromLocalFile(localPath);
        return {};
    }

    if (url.isValid() && !url.scheme().isEmpty())
        return {};

    if (candidate == QStringLiteral("~")) {
        candidate = QDir::homePath();
    } else if (candidate.startsWith(QStringLiteral("~/"))
               || candidate.startsWith(QStringLiteral("~\\"))) {
        candidate = QDir::home().filePath(candidate.mid(2));
    }

    const QString nativePath = QDir::fromNativeSeparators(candidate);
    if (!QDir::isAbsolutePath(nativePath))
        return {};

    return QUrl::fromLocalFile(QDir::cleanPath(nativePath));
}

QString sanitizeAssetBaseName(QString name) {
    name = name.trimmed();
    name.replace(QRegularExpression(QStringLiteral("[\\\\/:*?\"<>|]")), QStringLiteral("-"));
    name.replace(lineBreakRe, QStringLiteral(" "));
    name = name.simplified();
    if (name.isEmpty())
        name = QStringLiteral("image");
    return name;
}

QString uniqueAssetFilePath(const QString &directory, const QString &baseName,
                            const QString &extension) {
    QString candidate = directory + QLatin1Char('/') + baseName + QLatin1Char('.') + extension;
    int suffix = 1;
    while (QFileInfo::exists(candidate)) {
        candidate = directory + QLatin1Char('/') + baseName + QLatin1Char('-')
            + QString::number(++suffix) + QLatin1Char('.') + extension;
    }
    return candidate;
}

QString encodeRelativeMarkdownPath(const QString &relativePath) {
    const QStringList parts = relativePath.split(QLatin1Char('/'));
    QStringList encoded;
    encoded.reserve(parts.size());
    for (const QString &part : parts) {
        if (part == QLatin1String(".") || part == QLatin1String("..") || part.isEmpty())
            encoded.append(part);
        else
            encoded.append(QString::fromUtf8(QUrl::toPercentEncoding(part)));
    }
    return encoded.join(QLatin1Char('/'));
}

bool isImageUrl(const QUrl &url) {
    return url.isLocalFile() && isSupportedImageFilePath(url.toLocalFile());
}

QString findThreadsScheduleBinary() {
    const QString found = QStandardPaths::findExecutable(QStringLiteral("threads-schedule"));
    if (!found.isEmpty())
        return found;
    const QString homeBin = QDir::homePath() + QStringLiteral("/bin/threads-schedule");
    if (QFileInfo::exists(homeBin))
        return homeBin;
    return {};
}

QVariantMap emptyThreadsScheduleResult(const QString &message) {
    QVariantMap result;
    result.insert(QStringLiteral("ok"), false);
    result.insert(QStringLiteral("output"), message);
    result.insert(QStringLiteral("errors"), QStringList{message});
    result.insert(QStringLiteral("warnings"), QStringList{});
    result.insert(QStringLiteral("mainChars"), 0);
    result.insert(QStringLiteral("replies"), 0);
    result.insert(QStringLiteral("permalink"), QString());
    result.insert(QStringLiteral("queueId"), QString());
    result.insert(QStringLiteral("published"), false);
    result.insert(QStringLiteral("dryRun"), false);
    return result;
}

QVariantMap parseThreadsScheduleOutput(const QString &text, int exitCode) {
    QVariantMap result;
    result.insert(QStringLiteral("ok"), exitCode == 0);
    result.insert(QStringLiteral("output"), text);
    result.insert(QStringLiteral("mainChars"), 0);
    result.insert(QStringLiteral("replies"), 0);
    result.insert(QStringLiteral("permalink"), QString());
    result.insert(QStringLiteral("queueId"), QString());
    result.insert(QStringLiteral("published"), false);
    result.insert(QStringLiteral("dryRun"), false);
    QStringList errors;
    QStringList warnings;
    static const QRegularExpression statsRe(
        QStringLiteral("status=(\\S+)\\s+main=(\\d+)\\s+replies=(\\d+)"));
    static const QRegularExpression permalinkRe(QStringLiteral("^\\s*permalink=(.+)\\s*$"));
    static const QRegularExpression queueRe(QStringLiteral("^\\s*queue_id=(\\S+)\\s*$"));
    for (QString line : text.split(QLatin1Char('\n'))) {
        const QString trimmed = line.trimmed();
        if (trimmed.startsWith(QStringLiteral("error:")))
            errors.append(trimmed.mid(6).trimmed());
        else if (trimmed.startsWith(QStringLiteral("warn:")))
            warnings.append(trimmed.mid(5).trimmed());
        const QRegularExpressionMatch stats = statsRe.match(line);
        if (stats.hasMatch()) {
            result.insert(QStringLiteral("mainChars"), stats.captured(2).toInt());
            result.insert(QStringLiteral("replies"), stats.captured(3).toInt());
        }
        const QRegularExpressionMatch permalink = permalinkRe.match(line);
        if (permalink.hasMatch())
            result.insert(QStringLiteral("permalink"), permalink.captured(1).trimmed());
        const QRegularExpressionMatch queue = queueRe.match(line);
        if (queue.hasMatch())
            result.insert(QStringLiteral("queueId"), queue.captured(1).trimmed());
        if (trimmed.contains(QStringLiteral("published=True")))
            result.insert(QStringLiteral("published"), true);
        if (trimmed.startsWith(QStringLiteral("DRY-RUN")))
            result.insert(QStringLiteral("dryRun"), true);
    }
    result.insert(QStringLiteral("errors"), errors);
    result.insert(QStringLiteral("warnings"), warnings);
    return result;
}

} // namespace

QString Backend::normalizedLinkUrl(const QString &clipboardText) {
    QString candidate = firstClipboardItem(clipboardText);
    if (candidate.isEmpty())
        return {};

    if (candidate.startsWith(QStringLiteral("www."), Qt::CaseInsensitive))
        candidate.prepend(QStringLiteral("https://"));

    static const QRegularExpression schemeRe(
        QStringLiteral("^[A-Za-z][A-Za-z0-9+.-]*:"));
    if (!schemeRe.match(candidate).hasMatch())
        return {};

    const QUrl url(candidate);
    if (!url.isValid() || url.scheme().isEmpty())
        return {};

    const QString scheme = url.scheme().toLower();
    const bool webUrl = scheme == QStringLiteral("http")
        || scheme == QStringLiteral("https")
        || scheme == QStringLiteral("ftp");
    if (webUrl && url.host().isEmpty())
        return {};

    if (!webUrl && scheme != QStringLiteral("mailto"))
        return {};

    return url.toString();
}

Backend::Backend(QObject *parent) : QObject(parent) {
    const QString stateDirectory = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(stateDirectory);
    // Claim an orphaned snapshot before taking an empty slot. This ensures a
    // crash in window 2 is still recovered even if window 1 exited normally.
    for (int pass = 0; pass < 2 && !m_recoveryLock; ++pass) {
        for (int slot = 0; slot < 100; ++slot) {
            const QString base = QDir(stateDirectory).filePath(
                QStringLiteral("recovery-%1").arg(slot));
            const bool snapshotExists = QFileInfo::exists(base + QStringLiteral(".json"));
            if ((pass == 0) != snapshotExists)
                continue;
            auto lock = std::make_unique<QLockFile>(base + QStringLiteral(".lock"));
            if (lock->tryLock()) {
                m_recoveryPath = base + QStringLiteral(".json");
                m_recoveryLock = std::move(lock);
                break;
            }
        }
    }
    m_wordCountTimer.setSingleShot(true);
    m_wordCountTimer.setInterval(120);
    connect(&m_wordCountTimer, &QTimer::timeout, this, [this]() {
        refreshWordCount();
        refreshDocumentOutline();
        refreshOutgoingLinks();
        emit fileChanged(currentFilePath());
    });
    m_previewTimer.setSingleShot(true);
    m_previewTimer.setInterval(100);
    connect(&m_previewTimer, &QTimer::timeout, this, &Backend::updatePreviewDocument);
    m_recoveryTimer.setSingleShot(true);
    m_recoveryTimer.setInterval(750);
    connect(&m_recoveryTimer, &QTimer::timeout, this, &Backend::writeRecovery);
    m_untitledRenameTimer.setSingleShot(true);
    m_untitledRenameTimer.setInterval(450);
    connect(&m_untitledRenameTimer, &QTimer::timeout, this, &Backend::maybeAutoRenameUntitled);
    m_threadsTimeout.setSingleShot(true);
    m_threadsTimeout.setInterval(120000);
    connect(&m_threadsTimeout, &QTimer::timeout, this, [this]() {
        if (!m_threadsBusy)
            return;
        m_threadsAborting = true;
        if (m_threadsProcess.state() != QProcess::NotRunning) {
            m_threadsProcess.kill();
            m_threadsProcess.waitForFinished(2000);
        }
        finishThreadsJob(emptyThreadsScheduleResult(
            QStringLiteral("threads-schedule timed out. Check network or ~/bin/threads-schedule.")));
        m_threadsAborting = false;
    });
    connect(&m_threadsProcess, &QProcess::finished, this,
            [this](int exitCode, QProcess::ExitStatus) {
                if (m_threadsAborting || !m_threadsBusy)
                    return;
                m_threadsTimeout.stop();
                const QString output = QString::fromUtf8(m_threadsProcess.readAllStandardOutput())
                    + QString::fromUtf8(m_threadsProcess.readAllStandardError());
                finishThreadsJob(parseThreadsScheduleOutput(output, exitCode));
            });
    connect(&m_threadsProcess, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (m_threadsAborting || !m_threadsBusy || error == QProcess::Timedout)
            return;
        if (m_threadsProcess.state() != QProcess::NotRunning)
            return;
        m_threadsTimeout.stop();
        finishThreadsJob(emptyThreadsScheduleResult(
            QStringLiteral("Could not run threads-schedule.")));
    });
    m_slidevTimeout.setSingleShot(true);
    m_slidevTimeout.setInterval(120000);
    connect(&m_slidevTimeout, &QTimer::timeout, this, [this]() {
        if (!m_slidevBusy)
            return;
        m_slidevAborting = true;
        if (m_slidevProcess.state() != QProcess::NotRunning) {
            m_slidevProcess.kill();
            m_slidevProcess.waitForFinished(2000);
        }
        finishSlidevJob(emptySlidevResult(
            QStringLiteral("slidev-present timed out. Check Node/npx or ~/bin/slidev-present.")));
        m_slidevAborting = false;
    });
    connect(&m_slidevProcess, &QProcess::finished, this,
            [this](int exitCode, QProcess::ExitStatus) {
                if (m_slidevAborting || !m_slidevBusy)
                    return;
                m_slidevTimeout.stop();
                const QString output = QString::fromUtf8(m_slidevProcess.readAllStandardOutput())
                    + QString::fromUtf8(m_slidevProcess.readAllStandardError());
                finishSlidevJob(parseSlidevPresentOutput(output, exitCode));
            });
    connect(&m_slidevProcess, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (m_slidevAborting || !m_slidevBusy || error == QProcess::Timedout)
            return;
        if (m_slidevProcess.state() != QProcess::NotRunning)
            return;
        m_slidevTimeout.stop();
        finishSlidevJob(emptySlidevResult(QStringLiteral("Could not run slidev-present.")));
    });
    connect(&m_fileWatcher, &QFileSystemWatcher::fileChanged, this,
            [this](const QString &path) {
                if (path != m_fileUrl.toLocalFile())
                    return;

                const bool deleted = !QFileInfo::exists(path);
                if (!deleted && m_hasKnownFileContents) {
                    QFile file(path);
                    if (file.open(QIODevice::ReadOnly)
                            && file.readAll() == m_lastKnownFileContents) {
                        // Atomic saves can replace the watched inode. Re-arm the
                        // watcher, but do not report our own save as an outside edit.
                        watchCurrentFile();
                        return;
                    }
                }

                emit externalChangeDetected(deleted, m_modified);
            });
    connect(&m_inboxWatcher, &QFileSystemWatcher::directoryChanged, this,
            [this]() { refreshInboxFiles(); });
    connect(&m_inboxWatcher, &QFileSystemWatcher::fileChanged, this,
            [this]() { refreshInboxFiles(); });
    m_workspaceRefreshTimer.setSingleShot(true);
    m_workspaceRefreshTimer.setInterval(180);
    connect(&m_workspaceRefreshTimer, &QTimer::timeout, this,
            [this]() { refreshWorkspaceFiles(); });
    connect(&m_workspaceWatcher, &QFileSystemWatcher::directoryChanged, this,
            [this]() { m_workspaceRefreshTimer.start(); });

            QSettings settings;
            m_sidebarVisible = settings.value(sidebarVisibleSetting, true).toBool();
            m_previewVisible = settings.value(previewVisibleSetting, true).toBool();
            m_zenMode = settings.value(zenModeSetting, false).toBool();
            const QString savedEditorMode =
                settings.value(editorModeSetting, QStringLiteral("live")).toString();
            m_editorMode = savedEditorMode == QLatin1String("source")
                ? QStringLiteral("source")
                : QStringLiteral("live");
            m_typewriterScroll = settings.value(typewriterScrollSetting, true).toBool();
            m_propertiesExpanded = settings.value(propertiesExpandedSetting, true).toBool();
            m_sidebarSplitWidth = settings.value(sidebarSplitSetting, 280).toInt();
            m_previewSplitWidth = settings.value(previewSplitSetting, 420).toInt();
            if (m_sidebarSplitWidth < 180)
                m_sidebarSplitWidth = 180;
            if (m_previewSplitWidth < 240)
                m_previewSplitWidth = 240;
            const QString savedView = settings.value(projectViewSetting, QStringLiteral("editor")).toString();
            if (savedView == QLatin1String("table") || savedView == QLatin1String("board")
                || savedView == QLatin1String("calendar") || savedView == QLatin1String("editor")
                || savedView == QLatin1String("cards") || savedView == QLatin1String("mindmap"))
                m_projectView = savedView;
            m_boardField = settings.value(boardFieldSetting).toString();
            m_dateField = settings.value(dateFieldSetting).toString();
            // Board lanes come from the current folder's YAML, not leftover
            // global defaults from earlier projects.
            settings.remove(statusChoicesSetting);
            m_statusChoices.clear();
            m_outlineMaxLevel = qBound(1, settings.value(outlineMaxLevelSetting, 6).toInt(), 6);
            const QString savedPreview = settings.value(previewThemeSetting, QStringLiteral("night")).toString();
            if (savedPreview == QLatin1String("newsprint") || savedPreview == QLatin1String("gothic")
                || savedPreview == QLatin1String("night"))
                m_previewTheme = savedPreview;
            m_threadsDraftsFolder = settings.value(threadsDraftsFolderSetting).toString();
            m_notesSiteFolder = settings.value(notesSiteFolderSetting).toString();
            if (m_notesSiteFolder.isEmpty() && QDir(defaultNotesSiteFolder()).exists()) {
                m_notesSiteFolder = defaultNotesSiteFolder();
                settings.setValue(notesSiteFolderSetting, m_notesSiteFolder);
            }
            m_changelogSiteFolder = settings.value(changelogSiteFolderSetting).toString();
            if (m_changelogSiteFolder.isEmpty() && QDir(defaultChangelogSiteFolder()).exists()) {
                m_changelogSiteFolder = defaultChangelogSiteFolder();
                settings.setValue(changelogSiteFolderSetting, m_changelogSiteFolder);
            }
            m_slidesSiteFolder = settings.value(slidesSiteFolderSetting).toString();
            m_threadsDisplayName = settings.value(threadsDisplayNameSetting).toString();
            m_threadsHandle = settings.value(threadsHandleSetting).toString();
            m_lastCodeLanguage = settings.value(lastCodeLanguageSetting,
                                                QStringLiteral("python")).toString();
            m_uiLanguage = UiLocale::normalize(settings.value(uiLanguageSetting,
                                                              QStringLiteral("zh-TW")).toString());
            m_confirmUnsavedChanges = settings.value(confirmUnsavedSetting, true).toBool();
            settings.beginGroup(hotkeysGroup);
            const QStringList keys = settings.childKeys();
            for (const QString &key : keys)
                m_hotkeyOverrides.insert(key, settings.value(key).toString());
            settings.endGroup();
            const QStringList recentPaths = settings.value(recentFilesSetting).toStringList();
            for (const QString &path : recentPaths) {
                if (!QFileInfo::exists(path))
                    continue;
                const QFileInfo info(path);
                m_recentFiles.append(QVariantMap{
                    {QStringLiteral("name"), info.fileName()},
                    {QStringLiteral("url"), QUrl::fromLocalFile(info.absoluteFilePath())}});
            }
            const QStringList recentWorkspacePaths =
                settings.value(recentWorkspacesSetting).toStringList();
            for (const QString &path : recentWorkspacePaths) {
                if (!QDir(path).exists())
                    continue;
                m_recentWorkspaces.append(workspaceEntry(path));
            }
            const QString savedSort = settings.value(fileSortSetting, QStringLiteral("name-asc")).toString();
            if (savedSort == QLatin1String("name-desc")
                || savedSort == QLatin1String("mtime-desc")
                || savedSort == QLatin1String("mtime-asc")
                || savedSort == QLatin1String("name-asc"))
                m_fileSort = savedSort;
            m_collapsedFolders = settings.value(collapsedFoldersSetting).toStringList();
            const QDate today = QDate::currentDate();
            m_calendarYear = today.year();
            m_calendarMonth = today.month();

            loadOmarchyTheme();
            watchOmarchyTheme();
            connect(&m_themeWatcher, &QFileSystemWatcher::fileChanged, this, [this]() {
                loadOmarchyTheme();
                watchOmarchyTheme();
    });
    connect(&m_themeWatcher, &QFileSystemWatcher::directoryChanged, this, [this]() {
        loadOmarchyTheme();
        watchOmarchyTheme();
    });
}

Backend::~Backend() {
    if (m_threadsBusy)
        abortThreadsPublish();
}

void Backend::setParentWindow(QWindow *window) {
    m_parentWindow = window;
}

QString Backend::fileName() const {
    if (!m_fileUrl.isValid() || m_fileUrl.isEmpty())
        return QStringLiteral("Untitled.md");

    if (m_fileUrl.isLocalFile()) {
        const QFileInfo info(m_fileUrl.toLocalFile());
        if (!info.fileName().isEmpty())
            return info.fileName();
    }

    const QString name = m_fileUrl.fileName();
    return name.isEmpty() ? QStringLiteral("Untitled.md") : name;
}

QString Backend::workspaceFolderName() const {
    if (!m_workspaceFolderUrl.isLocalFile())
        return {};

    const QFileInfo info(m_workspaceFolderUrl.toLocalFile());
    return info.fileName().isEmpty() ? info.absoluteFilePath() : info.fileName();
}

QString Backend::workspaceFolderPath() const {
    if (!m_workspaceFolderUrl.isLocalFile())
        return {};

    return QDir(m_workspaceFolderUrl.toLocalFile()).absolutePath();
}

QStringList Backend::markdownFilesInDirectory(const QString &directoryPath) {
    if (directoryPath.isEmpty())
        return {};

    const QDir directory(directoryPath);
    if (!directory.exists())
        return {};

    const QString rootPath = directory.absolutePath();
    QStringList files;
    QDirIterator iterator(rootPath, QDir::Files | QDir::Readable | QDir::NoDotAndDotDot,
                          QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        iterator.next();
        const QFileInfo entry = iterator.fileInfo();
        if (!isMarkdownFileName(entry.fileName()))
            continue;
        if (shouldSkipMarkdownFileName(entry.fileName()))
            continue;
        if (shouldSkipWorkspacePath(rootPath, entry.absoluteFilePath()))
            continue;
        files.append(entry.absoluteFilePath());
    }

    std::sort(files.begin(), files.end(), [&rootPath](const QString &left, const QString &right) {
        const QString leftName = QDir(rootPath).relativeFilePath(left);
        const QString rightName = QDir(rootPath).relativeFilePath(right);
        const int folded = QString::compare(leftName, rightName, Qt::CaseInsensitive);
        return folded == 0 ? leftName < rightName : folded < 0;
    });
    return files;
}

QStringList Backend::imageFilesInDirectory(const QString &directoryPath) {
    if (directoryPath.isEmpty())
        return {};

    const QDir directory(directoryPath);
    if (!directory.exists())
        return {};

    const QString rootPath = directory.absolutePath();
    QStringList files;
    QDirIterator iterator(rootPath, QDir::Files | QDir::Readable | QDir::NoDotAndDotDot,
                          QDirIterator::Subdirectories);
    while (iterator.hasNext()) {
        iterator.next();
        const QFileInfo entry = iterator.fileInfo();
        if (!isSupportedImageFilePath(entry.fileName()))
            continue;
        if (shouldSkipWorkspacePath(rootPath, entry.absoluteFilePath(), false))
            continue;
        files.append(entry.absoluteFilePath());
    }

    std::sort(files.begin(), files.end(), [&rootPath](const QString &left, const QString &right) {
        const QString leftName = QDir(rootPath).relativeFilePath(left);
        const QString rightName = QDir(rootPath).relativeFilePath(right);
        const int folded = QString::compare(leftName, rightName, Qt::CaseInsensitive);
        return folded == 0 ? leftName < rightName : folded < 0;
    });
    return files;
}

QUrl Backend::previewBaseUrl(const QUrl &fileUrl, const QUrl &workspaceFolderUrl) {
    if (fileUrl.isLocalFile())
        return directoryUrl(QFileInfo(fileUrl.toLocalFile()).absolutePath());
    if (workspaceFolderUrl.isLocalFile())
        return directoryUrl(workspaceFolderUrl.toLocalFile());
    return {};
}

void Backend::setDarkMode(bool darkMode) {
    if (m_darkMode == darkMode)
        return;

    m_darkMode = darkMode;
    loadOmarchyTheme();
    emit darkModeChanged();
}

void Backend::setTextScale(qreal textScale) {
    if (qFuzzyCompare(m_textScale, textScale))
        return;

    m_textScale = textScale;
    emit textScaleChanged();
    refreshMindmap();
}

void Backend::setSidebarVisible(bool visible) {
    if (m_sidebarVisible == visible)
        return;

    m_sidebarVisible = visible;
    QSettings().setValue(sidebarVisibleSetting, visible);
    emit sidebarVisibleChanged();
}

void Backend::setPreviewVisible(bool visible) {
    if (m_previewVisible == visible)
        return;

    m_previewVisible = visible;
    QSettings().setValue(previewVisibleSetting, visible);
    emit previewVisibleChanged();
    if (visible)
        updatePreviewDocument();
}

void Backend::setZenMode(bool zen) {
    if (m_zenMode == zen)
        return;
    m_zenMode = zen;
    QSettings().setValue(zenModeSetting, zen);
    emit zenModeChanged();
}

void Backend::setPropertiesExpanded(bool expanded) {
    if (m_propertiesExpanded == expanded)
        return;
    m_propertiesExpanded = expanded;
    QSettings().setValue(propertiesExpandedSetting, expanded);
    emit propertiesExpandedChanged();
}

void Backend::setTypewriterScroll(bool enabled) {
    if (m_typewriterScroll == enabled)
        return;
    m_typewriterScroll = enabled;
    QSettings().setValue(typewriterScrollSetting, enabled);
    emit typewriterScrollChanged();
}

void Backend::setEditorMode(const QString &mode) {
    const QString normalized = mode.compare(QLatin1String("source"), Qt::CaseInsensitive) == 0
        ? QStringLiteral("source")
        : QStringLiteral("live");
    if (m_editorMode == normalized)
        return;
    m_editorMode = normalized;
    QSettings().setValue(editorModeSetting, normalized);
    emit editorModeChanged();
    scheduleLiveFolding();
}

void Backend::setProjectKeywordFilter(const QString &query) {
    if (m_projectKeywordFilter == query)
        return;
    m_projectKeywordFilter = query;
    emit projectFiltersChanged();
    refreshProjectRecords();
}

void Backend::setProjectDateFilter(const QString &filter) {
    QString normalized = filter.trimmed().toLower();
    static const QStringList allowed = {
        QStringLiteral("all"), QStringLiteral("today"), QStringLiteral("yesterday"),
        QStringLiteral("week"), QStringLiteral("dated"), QStringLiteral("undated")
    };
    if (!allowed.contains(normalized))
        normalized = QStringLiteral("all");
    if (m_projectDateFilter == normalized)
        return;
    m_projectDateFilter = normalized;
    emit projectFiltersChanged();
    refreshProjectRecords();
}

void Backend::setProjectPublishFilter(const QString &filter) {
    QString normalized = filter.trimmed().toLower();
    static const QStringList allowed = {
        QStringLiteral("all"), QStringLiteral("published"), QStringLiteral("drafts")
    };
    if (!allowed.contains(normalized))
        normalized = QStringLiteral("all");
    if (m_projectPublishFilter == normalized)
        return;
    m_projectPublishFilter = normalized;
    emit projectFiltersChanged();
    refreshProjectRecords();
}

void Backend::setProjectView(const QString &view) {
    QString normalized = view;
    if (normalized != QLatin1String("editor") && normalized != QLatin1String("table")
            && normalized != QLatin1String("board") && normalized != QLatin1String("calendar")
            && normalized != QLatin1String("mindmap") && normalized != QLatin1String("cards"))
        normalized = QStringLiteral("editor");
    if (m_projectView == normalized)
        return;

    m_projectView = normalized;
    QSettings().setValue(projectViewSetting, normalized);
    emit projectViewChanged();
    if (normalized == QLatin1String("mindmap"))
        refreshMindmap();
}

void Backend::setBoardField(const QString &field) {
    if (m_boardField == field)
        return;
    m_boardField = field;
    QSettings().setValue(boardFieldSetting, field);
    emit projectViewChanged();
    emit projectRecordsChanged();
}

void Backend::setDateField(const QString &field) {
    if (m_dateField == field)
        return;
    m_dateField = field;
    QSettings().setValue(dateFieldSetting, field);
    emit projectViewChanged();
    emit projectRecordsChanged();
    emit calendarChanged();
}

void Backend::attachDocument(QObject *textDocument) {
    auto *quickDocument = qobject_cast<QQuickTextDocument *>(textDocument);
    if (!quickDocument || !quickDocument->textDocument()) {
        setStatus(QStringLiteral("Could not attach the Markdown renderer."));
        return;
    }

    if (m_highlighter)
        delete m_highlighter.data();

    m_document = quickDocument->textDocument();
    m_lastDocumentText = m_document->toPlainText();
    m_highlighter = new MarkdownHighlighter(m_document);
    m_highlighter->setDarkMode(m_darkMode);
    m_highlighter->setColors(m_themeBackground, m_themeForeground, m_themeAccent);
    scheduleLiveFolding();

    connect(m_document, &QTextDocument::contentsChange, this,
            [this](int position, int, int charsAdded) {
                if (m_formattingTypography || m_loading)
                    return;
                m_lastChangePos = position;
                m_lastChangeAdded = charsAdded;
            });
    connect(m_document, &QTextDocument::contentsChanged, this, [this]() {
        schedulePreviewUpdate();
    });

    applyDocumentTypography();
    restoreRecovery();
    updatePreviewDocument();
}

void Backend::attachPreviewDocument(QObject *textDocument) {
    auto *quickDocument = qobject_cast<QQuickTextDocument *>(textDocument);
    if (!quickDocument || !quickDocument->textDocument()) {
        setStatus(QStringLiteral("Could not attach the Markdown preview."));
        return;
    }

    m_previewDocument = quickDocument->textDocument();
    m_previewDocument->setUndoRedoEnabled(false);
    updatePreviewDocument();
}

void Backend::openDialog() {
    emit openDialogRequested();
}

void Backend::openFolderDialog() {
    emit openFolderDialogRequested();
}

void Backend::open(const QUrl &url) {
    if (!url.isLocalFile()) {
        setStatus(QStringLiteral("Only local files can be opened."));
        return;
    }

    const QString targetName = QFileInfo(url.toLocalFile()).fileName();
    QFile file(url.toLocalFile());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setStatus(QStringLiteral("Could not open %1.").arg(targetName));
        return;
    }

    const QByteArray contents = file.readAll();
    loadDocumentText(QString::fromUtf8(contents));
    clearRecovery();
    m_lastKnownFileContents = contents;
    m_hasKnownFileContents = true;
    setFileUrl(url);
    syncWorkspaceForFile(url);
    watchCurrentFile();
    setModified(false);
    setStatus(QStringLiteral("Opened %1").arg(fileName()));
    recordRecentFile(url);
    refreshDocumentOutline();
    applyPreviewModeForDocument();
    refreshNoteLinks();
    emit fileOpened(url.toLocalFile());
}

void Backend::openFolder(const QUrl &url) {
    if (!url.isLocalFile()) {
        setStatus(QStringLiteral("Only local folders can be opened."));
        return;
    }

    const QString directoryPath = QDir(url.toLocalFile()).absolutePath();
    const QFileInfo info(directoryPath);
    if (!info.exists() || !info.isDir()) {
        setStatus(QStringLiteral("Could not open %1.").arg(info.fileName().isEmpty()
                                                               ? directoryPath
                                                               : info.fileName()));
        return;
    }

    setWorkspaceFolderUrl(QUrl::fromLocalFile(directoryPath));
    rememberWorkspaceDirectory(directoryPath);
    recordRecentWorkspace(directoryPath);
    const int archived = archiveAlreadyPublishedThreadsDrafts();
    if (archived > 0)
        setStatus(QStringLiteral("Opened workspace %1 · %2 moved to published")
                      .arg(workspaceFolderName())
                      .arg(archived));
    else
        setStatus(QStringLiteral("Opened workspace %1").arg(workspaceFolderName()));
}

void Backend::save() {
    if (!m_fileUrl.isValid() || m_fileUrl.isEmpty()) {
        saveAsDialog();
        return;
    }

    saveTo(m_fileUrl);
}

void Backend::saveForClose() {
    if (!m_modified) {
        emit closeAfterSave();
        return;
    }

    m_closeAfterSave = true;
    save();
}

void Backend::saveAsDialog() {
    emit saveDialogRequested(suggestedSaveUrl());
}

void Backend::saveAs(const QUrl &url) {
    saveTo(url);
}

void Backend::fileDialogCanceled() {
    m_closeAfterSave = false;
}

void Backend::discardRecovery() {
    clearRecovery();
}

void Backend::reloadFromDisk() {
    if (m_fileUrl.isLocalFile())
        open(m_fileUrl);
}

void Backend::keepExternalVersion() {
    QFile file(m_fileUrl.toLocalFile());
    if (file.open(QIODevice::ReadOnly)) {
        m_lastKnownFileContents = file.readAll();
        m_hasKnownFileContents = true;
    } else {
        m_lastKnownFileContents.clear();
        m_hasKnownFileContents = false;
    }
    setModified(true);
    scheduleRecovery();
    watchCurrentFile();
    setStatus(QStringLiteral("Kept your version"));
}

void Backend::printDocument() {
    if (!m_document) {
        setStatus(QStringLiteral("There is no document to print."));
        return;
    }

    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog dialog(&printer);
    dialog.setWindowTitle(QStringLiteral("Print %1").arg(fileName()));
    dialog.winId();
    if (dialog.windowHandle() && m_parentWindow)
        dialog.windowHandle()->setTransientParent(m_parentWindow);

    if (dialog.exec() == QDialog::Accepted) {
        QTextDocument rendered;
        rendered.setDefaultFont(m_document->defaultFont());
        rendered.setBaseUrl(previewBaseUrl(m_fileUrl, m_workspaceFolderUrl));
        rendered.setMarkdown(currentDocumentText());
        rendered.print(&printer);
    }
}

void Backend::newWindow() {
    const bool started = QProcess::startDetached(QCoreApplication::applicationFilePath(),
                                                 QStringList());
    if (!started)
        setStatus(QStringLiteral("Could not open a new window."));
}

QString Backend::defaultThreadsDraftsFolder() const {
    const QString personal = QDir::homePath() + QStringLiteral("/Documents/01_ACTIVE/threads");
    if (QDir(personal).exists())
        return personal;
    return QDir::homePath() + QStringLiteral("/Documents/FMD/threads");
}

QString Backend::expandUserPath(const QString &path) const {
    QString expanded = path.trimmed();
    if (expanded == QLatin1String("~"))
        return QDir::homePath();
    if (expanded.startsWith(QStringLiteral("~/")))
        return QDir::home().filePath(expanded.mid(2));
    return expanded;
}

QString Backend::resolvedThreadsDraftsFolder() const {
    const QString configured = expandUserPath(m_threadsDraftsFolder);
    if (configured.isEmpty())
        return QDir::cleanPath(defaultThreadsDraftsFolder());
    return QDir::cleanPath(configured);
}

QString Backend::threadsPublishedFolder() const {
    return QDir(resolvedThreadsDraftsFolder()).filePath(QStringLiteral("published"));
}

void Backend::setThreadsDraftsFolder(const QString &path) {
    const QString normalized = expandUserPath(path);
    if (m_threadsDraftsFolder == normalized)
        return;
    m_threadsDraftsFolder = normalized;
    QSettings().setValue(threadsDraftsFolderSetting, normalized);
    emit threadsDraftsFolderChanged();
}

void Backend::setNotesSiteFolder(const QString &path) {
    const QString normalized = expandUserPath(path);
    if (m_notesSiteFolder == normalized)
        return;
    m_notesSiteFolder = normalized;
    QSettings().setValue(notesSiteFolderSetting, normalized);
    emit notesSiteFolderChanged();
}

void Backend::setChangelogSiteFolder(const QString &path) {
    const QString normalized = expandUserPath(path);
    if (m_changelogSiteFolder == normalized)
        return;
    m_changelogSiteFolder = normalized;
    QSettings().setValue(changelogSiteFolderSetting, normalized);
    emit changelogSiteFolderChanged();
}

void Backend::setSlidesSiteFolder(const QString &path) {
    const QString normalized = expandUserPath(path);
    if (m_slidesSiteFolder == normalized)
        return;
    m_slidesSiteFolder = normalized;
    QSettings().setValue(slidesSiteFolderSetting, normalized);
    emit slidesSiteFolderChanged();
}

QString Backend::defaultSlidesSiteFolder() const {
    return QDir::homePath() + QStringLiteral("/Documents/01_ACTIVE/SLIDEV");
}

QString Backend::defaultGardenNotesFolder() const {
    return QDir::homePath() + QStringLiteral("/Documents/01_ACTIVE/GARDEN");
}

QString Backend::defaultNotesSiteFolder() const {
    return QDir::homePath() + QStringLiteral("/Projects/andgreen-notes");
}

QString Backend::defaultChangelogSiteFolder() const {
    return QDir::homePath() + QStringLiteral("/Projects/fmd-site");
}

void Backend::setGardenNotesFolder(const QString &path) {
    m_gardenNotesFolder = expandUserPath(path);
}

QString Backend::resolvedGardenNotesFolder() const {
    if (!m_gardenNotesFolder.isEmpty())
        return QDir::cleanPath(m_gardenNotesFolder);
    return defaultGardenNotesFolder();
}

QString Backend::resolvedSlidesSiteFolder() const {
    const QString set = expandUserPath(m_slidesSiteFolder);
    if (!set.isEmpty())
        return set;
    return defaultSlidesSiteFolder();
}

QString Backend::resolvedNotesSiteFolder() const {
    const QString set = expandUserPath(m_notesSiteFolder);
    if (!set.isEmpty())
        return set;
    return defaultNotesSiteFolder();
}

QString Backend::resolvedChangelogSiteFolder() const {
    const QString set = expandUserPath(m_changelogSiteFolder);
    if (!set.isEmpty())
        return set;
    return defaultChangelogSiteFolder();
}

bool Backend::slidevNote() const {
    return FrontMatter::isSlidevNote(FrontMatter::parse(editorPlainText()).fields);
}

QStringList Backend::propertyChoices(const QString &key) const {
    const QString k = key.trimmed().toLower();
    if (k == QLatin1String("threads") || k == QLatin1String("publish")
            || k == QLatin1String("draft") || k == QLatin1String("presenter")
            || k == QLatin1String("slidev"))
        return {QStringLiteral("true"), QStringLiteral("false")};
    if (k == QLatin1String("status")) {
        QStringList out = {
            QStringLiteral("draft"), QStringLiteral("queued"), QStringLiteral("scheduled"),
            QStringLiteral("sent"), QStringLiteral("published"), QStringLiteral("failed"),
            QStringLiteral("cancelled")
        };
        for (const QString &status : statusChoices()) {
            if (!out.contains(status))
                out.append(status);
        }
        return out;
    }
    if (k == QLatin1String("slidev"))
        return {QStringLiteral("true"), QStringLiteral("false")};
    if (k == QLatin1String("layout"))
        return {QStringLiteral("post"), QStringLiteral("page"), QStringLiteral("slides")};
    if (k == QLatin1String("platform"))
        return {QStringLiteral("threads")};
    if (k == QLatin1String("media_type"))
        return {QStringLiteral("TEXT"), QStringLiteral("IMAGE"), QStringLiteral("VIDEO"),
                QStringLiteral("CAROUSEL")};
    return {};
}

QStringList Backend::knownPropertyKeys() const {
    return {
        QStringLiteral("title"), QStringLiteral("status"), QStringLiteral("threads"),
        QStringLiteral("publish"), QStringLiteral("draft"), QStringLiteral("layout"),
        QStringLiteral("slidev"), QStringLiteral("theme"), QStringLiteral("colorSchema"),
        QStringLiteral("date"), QStringLiteral("scheduled"), QStringLiteral("platform"),
        QStringLiteral("media_type"), QStringLiteral("tags"),
        QStringLiteral("cover"), QStringLiteral("image"), QStringLiteral("pin"),
        QStringLiteral("shortcut")
    };
}

namespace {

QStringList relativeMediaPaths(const QString &markdown) {
    QStringList paths;
    static const QRegularExpression mdImg(QStringLiteral(R"(!\[[^\]]*\]\(([^)\s]+)[^)]*\))"));
    static const QRegularExpression wikiImg(QStringLiteral(R"(!\[\[([^|#\]]+))"));
    QRegularExpressionMatchIterator md = mdImg.globalMatch(markdown);
    while (md.hasNext())
        paths.append(md.next().captured(1).trimmed());
    QRegularExpressionMatchIterator wiki = wikiImg.globalMatch(markdown);
    while (wiki.hasNext())
        paths.append(wiki.next().captured(1).trimmed());
    paths.removeDuplicates();
    return paths;
}

bool copyFileOverwrite(const QString &from, const QString &to) {
    QDir().mkpath(QFileInfo(to).absolutePath());
    if (QFile::exists(to))
        QFile::remove(to);
    return QFile::copy(from, to);
}

QString gardenRelativeName(const QString &sourcePath, const QString &workspace) {
    if (sourcePath.isEmpty())
        return QStringLiteral("untitled.md");
    if (workspace.isEmpty())
        return QFileInfo(sourcePath).fileName();
    const QString relative =
        QDir::fromNativeSeparators(QDir(workspace).relativeFilePath(sourcePath));
    if (relative.startsWith(QLatin1String("..")) || QDir::isAbsolutePath(relative))
        return QFileInfo(sourcePath).fileName();
    return relative;
}

QStringList gardenPublishedStems(const QString &site, const QStringList &sourcePaths) {
    QSet<QString> stems;
    auto addPath = [&](const QString &path) {
        const QFileInfo info(path);
        const QString base = info.completeBaseName().toLower();
        if (!base.isEmpty())
            stems.insert(base);
        const QString name = info.fileName().toLower();
        if (!name.isEmpty())
            stems.insert(name);
    };
    for (const QString &path : sourcePaths)
        addPath(path);
    const QString content = QDir(site).filePath(QStringLiteral("content"));
    if (QDir(content).exists()) {
        for (const QString &path : Backend::markdownFilesInDirectory(content))
            addPath(path);
    }
    return QStringList(stems.begin(), stems.end());
}

QVariantMap writeGardenNote(const QString &site, const QString &sourcePath, const QString &text,
                            const QString &relativeName, const QStringList &publishedStems) {
    QVariantMap result{{QStringLiteral("ok"), false}};
    const QString contentDir = QDir(site).filePath(QStringLiteral("content"));
    QDir().mkpath(contentDir);
    const QString dest = QDir(contentDir).filePath(relativeName);
    if (!sourcePath.isEmpty()
            && QFileInfo(sourcePath).absoluteFilePath() == QFileInfo(dest).absoluteFilePath()) {
        result.insert(QStringLiteral("message"),
                      QStringLiteral("Notes site folder cannot be the writing folder."));
        return result;
    }
    const QString rewritten = MindMap::rewriteGardenWikilinks(text, publishedStems);
    QSaveFile out(dest);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
        result.insert(QStringLiteral("message"), QStringLiteral("Could not write %1").arg(dest));
        return result;
    }
    out.write(rewritten.toUtf8());
    if (!out.commit()) {
        result.insert(QStringLiteral("message"), QStringLiteral("Could not write %1").arg(dest));
        return result;
    }

    int copiedImages = 0;
    if (!sourcePath.isEmpty()) {
        const FrontMatter::Document parsed = FrontMatter::parse(text);
        const QDir sourceDir = QFileInfo(sourcePath).dir();
        const QDir destDir = QFileInfo(dest).dir();
        for (const QString &rel : relativeMediaPaths(parsed.body)) {
            if (rel.startsWith(QLatin1String("http://")) || rel.startsWith(QLatin1String("https://"))
                    || rel.startsWith(QLatin1Char('/')))
                continue;
            const QString from = QFileInfo(sourceDir.filePath(rel)).absoluteFilePath();
            if (!QFileInfo::exists(from))
                continue;
            const QString to = destDir.filePath(rel);
            if (copyFileOverwrite(from, to))
                ++copiedImages;
        }
    }

    result.insert(QStringLiteral("ok"), true);
    result.insert(QStringLiteral("path"), dest);
    result.insert(QStringLiteral("images"), copiedImages);
    return result;
}

} // namespace

QVariantMap Backend::publishCurrentNoteToSite() {
    const QString site = resolvedNotesSiteFolder();
    QVariantMap result{{QStringLiteral("ok"), false}};
    if (site.isEmpty() || !QDir(site).exists()) {
        result.insert(QStringLiteral("message"),
                      t(QStringLiteral("notesSiteFolderMissing")).arg(site));
        setStatus(result.value(QStringLiteral("message")).toString());
        return result;
    }
    const QString text = editorPlainText();
    const FrontMatter::Document parsed = FrontMatter::parse(text);
    if (!FrontMatter::isGardenPublishable(parsed.fields)) {
        const QString message = FrontMatter::isTruthy(parsed.fields.value(QStringLiteral("draft")))
            ? QStringLiteral("draft: true — not copied.")
            : QStringLiteral("Add publish: true (or layout: post) before publishing to the garden.");
        result.insert(QStringLiteral("message"), message);
        setStatus(message);
        return result;
    }

    const QString sourcePath = currentFilePath();
    const QString relative = gardenRelativeName(sourcePath, workspaceFolderPath());
    const QStringList stems = gardenPublishedStems(site, {sourcePath.isEmpty()
        ? QStringLiteral("untitled.md")
        : sourcePath});
    result = writeGardenNote(site, sourcePath, text, relative, stems);
    if (!result.value(QStringLiteral("ok")).toBool()) {
        setStatus(result.value(QStringLiteral("message")).toString());
        return result;
    }
    result.insert(QStringLiteral("hint"),
                  QStringLiteral("cd \"%1\" && git add content && git status && git commit -m \"Publish %2\" && git push")
                      .arg(site, QFileInfo(relative).fileName()));
    setStatus(QStringLiteral("Copied %1 to the garden site").arg(QFileInfo(relative).fileName()));
    return result;
}

QVariantMap Backend::publishWorkspaceToSite() {
    const QString site = resolvedNotesSiteFolder();
    QVariantMap result{{QStringLiteral("ok"), false}};
    if (site.isEmpty() || !QDir(site).exists()) {
        result.insert(QStringLiteral("message"),
                      t(QStringLiteral("notesSiteFolderMissing")).arg(site));
        setStatus(result.value(QStringLiteral("message")).toString());
        return result;
    }
    const QString workspace = workspaceFolderPath();
    if (workspace.isEmpty()) {
        result.insert(QStringLiteral("message"),
                      QStringLiteral("Open a project folder before publishing it to the garden."));
        setStatus(result.value(QStringLiteral("message")).toString());
        return result;
    }

    struct Candidate {
        QString path;
        QString text;
        QString relative;
    };
    QVector<Candidate> batch;
    int skipped = 0;
    for (const QString &path : markdownFilesInDirectory(workspace)) {
        if (isInboxPath(path)) {
            ++skipped;
            continue;
        }
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            ++skipped;
            continue;
        }
        const QString text = QString::fromUtf8(file.readAll());
        if (!FrontMatter::isGardenPublishable(FrontMatter::parse(text).fields)) {
            ++skipped;
            continue;
        }
        batch.append({path, text, gardenRelativeName(path, workspace)});
    }
    if (batch.isEmpty()) {
        result.insert(QStringLiteral("message"),
                      QStringLiteral("No publish: true notes in this folder."));
        result.insert(QStringLiteral("copied"), 0);
        result.insert(QStringLiteral("skipped"), skipped);
        setStatus(result.value(QStringLiteral("message")).toString());
        return result;
    }

    QStringList sourcePaths;
    for (const Candidate &item : batch)
        sourcePaths.append(item.path);
    const QStringList stems = gardenPublishedStems(site, sourcePaths);

    QStringList copied;
    int images = 0;
    for (const Candidate &item : batch) {
        const QVariantMap written =
            writeGardenNote(site, item.path, item.text, item.relative, stems);
        if (!written.value(QStringLiteral("ok")).toBool()) {
            result = written;
            setStatus(written.value(QStringLiteral("message")).toString());
            return result;
        }
        copied.append(item.relative);
        images += written.value(QStringLiteral("images")).toInt();
    }

    result.insert(QStringLiteral("ok"), true);
    result.insert(QStringLiteral("copied"), copied.size());
    result.insert(QStringLiteral("skipped"), skipped);
    result.insert(QStringLiteral("images"), images);
    result.insert(QStringLiteral("files"), copied);
    result.insert(QStringLiteral("hint"),
                  QStringLiteral("cd \"%1\" && git add content && git status && git commit -m \"Publish folder\" && git push")
                      .arg(site));
    setStatus(QStringLiteral("Copied %1 notes to the garden site").arg(copied.size()));
    return result;
}

QVariantMap Backend::publishChangelogToSite() {
    const QString site = resolvedChangelogSiteFolder();
    QVariantMap result{{QStringLiteral("ok"), false}};
    if (site.isEmpty() || !QDir(site).exists()) {
        result.insert(QStringLiteral("message"),
                      t(QStringLiteral("changelogSiteFolderMissing")).arg(site));
        setStatus(result.value(QStringLiteral("message")).toString());
        return result;
    }
    const QString dest = QDir(site).filePath(QStringLiteral("CHANGELOG.md"));
    QSaveFile out(dest);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
        result.insert(QStringLiteral("message"), QStringLiteral("Could not write CHANGELOG.md"));
        setStatus(result.value(QStringLiteral("message")).toString());
        return result;
    }
    out.write(editorPlainText().toUtf8());
    if (!out.commit()) {
        result.insert(QStringLiteral("message"), QStringLiteral("Could not write CHANGELOG.md"));
        setStatus(result.value(QStringLiteral("message")).toString());
        return result;
    }
    result.insert(QStringLiteral("ok"), true);
    result.insert(QStringLiteral("path"), dest);
    result.insert(QStringLiteral("hint"),
                  QStringLiteral("cd \"%1\" && git add CHANGELOG.md && git status && git commit -m \"Update changelog\" && git push")
                      .arg(site));
    setStatus(QStringLiteral("Copied changelog to the product site"));
    return result;
}

QString Backend::findSitePushBinary() const {
    const QString found = QStandardPaths::findExecutable(QStringLiteral("site-push"));
    if (!found.isEmpty())
        return found;
    const QString homeBin = QDir::homePath() + QStringLiteral("/bin/site-push");
    if (QFileInfo::exists(homeBin))
        return homeBin;
    const QString bundled = QCoreApplication::applicationDirPath()
        + QStringLiteral("/../../../fmd/bin/site-push");
    if (QFileInfo::exists(bundled))
        return QFileInfo(bundled).absoluteFilePath();
    return {};
}

QVariantMap Backend::runSitePush(const QString &repo, const QStringList &paths,
                                 const QString &message, bool dryRun) {
    QVariantMap result{{QStringLiteral("ok"), false}};
    const QString binary = findSitePushBinary();
    if (binary.isEmpty()) {
        result.insert(QStringLiteral("message"),
                      QStringLiteral("Copied files, but site-push is missing. Put it in ~/bin."));
        return result;
    }
    QStringList arguments{
        QStringLiteral("--repo"), repo,
        QStringLiteral("--paths"), paths.join(QLatin1Char(',')),
        QStringLiteral("--message"), message,
    };
    if (dryRun)
        arguments.append(QStringLiteral("--dry-run"));

    m_sitePushBusy = true;
    emit sitePushBusyChanged();
    QProcess process;
    process.setProgram(binary);
    process.setArguments(arguments);
    process.setWorkingDirectory(repo);
    process.start();
    if (!process.waitForStarted(5000)) {
        m_sitePushBusy = false;
        emit sitePushBusyChanged();
        result.insert(QStringLiteral("message"), QStringLiteral("Could not start site-push."));
        return result;
    }
    if (!process.waitForFinished(120000)) {
        process.kill();
        process.waitForFinished(2000);
        m_sitePushBusy = false;
        emit sitePushBusyChanged();
        result.insert(QStringLiteral("message"), QStringLiteral("site-push timed out."));
        return result;
    }
    m_sitePushBusy = false;
    emit sitePushBusyChanged();

    const QString output = QString::fromUtf8(process.readAllStandardOutput()
                                             + process.readAllStandardError());
    result.insert(QStringLiteral("output"), output);
    result.insert(QStringLiteral("message"), output);
    bool ok = process.exitCode() == 0;
    bool noop = false;
    bool pushed = false;
    for (const QString &line : output.split(QLatin1Char('\n'))) {
        const QString trimmed = line.trimmed();
        if (trimmed.startsWith(QStringLiteral("ok=true")))
            ok = true;
        if (trimmed.startsWith(QStringLiteral("ok=false")))
            ok = false;
        if (trimmed.startsWith(QStringLiteral("noop=true")))
            noop = true;
        if (trimmed.startsWith(QStringLiteral("pushed=true")))
            pushed = true;
        if (trimmed.startsWith(QStringLiteral("error:")))
            result.insert(QStringLiteral("message"), trimmed.mid(6).trimmed());
    }
    result.insert(QStringLiteral("ok"), ok);
    result.insert(QStringLiteral("noop"), noop);
    result.insert(QStringLiteral("pushed"), pushed);
    result.insert(QStringLiteral("dryRun"), dryRun);
    return result;
}

QVariantMap Backend::publishThenPush(const QVariantMap &copied, const QString &repo,
                                     const QStringList &paths, const QString &message,
                                     bool dryRun) {
    QVariantMap result = copied;
    if (!copied.value(QStringLiteral("ok")).toBool())
        return result;
    const QVariantMap pushed = runSitePush(repo, paths, message, dryRun);
    result.insert(QStringLiteral("pushOk"), pushed.value(QStringLiteral("ok")));
    result.insert(QStringLiteral("pushed"), pushed.value(QStringLiteral("pushed")));
    result.insert(QStringLiteral("noop"), pushed.value(QStringLiteral("noop")));
    result.insert(QStringLiteral("pushOutput"), pushed.value(QStringLiteral("output")));
    result.insert(QStringLiteral("dryRun"), dryRun);
    if (!pushed.value(QStringLiteral("ok")).toBool()) {
        result.insert(QStringLiteral("ok"), false);
        result.insert(QStringLiteral("message"),
                      pushed.value(QStringLiteral("message")).toString());
        setStatus(result.value(QStringLiteral("message")).toString());
        return result;
    }
    if (dryRun)
        setStatus(QStringLiteral("Copied, dry-run push OK — not sent"));
    else if (pushed.value(QStringLiteral("noop")).toBool())
        setStatus(QStringLiteral("Copied. Git already up to date."));
    else
        setStatus(QStringLiteral("Copied and pushed to GitHub Pages."));
    result.insert(QStringLiteral("message"), m_status);
    return result;
}

QVariantMap Backend::publishGardenNow(bool dryRun) {
    return publishThenPush(publishCurrentNoteToSite(),
                           resolvedNotesSiteFolder(),
                           {QStringLiteral("content")},
                           QStringLiteral("Publish garden note from FMD"),
                           dryRun);
}

QVariantMap Backend::publishFolderGardenNow(bool dryRun) {
    return publishThenPush(publishWorkspaceToSite(),
                           resolvedNotesSiteFolder(),
                           {QStringLiteral("content")},
                           QStringLiteral("Publish garden folder from FMD"),
                           dryRun);
}

QVariantMap Backend::publishChangelogNow(bool dryRun) {
    return publishThenPush(publishChangelogToSite(),
                           resolvedChangelogSiteFolder(),
                           {QStringLiteral("CHANGELOG.md")},
                           QStringLiteral("Update changelog from FMD"),
                           dryRun);
}

QVariantMap Backend::emptySlidevResult(const QString &message) const {
    return {
        {QStringLiteral("ok"), false},
        {QStringLiteral("message"), message},
        {QStringLiteral("output"), message},
        {QStringLiteral("url"), QString()},
        {QStringLiteral("path"), QString()},
        {QStringLiteral("slides"), 0},
        {QStringLiteral("dryRun"), false},
        {QStringLiteral("started"), false},
    };
}

QVariantMap Backend::parseSlidevPresentOutput(const QString &text, int exitCode) const {
    QVariantMap result = emptySlidevResult(QString());
    result.insert(QStringLiteral("ok"), exitCode == 0);
    result.insert(QStringLiteral("output"), text);
    result.insert(QStringLiteral("message"), text);
    for (const QString &line : text.split(QLatin1Char('\n'))) {
        const QString trimmed = line.trimmed();
        if (trimmed.startsWith(QStringLiteral("ok=true")))
            result.insert(QStringLiteral("ok"), true);
        if (trimmed.startsWith(QStringLiteral("ok=false")))
            result.insert(QStringLiteral("ok"), false);
        if (trimmed.startsWith(QStringLiteral("url=")))
            result.insert(QStringLiteral("url"), trimmed.mid(4).trimmed());
        if (trimmed.startsWith(QStringLiteral("file=")))
            result.insert(QStringLiteral("path"), trimmed.mid(5).trimmed());
        if (trimmed.startsWith(QStringLiteral("DRY-RUN")) || trimmed.startsWith(QStringLiteral("dryRun=true")))
            result.insert(QStringLiteral("dryRun"), true);
        if (trimmed.startsWith(QStringLiteral("error:")))
            result.insert(QStringLiteral("message"), trimmed.mid(6).trimmed());
    }
    if (exitCode != 0)
        result.insert(QStringLiteral("ok"), false);
    return result;
}

QString Backend::findSlidevPresentBinary() const {
    const QString found = QStandardPaths::findExecutable(QStringLiteral("slidev-present"));
    if (!found.isEmpty())
        return found;
    const QString homeBin = QDir::homePath() + QStringLiteral("/bin/slidev-present");
    if (QFileInfo::exists(homeBin))
        return homeBin;
    const QString bundled = QCoreApplication::applicationDirPath()
        + QStringLiteral("/../../../fmd/bin/slidev-present");
    if (QFileInfo::exists(bundled))
        return QFileInfo(bundled).absoluteFilePath();
    return {};
}

bool Backend::ensureSlidesProject(const QString &folder, QString *error) const {
    QDir dir(folder);
    if (!dir.exists() && !QDir().mkpath(folder)) {
        if (error)
            *error = QStringLiteral("Could not create %1").arg(folder);
        return false;
    }
    QDir().mkpath(dir.filePath(QStringLiteral("decks")));
    const QString packagePath = dir.filePath(QStringLiteral("package.json"));
    if (!QFileInfo::exists(packagePath)) {
        QSaveFile package(packagePath);
        if (!package.open(QIODevice::WriteOnly | QIODevice::Text)) {
            if (error)
                *error = QStringLiteral("Could not write package.json");
            return false;
        }
        package.write(QByteArrayLiteral(
            "{\n"
            "  \"name\": \"fmd-slides\",\n"
            "  \"private\": true,\n"
            "  \"scripts\": {\n"
            "    \"dev\": \"slidev\",\n"
            "    \"build\": \"slidev build\"\n"
            "  },\n"
            "  \"devDependencies\": {\n"
            "    \"@slidev/cli\": \"^52.0.0\",\n"
            "    \"@slidev/theme-default\": \"^0.25.0\"\n"
            "  }\n"
            "}\n"));
        if (!package.commit()) {
            if (error)
                *error = QStringLiteral("Could not write package.json");
            return false;
        }
    }
    const QString gitignore = dir.filePath(QStringLiteral(".gitignore"));
    if (!QFileInfo::exists(gitignore)) {
        QSaveFile ignore(gitignore);
        if (ignore.open(QIODevice::WriteOnly | QIODevice::Text)) {
            ignore.write(QByteArrayLiteral("node_modules\n.slidev\ndist\n"));
            ignore.commit();
        }
    }
    return true;
}

QVariantMap Backend::writeSlidevDeck(const QString &markdown, const QString &sourcePath,
                                     const QString &folder) const {
    QVariantMap result = emptySlidevResult(QString());
    QString error;
    if (!ensureSlidesProject(folder, &error)) {
        result.insert(QStringLiteral("message"), error);
        result.insert(QStringLiteral("output"), error);
        return result;
    }
    QString slug = QFileInfo(sourcePath).completeBaseName();
    slug.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9._-]+")), QStringLiteral("-"));
    slug = slug.trimmed();
    if (slug.isEmpty())
        slug = QStringLiteral("untitled");
    const QString deckDir = QDir(folder).filePath(QStringLiteral("decks/") + slug);
    QDir().mkpath(deckDir);
    const QString exported = CodeBlocks::slidevMarkdown(markdown);
    const QString dest = QDir(deckDir).filePath(QStringLiteral("slides.md"));
    QSaveFile out(dest);
    if (!out.open(QIODevice::WriteOnly | QIODevice::Text)) {
        result.insert(QStringLiteral("message"), QStringLiteral("Could not write slides.md"));
        result.insert(QStringLiteral("output"), result.value(QStringLiteral("message")));
        return result;
    }
    out.write(exported.toUtf8());
    if (!out.commit()) {
        result.insert(QStringLiteral("message"), QStringLiteral("Could not write slides.md"));
        result.insert(QStringLiteral("output"), result.value(QStringLiteral("message")));
        return result;
    }
    const QString libraryCss = QDir(folder).filePath(QStringLiteral("style.css"));
    const QString deckCss = QDir(deckDir).filePath(QStringLiteral("style.css"));
    if (QFileInfo::exists(libraryCss) && !QFileInfo::exists(deckCss))
        copyFileOverwrite(libraryCss, deckCss);
    int images = 0;
    if (!sourcePath.isEmpty()) {
        const QDir sourceDir = QFileInfo(sourcePath).dir();
        const QDir destDir(deckDir);
        for (const QString &rel : relativeMediaPaths(markdown)) {
            if (rel.startsWith(QLatin1String("http://")) || rel.startsWith(QLatin1String("https://"))
                    || rel.startsWith(QLatin1Char('/')))
                continue;
            const QString from = QFileInfo(sourceDir.filePath(rel)).absoluteFilePath();
            if (!QFileInfo::exists(from))
                continue;
            if (copyFileOverwrite(from, destDir.filePath(rel)))
                ++images;
        }
    }
    result.insert(QStringLiteral("ok"), true);
    result.insert(QStringLiteral("path"), dest);
    result.insert(QStringLiteral("slides"), CodeBlocks::slidevSlideCount(exported));
    result.insert(QStringLiteral("images"), images);
    result.insert(QStringLiteral("message"), QStringLiteral("Wrote %1").arg(dest));
    result.insert(QStringLiteral("output"), result.value(QStringLiteral("message")));
    return result;
}

QVariantMap Backend::validateSlidevDraft() const {
    const QString text = editorPlainText();
    const FrontMatter::Document parsed = FrontMatter::parse(text);
    if (!FrontMatter::isSlidevNote(parsed.fields)) {
        const QString message = FrontMatter::isTruthy(parsed.fields.value(QStringLiteral("draft")))
            ? QStringLiteral("draft: true — not a Slidev deck.")
            : QStringLiteral("Set layout: slides (or slidev: true) in properties first.");
        QVariantMap result = emptySlidevResult(message);
        return result;
    }
    bool hasHeading = false;
    for (const QVariant &item : FrontMatter::headingOutline(text)) {
        if (item.toMap().value(QStringLiteral("level")).toInt() <= 2) {
            hasHeading = true;
            break;
        }
    }
    if (!hasHeading)
        return emptySlidevResult(QStringLiteral("Add a # or ## heading so Slidev has at least one slide."));
    const QString exported = CodeBlocks::slidevMarkdown(text);
    QVariantMap result;
    result.insert(QStringLiteral("ok"), true);
    result.insert(QStringLiteral("slides"), CodeBlocks::slidevSlideCount(exported));
    result.insert(QStringLiteral("path"), currentFilePath());
    result.insert(QStringLiteral("url"), QString());
    result.insert(QStringLiteral("message"),
                  QStringLiteral("%1 slides · will open Slidev in the browser").arg(
                      result.value(QStringLiteral("slides")).toInt()));
    result.insert(QStringLiteral("output"), result.value(QStringLiteral("message")));
    result.insert(QStringLiteral("dryRun"), false);
    return result;
}

QVariantMap Backend::publishSlidevNow(bool dryRun) {
    if (m_slidevBusy)
        return emptySlidevResult(QStringLiteral("Already presenting."));
    const QVariantMap check = validateSlidevDraft();
    if (!check.value(QStringLiteral("ok")).toBool()) {
        setStatus(check.value(QStringLiteral("message")).toString());
        return check;
    }
    const QString folder = resolvedSlidesSiteFolder();
    QVariantMap written = writeSlidevDeck(editorPlainText(), currentFilePath(), folder);
    if (!written.value(QStringLiteral("ok")).toBool()) {
        setStatus(written.value(QStringLiteral("message")).toString());
        return written;
    }
    if (m_slidesSiteFolder.isEmpty())
        setSlidesSiteFolder(folder);

    const QString dest = written.value(QStringLiteral("path")).toString();
    const QString binary = findSlidevPresentBinary();
    if (binary.isEmpty()) {
        written.insert(QStringLiteral("ok"), false);
        written.insert(QStringLiteral("message"),
                       QStringLiteral("Copied slides, but slidev-present is missing. Put it in ~/bin."));
        written.insert(QStringLiteral("output"), written.value(QStringLiteral("message")));
        setStatus(written.value(QStringLiteral("message")).toString());
        return written;
    }

    QStringList arguments{QStringLiteral("present"), dest};
    if (dryRun)
        arguments.insert(1, QStringLiteral("--dry-run"));

    if (dryRun) {
        QProcess process;
        process.setProgram(binary);
        process.setArguments(arguments);
        process.setWorkingDirectory(folder);
        process.start();
        if (!process.waitForStarted(5000))
            return emptySlidevResult(QStringLiteral("Could not start slidev-present."));
        if (!process.waitForFinished(15000)) {
            process.kill();
            process.waitForFinished(2000);
            return emptySlidevResult(QStringLiteral("slidev-present timed out."));
        }
        QVariantMap parsed = parseSlidevPresentOutput(
            QString::fromUtf8(process.readAllStandardOutput())
                + QString::fromUtf8(process.readAllStandardError()),
            process.exitCode());
        parsed.insert(QStringLiteral("path"), dest);
        parsed.insert(QStringLiteral("slides"), written.value(QStringLiteral("slides")));
        parsed.insert(QStringLiteral("dryRun"), true);
        if (parsed.value(QStringLiteral("ok")).toBool())
            setStatus(QStringLiteral("Slidev dry-run OK — not opened"));
        else
            setStatus(QStringLiteral("Slidev dry-run failed"));
        return parsed;
    }

    m_slidevDryRun = false;
    m_slidevBusy = true;
    emit slidevBusyChanged();
    setStatus(QStringLiteral("Opening Slidev…"));
    m_slidevProcess.setProgram(binary);
    m_slidevProcess.setArguments(arguments);
    m_slidevProcess.setWorkingDirectory(folder);
    m_slidevProcess.start();
    if (!m_slidevProcess.waitForStarted(5000)) {
        m_slidevBusy = false;
        emit slidevBusyChanged();
        return emptySlidevResult(QStringLiteral("Could not start slidev-present."));
    }
    m_slidevTimeout.start();
    QVariantMap started = written;
    started.insert(QStringLiteral("ok"), true);
    started.insert(QStringLiteral("started"), true);
    started.insert(QStringLiteral("output"), QStringLiteral("Starting Slidev…"));
    return started;
}

void Backend::abortSlidevPublish() {
    if (!m_slidevBusy)
        return;
    m_slidevAborting = true;
    m_slidevTimeout.stop();
    if (m_slidevProcess.state() != QProcess::NotRunning) {
        m_slidevProcess.kill();
        m_slidevProcess.waitForFinished(2000);
    }
    finishSlidevJob(emptySlidevResult(QStringLiteral("Slidev publish cancelled.")));
    m_slidevAborting = false;
}

void Backend::finishSlidevJob(const QVariantMap &result) {
    if (!m_slidevBusy)
        return;
    m_slidevTimeout.stop();
    m_slidevBusy = false;
    emit slidevBusyChanged();

    QVariantMap report = result;
    if (result.value(QStringLiteral("ok")).toBool() && !m_slidevDryRun) {
        const QString current = editorPlainText();
        QString updated = FrontMatter::setField(current, QStringLiteral("status"),
                                                QStringLiteral("sent"));
        updated = FrontMatter::setField(updated, QStringLiteral("slidev"), QStringLiteral("true"));
        if (FrontMatter::displayValue(FrontMatter::parse(updated).fields.value(QStringLiteral("layout")))
                .trimmed().compare(QStringLiteral("slides"), Qt::CaseInsensitive)
            != 0)
            updated = FrontMatter::setField(updated, QStringLiteral("layout"),
                                            QStringLiteral("slides"));
        if (updated != current) {
            replaceEditorRange(0, current.size(), updated);
            save();
        }
        const QString url = result.value(QStringLiteral("url")).toString();
        if (!url.isEmpty()) {
            setStatus(QStringLiteral("Slidev %1").arg(url));
            openExternalUrl(QUrl(url));
        } else {
            setStatus(QStringLiteral("Slidev sent"));
        }
    } else if (result.value(QStringLiteral("ok")).toBool()) {
        setStatus(QStringLiteral("Slidev dry-run OK — not opened"));
    } else {
        setStatus(QStringLiteral("Slidev publish failed"));
    }
    emit slidevPublishFinished(report);
}

QString Backend::threadsTemplateText(const QString &folderPath) const {
    const QString templatePath = QDir(folderPath).filePath(QStringLiteral("_TEMPLATE-threads.md"));
    QFile file(templatePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        return QString::fromUtf8(file.readAll());

    const QString today = QDate::currentDate().toString(Qt::ISODate);
    return QStringLiteral("---\n"
                          "title: \n"
                          "threads: true\n"
                          "status: draft\n"
                          "date: %1\n"
                          "scheduled: %1 09:00\n"
                          "platform: threads\n"
                          "media_type: TEXT\n"
                          "media: []\n"
                          "queue_id: \n"
                          "permalink: \n"
                          "last_error: \n"
                          "---\n"
                          "\n"
                          "\n"
                          "---\n"
                          "\n"
                          "---\n"
                          "\n")
        .arg(today);
}

QString Backend::uniqueThreadsDraftPath(const QString &folderPath) const {
    const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmm"));
    QString fileName = QStringLiteral("threads-%1.md").arg(stamp);
    int suffix = 2;
    while (QFileInfo::exists(QDir(folderPath).filePath(fileName))) {
        fileName = QStringLiteral("threads-%1-%2.md").arg(stamp).arg(suffix++);
    }
    return QDir(folderPath).filePath(fileName);
}

QString Backend::unusedThreadsDraftPath(const QString &folderPath) const {
    const QString templateBody =
        FrontMatter::parse(threadsTemplateText(folderPath)).body.trimmed();
    QDir dir(folderPath);
    const QFileInfoList files = dir.entryInfoList(QStringList{QStringLiteral("threads-*.md")},
                                                  QDir::Files, QDir::Time);
    for (const QFileInfo &info : files) {
        QFile file(info.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            continue;
        const FrontMatter::Document document =
            FrontMatter::parse(QString::fromUtf8(file.readAll()));
        if (!FrontMatter::isThreadsPost(document.fields))
            continue;
        const QString status = document.fields.value(QStringLiteral("status")).toString();
        if (!status.isEmpty() && status != QLatin1String("draft"))
            continue;
        if (!document.fields.value(QStringLiteral("queue_id")).toString().isEmpty())
            continue;
        if (!document.fields.value(QStringLiteral("permalink")).toString().isEmpty())
            continue;
        if (document.body.trimmed() != templateBody)
            continue;
        return info.absoluteFilePath();
    }
    return {};
}

QUrl Backend::createThreadsDraft() {
    QString folder = resolvedThreadsDraftsFolder();
    const QString workspace = workspaceFolderPath();
    if (!workspace.isEmpty()
        && QDir(workspace).canonicalPath() == QDir(folder).canonicalPath())
        folder = workspace;

    if (!QDir().mkpath(folder)) {
        setStatus(QStringLiteral("Could not create the Threads drafts folder."));
        return {};
    }

    const QUrl folderUrl = QUrl::fromLocalFile(folder);
    auto revealFolder = [&]() {
        if (workspaceFolderPath().isEmpty()
            || QDir(workspaceFolderPath()).canonicalPath() != QDir(folder).canonicalPath())
            openFolder(folderUrl);
    };

    const QString reusable = unusedThreadsDraftPath(folder);
    if (!reusable.isEmpty()) {
        revealFolder();
        setStatus(t(QStringLiteral("reusedDraft")).arg(QFileInfo(reusable).fileName()));
        return QUrl::fromLocalFile(reusable);
    }

    QString text = threadsTemplateText(folder);
    const QString today = QDate::currentDate().toString(Qt::ISODate);
    text = FrontMatter::setField(text, QStringLiteral("threads"), QStringLiteral("true"));
    text = FrontMatter::setField(text, QStringLiteral("status"), QStringLiteral("draft"));
    text = FrontMatter::setField(text, QStringLiteral("date"), today);
    text = FrontMatter::setField(text, QStringLiteral("scheduled"), today + QStringLiteral(" 09:00"));
    text = FrontMatter::setField(text, QStringLiteral("platform"), QStringLiteral("threads"));

    const QString path = uniqueThreadsDraftPath(folder);
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        setStatus(QStringLiteral("Could not create a Threads draft."));
        return {};
    }
    file.write(text.toUtf8());
    if (!file.commit()) {
        setStatus(QStringLiteral("Could not create a Threads draft."));
        return {};
    }

    revealFolder();
    setStatus(t(QStringLiteral("createdDraft")).arg(QFileInfo(path).fileName()));
    return QUrl::fromLocalFile(path);
}

QUrl Backend::createMarkdownNote() {
    const QString folder = createTargetDirectory();
    if (folder.isEmpty() || !QDir(folder).exists()) {
        setStatus(QStringLiteral("Open a folder first, then create a note."));
        emit openFolderDialogRequested();
        return {};
    }

    QString fileName = QStringLiteral("Untitled.md");
    int suffix = 2;
    while (QFileInfo::exists(QDir(folder).filePath(fileName))) {
        fileName = QStringLiteral("Untitled-%1.md").arg(suffix++);
    }
    const QString path = QDir(folder).filePath(fileName);

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        setStatus(QStringLiteral("Could not create %1.").arg(fileName));
        return {};
    }
    file.write(QByteArrayLiteral("\n"));
    if (!file.commit()) {
        setStatus(QStringLiteral("Could not create %1.").arg(fileName));
        return {};
    }

    m_collapsedFolders.removeAll(QDir(folder).absolutePath());
    persistCollapsedFolders();
    refreshWorkspaceFiles();
    setStatus(QStringLiteral("Created %1").arg(fileName));
    return QUrl::fromLocalFile(path);
}

QString Backend::renderTemplateText(const QString &source, const QString &title,
                                    const QString &filename)
{
    const QDateTime now = QDateTime::currentDateTime();
    static const QRegularExpression tokenRe(
        QStringLiteral(R"(\{\{(date|time)(?::([^}]*))?\}\})"));
    QString out;
    int last = 0;
    QRegularExpressionMatchIterator it = tokenRe.globalMatch(source);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        out += source.mid(last, match.capturedStart() - last);
        const QString kind = match.captured(1);
        QString format = match.captured(2).trimmed();
        if (format.isEmpty()) {
            format = kind == QLatin1String("time") ? QStringLiteral("HH:mm")
                                                   : QStringLiteral("yyyy-MM-dd");
        } else {
            format = templaterToQtDateFormat(format);
        }
        out += now.toString(format);
        last = match.capturedEnd();
    }
    out += source.mid(last);
    out.replace(QStringLiteral("{{title}}"), title);
    out.replace(QStringLiteral("{{filename}}"), filename);
    return out;
}

QString Backend::renderTemplate(const QUrl &templateUrl) const
{
    if (!templateUrl.isLocalFile())
        return {};
    QFile file(templateUrl.toLocalFile());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    const QString source = QString::fromUtf8(file.readAll());
    const QString name = fileName();
    const QString title = QFileInfo(name).completeBaseName();
    return renderTemplateText(source, title, name);
}

QStringList Backend::templateDirectoryPaths() const
{
    const QString root = workspaceFolderPath();
    if (root.isEmpty())
        return {};
    return {QDir(root).filePath(QStringLiteral("_templates")),
            QDir(root).filePath(QStringLiteral(".fmd/templates"))};
}

void Backend::refreshTemplateFiles()
{
    QVariantList files;
    const QString root = workspaceFolderPath();
    for (const QString &directoryPath : templateDirectoryPaths()) {
        const QDir directory(directoryPath);
        if (!directory.exists())
            continue;
        QDirIterator iterator(directory.absolutePath(), QDir::Files | QDir::Readable | QDir::NoDotAndDotDot,
                              QDirIterator::Subdirectories);
        while (iterator.hasNext()) {
            iterator.next();
            const QFileInfo entry = iterator.fileInfo();
            if (!isMarkdownFileName(entry.fileName()))
                continue;
            if (shouldSkipMarkdownFileName(entry.fileName()))
                continue;
            const QString relative =
                QDir::fromNativeSeparators(QDir(root).relativeFilePath(entry.absoluteFilePath()));
            files.append(QVariantMap{
                {QStringLiteral("name"), relative},
                {QStringLiteral("title"), entry.completeBaseName()},
                {QStringLiteral("url"), QUrl::fromLocalFile(entry.absoluteFilePath())},
            });
        }
    }
    std::sort(files.begin(), files.end(), [](const QVariant &left, const QVariant &right) {
        return QString::compare(left.toMap().value(QStringLiteral("name")).toString(),
                                right.toMap().value(QStringLiteral("name")).toString(),
                                Qt::CaseInsensitive)
            < 0;
    });
    if (files == m_templateFiles)
        return;
    m_templateFiles = files;
    emit templateFilesChanged();
}

QUrl Backend::ensureDefaultTemplate()
{
    const QString folder = workspaceFolderPath();
    if (folder.isEmpty() || !QDir(folder).exists()) {
        setStatus(QStringLiteral("Open a folder first, then create a template."));
        emit openFolderDialogRequested();
        return {};
    }

    const QString directoryPath = QDir(folder).filePath(QStringLiteral("_templates"));
    if (!QDir().mkpath(directoryPath)) {
        setStatus(QStringLiteral("Could not create _templates."));
        return {};
    }

    const QString path = QDir(directoryPath).filePath(QStringLiteral("note.md"));
    if (!QFileInfo::exists(path)) {
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            setStatus(QStringLiteral("Could not create _templates/note.md."));
            return {};
        }
        file.write(QByteArrayLiteral("# {{title}}\n\n{{date}} {{time}}\n"));
        if (!file.commit()) {
            setStatus(QStringLiteral("Could not create _templates/note.md."));
            return {};
        }
        setStatus(QStringLiteral("Created _templates/note.md"));
    }

    refreshTemplateFiles();
    return QUrl::fromLocalFile(QFileInfo(path).absoluteFilePath());
}

QUrl Backend::createNoteFromTemplate(const QUrl &templateUrl)
{
    const QString folder = workspaceFolderPath();
    if (folder.isEmpty() || !QDir(folder).exists()) {
        setStatus(QStringLiteral("Open a folder first, then create a note."));
        emit openFolderDialogRequested();
        return {};
    }

    QUrl sourceUrl = templateUrl;
    if (!sourceUrl.isLocalFile() || !QFileInfo::exists(sourceUrl.toLocalFile()))
        sourceUrl = ensureDefaultTemplate();
    if (!sourceUrl.isLocalFile())
        return {};

    QFile templateFile(sourceUrl.toLocalFile());
    if (!templateFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        setStatus(QStringLiteral("Could not read that template."));
        return {};
    }
    const QString source = QString::fromUtf8(templateFile.readAll());

    QString fileName = QStringLiteral("Untitled.md");
    int suffix = 2;
    while (QFileInfo::exists(QDir(folder).filePath(fileName)))
        fileName = QStringLiteral("Untitled-%1.md").arg(suffix++);
    const QString path = QDir(folder).filePath(fileName);
    const QString rendered =
        renderTemplateText(source, QFileInfo(fileName).completeBaseName(), fileName);

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        setStatus(QStringLiteral("Could not create %1.").arg(fileName));
        return {};
    }
    file.write(rendered.toUtf8());
    if (!file.commit()) {
        setStatus(QStringLiteral("Could not create %1.").arg(fileName));
        return {};
    }

    refreshWorkspaceFiles();
    setStatus(QStringLiteral("Created %1 from template").arg(fileName));
    return QUrl::fromLocalFile(path);
}

bool Backend::renameNote(const QUrl &url, const QString &newName) {
    if (!url.isLocalFile())
        return false;

    const QFileInfo from(url.toLocalFile());
    const QString sourcePath = from.absoluteFilePath();
    if (!from.exists() || !isMarkdownFileName(from.fileName())) {
        setStatus(QStringLiteral("Could not rename that file."));
        return false;
    }
    if (shouldSkipMarkdownFileName(from.fileName())) {
        setStatus(QStringLiteral("Protected file — not renamed."));
        return false;
    }

    const QString stem = sanitizeNoteStem(newName);
    if (stem.isEmpty()) {
        setStatus(QStringLiteral("Invalid name."));
        return false;
    }

    const QString extension = from.fileName().endsWith(QStringLiteral(".markdown"),
                                                       Qt::CaseInsensitive)
        ? QStringLiteral(".markdown")
        : QStringLiteral(".md");
    const QString destination = uniqueNotePath(from.absolutePath(), stem, extension, sourcePath);
    if (destination == sourcePath)
        return true;

    const bool current = m_fileUrl.isLocalFile()
        && QFileInfo(m_fileUrl.toLocalFile()).absoluteFilePath() == sourcePath;
    const QStringList watched = m_fileWatcher.files();
    if (current && !watched.isEmpty())
        m_fileWatcher.removePaths(watched);

    if (!QFile::rename(sourcePath, destination)) {
        if (current)
            watchCurrentFile();
        setStatus(QStringLiteral("Could not rename %1.").arg(from.fileName()));
        return false;
    }

    moveMarkdownAssets(from, QFileInfo(destination));

    if (current) {
        setFileUrl(QUrl::fromLocalFile(destination));
        watchCurrentFile();
        recordRecentFile(m_fileUrl);
    }

    refreshWorkspaceFiles();
    setStatus(QStringLiteral("Renamed to %1").arg(QFileInfo(destination).fileName()));
    return true;
}

QString Backend::gardenTemplateText() const {
    const QString today = QDate::currentDate().toString(Qt::ISODate);
    return QStringLiteral("---\n"
                          "title: 花園筆記\n"
                          "layout: post\n"
                          "publish: true\n"
                          "date: %1\n"
                          "tags:\n"
                          "  - garden\n"
                          "---\n"
                          "\n"
                          "第一段。用 [[連到這篇]] 接其他已發布的篇；還沒上站的連結發布時會變成純文字。\n"
                          "\n"
                          "## 這一則想講什麼\n"
                          "\n"
                          "- 一件事實\n"
                          "- 一個連出去的想法\n"
                          "\n")
        .arg(today);
}

QString Backend::slidevTemplateText() const {
    return QStringLiteral("---\n"
                          "title: 簡報\n"
                          "layout: slides\n"
                          "status: queued\n"
                          "slidev: true\n"
                          "colorSchema: light\n"
                          "---\n"
                          "\n"
                          "# 用 Markdown 寫完，在瀏覽器裡講 {layout: cover}\n"
                          "\n"
                          "聽眾是誰、這幾分鐘要帶走什麼、為什麼現在講。封面不要只剩標題加一句話。\n"
                          "\n"
                          "## 換主題\n"
                          "\n"
                          "屬性列加 `theme: seriph`。不要填 night／newsprint／gothic，那是 FMD 右側預覽。\n"
                          "\n"
                          "不填就是內建 default。`colorSchema: light` 或 `dark` 可鎖死，不跟系統跑。\n"
                          "\n"
                          "改完再按一次「立刻開 Slidev」。\n"
                          "\n"
                          "## 跳頁：g 或 ⌘G\n"
                          "\n"
                          "在 Slidev **瀏覽器**視窗按 `g`（Go to）。有的鍵盤是 ⌘G。那不是 FMD 的尋找。\n"
                          "\n"
                          "跳出頁碼／標題清單，輸入數字或點一張。關掉後右側不該再黏一份幽靈大綱。\n"
                          "\n"
                          "`o` 總覽　`v` 講者窗　`f` 全螢幕　`←` `→` 翻頁\n"
                          "\n"
                          "## 證據 {layout: two-cols}\n"
                          "\n"
                          "左邊放主張。一句講完。\n"
                          "\n"
                          "::right::\n"
                          "\n"
                          "右邊放對照。DOM class 是 `.two-columns`，庫根 `style.css` 已設 gap。\n"
                          "\n"
                          "## 下一步\n"
                          "\n"
                          "請他們做的一件具體的事。切燈用 `#`／`##`，不要用 `---`。完整學習稿在 `00-fmd-one-click-starter.md`。\n"
                          "\n");
}

QString Backend::uniquePrefixedDraftPath(const QString &folder, const QString &prefix) const {
    const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmm"));
    QString fileName = QStringLiteral("%1-%2.md").arg(prefix, stamp);
    int suffix = 2;
    while (QFileInfo::exists(QDir(folder).filePath(fileName))) {
        fileName = QStringLiteral("%1-%2-%3.md").arg(prefix, stamp).arg(suffix++);
    }
    return QDir(folder).filePath(fileName);
}

QString Backend::unusedMatchingDraftPath(const QString &folder, const QString &glob,
                                         const QString &templateText) const {
    const QString templateBody = FrontMatter::parse(templateText).body.trimmed();
    const QFileInfoList files = QDir(folder).entryInfoList(QStringList{glob}, QDir::Files, QDir::Time);
    for (const QFileInfo &info : files) {
        QFile file(info.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            continue;
        const FrontMatter::Document document =
            FrontMatter::parse(QString::fromUtf8(file.readAll()));
        const QString status = FrontMatter::displayValue(document.fields.value(QStringLiteral("status")));
        if (!status.isEmpty() && status != QLatin1String("draft") && status != QLatin1String("queued"))
            continue;
        if (document.body.trimmed() != templateBody)
            continue;
        return info.absoluteFilePath();
    }
    return {};
}

QUrl Backend::createPrefixedDraft(const QString &folder, const QString &prefix,
                                  const QString &templateFileName, const QString &fallbackText,
                                  const QVariantMap &fields) {
    if (!QDir().mkpath(folder)) {
        setStatus(QStringLiteral("Could not create %1").arg(folder));
        return {};
    }

    const QUrl folderUrl = QUrl::fromLocalFile(folder);
    auto revealFolder = [&]() {
        if (workspaceFolderPath().isEmpty()
            || QDir(workspaceFolderPath()).canonicalPath() != QDir(folder).canonicalPath())
            openFolder(folderUrl);
    };

    const QString templatePath = QDir(folder).filePath(templateFileName);
    if (!QFileInfo::exists(templatePath)) {
        QSaveFile seed(templatePath);
        if (seed.open(QIODevice::WriteOnly | QIODevice::Text)) {
            seed.write(fallbackText.toUtf8());
            seed.commit();
        }
    }
    QString text = fallbackText;
    QFile templateFile(templatePath);
    if (templateFile.open(QIODevice::ReadOnly | QIODevice::Text))
        text = QString::fromUtf8(templateFile.readAll());

    const QString reusable = unusedMatchingDraftPath(folder, prefix + QStringLiteral("-*.md"), text);
    if (!reusable.isEmpty()) {
        revealFolder();
        setStatus(t(QStringLiteral("reusedDraft")).arg(QFileInfo(reusable).fileName()));
        return QUrl::fromLocalFile(reusable);
    }

    for (auto it = fields.constBegin(); it != fields.constEnd(); ++it)
        text = FrontMatter::setField(text, it.key(), it.value().toString());

    const QString path = uniquePrefixedDraftPath(folder, prefix);
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        setStatus(QStringLiteral("Could not create a note."));
        return {};
    }
    file.write(text.toUtf8());
    if (!file.commit()) {
        setStatus(QStringLiteral("Could not create a note."));
        return {};
    }

    revealFolder();
    setStatus(t(QStringLiteral("createdDraft")).arg(QFileInfo(path).fileName()));
    return QUrl::fromLocalFile(path);
}

QUrl Backend::ensureNamedTemplate(const QString &folder, const QString &fileName,
                                  const QString &fallbackText, const QString &openedMessage) {
    if (!QDir().mkpath(folder)) {
        setStatus(QStringLiteral("Could not open %1").arg(folder));
        return {};
    }
    const QString path = QDir(folder).filePath(fileName);
    if (!QFileInfo::exists(path)) {
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            setStatus(QStringLiteral("Could not create %1.").arg(fileName));
            return {};
        }
        file.write(fallbackText.toUtf8());
        if (!file.commit()) {
            setStatus(QStringLiteral("Could not create %1.").arg(fileName));
            return {};
        }
    }
    const QUrl folderUrl = QUrl::fromLocalFile(folder);
    if (workspaceFolderPath().isEmpty()
        || QDir(workspaceFolderPath()).canonicalPath() != QDir(folder).canonicalPath())
        openFolder(folderUrl);
    setStatus(openedMessage);
    return QUrl::fromLocalFile(path);
}

QUrl Backend::createGardenNote() {
    const QString folder = resolvedGardenNotesFolder();
    QVariantMap fields;
    fields.insert(QStringLiteral("layout"), QStringLiteral("post"));
    fields.insert(QStringLiteral("publish"), QStringLiteral("true"));
    fields.insert(QStringLiteral("date"), QDate::currentDate().toString(Qt::ISODate));
    return createPrefixedDraft(folder, QStringLiteral("garden"),
                               QStringLiteral("_TEMPLATE-garden.md"), gardenTemplateText(), fields);
}

QUrl Backend::createSlidevNote() {
    const QString folder = resolvedSlidesSiteFolder();
    if (m_slidesSiteFolder.isEmpty())
        setSlidesSiteFolder(folder);
    QVariantMap fields;
    fields.insert(QStringLiteral("layout"), QStringLiteral("slides"));
    fields.insert(QStringLiteral("status"), QStringLiteral("queued"));
    fields.insert(QStringLiteral("slidev"), QStringLiteral("true"));
    return createPrefixedDraft(folder, QStringLiteral("slides"),
                               QStringLiteral("_TEMPLATE-slides.md"), slidevTemplateText(), fields);
}

QUrl Backend::ensureGardenTemplate() {
    return ensureNamedTemplate(resolvedGardenNotesFolder(), QStringLiteral("_TEMPLATE-garden.md"),
                               gardenTemplateText(), QStringLiteral("Opened garden template"));
}

QUrl Backend::ensureSlidevTemplate() {
    const QString folder = resolvedSlidesSiteFolder();
    if (m_slidesSiteFolder.isEmpty())
        setSlidesSiteFolder(folder);
    return ensureNamedTemplate(folder, QStringLiteral("_TEMPLATE-slides.md"),
                               slidevTemplateText(), QStringLiteral("Opened Slidev template"));
}

QUrl Backend::ensureThreadsTemplate() {
    const QString folder = resolvedThreadsDraftsFolder();
    if (!QDir().mkpath(folder)) {
        setStatus(QStringLiteral("Could not open the Threads drafts folder."));
        return {};
    }

    const QString path = QDir(folder).filePath(QStringLiteral("_TEMPLATE-threads.md"));
    if (!QFileInfo::exists(path)) {
        QSaveFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            setStatus(QStringLiteral("Could not create _TEMPLATE-threads.md."));
            return {};
        }
        file.write(threadsTemplateText(folder).toUtf8());
        if (!file.commit()) {
            setStatus(QStringLiteral("Could not create _TEMPLATE-threads.md."));
            return {};
        }
    }

    const QUrl folderUrl = QUrl::fromLocalFile(folder);
    if (workspaceFolderPath().isEmpty()
        || QDir(workspaceFolderPath()).canonicalPath() != QDir(folder).canonicalPath())
        openFolder(folderUrl);

    setStatus(QStringLiteral("Opened Threads template"));
    return QUrl::fromLocalFile(path);
}

bool Backend::moveNoteToTrash(const QUrl &url) {
    if (!url.isLocalFile())
        return false;

    const QFileInfo info(url.toLocalFile());
    const QString path = info.absoluteFilePath();
    const bool folder = info.isDir();
    if (!info.exists() || (!folder && !isMarkdownFileName(info.fileName()))) {
        setStatus(QStringLiteral("Could not move that file to Trash."));
        return false;
    }
    if (!folder && shouldSkipMarkdownFileName(info.fileName())) {
        setStatus(QStringLiteral("Protected file — not moved."));
        return false;
    }
    if (folder && path == workspaceFolderPath()) {
        setStatus(QStringLiteral("Cannot move the project root to Trash from here."));
        return false;
    }

    const bool current = m_fileUrl.isLocalFile()
        && (QFileInfo(m_fileUrl.toLocalFile()).absoluteFilePath() == path
            || (folder && isPathUnderDirectory(m_fileUrl.toLocalFile(), path)));

    QString error;
    if (!QFile::moveToTrash(path, &error)) {
        setStatus(error.isEmpty() ? QStringLiteral("Could not move %1 to Trash.").arg(info.fileName())
                                  : error);
        return false;
    }

    const QString assets = info.absolutePath() + QLatin1Char('/') + info.completeBaseName()
        + QStringLiteral(".assets");
    if (QFileInfo::exists(assets))
        QFile::moveToTrash(assets);

    if (current) {
        m_fileWatcher.removePaths(m_fileWatcher.files());
        setFileUrl({});
        loadDocumentText(QString());
        setModified(false);
        clearRecovery();
    }
    if (m_selectedWorkspacePath == path
        || m_selectedWorkspacePath.startsWith(path + QLatin1Char('/')))
        setSelectedWorkspacePath({});

    refreshWorkspaceFiles();
    refreshInboxFiles();
    setStatus(QStringLiteral("Moved %1 to Trash").arg(info.fileName()));
    return true;
}

void Backend::revealInFinder(const QUrl &url) {
    if (!url.isLocalFile())
        return;
    const QString path = url.toLocalFile();
    if (!QFileInfo::exists(path))
        return;
#if defined(Q_OS_MACOS)
    QProcess::startDetached(QStringLiteral("open"), {QStringLiteral("-R"), path});
#else
    QDesktopServices::openUrl(QUrl::fromLocalFile(QFileInfo(path).absolutePath()));
#endif
}

QVariantMap Backend::runThreadsSchedule(const QStringList &arguments, int timeoutMs) {
    if (!m_fileUrl.isLocalFile())
        return emptyThreadsScheduleResult(QStringLiteral("Save the note first."));

    const QString binary = findThreadsScheduleBinary();
    if (binary.isEmpty()) {
        return emptyThreadsScheduleResult(
            QStringLiteral("threads-schedule not found. Put it on PATH or in ~/bin."));
    }

    QProcess process;
    process.setProgram(binary);
    process.setArguments(arguments);
    process.setWorkingDirectory(QFileInfo(m_fileUrl.toLocalFile()).absolutePath());
    process.start();
    if (!process.waitForStarted(5000))
        return emptyThreadsScheduleResult(QStringLiteral("Could not start threads-schedule."));
    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        process.waitForFinished(2000);
        return emptyThreadsScheduleResult(QStringLiteral("threads-schedule timed out."));
    }

    const QString output = QString::fromUtf8(process.readAllStandardOutput())
        + QString::fromUtf8(process.readAllStandardError());
    return parseThreadsScheduleOutput(output, process.exitCode());
}

QVariantMap Backend::validateThreadsDraft() {
    if (!m_fileUrl.isLocalFile())
        return emptyThreadsScheduleResult(QStringLiteral("Save the note first."));
    setStatus(QStringLiteral("Checking Threads YAML…"));
    const QVariantMap result = runThreadsSchedule(
        {QStringLiteral("validate-omawrite"), m_fileUrl.toLocalFile()}, 15000);
    if (result.value(QStringLiteral("ok")).toBool())
        setStatus(QStringLiteral("Threads check passed"));
    else
        setStatus(QStringLiteral("Threads check failed"));
    return result;
}

QVariantMap Backend::publishThreadsNow(bool dryRun) {
    if (m_threadsBusy)
        return emptyThreadsScheduleResult(QStringLiteral("Already publishing."));
    if (!m_fileUrl.isLocalFile())
        return emptyThreadsScheduleResult(QStringLiteral("Save the note first."));

    const QString binary = findThreadsScheduleBinary();
    if (binary.isEmpty()) {
        return emptyThreadsScheduleResult(
            QStringLiteral("threads-schedule not found. Put it on PATH or in ~/bin."));
    }

    QStringList arguments{QStringLiteral("publish-now")};
    if (dryRun)
        arguments.append(QStringLiteral("--dry-run"));
    arguments.append(m_fileUrl.toLocalFile());

    m_threadsDryRun = dryRun;
    m_threadsBusy = true;
    emit threadsBusyChanged();
    setStatus(dryRun ? QStringLiteral("Dry-run publish…")
                     : QStringLiteral("Publishing to Threads…"));

    m_threadsProcess.setProgram(binary);
    m_threadsProcess.setArguments(arguments);
    m_threadsProcess.setWorkingDirectory(QFileInfo(m_fileUrl.toLocalFile()).absolutePath());
    m_threadsProcess.start();
    if (!m_threadsProcess.waitForStarted(5000)) {
        m_threadsBusy = false;
        emit threadsBusyChanged();
        return emptyThreadsScheduleResult(QStringLiteral("Could not start threads-schedule."));
    }
    m_threadsTimeout.start();
    QVariantMap started;
    started.insert(QStringLiteral("ok"), true);
    started.insert(QStringLiteral("started"), true);
    started.insert(QStringLiteral("output"), QStringLiteral("Publishing…"));
    started.insert(QStringLiteral("mainChars"), 0);
    started.insert(QStringLiteral("replies"), 0);
    return started;
}

void Backend::abortThreadsPublish() {
    if (!m_threadsBusy)
        return;
    m_threadsAborting = true;
    m_threadsTimeout.stop();
    if (m_threadsProcess.state() != QProcess::NotRunning) {
        m_threadsProcess.kill();
        m_threadsProcess.waitForFinished(2000);
    }
    finishThreadsJob(emptyThreadsScheduleResult(QStringLiteral("Publish cancelled.")));
    m_threadsAborting = false;
}

void Backend::finishThreadsJob(const QVariantMap &result) {
    if (!m_threadsBusy)
        return;
    m_threadsTimeout.stop();
    m_threadsBusy = false;
    emit threadsBusyChanged();

    if (result.value(QStringLiteral("ok")).toBool()) {
        const bool dry = m_threadsDryRun || result.value(QStringLiteral("dryRun")).toBool();
        const QString permalink = result.value(QStringLiteral("permalink")).toString();
        const bool published = result.value(QStringLiteral("published")).toBool()
            || !permalink.isEmpty();
        if (!dry && published) {
            const QString moved = archivePublishedThreadsPost(currentFilePath());
            if (!moved.isEmpty() && moved != currentFilePath()) {
                setFileUrl(QUrl::fromLocalFile(moved));
                watchCurrentFile();
            }
        }
        reloadFromDisk();
        if (dry)
            setStatus(QStringLiteral("Dry-run OK — not published"));
        else if (!permalink.isEmpty())
            setStatus(QStringLiteral("Published %1").arg(permalink));
        else
            setStatus(QStringLiteral("Published"));
    } else {
        setStatus(QStringLiteral("Publish failed"));
    }
    emit threadsPublishFinished(result);
}

bool Backend::isPathUnderDirectory(const QString &filePath, const QString &directoryPath) const {
    const QString root = QDir(directoryPath).canonicalPath();
    if (root.isEmpty())
        return false;

    QString file = QFileInfo(filePath).canonicalFilePath();
    if (file.isEmpty())
        file = QDir::cleanPath(QFileInfo(filePath).absoluteFilePath());
    return file == root || file.startsWith(root + QLatin1Char('/'));
}

void Backend::moveMarkdownAssets(const QFileInfo &from, const QFileInfo &to) const {
    const QString source = from.absolutePath() + QLatin1Char('/') + from.completeBaseName()
        + QStringLiteral(".assets");
    const QString destination = to.absolutePath() + QLatin1Char('/') + to.completeBaseName()
        + QStringLiteral(".assets");
    if (!QFileInfo::exists(source) || source == destination)
        return;
    if (QFileInfo::exists(destination))
        return;
    QDir().rename(source, destination);
}

bool threadsPostIsPublished(const QVariantMap &fields)
{
    const QString status =
        FrontMatter::displayValue(fields.value(QStringLiteral("status"))).trimmed().toLower();
    if (status == QLatin1String("sent") || status == QLatin1String("published"))
        return true;
    return false;
}

int Backend::archiveAlreadyPublishedThreadsDrafts()
{
    const QString drafts = resolvedThreadsDraftsFolder();
    const QString workspace = workspaceFolderPath();
    if (drafts.isEmpty() || workspace.isEmpty())
        return 0;
    const QString draftsAbs = QDir(drafts).absolutePath();
    const QString workspaceAbs = QDir(workspace).absolutePath();
    if (draftsAbs != workspaceAbs)
        return 0;

    const QDir dir(draftsAbs);
    const QFileInfoList files = dir.entryInfoList(
        QStringList{QStringLiteral("*.md"), QStringLiteral("*.markdown")},
        QDir::Files | QDir::Readable | QDir::NoDotAndDotDot);
    const QString current = currentFilePath();
    QString movedCurrent;
    int moved = 0;
    for (const QFileInfo &info : files) {
        if (shouldSkipMarkdownFileName(info.fileName()))
            continue;
        const QString path = info.absoluteFilePath();
        if (m_modified && path == current)
            continue;
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            continue;
        const QString text = QString::fromUtf8(file.readAll());
        file.close();
        const FrontMatter::Document parsed = FrontMatter::parse(text);
        if (!FrontMatter::isThreadsPost(parsed.fields) || !threadsPostIsPublished(parsed.fields))
            continue;
        const QString archived = archivePublishedThreadsPost(path);
        if (archived.isEmpty() || archived == path)
            continue;
        ++moved;
        if (path == current)
            movedCurrent = archived;
    }
    if (!movedCurrent.isEmpty()) {
        setFileUrl(QUrl::fromLocalFile(movedCurrent));
        watchCurrentFile();
    }
    if (moved > 0)
        refreshWorkspaceFiles();
    return moved;
}

QString Backend::archivePublishedThreadsPost(const QString &savedPath)
{
    if (savedPath.isEmpty() || !QFileInfo::exists(savedPath))
        return savedPath;
    const QString folder = threadsPublishedFolder();
    if (!QDir().mkpath(folder))
        return savedPath;
    if (isPathUnderDirectory(savedPath, folder))
        return savedPath;

    const QFileInfo sourceInfo(savedPath);
    QString candidate = QDir(folder).filePath(sourceInfo.fileName());
    if (QFileInfo::exists(candidate)
        && QFileInfo(candidate).canonicalFilePath() != sourceInfo.canonicalFilePath()) {
        candidate = uniqueNotePath(folder, sourceInfo.completeBaseName(),
                                   QStringLiteral(".") + sourceInfo.suffix(), savedPath);
    }
    if (!QFile::rename(savedPath, candidate)) {
        if (!QFile::copy(savedPath, candidate) || !QFile::remove(savedPath))
            return savedPath;
    }
    moveMarkdownAssets(sourceInfo, QFileInfo(candidate));
    refreshWorkspaceFiles();
    return candidate;
}

QString Backend::routeThreadsPostAfterSave(const QString &savedPath, const QString &contents) {
    const FrontMatter::Document parsed = FrontMatter::parse(contents);
    if (!FrontMatter::isThreadsPost(parsed.fields))
        return savedPath;

    QString patched = contents;
    if (!parsed.fields.contains(QStringLiteral("threads"))
        || !FrontMatter::isTruthy(parsed.fields.value(QStringLiteral("threads"))))
        patched = FrontMatter::setField(patched, QStringLiteral("threads"), QStringLiteral("true"));
    if (FrontMatter::displayValue(parsed.fields.value(QStringLiteral("platform")))
            .trimmed()
            .compare(QStringLiteral("threads"), Qt::CaseInsensitive)
        != 0)
        patched = FrontMatter::setField(patched, QStringLiteral("platform"), QStringLiteral("threads"));
    if (FrontMatter::displayValue(parsed.fields.value(QStringLiteral("media_type"))).trimmed().isEmpty())
        patched = FrontMatter::setField(patched, QStringLiteral("media_type"), QStringLiteral("TEXT"));

    const bool published = threadsPostIsPublished(parsed.fields);
    const QString folder = published ? threadsPublishedFolder() : resolvedThreadsDraftsFolder();
    if (!QDir().mkpath(folder))
        return savedPath;

    QString destination = savedPath;
    const QFileInfo sourceInfo(savedPath);
    if (!isPathUnderDirectory(savedPath, folder)) {
        QString fileName = sourceInfo.fileName();
        QString candidate = QDir(folder).filePath(fileName);
        if (QFileInfo::exists(candidate)) {
            const QString title = FrontMatter::recordTitle(parsed.fields, parsed.body,
                                                           sourceInfo.completeBaseName());
            QString slug = title.trimmed();
            slug.replace(QRegularExpression(QStringLiteral("[\\\\/:*?\"<>|]")), QStringLiteral("-"));
            slug.replace(QRegularExpression(QStringLiteral("\\s+")), QStringLiteral("-"));
            if (!slug.isEmpty()) {
                candidate = QDir(folder).filePath(slug + QStringLiteral(".md"));
                if (QFileInfo::exists(candidate))
                    candidate = uniqueThreadsDraftPath(folder);
            } else {
                candidate = uniqueThreadsDraftPath(folder);
            }
        }

        if (!QFile::rename(savedPath, candidate)) {
            if (!QFile::copy(savedPath, candidate) || !QFile::remove(savedPath))
                return savedPath;
        }
        moveMarkdownAssets(sourceInfo, QFileInfo(candidate));
        destination = candidate;
    }

    if (patched != contents) {
        QSaveFile file(destination);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            file.write(patched.toUtf8());
            file.commit();
        }
    }

    return destination;
}

QString Backend::clipboardUrl() const {
    const QClipboard *clipboard = QGuiApplication::clipboard();
    if (!clipboard)
        return {};

    const QMimeData *mimeData = clipboard->mimeData();
    if (!mimeData)
        return {};

    if (mimeData->hasUrls()) {
        const QList<QUrl> urls = mimeData->urls();
        for (const QUrl &url : urls) {
            const QString normalized = normalizedLinkUrl(url.toString());
            if (!normalized.isEmpty())
                return normalized;
        }
    }

    if (!mimeData->hasText())
        return {};

    return normalizedLinkUrl(mimeData->text());
}

QString Backend::clipboardText() const {
    const QClipboard *clipboard = QGuiApplication::clipboard();
    if (!clipboard)
        return {};

    const QMimeData *mimeData = clipboard->mimeData();
    return mimeData && mimeData->hasText() ? mimeData->text() : QString();
}

QUrl Backend::localImageUrlFromClipboardText(const QString &clipboardText) {
    const QUrl url = localFileUrlFromClipboardText(clipboardText);
    return isImageUrl(url) ? url : QUrl();
}

bool Backend::clipboardContainsImportableImage() const {
    const QClipboard *clipboard = QGuiApplication::clipboard();
    if (!clipboard)
        return false;

    const QMimeData *mimeData = clipboard->mimeData();
    if (!mimeData)
        return false;

    if (mimeData->hasUrls()) {
        for (const QUrl &url : mimeData->urls()) {
            if (isImageUrl(url))
                return true;
        }
    }

    if (mimeData->hasText() && localImageUrlFromClipboardText(mimeData->text()).isValid())
        return true;

    if (mimeData->hasImage()) {
        const QImage image = qvariant_cast<QImage>(mimeData->imageData());
        if (!image.isNull())
            return true;
    }

    return false;
}

QString Backend::documentAssetsDirectoryPath() const {
    const QFileInfo info(m_fileUrl.toLocalFile());
    return info.absolutePath() + QLatin1Char('/') + info.completeBaseName()
        + QStringLiteral(".assets");
}

QString Backend::markdownForDocumentImage(const QString &imagePath) const {
    const QString markdownDir = QFileInfo(m_fileUrl.toLocalFile()).absolutePath();
    QString relative = QDir(markdownDir).relativeFilePath(imagePath);
    relative.replace(QLatin1Char('\\'), QLatin1Char('/'));
    if (!relative.startsWith(QLatin1Char('.')))
        relative.prepend(QStringLiteral("./"));

    QString altText = QFileInfo(imagePath).completeBaseName().trimmed();
    if (altText.isEmpty())
        altText = QFileInfo(imagePath).fileName().trimmed();
    if (altText.isEmpty())
        altText = QStringLiteral("Image");

    return QStringLiteral("![%1](%2)")
        .arg(escapeMarkdownLinkText(altText),
             escapeMarkdownLinkDestination(encodeRelativeMarkdownPath(relative)));
}

QString Backend::importImageFile(const QUrl &sourceUrl) {
    if (!m_fileUrl.isLocalFile()) {
        setStatus(QStringLiteral("Save the document before pasting images."));
        return {};
    }
    if (!isImageUrl(sourceUrl))
        return {};

    const QString sourcePath = QDir::cleanPath(sourceUrl.toLocalFile());
    const QFileInfo sourceInfo(sourcePath);
    if (!sourceInfo.exists() || !sourceInfo.isFile()) {
        setStatus(QStringLiteral("Could not find the image file."));
        return {};
    }

    const QString markdownDir = QFileInfo(m_fileUrl.toLocalFile()).absolutePath();
    const QString sourceAbsolute = sourceInfo.absoluteFilePath();
    QString destination = sourceAbsolute;
    if (!sourceAbsolute.startsWith(markdownDir + QLatin1Char('/'))) {
        const QString assetsDir = documentAssetsDirectoryPath();
        if (!QDir().mkpath(assetsDir)) {
            setStatus(QStringLiteral("Could not create the image folder."));
            return {};
        }

        destination = uniqueAssetFilePath(assetsDir,
                                          sanitizeAssetBaseName(sourceInfo.completeBaseName()),
                                          sourceInfo.suffix().toLower());
        if (!QFile::copy(sourceAbsolute, destination)) {
            setStatus(QStringLiteral("Could not copy the image into the document folder."));
            return {};
        }
    }

    return markdownForDocumentImage(destination);
}

QString Backend::importClipboardBitmap(const QImage &image) {
    if (image.isNull())
        return {};
    if (!m_fileUrl.isLocalFile()) {
        setStatus(QStringLiteral("Save the document before pasting images."));
        return {};
    }

    const QString assetsDir = documentAssetsDirectoryPath();
    if (!QDir().mkpath(assetsDir)) {
        setStatus(QStringLiteral("Could not create the image folder."));
        return {};
    }

    const QString stamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"));
    const QString destination =
        uniqueAssetFilePath(assetsDir, QStringLiteral("pasted-") + stamp, QStringLiteral("png"));
    if (!image.save(destination, "PNG")) {
        setStatus(QStringLiteral("Could not save the pasted image."));
        return {};
    }

    return markdownForDocumentImage(destination);
}

QString Backend::clipboardImageMarkdown() {
    const QClipboard *clipboard = QGuiApplication::clipboard();
    if (!clipboard)
        return {};

    const QMimeData *mimeData = clipboard->mimeData();
    if (!mimeData)
        return {};

    if (!clipboardContainsImportableImage())
        return {};

    if (!m_fileUrl.isLocalFile()) {
        setStatus(QStringLiteral("Save the document before pasting images."));
        return {};
    }

    if (mimeData->hasUrls()) {
        for (const QUrl &url : mimeData->urls()) {
            const QString markdown = importImageFile(url);
            if (!markdown.isEmpty())
                return markdown;
        }
    }

    if (mimeData->hasText()) {
        const QString markdown =
            importImageFile(localImageUrlFromClipboardText(mimeData->text()));
        if (!markdown.isEmpty())
            return markdown;
    }

    if (mimeData->hasImage()) {
        const QImage image = qvariant_cast<QImage>(mimeData->imageData());
        const QString markdown = importClipboardBitmap(image);
        if (!markdown.isEmpty())
            return markdown;
    }

    return {};
}

bool Backend::editorTextChanged() {
    if (m_loading || m_formattingTypography)
        return false;

    const QString text = currentDocumentText();
    if (text == m_lastDocumentText)
        return false;
    m_lastDocumentText = text;

    if (m_document) {
        const int blockCount = m_document->blockCount();
        if (blockCount > m_formattedBlockCount)
            reapplyTypographyToChange();
        m_formattedBlockCount = blockCount;
    }

    scheduleWordCount();
    schedulePreviewUpdate();
    setModified(true);
    setStatus(QStringLiteral("Unsaved"));
    scheduleRecovery();
    scheduleUntitledRename();
    return true;
}

QVariantList Backend::hiddenRangesAt(int position) const {
    QVariantList ranges;
    QList<QPair<int, int>> spans;
    if (m_editorMode == QLatin1String("live")) {
        const FrontMatter::Document parsed = FrontMatter::parse(editorPlainText());
        if (parsed.hasFrontMatter && parsed.yamlStart >= 0 && parsed.yamlEnd > parsed.yamlStart)
            spans.append({parsed.yamlStart, parsed.yamlEnd});
    }
    if (!m_document) {
        for (const auto &span : spans) {
            ranges.append(QVariantMap{{QStringLiteral("start"), span.first},
                                      {QStringLiteral("end"), span.second}});
        }
        return ranges;
    }

    const QTextBlock block =
        m_document->findBlock(qBound(0, position, m_document->characterCount() - 1));
    if (!block.isValid()) {
        for (const auto &span : spans) {
            ranges.append(QVariantMap{{QStringLiteral("start"), span.first},
                                      {QStringLiteral("end"), span.second}});
        }
        return ranges;
    }

    const int lineStart = block.position();
    const QList<MarkdownHighlighter::InlineMarkup> markup =
        MarkdownHighlighter::inlineMarkup(block.text());
    for (const MarkdownHighlighter::InlineMarkup &item : markup) {
        for (const MarkdownHighlighter::Span &marker : item.markers) {
            spans.append({lineStart + marker.start,
                          lineStart + marker.start + marker.length});
        }
    }
    std::sort(spans.begin(), spans.end());

    for (const auto &span : spans) {
        ranges.append(QVariantMap{{QStringLiteral("start"), span.first},
                                  {QStringLiteral("end"), span.second}});
    }
    return ranges;
}

void Backend::setSearchHighlight(const QString &query, int currentMatchStart) {
    if (m_highlighter)
        m_highlighter->setSearch(query, currentMatchStart);
}

void Backend::openExternalUrl(const QUrl &url) {
    const QString scheme = url.scheme().toLower();
    if (scheme != QStringLiteral("http") && scheme != QStringLiteral("https")
            && scheme != QStringLiteral("mailto"))
        return;
    if (!QDesktopServices::openUrl(url)) {
        setStatus(QStringLiteral("Could not open that link."));
        return;
    }
    const QString label = url.host().isEmpty() ? url.toString() : url.host();
    setStatus(QStringLiteral("Opened %1").arg(label));
}

namespace {

QString trimTrailingUrlPunctuation(QString url)
{
    static const QString tails = QStringLiteral(".,;:!?)]}>」』、。\"'");
    while (!url.isEmpty() && tails.contains(url.back()))
        url.chop(1);
    return url;
}

bool rangeContains(int start, int length, int cursor)
{
    return cursor >= start && cursor < start + length;
}

} // namespace

QVariantMap Backend::followableLinkAt(const QString &text, int cursor) const
{
    QVariantMap none{{QStringLiteral("kind"), QString()}};
    cursor = qBound(0, cursor, text.size());
    if (CodeBlocks::oddFencesBefore(text, cursor))
        return none;

    int lineStart = cursor == 0 ? 0 : text.lastIndexOf(QLatin1Char('\n'), cursor - 1) + 1;
    int lineEnd = text.indexOf(QLatin1Char('\n'), cursor);
    if (lineEnd < 0)
        lineEnd = text.size();
    const QString line = text.mid(lineStart, lineEnd - lineStart);
    const int local = cursor - lineStart;

    auto hitExternal = [](const QString &href) -> QVariantMap {
        return {{QStringLiteral("kind"), QStringLiteral("external")},
                {QStringLiteral("url"), QUrl(href)}};
    };
    auto hitNote = [](const QUrl &url, const QString &fragmentKind, const QString &fragment) {
        return QVariantMap{{QStringLiteral("kind"), QStringLiteral("note")},
                           {QStringLiteral("url"), url},
                           {QStringLiteral("fragmentKind"), fragmentKind},
                           {QStringLiteral("fragment"), fragment}};
    };

    static const QRegularExpression wikiRe(
        QStringLiteral(R"(!?\[\[([^\]\n]+)\]\])"));
    QRegularExpressionMatchIterator wikiIt = wikiRe.globalMatch(line);
    while (wikiIt.hasNext()) {
        const QRegularExpressionMatch match = wikiIt.next();
        if (!rangeContains(match.capturedStart(), match.capturedLength(), local))
            continue;
        const MindMap::Markup markup = MindMap::parseMarkup(match.captured());
        if (markup.kind == QLatin1String("image"))
            continue;
        QUrl resolved = resolveWikiTarget(markup.target, currentFilePath());
        if (!resolved.isValid() && markup.target.isEmpty() && !markup.fragment.isEmpty()
            && m_fileUrl.isLocalFile())
            resolved = m_fileUrl;
        if (!resolved.isValid())
            return none;
        const QString href = normalizedLinkUrl(resolved.toString());
        if (!href.isEmpty())
            return hitExternal(href);
        return hitNote(resolved, markup.fragmentKind, markup.fragment);
    }

    static const QRegularExpression mdRe(
        QStringLiteral(R"(\[[^\]]+\]\(((?:\\.|[^)\n])+)\))"));
    QRegularExpressionMatchIterator mdIt = mdRe.globalMatch(line);
    while (mdIt.hasNext()) {
        const QRegularExpressionMatch match = mdIt.next();
        if (!rangeContains(match.capturedStart(), match.capturedLength(), local))
            continue;
        QString dest = match.captured(1).trimmed();
        dest.replace(QStringLiteral("\\)"), QStringLiteral(")"));
        const QString href = normalizedLinkUrl(dest);
        if (!href.isEmpty())
            return hitExternal(href);
        const QUrl resolved = resolveWikiTarget(dest, currentFilePath());
        if (resolved.isLocalFile()
            && isMarkdownFileName(QFileInfo(resolved.toLocalFile()).fileName()))
            return hitNote(resolved, QString(), QString());
        return none;
    }

    static const QRegularExpression angleRe(
        QStringLiteral(R"(<(https?://[^>\s]+)>)"));
    QRegularExpressionMatchIterator angleIt = angleRe.globalMatch(line);
    while (angleIt.hasNext()) {
        const QRegularExpressionMatch match = angleIt.next();
        if (!rangeContains(match.capturedStart(), match.capturedLength(), local))
            continue;
        const QString href = normalizedLinkUrl(match.captured(1));
        if (!href.isEmpty())
            return hitExternal(href);
    }

    static const QRegularExpression bareRe(
        QStringLiteral(R"((?<![\w/])https?://[^\s<>\"'）)」']+)"));
    QRegularExpressionMatchIterator bareIt = bareRe.globalMatch(line);
    while (bareIt.hasNext()) {
        const QRegularExpressionMatch match = bareIt.next();
        QString raw = trimTrailingUrlPunctuation(match.captured());
        const int start = match.capturedStart();
        const int length = raw.size();
        if (!rangeContains(start, length, local))
            continue;
        const QString href = normalizedLinkUrl(raw);
        if (!href.isEmpty())
            return hitExternal(href);
    }

    return none;
}

QUrl Backend::preferredDialogFolderUrl() const {
    return QUrl::fromLocalFile(preferredDialogDirectoryPath());
}

QVariantMap Backend::windowGeometry() const {
    QSettings settings;
    return {{QStringLiteral("x"), settings.value(QStringLiteral("window/x"), -1)},
            {QStringLiteral("y"), settings.value(QStringLiteral("window/y"), -1)},
            {QStringLiteral("width"), settings.value(QStringLiteral("window/width"), 1280)},
            {QStringLiteral("height"), settings.value(QStringLiteral("window/height"), 820)},
            {QStringLiteral("maximized"), settings.value(QStringLiteral("window/maximized"), false)}};
}

void Backend::saveWindowGeometry(int x, int y, int width, int height, bool maximized) {
    QSettings settings;
    if (!maximized) {
        settings.setValue(QStringLiteral("window/x"), x);
        settings.setValue(QStringLiteral("window/y"), y);
        settings.setValue(QStringLiteral("window/width"), width);
        settings.setValue(QStringLiteral("window/height"), height);
    }
    settings.setValue(QStringLiteral("window/maximized"), maximized);
}

void Backend::loadDocumentText(const QString &text) {
    m_lastDocumentText = text;
    if (!m_document) {
        setWordCount(countWords(text));
        refreshDocumentOutline();
        updatePreviewDocument();
        scheduleLiveFolding();
        return;
    }

    m_loading = true;
    m_document->setPlainText(text);
    m_loading = false;

    applyDocumentTypography();
    m_wordCountTimer.stop();
    setWordCount(countWords(text));
    updatePreviewDocument();
    refreshDocumentOutline();
    scheduleLiveFolding();
}

void Backend::setFileUrl(const QUrl &url) {
    if (m_fileUrl == url)
        return;

    m_fileUrl = url;
    emit fileUrlChanged();
    watchCurrentFile();
    updatePreviewDocument();
}

void Backend::setModified(bool modified) {
    if (m_modified == modified)
        return;

    m_modified = modified;
    emit modifiedChanged();
}

void Backend::setStatus(const QString &status) {
    if (m_status == status)
        return;

    m_status = status;
    emit statusChanged();
}

void Backend::saveTo(const QUrl &url) {
    if (!url.isLocalFile()) {
        m_closeAfterSave = false;
        setStatus(QStringLiteral("Only local files can be saved."));
        return;
    }

    const QString targetName = QFileInfo(url.toLocalFile()).fileName();
    QSaveFile file(url.toLocalFile());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        m_closeAfterSave = false;
        setStatus(QStringLiteral("Could not save %1.").arg(targetName));
        return;
    }

    const QByteArray contents = currentDocumentText().toUtf8();
    file.write(contents);

    // QSaveFile commits by replacing the target. Stop watching the old inode
    // before that replacement so our own write is not classified as external.
    const QStringList watched = m_fileWatcher.files();
    if (!watched.isEmpty())
        m_fileWatcher.removePaths(watched);

    // commit() flushes, fsyncs, and atomically renames the temp file into place,
    // returning false (and leaving the original untouched) on any write error.
    if (!file.commit()) {
        watchCurrentFile();
        m_closeAfterSave = false;
        setStatus(QStringLiteral("Could not write %1.").arg(targetName));
        return;
    }

    const bool shouldClose = m_closeAfterSave;
    m_closeAfterSave = false;
    const QString routedPath =
        routeThreadsPostAfterSave(url.toLocalFile(), QString::fromUtf8(contents));
    const QUrl finalUrl = QUrl::fromLocalFile(routedPath);
    QByteArray finalContents = contents;
    if (routedPath != url.toLocalFile() || QFileInfo(routedPath).exists()) {
        QFile routedFile(routedPath);
        if (routedFile.open(QIODevice::ReadOnly | QIODevice::Text))
            finalContents = routedFile.readAll();
    }
    m_lastKnownFileContents = finalContents;
    m_hasKnownFileContents = true;
    if (finalContents != contents)
        loadDocumentText(QString::fromUtf8(finalContents));
    setFileUrl(finalUrl);
    watchCurrentFile();
    QSettings().setValue(lastSaveDirectorySetting,
                         QFileInfo(finalUrl.toLocalFile()).absolutePath());
    syncWorkspaceForFile(finalUrl);
    setModified(false);
    maybeAutoRenameUntitled();
    if (QDir::cleanPath(routedPath) != QDir::cleanPath(url.toLocalFile())
        && QDir::cleanPath(currentFilePath()) == QDir::cleanPath(routedPath)) {
        if (isPathUnderDirectory(routedPath, threadsPublishedFolder()))
            setStatus(QStringLiteral("已移至 published"));
        else
            setStatus(QStringLiteral("已移至 Threads 資料夾"));
    } else
        setStatus(QStringLiteral("Saved %1").arg(fileName()));
    clearRecovery();
    refreshProjectRecords();
    refreshNoteLinks();
    emit saveSucceeded();
    emit fileSaved(finalUrl.toLocalFile());

    if (shouldClose)
        emit closeAfterSave();
}

void Backend::scheduleRecovery() {
    m_recoveryTimer.start();
}

QString Backend::recoveryPath() const {
    return m_recoveryPath;
}

void Backend::writeRecovery() {
    if (!m_modified)
        return;
    const QString path = recoveryPath();
    if (path.isEmpty())
        return;
    QDir().mkpath(QFileInfo(path).absolutePath());
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
        return;
    const QJsonObject recovery{{QStringLiteral("fileUrl"), m_fileUrl.toString()},
                               {QStringLiteral("text"), currentDocumentText()}};
    file.write(QJsonDocument(recovery).toJson(QJsonDocument::Compact));
    file.commit();
}

void Backend::restoreRecovery() {
    QFile file(recoveryPath());
    if (!file.open(QIODevice::ReadOnly))
        return;
    const QJsonDocument json = QJsonDocument::fromJson(file.readAll());
    if (!json.isObject() || !json.object().contains(QStringLiteral("text")))
        return;
    const QJsonObject recovery = json.object();
    loadDocumentText(recovery.value(QStringLiteral("text")).toString());
    const QUrl recoveredUrl(recovery.value(QStringLiteral("fileUrl")).toString());
    QFile diskFile(recoveredUrl.toLocalFile());
    if (recoveredUrl.isLocalFile() && diskFile.open(QIODevice::ReadOnly)) {
        m_lastKnownFileContents = diskFile.readAll();
        m_hasKnownFileContents = true;
    } else {
        m_lastKnownFileContents.clear();
        m_hasKnownFileContents = false;
    }
    setFileUrl(recoveredUrl);
    syncWorkspaceForFile(recoveredUrl);
    setModified(true);
    setStatus(QStringLiteral("Recovered unsaved changes"));
}

void Backend::clearRecovery() {
    m_recoveryTimer.stop();
    QFile::remove(recoveryPath());
}

void Backend::watchCurrentFile() {
    const QStringList watched = m_fileWatcher.files();
    if (!watched.isEmpty())
        m_fileWatcher.removePaths(watched);
    if (m_fileUrl.isLocalFile() && QFileInfo::exists(m_fileUrl.toLocalFile()))
        m_fileWatcher.addPath(m_fileUrl.toLocalFile());
}

void Backend::setWorkspaceFolderUrl(const QUrl &url) {
    QUrl normalizedUrl;
    if (url.isLocalFile()) {
        const QString directoryPath = QDir(url.toLocalFile()).absolutePath();
        if (QDir(directoryPath).exists())
            normalizedUrl = QUrl::fromLocalFile(directoryPath);
    }

    const bool changed = m_workspaceFolderUrl != normalizedUrl;
    m_workspaceFolderUrl = normalizedUrl;
    if (changed) {
        loadStatusChoicesForWorkspace();
        emit workspaceFolderChanged();
    }

    refreshWorkspaceFiles();
    refreshTemplateFiles();
    refreshNoteLinks();
    updatePreviewDocument();
}

void Backend::refreshWorkspaceFiles() {
    QVariantList files;
    const QStringList markdownFiles = markdownFilesInDirectory(workspaceFolderPath());
    files.reserve(markdownFiles.size());

    const QString rootPath = workspaceFolderPath();
    for (const QString &path : markdownFiles) {
        const QFileInfo info(path);
        const QString relativeName = QDir::fromNativeSeparators(QDir(rootPath).relativeFilePath(path));
        bool threadsPost = false;
        QFile peek(path);
        if (peek.open(QIODevice::ReadOnly | QIODevice::Text)) {
            const QString head = QString::fromUtf8(peek.read(4096));
            threadsPost = FrontMatter::isThreadsPost(FrontMatter::parse(head).fields);
        }
        files.append(QVariantMap{{QStringLiteral("name"), relativeName},
                                 {QStringLiteral("url"), QUrl::fromLocalFile(info.absoluteFilePath())},
                                 {QStringLiteral("threads"), threadsPost}});
    }

    refreshProjectRecords();

    if (!m_tagFilter.isEmpty()) {
        QSet<QString> matching;
        for (const QVariant &item : m_projectRecords)
            matching.insert(item.toMap().value(QStringLiteral("path")).toString());
        QVariantList filtered;
        for (const QVariant &item : files) {
            const QVariantMap file = item.toMap();
            const QString path = QFileInfo(file.value(QStringLiteral("url")).toUrl().toLocalFile())
                                     .absoluteFilePath();
            if (matching.contains(path))
                filtered.append(item);
        }
        files = filtered;
    }

    if (m_workspaceFiles != files) {
        m_workspaceFiles = files;
        emit workspaceFilesChanged();
    }

    rebuildWorkspaceTree();
    refreshTemplateFiles();
}

namespace {

struct FsNode {
    bool folder = false;
    QString path;
    QString name;
    qint64 modified = 0;
    bool threads = false;
    QVector<FsNode> children;
};

QString normalizedFileSort(const QString &sort)
{
    if (sort == QLatin1String("name-desc") || sort == QLatin1String("mtime-desc")
        || sort == QLatin1String("mtime-asc"))
        return sort;
    return QStringLiteral("name-asc");
}

void sortFsNodes(QVector<FsNode> &nodes, const QString &sort)
{
    std::sort(nodes.begin(), nodes.end(), [&](const FsNode &left, const FsNode &right) {
        if (left.folder != right.folder)
            return left.folder;
        if (sort == QLatin1String("mtime-desc") && left.modified != right.modified)
            return left.modified > right.modified;
        if (sort == QLatin1String("mtime-asc") && left.modified != right.modified)
            return left.modified < right.modified;
        const int folded = QString::compare(left.name, right.name, Qt::CaseInsensitive);
        if (folded != 0)
            return sort == QLatin1String("name-desc") ? folded > 0 : folded < 0;
        return left.name < right.name;
    });
    for (FsNode &child : nodes)
        sortFsNodes(child.children, sort);
}

FsNode scanWorkspaceDir(const QString &dirPath, const QString &rootPath)
{
    FsNode node;
    node.folder = true;
    node.path = QDir(dirPath).absolutePath();
    const QFileInfo self(node.path);
    node.name = self.fileName();
    node.modified = self.lastModified().toMSecsSinceEpoch();

    const QDir dir(node.path);
    const QFileInfoList entries = dir.entryInfoList(
        QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Readable,
        QDir::Name | QDir::IgnoreCase | QDir::DirsFirst);
    for (const QFileInfo &entry : entries) {
        const QString path = entry.absoluteFilePath();
        if (shouldSkipWorkspacePath(rootPath, path))
            continue;
        if (entry.isDir()) {
            node.children.append(scanWorkspaceDir(path, rootPath));
            continue;
        }
        if (!isMarkdownFileName(entry.fileName()) || shouldSkipMarkdownFileName(entry.fileName()))
            continue;
        FsNode file;
        file.path = path;
        file.name = entry.fileName();
        file.modified = entry.lastModified().toMSecsSinceEpoch();
        QFile peek(path);
        if (peek.open(QIODevice::ReadOnly | QIODevice::Text)) {
            const QString head = QString::fromUtf8(peek.read(4096));
            file.threads = FrontMatter::isThreadsPost(FrontMatter::parse(head).fields);
        }
        node.children.append(file);
    }
    return node;
}

bool pruneFsNode(FsNode &node, const QSet<QString> &matching)
{
    if (!node.folder)
        return matching.contains(QFileInfo(node.path).absoluteFilePath());
    QVector<FsNode> kept;
    kept.reserve(node.children.size());
    for (FsNode &child : node.children) {
        if (pruneFsNode(child, matching))
            kept.append(child);
    }
    node.children = kept;
    return !kept.isEmpty();
}

void collectFolderPaths(const FsNode &node, QStringList *out)
{
    for (const FsNode &child : node.children) {
        if (!child.folder)
            continue;
        out->append(child.path);
        collectFolderPaths(child, out);
    }
}

void flattenFsNode(const FsNode &node, int depth, const QSet<QString> &collapsed,
                   QVariantList *out)
{
    for (const FsNode &child : node.children) {
        QVariantMap row{
            {QStringLiteral("kind"), child.folder ? QStringLiteral("folder")
                                                  : QStringLiteral("file")},
            {QStringLiteral("name"), child.name},
            {QStringLiteral("path"), child.path},
            {QStringLiteral("url"), QUrl::fromLocalFile(child.path)},
            {QStringLiteral("depth"), depth},
            {QStringLiteral("expanded"), child.folder && !collapsed.contains(child.path)},
            {QStringLiteral("childCount"), child.children.size()},
            {QStringLiteral("threads"), child.threads},
            {QStringLiteral("modified"), child.modified},
        };
        out->append(row);
        if (child.folder && !collapsed.contains(child.path))
            flattenFsNode(child, depth + 1, collapsed, out);
    }
}

const QStringList boardFieldCandidates() {
    return {QStringLiteral("status"), QStringLiteral("state"), QStringLiteral("stage"),
            QStringLiteral("kanban"), QStringLiteral("progress")};
}

const QStringList dateFieldCandidates() {
    return {QStringLiteral("date"), QStringLiteral("due"), QStringLiteral("due_date"),
            QStringLiteral("scheduled"), QStringLiteral("published"), QStringLiteral("deadline")};
}

int boardLaneRank(const QString &value) {
    static const QStringList order = {QStringLiteral("todo"), QStringLiteral("to-do"),
                                      QStringLiteral("draft"), QStringLiteral("writing"),
                                      QStringLiteral("in-progress"), QStringLiteral("in progress"),
                                      QStringLiteral("review"), QStringLiteral("submitted"),
                                      QStringLiteral("done"), QStringLiteral("published")};
    const int index = order.indexOf(value.trimmed().toLower());
    return index >= 0 ? index : 100 + qHash(value) % 50;
}

QString recordFieldString(const QVariantMap &record, const QString &key) {
    const QVariantMap fields = record.value(QStringLiteral("fields")).toMap();
    return FrontMatter::displayValue(fields.value(key));
}

} // namespace

void Backend::rebuildWorkspaceTree()
{
    const QString rootPath = workspaceFolderPath();
    QVariantList tree;
    QStringList folderPaths;
    if (!rootPath.isEmpty() && QDir(rootPath).exists()) {
        FsNode root = scanWorkspaceDir(rootPath, rootPath);
        sortFsNodes(root.children, normalizedFileSort(m_fileSort));
        if (!m_tagFilter.isEmpty()) {
            QSet<QString> matching;
            for (const QVariant &item : m_projectRecords)
                matching.insert(item.toMap().value(QStringLiteral("path")).toString());
            pruneFsNode(root, matching);
        }
        collectFolderPaths(root, &folderPaths);
        QSet<QString> collapsed;
        for (const QString &path : m_collapsedFolders)
            collapsed.insert(path);
        flattenFsNode(root, 0, collapsed, &tree);
    }
    const bool foldersChanged = folderPaths != m_workspaceFolderPaths;
    m_workspaceFolderPaths = folderPaths;
    if (tree != m_workspaceTree || foldersChanged) {
        m_workspaceTree = tree;
        emit workspaceTreeChanged();
        emit workspaceNavChanged();
    }
    watchWorkspaceDirectories();
}

void Backend::watchWorkspaceDirectories()
{
    QSet<QString> wanted;
    const QString root = workspaceFolderPath();
    if (!root.isEmpty() && QDir(root).exists())
        wanted.insert(QDir(root).absolutePath());
    for (const QString &path : m_workspaceFolderPaths) {
        const QString abs = QDir(path).absolutePath();
        if (QDir(abs).exists())
            wanted.insert(abs);
        if (wanted.size() >= 200)
            break;
    }
    const QStringList have = m_workspaceWatcher.directories();
    QSet<QString> haveSet(have.begin(), have.end());
    QStringList toRemove;
    for (const QString &path : haveSet) {
        if (!wanted.contains(path))
            toRemove.append(path);
    }
    QStringList toAdd;
    for (const QString &path : wanted) {
        if (!haveSet.contains(path))
            toAdd.append(path);
    }
    if (!toRemove.isEmpty())
        m_workspaceWatcher.removePaths(toRemove);
    if (!toAdd.isEmpty())
        m_workspaceWatcher.addPaths(toAdd);
}

void Backend::persistCollapsedFolders()
{
    QStringList existing;
    for (const QString &path : m_collapsedFolders) {
        if (QDir(path).exists())
            existing.append(QDir(path).absolutePath());
    }
    existing.removeDuplicates();
    m_collapsedFolders = existing;
    QSettings().setValue(collapsedFoldersSetting, m_collapsedFolders);
}

int Backend::workspaceIndexOfCurrentFile() const
{
    const QString path = QFileInfo(currentFilePath()).absoluteFilePath();
    if (path.isEmpty())
        return -1;
    const QVariantList files = workspaceNavFiles();
    for (int i = 0; i < files.size(); ++i) {
        if (QFileInfo(files.at(i).toMap().value(QStringLiteral("path")).toString())
                .absoluteFilePath() == path)
            return i;
    }
    return -1;
}

int Backend::expandAncestorsOfCurrentFile()
{
    const QString root = workspaceFolderPath();
    const QString filePath = currentFilePath();
    if (!filePath.isEmpty())
        setSelectedWorkspacePath(filePath);
    if (root.isEmpty() || filePath.isEmpty())
        return workspaceIndexOfCurrentFile();

    QString dir = QFileInfo(filePath).absolutePath();
    bool changed = false;
    while (!dir.isEmpty() && dir != QLatin1String("/")
           && isPathUnderDirectory(dir, root)) {
        const QString absolute = QDir(dir).absolutePath();
        if (m_collapsedFolders.removeAll(absolute) > 0)
            changed = true;
        const QString parent = QFileInfo(dir).absolutePath();
        if (parent == dir)
            break;
        dir = parent;
    }
    if (changed) {
        persistCollapsedFolders();
        rebuildWorkspaceTree();
    }
    return workspaceIndexOfCurrentFile();
}

QString Backend::createTargetDirectory() const
{
    const QString root = workspaceFolderPath();
    if (root.isEmpty())
        return {};
    if (!m_selectedWorkspacePath.isEmpty()) {
        const QFileInfo selected(m_selectedWorkspacePath);
        if (selected.isDir() && selected.exists()
            && isPathUnderDirectory(selected.absoluteFilePath(), root))
            return selected.absoluteFilePath();
        if (selected.isFile() && selected.exists()
            && isPathUnderDirectory(selected.absoluteFilePath(), root))
            return selected.absolutePath();
    }
    return root;
}

void Backend::setFileSort(const QString &sort)
{
    const QString normalized = normalizedFileSort(sort);
    if (m_fileSort == normalized)
        return;
    m_fileSort = normalized;
    QSettings().setValue(fileSortSetting, m_fileSort);
    emit fileSortChanged();
    rebuildWorkspaceTree();
}

void Backend::setSelectedWorkspacePath(const QString &path)
{
    QString next;
    if (!path.isEmpty() && QFileInfo::exists(path)) {
        const QString absolute = QFileInfo(path).absoluteFilePath();
        if (workspaceFolderPath().isEmpty()
            || isPathUnderDirectory(absolute, workspaceFolderPath())
            || QDir(absolute).absolutePath() == workspaceFolderPath())
            next = absolute;
    }
    if (m_selectedWorkspacePath == next)
        return;
    m_selectedWorkspacePath = next;
    emit selectedWorkspacePathChanged();
    emit workspaceNavChanged();
}

void Backend::toggleWorkspaceFolder(const QString &path)
{
    if (path.isEmpty() || !QDir(path).exists())
        return;
    const QString absolute = QDir(path).absolutePath();
    setSelectedWorkspacePath(absolute);
    if (m_collapsedFolders.contains(absolute))
        m_collapsedFolders.removeAll(absolute);
    else
        m_collapsedFolders.append(absolute);
    persistCollapsedFolders();
    rebuildWorkspaceTree();
}

bool Backend::workspaceFoldersCollapsed() const
{
    if (m_workspaceFolderPaths.isEmpty())
        return true;
    for (const QString &path : m_workspaceFolderPaths) {
        if (!m_collapsedFolders.contains(path))
            return false;
    }
    return true;
}

void Backend::collapseAllWorkspaceFolders()
{
    const QString root = workspaceFolderPath();
    if (root.isEmpty() || m_workspaceFolderPaths.isEmpty())
        return;
    QStringList next;
    for (const QString &path : m_collapsedFolders) {
        if (!isPathUnderDirectory(path, root))
            next.append(path);
    }
    for (const QString &path : m_workspaceFolderPaths) {
        if (!next.contains(path))
            next.append(path);
    }
    if (next == m_collapsedFolders)
        return;
    m_collapsedFolders = next;
    persistCollapsedFolders();
    rebuildWorkspaceTree();
}

void Backend::expandAllWorkspaceFolders()
{
    const QString root = workspaceFolderPath();
    if (root.isEmpty())
        return;
    QStringList next;
    for (const QString &path : m_collapsedFolders) {
        if (!isPathUnderDirectory(path, root))
            next.append(path);
    }
    if (next == m_collapsedFolders)
        return;
    m_collapsedFolders = next;
    persistCollapsedFolders();
    rebuildWorkspaceTree();
}

void Backend::toggleAllWorkspaceFolders()
{
    if (workspaceFoldersCollapsed())
        expandAllWorkspaceFolders();
    else
        collapseAllWorkspaceFolders();
}

QUrl Backend::createFolder(const QString &name)
{
    const QString parent = createTargetDirectory();
    if (parent.isEmpty() || !QDir(parent).exists()) {
        setStatus(QStringLiteral("Open a folder first, then create a folder."));
        emit openFolderDialogRequested();
        return {};
    }

    QString stem = name.trimmed();
    if (stem.isEmpty())
        stem = t(QStringLiteral("newFolder"));
    stem.replace(QRegularExpression(QStringLiteral("[\\\\/:*?\"<>|\\x00-\\x1f\\x7f]")),
                 QStringLiteral("-"));
    stem.replace(QRegularExpression(QStringLiteral("\\s+")), QStringLiteral(" "));
    stem = stem.left(80).trimmed();
    while (stem.startsWith(QLatin1Char('.')))
        stem.remove(0, 1);
    while (stem.endsWith(QLatin1Char('.')))
        stem.chop(1);
    stem = stem.trimmed();
    if (stem.isEmpty())
        stem = t(QStringLiteral("newFolder"));

    QString folderName = stem;
    int suffix = 2;
    while (QFileInfo::exists(QDir(parent).filePath(folderName)))
        folderName = stem + QLatin1Char(' ') + QString::number(suffix++);
    const QString path = QDir(parent).filePath(folderName);
    if (!QDir().mkpath(path)) {
        setStatus(QStringLiteral("Could not create %1.").arg(folderName));
        return {};
    }

    m_collapsedFolders.removeAll(QDir(parent).absolutePath());
    persistCollapsedFolders();
    setSelectedWorkspacePath(path);
    refreshWorkspaceFiles();
    setStatus(QStringLiteral("Created folder %1").arg(folderName));
    return QUrl::fromLocalFile(QDir(path).absolutePath());
}

bool Backend::renameFolder(const QUrl &url, const QString &newName)
{
    if (!url.isLocalFile())
        return false;
    const QFileInfo from(url.toLocalFile());
    if (!from.exists() || !from.isDir()) {
        setStatus(QStringLiteral("Could not rename that folder."));
        return false;
    }
    if (from.absoluteFilePath() == workspaceFolderPath()) {
        setStatus(QStringLiteral("Cannot rename the project root here."));
        return false;
    }

    QString stem = newName.trimmed();
    stem.replace(QRegularExpression(QStringLiteral("[\\\\/:*?\"<>|\\x00-\\x1f\\x7f]")),
                 QStringLiteral("-"));
    stem.replace(QRegularExpression(QStringLiteral("\\s+")), QStringLiteral(" "));
    stem = stem.left(80).trimmed();
    while (stem.startsWith(QLatin1Char('.')))
        stem.remove(0, 1);
    while (stem.endsWith(QLatin1Char('.')))
        stem.chop(1);
    stem = stem.trimmed();
    if (stem.isEmpty()) {
        setStatus(QStringLiteral("Invalid name."));
        return false;
    }

    const QString destination = QDir(from.absolutePath()).filePath(stem);
    if (QFileInfo(destination).absoluteFilePath() == from.absoluteFilePath())
        return true;
    if (QFileInfo::exists(destination)) {
        setStatus(QStringLiteral("%1 already exists.").arg(stem));
        return false;
    }
    if (!QDir().rename(from.absoluteFilePath(), destination)) {
        setStatus(QStringLiteral("Could not rename %1.").arg(from.fileName()));
        return false;
    }

    const QString oldPath = from.absoluteFilePath();
    const QString newPath = QFileInfo(destination).absoluteFilePath();
    if (m_selectedWorkspacePath == oldPath
        || m_selectedWorkspacePath.startsWith(oldPath + QLatin1Char('/')))
        m_selectedWorkspacePath.replace(0, oldPath.size(), newPath);
    for (QString &collapsed : m_collapsedFolders) {
        if (collapsed == oldPath || collapsed.startsWith(oldPath + QLatin1Char('/')))
            collapsed.replace(0, oldPath.size(), newPath);
    }
    persistCollapsedFolders();
    if (m_fileUrl.isLocalFile()) {
        const QString current = QFileInfo(m_fileUrl.toLocalFile()).absoluteFilePath();
        if (current == oldPath || current.startsWith(oldPath + QLatin1Char('/')))
            setFileUrl(QUrl::fromLocalFile(newPath + current.mid(oldPath.size())));
    }
    refreshWorkspaceFiles();
    emit selectedWorkspacePathChanged();
    setStatus(QStringLiteral("Renamed to %1").arg(stem));
    return true;
}

void Backend::refreshProjectRecords() {
    QVariantList records;
    QStringList fieldNames;
    QMap<QString, int> tagCounts;
    QMap<QString, QString> tagNames;
    const QString rootPath = workspaceFolderPath();
    const QStringList markdownFiles = markdownFilesInDirectory(rootPath);
    const QString filter = m_tagFilter;

    for (const QString &path : markdownFiles) {
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            continue;

        const QString text = QString::fromUtf8(file.readAll());
        const FrontMatter::Document document = FrontMatter::parse(text);
        const QFileInfo info(path);
        const QString relativeName =
            QDir::fromNativeSeparators(QDir(rootPath).relativeFilePath(path));
        const QString title =
            FrontMatter::recordTitle(document.fields, document.body, info.completeBaseName());
        const QStringList tags = FrontMatter::allTags(text);

        QVariantMap fieldsDisplay;
        for (auto it = document.fields.constBegin(); it != document.fields.constEnd(); ++it) {
            if (!fieldNames.contains(it.key()))
                fieldNames.append(it.key());
            fieldsDisplay.insert(it.key(), FrontMatter::displayValue(it.value()));
        }

        for (const QString &tag : tags) {
            const QString key = tag.toLower();
            tagCounts[key] += 1;
            if (!tagNames.contains(key))
                tagNames.insert(key, tag);
        }

        if (!filter.isEmpty()) {
            bool matches = false;
            for (const QString &tag : tags) {
                if (tag.compare(filter, Qt::CaseInsensitive) == 0
                    || tag.startsWith(filter + QLatin1Char('/'), Qt::CaseInsensitive)) {
                    matches = true;
                    break;
                }
            }
            if (!matches)
                continue;
        }

        if (!m_projectKeywordFilter.trimmed().isEmpty()) {
            const QString needle = m_projectKeywordFilter.trimmed();
            bool hit = title.contains(needle, Qt::CaseInsensitive)
                || relativeName.contains(needle, Qt::CaseInsensitive)
                || text.contains(needle, Qt::CaseInsensitive);
            if (!hit) {
                for (const QString &tag : tags) {
                    if (tag.contains(needle, Qt::CaseInsensitive)) {
                        hit = true;
                        break;
                    }
                }
            }
            if (!hit)
                continue;
        }

        const QString dateValue = FrontMatter::isoDate(
            fieldsDisplay.value(resolvedDateField()).toString());
        if (!recordMatchesDateFilter(dateValue))
            continue;
        if (!recordMatchesPublishFilter(document.fields, path))
            continue;

        QString cover = FrontMatter::displayValue(document.fields.value(QStringLiteral("cover")));
        if (cover.isEmpty())
            cover = FrontMatter::displayValue(document.fields.value(QStringLiteral("image")));
        if (cover.isEmpty())
            cover = FrontMatter::displayValue(document.fields.value(QStringLiteral("banner")));
        if (cover.isEmpty()) {
            const QStringList media = relativeMediaPaths(document.body);
            if (!media.isEmpty())
                cover = media.first();
        }
        QString coverUrl;
        if (!cover.isEmpty() && !cover.startsWith(QLatin1String("http"))) {
            const QString resolved = QFileInfo(info.dir().filePath(cover)).absoluteFilePath();
            if (QFileInfo::exists(resolved))
                coverUrl = QUrl::fromLocalFile(resolved).toString();
        } else if (cover.startsWith(QLatin1String("http"))) {
            coverUrl = cover;
        }

        QString snippet;
        for (const QString &line : document.body.split(QLatin1Char('\n'))) {
            const QString trimmed = line.trimmed();
            if (trimmed.isEmpty() || trimmed.startsWith(QLatin1Char('#'))
                    || trimmed.startsWith(QLatin1String("---"))
                    || trimmed.startsWith(QLatin1String("![")))
                continue;
            snippet = trimmed;
            snippet.remove(QRegularExpression(QStringLiteral("[*_`>#\\[\\]]")));
            if (snippet.size() > 140)
                snippet = snippet.left(139) + QChar(0x2026);
            break;
        }

        const bool pinned = FrontMatter::isTruthy(document.fields.value(QStringLiteral("pin")))
            || FrontMatter::isTruthy(document.fields.value(QStringLiteral("shortcut")));

        records.append(QVariantMap{
            {QStringLiteral("url"), QUrl::fromLocalFile(info.absoluteFilePath())},
            {QStringLiteral("path"), info.absoluteFilePath()},
            {QStringLiteral("name"), relativeName},
            {QStringLiteral("title"), title},
            {QStringLiteral("fields"), fieldsDisplay},
            {QStringLiteral("tags"), tags},
            {QStringLiteral("cover"), coverUrl},
            {QStringLiteral("snippet"), snippet},
            {QStringLiteral("pinned"), pinned},
            {QStringLiteral("folder"), info.absolutePath()},
            {QStringLiteral("openTasks"), FrontMatter::openTaskCount(document.body)},
            {QStringLiteral("doneTasks"), FrontMatter::closedTaskCount(document.body)},
        });
    }

    fieldNames.removeAll(QStringLiteral("title"));
    std::sort(fieldNames.begin(), fieldNames.end());

    QVariantList tags;
    QStringList orderedKeys = tagCounts.keys();
    std::sort(orderedKeys.begin(), orderedKeys.end());
    for (const QString &key : orderedKeys) {
        tags.append(QVariantMap{{QStringLiteral("name"), tagNames.value(key)},
                                {QStringLiteral("count"), tagCounts.value(key)}});
    }
    const bool tagsChanged = m_workspaceTags != tags;
    m_workspaceTags = tags;
    if (tagsChanged)
        emit workspaceTagsChanged();

    const bool recordsChanged = m_projectRecords != records || m_projectFieldNames != fieldNames;
    m_projectRecords = records;
    m_projectFieldNames = fieldNames;
    if (recordsChanged) {
        emit projectRecordsChanged();
        emit calendarChanged();
        emit workspaceNavChanged();
    }
}

QString Backend::selectedFolderPath() const
{
    QString folder = m_selectedWorkspacePath;
    if (folder.isEmpty())
        return workspaceFolderPath();
    const QFileInfo info(folder);
    if (info.isFile())
        return info.absolutePath();
    if (info.isDir())
        return info.absoluteFilePath();
    return workspaceFolderPath();
}

QVariantList Backend::cardRecords() const
{
    QVariantList out;
    const QString folder = selectedFolderPath();
    const QString root = workspaceFolderPath();
    const bool limitFolder = !folder.isEmpty() && QDir(folder).absolutePath() != QDir(root).absolutePath();
    for (const QVariant &item : m_projectRecords) {
        const QVariantMap record = item.toMap();
        if (limitFolder) {
            const QString path = record.value(QStringLiteral("path")).toString();
            if (!isPathUnderDirectory(path, folder) && QFileInfo(path).absolutePath() != folder)
                continue;
        }
        bool tagsOk = true;
        const QStringList tags = record.value(QStringLiteral("tags")).toStringList();
        for (const QString &need : m_cardTagFilters) {
            bool hit = false;
            for (const QString &tag : tags) {
                if (tag.compare(need, Qt::CaseInsensitive) == 0) {
                    hit = true;
                    break;
                }
            }
            if (!hit) {
                tagsOk = false;
                break;
            }
        }
        if (!tagsOk)
            continue;
        out.append(record);
    }
    std::stable_sort(out.begin(), out.end(), [](const QVariant &left, const QVariant &right) {
        const bool a = left.toMap().value(QStringLiteral("pinned")).toBool();
        const bool b = right.toMap().value(QStringLiteral("pinned")).toBool();
        if (a != b)
            return a;
        return left.toMap().value(QStringLiteral("title")).toString()
            < right.toMap().value(QStringLiteral("title")).toString();
    });
    return out;
}

void Backend::toggleCardTag(const QString &tag)
{
    const QString trimmed = tag.trimmed();
    if (trimmed.isEmpty())
        return;
    bool removed = false;
    for (int i = m_cardTagFilters.size() - 1; i >= 0; --i) {
        if (m_cardTagFilters.at(i).compare(trimmed, Qt::CaseInsensitive) == 0) {
            m_cardTagFilters.removeAt(i);
            removed = true;
        }
    }
    if (!removed)
        m_cardTagFilters.append(trimmed);
    emit cardFiltersChanged();
    emit projectRecordsChanged();
}

void Backend::clearCardTags()
{
    if (m_cardTagFilters.isEmpty())
        return;
    m_cardTagFilters.clear();
    emit cardFiltersChanged();
    emit projectRecordsChanged();
}

QUrl Backend::randomCardUrl() const
{
    const QVariantList cards = cardRecords();
    if (cards.isEmpty())
        return {};
    const int index = QRandomGenerator::global()->bounded(cards.size());
    return cards.at(index).toMap().value(QStringLiteral("url")).toUrl();
}

void Backend::togglePin(const QUrl &url)
{
    if (!url.isLocalFile())
        return;
    const QString text = noteTextFor(url);
    if (text.isEmpty())
        return;
    const FrontMatter::Document document = FrontMatter::parse(text);
    const bool pinned = FrontMatter::isTruthy(document.fields.value(QStringLiteral("pin")))
        || FrontMatter::isTruthy(document.fields.value(QStringLiteral("shortcut")));
    const QString updated =
        FrontMatter::setField(text, QStringLiteral("pin"),
                              pinned ? QStringLiteral("false") : QStringLiteral("true"));
    applyNoteText(url, updated, true);
}

QVariantList Backend::activityHeatmap() const
{
    QMap<QString, int> counts;
    for (const QVariant &item : m_projectRecords) {
        const QString iso = FrontMatter::isoDate(
            recordFieldString(item.toMap(), resolvedDateField()));
        if (!iso.isEmpty())
            counts[iso] += 1;
    }
    QVariantList days;
    const QDate today = QDate::currentDate();
    const QDate start = today.addDays(-(16 * 7 - 1));
    for (QDate d = start; d <= today; d = d.addDays(1)) {
        const QString iso = d.toString(Qt::ISODate);
        const int count = counts.value(iso);
        int level = 0;
        if (count >= 4)
            level = 4;
        else if (count >= 3)
            level = 3;
        else if (count >= 2)
            level = 2;
        else if (count >= 1)
            level = 1;
        days.append(QVariantMap{
            {QStringLiteral("date"), iso},
            {QStringLiteral("count"), count},
            {QStringLiteral("level"), level},
        });
    }
    return days;
}

QVariantList Backend::workspaceNavFolders() const
{
    QVariantList folders;
    const QString root = workspaceFolderPath();
    if (root.isEmpty())
        return folders;
    const QString rootAbs = QDir(root).absolutePath();
    folders.append(QVariantMap{
        {QStringLiteral("kind"), QStringLiteral("folder")},
        {QStringLiteral("name"), workspaceFolderName()},
        {QStringLiteral("path"), rootAbs},
        {QStringLiteral("url"), QUrl::fromLocalFile(rootAbs)},
        {QStringLiteral("depth"), 0},
        {QStringLiteral("root"), true},
    });
    QStringList paths = m_workspaceFolderPaths;
    std::sort(paths.begin(), paths.end(), [](const QString &left, const QString &right) {
        return QString::compare(left, right, Qt::CaseInsensitive) < 0;
    });
    for (const QString &path : paths) {
        const QString abs = QDir(path).absolutePath();
        if (abs == rootAbs)
            continue;
        const QString rel = QDir::fromNativeSeparators(QDir(rootAbs).relativeFilePath(abs));
        if (rel.startsWith(QLatin1String("..")) || rel.isEmpty())
            continue;
        folders.append(QVariantMap{
            {QStringLiteral("kind"), QStringLiteral("folder")},
            {QStringLiteral("name"), QFileInfo(abs).fileName()},
            {QStringLiteral("path"), abs},
            {QStringLiteral("url"), QUrl::fromLocalFile(abs)},
            {QStringLiteral("depth"), rel.count(QLatin1Char('/')) + 1},
            {QStringLiteral("root"), false},
        });
    }
    return folders;
}

QVariantList Backend::workspaceNavFiles() const
{
    const QString folder = QDir(selectedFolderPath()).absolutePath();
    if (folder.isEmpty() || !QDir(folder).exists())
        return {};

    QHash<QString, QVariantMap> byPath;
    for (const QVariant &item : m_projectRecords) {
        const QVariantMap rec = item.toMap();
        byPath.insert(QFileInfo(rec.value(QStringLiteral("path")).toString()).absoluteFilePath(),
                      rec);
    }

    QDir::SortFlags flags = QDir::Name | QDir::IgnoreCase;
    if (m_fileSort == QLatin1String("name-desc"))
        flags = QDir::Name | QDir::IgnoreCase | QDir::Reversed;
    else if (m_fileSort == QLatin1String("mtime-desc"))
        flags = QDir::Time;
    else if (m_fileSort == QLatin1String("mtime-asc"))
        flags = QDir::Time | QDir::Reversed;

    QVariantList files;
    const QString root = workspaceFolderPath();
    const QFileInfoList dirs = QDir(folder).entryInfoList(
        QDir::Dirs | QDir::NoDotAndDotDot | QDir::Readable, flags);
    for (const QFileInfo &info : dirs) {
        const QString abs = info.absoluteFilePath();
        if (!root.isEmpty() && shouldSkipWorkspacePath(root, abs))
            continue;
        files.append(QVariantMap{
            {QStringLiteral("kind"), QStringLiteral("folder")},
            {QStringLiteral("name"), info.fileName()},
            {QStringLiteral("path"), abs},
            {QStringLiteral("url"), QUrl::fromLocalFile(abs)},
            {QStringLiteral("depth"), 0},
            {QStringLiteral("modified"), info.lastModified().toMSecsSinceEpoch()},
        });
    }

    const QFileInfoList entries = QDir(folder).entryInfoList(
        QStringList{QStringLiteral("*.md"), QStringLiteral("*.markdown")}, QDir::Files, flags);

    for (const QFileInfo &info : entries) {
        const QString abs = info.absoluteFilePath();
        if (!m_tagFilter.isEmpty() && !byPath.contains(abs))
            continue;
        QVariantMap row{
            {QStringLiteral("kind"), QStringLiteral("file")},
            {QStringLiteral("name"), info.fileName()},
            {QStringLiteral("path"), abs},
            {QStringLiteral("url"), QUrl::fromLocalFile(abs)},
            {QStringLiteral("depth"), 0},
            {QStringLiteral("modified"), info.lastModified().toMSecsSinceEpoch()},
        };
        const QVariantMap rec = byPath.value(abs);
        if (!rec.isEmpty()) {
            row.insert(QStringLiteral("cover"), rec.value(QStringLiteral("cover")));
            row.insert(QStringLiteral("pinned"), rec.value(QStringLiteral("pinned")));
            row.insert(QStringLiteral("title"), rec.value(QStringLiteral("title")));
            row.insert(QStringLiteral("threads"),
                       FrontMatter::isTruthy(rec.value(QStringLiteral("fields")).toMap()
                                                 .value(QStringLiteral("threads"))));
        }
        files.append(row);
    }
    return files;
}

QVariantList Backend::workspaceShortcuts() const
{
    QVariantList pins;
    for (const QVariant &item : m_projectRecords) {
        if (item.toMap().value(QStringLiteral("pinned")).toBool())
            pins.append(item);
        if (pins.size() >= 12)
            break;
    }
    return pins;
}

bool Backend::recordMatchesDateFilter(const QString &isoDate) const {
    if (m_projectDateFilter.isEmpty() || m_projectDateFilter == QLatin1String("all"))
        return true;
    const QDate date = QDate::fromString(isoDate, Qt::ISODate);
    const QDate today = QDate::currentDate();
    if (m_projectDateFilter == QLatin1String("dated"))
        return date.isValid();
    if (m_projectDateFilter == QLatin1String("undated"))
        return !date.isValid();
    if (!date.isValid())
        return false;
    if (m_projectDateFilter == QLatin1String("today"))
        return date == today;
    if (m_projectDateFilter == QLatin1String("yesterday"))
        return date == today.addDays(-1);
    if (m_projectDateFilter == QLatin1String("week"))
        return date >= today.addDays(-7) && date <= today;
    const QDate exact = QDate::fromString(m_projectDateFilter, Qt::ISODate);
    if (exact.isValid())
        return date == exact;
    return true;
}

bool Backend::recordMatchesPublishFilter(const QVariantMap &fields, const QString &path) const {
    if (m_projectPublishFilter.isEmpty() || m_projectPublishFilter == QLatin1String("all"))
        return true;
    const bool published = threadsPostIsPublished(fields)
        || FrontMatter::isTruthy(fields.value(QStringLiteral("publish")))
        || (!threadsPublishedFolder().isEmpty()
            && isPathUnderDirectory(path, threadsPublishedFolder()));
    if (m_projectPublishFilter == QLatin1String("published"))
        return published;
    if (m_projectPublishFilter == QLatin1String("drafts"))
        return !published;
    return true;
}

QString Backend::resolvedBoardField() const {
    if (!m_boardField.isEmpty())
        return m_boardField;
    for (const QString &candidate : boardFieldCandidates()) {
        if (m_projectFieldNames.contains(candidate))
            return candidate;
    }
    return QStringLiteral("status");
}

QString Backend::resolvedDateField() const {
    if (!m_dateField.isEmpty())
        return m_dateField;
    for (const QString &candidate : dateFieldCandidates()) {
        if (m_projectFieldNames.contains(candidate))
            return candidate;
    }
    return QStringLiteral("date");
}

QString Backend::boardField() const {
    return resolvedBoardField();
}

QString Backend::dateField() const {
    return resolvedDateField();
}

QStringList Backend::projectTableColumns() const {
    QStringList columns{QStringLiteral("File"), QStringLiteral("Title"), resolvedBoardField(),
                        resolvedDateField()};
    for (const QString &field : m_projectFieldNames) {
        if (columns.contains(field) || field == QLatin1String("title"))
            continue;
        columns.append(field);
        if (columns.size() >= 8)
            break;
    }
    return columns;
}

QStringList Backend::statusChoices() const {
    QStringList choices = m_statusChoices;
    const QString field = resolvedBoardField();
    for (const QVariant &item : m_projectRecords) {
        const QString value = recordFieldString(item.toMap(), field).trimmed();
        if (!value.isEmpty() && !choices.contains(value, Qt::CaseInsensitive))
            choices.append(value);
    }
    return choices;
}

void Backend::addStatusChoice(const QString &status) {
    const QString trimmed = status.trimmed();
    if (trimmed.isEmpty())
        return;
    for (const QString &existing : m_statusChoices) {
        if (existing.compare(trimmed, Qt::CaseInsensitive) == 0)
            return;
    }
    m_statusChoices.append(trimmed);
    saveStatusChoicesForWorkspace();
    emit projectRecordsChanged();
}

void Backend::renameStatus(const QString &from, const QString &to) {
    const QString source = from.trimmed();
    const QString target = to.trimmed();
    if (source.isEmpty() || target.isEmpty() || source.compare(target, Qt::CaseInsensitive) == 0)
        return;

    for (int i = 0; i < m_statusChoices.size(); ++i) {
        if (m_statusChoices.at(i).compare(source, Qt::CaseInsensitive) == 0)
            m_statusChoices[i] = target;
    }
    if (!m_statusChoices.contains(target, Qt::CaseInsensitive))
        m_statusChoices.append(target);
    saveStatusChoicesForWorkspace();

    const QString field = resolvedBoardField();
    const QVariantList snapshot = m_projectRecords;
    for (const QVariant &item : snapshot) {
        const QVariantMap record = item.toMap();
        if (recordFieldString(record, field).compare(source, Qt::CaseInsensitive) == 0)
            setRecordField(record.value(QStringLiteral("url")).toUrl(), field, target);
    }
    emit projectRecordsChanged();
}

void Backend::removeStatusChoice(const QString &status) {
    const QString trimmed = status.trimmed();
    if (trimmed.isEmpty())
        return;

    m_statusChoices.erase(std::remove_if(m_statusChoices.begin(), m_statusChoices.end(),
                                         [&](const QString &item) {
                                             return item.compare(trimmed, Qt::CaseInsensitive) == 0;
                                         }),
                          m_statusChoices.end());
    saveStatusChoicesForWorkspace();

    const QString field = resolvedBoardField();
    const QVariantList snapshot = m_projectRecords;
    for (const QVariant &item : snapshot) {
        const QVariantMap record = item.toMap();
        if (recordFieldString(record, field).compare(trimmed, Qt::CaseInsensitive) == 0)
            setRecordField(record.value(QStringLiteral("url")).toUrl(), field, QString());
    }
    emit projectRecordsChanged();
}

void Backend::setTagFilter(const QString &tag) {
    const QString name = FrontMatter::normalizeTag(tag);
    const QString next = name.compare(m_tagFilter, Qt::CaseInsensitive) == 0 ? QString() : name;
    if (next == m_tagFilter)
        return;
    m_tagFilter = next;
    emit tagFilterChanged();
    refreshWorkspaceFiles();
}

void Backend::insertTag(const QString &tag) {
    const QString name = FrontMatter::normalizeTag(tag);
    if (name.isEmpty())
        return;

    const QUrl url = m_fileUrl;
    const QString original = noteTextFor(url);
    if (original.isEmpty() && !m_document) {
        setStatus(QStringLiteral("Open a note to add a tag."));
        return;
    }

    const QString updated = FrontMatter::addTag(original, name);
    if (updated == original) {
        setStatus(QStringLiteral("Already tagged #%1").arg(name));
        return;
    }

    applyNoteText(url, updated, true);
    setStatus(QStringLiteral("Tagged #%1").arg(name));
}

void Backend::renameTag(const QString &from, const QString &to) {
    const QString source = FrontMatter::normalizeTag(from);
    const QString target = FrontMatter::normalizeTag(to);
    if (source.isEmpty() || target.isEmpty()
        || source.compare(target, Qt::CaseInsensitive) == 0)
        return;

    QStringList paths = markdownFilesInDirectory(workspaceFolderPath());
    if (m_fileUrl.isLocalFile()) {
        const QString current = QFileInfo(m_fileUrl.toLocalFile()).absoluteFilePath();
        if (!paths.contains(current))
            paths.prepend(current);
    }

    int changed = 0;
    for (const QString &path : paths) {
        const QUrl url = QUrl::fromLocalFile(path);
        const QString original = noteTextFor(url);
        const QString updated = FrontMatter::rewriteTags(original, source, target);
        if (updated == original)
            continue;
        applyNoteText(url, updated, false);
        ++changed;
    }

    refreshWorkspaceFiles();
    if (changed == 0)
        setStatus(QStringLiteral("No notes used #%1").arg(source));
    else
        setStatus(QStringLiteral("Renamed #%1 to #%2 in %3 notes")
                      .arg(source, target, QString::number(changed)));
}

QStringList Backend::tagsFromText(const QString &text) const {
    return FrontMatter::allTags(text);
}

void Backend::setOutlineMaxLevel(int level) {
    level = qBound(1, level, 6);
    if (m_outlineMaxLevel == level)
        return;
    m_outlineMaxLevel = level;
    QSettings().setValue(outlineMaxLevelSetting, level);
    emit outlineMaxLevelChanged();
}

void Backend::copyOutlineHeadings() {
    QStringList lines;
    for (const QVariant &item : m_documentOutline) {
        const QVariantMap heading = item.toMap();
        const int level = qBound(1, heading.value(QStringLiteral("level")).toInt(), 6);
        lines.append(QString(level, QLatin1Char('#')) + QLatin1Char(' ')
                     + heading.value(QStringLiteral("title")).toString());
    }
    if (auto *clipboard = QGuiApplication::clipboard())
        clipboard->setText(lines.join(QLatin1Char('\n')));
    setStatus(lines.isEmpty() ? QStringLiteral("No headings to copy")
                              : QStringLiteral("Copied %1 headings").arg(lines.size()));
}

void Backend::refreshDocumentOutline() {
    const QString text = m_document ? m_document->toPlainText() : m_lastDocumentText;
    const QVariantList outline = FrontMatter::headingOutline(text);
    if (outline != m_documentOutline) {
        m_documentOutline = outline;
        emit outlineChanged();
    }
    refreshMindmap();
}

void Backend::refreshMindmap()
{
    const QString text = m_document ? m_document->toPlainText() : m_lastDocumentText;
    const MindMap::Tree tree = MindMap::parse(text);
    const QStringList collapsed = MindMap::collapsedIds(text);
    m_mindmapLayout = MindMap::layout(tree, m_textScale, collapsed);
    enrichMindmapLayout();
    m_mindmapOutline = MindMap::flatten(tree.root, 0, collapsed);
    if (m_mindmapSelectedId.isEmpty() || !MindMap::findNode(tree.root, m_mindmapSelectedId))
        m_mindmapSelectedId = tree.root.id;
    ++m_mindmapStamp;
    emit mindmapChanged();
}

void Backend::commitMindmap(const QString &selectedId)
{
    m_mindmapSelectedId = selectedId;
}

void Backend::commitMindmapText(const QString &text, const QString &selectedId)
{
    m_mindmapSelectedId = selectedId;
    if (!m_document) {
        setEditorPlainText(text);
        return;
    }
    if (m_document->toPlainText() == text) {
        refreshMindmap();
        return;
    }

    m_loading = true;
    QTextCursor cursor(m_document);
    cursor.beginEditBlock();
    cursor.select(QTextCursor::Document);
    cursor.insertText(text);
    QTextBlockFormat blockFormat;
    blockFormat.setLineHeight(typoraLineHeightPercent, QTextBlockFormat::ProportionalHeight);
    cursor.select(QTextCursor::Document);
    cursor.mergeBlockFormat(blockFormat);
    cursor.endEditBlock();
    m_loading = false;

    m_lastDocumentText = text;
    m_formattedBlockCount = m_document->blockCount();
    setModified(true);
    setStatus(QStringLiteral("Unsaved"));
    scheduleWordCount();
    schedulePreviewUpdate();
    refreshDocumentOutline();
}

void Backend::setMindmapSelectedId(const QString &id)
{
    if (m_mindmapSelectedId == id)
        return;
    m_mindmapSelectedId = id;
    emit mindmapChanged();
}

void Backend::mindmapRename(const QString &id, const QString &title)
{
    const QString source = editorPlainText();
    MindMap::Tree tree = MindMap::parse(source);
    MindMap::Node *node = MindMap::findNode(tree.root, id);
    if (!node)
        return;
    const QString keptColor = MindMap::nodeColor(node->title);
    QString nextTitle = title;
    if (MindMap::nodeColor(nextTitle).isEmpty() && !keptColor.isEmpty())
        nextTitle = MindMap::applyNodeColor(nextTitle, keptColor);
    const QString nextId = MindMap::rename(tree.root, id, nextTitle);
    if (nextId.isEmpty())
        return;
    commitMindmapText(MindMap::serialize(source, tree), nextId);
}

void Backend::mindmapAddChild(const QString &id)
{
    const QString source = editorPlainText();
    MindMap::Tree tree = MindMap::parse(source);
    const QString nextId = MindMap::addChild(tree.root, id, t(QStringLiteral("newNode")));
    if (nextId.isEmpty())
        return;
    m_mindmapSelectedId = nextId;
    commitMindmapText(MindMap::serialize(source, tree), nextId);
}

void Backend::mindmapAddSibling(const QString &id)
{
    const QString source = editorPlainText();
    MindMap::Tree tree = MindMap::parse(source);
    const QString nextId = MindMap::addSibling(tree.root, id, t(QStringLiteral("newNode")));
    if (nextId.isEmpty())
        return;
    m_mindmapSelectedId = nextId;
    commitMindmapText(MindMap::serialize(source, tree), nextId);
}

void Backend::mindmapRemove(const QString &id)
{
    const QString source = editorPlainText();
    MindMap::Tree tree = MindMap::parse(source);
    const QString nextId = MindMap::removeNode(tree.root, id);
    if (nextId.isEmpty())
        return;
    m_mindmapSelectedId = nextId;
    commitMindmapText(MindMap::serialize(source, tree), nextId);
}

void Backend::mindmapReparent(const QString &id, const QString &parentId, int index)
{
    const QString source = editorPlainText();
    MindMap::Tree tree = MindMap::parse(source);
    const QString nextId = MindMap::reparent(tree.root, id, parentId, index);
    if (nextId.isEmpty())
        return;
    m_mindmapSelectedId = nextId;
    commitMindmapText(MindMap::serialize(source, tree), nextId);
}

void Backend::mindmapIndent(const QString &id)
{
    const QString source = editorPlainText();
    MindMap::Tree tree = MindMap::parse(source);
    const QString nextId = MindMap::indent(tree.root, id);
    if (nextId.isEmpty())
        return;
    m_mindmapSelectedId = nextId;
    commitMindmapText(MindMap::serialize(source, tree), nextId);
}

void Backend::mindmapOutdent(const QString &id)
{
    const QString source = editorPlainText();
    MindMap::Tree tree = MindMap::parse(source);
    const QString nextId = MindMap::outdent(tree.root, id);
    if (nextId.isEmpty())
        return;
    m_mindmapSelectedId = nextId;
    commitMindmapText(MindMap::serialize(source, tree), nextId);
}

QVariantList Backend::mindmapPalette() const
{
    return {
        QString(),
        QStringLiteral("#E57373"),
        QStringLiteral("#FFB74D"),
        QStringLiteral("#81C784"),
        QStringLiteral("#64B5F6"),
        QStringLiteral("#BA68C8"),
        QStringLiteral("#F06292"),
        QStringLiteral("#4DB6AC"),
    };
}

void Backend::mindmapSetCollapsed(const QString &id, bool collapsed)
{
    const QString source = editorPlainText();
    MindMap::Tree tree = MindMap::parse(source);
    const MindMap::Node *node = MindMap::findNode(tree.root, id);
    if (!node || node->children.isEmpty())
        return;
    QStringList ids = MindMap::collapsedIds(source);
    ids.removeAll(id);
    if (collapsed)
        ids.append(id);
    QString next = MindMap::serialize(source, tree);
    if (ids.isEmpty())
        next = FrontMatter::removeField(next, QStringLiteral("collapsed"));
    else
        next = FrontMatter::setField(next, QStringLiteral("collapsed"), ids.join(QLatin1Char('|')));
    commitMindmapText(next, id);
}

void Backend::mindmapToggleCollapse(const QString &id)
{
    const QStringList ids = MindMap::collapsedIds(editorPlainText());
    mindmapSetCollapsed(id, !ids.contains(id));
}

void Backend::mindmapSetColor(const QString &id, const QString &color)
{
    const QString source = editorPlainText();
    MindMap::Tree tree = MindMap::parse(source);
    MindMap::Node *node = MindMap::findNode(tree.root, id);
    if (!node)
        return;
    node->title = MindMap::applyNodeColor(node->title, color);
    commitMindmapText(MindMap::serialize(source, tree), id);
}

void Backend::enrichMindmapLayout()
{
    QVariantList nodes = m_mindmapLayout.value(QStringLiteral("nodes")).toList();
    for (int i = 0; i < nodes.size(); ++i) {
        QVariantMap node = nodes.at(i).toMap();
        QUrl resolved = resolveNodeTarget(node.value(QStringLiteral("target")).toString());
        if (!resolved.isValid()
            && node.value(QStringLiteral("target")).toString().isEmpty()
            && !node.value(QStringLiteral("fragment")).toString().isEmpty()
            && m_fileUrl.isLocalFile())
            resolved = m_fileUrl;
        node.insert(QStringLiteral("resolved"), resolved.toString());
        const bool image = node.value(QStringLiteral("kind")).toString() == QLatin1String("image")
            || (node.value(QStringLiteral("embed")).toBool()
                && MindMap::looksLikeImageTarget(node.value(QStringLiteral("target")).toString()));
        node.insert(QStringLiteral("isImage"),
                    image && resolved.isLocalFile()
                        && isSupportedImageFilePath(resolved.toLocalFile()));
        nodes[i] = node;
    }
    m_mindmapLayout.insert(QStringLiteral("nodes"), nodes);
}

QUrl Backend::resolveNodeTarget(const QString &target) const
{
    return resolveWikiTarget(target, currentFilePath());
}

QUrl Backend::resolveWikiTarget(const QString &target, const QString &fromFile) const
{
    QString text = QUrl::fromPercentEncoding(target.trimmed().toUtf8());
    text.replace(QLatin1Char('\\'), QLatin1Char('/'));
    if (text.startsWith(QLatin1String("./")))
        text = text.mid(2);
    const int hash = text.indexOf(QLatin1Char('#'));
    if (hash >= 0)
        text = text.left(hash).trimmed();
    if (text.isEmpty())
        return {};
    if (text.startsWith(QLatin1String("http://"), Qt::CaseInsensitive)
        || text.startsWith(QLatin1String("https://"), Qt::CaseInsensitive)
        || text.startsWith(QLatin1String("file:")))
        return QUrl(text);

    const QString currentDir = !fromFile.isEmpty()
        ? QFileInfo(fromFile).absolutePath()
        : (m_fileUrl.isLocalFile() ? QFileInfo(m_fileUrl.toLocalFile()).absolutePath()
                                   : workspaceFolderPath());
    const QString workspace = workspaceFolderPath();

    auto existingFile = [](const QString &path) -> QUrl {
        const QFileInfo info(path);
        if (info.exists() && info.isFile())
            return QUrl::fromLocalFile(info.absoluteFilePath());
        return {};
    };

    const QStringList roots = {currentDir, workspace};
    for (const QString &root : roots) {
        if (root.isEmpty())
            continue;
        if (const QUrl found = existingFile(QDir(root).filePath(text)); found.isValid())
            return found;
        if (QFileInfo(text).suffix().isEmpty()) {
            if (const QUrl found = existingFile(QDir(root).filePath(text + QStringLiteral(".md")));
                found.isValid())
                return found;
            if (const QUrl found =
                    existingFile(QDir(root).filePath(text + QStringLiteral(".markdown")));
                found.isValid())
                return found;
        }
    }
    if (m_fileUrl.isLocalFile()) {
        if (const QUrl found =
                existingFile(QDir(documentAssetsDirectoryPath()).filePath(QFileInfo(text).fileName()));
            found.isValid())
            return found;
    }

    const QString stem = QFileInfo(text).completeBaseName().isEmpty()
        ? QFileInfo(text).fileName()
        : QFileInfo(text).completeBaseName();
    const QString searchRoot = !workspace.isEmpty() ? workspace : currentDir;
    if (searchRoot.isEmpty() || stem.isEmpty())
        return {};
    QStringList matches;
    for (const QString &path : markdownFilesInDirectory(searchRoot)) {
        if (QFileInfo(path).completeBaseName().compare(stem, Qt::CaseInsensitive) == 0)
            matches.append(path);
    }
    if (matches.size() == 1)
        return QUrl::fromLocalFile(matches.constFirst());
    if (matches.size() > 1) {
        const QString wanted = QDir::fromNativeSeparators(text);
        for (const QString &path : matches) {
            const QString relative =
                QDir::fromNativeSeparators(QDir(searchRoot).relativeFilePath(path));
            const QString withoutExt = relative.section(QLatin1Char('.'), 0, -2);
            if (withoutExt.compare(wanted, Qt::CaseInsensitive) == 0)
                return QUrl::fromLocalFile(path);
        }
        return QUrl::fromLocalFile(matches.constFirst());
    }

    const QStringList images = imageFilesInDirectory(searchRoot);
    QStringList imageHits;
    const QString fileName = QFileInfo(text).fileName();
    for (const QString &path : images) {
        const QString relative =
            QDir::fromNativeSeparators(QDir(searchRoot).relativeFilePath(path));
        if (relative.compare(text, Qt::CaseInsensitive) == 0
            || relative.compare(QStringLiteral("./") + text, Qt::CaseInsensitive) == 0)
            return QUrl::fromLocalFile(path);
        if (QFileInfo(path).fileName().compare(fileName, Qt::CaseInsensitive) == 0)
            imageHits.append(path);
    }
    if (imageHits.size() == 1)
        return QUrl::fromLocalFile(imageHits.constFirst());
    if (imageHits.size() > 1)
        return QUrl::fromLocalFile(imageHits.constFirst());
    return {};
}

QVariantList Backend::buildOutgoingLinks() const
{
    QVariantList outgoing;
    QSet<QString> seen;
    const QString fromFile = currentFilePath();
    const QString root = workspaceFolderPath();
    for (const QString &target : MindMap::wikiTargets(currentDocumentText())) {
        const QUrl resolved = resolveWikiTarget(target, fromFile);
        const QString key = resolved.isLocalFile() ? fileIdentity(resolved.toLocalFile())
                                                   : target.toLower();
        if (key.isEmpty() || seen.contains(key))
            continue;
        seen.insert(key);
        QString name = target;
        QString title = target;
        if (resolved.isLocalFile()) {
            const QFileInfo info(resolved.toLocalFile());
            name = root.isEmpty()
                ? info.fileName()
                : QDir::fromNativeSeparators(QDir(root).relativeFilePath(info.absoluteFilePath()));
            title = info.completeBaseName();
        }
        outgoing.append(QVariantMap{
            {QStringLiteral("name"), name},
            {QStringLiteral("title"), title},
            {QStringLiteral("url"), resolved},
            {QStringLiteral("target"), target},
            {QStringLiteral("resolved"), resolved.isLocalFile()},
        });
    }
    return outgoing;
}

QVariantList Backend::buildBacklinks() const
{
    QVariantList backlinks;
    const QString currentId = fileIdentity(currentFilePath());
    if (currentId.isEmpty())
        return backlinks;

    const QString root = workspaceFolderPath();
    if (root.isEmpty())
        return backlinks;

    for (const QString &path : markdownFilesInDirectory(root)) {
        if (fileIdentity(path) == currentId)
            continue;
        QFile file(path);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            continue;
        const QString text = QString::fromUtf8(file.readAll());
        bool linked = false;
        for (const QString &target : MindMap::wikiTargets(text)) {
            const QUrl resolved = resolveWikiTarget(target, path);
            if (resolved.isLocalFile() && fileIdentity(resolved.toLocalFile()) == currentId) {
                linked = true;
                break;
            }
        }
        if (!linked)
            continue;
        const QFileInfo info(path);
        const FrontMatter::Document document = FrontMatter::parse(text);
        const QString relative =
            QDir::fromNativeSeparators(QDir(root).relativeFilePath(info.absoluteFilePath()));
        const QString title =
            FrontMatter::recordTitle(document.fields, document.body, info.completeBaseName());
        backlinks.append(QVariantMap{
            {QStringLiteral("name"), relative},
            {QStringLiteral("title"), title},
            {QStringLiteral("url"), QUrl::fromLocalFile(info.absoluteFilePath())},
        });
    }
    return backlinks;
}

void Backend::refreshOutgoingLinks()
{
    const QVariantList outgoing = buildOutgoingLinks();
    if (outgoing == m_outgoingLinks)
        return;
    m_outgoingLinks = outgoing;
    emit noteLinksChanged();
}

void Backend::refreshNoteLinks()
{
    const QVariantList outgoing = buildOutgoingLinks();
    const QVariantList backlinks = buildBacklinks();
    if (outgoing == m_outgoingLinks && backlinks == m_backlinks)
        return;
    m_outgoingLinks = outgoing;
    m_backlinks = backlinks;
    emit noteLinksChanged();
}

QString Backend::markupForDroppedUrl(const QUrl &url)
{
    if (!url.isValid() || url.isEmpty())
        return {};
    if (!url.isLocalFile()) {
        const QString href = url.toString();
        QString label = url.host();
        if (label.isEmpty())
            label = href;
        return QStringLiteral("[%1](%2)").arg(label, href);
    }
    if (isSupportedImageFilePath(url.toLocalFile())) {
        if (currentWorkspaceContains(url)) {
            const QString root = workspaceFolderPath();
            const QString fileName = QFileInfo(url.toLocalFile()).fileName();
            int same = 0;
            QString relative;
            if (!root.isEmpty()) {
                relative = QDir::fromNativeSeparators(
                    QDir(root).relativeFilePath(url.toLocalFile()));
                for (const QString &path : imageFilesInDirectory(root)) {
                    if (QFileInfo(path).fileName().compare(fileName, Qt::CaseInsensitive) == 0)
                        ++same;
                }
            }
            const QString target = same > 1 && !relative.isEmpty() ? relative : fileName;
            return QStringLiteral("![[%1]]").arg(target);
        }
        return importImageFile(url);
    }

    const QFileInfo info(url.toLocalFile());
    if (!info.exists())
        return {};
    if (isMarkdownFileName(info.fileName())) {
        QString target = info.completeBaseName();
        const QString root = workspaceFolderPath();
        if (!root.isEmpty()) {
            int same = 0;
            for (const QString &path : markdownFilesInDirectory(root)) {
                if (QFileInfo(path).completeBaseName().compare(target, Qt::CaseInsensitive) == 0)
                    ++same;
            }
            if (same > 1) {
                QString relative =
                    QDir::fromNativeSeparators(QDir(root).relativeFilePath(info.absoluteFilePath()));
                if (relative.endsWith(QStringLiteral(".markdown"), Qt::CaseInsensitive))
                    relative.chop(9);
                else if (relative.endsWith(QStringLiteral(".md"), Qt::CaseInsensitive))
                    relative.chop(3);
                target = relative;
            }
        }
        return QStringLiteral("[[%1]]").arg(target);
    }
    return QStringLiteral("[[%1]]").arg(info.fileName());
}

void Backend::mindmapAttachUrl(const QString &id, const QUrl &url)
{
    const QString markup = markupForDroppedUrl(url);
    if (markup.isEmpty())
        return;

    const QString source = editorPlainText();
    MindMap::Tree tree = MindMap::parse(source);
    QString targetId = id;
    if (targetId.isEmpty() || !MindMap::findNode(tree.root, targetId))
        targetId = m_mindmapSelectedId.isEmpty() ? tree.root.id : m_mindmapSelectedId;

    MindMap::Node *node = MindMap::findNode(tree.root, targetId);
    QString nextId = targetId;
    if (!node) {
        nextId = MindMap::addChild(tree.root, tree.root.id, markup);
    } else if (id.isEmpty()) {
        nextId = MindMap::addChild(tree.root, targetId, markup);
    } else if (MindMap::isLinkPlaceholder(node->title)
               || MindMap::parseMarkup(node->title).kind != QLatin1String("plain")) {
        nextId = MindMap::rename(tree.root, targetId, markup);
    } else {
        const MindMap::Markup dropped = MindMap::parseMarkup(markup);
        if (dropped.kind == QLatin1String("wiki") && !dropped.target.isEmpty()) {
            nextId = MindMap::rename(tree.root, targetId,
                                     QStringLiteral("[[%1|%2]]")
                                         .arg(dropped.target, node->title.trimmed()));
        } else {
            nextId = MindMap::addChild(tree.root, targetId, markup);
        }
    }
    if (nextId.isEmpty())
        return;
    commitMindmapText(MindMap::serialize(source, tree), nextId);
}

QVariantMap Backend::linkQueryAt(const QString &text, int cursor) const
{
    QVariantMap result{{QStringLiteral("active"), false},
                       {QStringLiteral("imagesPreferred"), false},
                       {QStringLiteral("query"), QString()},
                       {QStringLiteral("start"), 0},
                       {QStringLiteral("end"), cursor}};
    if (cursor < 0)
        cursor = 0;
    if (cursor > text.size())
        cursor = text.size();
    const QString before = text.left(cursor);

    static const QRegularExpression wikiEmbed(QStringLiteral(R"(!\[\[([^\]\n]*)$)"));
    static const QRegularExpression wiki(QStringLiteral(R"((?<!!)\[\[([^\]\n]*)$)"));
    static const QRegularExpression mdImage(QStringLiteral(R"(!\[[^\]]{0,80}\]\(([^)\n]*)$)"));
    static const QRegularExpression bang(QStringLiteral(R"(!\[\]?$)"));

    QRegularExpressionMatch match = wikiEmbed.match(before);
    if (match.hasMatch()) {
        result.insert(QStringLiteral("active"), true);
        result.insert(QStringLiteral("imagesPreferred"), true);
        result.insert(QStringLiteral("query"), match.captured(1));
        result.insert(QStringLiteral("start"), match.capturedStart());
        result.insert(QStringLiteral("end"), cursor);
        return result;
    }
    match = wiki.match(before);
    if (match.hasMatch()) {
        result.insert(QStringLiteral("active"), true);
        result.insert(QStringLiteral("query"), match.captured(1));
        result.insert(QStringLiteral("start"), match.capturedStart());
        result.insert(QStringLiteral("end"), cursor);
        return result;
    }
    match = mdImage.match(before);
    if (match.hasMatch()) {
        result.insert(QStringLiteral("active"), true);
        result.insert(QStringLiteral("imagesPreferred"), true);
        result.insert(QStringLiteral("query"), match.captured(1));
        result.insert(QStringLiteral("start"), match.capturedStart());
        result.insert(QStringLiteral("end"), cursor);
        return result;
    }
    match = bang.match(before);
    if (match.hasMatch()) {
        result.insert(QStringLiteral("active"), true);
        result.insert(QStringLiteral("imagesPreferred"), true);
        result.insert(QStringLiteral("query"), QString());
        result.insert(QStringLiteral("start"), match.capturedStart());
        result.insert(QStringLiteral("end"), cursor);
        return result;
    }
    return result;
}

int Backend::fragmentPosition(const QString &kind, const QString &fragment) const
{
    return FrontMatter::fragmentPosition(editorPlainText(), kind, fragment);
}

QVariantList Backend::linkSuggestions(const QString &query, bool imagesPreferred) const
{
    const QString needle = query.trimmed();
    QString noteQuery = needle;
    QString fragQuery;
    bool headingMode = false;
    bool blockMode = false;
    const int hash = needle.indexOf(QLatin1Char('#'));
    if (hash >= 0) {
        headingMode = true;
        noteQuery = needle.left(hash).trimmed();
        fragQuery = needle.mid(hash + 1).trimmed();
        if (fragQuery.startsWith(QLatin1Char('^'))) {
            blockMode = true;
            fragQuery = fragQuery.mid(1).trimmed();
        }
    } else if (needle.startsWith(QLatin1Char('^'))) {
        blockMode = true;
        headingMode = true;
        noteQuery.clear();
        fragQuery = needle.mid(1).trimmed();
    }

    QVariantList notes;
    QVariantList headings;
    QVariantList blocks;
    QVariantList images;
    const QString root = workspaceFolderPath();
    QHash<QString, int> stems;
    QStringList markdownFiles;
    QSet<QString> listedNotes;
    if (!root.isEmpty()) {
        markdownFiles = markdownFilesInDirectory(root);
        for (const QString &path : markdownFiles)
            stems[QFileInfo(path).completeBaseName().toLower()] += 1;
        for (const QString &path : markdownFiles) {
            const QFileInfo info(path);
            const QString relative =
                QDir::fromNativeSeparators(QDir(root).relativeFilePath(path));
            const QString insertTarget = wikiInsertTarget(path, stems, root);
            const QString haystack = relative + QLatin1Char(' ') + insertTarget;
            const bool noteHit = noteQuery.isEmpty()
                || haystack.contains(noteQuery, Qt::CaseInsensitive);
            if (noteHit && !headingMode) {
                notes.append(QVariantMap{
                    {QStringLiteral("kind"), QStringLiteral("note")},
                    {QStringLiteral("title"), info.completeBaseName()},
                    {QStringLiteral("subtitle"), relative},
                    {QStringLiteral("insert"), QStringLiteral("[[%1]]").arg(insertTarget)},
                });
                listedNotes.insert(path);
            }
            if (!noteHit && headingMode)
                continue;
            if (!headingMode && (noteQuery.size() < 2 || listedNotes.contains(path)))
                continue;

            const QString contents = noteTextFor(QUrl::fromLocalFile(path));
            if (headingMode && !blockMode) {
                for (const QVariant &item : FrontMatter::headingOutline(contents)) {
                    const QVariantMap heading = item.toMap();
                    const QString title = heading.value(QStringLiteral("title")).toString();
                    if (!fragQuery.isEmpty() && !title.contains(fragQuery, Qt::CaseInsensitive))
                        continue;
                    const bool sameFile = m_fileUrl.isLocalFile()
                        && QFileInfo(path).absoluteFilePath()
                            == QFileInfo(m_fileUrl.toLocalFile()).absoluteFilePath();
                    const QString insert = sameFile && noteQuery.isEmpty()
                        ? QStringLiteral("[[#%1]]").arg(title)
                        : QStringLiteral("[[%1#%2]]").arg(insertTarget, title);
                    headings.append(QVariantMap{
                        {QStringLiteral("kind"), QStringLiteral("heading")},
                        {QStringLiteral("title"), title},
                        {QStringLiteral("subtitle"), info.completeBaseName()},
                        {QStringLiteral("insert"), insert},
                    });
                    if (headings.size() >= 8)
                        break;
                }
            } else if (blockMode) {
                for (const QVariant &item : FrontMatter::blockAnchors(contents)) {
                    const QString id = item.toMap().value(QStringLiteral("id")).toString();
                    if (!fragQuery.isEmpty() && !id.contains(fragQuery, Qt::CaseInsensitive))
                        continue;
                    const bool sameFile = m_fileUrl.isLocalFile()
                        && QFileInfo(path).absoluteFilePath()
                            == QFileInfo(m_fileUrl.toLocalFile()).absoluteFilePath();
                    const QString insert = sameFile && noteQuery.isEmpty()
                        ? QStringLiteral("[[#^%1]]").arg(id)
                        : QStringLiteral("[[%1#^%2]]").arg(insertTarget, id);
                    blocks.append(QVariantMap{
                        {QStringLiteral("kind"), QStringLiteral("block")},
                        {QStringLiteral("title"), QLatin1Char('^') + id},
                        {QStringLiteral("subtitle"), info.completeBaseName()},
                        {QStringLiteral("insert"), insert},
                    });
                    if (blocks.size() >= 8)
                        break;
                }
            } else if (!headingMode && noteQuery.size() >= 2) {
                for (const QVariant &item : FrontMatter::headingOutline(contents)) {
                    const QString title = item.toMap().value(QStringLiteral("title")).toString();
                    if (!title.contains(noteQuery, Qt::CaseInsensitive))
                        continue;
                    headings.append(QVariantMap{
                        {QStringLiteral("kind"), QStringLiteral("heading")},
                        {QStringLiteral("title"), title},
                        {QStringLiteral("subtitle"), info.completeBaseName()},
                        {QStringLiteral("insert"),
                         QStringLiteral("[[%1#%2]]").arg(insertTarget, title)},
                    });
                    if (headings.size() >= 8)
                        break;
                }
            }
            if (headings.size() >= 8 && blocks.size() >= 8)
                break;
        }
    }

    const QString imageRoot = !root.isEmpty()
        ? root
        : (m_fileUrl.isLocalFile() ? QFileInfo(m_fileUrl.toLocalFile()).absolutePath()
                                   : QString());
    if (!imageRoot.isEmpty() && !headingMode) {
        QHash<QString, int> imageNames;
        const QStringList imageFiles = imageFilesInDirectory(imageRoot);
        for (const QString &path : imageFiles)
            imageNames[QFileInfo(path).fileName().toLower()] += 1;
        const QString imageNeedle = noteQuery;
        for (const QString &path : imageFiles) {
            const QFileInfo info(path);
            const QString relative =
                QDir::fromNativeSeparators(QDir(imageRoot).relativeFilePath(path));
            if (!imageNeedle.isEmpty()
                && !info.fileName().contains(imageNeedle, Qt::CaseInsensitive)
                && !relative.contains(imageNeedle, Qt::CaseInsensitive))
                continue;
            const QString target = imageNames.value(info.fileName().toLower()) > 1
                ? relative
                : info.fileName();
            images.append(QVariantMap{
                {QStringLiteral("kind"), QStringLiteral("image")},
                {QStringLiteral("title"), info.fileName()},
                {QStringLiteral("subtitle"), relative},
                {QStringLiteral("insert"), QStringLiteral("![[%1]]").arg(target)},
            });
            if (images.size() >= 12)
                break;
        }
    }

    QVariantList out;
    if (imagesPreferred)
        out = images + notes + headings + blocks;
    else
        out = notes + headings + blocks + images;
    if (out.size() > 16)
        out = out.mid(0, 16);
    return out;
}

void Backend::recordRecentFile(const QUrl &url) {
    if (!url.isLocalFile())
        return;

    const QFileInfo info(url.toLocalFile());
    if (!info.exists() || !isMarkdownFileName(info.fileName())
        || shouldSkipMarkdownFileName(info.fileName()))
        return;

    const QString path = info.absoluteFilePath();
    QVariantList next;
    next.append(QVariantMap{{QStringLiteral("name"), info.fileName()},
                            {QStringLiteral("url"), QUrl::fromLocalFile(path)}});
    for (const QVariant &item : m_recentFiles) {
        const QVariantMap entry = item.toMap();
        const QString existing = QFileInfo(entry.value(QStringLiteral("url")).toUrl().toLocalFile())
                                     .absoluteFilePath();
        if (existing == path || !QFileInfo::exists(existing))
            continue;
        next.append(entry);
        if (next.size() >= 20)
            break;
    }

    if (next == m_recentFiles)
        return;
    m_recentFiles = next;
    QStringList paths;
    for (const QVariant &item : m_recentFiles)
        paths.append(item.toMap().value(QStringLiteral("url")).toUrl().toLocalFile());
    QSettings().setValue(recentFilesSetting, paths);
    QSettings().setValue(lastFileSetting, path);
    emit recentFilesChanged();
}

void Backend::recordRecentWorkspace(const QString &directoryPath)
{
    if (directoryPath.isEmpty())
        return;
    const QString path = QDir(directoryPath).absolutePath();
    if (!QDir(path).exists())
        return;

    QVariantList next;
    next.append(workspaceEntry(path));
    for (const QVariant &item : m_recentWorkspaces) {
        const QVariantMap entry = item.toMap();
        const QString existing = QDir(entry.value(QStringLiteral("path")).toString()).absolutePath();
        if (existing == path || !QDir(existing).exists())
            continue;
        next.append(workspaceEntry(existing));
        if (next.size() >= recentWorkspaceLimit)
            break;
    }

    if (next == m_recentWorkspaces)
        return;
    m_recentWorkspaces = next;
    QStringList paths;
    for (const QVariant &item : m_recentWorkspaces)
        paths.append(item.toMap().value(QStringLiteral("path")).toString());
    QSettings().setValue(recentWorkspacesSetting, paths);
    emit recentWorkspacesChanged();
}

void Backend::restoreLastSession()
{
    refreshInboxFiles();
    if (m_fileUrl.isValid() && !m_fileUrl.isEmpty())
        return;

    QString folder = QSettings().value(lastWorkspaceDirectorySetting).toString();
    if (folder.isEmpty() && !m_recentWorkspaces.isEmpty())
        folder = m_recentWorkspaces.constFirst().toMap().value(QStringLiteral("path")).toString();
    folder = QDir(folder).absolutePath();
    if (!folder.isEmpty() && QDir(folder).exists())
        openFolder(QUrl::fromLocalFile(folder));

    if (m_modified)
        return;

    QString lastFile = QSettings().value(lastFileSetting).toString();
    if (lastFile.isEmpty())
        return;
    if (!QFileInfo::exists(lastFile)
        && isPathUnderDirectory(lastFile, resolvedThreadsDraftsFolder())) {
        const QString publishedTwin =
            QDir(threadsPublishedFolder()).filePath(QFileInfo(lastFile).fileName());
        if (QFileInfo::exists(publishedTwin))
            lastFile = publishedTwin;
    }
    if (!QFileInfo::exists(lastFile))
        return;
    const QUrl fileUrl = QUrl::fromLocalFile(QFileInfo(lastFile).absoluteFilePath());
    if (!workspaceFolderPath().isEmpty() && !currentWorkspaceContains(fileUrl)
        && !isInboxPath(fileUrl.toLocalFile()))
        return;
    open(fileUrl);
}

QString Backend::noteTextFor(const QUrl &url) const {
    if (m_document && (url == m_fileUrl || !url.isValid() || url.isEmpty()))
        return m_document->toPlainText();
    if (url.isLocalFile()) {
        QFile file(url.toLocalFile());
        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
            return QString::fromUtf8(file.readAll());
    }
    return m_lastDocumentText;
}

void Backend::applyNoteText(const QUrl &url, const QString &updated, bool refreshLists) {
    const bool currentDocument = m_document
        && (url == m_fileUrl || !url.isValid() || url.isEmpty());
    if (currentDocument) {
        if (m_document->toPlainText() == updated)
            return;
        m_loading = true;
        m_document->setPlainText(updated);
        m_lastDocumentText = updated;
        m_loading = false;
        setModified(true);
        refreshWordCount();
        updatePreviewDocument();
        scheduleRecovery();
        refreshDocumentOutline();
        if (refreshLists)
            refreshWorkspaceFiles();
        return;
    }

    if (!url.isLocalFile())
        return;

    QSaveFile file(url.toLocalFile());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        setStatus(QStringLiteral("Could not update %1.").arg(QFileInfo(url.toLocalFile()).fileName()));
        return;
    }
    file.write(updated.toUtf8());
    if (!file.commit()) {
        setStatus(QStringLiteral("Could not update %1.").arg(QFileInfo(url.toLocalFile()).fileName()));
        return;
    }
    if (refreshLists)
        refreshWorkspaceFiles();
}

void sortBoardRecordsByDate(QVariantList &records, const QString &dateField) {
    std::stable_sort(records.begin(), records.end(),
                     [&](const QVariant &left, const QVariant &right) {
                         const QString a =
                             FrontMatter::isoDate(recordFieldString(left.toMap(), dateField));
                         const QString b =
                             FrontMatter::isoDate(recordFieldString(right.toMap(), dateField));
                         if (a.isEmpty() != b.isEmpty())
                             return b.isEmpty();
                         return a < b;
                     });
}

QVariantList Backend::boardColumns() const {
    const QString field = resolvedBoardField();
    QStringList lanes;
    for (const QVariant &item : m_projectRecords) {
        const QString value = recordFieldString(item.toMap(), field).trimmed();
        if (value.isEmpty())
            continue;
        if (!lanes.contains(value, Qt::CaseInsensitive))
            lanes.append(value);
    }
    std::sort(lanes.begin(), lanes.end(), [](const QString &left, const QString &right) {
        return boardLaneRank(left) < boardLaneRank(right);
    });
    for (const QString &extra : m_statusChoices) {
        if (!extra.trimmed().isEmpty() && !lanes.contains(extra, Qt::CaseInsensitive))
            lanes.append(extra);
    }

    QVariantList columns;
    QSet<QString> used;
    for (const QString &lane : lanes) {
        const QString key = lane.toLower();
        if (used.contains(key))
            continue;
        used.insert(key);
        QVariantList records;
        for (const QVariant &item : m_projectRecords) {
            const QVariantMap record = item.toMap();
            if (recordFieldString(record, field).compare(lane, Qt::CaseInsensitive) == 0)
                records.append(record);
        }
        sortBoardRecordsByDate(records, resolvedDateField());
        columns.append(QVariantMap{{QStringLiteral("value"), lane},
                                   {QStringLiteral("label"), lane},
                                   {QStringLiteral("records"), records}});
    }

    QVariantList none;
    for (const QVariant &item : m_projectRecords) {
        const QVariantMap record = item.toMap();
        if (recordFieldString(record, field).trimmed().isEmpty())
            none.append(record);
    }
    sortBoardRecordsByDate(none, resolvedDateField());
    columns.append(QVariantMap{{QStringLiteral("value"), QString()},
                               {QStringLiteral("label"), QStringLiteral("None")},
                               {QStringLiteral("records"), none}});
    return columns;
}

QString Backend::calendarTitle() const {
    return QLocale().standaloneMonthName(m_calendarMonth) + QLatin1Char(' ')
        + QString::number(m_calendarYear);
}

QVariantList Backend::calendarCells() const {
    QVariantList cells;
    if (m_calendarYear < 1 || m_calendarMonth < 1)
        return cells;

    const QString field = resolvedDateField();
    const QDate first(m_calendarYear, m_calendarMonth, 1);
    const int mondayOffset = first.dayOfWeek() - 1;
    QDate cursor = first.addDays(-mondayOffset);
    for (int i = 0; i < 42; ++i) {
        const QString iso = cursor.toString(Qt::ISODate);
        QVariantList records;
        for (const QVariant &item : m_projectRecords) {
            const QVariantMap record = item.toMap();
            if (FrontMatter::isoDate(recordFieldString(record, field)) == iso)
                records.append(record);
        }
        cells.append(QVariantMap{
            {QStringLiteral("date"), iso},
            {QStringLiteral("day"), cursor.day()},
            {QStringLiteral("inMonth"), cursor.month() == m_calendarMonth},
            {QStringLiteral("records"), records},
        });
        cursor = cursor.addDays(1);
    }
    return cells;
}

QVariantList Backend::unscheduledRecords() const {
    const QString field = resolvedDateField();
    QVariantList records;
    for (const QVariant &item : m_projectRecords) {
        const QVariantMap record = item.toMap();
        if (FrontMatter::isoDate(recordFieldString(record, field)).isEmpty())
            records.append(record);
    }
    return records;
}

void Backend::stepCalendar(int monthDelta) {
    QDate cursor(m_calendarYear, m_calendarMonth, 1);
    cursor = cursor.addMonths(monthDelta);
    showCalendarMonth(cursor.year(), cursor.month());
}

void Backend::showCalendarToday() {
    const QDate today = QDate::currentDate();
    showCalendarMonth(today.year(), today.month());
}

QStringList Backend::calendarMonthNames() const {
    QStringList names;
    const QLocale locale;
    for (int month = 1; month <= 12; ++month)
        names.append(locale.standaloneMonthName(month, QLocale::ShortFormat));
    return names;
}

QVariantMap Backend::tableAt(const QString &text, int cursor) const {
    return CodeBlocks::tableAt(text, cursor);
}

QVariantMap Backend::insertTableColumn(const QString &text, int cursor) const {
    return CodeBlocks::insertTableColumn(text, cursor);
}

QVariantMap Backend::insertTableRow(const QString &text, int cursor) const {
    return CodeBlocks::insertTableRow(text, cursor);
}

QVariantMap Backend::tableMoveCell(const QString &text, int cursor, int delta) const {
    return CodeBlocks::tableMoveCell(text, cursor, delta);
}

QUrl Backend::createBoardNote(const QString &status) {
    const QUrl created = createMarkdownNote();
    if (!created.isLocalFile())
        return {};
    if (!status.trimmed().isEmpty())
        setRecordField(created, resolvedBoardField(), status);
    refreshProjectRecords();
    return created;
}

void Backend::showCalendarMonth(int year, int month) {
    const QDate cursor(year, month, 1);
    if (!cursor.isValid())
        return;
    if (cursor.year() == m_calendarYear && cursor.month() == m_calendarMonth)
        return;
    m_calendarYear = cursor.year();
    m_calendarMonth = cursor.month();
    emit calendarChanged();
}

void Backend::setRecordField(const QUrl &url, const QString &key, const QString &value) {
    if (!url.isLocalFile() || key.trimmed().isEmpty())
        return;

    const QString original = noteTextFor(url);
    if (original.isEmpty() && !QFileInfo::exists(url.toLocalFile())) {
        setStatus(QStringLiteral("Could not update %1.").arg(QFileInfo(url.toLocalFile()).fileName()));
        return;
    }

    const QString updated = value.trimmed().isEmpty()
        ? FrontMatter::removeField(original, key)
        : FrontMatter::setField(original, key, value);
    if (updated == original)
        return;

    applyNoteText(url, updated, true);
    setStatus(QStringLiteral("Updated %1").arg(key));
}

bool Backend::currentWorkspaceContains(const QUrl &url) const {
    if (!url.isLocalFile() || !m_workspaceFolderUrl.isLocalFile())
        return false;

    const QString root = QDir(m_workspaceFolderUrl.toLocalFile()).absolutePath();
    const QString filePath = QFileInfo(url.toLocalFile()).absoluteFilePath();
    return filePath == root || filePath.startsWith(root + QLatin1Char('/'));
}

QString Backend::inboxFolderPath() const {
    const QString custom = QSettings().value(inboxFolderSetting).toString().trimmed();
    if (!custom.isEmpty())
        return QDir::cleanPath(expandUserPath(custom));
    return QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
        .filePath(QStringLiteral("FMD/Inbox"));
}

QUrl Backend::inboxFolderUrl() const {
    return QUrl::fromLocalFile(inboxFolderPath());
}

bool Backend::isInboxPath(const QString &path) const {
    if (path.isEmpty())
        return false;
    const QString root = QDir(inboxFolderPath()).absolutePath();
    const QString absolute = QFileInfo(path).absoluteFilePath();
    return absolute == root || absolute.startsWith(root + QLatin1Char('/'));
}

void Backend::watchInboxFolder() {
    const QString folder = QDir(inboxFolderPath()).absolutePath();
    if (folder.isEmpty())
        return;
    const QStringList dirs = m_inboxWatcher.directories();
    if (!dirs.contains(folder)) {
        if (!dirs.isEmpty())
            m_inboxWatcher.removePaths(dirs);
        if (QFileInfo::exists(folder))
            m_inboxWatcher.addPath(folder);
    }
    const QStringList files = m_inboxWatcher.files();
    if (!files.isEmpty())
        m_inboxWatcher.removePaths(files);
    for (const QVariant &item : m_inboxFiles) {
        const QString path = item.toMap().value(QStringLiteral("path")).toString();
        if (!path.isEmpty() && QFileInfo::exists(path))
            m_inboxWatcher.addPath(path);
    }
}

void Backend::refreshInboxFiles() {
    if (m_refreshingInbox)
        return;
    m_refreshingInbox = true;
    const QString folder = inboxFolderPath();
    if (!QDir(folder).exists())
        QDir().mkpath(folder);
    QVariantList files;
    const QStringList markdown = markdownFilesInDirectory(folder);
    for (const QString &path : markdown) {
        const QFileInfo info(path);
        files.append(QVariantMap{
            {QStringLiteral("url"), QUrl::fromLocalFile(info.absoluteFilePath())},
            {QStringLiteral("path"), info.absoluteFilePath()},
            {QStringLiteral("name"), info.completeBaseName()},
            {QStringLiteral("fileName"), info.fileName()},
        });
    }
    const bool changed = files != m_inboxFiles;
    if (changed)
        m_inboxFiles = files;
    watchInboxFolder();
    m_refreshingInbox = false;
    if (changed)
        emit inboxChanged();
}

QUrl Backend::createInboxNote() {
    const QString folder = inboxFolderPath();
    if (!QDir().mkpath(folder)) {
        setStatus(QStringLiteral("Could not create the inbox folder."));
        return {};
    }
    QString fileName = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"));
    fileName += QStringLiteral(".md");
    int suffix = 2;
    while (QFileInfo::exists(QDir(folder).filePath(fileName))) {
        fileName = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss"))
            + QStringLiteral("-%1.md").arg(suffix++);
    }
    const QString path = QDir(folder).filePath(fileName);
    const QString today = QDate::currentDate().toString(Qt::ISODate);
    QString text = QStringLiteral("---\ninbox: true\ndate: %1\n---\n\n").arg(today);
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        setStatus(QStringLiteral("Could not create an inbox note."));
        return {};
    }
    file.write(text.toUtf8());
    if (!file.commit()) {
        setStatus(QStringLiteral("Could not create an inbox note."));
        return {};
    }
    refreshInboxFiles();
    setStatus(QStringLiteral("Inbox %1").arg(fileName));
    return QUrl::fromLocalFile(path);
}

void Backend::syncWorkspaceForFile(const QUrl &url) {
    if (!url.isLocalFile())
        return;

    if (isInboxPath(url.toLocalFile())) {
        refreshInboxFiles();
        return;
    }

    if (!m_workspaceFolderUrl.isLocalFile() || m_workspaceFolderUrl.isEmpty()) {
        const QString directoryPath = QFileInfo(url.toLocalFile()).absolutePath();
        rememberWorkspaceDirectory(directoryPath);
        setWorkspaceFolderUrl(QUrl::fromLocalFile(directoryPath));
        return;
    }

    if (currentWorkspaceContains(url)) {
        rememberWorkspaceDirectory(workspaceFolderPath());
        refreshWorkspaceFiles();
        return;
    }

    rememberWorkspaceDirectory(QFileInfo(url.toLocalFile()).absolutePath());
}

void Backend::loadStatusChoicesForWorkspace() {
    m_statusChoices = QSettings().value(statusChoicesKeyForFolder(workspaceFolderPath())).toStringList();
}

void Backend::saveStatusChoicesForWorkspace() {
    QSettings().setValue(statusChoicesKeyForFolder(workspaceFolderPath()), m_statusChoices);
}

void Backend::rememberWorkspaceDirectory(const QString &directoryPath) {
    if (directoryPath.isEmpty())
        return;

    QSettings().setValue(lastWorkspaceDirectorySetting, QDir(directoryPath).absolutePath());
}

QString Backend::preferredDialogDirectoryPath() const {
    if (m_workspaceFolderUrl.isLocalFile() && QDir(m_workspaceFolderUrl.toLocalFile()).exists())
        return QDir(m_workspaceFolderUrl.toLocalFile()).absolutePath();

    if (m_fileUrl.isLocalFile()) {
        const QString currentFileDirectory = QFileInfo(m_fileUrl.toLocalFile()).absolutePath();
        if (QDir(currentFileDirectory).exists())
            return currentFileDirectory;
    }

    const QString savedWorkspaceDirectory =
        QSettings().value(lastWorkspaceDirectorySetting).toString();
    if (!savedWorkspaceDirectory.isEmpty() && QDir(savedWorkspaceDirectory).exists())
        return QDir(savedWorkspaceDirectory).absolutePath();

    const QString savedDirectory = QSettings().value(lastSaveDirectorySetting).toString();
    if (!savedDirectory.isEmpty() && QDir(savedDirectory).exists())
        return QDir(savedDirectory).absolutePath();

    return QDir::homePath();
}

QString Backend::previewMarkdownFrom(const QString &text) {
    return FrontMatter::previewMarkdown(text);
}

void Backend::setPreviewTheme(const QString &theme) {
    QString normalized = theme;
    if (normalized != QLatin1String("night") && normalized != QLatin1String("newsprint")
        && normalized != QLatin1String("gothic"))
        normalized = QStringLiteral("night");
    if (m_previewTheme == normalized)
        return;
    m_previewTheme = normalized;
    QSettings().setValue(previewThemeSetting, normalized);
    emit previewThemeChanged();
    updatePreviewDocument();
}

void Backend::setPreviewMode(const QString &mode) {
    const QString normalized = mode == QLatin1String("threads")
        ? QStringLiteral("threads")
        : QStringLiteral("markdown");
    if (m_previewMode == normalized)
        return;
    m_previewMode = normalized;
    emit previewModeChanged();
}

void Backend::applyPreviewModeForDocument() {
    const FrontMatter::Document parsed = FrontMatter::parse(editorPlainText());
    if (FrontMatter::isThreadsPost(parsed.fields))
        setPreviewMode(QStringLiteral("threads"));
    else if (FrontMatter::isSlidevNote(parsed.fields))
        setPreviewMode(QStringLiteral("slides"));
    else if (m_previewMode == QLatin1String("threads")
             || m_previewMode == QLatin1String("slides"))
        setPreviewMode(QStringLiteral("markdown"));
}

QString Backend::previewBackground() const {
    if (m_previewTheme == QLatin1String("newsprint"))
        return QStringLiteral("#f6f3ee");
    if (m_previewTheme == QLatin1String("gothic"))
        return QStringLiteral("#faf8f5");
    return QStringLiteral("#161513");
}

QString Backend::previewForeground() const {
    if (m_previewTheme == QLatin1String("newsprint"))
        return QStringLiteral("#2f2c28");
    if (m_previewTheme == QLatin1String("gothic"))
        return QStringLiteral("#1d1c1a");
    return QStringLiteral("#eceae6");
}

QString Backend::previewMuted() const {
    if (m_previewTheme == QLatin1String("night"))
        return QStringLiteral("#9a958c");
    return QStringLiteral("#8b8680");
}

QString Backend::previewCanvas() const {
    if (m_previewTheme == QLatin1String("newsprint"))
        return QStringLiteral("#ddd6cb");
    if (m_previewTheme == QLatin1String("gothic"))
        return QStringLiteral("#e7e2da");
    return QStringLiteral("#0c0b0a");
}

void Backend::setSidebarSplitWidth(int width) {
    width = qBound(180, width, 560);
    if (m_sidebarSplitWidth == width)
        return;
    m_sidebarSplitWidth = width;
    QSettings().setValue(sidebarSplitSetting, width);
    emit splitSizesChanged();
}

void Backend::setPreviewSplitWidth(int width) {
    width = qBound(240, width, 720);
    if (m_previewSplitWidth == width)
        return;
    m_previewSplitWidth = width;
    QSettings().setValue(previewSplitSetting, width);
    emit splitSizesChanged();
}

void Backend::setThreadsDisplayName(const QString &name) {
    const QString trimmed = name.trimmed();
    if (m_threadsDisplayName == trimmed)
        return;
    m_threadsDisplayName = trimmed;
    QSettings().setValue(threadsDisplayNameSetting, trimmed);
    emit threadsProfileChanged();
}

void Backend::setThreadsHandle(const QString &handle) {
    QString trimmed = handle.trimmed();
    if (trimmed.startsWith(QLatin1Char('@')))
        trimmed.remove(0, 1);
    if (m_threadsHandle == trimmed)
        return;
    m_threadsHandle = trimmed;
    QSettings().setValue(threadsHandleSetting, trimmed);
    emit threadsProfileChanged();
}

void Backend::refreshThreadPreview(const QString &source) {
    const FrontMatter::Document parsed = FrontMatter::parse(source);
    const bool threads = FrontMatter::isThreadsPost(parsed.fields);
    const QVariantList posts = threads ? FrontMatter::threadPosts(source) : QVariantList{};
    if (threads == m_threadsPreview && posts == m_threadPosts)
        return;
    m_threadsPreview = threads;
    m_threadPosts = posts;
    emit threadsPreviewChanged();
}

void Backend::schedulePreviewUpdate() {
    if (m_loading)
        return;
    if (!m_previewVisible)
        return;
    m_previewTimer.start();
}

QVariantMap Backend::previewBlockAt(int index) const
{
    if (index < 0 || index >= m_previewBlocks.size())
        return {};
    return m_previewBlocks.at(index).toMap();
}

int Backend::previewBlockIndexAt(int cursor) const
{
    if (m_previewBlocks.isEmpty())
        return -1;
    int fallback = m_previewBlocks.size() - 1;
    for (int i = 0; i < m_previewBlocks.size(); ++i) {
        const QVariantMap block = m_previewBlocks.at(i).toMap();
        const int from = block.value(QStringLiteral("from")).toInt();
        const int to = block.value(QStringLiteral("to")).toInt();
        if (cursor < from)
            return i > 0 ? i - 1 : 0;
        if (cursor <= to)
            return i;
        fallback = i;
    }
    return fallback;
}

void Backend::setPreviewSlideIndex(int index) {
    const int clamped = m_previewSlides.isEmpty()
        ? 0
        : qBound(0, index, m_previewSlides.size() - 1);
    if (m_previewSlideIndex == clamped)
        return;
    m_previewSlideIndex = clamped;
    emit previewSlidesChanged();
}

void Backend::stepPreviewSlide(int delta) {
    setPreviewSlideIndex(m_previewSlideIndex + delta);
}

void Backend::refreshPreviewSlides(const QString &source) {
    const QVariantList slides = FrontMatter::isSlidevNote(FrontMatter::parse(source).fields)
        ? CodeBlocks::slidevPreviewSlides(source)
        : QVariantList{};
    const int previous = m_previewSlideIndex;
    m_previewSlides = slides;
    m_previewSlideIndex = slides.isEmpty() ? 0 : qBound(0, previous, slides.size() - 1);
    emit previewSlidesChanged();
}

void Backend::updatePreviewDocument() {
    const QString source = m_document ? m_document->toPlainText() : m_lastDocumentText;
    refreshThreadPreview(source);
    refreshPreviewSlides(source);

    const FrontMatter::Document parsed = FrontMatter::parse(source);
    QVariantList properties;
    QStringList order = parsed.fieldOrder;
    for (auto it = parsed.fields.constBegin(); it != parsed.fields.constEnd(); ++it) {
        if (!order.contains(it.key()))
            order.append(it.key());
    }
    for (const QString &key : order) {
        properties.append(QVariantMap{
            {QStringLiteral("key"), key},
            {QStringLiteral("value"), FrontMatter::displayValue(parsed.fields.value(key))},
        });
    }
    if (m_previewProperties != properties) {
        m_previewProperties = properties;
        emit previewPropertiesChanged();
    }

    const QVariantList blocks = CodeBlocks::previewBlocks(source);
    bool structureChanged = m_previewBlocks.size() != blocks.size();
    if (!structureChanged) {
        for (int i = 0; i < blocks.size(); ++i) {
            const QVariantMap previous = m_previewBlocks.at(i).toMap();
            const QVariantMap next = blocks.at(i).toMap();
            if (previous.value(QStringLiteral("kind")) != next.value(QStringLiteral("kind"))
                || previous.value(QStringLiteral("language")) != next.value(QStringLiteral("language"))) {
                structureChanged = true;
                break;
            }
        }
    }
    m_previewBlocks = blocks;
    if (structureChanged)
        emit previewBlocksChanged();
    ++m_previewRevision;
    emit previewRevisionChanged();

    if (!m_previewDocument)
        return;

    QFont previewFont(QStringLiteral("PingFang TC"));
    previewFont.setPixelSize(17);
    previewFont.setWeight(QFont::Normal);
    previewFont.setStyleStrategy(QFont::PreferTypoLineMetrics);
    m_previewDocument->setDefaultFont(previewFont);
    m_previewDocument->setBaseUrl(previewBaseUrl(m_fileUrl, m_workspaceFolderUrl));
    m_previewDocument->setMarkdown(FrontMatter::previewMarkdown(source),
                                   QTextDocument::MarkdownDialectGitHub);
    stylePreviewDocument(m_previewDocument);
}

void Backend::stylePreviewDocument(QTextDocument *document) {
    if (!document)
        return;

    QFont previewFont(QStringLiteral("PingFang TC"));
    previewFont.setPixelSize(17);
    previewFont.setWeight(QFont::Normal);
    previewFont.setStyleStrategy(QFont::PreferTypoLineMetrics);

    const QColor headingColor = QColor(previewForeground());
    const QColor bodyColor = QColor(previewForeground());
    const QColor ruleColor = m_previewTheme == QLatin1String("night")
        ? QColor(QStringLiteral("#3a3732"))
        : QColor(QStringLiteral("#e4dfd6"));
    const QColor codeBackground = m_previewTheme == QLatin1String("night")
        ? QColor(QStringLiteral("#1c1a18"))
        : QColor(QStringLiteral("#f3efe8"));

    for (QTextBlock block = document->begin(); block.isValid(); block = block.next()) {
        QTextCursor cursor(block);
        cursor.select(QTextCursor::BlockUnderCursor);
        QTextCharFormat format = cursor.charFormat();
        QTextBlockFormat blockFormat = block.blockFormat();
        blockFormat.setLineHeight(168, QTextBlockFormat::ProportionalHeight);
        blockFormat.setLeftMargin(0);
        blockFormat.setRightMargin(0);

        const int level = block.blockFormat().headingLevel();
        const QString codeLanguage =
            block.blockFormat().stringProperty(QTextFormat::BlockCodeLanguage);
        const bool isCode = block.blockFormat().hasProperty(QTextFormat::BlockCodeFence)
            || !codeLanguage.isEmpty();
        QFont font = previewFont;
        if (isCode) {
            font = QFont(QStringLiteral("iA Writer Mono S"));
            font.setPixelSize(14);
            font.setWeight(QFont::Normal);
            blockFormat.setTopMargin(2);
            blockFormat.setBottomMargin(2);
            format.setBackground(codeBackground);
        } else if (level == 1) {
            font.setPixelSize(26);
            font.setWeight(QFont::DemiBold);
            blockFormat.setTopMargin(8);
            blockFormat.setBottomMargin(14);
        } else if (level == 2) {
            font.setPixelSize(19);
            font.setWeight(QFont::DemiBold);
            blockFormat.setTopMargin(26);
            blockFormat.setBottomMargin(10);
        } else if (level == 3) {
            font.setPixelSize(16);
            font.setWeight(QFont::DemiBold);
            blockFormat.setTopMargin(20);
            blockFormat.setBottomMargin(8);
        } else {
            font.setPixelSize(17);
            font.setWeight(QFont::Normal);
            blockFormat.setTopMargin(0);
            blockFormat.setBottomMargin(14);
        }
        format.setFont(font);
        format.setForeground(level > 0 ? headingColor : bodyColor);
        cursor.mergeCharFormat(format);
        cursor.mergeBlockFormat(blockFormat);
    }

    const QColor linkColor = m_previewTheme == QLatin1String("night")
        ? QColor(QStringLiteral("#6cb6ff"))
        : QColor(QStringLiteral("#1a73e8"));
    for (QTextBlock block = document->begin(); block.isValid(); block = block.next()) {
        for (QTextBlock::iterator it = block.begin(); !(it.atEnd()); ++it) {
            const QTextFragment fragment = it.fragment();
            const QTextCharFormat fragmentFormat = fragment.charFormat();
            if (!fragmentFormat.isAnchor() && fragmentFormat.anchorHref().isEmpty())
                continue;
            QTextCursor linkCursor(document);
            linkCursor.setPosition(fragment.position());
            linkCursor.setPosition(fragment.position() + fragment.length(),
                                   QTextCursor::KeepAnchor);
            QTextCharFormat linkFormat;
            linkFormat.setForeground(linkColor);
            linkFormat.setFontUnderline(true);
            linkCursor.mergeCharFormat(linkFormat);
        }
    }

    QTextCursor tableCursor(document);
    tableCursor.movePosition(QTextCursor::Start);
    const QColor headerBackground = m_previewTheme == QLatin1String("night")
        ? QColor(QStringLiteral("#241f1c"))
        : QColor(QStringLiteral("#efe8dc"));
    while (!tableCursor.atEnd()) {
        if (QTextTable *table = tableCursor.currentTable()) {
            QTextTableFormat tableFormat = table->format();
            tableFormat.setBorder(1.0);
            tableFormat.setBorderBrush(ruleColor);
            tableFormat.setBorderStyle(QTextFrameFormat::BorderStyle_Solid);
            tableFormat.setCellPadding(8);
            tableFormat.setCellSpacing(0);
            tableFormat.setMargin(10);
            tableFormat.setHeaderRowCount(1);
            table->setFormat(tableFormat);
            for (int column = 0; column < table->columns(); ++column) {
                QTextTableCell cell = table->cellAt(0, column);
                QTextCharFormat cellFormat = cell.format();
                cellFormat.setBackground(headerBackground);
                cellFormat.setFontWeight(QFont::DemiBold);
                cell.setFormat(cellFormat);
            }
            tableCursor.setPosition(table->lastPosition() + 1);
            continue;
        }
        if (!tableCursor.movePosition(QTextCursor::NextBlock))
            break;
    }
}

void Backend::renderPreviewFragment(QObject *textDocument, const QString &markdown) {
    auto *quickDocument = qobject_cast<QQuickTextDocument *>(textDocument);
    if (!quickDocument || !quickDocument->textDocument())
        return;

    QTextDocument *document = quickDocument->textDocument();
    document->setUndoRedoEnabled(false);
    QFont previewFont(QStringLiteral("PingFang TC"));
    previewFont.setPixelSize(17);
    previewFont.setWeight(QFont::Normal);
    previewFont.setStyleStrategy(QFont::PreferTypoLineMetrics);
    document->setDefaultFont(previewFont);
    document->setBaseUrl(previewBaseUrl(m_fileUrl, m_workspaceFolderUrl));
    document->setMarkdown(markdown, QTextDocument::MarkdownDialectGitHub);
    stylePreviewDocument(document);
}

void Backend::setUiLanguage(const QString &language) {
    const QString normalized = UiLocale::normalize(language);
    if (m_uiLanguage == normalized)
        return;
    m_uiLanguage = normalized;
    QSettings().setValue(uiLanguageSetting, normalized);
    emit uiLanguageChanged();
}

QString Backend::t(const QString &key) const {
    return UiLocale::t(m_uiLanguage, key);
}

void Backend::setConfirmUnsavedChanges(bool confirm) {
    if (m_confirmUnsavedChanges == confirm)
        return;
    m_confirmUnsavedChanges = confirm;
    QSettings().setValue(confirmUnsavedSetting, confirm);
    emit confirmUnsavedChangesChanged();
}

bool Backend::isBlankDocument() const {
    return editorPlainText().trimmed().isEmpty();
}

bool Backend::isUntitledDocument() const {
    static const QRegularExpression untitledRe(
        QStringLiteral("^Untitled(?:-\\d+)?\\.md$"), QRegularExpression::CaseInsensitiveOption);
    return untitledRe.match(fileName()).hasMatch();
}

bool Backend::shouldPromptForUnsaved() const {
    if (!m_modified)
        return false;
    if (!m_confirmUnsavedChanges)
        return false;
    if (isBlankDocument() && isUntitledDocument())
        return false;
    return true;
}

QString Backend::hotkey(const QString &id) const {
    if (m_hotkeyOverrides.contains(id))
        return m_hotkeyOverrides.value(id);
    return defaultHotkey(id);
}

QVariantMap Backend::hotkeys() const {
    QVariantMap map;
    for (const HotkeySpec &spec : kHotkeys)
        map.insert(QString::fromLatin1(spec.id), hotkey(QString::fromLatin1(spec.id)));
    return map;
}

QVariantList Backend::hotkeyRows() const {
    QVariantList rows;
    QHash<QString, QStringList> owners;
    for (const HotkeySpec &spec : kHotkeys) {
        const QString id = QString::fromLatin1(spec.id);
        const QString seq = hotkey(id);
        if (!seq.isEmpty())
            owners[seq].append(id);
    }
    for (const HotkeySpec &spec : kHotkeys) {
        const QString id = QString::fromLatin1(spec.id);
        const QString seq = hotkey(id);
        const QString def = QString::fromLatin1(spec.def);
        QStringList conflicts = owners.value(seq);
        conflicts.removeAll(id);
        rows.append(QVariantMap{
            {QStringLiteral("id"), id},
            {QStringLiteral("title"), QString::fromLatin1(spec.title)},
            {QStringLiteral("sequence"), seq},
            {QStringLiteral("display"), hotkeyDisplay(seq)},
            {QStringLiteral("isDefault"), seq == def && !m_hotkeyOverrides.contains(id)},
            {QStringLiteral("conflict"), !seq.isEmpty() && !conflicts.isEmpty()},
        });
    }
    return rows;
}

void Backend::setHotkey(const QString &id, const QString &sequence) {
    if (defaultHotkey(id).isEmpty())
        return;
    const QString trimmed = sequence.trimmed();
    if (trimmed.isEmpty()) {
        clearHotkey(id);
        return;
    }
    m_hotkeyOverrides.insert(id, trimmed);
    QSettings settings;
    settings.beginGroup(hotkeysGroup);
    settings.setValue(id, trimmed);
    settings.endGroup();
    emit hotkeysChanged();
}

void Backend::clearHotkey(const QString &id) {
    m_hotkeyOverrides.insert(id, QString());
    QSettings settings;
    settings.beginGroup(hotkeysGroup);
    settings.setValue(id, QString());
    settings.endGroup();
    emit hotkeysChanged();
}

void Backend::resetHotkey(const QString &id) {
    m_hotkeyOverrides.remove(id);
    QSettings settings;
    settings.beginGroup(hotkeysGroup);
    settings.remove(id);
    settings.endGroup();
    emit hotkeysChanged();
}

QString Backend::captureHotkey(int key, int modifiers) const {
    if (key == Qt::Key_Escape || key == Qt::Key_unknown)
        return {};
    if (key == Qt::Key_Control || key == Qt::Key_Shift || key == Qt::Key_Alt
            || key == Qt::Key_Meta || key == Qt::Key_AltGr)
        return {};
    const QKeyCombination combination(static_cast<Qt::KeyboardModifiers>(modifiers),
                                      static_cast<Qt::Key>(key));
    const QKeySequence sequence(combination);
    return sequence.toString(QKeySequence::PortableText);
}

QString Backend::hotkeyDisplay(const QString &sequence) const {
    if (sequence.trimmed().isEmpty())
        return QStringLiteral("Blank");
    return QKeySequence(sequence, QKeySequence::PortableText).toString(QKeySequence::NativeText);
}

void Backend::discardBlankUntitled() {
    if (!isUntitledDocument() || !isBlankDocument())
        return;
    const QString path = currentFilePath();
    if (!path.isEmpty())
        QFile::remove(path);
    loadDocumentText(QString());
    setFileUrl(QUrl());
    setModified(false);
    refreshWorkspaceFiles();
}

void Backend::scheduleUntitledRename() {
    if (!isUntitledDocument() || !m_fileUrl.isLocalFile() || isBlankDocument())
        return;
    m_untitledRenameTimer.start();
}

void Backend::maybeAutoRenameUntitled() {
    m_untitledRenameTimer.stop();
    if (!m_fileUrl.isLocalFile() || !isUntitledDocument() || isBlankDocument())
        return;
    const QString stem = filenameStemFromDocument(editorPlainText());
    if (stem.isEmpty())
        return;
    renameNote(m_fileUrl, stem);
}

void Backend::setLastCodeLanguage(const QString &language) {
    QString normalized = language.trimmed().toLower();
    if (normalized.isEmpty())
        normalized = QStringLiteral("python");
    if (m_lastCodeLanguage == normalized)
        return;
    m_lastCodeLanguage = normalized;
    QSettings().setValue(lastCodeLanguageSetting, normalized);
    emit lastCodeLanguageChanged();
}

QVariantList Backend::codeLanguages(const QString &query) const {
    return CodeBlocks::languages(query, m_lastCodeLanguage);
}

QVariantList Backend::slashCommands(const QString &query) const {
    return CodeBlocks::slashCommands(query);
}

QVariantMap Backend::slashQueryAt(const QString &text, int cursor) const {
    return CodeBlocks::slashQueryAt(text, cursor);
}

QVariantMap Backend::backtickTriggerAt(const QString &text, int cursor) const {
    return CodeBlocks::backtickTriggerAt(text, cursor);
}

QVariantMap Backend::leaveFenceAt(const QString &text, int cursor) const {
    return CodeBlocks::leaveFenceAt(text, cursor);
}

QString Backend::codeFenceText(const QString &language, const QString &inner) const {
    return CodeBlocks::fenceText(language, inner);
}

int Backend::codeFenceCaretOffset(const QString &language, const QString &inner) const {
    return CodeBlocks::fenceCaretOffset(language, inner);
}

QString Backend::editorPlainText() const {
    if (m_document)
        return m_document->toPlainText();
    return m_lastDocumentText;
}

void Backend::setEditorPlainText(const QString &text) {
    loadDocumentText(text);
    setModified(true);
    setStatus(QStringLiteral("Unsaved"));
    emit fileChanged(currentFilePath());
}

void Backend::replaceEditorRange(int start, int end, const QString &text) {
    const QString current = editorPlainText();
    start = qBound(0, start, current.size());
    end = qBound(start, end, current.size());
    if (m_document) {
        QTextCursor cursor(m_document);
        cursor.beginEditBlock();
        cursor.setPosition(start);
        cursor.setPosition(end, QTextCursor::KeepAnchor);
        cursor.insertText(text);
        cursor.endEditBlock();
    } else {
        m_lastDocumentText = current.left(start) + text + current.mid(end);
        setModified(true);
        setStatus(QStringLiteral("Unsaved"));
        emit fileChanged(currentFilePath());
    }
}

void Backend::setEditorCaret(int position) {
    m_editorCaret = qMax(0, position);
    if (m_highlighter)
        m_highlighter->setCaretPosition(m_editorCaret);
}

void Backend::refreshLiveFolding() {
    applyLiveFolding();
}

void Backend::scheduleLiveFolding() {
    applyLiveFolding();
    if (m_liveFoldingQueued)
        return;
    m_liveFoldingQueued = true;
    QMetaObject::invokeMethod(this, [this]() {
        m_liveFoldingQueued = false;
        applyLiveFolding();
    }, Qt::QueuedConnection);
}

void Backend::applyLiveFolding() {
    if (m_applyingLiveFolding)
        return;
    m_applyingLiveFolding = true;

    const FrontMatter::Document parsed = FrontMatter::parse(editorPlainText());
    const int yamlEnd = parsed.hasFrontMatter ? qMax(0, parsed.yamlEnd) : 0;
    if (m_frontMatterEnd != yamlEnd) {
        m_frontMatterEnd = yamlEnd;
        emit previewPropertiesChanged();
    }
    if (m_highlighter) {
        m_highlighter->setLiveMode(m_editorMode == QLatin1String("live"));
        m_highlighter->setFrontMatterRange(parsed.yamlStart, parsed.yamlEnd);
        m_highlighter->setCaretPosition(m_editorCaret);
    }
    if (!m_document) {
        m_applyingLiveFolding = false;
        return;
    }

    const bool live = m_editorMode == QLatin1String("live");
    // Do not toggle undo/redo here: setUndoRedoEnabled(false) clears the stack,
    // which is why ⌘Z looked dead while typing in Live.
    QTextBlock block = m_document->begin();
    bool changed = false;
    while (block.isValid()) {
        const int pos = block.position();
        const bool inYaml = parsed.hasFrontMatter && parsed.yamlStart >= 0
            && pos >= parsed.yamlStart && pos < parsed.yamlEnd;
        const bool wantVisible = !(live && inYaml);
        if (block.isVisible() != wantVisible) {
            block.setVisible(wantVisible);
            changed = true;
        }
        block = block.next();
    }

    if (m_highlighter)
        m_highlighter->rehighlight();
    if (changed)
        m_document->markContentsDirty(0, qMax(1, m_document->characterCount() - 1));

    m_applyingLiveFolding = false;
}

void Backend::setCurrentFrontMatterField(const QString &key, const QString &value) {
    const QString trimmedKey = key.trimmed();
    if (trimmedKey.isEmpty() || trimmedKey.contains(QLatin1Char(':')))
        return;
    const QString current = editorPlainText();
    const QString updated = FrontMatter::setField(current, trimmedKey, value);
    if (updated == current)
        return;
    replaceEditorRange(0, current.size(), updated);
    applyLiveFolding();
    updatePreviewDocument();
    setStatus(QStringLiteral("Updated %1").arg(trimmedKey));
}

void Backend::removeCurrentFrontMatterField(const QString &key) {
    const QString trimmedKey = key.trimmed();
    if (trimmedKey.isEmpty())
        return;
    const QString current = editorPlainText();
    const QString updated = FrontMatter::removeField(current, trimmedKey);
    if (updated == current)
        return;
    replaceEditorRange(0, current.size(), updated);
    applyLiveFolding();
    updatePreviewDocument();
    setStatus(QStringLiteral("Removed %1").arg(trimmedKey));
}

QString Backend::currentFilePath() const {
    return m_fileUrl.isLocalFile() ? m_fileUrl.toLocalFile() : QString();
}

void Backend::copyText(const QString &text) {
    if (auto *clipboard = QGuiApplication::clipboard())
        clipboard->setText(text);
    setStatus(text.isEmpty() ? QStringLiteral("Nothing to copy")
                             : QStringLiteral("Copied"));
}

void Backend::loadOmarchyTheme() {
    m_themeBackground = m_darkMode ? QStringLiteral("#101010") : QStringLiteral("#ffffff");
    m_themeForeground = m_darkMode ? QStringLiteral("#eeeeee") : QStringLiteral("#222324");
    m_themeAccent = m_darkMode ? QStringLiteral("#5584aa") : QStringLiteral("#2077b2");
    m_themeSelection = m_darkMode ? QStringLiteral("#186a9a") : QStringLiteral("#2077b2");

    const QString colorsPath = QDir::homePath()
        + QStringLiteral("/.local/state/omarchy/current/theme/colors.toml");
    QString themeMode;
    QFile file(colorsPath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd()) {
            const QString line = in.readLine().trimmed();
            if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
                continue;

            const int equals = line.indexOf(QLatin1Char('='));
            if (equals < 0)
                continue;

            const QString key = line.left(equals).trimmed();
            QString value = line.mid(equals + 1).trimmed();
            if (value.size() >= 2
                    && ((value.front() == QLatin1Char('"') && value.back() == QLatin1Char('"'))
                        || (value.front() == QLatin1Char('\'') && value.back() == QLatin1Char('\''))))
                value = value.mid(1, value.size() - 2);

            if (key == QStringLiteral("mode"))
                themeMode = value;
            else if (key == QStringLiteral("background"))
                m_themeBackground = value;
            else if (key == QStringLiteral("foreground"))
                m_themeForeground = value;
            else if (key == QStringLiteral("accent"))
                m_themeAccent = value;
            else if (key == QStringLiteral("selection"))
                m_themeSelection = value;
        }
    }

    bool themeModeKnown = false;
    bool themeIsDark = m_darkMode;
    if (themeMode == QStringLiteral("dark")) {
        themeIsDark = true;
        themeModeKnown = true;
    } else if (themeMode == QStringLiteral("light")) {
        themeIsDark = false;
        themeModeKnown = true;
    } else {
        const QColor background(m_themeBackground);
        if (background.isValid()) {
            const double luminance = 0.299 * background.redF()
                + 0.587 * background.greenF() + 0.114 * background.blueF();
            themeIsDark = luminance < 0.5;
            themeModeKnown = true;
        }
    }
    if (themeModeKnown && themeIsDark != m_darkMode) {
        m_darkMode = themeIsDark;
        emit darkModeChanged();
    }

    if (m_highlighter) {
        m_highlighter->setDarkMode(m_darkMode);
        m_highlighter->setColors(m_themeBackground, m_themeForeground, m_themeAccent);
    }

    emit themeColorsChanged();
}

void Backend::watchOmarchyTheme() {
    const QStringList watched = m_themeWatcher.files() + m_themeWatcher.directories();
    if (!watched.isEmpty())
        m_themeWatcher.removePaths(watched);

    const QString currentDir = QDir::homePath()
        + QStringLiteral("/.local/state/omarchy/current");
    const QString themeDir = currentDir + QStringLiteral("/theme");
    const QString colorsPath = themeDir + QStringLiteral("/colors.toml");

    if (QDir(currentDir).exists())
        m_themeWatcher.addPath(currentDir);
    if (QDir(themeDir).exists())
        m_themeWatcher.addPath(themeDir);
    if (QFile::exists(colorsPath))
        m_themeWatcher.addPath(colorsPath);
}

QUrl Backend::suggestedSaveUrl() const {
    if (m_fileUrl.isLocalFile())
        return m_fileUrl;

    QDir directory = QDir::home();
    if (m_workspaceFolderUrl.isLocalFile() && QDir(m_workspaceFolderUrl.toLocalFile()).exists())
        directory = QDir(m_workspaceFolderUrl.toLocalFile());
    else {
        const QString savedDirectory = QSettings().value(lastSaveDirectorySetting).toString();
        if (!savedDirectory.isEmpty() && QDir(savedDirectory).exists())
            directory = QDir(savedDirectory);
    }
    return QUrl::fromLocalFile(
        directory.filePath(suggestedFileName(currentDocumentText())));
}

QString Backend::currentDocumentText() const {
    return editorPlainText();
}

int Backend::countWords(const QString &text) {
    static const QRegularExpression wordRe(
        QStringLiteral("[\\p{L}\\p{N}]+(?:['-][\\p{L}\\p{N}]+)*"));
    int count = 0;
    QRegularExpressionMatchIterator it = wordRe.globalMatch(text);
    while (it.hasNext()) {
        it.next();
        ++count;
    }
    return count;
}

QString Backend::filenameStemFromDocument(const QString &text) {
    const FrontMatter::Document document = FrontMatter::parse(text);
    QString raw = FrontMatter::headingTitle(document.body);
    if (raw.isEmpty()) {
        const QStringList lines = document.body.split(QLatin1Char('\n'));
        for (QString line : lines) {
            line = line.trimmed();
            if (line.isEmpty())
                continue;
            static const QRegularExpression listMark(
                QStringLiteral(R"(^(?:[-*+]|\d+\.)\s+)"));
            line.remove(listMark);
            raw = line.trimmed();
            if (!raw.isEmpty())
                break;
        }
    }
    if (raw.isEmpty())
        raw = FrontMatter::displayValue(document.fields.value(QStringLiteral("title"))).trimmed();
    raw.replace(QRegularExpression(QStringLiteral("^#{1,6}\\s+")), QString());
    const QString stem = sanitizeNoteStem(raw);
    if (stem.isEmpty() || isUntitledStem(stem))
        return {};
    return stem;
}

QString Backend::suggestedFileName(const QString &text) {
    QString name = filenameStemFromDocument(text);
    if (name.isEmpty()) {
        QString firstLine = text.section(QLatin1Char('\n'), 0, 0).trimmed();
        firstLine.replace(QRegularExpression(QStringLiteral("[/\\x00-\\x1f\\x7f]")),
                          QStringLiteral("-"));
        firstLine = firstLine.left(120).trimmed();
        if (firstLine.isEmpty() || firstLine == QStringLiteral(".")
            || firstLine == QStringLiteral("..") || firstLine == QStringLiteral("---"))
            name = QStringLiteral("Untitled");
        else
            name = sanitizeNoteStem(firstLine);
        if (name.isEmpty())
            name = QStringLiteral("Untitled");
    }
    if (!name.endsWith(QStringLiteral(".md"), Qt::CaseInsensitive)
        && !name.endsWith(QStringLiteral(".markdown"), Qt::CaseInsensitive))
        name += QStringLiteral(".md");
    return name;
}

void Backend::setWordCount(int words) {
    if (m_wordCount == words)
        return;

    m_wordCount = words;
    emit wordCountChanged();
}

void Backend::refreshWordCount() {
    const QString text = currentDocumentText();
    setWordCount(countWords(text));
    const FrontMatter::Document document = FrontMatter::parse(text);
    const int open = FrontMatter::openTaskCount(document.body);
    const int closed = FrontMatter::closedTaskCount(document.body);
    if (open != m_openTaskCount || closed != m_closedTaskCount) {
        m_openTaskCount = open;
        m_closedTaskCount = closed;
        emit wordCountChanged();
    }
}

void Backend::scheduleWordCount() {
    m_wordCountTimer.start();
}

void Backend::applyDocumentTypography() {
    if (!m_document)
        return;

    QTextBlockFormat blockFormat;
    blockFormat.setLineHeight(typoraLineHeightPercent, QTextBlockFormat::ProportionalHeight);

    // A full pass is only used for freshly loaded/attached documents, so it is
    // safe to drop undo history here (re-enabling clears the stack anyway).
    const bool undoEnabled = m_document->isUndoRedoEnabled();
    m_document->setUndoRedoEnabled(false);

    m_formattingTypography = true;
    QTextCursor cursor(m_document);
    cursor.select(QTextCursor::Document);
    cursor.mergeBlockFormat(blockFormat);
    m_formattingTypography = false;

    m_document->setUndoRedoEnabled(undoEnabled);

    m_formattedBlockCount = m_document->blockCount();
}

void Backend::reapplyTypographyToChange() {
    if (!m_document)
        return;

    QTextBlockFormat blockFormat;
    blockFormat.setLineHeight(typoraLineHeightPercent, QTextBlockFormat::ProportionalHeight);

    // Format only the block(s) touched by the last edit instead of the whole
    // document, and fold the change into the preceding edit command so a single
    // undo reverts both the text and its formatting.
    const int maxPos = m_document->characterCount() - 1;
    const int start = qBound(0, m_lastChangePos, maxPos);
    const int end = qBound(start, m_lastChangePos + m_lastChangeAdded, maxPos);

    m_formattingTypography = true;
    QTextCursor cursor(m_document);
    cursor.joinPreviousEditBlock();
    cursor.setPosition(start);
    cursor.setPosition(end, QTextCursor::KeepAnchor);
    cursor.mergeBlockFormat(blockFormat);
    cursor.endEditBlock();
    m_formattingTypography = false;
}
