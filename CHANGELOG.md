# FMD Changelog

給人看的版本紀錄。語氣對齊 [Heptabase Changelog](https://wiki.heptabase.com/changelog) 與 [Grok Release Notes](https://grok.com/release-notes)：日期、分區、你實際會碰到的改變。

開發進行中、測試筆數、未完成項寫在 [PROGRESS.md](PROGRESS.md)。過程寫 [dev-logs/](dev-logs/)。不要把三份合成一份。

版本用 0.x，在還沒對外銷售前不跳 1.0。使用者看得見的功能加 **minor**；修既有功能加 **patch**。日期是裝進 `/Applications/FMD.app` 的紀錄日，不推估工時。

無獨立版本號的時期（clone → 改名 FMD）寫在 0.1.0，不發明 0.1.x。上游 omawrite 的 tag 不是 FMD 版本。

這份也會同步到寫作資料夾 `Documents/01_ACTIVE/threads/CHANGELOG/`。產品站快照在獨立 repo `fmd-site`（`fmd.andgreen.org`，DNS 待加）。

---

## 0.5.2 — 2026-09-18

### Library

- 雙欄側欄的右邊會列出**這一層的子資料夾**，不只 Markdown。點進去就進下一層，左邊資料夾樹也會縮排顯示更深的資料夾。在 Finder 新建的資料夾會被監看、不用重開才出現。

## 0.5.1 — 2026-09-18

### Sites

- 「發布花園」不再要求你先去偏好設定手填路徑。沒填時預設用 `~/Projects/andgreen-notes`（本機已有這座 Quartz 倉庫）。changelog 站預設 `~/Projects/fmd-site`。資料夾真的不存在時，錯誤會用中文寫出路徑。

## 0.5.0 — 2026-09-18

### Library

- 多了 **卡片** 視圖（⌘6）。封面來自 YAML `cover`／`image`／`banner`，或正文第一張圖。多個標籤是 AND。隨機抽一張、☆ 釘成捷徑、上面那條熱力圖點一下就篩那天。表格／看板／日曆還在，卡片是同一批筆記的另一種首頁。
- 檔案側欄改成雙欄：左邊資料夾、右邊目前這個資料夾的檔。有封面的會顯示小圖；釘過的在上面當捷徑。⌘⌥E 仍會捲到正在編的那篇。

### Present

- 「新簡報」不再是標題加一句話。模板本身就教換主題、`g`／⌘G 跳頁、兩欄。完整學習稿在 `Documents/01_ACTIVE/SLIDEV/00-fmd-one-click-starter.md`。

## 0.4.1 — 2026-09-18

### Present

- 「立刻開 Slidev」修好了：不再寫會害 Slidev 去找沒裝套件的 `theme: default`；專案缺 `@slidev/theme-default` 時會自動裝；就緒檢查同時看 `localhost`／`127.0.0.1`／`::1`（Slidev 53 只聽 IPv6 時不再誤判失敗）。失敗時對話框會帶 log。

### Library

- ⌘⌥E（指令「在檔案清單顯示目前這篇」）會展開上層資料夾並把側欄捲到正在編的檔。開檔時也會跟著捲，不必再往下滾。

## 0.4.0 — 2026-09-18

### Present

- `layout: slides` 時，右側預覽自動變成燈片（`#`／`##` 一張，左右鍵翻頁）。這是 FMD 自己的講臺，不是把 Vite 塞進 app。完整 Slidev（講者視窗、PDF）仍按 **立刻開 Slidev**，外面跑 `slidev-present`。
- 預覽下拉多了 **Slides**。

### Sites

- **發布花園** 會拷檔再呼叫外面的 `site-push` 做 `git commit`／`git push`，手感跟 Threads「立刻發」一樣。FMD 不存 GitHub token，用你本機已登入的 git。changelog 站同一條路。

## 0.3.2 — 2026-09-18

### Editor

- Live 屬性列展開時，編輯區會讓出高度，正文不再躲在欄位底下。短稿開著打字機捲動時，也不會被推到畫面中段，看起來像整頁跑掉。

### Present

- 跟 Threads「新帖」一樣：底下 **新花園**、**新簡報** 各開一篇帶好 YAML 的稿。未改過的範例會沿用，不會一直生空檔。花園預設寫進 `Documents/01_ACTIVE/GARDEN`，簡報寫進 Slidev 資料夾。

## 0.3.1 — 2026-09-18

### Present

- 一鍵 Slidev 的預設資料夾改成 `Documents/01_ACTIVE/SLIDEV`。之後發布過的簡報會進這個範例庫的 `decks/`。偏好設定仍可改。

## 0.3.0 — 2026-09-18

### Present

- 屬性設 `layout: slides`（或 `slidev: true`）後，指令「立刻開 Slidev」會把這篇轉成 Slidev 的 `slides.md`（`#`／`##` 一張燈），拷進獨立專案，再叫外面的 `slidev-present` 開瀏覽器。成功後 `status` 改成 `sent`，看板／日曆可以管狀態。`draft: true` 不發。
- Vite / Node 不進 FMD.app。跟 Threads 一樣：app 寫稿與改 YAML，真正跑起來的是 `~/bin/slidev-present`。

## 0.2.9 — 2026-09-18

### Editor

- Live 裡的 Markdown 表會對齊儲存格、表頭加底、格線變淡；`---` 分隔列在游標不在那一行時會藏起來，比較接近 Obsidian 的格子，不是一堆亂掉的 `|`。
- 在表頭按加列，會加在分隔列下面，不會再插進表頭與 `---` 中間。加欄／加列後會把欄寬對齊。

### Preview

- Markdown 預覽的表有表頭底色與清楚框線。
- Threads 預覽把 pipe 表畫成格子（Threads 送出仍是純文字 `|`）。

## 0.2.8 — 2026-09-18

### Editor

- 表格底下的 **加欄 →**／**加列 ↓** 現在點得到了。先前編輯區蓋在按鈕上面，看起來有按鈕、點下去卻是在點文字。

## 0.2.7 — 2026-09-18

### Sites

- 指令「發布本資料夾到花園站」會把目前專案裡所有 `publish: true`（或 `layout: post|page`）拷進花園 `content/`。`draft: true` 不拷；收件匣不拷。Threads 已發仍不是上站。
- 拷過去時，目標篇還沒在花園裡的 `[[wikilink]]` 會變成顯示文字（有 `|別名` 就用別名）。已經要上站或已經在 `content/` 的連結維持 `[[ ]]`，讓 Quartz 繼續互連。程式碼圍欄與 `![[圖]]` 不動。

## 0.2.6 — 2026-09-18

### Editor

- 打字機捲動：寫字時游標停在畫面中段（iA Writer 那種）。偏好設定可關。
- 表格裡 Tab / Shift+Tab 在儲存格間跳；最後一格再 Tab 會加一列。清單縮排仍是游標不在表內時的 Tab。

### Library

- 看板同一欄的卡片依日期由早到晚排，沒日期的放最後，方便看出發布節奏。

## 0.2.5 — 2026-09-18

### Editor

- ⌘Z 會還原你剛打的字。先前 Live 折 YAML 時關掉了文件的 undo 棧，看起來像編輯器壞了。
- 屬性列裡固定選項改下拉：`status`（draft / queued / sent / published…）、`threads`、`publish`、`draft`、`layout`、`platform`、`media_type`。仍可自己打沒列到的值。
- 指令面板新增「發布到花園站」。只拷 `publish: true`（或 `layout: post`）的目前這篇＋相對路徑的圖，進獨立 Quartz 倉庫的 `content/`，然後提示你 `git push`。`draft: true` 不拷。Threads 的 `status: sent` 不會自動上站。

### Sites

- 花園空站：private `andgreen-notes`（Quartz）。Changelog 空站：private `fmd-site`。apex / www 不動。

## 0.2.4 — 2026-09-18

- Library: 表格／看板／日曆可篩「節奏：已發表」。已發是 YAML `status: sent` 或 `published`、`publish: true`，或檔案已在 `published/`。日曆上只留這些稿，方便看發布節奏；草稿仍在「節奏：草稿」。
- Library: 點日曆月份標題會跳出年／月格子，可跳 **今天**／**昨天** 那個月份。左右箭頭仍一次換一個月。
- Board: 每欄顯示張數；卡片上若有 `date`／`scheduled` 會寫出來；欄上的 **+** 會在該 status 開一篇新筆記。
- Editor: 游標在 Markdown 表格裡時，編輯區下方出現 **加欄 →**／**加列 ↓**（對齊 Obsidian live table 的加在右邊／下面）。`/tablecol`、`/tablerow` 同樣可用。改的是 pipe 表，不是另外一套視覺表。

## 0.2.3 — 2026-09-18

- Editor: Live 點進有 YAML 的筆記時，不必再切一次 Source／Live 才把 metadata 藏起來。筆記上方的屬性列可收合，按「新增屬性」可自己加欄位。Source 仍編原始 YAML。

## 0.2.2 — 2026-09-18

- Editor: Live 打開有 YAML 的筆記時不再把 metadata 留在正文裡；屬性改在筆記上方，可收合、可改值、可「新增屬性」。切到 Source 仍編原始 YAML。

## 0.2.1 — 2026-09-18

- Inbox: 在 Finder 刪檔、或在 FMD 丟進垃圾桶後，側欄收件匣清單會跟著消失。

## 0.2.0 — 2026-09-18

從 omawrite 分出並改名 FMD 之後，第一次標版本號。下面這些你已經能在 `/Applications/FMD.app` 裡用。

- Editor: Live / Source。Live 藏 YAML 與 `**`、`==` 等標記，Source 顯示原始 Markdown。禪模式 ⌘⇧U，Esc 離開。
- Editor: `/table`、`/toc`、`/highlight`、`/color`（文字色與背景色可同時用）。` ```toc ` 預覽會依標題即時展開。
- Editor: 屬性列（0.2.2 起）對齊 Obsidian properties：可收合、改值、新增／刪欄。鍵值寫回 YAML。
- Editor: `[[wikilink]]`、反向連結、大綱、模板、指令面板、可自訂快捷鍵。
- Inbox: 跨專案收件匣（預設 `文件/FMD/Inbox`），打開不會切走目前專案。
- Library: 表格／看板／日曆／智圖。可依關鍵字、標籤、日期篩選（今天／昨天／近一週／有無日期）。看板欄是這個資料夾的 `status`，拖卡片只改 YAML。日曆可把稿拖到另一天或 Unscheduled。
- Mindmap: 從標題或列表長出來；Enter 同層、Tab 子節點；可收合、改色、⌘Z。
- Plugins: 核心與社群分開；社群預設關閉（Restricted Mode）。`replaceSelection`、`vault.list`、`frontmatter.set` 可用。不能載入 Obsidian `.js`。
- Threads: 草稿預設可走 `~/Documents/01_ACTIVE/threads`。已發稿（`status: sent` 且發布成功）進 `{drafts}/published/`。打開該資料夾時會補搬舊的已發稿。發布仍走 `~/bin/threads-schedule`，app 內不打 Threads API。
- Files: 側欄資料夾可一次收合／展開。預覽主題可切。Cmd+N 仍是新視窗。

## 0.1.0 — 無獨立版本號時期

從對話、session overview、本機 `.bak` 與 clone 紀錄補上。**沒有 FMD 的 0.1.1／0.1.2。** 上游 [omawrite](https://github.com/omacom-io/omawrite) 的 v0.x tag 不記入本表。起點仍是 clone 的 `8f98892`。日期是紀錄日。

- **2026-09-14：** 自 `omacom/omawrite` clone 進 `acousticpaper`；macOS Homebrew Qt 編成可開的 Omawrite.app。
- **2026-09-15：** 資料夾工作區、側欄與預覽、自含 Qt bundle；Preferences、收合側欄／預覽、貼圖。Copilot 做到一半撞 quota，之後換手。
- **2026-09-16：** Table／Board／Calendar；Threads 草稿與 `threads-schedule`（app 內不打 API）；程式碼塊。**改名 FMD**（Fred's Markdown），倉名 `fmd`，remote `fred1357944/FMD`。致謝 DHH／omawrite。
- **2026-09-17：** 裝進 `/Applications/FMD.app`（binary 不得連 Homebrew Qt）；智圖、`[[wikilink]]`、反向連結、模板、外掛 sample、`threads/published`。
- **2026-09-18：** 第一次標 **0.2.0**（Live／Source、禪、Inbox 等收進編號表）。

此時期功能在工作樹與備份裡，本地 git 長時間仍只有 clone 那一筆。不准把尚未存在的 SHA 寫成已發布。
