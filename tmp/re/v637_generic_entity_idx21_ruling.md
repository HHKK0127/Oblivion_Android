# v637 — `&generic;` の値と idx 21（`+0x38` = vertical_scroll_marker）の確定

- **日付**: 2026-11-22
- **帯域**: XML interpreter の entity 定義 / RaceSexMenu の scroll 束縛
- **判定語彙**: `[確定(命令)]` / `[確定(データ)]` / `[確定(XML)]` / `[部分確定]` / `[未確定]`
- **読み取り宣言**: capstone probe（`D:\re\v307_common.py`）。XML は `D:\re\race_sex_menu.xml`（22156B, raw md5 `d367341334fa89101ba4fc077edb52fe`）と、抽出済み `tools/BSAFileExtractor/extracted/menus/`（`CharGen\race_sex_menu.xml` / `Prefabs\vertical_scroll.xml` / `strings.xml`）。
- **起点**: v636（row613）の残点 V-636-1 / V-636-2 を潰す。

## §0 総括

| 命題 | 判定 |
|---|---|
| `&generic;` = **-1** | `[確定(命令)]` |
| idx 21 → `+0x38` = **`vertical_scroll_marker`** | `[確定(命令)]`+`[確定(XML)]` |
| `<id>` は式を取り得る（動的 id） | `[確定(XML)]` |

## §1 — `&generic;` = -1 `[確定(命令)]`

`FUN_0058a0a0(name, value)`（entity/trait 登録）を呼ぶ組み込み entity 初期化 `0x58A1C0`:

```
0x58A1C0 push esi ; push -1 ; push 0xA6AAE0 ; mov esi,ecx ; call 0x58a0a0   ; "&generic;" = -1
0x58A1CF push 1  ; push 0xA6AAD8 ; call 0x58a0a0                          ; "&false;"   = 1
0x58A1DD push 2  ; push 0xA6AAD0 ; call 0x58a0a0                          ; "&true;"    = 2
0x58A1EB push 1  ; push 0xA6AAC8 ; call 0x58a0a0                          ; "&left;"    = 1
0x58A1F9 push 2  ; push 0xA6AABC ; call 0x58a0a0
0x58A207 push 4  ; push 0xA6AAB4 ; call 0x58a0a0
0x58A215 push -1 ; push 0xA6AAAC ; call 0x58a0a0
```

- 文字列は `&left;`(0xA6AAC8) / `&true;`(0xA6AAD0) / `&false;`(0xA6AAD8) / `&generic;`(0xA6AAE0) の並び。`&generic;` のポインタは `.text` の `0x58A1C4` に 1 箇所のみ。
- `&false;`=1 / `&true;`=2 は race_sex_menu.xml L229 のコメント「`1 (false) for Done and 2 (true) for Back`」と一致（enum は 0=未設定 / 1=false / 2=true）。
- → **`<id> &generic; </id>` は `<id> -1 </id>`**。id=-1 に対し `SetField(-1, tile)` は分岐が無いため **no-op**。`vertical_scroll.xml` の 5 箇所（L4/48/61/79/94）と各 menu の `&generic;` は menu フィールドを束縛しない。
- `&generic;` は **EXE 内蔵 entity**（strings.xml には無い。strings.xml の entity は `<rect name="Strings">` 配下の要素名）。

## §2 — idx 21 → `+0x38` = `vertical_scroll_marker` `[確定(命令)]`+`[確定(XML)]`

- `race_sex_menu.xml` の `race_scroll_bar`（`<id>20</id>`）:
  ```
  <user6> 21 </user6>   <!-- A unique ID so that the scroll marker can be dragged around -->
  ```
- `Prefabs\vertical_scroll.xml`（`race_scroll_bar` が `<include>`）:
  ```
  <image name="vertical_scroll_marker">
      <id> <copy src="parent()" trait="user6"/> </id>
  ```
- → marker の id = 親（race_scroll_bar）の user6 = **21**。→ `SetField(21, vertical_scroll_marker)` → **`menu+0x38 = vertical_scroll_marker`**。
- 整合: v636 §5 のとおり slot8 `0x5C3BE0` が `+0x34`(race_scroll_bar) と `+0x38`(marker) を同時に読み、`SetFloat(user8=0xFB6/user9=0xFB7)` を書く（スクロール位置更新）。
- 注: `&generic;`=-1 のため、`vertical_scroll.xml` の他の `<id> &generic; </id>` は束縛しない。idx 21 だけが動的 id 経由で束縛される。

## §3 — `<id>` は式を取り得る（動的 id）`[確定(XML)]`

- `<id> <copy src="parent()" trait="user6"/> </id>` のように `<id>` の値は式で書ける。
- interpreter は id trait(0xFA8) をセットする時点で式を評価済みの数値（=21）を `SetField` に渡す（v636 §3 の経路）。
- → 束縛層は「静的 id も式 id も同様に処理」する。

## §4 — 6 スロット最終表

| offset | idx | XML tile |
|---|---|---|
| `+0x28` | 1 | race_window |
| `+0x2c` | 2 | race_player_grab |
| `+0x30` | 10 | race_name |
| `+0x34` | 20 | race_scroll_bar |
| `+0x38` | 21 | **vertical_scroll_marker**（動的 id = race_scroll_bar.user6=21） |
| `+0x3c` | 90 | race_button_done |

## §5 — 残点

| ID | 内容 | 状態 |
|---|---|---|
| V-637-1 | `+0x94` の宣言位置（基底 Menu か RaceSexMenu か。V-636-3 継続） | `[部分確定]` |
| V-637-2 | `+0x2c`（race_player_grab）の実使用経路（V-636-4 継続） | `[未確定]` |

## §6 — 作業ディレクトリ追記

- **抽出済み menus の正典ローカルコピー**: `tools/BSAFileExtractor/extracted/menus/`（`CharGen\race_sex_menu.xml`, `Prefabs\{vertical_scroll,generic_background,button_long}.xml`, `strings.xml`, `master_menu_file.txt`）。`D:\re\race_sex_menu.xml` と併せて include 解決に使う。
