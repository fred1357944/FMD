# FMD Plugin Platform — MVP Design (v0)

Goal: Obsidian-like extension surface on a Qt shell. AI-assisted authors write plugins against a stable API; community can collaborate later. Built-in “fake plugins” prove the seams before opening a public store.

Adapted from the omawrite plugin platform MVP design; product renamed to **FMD** (Fred's Markdown).

## 0. Non-goals (v0)
- Do not load Obsidian `.js` plugins as-is (different host APIs).
- Do not ship a plugin marketplace / review queue yet.
- Do not grant plugins raw filesystem or network by default.
- Do not require contributors to write C++/QML for v0 plugins.

## 1. Host architecture
```
┌─────────────────────────────────────────────┐
│  Qt / QML UI  (Main.qml, Board, Calendar…)  │
│           ▲  signals / invokables            │
│  C++ PluginHost (loads, sandboxes, routes)   │
│           ▲                                  │
│  Script runtime: Qt QJSEngine (JS ES-ish)     │
│  each plugin = folder + main.js + manifest   │
└─────────────────────────────────────────────┘
```
- **Why JS now:** matches “AI can write the plugin” and lowers contributor barrier; QJSEngine ships with Qt (no Electron).
- **C++/QML plugins** remain an advanced path later; v0 contract is JS (TypeScript → `main.js` via esbuild).
- Optional later: Lua if you want smaller sandboxes; start with one language.

## 2. On-disk layout
```
~/.config/fmd/plugins/
  com.example.wordcount/
    manifest.json
    main.js
    styles.css          # optional, if preview theming allowed

<workspace>/.fmd/plugins/
  your-plugin-id/
    manifest.json
    main.js
    styles.css

builtin/   (shipped inside app resources, same shape)
  fmd.core.statusbar-stats/
  fmd.core.markdown-tidy/
  fmd.core.frontmatter-defaults/     # planned (needs live frontmatter.set)
```

Workspace `.fmd/plugins/` is supported in v0 (Obsidian-style vault-local plugins).

## 3. manifest.json
```json
{
  "id": "fmd.core.statusbar-stats",
  "name": "Status bar stats",
  "version": "0.1.0",
  "minAppVersion": "0.1.0",
  "author": "FMD",
  "description": "Shows word/char count in the footer.",
  "main": "main.js",
  "permissions": ["editor.read", "ui.statusbar"]
}
```
Permissions are explicit; unknown permission → load fails (strict mode TBD; v0 logs and denies API calls).

## 4. Runtime API (minimal, stable names)
Exposed as global `fmd` (alias `api`) in `main.js`.

### Lifecycle
- `module.exports = class Plugin { onload(api) {} onunload() {} }`
- Host calls `onload` after inject; `onunload` on disable/quit.
- TypeScript authors extend `Plugin` from `fmd.d.ts` and bundle to `main.js`.

### `api.workspace`
- `getActiveFile(): { path, content, frontmatter } | null`
- `on(event, cb)` / `off` — events: `file-open`, `file-save`, `file-change`, `frontmatter-change`, `view-change` (`editor|table|board|calendar`)

### `api.editor` (requires `editor.read` / `editor.write`)
- `getSelection()`, `getValue()`, `setValue(text)` (prefer transactional edits later)
- `replaceSelection(text)`
- `getCursor()` / `setCursor(pos)` — keep positions simple (offset) in v0

### `api.frontmatter` (requires `frontmatter.read` / `.write`)
- `get(path?)`, `set(key, value)`, `update(patch)` — writes YAML; emits `frontmatter-change`

### `api.commands` (requires `commands.register`)
- `register({ id, name, shortcut?, callback })`
- Appears in command list / shortcut table

### `api.ui`
- `setStatusBarItem({ id, text })` — `ui.statusbar`
- `addSidebarView({ id, title, htmlOrMarkdown })` — v0 can be Markdown-only panel; rich QML later
- `notice(message)` toast

### `api.settings`
- `get(key, default)`, `set(key, value)` — scoped per plugin id, JSON file

### `api.vault` (requires `vault.read` / `vault.write`)
- `read(path)`, `write(path, text)`, `list(folder)` — **rooted at current workspace only**
- No `..` escape; no absolute paths outside workspace

### Deliberately omitted in v0
`fetch` / network, arbitrary shell, native modules, modifying other plugins, intercepting save without `editor.write`.

## 5. Three built-in “fake” plugins (prove seams)

### A. `fmd.core.statusbar-stats` (shipped)
- **Seams:** `editor.read`, `workspace` events, `ui.statusbar`
- On `file-open` / `file-change`: count words/chars → footer item
- Proves: read-only editor + UI chrome

### B. `fmd.core.markdown-tidy` (shipped)
- **Seams:** `editor.read` / `editor.write`, `commands.register`
- Command palette: **Tidy Markdown** — strip trailing spaces, keep at most one blank line outside fences, one trailing newline
- Proves: a plugin can rewrite the current note

### C. `fmd.core.frontmatter-defaults` (planned)
- **Seams:** `frontmatter.write`, `workspace` `file-open`
- If new note missing `status`/`date`, set defaults (`draft`, today)
- Blocked until `frontmatter.set` / `update` write through (currently no-ops in the JS bootstrap)

### D. `fmd.core.threads-draft-helper` (planned)
- **Seams:** `commands.register`, `vault.read`/`write`, settings
- Command: “New Threads draft”
- Reads a template from configured folder; writes new file; opens it
- Proves: commands + vault IO + product wedge. Native Threads draft already exists; this stays a seam demo.

Disable any built-in from Preferences → Plugins to prove unload.

## 6. Collaboration model (pre-marketplace)
1. **Spec-first:** this doc + `PLUGIN_API.md` so AI tools generate against the contract.
2. **Example repo:** https://github.com/fred1357944/fmd-plugin-template (GitHub template; in-tree copy `templates/fmd-sample-plugin/`).
3. **Convention:** plugin id reverse-DNS or slug; semver; changelog; `versions.json`.
4. **Trust:** v0 = manual install (drop folder). No auto-update.
5. **Later:** signed packages, community repo index, review checklist (permissions, no network unless requested).

## 7. Implementation order (engineering)
1. `PluginHost` + manifest parse + enable/disable persistence
2. QJSEngine sandbox + permission gate on each API call
3. Wire events from `Backend` (file open/save/change)
4. Ship built-ins A→B→C
5. Preferences UI: list plugins, toggle, show permissions
6. Publish `PLUGIN_API.md` + sample template
7. Harden vault-local plugins + optional network permission

## 8. Success criteria
- A new JS/TS plugin ≤50 lines can add a command + status bar text without recompiling the app.
- Disabling a built-in removes its UI/commands immediately.
- Board/Calendar still work when plugins only touch frontmatter via API (no private hooks).
- AI given `PLUGIN_API.md` + `fmd.d.ts` + `AGENTS.md` can scaffold a useful plugin on first try.

## 9. Positioning note
Competing with early Obsidian means: **sharp writing product + boring-stable API**, not feature parity day one. Plugins amplify the writing wedge.
