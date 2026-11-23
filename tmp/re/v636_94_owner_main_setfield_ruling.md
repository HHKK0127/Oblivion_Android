# v636 — `+0x94` の所属・Main 特別扱いの否定・`<id>`→menu 束縛層の確定

- **日付**: 2026-11-22
- **帯域**: RaceSexMenu / RTTI / XML interpreter（trait `0xFA8`）
- **判定語彙**: `[確定(命令)]` / `[確定(命令)(negative)]` / `[確定(データ)]` / `[確定(XML)]` / `[部分確定]` / `[未確定]`
- **読み取り宣言**: capstone probe（`D:\re\v307_common.py`, `Oblivion.exe` Steam版）と `docs/Oblivion_decompiled.c`。XML は `D:\re\race_sex_menu.xml`。引用は tool 表示のみ。範囲は半開区間。
- **起点**: v635（row612）の続き。ユーザー指示の順序 **1 → 2 → 3 → 4 → 5**。

## §0 総括表

| # | 命題 | 判定 |
|---|---|---|
| 1 | `+0x94` は RaceSexMenu の id→tile 表（変数索引でのみ読まれる） | `[確定(命令)]` |
| 2 | RaceSexMenu に「索引 0 の特別扱い（Main）」は**無い** | `[確定(命令)(negative)]` |
| 3 | `<id>` trait(0xFA8) ハンドラが `menu->SetField(id, tile)` を呼ぶ | `[確定(命令)]` |
| 4 | idx 21 → `+0x38` は scroll 系（`<include>` 由来）で、builder guard 対象外 | `[部分確定]` |
| 5 | 6 スロットの使途を確定 | `[確定(命令)]` |

---

## §1 — `+0x94` の宣言クラスは RaceSexMenu（Persuasion の直読みは切り離す）`[確定(命令)]`

`0x5B0000..0x5D2000` の `[+0x94]` 参照（`u134_2`）:

- **RaceSexMenu 系（すべて変数索引 `[this + idx*4 + 0x94]`）**:
  - vtable slot2 `0x5C2790` @ `0x5C27A8 cmp dword [esi+ebx*4+0x94],0`
  - vtable slot8 `0x5C3BE0` @ `0x5C3BF7 mov ebx,[esi+eax*4+0x94]`
  - 非仮想 `0x5C6390` @ 0x5C64E0/F8/655C/6638/67B9, `0x5C6860` @ 0x5C698B/69A3, `0x5C6AF0` @ 0x5C6C59/6C8A/6CA2/6D57/6D6F
  - builder generator store: `0x5C4340` @0x5C4444, `0x5C4480` @0x5C45EE, `0x5C4630` @0x5C47C9, `0x5C4800` @0x5C48E1, `0x5C93F0` @0x5C9584/958B
  - Init `0x5C4E40` @ `0x5C4EA7 lea edi,[esi+0x94]`（500 dwords クリア）
- **リテラル索引 0 の `[reg+0x94]` 読み（RaceSexMenu 外）**:
  - `0x5C0440`（**PersuasionMenu** slot3）@ `0x5C044E mov ecx,[esi+0x94]`
  - `0x5BF170`（0x5BExxx 帯・RaceSexMenu 帯外）@ 0x5BF3DE/0x5BF433
- **RaceSexMenu の RTTI 基底鎖**（`u134_1`）: `numBaseClasses=2` = {`.?AVRaceSexMenu@@`(mdisp 0), `.?AVMenu@@`(mdisp 0)}。中間基底なし。

→ **`+0x94` は RaceSexMenu の id→tile 表** `[確定(命令)]`（RaceSexMenu 系関数が一様に変数索引で読む）。
→ **PersuasionMenu の `[+0x94]` 直読み（`0x5C044E`）は Persuasion 自身の別フィールド**であり、chargen の根拠から**切り離す**（`[確定(命令)(negative)]`）。
→ 宣言位置（基底 `Menu` か RaceSexMenu か）は `[部分確定]`。基底は `Menu` のみで、PersuasionMenu も同オフセットの表を持つため共通基底フィールドの可能性が高いが、宣言そのものは未観測。**ポートには影響しない**（RaceSexMenu の表として使う）。

## §2 — Main の特別扱いは無い `[確定(命令)(negative)]`

- RaceSexMenu 配下の関数で `[+0x94]` を**リテラル索引 0** で読む箇所は **0 件**（§1 のとおり全件 `+ idx*4`）。
- Main は id 0 の pane にすぎず、`+0x94[0]` に構造的に入るだけ。
- v634 §9 の「`+0x94[0]=Main` は `FUN_005c0440` のゲート」は、当該関数が PersuasionMenu と判明した時点で**根拠として無効**。→ Main 特別扱いの主張は**撤回**（`[部分確定]`→`[確定(命令)(negative)]`）。
- **ポート: `main` 別フィールドは不要。`panes[0]` が Main。**

## §3 — `<id>`(0xFA8) ハンドラが `menu->SetField(id, tile)` を呼ぶ `[確定(命令)]`

trait setter `FUN_0058b2f0`（`esi`=trait id, `edi`=tile, 値はスタック）:

```
0x58B4E1 cmp esi, 0xfa8              ; trait id == 0xFA8 (id)
0x58B4E7 jne 0x58b50a
0x58B4E9 mov ecx, edi
0x58B4EB call 0x5898f0               ; Tile::GetContainingMenu(tile) → esi = menu
0x58B4F0 mov esi, eax
0x58B4F2 test esi, esi ; je 0x58b50a
0x58B4F6 fld dword ptr [esp+0x14]    ; id 値 (float)
0x58B4FA mov ebx, dword ptr [esi]    ; menu vtable
0x58B4FC push edi                    ; arg2 = tile
0x58B4FD call 0x9828c0               ; float→int → eax = int(id)
0x58B502 push eax                    ; arg1 = int(id)
0x58B503 mov eax, dword ptr [ebx+4]  ; menu vtable slot 1 = SetField
0x58B506 mov ecx, esi
0x58B508 call eax                    ; menu->SetField(int(id), tile)
```

- 呼び出し形は `mov eax,[vtable+4]; ...; call eax`。**`call [reg+4]` 形ではない**ため、menu 帯の `call dword [reg+4]` 掃引が 0 件だった理由が説明できる。
- 帰結: **tile T に `<id>N</id>` が付くと `containingMenu->SetField(N, T)`**。→ `race_window`（`<id>1</id>`）→ `menu+0x28 = race_window`。**束縛層が確定**（V-635-1 解決）。
- `MENU_XML_INTERPRETER_DESIGN.md` への追記内容: **interpreter は tile の `id` trait(0xFA8) をセットした時点で、所属 menu を解決し `menu->SetField(id値, tile)` を呼ぶ**。menu クラスが id→フィールドオフセットを定義する（RaceSexMenu: `1→0x28 / 2→0x2c / 10→0x30 / 20→0x34 / 21→0x38 / 90→0x3c`）。

## §4 — idx 21（`+0x38`）`[部分確定]`

- `race_sex_menu.xml` に `<id>21</id>` は**無い**（`<id>` は 1,2,10,20,90,100-104 + `&generic;`×4）。
- `&generic;` の 4 件は **`<template>` 本体**（race_template_toggle_left/right 等）内で、ビルド時に実体化されない（§3.6）。→ 束縛に寄与しない。
- SetField は §3 の id ハンドラからのみ呼ばれる → **`SetField(21,…)` の直呼びは存在しない**。
- slot2 `0x5C2790` / slot8 `0x5C3BE0` は `+0x34`(race_scroll_bar) と `+0x38` を**同時に**読み、`SetFloat(user9=0xFB7)` / `SetFloat(user8=0xFB6)` を書く（§5）。→ `+0x38` はスクロールバー群に属する。
- `race_scroll_bar`（`<id>20</id>`）は `<include src="vertical_scroll.xml" />` を持つ。→ **idx 21 は vertical_scroll.xml 由来の scroll 子タイル**が最有力（当該 include はローカル未抽出のため `[部分確定]`）。
- builder guard は `+0x28/+0x2c/+0x30/+0x3c` のみ要求し、`+0x38` は**対象外**（optional）。

## §5 — 6 スロットの使途 `[確定(命令)]`

| offset | idx | XML tile | 読み手と使途 |
|---|---|---|---|
| `+0x28` | 1 | race_window | builder が全21 pane の親として読む（各 generator 直前に `mov eax,[esi+0x28]`） |
| `+0x2c` | 2 | race_player_grab | builder は **guard のみ**（`0x5CA139 cmp [esi+0x2c],0`）。実使用は 3D グラブ/回転系（別経路） |
| `+0x30` | 10 | race_name | builder 末尾 `0x5CBC67/0x5CBC7A` が `SetString(race_name, 0xFDE=string)` と `SetFloat(race_name,0xFF0)` → **名前テキスト設定**。HandleClick も 5 箇所で読む |
| `+0x34` | 20 | race_scroll_bar | slot2 `0x5C2790` と slot8 `0x5C3BE0` が読み `SetFloat(user9=0xFB7)` → スクロールバー更新 |
| `+0x38` | 21 | (idx21, §4) | slot8 `0x5C3BE0` が読み `SetFloat(user8=0xFB6 / user9=0xFB7)` → スクロール系 |
| `+0x3c` | 90 | race_button_done | HandleClick `0x5CC816` が `SetFloat(done, 0xFB3=user5, 1.0)` → **Done 押下フラグ**。builder は guard のみ |

- builder が `+0x2c/+0x30/+0x3c` を読む箇所: `+0x2c`/`+0x3c` は **guard のみ**、`+0x30` は guard に加えて `0x5CBC67/0x5CBC7A`（名前設定）。

## §6 — レイアウト裏付け

- ctor `FUN_005c4e40` が 0 クリアするのは **`+0x28/+0x2c/+0x30/+0x3c`（idx 1,2,10,90）の 4 つ**（`0x5C4E99/E9C/E9F/EA2`）。`+0x34/+0x38`（idx 20,21）はクリアしない → **builder guard が要求する 4 つと一致**（束縛フィールド集合の裏付け）。
- オブジェクトサイズ **0x9B0**: ctor/dtor が `_eh_vector_*_iterator_(param_1+0x24c, 8, 0x10, …)` を持つ → `+0x930` に 8×0x10=0x80 の配列 → `0x930+0x80 = 0x9B0` がサイズと一致。pane 表は `+0x94..+0x8C4`（500 dwords）。

## §7 — 残点

| ID | 内容 | 状態 |
|---|---|---|
| V-636-1 | `&generic;` の実値（strings.xml 未抽出） | `[未確定]` |
| V-636-2 | idx 21 の実タイル（vertical_scroll.xml 未抽出） | `[部分確定]` |
| V-636-3 | `+0x94` の宣言位置（基底 Menu か RaceSexMenu か） | `[部分確定]` |
| V-636-4 | `+0x2c`（race_player_grab）の実使用経路（3D グラブ） | `[未確定]` |
