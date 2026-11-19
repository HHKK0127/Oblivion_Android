# v562 — U6 / U4 裁定書: 起動時の素材配置の正典・タイトル／キャラ作成の左上要素（ai-todo #166 / #156）

対象: `Oblivion.exe` v1.2.0416 (Steam)＋実データ配置。根拠は BSA 実読（`bethesda_structs`）＋ loose ファイル実測 ＋ 実物 XML。
判定: `[確定]` / `[確定(XML)]` / `[確定負]` / `[部分確定]`

---

## U6 — 起動時の素材展開順序の正典

### 結論 **[確定] 原版に「展開」という工程は存在しない**

| 素材 | 原版の置き場所 | 根拠 |
|---|---|---|
| メニュー XML（`menus\**`） | **`Oblivion - Misc.bsa` 内** | BSA 実読で `menus\` 配下 89 個の XML を確認 |
| 動画（`.bik`） | **loose ファイル `Data\Video\*.bik`** | 5 つの主要 BSA（Misc/Meshes/Sounds/Voices1/Voices2・計 89,610 エントリ）に **`.bik` / `video\` エントリは 0 件** |
| 動画の参照方法 | `"Data\Video\" + 名前` を `_BinkOpen` | `FUN_00410160`（v557 §2.2） |

* 原版は BSA を**ランタイムでマウント**する（展開しない）。動画だけが BSA 外の loose ファイル。
* したがって「アセット展開順序」の正典は存在せず、**移植側の `AssetExtractor` の順序設計は独自実装**である。
* 移植が守るべき対応関係は「**メニュー XML はアーカイブ内・動画は loose 相当（外部ディレクトリ）**」という**配置**であって、順序ではない。
* 参考: 移植の第 3 段（APK `assets/videos/*` → `cacheDir`）は、原版でいう「`Data\Video\` にファイルが存在する」状態を作る自己修復であり、原版に対応物は無い（原版は最初から loose で存在する前提）。

---

## U4 — タイトル／キャラ作成中に左上へ出す要素

### 結論 **[確定(XML)] 原版に左上要素は無い**

#### タイトル（`menus\options\main_menu.xml`）

| 要素 | x | y |
|---|---|---|
| `backdrop`（`loading_background.dds`・可視 false） | 画面中央 | 画面中央 |
| `oblivion_logo`（`tes_oblivion_logo_final.dds`） | 中央+3 | 画面高/4 + 22.5 |
| 内側 `oblivion_logo`（`tes_oblivion_logo_bink.dds`） | -5（親相対） | -5 |
| `press_start` | 中央 | 725 |
| `button_layout`（ボタン 6 個） | 中央 | 725 |
| **`version`**（バージョン表記） | **10** | **935（画面左下）** |

→ 左上（x≈0, y≈0）に置かれる要素は**存在しない**。唯一の隅要素は `version` の**左下**。

#### キャラ作成（`menus\chargen\race_sex_menu.xml`）

トップレベルは `race_background`（`generic_background.xml` を include・全画面）と、その中の `race_window`（x=30, y=85, 500x558）、`race_button_done`（右下）のみ。**左上専用の要素は無い**。

#### 補足

HUD は別メニュー（`HUDMainMenu`=0x3EC 等）であり、タイトル／キャラ作成の XML には含まれない。したがって「原版がタイトルや chargen 中に左上へ常設要素を出す」根拠は無い。
→ 移植の恒久 HUD 位置（左上）は**独自 UI**。chargen 中に抑制するかは設計判断で、原版準拠なら「出さない」。

---

## 残点

| ID | 内容 | 重要度 |
|---|---|---|
| V-562-1 | `Oblivion - Textures.bsa` の正式ファイル名（本 probe では名前不一致で未走査）と、テクスチャが BSA 内か loose かの最終確認 | 低 |
| V-562-2 | タイトル `version` の文字列キー（`strings()` のどの trait か） | 低 |
| V-562-3 | chargen 中に HUD メニュー（0x3EC）が開かないことの命令的裏付け（メニュースタックの実測） | 中 |
