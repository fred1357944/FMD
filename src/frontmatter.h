#pragma once

#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

namespace FrontMatter {

struct Document {
    bool hasFrontMatter = false;
    QVariantMap fields;
    QStringList fieldOrder;
    QString yamlBlock;
    QString body;
    int yamlStart = -1;
    int yamlEnd = -1;
};

Document parse(const QString &text);
QString setField(const QString &text, const QString &key, const QString &value);
QString setListField(const QString &text, const QString &key, const QStringList &values);
QString removeField(const QString &text, const QString &key);
QString displayValue(const QVariant &value);
QString headingTitle(const QString &body);
QString recordTitle(const QVariantMap &fields, const QString &body, const QString &fallbackName);
bool looksLikeIsoDate(const QString &value);
QString isoDate(const QString &value);
QString previewMarkdown(const QString &text);
bool isTruthy(const QVariant &value);
bool isGardenPublishable(const QVariantMap &fields);
bool isSlidevNote(const QVariantMap &fields);
bool isThreadsPost(const QVariantMap &fields);
QVariantList threadPosts(const QString &text);
int threadCharacterLimit();
QString normalizeTag(const QString &tag);
QStringList tagsFromValue(const QVariant &value);
QStringList hashtagsFromBody(const QString &body);
QStringList allTags(const QString &text);
QString addTag(const QString &text, const QString &tag);
QString rewriteTags(const QString &text, const QString &from, const QString &to);
QVariantList headingOutline(const QString &text);
QVariantList blockAnchors(const QString &text);
int fragmentPosition(const QString &text, const QString &kind, const QString &fragment);
int openTaskCount(const QString &body);
int closedTaskCount(const QString &body);

} // namespace FrontMatter
