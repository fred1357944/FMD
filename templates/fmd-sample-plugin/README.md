# FMD plugin template

Starter for **[FMD](https://github.com/fred1357944/FMD)** (Fred's Markdown) community plugins.

This is a GitHub **template** repository. Click **Use this template** → create a new repo, then write your plugin. Do not send plugin PRs here.

FMD is not Obsidian. Plugins are `manifest.json` + bundled `main.js` against the `fmd` / `api` host (Qt QJSEngine). Obsidian community `.js` will not load.

## 中文（給要開放別人開發的人）

1. 開 https://github.com/fred1357944/fmd-plugin-template → **Use this template**（或 Fork）。
2. 改 `manifest.json` 的 `id` / `name` / `author`（`id` 一旦發布就不要再改）。
3. `npm i` → `npm run dev`（watch）或 `npm run build`。
4. 把 `main.js`、`manifest.json`、可選的 `styles.css` 放到：
   - `~/.config/fmd/plugins/<id>/`，或
   - `<工作區>/.fmd/plugins/<id>/`
5. 在 FMD：**偏好設定 → 已安裝的外掛 → 重新載入**，打開開關。

完整 API：本 repo 的 `docs/PLUGIN_API.md` 與 `fmd.d.ts`。應用本體：https://github.com/fred1357944/FMD

## Quick start

```bash
npm i
npm run dev     # watch → main.js
# or
npm run build   # production main.js
```

Node.js 18+.

Install into FMD:

```text
~/.config/fmd/plugins/<your-plugin-id>/
    manifest.json
    main.js
    styles.css          # optional
```

Folder name should match `manifest.json` `id`. Reload plugins, then enable in **Preferences → Plugins**.

## What this sample does

- Registers a command that shows a notice
- Updates a status bar item with a word count
- Loads / saves settings
- Listens to `file-open` / `file-change`

Edit `src/main.ts`. Types: `fmd.d.ts`. Runtime `Plugin` base: `fmd.ts`. Agent notes: `AGENTS.md`.

## Rename before you ship

1. `manifest.json` → new `id`, `name`, `author`, `description`
2. `package.json` → `name`
3. Command ids in `src/main.ts` (stable after release)
4. Rebuild `main.js`

## Release

- Bump `version` in `manifest.json` and map it in `versions.json`
- GitHub release tag must match `manifest.json` `version` exactly (no leading `v`)
- Attach `manifest.json`, `main.js`, and `styles.css` (if any)

`npm version patch|minor|major` runs `version-bump.mjs`.

## API

- [PLUGIN_API.md](docs/PLUGIN_API.md) (snapshot; live spec lives in the FMD app repo)
- [fmd.d.ts](fmd.d.ts)

v0 has **no network**, **no Node APIs**, and **no plugin store**. `frontmatter.set` / `editor.replaceSelection` / `vault.list` are reserved names; check `PLUGIN_API.md` before relying on them.

## License

MIT. See [LICENSE](LICENSE).
