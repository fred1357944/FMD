# DNS 說明卡 — notes.andgreen.org / fmd.andgreen.org

**不要改 apex 或 www。** `https://andgreen.org` 是綠亦（Vercel）。只加子網域，Cloudflare **灰雲（DNS only）**。

## 你要開的頁面

1. Cloudflare 登入：<https://dash.cloudflare.com/login>
2. 選網域 **andgreen.org**（左側網域清單，不要點錯成別的 zone）。
3. 左側 **DNS** → **Records**（或直接從總覽點 DNS）。

完整 DNS 紀錄頁（登入後選 zone）：<https://dash.cloudflare.com/?to=/:account/:zone/dns/records>

GitHub Pages 已能部署（`fred1357944.github.io/andgreen-notes/` 會 301 到自訂網域）。2026-09-18 兩筆灰雲 CNAME 已在 Cloudflare（`notes`／`fmd` → `fred1357944.github.io`）。權威 NS 與 1.1.1.1 已能解析；本機若仍 `ERR_NAME_NOT_RESOLVED` 是否定快取（SOA ~1800s），不是紀錄加錯。HTTP 已回 Quartz `Notes`；HTTPS 憑證可能還要等 GitHub 簽。

## 要加的兩筆（僅這兩筆）

先用 GitHub Pages 當 CNAME 目標（Education private repo 可掛 Pages）。之後若改掛 Zeabur 東京，只改 **Target**，Name 不變。

| Type | Name | Target | Proxy |
|------|------|--------|--------|
| CNAME | `notes` | `fred1357944.github.io` | **DNS only（灰雲）** |
| CNAME | `fmd` | `fred1357944.github.io` | **DNS only（灰雲）** |

TTL 用 Auto。

**不要**加 `@`、`www`，不要碰 MX / TXT / mail。

## GitHub Pages

兩個 private repo 建好後，在各 repo **Settings → Pages**：

- Source: **GitHub Actions**
- Custom domain: `notes.andgreen.org` 或 `fmd.andgreen.org`
- 勾選 Enforce HTTPS（憑證要等 DNS 生效）

本機倉庫：

- 花園：`/Users/laihongyi/Projects/andgreen-notes`（`CNAME` 檔已寫 `notes.andgreen.org`）
- Changelog：`/Users/laihongyi/Projects/fmd-site`（`CNAME` 檔已寫 `fmd.andgreen.org`）

## 驗收

```bash
dig +short notes.andgreen.org CNAME
curl -sL https://andgreen.org | grep -i title
curl -sL -o /dev/null -w '%{http_code}' https://notes.andgreen.org
```

- `notes` 的 CNAME 應是 `fred1357944.github.io`
- apex 的 `<title>` 仍是「Andgreen | 綠亦有限公司」
- `notes.andgreen.org` 200 且不是綠亦官網

`ERR_NAME_NOT_RESOLVED` 而權威 NS 已有紀錄 = 否定快取（andgreen.org SOA 約 1800s），清瀏覽器 DNS，不是加錯。
