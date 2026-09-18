# 01｜核心切片 + published 補搬（2026-09-18）

過程日誌，現況以 [PROGRESS.md](../PROGRESS.md) 為準。

## 做了

- Live/Source（藏 YAML／標記）、禪模式、跨專案 Inbox、表格／看板／日曆關鍵字＋日期篩選、原生 ` ```toc `、外掛頁核心／社群（社群預設關）、`replaceSelection`／`vault.list`／`frontmatter.set`、Vault changelog 社群外掛。
- 不安 Obsidian JS、不搬 Projects 外掛、不做白板／TaskNotes／XMind 全套。
- Threads：`published/` 應為 `{drafts}/published/`。9/17 14:01、14:44 已發兩篇時搬家碼還沒進已安裝 app，之後只打開沒存檔，資料夾沒被建出。補 `archiveAlreadyPublishedThreadsDrafts`：打開等於 drafts 的 workspace 時搬 sent/published。

## 驗收

- `./bin/test`：88 passed（核心切片），其後 89 passed（含 openingThreadsFolderArchivesAlreadySentPosts）。
- 安裝：`./bin/install-macos` → `/Applications/FMD.app` 09:14；`otool -L` 無 Homebrew。
- 限制：使用者尚未重開 app 確認側欄 `published/`。未 commit、未 push。

## 否決

把 archive 的 Obsidian Dynamic TOC / Projects 當 JS 載入；把「測試綠」當「published 資料夾已出現在使用者螢幕」。
