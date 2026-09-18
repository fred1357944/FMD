#pragma once

#include <QHash>
#include <QJSEngine>
#include <QJSValue>
#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

class Backend;

class PluginHost;

class PluginApiBridge : public QObject {
    Q_OBJECT

public:
    explicit PluginApiBridge(Backend *backend,
                             PluginHost *host,
                             const QString &pluginId,
                             const QStringList &permissions,
                             QObject *parent = nullptr);

    Q_INVOKABLE void setStatusBarItem(const QVariantMap &item);
    Q_INVOKABLE void notice(const QString &message);
    Q_INVOKABLE QString editorGetValue() const;
    Q_INVOKABLE void editorSetValue(const QString &text);
    Q_INVOKABLE QVariantMap getActiveFile() const;
    Q_INVOKABLE void registerCommand(const QString &id, const QString &name,
                                    const QJSValue &callback);
    Q_INVOKABLE QVariant settingsGet(const QString &key,
                                    const QVariant &defaultValue = QVariant());
    Q_INVOKABLE void settingsSet(const QString &key, const QVariant &value);
    Q_INVOKABLE QString pluginId() const { return m_pluginId; }
    Q_INVOKABLE QVariantMap frontmatterGet() const;
    Q_INVOKABLE void frontmatterSet(const QString &key, const QVariant &value);
    Q_INVOKABLE void frontmatterUpdate(const QVariantMap &fields);
    Q_INVOKABLE QString vaultRead(const QString &relativePath) const;
    Q_INVOKABLE bool vaultWrite(const QString &relativePath, const QString &text);
    Q_INVOKABLE QStringList vaultList() const;
    Q_INVOKABLE QString editorGetSelection() const;
    Q_INVOKABLE void editorReplaceSelection(const QString &text);

    bool hasPermission(const QString &perm) const;

signals:
    void workspaceEvent(const QString &event, const QVariantMap &payload);

private:
    QString jailedWorkspacePath(const QString &relativePath) const;

    Backend *m_backend = nullptr;
    PluginHost *m_host = nullptr;
    QString m_pluginId;
    QStringList m_permissions;
};

class PluginHost : public QObject {
    Q_OBJECT
    Q_PROPERTY(QStringList loadedPluginIds READ loadedPluginIds NOTIFY loadedPluginsChanged)
    Q_PROPERTY(QVariantList loadedPlugins READ loadedPlugins NOTIFY loadedPluginsChanged)
    Q_PROPERTY(QVariantList commands READ commands NOTIFY commandsChanged)
    Q_PROPERTY(QString pluginStatusText READ pluginStatusText NOTIFY pluginStatusTextChanged)
    Q_PROPERTY(bool communityPluginsEnabled READ communityPluginsEnabled WRITE setCommunityPluginsEnabled NOTIFY communityPluginsEnabledChanged)

public:
    explicit PluginHost(Backend *backend, QObject *parent = nullptr);
    ~PluginHost() override;

    QStringList loadedPluginIds() const { return m_loadedIds; }
    QVariantList loadedPlugins() const { return m_loadedPluginInfo; }
    QVariantList commands() const { return m_commandList; }
    QString pluginStatusText() const { return m_pluginStatusText; }

    Q_INVOKABLE bool loadPluginFromMemory(const QString &id,
                                         const QString &manifestJson,
                                         const QString &mainJs);
    Q_INVOKABLE void setPluginEnabled(const QString &id, bool enabled);
    Q_INVOKABLE void revealUserPluginsFolder() const;
    Q_INVOKABLE QString userPluginsFolder() const;
    bool communityPluginsEnabled() const { return m_communityPluginsEnabled; }
    void setCommunityPluginsEnabled(bool enabled);
    Q_INVOKABLE void setEditorSelectionRange(int start, int end);
    QString editorSelection() const;
    void replaceCurrentSelection(const QString &text);
    int editorSelectionStart() const { return m_selStart; }
    int editorSelectionEnd() const { return m_selEnd; }

public slots:
    void loadBuiltinAndUserPlugins();
    void loadWorkspacePlugins(const QString &workspacePath);
    void unloadAll();
    void reloadAll();
    void emitWorkspaceEvent(const QString &event, const QVariantMap &payload = {});
    void invokeCommand(const QString &id);
    void setStatusBarItem(const QString &pluginId, const QString &itemId, const QString &text);
    void addCommand(const QString &pluginId, const QString &id, const QString &name,
                    const QJSValue &callback);

signals:
    void loadedPluginsChanged();
    void commandsChanged();
    void pluginStatusTextChanged();
    void communityPluginsEnabledChanged();
    void pluginError(const QString &pluginId, const QString &message);

private:
    struct PluginInstance {
        QString id;
        QString name;
        QString path;
        QStringList permissions;
        QJSEngine *engine = nullptr;
        PluginApiBridge *bridge = nullptr;
        QJSValue pluginObject;
        bool fromBuiltin = false;
    };

    struct PluginRecord {
        QString id;
        QString name;
        QString description;
        QString author;
        QString version;
        QString path;
        bool builtin = false;
        bool enabled = true;
    };

    bool registerPluginDir(const QString &dirPath, bool builtin);
    bool loadPluginDir(const QString &dirPath, bool builtin);
    bool loadPluginFromSource(const QString &id,
                              const QString &manifestJson,
                              const QString &mainJs,
                              const QString &originPath,
                              bool builtin);
    void unloadPlugin(PluginInstance &inst);
    void rebuildCommandList();
    void rebuildStatusText();
    void rebuildPluginInfo();

    Backend *m_backend = nullptr;
    QHash<QString, PluginInstance> m_plugins;
    QHash<QString, PluginRecord> m_catalog;
    QStringList m_catalogOrder;
    QStringList m_loadedIds;
    QVariantList m_loadedPluginInfo;
    QVariantList m_commandList;
    QHash<QString, QJSValue> m_commandCallbacks;
    QHash<QString, QString> m_commandNames;
    QHash<QString, QString> m_statusItems;
    QString m_pluginStatusText;
    QString m_workspacePath;
    bool m_communityPluginsEnabled = false;
    int m_selStart = 0;
    int m_selEnd = 0;
};
