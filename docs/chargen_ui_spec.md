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
6. **既定 UI は `Panel`（実験的）** `[方針]`: New Game は `RaceSexPanel` を既定で開く。`Legacy` は console `chargenpanel` で戻せる。**実験的**である理由: race の適用は `Player::raceFormID` のみで、**bonus/HP/spell/mesh は未適用**（`V-654-4`）。実機確認は人間ゲート。

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

### 6.3 層 2: family（各要素の遷移＝タブボタン＋一部 toggle/slider）`arg1 = 要素 id`

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
- **family は「タブボタン」に限らない**（toggles/sliders も含む: (Race,Gender) toggle、(Hair,Red/Green/Blue/Length)・(Main,Age) slider）。**Main の Eyes トグル（id 204）と Complexion スライダー（id 207）は family に無く、`FUN_005CC200` にメニュー側のハンドラも無い**（HandleClick 内の `[0xB38FA0]` 参照は 0 件、`[0xB38F90]` は chain の Eyes ページと family の (Shape,Eyes) のみ）＝ **no menu handler (v662)**。→ HandleClick は何もしない（family 全不一致は `0x5CDA11`=epilogue で return。**Main 復帰ではない**。Main 復帰は chain=`arg1==90` の末尾 `0x5CC816`）。**ただし「無反応」ではない**: **204 は toggle なので値の反転は widget 側の責務**（HandleClick の不在とは独立）、**207 は slider でドラッグによる値変更が正常動作**（クリックハンドラが無いのは当然）`[確定(命令)(negative)]`（v662 / 裁定書 §B.2）。
- **51 `push 0xFA8` サイトの内訳**（v662）: chain 現在ページ判定 19 + chain commit 先 3（Tone`0x5CC984`/Shape`0x5CC9C9`/Face`0x5CCA0E`）+ chain 末尾（Main 復帰）1（`0x5CC82E`）+ family tab 26 + Random 1 + Reset 1 = 51。spec の family「28」= 26 tab + Random + Reset なので `19+28=47`、残り 4 = chain 末尾 1 + chain commit 3。

### 6.4 遷移の補足

- **chain は Done/Return の「1 階層上へ」**。`0x5C9650` の `user5=2`（Return ラベル）と整合。`0x5c9650` は Main 以外のページでも到達する。
- **family は各要素（タブボタン＋一部 toggle/slider）の遷移**。`user5` の Done/Return ラベル切替とは独立。Main の Eyes トグル/Complexion スライダーは family 外＝無反応（§6.3）。

---

## 7. 実装指針（まとめ）

1. 名前は表示専用。引当・索引は id。`+0x40`（21 pane）と `+0x94`（500 id→tile）の 2 配列を持つ。
2. id は実行時採番（counter 200 起点、Main のみ 0）。`<id>N</id>` は `SetField` で 6 束縛スロットへ。
3. メニューを開くたびに **21 pane スロットを NULL 初期化**（exe は消さないが安全側）。
4. `&generic;`（-1）は非束縛。
5. 遷移は 2 層: chain（Done/Return・現在ページで分岐）＋ family（**28 = 26 要素 + Random/Reset**）。family はタブボタンに限らず一部 toggle/slider を含む。**Main の Eyes トグル（204）/ Complexion スライダー（207）は family 外＝no menu handler (v662)**（204 の toggle 値反転は widget 側の責務、207 は slider でドラッグ変更が正常。§6.3）。
6. label は `Tile::name`（`0xBBA`）。trait `0xFA8`=id / `0xFAE`=user0（現在ページ）/ `0xFB3`=user5（Done/Return ラベル）。
7. `Element` は **descriptor**（`{id, user0_id, type, label, page_slot, value/min/max/step/jump, checked, widget}`）。identity は id（§0）。**widget 化は device-only の `RaceSexPanel::instantiate(TextRenderer*)`**（`app/src/main/cpp/ui/race_sex_panel_instantiator.cpp`）が行い `Element::widget` を埋める（Button→`UIButton`、Text→`MenuTextComponent`、Toggle→`UIToggle`、Slider→`UISlider`）。descriptor と widget を分けるのは `UIButton`/`MenuTextComponent` が `UISystem`/`TextRenderer` を引き込むため（コードベースの `menu_ui_builder`（descriptor・host 可） vs `menu_ui_instantiator`（device-only）と同じ分離）。slider/toggle の汎用 widget は既存に無いので新設（基底 `UIComponent`・命名は `UIButton` に準拠）。`add_*` のシグネチャは不変（パリティスクリプト `verify_race_sex_impl.py` は無変更）。

---

## 8. 残点

| ID | 内容 | 状態 |
|---|---|---|
| V-649-1 | family の定数 `[esp+0x3c]`/`[esp+0x48]` は **HandleClick 内に読者 0**、かつ **HandleClick が呼ぶ finalize の 3 関数（`0x7b4280`/`0x7c5e70`/`0x7c7eb0`）も戻りアドレスより上（`[esp+0x2c]`〜`[esp+0x50]`）を読まない** → **デッド（未使用）** | `[確定(命令)(negative)]` |
| V-642-1 | id 98 の**経路は判明・意味は未確定**。`FUN_005CC200` の `0x5CCBF4 cmp ebx,0x62(98)` → `[0xB333C4] vtable[0x170]()` → `0x526F00` → `0x5C91E0`。`0x5C91E0` は player/animation 系で **tile trait を読まない**（`0x588BD0`/`0x588C10` 0 件）。98 が何を起動するかは未確定 | `[部分確定]` |
| V-646-3 | fld 反復の増分定数の用途 | `[未確定]` |
| V-629-1 | `player[0xE8]==-1` の実行時裏取り（pane id には不要） | `[未確定]` |
| V-631-1b | `RaceSexPanel` の配線は **row651 で完了**（`ChargenUi::Panel`、console `chargenpanel`、Done=`setOnDone`→close）。device 実機での表示・タップ確認は**未実施**（`ChargenUi::Panel` は dev トグル既定 OFF） | `[確定(データ)]` |
| V-652-1 | **close**。スクロールバーの hit-test と `user7`（`scrollRows`）の更新を実装（§9.7: `isScrollBarHit`/`beginScrollDrag`/`updateScrollDrag`、クランプ、ページ切替で 0 リセット） | `[確定(データ)]` |
| V-652-2 | **close**。`screen()` の canvas 1280x960 はアプリ既存規約（`ui_inventory_panel`）に追従して `[確定]` に上げ、描画側も同じ変換（`layoutScale`＋`panelOrigin`）を通した（§9.5） | `[確定(データ)]` |
| V-652-3 | マウスホイール/キーボードによるスクロールは**対象外**（原典未計測）。ドラッグ入力のみ実装 | `[未確定]` |
| V-653-1 | `[0xB39440]`（`race_name` の値の片方）の設定元。`.data` 実行時初期化で `.text` に絶対書き込み 0 件（§10.1） | `[未確定]` |
| V-653-2 | **選択 race の出所は判明**（Race トグル id 211、レンジ/文字列は `FUN_005C7BA0` が `[player+0xE8]+0xA8` のリンクリストから設定＝§10.4）。残るは `[0xB37D00]`（選択 race FormID）の**書き手**と、`0x5C28F0`（commit）/`0x5C9CD0`（Random 本体）の各 callee（`0x9832E6`/`0x5C91E0`/`0x984BCF`/`0x5221C0`/`0x553B30`）の役割 | `[部分確定]` |
| V-653-3 | 名前検証の成功/失敗時のダイアログ文言と `0x5C2BA0` callback の動作（§10.3） | `[未確定]` |
| V-654-1 | **close**。port に race 受け皿あり（一覧 `ESMManager::getAllRaces()` ＋ playable フィルタ `(flags & 1u)`、適用 `NpcManager::initializePlayerFromESM()` ＋ `Player::raceFormID`）。§10.5 | `[確定(データ)]` |
| V-654-2 | 確認文の冠詞（`[0xB38660]`/`[0xB38668]` = "a"/"an"）の判定ロジックは**未計測**。当面「先頭が母音なら an、他は a」の簡易規則 | `[未確定]` |
| V-654-3 | **close（v635）**。race トグル（template 102）の端挙動は **wrap**。XML `user0` の式が `<mod>`（`race_sex_menu.xml` L417-430）＝ clamp する要素が無い。**実装の `clamp` は原作とずれ**（`cycleRace`→`setSelectedRaceIndex`。`#195` 本体で wrap に直す） | `[確定(XML)]` |
| V-654-4 | **一部 close（v664）**。`Player` への race 適用は **属性（RACE `ATTR`）＋ skill bonus（RACE DATA）＋ racial spell（SPLO）を実装**（`Player::applyRace`、§10.8）。**残**: health（`V-664-3`）・mesh/morph（未適用）。`NpcManager::initializePlayerFromESM` は `NPC&` を取り `Player` は別 struct のため、`applyRaceToPlayer` を新設 | `[部分確定]` |
| V-660-A1 | **close（v663）**。Reset の既定値＝**raw 0**。`0x521E40` は `0x5538D0`（`[0xb39b80]+8`）→ `0x5528F0(src=テンプレート, dest=actor)` → `0x552880`（3 群 50/30/50 = **130** を `0.0`）の順で、**actor のモーフ値は最終的に 0.0**。対象は**全モーフ（カレントページ非依存）**。`-2.0` は Reset 経路に無い | `[確定(命令)]` |
| V-660-A2 | **close（v663）**。乱数源＝**CRT `rand()`**（`0x9859DD`、MSVC LCG `*0x343fd+0x269ec3, (>>16)&0x7fff`、[0,32767]）、`0x47DF80` 経由。raw=`rand()/32767*50+15`∈**[15,65]**（`0x5273A0` で clamp）、`value=(raw-15)/50`∈[0,1]。`user3`(0xFB1) は (Main,Age)/(Main,Complexion) へ 3 連書き（sentinel/値/0.0、最終 0.0） | `[確定(命令)]` |
| V-660-B1 | **close（v662）**。Main "Eyes" トグル（id 204）／Complexion スライダー（id 207）は **family に無く、HandleClick にメニュー側ハンドラも無い＝no menu handler**（`[0xB38FA0]` は HandleClick 内 0 参照）。**204 の toggle 値反転は widget 側の責務、207 は slider でドラッグ変更が正常**（HandleClick の不在とは独立）。family 全不一致は `0x5CDA11`（epilogue）で return（**Main 復帰ではない**）。family は **28**（26 tab + Random + Reset）で `static_assert(28)` は正しい | `[確定(命令)(negative)]` |
| V-660-C1 | **close（v663）**。Race 説明文の受け先＝**pane `[0xB38F78]`("Race") × tile `[0xB38FB0]`("Race Description")**＝Race ページ text **id 212**。`0x5C7BA0` が `0x43534544`("DESC") を読み `0xFB4`(user6) へ `SetString`（フォールバック `0xA36430`=" "）。race 名は別 tile | `[確定(命令)]` |
| V-664-2 | RACE **`ATTR`（16B）** レイアウト確定・実装済み: **16B = 男性 8u8 ＋ 女性 8u8**、`AttrIndex` 順 Strength..Luck（Oblivion.esm 15 レコード実測＋UESP 表一致）。`decodeRace` が populate（`RaceData::hasAttr`/`attrMale`/`attrFemale`）、`applyRace` が性別行を**代入**（#196/#204）。残: race 未選択時の戻し（#205） | `[確定]` |
| V-664-3 | RACE に **starting-health フィールドが無い**（`RaceData.startingHealth` は幻）。プレイヤー HP の導出元（Endurance 由来か）と **真の base 値（属性 30 か 40 か）** は未計測 | `[未確定]` |

**RE 待ち（ai-todo #179 に集約）**: `V-664-3`（HP 導出元・真の base 30 or 40）／morph・mesh 適用（`V-654-4` の残）。`V-664-2`（`ATTR` レイアウト）は解決済み（#196/#204）。health が揃うまで race 適用は **属性＋skill bonus＋spell に留まる**（§10.8）。

### V-649-1 の根拠（`[確定(命令)(negative)]`）

| 関数 | prologue の `[esp+NN]` 読 | 判定 |
|---|---|---|
| `0x7B4280` | `mov eax,[esp+4]` のみ（`[eax*4+0xb42f54]` を返す 3 命令） | 上を読まない |
| `0x7C5E70` | `mov ecx,[esp+8]`（arg1）のみ | 上を読まない |
| `0x7C7EB0` | `cmp byte [esp+4],0` / `cmp byte [esp+0x10],0`（arg1/arg2） | 上を読まない |

→ 定数はフレーム引数として渡されず、**書かれるが読まれない（デッド）**。spec の定数列は情報として残す（実装では不要）。

---

## 9. geometry（座標と hit-test）`[確定(XML)]`

**item の座標はコードではなく XML template が与える**。builder は x/width/height を書かない（`push 0xFAD`(x) / `0xFCB`(width) / `0xFCA`(height) は builder 領域 `[0x5C4000,0x5CA000)` に **0 件**。全域の 47/57/73 件は `[0x57..0x59]` の汎用 tile 層）。座標は 5 template（pane `100` / button `101` / toggle `102` / slider `103` / text `104`）が持つ。item は template の実体化（§5）。

### 9.1 共通 item レイアウト（button / toggle / slider / text）

| 項目 | 式（XML template） |
|---|---|
| `x` | `0` |
| `width` | `parent().width`（= pane 幅） |
| `height` | `93` |
| `y` | `me().listindex * me().height`（= `listindex × 93`） |
| `repeatvertical` | `&true;` |
| `listindex` | **builder が `0xFAA` に設定**（XML の `<listindex> 0` は初期値。§9.4） |

→ **1 列・行高 93・行間 0**（`y = listindex*93` で連続。列は無い）。

### 9.2 pane（`race_template_pane` / `race_pane`, id `100`）

XML 実値:

```xml
<width>
    <copy src="parent()" trait="width"/>
    <sub>
        <copy src="me()" trait="childcount"/>
        <gt> 6 </gt>
        <sub> 1 </sub>
        <mul> 40 </mul>
    </sub>
</width>
<y>
    <copy> 0 </copy>
    <sub> <copy src="race_scroll_bar" trait="user7"/> <mul> 93 </mul> </sub>
    <onlyif src="me()" trait="visible"/>
</y>
```

**閉形式**:

| 項目 | 式 |
|---|---|
| `width` | `parent().width - (childcount > 6 ? 40 : 0)` → 6 item 以下は **500**、7 item 以上は **460** |
| `y` | `- race_scroll_bar.user7 * 93`（非表示時 0）＝スクロールオフセット |

**`sub 1` の意味（重要）**: Oblivion の boolean は **`&false;`=1 / `&true;`=2**（正典 XML 229 行 `<user5> &false; </user5> <!-- 1 (false) for Done and 2 (true) for Back -->`）。`gt` は `&true;`/`&false;` を返す（UESP *Oblivion XML Reference/Operators*、逆アセンブル由来）ため、`<sub> 1 </sub>` が 2/1 を 1/0 に落とし、`<mul> 40 </mul>` で **定数 40px**（スクロールバー用右ガター）になる。

- **[確定(命令)(negative)] 誤読の記録**: `(childcount-1)*40` と読むと Nose（14 item）で `w = 500-520 = -20` となり、ページ全体がタップ不能になる。実機の Nose は操作できるため、この読みは**棄却**。
- **実装の追随 (row652)**: `menu_xml_interpreter.cpp` の `EvalValue::toNumber()` は Bool を **1/2** で返し、`childcount` は XML 宣言済みの直接子数を返す。これで同ファイルの式評価が本節の閉形式と一致する（従来は `&true;`=1 のため 500/540 を返していた）。
- pane の親は `race_window`（500x558）。ガターは pane の外ではなく **pane 幅を 40 削る**形で確保される。

### 9.3 スクロールバーと Done の位置（pane の外）

| 要素 | 親 | 束縛 | 位置（親ローカル） |
|---|---|---|---|
| `race_scroll_bar` (id 20) | `race_window` の子（21 pane の**兄弟**） | `+0x34` | `x=parent.w-20`, `w=20`, `y=25`, `h=parent.h-50` |
| `race_button_done` (id 90) | `race_background` の子（`race_window` の**兄弟**） | `+0x3c` | `x=parent.w-177-30`, `y=parent.h-44-20` |

- `race_background` = **560x740**（`user0`/`user1`、`generic_background.xml` の `_rounded_*` は tile_size 1 なので同値）、canvas 上の位置 `(70, (960-740)/2) = (70,110)`。
- `race_window` は background の **(30,85)**、500x558。
- Done は background ローカル **(353,676)** → **window ローカル (323,591)**。`323+177 = 500` で右端が window 幅に一致。
- **座標系の約束**: hit-test の入力は **window ローカル（pane 原点）**。Done も同じ系の矩形 `[323,500) × [591,635)` で判定する。

### 9.4 listindex（build 順の兄弟 index）

`0x5C4536..0x5C455B`（button 生成器 `0x5C4480`）:

```
push 0xfd0; call 0x588bd0        ; GetFloat(parent, 0xFD0)
fsub qword ptr [0xa2f928]        ; - 1.0 (double @0xA2F928 = 1.0)
push 0xfaa; call 0x58ceb0        ; SetFloat(tile, 0xFAA, parent[0xFD0]-1)
```

→ `listindex = parent[0xFD0] - 1` = **挿入時点の子 index**（0 起点、build 順）。`0xFD0` は親 pane の子数。

### 9.5 hit-test（実装仕様）

```
if (!visible)                    return -1
if (x,y) in Done rect            return 90    // §9.3, pane より先
w = 500 - (itemCount>6 ? 40 : 0)               // §9.2
if (x < 0 || x >= w)             return -1    // 右ガター = scrollbar
ey = floor(y + scroll_rows*93)                 // §9.1 の逆算
if (ey < 0)                      return -1
row = ey / 93
if (row >= itemCount)            return -1
return elementIdAt(row)                        // §9.4
```

- `scroll_rows` = `race_scroll_bar.user7`。実装は `scrollRows()` / `setScrollRows()` で、**入力への接続は未実装**（V-652-1）。
- 画面→ローカル: `scale = min(screenW/1280, screenH/960)`、`origin = レターボックス + race_window の canvas 位置 (100,195)`。`hitTestScreen()` が変換して `hitTest()` を呼ぶ。canvas 1280x960 はアプリ既存の XML 再構築（`ui_inventory_panel`）と同じ規約 **`[確定]`**（V-652-2 close）。
- 実装: `RaceSexPanel::hitTest` / `hitTestScreen` / `paneWidth` / `elementIdAt` / `itemCount`。host テストで固定。

### 9.5b 描画レイアウト（実装仕様）`[確定(データ)]`

**描画とヒットテストは同一の変換を共有する**（片方だけ canvas 座標のままだと必ずズレる）。

| 対象 | canvas 矩形 | device 矩形 |
|---|---|---|
| item (slot,row) | `(0, row*93 - scroll*93, pane_w, 93)` | `x = originX + cx*scale`, `y = originY + cy*scale`, `w = cw*scale`, `h = ch*scale` |
| Done (id 90) | `(323, 591, 177, 44)`（§9.3） | 同上 |
| scroll bar | `(480, 25, 20, 508)`（§9.3: `500-20`, `25`, `20`, `558-50`） | 同上 |

- `scale = layoutScale()`、`origin = (panelOriginX(), panelOriginY())`（§9.5 と同じ式）。
- **親は (0,0) 固定・size = 画面サイズ**。子は device 座標を直接持ち、`getAbsolutePosition()` が足す親オフセットは (0,0) なので二重加算にならない。
- `UISlider`/`UIToggle` の `render()` は `getAbsolutePosition()` を使うよう統一（`UIButton`/`MenuTextComponent` と同じ）。従来は `position` 直読みで、親原点が (0,0) 以外だとずれた。
- ラベルの `scale_` にも `layoutScale()` を掛ける（`UIButton::setLabelScale` / `MenuTextComponent::setTextScale`）。

**行カリング（クリップ代替）** `[確定(データ)]`:
- `UIComponent` にクリップ機構が無い（XML の `clipwindow`/`clips` は未実装）ため、窓外の行は `setVisible(false)` にする。
- `visibleRows = race_window.height / ROW_HEIGHT = 558/93 = 6`。XML の `race_scroll_bar.user8`（`<!-- How many items are visible at once -->`）も **6** で一致。
  - **user8 の由来** `[部分確定]`: `user8`(0xFB6) は「一度に見える項目数」で、原作はコードからも書く（リスト生成ヘルパ `0x5C93F0` が `0x5C9571 push 0xFB6` → `0x58CEB0 SetFloat` で設定）。XML の `<user8> 6 </user8>` はエディタ既定で、両者とも **6**。本実装は既定 6 を持ち、`setVisibleRows(int)` で XML 由来の値を差し込める（XML を実行時ロードするようになったら `race_scroll_bar.user8` を読む）。
- 可視条件: `slot == current_page_` かつ `floor(scroll) <= row < floor(scroll) + visibleRows`。
- scroll 変化・ページ切替のたびに再計算（`applyLayout()`）。
- **指示との差分**: 依頼時のテスト値「`visibleRows=7` で row 7 が非表示」は 1 大きい。`558/93 = 6`（python 検算）および `user8=6` に合わせて **6** を採用（row 6 が最初の非表示行）。

### 9.7 scroll 入力（実装仕様）`[確定(データ)]`

- `setScrollRows(int rows)`: `[0, max(0, itemCount(current_page_) - visibleRows)]` にクランプ。**値が変化したときだけ** layout hook（`setOnLayoutChanged` → `applyLayout()`）を呼ぶ。
- ドラッグ: `scrollBarRectDevice()` 内の touch down で捕捉（`beginScrollDrag`）。`Δy / (93 * layoutScale())` を行数に換算し、ドラッグ開始行に加算。移動は `updateScrollDrag`、離すと `endScrollDrag`。
  - **丸め規則**: 行数は **0 から遠い側への四捨五入**（`std::llround`、half-away-from-zero）。1 行 = `93 * layoutScale()` px。例: scale 1 で 93px→+1、46.5px→+1（半分行で切り上がる）、scale 1.125 で 104.625px→+1、93px→+1（0.889→1）。
- バーは `itemCount > SCROLL_VISIBLE(6)` のページにのみ存在（`isScrollBarHit` はそれ以外で false）。
- **優先順位**: renderer は touch down で**まずバーを判定**し、捕捉できたら `hitTestScreen`/`handleClick` に流さない（バー上で Done を踏まない）。
- **ページ切替で `scroll = 0` にリセット**（`user7` は 1 本の scrollbar を全ページが共有するため、前ページの値を引き継がない）`[部分確定]`。
- **ホイールは対象外**（原典未計測）→ `V-652-3`。

### 9.6 結論

- **`Element` に x/y/w/h を持たせない**。幾何は式（`x=0`, `w=pane幅`, `h=93`, `y=listindex*93`）から算出する。
- `renderer.cpp` の Panel 分岐は `hitTestScreen(x,y)` → `handleClick(id)`（`id>=0` のときのみ）。ページが変わったら `applyLayout()` で再配置・再カリング。
- **描画実装（row653）**: `RaceSexPanel::instantiate()`（device-only）が widget を作り、`applyLayout()` が §9.5b の矩形を適用して可視集合を決める。pane 外の Done / scroll bar も同じ変換を通す。`UIPanel` のタイトルバー/閉じるボタンは無効化（`setTitleBarHeight(0)`/`setCloseButtonVisible(false)`）。
- host テストは**矩形と可視集合**まで（pixel 比較はしない。device 目視は人間ゲート）。
- **scroll 入力（row654）**: `setScrollRows` のクランプ＋変化時 layout hook、`beginScrollDrag`/`updateScrollDrag`/`endScrollDrag`、バー優先の touch 分岐。ページ切替で scroll=0。ホイールは `V-652-3`。

---

## 10. chargen 確定経路（name / race / validation）`[部分確定]`

RE 2026-11-23（capstone, `D:\re\v655_*.py`）。対象関数: builder `FUN_005CA010`、menu handler `FUN_005CC200`（`ret 8`・vtable 経由）、**Random 本体 `0x5C9CD0`（v660 で同定。旧記載の「commit」は誤り）**、UI refresh `0x5C9650`、list helper `0x5C93F0`。

### 10.1 `race_name` の文字列（builder 末尾）`[部分確定]`

- `0x5CBC67`（`SetString(tile, 0xFDE, name)`）と `0x5CBC7A`（`SetFloat(tile, 0xFF0, 2.0)`; `0xA379B4`=2.0）は **build のたびに無条件で実行**（分岐で囲われていない）。
- `name` は 2 経路:
  - **A**（`0x5CBC3F`）: `playerName = [0xB333C4]->name`（`0x4DA2A0`）をそのまま使う。
  - **B**（`0x5CBC42`→`0x5CBC60`）: `playerName` を buffer `0xB3B4D0` にコピーし、`[0xB39440]` を使う。
- A/B の選択（`0x5CBBF1`〜）: フラグ `[0xB14500]` が非 0 → B。フラグ 0 のとき、`strcmp(buffer, playerName)==0` または `playerName == "Bendu Olo"`（`0xA6D3E8`）→ B、それ以外 → A。
- `[0xB39440]` / `[0xB3B4D0]` / `[0xB39438]` は `.data` の**実行時初期化グローバル**（`.text` に絶対書き込み 0 件＝間接設定）。`[0xB39440]` の設定元は **V-653-1**。
- **実装への含意**: `race_name` は「プレイヤーの現名 or 保存名」。空欄回避には builder 相当の「現名を入れる」処理が要る。
- **実装（row655）**: `RaceSexPanel::setPlayerName(name)` を追加し、`race_name` 相当のラベル（`name_widget_`）に player の現名を入れる。`[0xB39440]`（保存名）は**初期値には使わない**（`V-653-1` のまま）。位置は canvas `(250,-65)`（race_name は justify center / canvas x=350, y=130）。renderer は open 時に `playerController->getPlayer()->name` を渡す。

### 10.2 race 選択の player 反映`[部分確定]`

- `FUN_005CC200` は menu 0x40C の **message handler**（arg1 が要素 id / コード。`arg1==0x5A(90)` が Done）。vtable 経由で呼ばれる（直接 `call` 0 件）。
- **`0x5C9650` は player 書き込みではない**。呼び元は `FUN_005CC200`（`0x5CCFCC`）のみで、本体は tile の trait（`0xFB3`/`0xFAE`/`0xFAF`/`0xFB7`）を設定する **UI 更新（player→UI）**。
- **訂正（v660）**: `0x5C9CD0` は **Random 本体**（cb `0x5C9FC0` が `0x5C9FFC: jmp 0x5C9CD0`）であり「commit」ではない。本体は `FindMenu(0x40C)` → base tile → `0x9832E6` → player actor（vtable `+0x170`）→ `0x5273A0` → `0x5C91E0` → struct ctor（`0x984BCF`, `0x43ACE0`/`0x43EB30`）→ `0x5221C0` / `0x553B30` を呼び、**書く trait は `0xFB1`(user3) のみ**（裁定書 `tmp/re/v660_random_reset_eyes_race_ruling.md` A.1）。**confirmed 経路（commit）は `0x5C28F0`**。各 callee の役割は `V-653-2`。
- trait `user1`(0xFAF) は、**slider/toggle では「開始値/最小値」**（XML 578/636 行）であり、race editor id ではない。race の同一性は **要素 id**（spec §0）で運ばれる。→ 「user1 に race editor id が入る」前提の実装は不可。

**確定フロー（`0x5C28F0` / 比較の向き）** `[部分確定]`:

```
Done(arg1=0x5A) → 0x5CC280（user0 比較）→ 0x5CCA77（名前検証）
  → 成功: 0x579C10(callback=0x5C2BA0) で確認ダイアログ
     → 0x5C2BA0: FindMenu(0x40C)→base→0x578D70()==2(Yes) のときだけ jmp 0x5C28F0
        → 0x5C28F0: SetFloat(menu,0x1772,2.0); [0xB3B5D8]=1; [edi+0x864]→0x5FAEA0
           ; ecx=[0xB37D00]; eax=0x5E3320()  ; cmp ecx,[eax+0xC]; je 0x5C2ABC
```

- `0x5E3320()` = `[player_actor + 0x104]`（`0x4D7F80` ゲート後、`actor->vtable[0x170]()` の戻り `[+0x104]`）。
- **比較は「選択 race == 現 race」**: `je 0x5C2ABC`（**同じ**）→ race 変更処理を**スキップ**して finalize（`0x5222D0(actor,1)`, `0x57A7D0`, `0x522760`, `0x6626E0`, `0x5F2530`, `0x5F25F0`, `0x60D0A0`, `0x664700`, `0x6670C0`, `0x66C580`）。**違う** → `0x5C298C` で race を適用（`player->vtable[0x268]()`, `[edi+0x3C]` リストを `player->vtable[0x2E0]` で走査, `0x6A2280`）してから同じ finalize へ合流。
- → **「同じなら race 変更をしない／違えば適用」の両方を 1 つの `je` が担う**（合流点は共通の finalize）。

**`[0xB37D00]` の正体** `[部分確定]`:
- 静的サイトは **5 件すべて「読み」**（`cmp [0xB37D00], [obj+0xC]` 形: `0x51C095` / `0x52231F` / `0x5C297D` / `0x66275B` / `0x663Cxx`）。**絶対アドレスでの書き込みは 0 件**（`mov [0xB37D00], reg` / `mov …, imm32` ともに無し）→ 計算ベース（グローバル構造体のフィールド）経由で書かれる。
- 比較相手は actor/フォーム側の `+0xC`（`0x662750`/`0x663C50` はフォーム型ディスパッチを行う）→ **`[0xB37D00]` は race の FormID**（選択 race）。
- `0x51C090`: `if ([ecx+0xC] == [0xB37D00]) ret 4`（＝既にその race なら no-op にする SetRace 風ガード）。
- `0x5222D0`: actor メソッド（`(actor,1)` で呼ばれる）。外見/race の再評価で、内部でも `[0xB37D00]` と `[eax+0xC]` を比較。
- **port への含意**: `[0xB37D00]` の格納先を真似る必要はなく、**「選択 race（Race トグル）と player の現 race を比べ、違えば適用」**という意味だけ実装すればよい。

### 10.3 名前検証 `0x5CCA77`（`FUN_005CC200` 内）`[部分確定]`

- 入力名 `GetString(tile, 0xFDE)`（`0x588C10`）→ 長さと空白（0x20）スキャン。
- `strcmp(入力名, [0xB39440])`（`0x98539F`）が 0、**または** 空白フラグが立っていれば `0x5CCBB7`。
- `0x5CCBB7`: `0x579C10` を 5 引数で呼び、そのまま `ret 8`（**commit しない**）。
- 成功側（`0x5CCB0E`〜）: `sprintf("%s %s?" /*0xA6B4D8*/, 冠詞 /*[0xB38660]/[0xB38668]*/, 名前)`（`0x402E30`）を作り、`0x579C10` を callback `0x5C2BA0` 付きで呼ぶ。
- `0x579C10` は menu メッセージ送出系（内部で `0x582160`=FindMenu を反復）。→ **失敗時は通知/ダイアログで戻る（無視ではない）**。成功側も `"%s %s?"` から**確認ダイアログ**。厳密な文言・`0x5C2BA0` callback は **V-653-3**。
- **確定チェーン（実測）**: Done(`0x5A`) → 名前検証 → `0x579C10`（callback **`0x5C2BA0`**）→ Yes で `0x5C2BA0` → `0x5C28F0`（**commit**）→ 適用。`0x5C2BA0` は `0x578D70()` が 2 のときだけ `0x5C28F0` へ。`0x5C9CD0` は Random 本体（v660）。
- **実装（row656）**: `RaceSexPanel::handleDone()` が検証（`NameCheck` = Empty/HasSpace/Unchanged）→ `on_notify_`（通知のみ・commit しない）or `on_confirm_`（`"%s %s?"` = 冠詞＋名前）。Yes で `commit()`（`confirmed_=true` → `on_commit_`）。renderer は `showMessage(body, {"OK"})` / `showMessage(body, {"Yes","No"})`（`UIMessageBox`）に委譲し、`on_commit_` で **race（変化時のみ `player->raceFormID`＋`initializePlayerFromESM`）＋名前反映 → `setChargenUi(None)`**。冠詞は `articleFor()`（先頭母音で "an"、他は "a"／`V-654-2`）。race 適用は **`Player::raceFormID` のみ**（`NpcManager::initializePlayerFromESM` は `NPC&` を取り、`Player` は別 struct で呼べない／`V-654-4`）。

### 10.4 Race ページの中身（race 選択の実体）`[確定(命令)]`

Race ページ（pane id 209 / slot `+0x44`）は **2 toggle + 1 text**（§2 と一致）。builder の生成サイトは `0x5CA5C9` / `0x5CA5EB` / `0x5CA60B`:

| id | 種別 | label | builder サイト | 備考 |
|---|---|---|---|---|
| 210 | toggle | **Gender** | `0x5CA5C9`（label global `[0xB38FA8]`） | FAMILY `{Race, Gender, target=-1}` の 1 組 |
| 211 | toggle | **Race** | `0x5CA5EB`（label global `[0xB38F78]`） | **FAMILY に無い** |
| 212 | text | **Race Description** | `0x5CA60B`（string global `[0xB38FB0]`） | 説明文 |

- **race を選ぶのは Race トグル（id 211）**: `race_template_toggle` は `user4`=min index / `user5`=max index / `user0`=current index / `user6`=現在の文字列（コードが index から設定）。左右の子（`race_template_toggle_left`/`_right`）が index を増減する＝**リストではなく 1 個のトグルで race を巡回選択**する。`user1`=開始値（ビルド時の値）。
- **クリック処理**: FAMILY は `(Race, Gender)` の 1 組だけ（Gender トグル、target `-1`＝ページ切替なし＝finalize 直行）。**Race トグルは FAMILY に無い** → 本体クリックはページ遷移ではなく、左右の子ボタンによる **値変更（race index）** として扱われる。選択結果は commit 時に読む。
  - **実装（row656）**: クリック x がトグル幅の**左半分なら index-1、右半分なら +1**。端は **wrap**（`V-654-3` close／v635。XML `user0` が `<mod>`。**現行実装は `clamp` で原作とずれ**＝`#195` 本体で修正）。変化時に `applyLayout()` が race トグル label（`user6` 相当）と説明 text を更新。
- **text の文字列源**: `Race Description`（`race_template_text` の `<string>` をコードが設定）。race **名**はトグルの `user6`（index から引く）で、text は説明文。

**race 一覧の実体（item 2 の核心）** `[確定(命令)]`:

- **別メニューではない**。race 一覧はゲーム状態の**リンクリスト `[player+0xE8]+0xA8`** にある（各ノードの `+0x1C` が race 名）。
- builder 末尾の `0x5CBC8F call 0x5C7BA0`（`FUN_005C7BA0`）が Race トグルを埋める:
  - `0x5C7C20`〜: `player->vtable[0x170]()` → `[player+0xE8]+0xA8` のリンクリストを走査して**件数を数える**。
  - `0x5C7FBC`〜`0x5C7FD7`: `push 0xFB3` → **`SetFloat(tile, user5, 件数-1)`**（＝レンジ `0..件数-1`）。
  - `0x5C80EE`〜`0x5C8100`: `[node+0x1C]`（race 名。0 なら空文字 `0xA2F7EC`）→ `push 0xFB4` → **`SetString(tile, user6, race名)`**。
- したがって **race 選択は Race トグル（id 211）の左右巡回**で、レンジは「プレイヤーの選択可能 race 数」、表示は race 名。選択結果は commit 時に読む。
- `FUN_005C7BA0` はこのほか `0xFB3`/`0xFB4` を複数サイトで書く（`0x5C7BA0` の head 内 4+ sites）＝トグル群の範囲と文字列をまとめて設定する関数。

### 10.5 port 側の race 受け皿（item 1 の答え）`[確定(データ)]`

**両方あり**（Legacy UI `ui_character_creation.cpp` / `renderer.cpp` が既に使用中）:

| 用途 | API | 根拠 |
|---|---|---|
| race 一覧 | `ESMManager::getAllRaces()` → `std::vector<RaceData>` | `esm_reader.h:1665`、`ui_character_creation.cpp:715` |
| playable フィルタ | `(race.flags & 1u)` | `ui_character_creation.cpp:721`（原作の race リスト走査 bit `[TESRace+0x70]` bit0 = DATA byte32 bit0, RE v369 と同一） |
| race 適用 | `NpcManager::initializePlayerFromESM(player, raceFormID, classFormID, birthsignFormID)` | `npc_manager.h:77` |
| 選択 race の保持 | `Player::raceFormID` | `npc.h:31`、`renderer.cpp:924` |
| 名前の適用 | `player->name = character.name` | `renderer.cpp:922` |

- `RaceData` の実装状況（v664 実測＋#196/#204）: **`fullName` / `description` / `maleModelPath` / `femaleModelPath` / `spellFormIDs`(SPLO) / `skillBonuses`(DATA) / `flags` / `attrMale`/`attrFemale`(＋`hasAttr`) は populate される**。`attr*` は `ATTR`（16B = 男性 8u8 ＋ 女性 8u8、`AttrIndex` 順 Strength..Luck）から（#196）。一方 **`startingHealth` は未 populate**（health の導出元は `V-664-3`）。`skillBonuses` は `pair<uint8_t AV, int8_t bonus>`（AV 0x0C..0x20、`0xFF`=未使用スロット）。
- → **`V-654-1` はクローズ**。`commit()` は選択 race を適用できる（Legacy と同じ `player->raceFormID` ＋ `initializePlayerFromESM`）。

### 10.6 Random/Reset 本体・Main "Eyes" トグル・Race 説明文（v660 / ai-todo #175-#177）`[部分確定]`

詳細: `tmp/re/v660_random_reset_eyes_race_ruling.md`（probe `D:\re\v660_probe.py` EXIT=0）。

- **Random 本体 = `0x5C9CD0`**（cb `0x5C9FC0` が `0x5C9FFC: jmp`）、**Reset 本体 = `0x5C9C70`**（cb `0x5C9F70` が `0x5C9FAC: jmp`）。cb は `0x578D70()!=2` ならフラグ `[0xB3B4C8]`/`[0xB3B4C9]` を 0 にして ret（**キャンセル時は本体不実行＝状態不変**）`[確定(命令)]`。
- **Random 本体が書く trait は `0xFB1`(user3) のみ**（2 tile へ各 3 回、tile は名前引き）。顔モーフ軸（`0xFB5` 等）への直接書込は無い `[確定(命令)]`。
- **Reset 本体は trait を書かない**: `0x521E40(actor)` → `0x5C91E0(base)` → `0x5C7BA0(base,0)`。既定値は actor 外見側（`V-660-A1`）。
- **定数**: `15.0`/`50.0`（`(v-15)/50`）と `-2.0`/`0.25`/`0.0`（`(v+2)*0.25*x`）。乱数源は未同定（`V-660-A2`）。
- **ダイアログ**: `FUN_00579C10`（message_menu.xml）。ボタンは `button_1..button_10` = **id 4..13**。「Yes」は `0x578D70()==2`（コールバック側の規約）。
- **Main "Eyes" トグル（id 204）/ Complexion スライダー（id 207）**: `FUN_005CC200` の arg1 即値比較は **90/10/98 のみ**。以後は「各 pane の特定 tile の `0xFA8` と arg1 を比較するブロック列」で、**`0x5CC8BA` = Random ボタン**。**family（28）に (Main, Eyes)/(Main, Complexion) は無く、HandleClick にメニュー側ハンドラも無い＝no menu handler**（**204 の toggle 値反転は widget 側の責務、207 は slider でドラッグ変更が正常**。HandleClick の不在とは独立）。family 全不一致は `0x5CDA11`（epilogue）で return（Main 復帰ではない）。`V-660-B1` は **close** `[確定(命令)(negative)]`（v662 / probe `D:\re\v662_probe.py` EXIT=0）。
- **Race 説明文**: `0x5C7BA0` が RACE の **`DESC` サブレコード**（`0x5C81E5 push 0x43534544`）を読み、**`0xFB4`(user6)** へ `SetString`（`0x5C8205`/`0x5C8236`、フォールバック `0xA36430`=`" "`）。見出し（`sRaceDescription` GMST）は別。残は受ける tile の実名と割当（`V-660-C1` `[部分確定]`）。

### 10.7 Random / Reset の port 実装（v663 / ai-todo #175-#177）`[部分確定]`

詳細: `tmp/re/v663_random_reset_impl_ruling.md`（probe `D:\re\v663_probe.py` EXIT=0）。実装: `app/src/main/cpp/ui/race_sex_panel.cpp`（`doRandom`/`doReset`/`handleRandom`/`handleReset`/`rngNext`）、host テスト `race_sex_panel_tests.cpp`。

- **Face 4 ボタン** `[確定(命令)]`: builder `0x5CA726..0x5CA85B`（Face pane id 224）で **225 Shape / 226 Tone / 227 Random / 228 Reset**。port は `RANDOM_BUTTON_ID=227` / `RESET_BUTTON_ID=228`。
- **RNG = CRT `rand()`（MSVC LCG）** `[確定(命令)]`（`0x9859DD`）: `seed = seed*214013 + 2531011; return (seed>>16) & 0x7FFF`（[0,32767]、`uint32_t` 桁溢れ）。port は `rngNext()`。open 時（`init()`）に `std::time` でシード（原作 `0x47DF80(arg=0)` 経路）。`setSeed()` はテスト用。
- **`doRandom()`** `[確定(命令)]`: 全 **90 スライダー**を **builder 順**に `value = rngNext()/32767.0`。原作は `raw=rand()/32767*50+15`（`0x5273A0` で [15,65] クランプ）→ `value=(raw-15)/50` で、これは `rand()/32767 ∈ [0,1]` と同値。加えて (Main,Age)/(Main,Complexion) の 2 tile に **`user3`(0xFB1) 3 連書き**（sentinel `[0xa6d2d8]` → 算出値 → `0.0`、**最終 0.0**）。ページは Face のまま。
- **`doReset()`** `[確定(命令)]`（`V-660-A1`）: 全 90 スライダー `value = 0.0`（原作は actor のモーフ配列全体を 3 群 50/30/50 = 130 ぶん `0.0` 初期化、カレントページ非依存）。trait は書かない。ページは Face のまま。
- **確認ダイアログ** `[確定(命令)]`: Random `[0xB3B4C8]` / Reset `[0xB3B4C9]`。初回クリックで `on_confirm_`（Yes/No）を出しフラグを立て、armed 中クリックは本体直行（`0x5CCBE5`/`0x5CCCC8`）。**No は cb がフラグを 0 に戻す**（次回クリックで再度ダイアログ）。ページ離脱ではフラグをリセットしない（原作はグローバル）。**ダイアログ本文**は GameSetting `sRandomizeFace`＝**"Randomize face?"** / `sResetFace`＝**"Reset face?"**（v664）。`[0xB39450]`/`[0xB39448]` は文字列ポインタでなく **GameSetting オブジェクト**で、起動時 init thunk `0x9f7b70`/`0x9f7b50` が ctor `0x419a70(EDID, 既定値)` で構築し `0x981fb4` で登録する（`%s` 組み立てではない）。既定値リテラルは exe 内 `0xa63b40`/`0xa63b24` に実在し、`Oblivion.esm` には両文字列とも無い（生検索で未検出）ため exe 既定値が表示される。port はこの 2 文字列をそのまま使う（**`V-664-1` close**）。
- **`V-654-4` の範囲内** `[方針]`: morph への実際の適用は未配線（`Player` は `NPC&` を取る API を呼べない）。`morph → slider` の対応は未確定のため **builder 順 1:1 近似**で引き、値は descriptor の `value` に留まる（widget は `applyLayout()` が反映）。

### 10.8 race の Player 適用（属性＋skill bonus＋spell、v664）`[部分確定]`

実装: `app/src/main/cpp/game/player.{h,cpp}`（`applyRace`）、`game/npc_manager.cpp`（`applyRaceToPlayer`）、`engine/renderer.cpp`（commit 配線）、`ui/ui_character_creation.cpp`（Legacy base 統一）。テスト: `tests/race_sex_panel_tests.cpp`。

- **chargen base は Player の struct 既定が唯一の真実** `[方針]`: `BASE_ATTRIBUTE=40` / `BASE_SKILL=10` / `BASE_MAX_HEALTH|MAGICKA|STAMINA=100`（`player.h`）。`resetAttributes()`/`resetSkills()` は `Attributes{}`/`Skills{}`（＝既定）へ戻し、`applyRace()` は最初にこの base へ revert する。ただし **`BASE_ATTRIBUTE` は race 不在／`ATTR` 無しのフォールバック**で、race が `ATTR` を持つときはその値が base になる（下記）。**Legacy の `initializeDefaultCharacter()`/`applyRaceBonuses()` も同じ base を参照**（旧 50/15/100/50/100 の直書きを撤去。**Legacy 初期値が 40/10/100/100/100 に変わる**）。
- **適用順** `[方針]`: ①base へ revert（属性・skill・HP/MP/SP）②**RACE `ATTR` の base 属性を代入**（V-664-2。性別は `Player::isFemale()` で男性/女性行を選ぶ。`ATTR` 無しの race は 40 のまま）③race の skill bonus を加算（AV→skill は `Player::skillAvBindings()` の 1 表、AV 0x0C..0x20）④SPLO の spell を付与（**前 race の spell は除去**）⑤名前・raceFormID。**Gender トグルは race 適用の前に書く**（`applyRaceToPlayer` が `player.gender` で `ATTR` 行を選ぶ）。**gender 単独変更でも race を再適用**する（`chargen_commit.cpp`）。
- **条件** `[方針]`: **race が変わったときだけ**（`raceChanged()`、原作 `0x51C090` no-op ガード）。同一 race の再適用は revert のため二重加算しない（host テストで固定）。
- **RACE DATA skill bonus** `[確定(データ)]`: `DATA`（36B）byte0..13 = `7 × (int8 skillAV, int8 bonus)`（AV 0x0C..0x20、`0xFF`=未使用）。実測が既知値と一致（Redguard = Athletics/Blade/Blunt +10、HeavyArmor/LightArmor/Mercantile +5）。
- **未配線** `[未確定]`: health（RACE に starting-health フィールドが無い＝`V-664-3`）と真の base 値（30 か 40 か）。属性（`V-664-2`）は配線済み。`Player::RaceBonus` は属性（`hasAttr`＋`attrs[8]`、性別選択済み）／skillBonuses／spells を運ぶ。

### 10.9 Hair ページの toggle 214 Style / 215 Hair Color（v635 / ai-todo #201）`[確定(XML+命令)]`

詳細: `tmp/re/v635_style_haircolor_ruling.md`（Ghidra 12.1.4 + GhidraMCP、base `0x00400000`、`0x5C4340` のベース一致確認済み）。

- **214/215 は bool 反転ではなく index 巡回（wrap）** `[確定(XML)]`。`race_template_toggle`（template 102）の `user0`（current index）は XML の式で決まる:
  `user0 = ((clicked − left.clicked + right.clicked + user3) − user4) mod (user5 − user4 + 1) + user4`（`race_sex_menu.xml` L417-430）。左右の子が ±1、端は **`<mod>` による wrap**。既定 `user4=0`/`user5=1` のときだけ見かけ上 bool になる（`user1`=開始値、`user6`=コードが index から設定する現在文字列）。
- **通知 dispatch に 214/215 の行は無い** `[確定(命令)(negative)]`。`FUN_005CC200` の 28 ブロックで Hair に対応するのは 4 Red / 5 Green / 6 Blue / 7 Length のみ。クリックはメニュー側ハンドラに行かず、値変更は widget 側で完結（Race(211) と同じ構図）。
- **要素名は GameSetting 実体** `[確定(命令)]`: `0xB38FB8`(`sStyle`="Style") / `0xB39330`(`sHairColor`="Hair Color"、文字列 `0xA638D8`)。毎 tick の `FUN_005cdef0` が `PUSH 0xFAE`(user0) を 12 回読み、**Style は int のまま `[+0x870]`、Hair Color は int のまま `[+0x88C]`** へキャッシュする（bool に正規化されるのは Gender だけ）。
- **決定打** `[確定(命令)]`: Hair Color 適用 `FUN_005c5f00` が `user0` を **`[actor+0x8F0 + idx*4]` の添字**に使う（色テーブルの選択）。bool では成立しない。Style 適用 `FUN_005c5b00` も int のまま `[+0x870]` と比較する。
- **min/max はコードが供給** `[確定(命令)]`: `FUN_005c7ba0` に `PUSH 0xFB2`(user4) / `PUSH 0xFB3`(user5) があり、`sStyle`/`sHairColor` を参照する＝ Race トグル（§10.4）と同じく件数が入る。
- **id 順** `[確定(命令)]`: builder `FUN_005ca010` の生成順で **Style=214 / Hair Color=215**（`0x5CA652` が先）。port `race_sex_panel.cpp:148-149` と一致。
- **port の受け皿は無い** `[確定(データ)]`: `UIToggle` は `bool checked_` のみ（左右ヒット領域も index も持たない）。`add_toggle` は左右の子を生成しない。適用先（`[+0x870]`/`[+0x88C]`/`[+0x8F0]`）に相当する構造体も無い。→ `#195` 本体（`UIToggle` の index 化・wrap）＋ min/max 供給と適用先（別チケット）。

