# FMD Plugin API

Host API for **humans and AI coding agents**. Copy-paste examples below work against the QJSEngine runtime (TypeScript authors compile to `main.js`).

Global injected into every plugin: **`fmd`** (alias **`api`**).

Full platform design: [PLUGIN_PLATFORM_MVP.md](./PLUGIN_PLATFORM_MVP.md)  
Types: [`templates/fmd-sample-plugin/fmd.d.ts`](../templates/fmd-sample-plugin/fmd.d.ts)  
Agent guide: [`templates/fmd-sample-plugin/AGENTS.md`](../templates/fmd-sample-plugin/AGENTS.md)

---

## Quick mental model

```
Your plugin folder
  manifest.json     # id, permissions, minAppVersion
  main.js           # required entry (bundled from src/main.ts)
  styles.css        # optional

Host loads folder → creates QJSEngine → injects `fmd` → calls plugin.onload(fmd)
```

Install paths:
- User: `~/.config/fmd/plugins/<id>/`
- Workspace: `<Workspace>/.fmd/plugins/<id>/`
- Builtin: app resources (`:/plugins/...`)

---

## Lifecycle

```js
// main.js (CommonJS-style; host wraps module.exports)
class MyPlugin {
  onload(api) {
    // register commands, listeners, UI
  }
  onunload() {
    // cleanup; host also destroys the engine
  }
}
module.exports = MyPlugin;
```

TypeScript (sample template):

```ts
import { Plugin } from "./fmd"; // or from 'fmd.d.ts'

export default class SamplePlugin extends Plugin {
  async onload() {
    this.addCommand({
      id: "hello",
      name: "Say hello",
      callback: () => this.api.ui.notice("Hello from FMD"),
    });
  }

  onunload() {}
}
```

---

## Permissions

Declare in `manifest.json` → `permissions: string[]`. Missing permission → API call is a no-op (and may log).

| Permission | Allows |
|---|---|
| `editor.read` | `fmd.editor.getValue`, selection getters |
| `editor.write` | `setValue`, `replaceSelection` |
| `ui.statusbar` | `fmd.ui.setStatusBarItem` |
| `commands.register` | `fmd.commands.register` |
| `frontmatter.read` / `frontmatter.write` | frontmatter API |
| `vault.read` / `vault.write` | workspace-scoped file IO |

---

## `fmd.workspace`

### `getActiveFile()`
Returns `{ path, content, frontmatter? }` or empty/null-ish if none.

```js
const file = fmd.workspace.getActiveFile();
if (file && file.path) {
  console.log("Active:", file.path);
}
```

### Events — `on` / `off`

```js
function onChange(payload) {
  // payload.path when available
  fmd.ui.setStatusBarItem({ id: "demo", text: "changed: " + (payload.path || "") });
}
fmd.workspace.on("file-change", onChange);
// later: fmd.workspace.off("file-change", onChange);
```

Events: `file-open`, `file-save`, `file-change`, `frontmatter-change`, `view-change`.

---

## `fmd.editor`

```js
// requires editor.read
const text = fmd.editor.getValue();

// requires editor.write
fmd.editor.setValue(text + "\n\n<!-- stamped by plugin -->\n");
fmd.editor.replaceSelection("INSERT");
```

---

## `fmd.ui`

```js
fmd.ui.setStatusBarItem({ id: "my-plugin-status", text: "42 words" });
fmd.ui.notice("Saved draft");
// addSidebarView — planned
```

---

## `fmd.commands`

```js
fmd.commands.register({
  id: "my-plugin-insert-date",
  name: "Insert today's date",
  shortcut: "Ctrl+Shift+D", // optional; host-dependent
  callback: function () {
    const d = new Date().toISOString().slice(0, 10);
    fmd.editor.replaceSelection(d);
  }
});
```

Requires `commands.register`. Prefer stable command `id`s — do not rename after release.

---

## `fmd.settings`

Scoped per plugin id (in-memory in skeleton; persisted JSON later).

```js
const enabled = fmd.settings.get("enabled", true);
fmd.settings.set("enabled", false);
```

---

## `fmd.frontmatter` / `fmd.vault`

Stubs in the v0 skeleton — contract reserved:

```js
// frontmatter
fmd.frontmatter.get();
fmd.frontmatter.set("status", "draft");
fmd.frontmatter.update({ status: "published", date: "2026-09-16" });

// vault — paths relative to workspace only
fmd.vault.read("notes/hello.md");
fmd.vault.write("notes/hello.md", "# Hello\n");
fmd.vault.list("notes");
```

---

## Minimal complete plugin

`manifest.json`:

```json
{
  "id": "com.example.hello",
  "name": "Hello",
  "version": "0.1.0",
  "minAppVersion": "0.1.0",
  "description": "Notices on load",
  "author": "You",
  "main": "main.js",
  "permissions": ["ui.statusbar", "commands.register"]
}
```

`main.js`:

```js
module.exports = class HelloPlugin {
  onload(api) {
    api.ui.notice("Hello plugin loaded");
    api.commands.register({
      id: "hello-wave",
      name: "Wave",
      callback: function () { api.ui.notice("👋"); }
    });
  }
  onunload() {}
};
```

Manual install: copy `main.js` + `manifest.json` (+ `styles.css`) into  
`<Workspace>/.fmd/plugins/com.example.hello/`, then **Reload Plugins** (or restart FMD) and enable.

---

## TypeScript workflow

Start from the public GitHub template (Use this template / fork):

**https://github.com/fred1357944/fmd-plugin-template**

In-tree copy: [`templates/fmd-sample-plugin`](../templates/fmd-sample-plugin).

```bash
# After creating a repo from the GitHub template:
npm i
npm run dev    # watch → main.js
# Copy main.js + manifest.json into ~/.config/fmd/plugins/<id>/ or <Workspace>/.fmd/plugins/<id>/
# Reload plugins in FMD → Preferences → Plugins
```

---

## Do / Don't (short)

**Do:** declare only needed permissions; clean up in `onunload`; keep `main.js` self-contained (bundle deps).

**Don't:** expect Node/Electron APIs; fetch the network (not in v0); write outside the workspace; rename plugin `id` after publishing.
