#include "codeblocks.h"

#include "frontmatter.h"
#include "mindmap.h"

#include <QRegularExpression>
#include <QVector>
#include <algorithm>

namespace CodeBlocks {
namespace {

struct Language {
    const char *id;
    const char *title;
};

const Language kLanguages[] = {
    {"python", "Python"},
    {"mermaid", "Mermaid"},
    {"javascript", "JavaScript"},
    {"typescript", "TypeScript"},
    {"json", "JSON"},
    {"html", "HTML"},
    {"css", "CSS"},
    {"bash", "Bash"},
    {"yaml", "YAML"},
    {"rust", "Rust"},
    {"go", "Go"},
    {"cpp", "C++"},
    {"c", "C"},
    {"java", "Java"},
    {"swift", "Swift"},
    {"kotlin", "Kotlin"},
    {"sql", "SQL"},
    {"markdown", "Markdown"},
    {"toml", "TOML"},
    {"xml", "XML"},
    {"ruby", "Ruby"},
    {"php", "PHP"},
    {"r", "R"},
    {"lua", "Lua"},
    {"dart", "Dart"},
    {"scala", "Scala"},
    {"haskell", "Haskell"},
    {"elixir", "Elixir"},
    {"graphql", "GraphQL"},
    {"dockerfile", "Dockerfile"},
    {"makefile", "Makefile"},
    {"diff", "Diff"},
    {"ini", "INI"},
    {"zsh", "Zsh"},
    {"plaintext", "Plaintext"},
};

struct SlashItem {
    const char *id;
    const char *title;
    const char *hint;
};

const SlashItem kSlashItems[] = {
    {"code", "Code", "Insert a searchable code block"},
    {"mermaid", "Mermaid", "Insert a Mermaid diagram"},
    {"heading1", "Heading 1", "Turn this line into #"},
    {"heading2", "Heading 2", "Turn this line into ##"},
    {"heading3", "Heading 3", "Turn this line into ###"},
    {"quote", "Quote", "Insert a quote block"},
    {"bullet", "Bullet list", "Insert a bullet list"},
    {"todo", "To-do", "Insert a checkbox"},
    {"table", "Table", "Insert a Markdown table"},
    {"tablecol", "Table column", "Add a column to the right"},
    {"tablerow", "Table row", "Add a row below"},
    {"toc", "Table of contents", "Insert a live ```toc block"},
    {"highlight", "Highlight", "Mark text with ==highlight=="},
    {"color", "Color", "Text + background color span"},
    {"divider", "Divider", "Insert a horizontal rule"},
    {"template", "Template", "Insert a note template"},
};

int lineStartAt(const QString &text, int cursor) {
    if (cursor <= 0)
        return 0;
    const int at = text.lastIndexOf(QLatin1Char('\n'), cursor - 1);
    return at < 0 ? 0 : at + 1;
}

QVariantMap inactiveMap() {
    return QVariantMap{{QStringLiteral("active"), false}};
}

bool containsInsensitive(const QString &haystack, const QString &needle) {
    return haystack.contains(needle, Qt::CaseInsensitive);
}

} // namespace

bool isFenceLine(const QString &line, QString *marker, QString *info) {
    int i = 0;
    while (i < line.size() && i < 3 && line.at(i) == QLatin1Char(' '))
        ++i;
    if (i >= line.size())
        return false;

    const QChar ch = line.at(i);
    if (ch != QLatin1Char('`') && ch != QLatin1Char('~'))
        return false;

    const int runStart = i;
    while (i < line.size() && line.at(i) == ch)
        ++i;
    if (i - runStart < 3)
        return false;

    const QString rest = line.mid(i).trimmed();
    if (ch == QLatin1Char('`') && rest.contains(QLatin1Char('`')))
        return false;

    if (marker)
        *marker = QString(i - runStart, ch);
    if (info)
        *info = rest;
    return true;
}

QString languageFromInfo(const QString &info) {
    if (info.isEmpty())
        return {};

    QString token = info.trimmed();
    const int space = token.indexOf(QRegularExpression(QStringLiteral("\\s")));
    if (space >= 0)
        token = token.left(space);
    if (token.startsWith(QLatin1Char('{')) && token.endsWith(QLatin1Char('}')))
        token = token.mid(1, token.size() - 2).trimmed();
    while (token.startsWith(QLatin1Char('.')))
        token.remove(0, 1);
    static const QRegularExpression junk(QStringLiteral("[^A-Za-z0-9_+#-].*$"));
    token.remove(junk);
    return token.toLower();
}

QList<Fence> parse(const QString &text) {
    QString normalized = text;
    normalized.replace(QLatin1Char('\r'), QString());
    QList<Fence> fences;
    Fence current;
    bool open = false;
    int pos = 0;
    int lineIndex = 0;
    const QStringList lines = normalized.split(QLatin1Char('\n'));
    for (int i = 0; i < lines.size(); ++i, ++lineIndex) {
        const QString &line = lines.at(i);
        const int linePos = pos;
        QString marker;
        QString info;
        const bool fence = isFenceLine(line, &marker, &info);
        if (open) {
            if (fence && marker.front() == current.marker.front()
                    && marker.size() >= current.marker.size() && info.trimmed().isEmpty()) {
                current.closeLine = lineIndex;
                current.innerEnd = linePos;
                current.closePos = linePos + line.size();
                if (i + 1 < lines.size())
                    current.closePos += 1;
                current.code = normalized.mid(current.innerStart,
                                        qMax(0, current.innerEnd - current.innerStart));
                if (current.code.endsWith(QLatin1Char('\n')))
                    current.code.chop(1);
                fences.append(current);
                current = Fence();
                open = false;
            }
        } else if (fence) {
            current.openLine = lineIndex;
            current.openPos = linePos;
            current.marker = marker;
            current.language = languageFromInfo(info);
            current.innerStart = linePos + line.size();
            if (i + 1 < lines.size())
                current.innerStart += 1;
            current.innerEnd = normalized.size();
            current.closePos = -1;
            open = true;
        }
        pos += line.size();
        if (i + 1 < lines.size())
            pos += 1;
    }
    if (open) {
        current.innerEnd = normalized.size();
        current.code = normalized.mid(current.innerStart);
        fences.append(current);
    }
    return fences;
}

bool oddFencesBefore(const QString &text, int position) {
    position = qBound(0, position, text.size());
    int count = 0;
    int i = 0;
    while (i < position) {
        int newline = text.indexOf(QLatin1Char('\n'), i);
        if (newline < 0 || newline >= position)
            break;
        if (isFenceLine(text.mid(i, newline - i)))
            ++count;
        i = newline + 1;
    }
    return (count % 2) == 1;
}

QString preserveLineBreaks(const QString &text) {
    QString body = text;
    body.replace(QLatin1Char('\r'), QString());
    const QStringList lines = body.split(QLatin1Char('\n'));
    QStringList rendered;
    rendered.reserve(lines.size());
    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines.at(i);
        if (i + 1 < lines.size()) {
            const QString next = lines.at(i + 1);
            if (!line.trimmed().isEmpty() && !next.trimmed().isEmpty()
                    && !line.endsWith(QStringLiteral("  ")))
                line.append(QStringLiteral("  "));
        }
        rendered.append(line);
    }
    return rendered.join(QLatin1Char('\n'));
}

QVariantList previewBlocks(const QString &source) {
    const FrontMatter::Document document = FrontMatter::parse(source);
    QString body = document.body;
    body.replace(QLatin1Char('\r'), QString());
    int bodyOffset = source.size() - document.body.size();
    if (bodyOffset < 0)
        bodyOffset = 0;
    const QList<Fence> fences = parse(body);
    QVariantList blocks;
    int cursor = 0;
    int index = 0;
    auto appendMarkdown = [&](const QString &chunk, int from, int to) {
        if (chunk.trimmed().isEmpty())
            return;
        blocks.append(QVariantMap{
            {QStringLiteral("kind"), QStringLiteral("markdown")},
            {QStringLiteral("text"), preserveLineBreaks(chunk)},
            {QStringLiteral("from"), bodyOffset + from},
            {QStringLiteral("to"), bodyOffset + to},
        });
    };

    for (const Fence &fence : fences) {
        if (fence.openPos > cursor)
            appendMarkdown(body.mid(cursor, fence.openPos - cursor),
                           cursor, fence.openPos);
        const QString language = fence.language.isEmpty()
            ? QStringLiteral("plaintext")
            : fence.language;
        const int fenceEnd = fence.closePos >= 0 ? fence.closePos : body.size();
        if (language.compare(QLatin1String("toc"), Qt::CaseInsensitive) == 0) {
            appendMarkdown(renderToc(source, fence.code),
                           fence.openPos, fenceEnd);
            cursor = fenceEnd;
            continue;
        }
        const bool mermaid = language == QLatin1String("mermaid");
        blocks.append(QVariantMap{
            {QStringLiteral("kind"), mermaid ? QStringLiteral("mermaid")
                                             : QStringLiteral("code")},
            {QStringLiteral("language"), language},
            {QStringLiteral("text"), fence.code},
            {QStringLiteral("index"), index++},
            {QStringLiteral("mermaid"), mermaid},
            {QStringLiteral("from"), bodyOffset + fence.openPos},
            {QStringLiteral("to"), bodyOffset + fenceEnd},
        });
        cursor = fenceEnd;
    }
    if (cursor < body.size())
        appendMarkdown(body.mid(cursor), cursor, body.size());
    if (blocks.isEmpty() && !body.trimmed().isEmpty())
        appendMarkdown(body, 0, body.size());
    return blocks;
}

QVariantList languages(const QString &query, const QString &lastUsed) {
    const QString needle = query.trimmed();
    QVariantList out;
    QStringList seen;
    auto append = [&](const Language &language) {
        const QString id = QString::fromLatin1(language.id);
        if (seen.contains(id))
            return;
        if (!needle.isEmpty()
                && !containsInsensitive(id, needle)
                && !containsInsensitive(QString::fromLatin1(language.title), needle))
            return;
        seen.append(id);
        out.append(QVariantMap{
            {QStringLiteral("id"), id},
            {QStringLiteral("title"), QString::fromLatin1(language.title)},
        });
    };

    if (!lastUsed.trimmed().isEmpty()) {
        for (const Language &language : kLanguages) {
            if (lastUsed.compare(QLatin1String(language.id), Qt::CaseInsensitive) == 0) {
                if (needle.isEmpty()
                        || containsInsensitive(QLatin1String(language.id), needle)
                        || containsInsensitive(QLatin1String(language.title), needle)) {
                    append(language);
                }
                break;
            }
        }
        if (out.isEmpty() && needle.isEmpty()) {
            out.append(QVariantMap{
                {QStringLiteral("id"), lastUsed.trimmed().toLower()},
                {QStringLiteral("title"), lastUsed.trimmed()},
            });
            seen.append(lastUsed.trimmed().toLower());
        }
    }

    for (const Language &language : kLanguages)
        append(language);
    return out;
}

QString renderToc(const QString &source, const QString &options) {
    int minDepth = 2;
    int maxDepth = 6;
    QString style = QStringLiteral("bullet");
    QString title;
    QString delimiter = QStringLiteral(" | ");
    bool varied = false;
    const QStringList lines = options.split(QLatin1Char('\n'));
    for (QString line : lines) {
        line = line.trimmed();
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
            continue;
        const int colon = line.indexOf(QLatin1Char(':'));
        if (colon <= 0)
            continue;
        const QString key = line.left(colon).trimmed().toLower();
        QString value = line.mid(colon + 1).trimmed();
        if (value.size() >= 2
            && ((value.front() == QLatin1Char('"') && value.back() == QLatin1Char('"'))
                || (value.front() == QLatin1Char('\'') && value.back() == QLatin1Char('\''))))
            value = value.mid(1, value.size() - 2);
        if (key == QLatin1String("min_depth"))
            minDepth = qBound(1, value.toInt(), 6);
        else if (key == QLatin1String("max_depth"))
            maxDepth = qBound(1, value.toInt(), 6);
        else if (key == QLatin1String("style"))
            style = value.toLower();
        else if (key == QLatin1String("title"))
            title = value;
        else if (key == QLatin1String("delimiter"))
            delimiter = value;
        else if (key == QLatin1String("varied_style"))
            varied = value.compare(QLatin1String("true"), Qt::CaseInsensitive) == 0
                || value == QLatin1String("1");
    }
    if (maxDepth < minDepth)
        std::swap(maxDepth, minDepth);

    const QVariantList outline = FrontMatter::headingOutline(source);
    QStringList out;
    if (!title.isEmpty()) {
        out.append(title);
        out.append(QString());
    }
    int number = 1;
    QStringList inlineItems;
    int topLevel = -1;
    for (const QVariant &item : outline) {
        const QVariantMap heading = item.toMap();
        const int level = heading.value(QStringLiteral("level")).toInt();
        if (level < minDepth || level > maxDepth)
            continue;
        if (topLevel < 0)
            topLevel = level;
        const QString headingTitle = heading.value(QStringLiteral("title")).toString();
        const int indent = qMax(0, level - minDepth);
        const bool numbered = style == QLatin1String("number")
            || (varied && style == QLatin1String("bullet") && level > topLevel)
            || (varied && style == QLatin1String("number") && level == topLevel);
        if (style == QLatin1String("inline")) {
            inlineItems.append(headingTitle);
            continue;
        }
        const QString pad = QString(indent * 2, QLatin1Char(' '));
        if (numbered)
            out.append(pad + QString::number(number++) + QStringLiteral(". ") + headingTitle);
        else
            out.append(pad + QStringLiteral("- ") + headingTitle);
    }
    if (style == QLatin1String("inline"))
        return (title.isEmpty() ? QString() : title + QLatin1Char('\n'))
            + inlineItems.join(delimiter);
    return out.join(QLatin1Char('\n'));
}

QVariantList slashCommands(const QString &query) {
    const QString needle = query.trimmed();
    QVariantList out;
    for (const SlashItem &item : kSlashItems) {
        if (!needle.isEmpty()
                && !containsInsensitive(QLatin1String(item.id), needle)
                && !containsInsensitive(QLatin1String(item.title), needle))
            continue;
        out.append(QVariantMap{
            {QStringLiteral("id"), QString::fromLatin1(item.id)},
            {QStringLiteral("title"), QString::fromLatin1(item.title)},
            {QStringLiteral("hint"), QString::fromLatin1(item.hint)},
        });
    }
    return out;
}

QVariantMap slashQueryAt(const QString &text, int cursor) {
    cursor = qBound(0, cursor, text.size());
    if (oddFencesBefore(text, cursor))
        return inactiveMap();

    const int start = lineStartAt(text, cursor);
    const QString line = text.mid(start, cursor - start);
    static const QRegularExpression slashRe(QStringLiteral("^(\\s*)/([^\\s]*)$"));
    const QRegularExpressionMatch match = slashRe.match(line);
    if (!match.hasMatch())
        return inactiveMap();

    return QVariantMap{
        {QStringLiteral("active"), true},
        {QStringLiteral("replaceStart"), start + match.capturedLength(1)},
        {QStringLiteral("replaceEnd"), cursor},
        {QStringLiteral("query"), match.captured(2)},
    };
}

QVariantMap backtickTriggerAt(const QString &text, int cursor) {
    cursor = qBound(0, cursor, text.size());
    if (oddFencesBefore(text, cursor))
        return inactiveMap();

    const int start = lineStartAt(text, cursor);
    const QString line = text.mid(start, cursor - start);
    QString marker;
    QString info;
    if (!isFenceLine(line, &marker, &info))
        return inactiveMap();
    if (!info.trimmed().isEmpty())
        return inactiveMap();
    if (cursor != start + line.size())
        return inactiveMap();

    return QVariantMap{
        {QStringLiteral("active"), true},
        {QStringLiteral("replaceStart"), start},
        {QStringLiteral("replaceEnd"), cursor},
        {QStringLiteral("marker"), marker},
    };
}

QVariantMap leaveFenceAt(const QString &text, int cursor) {
    cursor = qBound(0, cursor, text.size());
    const QList<Fence> fences = parse(text);
    for (const Fence &fence : fences) {
        const int end = fence.closePos >= 0 ? fence.closePos : text.size();
        if (cursor < fence.openPos || cursor > end)
            continue;
        const bool above = cursor <= fence.innerStart;
        return QVariantMap{
            {QStringLiteral("active"), true},
            {QStringLiteral("above"), above},
            {QStringLiteral("insertPos"), above ? fence.openPos : end},
            {QStringLiteral("needsClose"), fence.closePos < 0 && !above},
        };
    }
    return inactiveMap();
}

QString fenceText(const QString &language, const QString &inner) {
    QString lang = language.trimmed();
    if (lang.isEmpty())
        lang = QStringLiteral("plaintext");
    QString body = inner;
    body.replace(QLatin1Char('\r'), QString());
    if (body.endsWith(QLatin1Char('\n')))
        body.chop(1);
    return QStringLiteral("```") + lang + QLatin1Char('\n') + body + QStringLiteral("\n```");
}

int fenceCaretOffset(const QString &language, const QString &inner) {
    QString lang = language.trimmed();
    if (lang.isEmpty())
        lang = QStringLiteral("plaintext");
    Q_UNUSED(inner);
    return 3 + lang.size() + 1;
}

QStringList tableCells(const QString &line) {
    QString t = line;
    if (t.endsWith(QLatin1Char('\r')))
        t.chop(1);
    t = t.trimmed();
    if (t.startsWith(QLatin1Char('|')))
        t.remove(0, 1);
    if (t.endsWith(QLatin1Char('|')))
        t.chop(1);
    return t.split(QLatin1Char('|'));
}

bool isTableSeparator(const QString &line) {
    const QStringList cells = tableCells(line);
    if (cells.isEmpty())
        return false;
    for (const QString &cell : cells) {
        const QString s = cell.trimmed();
        if (s.isEmpty() || !s.contains(QLatin1Char('-')))
            return false;
        for (QChar ch : s) {
            if (ch != QLatin1Char('-') && ch != QLatin1Char(':') && !ch.isSpace())
                return false;
        }
    }
    return true;
}

bool isTableLine(const QString &line) {
    const QString t = line.trimmed();
    return t.startsWith(QLatin1Char('|')) && t.count(QLatin1Char('|')) >= 2;
}

QString formatTableRow(const QStringList &cells, bool separator) {
    QString out = QStringLiteral("|");
    for (const QString &cell : cells) {
        if (separator) {
            QString dashes = cell.trimmed();
            if (dashes.isEmpty())
                dashes = QStringLiteral("---");
            out += QLatin1Char(' ') + dashes + QStringLiteral(" |");
        } else {
            out += QLatin1Char(' ') + cell + QStringLiteral(" |");
        }
    }
    return out;
}

QStringList prettyTableLines(QStringList lines) {
    if (lines.size() < 2)
        return lines;

    int sepIndex = -1;
    QVector<QStringList> rows;
    rows.reserve(lines.size());
    for (int i = 0; i < lines.size(); ++i) {
        rows.append(tableCells(lines.at(i)));
        if (sepIndex < 0 && isTableSeparator(lines.at(i)))
            sepIndex = i;
    }
    if (sepIndex < 0) {
        QStringList separator;
        int cols = 1;
        for (const QStringList &row : rows)
            cols = qMax(cols, row.size());
        for (int i = 0; i < cols; ++i)
            separator.append(QStringLiteral("---"));
        sepIndex = qMin(1, rows.size());
        rows.insert(sepIndex, separator);
    } else if (sepIndex != 1 && rows.size() > 1) {
        const QStringList separator = rows.takeAt(sepIndex);
        rows.insert(1, separator);
        sepIndex = 1;
    }

    int cols = 1;
    for (const QStringList &row : rows)
        cols = qMax(cols, row.size());
    QVector<int> widths(cols, 3);
    for (int r = 0; r < rows.size(); ++r) {
        while (rows[r].size() < cols)
            rows[r].append(QString());
        if (r == sepIndex)
            continue;
        for (int c = 0; c < cols; ++c)
            widths[c] = qMax(widths[c], rows[r].at(c).trimmed().size());
    }

    QStringList pretty;
    pretty.reserve(rows.size());
    for (int r = 0; r < rows.size(); ++r) {
        QStringList padded;
        padded.reserve(cols);
        const bool sep = r == sepIndex;
        for (int c = 0; c < cols; ++c) {
            QString cell = rows[r].at(c).trimmed();
            if (sep) {
                if (cell.isEmpty())
                    cell = QStringLiteral("---");
                while (cell.size() < widths.at(c))
                    cell.append(QLatin1Char('-'));
            } else {
                cell = cell.leftJustified(widths.at(c), QLatin1Char(' '));
            }
            padded.append(cell);
        }
        pretty.append(formatTableRow(padded, sep));
    }
    return pretty;
}

int columnAtOffset(const QString &line, int offsetInLine) {
    int pipes = 0;
    const int limit = qBound(0, offsetInLine, line.size());
    for (int i = 0; i < limit; ++i) {
        if (line.at(i) == QLatin1Char('|'))
            ++pipes;
    }
    return qMax(0, pipes - 1);
}

struct TableSpan {
    int start = -1;
    int end = -1;
    int cursorLine = -1;
    int col = 0;
    QStringList lines;
};

TableSpan locateTable(const QString &text, int cursor) {
    TableSpan span;
    cursor = qBound(0, cursor, text.size());
    QStringList lines;
    QList<int> starts;
    int pos = 0;
    while (pos <= text.size()) {
        starts.append(pos);
        const int nl = text.indexOf(QLatin1Char('\n'), pos);
        if (nl < 0) {
            lines.append(text.mid(pos));
            break;
        }
        lines.append(text.mid(pos, nl - pos));
        pos = nl + 1;
    }
    if (lines.isEmpty())
        return span;

    int lineIndex = 0;
    for (int i = 0; i < starts.size(); ++i) {
        const int lineEnd = i + 1 < starts.size() ? starts.at(i + 1) : text.size();
        if (cursor >= starts.at(i) && cursor <= lineEnd)
            lineIndex = i;
    }
    if (!isTableLine(lines.at(lineIndex)))
        return span;

    int first = lineIndex;
    int last = lineIndex;
    while (first > 0 && isTableLine(lines.at(first - 1)))
        --first;
    while (last + 1 < lines.size() && isTableLine(lines.at(last + 1)))
        ++last;
    if (last - first < 1)
        return span;
    bool hasSep = false;
    for (int i = first; i <= last; ++i) {
        if (isTableSeparator(lines.at(i))) {
            hasSep = true;
            break;
        }
    }
    if (!hasSep)
        return span;

    span.start = starts.at(first);
    span.end = last + 1 < starts.size() ? starts.at(last + 1) - 1 : text.size();
    if (span.end < span.start)
        span.end = text.size();
    span.cursorLine = lineIndex - first;
    span.col = columnAtOffset(lines.at(lineIndex), cursor - starts.at(lineIndex));
    for (int i = first; i <= last; ++i)
        span.lines.append(lines.at(i));
    return span;
}

int tableColumnCount(const QStringList &lines) {
    int cols = 0;
    for (const QString &line : lines)
        cols = qMax(cols, tableCells(line).size());
    return cols;
}

QVariantMap tableAt(const QString &text, int cursor) {
    const TableSpan span = locateTable(text, cursor);
    if (span.start < 0)
        return inactiveMap();
    return QVariantMap{
        {QStringLiteral("active"), true},
        {QStringLiteral("start"), span.start},
        {QStringLiteral("end"), span.end},
        {QStringLiteral("row"), span.cursorLine},
        {QStringLiteral("col"), span.col},
        {QStringLiteral("rows"), span.lines.size()},
        {QStringLiteral("cols"), tableColumnCount(span.lines)},
    };
}

QVariantMap rewriteTable(const TableSpan &span, const QStringList &newLines, int caretOffsetInTable) {
    QVariantMap result = inactiveMap();
    if (span.start < 0 || newLines.isEmpty())
        return result;
    result.insert(QStringLiteral("active"), true);
    result.insert(QStringLiteral("start"), span.start);
    result.insert(QStringLiteral("end"), span.end);
    result.insert(QStringLiteral("text"), newLines.join(QLatin1Char('\n')));
    result.insert(QStringLiteral("caret"), span.start + qBound(0, caretOffsetInTable, newLines.join(QLatin1Char('\n')).size()));
    return result;
}

QVariantMap insertTableColumn(const QString &text, int cursor) {
    const TableSpan span = locateTable(text, cursor);
    if (span.start < 0)
        return inactiveMap();
    const int cols = qMax(1, tableColumnCount(span.lines));
    const int insertAt = qBound(0, span.col + 1, cols);
    QStringList next;
    for (const QString &line : span.lines) {
        QStringList cells = tableCells(line);
        while (cells.size() < cols)
            cells.append(QString());
        const bool sep = isTableSeparator(line);
        cells.insert(insertAt, sep ? QStringLiteral("---") : QString());
        next.append(formatTableRow(cells, sep));
    }
    next = prettyTableLines(next);
    const QString joined = next.join(QLatin1Char('\n'));
    QVariantMap result = rewriteTable(span, next, qMin(joined.size(), 0));
    result.insert(QStringLiteral("caret"), span.start + next.first().size());
    return result;
}

QVariantMap insertTableRow(const QString &text, int cursor) {
    const TableSpan span = locateTable(text, cursor);
    if (span.start < 0)
        return inactiveMap();
    const int cols = qMax(1, tableColumnCount(span.lines));
    int sepIndex = -1;
    for (int i = 0; i < span.lines.size(); ++i) {
        if (isTableSeparator(span.lines.at(i))) {
            sepIndex = i;
            break;
        }
    }
    int insertAfter = span.cursorLine;
    if (sepIndex >= 0 && insertAfter < sepIndex)
        insertAfter = sepIndex;
    QStringList empty;
    for (int i = 0; i < cols; ++i)
        empty.append(QString());
    QStringList next = span.lines;
    next.insert(insertAfter + 1, formatTableRow(empty, false));
    next = prettyTableLines(next);
    int caret = 0;
    for (int i = 0; i <= insertAfter; ++i)
        caret += next.at(i).size() + 1;
    caret += 2;
    return rewriteTable(span, next, caret);
}

QVariantMap formatTable(const QString &text, int cursor) {
    const TableSpan span = locateTable(text, cursor);
    if (span.start < 0)
        return inactiveMap();
    return rewriteTable(span, prettyTableLines(span.lines), 2);
}

QVariantList bodyBlocks(const QString &markdown) {
    QVariantList blocks;
    const QStringList lines = QString(markdown).replace(QLatin1Char('\r'), QString()).split(QLatin1Char('\n'));
    QStringList textLines;
    auto flushText = [&]() {
        if (textLines.isEmpty())
            return;
        while (!textLines.isEmpty() && textLines.last().trimmed().isEmpty())
            textLines.removeLast();
        while (!textLines.isEmpty() && textLines.first().trimmed().isEmpty())
            textLines.removeFirst();
        if (textLines.isEmpty())
            return;
        blocks.append(QVariantMap{
            {QStringLiteral("kind"), QStringLiteral("text")},
            {QStringLiteral("text"), textLines.join(QLatin1Char('\n'))},
        });
        textLines.clear();
    };

    bool inFence = false;
    for (int i = 0; i < lines.size();) {
        const QString &line = lines.at(i);
        if (line.trimmed().startsWith(QStringLiteral("```"))) {
            inFence = !inFence;
            textLines.append(line);
            ++i;
            continue;
        }
        if (!inFence && isTableLine(line)) {
            int last = i;
            while (last + 1 < lines.size() && isTableLine(lines.at(last + 1)))
                ++last;
            bool hasSep = false;
            for (int n = i; n <= last; ++n) {
                if (isTableSeparator(lines.at(n))) {
                    hasSep = true;
                    break;
                }
            }
            if (hasSep && last > i) {
                flushText();
                QStringList headers;
                QVariantList rows;
                bool seenSep = false;
                int columns = 1;
                for (int n = i; n <= last; ++n) {
                    QStringList cells = tableCells(lines.at(n));
                    for (QString &cell : cells)
                        cell = cell.trimmed();
                    columns = qMax(columns, cells.size());
                    if (isTableSeparator(lines.at(n))) {
                        seenSep = true;
                        continue;
                    }
                    if (!seenSep)
                        headers = cells;
                    else {
                        QVariantList row;
                        for (const QString &cell : cells)
                            row.append(cell);
                        rows.append(row);
                    }
                }
                while (headers.size() < columns)
                    headers.append(QString());
                QVariantList cells;
                for (const QString &header : headers) {
                    cells.append(QVariantMap{
                        {QStringLiteral("text"), header},
                        {QStringLiteral("header"), true},
                    });
                }
                for (const QVariant &rowValue : rows) {
                    QVariantList row = rowValue.toList();
                    while (row.size() < columns)
                        row.append(QString());
                    for (const QVariant &cell : row) {
                        cells.append(QVariantMap{
                            {QStringLiteral("text"), cell.toString()},
                            {QStringLiteral("header"), false},
                        });
                    }
                }
                blocks.append(QVariantMap{
                    {QStringLiteral("kind"), QStringLiteral("table")},
                    {QStringLiteral("columns"), columns},
                    {QStringLiteral("cells"), cells},
                });
                i = last + 1;
                continue;
            }
        }
        textLines.append(line);
        ++i;
    }
    flushText();
    if (blocks.isEmpty() && !markdown.trimmed().isEmpty()) {
        blocks.append(QVariantMap{
            {QStringLiteral("kind"), QStringLiteral("text")},
            {QStringLiteral("text"), markdown.trimmed()},
        });
    }
    return blocks;
}

int caretInTableCell(const TableSpan &span, int row, int col) {
    int offset = 0;
    for (int i = 0; i < row && i < span.lines.size(); ++i)
        offset += span.lines.at(i).size() + 1;
    if (row < 0 || row >= span.lines.size())
        return span.start + offset;
    const QString &line = span.lines.at(row);
    int pipes = 0;
    for (int i = 0; i < line.size(); ++i) {
        if (line.at(i) != QLatin1Char('|'))
            continue;
        if (pipes == col) {
            int pos = i + 1;
            if (pos < line.size() && line.at(pos) == QLatin1Char(' '))
                ++pos;
            return span.start + offset + pos;
        }
        ++pipes;
    }
    return span.start + offset + line.size();
}

bool skippableTableRow(const QString &line) {
    return isTableSeparator(line);
}

QVariantMap tableMoveCell(const QString &text, int cursor, int delta) {
    if (delta == 0)
        return tableAt(text, cursor);
    TableSpan span = locateTable(text, cursor);
    if (span.start < 0)
        return inactiveMap();
    const int cols = qMax(1, tableColumnCount(span.lines));
    int row = span.cursorLine;
    int col = span.col + delta;
    auto wrap = [&]() {
        while (col >= cols) {
            col -= cols;
            ++row;
        }
        while (col < 0) {
            col += cols;
            --row;
        }
    };
    wrap();
    const int step = delta > 0 ? 1 : -1;
    while (row >= 0 && row < span.lines.size() && skippableTableRow(span.lines.at(row)))
        row += step;

    if (row >= span.lines.size()) {
        QVariantMap inserted = insertTableRow(text, cursor);
        inserted.insert(QStringLiteral("replaced"), true);
        return inserted;
    }
    if (row < 0)
        row = 0;
    while (row < span.lines.size() && skippableTableRow(span.lines.at(row)))
        ++row;
    if (row >= span.lines.size())
        row = span.lines.size() - 1;

    return QVariantMap{
        {QStringLiteral("active"), true},
        {QStringLiteral("replaced"), false},
        {QStringLiteral("caret"), caretInTableCell(span, row, col)},
    };
}

QString slidevYamlScalar(const QString &value) {
    if (value.isEmpty())
        return QStringLiteral("\"\"");
    static const QRegularExpression unsafe(QStringLiteral(R"([:#{}[\],&*?|<>=!%@`])"));
    if (unsafe.match(value).hasMatch() || value.contains(QLatin1Char('\n'))
            || value.startsWith(QLatin1Char(' ')) || value.endsWith(QLatin1Char(' '))) {
        QString escaped = value;
        escaped.replace(QLatin1Char('\\'), QStringLiteral("\\\\"));
        escaped.replace(QLatin1Char('"'), QStringLiteral("\\\""));
        return QLatin1Char('"') + escaped + QLatin1Char('"');
    }
    return value;
}

QString slidevMarkdown(const QString &markdown) {
    const QString rewritten = MindMap::rewriteGardenWikilinks(markdown, {});
    const FrontMatter::Document document = FrontMatter::parse(rewritten);
    QString title = FrontMatter::displayValue(document.fields.value(QStringLiteral("title"))).trimmed();
    QString theme = FrontMatter::displayValue(document.fields.value(QStringLiteral("theme"))).trimmed();
    if (theme.isEmpty() || theme == QLatin1String("night") || theme == QLatin1String("newsprint")
            || theme == QLatin1String("gothic"))
        theme = QStringLiteral("default");

    QString body = document.body;
    body.replace(QLatin1Char('\r'), QString());
    const QStringList lines = body.split(QLatin1Char('\n'));
    QStringList slides;
    QStringList layouts;
    QString current;
    QString currentLayout;
    bool inFence = false;
    static const QRegularExpression slideHeading(QStringLiteral(R"(^#{1,2}[ \t]+.+)"));
    static const QRegularExpression layoutAttr(
        QStringLiteral(R"(^(#{1,2}[ \t]+.+?)[ \t]*\{layout:\s*([^}]+)\}\s*$)"));
    auto startSlide = [&](QString heading) {
        if (!current.trimmed().isEmpty()) {
            slides.append(current.trimmed());
            layouts.append(currentLayout);
        }
        currentLayout.clear();
        const QRegularExpressionMatch attr = layoutAttr.match(heading);
        if (attr.hasMatch()) {
            currentLayout = attr.captured(2).trimmed();
            heading = attr.captured(1).trimmed();
        }
        current = heading + QLatin1Char('\n');
    };
    for (const QString &line : lines) {
        if (line.trimmed().startsWith(QStringLiteral("```")))
            inFence = !inFence;
        if (!inFence && line.trimmed() == QLatin1String("---"))
            continue;
        if (!inFence && slideHeading.match(line).hasMatch() && !current.trimmed().isEmpty()) {
            startSlide(line);
            continue;
        }
        if (!inFence && slideHeading.match(line).hasMatch() && current.trimmed().isEmpty()) {
            startSlide(line);
            continue;
        }
        current += line + QLatin1Char('\n');
    }
    if (!current.trimmed().isEmpty()) {
        slides.append(current.trimmed());
        layouts.append(currentLayout);
    }
    if (slides.isEmpty()) {
        if (title.isEmpty())
            title = QStringLiteral("Untitled");
        slides.append(QStringLiteral("# ") + title);
        layouts.append(QString());
    }

    QString out = QStringLiteral("---\n");
    // "default" is built in. Writing theme: default makes Slidev look for
    // @slidev/theme-default, which is not installed and cannot prompt.
    if (!theme.isEmpty() && theme != QLatin1String("default"))
        out += QStringLiteral("theme: ") + theme + QLatin1Char('\n');
    const QString colorSchema =
        FrontMatter::displayValue(document.fields.value(QStringLiteral("colorSchema"))).trimmed();
    if (!colorSchema.isEmpty())
        out += QStringLiteral("colorSchema: ") + colorSchema + QLatin1Char('\n');
    if (!title.isEmpty())
        out += QStringLiteral("title: ") + slidevYamlScalar(title) + QLatin1Char('\n');
    out += QStringLiteral("highlighter: shiki\nmdc: true\n---\n\n");
    for (int i = 0; i < slides.size(); ++i) {
        if (i > 0)
            out += QStringLiteral("\n\n---\n");
        if (!layouts.at(i).isEmpty()) {
            if (i == 0)
                out += QStringLiteral("---\n");
            out += QStringLiteral("layout: ") + layouts.at(i) + QStringLiteral("\n---\n\n");
        } else if (i > 0) {
            out += QLatin1Char('\n');
        }
        out += slides.at(i);
    }
    if (!out.endsWith(QLatin1Char('\n')))
        out.append(QLatin1Char('\n'));
    return out;
}

int slidevSlideCount(const QString &markdown) {
    const FrontMatter::Document document = FrontMatter::parse(markdown);
    int count = 1;
    bool inFence = false;
    for (const QString &line : document.body.split(QLatin1Char('\n'))) {
        if (line.trimmed().startsWith(QStringLiteral("```")))
            inFence = !inFence;
        if (!inFence && line.trimmed() == QLatin1String("---"))
            ++count;
    }
    return qMax(1, count);
}

QVariantList slidevPreviewSlides(const QString &markdown) {
    const QString exported = slidevMarkdown(markdown);
    const FrontMatter::Document document = FrontMatter::parse(exported);
    QStringList parts;
    QString current;
    bool inFence = false;
    for (const QString &line : document.body.split(QLatin1Char('\n'))) {
        if (line.trimmed().startsWith(QStringLiteral("```")))
            inFence = !inFence;
        if (!inFence && line.trimmed() == QLatin1String("---")) {
            if (!current.trimmed().isEmpty())
                parts.append(current.trimmed());
            current.clear();
            continue;
        }
        current += line + QLatin1Char('\n');
    }
    if (!current.trimmed().isEmpty())
        parts.append(current.trimmed());
    if (parts.isEmpty())
        parts.append(QStringLiteral("# Untitled"));

    QVariantList out;
    int index = 0;
    for (const QString &part : parts) {
        if (part.startsWith(QStringLiteral("layout:")) && !part.contains(QLatin1Char('#')))
            continue;
        QString title;
        QStringList bodyLines;
        for (const QString &line : part.split(QLatin1Char('\n'))) {
            if (title.isEmpty() && line.trimmed().startsWith(QLatin1Char('#'))) {
                title = line.trimmed();
                while (title.startsWith(QLatin1Char('#')))
                    title.remove(0, 1);
                title = title.trimmed();
                continue;
            }
            bodyLines.append(line);
        }
        while (!bodyLines.isEmpty() && bodyLines.first().trimmed().isEmpty())
            bodyLines.removeFirst();
        while (!bodyLines.isEmpty() && bodyLines.last().trimmed().isEmpty())
            bodyLines.removeLast();
        out.append(QVariantMap{
            {QStringLiteral("index"), index},
            {QStringLiteral("title"), title},
            {QStringLiteral("body"), bodyLines.join(QLatin1Char('\n'))},
        });
        ++index;
    }
    return out;
}

} // namespace CodeBlocks
