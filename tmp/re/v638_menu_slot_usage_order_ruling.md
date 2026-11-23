# v638 — `+0x94` の所属・Main 特別扱い否定・`<id>`→SetField dispatch・6 スロット使途・entity 表・初期化順

- **日付**: 2026-11-22
- **帯域**: RaceSexMenu / RTTI / XML interpreter（trait `0xFA8`）/ entity 表 / 初期化順
- **判定語彙**: `[確定(命令)]` / `[確定(命令)(negative)]` / `[確定(データ)]` / `[確定(XML)]` / `[部分確定]` / `[未確定]`
- **読み取り宣言**: capstone probe（`D:\re\v307_common.py`）。XML は `D:\re\race_sex_menu.xml` と `tools/BSAFileExtractor/extracted/menus/`。引用は tool 表示のみ。
- **起点**: v637（row614）の残点 + ユーザー指示（1→2→3→4→5→6→7→9→10）。

## §1 — `+0x94` の宣言クラス `[確定(命令)]`

| 関数 | vtable | caller（head） | 判定 |
|---|---|---|---|
| `0x5C3BE0` | **RaceSexMenu slot 8**（vtable `0xA6D38C`） | — | RaceSexMenu |
| `0x5C6390` | 非仮想 | `0x5C9AB0`（RaceSexMenu 帯） | RaceSexMenu |
| `0x5C6860` | 非仮想 | `0x5C93F0`（slider generator, RaceSexMenu） | RaceSexMenu |
| `0x5C6AF0` | 非仮想 | `0x5C9650`, `0x5C9AB0` | RaceSexMenu |
| `0x5C4340` | 非仮想 | `0x5CA010`（builder）×21 | RaceSexMenu |
| `0x5BF170` | 非仮想 | `0x5BF470`, `0x5BF7D0`, `0x5BFB90` | **PersuasionMenu** |

- RTTI 階層: `Menu`(td `0xB0A018`) → 基底なし（numBase=1 で自身のみ）。`RaceSexMenu`(td `0xB13348`) → {RaceSexMenu, Menu}。`PersuasionMenu`(td `0xB13384`) → {PersuasionMenu, Menu}。
- → **共通基底は `Menu` のみ**。`+0x94` を触るのは RaceSexMenu 系と Persuasion 系のサブクラスのみで、**基底メソッドは `+0x94` を触らない**。
- → **`+0x94` は RaceSexMenu の id→tile 表で確定**。**PersuasionMenu の literal-0 読み（`0x5C044E` / `0x5BF170`）は Persuasion 自身の別フィールド**として切り離す。
- 宣言位置（基底 `Menu` か RaceSexMenu か）は `[部分確定]`（基底メソッド未参照・RaceSexMenu ctor が 500 dword をクリアするため RaceSexMenu 側の可能性が高い）。ポートには影響しない。

## §2 — Main の特別扱いは無い `[確定(命令)(negative)]`

- RaceSexMenu 系の `[+0x94]` 参照は**すべて `[this + idx*4 + 0x94]`（変数索引）**。**リテラル索引 0 は 0 件**。
- → Main は id 0 の pane にすぎず `panes[0]`。**ポートの `main` 別フィールドは不要**（設計を 1 つ削減）。

## §3 — trait `0xFA8` の dispatch `[確定(命令)]`

trait 適用関数 `FUN_0058b2f0`（`ret 0xc`; args = (tile=`ecx`, traitId=word, value)）:

```
0x58B4E1 cmp esi, 0xfa8                 ; esi = traitId
0x58B4E7 jne 0x58b50a
0x58B4E9 mov ecx, edi ; call 0x5898f0   ; Tile::GetContainingMenu(tile) → esi = menu
0x58B4F6 fld [esp+0x14] ; push edi ; call 0x9828c0   ; eax = int(id値)
0x58B503 mov eax, [ebx+4]               ; menu vtable slot 1
0x58B508 call eax                       ; menu->SetField(int(id), tile)
```

- caller（interpreter の trait 適用）: `FUN_0058bEE0`(site `0x58C936`) / `0x591600`(`0x591A17`) / `0x591DC0`(`0x59220D`,`0x59222B`) / `0x592A70`(`0x592E3F`)。→ **interpreter が trait を適用する経路から `SetField` が呼ばれる**。
- 呼び出し形は `mov eax,[reg+4]; …; call eax`。**`call dword [reg+4]` 形は存在しない**（menu 帯の掃引が 0 件だった理由）。
- 式 id（`<copy src="parent()" trait="user6"/>`）: 値は既に評価済みの float がスタック `[esp+0x14]` にあり、`0x9828c0`（float→int）で idx 化。→ **式 id も静的 id も同一経路**。
- 帰結: `<id>N</id>` ⇒ `containingMenu->SetField(N, tile)`。`<id>1</id>`(race_window) ⇒ `menu+0x28`。

## §4 — 6 スロットの実使用 `[確定(命令)]`

| offset | idx | XML tile | builder の読み | 使途 |
|---|---|---|---|---|
| `+0x28` | 1 | race_window | 全 21 pane の親 | 親タイル |
| `+0x2c` | 2 | race_player_grab | **guard のみ**（`0x5CA139`） | 存在チェックのみ（3D グラブは別経路） |
| `+0x30` | 10 | race_name | guard ＋ 末尾 `0x5CBC67/0x5CBC7A` | `SetString(race_name, 0xFDE=string, [0xb39440])` ＋ `SetFloat(race_name, 0xFF0=xdefault, 2.0)` → **テキスト設定** |
| `+0x34` | 20 | race_scroll_bar | — | slot2/slot8 が user9 更新 |
| `+0x38` | 21 | vertical_scroll_marker | — | slot8 が user8/user9 更新 |
| `+0x3c` | 90 | race_button_done | **guard のみ**（`0x5CA145`） | 押下は HandleClick `0x5CC816` |

- `+0x30` は `0x58CED0`（**SetString**）であり `0x58A020`(SetName) ではない → **text 設定**。

## §5 — entity 表（`0x58A1C0`〜末尾, 13 件）`[確定(命令)]`

| entity | 値 | entity | 値 |
|---|---|---|---|
| `&generic;` | -1 | `&click_past;` | 0x65 |
| `&false;` | 1 | `&no_click_past;` | 0x66 |
| `&true;` | 2 | `&does_not_stack;` | 0x1778 |
| `&left;` | 1 | `&mixed_menu;` | 0x67 |
| `&center;` | 2 | `&xenon;` | 1 |
| `&right;` | 4 | `&xbox;` | 1 |
| `&scale;` | -1 | | |

- **`&false;`=1 / `&true;`=2 が bool 規約（1=false, 2=true）の出所**。race_sex_menu.xml L229 のコメントと一致。`&no_click_past;`=102 は v626 の depth ゲート（`0xA69778`=102.0）と一致。
- `&left;/&center;/&right;` = 1/2/4（justify enum）。`&generic;`=-1 / `&scale;`=-1。

## §6 — `id = -1` の副作用 `[確定(命令)]`

- 0xFA8 分岐は **id trait を書かない**（`SetField` のみ）。trait 値の格納は trait 適用経路（`FUN_0058b2f0` 呼び出し）側。
- `SetField(-1, tile)` は分岐が無く **no-op** → **`&generic;`（id -1）の tile は `+0x94` に登録されない**。
- → spec: **「id -1 は束縛（SetField）も索引（`+0x94[id]`）もされない」**。XML 専用 tile（background / toggle_left / toggle_right / slider_bar 等）は id -1 でも実害なし。
- `+0x94` を id で引く経路に -1 が渡らないことは `SetField` が -1 を登録しないことから従う。

## §7 — 初期化順（`0x57B7E0..0x57B8DF`）`[確定(命令)]`

```
0x57B7E4 call 0x582160(1,0)          ; player/character 取得
0x57B800 cmp [eax+0x1c],0 ; je 0x57b8df   ; 失敗→return
0x57B816 cmp [eax+0x68],0 ; je 0x57b8df
0x57B836 GetFloat(0xfae=user0) ; fcomp [0xa379b4=2.0] ; jp 0x57b8df   ; user0!=2.0 → return
... (0x6a2190/0x6a2280/0x678d90/0x67aca0 設定)
0x57B892 fldz ; push 0 ; push 0xb13348(RaceSexMenu RTTI) ; push 0xb0a018(Menu RTTI) ; push 0 ; push 0x40c
0x57B8B1 call 0x589b70                ; menu 生成（ctor）
0x57B8B9 mov ecx,eax ; call 0x5898f0  ; menu 取得
0x57B8C0 push eax ; call 0x9832e6     ; 補助
0x57B8C9 test eax,eax ; je 0x57b8d7
0x57B8CD mov edx,[eax] ; mov ecx,eax ; mov eax,[edx] ; push 1 ; call eax   ; vtable[0](1)
0x57B8D7 push 1
0x57B8D9 call 0x5ca010                ; ← builder
0x57B8DF ret
```

- builder 自身が冒頭で `FUN_00590420()`(ReadXML) を呼び、tile 木構築＋`SetField` 束縛を行う。→ guard は ReadXML の後で成立。
- → **ポート初期化順 = ①menu 生成 → ②ReadXML（parse + tile 木 + `<id>` 束縛）→ ③builder（pane/item 生成）**。
- 失敗分岐: player 状態チェックで `je 0x57b8df`（return）。`FUN_009832e6` が null なら vtable 呼びを skip して builder へ。builder の guard 失敗は警告→return。

## §8 — `+0x38` は guard 対象外 `[確定(命令)]`

- guard は `+0x28/+0x2c/+0x30/+0x3c` の 4 本のみ。`+0x38`（vertical_scroll_marker）は含まれない。
- → spec: **「marker 未束縛でも build は成立。スクロールバーのドラッグのみ不能」**。

## §9 — `0xFF0` の同定 `[確定(データ)]`

- `FUN_0058a0a0("xdefault", 0xff0)` → **`0xFF0` = `xdefault`**（既定ボタン trait）。
- `[0xa379b4]` = **2.0**。→ `SetFloat(race_name, xdefault, 2.0)` = race_name の **xdefault を true** に。

## §10 — `user5`(0xFB3) の読み手 `[確定(命令)]`

- RaceSexMenu 帯（`0x5Cxxxx`）の `push 0xfb3` は**すべて `SetFloat`（書込）**:
  `0x5C4787`(toggle gen, 0) / `0x5C7FCD`,`0x5C8394`,`0x5C8826`,`0x5C89DB`,`0x5C8E94`(`0x5C7BA0`, pane タブ) / `0x5C96B3`(`0x5C9650`, race_button_done に **2.0=Back**) / `0x5CC81F`(HandleClick, race_button_done に **1.0=Done**)。
- `GetFloat(0xfb3)` は `0x598D9F`,`0x598FA0`（head `0x5987F0`、**RaceSexMenu 外**）のみ → **RaceSexMenu に C++ 読者は無い**。
- → race_button_done の `user5` の消費者は **XML 層**: `<user0>` が `<copy src="me()" trait="user5"/>` を参照し、`_text_1`(Done)/`_text_2`(Return) を user5 で選択。
- → ポートでは **`user5` = done ボタンのラベル選択子**（Done=1 / Return=2）。「押下記録だけ」ではない。

## §11 — 次段取り（実装入力）

順序: **1→3→6→7→9→10（本裁定で完了）→ ラベル（Main 6 件で書式確認）→ 残りラベル → command 対応 → `chargen_ui_spec.md`**。

- ラベルは 90 slider が最大。まず **Main の 6 件（2 slider + 3 button + 1 toggle）** だけで書式を固め、量産前にユーザー確認を入れる。
- その後 118 item ラベル → 24 HandleClick サイトの command 対応。

## §12 — 残点

| ID | 内容 | 状態 |
|---|---|---|
| V-638-1 | `+0x94` の宣言位置（基底 Menu か RaceSexMenu か） | `[部分確定]` |
| V-638-2 | `+0x2c`（race_player_grab）の実使用経路（3D グラブ） | `[未確定]` |
| V-638-3 | `_text_`（数値サフィックス trait）の解決機構（user5→`_text_1/2`） | `[未確定]` |
