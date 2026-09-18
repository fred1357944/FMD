#pragma once

#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>

namespace MindMap {

struct Node {
    QString id;
    QString title;
    QVector<Node> children;
};

struct Tree {
    Node root;
    QString tail;
    QString layout = QStringLiteral("right");
    bool fromLists = false;
};

Tree parse(const QString &markdown);
QString serialize(const QString &originalMarkdown, const Tree &tree);
void assignIds(Node &node, const QString &id = QStringLiteral("0"));
Node *findNode(Node &node, const QString &id);
const Node *findNode(const Node &node, const QString &id);
QVariantMap toVariant(const Node &node);
Node fromVariant(const QVariantMap &map);
QVariantList flatten(const Node &node, int depth = 0,
                     const QStringList &collapsedIds = {});
QVariantMap layout(const Tree &tree, qreal textScale = 1.0,
                   const QStringList &collapsedIds = {});
QStringList collapsedIds(const QString &markdown);
QString applyNodeColor(const QString &title, const QString &color);
QString nodeColor(const QString &title);

struct Markup {
    QString kind = QStringLiteral("plain");
    QString display;
    QString target;
    QString fragment;
    QString fragmentKind;
    QString color;
    bool embed = false;
};

Markup parseMarkup(const QString &title);
QVariantMap markupToVariant(const Markup &markup);
bool looksLikeImageTarget(const QString &target);
bool isLinkPlaceholder(const QString &title);
QStringList wikiTargets(const QString &markdown);
QString rewriteGardenWikilinks(const QString &markdown, const QStringList &publishedStems);

QString rename(Node &root, const QString &id, const QString &title);
QString addChild(Node &root, const QString &parentId, const QString &title);
QString addSibling(Node &root, const QString &id, const QString &title);
QString removeNode(Node &root, const QString &id);
QString reparent(Node &root, const QString &id, const QString &newParentId, int index = -1);
QString indent(Node &root, const QString &id);
QString outdent(Node &root, const QString &id);

} // namespace MindMap
