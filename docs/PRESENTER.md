# 設計討論 — FMD × Slidev × Presenter — 2026-09-18

先不改 code。這是整合強度討論，不是實作規格定案。

## 要解的問題

在 FMD 裡用 Markdown 寫完之後，要能**講**。參考：

- [sli.dev](https://sli.dev/)：Markdown → 瀏覽器簡報（Vite、Vue 元件、presenter 視窗、PDF）。
- [iA Presenter](https://ia.net/presenter)：跟 iA Writer 同一哲學，Markdown 就是簡報，本地、排版、講者視圖，不是再開一套幻燈軟體。

FMD 已經是 Markdown 工作區（Live/Source、YAML、wikilink）。不要變成第三個簡報 app，也不要把 Slidev 塞進 Qt。

## 生命週期（不合庫）

| 系統 | writable | 角色 |
|------|----------|------|
| FMD 筆記 | vault 裡的 `.md` | 寫作 SSOT |
| Slidev 專案 | 獨立 repo 的 `slides.md` | 已發布／可 `npx slidev` 的快照 |
| 原生「講」模式 | 無第二份檔 | FMD 當下這篇的投影視圖 |

強度：**L1 拷貝**（要給 Slidev／GitHub／課堂）＋可選 **FMD 內建講者模式**（不要另存一份）。不要 monorepo、不要在 `.app` 裡跑 Vite。

## 建議怎麼拆（超越「再包一個 Slidev」）

iA Presenter 強在：**同一份 Markdown，寫與講是兩種視圖**，不是匯出後就分叉。FMD 要贏，應該走這條，而不是在 Qt 裡嵌 Chromium 跑 `npx slidev`。

1. **原生 Presenter 視圖（主路徑）**  
   切到「講」：一個 `#`／`##` 一張燈；Live 排版；講者註解用 HTML 註解或 YAML `notes:`。預覽欄變當前燈、下一燈、計時。禪模式已經有全螢幕底子。這是 iA 的核心，FMD 還能多 YAML 屬性、wikilink 跳到另一篇燈、Threads 長文不要誤當簡報。

2. **Slidev 當輸出，不當宿主（支路徑）**  
   一篇標 `slidev: true` 或 `layout: slides` 時，「發布到 Slidev」= 拷進獨立 repo 的 `slides.md`（可加 frontmatter theme），提示 `npx slidev`。互動元件、Mermaid 進階、線上分享走這條。FMD 不執行 Node。

3. **明確不做**  
   在 app 內起 Vite；把 Vue 元件寫進 FMD；Obsidian 簡報外掛；把花園站、changelog 站、簡報 repo 合成一個 git。

## 簡報語意（先講清楚，再做）

一篇筆記是不是簡報，要 opt-in，避免把 Threads 草稿或論文當燈片：

- `layout: slides` 或 `presenter: true`
- 或檔在 `_slides/`／檔名 `.slides.md`

燈的切法預設：ATX 標題。`---` 只當 YAML，不當燈分隔（跟 Slidev 的 `---` 衝突，匯出時才轉）。

## 跟 Threads 同一套邊界（2026-09-18 建議）

Threads 已經是這個形狀：**FMD 只寫 Markdown；真正發出去的動作在 app 外面**（`~/bin/threads-schedule`）。花園站是另一種：app 裡拷檔，人去 `git push`。

Slidev 比較像 Threads，不像花園：

| | Threads | Slidev | 花園 |
|--|---------|--------|------|
| FMD 裡做 | 寫稿、YAML | 寫稿、`layout: slides`；可選「拷到 slides 倉庫」 | 拷 `publish: true` 進 `content/` |
| app 外面做 | `threads-schedule` 打 API | `npx slidev` / 瀏覽器講者視窗 | `git push` |
| 為什麼不塞進 `.app` | 不內建 Threads API | **不內建 Vite / Node / Chromium** | 本來就沒有產生器 |

**建議維持：Vite 不塞進 FMD.app。** Slidev 的價值是 Vue 元件、presenter 視窗、PDF，那些都活在 Node 裡；塞進來等於在 Qt 裡養第二個瀏覽器。要「站著講這篇」走 FMD 原生講者視圖（跟 Live／Source 一樣是同一份 md 的另一種看法）。要「課堂上用 Slidev 那套」走 L1 拷貝 + 外面的 `npx slidev`，指令面板可以提示那一行，跟提示 `git push` 同一哲學。

發布功能：**0.3.0 已做。** 拷貝在 app 裡；`~/bin/slidev-present` 在外面啟動 Vite。YAML `layout: slides` + `status`（draft／queued／sent）。

預設目的地（0.3.1）：`~/Documents/01_ACTIVE/SLIDEV`（範例庫／已發布快照）。一鍵寫入 `decks/<檔名>/slides.md`。專案層級的舊 deck 只複製進 `published/`，不搬原專案。課堂產線仍是 `~/Downloads/yttrans`。

## 切片

1. 討論定案（本檔）。  
2. **0.3.0**：一鍵發布到 Slidev + 狀態。  
3. **0.4.0**：FMD 右側「Slides」預覽（現有這篇、標題分燈、左右鍵）。完整 Slidev 仍走外面的 `slidev-present`。  
4. 以後：講者雙欄／全螢幕講。

## 跟 Hyperframes

[Hyperframes slideshow](https://hyperframes.heygen.com/) 是 HTML 時間軸簡報，產的是可播的 composition，不是 FMD 裡的 Markdown 視圖。要「把這篇變成一段影片」再走 Hyperframes；要「站著講這篇」走上面的 Presenter。不要混成同一條功能。
