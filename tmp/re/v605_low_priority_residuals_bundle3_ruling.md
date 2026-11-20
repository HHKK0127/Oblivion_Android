# v605 — 低優先残点クローズ（バンドル3）: `0x66xxxx` 使用量系・`0x5DC8F3` 初期化・Textures BSA 正式名・title `version` 文字列・credits 描画元（ai-todo #96/#156/#157/#166 フォロー）

対象: `Oblivion.exe` v1.2.0416 (Steam, 7,898,624 B)。根拠は命令実読（probe **u18_1〜u18_2**・全 EXIT=0・冒頭 Test-Path）＋実物 XML/BSA。
先行: `tmp/re/v562_u6_u4_assets_topleft_ruling.md`・`v566_u9_left_top_blocks_ruling.md`・`v568_bink_residuals_ruling.md`・`v569_p3_p5_rows_ruling.md`・`v572_skill_grouping_predicate_ruling.md`
判定: `[確定(命令)]` / `[確定(XML)]` / `[部分確定]`

---

## 1. 結論

| 残点 | 判定 | 結果 |
|---|---|---|
| **V-572-1** `0x66xxxx` 系3システム | **[確定(命令)]** | 3 関数はすべて**スキル一覧/使用量表示**系で、`0x5E3320([0xB333C4])`（=player の class）→`0x6ECC80`/`0x51C090`（管輪スキル判定）を使う: **`0x663C50`＝スキルの「上昇難易度/習得速度」計算**（`0x565C50(2,av)`→`0x446AF0(player)`→`0x5F1910`→`0x548030`）／**`0x666BB0`＝スキル画面の文字列レイアウト**（`'PLAYER CHARACTER'`/`'Skill Usage'`）／**`0x668070`＝スキル使用量の差分計算**（`[esi+0x130]` 配列＝スキルポイント、`0x65FAD0` で基準値取得）。→ **統計画面のスキル表示系**。 |
| **V-569-5** `0x5DC8F3-5DC8FC` | **[確定(命令)]** | 3 命令は **`0x5DC6F0` ループの末尾**（`cmp ebp,0x15`／`jl 0x5DC6F0`）にあり、**次イテレーションの `0x5DC6F0` 冒頭で使う構造体（`{ptr,len,cap}` 相当）を初期化**するもの（`[esp+0x1c]=ebx(0)`／`word[esp+0x22]=0`／`word[esp+0x20]=0`）。**上書きされて dead に見えるのは normal**（ループ先頭で再設定）。エラー時は `'Error creating skill item in Stats menu. Template not va…'`（`0xA6DB10`）。 |
| **V-562-1** Textures BSA 正式名 | **[確定(実測)]** | `Data\` 配下の実 BSA 名は **`Oblivion - Textures - Compressed.bsa`**（旧記述の `Oblivion - Textures.bsa` は**存在しない**）。DLC は `DLCShiveringIsles - Textures.bsa`。 |
| **V-562-2** title `version` の文字列キー | **[確定(XML)]** | `title_main_menu.xml` の `<text name="version">`（id 9・x=10・y=935・zoom 75・`<visible><not>&xenon;</not></visible>`）の `<string>` は **固定文字列 `Version`（` Version `）** で、**`strings()` の trait ではない**。→ v562 の「strings() キー」は**不存在**（[確定負]）。移植は "Version" を直書きでよい。 |
| **V-568-3** クレジット描画元 | **[部分確定]** | `credits_menu.xml` の `<image name="credits_rect">`（id 1・clipwindow・width 0・x 640）はテキストスクロール窓のクリップ矩形で、**`.bik` の描画枠ではない**。クレジット動画は v557/v568 のとおり **再生されない**（`CreditsMenu.bik` の `.text` 参照 0 件）ため、動画枠描画元は**存在しない**。 |
| **V-566-2** `getContentPosition`/`getContentSize` の宣言先 | **[確定(移植側)]** | `getContentSize()` は `UIComponent`（`ui_component.h`）宣言で `UIPanel` がオーバーライド。`anchor` 既定値は `UIComponent` の `position`（初期化子なし＝(0,0)）で `getAbsolutePosition()` のオフセットは親チェーン加算のみ。**EXE 側に相当物は無く移植側の話**（v566 の U9 反証と整合）。 |
| **V-566-3** `setSize` 未呼び出しの横断確認 | **[部分確定]** | `UICharacterSheet` は `setPosition/setSize` 呼び出し済み。他 UI（`UICharacterCreation` は `setScreenSize` オーバーライドで呼ぶ＝v566 反証）。**横断確認は移植側の作業**で、EXE 側残点ではない。 |

---

## 2. 根拠（命令逐語）[確定(命令)]

### 2.1 V-572-1 スキル使用量系

```
0x663C50(this, av):
  663C5A lea ecx,[eax-0xC] ; cmp ecx,0x14 ; ja      ; av 0xC..0x20
  663C69 push 2 ; call 0x565C50(2,av)               ; AV 変換
  663C81 call 0x446AF0(player) ; 663C99 call 0x5F1910
  663CAA call 0x5E3320 ; 663CAF mov edx,[eax+0xC] ; cmp edx,[0xB37D00]  ; class 判定
  663CE1 call 0x51C090                               ; 管輪スキル判定
  663CF5 call 0x548030                              ; 係数計算
0x666BB0:  666C02 push 'PLAYER CHARACTER' ; 666C38 push 'Skill Usage'
0x668070:  668089 add [esi+0x660],1 ; 6680C5 fld [esi+edx*4+0x130]  ; スキルポイント配列
           6680DB call 0x65FAD0
```

### 2.2 V-569-5 ループ末尾

```
5DC8E5 add [esp+0x28],4 ; 5DC8EA add ebp,1 ; 5DC8F0 cmp ebp,0x15
5DC8F3 mov dword [esp+0x1c],ebx   ; ★ 次イテレーション用の初期化
5DC8F7 mov word [esp+0x22],bx
5DC8FC mov word [esp+0x20],bx
5DC901 jl 0x5DC6F0                ; ループ先頭へ
5DC909 push 'Error creating skill item in Stats menu. Template not va…'(0xA6DB10)
```

### 2.3 V-562-1/2 実測

```
Data\*.bsa: ...Oblivion - Textures - Compressed.bsa...（'Oblivion - Textures.bsa' は不在）
title_main_menu.xml:
  <text name="version"><id> 9 </id><zoom> 75 </zoom>
    <visible><not> &xenon; </not></visible><x> 10 </x><y> 935 </y>
    <string> Version </string></text>
```
（`main_menu.xml` にも同名 `<text name="version"><id>9</id><string> Version </string>` が存在。）

---

## 3. 残点（更新後）

- **V-568-1【低】**: `BinkOpen` フラグ `0x2000`/`0x08000000` の SDK 名（bink.h 依存）。
- **V-569-5【低】**（解消方向で記載済）。
- **V-572-1【低】**（用途同定済）。
- **V-573-2・V-599-2・V-598-4・V-601-1【低】**: 動的領域/実用価値低（打切り継続）。
- **V-566-2/3【低】**: 移植側の話（EXE 残点ではない）。

---

## 4. 検証

- probe: **u18_1〜u18_2（2本）全 EXIT=0**（冒頭 Test-Path 実施）。
- 主要生バイト: `0x663C50`/`0x666BB0`/`0x668070`（スキル使用量系・`0x5E3320`/`0x51C090`/`0x6ECC80`）／`0x5DC8F3`（ループ末尾初期化）／`0xA6DB10`（エラー文字列）。
- 実測: `Data\*.bsa` 列挙（`Oblivion - Textures - Compressed.bsa`）・`title_main_menu.xml`/`main_menu.xml` の `version` 要素（`<string> Version </string>`）。
