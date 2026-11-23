# v639 — `id` の実体（保存される trait）・`_text_`・race_button_done・`0x5C9650`・Main 6 item ラベル

- **日付**: 2026-11-22
- **帯域**: Tile trait ストレージ / XML interpreter / RaceSexMenu 初期化 / Main pane item ラベル
- **判定語彙**: `[確定(命令)]` / `[確定(命令)(negative)]` / `[確定(データ)]` / `[確定(XML)]` / `[部分確定]` / `[未確定]`
- **読み取り宣言**: capstone probe（`D:\re\v307_common.py`）。XML は `D:\re\race_sex_menu.xml` と `tools/BSAFileExtractor/extracted/menus/`。
- **起点**: v638（row615）の残点 + ユーザー指示（1→2→3→4→5）。

## §1 — `id` は保存される trait（前提の訂正）`[確定(命令)]`

- **`SetFloat`（`0x58CEB0`）= 格納関数**:
  ```
  0x58CEB0 mov eax,[esp+4] ; push eax ; call 0x58b220   ; GetOrCreateEntry(traitId) → eax
  0x58CEBE fld [esp+8] ; push ecx ; mov ecx,eax ; fstp [esp] ; call 0x58ca00   ; store
  ```
- **`0x58CA00(entry, value)` = 値の格納**:
  ```
  0x58CA03 fld [esi+4] / 0x58CA0A fld [esp+8] / fucompp  ; 新旧比較
  0x58CA17 cmp word [esi+0x18], 0xfde                     ; string trait は常に更新
  0x58CA34 fstp dword ptr [esi+4]                         ; ← value を格納
  0x58CA40 call 0x58bee0                                  ; → FUN_0058bEE0 → FUN_0058b2f0 → SetField
  ```
  → **`SetFloat(tile, 0xFA8, n)` は id を格納し、かつ（`FUN_0058bEE0` 経由で）`SetField(n, tile)` も呼ぶ**。**store が先、SetField が後**。
- **`GetFloat`（`0x588BD0`）** は tile の trait リスト（`[ecx+0x18]` 起点）を走査し、`word[entry+0x18]==traitId` の `[entry+4]` を返す（無ければ 0.0）。
- **`GetFloat(tile, 0xFA8)` の読み手は多数存在**（全 .text の `push 0xfa8` を分類）: 例 `0x579A2E`/`0x579A86`/`0x57D76E`/`0x57FB08`/`0x58245A`/`0x5894F9`/`0x59929B`/`0x5B1B00`/`0x5B8F2E`/`0x5BC78D` が READ。
- → **`id` は tile に保存され、読まれる**。ユーザーの前提「`SetFloat` が `SetField` を呼ぶだけで id を書かない」は**逆**（`SetFloat` は格納し、`SetField` はその副作用）。**「tile.id」は仕様から消せない**。
- 補足: `0x58CA00` は**値が変化したときのみ**格納する（`fucompp` で同値なら string trait 以外は skip）。→ 新品 tile への `id=0`（Main）は no-op（既定 0 のまま）、`id=-1`（`&generic;`）は 0→-1 の変化で格納される。

## §2 — `_text_` の解決機構 `[部分確定]`（V-639-1）

- race_sex_menu.xml の `_text_` 出現は 3 箇所のみ: L227 `<copy src="me()" trait="_text_"/>`、L231 `<_text_1> <copy src="strings()" trait="_done"/> </_text_1>`、L232 `<_text_2> <copy src="strings()" trait="_return"/> </_text_2>`。
- **EXE に `_text_` の文字列は存在しない**（`b"_text_"` 0 ヒット）→ `_text_N` は**データ駆動の数値サフィックス trait**。
- trait 名表（`FUN_0058a0a0` 登録）に `_text_` の項目は無い → サフィックスは別機構で解決。
- 現時点の結論: `_text_N` は **indexed trait**（`<copy trait="_text_"/>` が tile の状態値 N で `_text_N` を選ぶ）と推定。`user5`（1=Done / 2=Return）が選択子。**選択の実コードパスは未特定** → V-639-1。

## §3 — `race_button_done` の XML 実体 `[確定(XML)]`

- `race_button_done`（L211-237）: `<id>90</id>`、`<user0>` は**ボタン自身**（子 text ではない）:
  ```
  <user0> <copy src="me()" trait="user5"/> <copy src="me()" trait="_text_"/> </user0>
  <user5> &false; </user5>   <!-- 1 (false) for Done and 2 (true) for Back -->
  <_text_1> <copy src="strings()" trait="_done"/> </_text_1>
  <_text_2> <copy src="strings()" trait="_return"/> </_text_2>
  ```
- `<id>90</id>` は trait `0xFA8`、`<user0>` は trait `0xFAE`。**別 trait** なので取り合いにはならない。
- `_text_1` = strings `_done`（"Done"）/ `_text_2` = strings `_return`（"Return"）。
- コードが書く user0 は pane/item 生成器（`FUN_005c4340` 等）の tile であり、race_button_done の user0 ではない。

## §4 — `0x5C9650` の役割 `[確定(命令)]`

- head `0x5C9650`、**唯一の caller は `0x5CCFCC`（= HandleClick `FUN_005cc200` 内）**。
- 動作: `ebx=[esp+0x30]`（名前文字列）で BSStringT を作り `0x5C3110`（名前→pane）を引く → `SetFloat([esi+0x3c]=race_button_done, user5=0xFB3, 2.0)`（`[0xa379b4]`=2.0）。
- → **`0x5C9650` = done ボタンを Return モード（user5=2）にする**。HandleClick がコマンドに応じて `0x5C9650`（→2=Return）または `0x5CC816`（→1=Done）を呼ぶ。
- `user5` を 1.0 に戻すのは HandleClick `0x5CC816` のみ（RaceSexMenu 帯の `push 0xfb3` はすべて SetFloat、§v638 §10）。

## §5 — Main pane の 6 item ラベル `[確定(命令)]`

Main pane は `0x5CA448`（`FUN_005c4340`, arg1=[esi+0x28]）で生成され `[esi+0x40]` に入る。続く 6 item:

| # | item 種別 | call site | ラベル元（8B name object） | ラベル trait |
|---|---|---|---|---|
| 0 | button (`0x5C4480`) | `0x5CA491` | `[0xB38F78]` | name (0xBBA) |
| 1 | button | `0x5CA4D5` | `[0xB38F80]` | name |
| 2 | button | `0x5CA518` | `[0xB38F88]` | name |
| 3 | toggle (`0x5C4630`) | `0x5CA53A` | `[0xB38F90]` | name |
| 4 | slider (`0x5C93F0`) | `0x5CA55E` | `[0xB38F98]` | name |
| 5 | slider | `0x5CA582` | `[0xB38FA0]` | name |

- 各 site の直前で `mov edx,[0xB38FXX]; sub esp,8; …; push edx; call 0x419b10`（name object → BSStringT）→ 生成器が `0x58A020`(**SetName**) で tile の **name(0xBBA)** に書く（button 生成器 `0x5C451D` で実測）。**user2(0xFB0) ではない**。
- name object の並び（editorid リテラル `0xA62964` 起点）: `sMain, sRace, sFace, sHair, sEyes, sAge, sComplexion, …`。globals は `0xB38F38+i*8`（idx7=`0xB38F70` Main, 8=Race, 9=Face, 10=Hair, 11=Eyes, 12=`0xB38F98`, 13=`0xB38FA0`）。
- → Main 6 item のラベル = **Race / Face / Hair / Eyes / (idx12) / (idx13)**。idx12/13 は editorid 順から **sAge / sComplexion** が最有力（表示名は登録 ctor の display リテラル依存）`[部分確定]`。
- **再利用**: `[0xB38F78]`(Race) は Main の button#0 ラベルと Race pane 名の両方で使われる（同 global の共有）。`0x419b10` は name object → BSStringT の変換。

## §6 — 小さい

- **V-638-1 を閉じる**: `+0x94` の宣言位置は **RaceSexMenu**（ctor `0x5C4E40` が `+0x94` から 500 dword をクリアする以上、基底 `Menu` ではない）`[確定(命令)]`。
- **`&no_click_past;`=102 / `&click_past;`=101**: depth（`0xFAB`）の stackingtype 比較は z 順だけでなく**ヒットテスト制御（クリック透過）**も表す（名前 `click_past`/`no_click_past` と v626 の depth ゲート）。spec に明記。
- **`&scale;`=-1 の用法**: 抽出済み 89 XML に `&scale;` は**出現 0 件**（`<id> &scale;` も 0 件）→ `&scale;`=-1 を id に使う用法は**無い**。懸念は不要 `[確定(XML)]`。

## §7 — 残点

| ID | 内容 | 状態 |
|---|---|---|
| V-639-1 | `_text_`（数値サフィックス trait）の選択コードパス | `[部分確定]` |
| V-639-2 | Main item idx12/13 の表示名（name 登録 ctor の display リテラル） | `[部分確定]` |
| V-639-3 | `+0x2c`（race_player_grab）の実使用経路（v638 から継続） | `[未確定]` |
