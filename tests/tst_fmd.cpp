#include <QtTest>
#include <QClipboard>
#include <QCoreApplication>
#include <QDate>
#include <QDateTime>
#include <QDir>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QFont>
#include <QHash>
#include <QRegularExpression>
#include <QImage>
#include <QKeySequence>
#include <QMimeData>
#include <QProcess>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickStyle>
#include <QQuickTextDocument>
#include <QSettings>
#include <QStandardPaths>
#include <QTextBlock>
#include <QTextCursor>
#include <QTextDocument>
#include <QTextEdit>
#include <QTextFragment>

#include "backend.h"
#include "codeblocks.h"
#include "frontmatter.h"
#include "markdownhighlighter.h"
#include "mindmap.h"
#include "pluginhost.h"

namespace {

QString invokeStringMethod(QObject *object, const char *method) {
    QString result;
    const bool invoked = QMetaObject::invokeMethod(object, method, Q_RETURN_ARG(QString, result));
    if (!invoked)
        return QStringLiteral("!__invoke_failed__!");
    return result;
}

QString invokeStringMethod(QObject *object, const char *method, const QString &argument) {
    QString result;
    const bool invoked = QMetaObject::invokeMethod(
        object, method, Q_RETURN_ARG(QString, result), Q_ARG(QString, argument));
    if (!invoked)
        return QStringLiteral("!__invoke_failed__!");
    return result;
}

QStringList shortcutSequences(QObject *shortcut) {
    const QVariant sequencesValue = shortcut->property("sequences");
    if (sequencesValue.canConvert<QStringList>())
        return sequencesValue.toStringList();

    QStringList sequences;
    const QVariantList values = sequencesValue.toList();
    for (const QVariant &value : values) {
        if (value.canConvert<QKeySequence>())
            sequences.append(value.value<QKeySequence>().toString(QKeySequence::PortableText));
        else
            sequences.append(value.toString());
    }
    return sequences;
}

bool writeTextFile(const QString &path) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    file.close();
    return true;
}

bool writeImageFile(const QString &path) {
    QImage image(1, 1, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::black);
    return image.save(path);
}

} // namespace

class FmdTest : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        QVERIFY(m_settingsDirectory.isValid());
        QQuickStyle::setStyle(QStringLiteral("Material"));
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                           m_settingsDirectory.path());
    }

    void init() {
        QSettings settings;
        settings.clear();
        settings.sync();

        if (QClipboard *clipboard = QGuiApplication::clipboard())
            clipboard->clear();
    }

    void countsWords() {
        QCOMPARE(Backend::countWords(QStringLiteral("one two-three don't 42")), 4);
        QCOMPARE(Backend::countWords(QStringLiteral("你好 世界")), 2);
        QCOMPARE(Backend::countWords(QString()), 0);
    }

    void normalizesLinks() {
        QCOMPARE(Backend::normalizedLinkUrl(QStringLiteral("www.example.com/path")),
                 QStringLiteral("https://www.example.com/path"));
        QCOMPARE(Backend::normalizedLinkUrl(QStringLiteral("mailto:writer@example.com")),
                 QStringLiteral("mailto:writer@example.com"));
        QVERIFY(Backend::normalizedLinkUrl(QStringLiteral("example.com")).isEmpty());
        QVERIFY(Backend::normalizedLinkUrl(QStringLiteral("file:///tmp/private")).isEmpty());
    }

    void followsBareAndMarkdownLinksAtCursor() {
        Backend backend;
        const QString bare = QStringLiteral(
            "一手：https://www.gooood.cn/lucas-museum-of-narrative-art-by-mad.htm");
        const int urlAt = bare.indexOf(QStringLiteral("https://"));
        const QVariantMap hit = backend.followableLinkAt(bare, urlAt + 8);
        QCOMPARE(hit.value(QStringLiteral("kind")).toString(), QStringLiteral("external"));
        QCOMPARE(hit.value(QStringLiteral("url")).toUrl().host(),
                 QStringLiteral("www.gooood.cn"));
        QVERIFY(backend.followableLinkAt(bare, 0).value(QStringLiteral("kind")).toString().isEmpty());

        const QString wrapped = QStringLiteral("see [Gooood](https://www.gooood.cn/page) today.");
        const QVariantMap md = backend.followableLinkAt(wrapped, wrapped.indexOf(QLatin1Char('G')));
        QCOMPARE(md.value(QStringLiteral("kind")).toString(), QStringLiteral("external"));
        QCOMPARE(md.value(QStringLiteral("url")).toUrl().path(), QStringLiteral("/page"));

        const QString punct = QStringLiteral("https://example.com/a.htm.");
        const QVariantMap trimmed = backend.followableLinkAt(punct, 8);
        QCOMPARE(trimmed.value(QStringLiteral("url")).toUrl().toString(),
                 QStringLiteral("https://example.com/a.htm"));

        const QString fenced = QStringLiteral("```\nhttps://example.com/nope\n```\n");
        const int inside = fenced.indexOf(QStringLiteral("https://"));
        QVERIFY(backend.followableLinkAt(fenced, inside + 4)
                    .value(QStringLiteral("kind")).toString().isEmpty());

        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        QFile note(directory.filePath(QStringLiteral("target.md")));
        QVERIFY(note.open(QIODevice::WriteOnly | QIODevice::Text));
        note.write("# Target\n");
        note.close();
        backend.openFolder(QUrl::fromLocalFile(directory.path()));
        const QString wiki = QStringLiteral("see [[target]] here");
        const QVariantMap wikiHit = backend.followableLinkAt(wiki, wiki.indexOf(QLatin1Char('t')));
        QCOMPARE(wikiHit.value(QStringLiteral("kind")).toString(), QStringLiteral("note"));
        QCOMPARE(QFileInfo(wikiHit.value(QStringLiteral("url")).toUrl().toLocalFile()).fileName(),
                 QStringLiteral("target.md"));
    }

    void suggestsSafeNames() {
        QCOMPARE(Backend::suggestedFileName(QStringLiteral("My first draft\nBody")),
                 QStringLiteral("My first draft.md"));
        QCOMPARE(Backend::suggestedFileName(QStringLiteral("A/B")), QStringLiteral("A-B.md"));
        QCOMPARE(Backend::suggestedFileName(QString()), QStringLiteral("Untitled.md"));
        QCOMPARE(Backend::suggestedFileName(QStringLiteral("Already.md")),
                 QStringLiteral("Already.md"));
        QCOMPARE(Backend::suggestedFileName(QStringLiteral("---\nmindmap: true\n---\n\n# 會議紀錄\n")),
                 QStringLiteral("會議紀錄.md"));
        QCOMPARE(Backend::filenameStemFromDocument(QStringLiteral("# Untitled\n")),
                 QString());
    }

    void listsWorkspaceMarkdownFilesSorted() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());

        const auto touch = [](const QString &path) {
            QFile file(path);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
                return false;
            file.close();
            return true;
        };

        QVERIFY(touch(directory.filePath(QStringLiteral("zeta.md"))));
        QVERIFY(touch(directory.filePath(QStringLiteral("Alpha.markdown"))));
        QVERIFY(touch(directory.filePath(QStringLiteral("beta.md"))));
        QVERIFY(touch(directory.filePath(QStringLiteral("notes.txt"))));
        QVERIFY(QDir().mkpath(directory.filePath(QStringLiteral("nested"))));
        QVERIFY(touch(directory.filePath(QStringLiteral("nested/child.md"))));
        QVERIFY(QDir().mkpath(directory.filePath(QStringLiteral("note.assets"))));
        QVERIFY(touch(directory.filePath(QStringLiteral("note.assets/ignored.md"))));
        QVERIFY(QDir().mkpath(directory.filePath(QStringLiteral(".hidden"))));
        QVERIFY(touch(directory.filePath(QStringLiteral(".hidden/secret.md"))));
        QVERIFY(touch(directory.filePath(QStringLiteral("README.md"))));
        QVERIFY(touch(directory.filePath(QStringLiteral("SCHEMA.md"))));
        QVERIFY(touch(directory.filePath(QStringLiteral("_TEMPLATE-threads.md"))));

        const QStringList files = Backend::markdownFilesInDirectory(directory.path());
        QStringList relativeNames;
        for (const QString &path : files)
            relativeNames.append(QDir::fromNativeSeparators(
                QDir(directory.path()).relativeFilePath(path)));

        QCOMPARE(relativeNames, QStringList({QStringLiteral("Alpha.markdown"),
                                             QStringLiteral("beta.md"),
                                             QStringLiteral("nested/child.md"),
                                             QStringLiteral("zeta.md")}));
    }

    void workspaceTreeShowsFoldersAndCanCreateAndSort() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        QVERIFY(QDir().mkpath(directory.filePath(QStringLiteral("notes/empty"))));
        QFile alpha(directory.filePath(QStringLiteral("notes/alpha.md")));
        QVERIFY(alpha.open(QIODevice::WriteOnly | QIODevice::Text));
        alpha.write("# A\n");
        alpha.close();
        QFile zeta(directory.filePath(QStringLiteral("zeta.md")));
        QVERIFY(zeta.open(QIODevice::WriteOnly | QIODevice::Text));
        zeta.write("# Z\n");
        zeta.close();

        Backend backend;
        backend.openFolder(QUrl::fromLocalFile(directory.path()));
        QStringList kinds;
        QStringList names;
        for (const QVariant &item : backend.workspaceTree()) {
            const QVariantMap row = item.toMap();
            kinds.append(row.value(QStringLiteral("kind")).toString());
            names.append(row.value(QStringLiteral("name")).toString());
        }
        QVERIFY(kinds.contains(QStringLiteral("folder")));
        QVERIFY(names.contains(QStringLiteral("notes")));
        QVERIFY(names.contains(QStringLiteral("empty")));
        QVERIFY(names.contains(QStringLiteral("alpha.md")));
        QVERIFY(names.contains(QStringLiteral("zeta.md")));
        QCOMPARE(kinds.first(), QStringLiteral("folder"));

        const QString notesPath = QDir(directory.path()).filePath(QStringLiteral("notes"));
        backend.toggleWorkspaceFolder(notesPath);
        names.clear();
        for (const QVariant &item : backend.workspaceTree())
            names.append(item.toMap().value(QStringLiteral("name")).toString());
        QVERIFY(names.contains(QStringLiteral("notes")));
        QVERIFY(!names.contains(QStringLiteral("alpha.md")));
        QVERIFY(!names.contains(QStringLiteral("empty")));
        QVERIFY(names.contains(QStringLiteral("zeta.md")));

        backend.expandAllWorkspaceFolders();
        names.clear();
        for (const QVariant &item : backend.workspaceTree())
            names.append(item.toMap().value(QStringLiteral("name")).toString());
        QVERIFY(names.contains(QStringLiteral("alpha.md")));
        QVERIFY(names.contains(QStringLiteral("empty")));
        QVERIFY(!backend.workspaceFoldersCollapsed());

        backend.collapseAllWorkspaceFolders();
        names.clear();
        for (const QVariant &item : backend.workspaceTree())
            names.append(item.toMap().value(QStringLiteral("name")).toString());
        QVERIFY(names.contains(QStringLiteral("notes")));
        QVERIFY(names.contains(QStringLiteral("zeta.md")));
        QVERIFY(!names.contains(QStringLiteral("alpha.md")));
        QVERIFY(!names.contains(QStringLiteral("empty")));
        QVERIFY(backend.workspaceFoldersCollapsed());

        backend.toggleAllWorkspaceFolders();
        names.clear();
        for (const QVariant &item : backend.workspaceTree())
            names.append(item.toMap().value(QStringLiteral("name")).toString());
        QVERIFY(names.contains(QStringLiteral("alpha.md")));
        QVERIFY(!backend.workspaceFoldersCollapsed());

        backend.setSelectedWorkspacePath(QDir(directory.path()).absolutePath());
        const QUrl created = backend.createFolder(QStringLiteral("inbox"));
        QVERIFY(created.isLocalFile());
        QVERIFY(QDir(created.toLocalFile()).exists());
        names.clear();
        for (const QVariant &item : backend.workspaceTree())
            names.append(item.toMap().value(QStringLiteral("name")).toString());
        QVERIFY(names.contains(QStringLiteral("inbox")));

        backend.setSelectedWorkspacePath(created.toLocalFile());
        const QUrl nestedNote = backend.createMarkdownNote();
        QVERIFY(nestedNote.isLocalFile());
        QVERIFY(nestedNote.toLocalFile().startsWith(created.toLocalFile()));

        backend.setFileSort(QStringLiteral("name-desc"));
        QCOMPARE(backend.fileSort(), QStringLiteral("name-desc"));
        QStringList folderNames;
        for (const QVariant &item : backend.workspaceTree()) {
            const QVariantMap row = item.toMap();
            if (row.value(QStringLiteral("kind")).toString() == QLatin1String("folder")
                && row.value(QStringLiteral("depth")).toInt() == 0)
                folderNames.append(row.value(QStringLiteral("name")).toString());
        }
        QVERIFY(folderNames.size() >= 2);
        QVERIFY(QString::compare(folderNames.at(0), folderNames.at(1), Qt::CaseInsensitive) >= 0);
    }

    void revealCurrentFileExpandsCollapsedParents() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        QVERIFY(QDir().mkpath(directory.filePath(QStringLiteral("notes/deep"))));
        QFile note(directory.filePath(QStringLiteral("notes/deep/hidden.md")));
        QVERIFY(note.open(QIODevice::WriteOnly | QIODevice::Text));
        note.write("# Hidden\n");
        note.close();

        Backend backend;
        backend.openFolder(QUrl::fromLocalFile(directory.path()));
        backend.toggleWorkspaceFolder(QDir(directory.path()).filePath(QStringLiteral("notes")));
        backend.open(QUrl::fromLocalFile(note.fileName()));
        const int index = backend.expandAncestorsOfCurrentFile();
        QVERIFY(index >= 0);
        QCOMPARE(backend.workspaceNavFiles().at(index).toMap().value(QStringLiteral("name")).toString(),
                 QStringLiteral("hidden.md"));
        QCOMPARE(QDir(backend.selectedFolderPath()).absolutePath(),
                 QDir(directory.filePath(QStringLiteral("notes/deep"))).absolutePath());
    }

    void createsUntitledMarkdownInTheOpenFolder() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        Backend backend;
        QVERIFY(backend.createMarkdownNote().isEmpty());

        backend.openFolder(QUrl::fromLocalFile(directory.path()));
        const QUrl first = backend.createMarkdownNote();
        QCOMPARE(QFileInfo(first.toLocalFile()).fileName(), QStringLiteral("Untitled.md"));
        QVERIFY(QFileInfo::exists(first.toLocalFile()));
        const QUrl second = backend.createMarkdownNote();
        QCOMPARE(QFileInfo(second.toLocalFile()).fileName(), QStringLiteral("Untitled-2.md"));
        QVERIFY(first != second);
        QFile reread(first.toLocalFile());
        QVERIFY(reread.open(QIODevice::ReadOnly | QIODevice::Text));
        QVERIFY(!FrontMatter::isThreadsPost(
            FrontMatter::parse(QString::fromUtf8(reread.readAll())).fields));
    }

    void buildsPreviewBaseUrlFromMarkdownLocation() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        QVERIFY(QDir().mkpath(directory.filePath(QStringLiteral("drafts"))));

        const QString markdownPath = directory.filePath(QStringLiteral("drafts/chapter.md"));
        QFile markdownFile(markdownPath);
        QVERIFY(markdownFile.open(QIODevice::WriteOnly | QIODevice::Text));
        markdownFile.close();

        const QUrl baseUrl = Backend::previewBaseUrl(QUrl::fromLocalFile(markdownPath));
        QCOMPARE(baseUrl, QUrl::fromLocalFile(
                              QFileInfo(markdownPath).absolutePath() + QDir::separator()));
        QCOMPARE(baseUrl.resolved(QUrl(QStringLiteral("figure.png"))),
                 QUrl::fromLocalFile(QFileInfo(markdownPath).absolutePath()
                                     + QStringLiteral("/figure.png")));

        const QUrl workspaceBase =
            Backend::previewBaseUrl(QUrl(), QUrl::fromLocalFile(directory.path()));
        QCOMPARE(workspaceBase,
                 QUrl::fromLocalFile(directory.path() + QDir::separator()));
    }

    void resolvesPreviewImagesRelativeToMarkdownFile() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());

        const QString markdownPath = directory.filePath(QStringLiteral("chapter.md"));
        QFile markdownFile(markdownPath);
        QVERIFY(markdownFile.open(QIODevice::WriteOnly | QIODevice::Text));
        markdownFile.close();

        const QString imagePath = directory.filePath(QStringLiteral("figure.png"));
        QImage image(1, 1, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::black);
        QVERIFY(image.save(imagePath));

        QTextDocument preview;
        preview.setBaseUrl(Backend::previewBaseUrl(QUrl::fromLocalFile(markdownPath)));
        preview.setMarkdown(QStringLiteral("![Figure](figure.png)"));

        const QVariant imageResource = preview.resource(
            QTextDocument::ImageResource, preview.baseUrl().resolved(QUrl(QStringLiteral("figure.png"))));
        QVERIFY(imageResource.isValid());
    }

    void resolvesPreviewImagesFromAbsoluteFileUrls() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());

        const QString markdownPath = directory.filePath(QStringLiteral("chapter.md"));
        QVERIFY(writeTextFile(markdownPath));

        const QString imagePath = directory.filePath(QStringLiteral("figure.png"));
        QVERIFY(writeImageFile(imagePath));

        const QUrl imageUrl = QUrl::fromLocalFile(imagePath);
        QTextDocument preview;
        preview.setBaseUrl(Backend::previewBaseUrl(QUrl::fromLocalFile(markdownPath)));
        preview.setMarkdown(QStringLiteral("![Figure](%1)").arg(imageUrl.toString()));

        const QVariant imageResource =
            preview.resource(QTextDocument::ImageResource, imageUrl);
        QVERIFY(imageResource.isValid());
    }

    void parsesLocalImageClipboardText() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());

        const QString pngPath = directory.filePath(QStringLiteral("CleanShot [draft].png"));
        const QString pdfPath = directory.filePath(QStringLiteral("notes.pdf"));
        QVERIFY(writeImageFile(pngPath));
        QVERIFY(writeTextFile(pdfPath));

        QCOMPARE(Backend::localImageUrlFromClipboardText(QUrl::fromLocalFile(pngPath).toString()),
                 QUrl::fromLocalFile(pngPath));
        QCOMPARE(Backend::localImageUrlFromClipboardText(pngPath),
                 QUrl::fromLocalFile(pngPath));
        QVERIFY(Backend::localImageUrlFromClipboardText(
                    QUrl::fromLocalFile(pdfPath).toString()).isEmpty());
        QVERIFY(Backend::localImageUrlFromClipboardText(QStringLiteral("hello")).isEmpty());
    }

    void copiesExternalImageIntoDocumentAssets() {
        QTemporaryDir documentDir;
        QTemporaryDir sourceDir;
        QVERIFY(documentDir.isValid());
        QVERIFY(sourceDir.isValid());

        const QString markdownPath = documentDir.filePath(QStringLiteral("note.md"));
        const QString pngPath = sourceDir.filePath(QStringLiteral("CleanShot [draft].png"));
        QVERIFY(writeTextFile(markdownPath));
        QVERIFY(writeImageFile(pngPath));

        Backend backend;
        backend.open(QUrl::fromLocalFile(markdownPath));
        QCOMPARE(backend.importImageFile(QUrl::fromLocalFile(pngPath)),
                 QStringLiteral("![CleanShot \\[draft\\]](./note.assets/CleanShot%20%5Bdraft%5D.png)"));
        QVERIFY(QFileInfo::exists(
            documentDir.filePath(QStringLiteral("note.assets/CleanShot [draft].png"))));
        QVERIFY(QFileInfo::exists(pngPath));
    }

    void linksImagesAlreadyBesideTheDocumentWithoutCopying() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());

        const QString markdownPath = directory.filePath(QStringLiteral("chapter.md"));
        const QString imagePath = directory.filePath(QStringLiteral("figure.png"));
        QVERIFY(writeTextFile(markdownPath));
        QVERIFY(writeImageFile(imagePath));

        Backend backend;
        backend.open(QUrl::fromLocalFile(markdownPath));
        QCOMPARE(backend.importImageFile(QUrl::fromLocalFile(imagePath)),
                 QStringLiteral("![figure](./figure.png)"));
        QVERIFY(!QFileInfo::exists(directory.filePath(QStringLiteral("chapter.assets"))));
    }

    void copiesClipboardImageUrlsIntoDocumentAssets() {
        QTemporaryDir documentDir;
        QTemporaryDir sourceDir;
        QVERIFY(documentDir.isValid());
        QVERIFY(sourceDir.isValid());

        const QString markdownPath = documentDir.filePath(QStringLiteral("note.md"));
        const QString imagePath = sourceDir.filePath(QStringLiteral("Screenshot 42.png"));
        QVERIFY(writeTextFile(markdownPath));
        QVERIFY(writeImageFile(imagePath));

        auto *mimeData = new QMimeData;
        const QUrl imageUrl = QUrl::fromLocalFile(imagePath);
        mimeData->setUrls({imageUrl});
        mimeData->setText(imageUrl.toString());
        QGuiApplication::clipboard()->setMimeData(mimeData);

        Backend backend;
        backend.open(QUrl::fromLocalFile(markdownPath));
        QVERIFY(backend.clipboardContainsImportableImage());
        QCOMPARE(invokeStringMethod(&backend, "clipboardImageMarkdown"),
                 QStringLiteral("![Screenshot 42](./note.assets/Screenshot%2042.png)"));
        QVERIFY(QFileInfo::exists(
            documentDir.filePath(QStringLiteral("note.assets/Screenshot 42.png"))));
    }

    void writesClipboardBitmapIntoDocumentAssets() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());

        const QString markdownPath = directory.filePath(QStringLiteral("note.md"));
        QVERIFY(writeTextFile(markdownPath));

        QImage image(2, 2, QImage::Format_ARGB32_Premultiplied);
        image.fill(Qt::red);
        auto *mimeData = new QMimeData;
        mimeData->setImageData(image);
        QGuiApplication::clipboard()->setMimeData(mimeData);

        Backend backend;
        backend.open(QUrl::fromLocalFile(markdownPath));
        QVERIFY(backend.clipboardContainsImportableImage());
        const QString markdown = invokeStringMethod(&backend, "clipboardImageMarkdown");
        QVERIFY(markdown.startsWith(QStringLiteral("![pasted-")));
        QVERIFY(markdown.contains(QStringLiteral("](./note.assets/pasted-")));
        QVERIFY(markdown.endsWith(QStringLiteral(".png)")));

        const QDir assets(directory.filePath(QStringLiteral("note.assets")));
        QCOMPARE(assets.entryList(QStringList{QStringLiteral("*.png")}, QDir::Files).size(), 1);
    }

    void refusesImagePasteWhenDocumentIsUnsaved() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());

        const QString imagePath = directory.filePath(QStringLiteral("shot.png"));
        QVERIFY(writeImageFile(imagePath));

        auto *mimeData = new QMimeData;
        mimeData->setUrls({QUrl::fromLocalFile(imagePath)});
        QGuiApplication::clipboard()->setMimeData(mimeData);

        Backend backend;
        QVERIFY(backend.clipboardContainsImportableImage());
        QVERIFY(invokeStringMethod(&backend, "clipboardImageMarkdown").isEmpty());
        QVERIFY(backend.status().contains(QStringLiteral("Save the document")));
        QVERIFY(!QFileInfo::exists(directory.filePath(QStringLiteral("shot.assets"))));
    }

    void recognizesThreadsFrontMatterAliases() {
        QVERIFY(FrontMatter::isThreadsPost(
            FrontMatter::parse(QStringLiteral("---\nthreads: true\n---\n")).fields));
        QVERIFY(FrontMatter::isThreadsPost(
            FrontMatter::parse(QStringLiteral("---\nthreads: yes\n---\n")).fields));
        QVERIFY(FrontMatter::isThreadsPost(
            FrontMatter::parse(QStringLiteral("---\nthreads: 1\n---\n")).fields));
        QVERIFY(FrontMatter::isThreadsPost(
            FrontMatter::parse(QStringLiteral("---\nplatform: threads\n---\n")).fields));
        QVERIFY(!FrontMatter::isThreadsPost(
            FrontMatter::parse(QStringLiteral("---\ntitle: note\n---\n")).fields));
        QVERIFY(!FrontMatter::isThreadsPost(
            FrontMatter::parse(QStringLiteral("no yaml")).fields));
    }

    void routesThreadsFlagOnSaveAndLeavesOtherNotes() {
        QTemporaryDir notes;
        QTemporaryDir drafts;
        QVERIFY(notes.isValid());
        QVERIFY(drafts.isValid());

        const QString outsidePath = notes.filePath(QStringLiteral("idea.md"));
        const QString assetsDir = notes.filePath(QStringLiteral("idea.assets"));
        QVERIFY(QDir().mkpath(assetsDir));
        QVERIFY(QFile(assetsDir + QStringLiteral("/shot.png")).open(QIODevice::WriteOnly));
        QFile outside(outsidePath);
        QVERIFY(outside.open(QIODevice::WriteOnly | QIODevice::Text));
        outside.write("---\nthreads: true\nstatus: draft\n---\nhello\n");
        outside.close();

        const QString plainPath = notes.filePath(QStringLiteral("journal.md"));
        QFile plain(plainPath);
        QVERIFY(plain.open(QIODevice::WriteOnly | QIODevice::Text));
        plain.write("# just a note\n");
        plain.close();

        const QString alreadyPath = drafts.filePath(QStringLiteral("already.md"));
        QFile already(alreadyPath);
        QVERIFY(already.open(QIODevice::WriteOnly | QIODevice::Text));
        already.write("---\nthreads: true\nplatform: threads\n---\nin place\n");
        already.close();

        Backend backend;
        backend.setThreadsDraftsFolder(drafts.path());

        const QString moved = backend.routeThreadsPostAfterSave(
            outsidePath, QStringLiteral("---\nthreads: true\nstatus: draft\n---\nhello\n"));
        QVERIFY(moved.startsWith(drafts.path()));
        QVERIFY(!QFileInfo::exists(outsidePath));
        QVERIFY(QFileInfo::exists(moved));
        QVERIFY(QFileInfo::exists(QFileInfo(moved).absolutePath() + QStringLiteral("/")
                                  + QFileInfo(moved).completeBaseName() + QStringLiteral(".assets/shot.png")));

        QCOMPARE(backend.routeThreadsPostAfterSave(plainPath, QStringLiteral("# just a note\n")),
                 plainPath);
        QVERIFY(QFileInfo::exists(plainPath));

        QCOMPARE(backend.routeThreadsPostAfterSave(
                     alreadyPath, QStringLiteral("---\nthreads: true\nplatform: threads\n---\nin place\n")),
                 alreadyPath);
        QCOMPARE(QDir(drafts.path()).entryList(QStringList{QStringLiteral("already.md")}, QDir::Files).size(),
                 1);

        const QString sentPath = drafts.filePath(QStringLiteral("sent-post.md"));
        QFile sent(sentPath);
        QVERIFY(sent.open(QIODevice::WriteOnly | QIODevice::Text));
        sent.write("---\nthreads: true\nstatus: sent\npermalink: https://www.threads.net/t/x\n---\ndone\n");
        sent.close();
        const QString archived = backend.routeThreadsPostAfterSave(
            sentPath, QStringLiteral("---\nthreads: true\nstatus: sent\npermalink: https://www.threads.net/t/x\n---\ndone\n"));
        QVERIFY(archived.contains(QStringLiteral("/published/")));
        QVERIFY(QFileInfo::exists(archived));
        QVERIFY(!QFileInfo::exists(sentPath));
        QCOMPARE(backend.threadsPublishedFolder(),
                 QDir(drafts.path()).filePath(QStringLiteral("published")));
    }

    void openingThreadsFolderArchivesAlreadySentPosts() {
        QTemporaryDir drafts;
        QVERIFY(drafts.isValid());
        QFile sent(drafts.filePath(QStringLiteral("already-sent.md")));
        QVERIFY(sent.open(QIODevice::WriteOnly | QIODevice::Text));
        sent.write("---\nthreads: true\nstatus: sent\npermalink: https://www.threads.net/t/x\n---\ndone\n");
        sent.close();
        QFile draft(drafts.filePath(QStringLiteral("still-draft.md")));
        QVERIFY(draft.open(QIODevice::WriteOnly | QIODevice::Text));
        draft.write("---\nthreads: true\nstatus: draft\n---\nhello\n");
        draft.close();

        Backend backend;
        backend.setThreadsDraftsFolder(drafts.path());
        backend.openFolder(QUrl::fromLocalFile(drafts.path()));
        QVERIFY(!QFileInfo::exists(drafts.filePath(QStringLiteral("already-sent.md"))));
        QVERIFY(QFileInfo::exists(QDir(drafts.path()).filePath(QStringLiteral("published/already-sent.md"))));
        QVERIFY(QFileInfo::exists(drafts.filePath(QStringLiteral("still-draft.md"))));
        QCOMPARE(backend.archiveAlreadyPublishedThreadsDrafts(), 0);
    }

    void createsThreadsDraftFromTemplateWithoutTouchingOtherFolders() {
        QTemporaryDir drafts;
        QVERIFY(drafts.isValid());
        QTemporaryDir notes;
        QVERIFY(notes.isValid());

        QFile templateFile(drafts.filePath(QStringLiteral("_TEMPLATE-threads.md")));
        QVERIFY(templateFile.open(QIODevice::WriteOnly | QIODevice::Text));
        templateFile.write("---\ntitle: template\nstatus: draft\nplatform: threads\n---\nMain post\n---\nReply one\n");
        templateFile.close();

        Backend backend;
        backend.openFolder(QUrl::fromLocalFile(notes.path()));
        backend.setThreadsDraftsFolder(drafts.path());
        const QUrl draftUrl = backend.createThreadsDraft();
        QVERIFY(draftUrl.isLocalFile());
        QVERIFY(draftUrl.toLocalFile().startsWith(drafts.path()));
        QVERIFY(QFileInfo(draftUrl.toLocalFile()).fileName().startsWith(QStringLiteral("threads-")));
        QVERIFY(!QFileInfo::exists(notes.filePath(QFileInfo(draftUrl.toLocalFile()).fileName())));

        QFile draftFile(draftUrl.toLocalFile());
        QVERIFY(draftFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const FrontMatter::Document document =
            FrontMatter::parse(QString::fromUtf8(draftFile.readAll()));
        QCOMPARE(document.fields.value(QStringLiteral("status")).toString(),
                 QStringLiteral("draft"));
        QCOMPARE(document.fields.value(QStringLiteral("platform")).toString(),
                 QStringLiteral("threads"));
        QVERIFY(document.fields.value(QStringLiteral("date")).toString().startsWith(
            QDate::currentDate().toString(Qt::ISODate)));
        QVERIFY(document.body.contains(QStringLiteral("Main post")));
        QVERIFY(document.body.contains(QStringLiteral("---")));
    }

    void reusesUneditedThreadsDraftInsteadOfCreatingAnother() {
        QTemporaryDir drafts;
        QVERIFY(drafts.isValid());
        Backend backend;
        backend.setThreadsDraftsFolder(drafts.path());

        const QUrl first = backend.createThreadsDraft();
        QVERIFY(first.isLocalFile());
        QCOMPARE(QDir(drafts.path()).entryList(QStringList{QStringLiteral("threads-*.md")},
                                               QDir::Files).size(),
                 1);

        const QUrl second = backend.createThreadsDraft();
        QCOMPARE(second, first);
        QCOMPARE(QDir(drafts.path()).entryList(QStringList{QStringLiteral("threads-*.md")},
                                               QDir::Files).size(),
                 1);
        QVERIFY(backend.status().contains(QFileInfo(first.toLocalFile()).fileName()));

        QFile edited(first.toLocalFile());
        QVERIFY(edited.open(QIODevice::ReadOnly | QIODevice::Text));
        const QByteArray original = edited.readAll();
        edited.close();
        QVERIFY(edited.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text));
        edited.write(original + "a real post body\n");
        edited.close();

        const QUrl third = backend.createThreadsDraft();
        QVERIFY(third.isLocalFile());
        QVERIFY(third != first);
        QCOMPARE(QDir(drafts.path()).entryList(QStringList{QStringLiteral("threads-*.md")},
                                               QDir::Files).size(),
                 2);
    }

    void switchesInterfaceLanguageBetweenChineseAndEnglish() {
        Backend backend;
        QCOMPARE(backend.uiLanguage(), QStringLiteral("zh-TW"));
        QCOMPARE(backend.t(QStringLiteral("newPost")), QStringLiteral("新帖"));
        backend.setUiLanguage(QStringLiteral("en"));
        QCOMPARE(backend.uiLanguage(), QStringLiteral("en"));
        QCOMPARE(backend.t(QStringLiteral("newPost")), QStringLiteral("New post"));
        backend.setUiLanguage(QStringLiteral("fr"));
        QCOMPARE(backend.uiLanguage(), QStringLiteral("zh-TW"));
    }

    void splitsThreadsPostsOnBareRulesAndFlagsLength() {
        const QString source = QStringLiteral(
            "---\nthreads: true\nstatus: queued\n---\n"
            "Main post here\n\n"
            "```\n---\nnot a split\n```\n\n"
            "---\n"
            "First reply\n");
        QVERIFY(FrontMatter::isThreadsPost(FrontMatter::parse(source).fields));
        const QVariantList posts = FrontMatter::threadPosts(source);
        QCOMPARE(posts.size(), 2);
        QCOMPARE(posts.at(0).toMap().value(QStringLiteral("role")).toString(),
                 QStringLiteral("post"));
        QVERIFY(posts.at(0).toMap().value(QStringLiteral("text")).toString().contains(
            QStringLiteral("Main post here")));
        QVERIFY(posts.at(0).toMap().value(QStringLiteral("text")).toString().contains(
            QStringLiteral("not a split")));
        QCOMPARE(posts.at(1).toMap().value(QStringLiteral("role")).toString(),
                 QStringLiteral("reply"));
        QCOMPARE(posts.at(1).toMap().value(QStringLiteral("text")).toString(),
                 QStringLiteral("First reply"));
        QVERIFY(!posts.at(0).toMap().value(QStringLiteral("overLimit")).toBool());

        const QString tooLong = QStringLiteral("---\nplatform: threads\n---\n")
            + QString(FrontMatter::threadCharacterLimit() + 1, QLatin1Char('x'));
        const QVariantList overflow = FrontMatter::threadPosts(tooLong);
        QCOMPARE(overflow.size(), 1);
        QVERIFY(overflow.at(0).toMap().value(QStringLiteral("overLimit")).toBool());
        QCOMPARE(overflow.at(0).toMap().value(QStringLiteral("chars")).toInt(),
                 FrontMatter::threadCharacterLimit() + 1);

        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = directory.filePath(QStringLiteral("queued.md"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write(source.toUtf8());
        file.close();
        Backend backend;
        backend.open(QUrl::fromLocalFile(path));
        QVERIFY(backend.threadsPreview());
        QCOMPARE(backend.previewMode(), QStringLiteral("threads"));
        backend.setPreviewMode(QStringLiteral("markdown"));
        QCOMPARE(backend.previewMode(), QStringLiteral("markdown"));
        QVERIFY(backend.threadsPreview());
        QCOMPARE(backend.threadPosts().size(), 2);
    }

    void validateThreadsDraftUsesSchedulerCliWithoutPublishing() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = directory.filePath(QStringLiteral("plain.md"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write("just a note\n");
        file.close();

        Backend backend;
        backend.open(QUrl::fromLocalFile(path));
        const QVariantMap failed = backend.validateThreadsDraft();
        QVERIFY(failed.contains(QStringLiteral("ok")));
        QVERIFY(!failed.value(QStringLiteral("ok")).toBool());

        const QString cli = QStandardPaths::findExecutable(QStringLiteral("threads-schedule"));
        const QString homeCli = QDir::homePath() + QStringLiteral("/bin/threads-schedule");
        if (cli.isEmpty() && !QFileInfo::exists(homeCli))
            return;

        const QString validPath = directory.filePath(QStringLiteral("post.md"));
        QFile valid(validPath);
        QVERIFY(valid.open(QIODevice::WriteOnly | QIODevice::Text));
        valid.write("---\nthreads: true\nmedia_type: TEXT\n---\nHello from fmd tests.\n");
        valid.close();
        backend.open(QUrl::fromLocalFile(validPath));
        const QVariantMap passed = backend.validateThreadsDraft();
        QVERIFY2(passed.value(QStringLiteral("ok")).toBool(),
                 qPrintable(passed.value(QStringLiteral("output")).toString()));
        QCOMPARE(passed.value(QStringLiteral("replies")).toInt(), 0);
        QVERIFY(passed.value(QStringLiteral("mainChars")).toInt() > 0);
    }

    void openingThreadsTemplateDoesNotCreateANewDraft() {
        QTemporaryDir drafts;
        QVERIFY(drafts.isValid());
        Backend backend;
        backend.setThreadsDraftsFolder(drafts.path());
        const QUrl first = backend.ensureThreadsTemplate();
        const QUrl second = backend.ensureThreadsTemplate();
        QCOMPARE(first, second);
        QCOMPARE(QFileInfo(first.toLocalFile()).fileName(),
                 QStringLiteral("_TEMPLATE-threads.md"));
        const QStringList created = QDir(drafts.path()).entryList(QStringList{QStringLiteral("*.md")},
                                                                 QDir::Files);
        QCOMPARE(created, QStringList{QStringLiteral("_TEMPLATE-threads.md")});
    }

    void movesANoteToTrash() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = directory.filePath(QStringLiteral("gone.md"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write("bye\n");
        file.close();

        Backend backend;
        backend.openFolder(QUrl::fromLocalFile(directory.path()));
        QVERIFY(backend.moveNoteToTrash(QUrl::fromLocalFile(path)));
        QVERIFY(!QFileInfo::exists(path));
        QCOMPARE(backend.workspaceFiles().size(), 0);
    }

    void parsesYamlFrontMatter() {
        const FrontMatter::Document document = FrontMatter::parse(
            QStringLiteral("---\nstatus: draft\ndate: 2026-09-16\ntags: [one, two]\n---\n\n# Hello\n"));
        QVERIFY(document.hasFrontMatter);
        QCOMPARE(document.fields.value(QStringLiteral("status")).toString(),
                 QStringLiteral("draft"));
        QCOMPARE(document.fields.value(QStringLiteral("date")).toString(),
                 QStringLiteral("2026-09-16"));
        QCOMPARE(document.fields.value(QStringLiteral("tags")).toStringList(),
                 QStringList({QStringLiteral("one"), QStringLiteral("two")}));
        QVERIFY(document.body.contains(QStringLiteral("# Hello")));
        QCOMPARE(FrontMatter::recordTitle(document.fields, document.body, QStringLiteral("file")),
                 QStringLiteral("Hello"));
    }

    void writesFrontMatterWithoutLosingOtherFields() {
        const QString original =
            QStringLiteral("---\nstatus: draft\ndate: 2026-01-01\n---\nbody\n");
        const QString updated =
            FrontMatter::setField(original, QStringLiteral("status"), QStringLiteral("review"));
        const FrontMatter::Document document = FrontMatter::parse(updated);
        QCOMPARE(document.fields.value(QStringLiteral("status")).toString(),
                 QStringLiteral("review"));
        QCOMPARE(document.fields.value(QStringLiteral("date")).toString(),
                 QStringLiteral("2026-01-01"));
        QVERIFY(document.body.contains(QStringLiteral("body")));

        const QString cleared = FrontMatter::removeField(original, QStringLiteral("date"));
        const FrontMatter::Document withoutDate = FrontMatter::parse(cleared);
        QVERIFY(!withoutDate.fields.contains(QStringLiteral("date")));
        QCOMPARE(withoutDate.fields.value(QStringLiteral("status")).toString(),
                 QStringLiteral("draft"));

        const QString created = FrontMatter::setField(QStringLiteral("# Note\n"),
                                                      QStringLiteral("status"),
                                                      QStringLiteral("draft"));
        QVERIFY(created.startsWith(QStringLiteral("---\n")));
        QCOMPARE(FrontMatter::parse(created).fields.value(QStringLiteral("status")).toString(),
                 QStringLiteral("draft"));
    }

    void clearingDateFieldUnschedulesANote() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = directory.filePath(QStringLiteral("talk.md"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write("---\ntitle: Talk\ndate: 2026-09-16\n---\nbody\n");
        file.close();

        Backend backend;
        backend.openFolder(QUrl::fromLocalFile(directory.path()));
        backend.setRecordField(QUrl::fromLocalFile(path), QStringLiteral("date"), QString());
        QFile reread(path);
        QVERIFY(reread.open(QIODevice::ReadOnly | QIODevice::Text));
        const FrontMatter::Document document =
            FrontMatter::parse(QString::fromUtf8(reread.readAll()));
        QVERIFY(!document.fields.contains(QStringLiteral("date")));
        QCOMPARE(backend.unscheduledRecords().size(), 1);
    }

    void loadsProjectRecordsBoardAndCalendar() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());

        auto writeNote = [&](const QString &relative, const QString &contents) {
            const QString path = directory.filePath(relative);
            QDir().mkpath(QFileInfo(path).absolutePath());
            QFile file(path);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
                return false;
            file.write(contents.toUtf8());
            return true;
        };

        QVERIFY(writeNote(QStringLiteral("draft.md"),
                          QStringLiteral("---\nstatus: draft\ndate: 2026-09-16\n"
                                         "title: Draft paper\n---\ntext\n")));
        QVERIFY(writeNote(QStringLiteral("review.md"),
                          QStringLiteral("---\nstatus: review\n---\n# Review copy\n")));
        QVERIFY(writeNote(QStringLiteral("plain.md"), QStringLiteral("no front matter\n")));

        Backend backend;
        backend.openFolder(QUrl::fromLocalFile(directory.path()));
        QCOMPARE(backend.projectRecords().size(), 3);
        QVERIFY(backend.projectFieldNames().contains(QStringLiteral("status")));
        QCOMPARE(backend.boardField(), QStringLiteral("status"));
        QCOMPARE(backend.dateField(), QStringLiteral("date"));

        bool sawDraft = false;
        bool sawReview = false;
        bool sawNone = false;
        for (const QVariant &columnValue : backend.boardColumns()) {
            const QVariantMap column = columnValue.toMap();
            const QString value = column.value(QStringLiteral("value")).toString();
            const int count = column.value(QStringLiteral("records")).toList().size();
            if (value == QLatin1String("draft")) {
                sawDraft = true;
                QCOMPARE(count, 1);
            } else if (value == QLatin1String("review")) {
                sawReview = true;
                QCOMPARE(count, 1);
            } else if (value.isEmpty()) {
                sawNone = true;
                QCOMPARE(count, 1);
            }
        }
        QVERIFY(sawDraft);
        QVERIFY(sawReview);
        QVERIFY(sawNone);
        for (const QVariant &columnValue : backend.boardColumns()) {
            const QString value = columnValue.toMap().value(QStringLiteral("value")).toString();
            QVERIFY(value != QLatin1String("writing"));
            QVERIFY(value != QLatin1String("submitted"));
            QVERIFY(value != QLatin1String("published"));
        }

        backend.showCalendarMonth(2026, 9);
        bool sawDatedNote = false;
        for (const QVariant &cellValue : backend.calendarCells()) {
            const QVariantMap cell = cellValue.toMap();
            if (cell.value(QStringLiteral("date")).toString() == QLatin1String("2026-09-16")) {
                QCOMPARE(cell.value(QStringLiteral("records")).toList().size(), 1);
                sawDatedNote = true;
            }
        }
        QVERIFY(sawDatedNote);
        QCOMPARE(backend.unscheduledRecords().size(), 2);

        backend.setRecordField(QUrl::fromLocalFile(directory.filePath(QStringLiteral("review.md"))),
                               QStringLiteral("status"), QStringLiteral("submitted"));
        const QString reviewText = [&] {
            QFile file(directory.filePath(QStringLiteral("review.md")));
            file.open(QIODevice::ReadOnly | QIODevice::Text);
            return QString::fromUtf8(file.readAll());
        }();
        QCOMPARE(FrontMatter::parse(reviewText).fields.value(QStringLiteral("status")).toString(),
                 QStringLiteral("submitted"));
    }

    void previewOmitsYamlAndKeepsBody() {
        const QString source = QStringLiteral(
            "---\ntitle: Hidden\nstatus: draft\n---\n\n# Hello\n\nfirst line\nsecond line\n");
        const QString preview = Backend::previewMarkdownFrom(source);
        QVERIFY(!preview.contains(QStringLiteral("title:")));
        QVERIFY(!preview.contains(QStringLiteral("status:")));
        QVERIFY(preview.contains(QStringLiteral("# Hello")));
        QVERIFY(preview.contains(QStringLiteral("first line")));
        QVERIFY(preview.contains(QStringLiteral("second line")));

        const QString withUrl = Backend::previewMarkdownFrom(
            QStringLiteral("一手：https://www.archdaily.com/1184404/split-level\n"));
        QVERIFY(withUrl.contains(
            QStringLiteral("<https://www.archdaily.com/1184404/split-level>")));
    }

    void customStatusIsSharedAcrossViews() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = directory.filePath(QStringLiteral("note.md"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write("---\nstatus: draft\n---\nbody\n");
        file.close();

        Backend backend;
        backend.openFolder(QUrl::fromLocalFile(directory.path()));
        backend.addStatusChoice(QStringLiteral("camera-ready"));
        QVERIFY(backend.statusChoices().contains(QStringLiteral("camera-ready")));

        bool sawCustomLane = false;
        for (const QVariant &columnValue : backend.boardColumns()) {
            if (columnValue.toMap().value(QStringLiteral("value")).toString()
                == QLatin1String("camera-ready"))
                sawCustomLane = true;
        }
        QVERIFY(sawCustomLane);

        backend.renameStatus(QStringLiteral("draft"), QStringLiteral("camera-ready"));
        QFile reread(path);
        QVERIFY(reread.open(QIODevice::ReadOnly | QIODevice::Text));
        QCOMPARE(FrontMatter::parse(QString::fromUtf8(reread.readAll()))
                     .fields.value(QStringLiteral("status"))
                     .toString(),
                 QStringLiteral("camera-ready"));
    }

    void removesBoardColumnAndClearsMatchingStatus() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString occupied = directory.filePath(QStringLiteral("occupied.md"));
        QFile occupiedFile(occupied);
        QVERIFY(occupiedFile.open(QIODevice::WriteOnly | QIODevice::Text));
        occupiedFile.write("---\nstatus: 測試\n---\nbody\n");
        occupiedFile.close();
        const QString other = directory.filePath(QStringLiteral("other.md"));
        QFile otherFile(other);
        QVERIFY(otherFile.open(QIODevice::WriteOnly | QIODevice::Text));
        otherFile.write("---\nstatus: writing\n---\nbody\n");
        otherFile.close();

        Backend backend;
        backend.openFolder(QUrl::fromLocalFile(directory.path()));
        backend.addStatusChoice(QStringLiteral("測試"));

        bool sawLane = false;
        for (const QVariant &columnValue : backend.boardColumns()) {
            if (columnValue.toMap().value(QStringLiteral("value")).toString()
                == QStringLiteral("測試"))
                sawLane = true;
        }
        QVERIFY(sawLane);

        backend.removeStatusChoice(QStringLiteral("測試"));
        for (const QVariant &columnValue : backend.boardColumns()) {
            QVERIFY(columnValue.toMap().value(QStringLiteral("value")).toString()
                    != QStringLiteral("測試"));
        }

        QFile reread(occupied);
        QVERIFY(reread.open(QIODevice::ReadOnly | QIODevice::Text));
        QVERIFY(!FrontMatter::parse(QString::fromUtf8(reread.readAll()))
                     .fields.contains(QStringLiteral("status")));
    }

    void boardColumnsComeFromTheCurrentFolderOnly() {
        QTemporaryDir folderA;
        QTemporaryDir folderB;
        QVERIFY(folderA.isValid());
        QVERIFY(folderB.isValid());

        auto writeNote = [](const QString &path, const QString &contents) {
            QFile file(path);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
                return false;
            file.write(contents.toUtf8());
            return true;
        };
        QVERIFY(writeNote(folderA.filePath(QStringLiteral("a.md")),
                          QStringLiteral("---\nstatus: draft\n---\nA\n")));
        QVERIFY(writeNote(folderB.filePath(QStringLiteral("b.md")),
                          QStringLiteral("---\nstatus: queued\n---\nB\n")));

        Backend backend;
        backend.openFolder(QUrl::fromLocalFile(folderA.path()));
        backend.addStatusChoice(QStringLiteral("inbox"));

        QStringList valuesA;
        for (const QVariant &columnValue : backend.boardColumns())
            valuesA.append(columnValue.toMap().value(QStringLiteral("value")).toString());
        QVERIFY(valuesA.contains(QStringLiteral("draft")));
        QVERIFY(valuesA.contains(QStringLiteral("inbox")));
        QVERIFY(!valuesA.contains(QStringLiteral("queued")));
        QVERIFY(!valuesA.contains(QStringLiteral("writing")));

        backend.openFolder(QUrl::fromLocalFile(folderB.path()));
        QStringList valuesB;
        for (const QVariant &columnValue : backend.boardColumns())
            valuesB.append(columnValue.toMap().value(QStringLiteral("value")).toString());
        QVERIFY(valuesB.contains(QStringLiteral("queued")));
        QVERIFY(!valuesB.contains(QStringLiteral("draft")));
        QVERIFY(!valuesB.contains(QStringLiteral("inbox")));
        QVERIFY(!valuesB.contains(QStringLiteral("writing")));
    }

    void manuscriptTableAlwaysHasStatusAndDateColumns() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = directory.filePath(QStringLiteral("plain.md"));
        QVERIFY(writeTextFile(path));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write("no front matter yet\n");
        file.close();

        Backend backend;
        backend.openFolder(QUrl::fromLocalFile(directory.path()));
        const QStringList columns = backend.projectTableColumns();
        QVERIFY(columns.contains(QStringLiteral("File")));
        QVERIFY(columns.contains(QStringLiteral("Title")));
        QVERIFY(columns.contains(QStringLiteral("status")));
        QVERIFY(columns.contains(QStringLiteral("date")));

        const QUrl noteUrl = QUrl::fromLocalFile(path);
        backend.setRecordField(noteUrl, QStringLiteral("status"), QStringLiteral("writing"));
        backend.setRecordField(noteUrl, QStringLiteral("date"), QStringLiteral("2026-10-02"));

        QFile reread(path);
        QVERIFY(reread.open(QIODevice::ReadOnly | QIODevice::Text));
        const FrontMatter::Document document =
            FrontMatter::parse(QString::fromUtf8(reread.readAll()));
        QCOMPARE(document.fields.value(QStringLiteral("status")).toString(),
                 QStringLiteral("writing"));
        QCOMPARE(document.fields.value(QStringLiteral("date")).toString(),
                 QStringLiteral("2026-10-02"));
        QVERIFY(document.body.contains(QStringLiteral("no front matter yet")));
    }

    void ignoresPlainClipboardTextAsAnImage() {
        auto *mimeData = new QMimeData;
        mimeData->setText(QStringLiteral("just a sentence"));
        QGuiApplication::clipboard()->setMimeData(mimeData);

        Backend backend;
        QVERIFY(!backend.clipboardContainsImportableImage());
        QVERIFY(invokeStringMethod(&backend, "clipboardImageMarkdown").isEmpty());
    }

    void findsInlineMarkdownRanges() {
        const auto markup = MarkdownHighlighter::inlineMarkup(
            QStringLiteral("**bold** and *italic* and [site](https://example.com)"));
        QCOMPARE(markup.size(), 3);
        QCOMPARE(markup.at(0).content.start, 2);
        QCOMPARE(markup.at(0).content.length, 4);
        QCOMPARE(markup.at(2).content.length, 4);
        QCOMPARE(markup.at(2).markers[0].length, 1);
    }

    void underlinesBareHttpUrlsInTheEditor() {
        const QString line =
            QStringLiteral("一手：https://www.archdaily.com/1184404/split-level");
        const QList<MarkdownHighlighter::Span> spans = MarkdownHighlighter::urlSpans(line);
        QCOMPARE(spans.size(), 1);
        QCOMPARE(spans.at(0).start, line.indexOf(QStringLiteral("https://")));
        QVERIFY(spans.at(0).length > 12);
        QVERIFY(MarkdownHighlighter::urlSpans(QStringLiteral("no links here")).isEmpty());
    }

    void loadsCurrentOmarchyTheme() {
        QTemporaryDir homeDirectory;
        QVERIFY(homeDirectory.isValid());

        const QByteArray originalHome = qgetenv("HOME");
        struct HomeRestorer {
            QByteArray value;
            ~HomeRestorer() { qputenv("HOME", value); }
        } restoreHome{originalHome};
        QVERIFY(qputenv("HOME", homeDirectory.path().toUtf8()));

        const QString themeDirectory = homeDirectory.path()
            + QStringLiteral("/.local/state/omarchy/current/theme");
        QVERIFY(QDir().mkpath(themeDirectory));

        QFile colorsFile(themeDirectory + QStringLiteral("/colors.toml"));
        QVERIFY(colorsFile.open(QIODevice::WriteOnly | QIODevice::Text));
        const QByteArray palette(
            "mode = \"light\"\n"
            "accent = \"#112233\"\n"
            "selection = \"#445566\"\n"
            "background = \"#fefefe\"\n"
            "foreground = \"#101010\"\n");
        QCOMPARE(colorsFile.write(palette), qint64(palette.size()));
        colorsFile.close();

        Backend backend;
        QCOMPARE(backend.themeBackground(), QStringLiteral("#fefefe"));
        QCOMPARE(backend.themeForeground(), QStringLiteral("#101010"));
        QCOMPARE(backend.themeAccent(), QStringLiteral("#112233"));
        QCOMPARE(backend.themeSelection(), QStringLiteral("#445566"));
        QVERIFY(!backend.darkMode());
    }

    void ignoresFileWatcherEventsForSavedContents() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());

        const QString path = directory.filePath(QStringLiteral("first-save.md"));
        Backend backend;
        QSignalSpy externalChangeSpy(&backend, &Backend::externalChangeDetected);

        backend.saveAs(QUrl::fromLocalFile(path));
        QVERIFY(QFileInfo::exists(path));

        QFile sameContents(path);
        QVERIFY(sameContents.open(QIODevice::WriteOnly | QIODevice::Truncate));
        sameContents.close();
        QTest::qWait(100);
        QCOMPARE(externalChangeSpy.count(), 0);

        QFile changedContents(path);
        QVERIFY(changedContents.open(QIODevice::WriteOnly | QIODevice::Truncate));
        QCOMPARE(changedContents.write("changed elsewhere"), qint64(17));
        changedContents.close();
        QTRY_COMPARE(externalChangeSpy.count(), 1);
    }

    void collectsAndRenamesTagsFromYamlAndHashtags() {
        QCOMPARE(FrontMatter::normalizeTag(QStringLiteral("#Draft")), QStringLiteral("Draft"));
        const QString source = QStringLiteral(
            "---\ntags: [paper, draft]\n---\n\n# Hello\n\nSee #draft and #paper/methods.\n");
        QCOMPARE(FrontMatter::allTags(source),
                 QStringList({QStringLiteral("paper"), QStringLiteral("draft"),
                              QStringLiteral("paper/methods")}));

        const QString rewritten =
            FrontMatter::rewriteTags(source, QStringLiteral("paper"), QStringLiteral("journal"));
        QCOMPARE(FrontMatter::allTags(rewritten),
                 QStringList({QStringLiteral("journal"), QStringLiteral("draft"),
                              QStringLiteral("journal/methods")}));
        QVERIFY(rewritten.contains(QStringLiteral("#journal/methods")));
        QVERIFY(!rewritten.contains(QStringLiteral("#paper/methods")));

        const QString tagged = FrontMatter::addTag(QStringLiteral("# Note\n"), QStringLiteral("聲學"));
        QVERIFY(FrontMatter::allTags(tagged).contains(QStringLiteral("聲學")));
        QVERIFY(tagged.contains(QStringLiteral("#聲學")));

        QCOMPARE(FrontMatter::openTaskCount(QStringLiteral("- [ ] one\n- [x] two\n- [ ] three\n")),
                 2);
        QCOMPARE(FrontMatter::closedTaskCount(QStringLiteral("- [ ] one\n- [x] two\n- [ ] three\n")),
                 1);

        const QVariantList outline = FrontMatter::headingOutline(
            QStringLiteral("---\ntitle: Hidden\n---\n\n# One\n\n```\n# not a heading\n```\n\n## Two\n"));
        QCOMPARE(outline.size(), 2);
        QCOMPARE(outline.at(0).toMap().value(QStringLiteral("title")).toString(),
                 QStringLiteral("One"));
        QCOMPARE(outline.at(0).toMap().value(QStringLiteral("level")).toInt(), 1);
        QCOMPARE(outline.at(1).toMap().value(QStringLiteral("title")).toString(),
                 QStringLiteral("Two"));
        QCOMPARE(outline.at(1).toMap().value(QStringLiteral("level")).toInt(), 2);
        QVERIFY(outline.at(0).toMap().value(QStringLiteral("position")).toInt() > 0);
    }

    void copiesOutlineHeadingsAndRemembersLevel() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = directory.filePath(QStringLiteral("headings.md"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write("# Alpha\n\n## Beta\n\n### Gamma\n");
        file.close();

        Backend backend;
        backend.open(QUrl::fromLocalFile(path));
        QCOMPARE(backend.documentOutline().size(), 3);
        backend.copyOutlineHeadings();
        QCOMPARE(QGuiApplication::clipboard()->text(),
                 QStringLiteral("# Alpha\n## Beta\n### Gamma"));

        backend.setOutlineMaxLevel(2);
        QCOMPARE(backend.outlineMaxLevel(), 2);
        Backend restored;
        QCOMPARE(restored.outlineMaxLevel(), 2);
    }

    void parsesMindmapListsAndRoundtrips() {
        const QString source = QStringLiteral(
            "---\ntitle: Demo\n---\n\n# Root\n\n- A\n  - A1\n- B\n");
        const MindMap::Tree tree = MindMap::parse(source);
        QCOMPARE(tree.root.title, QStringLiteral("Root"));
        QCOMPARE(tree.root.children.size(), 2);
        QCOMPARE(tree.root.children.at(0).title, QStringLiteral("A"));
        QCOMPARE(tree.root.children.at(0).children.size(), 1);
        QCOMPARE(tree.root.children.at(0).children.at(0).title, QStringLiteral("A1"));
        QCOMPARE(tree.root.children.at(1).title, QStringLiteral("B"));
        QVERIFY(tree.fromLists);

        const QString written = MindMap::serialize(source, tree);
        QVERIFY(written.contains(QStringLiteral("mindmap: true")));
        const MindMap::Tree again = MindMap::parse(written);
        QCOMPARE(again.root.title, QStringLiteral("Root"));
        QCOMPARE(again.root.children.size(), 2);
        QCOMPARE(again.root.children.at(0).children.at(0).title, QStringLiteral("A1"));
    }

    void parsesMindmapFromHeadings() {
        const MindMap::Tree tree = MindMap::parse(QStringLiteral("# Root\n\n## A\n\n### A1\n\n## B\n"));
        QCOMPARE(tree.root.title, QStringLiteral("Root"));
        QCOMPARE(tree.root.children.size(), 2);
        QCOMPARE(tree.root.children.at(0).children.at(0).title, QStringLiteral("A1"));
    }

    void mindmapIndentReparentAndPreserveTail() {
        MindMap::Tree tree = MindMap::parse(
            QStringLiteral("# Root\n\n- A\n- B\n- C\n\nKeep this paragraph.\n"));
        QCOMPARE(tree.tail.trimmed(), QStringLiteral("Keep this paragraph."));
        const QString indented = MindMap::indent(tree.root, QStringLiteral("0.1"));
        QCOMPARE(indented, QStringLiteral("0.0.0"));
        QCOMPARE(tree.root.children.size(), 2);
        QCOMPARE(tree.root.children.at(0).children.at(0).title, QStringLiteral("B"));

        const QString written = MindMap::serialize(QStringLiteral("# Root\n"), tree);
        QVERIFY(written.contains(QStringLiteral("Keep this paragraph.")));
        QVERIFY(written.contains(QStringLiteral("  - B")));
    }

    void backendMindmapEditsWriteMarkdown() {
        Backend backend;
        backend.setEditorPlainText(QStringLiteral("# Root\n\n- A\n"));
        QCOMPARE(backend.mindmapOutline().size(), 2);
        QCOMPARE(backend.mindmapSelectedId(), QStringLiteral("0"));
        backend.mindmapAddChild(QStringLiteral("0"));
        QVERIFY(backend.editorPlainText().contains(QStringLiteral("mindmap: true")));
        QVERIFY(backend.editorPlainText().contains(backend.t(QStringLiteral("newNode"))));
        QCOMPARE(backend.mindmapOutline().size(), 3);
        backend.mindmapRename(backend.mindmapSelectedId(), QStringLiteral("Child"));
        QVERIFY(backend.editorPlainText().contains(QStringLiteral("- Child")));
        backend.setProjectView(QStringLiteral("mindmap"));
        QCOMPARE(backend.projectView(), QStringLiteral("mindmap"));
    }

    void mindmapRefreshDoesNotDependOnHeadingChange() {
        Backend backend;
        backend.setEditorPlainText(
            QStringLiteral("---\nmindmap: true\n---\n\n# Root\n\n- A\n"));
        QCOMPARE(backend.mindmapOutline().size(), 2);
        const int stamp = backend.mindmapStamp();
        backend.mindmapAddSibling(QStringLiteral("0.0"));
        QCOMPARE(backend.mindmapOutline().size(), 3);
        QVERIFY(backend.mindmapStamp() > stamp);
        QVERIFY(backend.editorPlainText().count(QStringLiteral("- ")) >= 2);
    }

    void mindmapLayoutAlignsColumnsAndCentersParent() {
        const MindMap::Tree tree = MindMap::parse(
            QStringLiteral("# Root\n\n- Short\n- Much longer sibling\n  - Leaf\n- Mid\n"));
        const QVariantMap laid = MindMap::layout(tree, 1.0);
        const QVariantList nodes = laid.value(QStringLiteral("nodes")).toList();
        QHash<QString, QVariantMap> byId;
        for (const QVariant &item : nodes) {
            const QVariantMap node = item.toMap();
            byId.insert(node.value(QStringLiteral("id")).toString(), node);
        }
        const QVariantMap root = byId.value(QStringLiteral("0"));
        const QVariantMap a = byId.value(QStringLiteral("0.0"));
        const QVariantMap b = byId.value(QStringLiteral("0.1"));
        const QVariantMap leaf = byId.value(QStringLiteral("0.1.0"));
        const QVariantMap c = byId.value(QStringLiteral("0.2"));
        QVERIFY(!root.isEmpty());
        QCOMPARE(a.value(QStringLiteral("x")).toReal(), b.value(QStringLiteral("x")).toReal());
        QCOMPARE(b.value(QStringLiteral("x")).toReal(), c.value(QStringLiteral("x")).toReal());
        QCOMPARE(a.value(QStringLiteral("w")).toReal(), b.value(QStringLiteral("w")).toReal());
        QVERIFY(leaf.value(QStringLiteral("x")).toReal() > b.value(QStringLiteral("x")).toReal());
        QVERIFY(a.value(QStringLiteral("y")).toReal() < b.value(QStringLiteral("y")).toReal());
        QVERIFY(b.value(QStringLiteral("y")).toReal() < c.value(QStringLiteral("y")).toReal());
        const qreal firstMid = a.value(QStringLiteral("y")).toReal()
            + a.value(QStringLiteral("h")).toReal() / 2;
        const qreal lastMid = c.value(QStringLiteral("y")).toReal()
            + c.value(QStringLiteral("h")).toReal() / 2;
        const qreal rootMid = root.value(QStringLiteral("y")).toReal()
            + root.value(QStringLiteral("h")).toReal() / 2;
        QCOMPARE(rootMid, (firstMid + lastMid) / 2);

        const QVariantList edges = laid.value(QStringLiteral("edges")).toList();
        QVERIFY(edges.size() >= 3);
        const qreal spine = edges.at(0).toMap().value(QStringLiteral("cx")).toReal();
        int siblingEdges = 0;
        for (const QVariant &item : edges) {
            const QVariantMap edge = item.toMap();
            if (edge.value(QStringLiteral("from")).toString() != QLatin1String("0"))
                continue;
            QCOMPARE(edge.value(QStringLiteral("cx")).toReal(), spine);
            ++siblingEdges;
        }
        QCOMPARE(siblingEdges, 3);
    }

    void mindmapCollapseHidesChildrenAndWritesYaml() {
        const MindMap::Tree tree = MindMap::parse(
            QStringLiteral("# Root\n\n- A\n  - A1\n  - A2\n- B\n"));
        const QVariantMap open = MindMap::layout(tree, 1.0);
        QCOMPARE(open.value(QStringLiteral("nodes")).toList().size(), 5);

        const QVariantMap folded = MindMap::layout(tree, 1.0, QStringList{QStringLiteral("0.0")});
        QStringList ids;
        for (const QVariant &item : folded.value(QStringLiteral("nodes")).toList())
            ids.append(item.toMap().value(QStringLiteral("id")).toString());
        QCOMPARE(ids.size(), 3);
        QVERIFY(ids.contains(QStringLiteral("0")));
        QVERIFY(ids.contains(QStringLiteral("0.0")));
        QVERIFY(ids.contains(QStringLiteral("0.1")));
        QVERIFY(!ids.contains(QStringLiteral("0.0.0")));
        const QVariantList outline = MindMap::flatten(tree.root, 0, {QStringLiteral("0.0")});
        QCOMPARE(outline.size(), 3);
        QCOMPARE(outline.at(1).toMap().value(QStringLiteral("collapsed")).toBool(), true);
        QCOMPARE(outline.at(1).toMap().value(QStringLiteral("collapsible")).toBool(), true);

        Backend backend;
        backend.setEditorPlainText(QStringLiteral("# Root\n\n- A\n  - A1\n- B\n"));
        backend.mindmapSetCollapsed(QStringLiteral("0.0"), true);
        const QString foldedText = backend.editorPlainText();
        QVERIFY(foldedText.contains(QStringLiteral("collapsed:")));
        QVERIFY(foldedText.contains(QStringLiteral("0.0")));
        QCOMPARE(MindMap::collapsedIds(foldedText), QStringList{QStringLiteral("0.0")});
        QCOMPARE(backend.mindmapLayout().value(QStringLiteral("nodes")).toList().size(), 3);

        backend.mindmapSetCollapsed(QStringLiteral("0.0"), false);
        QVERIFY(!backend.editorPlainText().contains(QStringLiteral("collapsed:")));
        QCOMPARE(backend.mindmapLayout().value(QStringLiteral("nodes")).toList().size(), 4);
    }

    void mindmapColorRoundtripSurvivesRename() {
        QCOMPARE(MindMap::applyNodeColor(QStringLiteral("Hello"), QStringLiteral("#E57373")),
                 QStringLiteral("Hello <!--c:#E57373-->"));
        const MindMap::Markup marked =
            MindMap::parseMarkup(QStringLiteral("Hello <!--c:#E57373-->"));
        QCOMPARE(marked.display, QStringLiteral("Hello"));
        QCOMPARE(marked.color, QStringLiteral("#E57373"));
        QCOMPARE(MindMap::applyNodeColor(QStringLiteral("Hello <!--c:#E57373-->"), QString()),
                 QStringLiteral("Hello"));

        Backend backend;
        backend.setEditorPlainText(QStringLiteral("# Root\n\n- A\n"));
        backend.mindmapSetColor(QStringLiteral("0.0"), QStringLiteral("#64B5F6"));
        QVERIFY(backend.editorPlainText().contains(QStringLiteral("<!--c:#64B5F6-->")));
        QVariantMap node;
        for (const QVariant &item : backend.mindmapLayout().value(QStringLiteral("nodes")).toList()) {
            if (item.toMap().value(QStringLiteral("id")).toString() == QLatin1String("0.0"))
                node = item.toMap();
        }
        QCOMPARE(node.value(QStringLiteral("color")).toString(), QStringLiteral("#64B5F6"));
        QCOMPARE(node.value(QStringLiteral("display")).toString(), QStringLiteral("A"));

        backend.mindmapRename(QStringLiteral("0.0"), QStringLiteral("Alpha"));
        QVERIFY(backend.editorPlainText().contains(QStringLiteral("- Alpha <!--c:#64B5F6-->")));
        QVERIFY(!backend.editorPlainText().contains(QStringLiteral("- A <!--c:")));
    }

    void mindmapEditsUndoThroughAttachedDocument() {
        Backend backend;
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData(QByteArray("import QtQuick\nTextEdit { objectName: \"undoEdit\" }\n"),
                          QUrl(QStringLiteral("qrc:/MindmapUndoHarness.qml")));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> editor(component.create());
        QVERIFY2(editor, qPrintable(component.errorString()));
        auto *quick = editor->property("textDocument").value<QQuickTextDocument *>();
        QVERIFY(quick);
        QVERIFY(quick->textDocument());
        backend.attachDocument(quick);
        quick->textDocument()->setPlainText(QStringLiteral("# Root\n\n- A\n"));
        backend.mindmapAddChild(QStringLiteral("0"));
        QVERIFY(backend.editorPlainText().contains(backend.t(QStringLiteral("newNode"))));
        QCOMPARE(backend.mindmapOutline().size(), 3);
        QVERIFY(quick->textDocument()->isUndoAvailable());
        quick->textDocument()->undo();
        QVERIFY(!backend.editorPlainText().contains(backend.t(QStringLiteral("newNode"))));
        QVERIFY(backend.editorPlainText().contains(QStringLiteral("- A")));
        QCOMPARE(MindMap::parse(backend.editorPlainText()).root.children.size(), 1);
    }

    void parsesMindmapMultilineTitlesAndGrowsLayout() {
        const QString source = QStringLiteral(
            "# Root\n  second root line\n\n- A\n  more A\n- B\n");
        const MindMap::Tree tree = MindMap::parse(source);
        QCOMPARE(tree.root.title, QStringLiteral("Root\nsecond root line"));
        QCOMPARE(tree.root.children.size(), 2);
        QCOMPARE(tree.root.children.at(0).title, QStringLiteral("A\nmore A"));
        QCOMPARE(tree.root.children.at(1).title, QStringLiteral("B"));

        const QString written = MindMap::serialize(source, tree);
        const MindMap::Tree again = MindMap::parse(written);
        QCOMPARE(again.root.title, QStringLiteral("Root\nsecond root line"));
        QCOMPARE(again.root.children.at(0).title, QStringLiteral("A\nmore A"));

        const QVariantMap laid = MindMap::layout(tree, 1.0);
        QHash<QString, QVariantMap> byId;
        for (const QVariant &item : laid.value(QStringLiteral("nodes")).toList()) {
            const QVariantMap node = item.toMap();
            byId.insert(node.value(QStringLiteral("id")).toString(), node);
        }
        QVERIFY(byId.value(QStringLiteral("0.0")).value(QStringLiteral("h")).toReal()
                > byId.value(QStringLiteral("0.1")).value(QStringLiteral("h")).toReal());
    }

    void parsesMindmapWikiAndImageMarkup() {
        const MindMap::Markup wiki = MindMap::parseMarkup(QStringLiteral("[[研究]]"));
        QCOMPARE(wiki.kind, QStringLiteral("wiki"));
        QCOMPARE(wiki.target, QStringLiteral("研究"));
        QCOMPARE(wiki.display, QStringLiteral("研究"));

        const MindMap::Markup aliased =
            MindMap::parseMarkup(QStringLiteral("[[研究|會議紀錄]]"));
        QCOMPARE(aliased.target, QStringLiteral("研究"));
        QCOMPARE(aliased.display, QStringLiteral("會議紀錄"));

        const MindMap::Markup image =
            MindMap::parseMarkup(QStringLiteral("![alt](./pic.png)"));
        QCOMPARE(image.kind, QStringLiteral("image"));
        QCOMPARE(image.target, QStringLiteral("./pic.png"));
        QVERIFY(image.embed);

        const MindMap::Markup emptyImage = MindMap::parseMarkup(QStringLiteral("![]"));
        QCOMPARE(emptyImage.kind, QStringLiteral("image"));
        QVERIFY(MindMap::isLinkPlaceholder(QStringLiteral("![]")));
        QVERIFY(MindMap::isLinkPlaceholder(QStringLiteral("新節點")));
        QVERIFY(MindMap::looksLikeImageTarget(QStringLiteral("shot.webp")));

        const MindMap::Markup heading =
            MindMap::parseMarkup(QStringLiteral("[[研究#方法]]"));
        QCOMPARE(heading.kind, QStringLiteral("wiki"));
        QCOMPARE(heading.target, QStringLiteral("研究"));
        QCOMPARE(heading.fragmentKind, QStringLiteral("heading"));
        QCOMPARE(heading.fragment, QStringLiteral("方法"));
        QCOMPARE(heading.display, QStringLiteral("研究 › 方法"));

        const MindMap::Markup block =
            MindMap::parseMarkup(QStringLiteral("[[研究#^quote-1]]"));
        QCOMPARE(block.fragmentKind, QStringLiteral("block"));
        QCOMPARE(block.fragment, QStringLiteral("quote-1"));

        const MindMap::Markup sameFile =
            MindMap::parseMarkup(QStringLiteral("[[#結論]]"));
        QCOMPARE(sameFile.target, QString());
        QCOMPARE(sameFile.fragment, QStringLiteral("結論"));

        const MindMap::Tree tree = MindMap::parse(
            QStringLiteral("# Root\n\n- ![alt](./x.png)\n- Hello\n"));
        const QVariantMap laid = MindMap::layout(tree, 1.0);
        QHash<QString, QVariantMap> byId;
        for (const QVariant &item : laid.value(QStringLiteral("nodes")).toList()) {
            const QVariantMap node = item.toMap();
            byId.insert(node.value(QStringLiteral("id")).toString(), node);
        }
        QVERIFY(byId.value(QStringLiteral("0.0")).value(QStringLiteral("h")).toReal()
                > byId.value(QStringLiteral("0.1")).value(QStringLiteral("h")).toReal());
        QCOMPARE(byId.value(QStringLiteral("0.0")).value(QStringLiteral("kind")).toString(),
                 QStringLiteral("image"));
    }

    void resolvesWikiLinksAndDropMarkup() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        QFile note(directory.filePath(QStringLiteral("研究.md")));
        QVERIFY(note.open(QIODevice::WriteOnly | QIODevice::Text));
        note.write("# 研究\n");
        note.close();
        QFile other(directory.filePath(QStringLiteral("other.md")));
        QVERIFY(other.open(QIODevice::WriteOnly | QIODevice::Text));
        other.write("# other\n");
        other.close();

        Backend backend;
        backend.openFolder(QUrl::fromLocalFile(directory.path()));
        backend.open(QUrl::fromLocalFile(directory.filePath(QStringLiteral("other.md"))));

        const QString typed = QStringLiteral("[[研");
        const QVariantMap query = backend.linkQueryAt(typed, typed.size());
        QVERIFY(query.value(QStringLiteral("active")).toBool());
        QCOMPARE(query.value(QStringLiteral("query")).toString(), QStringLiteral("研"));

        const QVariantList hits = backend.linkSuggestions(QStringLiteral("研"), false);
        QCOMPARE(hits.size(), 1);
        QCOMPARE(hits.at(0).toMap().value(QStringLiteral("insert")).toString(),
                 QStringLiteral("[[研究]]"));
        QCOMPARE(backend.resolveNodeTarget(QStringLiteral("研究")).toLocalFile(),
                 QFileInfo(directory.filePath(QStringLiteral("研究.md"))).absoluteFilePath());
        QCOMPARE(backend.markupForDroppedUrl(
                     QUrl::fromLocalFile(directory.filePath(QStringLiteral("研究.md")))),
                 QStringLiteral("[[研究]]"));

        QVERIFY(backend.linkQueryAt(QStringLiteral("![]"), 3)
                    .value(QStringLiteral("imagesPreferred"))
                    .toBool());
    }

    void jumpsToHeadingAndBlockFragments() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = directory.filePath(QStringLiteral("研究.md"));
        QFile note(path);
        QVERIFY(note.open(QIODevice::WriteOnly | QIODevice::Text));
        note.write("# 研究\n\nintro\n\n## 方法\n\nbody ^quote-1\n\n## 結論\n");
        note.close();

        Backend backend;
        backend.open(QUrl::fromLocalFile(path));
        const int method = backend.fragmentPosition(QStringLiteral("heading"),
                                                    QStringLiteral("方法"));
        const int block = backend.fragmentPosition(QStringLiteral("block"),
                                                   QStringLiteral("quote-1"));
        const int conclusion = backend.fragmentPosition(QStringLiteral("heading"),
                                                        QStringLiteral("結論"));
        QVERIFY(method > 0);
        QVERIFY(block > method);
        QVERIFY(conclusion > block);
    }

    void suggestsVaultImagesAndHeadingLinks() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        QVERIFY(QDir().mkpath(directory.filePath(QStringLiteral("note.assets"))));
        QImage image(1, 1, QImage::Format_ARGB32);
        image.fill(Qt::black);
        QVERIFY(image.save(directory.filePath(QStringLiteral("note.assets/shot.png")), "PNG"));
        QFile note(directory.filePath(QStringLiteral("note.md")));
        QVERIFY(note.open(QIODevice::WriteOnly | QIODevice::Text));
        note.write("# Note\n\n## 方法\n");
        note.close();
        QFile other(directory.filePath(QStringLiteral("other.md")));
        QVERIFY(other.open(QIODevice::WriteOnly | QIODevice::Text));
        other.write("# other\n");
        other.close();

        Backend backend;
        backend.openFolder(QUrl::fromLocalFile(directory.path()));
        backend.open(QUrl::fromLocalFile(directory.filePath(QStringLiteral("other.md"))));

        const QVariantList images = backend.linkSuggestions(QStringLiteral("shot"), true);
        QVERIFY(!images.isEmpty());
        QCOMPARE(images.at(0).toMap().value(QStringLiteral("kind")).toString(),
                 QStringLiteral("image"));
        QCOMPARE(images.at(0).toMap().value(QStringLiteral("insert")).toString(),
                 QStringLiteral("![[shot.png]]"));
        QCOMPARE(QFileInfo(backend.resolveNodeTarget(QStringLiteral("shot.png")).toLocalFile())
                     .fileName(),
                 QStringLiteral("shot.png"));

        const QVariantList headings = backend.linkSuggestions(QStringLiteral("note#方"), false);
        QVERIFY(!headings.isEmpty());
        QCOMPARE(headings.at(0).toMap().value(QStringLiteral("insert")).toString(),
                 QStringLiteral("[[note#方法]]"));
    }

    void mindmapAttachUrlReplacesPlaceholder() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        QFile note(directory.filePath(QStringLiteral("研究.md")));
        QVERIFY(note.open(QIODevice::WriteOnly | QIODevice::Text));
        note.write("# 研究\n");
        note.close();

        Backend backend;
        backend.openFolder(QUrl::fromLocalFile(directory.path()));
        backend.open(QUrl::fromLocalFile(directory.filePath(QStringLiteral("研究.md"))));
        backend.setEditorPlainText(
            QStringLiteral("---\nmindmap: true\n---\n\n# Root\n\n- 新節點\n"));
        backend.mindmapAttachUrl(QStringLiteral("0.0"),
                                 QUrl::fromLocalFile(directory.filePath(QStringLiteral("研究.md"))));
        QVERIFY(backend.editorPlainText().contains(QStringLiteral("[[研究]]")));
    }

    void workspaceTagsFilterRenameAndRecentFiles() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString acoustics = directory.filePath(QStringLiteral("acoustics.md"));
        QFile acousticsFile(acoustics);
        QVERIFY(acousticsFile.open(QIODevice::WriteOnly | QIODevice::Text));
        acousticsFile.write("---\ntags: [paper]\nstatus: writing\n---\n# Hall\n\n#fieldwork\n");
        acousticsFile.close();
        const QString other = directory.filePath(QStringLiteral("other.md"));
        QFile otherFile(other);
        QVERIFY(otherFile.open(QIODevice::WriteOnly | QIODevice::Text));
        otherFile.write("---\nstatus: draft\n---\n# Other\n");
        otherFile.close();

        Backend backend;
        backend.openFolder(QUrl::fromLocalFile(directory.path()));
        QCOMPARE(backend.workspaceFiles().size(), 2);

        QStringList tagNames;
        for (const QVariant &item : backend.workspaceTags())
            tagNames.append(item.toMap().value(QStringLiteral("name")).toString());
        QVERIFY(tagNames.contains(QStringLiteral("paper")));
        QVERIFY(tagNames.contains(QStringLiteral("fieldwork")));
        QVERIFY(!FrontMatter::allTags(QStringLiteral("---\n---\n<!--c:#81C784-->\n#keep\n"))
                     .contains(QStringLiteral("81C784")));
        QVERIFY(FrontMatter::allTags(QStringLiteral("---\n---\n<!--c:#81C784-->\n#keep\n"))
                    .contains(QStringLiteral("keep")));

        backend.setTagFilter(QStringLiteral("paper"));
        QCOMPARE(backend.tagFilter(), QStringLiteral("paper"));
        QCOMPARE(backend.workspaceFiles().size(), 1);
        QCOMPARE(backend.projectRecords().size(), 1);

        backend.setTagFilter(QStringLiteral("paper"));
        QVERIFY(backend.tagFilter().isEmpty());
        QCOMPARE(backend.workspaceFiles().size(), 2);

        backend.renameTag(QStringLiteral("paper"), QStringLiteral("journal"));
        QFile reread(acoustics);
        QVERIFY(reread.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString updated = QString::fromUtf8(reread.readAll());
        QVERIFY(FrontMatter::allTags(updated).contains(QStringLiteral("journal")));
        QVERIFY(!FrontMatter::allTags(updated).contains(QStringLiteral("paper")));

        backend.open(QUrl::fromLocalFile(acoustics));
        QCOMPARE(backend.documentOutline().size(), 1);
        QCOMPARE(backend.documentOutline().at(0).toMap().value(QStringLiteral("title")).toString(),
                 QStringLiteral("Hall"));
        QCOMPARE(backend.recentFiles().size(), 1);
        QCOMPARE(backend.recentFiles().at(0).toMap().value(QStringLiteral("name")).toString(),
                 QStringLiteral("acoustics.md"));

        backend.insertTag(QStringLiteral("review"));
        QFile tagged(acoustics);
        QVERIFY(tagged.open(QIODevice::ReadOnly | QIODevice::Text));
        QVERIFY(FrontMatter::allTags(QString::fromUtf8(tagged.readAll()))
                    .contains(QStringLiteral("review")));
    }

    void keepsCursorAndSelectionStableAcrossInsertions() {
        const QString mutationsPath = QFINDTESTDATA("../src/EditorMutations.js");
        QVERIFY(!mutationsPath.isEmpty());

        QQmlEngine engine;
        QQmlComponent component(&engine);
        const QByteArray harness = R"QML(
            import QtQuick
            import "EditorMutations.js" as EditorMutations

            TextEdit {
                property string insertionText
                property int insertionCursor
                property string wrappedText
                property int wrappedSelectionStart
                property int wrappedSelectionEnd

                Component.onCompleted: {
                    text = "alpha omega";
                    cursorPosition = 5;
                    EditorMutations.replaceRange(this, 5, 5, "one\r\ntwo");
                    insertionText = text;
                    insertionCursor = cursorPosition;

                    text = "alpha beta omega";
                    select(6, 10);
                    EditorMutations.replaceRange(this, selectionStart, selectionEnd,
                                                 "**beta**", 2, 6);
                    wrappedText = text;
                    wrappedSelectionStart = selectionStart;
                    wrappedSelectionEnd = selectionEnd;
                }
            }
        )QML";
        const QUrl harnessUrl = QUrl::fromLocalFile(
            QFileInfo(mutationsPath).absolutePath() + QStringLiteral("/MutationHarness.qml"));
        component.setData(harness, harnessUrl);
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> editor(component.create());
        QVERIFY2(editor, qPrintable(component.errorString()));

        QCOMPARE(editor->property("insertionText").toString(),
                 QStringLiteral("alphaone\ntwo omega"));
        QCOMPARE(editor->property("insertionCursor").toInt(), 12);
        QCOMPARE(editor->property("wrappedText").toString(),
                 QStringLiteral("alpha **beta** omega"));
        QCOMPARE(editor->property("wrappedSelectionStart").toInt(), 8);
        QCOMPARE(editor->property("wrappedSelectionEnd").toInt(), 12);
    }

    void showsWorkspacePreviewAndFooterActions() {
        const QString mainQmlPath = QFINDTESTDATA("../src/Main.qml");
        QVERIFY(!mainQmlPath.isEmpty());

        Backend backend;
        PluginHost pluginHost(&backend);
        QQmlEngine engine;
        engine.rootContext()->setContextProperty(QStringLiteral("backend"), &backend);
        engine.rootContext()->setContextProperty(QStringLiteral("pluginHost"), &pluginHost);
        QQmlComponent component(&engine, QUrl::fromLocalFile(mainQmlPath));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> window(component.create());
        QVERIFY2(window, qPrintable(component.errorString()));

        QVERIFY(window->findChild<QObject *>(QStringLiteral("sourceEditor")));
        QVERIFY(window->findChild<QObject *>(QStringLiteral("workspaceSidebar")));
        QVERIFY(window->findChild<QObject *>(QStringLiteral("renderedPreview")));
        QVERIFY(window->findChild<QObject *>(QStringLiteral("slashMenu")));
        QVERIFY(window->findChild<QObject *>(QStringLiteral("languagePicker")));
        QVERIFY(window->findChild<QObject *>(QStringLiteral("templatePicker")));
        QVERIFY(window->findChild<QObject *>(QStringLiteral("linksSidebar")));
        QVERIFY(window->findChild<QObject *>(QStringLiteral("backlinkList")));
        QVERIFY(window->findChild<QObject *>(QStringLiteral("outgoingList")));
        QVERIFY(window->findChild<QObject *>(QStringLiteral("projectViewBar")));
        QVERIFY(window->findChild<QObject *>(QStringLiteral("projectTableView")));
        QVERIFY(window->findChild<QObject *>(QStringLiteral("projectBoardView")));
        QVERIFY(window->findChild<QObject *>(QStringLiteral("projectCalendarView")));
        QVERIFY(window->findChild<QObject *>(QStringLiteral("mindmapView")));
        QVERIFY(window->findChild<QObject *>(QStringLiteral("projectCardsView")));
        QVERIFY(window->findChild<QObject *>(QStringLiteral("workspaceFolderSidebar")));
        QObject *addStatusButton = window->findChild<QObject *>(QStringLiteral("addStatusButton"));
        QVERIFY(addStatusButton);
        QCOMPARE(addStatusButton->property("text").toString(), QStringLiteral("Add"));
        QVERIFY(!window->findChild<QObject *>(QStringLiteral("modeToggle")));

        QObject *saveButton = window->findChild<QObject *>(QStringLiteral("saveButton"));
        QObject *openButton = window->findChild<QObject *>(QStringLiteral("openButton"));
        QObject *openFolderButton = window->findChild<QObject *>(QStringLiteral("openFolderButton"));
        QVERIFY(saveButton);
        QVERIFY(openButton);
        QVERIFY(openFolderButton);
        QVERIFY(window->findChild<QObject *>(QStringLiteral("commandPaletteShortcut")));
        QVERIFY(window->findChild<QObject *>(QStringLiteral("editThreadsTemplateButton")));
        QVERIFY(window->findChild<QObject *>(QStringLiteral("publishThreadsShortcut")));
        QVERIFY(window->findChild<QObject *>(QStringLiteral("publishThreadsButton")));
        QVERIFY(window->findChild<QObject *>(QStringLiteral("newMarkdownNoteShortcut")));
        QVERIFY(window->findChild<QObject *>(QStringLiteral("newMarkdownNoteButton")));
        QCOMPARE(window->findChild<QObject *>(QStringLiteral("newThreadsPostButton"))
                     ->property("text")
                     .toString(),
                 QStringLiteral("新帖"));
        QCOMPARE(window->findChild<QObject *>(QStringLiteral("newGardenNoteButton"))
                     ->property("text")
                     .toString(),
                 QStringLiteral("新花園"));
        QCOMPARE(window->findChild<QObject *>(QStringLiteral("newSlidevNoteButton"))
                     ->property("text")
                     .toString(),
                 QStringLiteral("新簡報"));
        QVERIFY(window->findChild<QObject *>(QStringLiteral("slidePreview")));
        QVERIFY(window->findChild<QObject *>(QStringLiteral("publishGardenButton")));
        QVERIFY(window->findChild<QObject *>(QStringLiteral("gardenPublishConfirmDialog")));
        QVERIFY(window->findChild<QObject *>(QStringLiteral("revealFileShortcut")));

        QObject *tableShortcut = window->findChild<QObject *>(QStringLiteral("viewTableShortcut"));
        QObject *boardShortcut = window->findChild<QObject *>(QStringLiteral("viewBoardShortcut"));
        QVERIFY(tableShortcut);
        QVERIFY(boardShortcut);
        QVERIFY(tableShortcut->property("enabled").toBool());
        QVERIFY(boardShortcut->property("enabled").toBool());
        QCOMPARE(backend.projectView(), QStringLiteral("editor"));
        QVERIFY(QMetaObject::invokeMethod(tableShortcut, "activated"));
        QCOMPARE(backend.projectView(), QStringLiteral("table"));
        QVERIFY(QMetaObject::invokeMethod(boardShortcut, "activated"));
        QCOMPARE(backend.projectView(), QStringLiteral("board"));
        QObject *mindmapShortcut = window->findChild<QObject *>(QStringLiteral("viewMindmapShortcut"));
        QVERIFY(mindmapShortcut);
        QVERIFY(QMetaObject::invokeMethod(mindmapShortcut, "activated"));
        QCOMPARE(backend.projectView(), QStringLiteral("mindmap"));
        QObject *cardsShortcut = window->findChild<QObject *>(QStringLiteral("viewCardsShortcut"));
        QVERIFY(cardsShortcut);
        QVERIFY(QMetaObject::invokeMethod(cardsShortcut, "activated"));
        QCOMPARE(backend.projectView(), QStringLiteral("cards"));

        const QString barPath = QFINDTESTDATA("../src/ProjectViewBar.qml");
        QVERIFY(!barPath.isEmpty());
        QFile barFile(barPath);
        QVERIFY(barFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString bar = QString::fromUtf8(barFile.readAll());
        const int mindmapAt = bar.indexOf(QStringLiteral("id: \"mindmap\""));
        const int cardsAt = bar.indexOf(QStringLiteral("id: \"cards\""));
        QVERIFY(mindmapAt > 0);
        QVERIFY(cardsAt > mindmapAt);
    }

    void scalesTextWithDesktopTextSize() {
        const QString mainQmlPath = QFINDTESTDATA("../src/Main.qml");
        QVERIFY(!mainQmlPath.isEmpty());

        Backend backend;
        PluginHost pluginHost(&backend);
        QQmlEngine engine;
        engine.rootContext()->setContextProperty(QStringLiteral("backend"), &backend);
        engine.rootContext()->setContextProperty(QStringLiteral("pluginHost"), &pluginHost);
        QQmlComponent component(&engine, QUrl::fromLocalFile(mainQmlPath));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> window(component.create());
        QVERIFY2(window, qPrintable(component.errorString()));

        QObject *editor = window->findChild<QObject *>(QStringLiteral("sourceEditor"));
        QVERIFY(editor);
        QCOMPARE(editor->property("font").value<QFont>().pixelSize(), 20);

        // `omarchy display text size 16` sets the GNOME factor to 16/12.
        backend.setTextScale(16.0 / 12.0);
        QCOMPARE(window->property("editorFontPixelSize").toInt(), 27);
        QCOMPARE(editor->property("font").value<QFont>().pixelSize(), 27);

        backend.setTextScale(9.0 / 12.0);
        QCOMPARE(window->property("editorFontPixelSize").toInt(), 15);
        QCOMPARE(editor->property("font").value<QFont>().pixelSize(), 15);
    }

    void restoresLastWorkspaceAndRemembersRecentProjects() {
        QTemporaryDir firstDir;
        QTemporaryDir secondDir;
        QVERIFY(firstDir.isValid());
        QVERIFY(secondDir.isValid());
        QVERIFY(QDir(firstDir.path()).mkdir(QStringLiteral("keep")));
        QVERIFY(QDir(secondDir.path()).mkdir(QStringLiteral("keep")));

        const QString notePath = firstDir.filePath(QStringLiteral("note.md"));
        QFile note(notePath);
        QVERIFY(note.open(QIODevice::WriteOnly | QIODevice::Text));
        note.write("# Hall\n");
        note.close();

        Backend first;
        QCOMPARE(first.workspaceFolderPath(), QString());
        first.openFolder(QUrl::fromLocalFile(firstDir.path()));
        QCOMPARE(first.recentWorkspaces().size(), 1);
        QCOMPARE(first.recentWorkspaces().at(0).toMap().value(QStringLiteral("path")).toString(),
                 QDir(firstDir.path()).absolutePath());
        first.open(QUrl::fromLocalFile(notePath));
        first.openFolder(QUrl::fromLocalFile(secondDir.path()));
        QCOMPARE(first.workspaceFolderPath(), QDir(secondDir.path()).absolutePath());
        QCOMPARE(first.recentWorkspaces().size(), 2);
        QCOMPARE(first.recentWorkspaces().at(0).toMap().value(QStringLiteral("path")).toString(),
                 QDir(secondDir.path()).absolutePath());

        Backend second;
        QCOMPARE(second.workspaceFolderPath(), QString());
        QCOMPARE(second.recentWorkspaces().size(), 2);
        second.restoreLastSession();
        QCOMPARE(second.workspaceFolderPath(), QDir(secondDir.path()).absolutePath());
        QVERIFY(second.fileUrl().isEmpty() || !second.fileUrl().isValid()
                || !second.currentFilePath().endsWith(QStringLiteral("note.md")));

        QSettings().setValue(QStringLiteral("workspace/lastDirectory"),
                             QDir(firstDir.path()).absolutePath());
        Backend third;
        third.restoreLastSession();
        QCOMPARE(third.workspaceFolderPath(), QDir(firstDir.path()).absolutePath());
        QCOMPARE(third.currentFilePath(), QFileInfo(notePath).absoluteFilePath());
    }

    void remembersLastSaveDirectory() {
        QTemporaryDir saveDirectory;
        QVERIFY(saveDirectory.isValid());

        const QString savedPath = saveDirectory.filePath(QStringLiteral("first.md"));
        Backend savedDocument;
        savedDocument.saveAs(QUrl::fromLocalFile(savedPath));

        Backend nextDocument;
        QSignalSpy saveDialogSpy(&nextDocument, &Backend::saveDialogRequested);
        nextDocument.saveAsDialog();
        QCOMPARE(saveDialogSpy.count(), 1);

        const QUrl suggestedUrl = saveDialogSpy.takeFirst().constFirst().toUrl();
        QCOMPARE(QFileInfo(suggestedUrl.toLocalFile()).absolutePath(),
                 saveDirectory.path());
        QCOMPARE(QFileInfo(suggestedUrl.toLocalFile()).fileName(),
                 QStringLiteral("Untitled.md"));

        QSettings().setValue(QStringLiteral("file/lastSaveDirectory"),
                             saveDirectory.filePath(QStringLiteral("missing")));
        Backend fallbackDocument;
        QSignalSpy fallbackDialogSpy(&fallbackDocument, &Backend::saveDialogRequested);
        fallbackDocument.saveAsDialog();
        const QUrl fallbackUrl = fallbackDialogSpy.takeFirst().constFirst().toUrl();
        QCOMPARE(QFileInfo(fallbackUrl.toLocalFile()).absolutePath(), QDir::homePath());
    }

    void persistsPaneVisibilityPreferences() {
        Backend backend;
        QVERIFY(backend.property("sidebarVisible").isValid());
        QVERIFY(backend.property("previewVisible").isValid());
        QCOMPARE(backend.property("sidebarVisible").toBool(), true);
        QCOMPARE(backend.property("previewVisible").toBool(), true);

        QVERIFY(backend.setProperty("sidebarVisible", false));
        QVERIFY(backend.setProperty("previewVisible", false));
        QCOMPARE(backend.property("sidebarVisible").toBool(), false);
        QCOMPARE(backend.property("previewVisible").toBool(), false);

        Backend restored;
        QCOMPARE(restored.property("sidebarVisible").toBool(), false);
        QCOMPARE(restored.property("previewVisible").toBool(), false);
    }

    void showsPreferencesDialogAndPaneRevealControls() {
        const QString mainQmlPath = QFINDTESTDATA("../src/Main.qml");
        QVERIFY(!mainQmlPath.isEmpty());

        Backend backend;
        PluginHost pluginHost(&backend);
        QVERIFY(backend.setProperty("sidebarVisible", false));
        QVERIFY(backend.setProperty("previewVisible", false));

        QQmlEngine engine;
        engine.rootContext()->setContextProperty(QStringLiteral("backend"), &backend);
        engine.rootContext()->setContextProperty(QStringLiteral("pluginHost"), &pluginHost);
        QQmlComponent component(&engine, QUrl::fromLocalFile(mainQmlPath));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> window(component.create());
        QVERIFY2(window, qPrintable(component.errorString()));

        QObject *preferencesDialog = window->findChild<QObject *>(QStringLiteral("preferencesDialog"));
        QObject *preferencesButton = window->findChild<QObject *>(QStringLiteral("preferencesButton"));
        QObject *preferencesShortcut = window->findChild<QObject *>(QStringLiteral("preferencesShortcut"));
        QObject *toggleSidebarShortcut = window->findChild<QObject *>(QStringLiteral("toggleSidebarShortcut"));
        QObject *togglePreviewShortcut = window->findChild<QObject *>(QStringLiteral("togglePreviewShortcut"));
        QObject *showSidebarButton = window->findChild<QObject *>(QStringLiteral("showSidebarButton"));
        QObject *showPreviewButton = window->findChild<QObject *>(QStringLiteral("showPreviewButton"));
        QObject *shortcutText = window->findChild<QObject *>(QStringLiteral("shortcutText"));

        QVERIFY(preferencesDialog);
        QVERIFY(window->findChild<QObject *>(QStringLiteral("preferencesFlick")));
        QVERIFY(window->findChild<QObject *>(QStringLiteral("uiLanguageBox")));
        QVERIFY(preferencesButton);
        QVERIFY(preferencesShortcut);
        QVERIFY(toggleSidebarShortcut);
        QVERIFY(togglePreviewShortcut);
        QVERIFY(showSidebarButton);
        QVERIFY(showPreviewButton);
        QVERIFY(shortcutText);
        QVERIFY(showSidebarButton->property("visible").toBool());
        QVERIFY(showPreviewButton->property("visible").toBool());
        QCOMPARE(window->findChild<QObject *>(QStringLiteral("hideSidebarButton"))
                     ->property("text")
                     .toString(),
                 backend.t(QStringLiteral("hide")));

        const QStringList preferenceSequences = shortcutSequences(preferencesShortcut);
        QVERIFY(preferenceSequences.contains(QStringLiteral("Ctrl+,")));

        const QStringList sidebarSequences = shortcutSequences(toggleSidebarShortcut);
        QVERIFY(sidebarSequences.contains(QStringLiteral("Ctrl+Shift+L")));

        const QStringList previewSequences = shortcutSequences(togglePreviewShortcut);
        QVERIFY(previewSequences.contains(QStringLiteral("Ctrl+Shift+P")));

        const QString shortcutSummary = shortcutText->property("text").toString();
        QVERIFY(shortcutSummary.contains(QStringLiteral("Ctrl/Cmd+,  Preferences")));
        QVERIFY(shortcutSummary.contains(QStringLiteral("Ctrl/Cmd+Shift+L  Toggle Sidebar")));
        QVERIFY(shortcutSummary.contains(QStringLiteral("Ctrl/Cmd+Shift+P  Toggle Preview")));
        QVERIFY(shortcutSummary.contains(QStringLiteral("Ctrl/Cmd+P  Command palette")));
        QVERIFY(shortcutSummary.contains(QStringLiteral("/ then Code  Insert code block")));
        QVERIFY(shortcutSummary.contains(QStringLiteral("Ctrl/Cmd+Shift+Enter  Publish Threads now")));

        QObject *commandPaletteShortcut =
            window->findChild<QObject *>(QStringLiteral("commandPaletteShortcut"));
        QVERIFY(commandPaletteShortcut);
        const QStringList paletteSequences = shortcutSequences(commandPaletteShortcut);
        QVERIFY(paletteSequences.contains(QStringLiteral("Ctrl+P")));

        QVERIFY(QMetaObject::invokeMethod(preferencesShortcut, "activated"));
        QCoreApplication::processEvents();
        QVERIFY(preferencesDialog->property("visible").toBool());

        QVERIFY(QMetaObject::invokeMethod(showSidebarButton, "clicked"));
        QVERIFY(QMetaObject::invokeMethod(showPreviewButton, "clicked"));
        QCOMPARE(backend.property("sidebarVisible").toBool(), true);
        QCOMPARE(backend.property("previewVisible").toBool(), true);
    }

    void parsesFencedCodeAndSlashQueries() {
        const QString source = QStringLiteral(
            "---\ntitle: Demo\n---\n\nhello\n\n```python\nprint(1)\n```\n\n"
            "```mermaid\ngraph TD; A-->B;\n```\n");
        const QList<CodeBlocks::Fence> fences = CodeBlocks::parse(
            FrontMatter::parse(source).body);
        QCOMPARE(fences.size(), 2);
        QCOMPARE(fences.at(0).language, QStringLiteral("python"));
        QCOMPARE(fences.at(0).code, QStringLiteral("print(1)"));
        QCOMPARE(fences.at(1).language, QStringLiteral("mermaid"));

        const QVariantList blocks = CodeBlocks::previewBlocks(source);
        QCOMPARE(blocks.size(), 3);
        QCOMPARE(blocks.at(0).toMap().value(QStringLiteral("kind")).toString(),
                 QStringLiteral("markdown"));
        QCOMPARE(blocks.at(1).toMap().value(QStringLiteral("language")).toString(),
                 QStringLiteral("python"));
        QVERIFY(blocks.at(2).toMap().value(QStringLiteral("mermaid")).toBool());
        QVERIFY(blocks.at(0).toMap().value(QStringLiteral("to")).toInt()
                > blocks.at(0).toMap().value(QStringLiteral("from")).toInt());
        QCOMPARE(blocks.at(2).toMap().value(QStringLiteral("from")).toInt(),
                 source.lastIndexOf(QStringLiteral("```mermaid")));

        Backend previewBackend;
        previewBackend.setEditorPlainText(QStringLiteral("hello\n"));
        QCOMPARE(previewBackend.previewBlockCount(), 1);
        const int rev = previewBackend.previewRevision();
        previewBackend.setEditorPlainText(QStringLiteral("hello world\n"));
        QCOMPARE(previewBackend.previewBlockCount(), 1);
        QVERIFY(previewBackend.previewRevision() > rev);
        QCOMPARE(previewBackend.previewBlockAt(0).value(QStringLiteral("kind")).toString(),
                 QStringLiteral("markdown"));
        QCOMPARE(previewBackend.previewBlockIndexAt(0), 0);

        QQmlEngine previewEngine;
        previewEngine.rootContext()->setContextProperty(QStringLiteral("backend"),
                                                        &previewBackend);
        QQmlComponent previewHarness(&previewEngine);
        previewHarness.setData(QByteArray(R"QML(
            import QtQuick
            Item {
                id: root
                property string shown: ""
                Repeater {
                    model: backend.previewBlockCount
                    delegate: Item {
                        required property int index
                        Connections {
                            target: backend
                            function onPreviewRevisionChanged() {
                                root.shown = String(backend.previewBlockAt(index).text || "")
                            }
                        }
                        Component.onCompleted: {
                            root.shown = String(backend.previewBlockAt(index).text || "")
                        }
                    }
                }
            }
        )QML"), QUrl(QStringLiteral("qrc:/PreviewRevisionHarness.qml")));
        QVERIFY2(previewHarness.isReady(), qPrintable(previewHarness.errorString()));
        QScopedPointer<QObject> previewRoot(previewHarness.create());
        QVERIFY2(previewRoot, qPrintable(previewHarness.errorString()));
        QCOMPARE(previewRoot->property("shown").toString(),
                 previewBackend.previewBlockAt(0).value(QStringLiteral("text")).toString());
        previewBackend.setEditorPlainText(QStringLiteral("hello world again\n"));
        QCOMPARE(previewBackend.previewBlockCount(), 1);
        QVERIFY(previewRoot->property("shown").toString().contains(QStringLiteral("again")));

        previewBackend.setEditorPlainText(
            QStringLiteral("hello\n\n```python\nprint(1)\n```\n\ntail\n"));
        QCOMPARE(previewBackend.previewBlockCount(), 3);
        QCOMPARE(previewBackend.previewBlockIndexAt(0), 0);
        const int tailAt = previewBackend.editorPlainText().indexOf(QStringLiteral("tail"));
        QCOMPARE(previewBackend.previewBlockIndexAt(tailAt), 2);
        QCOMPARE(previewBackend.previewBlockIndexAt(previewBackend.editorPlainText().size()), 2);

        const QVariantMap slash = CodeBlocks::slashQueryAt(QStringLiteral("/co"), 3);
        QVERIFY(slash.value(QStringLiteral("active")).toBool());
        QCOMPARE(slash.value(QStringLiteral("query")).toString(), QStringLiteral("co"));
        QVERIFY(!CodeBlocks::slashQueryAt(QStringLiteral("http://x"), 8)
                     .value(QStringLiteral("active")).toBool());

        const QVariantMap tick = CodeBlocks::backtickTriggerAt(QStringLiteral("```"), 3);
        QVERIFY(tick.value(QStringLiteral("active")).toBool());
        QVERIFY(!CodeBlocks::backtickTriggerAt(QStringLiteral("```python"), 9)
                     .value(QStringLiteral("active")).toBool());

        const QString fence = CodeBlocks::fenceText(QStringLiteral("python"), QString());
        QCOMPARE(fence, QStringLiteral("```python\n\n```"));
        QCOMPARE(CodeBlocks::fenceCaretOffset(QStringLiteral("python"), QString()), 10);

        const QVariantList languages = CodeBlocks::languages(QStringLiteral("mer"));
        QVERIFY(!languages.isEmpty());
        QCOMPARE(languages.at(0).toMap().value(QStringLiteral("id")).toString(),
                 QStringLiteral("mermaid"));

        const QVariantList commands = CodeBlocks::slashCommands(QStringLiteral("code"));
        QCOMPARE(commands.size(), 1);
        QCOMPARE(commands.at(0).toMap().value(QStringLiteral("id")).toString(),
                 QStringLiteral("code"));

        const QVariantList templateCommands = CodeBlocks::slashCommands(QStringLiteral("template"));
        QCOMPARE(templateCommands.size(), 1);
        QCOMPARE(templateCommands.at(0).toMap().value(QStringLiteral("id")).toString(),
                 QStringLiteral("template"));
    }

    void highlightsFencedCodeBlocks() {
        QTextEdit editor;
        MarkdownHighlighter highlighter(editor.document());
        editor.setPlainText(QStringLiteral("hi\n```python\nprint(1)\n```\n"));
        highlighter.rehighlight();

        QTextBlock open = editor.document()->begin().next();
        QCOMPARE(open.text(), QStringLiteral("```python"));
        QCOMPARE(open.userState(), 1);
        QTextBlock inner = open.next();
        QCOMPARE(inner.text(), QStringLiteral("print(1)"));
        QCOMPARE(inner.userState(), 1);
        QTextBlock close = inner.next();
        QCOMPARE(close.text(), QStringLiteral("```"));
        QCOMPARE(close.userState(), 0);

        const QString source = editor.toPlainText();
        const QVariantMap leave = CodeBlocks::leaveFenceAt(
            source, source.indexOf(QStringLiteral("print")) + 2);
        QVERIFY(leave.value(QStringLiteral("active")).toBool());
        QVERIFY(!leave.value(QStringLiteral("above")).toBool());
        const QVariantMap above = CodeBlocks::leaveFenceAt(
            source, source.indexOf(QStringLiteral("```python")));
        QVERIFY(above.value(QStringLiteral("above")).toBool());
    }

    void skipsUnsavedPromptForBlankUntitled() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        Backend backend;
        backend.openFolder(QUrl::fromLocalFile(directory.path()));
        const QUrl created = backend.createMarkdownNote();
        QVERIFY(created.isLocalFile());
        backend.open(created);
        QVERIFY(backend.isUntitledDocument());
        QVERIFY(backend.isBlankDocument());
        backend.setEditorPlainText(QString());
        QVERIFY(backend.modified());
        QVERIFY(!backend.shouldPromptForUnsaved());
        backend.discardBlankUntitled();
        QVERIFY(!QFileInfo::exists(created.toLocalFile()));
        QVERIFY(!backend.modified());
    }

    void renameNoteMovesMarkdownAndAssets() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString source = directory.filePath(QStringLiteral("note.md"));
        QFile file(source);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write("# Note\n");
        file.close();
        QVERIFY(QDir().mkpath(directory.filePath(QStringLiteral("note.assets"))));
        QFile asset(directory.filePath(QStringLiteral("note.assets/pic.png")));
        QVERIFY(asset.open(QIODevice::WriteOnly));
        asset.write("x");
        asset.close();

        Backend backend;
        backend.openFolder(QUrl::fromLocalFile(directory.path()));
        QVERIFY(backend.renameNote(QUrl::fromLocalFile(source), QStringLiteral("Hello")));
        QVERIFY(QFileInfo::exists(directory.filePath(QStringLiteral("Hello.md"))));
        QVERIFY(!QFileInfo::exists(source));
        QVERIFY(QFileInfo::exists(directory.filePath(QStringLiteral("Hello.assets/pic.png"))));
        QVERIFY(!QFileInfo::exists(directory.filePath(QStringLiteral("note.assets"))));

        QFile collision(directory.filePath(QStringLiteral("Taken.md")));
        QVERIFY(collision.open(QIODevice::WriteOnly | QIODevice::Text));
        collision.write("taken\n");
        collision.close();
        QVERIFY(backend.renameNote(QUrl::fromLocalFile(directory.filePath(QStringLiteral("Hello.md"))),
                                   QStringLiteral("Taken")));
        QVERIFY(QFileInfo::exists(directory.filePath(QStringLiteral("Taken-2.md"))));
        QVERIFY(QFileInfo::exists(directory.filePath(QStringLiteral("Taken.md"))));
    }

    void untitledNoteTakesHeadingAsFilenameOnSave() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        Backend backend;
        backend.openFolder(QUrl::fromLocalFile(directory.path()));
        const QUrl created = backend.createMarkdownNote();
        QVERIFY(created.isLocalFile());
        backend.open(created);
        backend.setEditorPlainText(QStringLiteral("# 會議紀錄\n\n內容\n"));
        backend.save();
        QCOMPARE(QFileInfo(backend.fileUrl().toLocalFile()).fileName(),
                 QStringLiteral("會議紀錄.md"));
        QVERIFY(!QFileInfo::exists(created.toLocalFile()));
        QVERIFY(QFileInfo::exists(directory.filePath(QStringLiteral("會議紀錄.md"))));

        const QUrl second = backend.createMarkdownNote();
        backend.open(second);
        backend.setEditorPlainText(QStringLiteral("# 會議紀錄\n"));
        backend.save();
        QCOMPARE(QFileInfo(backend.fileUrl().toLocalFile()).fileName(),
                 QStringLiteral("會議紀錄-2.md"));
    }

    void customizesAndResetsHotkeys() {
        Backend backend;
        QCOMPARE(backend.hotkey(QStringLiteral("save")), QStringLiteral("Ctrl+S"));
        backend.setHotkey(QStringLiteral("save"), QStringLiteral("Ctrl+Shift+S"));
        QCOMPARE(backend.hotkey(QStringLiteral("save")), QStringLiteral("Ctrl+Shift+S"));
        QVERIFY(backend.hotkeyRows().at(0).toMap().contains(QStringLiteral("title")));
        backend.resetHotkey(QStringLiteral("save"));
        QCOMPARE(backend.hotkey(QStringLiteral("save")), QStringLiteral("Ctrl+S"));
        backend.clearHotkey(QStringLiteral("save"));
        QVERIFY(backend.hotkey(QStringLiteral("save")).isEmpty());
        backend.resetHotkey(QStringLiteral("save"));
        QCOMPARE(backend.hotkeyDisplay(QStringLiteral("Ctrl+S")).isEmpty(), false);
    }

    void canDisableUnsavedPrompt() {
        Backend backend;
        QVERIFY(backend.confirmUnsavedChanges());
        backend.setConfirmUnsavedChanges(false);
        QVERIFY(!backend.confirmUnsavedChanges());
        QVERIFY(!backend.shouldPromptForUnsaved());
    }

    void copiesCodeBlockText() {
        Backend backend;
        backend.copyText(QStringLiteral("print(1)"));
        QCOMPARE(QGuiApplication::clipboard()->text(), QStringLiteral("print(1)"));
        QCOMPARE(backend.status(), QStringLiteral("Copied"));
    }

    void extractsWikiTargetsSkippingFences() {
        const QStringList targets = MindMap::wikiTargets(QStringLiteral(
            "---\ntitle: \"[[nope]]\"\n---\n\nsee [[alive]]\n\n```\n[[dead]]\n```\n\n[[also|別名]]\n"));
        QCOMPARE(targets, QStringList() << QStringLiteral("alive") << QStringLiteral("also"));
    }

    void rendersTemplaterDateTitleAndFilename() {
        const QString rendered = Backend::renderTemplateText(
            QStringLiteral("# {{title}}\n{{filename}}\n{{date}} {{time}}\n{{date:YYYY-MM-DD}}"),
            QStringLiteral("Hello"), QStringLiteral("Hello.md"));
        QVERIFY(rendered.startsWith(QStringLiteral("# Hello\nHello.md\n")));
        const QString today = QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd"));
        QVERIFY(rendered.contains(today));
        QCOMPARE(rendered.count(today), 2);
        QVERIFY(QRegularExpression(QStringLiteral("\\d{2}:\\d{2}")).match(rendered).hasMatch());
    }

    void listsBacklinksOutgoingAndTemplates() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString root = directory.path();
        QVERIFY(QDir().mkpath(directory.filePath(QStringLiteral("sub"))));
        QVERIFY(QDir().mkpath(directory.filePath(QStringLiteral("_templates"))));
        QVERIFY(QDir().mkpath(directory.filePath(QStringLiteral(".fmd/templates"))));

        QFile target(directory.filePath(QStringLiteral("target.md")));
        QVERIFY(target.open(QIODevice::WriteOnly | QIODevice::Text));
        target.write("# Target\n\n[[self-should-not-count]]\nsee [[missing]]\n");
        target.close();

        QFile from(directory.filePath(QStringLiteral("from.md")));
        QVERIFY(from.open(QIODevice::WriteOnly | QIODevice::Text));
        from.write("# From\n\nLink to [[target]] and a fence:\n```\n[[target]]\n```\n");
        from.close();

        QFile relative(directory.filePath(QStringLiteral("sub/note.md")));
        QVERIFY(relative.open(QIODevice::WriteOnly | QIODevice::Text));
        relative.write("# Nested\n\n[[../target]]\n");
        relative.close();

        QFile extraTemplate(directory.filePath(QStringLiteral(".fmd/templates/meeting.md")));
        QVERIFY(extraTemplate.open(QIODevice::WriteOnly | QIODevice::Text));
        extraTemplate.write("# {{title}} meeting\n");
        extraTemplate.close();

        Backend backend;
        backend.openFolder(QUrl::fromLocalFile(root));
        const QUrl defaultTemplate = backend.ensureDefaultTemplate();
        QVERIFY(defaultTemplate.isLocalFile());
        QVERIFY(QFileInfo::exists(directory.filePath(QStringLiteral("_templates/note.md"))));
        QCOMPARE(backend.templateFiles().size(), 2);

        backend.open(QUrl::fromLocalFile(directory.filePath(QStringLiteral("target.md"))));
        QCOMPARE(backend.backlinks().size(), 2);
        QStringList backNames;
        for (const QVariant &item : backend.backlinks())
            backNames.append(item.toMap().value(QStringLiteral("name")).toString());
        QVERIFY(backNames.contains(QStringLiteral("from.md")));
        QVERIFY(backNames.contains(QStringLiteral("sub/note.md")));

        QCOMPARE(backend.outgoingLinks().size(), 2);
        QStringList outgoingTargets;
        for (const QVariant &item : backend.outgoingLinks())
            outgoingTargets.append(item.toMap().value(QStringLiteral("target")).toString());
        QVERIFY(outgoingTargets.contains(QStringLiteral("self-should-not-count")));
        QVERIFY(outgoingTargets.contains(QStringLiteral("missing")));

        const QUrl created = backend.createNoteFromTemplate(defaultTemplate);
        QVERIFY(created.isLocalFile());
        QCOMPARE(QFileInfo(created.toLocalFile()).fileName(), QStringLiteral("Untitled.md"));
        QFile createdFile(created.toLocalFile());
        QVERIFY(createdFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString createdText = QString::fromUtf8(createdFile.readAll());
        QVERIFY(createdText.startsWith(QStringLiteral("# Untitled\n")));
        QVERIFY(createdText.contains(QDate::currentDate().toString(QStringLiteral("yyyy-MM-dd"))));
    }

    void canDisableInstalledPlugin() {
        Backend backend;
        PluginHost host(&backend);
        const QString manifest = QStringLiteral(
            "{\"id\":\"test.toggle\",\"name\":\"Toggle\",\"version\":\"0.1.0\","
            "\"main\":\"main.js\",\"permissions\":[]}");
        const QString js = QStringLiteral(
            "module.exports = class Toggle { onload() {} };\n");
        QVERIFY(host.loadPluginFromMemory(QStringLiteral("test.toggle"), manifest, js));
        QVERIFY(host.loadedPluginIds().contains(QStringLiteral("test.toggle")));
        host.setPluginEnabled(QStringLiteral("test.toggle"), false);
        QVERIFY(!host.loadedPluginIds().contains(QStringLiteral("test.toggle")));
        QCOMPARE(host.loadedPlugins().at(0).toMap().value(QStringLiteral("enabled")).toBool(),
                 false);
    }

    void tidiesMarkdownThroughPluginCommand() {
        const QString mainPath =
            QFINDTESTDATA("../src/plugins/fmd.core.markdown-tidy/main.js");
        const QString manifestPath =
            QFINDTESTDATA("../src/plugins/fmd.core.markdown-tidy/manifest.json");
        QVERIFY(!mainPath.isEmpty());
        QVERIFY(!manifestPath.isEmpty());
        QFile mainFile(mainPath);
        QFile manifestFile(manifestPath);
        QVERIFY(mainFile.open(QIODevice::ReadOnly | QIODevice::Text));
        QVERIFY(manifestFile.open(QIODevice::ReadOnly | QIODevice::Text));

        Backend backend;
        PluginHost host(&backend);
        backend.setEditorPlainText(QStringLiteral(
            "hello  \n\n\n\nworld  \n\n```\nkeep\n\n\nblank\n```\n"));
        QVERIFY(host.loadPluginFromMemory(
            QStringLiteral("fmd.core.markdown-tidy"),
            QString::fromUtf8(manifestFile.readAll()),
            QString::fromUtf8(mainFile.readAll())));
        QVERIFY(host.loadedPluginIds().contains(QStringLiteral("fmd.core.markdown-tidy")));
        bool hasTidyCommand = false;
        for (const QVariant &item : host.commands()) {
            if (item.toMap().value(QStringLiteral("id")).toString()
                == QStringLiteral("fmd.core.markdown-tidy.tidy"))
                hasTidyCommand = true;
        }
        QVERIFY(hasTidyCommand);
        host.invokeCommand(QStringLiteral("fmd.core.markdown-tidy.tidy"));
        QCOMPARE(backend.editorPlainText(),
                 QStringLiteral("hello\n\nworld\n\n```\nkeep\n\n\nblank\n```\n"));
        QCOMPARE(backend.status(), QStringLiteral("Tidied Markdown"));
        host.invokeCommand(QStringLiteral("fmd.core.markdown-tidy.tidy"));
        QCOMPARE(backend.status(), QStringLiteral("Already tidy"));
    }

    void loadsJavascriptPluginFromMemory() {
        Backend backend;
        PluginHost host(&backend);
        const QString manifest = QStringLiteral(
            "{\"id\":\"test.hello\",\"name\":\"Hello\",\"version\":\"0.1.0\","
            "\"main\":\"main.js\",\"permissions\":[\"ui.statusbar\",\"editor.read\"]}");
        const QString js = QStringLiteral(
            "module.exports = class Hello {\n"
            "  onload(api) {\n"
            "    api.ui.setStatusBarItem({ id: 'hello', text: 'plugin-ok' });\n"
            "  }\n"
            "};\n");
        QVERIFY(host.loadPluginFromMemory(QStringLiteral("test.hello"), manifest, js));
        QVERIFY(host.loadedPluginIds().contains(QStringLiteral("test.hello")));
        QCOMPARE(host.pluginStatusText(), QStringLiteral("plugin-ok"));
    }

    void slashCommandsIncludeTableTocAndColor() {
        auto idsFor = [](const QString &query) {
            QStringList ids;
            for (const QVariant &item : CodeBlocks::slashCommands(query))
                ids.append(item.toMap().value(QStringLiteral("id")).toString());
            return ids;
        };
        QVERIFY(idsFor(QStringLiteral("table")).contains(QStringLiteral("table")));
        QCOMPARE(idsFor(QStringLiteral("toc")), QStringList{QStringLiteral("toc")});
        QVERIFY(idsFor(QStringLiteral("highlight")).contains(QStringLiteral("highlight")));
        QVERIFY(idsFor(QStringLiteral("color")).contains(QStringLiteral("color")));
    }

    void rendersDynamicTocFromHeadings() {
        const QString source = QStringLiteral(
            "---\ntitle: Doc\n---\n"
            "# Title\n\n"
            "```toc\nmin_depth: 2\nmax_depth: 3\n```\n\n"
            "## One\n### Nested\n#### Deep\n## Two\n");
        const QString toc = CodeBlocks::renderToc(
            source, QStringLiteral("min_depth: 2\nmax_depth: 3"));
        QVERIFY(toc.contains(QStringLiteral("- One")));
        QVERIFY(toc.contains(QStringLiteral("  - Nested")));
        QVERIFY(!toc.contains(QStringLiteral("Deep")));
        QVERIFY(toc.contains(QStringLiteral("- Two")));

        const QVariantList blocks = CodeBlocks::previewBlocks(source);
        bool sawTocMarkdown = false;
        for (const QVariant &item : blocks) {
            const QVariantMap block = item.toMap();
            if (block.value(QStringLiteral("kind")).toString() == QLatin1String("markdown")
                && block.value(QStringLiteral("text")).toString().contains(QStringLiteral("- One")))
                sawTocMarkdown = true;
        }
        QVERIFY(sawTocMarkdown);
    }

    void pluginApiReplaceSelectionFrontmatterAndVaultList() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        QFile a(directory.filePath(QStringLiteral("a.md")));
        QVERIFY(a.open(QIODevice::WriteOnly | QIODevice::Text));
        a.write("alpha\n");
        a.close();
        QFile b(directory.filePath(QStringLiteral("b.md")));
        QVERIFY(b.open(QIODevice::WriteOnly | QIODevice::Text));
        b.write("beta\n");
        b.close();

        Backend backend;
        backend.openFolder(QUrl::fromLocalFile(directory.path()));
        backend.setEditorPlainText(QStringLiteral("---\nstatus: draft\n---\nhello WORLD\n"));
        PluginHost host(&backend);
        host.setEditorSelectionRange(
            backend.editorPlainText().indexOf(QStringLiteral("WORLD")),
            backend.editorPlainText().indexOf(QStringLiteral("WORLD")) + 5);

        const QString manifest = QStringLiteral(
            "{\"id\":\"test.api\",\"name\":\"Api\",\"version\":\"0.1.0\","
            "\"main\":\"main.js\",\"permissions\":["
            "\"editor.read\",\"editor.write\",\"vault.read\",\"frontmatter.write\","
            "\"commands.register\"]}");
        const QString js = QStringLiteral(
            "module.exports = class Api {\n"
            "  onload(api) {\n"
            "    this.api = api;\n"
            "    api.commands.register({ id: 'go', name: 'Go', callback: function() {\n"
            "      api.editor.replaceSelection('there');\n"
            "      api.frontmatter.set('status', 'done');\n"
            "      var listed = api.vault.list();\n"
            "      api.ui.notice('listed:' + listed.length + ':' + api.editor.getSelection());\n"
            "    }});\n"
            "  }\n"
            "};\n");
        QVERIFY(host.loadPluginFromMemory(QStringLiteral("test.api"), manifest, js));
        host.invokeCommand(QStringLiteral("test.api.go"));
        QVERIFY(backend.editorPlainText().contains(QStringLiteral("hello there")));
        QCOMPARE(FrontMatter::parse(backend.editorPlainText())
                     .fields.value(QStringLiteral("status")).toString(),
                 QStringLiteral("done"));
        QVERIFY(backend.status().startsWith(QStringLiteral("listed:")));
        QVERIFY(backend.status().contains(QStringLiteral("listed:2")));
    }

    void communityPluginsStayUnloadedUntilEnabled() {
        Backend backend;
        PluginHost host(&backend);
        QVERIFY(!host.communityPluginsEnabled());
        host.setCommunityPluginsEnabled(true);
        QVERIFY(host.communityPluginsEnabled());
        const QString manifest = QStringLiteral(
            "{\"id\":\"test.community\",\"name\":\"Community\",\"version\":\"0.1.0\","
            "\"main\":\"main.js\",\"permissions\":[]}");
        const QString js = QStringLiteral(
            "module.exports = class Community { onload() {} };\n");
        QVERIFY(host.loadPluginFromMemory(QStringLiteral("test.community"), manifest, js));
        QVERIFY(host.loadedPluginIds().contains(QStringLiteral("test.community")));
        host.setCommunityPluginsEnabled(false);
        QVERIFY(!host.communityPluginsEnabled());
        QVERIFY(!host.loadedPluginIds().contains(QStringLiteral("test.community")));
    }

    void libraryFiltersByKeywordAndDate() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        auto writeNote = [](const QString &path, const QString &contents) {
            QFile file(path);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
                return false;
            file.write(contents.toUtf8());
            return true;
        };
        const QString today = QDate::currentDate().toString(Qt::ISODate);
        QVERIFY(writeNote(directory.filePath(QStringLiteral("today.md")),
                          QStringLiteral("---\ndate: %1\ntags: [paper]\n---\nToday note\n")
                              .arg(today)));
        QVERIFY(writeNote(directory.filePath(QStringLiteral("old.md")),
                          QStringLiteral("---\ndate: 2020-01-01\ntags: [misc]\n---\nOld note\n")));
        QVERIFY(writeNote(directory.filePath(QStringLiteral("plain.md")),
                          QStringLiteral("# No date\nkeyword-only\n")));

        Backend backend;
        backend.openFolder(QUrl::fromLocalFile(directory.path()));
        QCOMPARE(backend.projectRecords().size(), 3);

        backend.setProjectDateFilter(QStringLiteral("today"));
        QCOMPARE(backend.projectRecords().size(), 1);
        QCOMPARE(backend.projectRecords().at(0).toMap().value(QStringLiteral("name")).toString(),
                 QStringLiteral("today.md"));

        backend.setProjectDateFilter(QStringLiteral("undated"));
        QCOMPARE(backend.projectRecords().size(), 1);
        QCOMPARE(backend.projectRecords().at(0).toMap().value(QStringLiteral("name")).toString(),
                 QStringLiteral("plain.md"));

        backend.setProjectDateFilter(QStringLiteral("all"));
        backend.setProjectKeywordFilter(QStringLiteral("keyword-only"));
        QCOMPARE(backend.projectRecords().size(), 1);
        QCOMPARE(backend.projectRecords().at(0).toMap().value(QStringLiteral("name")).toString(),
                 QStringLiteral("plain.md"));
        backend.setProjectKeywordFilter(QString());
    }

    void libraryFiltersPublishedPosts() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        auto writeNote = [](const QString &path, const QString &contents) {
            QFile file(path);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
                return false;
            file.write(contents.toUtf8());
            return true;
        };
        QVERIFY(writeNote(directory.filePath(QStringLiteral("sent.md")),
                          QStringLiteral("---\nthreads: true\nstatus: sent\ndate: 2026-09-17\n---\nSent\n")));
        QVERIFY(writeNote(directory.filePath(QStringLiteral("draft.md")),
                          QStringLiteral("---\nthreads: true\nstatus: draft\ndate: 2026-09-18\n---\nDraft\n")));
        QVERIFY(writeNote(directory.filePath(QStringLiteral("garden.md")),
                          QStringLiteral("---\npublish: true\ndate: 2026-09-16\n---\nGarden\n")));

        Backend backend;
        backend.openFolder(QUrl::fromLocalFile(directory.path()));
        QCOMPARE(backend.projectRecords().size(), 3);
        backend.setProjectPublishFilter(QStringLiteral("published"));
        QCOMPARE(backend.projectRecords().size(), 2);
        backend.setProjectPublishFilter(QStringLiteral("drafts"));
        QCOMPARE(backend.projectRecords().size(), 1);
        QCOMPARE(backend.projectRecords().at(0).toMap().value(QStringLiteral("name")).toString(),
                 QStringLiteral("draft.md"));
        backend.setProjectPublishFilter(QStringLiteral("all"));
        QCOMPARE(backend.projectRecords().size(), 3);
    }

    void markdownTableInsertsColumnAndRow() {
        const QString text = QStringLiteral("| A | B |\n| --- | --- |\n| 1 | 2 |\n");
        const QVariantMap at = CodeBlocks::tableAt(text, 2);
        QCOMPARE(at.value(QStringLiteral("active")).toBool(), true);
        QCOMPARE(at.value(QStringLiteral("cols")).toInt(), 2);

        const QVariantMap col = CodeBlocks::insertTableColumn(text, 2);
        QVERIFY(col.value(QStringLiteral("active")).toBool());
        const QString withCol = col.value(QStringLiteral("text")).toString();
        QVERIFY(withCol.contains(QStringLiteral("| A")));
        QVERIFY(withCol.contains(QStringLiteral("| B")));
        QCOMPARE(withCol.split(QLatin1Char('\n')).constFirst().count(QLatin1Char('|')), 4);

        const QVariantMap row = CodeBlocks::insertTableRow(text, text.indexOf(QLatin1String("| 1 |")));
        QVERIFY(row.value(QStringLiteral("active")).toBool());
        QCOMPARE(row.value(QStringLiteral("text")).toString().count(QLatin1Char('\n')), 3);

        const QVariantMap fromHeader = CodeBlocks::insertTableRow(text, 2);
        QVERIFY(fromHeader.value(QStringLiteral("active")).toBool());
        const QStringList fromHeaderLines =
            fromHeader.value(QStringLiteral("text")).toString().split(QLatin1Char('\n'));
        QVERIFY(fromHeaderLines.size() >= 3);
        QVERIFY(CodeBlocks::isTableSeparator(fromHeaderLines.at(1)));

        const QVariantMap moved = CodeBlocks::tableMoveCell(text, 2, 1);
        QVERIFY(moved.value(QStringLiteral("active")).toBool());
        QVERIFY(moved.value(QStringLiteral("caret")).toInt() > 2);

        const QString slashTable = QStringLiteral(
            "| Column 1 | Column 2 | Column 3 |\n| --- | --- | --- |\n|  |  |  |\n|  |  |  |\n");
        const QVariantMap slashCol = CodeBlocks::insertTableColumn(slashTable, 2);
        QVERIFY(slashCol.value(QStringLiteral("active")).toBool());
        QVERIFY(slashCol.value(QStringLiteral("text")).toString().contains(QStringLiteral("Column 1")));
        QVERIFY(slashCol.value(QStringLiteral("text")).toString().contains(QStringLiteral("Column 3")));
        const int lastRow = slashTable.lastIndexOf(QStringLiteral("|  |  |  |"));
        QVERIFY(lastRow >= 0);
        const QVariantMap slashRow = CodeBlocks::insertTableRow(slashTable, lastRow + 1);
        QVERIFY(slashRow.value(QStringLiteral("active")).toBool());
        QCOMPARE(slashRow.value(QStringLiteral("text")).toString().count(QLatin1Char('\n')), 4);

        const QString ragged = QStringLiteral(
            "| Column 1 | Column 2 | Column 3 |  |  |\n"
            "|  |  |  |  |  |\n"
            "| --- | --- | --- | --- | --- |\n"
            "|  |  |  |  |  |\n"
            "|  |  |  | test |\n");
        const QVariantMap formatted = CodeBlocks::formatTable(ragged, 2);
        QVERIFY(formatted.value(QStringLiteral("active")).toBool());
        const QStringList pretty =
            formatted.value(QStringLiteral("text")).toString().split(QLatin1Char('\n'));
        QVERIFY(pretty.size() >= 3);
        QVERIFY(CodeBlocks::isTableSeparator(pretty.at(1)));
        QVERIFY(pretty.last().contains(QStringLiteral("test")));

        const QVariantList blocks = CodeBlocks::bodyBlocks(
            QStringLiteral("hello\n\n| A | B |\n| --- | --- |\n| 1 | 2 |\n\nbye"));
        QCOMPARE(blocks.size(), 3);
        QCOMPARE(blocks.at(0).toMap().value(QStringLiteral("kind")).toString(),
                 QStringLiteral("text"));
        QCOMPARE(blocks.at(1).toMap().value(QStringLiteral("kind")).toString(),
                 QStringLiteral("table"));
        QCOMPARE(blocks.at(1).toMap().value(QStringLiteral("columns")).toInt(), 2);
        QCOMPARE(blocks.at(2).toMap().value(QStringLiteral("kind")).toString(),
                 QStringLiteral("text"));
    }

    void calendarTodayOpensCurrentMonth() {
        Backend backend;
        backend.showCalendarMonth(2024, 1);
        QCOMPARE(backend.calendarYear(), 2024);
        QCOMPARE(backend.calendarMonth(), 1);
        backend.showCalendarToday();
        const QDate today = QDate::currentDate();
        QCOMPARE(backend.calendarYear(), today.year());
        QCOMPARE(backend.calendarMonth(), today.month());
        QCOMPARE(backend.calendarMonthNames().size(), 12);
        QVERIFY(backend.typewriterScroll());
        backend.setTypewriterScroll(false);
        QVERIFY(!backend.typewriterScroll());
        backend.setTypewriterScroll(true);
    }

    void boardColumnsSortRecordsByDate() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        auto writeNote = [&](const QString &name, const QString &contents) {
            QFile file(directory.filePath(name));
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
                return false;
            file.write(contents.toUtf8());
            return true;
        };
        QVERIFY(writeNote(QStringLiteral("later.md"),
                          QStringLiteral("---\nstatus: queued\ndate: 2026-09-20\n---\nLater\n")));
        QVERIFY(writeNote(QStringLiteral("earlier.md"),
                          QStringLiteral("---\nstatus: queued\ndate: 2026-09-10\n---\nEarlier\n")));
        QVERIFY(writeNote(QStringLiteral("nodate.md"),
                          QStringLiteral("---\nstatus: queued\n---\nNo date\n")));
        Backend backend;
        backend.openFolder(QUrl::fromLocalFile(directory.path()));
        QVariantList queued;
        for (const QVariant &columnValue : backend.boardColumns()) {
            const QVariantMap column = columnValue.toMap();
            if (column.value(QStringLiteral("value")).toString() == QLatin1String("queued"))
                queued = column.value(QStringLiteral("records")).toList();
        }
        QCOMPARE(queued.size(), 3);
        QCOMPARE(queued.at(0).toMap().value(QStringLiteral("name")).toString(),
                 QStringLiteral("earlier.md"));
        QCOMPARE(queued.at(1).toMap().value(QStringLiteral("name")).toString(),
                 QStringLiteral("later.md"));
        QCOMPARE(queued.at(2).toMap().value(QStringLiteral("name")).toString(),
                 QStringLiteral("nodate.md"));
    }

    void liveTypingKeepsUndoStack() {
        Backend backend;
        backend.setEditorMode(QStringLiteral("live"));
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData(QByteArray("import QtQuick\nTextEdit {}\n"),
                          QUrl(QStringLiteral("qrc:/UndoHarness.qml")));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> editor(component.create());
        auto *quick = editor->property("textDocument").value<QQuickTextDocument *>();
        QVERIFY(quick);
        backend.attachDocument(quick);
        QTextDocument *doc = quick->textDocument();
        QTextCursor cursor(doc);
        cursor.insertText(QStringLiteral("hello"));
        QVERIFY(doc->isUndoAvailable());
        backend.refreshWordCount();
        backend.refreshLiveFolding();
        QVERIFY(doc->isUndoAvailable());
        doc->undo();
        QVERIFY(!doc->toPlainText().contains(QLatin1String("hello")));
    }

    void propertyChoicesCoverStatusPublishAndThreads() {
        Backend backend;
        QVERIFY(backend.propertyChoices(QStringLiteral("status")).contains(QStringLiteral("draft")));
        QVERIFY(backend.propertyChoices(QStringLiteral("status")).contains(QStringLiteral("sent")));
        QCOMPARE(backend.propertyChoices(QStringLiteral("publish")),
                 (QStringList{QStringLiteral("true"), QStringLiteral("false")}));
        QCOMPARE(backend.propertyChoices(QStringLiteral("threads")),
                 (QStringList{QStringLiteral("true"), QStringLiteral("false")}));
        QVERIFY(backend.propertyChoices(QStringLiteral("title")).isEmpty());
    }

    void notesSiteFolderDefaultsToAndgreenNotes() {
        Backend backend;
        QVERIFY(backend.defaultNotesSiteFolder().endsWith(
            QStringLiteral("/Projects/andgreen-notes")));
        QVERIFY(backend.defaultChangelogSiteFolder().endsWith(
            QStringLiteral("/Projects/fmd-site")));
        QCOMPARE(backend.resolvedNotesSiteFolder(), backend.defaultNotesSiteFolder());
        QCOMPARE(backend.resolvedChangelogSiteFolder(), backend.defaultChangelogSiteFolder());
        QTemporaryDir site;
        QVERIFY(site.isValid());
        backend.setNotesSiteFolder(site.path());
        QCOMPARE(QDir(backend.resolvedNotesSiteFolder()).absolutePath(),
                 QDir(site.path()).absolutePath());
        backend.setNotesSiteFolder(QString());
        QCOMPARE(backend.resolvedNotesSiteFolder(), backend.defaultNotesSiteFolder());
    }

    void publishCurrentNoteCopiesOnlyPublishTrue() {
        QTemporaryDir vault;
        QTemporaryDir site;
        QVERIFY(vault.isValid());
        QVERIFY(site.isValid());
        const QString gardenPath = vault.filePath(QStringLiteral("garden.md"));
        const QString draftPath = vault.filePath(QStringLiteral("secret.md"));
        auto writeNote = [](const QString &path, const QString &contents) {
            QFile file(path);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
                return false;
            file.write(contents.toUtf8());
            return true;
        };
        QVERIFY(writeNote(gardenPath, QStringLiteral("---\npublish: true\n---\nHello garden\n")));
        QVERIFY(writeNote(draftPath, QStringLiteral("---\nstatus: draft\n---\nSecret\n")));
        Backend backend;
        backend.setNotesSiteFolder(site.path());
        backend.open(QUrl::fromLocalFile(gardenPath));
        const QVariantMap copied = backend.publishCurrentNoteToSite();
        QCOMPARE(copied.value(QStringLiteral("ok")).toBool(), true);
        QVERIFY(QFileInfo::exists(QDir(site.path()).filePath(QStringLiteral("content/garden.md"))));
        backend.open(QUrl::fromLocalFile(draftPath));
        const QVariantMap refused = backend.publishCurrentNoteToSite();
        QCOMPARE(refused.value(QStringLiteral("ok")).toBool(), false);
        QVERIFY(!QFileInfo::exists(QDir(site.path()).filePath(QStringLiteral("content/secret.md"))));
    }

    void gardenPublishableIgnoresDraftEvenWithPublishTrue() {
        const FrontMatter::Document garden = FrontMatter::parse(
            QStringLiteral("---\npublish: true\n---\nHi\n"));
        QVERIFY(FrontMatter::isGardenPublishable(garden.fields));
        const FrontMatter::Document layout = FrontMatter::parse(
            QStringLiteral("---\nlayout: post\n---\nHi\n"));
        QVERIFY(FrontMatter::isGardenPublishable(layout.fields));
        const FrontMatter::Document draft = FrontMatter::parse(
            QStringLiteral("---\npublish: true\ndraft: true\n---\nNope\n"));
        QVERIFY(!FrontMatter::isGardenPublishable(draft.fields));
        const FrontMatter::Document threads = FrontMatter::parse(
            QStringLiteral("---\nstatus: sent\n---\nNope\n"));
        QVERIFY(!FrontMatter::isGardenPublishable(threads.fields));
    }

    void rewriteUnpublishedWikilinksLeavesPublishedTargets() {
        const QString markdown = QStringLiteral(
            "---\npublish: true\n---\n"
            "see [[Keep]] and [[Secret|hidden]] and [[Keep#x]]\n"
            "```\n[[dead]]\n```\n"
            "![[pic.png]]\n");
        const QString rewritten = MindMap::rewriteGardenWikilinks(
            markdown, QStringList{QStringLiteral("Keep")});
        QVERIFY(rewritten.contains(QStringLiteral("[[Keep]]")));
        QVERIFY(rewritten.contains(QStringLiteral("[[Keep#x]]")));
        QVERIFY(rewritten.contains(QStringLiteral("hidden")));
        QVERIFY(!rewritten.contains(QStringLiteral("[[Secret")));
        QVERIFY(rewritten.contains(QStringLiteral("```\n[[dead]]\n```")));
        QVERIFY(rewritten.contains(QStringLiteral("![[pic.png]]")));
        QVERIFY(rewritten.startsWith(QStringLiteral("---\npublish: true\n---\n")));
    }

    void publishWorkspaceCopiesPublishTrueAndRewritesWikilinks() {
        QTemporaryDir vault;
        QTemporaryDir site;
        QTemporaryDir inbox;
        QVERIFY(vault.isValid());
        QVERIFY(site.isValid());
        QVERIFY(inbox.isValid());
        auto writeNote = [](const QString &path, const QString &contents) {
            QDir().mkpath(QFileInfo(path).absolutePath());
            QFile file(path);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
                return false;
            file.write(contents.toUtf8());
            return true;
        };
        QVERIFY(writeNote(vault.filePath(QStringLiteral("keep.md")),
                          QStringLiteral("---\npublish: true\n---\nKept\n")));
        QVERIFY(writeNote(vault.filePath(QStringLiteral("linked.md")),
                          QStringLiteral("---\npublish: true\n---\nsee [[keep]] and [[secret]]\n")));
        QVERIFY(writeNote(vault.filePath(QStringLiteral("secret.md")),
                          QStringLiteral("---\nstatus: draft\n---\nSecret\n")));
        QVERIFY(writeNote(inbox.filePath(QStringLiteral("inbox.md")),
                          QStringLiteral("---\npublish: true\n---\nInbox\n")));
        QSettings().setValue(QStringLiteral("inbox/folder"), inbox.path());
        Backend backend;
        backend.setNotesSiteFolder(site.path());
        backend.openFolder(QUrl::fromLocalFile(vault.path()));
        const QVariantMap published = backend.publishWorkspaceToSite();
        QCOMPARE(published.value(QStringLiteral("ok")).toBool(), true);
        QCOMPARE(published.value(QStringLiteral("copied")).toInt(), 2);
        QVERIFY(QFileInfo::exists(QDir(site.path()).filePath(QStringLiteral("content/keep.md"))));
        QVERIFY(QFileInfo::exists(QDir(site.path()).filePath(QStringLiteral("content/linked.md"))));
        QVERIFY(!QFileInfo::exists(QDir(site.path()).filePath(QStringLiteral("content/secret.md"))));
        QVERIFY(!QFileInfo::exists(QDir(site.path()).filePath(QStringLiteral("content/inbox.md"))));
        QFile linked(QDir(site.path()).filePath(QStringLiteral("content/linked.md")));
        QVERIFY(linked.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString copied = QString::fromUtf8(linked.readAll());
        QVERIFY(copied.contains(QStringLiteral("[[keep]]")));
        QVERIFY(copied.contains(QStringLiteral("secret")));
        QVERIFY(!copied.contains(QStringLiteral("[[secret]]")));
    }

    void slidevMarkdownSplitsOnHeadingsAndStripsLayout() {
        const QString source = QStringLiteral(
            "---\ntitle: Talk\nlayout: slides\ndraft: true\n---\n"
            "preamble\n\n# First\n\nbody one\n\n## Second\n\nsee [[Secret|hidden]]\n");
        QVERIFY(!FrontMatter::isSlidevNote(FrontMatter::parse(source).fields));
        const QString live = QStringLiteral(
            "---\ntitle: Talk\nlayout: slides\nstatus: queued\n---\n"
            "preamble\n\n# First\n\nbody one\n\n## Second\n\nsee [[Secret|hidden]]\n");
        QVERIFY(FrontMatter::isSlidevNote(FrontMatter::parse(live).fields));
        const QString exported = CodeBlocks::slidevMarkdown(live);
        QVERIFY(exported.startsWith(QStringLiteral("---\n")));
        QVERIFY(!exported.contains(QStringLiteral("theme: default")));
        QVERIFY(exported.contains(QStringLiteral("title: Talk")));
        QVERIFY(!exported.contains(QStringLiteral("layout: slides")));
        QVERIFY(exported.contains(QStringLiteral("preamble")));
        QVERIFY(exported.contains(QStringLiteral("\n---\n")));
        QVERIFY(exported.contains(QStringLiteral("# First")));
        QVERIFY(exported.contains(QStringLiteral("## Second")));
        QVERIFY(exported.contains(QStringLiteral("hidden")));
        QVERIFY(!exported.contains(QStringLiteral("[[Secret")));
        QCOMPARE(CodeBlocks::slidevSlideCount(exported), 3);
        const QVariantList preview = CodeBlocks::slidevPreviewSlides(live);
        QCOMPARE(preview.size(), 3);
        QCOMPARE(preview.at(1).toMap().value(QStringLiteral("title")).toString(),
                 QStringLiteral("First"));
        QVERIFY(preview.at(1).toMap().value(QStringLiteral("body")).toString().contains(
            QStringLiteral("body one")));
    }

    void slidevHeadingLayoutAttributeBecomesSlideFrontMatter() {
        const QString live = QStringLiteral(
            "---\ntitle: Talk\nlayout: slides\ntheme: seriph\ncolorSchema: light\n---\n"
            "# Hello {layout: cover}\n\nbody\n\n## Two cols {layout: two-cols}\n\nleft\n\n"
            "::right::\n\nright\n");
        const QString exported = CodeBlocks::slidevMarkdown(live);
        QVERIFY(exported.contains(QStringLiteral("theme: seriph")));
        QVERIFY(exported.contains(QStringLiteral("colorSchema: light")));
        QVERIFY(exported.contains(QStringLiteral("layout: cover")));
        QVERIFY(exported.contains(QStringLiteral("layout: two-cols")));
        QVERIFY(!exported.contains(QStringLiteral("{layout: cover}")));
        QVERIFY(exported.contains(QStringLiteral("# Hello")));
        QVERIFY(!exported.contains(QStringLiteral("theme: default")));
        QCOMPARE(CodeBlocks::slidevPreviewSlides(live).size(), 2);
        QVERIFY(!exported.contains(QStringLiteral("---\n---\nlayout: cover")));
    }

    void nativeSlidevMarkdownIsDetectedAndNotGivenAnEmptyCoverSlide() {
        const QString native = QStringLiteral(
            "---\ntheme: seriph\nlayout: default\n---\n\n# 從拆解到縫合\n\n---\n\n## 大綱\n");
        QVERIFY(CodeBlocks::isNativeSlidevMarkdown(native));
        QVERIFY(!CodeBlocks::isNativeSlidevMarkdown(
            QStringLiteral("---\nlayout: slides\n---\n\n# A\n\n## B\n")));
        const QString fmd = QStringLiteral(
            "---\ntitle: Talk\nlayout: slides\n---\n# Hello {layout: cover}\n\nbody\n");
        const QString exported = CodeBlocks::slidevMarkdown(fmd);
        QCOMPARE(CodeBlocks::slidevSlideCount(exported), 1);
        QVERIFY(exported.contains(QStringLiteral("layout: cover")));
        QVERIFY(!exported.contains(QStringLiteral("---\n---\nlayout: cover")));
        QVERIFY(exported.contains(QStringLiteral("# Hello")));
        QVERIFY(!CodeBlocks::isNativeSlidevMarkdown(
            QStringLiteral("---\nlayout: post\npublish: true\n---\n\nHi\n\n---\n\nMore\n")));
    }

    void nativeSlidevFilePresentsInPlaceWithoutRewritingLayout() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const QString path = directory.filePath(QStringLiteral("slides.md"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write(QStringLiteral(
                       "---\ntheme: seriph\nlayout: default\n---\n\n# 從拆解到縫合\n\n---\n\n## 大綱\n")
                       .toUtf8());
        file.close();
        Backend backend;
        backend.open(QUrl::fromLocalFile(path));
        QVERIFY(backend.slidevNote());
        const QVariantMap check = backend.validateSlidevDraft();
        QCOMPARE(check.value(QStringLiteral("ok")).toBool(), true);
        QCOMPARE(check.value(QStringLiteral("inPlace")).toBool(), true);
        const QVariantMap published = backend.publishSlidevNow(true);
        QCOMPARE(published.value(QStringLiteral("ok")).toBool(), true);
        QCOMPARE(QFileInfo(published.value(QStringLiteral("path")).toString()).fileName(),
                 QStringLiteral("slides.md"));
        QCOMPARE(FrontMatter::displayValue(FrontMatter::parse(backend.editorPlainText())
                                               .fields.value(QStringLiteral("layout"))),
                 QStringLiteral("default"));
    }

    void sitePushDryRunReportsNothingToCommitOnEmptyRepo() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QProcess git;
        git.setWorkingDirectory(dir.path());
        git.start(QStringLiteral("git"), QStringList{QStringLiteral("init")});
        QVERIFY(git.waitForFinished(10000));
        QCOMPARE(git.exitCode(), 0);
        QDir(dir.path()).mkpath(QStringLiteral("content"));
        QFile note(QDir(dir.path()).filePath(QStringLiteral("content/hello.md")));
        QVERIFY(note.open(QIODevice::WriteOnly | QIODevice::Text));
        note.write("hi\n");
        note.close();
        const QString script = QFINDTESTDATA("../bin/site-push");
        QVERIFY(!script.isEmpty());
        QProcess push;
        push.start(script, QStringList{
            QStringLiteral("--repo"), dir.path(),
            QStringLiteral("--paths"), QStringLiteral("content"),
            QStringLiteral("--dry-run"),
        });
        QVERIFY(push.waitForFinished(10000));
        const QString out = QString::fromUtf8(push.readAllStandardOutput()
                                              + push.readAllStandardError());
        QVERIFY2(push.exitCode() == 0, qPrintable(out));
        QVERIFY(out.contains(QStringLiteral("ok=true")));
        QVERIFY(out.contains(QStringLiteral("dryRun=true")));
        QVERIFY(out.contains(QStringLiteral("noop=false")));
    }

    void publishSlidevDryRunWritesDeckAndLeavesStatus() {
        QTemporaryDir vault;
        QTemporaryDir slides;
        QVERIFY(vault.isValid());
        QVERIFY(slides.isValid());
        const QString path = vault.filePath(QStringLiteral("talk.md"));
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
        file.write(QStringLiteral("---\nlayout: slides\nstatus: queued\ntitle: Talk\n---\n# Hello\n\nworld\n").toUtf8());
        file.close();
        Backend backend;
        backend.setSlidesSiteFolder(slides.path());
        backend.open(QUrl::fromLocalFile(path));
        const QVariantMap refused = [&]() {
            Backend other;
            other.setEditorPlainText(QStringLiteral("---\nstatus: draft\n---\n# Nope\n"));
            return other.validateSlidevDraft();
        }();
        QCOMPARE(refused.value(QStringLiteral("ok")).toBool(), false);
        const QVariantMap check = backend.validateSlidevDraft();
        QCOMPARE(check.value(QStringLiteral("ok")).toBool(), true);
        const QVariantMap published = backend.publishSlidevNow(true);
        QCOMPARE(published.value(QStringLiteral("ok")).toBool(), true);
        QVERIFY(published.value(QStringLiteral("dryRun")).toBool());
        const QString dest = QDir(slides.path()).filePath(QStringLiteral("decks/talk/slides.md"));
        QVERIFY(QFileInfo::exists(dest));
        QFile out(dest);
        QVERIFY(out.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString exported = QString::fromUtf8(out.readAll());
        QVERIFY(exported.contains(QStringLiteral("# Hello")));
        QVERIFY(!exported.contains(QStringLiteral("layout: slides")));
        QCOMPARE(FrontMatter::displayValue(FrontMatter::parse(backend.editorPlainText())
                                               .fields.value(QStringLiteral("status"))),
                 QStringLiteral("queued"));
    }

    void slidevPresentScriptKillsWholeTreeAndPinsIpv4() {
        const QString script = QFINDTESTDATA("../bin/slidev-present");
        QVERIFY(!script.isEmpty());
        QFile file(script);
        QVERIFY(file.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString text = QString::fromUtf8(file.readAll());
        QVERIFY(text.contains(QStringLiteral("dns-result-order=ipv4first")));
        QVERIFY(text.contains(QStringLiteral("kill_tree")));
        QVERIFY(text.contains(QStringLiteral("kill_slidev_for_file")));
        QVERIFY(text.contains(QStringLiteral("slidev_pids_for_file")));
        QVERIFY(text.contains(QStringLiteral("slidev-present")));
        QVERIFY(text.contains(QStringLiteral("http://127.0.0.1:")));
        QVERIFY(text.contains(QStringLiteral("v self=")));
    }

    void defaultSlidesSiteFolderIsTheLibrary() {
        Backend backend;
        QVERIFY(backend.defaultSlidesSiteFolder().endsWith(
            QStringLiteral("/Documents/01_ACTIVE/SLIDEV")));
    }

    void createGardenAndSlidevNotesFromTemplates() {
        QTemporaryDir slides;
        QTemporaryDir garden;
        QVERIFY(slides.isValid());
        QVERIFY(garden.isValid());
        Backend backend;
        backend.setSlidesSiteFolder(slides.path());
        backend.setGardenNotesFolder(garden.path());
        const QUrl slide = backend.createSlidevNote();
        QVERIFY(slide.isLocalFile());
        QVERIFY(slide.toLocalFile().contains(QStringLiteral("/slides-")));
        QVERIFY(QFileInfo::exists(QDir(slides.path()).filePath(QStringLiteral("_TEMPLATE-slides.md"))));
        QFile slideFile(slide.toLocalFile());
        QVERIFY(slideFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const QString slideText = QString::fromUtf8(slideFile.readAll());
        QVERIFY(FrontMatter::isSlidevNote(FrontMatter::parse(slideText).fields));
        QVERIFY(slideText.contains(QStringLiteral("{layout: cover}")));
        QVERIFY(slideText.contains(QStringLiteral("`g`")));

        const QUrl gardenNote = backend.createGardenNote();
        QVERIFY(gardenNote.isLocalFile());
        QVERIFY(gardenNote.toLocalFile().contains(QStringLiteral("/garden-")));
        QFile gardenFile(gardenNote.toLocalFile());
        QVERIFY(gardenFile.open(QIODevice::ReadOnly | QIODevice::Text));
        const FrontMatter::Document parsed = FrontMatter::parse(QString::fromUtf8(gardenFile.readAll()));
        QCOMPARE(FrontMatter::displayValue(parsed.fields.value(QStringLiteral("layout"))),
                 QStringLiteral("post"));
        QVERIFY(FrontMatter::isTruthy(parsed.fields.value(QStringLiteral("publish"))));
        QVERIFY(!FrontMatter::isTruthy(parsed.fields.value(QStringLiteral("draft"))));
    }

    void livePropertiesBarHasHeightSoEditorIsNotCovered() {
        const QString mainQmlPath = QFINDTESTDATA("../src/Main.qml");
        QVERIFY(!mainQmlPath.isEmpty());
        Backend backend;
        PluginHost pluginHost(&backend);
        QQmlEngine engine;
        engine.rootContext()->setContextProperty(QStringLiteral("backend"), &backend);
        engine.rootContext()->setContextProperty(QStringLiteral("pluginHost"), &pluginHost);
        QQmlComponent component(&engine, QUrl::fromLocalFile(mainQmlPath));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> window(component.create());
        QVERIFY2(window, qPrintable(component.errorString()));
        backend.setEditorMode(QStringLiteral("live"));
        backend.setEditorPlainText(
            QStringLiteral("---\ntitle: Gap\nstatus: draft\nthreads: true\n---\nHello body\n"));
        backend.setPropertiesExpanded(true);
        QCoreApplication::processEvents();
        QObject *bar = window->findChild<QObject *>(QStringLiteral("propertiesBar"));
        QVERIFY(bar);
        QVERIFY(bar->property("visible").toBool());
        QTRY_VERIFY(bar->property("height").toReal() > 40);
    }

    void inboxNotesStayOutsideTheProjectFolder() {
        QTemporaryDir project;
        QTemporaryDir inbox;
        QVERIFY(project.isValid());
        QVERIFY(inbox.isValid());
        QSettings().setValue(QStringLiteral("inbox/folder"), inbox.path());
        QFile note(project.filePath(QStringLiteral("in-project.md")));
        QVERIFY(note.open(QIODevice::WriteOnly | QIODevice::Text));
        note.write("project\n");
        note.close();

        Backend backend;
        backend.openFolder(QUrl::fromLocalFile(project.path()));
        const QString projectPath = backend.workspaceFolderPath();
        const QUrl created = backend.createInboxNote();
        QVERIFY(created.isLocalFile());
        QVERIFY(backend.isInboxPath(created.toLocalFile()));
        QVERIFY(created.toLocalFile().startsWith(QDir(inbox.path()).absolutePath()));
        backend.open(created);
        QCOMPARE(backend.workspaceFolderPath(), projectPath);
        QVERIFY(backend.inboxFiles().size() >= 1);
    }

    void trashingAnInboxNoteRemovesItFromTheInboxList() {
        QTemporaryDir inbox;
        QVERIFY(inbox.isValid());
        QSettings().setValue(QStringLiteral("inbox/folder"), inbox.path());
        Backend backend;
        const QUrl created = backend.createInboxNote();
        QVERIFY(created.isLocalFile());
        QCOMPARE(backend.inboxFiles().size(), 1);
        QVERIFY(backend.moveNoteToTrash(created));
        QCOMPARE(backend.inboxFiles().size(), 0);
        QVERIFY(!QFileInfo::exists(created.toLocalFile()));
    }

    void refreshingInboxDropsDeletedFiles() {
        QTemporaryDir inbox;
        QVERIFY(inbox.isValid());
        QSettings().setValue(QStringLiteral("inbox/folder"), inbox.path());
        Backend backend;
        const QUrl created = backend.createInboxNote();
        QCOMPARE(backend.inboxFiles().size(), 1);
        QVERIFY(QFile::remove(created.toLocalFile()));
        backend.refreshInboxFiles();
        QCOMPARE(backend.inboxFiles().size(), 0);
    }

    void liveModeHidesYamlEvenWhenCaretIsAtStart() {
        const QString text = QStringLiteral("---\ntitle: Hi\nstatus: queued\n---\nHello\n");
        const FrontMatter::Document parsed = FrontMatter::parse(text);
        QVERIFY(parsed.hasFrontMatter);
        QVERIFY(parsed.yamlEnd > parsed.yamlStart);
        QCOMPARE(parsed.fieldOrder.first(), QStringLiteral("title"));

        Backend backend;
        backend.setEditorMode(QStringLiteral("live"));
        backend.setEditorPlainText(text);
        QCOMPARE(backend.frontMatterEnd(), parsed.yamlEnd);
        const QVariantList hidden = backend.hiddenRangesAt(parsed.yamlEnd);
        bool coversYaml = false;
        for (const QVariant &item : hidden) {
            const QVariantMap range = item.toMap();
            if (range.value(QStringLiteral("start")).toInt() <= parsed.yamlStart
                && range.value(QStringLiteral("end")).toInt() >= parsed.yamlEnd)
                coversYaml = true;
        }
        QVERIFY(coversYaml);
    }

    void liveModeHidesYamlBlocksOnAttachedDocument() {
        const QString text = QStringLiteral("---\ntitle: Hi\nstatus: queued\n---\nHello\n");
        const FrontMatter::Document parsed = FrontMatter::parse(text);
        QVERIFY(parsed.hasFrontMatter);

        Backend backend;
        backend.setEditorMode(QStringLiteral("live"));
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData(QByteArray("import QtQuick\nTextEdit {}\n"),
                          QUrl(QStringLiteral("qrc:/LiveYamlHarness.qml")));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> editor(component.create());
        QVERIFY2(editor, qPrintable(component.errorString()));
        auto *quick = editor->property("textDocument").value<QQuickTextDocument *>();
        QVERIFY(quick);
        QVERIFY(quick->textDocument());
        backend.attachDocument(quick);
        backend.setEditorPlainText(text);
        QCoreApplication::processEvents();
        backend.refreshLiveFolding();

        QTextDocument *doc = quick->textDocument();
        int yamlBlocks = 0;
        int visibleYaml = 0;
        for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
            if (block.position() >= parsed.yamlEnd)
                break;
            ++yamlBlocks;
            if (block.isVisible())
                ++visibleYaml;
        }
        QVERIFY(yamlBlocks >= 3);
        QCOMPARE(visibleYaml, 0);

        bool bodyVisible = false;
        for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
            if (block.position() >= parsed.yamlEnd && block.isVisible()
                    && block.text().contains(QLatin1String("Hello")))
                bodyVisible = true;
        }
        QVERIFY(bodyVisible);
    }

    void liveModeHidesYamlWhenSecondFileHasSameYamlLength() {
        const QString first = QStringLiteral("---\ntitle: AAA\nstatus: queued\n---\nFirst\n");
        const QString second = QStringLiteral("---\ntitle: BBB\nstatus: queued\n---\nSecond\n");
        QCOMPARE(FrontMatter::parse(first).yamlEnd, FrontMatter::parse(second).yamlEnd);

        Backend backend;
        backend.setEditorMode(QStringLiteral("live"));
        QQmlEngine engine;
        QQmlComponent component(&engine);
        component.setData(QByteArray("import QtQuick\nTextEdit {}\n"),
                          QUrl(QStringLiteral("qrc:/LiveYamlHarness.qml")));
        QVERIFY2(component.isReady(), qPrintable(component.errorString()));
        QScopedPointer<QObject> editor(component.create());
        auto *quick = editor->property("textDocument").value<QQuickTextDocument *>();
        QVERIFY(quick);
        backend.attachDocument(quick);
        backend.setEditorPlainText(first);
        QCoreApplication::processEvents();
        backend.setEditorPlainText(second);
        QCoreApplication::processEvents();
        backend.refreshLiveFolding();

        const FrontMatter::Document parsed = FrontMatter::parse(second);
        int visibleYaml = 0;
        for (QTextBlock block = quick->textDocument()->begin(); block.isValid();
             block = block.next()) {
            if (block.position() >= parsed.yamlEnd)
                break;
            if (block.isVisible())
                ++visibleYaml;
        }
        QCOMPARE(visibleYaml, 0);
        QVERIFY(backend.editorPlainText().contains(QLatin1String("Second")));
    }

    void currentFrontMatterFieldRoundtrip() {
        Backend backend;
        backend.setEditorPlainText(QStringLiteral("---\ntitle: Old\n---\nBody\n"));
        backend.setCurrentFrontMatterField(QStringLiteral("status"), QStringLiteral("queued"));
        const FrontMatter::Document parsed = FrontMatter::parse(backend.editorPlainText());
        QCOMPARE(parsed.fields.value(QStringLiteral("title")).toString(), QStringLiteral("Old"));
        QCOMPARE(parsed.fields.value(QStringLiteral("status")).toString(), QStringLiteral("queued"));
        backend.removeCurrentFrontMatterField(QStringLiteral("status"));
        const FrontMatter::Document after = FrontMatter::parse(backend.editorPlainText());
        QVERIFY(!after.fields.contains(QStringLiteral("status")));
        QCOMPARE(after.fields.value(QStringLiteral("title")).toString(), QStringLiteral("Old"));
    }

    void liveModeAndZenAndTaskCounts() {
        Backend backend;
        QCOMPARE(backend.editorMode(), QStringLiteral("live"));
        backend.setEditorMode(QStringLiteral("source"));
        QCOMPARE(backend.editorMode(), QStringLiteral("source"));
        backend.setEditorMode(QStringLiteral("live"));
        QCOMPARE(backend.editorMode(), QStringLiteral("live"));

        QVERIFY(!backend.zenMode());
        backend.setZenMode(true);
        QVERIFY(backend.zenMode());
        backend.setZenMode(false);
        QVERIFY(!backend.zenMode());

        backend.setEditorPlainText(QStringLiteral("- [ ] one\n- [x] two\n- [ ] three\n"));
        backend.refreshWordCount();
        QCOMPARE(backend.openTaskCount(), 2);
        QCOMPARE(backend.closedTaskCount(), 1);

        const FrontMatter::Document parsed = FrontMatter::parse(
            QStringLiteral("---\ntitle: x\n---\nbody\n"));
        QVERIFY(parsed.hasFrontMatter);
        QVERIFY(parsed.yamlStart >= 0);
        QVERIFY(parsed.yamlEnd > parsed.yamlStart);
    }

    void replaceEditorRangeWithoutAttachedDocument() {
        Backend backend;
        backend.setEditorPlainText(QStringLiteral("hello WORLD"));
        const int start = backend.editorPlainText().indexOf(QStringLiteral("WORLD"));
        backend.replaceEditorRange(start, start + 5, QStringLiteral("there"));
        QCOMPARE(backend.editorPlainText(), QStringLiteral("hello there"));
    }

    void dualPaneMarksPdfAndImageKinds() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        QFile note(directory.filePath(QStringLiteral("note.md")));
        QVERIFY(note.open(QIODevice::WriteOnly | QIODevice::Text));
        note.write("# Hi\n");
        note.close();
        QFile pdf(directory.filePath(QStringLiteral("plan.pdf")));
        QVERIFY(pdf.open(QIODevice::WriteOnly));
        pdf.write("%PDF-1.4\n");
        pdf.close();
        QFile png(directory.filePath(QStringLiteral("cover.png")));
        QVERIFY(png.open(QIODevice::WriteOnly));
        png.write("x");
        png.close();
        QFile lock(directory.filePath(QStringLiteral("package.json")));
        QVERIFY(lock.open(QIODevice::WriteOnly | QIODevice::Text));
        lock.write("{}\n");
        lock.close();

        Backend backend;
        backend.openFolder(QUrl::fromLocalFile(directory.path()));
        backend.setSelectedWorkspacePath(directory.path());
        QHash<QString, QString> kinds;
        QStringList names;
        for (const QVariant &item : backend.workspaceNavFiles()) {
            const QVariantMap row = item.toMap();
            names.append(row.value(QStringLiteral("name")).toString());
            kinds.insert(row.value(QStringLiteral("name")).toString(),
                         row.value(QStringLiteral("kind")).toString());
        }
        QCOMPARE(kinds.value(QStringLiteral("note.md")), QStringLiteral("markdown"));
        QCOMPARE(kinds.value(QStringLiteral("plan.pdf")), QStringLiteral("pdf"));
        QCOMPARE(kinds.value(QStringLiteral("cover.png")), QStringLiteral("image"));
        QVERIFY(!names.contains(QStringLiteral("package.json")));
    }

    void dualPaneListsNestedEmptyFoldersInParent() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        QVERIFY(QDir().mkpath(directory.filePath(QStringLiteral("published/inner"))));
        QFile note(directory.filePath(QStringLiteral("published/a.md")));
        QVERIFY(note.open(QIODevice::WriteOnly | QIODevice::Text));
        note.write("hi\n");
        note.close();

        Backend backend;
        backend.openFolder(QUrl::fromLocalFile(directory.path()));
        QStringList folderNames;
        QHash<QString, int> depths;
        for (const QVariant &item : backend.workspaceNavFolders()) {
            const QVariantMap row = item.toMap();
            const QString name = row.value(QStringLiteral("name")).toString();
            folderNames.append(name);
            depths.insert(name, row.value(QStringLiteral("depth")).toInt());
        }
        QVERIFY(folderNames.contains(QStringLiteral("published")));
        QVERIFY(folderNames.contains(QStringLiteral("inner")));
        QCOMPARE(depths.value(QStringLiteral("inner")), 2);

        backend.toggleWorkspaceFolder(directory.filePath(QStringLiteral("published")));
        folderNames.clear();
        for (const QVariant &item : backend.workspaceNavFolders())
            folderNames.append(item.toMap().value(QStringLiteral("name")).toString());
        QVERIFY(folderNames.contains(QStringLiteral("published")));
        QVERIFY(!folderNames.contains(QStringLiteral("inner")));
        backend.toggleWorkspaceFolder(directory.filePath(QStringLiteral("published")));
        folderNames.clear();
        for (const QVariant &item : backend.workspaceNavFolders())
            folderNames.append(item.toMap().value(QStringLiteral("name")).toString());
        QVERIFY(folderNames.contains(QStringLiteral("inner")));

        backend.setSelectedWorkspacePath(directory.filePath(QStringLiteral("published")));
        QStringList names;
        QStringList kinds;
        for (const QVariant &item : backend.workspaceNavFiles()) {
            names.append(item.toMap().value(QStringLiteral("name")).toString());
            kinds.append(item.toMap().value(QStringLiteral("kind")).toString());
        }
        QVERIFY(names.contains(QStringLiteral("inner")));
        QVERIFY(names.contains(QStringLiteral("a.md")));
        QCOMPARE(kinds.first(), QStringLiteral("folder"));
        QVERIFY(kinds.contains(QStringLiteral("markdown")));

        backend.setSelectedWorkspacePath(directory.filePath(QStringLiteral("published/inner")));
        QVERIFY(backend.workspaceNavCanGoUp());
        QCOMPARE(backend.workspaceNavCrumbs().size(), 3);
        QCOMPARE(backend.workspaceNavCrumbs().at(0).toMap().value(QStringLiteral("root")).toBool(),
                 true);
        QCOMPARE(backend.workspaceNavCrumbs().at(2).toMap().value(QStringLiteral("name")).toString(),
                 QStringLiteral("inner"));
        backend.selectParentWorkspaceFolder();
        QCOMPARE(QDir(backend.selectedFolderPath()).dirName(), QStringLiteral("published"));
        QCOMPARE(backend.workspaceNavCrumbs().size(), 2);
        backend.selectParentWorkspaceFolder();
        QVERIFY(!backend.workspaceNavCanGoUp());
        QCOMPARE(backend.workspaceNavCrumbs().size(), 1);
    }

    void cardsViewCoverPinTagAndAndHeatmap() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        QVERIFY(QDir().mkpath(directory.filePath(QStringLiteral("garden"))));
        QFile pic(directory.filePath(QStringLiteral("pic.png")));
        QVERIFY(pic.open(QIODevice::WriteOnly));
        pic.write("x");
        pic.close();
        const QString today = QDate::currentDate().toString(Qt::ISODate);
        auto writeNote = [&](const QString &rel, const QString &body) {
            QFile file(directory.filePath(rel));
            QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Text));
            file.write(body.toUtf8());
            file.close();
        };
        writeNote(QStringLiteral("alpha-beta.md"),
                  QStringLiteral("---\ntitle: Both\ntags: [alpha, beta]\ncover: pic.png\n"
                                 "pin: true\ndate: %1\n---\n\n# Both\n\nbody\n")
                      .arg(today));
        writeNote(QStringLiteral("alpha-only.md"),
                  QStringLiteral("---\ntitle: Alpha\ntags: [alpha]\ndate: %1\n---\n\n# Alpha\n")
                      .arg(today));
        writeNote(QStringLiteral("garden/beta-only.md"),
                  QStringLiteral("---\ntitle: Beta\ntags: [beta]\ndate: %1\n---\n\n# Beta\n")
                      .arg(today));

        Backend backend;
        backend.openFolder(QUrl::fromLocalFile(directory.path()));
        backend.setProjectView(QStringLiteral("cards"));
        QCOMPARE(backend.projectView(), QStringLiteral("cards"));
        QCOMPARE(backend.cardRecords().size(), 3);

        QVariantMap pinned;
        for (const QVariant &item : backend.cardRecords()) {
            const QVariantMap rec = item.toMap();
            if (rec.value(QStringLiteral("title")).toString() == QLatin1String("Both"))
                pinned = rec;
        }
        QVERIFY(pinned.value(QStringLiteral("pinned")).toBool());
        QVERIFY(pinned.value(QStringLiteral("cover")).toString().contains(QStringLiteral("pic.png")));
        QCOMPARE(backend.cardRecords().first().toMap().value(QStringLiteral("title")).toString(),
                 QStringLiteral("Both"));

        backend.toggleCardTag(QStringLiteral("alpha"));
        QCOMPARE(backend.cardRecords().size(), 2);
        backend.toggleCardTag(QStringLiteral("beta"));
        QCOMPARE(backend.cardRecords().size(), 1);
        QCOMPARE(backend.cardRecords().first().toMap().value(QStringLiteral("title")).toString(),
                 QStringLiteral("Both"));
        backend.clearCardTags();
        QCOMPARE(backend.cardRecords().size(), 3);

        backend.setSelectedWorkspacePath(directory.filePath(QStringLiteral("garden")));
        QCOMPARE(QDir(backend.selectedFolderPath()).absolutePath(),
                 QDir(directory.filePath(QStringLiteral("garden"))).absolutePath());
        QCOMPARE(backend.cardRecords().size(), 1);
        QCOMPARE(backend.workspaceNavFiles().size(), 1);
        QCOMPARE(backend.workspaceNavFiles().first().toMap().value(QStringLiteral("name")).toString(),
                 QStringLiteral("beta-only.md"));
        QVERIFY(backend.workspaceNavFolders().size() >= 2);
        QVERIFY(backend.randomCardUrl().isLocalFile());
        const QVariantList heatDays = backend.activityHeatmap();
        QVERIFY(!heatDays.isEmpty());
        QCOMPARE(heatDays.size() % 7, 0);
        QCOMPARE(heatDays.first().toMap().value(QStringLiteral("weekday")).toInt(), 0);
        int heat = 0;
        for (const QVariant &day : heatDays) {
            if (day.toMap().value(QStringLiteral("date")).toString() == today)
                heat = day.toMap().value(QStringLiteral("count")).toInt();
        }
        QCOMPARE(heat, 3);
        backend.revealCalendarDate(today);
        QCOMPARE(backend.calendarYear(), QDate::currentDate().year());
        QCOMPARE(backend.calendarMonth(), QDate::currentDate().month());
        QCOMPARE(backend.workspaceShortcuts().size(), 1);
    }

private:
    QTemporaryDir m_settingsDirectory;
};

QTEST_MAIN(FmdTest)
#include "tst_fmd.moc"
