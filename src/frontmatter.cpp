#include "frontmatter.h"
#include "codeblocks.h"

#include <QRegularExpression>
#include <QSet>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>

namespace FrontMatter {
namespace {

QString unquote(QString value) {
    value = value.trimmed();
    if (value.size() >= 2) {
        const QChar first = value.front();
        const QChar last = value.back();
        if ((first == QLatin1Char('"') && last == QLatin1Char('"'))
            || (first == QLatin1Char('\'') && last == QLatin1Char('\''))) {
            value = value.mid(1, value.size() - 2);
            value.replace(QStringLiteral("\\\""), QStringLiteral("\""));
            value.replace(QStringLiteral("\\'"), QStringLiteral("'"));
        }
    }
    return value;
}

QVariant parseScalar(const QString &raw) {
    const QString trimmed = raw.trimmed();
    if (trimmed.startsWith(QLatin1Char('[')) && trimmed.endsWith(QLatin1Char(']'))) {
        const QString inner = trimmed.mid(1, trimmed.size() - 2).trimmed();
        QStringList items;
        QString current;
        bool inQuotes = false;
        QChar quote;
        for (const QChar ch : inner) {
            if (!inQuotes && (ch == QLatin1Char('"') || ch == QLatin1Char('\''))) {
                inQuotes = true;
                quote = ch;
                current.append(ch);
            } else if (inQuotes && ch == quote) {
                inQuotes = false;
                current.append(ch);
            } else if (!inQuotes && ch == QLatin1Char(',')) {
                const QString item = unquote(current);
                if (!item.isEmpty())
                    items.append(item);
                current.clear();
            } else {
                current.append(ch);
            }
        }
        const QString item = unquote(current);
        if (!item.isEmpty())
            items.append(item);
        return items;
    }
    return unquote(trimmed);
}

QString escapeYamlValue(const QString &value) {
    if (value.isEmpty())
        return QStringLiteral("\"\"");
    static const QRegularExpression numericId(QStringLiteral(R"(^[0-9]+(?:\.[0-9]+)+$)"));
    const bool needsQuotes = value.contains(QLatin1Char(':')) || value.contains(QLatin1Char('#'))
        || value.contains(QLatin1Char('{')) || value.contains(QLatin1Char('['))
        || value.contains(QLatin1Char('|'))
        || numericId.match(value).hasMatch()
        || value.startsWith(QLatin1Char(' ')) || value.endsWith(QLatin1Char(' '))
        || value.contains(QLatin1Char('\n'));
    if (!needsQuotes)
        return value;
    QString escaped = value;
    escaped.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
    escaped.replace(QLatin1Char('"'), QStringLiteral("\\\""));
    return QLatin1Char('"') + escaped + QLatin1Char('"');
}

int frontMatterOpenOffset(const QString &text) {
    int i = 0;
    if (!text.isEmpty() && text.front() == QChar(0xFEFF))
        i = 1;
    while (i < text.size() && (text[i] == QLatin1Char('\n') || text[i] == QLatin1Char('\r')))
        ++i;
    if (text.mid(i, 3) != QLatin1String("---"))
        return -1;
    const int after = i + 3;
    if (after < text.size() && text[after] != QLatin1Char('\n') && text[after] != QLatin1Char('\r')
        && !text[after].isSpace())
        return -1;
    return i;
}

int frontMatterCloseOffset(const QString &text, int openOffset) {
    const int searchFrom = text.indexOf(QLatin1Char('\n'), openOffset);
    if (searchFrom < 0)
        return -1;

    static const QRegularExpression closer(QStringLiteral("(?m)^(?:---|\\.\\.\\.)\\s*$"));
    QRegularExpressionMatchIterator it = closer.globalMatch(text.mid(searchFrom + 1));
    if (!it.hasNext())
        return -1;
    const QRegularExpressionMatch match = it.next();
    return searchFrom + 1 + match.capturedStart();
}

QString writeYamlField(const QString &text, const QString &key, const QString &renderedValue) {
    if (key.trimmed().isEmpty())
        return text;

    const QString rendered = key.trimmed() + QStringLiteral(": ") + renderedValue;
    const int open = frontMatterOpenOffset(text);
    if (open < 0) {
        QString prefix = QStringLiteral("---\n") + rendered + QStringLiteral("\n---\n");
        if (!text.isEmpty() && !text.startsWith(QLatin1Char('\n')))
            prefix.append(QLatin1Char('\n'));
        return prefix + text;
    }

    const int close = frontMatterCloseOffset(text, open);
    if (close < 0)
        return text;

    const int yamlStart = text.indexOf(QLatin1Char('\n'), open);
    QString yaml = text.mid(yamlStart + 1, close - (yamlStart + 1));
    const QStringList lines = yaml.split(QRegularExpression(QStringLiteral("\\r?\\n")));
    QStringList rewritten;
    bool replaced = false;
    for (int i = 0; i < lines.size(); ++i) {
        const QString line = lines.at(i);
        if (!replaced && !line.isEmpty() && !line.front().isSpace()) {
            const int colon = line.indexOf(QLatin1Char(':'));
            if (colon > 0 && line.left(colon).trimmed() == key.trimmed()) {
                rewritten.append(rendered);
                replaced = true;
                if (line.mid(colon + 1).trimmed().isEmpty()) {
                    while (i + 1 < lines.size() && !lines.at(i + 1).isEmpty()
                           && lines.at(i + 1).front().isSpace())
                        ++i;
                }
                continue;
            }
        }
        rewritten.append(line);
    }
    if (!replaced) {
        while (!rewritten.isEmpty() && rewritten.last().isEmpty())
            rewritten.removeLast();
        rewritten.append(rendered);
        rewritten.append(QString());
    }

    yaml = rewritten.join(QLatin1Char('\n'));
    if (!yaml.endsWith(QLatin1Char('\n')))
        yaml.append(QLatin1Char('\n'));

    return text.left(yamlStart + 1) + yaml + text.mid(close);
}

QString formatYamlList(const QStringList &values) {
    QStringList items;
    for (const QString &value : values) {
        const QString trimmed = value.trimmed();
        if (!trimmed.isEmpty())
            items.append(escapeYamlValue(trimmed));
    }
    return QLatin1Char('[') + items.join(QStringLiteral(", ")) + QLatin1Char(']');
}

QStringList uniqueTags(const QStringList &tags) {
    QStringList unique;
    QSet<QString> seen;
    for (const QString &tag : tags) {
        const QString key = tag.toLower();
        if (key.isEmpty() || seen.contains(key))
            continue;
        seen.insert(key);
        unique.append(tag);
    }
    return unique;
}

bool tagEquals(const QString &left, const QString &right) {
    return left.compare(right, Qt::CaseInsensitive) == 0;
}

bool tagIsOrChildOf(const QString &tag, const QString &parent) {
    if (tagEquals(tag, parent))
        return true;
    return tag.startsWith(parent + QLatin1Char('/'), Qt::CaseInsensitive);
}

QString retargetTag(const QString &tag, const QString &from, const QString &to) {
    if (tagEquals(tag, from))
        return to;
    if (tag.startsWith(from + QLatin1Char('/'), Qt::CaseInsensitive))
        return to + tag.mid(from.size());
    return tag;
}

const QRegularExpression &hashtagRe() {
    static const QRegularExpression re(
        QStringLiteral(R"((?<![\w#])#([\p{L}\p{N}_][\p{L}\p{N}_/\-]*))"),
        QRegularExpression::UseUnicodePropertiesOption);
    return re;
}

const QRegularExpression &openTaskRe() {
    static const QRegularExpression re(QStringLiteral(R"((?m)^\s*[-*+]\s+\[ \](?:\s|$))"));
    return re;
}

const QRegularExpression &closedTaskRe() {
    static const QRegularExpression re(QStringLiteral(R"((?m)^\s*[-*+]\s+\[[xX]\](?:\s|$))"));
    return re;
}

int countMatches(const QRegularExpression &re, const QString &text) {
    int count = 0;
    QRegularExpressionMatchIterator it = re.globalMatch(text);
    while (it.hasNext()) {
        it.next();
        ++count;
    }
    return count;
}

} // namespace

Document parse(const QString &text) {
    Document document;
    document.body = text;

    const int open = frontMatterOpenOffset(text);
    if (open < 0)
        return document;

    const int close = frontMatterCloseOffset(text, open);
    if (close < 0)
        return document;

    const int yamlStart = text.indexOf(QLatin1Char('\n'), open);
    if (yamlStart < 0 || yamlStart >= close)
        return document;

    document.hasFrontMatter = true;
    document.yamlBlock = text.mid(yamlStart + 1, close - (yamlStart + 1));
    document.yamlStart = open;

    int bodyStart = text.indexOf(QLatin1Char('\n'), close);
    document.yamlEnd = bodyStart < 0 ? text.size() : bodyStart + 1;
    document.body = bodyStart < 0 ? QString() : text.mid(bodyStart + 1);
    if (document.body.startsWith(QLatin1Char('\r')))
        document.body.remove(0, 1);

    const QStringList lines = document.yamlBlock.split(QRegularExpression(QStringLiteral("\\r?\\n")));
    for (int i = 0; i < lines.size(); ++i) {
        const QString line = lines.at(i);
        if (line.trimmed().isEmpty() || line.trimmed().startsWith(QLatin1Char('#')))
            continue;
        if (!line.isEmpty() && line.front().isSpace())
            continue;

        const int colon = line.indexOf(QLatin1Char(':'));
        if (colon <= 0)
            continue;

        const QString key = line.left(colon).trimmed();
        if (key.isEmpty())
            continue;

        QString rawValue = line.mid(colon + 1);
        if (rawValue.trimmed().isEmpty()) {
            while (i + 1 < lines.size() && !lines.at(i + 1).isEmpty()
                   && lines.at(i + 1).front().isSpace())
                ++i;
            continue;
        }

        if (!document.fields.contains(key))
            document.fieldOrder.append(key);
        document.fields.insert(key, parseScalar(rawValue));
    }

    return document;
}

QString setField(const QString &text, const QString &key, const QString &value) {
    return writeYamlField(text, key, escapeYamlValue(value));
}

QString setListField(const QString &text, const QString &key, const QStringList &values) {
    QStringList cleaned;
    for (const QString &value : values) {
        const QString tag = normalizeTag(value);
        if (!tag.isEmpty())
            cleaned.append(tag);
    }
    cleaned = uniqueTags(cleaned);
    if (cleaned.isEmpty())
        return removeField(text, key);
    return writeYamlField(text, key, formatYamlList(cleaned));
}

QString removeField(const QString &text, const QString &key) {
    if (key.trimmed().isEmpty())
        return text;

    const int open = frontMatterOpenOffset(text);
    if (open < 0)
        return text;
    const int close = frontMatterCloseOffset(text, open);
    if (close < 0)
        return text;

    const int yamlStart = text.indexOf(QLatin1Char('\n'), open);
    QString yaml = text.mid(yamlStart + 1, close - (yamlStart + 1));
    const QStringList lines = yaml.split(QRegularExpression(QStringLiteral("\\r?\\n")));
    QStringList rewritten;
    bool removed = false;
    for (int i = 0; i < lines.size(); ++i) {
        const QString line = lines.at(i);
        if (!removed && !line.isEmpty() && !line.front().isSpace()) {
            const int colon = line.indexOf(QLatin1Char(':'));
            if (colon > 0 && line.left(colon).trimmed() == key.trimmed()) {
                removed = true;
                if (line.mid(colon + 1).trimmed().isEmpty()) {
                    while (i + 1 < lines.size() && !lines.at(i + 1).isEmpty()
                           && lines.at(i + 1).front().isSpace())
                        ++i;
                }
                continue;
            }
        }
        rewritten.append(line);
    }
    if (!removed)
        return text;

    yaml = rewritten.join(QLatin1Char('\n'));
    if (!yaml.endsWith(QLatin1Char('\n')))
        yaml.append(QLatin1Char('\n'));
    return text.left(yamlStart + 1) + yaml + text.mid(close);
}

QString displayValue(const QVariant &value) {
    if (value.metaType().id() == QMetaType::QStringList)
        return value.toStringList().join(QStringLiteral(", "));
    return value.toString();
}

QString headingTitle(const QString &body) {
    static const QRegularExpression heading(QStringLiteral("(?m)^#+\\s+(.+)$"));
    const QRegularExpressionMatch match = heading.match(body);
    return match.hasMatch() ? match.captured(1).trimmed() : QString();
}

QString recordTitle(const QVariantMap &fields, const QString &body, const QString &fallbackName) {
    const QString fromFields = displayValue(fields.value(QStringLiteral("title")));
    if (!fromFields.isEmpty())
        return fromFields;
    const QString heading = headingTitle(body);
    if (!heading.isEmpty())
        return heading;
    return fallbackName;
}

bool looksLikeIsoDate(const QString &value) {
    return !isoDate(value).isEmpty();
}

QString isoDate(const QString &value) {
    const QString trimmed = value.trimmed();
    static const QRegularExpression iso(QStringLiteral("^(\\d{4}-\\d{2}-\\d{2})"));
    const QRegularExpressionMatch match = iso.match(trimmed);
    return match.hasMatch() ? match.captured(1) : QString();
}

bool isTruthy(const QVariant &value) {
    if (!value.isValid() || value.isNull())
        return false;
    if (value.metaType().id() == QMetaType::Bool)
        return value.toBool();
    if (value.metaType().id() == QMetaType::Int || value.metaType().id() == QMetaType::LongLong)
        return value.toLongLong() != 0;

    const QString text = displayValue(value).trimmed().toLower();
    return text == QLatin1String("true") || text == QLatin1String("yes")
        || text == QLatin1String("1") || text == QLatin1String("on");
}

bool isGardenPublishable(const QVariantMap &fields) {
    if (isTruthy(fields.value(QStringLiteral("draft"))))
        return false;
    if (isTruthy(fields.value(QStringLiteral("publish"))))
        return true;
    const QString layout = displayValue(fields.value(QStringLiteral("layout"))).trimmed().toLower();
    return layout == QLatin1String("post") || layout == QLatin1String("page");
}

bool isSlidevNote(const QVariantMap &fields) {
    if (isTruthy(fields.value(QStringLiteral("draft"))))
        return false;
    if (isTruthy(fields.value(QStringLiteral("slidev"))))
        return true;
    const QString layout = displayValue(fields.value(QStringLiteral("layout"))).trimmed().toLower();
    return layout == QLatin1String("slides");
}

bool isThreadsPost(const QVariantMap &fields) {
    if (fields.contains(QStringLiteral("threads")))
        return isTruthy(fields.value(QStringLiteral("threads")));
    return displayValue(fields.value(QStringLiteral("platform"))).trimmed().compare(
               QStringLiteral("threads"), Qt::CaseInsensitive)
        == 0;
}

int threadCharacterLimit() {
    return 500;
}

QVariantList threadPosts(const QString &text) {
    const Document document = parse(text);
    QString body = document.body;
    body.replace(QLatin1Char('\r'), QString());
    const QStringList lines = body.split(QLatin1Char('\n'));
    QStringList segments{QString()};
    bool inFence = false;
    for (const QString &line : lines) {
        if (line.trimmed().startsWith(QStringLiteral("```")))
            inFence = !inFence;
        if (!inFence && line.trimmed() == QLatin1String("---")) {
            segments.append(QString());
            continue;
        }
        QString &current = segments.last();
        if (!current.isEmpty())
            current.append(QLatin1Char('\n'));
        current.append(line);
    }
    while (!segments.isEmpty() && segments.last().trimmed().isEmpty())
        segments.removeLast();

    QVariantList posts;
    const int total = segments.size();
    for (int i = 0; i < total; ++i) {
        const QString segment = segments.at(i).trimmed();
        const int chars = segment.size();
        posts.append(QVariantMap{
            {QStringLiteral("index"), i},
            {QStringLiteral("total"), total},
            {QStringLiteral("role"), i == 0 ? QStringLiteral("post") : QStringLiteral("reply")},
            {QStringLiteral("text"), segment},
            {QStringLiteral("blocks"), CodeBlocks::bodyBlocks(segment)},
            {QStringLiteral("chars"), chars},
            {QStringLiteral("overLimit"), chars > threadCharacterLimit()},
        });
    }
    return posts;
}

QString previewMarkdown(const QString &text) {
    const Document document = parse(text);
    QString body = document.body;
    body.replace(QLatin1Char('\r'), QString());
    const QStringList lines = body.split(QLatin1Char('\n'));
    QStringList rendered;
    rendered.reserve(lines.size());
    bool inFence = false;
    static const QRegularExpression bareUrl(
        QStringLiteral(R"((?<![\w/<(])((?:https?://|www\.)[^\s<>\"'）)」']+))"));
    static const QString urlTails = QStringLiteral(".,;:!?)]}>\"'");
    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines.at(i);
        if (line.trimmed().startsWith(QStringLiteral("```"))) {
            inFence = !inFence;
            rendered.append(line);
            continue;
        }
        if (!inFence) {
            QString rebuilt;
            int last = 0;
            QRegularExpressionMatchIterator it = bareUrl.globalMatch(line);
            while (it.hasNext()) {
                QRegularExpressionMatch match = it.next();
                QString url = match.captured(1);
                int length = url.size();
                while (length > 0 && urlTails.contains(url.back())) {
                    url.chop(1);
                    --length;
                }
                if (length <= 0)
                    continue;
                const int start = match.capturedStart(1);
                const QString before = line.left(start);
                if (start > 0 && (line.at(start - 1) == QLatin1Char('<')
                                  || line.at(start - 1) == QLatin1Char('(')))
                    continue;
                rebuilt += line.mid(last, start - last);
                const QString href = url.startsWith(QLatin1String("www."), Qt::CaseInsensitive)
                    ? QStringLiteral("https://") + url
                    : url;
                rebuilt += QLatin1Char('<') + href + QLatin1Char('>');
                last = start + length;
            }
            rebuilt += line.mid(last);
            line = rebuilt;
        }
        if (!inFence && i + 1 < lines.size()) {
            const QString next = lines.at(i + 1);
            const bool nextFence = next.trimmed().startsWith(QStringLiteral("```"));
            if (!line.trimmed().isEmpty() && !next.trimmed().isEmpty() && !nextFence
                && !line.endsWith(QStringLiteral("  ")))
                line.append(QStringLiteral("  "));
        }
        rendered.append(line);
    }
    return rendered.join(QLatin1Char('\n'));
}

QString normalizeTag(const QString &tag) {
    QString value = tag.trimmed();
    while (value.startsWith(QLatin1Char('#')))
        value.remove(0, 1);
    value = value.trimmed();
    value.replace(QLatin1Char(' '), QLatin1Char('-'));
    if (value.isEmpty() || value == QLatin1String("/") || value.contains(QRegularExpression(QStringLiteral("\\s"))))
        return {};
    return value;
}

QStringList tagsFromValue(const QVariant &value) {
    QStringList tags;
    if (value.metaType().id() == QMetaType::QStringList) {
        for (const QString &item : value.toStringList()) {
            const QString tag = normalizeTag(item);
            if (!tag.isEmpty())
                tags.append(tag);
        }
        return uniqueTags(tags);
    }

    const QString text = displayValue(value).trimmed();
    if (text.isEmpty())
        return {};
    for (const QString &part : text.split(QRegularExpression(QStringLiteral("[,\\s]+")),
                                          Qt::SkipEmptyParts)) {
        const QString tag = normalizeTag(part);
        if (!tag.isEmpty())
            tags.append(tag);
    }
    return uniqueTags(tags);
}

QStringList hashtagsFromBody(const QString &body) {
    QStringList tags;
    const QStringList lines = body.split(QRegularExpression(QStringLiteral("\\r?\\n")));
    bool inFence = false;
    for (const QString &line : lines) {
        if (line.trimmed().startsWith(QStringLiteral("```"))) {
            inFence = !inFence;
            continue;
        }
        if (inFence)
            continue;
        QRegularExpressionMatchIterator it = hashtagRe().globalMatch(line);
        while (it.hasNext()) {
            const QString tag = normalizeTag(it.next().captured(1));
            if (!tag.isEmpty())
                tags.append(tag);
        }
    }
    return uniqueTags(tags);
}

QStringList allTags(const QString &text) {
    const Document document = parse(text);
    QStringList tags = tagsFromValue(document.fields.value(QStringLiteral("tags")));
    tags += hashtagsFromBody(document.body);
    return uniqueTags(tags);
}

QString addTag(const QString &text, const QString &tag) {
    const QString name = normalizeTag(tag);
    if (name.isEmpty())
        return text;

    const Document document = parse(text);
    QStringList yamlTags = tagsFromValue(document.fields.value(QStringLiteral("tags")));
    bool yamlHas = false;
    for (const QString &existing : yamlTags) {
        if (tagEquals(existing, name)) {
            yamlHas = true;
            break;
        }
    }
    if (!yamlHas)
        yamlTags.append(name);

    QString updated = setListField(text, QStringLiteral("tags"), yamlTags);
    const Document afterYaml = parse(updated);
    bool bodyHas = false;
    for (const QString &existing : hashtagsFromBody(afterYaml.body)) {
        if (tagEquals(existing, name)) {
            bodyHas = true;
            break;
        }
    }
    if (bodyHas)
        return updated;

    QString body = afterYaml.body;
    if (!body.isEmpty() && !body.endsWith(QLatin1Char('\n')))
        body.append(QLatin1Char('\n'));
    body.append(QLatin1Char('#') + name + QLatin1Char('\n'));
    const int bodyOffset = updated.size() - afterYaml.body.size();
    if (bodyOffset < 0 || bodyOffset > updated.size())
        return updated + QLatin1Char('\n') + QLatin1Char('#') + name + QLatin1Char('\n');
    return updated.left(bodyOffset) + body;
}

QString rewriteTags(const QString &text, const QString &fromRaw, const QString &toRaw) {
    const QString from = normalizeTag(fromRaw);
    const QString to = normalizeTag(toRaw);
    if (from.isEmpty() || to.isEmpty() || tagEquals(from, to))
        return text;

    const Document document = parse(text);
    QStringList yamlTags = tagsFromValue(document.fields.value(QStringLiteral("tags")));
    QStringList rewrittenYaml;
    for (const QString &tag : yamlTags)
        rewrittenYaml.append(tagIsOrChildOf(tag, from) ? retargetTag(tag, from, to) : tag);
    rewrittenYaml = uniqueTags(rewrittenYaml);

    QString updated = document.hasFrontMatter || !yamlTags.isEmpty()
        ? setListField(text, QStringLiteral("tags"), rewrittenYaml)
        : text;

    const Document afterYaml = parse(updated);
    QString body = afterYaml.body;
    const QString escaped = QRegularExpression::escape(from);
    QRegularExpression bodyRe(
        QStringLiteral(R"((?<![\w#])#)") + escaped + QStringLiteral(R"((?=/|[^\p{L}\p{N}_/\-]|$))"),
        QRegularExpression::UseUnicodePropertiesOption);
    body.replace(bodyRe, QLatin1Char('#') + to);

    const int bodyOffset = updated.size() - afterYaml.body.size();
    if (bodyOffset < 0 || bodyOffset > updated.size())
        return updated;
    return updated.left(bodyOffset) + body;
}

QVariantList headingOutline(const QString &text) {
    const Document document = parse(text);
    const int offset = qMax(0, text.size() - document.body.size());
    static const QRegularExpression heading(
        QStringLiteral(R"(^(#{1,6})[ \t]+(.+?)[ \t]*#*[ \t]*$)"));

    QVariantList outline;
    bool inFence = false;
    int cursor = offset;
    while (cursor <= text.size()) {
        int eol = text.indexOf(QLatin1Char('\n'), cursor);
        if (eol < 0)
            eol = text.size();
        QString line = text.mid(cursor, eol - cursor);
        if (line.endsWith(QLatin1Char('\r')))
            line.chop(1);
        if (line.trimmed().startsWith(QStringLiteral("```")))
            inFence = !inFence;
        else if (!inFence) {
            const QRegularExpressionMatch match = heading.match(line);
            if (match.hasMatch()) {
                outline.append(QVariantMap{
                    {QStringLiteral("level"), match.captured(1).size()},
                    {QStringLiteral("title"), match.captured(2).trimmed()},
                    {QStringLiteral("position"), cursor},
                });
            }
        }
        if (eol == text.size())
            break;
        cursor = eol + 1;
    }
    return outline;
}

QString normalizedHeading(const QString &title) {
    QString text = title.trimmed().toLower();
    text.replace(QRegularExpression(QStringLiteral("[#*_`]+")), QString());
    text.replace(QRegularExpression(QStringLiteral("\\s+")), QStringLiteral(" "));
    return text.trimmed();
}

QVariantList blockAnchors(const QString &text) {
    static const QRegularExpression block(
        QStringLiteral(R"((?:^|[ \t])\^([A-Za-z0-9-]{1,64})[ \t]*$)"),
        QRegularExpression::MultilineOption);
    QVariantList anchors;
    QRegularExpressionMatchIterator it = block.globalMatch(text);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        const int lineStart = text.lastIndexOf(QLatin1Char('\n'), match.capturedStart());
        anchors.append(QVariantMap{
            {QStringLiteral("id"), match.captured(1)},
            {QStringLiteral("position"), lineStart < 0 ? 0 : lineStart + 1},
        });
    }
    return anchors;
}

int fragmentPosition(const QString &text, const QString &kind, const QString &fragment) {
    const QString needle = fragment.trimmed();
    if (needle.isEmpty())
        return -1;
    if (kind == QLatin1String("block")) {
        const QVariantList anchors = blockAnchors(text);
        for (const QVariant &item : anchors) {
            if (item.toMap().value(QStringLiteral("id")).toString().compare(
                    needle, Qt::CaseInsensitive)
                == 0)
                return item.toMap().value(QStringLiteral("position")).toInt();
        }
        return -1;
    }
    const QString want = normalizedHeading(needle);
    if (want.isEmpty())
        return -1;
    const QVariantList outline = headingOutline(text);
    int prefix = -1;
    int prefixCount = 0;
    for (const QVariant &item : outline) {
        const QVariantMap heading = item.toMap();
        const QString title = heading.value(QStringLiteral("title")).toString();
        const int position = heading.value(QStringLiteral("position")).toInt();
        const QString have = normalizedHeading(title);
        if (have == want)
            return position;
        if (have.startsWith(want) || want.startsWith(have)) {
            prefix = position;
            ++prefixCount;
        }
    }
    return prefixCount == 1 ? prefix : -1;
}

int openTaskCount(const QString &body) {
    return countMatches(openTaskRe(), body);
}

int closedTaskCount(const QString &body) {
    return countMatches(closedTaskRe(), body);
}

} // namespace FrontMatter
