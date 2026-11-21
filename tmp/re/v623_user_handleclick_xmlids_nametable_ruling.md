# v623 — RaceSexMenu slot 3 = HandleClick の実体・`race_sex_menu.xml` の `<id>` 全抽出・名前表・`0x5CE570` の mask

対象: `Oblivion.exe` v1.2.0416 (md5 `cdd2f0c5eff198d4f26b7b5b54ce4930`)。
根拠: 実物 XML `race_sex_menu.xml`（BSA 抽出）／xOBSE `GameMenus.h`・`GameTiles.h`／既存 Ghidra 逆コンパイル `docs/Oblivion_decompiled.c`／我々の実読 probe **u70_1〜u78_1**（EXIT=0）。
判定: `[確定(実物XML)]` / `[確定(命令)]` / `[確定(命令)（negative）]` / `[部分確定]`

---

## 0. 訂正（査読受理）

- 従来 slot 3 を「Done dispatch」「init」と読んできたが、正しくは
  **`RaceSexMenu::HandleClick(UInt32 buttonID, Tile* clickedTile)`**（xOBSE `GameMenus.h` の Menu slot 3）。
- `docs/RE_EXE_REPORT.md` §29.4 が `FUN_005cc200` を「**[3] init** 1586 行の大規模初期化」と
  記述しているのは**誤り**。init（構築）は `FUN_005ca010`（builder）側であり、`FUN_005cc200` は
  **クリックハンドラ**（buttonID を比較して分岐する）である。→ 文書とコードの不一致（本節で訂正）。

---

## 1. `race_sex_menu.xml` の `<id>` 全抽出 [確定(実物XML)]

実物: `%TEMP%\opencode\bsa_chargen\menus\chargen\race_sex_menu.xml`（22156 bytes）。

| line | `<id>` | tile 名 | 役割 |
|---|---|---|---|
| 21 | `&generic;` | `race_background` (rect) | 背景 |
| 37 | **2** | `race_player_grab` (rect) | モデル操作領域 |
| 49 | **10** | `race_name` (text) | 名前表示 |
| 149 | **1** | `race_window` (rect) | ページ枠 |
| 171 | **20** | `race_scroll_bar` (image) | スクロールバー |
| 213 | **90** | `race_button_done` (image) | Done/Back ボタン |
| 249 | **100** | `race_pane` (rect, template) | ページ枠 |
| 297 | **101** | `race_template_button_layout` (rect) | ボタン項目 |
| 403 | **102** | `race_template_toggle` (rect) | トグル項目 |
| 474 | `&generic;` | `race_template_toggle_left` | 左矢印 |
| 509 | `&generic;` | `race_template_toggle_right` | 右矢印 |
| 564 | **103** | `race_template_slider` (rect) | スライダ項目 |
| 626 | `&generic;` | `race_template_slider_bar` | スライダバー |
| 687 | **104** | `race_template_text` (text) | テキスト項目 |

- `&generic;` は実体 `0xFFFFFFFF`（`FUN_0058a0a0("&generic;",0xffffffff)` @ `0x58A0A0` の登録で確定）。
- **menu 直下の `<user0>` に注記がある**:
  ```
  <user0> 0 </user0>   <!-- what pane/page is currently visible -->
  <user1> 0 </user1>   <!-- how many items are in the currently visible pane -->
  ```
  → **[確定(実物XML)] `RaceSexMenu`（menuRoot）の user0 は「現在表示中のページ番号」**。
  各 `race_pane` の `visible` は `me().user0 == RaceSexMenu.user0` で決まる。
- `race_template_button_layout.user0` = 現在 index、`race_template_toggle.user0` = 現在 index、
  `race_template_slider.user0` = スライダ現在値、`race_template_slider.user6` = 「ID for marker/slider」。

---

## 2. vtable 検証: slot 3 = `0x5CC200` [確定(命令)]

`0xA6D38C`（COL `0xAC2AF4` → `.?AVRaceSexMenu@@`）の 16 slot 実読:

| slot | 値 | xOBSE 名 |
|---|---|---|
| 0 | `0x5C4E20` | Destructor |
| 1 | `0x5C26D0` | SetField |
| 2 | `0x5C2790` | Unk_02 |
| **3** | **`0x5CC200`** | **HandleClick** |
| 4 | `0x60CF60` | HandleMouseover |
| 8 | `0x5C3BE0` | Unk_08 |
| 10 | `0x5C2870` | Unk_0A |
| 11 | `0x5CE570` | Unk_0B（Update 相当） |
| 12 | `0x5C3DD0` | HandleKeyboardInput |
| 13 | `0x5C4E10` | GetID |
| 14 | `0x725DA0` | Unk_0E |

---

## 3. HandleClick の実体 [確定(命令)] / [確定(命令)（negative）]

`0x5CC200..0x5CDA30`（次の head `0x5CDA30` まで、1723 命令、`ret 8` ×2）。

- 引数: `this=ecx`、`arg1=[ebp+8]=buttonID`、`arg2=clickedTile`（**未使用**）。`ret 8`。
- **即値比較は 3 つのみ**: `cmp ebx,0xa`(10) @`0x5CC8A3`／`cmp ebx,0x5a`(90) @`0x5CC277`／`cmp ebx,0x62`(98) @`0x5CCBF4`。
- **jump table は 0 件**（`jmp [reg*4+disp]` なし）。
- → **[確定(命令)（negative）] `0x5CC200` は XML `<id>` の switch ではない。**
  10/90 は XML id と一致（`race_name`=10、`race_button_done`=90）だが、98 は XML に無い（実行時 id）。
- 残りの本体は、逆コンパイル（`Oblivion_decompiled.c` line 333780〜334706）で
  **`if (buttonID == GetFloat(GetTile(要素名, ページ名), 0xFAE)) { ハンドラ }` の巨大チェーン**
  （`0xFAE` = user0）。すなわち **buttonID を「名前で引いた tile の user0」と比較**して分岐する。
  - 例（`0x5CC8BA` 起点）: `GetTile(sRandom, sFace).user0 == buttonID` → `FUN_00579c10(sRandomizeFace)`
    ＝ **Randomize Face ボタン**。
  - `0x62`(98): `(**(code**)(*[0xB333C4]+0x170))(); FUN_00526f00(); FUN_005c91e0();`。

---

## 4. 名前表（`0xB38F70..0xB39500`）[確定(命令)]

登録バンド `0x9D9D6F` 付近で `mov ecx,<global>; push <name>; call 0x419A70` が **1312 回**、
うち `0xB38F70..0xB39500` に **173 件**（`callers_of(0x419A70)=0` は走査範囲の都合＝直接 call は `.text` 外の初期化域にある）。

**ページ名（5）**: `Main`(0xB38F70) / `Race`(0xB38F78) / `Face`(0xB38F80) / `Hair`(0xB38F88) / `Eyes`(0xB38F90)。

**要素名（主要）**:

| global | name | global | name |
|---|---|---|---|
| 0xB38F98 | Age | 0xB39000 | GeneralFace |
| 0xB38FA0 | Complexion | 0xB39008 | Brow |
| 0xB38FA8 | Gender | 0xB39010 | Cheeks |
| 0xB38FB0 | RaceDescription | 0xB39018 | Chin |
| 0xB38FB8 | Style | 0xB39020 | Forehead |
| 0xB38FC0 | Red | 0xB39028 | Jaw |
| 0xB38FC8 | Green | 0xB39030 | Mouth |
| 0xB38FD0 | Blue | 0xB39038 | Nose |
| 0xB38FD8 | Length | 0xB39040 | GeneralSkin |
| 0xB38FE0 | Shape | 0xB39048 | Skin |
| 0xB38FE8 | Tone | 0xB39050 | Beard |
| 0xB38FF0 | Random | 0xB39058 | Eyebrows |
| 0xB38FF8 | Reset | 0xB39060 | MouthLips |
| 0xB39438 | NoNameCharacter | 0xB39068 | Lips |
| 0xB39440 | EnterName | 0xB39320 | EyesTex |
| 0xB39448 | ResetFace | 0xB39328 | NoseTex |
| 0xB39450 | RandomizeFace | 0xB39330 | Color |

- `0xB39070..0xB39318` = 顔詳細スライダ名（Facebrow / BrowRidge / Cheekbones… / Nasolabiallines / Nares 等）。
- `0xB39338..0xB393B0` = `HairColor0..15`、`0xB393B8..0xB39430` = `iHairColor00..15`。
- **【item 6 の prior と整合】** 低い user = ラベル文字列（Age/Red/…）、高い user = 数値、という
  `ClassMenu`（chargen 系）の慣習どおり、ここでも **名前はラベル/ページ識別子**で、数値は user0 系に載る。

---

## 5. `0x5CE570`（slot 11）の mask = 0 [確定(命令)]

`0x5790E0(arg1=menuId, arg2=mask)`（逆コンパイル line 281245 と一致）:
```
if (singleton && singleton+0x1c && menuRoot) {
  if (GetFloat(menuRoot,0xFAE) == 2.0) {
    menu = GetTileMenu(menuId);
    if (menu) {
      if (mask != 0) {
        m = GetContainingMenu();                 // 0x5898F0
        if ((*(uint*)(m+0x24) & mask) == 0) return 0;   // ★ +0x24 のビットテスト
      }
      if (GetFloat(menu,0xFA1) != 0xA2F948) return 1;   // visible
    }
  }
}
return 0;
```
- **`test [menu+0x24], mask` は mask≠0 のときだけ**効く（`+0x24` は xOBSE の `unk24` = 4=作成済み/8=有効 の語）。
- `0x5CE570` は `push 0; push 0x3e9` → **`0x5790E0(0x3E9, 0)`**。**mask = 0**。
  → **slot 11 のこの呼び出しでは `+0x24` のビットテストは行われない**（MessageMenu の存在チェックのみ）。
- 逆に `0x5790E0` の他の呼出は mask を使う（例: `FUN_005790e0(0x3eb, -(x!=0) & 0xb)`）。

---

## 6. ページ遷移 [部分確定]

- `buttonID == 90` の分岐は `0x5CC280` 起点で、**ページ反復**: 各ページ名で `GetPage(name)` →
  `GetFloat(page,0xFAE)` と `GetFloat(menuRoot,0xFAE)` を比較 → 一致したページのハンドラへ
  （`0x5cca77` / `0x5cca09` / `0x5cc9c4` / `0x5cc97f`）。
  ページ候補（`0x5CC280..0x5CC863`）: `Main` / `Shape` / `Tone` / `GeneralFace` / `Brow` / `Cheeks` /
  `Chin` / `Eyes` / `Forehead` / `Jaw` / `Mouth` / `Nose` / `GeneralSkin` / `Beard` / `Eyebrows` /
  `MouthLips` / `EyesTex` / `NoseTex`（18 前後）。
- 各ページハンドラは `GetFloat(page,0xFAE)` を読み、`UpdateFloat(menu->tile, 0xFAE, 値)` で
  **menuRoot.user0 にそのページ番号を書き戻す**（＝「Done/Back で現在ページを確定」）。
- これは §1 の `<user0> = what pane/page is currently visible` と一致する。

---

## 7. 残点

- **V-623-1【中】**: `(要素名, ページ名) -> ハンドラ` の完全表（チェーン 28 件。name1 の抽出は C の
  入れ子のため未完、disasm の「ブロック先頭グローバル」から 18 ページ＋要素を確認済み）。
- **V-623-2【低】**: `0x62`(98) の実行時 id の出所。
- **V-623-3【中】**: ページボタン tile の特定と、その **user0 書き手**（item 5。155 母数で層別）。
- **V-623-4【中】**: 31 名の用途分類（item 6。§4 の名前表で材料は揃った）。
- **V-623-5【低】**: `0x589980`（削除系）5 件と未同定 2 件（item 5 の後）。
- （継続）V-620-2（155 の層別）／V-621-3（読み手 160 の層別）。

---

## 8. 検証

- probe **u70_1**（vtable 16 slot／`0x5CC200` 先頭／`0x5CE570`／`0x5790E0`）。
- probe **u71_1 / u74_1**（`0x5CC200..0x5CDA30` 全走査: 即値比較 3 件・jump table 0・ページ名分岐 52）。
- probe **u72_1**（dispatch 本体／登録関数 `0x419A70`）。
- probe **u77_1**（登録バンド `call 0x419A70` 1312 件 → **173 名の global→name 全表**）。
- probe **u78_1**（アドレス検算: `0x5CC8BA` = `a1 f0 8f b3 00` = `[0xB38FF0]=sRandom`）。
- 実物 XML `race_sex_menu.xml`（`<id>` 14 件・`<user0>` 注記）。
- 逆コンパイル `docs/Oblivion_decompiled.c` line 333780〜334706（`FUN_005cc200`）／281241〜281288（`FUN_005790e0`）。
- **XML 正典一致**: `race_sex_menu.xml` の 3 コピー（`%TEMP%\opencode\bsa_chargen\...`／リポジトリ内
  `tools\BSAFileExtractor\extracted\menus\chargen\...`／worktree `..._tmp_wt\fix-chargen-ui-quality\app\src\main\assets\menus\chargen\...`）は
  **すべて md5 `d367341334fa89101ba4fc077edb52fe`・22156 bytes で完全一致**。→ §1 の `<id>` 抽出は正典ファイル上のもの。
