#include "markdownhighlighter.h"
#include "codeblocks.h"

#include <QColor>
#include <QFont>
#include <QFontMetricsF>
#include <QTextDocument>

MarkdownHighlighter::MarkdownHighlighter(QTextDocument *document)
    : QSyntaxHighlighter(document) {
    rebuildFormats();
}

void MarkdownHighlighter::setDarkMode(bool darkMode) {
    if (m_darkMode == darkMode)
        return;

    m_darkMode = darkMode;
    rebuildFormats();
    rehighlight();
}

void MarkdownHighlighter::setColors(const QString &background, const QString &foreground,
                                    const QString &accent) {
    if (m_customBackground == background && m_customForeground == foreground
            && m_customAccent == accent)
        return;

    m_customBackground = background;
    m_customForeground = foreground;
    m_customAccent = accent;
    rebuildFormats();
    rehighlight();
}

void MarkdownHighlighter::setSearch(const QString &query, int currentMatchStart) {
    if (m_searchQuery == query && m_currentMatchStart == currentMatchStart)
        return;
    m_searchQuery = query;
    m_currentMatchStart = currentMatchStart;
    rehighlight();
}

void MarkdownHighlighter::setLiveMode(bool live) {
    if (m_liveMode == live)
        return;
    m_liveMode = live;
    rehighlight();
}

void MarkdownHighlighter::setFrontMatterRange(int start, int end) {
    m_yamlStart = start;
    m_yamlEnd = end;
}

void MarkdownHighlighter::setCaretPosition(int position) {
    const int previous = m_caret;
    m_caret = position;
    if (!document())
        return;
    const QTextBlock oldBlock = document()->findBlock(qMax(0, previous));
    const QTextBlock newBlock = document()->findBlock(qMax(0, m_caret));
    if (oldBlock.isValid())
        rehighlightBlock(oldBlock);
    if (newBlock.isValid() && newBlock != oldBlock)
        rehighlightBlock(newBlock);
    if (m_liveMode && m_yamlStart >= 0) {
        const bool oldInside = previous >= m_yamlStart && previous < m_yamlEnd;
        const bool nowInside = m_caret >= m_yamlStart && m_caret < m_yamlEnd;
        if (oldInside != nowInside)
            rehighlight();
    }
}

bool MarkdownHighlighter::currentBlockIsYaml() const {
    if (m_yamlStart < 0 || m_yamlEnd <= m_yamlStart)
        return false;
    const int pos = currentBlock().position();
    return pos >= m_yamlStart && pos < m_yamlEnd;
}

void MarkdownHighlighter::rebuildFormats() {
    const QColor marker = m_darkMode ? QColor(QStringLiteral("#4f525a"))
                                     : QColor(QStringLiteral("#aeb1b5"));
    const QColor background = !m_customBackground.isEmpty() ? QColor(m_customBackground)
        : (m_darkMode ? QColor(QStringLiteral("#101010")) : QColor(QStringLiteral("#ffffff")));
    const QColor text = !m_customForeground.isEmpty() ? QColor(m_customForeground)
        : (m_darkMode ? QColor(QStringLiteral("#eeeeee")) : QColor(QStringLiteral("#222324")));
    const QColor link = m_darkMode ? QColor(QStringLiteral("#6cb6ff"))
                                   : QColor(QStringLiteral("#1a73e8"));
    const QColor quote = marker;
    const QColor codeBackground = m_darkMode ? QColor(QStringLiteral("#1c1a1a"))
                                             : QColor(QStringLiteral("#f8f8f8"));

    m_markerFormat = QTextCharFormat();
    m_markerFormat.setForeground(marker);

    // QML TextEdit sets font.pixelSize on the control; a pointSize char
    // format is often ignored, so hide with a 1px font plus negative spacing.
    m_hiddenMarkerFormat = QTextCharFormat();
    m_hiddenMarkerFormat.setForeground(background);

    QFont hiddenFont = document() ? document()->defaultFont() : QFont();
    hiddenFont.setPixelSize(1);
    m_hiddenMarkerFormat.setFont(hiddenFont);
    const qreal charWidth = QFontMetricsF(hiddenFont).horizontalAdvance(QLatin1Char('['));

    m_hiddenMarkerFormat.setFontLetterSpacingType(QFont::AbsoluteSpacing);
    m_hiddenMarkerFormat.setFontLetterSpacing(-charWidth);

    m_headingFormat = QTextCharFormat();
    m_headingFormat.setForeground(text);
    m_headingFormat.setFontWeight(QFont::Bold);

    m_boldFormat = QTextCharFormat();
    m_boldFormat.setFontWeight(QFont::Bold);
    m_boldFormat.setForeground(text);

    m_italicFormat = QTextCharFormat();
    m_italicFormat.setFontItalic(true);
    m_italicFormat.setForeground(text);

    m_codeFormat = QTextCharFormat();
    m_codeFormat.setForeground(text);
    m_codeFormat.setBackground(codeBackground);

    m_codeBlockFormat = QTextCharFormat();
    m_codeBlockFormat.setForeground(text);
    m_codeBlockFormat.setBackground(codeBackground);

    m_fenceLineFormat = QTextCharFormat();
    m_fenceLineFormat.setForeground(marker);
    m_fenceLineFormat.setBackground(codeBackground);

    m_fenceLanguageFormat = QTextCharFormat();
    m_fenceLanguageFormat.setForeground(link);
    m_fenceLanguageFormat.setBackground(codeBackground);
    m_fenceLanguageFormat.setFontWeight(QFont::DemiBold);

    m_quoteFormat = QTextCharFormat();
    m_quoteFormat.setForeground(quote);
    m_quoteFormat.setFontItalic(true);

    m_linkFormat = QTextCharFormat();
    m_linkFormat.setForeground(link);
    m_linkFormat.setFontUnderline(true);

    m_tablePipeFormat = QTextCharFormat();
    m_tablePipeFormat.setForeground(m_darkMode ? QColor(QStringLiteral("#3d4148"))
                                               : QColor(QStringLiteral("#d4d0c8")));

    m_tableHeaderFormat = QTextCharFormat();
    m_tableHeaderFormat.setForeground(text);
    m_tableHeaderFormat.setFontWeight(QFont::DemiBold);
    m_tableHeaderFormat.setBackground(m_darkMode ? QColor(QStringLiteral("#1c1e22"))
                                                 : QColor(QStringLiteral("#f3efe8")));

    m_tableCellFormat = QTextCharFormat();
    m_tableCellFormat.setForeground(text);
    m_tableCellFormat.setBackground(m_darkMode ? QColor(QStringLiteral("#141518"))
                                               : QColor(QStringLiteral("#faf8f4")));

    m_searchFormat = QTextCharFormat();
    m_searchFormat.setBackground(m_darkMode ? QColor(QStringLiteral("#725b18"))
                                            : QColor(QStringLiteral("#ffe58a")));
    m_currentSearchFormat = QTextCharFormat();
    m_currentSearchFormat.setBackground(m_darkMode ? QColor(QStringLiteral("#b36b20"))
                                                   : QColor(QStringLiteral("#ffad42")));
}

void MarkdownHighlighter::highlightBlock(const QString &text) {
    if (m_liveMode && currentBlockIsYaml()) {
        setFormat(0, text.length(), m_hiddenMarkerFormat);
        setCurrentBlockState(0);
        return;
    }

    const bool inFence = previousBlockState() == 1;
    QString marker;
    QString info;
    if (CodeBlocks::isFenceLine(text, &marker, &info)) {
        setFormat(0, text.length(), m_fenceLineFormat);
        const int infoStart = text.indexOf(info, 0, Qt::CaseSensitive);
        if (!info.isEmpty() && infoStart >= 0)
            setFormat(infoStart, info.size(), m_fenceLanguageFormat);
        const bool closing = inFence && info.trimmed().isEmpty();
        setCurrentBlockState(closing ? 0 : 1);
        highlightSearch(text);
        return;
    }

    if (inFence) {
        setFormat(0, text.length(), m_codeBlockFormat);
        setCurrentBlockState(1);
        highlightSearch(text);
        return;
    }

    setCurrentBlockState(0);
    if (!text.isEmpty() && CodeBlocks::isTableLine(text)) {
        highlightTableLine(text);
        highlightSearch(text);
        return;
    }
    if (!text.isEmpty()) {
        highlightMarkers(text);
        if (text.contains(QLatin1Char('`')) || text.contains(QLatin1Char('*'))
            || text.contains(QLatin1Char('_')) || text.contains(QLatin1Char('['))
            || text.contains(QLatin1Char('=')) || text.contains(QLatin1Char('<'))
            || text.contains(QLatin1String("http"), Qt::CaseInsensitive)
            || text.contains(QLatin1String("www."), Qt::CaseInsensitive)) {
            highlightInline(text);
            highlightColors(text);
        }
    }
    highlightSearch(text);
}

void MarkdownHighlighter::highlightSearch(const QString &text) {
    if (m_searchQuery.isEmpty())
        return;

    int from = 0;
    while ((from = text.indexOf(m_searchQuery, from, Qt::CaseInsensitive)) >= 0) {
        const int documentStart = currentBlock().position() + from;
        QTextCharFormat format = this->format(from);
        format.setBackground(documentStart == m_currentMatchStart
                                 ? m_currentSearchFormat.background()
                                 : m_searchFormat.background());
        setFormat(from, m_searchQuery.length(), format);
        from += qMax(1, m_searchQuery.length());
    }
}

void MarkdownHighlighter::highlightMarkers(const QString &text) {
    int first = 0;
    while (first < text.length() && text.at(first).isSpace())
        ++first;
    if (first >= text.length())
        return;

    const QChar firstChar = text.at(first);
    if (first == 0 && firstChar == QLatin1Char('#')) {
        static const QRegularExpression headingRe(QStringLiteral("^(#{1,6})(\\s+)(.*)$"));
        const QRegularExpressionMatch heading = headingRe.match(text);
        if (heading.hasMatch()) {
            const int markerLength = heading.capturedLength(1) + heading.capturedLength(2);
            const int blockPos = currentBlock().position();
            const bool caretOnLine = m_caret >= blockPos && m_caret <= blockPos + text.size();
            setFormat(0, markerLength,
                      (m_liveMode && !caretOnLine) ? m_hiddenMarkerFormat : m_markerFormat);
            setFormat(heading.capturedStart(3), heading.capturedLength(3),
                      m_headingFormat);
            return;
        }
    }

    if (firstChar == QLatin1Char('>')) {
        static const QRegularExpression quoteRe(QStringLiteral("^(\\s*>+\\s?)(.*)$"));
        const QRegularExpressionMatch quote = quoteRe.match(text);
        if (quote.hasMatch()) {
            setFormat(0, quote.capturedLength(1), m_markerFormat);
            setFormat(quote.capturedStart(2), quote.capturedLength(2), m_quoteFormat);
        }
    }

    if (firstChar == QLatin1Char('-') || firstChar == QLatin1Char('+')
            || firstChar == QLatin1Char('*') || firstChar.isDigit()) {
        static const QRegularExpression listRe(
            QStringLiteral("^(\\s*(?:[-+*]|\\d+[.)])\\s+)(.*)$"));
        const QRegularExpressionMatch list = listRe.match(text);
        if (list.hasMatch())
            setFormat(0, list.capturedLength(1), m_markerFormat);
    }

    if (firstChar == QLatin1Char('-') || firstChar == QLatin1Char('*')
            || firstChar == QLatin1Char('_')) {
        static const QRegularExpression ruleRe(QStringLiteral("^\\s{0,3}([-*_])(?:\\s*\\1){2,}\\s*$"));
        const QRegularExpressionMatch rule = ruleRe.match(text);
        if (rule.hasMatch())
            setFormat(0, text.length(), m_markerFormat);
    }
}

void MarkdownHighlighter::highlightTableLine(const QString &text) {
    const int blockPos = currentBlock().position();
    const bool caretOnLine = m_caret >= blockPos && m_caret <= blockPos + text.size();
    const QTextBlock previous = currentBlock().previous();
    const bool firstRow = !previous.isValid() || !CodeBlocks::isTableLine(previous.text());
    const bool separator = CodeBlocks::isTableSeparator(text);
    const bool header = firstRow && !separator;

    if (m_liveMode && separator && !caretOnLine) {
        setFormat(0, text.length(), m_hiddenMarkerFormat);
        return;
    }

    const QTextCharFormat &cellFormat = header ? m_tableHeaderFormat
        : (separator ? m_markerFormat : m_tableCellFormat);
    setFormat(0, text.length(), cellFormat);
    for (int i = 0; i < text.size(); ++i) {
        if (text.at(i) == QLatin1Char('|'))
            setFormat(i, 1, m_liveMode ? m_tablePipeFormat : m_markerFormat);
    }
}

void MarkdownHighlighter::highlightInline(const QString &text) {
    if (text.contains(QLatin1Char('`'))) {
        static const QRegularExpression codeRe(QStringLiteral("`([^`]+)`"));
        QRegularExpressionMatchIterator codeMatches = codeRe.globalMatch(text);
        while (codeMatches.hasNext()) {
            const QRegularExpressionMatch match = codeMatches.next();
            setFormat(match.capturedStart(0), match.capturedLength(0), m_codeFormat);
        }
    }

    const QList<InlineMarkup> markup = inlineMarkup(text);
    for (const InlineMarkup &item : markup) {
        const QTextCharFormat &contentFormat =
            item.kind == InlineKind::Bold ? m_boldFormat
            : item.kind == InlineKind::Italic ? m_italicFormat
                                              : m_linkFormat;
        setFormat(item.content.start, item.content.length, contentFormat);
        for (const Span &marker : item.markers)
            setFormat(marker.start, marker.length, m_hiddenMarkerFormat);
    }

    QList<QPair<int, int>> codeSpans;
    if (text.contains(QLatin1Char('`'))) {
        static const QRegularExpression codeRe(QStringLiteral("`([^`]+)`"));
        QRegularExpressionMatchIterator codeMatches = codeRe.globalMatch(text);
        while (codeMatches.hasNext()) {
            const QRegularExpressionMatch match = codeMatches.next();
            codeSpans.append({int(match.capturedStart()), int(match.capturedLength())});
        }
    }
    auto inCode = [&codeSpans](int start, int length) {
        const int end = start + length;
        for (const auto &span : codeSpans) {
            if (start < span.first + span.second && end > span.first)
                return true;
        }
        return false;
    };
    for (const Span &span : urlSpans(text)) {
        if (inCode(span.start, span.length))
            continue;
        setFormat(span.start, span.length, m_linkFormat);
    }
}

void MarkdownHighlighter::highlightColors(const QString &text) {
    static const QRegularExpression highlightRe(QStringLiteral("==([^=\\n]+)=="));
    QRegularExpressionMatchIterator hi = highlightRe.globalMatch(text);
    while (hi.hasNext()) {
        const QRegularExpressionMatch match = hi.next();
        QTextCharFormat format;
        format.setBackground(m_darkMode ? QColor(QStringLiteral("#725b18"))
                                        : QColor(QStringLiteral("#ffe58a")));
        setFormat(match.capturedStart(1), match.capturedLength(1), format);
        if (m_liveMode) {
            setFormat(match.capturedStart(0), 2, m_hiddenMarkerFormat);
            setFormat(match.capturedEnd(1), 2, m_hiddenMarkerFormat);
        }
    }

    static const QRegularExpression spanRe(
        QString::fromLatin1("<span\\s+style=\"([^\"]*)\">(.*?)</span>"),
        QRegularExpression::DotMatchesEverythingOption | QRegularExpression::CaseInsensitiveOption);
    QRegularExpressionMatchIterator spans = spanRe.globalMatch(text);
    while (spans.hasNext()) {
        const QRegularExpressionMatch match = spans.next();
        const QString style = match.captured(1);
        QColor fg;
        QColor bg;
        static const QRegularExpression colorRe(
            QStringLiteral(R"(color\s*:\s*(#[0-9A-Fa-f]{3,8}|[A-Za-z]+))"),
            QRegularExpression::CaseInsensitiveOption);
        static const QRegularExpression bgRe(
            QStringLiteral(R"(background(?:-color)?\s*:\s*(#[0-9A-Fa-f]{3,8}|[A-Za-z]+))"),
            QRegularExpression::CaseInsensitiveOption);
        const QRegularExpressionMatch colorMatch = colorRe.match(style);
        const QRegularExpressionMatch bgMatch = bgRe.match(style);
        if (colorMatch.hasMatch())
            fg = QColor(colorMatch.captured(1));
        if (bgMatch.hasMatch())
            bg = QColor(bgMatch.captured(1));
        QTextCharFormat format;
        if (fg.isValid())
            format.setForeground(fg);
        if (bg.isValid())
            format.setBackground(bg);
        setFormat(match.capturedStart(2), match.capturedLength(2), format);
        if (m_liveMode) {
            const int openLen = match.capturedStart(2) - match.capturedStart(0);
            const int closeStart = match.capturedEnd(2);
            const int closeLen = match.capturedEnd(0) - closeStart;
            if (openLen > 0)
                setFormat(match.capturedStart(0), openLen, m_hiddenMarkerFormat);
            if (closeLen > 0)
                setFormat(closeStart, closeLen, m_hiddenMarkerFormat);
        }
    }
}

QList<MarkdownHighlighter::Span> MarkdownHighlighter::urlSpans(const QString &text)
{
    QList<Span> spans;
    static const QRegularExpression urlRe(
        QStringLiteral(R"((?:^|[^A-Za-z0-9_/<(])((?:https?://|www\.)[^\s<>\"'）)」']+))"));
    static const QString urlTails = QStringLiteral(".,;:!?)]}>\"'");
    QRegularExpressionMatchIterator urlMatches = urlRe.globalMatch(text);
    while (urlMatches.hasNext()) {
        const QRegularExpressionMatch match = urlMatches.next();
        QString url = match.captured(1);
        int length = int(match.capturedLength(1));
        while (length > 0 && urlTails.contains(url.back())) {
            url.chop(1);
            --length;
        }
        if (length <= 0)
            continue;
        spans.append(Span{int(match.capturedStart(1)), length});
    }
    return spans;
}

QList<MarkdownHighlighter::InlineMarkup> MarkdownHighlighter::inlineMarkup(const QString &text) {
    QList<InlineMarkup> markup;
    if (!text.contains(QLatin1Char('*')) && !text.contains(QLatin1Char('_'))
            && !text.contains(QLatin1Char('['))) {
        return markup;
    }

    const auto span = [](const QRegularExpressionMatch &match, int group) {
        return Span{int(match.capturedStart(group)), int(match.capturedLength(group))};
    };

    static const QRegularExpression boldRe(QStringLiteral("(\\*\\*|__)(.+?)(\\1)"));
    QRegularExpressionMatchIterator boldMatches = boldRe.globalMatch(text);
    while (boldMatches.hasNext()) {
        const QRegularExpressionMatch match = boldMatches.next();
        markup.append({InlineKind::Bold, span(match, 2),
                       {span(match, 1), span(match, 3)}});
    }

    static const QRegularExpression italicRe(
        QStringLiteral("(?<!\\*)\\*([^*\\n]+)\\*(?!\\*)|(?<!_)_([^_\\n]+)_(?!_)"));
    QRegularExpressionMatchIterator italicMatches = italicRe.globalMatch(text);
    while (italicMatches.hasNext()) {
        const QRegularExpressionMatch match = italicMatches.next();
        const Span whole = span(match, 0);
        const int contentIndex = match.capturedStart(1) >= 0 ? 1 : 2;
        markup.append({InlineKind::Italic, span(match, contentIndex),
                       {{whole.start, 1}, {whole.start + whole.length - 1, 1}}});
    }

    static const QRegularExpression linkRe(
        QStringLiteral("\\[([^\\]]+)\\]\\(((?:\\\\.|[^)])+)\\)"));
    QRegularExpressionMatchIterator linkMatches = linkRe.globalMatch(text);
    while (linkMatches.hasNext()) {
        const QRegularExpressionMatch match = linkMatches.next();
        const Span whole = span(match, 0);
        const Span content = span(match, 1);
        const int contentEnd = content.start + content.length;
        markup.append({InlineKind::Link, content,
                       {{whole.start, 1},
                        {contentEnd, whole.start + whole.length - contentEnd}}});
    }

    return markup;
}
