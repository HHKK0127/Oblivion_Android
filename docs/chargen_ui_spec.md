# chargen_ui_spec.md — Oblivion RaceSexMenu（キャラクター作成 UI）構造仕様

- **対象**: `Oblivion.exe` v1.2.0416（md5 `cdd2f0c5eff198d4f26b7b5b54ce4930`）／`RaceSexMenu`（menu id **0x40C**）
- **正典**: 本 spec は EXE 実読（v634/v636〜v649 の裁定）と実物 XML（`race_sex_menu.xml` 22156B, raw md5 `d367341334fa89101ba4fc077edb52fe`, CRLF, 705 行）に基づく。**XML を開かずに構造を説明できること**が本 spec の目的。
- **判定語彙**: `[確定(命令)]` / `[確定(データ)]` / `[確定(XML)]` / `[部分確定]` / `[未確定]`

---

## 0. 設計方針（冒頭・最重要）

1. **名前は表示専用、引当（identity）は id**。
   - 21 ページ・118 item の**名前（`Main`/`Face`/`Shape`…）は EXE に焼かれた表示文字列**で、XML はこれを 1 件も与えない `[確定(命令)]`（v634 §1/§4）。名前は UI 表示とデバッグ用。**実装の同一性判定・配列索引には使わない**。
   - 実装の引当は **id（実行時採番の整数）**。`<id>N</id>` の束縛も id。
2. **id は実行時採番**。counter 初期 **200**、`id = arg4`（明示時）または `counter+1`。**Main のみ `arg4=0` 明示**で counter 非消費 `[確定(命令)]`（v634 §5）。
3. **ページ ↔ item の対応は「builder が生成した順」＋「template の実体化」で決まる**。XML の `<id>` は pane/item id と一致しない（後述 §5）。
4. **exe の Init は pane slot をクリアしない**が、**実装はメニューを開くたびに 21 スロットを明示 NULL 初期化する（安全側）** `[方針]`（v634 §13）。
5. **原作との差（意図的逸脱）**: 原作は HandleClick で **`0x5C3110` による名前引き**（24 サイト）＋ family で `0x5C3440` の名前引きを行う。**本 spec の実装は「id 引き」に統一する**（`+0x40`/`+0x94` の id 索引）。名前引きは表示・デバッグ用に留め、同一性判定・遷移は id で行う。これは原作の名前プール（UI 共通・136 個・実行時 BSS 充填）に依存しないための逸脱 `[方針]`。

---

## 1. メニュー同定 `[確定(命令)]`

| 項目 | 値 |
|---|---|
| menu id | **0x40C** |
| RTTI locator | `0xAC2AF4`（bases {RaceSexMenu, Menu}） |
| XML | `Data\Menus\CharGen\race_sex_menu.xml`（`ReadXML` = `0x590420`） |
| ctor / 登録 | `0x57B8B1 FUN_00589b70(0x40c, &MenuRTTI, &RaceSexMenuRTTI, 0, …)` |
| builder | `FUN_005ca010`（chargen 専用・0x40C 直値。`0x57B8D9 call 0x5ca010`） |
| 初期化順 | menu 生成 → `ReadXML`（parse＋tile 木＋`<id>` 束縛）→ builder（pane/item 生成） |
| builder guard | `+0x28/+0x2c/+0x30/+0x3c` が**全て非 NULL でないと build 全体を中止**（`0x5CA14B jmp 0x5CC1DB`）。**`+0x38` は guard 対象外**（marker 未束縛でも成立）`[確定(命令)]` |

- 6 C ABI の純ロジック層は `rust/`（本 spec は UI 構造のみ。ロジックは `chargen_*` を参照）。

---

## 2. 21 ページ一覧（pane 名 / `+0x40` slot / id / item 数）`[確定(命令)]`

`+0x40` slot 配列 = `menu + 0x40 + i*4`（21 個）。pane 名は表示専用（§0）。

| # | pane 名 | slot | id | button | toggle | text | slider | 計 |
|---|---|---|---|---|---|---|---|---|
| 1 | **Main** | `+0x40` | **0** | 3 | 1 | 0 | 2 | 6 |
| 2 | Race | `+0x44` | 209 | 0 | 2 | 1 | 0 | 3 |
| 3 | Hair | `+0x48` | 213 | 0 | 2 | 0 | 4 | 6 |
| 4 | Face | `+0x4c` | 224 | 4 | 0 | 0 | 0 | 4 |
| 5 | Shape | `+0x50` | 229 | 9 | 0 | 0 | 0 | 9 |
| 6 | Tone | `+0x54` | 239 | 6 | 0 | 0 | 0 | 6 |
| 7 | General(Face) | `+0x58` | 246 | 0 | 0 | 0 | 5 | 5 |
| 8 | Brow | `+0x5c` | 257 | 0 | 0 | 0 | 3 | 3 |
| 9 | Cheeks | `+0x60` | 264 | 0 | 0 | 0 | 5 | 5 |
| 10 | Chin | `+0x64` | 275 | 0 | 0 | 0 | 7 | 7 |
| 11 | Eyes | `+0x68` | 290 | 0 | 0 | 0 | 4 | 4 |
| 12 | Forehead | `+0x6c` | 299 | 0 | 0 | 0 | 3 | 3 |
| 13 | Jaw | `+0x70` | 306 | 0 | 0 | 0 | 4 | 4 |
| 14 | Mouth | `+0x74` | 315 | 0 | 0 | 0 | 10 | 10 |
| 15 | Nose | `+0x78` | 336 | 0 | 0 | 0 | 14 | 14 |
| 16 | General(Skin) | `+0x7c` | 365 | 0 | 0 | 0 | 5 | 5 |
| 17 | Beard | `+0x80` | 376 | 0 | 0 | 0 | 6 | 6 |
| 18 | Eye shades | `+0x84` | 389 | 0 | 0 | 0 | 6 | 6 |
| 19 | Eyebrows | `+0x88` | 402 | 0 | 0 | 0 | 7 | 7 |
| 20 | Mouth(Lips) | `+0x8c` | 417 | 0 | 0 | 0 | 3 | 3 |
| 21 | Nose shades | `+0x90` | 424 | 0 | 0 | 0 | 2 | 2 |
| | | | 最終 counter | 22 | 5 | 1 | 90 | **118** |

- 種別計: **button 22 / toggle 5 / text 1 / slider 90 = 118**。
- `+0x94` は **id→tile 表**（後述 §3）。
- 生成器: button `0x5C4480`(+1) / toggle `0x5C4630`(+1) / text `0x5C4800`(+1) / slider `0x5C93F0`(**+2**)。増分は無条件 `[確定(命令)]`。

---

## 3. pane 構造と `+0x94` `[確定(命令)]`

| 領域 | 意味 |
|---|---|
| `menu+0x40 .. +0x93` | **pane slot 配列（21 × 4B）**。builder が生成順に格納。 |
| `menu+0x94 .. +0x863` | **id→tile 表（500 × 4B = `0x1F4` dwords）**。`+0x94[id] = tile`（`0x5C4444 mov [ebx+edx*4+0x94], esi`）。 |
| `menu+0x28/0x2c/0x30/0x34/0x38/0x3c` | 束縛スロット（§4） |

- **id 範囲**: counter 起点 200。id は **`0`（Main）と `209..424`**（他 20 pane）、最終 counter **428**。`+0x94` は 500 枠なので id は 0..499 を収容（実使用は ~201..428）`[部分確定]`。
- **`+0x94[0]` = Main**。固定 index 0 を読むのは `FUN_005c0440` のみ（command ゲート `GetFloat(Main, 0xFA7=alpha)==255.0`）。他は変数索引の汎用アクセサ（`0x5C3BE0`/`0x5C6390`/`0x5C6860`/`0x5C6AF0`）`[確定(命令)]`。
- **Init（`0x5C4E40`）のクリア範囲**: `xor eax,eax; lea edi,[esi+0x94]; mov ecx,0x1F4; rep stosd` → **`+0x94..+0x864` のみ**。`+0x40..+0x93`（pane slot）は**クリアしない**。→ **本 spec は「開くたびに 21 スロットを NULL 初期化」を要求（安全側）** `[方針]`。
- **`&generic;` = -1 は非束縛**: XML の `&generic;`（entity）は id `-1`。`SetFloat(tile, 0xFA8, -1)` は**格納される**が、`+0x94` 表の索引には使われない（`-1`）。background / toggle_left / toggle_right / slider_bar の 4 箇所が該当 `[確定(XML)]`。

---

## 4. 束縛層（`<id>N</id>` → `SetField`）`[確定(命令)]`

- XML の `<id>N</id>` は **trait `0xFA8` ハンドラ**経由で **`menu->SetField(N, tile)`** を呼ぶ。`SetField` = RaceSexMenu vtable `0xA6D38C` slot1 **`0x5C26D0`**（即値分岐・`ret 8`）。
- **6 束縛スロット**（XML 実値 → menu フィールド）:

| XML `<id>` | 対象 | SetField idx | menu フィールド |
|---|---|---|---|
| 1 | `race_window` | 1 | `+0x28` |
| 2 | `race_player_grab` | 2 | `+0x2c` |
| 10 | `race_name` | 0xA | `+0x30` |
| 20 | `race_scroll_bar` | 0x14 | `+0x34` |
| 21 | `vertical_scroll_marker` | 0x15 | `+0x38`（動的 id = `race_scroll_bar.user6`） |
| 90 | `race_button_done` | 0x5A | `+0x3c` |

- **id は保存される trait**: `SetFloat(tile, 0xFA8, id)` は「同値なら string trait 以外 skip」で格納（`0x58CA00`）。Main の `id=0` は既定 0 のため未格納、`&generic;` の `-1` は格納。
- **label は `SetName`（`0x58A020`）→ `Tile::name`（trait `0xBBA`, `this+8`）**。`user2` ではない。

### trait 表（出所: XOBSE `obse/obse/GameTiles.h` `eTileValue`、v634 §17 実読）`[確定(データ)]`

| id | 名前 | id | 名前 | id | 名前 |
|---|---|---|---|---|---|
| 0xFA1 | visible | 0xFA7 | alpha | 0xFA8 | **id** |
| 0xFAA | listindex | 0xFAB | depth | 0xFAE | **user0** |
| 0xFAF | user1 | 0xFB0 | user2 | 0xFB3 | user5 |
| 0xFB4 | user6 | 0xFB5 | user7 | 0xFB6 | user8 |
| 0xFDE | string | 0xFE1 | clicked（出所 XOBSE のみ・本 spec では未使用） | 0xBBA | **name** |

- 本 spec が実際に使う trait は `0xFA8`(id) / `0xFAE`(user0) / `0xFB3`(user5) / `0xBBA`(name) / `0xFB5`(user7) / `0xFB6`(user8)。`0xFE1`=clicked は XOBSE 出所のみで、本 spec の実装では使用しない `[確定(データ)]`。

### 名前引きヘルパ `0x5C3440` `[確定(命令)]`

- **`0x5C3440(this, str1, str2)` = `str1`（pane 名）で pane を引き（`0x5C3110`）、`0x589930` でその pane 内の tile を `str2`（tile 名）で検索して tile を返す**。`ret 0x10`（4 引数 = 2 つの BSFixedString）。
- 引数アクセス: `mov edi,[esp+0x2c]`(arg3) / `mov ebx,[esp+0x24]`(arg1)。`arg1` → `0x5C3110`（pane 名）、`arg3` → `0x589930`（tile 名）。
- family の各サイトは **`0x5C3440(paneName, tileName)` の戻り tile の `0xFA8` を arg1 と比較**する（§6.3）。本 spec の実装では **id 引きに置換**する（§0-5）。

---

## 5. XML の `<id>` は pane/item id と一致しない `[確定(XML)(negative)]`

- `race_sex_menu.xml` の numeric `<id>` = **{1, 2, 10, 20, 90, 100, 101, 102, 103, 104}**（10 件）＋ `&generic;` ×4。min=1 / max=104 / **連続でない**。
- **pane id（21）との一致 = 0 件**。code id は 0 と 200+、XML id は ≤104。
- item ↔ XML ブロックの対応は `<id>` ではなく **template（100..104）の実体化**で決まる。各 item は `race_template_button`(101)/`race_template_toggle`(102)/`race_template_slider`(103)/`race_template_text`(104) のインスタンス。
- `<parent>` 要素は 0 件。XML は pane 名を参照しない（pane はコード生成）。

### template 実値 `[確定(データ)]`

- `race_template_slider`(103): `user0`=copy(bar.user7) 現在値 / `user1`=0 / `user2`=Label / `user6`=-1(marker id) / `user7`=-1(editor id) / `user8`=-1(geometry/texture flag) / `user10`=0 / `user13`=true。
- 子 `race_template_slider_bar`: `user1`=0(**min**) / `user2`=1(**max**) / `user3`=0.04(**step**, xbox 0.05) / `user4`=0.1(**jump**)。
- → slider の範囲は **template 既定（0..1, step 0.04, jump 0.1）**。コードは `user7`（対象 morph）と label のみ個別設定。

---

## 6. 遷移表（2 層）`[確定(命令)]`

HandleClick `FUN_005cc200` の arg1 = **クリック要素の id**。分岐は 2 層。

### 6.1 head の arg1 分岐

```
arg1 == 90  → chain（0x5CC2A8）
arg1 == 10  → 0x5CC8AC（0x5C2730/0x5C30C0 の 2 call）→ family（0x5cc8ba）
arg1 == 98  → 0x5CCBF4: [0xB333C4].vtable[0x170]() → 0x526F00（リスト操作）→ 0x5C91E0 → finalize（詳細は V-642-1）
その他      → family（0x5cc8ba）
```
- `arg1==98` の内容 `[確定(命令)]`: `0x5CCBF9 mov ecx,[0xb333c4]` / `0x5CCC01 mov eax,[edx+0x170]` / `0x5CCC07 call eax` / `0x5CCC0B call 0x526f00` / `0x5CCC12 call 0x5c91e0` / `0x5CCC17 jmp 0x5cd9c5`。`[0xB333C4]` は Player 系。**id 98 は静的定数として存在しない**（`push 0x62` 全 `.text` 0 件）→ `[部分確定]`（V-642-1）。

### 6.2 層 1: chain（Done/Return の階層上昇）`arg1==90`

- ガード: **`GetFloat(this->tile, 0xFAE=user0) == GetFloat(paneN, 0xFA8=id)`**（現在ページ判定）。一致 → **`SetFloat(this->tile, 0xFAE, paneX.id)`**（ページ切替）。
- **19 サイトは直列チェーン**。各サイトは `mov eax/dx, [0xB38F38+idx*8]`（pane 名オブジェクト）→ `0x5C3110`（pane 引き）→ `GetFloat(this->tile, 0xFAE)` と `GetFloat(pane, 0xFA8)` を **`fcomp qword [esp+0x1c]; fnstsw ax; test ah,0x44; jnp <paneX commit>`** で比較する。**一致時のみ `jnp` で paneX の commit へ飛ぶ**（`jnp` 先が paneX を表す）。不一致なら素通りし、末尾 `0x5CC816` で **`SetFloat(race_button_done, 0xFB3=user5, 1.0)` → Main 復帰**。
- **19 サイトの実アドレスと `jnp` 先** `[確定(命令)]`: Main(`0x5CC280`→`0x5CCA77`) / Shape(`0x5CC2D6`→`0x5CCA09`) / Tone(`0x5CC32A`→`0x5CCA09`) / General(Face)(`0x5CC37E`→`0x5CC9C4`) / Brow(`0x5CC3C7`→`0x5CC9C4`) / Cheeks(`0x5CC411`→`0x5CC9C4`) / Chin(`0x5CC45A`→`0x5CC9C4`) / Eyes(`0x5CC4A4`→`0x5CC9C4`) / Forehead(`0x5CC4ED`→`0x5CC9C4`) / Jaw(`0x5CC537`→`0x5CC9C4`) / **Brow(`0x5CC580`→`0x5CC9C4`)** / Mouth(`0x5CC5CA`→`0x5CC9C4`) / Nose(`0x5CC613`→`0x5CC9C4`) / General(Skin)(`0x5CC65D`→`0x5CC97F`) / Beard(`0x5CC6A6`→`0x5CC97F`) / Eye shades(`0x5CC6F0`→`0x5CC97F`) / Eyebrows(`0x5CC739`→`0x5CC97F`) / Mouth(Lips)(`0x5CC783`→`0x5CC97F`) / Nose shades(`0x5CC7CC`→`0x5CC97F`)。
- **hub/leaf 構造**:

| 現在ページ（paneN） | サイト数 | 遷移先（paneX） |
|---|---|---|
| Main | 1 | （commit なし → 名前検証 = Done, `0x5CCA77`） |
| Shape, Tone | 2 | **Face** |
| Face 系（General(Face), Brow, Cheeks, Chin, Eyes, Forehead, Jaw, **Brow（重複）**, Mouth, Nose） | **10 サイト / 9 名** | **Shape** |
| Skin 系（General(Skin), Beard, Eye shades, Eyebrows, Mouth(Lips), Nose shades） | 6 | **Tone** |
| その他（Race, Hair, …） | 0 | （素通り → Main 復帰） |

- **Brow は 2 サイト存在するが重複** `[確定(命令)]`: `0x5CC3C7` と `0x5CC580` は**同一の pane 名オブジェクト `[0xB39008]`（registry idx 26 = Brow）を引き、`jnp` 先も同一（`0x5CC9C4` = Shape commit）**。両ブロックは 20 命令が完全一致（相対分岐の変位のみ相違）→ **別物ではなく重複**。Face 系の「10 サイト / 9 名」はこの重複を含む（コードの 19 行はこの 2 行を保持する）。

### 6.3 層 2: family（各タブボタンの遷移）`arg1 = ボタン id`

- ガード: **`GetFloat(0x5c3440(paneName, tileName), 0xFA8=id) == arg1`**。一致 → **`0x5c9650(this, paneX 名)`** = `SetFloat(race_button_done, 0xFB3=user5, 2.0)`（Return ラベル）＋ **`SetFloat(this->tile, 0xFAE, GetFloat(paneX, 0xFA8))`**（ページ切替）。
- **28 サイト**（直列チェーン、`jp` で連結）。定数は 2 スロットに分かれる: `[esp+0x48]`（guard 前・主に奇数）と `[esp+0x3c]`（一致分岐内・主に偶数）。合成で `3`〜`0x30` の連番 `[確定(命令)]`。用途は**未使用（デッド, V-649-1 確定負）**。

| pane（引く名前） | tile（引く名前） | fucompp | `[esp+0x48]` | `[esp+0x3c]` |
|---|---|---|---|---|
| Main | Age | `0x5CCD26` | `3` | — |
| Main | Face | `0x5CCF8B` | `9` | `0xa` |
| Main | Hair | `0x5CD136` | `0xf` | `0x10` |
| Main | Race | `0x5CD1B6` | `0x11` | `0x12` |
| Race | Gender | `0x5CCF22` | `8` | — |
| Hair | Red | `0x5CCD96` | `4` | — |
| Hair | Green | `0x5CCDF7` | `5` | — |
| Hair | Blue | `0x5CCE58` | `6` | — |
| Hair | Length | `0x5CCEB9` | `7` | — |
| Face | Shape | `0x5CD02A` | `0xb` | `0xc` |
| Face | Tone | `0x5CD0B0` | `0xd` | `0xe` |
| Shape | General(Face) | `0x5CD236` | `0x13` | `0x14` |
| Shape | Brow | `0x5CD2BC` | `0x15` | `0x16` |
| Shape | Cheeks | `0x5CD342` | `0x17` | `0x18` |
| Shape | Chin | `0x5CD3C8` | `0x19` | `0x1a` |
| Shape | Eyes | `0x5CD44E` | `0x1b` | `0x1c` |
| Shape | Forehead | `0x5CD4D4` | `0x1d` | `0x1e` |
| Shape | Jaw | `0x5CD55A` | `0x1f` | `0x20` |
| Shape | Mouth | `0x5CD5E0` | `0x21` | `0x22` |
| Shape | Nose | `0x5CD666` | `0x23` | `0x24` |
| Tone | General(Skin) | `0x5CD6EC` | `0x25` | `0x26` |
| Tone | Beard | `0x5CD772` | `0x27` | `0x28` |
| Tone | Eye shades | `0x5CD7F8` | `0x29` | `0x2a` |
| Tone | Eyebrows | `0x5CD87E` | `0x2b` | `0x2c` |
| Tone | Mouth(Lips) | `0x5CD904` | `0x2d` | `0x2e` |
| Tone | Nose shades | `0x5CD98A` | `0x2f` | `0x30` |
| Random | Face | `0x5CC92F` | — | — |
| Reset | Face | `0x5CCC87` | — | — |

- `[esp+0x48]` を書かない = **2 サイト**（Random / Reset）。`[esp+0x3c]` を書かない = **8 サイト**（Random / Reset / Main-Age / Hair×4 / Race-Gender）。後者 8 は一致時に `jmp 0x5cd9c5`（finalize）へ直行する。
- **切替先は `paneX` 名で確定**しているので、定数未確定でも表は完成する（§0-5 の id 引き実装では定数は不要）。
- **Random / Reset はページ遷移しない**（finalize 直行）: **Random = キャラの乱数生成（ランダム化）／ Reset = 初期化**。表示名と「ページ切替 commit を持たない」構造から `[部分確定]`。

### 6.4 遷移の補足

- **chain は Done/Return の「1 階層上へ」**。`0x5C9650` の `user5=2`（Return ラベル）と整合。`0x5c9650` は Main 以外のページでも到達する。
- **family は各タブボタンの遷移**。`user5` の Done/Return ラベル切替とは独立。

---

## 7. 実装指針（まとめ）

1. 名前は表示専用。引当・索引は id。`+0x40`（21 pane）と `+0x94`（500 id→tile）の 2 配列を持つ。
2. id は実行時採番（counter 200 起点、Main のみ 0）。`<id>N</id>` は `SetField` で 6 束縛スロットへ。
3. メニューを開くたびに **21 pane スロットを NULL 初期化**（exe は消さないが安全側）。
4. `&generic;`（-1）は非束縛。
5. 遷移は 2 層: chain（Done/Return・現在ページで分岐）＋ family（28 タブボタン）。
6. label は `Tile::name`（`0xBBA`）。trait `0xFA8`=id / `0xFAE`=user0（現在ページ）/ `0xFB3`=user5（Done/Return ラベル）。
7. `Element` は **descriptor**（`{id, user0_id, type, label, page_slot, value/min/max/step/jump, checked, widget}`）。identity は id（§0）。**widget 化は device-only の `RaceSexPanel::instantiate(TextRenderer*)`**（`app/src/main/cpp/ui/race_sex_panel_instantiator.cpp`）が行い `Element::widget` を埋める（Button→`UIButton`、Text→`MenuTextComponent`、Toggle→`UIToggle`、Slider→`UISlider`）。descriptor と widget を分けるのは `UIButton`/`MenuTextComponent` が `UISystem`/`TextRenderer` を引き込むため（コードベースの `menu_ui_builder`（descriptor・host 可） vs `menu_ui_instantiator`（device-only）と同じ分離）。slider/toggle の汎用 widget は既存に無いので新設（基底 `UIComponent`・命名は `UIButton` に準拠）。`add_*` のシグネチャは不変（パリティスクリプト `verify_race_sex_impl.py` は無変更）。

---

## 8. 残点

| ID | 内容 | 状態 |
|---|---|---|
| V-649-1 | family の定数 `[esp+0x3c]`/`[esp+0x48]` は **HandleClick 内に読者 0**、かつ **HandleClick が呼ぶ finalize の 3 関数（`0x7b4280`/`0x7c5e70`/`0x7c7eb0`）も戻りアドレスより上（`[esp+0x2c]`〜`[esp+0x50]`）を読まない** → **デッド（未使用）** | `[確定(命令)(negative)]` |
| V-642-1 | id 98（定数なし。`0xB333C4` vtable[0x170]→`0x526F00`→`0x5C91E0` の Player 系リスト操作） | `[部分確定]` |
| V-646-3 | fld 反復の増分定数の用途 | `[未確定]` |
| V-629-1 | `player[0xE8]==-1` の実行時裏取り（pane id には不要） | `[未確定]` |
| V-631-1b | `RaceSexPanel` は**どこからも参照されていない（未配線）**。実装 `app/src/main/cpp/ui/race_sex_panel.{h,cpp}` とテスト `app/src/main/cpp/tests/race_sex_panel_tests.cpp` は存在するが、`UICharacterCreation` 等への接続は未着手 | `[確定(データ)]` |

### V-649-1 の根拠（`[確定(命令)(negative)]`）

| 関数 | prologue の `[esp+NN]` 読 | 判定 |
|---|---|---|
| `0x7B4280` | `mov eax,[esp+4]` のみ（`[eax*4+0xb42f54]` を返す 3 命令） | 上を読まない |
| `0x7C5E70` | `mov ecx,[esp+8]`（arg1）のみ | 上を読まない |
| `0x7C7EB0` | `cmp byte [esp+4],0` / `cmp byte [esp+0x10],0`（arg1/arg2） | 上を読まない |

→ 定数はフレーム引数として渡されず、**書かれるが読まれない（デッド）**。spec の定数列は情報として残す（実装では不要）。

---

## 9. geometry（item の座標）`[確定(XML)]`

**item の座標はコードではなく XML template が与える**。builder は x/width/height を書かない（`push 0xFAD`(x) / `0xFCB`(width) / `0xFCA`(height) は builder 領域 `[0x5C4000,0x5CA000)` に **0 件**。全域の 47/57/73 件は `[0x57..0x59]` の汎用 tile 層）。座標は 5 template（pane `100` / button `101` / toggle `102` / slider `103` / text `104`）が持つ。item は template の実体化（§5）。

### 9.1 共通 item レイアウト（button / toggle / slider / text）

| 項目 | 式（XML template） |
|---|---|
| `x` | `0` |
| `width` | `parent().width`（= pane 幅） |
| `height` | `93` |
| `y` | `me().listindex * me().height`（= `listindex × 93`） |
| `repeatvertical` | `&true;` |
| `listindex` | **builder が `0xFAA` に設定**（XML の `<listindex> 0` は初期値） |

→ **1 列・行高 93・行間 0**（`y = listindex*93` で連続。列は無い）。`listindex`（`0xFAA`）は item 生成器が `SetFloat(tile, 0xFAA, GetFloat(parent, 0xFD0) - const)` で設定（button 生成器 `0x5C4480` の `0x5C4536..0x5C455B`。`0xFD0` は親 pane の値）。

### 9.2 pane（`race_template_pane` / `race_pane`, id `100`）

| 項目 | 式（XML template） |
|---|---|
| `x` | `0` |
| `width` | `parent().width - (me().childcount > 6 ? (me().childcount-1)*40 : 0)`（>6 でスクロールバー分を差し引く） |
| `y` | `0 - race_scroll_bar.user7 * 93`（`onlyif me().visible`）＝スクロールオフセット |

### 9.3 結論

- **item の geometry は XML template が与える**（コード生成ではない）。builder は id/label/`listindex`/user trait のみ設定する。
- 実装の `Element` に x/y/w/h を持たせる必要はなく、**描画・hit-test は template の式（`x=0`, `w=pane幅`, `h=93`, `y=listindex*93`）から算出**する。`listindex` は builder が `0xFAA` に書く値を使う。
