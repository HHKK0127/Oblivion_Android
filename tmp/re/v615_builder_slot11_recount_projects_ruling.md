# v615 — 3 点の実測回答: builder は chargen 専用（0x40C 直値）・slot 11 の実体は `0x5CDEF0`（user0 を書く）・139 は再計数でも 139（偽陽性 0）＋ xOBSE 一次資料による確定と内部整合の指摘

> **【訂正 2026-11-21 (v618)】** 本裁定の「`0x5CDEF0` が user0 を書く」は **v616 で訂正済み**（実際は `0x588BD0 GetFloat` で**読む**）。また §5 の **「153 件の user0 set サイト」は出所不明の誤記**であり、**v617 の実測（`0xFAE` 即値 322／trait 直後 setter 116／getter 22）に置換**する。

対象: `Oblivion.exe` v1.2.0416 (Steam, md5 `cdd2f0c5eff198d4f26b7b5b54ce4930`)。
根拠: 我々の実読 probe **u54_1 / u55_1 / u56_1**（EXIT=0）＋ **xOBSE `GameTiles.h` / `GameMenus.cpp`（一次資料）**。
判定: `[確定(命令)]` / `[確定(一次資料)]` / `[確定(命令)（negative）]` / `[部分確定]`

---

## 1. 結論

| # | 問い | 判定 | 答え |
|---|---|---|---|
| 1 | builder `0x5CA010` は `0x40C`/1036 を参照するか | **[確定(命令)] 参照する＝chargen 専用** | 冒頭 `0x5CA037 push 0x40C` → `call 0x589B70 GetTileMenu(0x40C)` → 既存なら `0x5CC1DD` へジャンプ（構築スキップ）。**menu id を直値で持つ＝汎用ではない**。 |
| 2 | slot 11 の callee・`user*` 書込・`0x5790E0` | **[確定(命令)]** | slot 11 `0x5CE570` は**薄いラッパ**：`0x5790E0(0, 0x3E9)`（MessageMenu 表示判定）→ `visible(0xFA1)` 比較 → `Menu::EnableMenu(0x585190)` → **実体 `0x5CDEF0` を呼ぶ**。**`0x5CDEF0` が user0(0xFAE) を 12 箇所で書く**（ラッパ自身は 0）。 |
| 3 | ジャンプテーブル排除後の 139 | **[確定(命令)] 139 のまま** | **命令境界尊重 = 139／raw 0xE8 全走査 = 139**、差集合 **空**（偽陽性 0）。内訳 Pane21/Button22/Toggle5/Slider90/Text1。 |
| 4 | V-614-3（Value +0x10/+0x14） | **[確定(一次資料)] 閉じた** | xOBSE：`ExpressionList exprList; // 10`＝`Expression* info; ExpressionList* next`。`Expression` = `{prev+00, next+04, operand+08, opcode+0C(u16), unkE+0E, src+10}`・**size 0x18**＝ctor の確保量と一致。→ **式木 dump 仕様**へ格上げ。 |
| 5 | `0x58CEB0` | **[確定(一次資料)] 撤回** | xOBSE `void UpdateFloat(UInt32 valueType, float newValue)`。`0x58CEB0` はその関数ヘッド。査読の callAddr 読みは撤回。 |

---

## 2. (1) builder は chargen 専用 [確定(命令)]

```
0x5CA010  push -1 / push 0x9c1990 / ...（SEH prologue）
0x5CA037  push 0x40C
0x5CA03C  call 0x589B70          ; GetTileMenu(0x40C) = 既存 RaceSexMenu tile menu
0x5CA041  add esp,4
0x5CA044  test eax,eax
0x5CA046  jne 0x5CC1DD           ; 既存なら末尾へ（構築スキップ）
0x5CA04C  push 1 / push 0 / call 0x582160   ; InterfaceManager_GetSingleton
...
```
- **menu id `0x40C` を直値参照**（raw dword `0x0000040C` の出現は範囲内 1 箇所＝`0x5CA038`）。
- 末尾 `0x5CC132 push 0x3E9`（MessageMenu）も builder 内。
- caller は **1 箇所 `0x57B8D9`（head `0x57B7E0`）**。
- → **「139 は汎用 XML 生成側で chargen の意味ではない」という疑いは否定**。builder は `0x40C` を直値で持ち、**chargen 専用**。
- ただし **「XML から tile を生む処理」である点は正しい**（generator 5 種＝XML テンプレート生成器）。

---

## 3. (2) slot 11 の実体と user* の書き手 [確定(命令)]

### 3.1 slot 11 `0x5CE570`（ラッパ）
```
0x5CE572 push 0 / push 0x3E9 / call 0x5790E0   ; ★ MessageMenu(0x3E9) 表示判定
0x5CE583 test al,al / jne 0x5CE5AD             ; Message が表示中なら後段へ
0x5CE587 mov ecx,[esi+4] / push 0xFA1 / call 0x588BD0   ; visible を読む
0x5CE594 fcomp [0xA2F948] / ... / jp 0x5CE5AD
0x5CE5A1 push 0 / mov ecx,esi / call 0x585190  ; Menu::EnableMenu(esi, 0)
...
0x5CE613 mov ecx,esi / call 0x5CDEF0           ; ★ 実体（本体）を呼ぶ
```
- **callees**: `0x5790E0`／`0x588BD0`(GetFloat)／`0x585190`(Menu::EnableMenu)／**`0x5CDEF0`（本体）**／`0x5FCAB0`／`0x476D10`／`0x603CA0`。
- **`0x5790E0` の同定**：`InterfaceManager_GetSingleton` を 3 回、`[mgr+0x1C]!=0`・`[mgr+0x68]`(menuRoot)!=0 を確認 → `GetFloat(menuRoot, 0xFAE)` を **`0xA379B4`(=2.0)** と比較 → 一致時 `GetTileMenu(arg)` → `GetContainingMenu` → `test [menu+0x24], arg0`。＝**「menuRoot の user0==2.0 かつ対象メニューが存在」を返すメニュー表示判定**。`IsMenuMode(0x578F60)` の近傍の同族。
- **`0x3E9`(MessageMenu) の直値は chargen 固有の挙動**（chargen 中はメッセージ表示を抑止）→ 裁定に残す。

### 3.2 実体 `0x5CDEF0` が user0 を書く
- `0x5CDEF0..0x5CE561` に **`push 0xFAE`(user0) が 12 箇所**（0x5CDF29/0x5CDF92/0x5CDFF3/0x5CE0F5/0x5CE170/0x5CE1E3/0x5CE24B/0x5CE2C4/0x5CE345/0x5CE3A6/0x5CE45C/0x5CE4D5）。
- 各サイトは **ページ名グローバル**と共に現れる（登録バンドから実名を取得）:
  `0xB38F70`=**sMain**／`0xB38F78`=sRace／`0xB38F88`=sHair／`0xB38F90`=sEyes／`0xB38F98`=sAge／`0xB38FA0`=sComplexion／`0xB38FA8`=sGender／`0xB38FB8`=sStyle／`0xB38FC0`=sRed／`0xB38FC8`=sGreen／`0xB38FD0`=sBlue／`0xB38FD8`=sLength／`0xB39330`=sColor。
- → **user0(0xFAE) は「現在ページ名」を保持**（13 ページ名＝sMain/sRace/sHair/sEyes/sAge/sComplexion/sGender/sStyle/sRed/sGreen/sBlue/sLength/sColor）。

### 3.3 builder も user* を書く
- builder `0x5CA010` 内に **13 箇所**: user0(0xFAE)×4・user1(0xFAF)×1・user6(0xFB4)×2・user13(0xFBB)×6。うち user0/user1 は `0x58CEB0`(UpdateFloat) を伴う（数値）、他は `0x419B10`(文字列代入系) を伴う。
- → **user* の書き手は (a) builder（初期化）と (b) 実体 `0x5CDEF0`（毎フレームのページ名更新）の 2 系統**。

### 3.4 ⚠ 内部不整合の指摘（要解消）
- **v610** は `0x5CC816` を「`GetFloat(Main, 0xFAE)` → `SetFloat(menuRoot, 0xFAE, …)`」＝**user0 を float** として裁定した。
- **v615（本件）** は `0x5CDEF0` が user0 を**ページ名文字列**として書く（13 ページ名グローバル）ことを実測。
- → **同一 trait id 0xFAE の型（string=ページ名 か float=ページ index か）と対象 tile（menuRoot か ページ tile か）が未整合**。**V-615-1** で解消する（`Value+0x1A`(isNumber) を見れば決着する）。

---

## 4. (3) 139 の再計数 [確定(命令)]

| 方式 | 件数 | 内訳 |
|---|---|---|
| **命令境界尊重**（capstone で 1 命令ずつ decode し `call <generator>` のみ） | **139** | Pane21/Button22/Toggle5/Slider90/Text1 |
| raw `0xE8` 全走査（旧） | **139** | 同上 |
| 差集合 | **空** | 偽陽性 0（埋込ジャンプテーブル等による誤検出なし） |

→ **139 は確定**。`Slider=90` も実在（偽陽性ではない）。レンジは builder の関数 extent **`[0x5CA010, 0x5CC200)`**。

---

## 5. xOBSE 一次資料による確定 [確定(一次資料)]

### 5.1 Tile / Value レイアウト（我々の実読と全項目一致）
```cpp
class Tile {            // size 0x40
  UInt8  unk04;         // 04
  BSStringT name;       // 08  ← 一致
  Tile*  parent;        // 10  ← 一致
  ValueList valueList;  // 14  ← 一致（lea ecx,[this+0x14]）
  UInt32 unk24;         // 24  ← 一致（render node）
  UInt32 flags;         // 2C
  RefList childList;    // 30  ← +0x30 に vtable、先頭ノード +0x34
};
struct Value {          // size 0x1C
  Tile*  parentTile;    // 00  ← 一致
  float  num;           // 04  ← 一致
  BSStringT str;        // 08  ← 一致（len 0C / cap 0E）
  ExpressionList exprList; // 10  ← V-614-3（+0x10/+0x14）
  UInt16 id;            // 18  ← 一致
  UInt8  bIsNum;        // 1A  ← 一致
  UInt8  pad1B;         // 1B
};
```
- **V-614-3 は閉じた**：`+0x10` = 式リスト先頭、`+0x14` = 末尾/番兵。`Expression`（size 0x18）の `opcode`(u16, +0x0C) は xOBSE 表（`0x7D1 copy`／`0x7D2 add` … `0x7EB ref`）で名前化できる。
- **式木 dump 仕様に格上げ**：`copy src=` の飛び先を辿れば chargen XML の依存関係が取れる（V-614-3 を「低」から格上げ）。

### 5.2 trait ID 全表（xOBSE `eTileValue`）
- **bool は `1=false, 2=true`**（重要）。
- 属性域 `0xFA1..0x1001`：`visible 0xFA1`／`class 0xFA2`／`listclip 0xFA3`／`clipwindow 0xFA4`／`stackingtype 0xFA5`／`locus 0xFA6`／`alpha 0xFA7`／`id 0xFA8`／`listindex 0xFAA`／`depth 0xFAB`／`y 0xFAC`／`x 0xFAD`／**`user0 0xFAE` … `user25 0xFC7`**／`clips 0xFC8`／`target 0xFC9`／`height 0xFCA`／`width 0xFCB`／`red/green/blue 0xFCC..0xFCE`／`childcount 0xFD0`／`justify 0xFD1`／`zoom 0xFD2`／`font 0xFD3`／`wrapwidth 0xFD4`／`string 0xFDE`／`clicked 0xFE1`／`clicksound 0xFE5`／`filename 0xFE6`／`repeatvertical 0xFE9`／`repeathorizontal 0xFEA`／`xdefault 0xFF0`／`xup 0xFF1` … `xbuttonstart 0x1001`。
- 他域：要素型 `0x385 rect`/`0x386 image`/`0x387 text`/`0x389 menu`/`0x38B window`/`0x3E7 template`／演算子 `0x7D1..0x7EB`／値型 `0xBB9..0xBBC`／参照 `0x1389 parent`/`0x138A me`/`0x138C sibling`/`0x138D child`/`0x138E screen`/`0x138F strings`。
- **第二の源**：我々の **v548（row524）が同表を 223 対で確定済み**（`FUN_0058A1C0` が `FUN_0058A0A0(name,id)` を 223 回呼ぶ）。xOBSE 表は独立の第二源。

---

## 6. ⚠ 我々の過去裁定との不整合（要訂正）

| 場所 | 記述 | 正 |
|---|---|---|
| **row518（V542）** | `0xB13970` を「**ブック系マネージャ** singleton」 | **`g_TileMenuArray`**（xOBSE `GameMenus.cpp:8`・OBVR `kTileMenuArray`・全メニューを menu id で引く配列）。V542 は book レーンで見つけたため名称が偏った。**機構（`42BB90` で登録）は正しい**。 |
| **row518（V542）** | `42BB90` の `word[ecx+0xa]`＝「**count**」、`word[ecx+0xc]`＝「cap 系」 | **`+0x0A`=firstFreeEntry／`+0x0C`=numObjs**（v613 で訂正済み）。V542 の命名は v613 の訂正対象と同じ誤り。 |
| **v610** | user0(0xFAE) を **float**（GetFloat/SetFloat） | v615 では **ページ名文字列**として書かれる（0x5CDEF0）。**V-615-1** で型を確定。 |

- row518 の本文（`v542_book_tiles_ruling.md`）は残すが、**名称の訂正を v615 で記録**（原本改変はしない）。

---

## 7. 他に参考になりそうなプロジェクト

| プロジェクト | 内容 | 我々への価値 |
|---|---|---|
| **llde/xOBSE** | OBSE 本体（`GameTiles.h`/`GameMenus.cpp`/`GameAPI.h` 等）。**一次資料** | **最重要**。レイアウト・trait 表・アドレスの一次ソース（本件で実証） |
| **shadeMe/Blockhead** | OBSE プラグイン。性別別ヘッドメッシュ＋**「race/sex menu でフリーカメラ」** | **chargen (RaceSexMenu) 直撃**。ソース公開 |
| **DavidJCobb/NorthernUI** | Oblivion の UI mod（`ReverseEngineered/` フォルダに RE 知見） | **UI/tile/メニュー XML 直撃**。ソース公開 |
| **Ez0n3/OBSE-Plugins** / **Ez0n3/xOBSE-Plugins** | OBSE プラグインの実例 | プラグイン作法 |
| **ianpatt/obse64** / **libxse/commonlibob64** | **Oblivion Remastered**（UE5・別バイナリ）の再構築ライブラリ | **別ゲーム**。2006 版とは別物（混同注意） |
| **cs.uesp.net**（MenuQue カテゴリ等） | Construction Set Wiki の UI/tile ドキュメント | XML 属性・タイル仕様の二次資料 |
| **MenuQue**（shadeMe・Nexus 32200） | UI 関数追加（`InsertXML` 等） | XML 操作の実例（ソースは要確認） |

- 既出: Naragorn/OBNG・OBVR（GPL）、madpai/game-decomp-compendium（MIT）。

---

## 8. 残点

- **V-615-1【高】**: `user0(0xFAE)` の**型**（string=ページ名 or float=index）と**対象 tile**。v610（float）と v615（string）の整合。`Value+0x1A`(isNumber) で決着。
- **V-615-2【高】**: `0x5CDEF0`（slot 11 実体）の完全 decode＝ページ名→user0 書込の 12 サイトと、`0x5CE570` ラッパの役割分担。
- **V-615-3【中】**: `0x5790E0` の完全同定（`0xA379B4`=2.0 比較の意味＝menuRoot user0==2.0）。
- **V-615-4【低】**: `Expression`（size 0x18）の式木 dump 仕様の実装。
- **V-615-5【低】**: row518（V542）の名称訂正（`g_TileMenuArray`）を docs/GT 反映時に併せて実施。
- （継続）V-614-2（builder caller `0x57B7E0`）。

---

## 9. 検証

- probe **u54_1 / u55_1 / u56_1**（EXIT=0・冒頭 Test-Path）。
- (1) `0x5CA037 push 0x40C`＋`call 0x589B70`／raw dword `0x0000040C` 出現 1 件。
- (2) slot 11 callees（`0x5790E0`/`0x588BD0`/`0x585190`/`0x5CDEF0`/…）／`0x5CDEF0` の user0 書込 12 箇所／builder の user* 13 箇所／`0x5790E0` 本体（`0xA379B4`=2.0）。
- (3) 命令境界 139 ＝ raw 139、差集合 空。
- xOBSE `GameTiles.h`（Tile/Value/Expression/`eTileValue` 全表）・`GameMenus.cpp`（`g_TileMenuArray=0x00B13970`）。
- ページ名グローバル実名: 登録バンド `0x9F6000..0x9F8000` から 13 件（sMain/sRace/…/sColor）。
- probe `u47_1.py` を xOBSE 全表（user0..user25）に更新。
