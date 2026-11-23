# v640 — registry idx12/13・118 item ラベル表・label=name 副作用・`_text_` 動的マップ

- **日付**: 2026-11-22
- **帯域**: name registry（0xB38F38+i*8）/ RaceSexMenu item ラベル / `_text_` 動的 trait
- **判定語彙**: `[確定(命令)]` / `[確定(データ)]` / `[確定(XML)]` / `[部分確定]` / `[未確定]`
- **読み取り宣言**: capstone probe（`D:\re\v307_common.py`）。XML は `D:\re\race_sex_menu.xml` と `tools/BSAFileExtractor/extracted/menus/`。
- **起点**: v639（row616）の残点 + ユーザー指示（1→2→3→4）。

## §0 総括

| # | 命題 | 判定 |
|---|---|---|
| 1 | idx12 = "Age"(sAge) / idx13 = "Complexion"(sComplexion) | `[確定(データ)]` |
| 2 | 全 118 item ラベル表（1 枚） | `[確定(命令)]` |
| 3 | label は name(0xBBA) へ SetName。0x5C3110 は pane 名のみ走査 → 誤爆なし | `[確定(命令)]` |
| 4 | `_text_` は `_` プレフィックス動的マップ（id≥10000）。`0xFDE+n` ではない | `[部分確定]` |

## §1 — registry idx12/13 の確定 `[確定(データ)]`

- name object は `0xB38F38 + i*8`（{+0=display, +4=editorid}）。
- 初期化は **thunk 帯 `0x9F6710 + i*0x20`**。各 thunk:
  ```
  push <display lit> ; push <editorid lit> ; mov ecx, 0xB38F38+i*8 ; call 0x419a70   ; RegisterName(obj, editorid, display)
  ```
- 実読: idx7=`Main`/sMain, idx8=`Race`/sRace, idx9=`Face`/sFace, idx10=`Hair`/sHair, idx11=`Eyes`/sEyes,
  **idx12=`Age`/sAge, idx13=`Complexion`/sComplexion**, idx14=`Gender`/sGender, idx15=`Race Description`/sRaceDescription。
- → Main 6 item のラベルは **Race / Face / Hair / Eyes / Age / Complexion** で確定（推定を解消）。

## §2 — 全 118 item ラベル表 `[確定(命令)]`

- 出力ファイル: **`tmp/re/v640_item_labels.md`**（列 = pane | pane 名 | item# | 種別 | registry idx | display | site）。
- 抽出: builder `0x5CA010..0x5CBC20` を走査し、`call 0x5c4340`（pane）で pane++、`mov <reg>,[0xB38FXX..0xB393XX]` を label として直後の item 生成器（`0x5c4480` button / `0x5c4630` toggle / `0x5c4800` text / `0x5c93f0` slider）に紐付け。
- 内訳: **21 pane / 118 item**（button 42 / toggle 11 / text 1 / slider 64）。pane 0=Main は 6 item（button×3 + toggle×1 + slider×2）。
- **重複 idx**: `idx8`(Race) ×2（pane0 item0 button, pane1 item7 toggle）、`idx11`(Eyes) ×2（pane0 item3 toggle, pane4 item23 button）。

## §3 — label = name の副作用と 0x5C3110 `[確定(命令)]`

- item 生成器は `0x419b10`（name object → BSStringT）→ **`0x58A020`(SetName)** で tile の **name(0xBBA)** に書く（button 生成器 `0x5C451D` で実測）。**user2(0xFB0) ではない**。
- **`0x5C3110`（name→pane slot）**: `edi`（引数名）を **21 pane 名グローバル**（`0xB38F70` Main → `0xB38F78` Race → `0xB38F88` Hair → `0xB38F80` Face → `0xB38FE0` Shape → …）と **unrolled `strcmp`（0x98539f）** で順に比較し、一致した pane slot `[esi+0x40+i*4]` を返す。
- → **0x5C3110 は tile 名でなく pane 名グローバルを走査**するため、item 名（SetName）は関与しない。**誤爆なし**。呼び出し元 HandleClick は pane 名を渡す（v634 §16）。
- 重複 idx（Race/Eyes）は**表示文字列の再利用**であり、pane 解決（0x5C3110）には影響しない。ただし同名 tile が 2 つ存在するため、tile 名で引く汎用 API を使う場合は注意（本メニューでは 0x5C3110 はグローバル比較）。

## §4 — `_text_` の解決 `[部分確定]`（V-639-1 継続）

- trait 名→id 変換 `FUN_0058a0a0`（0x58A0A0）:
  - 先頭 `&` → entity マップ（`0xB3B0B4`）から解決。
  - 先頭 `_` → **別パス `0x58A12D`**。
  - それ以外 → 文字コードから id を算出（`al-0x40`、>0x20 で -0x20）。
- `0x58A12D` は **動的 trait マップ `0xB13BC4`** に `{name, id}` を挿入（`0x42d800`）。id は呼び出し元から渡る。
- 動的 id の採番は parser `FUN_0058b040` の `0x58B0B4`: `movzx ebp, word [0xB13BCE]; add ebp, 0x2710`（**counter + 10000**）。
- → **`_text_1`/`_text_2` は `0xFDE + n` ではなく、`_` プレフィックス動的マップ（id ≥ 10000）の trait**。`0xFDE`(string) の加算ではない。
- `<copy src="me()" trait="_text_"/>`（数字なし）→ `_text_<index>` の**選択規則（index 源 = 直前 copy の user5 か）は未特定** → V-639-1。
- 観測可能な振る舞い: race_button_done のラベルは `user5`(1=Done / 2=Return) に応じて `_text_1`(=strings `_done`)/`_text_2`(=strings `_return`) を選ぶ。ポートはこの振る舞いを実装すれば足りる。

## §5 — `同値 skip` の記録（v639 補足）`[確定(命令)]`

- `0x58CA00` は**新旧値が等しければ格納しない**（`fucompp` + `cmp word [esi+0x18],0xfde`）。
- → **Main の `id=0` は格納されない**（既定 0 のまま読める）。**`&generic;` の `id=-1` は 0→-1 の変化で格納される**。

## §6 — 残点

| ID | 内容 | 状態 |
|---|---|---|
| V-639-1 | `_text_`（数字なし）→ `_text_<index>` の選択規則 | `[部分確定]` |
| V-639-3 | `+0x2c`（race_player_grab）の実使用経路 | `[未確定]` |
| V-640-1 | 重複 idx（Race/Eyes）が tile 名検索系 API に与える影響 | `[未確定]` |
