#include "mindmap.h"

#include "frontmatter.h"

#include <QDir>
#include <QFileInfo>
#include <QFont>
#include <QFontMetricsF>
#include <QHash>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>

#include <algorithm>
#include <functional>

namespace MindMap {
namespace {

bool isFence(const QString &line)
{
    return line.trimmed().startsWith(QStringLiteral("```"));
}

const QRegularExpression &colorMarkerRe()
{
    static const QRegularExpression re(
        QStringLiteral(R"(\s*<!--c:(#[0-9A-Fa-f]{6})-->\s*)"));
    return re;
}

int leadingColumns(const QString &line)
{
    int cols = 0;
    for (const QChar ch : line) {
        if (ch == QLatin1Char(' '))
            cols += 1;
        else if (ch == QLatin1Char('\t'))
            cols += 2;
        else
            break;
    }
    return cols;
}

bool matchListItem(const QString &line, int *indent, QString *title)
{
    static const QRegularExpression re(
        QStringLiteral(R"(^([ \t]*)(?:[-*+]|\d+\.)[ \t]+(.*)$)"));
    const QRegularExpressionMatch match = re.match(line);
    if (!match.hasMatch())
        return false;
    if (indent)
        *indent = leadingColumns(match.captured(1));
    if (title)
        *title = match.captured(2).trimmed();
    return true;
}

bool matchHeading(const QString &line, int *level, QString *title)
{
    static const QRegularExpression re(
        QStringLiteral(R"(^(#{1,6})[ \t]+(.+?)[ \t]*#*[ \t]*$)"));
    const QRegularExpressionMatch match = re.match(line);
    if (!match.hasMatch())
        return false;
    if (level)
        *level = match.captured(1).size();
    if (title)
        *title = match.captured(2).trimmed();
    return true;
}

QStringList bodyLines(const QString &body)
{
    QString normalized = body;
    normalized.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    normalized.replace(QLatin1Char('\r'), QLatin1Char('\n'));
    return normalized.split(QLatin1Char('\n'));
}

struct FlatItem {
    int key = 0;
    QString title;
};

QVector<Node> nest(const QVector<FlatItem> &items, int parentKey)
{
    QVector<Node> children;
    for (int i = 0; i < items.size();) {
        if (items.at(i).key <= parentKey)
            break;
        Node node;
        node.title = items.at(i).title;
        const int myKey = items.at(i).key;
        int next = i + 1;
        while (next < items.size() && items.at(next).key > myKey)
            ++next;
        QVector<FlatItem> sub;
        for (int j = i + 1; j < next; ++j)
            sub.append(items.at(j));
        node.children = nest(sub, myKey);
        children.append(node);
        i = next;
    }
    return children;
}

QVector<Node> nestList(const QVector<FlatItem> &items)
{
    if (items.isEmpty())
        return {};
    int minKey = items.constFirst().key;
    for (const FlatItem &item : items)
        minKey = qMin(minKey, item.key);
    return nest(items, minKey - 1);
}

void appendListLines(QStringList *lines, const Node &node, int depth)
{
    const QString indent(depth * 2, QLatin1Char(' '));
    const QString continuation(depth * 2 + 2, QLatin1Char(' '));
    const QStringList parts = node.title.split(QLatin1Char('\n'));
    lines->append(indent + QStringLiteral("- ")
                  + (parts.isEmpty() ? QString() : parts.constFirst()));
    for (int i = 1; i < parts.size(); ++i)
        lines->append(continuation + parts.at(i));
    for (const Node &child : node.children)
        appendListLines(lines, child, depth + 1);
}

bool isDescendant(const Node &node, const QString &id)
{
    for (const Node &child : node.children) {
        if (child.id == id || isDescendant(child, id))
            return true;
    }
    return false;
}

struct ParentLoc {
    Node *parent = nullptr;
    int index = -1;
};

ParentLoc locate(Node &root, const QString &id)
{
    for (int i = 0; i < root.children.size(); ++i) {
        if (root.children.at(i).id == id)
            return {&root, i};
        const ParentLoc nested = locate(root.children[i], id);
        if (nested.parent)
            return nested;
    }
    return {};
}

qreal measureWidth(const QString &title, qreal scale)
{
    const Markup markup = parseMarkup(title);
    if (markup.embed && looksLikeImageTarget(markup.target) && !markup.target.isEmpty())
        return 168.0 * scale;
    QFont font(QStringLiteral("iA Writer Mono S"));
    font.setPixelSize(qMax(11, int(13 * scale)));
    const QFontMetricsF metrics(font);
    qreal width = 72.0 * scale;
    const QString shown = markup.display.isEmpty() ? title : markup.display;
    const QStringList lines = shown.split(QLatin1Char('\n'));
    for (const QString &line : lines)
        width = qMax(width, metrics.horizontalAdvance(line) + 24.0 * scale);
    if (markup.kind != QLatin1String("plain"))
        width += 14.0 * scale;
    return width;
}

qreal measureHeight(const QString &title, qreal scale)
{
    const Markup markup = parseMarkup(title);
    if (markup.embed && looksLikeImageTarget(markup.target) && !markup.target.isEmpty())
        return 100.0 * scale;
    const QString shown = markup.display.isEmpty() ? title : markup.display;
    const int lines = qMax(1, shown.split(QLatin1Char('\n')).size());
    return qMax(36.0 * scale, lines * 16.0 * scale + 20.0 * scale);
}

struct Box {
    QString id;
    QString title;
    QString parentId;
    qreal x = 0;
    qreal y = 0;
    qreal w = 0;
    qreal h = 0;
    int depth = 0;
};

void collectColumnWidths(const Node &node, int depth, QVector<qreal> *maxW, qreal scale,
                         const QSet<QString> &collapsed)
{
    while (maxW->size() <= depth)
        maxW->append(0);
    (*maxW)[depth] = qMax((*maxW)[depth], measureWidth(node.title, scale));
    if (collapsed.contains(node.id))
        return;
    for (const Node &child : node.children)
        collectColumnWidths(child, depth + 1, maxW, scale, collapsed);
}

qreal subtreeHeight(const Node &node, qreal scale, qreal vGap, const QSet<QString> &collapsed)
{
    const qreal h = measureHeight(node.title, scale);
    if (node.children.isEmpty() || collapsed.contains(node.id))
        return h;
    qreal sum = 0;
    for (int i = 0; i < node.children.size(); ++i) {
        if (i > 0)
            sum += vGap;
        sum += subtreeHeight(node.children.at(i), scale, vGap, collapsed);
    }
    return qMax(h, sum);
}

void placeColumns(const Node &node, int depth, qreal y, const QString &parentId,
                  const QVector<qreal> &colX, const QVector<qreal> &colW, qreal scale, qreal vGap,
                  const QSet<QString> &collapsed, QVector<Box> *boxes)
{
    const qreal h = measureHeight(node.title, scale);
    const qreal sh = subtreeHeight(node, scale, vGap, collapsed);
    const qreal w = colW.value(depth);
    const qreal x = colX.value(depth);
    boxes->append({node.id, node.title, parentId, x, y + sh / 2 - h / 2, w, h, depth});
    if (collapsed.contains(node.id))
        return;
    qreal childY = y;
    for (int i = 0; i < node.children.size(); ++i) {
        const qreal childH = subtreeHeight(node.children.at(i), scale, vGap, collapsed);
        placeColumns(node.children.at(i), depth + 1, childY, node.id, colX, colW, scale, vGap,
                     collapsed, boxes);
        childY += childH + vGap;
    }
}

} // namespace

bool looksLikeImageTarget(const QString &target)
{
    QString path = target;
    const int hash = path.indexOf(QLatin1Char('#'));
    if (hash >= 0)
        path = path.left(hash);
    const int pipe = path.indexOf(QLatin1Char('|'));
    if (pipe >= 0)
        path = path.left(pipe);
    const QString suffix = QFileInfo(path.trimmed()).suffix().toLower();
    static const QStringList extensions = {
        QStringLiteral("png"), QStringLiteral("jpg"), QStringLiteral("jpeg"),
        QStringLiteral("gif"), QStringLiteral("webp"), QStringLiteral("svg"),
    };
    return extensions.contains(suffix);
}

bool isLinkPlaceholder(const QString &title)
{
    QString t = title;
    t.remove(colorMarkerRe());
    t = t.trimmed();
    return t.isEmpty()
        || t == QStringLiteral("新節點")
        || t.compare(QStringLiteral("Untitled"), Qt::CaseInsensitive) == 0
        || t.compare(QStringLiteral("New node"), Qt::CaseInsensitive) == 0
        || t == QStringLiteral("![]")
        || t == QStringLiteral("![]()")
        || t == QStringLiteral("[[]]");
}

Markup parseMarkup(const QString &title)
{
    Markup markup;
    const QRegularExpressionMatch colorMatch = colorMarkerRe().match(title);
    if (colorMatch.hasMatch())
        markup.color = colorMatch.captured(1);
    QString working = title;
    working.remove(colorMarkerRe());
    markup.display = working;
    const QString trimmed = working.trimmed();
    if (trimmed.isEmpty())
        return markup;

    static const QRegularExpression wikiEmbed(
        QStringLiteral(R"(^!\[\[([^\]|]+)(?:\|([^\]]+))?\]\]$)"));
    static const QRegularExpression wiki(
        QStringLiteral(R"(^\[\[([^\]|]+)(?:\|([^\]]+))?\]\]$)"));
    static const QRegularExpression mdImage(
        QStringLiteral(R"(^!\[([^\]]*)\]\(([^)]*)\)$)"));
    static const QRegularExpression mdLink(
        QStringLiteral(R"(^\[([^\]]+)\]\(([^)]+)\)$)"));
    static const QRegularExpression inlineWiki(
        QStringLiteral(R"((!?)\[\[([^\]|]+)(?:\|([^\]]+))?\]\])"));
    static const QRegularExpression inlineImage(
        QStringLiteral(R"(!\[([^\]]*)\]\(([^)]+)\))"));

    auto applyWikiTarget = [&](const QString &raw) {
        QString path = raw.trimmed();
        QString frag;
        const int hash = path.indexOf(QLatin1Char('#'));
        if (hash >= 0) {
            frag = path.mid(hash + 1).trimmed();
            path = path.left(hash).trimmed();
        }
        markup.target = path;
        markup.fragment.clear();
        markup.fragmentKind.clear();
        if (frag.startsWith(QLatin1Char('^'))) {
            markup.fragmentKind = QStringLiteral("block");
            markup.fragment = frag.mid(1).trimmed();
        } else if (!frag.isEmpty()) {
            markup.fragmentKind = QStringLiteral("heading");
            markup.fragment = frag;
        }
    };

    auto fillWiki = [&](const QRegularExpressionMatch &match, bool embed, int targetIndex,
                        int aliasIndex) {
        markup.kind = embed ? QStringLiteral("embed") : QStringLiteral("wiki");
        applyWikiTarget(match.captured(targetIndex));
        markup.display = match.captured(aliasIndex).trimmed();
        if (markup.display.isEmpty()) {
            const QString stem = QFileInfo(markup.target).completeBaseName().isEmpty()
                ? markup.target
                : QFileInfo(markup.target).completeBaseName();
            if (!markup.fragment.isEmpty()) {
                const QString bit = markup.fragmentKind == QLatin1String("block")
                    ? QLatin1Char('^') + markup.fragment
                    : markup.fragment;
                markup.display = stem.isEmpty() ? bit : stem + QStringLiteral(" › ") + bit;
            } else {
                markup.display = stem;
            }
        }
        markup.embed = embed || looksLikeImageTarget(markup.target);
        if (markup.embed && looksLikeImageTarget(markup.target))
            markup.kind = QStringLiteral("image");
    };

    QRegularExpressionMatch match = wikiEmbed.match(trimmed);
    if (match.hasMatch()) {
        fillWiki(match, true, 1, 2);
        return markup;
    }
    match = wiki.match(trimmed);
    if (match.hasMatch()) {
        fillWiki(match, false, 1, 2);
        return markup;
    }
    match = mdImage.match(trimmed);
    if (match.hasMatch()) {
        markup.kind = QStringLiteral("image");
        markup.display = match.captured(1).trimmed();
        markup.target = match.captured(2).trimmed();
        markup.embed = true;
        if (markup.display.isEmpty())
            markup.display = QFileInfo(markup.target).completeBaseName();
        return markup;
    }
    match = mdLink.match(trimmed);
    if (match.hasMatch()) {
        markup.kind = QStringLiteral("link");
        markup.display = match.captured(1).trimmed();
        markup.target = match.captured(2).trimmed();
        return markup;
    }
    if (trimmed == QLatin1String("![]") || trimmed == QLatin1String("![]()")) {
        markup.kind = QStringLiteral("image");
        markup.display = QString();
        markup.target = QString();
        markup.embed = true;
        return markup;
    }

    match = inlineWiki.match(trimmed);
    if (match.hasMatch()) {
        const bool embed = match.captured(1) == QLatin1String("!");
        fillWiki(match, embed, 2, 3);
        if (match.captured(3).trimmed().isEmpty())
            markup.display = trimmed;
        return markup;
    }
    match = inlineImage.match(trimmed);
    if (match.hasMatch()) {
        markup.kind = QStringLiteral("image");
        markup.display = trimmed;
        markup.target = match.captured(2).trimmed();
        markup.embed = true;
        return markup;
    }
    return markup;
}

QVariantMap markupToVariant(const Markup &markup)
{
    return {
        {QStringLiteral("kind"), markup.kind},
        {QStringLiteral("display"), markup.display},
        {QStringLiteral("target"), markup.target},
        {QStringLiteral("fragment"), markup.fragment},
        {QStringLiteral("fragmentKind"), markup.fragmentKind},
        {QStringLiteral("color"), markup.color},
        {QStringLiteral("embed"), markup.embed},
    };
}

QStringList wikiTargets(const QString &markdown)
{
    static const QRegularExpression re(
        QStringLiteral(R"(!?\[\[([^\]|#]+)(?:#[^\]|]*)?(?:\|[^\]]*)?\]\])"));
    QStringList targets;
    const FrontMatter::Document document = FrontMatter::parse(markdown);
    bool inFence = false;
    const QStringList lines = document.body.split(QLatin1Char('\n'));
    for (const QString &line : lines) {
        if (line.trimmed().startsWith(QStringLiteral("```"))) {
            inFence = !inFence;
            continue;
        }
        if (inFence)
            continue;
        QRegularExpressionMatchIterator it = re.globalMatch(line);
        while (it.hasNext()) {
            const QString target = it.next().captured(1).trimmed();
            if (!target.isEmpty())
                targets.append(target);
        }
    }
    return targets;
}

QString rewriteGardenWikilinks(const QString &markdown, const QStringList &publishedStems)
{
    QSet<QString> stems;
    for (const QString &stem : publishedStems) {
        const QString key = QDir::fromNativeSeparators(stem.trimmed()).toLower();
        if (!key.isEmpty())
            stems.insert(key);
    }

    static const QRegularExpression re(
        QStringLiteral(R"((!?)\[\[([^\]|#]+)(#[^\]|]*)?(?:\|([^\]]*))?\]\])"));
    const FrontMatter::Document document = FrontMatter::parse(markdown);
    bool inFence = false;
    const QStringList lines = document.body.split(QLatin1Char('\n'));
    QStringList rewrittenLines;
    rewrittenLines.reserve(lines.size());
    for (const QString &line : lines) {
        if (line.trimmed().startsWith(QStringLiteral("```"))) {
            inFence = !inFence;
            rewrittenLines.append(line);
            continue;
        }
        if (inFence) {
            rewrittenLines.append(line);
            continue;
        }
        QString rewritten = line;
        QList<QRegularExpressionMatch> matches;
        QRegularExpressionMatchIterator it = re.globalMatch(line);
        while (it.hasNext())
            matches.append(it.next());
        for (int i = matches.size() - 1; i >= 0; --i) {
            const QRegularExpressionMatch match = matches.at(i);
            if (match.captured(1) == QLatin1String("!"))
                continue;
            const QString target = match.captured(2).trimmed();
            if (target.isEmpty())
                continue;
            const QString pathKey = QDir::fromNativeSeparators(target).toLower();
            const QString baseKey = QFileInfo(target).completeBaseName().toLower();
            if (stems.contains(pathKey) || stems.contains(baseKey))
                continue;
            const QString alias = match.captured(4).trimmed();
            rewritten.replace(match.capturedStart(), match.capturedLength(),
                              alias.isEmpty() ? target : alias);
        }
        rewrittenLines.append(rewritten);
    }
    const QString newBody = rewrittenLines.join(QLatin1Char('\n'));
    if (!document.hasFrontMatter)
        return newBody;
    return markdown.left(document.yamlEnd) + newBody;
}

void assignIds(Node &node, const QString &id)
{
    node.id = id;
    for (int i = 0; i < node.children.size(); ++i)
        assignIds(node.children[i], id + QLatin1Char('.') + QString::number(i));
}

Node *findNode(Node &node, const QString &id)
{
    if (node.id == id)
        return &node;
    for (Node &child : node.children) {
        if (Node *found = findNode(child, id))
            return found;
    }
    return nullptr;
}

const Node *findNode(const Node &node, const QString &id)
{
    if (node.id == id)
        return &node;
    for (const Node &child : node.children) {
        if (const Node *found = findNode(child, id))
            return found;
    }
    return nullptr;
}

QVariantMap toVariant(const Node &node)
{
    QVariantList children;
    for (const Node &child : node.children)
        children.append(toVariant(child));
    return {
        {QStringLiteral("id"), node.id},
        {QStringLiteral("title"), node.title},
        {QStringLiteral("children"), children},
    };
}

Node fromVariant(const QVariantMap &map)
{
    Node node;
    node.id = map.value(QStringLiteral("id")).toString();
    node.title = map.value(QStringLiteral("title")).toString();
    const QVariantList children = map.value(QStringLiteral("children")).toList();
    for (const QVariant &child : children)
        node.children.append(fromVariant(child.toMap()));
    return node;
}

QVariantList flatten(const Node &node, int depth, const QStringList &collapsedIds)
{
    QVariantList rows;
    const Markup markup = parseMarkup(node.title);
    const bool collapsed = collapsedIds.contains(node.id);
    rows.append(QVariantMap{
        {QStringLiteral("id"), node.id},
        {QStringLiteral("title"), node.title},
        {QStringLiteral("display"), markup.display},
        {QStringLiteral("kind"), markup.kind},
        {QStringLiteral("color"), markup.color},
        {QStringLiteral("depth"), depth},
        {QStringLiteral("childCount"), node.children.size()},
        {QStringLiteral("collapsible"), !node.children.isEmpty()},
        {QStringLiteral("collapsed"), collapsed},
    });
    if (collapsed)
        return rows;
    for (const Node &child : node.children)
        rows.append(flatten(child, depth + 1, collapsedIds));
    return rows;
}

Tree parse(const QString &markdown)
{
    Tree tree;
    const FrontMatter::Document document = FrontMatter::parse(markdown);
    const QString yamlLayout = FrontMatter::displayValue(document.fields.value(QStringLiteral("layout")));
    if (yamlLayout == QLatin1String("right") || yamlLayout == QLatin1String("down"))
        tree.layout = yamlLayout;

    const QStringList lines = bodyLines(document.body);
    QVector<FlatItem> listItems;
    QVector<FlatItem> headings;
    QString firstHeading;
    bool inFence = false;
    int listStart = -1;
    int listEnd = -1;

    for (int i = 0; i < lines.size(); ++i) {
        const QString line = lines.at(i);
        if (isFence(line)) {
            inFence = !inFence;
            continue;
        }
        if (inFence)
            continue;

        int headingLevel = 0;
        QString headingTitle;
        if (matchHeading(line, &headingLevel, &headingTitle)) {
            headings.append({headingLevel, headingTitle});
            if (firstHeading.isEmpty())
                firstHeading = headingTitle;
            continue;
        }

        int indent = 0;
        QString itemTitle;
        if (matchListItem(line, &indent, &itemTitle)) {
            if (listStart < 0)
                listStart = i;
            listEnd = i;
            listItems.append({indent, itemTitle});
            continue;
        }

        const int cols = leadingColumns(line);
        if (!listItems.isEmpty() && cols > listItems.last().key && !isFence(line)) {
            listItems.last().title += QLatin1Char('\n') + line.trimmed();
            listEnd = i;
            continue;
        }
        if (listItems.isEmpty() && !headings.isEmpty() && cols >= 2 && !isFence(line)
            && !line.trimmed().isEmpty()) {
            headings.last().title += QLatin1Char('\n') + line.trimmed();
            if (headings.size() == 1)
                firstHeading = headings.last().title;
            continue;
        }
    }

    if (!listItems.isEmpty()) {
        tree.fromLists = true;
        tree.root.title = firstHeading.isEmpty()
            ? FrontMatter::displayValue(document.fields.value(QStringLiteral("title")))
            : firstHeading;
        if (tree.root.title.isEmpty())
            tree.root.title = QStringLiteral("Untitled");
        tree.root.children = nestList(listItems);

        QStringList tailLines;
        bool afterTree = false;
        inFence = false;
        for (int i = 0; i < lines.size(); ++i) {
            const QString line = lines.at(i);
            if (isFence(line))
                inFence = !inFence;
            if (!afterTree) {
                if (listEnd >= 0 && i > listEnd && !line.trimmed().isEmpty()
                    && !matchListItem(line, nullptr, nullptr)
                    && !matchHeading(line, nullptr, nullptr)
                    && !isFence(line))
                    afterTree = true;
            }
            if (afterTree)
                tailLines.append(line);
        }
        while (!tailLines.isEmpty() && tailLines.constFirst().trimmed().isEmpty())
            tailLines.removeFirst();
        tree.tail = tailLines.join(QLatin1Char('\n'));
    } else if (!headings.isEmpty()) {
        int h1Count = 0;
        for (const FlatItem &item : headings) {
            if (item.key == 1)
                ++h1Count;
        }
        if (h1Count <= 1 && headings.constFirst().key == 1) {
            tree.root.title = headings.constFirst().title;
            QVector<FlatItem> rest = headings;
            rest.removeFirst();
            tree.root.children = nest(rest, 1);
        } else {
            tree.root.title = FrontMatter::displayValue(document.fields.value(QStringLiteral("title")));
            if (tree.root.title.isEmpty())
                tree.root.title = QStringLiteral("Mindmap");
            tree.root.children = nest(headings, 0);
        }
    } else {
        tree.root.title = FrontMatter::displayValue(document.fields.value(QStringLiteral("title")));
        if (tree.root.title.isEmpty())
            tree.root.title = FrontMatter::headingTitle(document.body);
        if (tree.root.title.isEmpty())
            tree.root.title = QStringLiteral("Untitled");
        tree.tail = document.body.trimmed();
        if (tree.tail == QStringLiteral("# ") + tree.root.title)
            tree.tail.clear();
    }

    assignIds(tree.root);
    return tree;
}

QString serialize(const QString &originalMarkdown, const Tree &tree)
{
    QString flagged = FrontMatter::setField(originalMarkdown, QStringLiteral("mindmap"),
                                            QStringLiteral("true"));
    if (tree.layout != QLatin1String("right"))
        flagged = FrontMatter::setField(flagged, QStringLiteral("layout"), tree.layout);
    else
        flagged = FrontMatter::removeField(flagged, QStringLiteral("layout"));

    QStringList body;
    const QStringList rootLines = tree.root.title.split(QLatin1Char('\n'));
    body.append(QStringLiteral("# ")
                + (rootLines.isEmpty() ? QString() : rootLines.constFirst()));
    for (int i = 1; i < rootLines.size(); ++i)
        body.append(QStringLiteral("  ") + rootLines.at(i));
    body.append(QString());
    for (const Node &child : tree.root.children)
        appendListLines(&body, child, 0);
    if (!tree.tail.trimmed().isEmpty()) {
        body.append(QString());
        body.append(tree.tail.trimmed());
    }
    body.append(QString());
    const QString newBody = body.join(QLatin1Char('\n'));

    const FrontMatter::Document document = FrontMatter::parse(flagged);
    if (!document.hasFrontMatter)
        return newBody;
    const int bodyOffset = flagged.size() - document.body.size();
    if (bodyOffset < 0 || bodyOffset > flagged.size())
        return newBody;
    return flagged.left(bodyOffset) + newBody;
}

QStringList collapsedIds(const QString &markdown)
{
    const QVariant value = FrontMatter::parse(markdown).fields.value(QStringLiteral("collapsed"));
    const int type = value.metaType().id();
    if (type == QMetaType::QStringList)
        return value.toStringList();
    if (type == QMetaType::QVariantList) {
        QStringList ids;
        for (const QVariant &item : value.toList()) {
            const QString id = item.toString().trimmed();
            if (!id.isEmpty())
                ids.append(id);
        }
        return ids;
    }
    const QString raw = FrontMatter::displayValue(value).trimmed();
    if (raw.isEmpty())
        return {};
    QStringList ids;
    for (const QString &part : raw.split(QLatin1Char('|'), Qt::SkipEmptyParts)) {
        const QString id = part.trimmed();
        if (!id.isEmpty())
            ids.append(id);
    }
    return ids;
}

QString nodeColor(const QString &title)
{
    const QRegularExpressionMatch match = colorMarkerRe().match(title);
    return match.hasMatch() ? match.captured(1) : QString();
}

QString applyNodeColor(const QString &title, const QString &color)
{
    QString plain = title;
    plain.remove(colorMarkerRe());
    plain = plain.trimmed();
    QString normalized = color.trimmed();
    if (normalized.startsWith(QLatin1Char('#')))
        normalized = normalized.left(7);
    static const QRegularExpression hex(QStringLiteral("^#[0-9A-Fa-f]{6}$"));
    if (!hex.match(normalized).hasMatch())
        return plain;
    return plain + QStringLiteral(" <!--c:") + normalized + QStringLiteral("-->");
}

QVariantMap layout(const Tree &tree, qreal textScale, const QStringList &collapsedIds)
{
    const qreal scale = qBound(0.8, textScale, 2.4);
    const qreal pad = 48.0 * scale;
    const qreal hGap = 88.0 * scale;
    const qreal vGap = 16.0 * scale;
    QSet<QString> collapsed;
    for (const QString &id : collapsedIds)
        collapsed.insert(id);

    QVector<qreal> colW;
    collectColumnWidths(tree.root, 0, &colW, scale, collapsed);
    QVector<qreal> colX(colW.size(), pad);
    for (int d = 1; d < colW.size(); ++d)
        colX[d] = colX[d - 1] + colW[d - 1] + hGap;

    QVector<Box> boxes;
    placeColumns(tree.root, 0, pad, QString(), colX, colW, scale, vGap, collapsed, &boxes);

    QHash<QString, Box> byId;
    for (const Box &box : boxes)
        byId.insert(box.id, box);

    qreal maxX = pad;
    qreal maxY = pad;
    QVariantList nodes;
    QVariantList edges;
    for (const Box &box : boxes) {
        maxX = qMax(maxX, box.x + box.w);
        maxY = qMax(maxY, box.y + box.h);
        const Markup markup = parseMarkup(box.title);
        QVariantMap node = markupToVariant(markup);
        node.insert(QStringLiteral("id"), box.id);
        node.insert(QStringLiteral("title"), box.title);
        node.insert(QStringLiteral("parentId"), box.parentId);
        node.insert(QStringLiteral("x"), box.x);
        node.insert(QStringLiteral("y"), box.y);
        node.insert(QStringLiteral("w"), box.w);
        node.insert(QStringLiteral("h"), box.h);
        node.insert(QStringLiteral("depth"), box.depth);
        const Node *source = findNode(tree.root, box.id);
        const int childCount = source ? source->children.size() : 0;
        node.insert(QStringLiteral("childCount"), childCount);
        node.insert(QStringLiteral("collapsible"), childCount > 0);
        node.insert(QStringLiteral("collapsed"), collapsed.contains(box.id));
        nodes.append(node);
        if (box.parentId.isEmpty() || !byId.contains(box.parentId))
            continue;
        const Box &parent = byId.value(box.parentId);
        const qreal x1 = parent.x + parent.w;
        const qreal y1 = parent.y + parent.h / 2;
        const qreal x2 = box.x;
        const qreal y2 = box.y + box.h / 2;
        edges.append(QVariantMap{
            {QStringLiteral("from"), parent.id},
            {QStringLiteral("to"), box.id},
            {QStringLiteral("x1"), x1},
            {QStringLiteral("y1"), y1},
            {QStringLiteral("x2"), x2},
            {QStringLiteral("y2"), y2},
            {QStringLiteral("cx"), (x1 + x2) / 2},
        });
    }

    return {
        {QStringLiteral("width"), maxX + pad},
        {QStringLiteral("height"), maxY + pad},
        {QStringLiteral("nodes"), nodes},
        {QStringLiteral("edges"), edges},
    };
}

QString rename(Node &root, const QString &id, const QString &title)
{
    Node *node = findNode(root, id);
    if (!node)
        return {};
    node->title = title.trimmed().isEmpty() ? QStringLiteral("Untitled") : title.trimmed();
    return id;
}

QString addChild(Node &root, const QString &parentId, const QString &title)
{
    Node *parent = findNode(root, parentId);
    if (!parent)
        return {};
    Node child;
    child.title = title.trimmed().isEmpty() ? QStringLiteral("新節點") : title.trimmed();
    parent->children.append(child);
    assignIds(root);
    return parent->children.last().id;
}

QString addSibling(Node &root, const QString &id, const QString &title)
{
    if (root.id == id)
        return addChild(root, id, title);
    const ParentLoc loc = locate(root, id);
    if (!loc.parent)
        return {};
    Node sibling;
    sibling.title = title.trimmed().isEmpty() ? QStringLiteral("新節點") : title.trimmed();
    loc.parent->children.insert(loc.index + 1, sibling);
    assignIds(root);
    return loc.parent->children.at(loc.index + 1).id;
}

QString removeNode(Node &root, const QString &id)
{
    if (root.id == id)
        return {};
    const ParentLoc loc = locate(root, id);
    if (!loc.parent)
        return {};
    const QVector<Node> promoted = loc.parent->children.at(loc.index).children;
    loc.parent->children.removeAt(loc.index);
    for (int i = 0; i < promoted.size(); ++i)
        loc.parent->children.insert(loc.index + i, promoted.at(i));
    assignIds(root);
    return loc.parent->id;
}

QString reparent(Node &root, const QString &id, const QString &newParentId, int index)
{
    if (id == newParentId || root.id == id)
        return {};
    Node *destination = findNode(root, newParentId);
    const Node *movedNode = findNode(root, id);
    if (!destination || !movedNode)
        return {};
    if (isDescendant(*movedNode, newParentId))
        return {};

    const ParentLoc loc = locate(root, id);
    if (!loc.parent)
        return {};
    Node moved = loc.parent->children.takeAt(loc.index);
    if (destination == loc.parent && index > loc.index)
        --index;
    if (index < 0 || index > destination->children.size())
        index = destination->children.size();
    destination->children.insert(index, moved);
    assignIds(root);
    return destination->children.at(index).id;
}

QString indent(Node &root, const QString &id)
{
    if (root.id == id)
        return {};
    const ParentLoc loc = locate(root, id);
    if (!loc.parent || loc.index <= 0)
        return {};
    return reparent(root, id, loc.parent->children.at(loc.index - 1).id, -1);
}

QString outdent(Node &root, const QString &id)
{
    if (root.id == id)
        return {};
    const ParentLoc loc = locate(root, id);
    if (!loc.parent || loc.parent->id == root.id)
        return {};
    const ParentLoc grand = locate(root, loc.parent->id);
    if (!grand.parent)
        return {};
    return reparent(root, id, grand.parent->id, grand.index + 1);
}

} // namespace MindMap
