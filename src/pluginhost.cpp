#include "pluginhost.h"

#include "backend.h"
#include "frontmatter.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMetaType>
#include <QDesktopServices>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>
#include <QVariantList>
#include <algorithm>

PluginApiBridge::PluginApiBridge(Backend *backend,
                                 PluginHost *host,
                                 const QString &pluginId,
                                 const QStringList &permissions,
                                 QObject *parent)
    : QObject(parent)
    , m_backend(backend)
    , m_host(host)
    , m_pluginId(pluginId)
    , m_permissions(permissions)
{
}

bool PluginApiBridge::hasPermission(const QString &perm) const
{
    return m_permissions.contains(perm);
}

void PluginApiBridge::setStatusBarItem(const QVariantMap &item)
{
    if (!hasPermission(QStringLiteral("ui.statusbar")) || !m_host)
        return;
    const QString itemId = item.value(QStringLiteral("id")).toString();
    const QString text = item.value(QStringLiteral("text")).toString();
    m_host->setStatusBarItem(m_pluginId, itemId.isEmpty() ? m_pluginId : itemId, text);
}

void PluginApiBridge::notice(const QString &message)
{
    if (m_backend)
        m_backend->setStatus(message);
}

QString PluginApiBridge::editorGetValue() const
{
    if (!hasPermission(QStringLiteral("editor.read")) || !m_backend)
        return {};
    return m_backend->editorPlainText();
}

void PluginApiBridge::editorSetValue(const QString &text)
{
    if (!hasPermission(QStringLiteral("editor.write")) || !m_backend)
        return;
    m_backend->setEditorPlainText(text);
}

QVariantMap PluginApiBridge::getActiveFile() const
{
    QVariantMap map;
    if (!m_backend || m_backend->currentFilePath().isEmpty())
        return map;
    const QString text = m_backend->editorPlainText();
    map.insert(QStringLiteral("path"), m_backend->currentFilePath());
    map.insert(QStringLiteral("content"), text);
    map.insert(QStringLiteral("frontmatter"), FrontMatter::parse(text).fields);
    return map;
}

void PluginApiBridge::registerCommand(const QString &id, const QString &name,
                                     const QJSValue &callback)
{
    if (!hasPermission(QStringLiteral("commands.register")) || !m_host)
        return;
    m_host->addCommand(m_pluginId, id, name, callback);
}

QVariant PluginApiBridge::settingsGet(const QString &key, const QVariant &defaultValue)
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("plugin/") + m_pluginId);
    return settings.value(key, defaultValue);
}

void PluginApiBridge::settingsSet(const QString &key, const QVariant &value)
{
    QSettings settings;
    settings.beginGroup(QStringLiteral("plugin/") + m_pluginId);
    settings.setValue(key, value);
}

QVariantMap PluginApiBridge::frontmatterGet() const
{
    if (!hasPermission(QStringLiteral("frontmatter.read")) && !hasPermission(QStringLiteral("editor.read")))
        return {};
    if (!m_backend)
        return {};
    return FrontMatter::parse(m_backend->editorPlainText()).fields;
}

QString PluginApiBridge::jailedWorkspacePath(const QString &relativePath) const
{
    if (!m_backend)
        return {};
    const QString root = QDir::cleanPath(m_backend->workspaceFolderPath());
    if (root.isEmpty())
        return {};
    if (relativePath.contains(QStringLiteral("..")))
        return {};
    const QString candidate = QDir::cleanPath(QDir(root).filePath(relativePath));
    const QString prefix = root.endsWith(QLatin1Char('/')) ? root : root + QLatin1Char('/');
    if (candidate != root && !candidate.startsWith(prefix))
        return {};
    return candidate;
}

QString PluginApiBridge::vaultRead(const QString &relativePath) const
{
    if (!hasPermission(QStringLiteral("vault.read")))
        return {};
    const QString path = jailedWorkspacePath(relativePath);
    if (path.isEmpty())
        return {};
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return {};
    return QString::fromUtf8(file.readAll());
}

bool PluginApiBridge::vaultWrite(const QString &relativePath, const QString &text)
{
    if (!hasPermission(QStringLiteral("vault.write")))
        return false;
    const QString path = jailedWorkspacePath(relativePath);
    if (path.isEmpty())
        return false;
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
        return false;
    return file.write(text.toUtf8()) >= 0;
}

QStringList PluginApiBridge::vaultList() const
{
    if (!hasPermission(QStringLiteral("vault.read")) || !m_backend)
        return {};
    const QString root = QDir::cleanPath(m_backend->workspaceFolderPath());
    if (root.isEmpty())
        return {};
    const QStringList files = Backend::markdownFilesInDirectory(root);
    QStringList relative;
    const QDir dir(root);
    for (const QString &path : files) {
        const QString rel = QDir::fromNativeSeparators(dir.relativeFilePath(path));
        if (!rel.isEmpty() && !rel.startsWith(QLatin1String("..")))
            relative.append(rel);
    }
    return relative;
}

QString PluginApiBridge::editorGetSelection() const
{
    if (!hasPermission(QStringLiteral("editor.read")) || !m_host)
        return {};
    return m_host->editorSelection();
}

void PluginApiBridge::editorReplaceSelection(const QString &text)
{
    if (!hasPermission(QStringLiteral("editor.write")) || !m_host)
        return;
    m_host->replaceCurrentSelection(text);
}

void PluginApiBridge::frontmatterSet(const QString &key, const QVariant &value)
{
    if ((!hasPermission(QStringLiteral("editor.write"))
         && !hasPermission(QStringLiteral("frontmatter.write")))
        || !m_backend)
        return;
    QVariantMap fields;
    fields.insert(key, value);
    frontmatterUpdate(fields);
}

void PluginApiBridge::frontmatterUpdate(const QVariantMap &fields)
{
    if ((!hasPermission(QStringLiteral("editor.write"))
         && !hasPermission(QStringLiteral("frontmatter.write")))
        || !m_backend || fields.isEmpty())
        return;
    QString text = m_backend->editorPlainText();
    for (auto it = fields.constBegin(); it != fields.constEnd(); ++it) {
        if (it.value().typeId() == QMetaType::QStringList
            || it.value().canConvert<QStringList>()) {
            const QStringList list = it.value().toStringList();
            if (list.size() > 1 || it.value().typeId() == QMetaType::QStringList)
                text = FrontMatter::setListField(text, it.key(), list);
            else
                text = FrontMatter::setField(text, it.key(), it.value().toString());
        } else if (!it.value().isValid() || it.value().toString().isEmpty()) {
            text = FrontMatter::removeField(text, it.key());
        } else {
            text = FrontMatter::setField(text, it.key(), it.value().toString());
        }
    }
    m_backend->setEditorPlainText(text);
}

PluginHost::PluginHost(Backend *backend, QObject *parent)
    : QObject(parent)
    , m_backend(backend)
{
    m_communityPluginsEnabled =
        QSettings().value(QStringLiteral("plugins/communityEnabled"), false).toBool();
    if (!m_backend)
        return;
    connect(m_backend, &Backend::fileOpened, this, [this](const QString &path) {
        emitWorkspaceEvent(QStringLiteral("file-open"), {{QStringLiteral("path"), path}});
    });
    connect(m_backend, &Backend::fileChanged, this, [this](const QString &path) {
        emitWorkspaceEvent(QStringLiteral("file-change"), {{QStringLiteral("path"), path}});
    });
    connect(m_backend, &Backend::fileSaved, this, [this](const QString &path) {
        emitWorkspaceEvent(QStringLiteral("file-save"), {{QStringLiteral("path"), path}});
    });
    connect(m_backend, &Backend::workspaceFolderChanged, this, [this]() {
        loadWorkspacePlugins(m_backend->workspaceFolderPath());
    });
}

PluginHost::~PluginHost()
{
    unloadAll();
}

void PluginHost::emitWorkspaceEvent(const QString &event, const QVariantMap &payload)
{
    for (auto &inst : m_plugins) {
        if (inst.bridge)
            emit inst.bridge->workspaceEvent(event, payload);
    }
}

void PluginHost::setStatusBarItem(const QString &pluginId, const QString &itemId, const QString &text)
{
    m_statusItems.insert(pluginId + QLatin1Char('/') + itemId, text);
    rebuildStatusText();
}

void PluginHost::addCommand(const QString &pluginId, const QString &id, const QString &name,
                           const QJSValue &callback)
{
    const QString fullId = pluginId + QLatin1Char('.') + id;
    m_commandCallbacks.insert(fullId, callback);
    m_commandNames.insert(fullId, name.isEmpty() ? id : name);
    rebuildCommandList();
}

void PluginHost::invokeCommand(const QString &id)
{
    if (!m_commandCallbacks.contains(id))
        return;
    QJSValue callback = m_commandCallbacks.value(id);
    if (callback.isCallable())
        callback.call();
}

bool PluginHost::loadPluginFromMemory(const QString &id,
                                      const QString &manifestJson,
                                      const QString &mainJs)
{
    return loadPluginFromSource(id, manifestJson, mainJs, QStringLiteral(":memory:"), false);
}

void PluginHost::loadBuiltinAndUserPlugins()
{
    QDir builtin(QStringLiteral(":/plugins"));
    if (builtin.exists()) {
        const auto ids = builtin.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &id : ids)
            registerPluginDir(builtin.absoluteFilePath(id), true);
    }

    QDir bundledCommunity(QStringLiteral(":/community-plugins"));
    if (bundledCommunity.exists()) {
        const auto ids = bundledCommunity.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &id : ids) {
            if (!m_catalog.contains(id))
                registerPluginDir(bundledCommunity.absoluteFilePath(id), false);
        }
    }

    const QString configRoot = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QStringList candidates;
    candidates << (configRoot + QStringLiteral("/plugins"));
    candidates << (QDir::homePath() + QStringLiteral("/.config/fmd/plugins"));
    for (const QString &c : candidates) {
        QDir d(c);
        if (!d.exists())
            continue;
        const auto ids = d.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &id : ids) {
            if (!m_catalog.contains(id))
                registerPluginDir(d.absoluteFilePath(id), false);
        }
    }
    rebuildPluginInfo();
    emit loadedPluginsChanged();
}

void PluginHost::loadWorkspacePlugins(const QString &workspacePath)
{
    m_workspacePath = workspacePath;
    if (workspacePath.isEmpty())
        return;
    QDir d(workspacePath + QStringLiteral("/.fmd/plugins"));
    if (!d.exists())
        return;
    const auto ids = d.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
    for (const QString &id : ids) {
        if (m_plugins.contains(id) && !m_plugins[id].fromBuiltin) {
            unloadPlugin(m_plugins[id]);
            m_plugins.remove(id);
            m_loadedIds.removeAll(id);
        }
        if (!m_catalog.contains(id) || !m_catalog.value(id).builtin)
            registerPluginDir(d.absoluteFilePath(id), false);
    }
    rebuildPluginInfo();
    emit loadedPluginsChanged();
}

void PluginHost::unloadAll()
{
    for (auto it = m_plugins.begin(); it != m_plugins.end(); ++it)
        unloadPlugin(it.value());
    m_plugins.clear();
    m_catalog.clear();
    m_catalogOrder.clear();
    m_loadedIds.clear();
    m_commandCallbacks.clear();
    m_commandNames.clear();
    m_commandList.clear();
    m_statusItems.clear();
    m_pluginStatusText.clear();
    m_loadedPluginInfo.clear();
    emit loadedPluginsChanged();
    emit commandsChanged();
    emit pluginStatusTextChanged();
}

void PluginHost::reloadAll()
{
    const QString workspace = m_workspacePath;
    unloadAll();
    loadBuiltinAndUserPlugins();
    if (!workspace.isEmpty())
        loadWorkspacePlugins(workspace);
}

bool PluginHost::loadPluginDir(const QString &dirPath, bool builtin)
{
    return registerPluginDir(dirPath, builtin);
}

QString PluginHost::userPluginsFolder() const
{
    const QString folder = QDir::homePath() + QStringLiteral("/.config/fmd/plugins");
    QDir().mkpath(folder);
    return folder;
}

void PluginHost::revealUserPluginsFolder() const
{
    QDesktopServices::openUrl(QUrl::fromLocalFile(userPluginsFolder()));
}

void PluginHost::setCommunityPluginsEnabled(bool enabled)
{
    if (m_communityPluginsEnabled == enabled)
        return;
    m_communityPluginsEnabled = enabled;
    QSettings().setValue(QStringLiteral("plugins/communityEnabled"), enabled);
    emit communityPluginsEnabledChanged();

    if (!enabled) {
        const QStringList ids = m_loadedIds;
        for (const QString &id : ids) {
            auto it = m_plugins.find(id);
            if (it == m_plugins.end() || it->fromBuiltin)
                continue;
            unloadPlugin(it.value());
            m_plugins.erase(it);
            m_loadedIds.removeAll(id);
        }
    } else {
        for (const QString &id : m_catalogOrder) {
            const PluginRecord rec = m_catalog.value(id);
            if (rec.builtin || !rec.enabled || m_plugins.contains(id))
                continue;
            QFile manFile(rec.path + QStringLiteral("/manifest.json"));
            QFile mainFile(rec.path + QStringLiteral("/main.js"));
            if (manFile.open(QIODevice::ReadOnly) && mainFile.open(QIODevice::ReadOnly)) {
                loadPluginFromSource(id, QString::fromUtf8(manFile.readAll()),
                                     QString::fromUtf8(mainFile.readAll()), rec.path, false);
            }
        }
    }
    rebuildPluginInfo();
    rebuildCommandList();
    emit loadedPluginsChanged();
}

void PluginHost::setEditorSelectionRange(int start, int end)
{
    if (end < start)
        std::swap(start, end);
    m_selStart = qMax(0, start);
    m_selEnd = qMax(m_selStart, end);
}

QString PluginHost::editorSelection() const
{
    if (!m_backend)
        return {};
    const QString text = m_backend->editorPlainText();
    const int start = qBound(0, m_selStart, text.size());
    const int end = qBound(start, m_selEnd, text.size());
    return text.mid(start, end - start);
}

void PluginHost::replaceCurrentSelection(const QString &text)
{
    if (!m_backend)
        return;
    const int start = m_selStart;
    const int end = m_selEnd;
    m_backend->replaceEditorRange(start, end, text);
    m_selStart = start;
    m_selEnd = start + text.size();
}

void PluginHost::setPluginEnabled(const QString &id, bool enabled)
{
    if (!m_catalog.contains(id))
        return;
    PluginRecord rec = m_catalog.value(id);
    if (rec.enabled == enabled)
        return;
    rec.enabled = enabled;
    m_catalog.insert(id, rec);
    QSettings().setValue(QStringLiteral("plugin/enabled/") + id, enabled);

    if (!enabled && m_plugins.contains(id)) {
        unloadPlugin(m_plugins[id]);
        m_plugins.remove(id);
        m_loadedIds.removeAll(id);
    } else if (enabled && !m_plugins.contains(id)
               && (rec.builtin || m_communityPluginsEnabled)) {
        QFile manFile(rec.path + QStringLiteral("/manifest.json"));
        QFile mainFile(rec.path + QStringLiteral("/main.js"));
        if (manFile.open(QIODevice::ReadOnly) && mainFile.open(QIODevice::ReadOnly)) {
            loadPluginFromSource(id, QString::fromUtf8(manFile.readAll()),
                                 QString::fromUtf8(mainFile.readAll()), rec.path, rec.builtin);
        }
    }
    rebuildPluginInfo();
    rebuildCommandList();
    emit loadedPluginsChanged();
}

bool PluginHost::registerPluginDir(const QString &dirPath, bool builtin)
{
    QFile manFile(dirPath + QStringLiteral("/manifest.json"));
    if (!manFile.open(QIODevice::ReadOnly)) {
        qWarning() << "PluginHost: missing manifest in" << dirPath;
        return false;
    }
    const QString manifestJson = QString::fromUtf8(manFile.readAll());
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(manifestJson.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        emit pluginError(dirPath, QStringLiteral("Invalid manifest.json"));
        return false;
    }
    const QJsonObject obj = doc.object();
    const QString id = obj.value(QStringLiteral("id")).toString();
    if (id.isEmpty()) {
        emit pluginError(dirPath, QStringLiteral("manifest missing id"));
        return false;
    }

    PluginRecord rec;
    rec.id = id;
    rec.name = obj.value(QStringLiteral("name")).toString(id);
    rec.description = obj.value(QStringLiteral("description")).toString();
    rec.author = obj.value(QStringLiteral("author")).toString();
    rec.version = obj.value(QStringLiteral("version")).toString();
    rec.path = dirPath;
    rec.builtin = builtin;
    rec.enabled = QSettings().value(QStringLiteral("plugin/enabled/") + id, builtin).toBool();
    if (!m_catalog.contains(id))
        m_catalogOrder.append(id);
    m_catalog.insert(id, rec);

    if (!rec.enabled)
        return true;
    if (!builtin && !m_communityPluginsEnabled)
        return true;
    if (m_plugins.contains(id))
        return true;

    QFile mainFile(dirPath + QStringLiteral("/main.js"));
    if (!mainFile.open(QIODevice::ReadOnly)) {
        qWarning() << "PluginHost: missing main.js in" << dirPath;
        return false;
    }
    return loadPluginFromSource(id, manifestJson, QString::fromUtf8(mainFile.readAll()),
                                dirPath, builtin);
}

bool PluginHost::loadPluginFromSource(const QString &id,
                                      const QString &manifestJson,
                                      const QString &mainJs,
                                      const QString &originPath,
                                      bool builtin)
{
    if (m_plugins.contains(id)) {
        qWarning() << "PluginHost: already loaded" << id;
        return false;
    }

    const QJsonObject obj = QJsonDocument::fromJson(manifestJson.toUtf8()).object();
    QStringList permissions;
    for (const QJsonValue &v : obj.value(QStringLiteral("permissions")).toArray())
        permissions << v.toString();
    const QString pluginName = obj.value(QStringLiteral("name")).toString(id);

    auto *engine = new QJSEngine(this);
    engine->installExtensions(QJSEngine::ConsoleExtension);

    auto *bridge = new PluginApiBridge(m_backend, this, id, permissions, engine);
    const QJSValue bridgeVal = engine->newQObject(bridge);

    const QString apiBootstrap = QStringLiteral(
        "(function(bridge) {\n"
        "  var listeners = {};\n"
        "  bridge.workspaceEvent.connect(function(event, payload) {\n"
        "    var list = listeners[event] || [];\n"
        "    for (var i = 0; i < list.length; i++) try { list[i](payload); } catch (e) { console.error(e); }\n"
        "  });\n"
        "  return {\n"
        "    pluginId: bridge.pluginId(),\n"
        "    ui: {\n"
        "      setStatusBarItem: function(item) { bridge.setStatusBarItem(item); },\n"
        "      notice: function(msg) { bridge.notice(msg); }\n"
        "    },\n"
        "    editor: {\n"
        "      getValue: function() { return bridge.editorGetValue(); },\n"
        "      setValue: function(t) { bridge.editorSetValue(t); },\n"
        "      getSelection: function() { return bridge.editorGetSelection(); },\n"
        "      replaceSelection: function(t) { bridge.editorReplaceSelection(t || ''); }\n"
        "    },\n"
        "    workspace: {\n"
        "      getActiveFile: function() { return bridge.getActiveFile(); },\n"
        "      on: function(event, cb) {\n"
        "        if (!listeners[event]) listeners[event] = [];\n"
        "        listeners[event].push(cb);\n"
        "      },\n"
        "      off: function(event, cb) {\n"
        "        if (!listeners[event]) return;\n"
        "        listeners[event] = listeners[event].filter(function(f){ return f !== cb; });\n"
        "      }\n"
        "    },\n"
        "    commands: {\n"
        "      register: function(cmd) {\n"
        "        bridge.registerCommand(cmd.id || '', cmd.name || '', cmd.callback);\n"
        "      }\n"
        "    },\n"
        "    settings: {\n"
        "      get: function(k, d) { return bridge.settingsGet(k, d); },\n"
        "      set: function(k, v) { bridge.settingsSet(k, v); }\n"
        "    },\n"
        "    frontmatter: {\n"
        "      get: function() { return bridge.frontmatterGet(); },\n"
        "      set: function(k, v) { bridge.frontmatterSet(k, v); },\n"
        "      update: function(obj) { bridge.frontmatterUpdate(obj || {}); }\n"
        "    },\n"
        "    vault: {\n"
        "      read: function(p) { return bridge.vaultRead(p); },\n"
        "      write: function(p, t) { return bridge.vaultWrite(p, t); },\n"
        "      list: function() { return bridge.vaultList(); }\n"
        "    }\n"
        "  };\n"
        "})");

    const QJSValue bootstrapFn = engine->evaluate(apiBootstrap);
    const QJSValue api = bootstrapFn.call(QJSValueList() << bridgeVal);
    if (api.isError()) {
        emit pluginError(id, api.toString());
        delete engine;
        return false;
    }
    engine->globalObject().setProperty(QStringLiteral("fmd"), api);
    engine->globalObject().setProperty(QStringLiteral("api"), api);

    const QString wrapper = QStringLiteral(
        "(function() {\n"
        "  var module = { exports: {} };\n"
        "  var exports = module.exports;\n"
        "  %1\n"
        "  var Exported = module.exports.default || module.exports.Plugin || module.exports;\n"
        "  if (typeof Exported === 'function') {\n"
        "    try { return new Exported(); } catch (e) {\n"
        "      try { return new Exported(fmd); } catch (e2) { throw e2; }\n"
        "    }\n"
        "  }\n"
        "  if (Exported && typeof Exported.onload === 'function') return Exported;\n"
        "  return { onload: function(){}, onunload: function(){} };\n"
        "})()")
        .arg(mainJs);

    const QJSValue result = engine->evaluate(wrapper, originPath + QStringLiteral("/main.js"));
    if (result.isError()) {
        emit pluginError(id, result.toString());
        delete engine;
        return false;
    }

    QJSValue pluginObj = result;
    if (pluginObj.hasProperty(QStringLiteral("onload"))) {
        const QJSValue onload = pluginObj.property(QStringLiteral("onload"));
        const QJSValue callResult =
            onload.callWithInstance(pluginObj, QJSValueList() << api);
        if (callResult.isError()) {
            emit pluginError(id, callResult.toString());
            delete engine;
            return false;
        }
    }

    PluginInstance inst;
    inst.id = id;
    inst.name = pluginName;
    inst.path = originPath;
    inst.permissions = permissions;
    inst.engine = engine;
    inst.bridge = bridge;
    inst.pluginObject = pluginObj;
    inst.fromBuiltin = builtin;
    m_plugins.insert(id, inst);
    m_loadedIds.append(id);
    if (!m_catalog.contains(id)) {
        PluginRecord rec;
        rec.id = id;
        rec.name = pluginName;
        rec.path = originPath;
        rec.builtin = builtin;
        rec.enabled = true;
        m_catalog.insert(id, rec);
        m_catalogOrder.append(id);
    }
    rebuildPluginInfo();
    qInfo() << "PluginHost: loaded" << id << "from" << originPath;
    return true;
}

void PluginHost::unloadPlugin(PluginInstance &inst)
{
    if (inst.engine && inst.pluginObject.hasProperty(QStringLiteral("onunload"))) {
        QJSValue onunload = inst.pluginObject.property(QStringLiteral("onunload"));
        onunload.callWithInstance(inst.pluginObject);
    }
    const auto keys = m_commandCallbacks.keys();
    for (const QString &key : keys) {
        if (key.startsWith(inst.id + QLatin1Char('.'))) {
            m_commandCallbacks.remove(key);
            m_commandNames.remove(key);
        }
    }
    const auto statusKeys = m_statusItems.keys();
    for (const QString &key : statusKeys) {
        if (key.startsWith(inst.id + QLatin1Char('/')))
            m_statusItems.remove(key);
    }
    if (inst.engine) {
        delete inst.engine;
        inst.engine = nullptr;
        inst.bridge = nullptr;
    }
    rebuildCommandList();
    rebuildStatusText();
}

void PluginHost::rebuildStatusText()
{
    QStringList parts;
    for (auto it = m_statusItems.constBegin(); it != m_statusItems.constEnd(); ++it) {
        if (!it.value().isEmpty())
            parts.append(it.value());
    }
    const QString joined = parts.join(QStringLiteral(" · "));
    if (joined == m_pluginStatusText)
        return;
    m_pluginStatusText = joined;
    emit pluginStatusTextChanged();
}

void PluginHost::rebuildPluginInfo()
{
    QVariantList info;
    for (const QString &id : m_catalogOrder) {
        const PluginRecord rec = m_catalog.value(id);
        info.append(QVariantMap{
            {QStringLiteral("id"), rec.id},
            {QStringLiteral("name"), rec.name.isEmpty() ? rec.id : rec.name},
            {QStringLiteral("description"), rec.description},
            {QStringLiteral("author"), rec.author},
            {QStringLiteral("version"), rec.version},
            {QStringLiteral("path"), rec.path},
            {QStringLiteral("builtin"), rec.builtin},
            {QStringLiteral("enabled"), rec.enabled},
        });
    }
    m_loadedPluginInfo = info;
}

void PluginHost::rebuildCommandList()
{
    QVariantList list;
    for (auto it = m_commandCallbacks.constBegin(); it != m_commandCallbacks.constEnd(); ++it) {
        list.append(QVariantMap{
            {QStringLiteral("id"), it.key()},
            {QStringLiteral("title"), m_commandNames.value(it.key(), it.key())},
            {QStringLiteral("kind"), QStringLiteral("plugin")},
        });
    }
    m_commandList = list;
    emit commandsChanged();
}
