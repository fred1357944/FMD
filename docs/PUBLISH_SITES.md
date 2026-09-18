# 整合決策 — FMD 筆記花園 + 產品 changelog — 2026-09-18

給人看的產品版本仍在 [CHANGELOG.md](../CHANGELOG.md)。本檔是跨生命週期的發布契約，不是第三份進度表。

## 一句話

兩個公開站、兩個 private git、同一套 FMD「opt-in 拷貝 + 提示 push」；寫作資料夾與 `fmd/CHANGELOG.md` 仍是各自的可寫真相源。綠亦 apex 不動。

## 系統與生命週期

| 系統 | 生命週期 | writable | 壞了傷誰 |
|------|----------|----------|----------|
| FMD.app 原始碼 | 產品開發 | `acousticpaper/fmd`（本地 git；origin 已指向公開 `fred1357944/FMD`，**未授權不 push**） | 用 FMD 寫作的人 |
| 寫作資料夾 | 作者手稿 | 本機 vault（Inbox、專案夾、threads 草稿） | 未發表的字與圖 |
| 個人花園站 | 已發布快照 | 獨立 private repo 的 `content/`（只吃拷貝） | 公開讀者 |
| FMD changelog 站 | 已發布快照 | 獨立 private repo（只吃 `CHANGELOG.md` 拷貝） | 想看 FMD 改了什麼的人 |
| 綠亦官網 | 公司產品 | Vercel（`andgreen.org` / `www`） | 綠亦客戶 |

**選定強度：L1 薄整合 published。** Monorepo 閘門否決：生命週期不同、部署邊界不同（Qt app ≠ 靜態站 ≠ 綠亦）、沒有共用算式需要 L3。

禁止：把 Hexo/Quartz 塞進 FMD.app、把整份 threads／Inbox／未發草稿推進站、在網站 repo 裡改文章當權威、`addDomain("andgreen.org")` / `www`。

## 建議（相對上一輪的調整）

上一輪預設「兩個都用 Hexo、`source/_posts`」。目標改成「筆記互連的花園」之後，Hexo 的 `_posts` 時間軸會把常青筆記逼成一篇篇 post，wikilink／反向連結要靠外掛硬接。

| 站 | 網址（子網域，DNS 尚空） | 產生器 | 為什麼 |
|----|--------------------------|--------|--------|
| 個人花園 | `notes.andgreen.org` | **Quartz 4** | `content/` 吃 Markdown；`[[wikilink]]`、反向連結、graph 是內建，不是之後再塞外掛。FMD 已經能寫 `[[ ]]`。 |
| 產品 changelog | `fmd.andgreen.org` | **單頁靜態**（把 `CHANGELOG.md` 編成 HTML） | 現在只有 0.2.x 幾條。第二套 Hexo／第二座花園是空轉。 |

FMD 端契約兩邊相同：不在 app 裡產生 HTML、不打網站 API。只把該拷的檔拷進對應 repo，然後提示 `git push`（跟 Threads 走 shell、不走站內 API 同一哲學）。

GitHub Education：兩個 repo 都先 **private**；Actions 跑 generate；Pages 或 Zeabur 都吃產物。要公開再改 visibility。本機 `gh api user` 的 plan 欄是空的，建 repo 時再確認 Education 額度還在。

公開的 `github.com/fred1357944/FMD` **不是**花園、也不是 changelog 站。App 原始碼與已發布文章不要合庫。

## YAML 契約（寫作 SSOT → 站）

閘門在**拷貝那一步**，opt-in。沒標就不走。

| 欄位 | 意思 |
|------|------|
| `publish: true` | 可進花園。canonical。 |
| `layout: post` | 視為同樣想發布（Hexo 肌肉記憶）。 |
| `draft: true` | 即使有 publish 也不拷。 |
| 缺 publish／layout | 不拷。 |
| `threads: true` 或 `platform: threads` | **不是**上站。Threads 的 `status: sent\|published` 只表示已發到 Threads。長文若也要進花園，必須另外加 `publish: true`。 |

Quartz 自己還有 `draft:`。我們不靠它當主閘：`content/` 裡只會出現已經過閘的快照。

圖片：只拷該篇相對路徑的圖（`![](./…)`、`![[…]]`），放到該篇在 `content/` 旁的同一相對位置。不整夾掃 vault。

## FMD 要做／不做

做（最小切片）：

- 設定一個「花園 repo 路徑」（之後 changelog 再一個）。
- 「發布到網站」：看**目前這篇**（可再加「發布本資料夾內所有 `publish: true`」）。過閘則拷 md＋相對圖 → 花園 `content/`；changelog 站只拷 `fmd/CHANGELOG.md`。
- 完成後提示在該 repo `git add`／`git status`／`git push`。App **不**存 GitHub token、不在程序裡實作 hexo/quartz generate。

不做：評論、站內搜尋、會員、把產生器塞進 `.app`、載入 Obsidian JS、在 app 裡重寫整套 HTML 佈景。yy4382「輸出時改 wikilink」在 0.2.7 用 C++ 做：未上站目標改成顯示文字；已上站或同批發布的維持 `[[ ]]`，Quartz 在 generate 時解析。`draft: true` 與收件匣仍不拷。

## 託管與 DNS

實測 2026-09-18：`https://andgreen.org` 的 `<title>` 仍是「Andgreen | 綠亦有限公司」。`notes`／`fmd`／`garden`／`blog` 子網域目前沒有 A/CNAME。

- Cloudflare：**各加一筆 CNAME**（灰雲／DNS only）。MX、TXT、apex、www **整列不動**。
- Zeabur 東京：兩個靜態服務，吃 Actions 產物或 repo 裡的 `public/`。不必為靜態站再租。
- Actions：private repo 裡 `quartz build`（花園）與 markdown→HTML（changelog）。Education 的 Actions 分鐘數先夠用。
- 拆錯綁：只 `removeDomain` 那個子網域。驗收兩側：新 host 是這個站；apex `<title>` 仍是綠亦。

## 外站借鑑（2026-09-18，討論結論）

看過 [anoni-net/docs](https://github.com/anoni-net/docs) 與 [Material for MkDocs](https://github.com/squidfunk/mkdocs-material)。**不把花園從 Quartz 換成 MkDocs。**

| 學 | 不學 |
|----|------|
| 發布是人按的閘（anoni 的 `main` ≠ 上線；對齊我們 L1 拷貝 + `git push`） | 把 pulse／OONI／onion／IPFS／PWA 搬進個人花園 |
| Material 的閱讀體驗：左欄目錄、站內搜尋、手機版、把 CDN 資產建置時抓回本地 | 用 `mkdocs.yml` 手列導覽當筆記互連（那是手冊，不是 `[[wikilink]]`） |
| changelog／產品說明若長成「使用手冊」，可另開 Material 站，不要塞進花園 | 在 FMD.app 裡跑 MkDocs；Insider 付費外掛 |

搜尋、評論仍在以後要做的清單；若做搜尋，行為對齊 Material 的即時篩選，實作仍走 Quartz 或靜態索引，不換棧。

## 切片（點頭後才動）

1. **花園空站**：private GitHub repo（Quartz 骨架 + 空 `content/` + Actions）。本機可 `npx quartz build` 看空首頁。
2. **DNS 說明卡**：`notes.andgreen.org` 要加的 Type/Name/Value（灰雲）。**先不出手改 apex。**
3. **FMD 最小發布**：讀 `publish: true`／`layout: post`，拷進 `content/`，提示 push。測試用一篇假筆記，不含 Inbox、不含 threads 整夾。
4. **changelog 空站**：private `fmd-site`；拷 `CHANGELOG.md`；`fmd.andgreen.org` 同一套灰雲。
5. **0.2.7**：拷貝時用 C++ 改未上站 wikilink；指令「發布本資料夾到花園站」。以後才做：已發 Threads 長文個別 opt-in。

順序：花園先（寫作主產品），changelog 同一週可做完，因為它幾乎是拷一份 md。

## 地基

- Apex 標題：2026-09-18 `curl https://andgreen.org`。
- 子網域空：同日 `dig`。
- 本機 FMD `origin`＝公開 `fred1357944/FMD.git`，但本工作樹未要求 push。
- Quartz `content/` 與 wikilink：<https://quartz.jzhao.xyz/authoring-content>
- 綠亦 apex 不可綁：skill `zeabur-headless-ops` §自訂網域；教訓「綁 X.org ≠ 把 apex 交給這個服務」。
