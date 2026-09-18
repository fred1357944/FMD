# FMD

**FMD** is Fred's Markdown writing workspace — a Qt Quick + C++ editor with live preview, project views, and a JavaScript plugin host.

Thanks to [DHH](https://github.com/dhh) and [omacom/omawrite](https://github.com/omacom-io/omawrite) (MIT) for the original writing app this work started from. New work is by [Hung Yi Lai](https://github.com/fred1357944).

A practical Markdown writing workspace built with Qt Quick and C++ that automatically follows system dark/light mode. Open a folder to browse top-level articles, edit source Markdown, and keep a live preview beside it.

<img width="2948" height="3227" alt="screenshot-2026-06-23_15-24-08" src="https://github.com/user-attachments/assets/4e930c0d-edda-4046-b444-a59eff523329" />
<img width="2948" height="3227" alt="screenshot-2026-06-23_15-23-23" src="https://github.com/user-attachments/assets/8ced7c26-961b-4ded-b263-84403001a951" />


## Install

macOS: `./bin/build` (qmake `fmd.pro` in `build-macos`), then **only** `./bin/install-macos` into `/Applications/FMD.app` (rewrites Homebrew Qt to `@executable_path`). Do not copy the qmake binary into Applications. Tests: `./bin/test`. Arch: `./bin/install` builds the `fmd` package. Agent rules: [AGENTS.md](AGENTS.md), live status [PROGRESS.md](PROGRESS.md).

## Shortcuts

- `Ctrl/Cmd+S` saves. Unsaved documents use the system file picker.
- `Ctrl/Cmd+Shift+S` saves as.
- `Ctrl/Cmd+O` opens a Markdown file.
- `Ctrl/Cmd+Shift+O` opens a workspace folder.
- `Ctrl/Cmd+P` opens the command palette (commands and files). Print is in the palette and the File menu.
- `Ctrl/Cmd+N` opens a new FMD window.
- `Ctrl/Cmd+Shift+N` creates `Untitled.md` in the open folder (then `Untitled-2.md`, …). File menu **New Markdown File**, the sidebar **+**, and the command palette do the same. If no folder is open, it asks you to open one first.
- `Ctrl/Cmd+Shift+T` or the footer **新帖** button opens a Threads draft. If the drafts folder already has a `threads-*.md` whose body still matches `_TEMPLATE-threads.md`, that unused file is reused instead of creating another copy. Footer **模板** opens the template; it does not create a draft. `Cmd+N` still opens a new window. `README.md`, `SCHEMA.md`, and `_TEMPLATE-*` files are omitted from the sidebar.
- Preferences: pick the Threads drafts folder (new installs default to `Documents/FMD/threads`; if `Documents/01_ACTIVE/threads` already exists, that path stays the default). Interface language is Traditional Chinese or English.
- `Ctrl/Cmd+Shift+Enter` (footer **立刻發**, File menu, or the command palette) saves the current note, runs `threads-schedule validate-omawrite`, then asks before `publish-now`. FMD never talks to the Threads API. Dry-run is in the File menu.
- `Ctrl/Cmd+Backspace` moves the current note to Trash (with confirmation). Right-click a file in the sidebar for Open / Reveal in Finder / Move to Trash.
- Saving a note whose YAML has `threads: true` (or `platform: threads`) moves that file — and `{stem}.assets` if present — into the Threads drafts folder and retargets the editor. Notes without the flag are left in place. Drafts already in the folder are not duplicated. Status stays `draft`; FMD does not queue or publish.
- `Ctrl/Cmd+,` opens Preferences, and `Ctrl/Cmd+?` keeps the shortcut reference one key away.
- `Ctrl/Cmd+Shift+L` toggles the article sidebar, and `Ctrl/Cmd+Shift+P` toggles the live preview pane.
- `Ctrl/Cmd+Z`, `Ctrl/Cmd+Shift+Z`, and `Ctrl/Cmd+Y` handle undo and redo.
- `F11` toggles fullscreen, and Linux keeps `Super+F`.
- `Ctrl/Cmd+F` searches the document. Use `Enter` or `Ctrl/Cmd+G` for the next match and `Shift+Enter` for the previous match.
- `Ctrl+H` opens find and replace on Linux. macOS uses `Cmd+Option+F` so `Cmd+H` keeps hiding the window.
- `Ctrl/Cmd+B`, `Ctrl/Cmd+I`, and `Ctrl/Cmd+K` insert bold, italic, and link Markdown.
- Type `/` at the start of a line for a Heptabase-style slash menu. **Code** opens a searchable language picker; `` ``` `` at the start of a paragraph does the same. Shift+Enter leaves a fenced code block. Preview turns fences into cards with a language label and Copy; Mermaid cards add Split / Code / Preview chrome (the diagram stays in the source; FMD is not a block editor).
- The footer also exposes a Preferences button, and collapsed panes show keyboard-focusable `Show Sidebar` / `Show Preview` controls so you can reopen them without reaching for the shortcuts.

## Workspace and preview

- The sidebar lists `.md` and `.markdown` files in the selected folder, including subfolders. Hidden directories and `{filename}.assets` image folders are skipped.
- After a folder is open, `Ctrl/Cmd+1`–`4` switch the center pane between Editor, Table, Board, and Calendar. The table is the manuscript schedule: File, Title, `status`, and `date` are always shown. Editing a status or date cell writes YAML front matter in that file. Board columns follow `status`; drag a card to change it. Press `×` on a column header to delete that status (empty columns disappear immediately; occupied columns ask first and move those notes to None). Calendar plots `date` (or `due` / `scheduled`); drag an unscheduled note onto a day to set the date. View settings stay in app preferences, not in the Markdown.
- The sidebar has Files, Outline, Tags, and Recent. Outline follows Quiet Outline lite: search headings, 1–6 level switch, collapse children without auto-expanding while you type, and highlight the current heading. Click a heading to jump; `Ctrl/Cmd+Shift+[` / `]` move to the previous/next heading. Drag-reorder and Vim keys are not cloned. Tags are collected from YAML `tags:` and inline `#tags`; click a tag to filter the folder, add one to the current note, or rename/merge it across files (Tag Wrangler, lite). Recent keeps the last 20 opened notes. Board cards show `- [ ]` / `- [x]` counts when a note has checkbox tasks. TaskGenius is not cloned: note-level status and dates already live in Board/Calendar.
- Opening a file with no workspace selected uses that file's parent folder as the workspace.
- Preferences remember whether the sidebar and live preview are shown, so the workspace comes back in the layout you last used.
- The preview hides YAML front matter and shows it as compact key/value chips (Obsidian-style properties). Body Markdown uses GitHub dialect, preserves line breaks, and uses a proportional CJK font. Fenced code is a rounded card with the language name and a Copy button. Preview themes Night / Newsprint / Gothic are simplified from Typora's open-source default themes. Notes with `threads: true` or `platform: threads` switch the right pane to a local Threads-style thread (main post plus `---` replies, 500-character meter). It never calls the Threads API. Set the display name and handle in Preferences.
- Pasting a screenshot, a local image `file:///...` URL, or an absolute image path copies the file into `{filename}.assets` beside the Markdown document and inserts a relative link such as `![Screenshot](./notes.assets/Screenshot.png)`. Images that already live in the document folder are linked in place. Save the document first; otherwise the paste is ignored instead of inserting a raw `file://` path. Supported types: PNG, JPEG/JPG, GIF, WebP, SVG, and clipboard bitmaps (saved as PNG).

Unsaved drafts are recovered after an abnormal exit. FMD also watches open files
and warns before an external change can replace local work.

Text follows the desktop text size — `omarchy display text size`, or GNOME's
`text-scaling-factor` — and re-flows without a restart. The default of 12px leaves
FMD at the size it is designed around; larger and smaller sizes scale from there.

## Plugins

JavaScript plugins load from app builtins, `~/.config/fmd/plugins/<id>/`, and `<workspace>/.fmd/plugins/<id>/`. Each plugin is `manifest.json` + `main.js`. The host injects `fmd` / `api` (see [docs/PLUGIN_API.md](docs/PLUGIN_API.md)). Builtins: **Status bar stats** and **Markdown tidy**. To write a community plugin, use the GitHub template [fred1357944/fmd-plugin-template](https://github.com/fred1357944/fmd-plugin-template) (or copy [templates/fmd-sample-plugin](templates/fmd-sample-plugin)), then **Reload plugins** from Preferences.

## Licensing (planned)

FMD will sell as a dual track: a lifetime license for the local editor, and an optional subscription for ongoing services. Activation will be a light license (trial, device count, online check with offline grace) — not an anti-reverse-engineering contest. Not wired in this build.

## Requirements

- Qt 6: `qt6-base`, `qt6-declarative`, `qt6-quickcontrols2`
- `xdg-desktop-portal` and a portal backend

The iA Writer Mono font is bundled under the SIL Open Font License 1.1; see
`fonts/OFL.txt`. The font is copyright Information Architects Inc. and based on
IBM Plex, copyright IBM Corp.
