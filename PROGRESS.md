# FMD 現況

> 最後更新：2026-09-18。狀態錨點；過程見 [dev-logs/](dev-logs/)。給人看的版本號見 [CHANGELOG.md](CHANGELOG.md)。規則見 [AGENTS.md](AGENTS.md)。

## 當前

已裝 0.6.0：SLIDEV 有封面範例；工具列 ★ 只看釘選；點卡片星星不會再誤開檔。請完全退出再開。

花園／changelog 空站已 push 為 private：`andgreen-notes`、`fmd-site`。偏好設定裡把花園站資料夾指到 `/Users/laihongyi/Projects/andgreen-notes`。

## 以後要做（未開工）

- 花園：評論、站內搜尋、會員
- FMD 內建「講」視圖（同一份 md 全螢幕分燈；[docs/PRESENTER.md](docs/PRESENTER.md)）
- 編輯器：拼字檢查、選取後 Markdown 預覽氣泡（[docs/EDITOR.md](docs/EDITOR.md)）
- 不安 Obsidian JS、不做白板、不開 iOS

## 工作紀錄（新在前）

| 紀錄 ID | 狀態 | 做了什麼 | 證據／限制 |
|---|---|---|---|
| FMD-20260918-060 | 已安裝 | 0.6.0 封面範例、★ 篩選、星星不誤開檔 | Totals: 121 passed；`/Applications/FMD.app` 16:40:18；otool 無 Homebrew；SLIDEV/threads 皆有 `card-cover-test.md` |
| FMD-20260918-059 | 已安裝 | 0.5.9 側欄文字省略＋標籤篩選不藏檔 | Totals: 121 passed；`/Applications/FMD.app` 16:25:15；otool 無 Homebrew |
| FMD-20260918-058 | 已安裝 | 0.5.8 檔案側欄左右欄可拖 | Totals: 121 passed；`/Applications/FMD.app` 16:20:17；otool 無 Homebrew；比例寫入 QSettings `view/workspaceNavSplitWidth` |
| FMD-20260918-057 | 已安裝 | 0.5.7 熱力圖進日曆、分頁對齊 ⌘1–6、智圖左側跳轉 | Totals: 120 passed；`/Applications/FMD.app` 16:09:27；otool 無 Homebrew；熱力圖週日起算、size%7==0；hex 色碼不當標籤 |
| FMD-20260918-056 | 已安裝 | 0.5.6 重開同一份 Slidev 時殺掉 npm+node 整組 | Totals: 120 passed；live `slidev-present` `ok=true url=http://127.0.0.1:3030/` 10.96s；舊 21424/21460 已死；listen `127.0.0.1:3030`；HTML title `FMD × Slidev 學習稿`；`/Applications/FMD.app` 15:55:43；otool 無 Homebrew |
| FMD-20260918-055 | 已安裝 | 0.5.5 Slidev 空白首頁＋native deck 原地開；側欄麵包屑／上一層 | Totals: 119 passed；`/Applications/FMD.app` 15:39:13；otool 無 Homebrew；Vite 仍在外面；未載入 Obsidian JS |
| FMD-20260918-054 | 已安裝 | 0.5.4 側欄資料夾 ▸／▾ 局部收合 | Totals: 117 passed；`/Applications/FMD.app` 15:16:22；otool 無 Homebrew |
| FMD-20260918-052 | 已安裝 | 0.5.2 雙欄右欄列出子資料夾＋監看 Finder 新增 | Totals: 116 passed；`/Applications/FMD.app` 14:49:01；otool 無 Homebrew；無深度上限；origin `https://github.com/fred1357944/FMD` master 已 push，未推 omacom/omawrite |
| FMD-20260918-050 | 已安裝 | 0.5.0 卡片牆＋雙欄側欄＋捷徑封面；Slidev 學習稿取代陽春標題頁 | Totals: 114 passed；`/Applications/FMD.app` 14:29:45；otool 無 Homebrew；未把 Vite 塞進 app；未載入 Obsidian JS；學習稿 PNG 溢出版測未在本輪逐張看 |
| FMD-20260918-041 | 已安裝 | 0.4.1 修 Slidev（theme-default + 聽 ::1）；⌘⌥E 在檔案清單顯示目前這篇 | Totals: 112 passed；`/Applications/FMD.app` 13:59:18；slidev-present 3045 `ok=true`；otool 無 Homebrew |
| FMD-20260918-040 | 已安裝 | 0.4.0 右側 Slides 預覽；site-push 一鍵推花園／changelog；Vite 仍在外面 | Totals: 111 passed；`/Applications/FMD.app` 13:31:47；`~/bin/site-push`；otool 無 Homebrew；notes CNAME 在 1.1.1.1 已指向 github.io，本機解析仍可能卡否定快取 |
| FMD-20260918-032 | 已安裝 | 0.3.2 屬性列讓出編輯區；短稿不打字機中段；新花園／新簡報模版；changelog 補 0.1.0 無版本號時期 | Totals: 110 passed；`/Applications/FMD.app` 13:07:36；otool 無 Homebrew；花園庫 `Documents/01_ACTIVE/GARDEN` |
| FMD-20260918-031 | 已安裝 | 0.3.1 預設 Slidev 資料夾 → `Documents/01_ACTIVE/SLIDEV`；專案 deck 只複製不搬；yttrans 29 套只指路 | Totals: 108 passed；`/Applications/FMD.app` 12:48:19；binary UTF-16 含 `/Documents/01_ACTIVE/SLIDEV`、無 `/Projects/fmd-slides`；otool 無 Homebrew；庫 14M；原專案未搬 |
| FMD-20260918-030 | 已安裝 | 0.3.0 Slidev 一鍵發布：layout: slides → 拷 decks/…/slides.md → slidev-present 開瀏覽器；成功後 status: sent | Totals: 107 passed；`/Applications/FMD.app` 12:27；`~/bin/slidev-present`；otool 無 Homebrew；Vite 不進 app |
| FMD-20260918-029 | 已安裝 | 0.2.9 Live 表格格線／對齊；表頭加列不再插進 `---` 上面；Threads 與 Markdown 預覽畫表 | Totals: 105 passed；`/Applications/FMD.app` 11:59；otool 無 Homebrew；[docs/PRESENTER.md](docs/PRESENTER.md) Slidev=Threads 式外掛 |
| FMD-20260918-028 | 已安裝 | 0.2.8 表格加欄／加列被編輯區蓋住點不到 | Totals: 105 passed；`/Applications/FMD.app` 11:40；otool 無 Homebrew；講者模式未做 |
| FMD-20260918-027 | 已安裝 | 0.2.7 資料夾批次發布；未上站 `[[wikilink]]` 拷貝時改純文字；Quartz 閱讀體驗（zh-TW、無 Plausible、字型建置時抓回） | Totals: 105 passed, 0 failed, 0 skipped, 0 blacklisted, 1614ms；`/Applications/FMD.app` 11:31；otool 無 Homebrew；花園 Pages `35303044158` success；changelog Pages `35303044405` success；apex 未改 |
| FMD-20260918-026 | 已安裝 | 0.2.6 打字機捲動、表格 Tab、看板依日期排序 | Totals: 102 passed, 0 failed, 0 skipped, 0 blacklisted, 1417ms；`/Applications/FMD.app` 11:17；[CHANGELOG.md](CHANGELOG.md) |
| FMD-20260918-REBUILD-INSTALL | 已安裝 | 在 `build-macos` 以 qmake6 + make 重建，再用 `bin/install-macos` 裝進 `/Applications/FMD.app` | 成功列 `installed /Applications/FMD.app/Contents/MacOS/fmd`；11:17；`otool -L` 無 `/opt/homebrew`；MacOS 目錄只有 `fmd`（未拷 qmake） |
| FMD-20260918-GARDEN-SLICE | 已安裝／待 DNS | ⌘Z；屬性下拉；拷 publish:true；private Quartz＋changelog 空站 | `tst_fmd` 101 passed；repos `andgreen-notes`／`fmd-site` private；[docs/DNS_NOTES.md](docs/DNS_NOTES.md)；apex 未改 |
| FMD-20260918-LIB-TABLE | 已安裝 | 日曆已發表篩選、月份拾取、pipe 表加欄加列、看板＋／日期；changelog 加厚並同步 threads | `tst_fmd` 98 passed；`/Applications/FMD.app` 10:38；[CHANGELOG.md](CHANGELOG.md)；[docs/PRESENTER.md](docs/PRESENTER.md) 只討論 |
| FMD-20260918-GARDEN-PLAN | 計畫 | 雙站花園：notes（Quartz）＋ fmd changelog（單頁）；L1 拷貝；不合庫 | [docs/PUBLISH_SITES.md](docs/PUBLISH_SITES.md)；apex 仍是綠亦；子網域 dig 空；未建站 |
| FMD-20260918-YAML-FOLD | 已安裝 | 0.2.2 測試沒掛 document，Live 開檔有時仍露出 YAML | `tst_fmd` 95 passed（含 attached-document）；`/Applications/FMD.app` 10:08；otool 無 Homebrew |
| FMD-20260918-PROPERTIES | 已安裝 | Live 開檔 YAML 仍可見；屬性列可收合／新增 | `tst_fmd` 93 passed；CHANGELOG 0.2.2；被 0.2.3 補測 |
| FMD-20260918-CHANGELOG | 完成 | 產品 changelog 與 PROGRESS 分開；0.2.0／0.2.1 | `CHANGELOG.md`；blog 站尚未做 |
| FMD-20260918-INBOX-REFRESH | 已安裝 | 刪收件匣文章後清單仍留著：trash 後 refresh + 監看 Inbox 資料夾 | `tst_fmd` 91 passed；`install-macos` |
| FMD-20260918-LOG | 完成 | 對齊 agora：PROGRESS + `fmd/AGENTS.md` + skill `fmd-qt-writer`／`dev-log-while-building` + workflow `fmd-ship` | `fmd-ship.rhai` smoke check passed（canned-host，未真跑安裝） |
| FMD-20260918-PUBLISHED | 已實作、待使用者看見 | 開 threads 資料夾時補搬 `status: sent\|published` 進 `published/` | `tst_fmd` 89 passed；磁碟上 9/17 兩篇 sent 仍在根目錄，等重開 app 才搬 |
| FMD-20260918-CORE | 已安裝 | Live/Source、禪、Inbox、庫篩選、` ```toc `、核心／社群外掛、API stubs | 88 then 89 tests；不安 Obsidian JS |

起點 commit `8f98892`（上游 omawrite）。本輪尚未 commit。不准編造尚未存在的 SHA。
