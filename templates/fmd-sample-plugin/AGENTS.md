# FMD community plugin

## Project overview

- Target: FMD Plugin (TypeScript → bundled JavaScript for Qt QJSEngine).
- Entry point: `src/main.ts` compiled to `main.js` and loaded by FMD `PluginHost`.
- Required release artifacts: `main.js`, `manifest.json`, and optional `styles.css`.
- Host API types: `fmd.d.ts` / `fmd.ts`. Docs: `docs/PLUGIN_API.md` in the FMD app repo, or the snapshot in the GitHub template.

## Environment & tooling

- Node.js: current LTS (Node 18+ recommended).
- **Package manager: npm** (required for this sample — `package.json` scripts).
- **Bundler: esbuild** (required for this sample — `esbuild.config.mjs`). Alternative bundlers OK if they emit a single CommonJS `main.js`.
- Types: `fmd.d.ts` + runtime `Plugin` base in `fmd.ts`.

### Install

```bash
npm install
```

### Dev (watch)

```bash
npm run dev
```

### Production build

```bash
npm run build
```

## File & folder conventions

- **Organize code into multiple files**: split features; keep `main.ts` focused on lifecycle.
- Source lives in `src/`.
- Example structure:
  ```
  src/
    main.ts           # Plugin entry, lifecycle
    settings.ts       # Settings interface + defaults
    commands/         # Command implementations
    ui/               # Modals / views (when host supports)
    utils/
  fmd.ts / fmd.d.ts   # Host API types + Plugin base
  manifest.json
  versions.json
  styles.css          # optional
  ```
- **Do not commit** `node_modules/` or generated `main.js` (see `.gitignore`).
- Keep the plugin small. Prefer browser-compatible packages (no Node APIs — QJSEngine is not Node).
- Release artifacts at the top level of the plugin folder: `main.js`, `manifest.json`, `styles.css`.

## Manifest rules (`manifest.json`)

- Must include: `id`, `name`, `version` (semver), `minAppVersion`, `description`, `permissions` (array).
- Optional: `author`, `authorUrl`, `fundingUrl`, `main` (default `main.js`).
- Never change `id` after release.
- Keep `minAppVersion` accurate when using newer APIs.
- `permissions` gate host API calls — request the minimum set.

## Testing / manual install

Copy `main.js`, `manifest.json`, `styles.css` (if any) to:

```text
<Workspace>/.fmd/plugins/<plugin-id>/
```

or

```text
~/.config/fmd/plugins/<plugin-id>/
```

Reload FMD plugins and enable in **Preferences → Plugins**.

## Commands & settings

- User-facing commands via `this.addCommand(...)` / `api.commands.register`.
- Persist settings with `api.settings.get` / `set` (scoped per plugin id).
- Use stable command IDs; avoid renaming once released.

## Versioning & releases

- Bump `version` in `manifest.json` and map in `versions.json`: plugin version → min FMD version.
- GitHub release tag must match `manifest.json` version exactly (no leading `v`).
- Attach `manifest.json`, `main.js`, `styles.css` as assets.

## Security, privacy, and compliance

- Default to local/offline. No network in v0 host API.
- No hidden telemetry. Disclose any future network use and require opt-in.
- Never execute remote code or auto-update outside normal releases.
- Read/write only inside the workspace via `vault.*` when available.
- Clean up listeners in `onunload` so disable/reload does not leak.

## UX & copy

- Sentence case for headings, buttons, titles.
- Arrow notation for navigation: **Preferences → Plugins**.
- Keep in-app strings short and consistent.

## Performance

- Keep `onload` light; defer heavy work.
- Debounce expensive work on `file-change`.

## Coding conventions

- TypeScript `"strict": true` preferred.
- Keep `main.ts` minimal; delegate feature logic.
- Bundle everything into `main.js` (CommonJS for QJSEngine host wrapper).
- Prefer `async/await`; handle errors gracefully.
- **Do not** use Node/Electron APIs.

## Agent do/don't

**Do**

- Add commands with stable IDs.
- Provide settings defaults.
- Write idempotent unload paths.
- Declare only needed `permissions` in `manifest.json`.
- Read `docs/PLUGIN_API.md` and keep examples copy-pasteable.

**Don't**

- Introduce network calls (unsupported in v0).
- Access files outside the workspace.
- Rename plugin `id` or published command ids.
- Commit `main.js` / `node_modules`.
- Assume Obsidian APIs exist — FMD uses `fmd` / `api`, not `obsidian`.

## Common tasks

### Add a command

```ts
this.addCommand({
  id: "your-command-id",
  name: "Do the thing",
  callback: () => this.doTheThing(),
});
```

### Status bar

```ts
this.api.ui.setStatusBarItem({ id: "my-status", text: "Ready" });
```

### Workspace events

```ts
this.api.workspace.on("file-open", (payload) => {
  console.log(payload.path);
});
```

## Troubleshooting

- Plugin doesn't load: ensure `main.js` + `manifest.json` are at `<Workspace>/.fmd/plugins/<id>/`.
- Missing `main.js`: run `npm run build` or `npm run dev`.
- Commands missing: verify `commands.register` permission and `addCommand` in `onload`.
- Status bar unchanged: need `ui.statusbar` + `editor.read` as applicable.
- `module is not defined`: build with esbuild `format: "cjs"` (this template already does).

## References

- FMD app: https://github.com/fred1357944/FMD
- Obsidian sample plugin (DX inspiration): https://github.com/obsidianmd/obsidian-sample-plugin
- API: `docs/PLUGIN_API.md`
- Platform MVP: `docs/PLUGIN_PLATFORM_MVP.md`
