# FMD — 開發入口

先讀本檔與 [PROGRESS.md](PROGRESS.md)。這是產品工作，不要用父目錄 `journal/RESEARCH_PROGRESS.md`。

每次開發（含診斷、失敗試驗、文件）都必須更新 PROGRESS.md：開工寫「進行中」，結束前記結果／限制、驗證與證據。過程寫 [dev-logs/](dev-logs/)。不要另開「最新回顧」或 `handoff/` 當第二入口。skill：`fmd-qt-writer`、`dev-log-while-building`。

回覆用繁體中文。測試全綠 ≠ 使用者畫面對。本 repo 只本地 git，不 push，除非當次授權。

## 裝機

`./bin/test` → `build-macos` 裡 `qmake6 ../fmd.pro && make` → **只** `./bin/install-macos`。禁止把 qmake binary raw-cp 進 `/Applications/FMD.app`。也可 `/workflow fmd-ship`。

## 紅線

- Cmd+N = 新視窗。
- 不呼叫 Threads API；`~/bin/threads-schedule`。已發稿在 `{threads drafts}/published/`。
- 不能載入 Obsidian 外掛 JS。
- 不開 iOS 重寫、不重裝 Omawrite.app。
