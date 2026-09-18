# 02｜Live YAML 開檔仍可見 + 屬性列（2026-09-18）

現況以 [PROGRESS.md](../PROGRESS.md) 為準。

## 現象

Live 點進有 YAML 的稿，有時正文仍顯示 `---` metadata，要手動切 Source／Live 才藏。屬性列要可收合、可「新增屬性」（對齊 Obsidian properties，不安 Obsidian JS）。

## 0.2.2 為什麼不夠

`liveModeHidesYamlEvenWhenCaretIsAtStart` 沒掛 `QTextDocument`，只查 `hiddenRangesAt`。開檔後 QML TextEdit 下一拍會把第一次 folding 蓋掉；切模式會強制 rehighlight，所以「有時」要手動切。

## 0.2.3

- 掛上 TextEdit 的測試：YAML block `isVisible()==false`；兩個同長度 YAML 檔連開也藏。
- 開檔／切模式：同步 folding + queued 再折一次。
- 屬性列：「新增屬性」按鈕才展開鍵／值欄。

## 驗收

- `./bin/test`：95 passed。
- `./bin/install-macos` → `/Applications/FMD.app` 2026-09-18 10:08；`otool -L` 無 `/opt/homebrew`。
- 限制：使用者尚未完全退出重開確認畫面。Source 仍顯示原始 YAML（故意）。
