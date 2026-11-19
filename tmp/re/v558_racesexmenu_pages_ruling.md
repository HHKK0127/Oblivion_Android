# v558 — U2 裁定書: RaceSexMenu(0x40C) のページ構成・枚数・並び順・切替経路（ai-todo #162）

対象: `Oblivion.exe` v1.2.0416 (Steam)。根拠は命令実読（probe v558_1〜v558_6）＋ BSA 実物 `menus\chargen\race_sex_menu.xml`(21451 B)。
判定: `[確定]` / `[確定(命令)]` / `[確定(XML+命令)]` / `[部分確定]` / `[推定]` / `[確定負]`

---

## 1. 結論（依頼の 4 点）

| # | 質問 | 判定 | 結論 |
|---|------|------|------|
| 1 | `user0` ページ index ↔ 意味 | **[確定(XML+命令)]** | `RaceSexMenu.user0` = **「現在表示しているページ（pane）の index」**。XML 宣言 `<user0> 0 </user0> <!-- what pane/page is currently visible -->`（`race_sex_menu.xml:12`）。ページ側は `<visible> me().user0 == RaceSexMenu.user0 </visible>`（同 `:250-253`）で**完全一致したページだけが可視**。 |
| 2 | 枚数と並び順 | **[確定(命令)]** | **21 ページ**（5 タブではない）。`0x5C4340 AddPane` が 21 回呼ばれ、`this+0x40/+0x44/+0x48/…/+0x90` に順に格納される。ページ名（GMST）と順序は §1.1。 |
| 3 | タブ切替の入力経路 | **[部分確定]** | **`<tab>` 要素も「タブ」という概念も存在しない**（[確定負]）。切替はコードが `SetTrait([menu+0x2c], user0=0xFAE, index)` を書く方式で、EXE 全域に 5 つの実装型がある（`(cur±1) & mask` の循環 3 箇所 + `arg+1` 2 箇所）。**RaceSexMenu 自身の呼出元の特定が未完**（残点 V-558-1）。 |
| 4 | 移植の `Name / Race / Attributes / Skills / Appearance` は妥当か | **[確定負]** | **原版に対応する 5 分割は存在しない。** 原版は「Main / Race / Hair / Face / Shape / Tone / 顔パーツ 8 種 / 肌 / 髭 / 眉 / 唇 / 瞳色 / 鼻色 / 髪色」の 21 ページ。`Attributes` / `Skills` / `Name` というページは無い（能力値は StatsMenu、名前は Main ページ内）。 |

### 1.1 21 ページの実名と順序（[確定(命令)]）

`0x9F67FA`〜`0x9F76FA` で各 String オブジェクトが**英語既定値＋GMST 名**で初期化される（`mov ecx, <String オブジェクト>` の直後に 2 つのリテラル: 表示既定値と `s*` GMST 名）。

| # | フィールド | 既定表示名 | GMST | 初期化 VA |
|---|-----------|-----------|------|-----------|
| 0 | `+0x40` | `Main` | `sMain` | 0x9F67FA |
| 1 | `+0x44` | `Race` | `sRace` | 0x9F681A |
| 2 | `+0x48` | `Hair` | `sHair` | 0x9F685A |
| 3 | `+0x4C` | `Face` | `sFace` | 0x9F683A |
| 4 | `+0x50` | `Shape` | `sShape` | 0x9F69BA |
| 5 | `+0x54` | `Tone` | `sTone` | 0x9F69DA |
| 6 | `+0x58` | `General(Face)` | `sGeneralFace` | 0x9F6A3A |
| 7 | `+0x5C` | `Brow` | `sBrow` | 0x9F6A5A |
| 8 | `+0x60` | `Cheeks` | `sCheeks` | 0x9F6A7A |
| 9 | `+0x64` | `Chin` | `sChin` | 0x9F6A9A |
| 10 | `+0x68` | `Forehead` | `sForehead` | 0x9F6ABA |
| 11 | `+0x6C` | `Jaw` | `sJaw` | 0x9F6ADA |
| 12 | `+0x70` | `Mouth` | `sMouth` | 0x9F6AFA |
| 13 | `+0x74` | `Nose` | `sNose` | 0x9F6B1A |
| 14 | `+0x78` | `General(Skin)` | `sGeneralSkin` | 0x9F6B3A |
| 15 | `+0x7C` | `Beard` | `sBeard` | 0x9F6B7A |
| 16 | `+0x80` | `Eye shades` | `sEyesTex` | 0x9F76BA |
| 17 | `+0x84` | `Eyebrows` | `sEyebrows` | 0x9F6B9A |
| 18 | `+0x88` | `Mouth(Lips)` | `sMouthLips` | 0x9F6BBA |
| 19 | `+0x8C` | `Nose shades` | `sNoseTex` | 0x9F76DA |
| 20 | `+0x90` | `Hair Color` | `sColor` | 0x9F76FA |

* フィールド↔名前の対応は `0x5C3110(this, name)`（名前→ページのフィールドを返す連鎖比較）で機械確定（`0x5C3236`〜`0x5C33F0`）。
* 表示文字列は GMST なので **ESM 側で差し替え可能**（日本語化・mod 対応）。
* 注意: `Hair`(+0x48) と `Face`(+0x4C) はフィールド順がこの順。String の初期化順（Face→Hair）とは異なるので**並び順はフィールド順で判断**した。

### 1.2 「タブ」に相当する画面要素

XML に `<tab>` / `<page>` 要素は無い。あるのは:
* `race_name`（id 10）＝**ページタイトルのテキスト**。左右に `stat_title_left.dds` / `stat_title_right.dds` を置き、`race_name_separator` で区切る（＝見た目はタブバー but 実体は単なるタイトル文字）。
* `race_name_marker`（id 1）＝タイトル下のマーカー。
* `race_window`（id 1, 500x558, `clipwindow`）＝ページの親。`xscroll` でスクロールバー（id 20）と連結。
* `race_button_done`（id 90）＝Done/Back 兼用（`<user5> 0 </user5> <!-- 1 (false) for Done and 2 (true) for Back -->`）。

---

## 2. 根拠

### 2.1 ページ生成 `FUN_005C4340` = AddPane（[確定(命令)]）

```
5C4369  eax = arg3                     ; 親（this+0x28 = メニューの tile）
5C4370  push "race_template_pane"
5C437A  call 0x585410                  ; テンプレートから tile を生成
5C4381  eax = arg?  ; cmp eax,-1 → 自動採番 [0xB3B5D4]++ （0xB3B5D4 は ctor で 200 に初期化）
5C43F5  SetTrait(pane, 0xFA8 /*id*/,     float(index))
5C4403  GetTrait(pane, 0xFD0 /*childcount*/) - 1.0(f64 0xA2F928)
5C4421  SetTrait(pane, 0xFAA /*listindex*/, float(childcount-1))
5C4435  SetTrait(pane, 0xFAE /*user0*/,     float(index))   ← ページ番号
5C4444  mov [ebx + edx*4 + 0x94], esi                       ← ページ配列 this+0x94
```
`0x5C3BE0`（vtable `+0x20`）が `[esi + arg*4 + 0x94]` でページを引いてスクロール位置（user6/user7）を更新する＝**`this+0x94` はページ配列**であることの裏付け。

### 2.2 21 回の AddPane 呼出（[確定(命令)]）

すべて head `0x5CA010` 内。呼出順に格納先フィールドが `+0x40, +0x44, +0x48, +0x4C, +0x50, +0x54, +0x58, +0x5C, +0x60, +0x64, +0x68, +0x6C, +0x70, +0x74, +0x78, +0x7C, +0x80, +0x84, +0x88, +0x8C, +0x90` と進む（`mov dword ptr [esi + 0xNN], eax` を各サイトで実読）。

### 2.3 ページ切替の実装型（[部分確定]）

EXE 全域の `SetTrait(…, 0xFAE, …)` 322 箇所のうち、**メニュー根 tile（`[this+0x2C]`）**に書くものは 5 箇所:

| サイト | head | 値の作り方 |
|---|---|---|
| `0x5AA842` | `0x5AA3A0` | `(cur - 1) & mask`（循環・前へ） |
| `0x5AB26A` | `0x5AADC0` | `cur - 1` |
| `0x5B1C0D` | `0x5B1A40` | `(cur - 1) & mask`（循環） |
| `0x5B3653` | `0x5B2B70` | `cur - 1` |
| `0x5C1C23` | `0x5C1B80` | `arg + 1`（`arg < 0` なら書かない） |

→ **原版のページ送りは「index を計算して `user0` に書く」関数呼出であり、キー入力は その関数を呼ぶ側の仕事**。RaceSexMenu 内でこの 5 型のどれを呼ぶかは未特定（残点）。

### 2.4 XML 実物（`menus\chargen\race_sex_menu.xml`, 21451 B）

* `:12` `<user0> 0 </user0> <!-- what pane/page is currently visible -->`
* `:147` `<!-- PAGES LAYOUT -->` セクション。`:148` `race_window`（500x558, clipwindow）
* `:200-205` `<!-- template PANES added here by code, and filled with other templates -->` ＝**ページはコードが動的生成**（XML には 1 個も書かれていない）
* `:247-275` `race_template_pane` / `race_pane`（id 100）:
  * `<visible> me().user0 == RaceSexMenu.user0 </visible>`
  * `<y> 0 - race_scroll_bar.user7 * 93 </y>`（`onlyif me().visible`）
  * `<width> parent.width - (childcount > 6 ? (childcount-1)*40 : 0) </width>`
* `:169-196` `race_scroll_bar`（id 20）: `user2 = RaceSexMenu.user1 - 6`（=項目数の上限）、`user8 = 6`（同時表示数）→ **`RaceSexMenu.user1` = 現在ページの項目数**
* ページ内の要素テンプレート: `race_template_button`(101) / `race_template_toggle`(102) / `race_template_slider`(103) / `race_template_text`(104)

### 2.5 trait ID（v557 の表と一致）

`user0=0xFAE` / `user1=0xFAF` / `user5=0xFB3` / `user7=0xFB7` / `id=0xFA8` / `listindex=0xFAA` / `childcount=0xFD0` / `font=0xFD3` / `wrapwidth=0xFD4`。メニュー型 `RaceSexMenu=0x40C`。

---

## 3. 移植への示唆

1. **移植の 5 タブ（Name / Race / Attributes / Skills / Appearance）は原版の構造ではない。** 原版は 21 ページの**単一ページ送り**で、`Attributes`/`Skills` は StatsMenu（別メニュー 0x3EB）の担当、名前入力は Main ページ内。
2. 原版の画面遷移は「Main（名前・性別など）→ Race → Face/Hair/Shape/Tone → 各パーツ → 髪色」という**1 本のページ列**。移植で 5 タブを維持するなら「原版のページ列をタブに再編した独自 UI」と明記すべき。
3. 各ページの表示名は GMST（`sMain`/`sRace`/…/`sColor`）なので、移植でも**文字列キーをそのまま使える**。
4. ページは `user0` の完全一致で可視制御する方式なので、移植でも「index 一致で 1 ページだけ表示」が原版挙動。

---

## 4. 残点

| ID | 内容 | 重要度 |
|---|---|---|
| V-558-1 | RaceSexMenu 自身のページ送り呼出元（`0x5AA3A0`/`0x5B1A40`/`0x5C1B80` 等のどれを、どの入力（Done ボタン/左右キー/クリック）で呼ぶか） | 中 |
| V-558-2 | AddPane の index 引数の実値（0 始まりか 1 始まりか。`0x5C1B80` が `arg+1` を書くため ±1 のずれが有り得る） | 中 |
| V-558-3 | `Main` ページの内容（名前入力 `TextEditMenu`(0x41B) を開くか、性別トグルか） | 中 |
| V-558-4 | `sGeneralFace`/`sGeneralSkin` など GMST の ESM 既定値（英語既定値と一致するか） | 低 |

---

## 5. probe 一覧（`%TEMP%\opencode\`）

`v558_1`〜`v558_6`（EXIT=0）: XML 構造抽出／`0xFAE` 全参照の文脈／user0 書き手 3 種／AddPane・AddButton 呼出元／ページ名グローバル→リテラル解決（`mov ecx, imm32` + 直後リテラル）／ページ名 String の全出現。
BSA 実物は `bethesda_structs` で `Oblivion - Misc.bsa` から直接抽出（`%TEMP%\opencode\bsa_menus\race_sex_menu.xml`）。
