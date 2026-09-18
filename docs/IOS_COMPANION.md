# FMD iOS companion (plan)

Status: planned. Not started. Desktop FMD stays the Mac writer; iPhone is a later, separate app.

## What we want

Edit Markdown on the phone and read it while moving. Same files as the Mac app. That includes notes that are mindmaps (`mindmap: true` + heading + nested lists).

## What we will not do

- Do not compile the current Qt/QML desktop UI for iOS.
- Do not wrap `/Applications/FMD.app` in Capacitor.
- Do not expect Obsidian-style community plugins on v1.

Qt *can* ship to the App Store (QField, Mergin Maps). Those apps rebuilt a touch UI around an existing engine. FMD’s desktop chrome (split panes, Cmd shortcuts, folder-as-workspace) is the wrong UI for a phone. A companion is cheaper and a better product.

## Product shape

A small native iOS app (SwiftUI first) that:

1. Reads and writes `.md` in an iCloud Drive container **and/or** On My iPhone → Files (document picker).
2. Opens one note at a time: editor with a readable preview.
3. Syncs by the filesystem. If the Mac vault already lives in iCloud Drive, the phone sees the same folder. No FMD server.
4. Treats a mindmap file as the same Markdown tree the desktop uses. v1 can be outline-only (edit the lists). Canvas on iOS is a later slice, not a blocker for “move around and edit”.

Suggested bundle id: `org.hungyilai.fmd` (or whatever we register with Apple). Apple Developer Program required ($99/year). TestFlight before store.

## File contract (shared with desktop)

Desktop mindmap serialize format is the phone’s contract:

```markdown
---
mindmap: true
---

# Root title

- Child
  - Grandchild
```

Ordinary notes stay ordinary Markdown. YAML front matter is optional. The phone must not rewrite files it is only displaying.

## Work slices (when we start)

1. Apple account, iCloud container, empty SwiftUI shell, Files/iCloud open-save.
2. Markdown editor + preview for one file; font large enough to walk and read.
3. Folder list of `.md` (the vault).
4. Outline editor for `mindmap: true` notes (indent/outdent the lists).
5. Optional: canvas later, or keep outline-only on phone.

## Open decisions (do not invent)

- Default vault: user-picked iCloud folder vs FMD-owned container.
- Paid app vs free companion to a Mac license.
- Whether Android is ever in scope (not iOS-first).
