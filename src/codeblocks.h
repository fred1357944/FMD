#pragma once

#include <QString>
#include <QVariantList>
#include <QVariantMap>

namespace CodeBlocks {

struct Fence {
    int openLine = -1;
    int closeLine = -1;
    int openPos = -1;
    int innerStart = -1;
    int innerEnd = -1;
    int closePos = -1;
    QString language;
    QString marker;
    QString code;
};

bool isFenceLine(const QString &line, QString *marker = nullptr, QString *info = nullptr);
QString languageFromInfo(const QString &info);
QList<Fence> parse(const QString &text);
bool oddFencesBefore(const QString &text, int position);
QString preserveLineBreaks(const QString &text);
QVariantList previewBlocks(const QString &source);
QVariantList languages(const QString &query, const QString &lastUsed = QString());
QVariantList slashCommands(const QString &query);
QString renderToc(const QString &source, const QString &options = QString());
QVariantMap slashQueryAt(const QString &text, int cursor);
QVariantMap backtickTriggerAt(const QString &text, int cursor);
QVariantMap leaveFenceAt(const QString &text, int cursor);
QString fenceText(const QString &language, const QString &inner);
int fenceCaretOffset(const QString &language, const QString &inner);
bool isTableLine(const QString &line);
bool isTableSeparator(const QString &line);
QVariantMap tableAt(const QString &text, int cursor);
QVariantMap insertTableColumn(const QString &text, int cursor);
QVariantMap insertTableRow(const QString &text, int cursor);
QVariantMap formatTable(const QString &text, int cursor);
QVariantMap tableMoveCell(const QString &text, int cursor, int delta);
QVariantList bodyBlocks(const QString &markdown);
QString slidevMarkdown(const QString &markdown);
int slidevSlideCount(const QString &slidevMarkdown);
QVariantList slidevPreviewSlides(const QString &markdown);

} // namespace CodeBlocks
