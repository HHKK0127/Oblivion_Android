# v634 — pane 名は EXE に焼かれている（XML は参照しない）／Main の id=0／Init クリア範囲／`0x58A020`

対象: `Oblivion.exe` v1.2.0416 (md5 `cdd2f0c5eff198d4f26b7b5b54ce4930`)。
根拠: 実読 probe **u101_1〜u118_1**（全て EXIT=0）。
判定: `[確定(命令)]` / `[確定(XML)]`（negative） / `[部分確定]`

---

## 0. 結論

1. **21 pane の名前は EXE に焼かれている**。`mov edx,[0xB38XXX]` は 8 バイト名前オブジェクト `{+0=display, +4=editorid}` の `+0`（display）を読む。**XML は pane 名を 1 件も与えない**（`<parent>` も 0 件）。
2. 名前グローバルの間隔は **4 ではなく 8 バイト**（登録簿 base=`0xB38F38`、stride 8、全 **136** 個、UI 共通の名前プール）。`.data` は BSS（静的初期化なし）。実行時 thunk（`0x9F6710` 帯）が `ctor 0x419A70` で構築。
3. **Main の id は 0**（`arg4=0` 明示、counter 非消費）。→ **旧 id 表は Main 以外が一律 -1 ずれる**（旧 Race=210 → **209**、最終 429 → **428**）。
4. **Init `0x5C4E40` は `+0x40..+0x93`（pane slot 配列）をクリアしない**。`rep stosd` は `+0x94` 起点の 1 発のみ。
5. `0x58A020` は **`this+8`** に文字列を書く（tile の name 系）。`[部分確定]`。

---

## 1. 21 pane 名グローバル全列挙（絶対アドレス）[確定(命令)]

builder `FUN_005ca010` を head から線形逆アセンブルし、各 `call 0x5C4340` 直前の名前ロードと直後の `mov [esi+0xNN],eax` を実測（u109_1）。

| # | store slot | call site | 名前グローバル | editorid(+4) | display(+0) = pane 名 |
|---|---|---|---|---|---|
| 1 | `+0x40` | `0x5CA448` | `0xB38F70` | `sMain` | **Main** |
| 2 | `+0x44` | `0x5CA5A4` | `0xB38F78` | `sRace` | **Race** |
| 3 | `+0x48` | `0x5CA62D` | `0xB38F88` | `sHair` | **Hair** |
| 4 | `+0x4c` | `0x5CA726` | `0xB38F80` | `sFace` | **Face** |
| 5 | `+0x50` | `0x5CA85B` | `0xB38FE0` | `sShape` | **Shape** |
| 6 | `+0x54` | `0x5CAAE4` | `0xB38FE8` | `sTone` | **Tone** |
| 7 | `+0x58` | `0x5CACA1` | `0xB39000` | `sGeneralFace` | **General(Face)** |
| 8 | `+0x5c` | `0x5CAD84` | `0xB39008` | `sBrow` | **Brow** |
| 9 | `+0x60` | `0x5CAE1B` | `0xB39010` | `sCheeks` | **Cheeks** |
| 10 | `+0x64` | `0x5CAEFE` | `0xB39018` | `sChin` | **Chin** |
| 11 | `+0x68` | `0x5CB02D` | `0xB38F90` | `sEyes` | **Eyes** |
| 12 | `+0x6c` | `0x5CB0EA` | `0xB39020` | `sForehead` | **Forehead** |
| 13 | `+0x70` | `0x5CB181` | `0xB39028` | `sJaw` | **Jaw** |
| 14 | `+0x74` | `0x5CB23E` | `0xB39030` | `sMouth` | **Mouth** |
| 15 | `+0x78` | `0x5CB3DF` | `0xB39038` | `sNose` | **Nose** |
| 16 | `+0x7c` | `0x5CB618` | `0xB39040` | `sGeneralSkin` | **General(Skin)** |
| 17 | `+0x80` | `0x5CB6FB` | `0xB39050` | `sBeard` | **Beard** |
| 18 | `+0x84` | `0x5CB819` | `0xB39320` | `sEyesTex` | **Eye shades** |
| 19 | `+0x88` | `0x5CB937` | `0xB39058` | `sEyebrows` | **Eyebrows** |
| 20 | `+0x8c` | `0x5CBA7E` | `0xB39060` | `sMouthLips` | **Mouth(Lips)** |
| 21 | `+0x90` | `0x5CBB21` | `0xB39328` | `sNoseTex` | **Nose shades** |

## 2. 間隔は 8 バイト（4 ではない）[確定(命令)]

`deltas = 0x8,0x8,0x8,0x50,0x8,0x18,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x10,0x8,0x8,0x2C0,0x8`（**全て 8 の倍数**、`all(d==4)==False`）。
- 登録簿全体 = 136 個の 8 バイトオブジェクト、`0xB38F38`〜`0xB39370`（`0xB38F38 + 135*8 = 0xB39370`）。
- 21 pane 名は部分集合。間に別名オブジェクト（Age/Complexion/Gender/RaceDescription/Style/Red/Green/Blue/Length/Random/Reset/Skin/Lips/Nares…）。

## 3. `.data` は静的初期化なし（BSS）[確定(命令)]

`.data` rawsize=`0x30C00`、`0xB38F70 - 0xB02000 = 0x36F70 > 0x30C00` → ファイル実体なし。ファイルから読むとゴミ（`find_off` が次セクション raw を拾う）。
- 実行時 thunk 群（`0x9F6710`〜`0x9F78xx`、**単一の連続ブロック**）が
  `push <display>; push <editorid>; mov ecx,<global>; call 0x419A70; push <dtor>; call <atexit>; pop ecx; ret`
- `FUN_00419a70(this, editorid, display)` = `[this+4]=editorid; [this+0]=display;` を格納し、`0xB35574` の map に **editorid をキー**に登録（重複時 `"Setting key '%s' already used in map."`）。
- **pane 名 = `+0` = display**。editorid(`sXxx`) はキー専用。
- 登録簿は **UI 共通の名前プール**（`sPotion`/`sClass`/`sPlayer`/`sSpell`/`sTheElderScrolls`/`sOblivionCaps` 等の汎用名を含む）。chargen 専用ではない。

## 4. `race_sex_menu.xml` は pane 名を参照しない [確定(XML)(negative)]

- **`<parent>` 要素は race_sex_menu.xml に 0 件**（`bsa_chargen` 配下全体でも 0 件）。
- widget 名は **`name` 属性**（`<name>` 要素ではない）。21 display 名は **0 件**（`Main`/`Race` は `RaceSexMenu` の部分文字列のみ）。
- 参照は `src="..."` のみ: `race_window` / `race_name` / `race_button_done` / `race_scroll_bar` / `race_template_*` / `horizontal_scroll_*` / `child()` / `sibling()` / `parent()` / `me()` / `screen()` / `strings()` / `RaceSexMenu`。**21 pane 名を参照する箇所は無い**。
- XML の pane は `<!-- template PANES added here by code -->`（L201）＝ **コード生成**。親子は XML の入れ子 + コード（arg1=`[esi+0x28]`=window）で決まる。
- → 「一致 0 件 → 別名前空間で親子を結ぶ」分岐には当たらない。**XML は pane を参照しない**。
- → `docs/MENU_XML_INTERPRETER_DESIGN.md` は「XML が pane 名を与える」とは主張していない（§3.6 で template/prefab は「エンジンコードがリスト行として実体化」と正しく記載）。**この前提はドキュメントに存在しないため訂正不要**。pane 名の正典は **EXE**。

## 5. Main の id=0 と counter 非消費 → id 表は -1 シフト [確定(命令)]

`FUN_005c4340` の id ロジック（u113_1）:

```
0x5C4381 mov eax, [esp+0x40]      ; eax = arg4
0x5C4385 cmp eax, -1              ; ★ 判定
0x5C4388 jne 0x5c4397             ; arg4 != -1 → counter ブロックを飛ばす
0x5C438A mov eax, [0xb3b5d4]      ; counter
0x5C438F add eax, 1
0x5C4392 mov [0xb3b5d4], eax      ; ★ 加算後を採用（arg4==-1 の fall-through のみ）
0x5C4397 mov [esp+0x40], eax      ; id = eax（arg4 そのまま or counter+1）
0x5C439B mov [esp+0x14], eax
...
0x5C43F5 push 0xfa8; call 0x58ceb0     ; SetFloat(tile, 0xfa8, id)
0x5C4435 push 0xfae; call 0x58ceb0     ; SetFloat(tile, 0xfae, id)
0x5C4444 mov [ebx + edx*4 + 0x94], esi ; +0x94[id] = tile
```

1. **判定と分岐先**: `cmp eax,-1` @`0x5C4385`、`jne 0x5c4397` @`0x5C4388`。counter 3 命令（`0x5C438A..0x5C4392`）は **`jne` の fall-through（arg4==-1 のとき）**。
2. **`arg4 != -1` のとき counter は進まない**（`jne` でブロックをスキップ）。
3. **id = arg4（明示時）または counter+1（arg4==-1 時）**。`0xfa8`/`0xfae` と `+0x94` 索引に使う値は同一。
- **呼出側の arg4**（u113_1/u112_1）: **pane #1 (Main) のみ `push 0; push 0`（即値 `6A 00 6A 00`）**、#2〜#21 は `push edi; push edi`（`57 57`、edi=`0xffffffff`）。
- **item generator の counter 増分**（u114_1）: button `0x5C4480` +1 / toggle `0x5C4630` +1 / text `0x5C4800` +1 / slider `0x5C93F0` **+2**。いずれも無条件（arg4 判定なし）。

### シミュレーション結果（u115_1、counter 初期 200）

| pane | 新 id | 旧 id | pane | 新 id | 旧 id |
|---|---|---|---|---|---|
| Main | **0** | 201 | Forehead | 299 | 300 |
| Race | **209** | 210 | Jaw | 306 | 307 |
| Hair | **213** | 214 | Mouth | 315 | 316 |
| Face | **224** | 225 | Nose | 336 | 337 |
| Shape | **229** | 230 | General(Skin) | 365 | 366 |
| Tone | **239** | 240 | Beard | 376 | 377 |
| General(Face) | **246** | 247 | Eye shades | 389 | 390 |
| Brow | **257** | 258 | Eyebrows | 402 | 403 |
| Cheeks | **264** | 265 | Mouth(Lips) | 417 | 418 |
| Chin | **275** | 276 | Nose shades | **424** | 425 |
| Eyes | **290** | 291 | 最終 counter | **428** | 429 |

- 分岐: **「Main=0 かつ counter 非消費」**。ただし他 20 pane のずれは **-1**（**-9 ではない**）。理由: Main の items は counter を消費する（8 増分）。旧表は「Main も自動採番（+1）」と仮定していたため、その +1 分だけ一律ずれる。相対ギャップ（Race→Hair=4 等）は新旧一致。
- **`+0x94[0]` = Main**。`FUN_005c0440` は `mov ecx,[esi+0x94]`（索引 0 直読み）で Main の trait `0xfa7` を読む（u118_1）。`FUN_005c3be0` は `[esi+eax*4+0x94]`（索引）。→ **`+0x94` は id→tile 表で確定**。

## 6. Init は `+0x40..+0x93` をクリアしない [確定(命令)]

```
0x5C4EA5 xor eax, eax
0x5C4EA7 lea edi, [esi+0x94]        ; ★ +0x94 起点
0x5C4EAD mov ecx, 0x1F4             ; 500 dwords
0x5C4EB2 rep stosd [edi], eax       ; ★ +0x94..+0x864 のみ
```
- Init 内の `[esi+..]` store は `+0x28/+0x2c/+0x30/+0x3c` と `+0x864` 以降のみ。**`+0x40..+0x93`（pane slot 配列）をクリアする store は無い**（別ループ・`rep stosd` 第2発も無い）。
- → 再 Init でも pane slot は残るが、builder が全 21 を上書きする。Init の caller は 1（`0x58847A`）。

## 7. `0x58A020` は `this+8` に文字列を書く [部分確定]

```
0x58A041 mov esi, [esp+0x18]        ; esi = arg1 (文字列)
0x58A045 push 0
0x58A047 add ecx, 8                 ; ★ ecx = this+8
0x58A04A push esi
0x58A053 call 0x4028d0              ; BSStringT assign(this+8, arg1, 0)
0x58A058 push esi
0x58A059 call 0x401f20              ; free(esi)
0x58A071 ret 8                      ; stack 2 引数
```
- **書込先 = `this+8`**。`ret 8`（文字列 + フラグ）。callers = **12**（`0x5C43E6` pane / `0x5C451D` button / `0x5C46C9` toggle / `0x5C489B` text / `0x5C9492` slider / XML interpreter の trait `0xbba` ハンドラ `0x58D079` / `0x593C19` / `0x5A2761` / `0x5A750A` / `0x5A7723` / `0x5D4DDB` / `0x5D8437`）。
- `FUN_00585410`(GetTile) は `this+8` を **リスト**として歩く（node=`{item, next@+4}`, `item+0`=名前）。container ctor `FUN_0058cd30` も `+8` をリスト初期化。→ **container の `+8` は子リスト、tile の `+8` は文字列**（クラス差）。
- 判定 **name 寄り**（根拠: chargen pane 名は `FUN_005c3110`（名前→pane slot 照合）で lookup key、XML interpreter も同関数を trait `0xbba` で呼ぶ）。ただし「`0xbba` = name trait」の最終同定と label(user2) との切り分けは **未確定** → `[部分確定]`。

## 8. 検証

- probe: u101〜u118（全て EXIT=0）。主: u109_1（21 名前源）／u110_1（`0x5C4340`）／u113_1（arg4 判定）／u114_1（増分）／u115_1（counter シミュ）／u116_1（Init クリア・`+0x94` 読取）／u117_1（`0x58A020`/GetTile）／u118_1（`+0x94[0]`）。
- 補助: `docs/Oblivion_decompiled.c` の `FUN_00419a70`(L16472) / `FUN_00419b10`(L16507) / `FUN_005c4340`(L330410) / `FUN_00585410`(L290959) / `FUN_0058cd30`(L297078) / `FUN_0058a020`(L295283)。

## 9. 残点

- **`0x58A020` の `+8` が name か label(user2) か**（trait `0xbba` の同定）→ RE 残点（§11 で `0x5C3110` 非読取を確認済み・消えず）。
- V-629-1（`player[0xE8]` 実行時裏取り）は pane id には不要（静的に確定）だが、他の id 用途で残る。
- display 名のローカライズ（`(Face)`/`(Skin)`/`(Lips)`/`Eye shades`）は spec で `strings.xml` 側に置く方針を明記する。

---

## 10. 追補 — XML `<id>` は pane/item id と一致しない [確定(XML)(negative)]

BSA（`Oblivion - Misc.bsa`）から `menus\chargen\race_sex_menu.xml` を再抽出（22126 bytes、`<menu name="RaceSexMenu">`）して全 `<id>` を実測（u119_2）:

| `<id>` | parent | `<id>` | parent |
|---|---|---|---|
| 1 | race_window | 100 | race_pane (template pane) |
| 2 | race_player_grab | 101 | race_template_button_layout |
| 10 | race_name | 102 | race_template_toggle |
| 20 | race_scroll_bar (include) | 103 | race_template_slider |
| 90 | race_button_done (include) | 104 | race_template_text |
| `&generic;` ×4 | background / toggle_left / toggle_right / slider_bar | | |

- **numeric = {1,2,10,20,90,100,101,102,103,104}（10 件）+ `&generic;` ×4**。min=1 / max=104 / **連続でない**。
- **pane id（21）との一致 = 0 件**（code id は 0 と 200+、XML id は ≤104 で非重複）。0..428 内に落ちる XML id は上記 10 件のみ。
- → **pane/item id は XML に存在しない**（外部照合なし）。item ↔ XML ブロックの対応は `<id>` ではなく **template（100〜104）の実体化**で決まる（各 item は `race_template_slider`=103 等のインスタンス）。counter 初期 200 は XML 静的 id（≤104）と衝突回避するための値と整合。
- → 「完全一致 → `[確定(XML)]` 裏付け」の分岐は不成立。**XML に id 列は無い**。

## 11. 追補 — `0x5C3110` は HandleClick から名前で引かれる（`+8` は読まない）[確定(命令)]

- `FUN_005c3110(this, name)` は `__mbscmp(name, <名前グローバル>)` で照合し、一致時に `[this+0x40+i*4]`（pane slot）を返す（`docs/Oblivion_decompiled.c` L329814〜）。**tile の `+8` は一切読まない**。
- `callers_of(0x5C3110) = 31`。内訳: **`FUN_005cc200`(HandleClick) が 26 サイトで直接呼ぶ**（0x5CC2A8〜0x5CCA2F）、他は 0x5C3440 / 0x5C6390 / 0x5C6860 / 0x5C6AF0 / 0x5C9650 / builder 0x5CA010。
- 前方 BFS: **HandleClick(0x5CC200) → 0x5C3110 は depth 0（直接）**。Update(0x5CDEF0) → depth 1/3。→ **ページ切替は名前経由で確定**。
- **§5 の `+8` 確認: `0x5C3110` は `+8` を読まない → `0x58A020` の name/label 残点は消えず、起票のまま**。

## 12. 追補 — `+0x94[0]`=Main は `FUN_005c0440` のゲート [部分確定]

- `FUN_005c0440(this, command)`（L327955）: `if ((*(this+0x28) != 2) || (GetFloat(Main, 0xfa7) == _DAT_00a3ddd8))` のとき `command ∈ {8,9,0x1a}` を switch して 4 ハンドラ（0x5BFB90/0x5BF470/0x5BEA10/0x5BEA40）へ。→ **`+0x94[0]`（Main）の trait `0xfa7` をゲートに使う**。`callers_of(0x5C0440)=0`（vtable 経由）。
- 他 4 関数は**変数索引**（`[base + id*4 + 0x94]`）の汎用 `id→tile` アクセサ: `FUN_005c3be0(this,id)`（L330132）、`FUN_005c6390`（`[ebp+edi*4+0x94]`）、`FUN_005c6860`（`[esi+edi*4+0x94]`）、`FUN_005c6af0`（`[edi+eax*4+0x94]`）。**固定 index 0 を読むのは `FUN_005c0440` のみ**。
- → **index 0（Main）は特別扱い**（少なくとも command ゲート）。ポートの pane 配列は `main` を別フィールドに分ける設計が妥当。

## 13. 追補 — Init の pane slot 非クリアは spec で安全側に [方針]

- exe は Init で `+0x40..+0x93` を消さず builder が全 21 を上書きするが、途中失敗時は前回ポインタが残る。
- **spec では「メニューを開くたびに 21 スロットを明示 NULL 初期化」とする**（exe の挙動をそのまま写さず安全側）。

## 14. 追補の検証

- probe: u119_2（BSA から XML 再抽出・`<id>` 全抽出）／u120_1（`0x5C3110` callers・到達性・`FUN_005c0440` 本体・他4関数の索引）。
- 補助: `docs/Oblivion_decompiled.c` L327955（`FUN_005c0440`）/ L330123（`FUN_005c3be0`）/ L329798（`FUN_005c3110`）。

---

## 15. XML 同一性の確定 [確定(データ)]

BSA（`Oblivion - Misc.bsa`、comment start=1107505）から **22156B** で厳密抽出（u121_2）:

| 項目 | 値 |
|---|---|
| raw md5 | **`d367341334fa89101ba4fc077edb52fe`**（既知値と MATCH） |
| normalized md5 (CRLF→LF) | `a9ab8fabd4db1a450e158237110dd382` |
| bytes (raw / normalized) | **22156** / 21451 |
| lines / 改行 | 705 / CRLF |

- 前回の 22126 は**切り詰め**（先頭 `<!-- race_sex_menu.xml -->` −28 と末尾 CRLF −2）。内容は同一 → §10 の `<id>` 0 件一致は有効。
- 正典記録 `D:\re\XML_CANONICAL.txt`、`D:\re\MANIFEST.sha256`。

## 16. 作業ディレクトリの移設 [方針]

- `%TEMP%` 掃除で消えるため、probe ハーネス・抽出 XML を **`D:\re\`** へ移設（`v307_common.py` / `race_sex_menu.xml` / `MANIFEST.sha256` / `XML_CANONICAL.txt`）。`AGENTS.md` に復旧1行を追記。

## 17. trait 名表（XOBSE GameTiles.h 実読）— `0x58A020` 残点の解決 [確定(データ)]

XOBSE `obse/obse/GameTiles.h` の `eTileValue` で trait id を同定:

| id | 名前 | id | 名前 | id | 名前 |
|---|---|---|---|---|---|
| 0xFA1 | visible | 0xFA5 | stackingtype | 0xFA6 | locus |
| **0xFA7** | **alpha** | 0xFA8 | id | 0xFA9 | disablefade |
| 0xFAA | listindex | 0xFAB | depth | 0xFAE | user0 |
| 0xFAF | user1 | 0xFB0 | user2 | 0xFB5 | user7 |
| 0xFB6 | user8 | 0xFB7 | user9 | 0xFDE | string |
| 0xFE1 | clicked | 0xFE6 | filename | **0xBBA** | **name** |

- `Tile` レイアウト（同ヘッダ）: `BSStringT name` は **+08**、`Tile* parent` は +10、`ValueList valueList` +14、`RefList childList` +30。`Tile::Value::str`(BSStringT) も **+08**。
- → **`0x58A020` は `this+8` = `Tile::name` に書く `SetName`**（XML interpreter は trait **`0xbba`=name** で呼ぶ）。**残点「+8 = name/label」は解決 = name**。§1 の 21 名は tile の名前そのもの。

## 18. `FUN_005c0440` のゲートと command ハンドラ [確定(データ)]

- ゲート `GetFloat(Main, 0xFA7=alpha) == _DAT_00a3ddd8`。**`_DAT_00a3ddd8` = 255.0（double, raw `00 00 00 00 00 E0 6F 40`）**。→ **Main の alpha が 255（不透明）のとき** command を処理。
- command `8`/`9`/`0x1a` → jump table `DAT_005c04d4` → 4 ハンドラ:
  - `0x5BEA10`: `[ecx+0xbc]` の `user1(0xFAF)` を `[0xa379b4]` と比較 → 一致で `0x5BE270` へ。
  - `0x5BEA40`: `[esi+0xc0]` の `user1` 比較 → 一致で `push 8`→`0x57de50`、`0x5be5c0(1)`、`SetFloat([esi+0xc0], 0xFAF, 1.0)`。
  - `0x5BFB90`: `[esi+0x28]==2` のとき `[edi+0x20/0x28]`・`[esi+0xe0/0xe4]` で座標計算 → `0x5BE6F0`。
  - `0x5BF470`: `FUN_005be870` ゲート後、描画グローバル（0xB38E38〜）と仮想呼出。
- → ハンドラは **Race/Sex 系（`[esi+0xbc]`/`[esi+0xc0]` の tile と user1）** を扱う。Main のゲートは「Main が不透明のときだけ command 有効」= Main ページの活性条件。

## 19. slider `0x5C93F0` の arg2 実体 [確定(命令)]

- 5 generator の template 名グローバル（u124_1）: pane=`0xA6D2DC` / button=`0xA6D2F4` / toggle=`0xA6D318` / text=`0xA6D330` / **slider=`0xA6D3D0`**。→ **全 90 slider が単一 template `race_template_slider` を共有**。
- `0x5C93F0` 本体: `push imm 0..0x36` は `[esp+0x44]` に載り、**`SetFloat(slider, 0xFB5=user7, arg2+1)`** に使う（`0x5C9548 add edx,1; 0x5C955D push 0xfb5`）。`0x5C957B cmp [esp+0x44], -1` で `-1` 判定。
- **表索引ではない**（`[base+arg2*size]` は無い）。arg2 は **trait `user7`（=editor id index）として書かれる**。加えて `user2(0xFB0)`=label 文字列、`user8(0xFB6)`=flag 引数、`user6(0xFB4)` を設定。
- **個体差の担い手は arg2（user7）＋ label ＋ user8** の 3 つ。min/max/step は slider 本体ではなく子 `race_template_slider_bar` 側。

## 20. `race_template_slider`(103) の実値（XML 実読）[確定(データ)]

`race_template_slider`(rect): `user0`=copy(bar.user7) 現在値 / `user1`=0 / **`user2`=Label** / `user3`=0 / `user6`=-1(marker id) / **`user7`=-1(editor id of what I'm sliding)** / **`user8`=-1(geometry/texture flag)** / `user10`=0(変更フラグ) / `user13`=true(enabled)。
子 `race_template_slider_bar`: `user1`=0(**min**) / `user2`=1(**max**) / `user3`=0.04（xbox は 0.05、**step**） / `user4`=0.1(**jump**) / `user5`=copy(parent.user3) / `user6`=copy(parent.user6) / `user8`=false / `user13`=copy(parent.user13)。
- → **slider の範囲は template 既定（0..1, step 0.04/0.05, jump 0.1）**。コードは範囲を上書きせず、`user7`（対象 morph）と label のみ個別設定。

## 21. HandleClick の名前引きサイト [確定(命令)]

- `FUN_005cc200`(HandleClick) 内の `call 0x5C3110` は **24 サイト**（u125_1、`0x5CC2A8`〜`0x5CCA2F`）。名前別: **Main×3 / Brow×2 / Shape×2 / Tone×2**、他 15 名が各 1（19 名）。Race/Hair は出ない。
- 不一致時の戻り値 = **`0x5C341E xor eax,eax` → NULL(0)**。
- → ページ切替・ボタン処理は pane 名で引き当てる。重複（Main/Brow/Shape/Tone）は同一 pane への別 command。

## 22. 起票の更新

- **`0x58A020` の `+8` = `Tile::name`（`SetName`）で解決**（§17）。trait `0xbba`=name。**残点クローズ**。
- 継続: V-631-1（`race_sex_panel` 未作成で実装単位列保留）。
