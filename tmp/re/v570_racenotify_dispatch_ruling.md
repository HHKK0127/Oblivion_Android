# v570 — V-561-1 クローズ: RaceSexMenu 通知ハンドラ `0x5CC200` の全分岐（90 以外の通知・Random/Reset の役割）（ai-todo #161 フォロー）

対象: `Oblivion.exe` v1.2.0416 (Steam)。根拠は命令実読（probe **v570_1〜v570_8**・8本 全 EXIT=0）＋ `bsa_menus\race_sex_menu.xml`（BSA 直抽出）。
先行: `tmp/re/v561_u5_done_button_ruling.md`（U5・V-561-1 の起票元）／`tmp/re/v567_u2_page_order_ruling.md`（ページ遷移表）
判定: `[確定(XML+命令)]` / `[確定(命令)]` / `[部分確定]` / `[未確定]`

---

## 1. 結論

| 問い | 判定 | 答え |
|---|---|---|
| このハンドラが比較する通知 id は何か | **[確定(XML+命令)]** | `cmp ebx` は **90 / 10 / 98 の3種のみ**。発生源は XML の `<id>`（`race_window`=1／`race_player_grab`=2／**`race_name` text=10**／`race_scroll_bar`=20／**`race_button_done`=90**／テンプレ 100-104）＝ **id10 = 名前入力テキストの通知 [確定(XML)]**。**id98 は race_sex_menu.xml に存在しない [確定(XML)]** → コード側生成 pane/行由来と推定（残点 V-570-3）。 |
| `0x5CC8BA` 以降（90 以外）の全分岐 | **[確定(命令)]** | **28 ブロックの線形 dispatch**（§3 の完全表）。各ブロックは「(要素名 strA, 親ページ名 strB, 準備 flag)」で `0x5C3440` により tile を取り、**通知 id とその tile の id trait（0xFA8）を浮動小数比較**し、`id < tile_id` → 次ブロック、`id >= tile_id` → **そのブロックの act**（不一致全滅は最終 epilogue・act なし）。 |
| `GetPage("Random")` / `GetPage("Reset")` の役割 | **[確定(命令)]** | **確認ダイアログの2系統**（§4）。Random = `sRandomizeFace`「Randomize face?」＋コールバック `0x5C9FC0` ＋1回限り flg `[0xB3B4C8]`、Reset = `sResetFace`「Reset face?」＋cb `0x5C9F70` ＋ flg `[0xB3B4C9]`。cb は**応答 `0x578D70` の al==2（Yes）で実行本体（Random=`0x5C9CD0`／Reset=`0x5C9C70`）へ jmp**、否なら flg を 0 に戻して ret。 |
| 90 以外の特別通知（10, 98）の内容 | **[確定(命令)]** | **id10**: テキスト入力ゲート `0x57D2F0([esi+0x8EC])==0` のときのみ受理 → `0x5C2730`（名前 tile の 0xFD3/0xFD4 を f2i して入力ウィジェットへ）→ `0x5C30C0`（**入力文字列を `race_name` の 0xFDE へ SetString**）→ dispatch へ。**id98**: `vfunc+0x170(player)` → `0x526F00`（`[esi+0x1FA]` 件の配列末尾操作＋vfunc+0x128）→ `0x5C91E0(esi)` → tail（顔フォーム系の再構築・内容は §5）。 |
| テールの2系統と flg の規則 | **[確定(命令)]** | §6。`0x5CD9C5`（ダイアログ act 系）は `[0xB3B4C9]` を無条件クリア、`[esp+0x13]==1`（ダイアログ表示）なら `[0xB3B4C8]` を**維持**、0 ならクリア。`0x5CD9D3`（Reset act 系）は `[0xB3B4C8]` をクリア。その後 player vfunc+0x154 → `0x7B4280` テーブル → `0x7C5E70`/`0x7C7EB0` [部分確定: UI 更新通知]。 |

---

## 2. エントリとゲート [確定(命令)]

```
0x5CC253  al = 0x57D2F0([esi+0x8EC])            ; テキスト入力ウィジェットの状態
0x5CC25A  ebx = [ebp+8]                          ; 通知 id（ret 8 の第1引数）
0x5CC25D  je 0x5CC8A3                            ; al==0（入力未アクティブ）→ id10 専用経路へ
        0x57DD90([esi+0x8EC], 0) + 0x5C30C0(this) ; 通常時前処理: 入力内容を race_name(0xFDE) へ反映
0x5CC277  cmp ebx,90 / jne 0x5CC8BA              ; 90 → ページ Done（v561/v567 で確定）、それ以外 → dispatch
0x5CC8A3  cmp ebx,10 / jne 0x5CC277              ; al==0 経路: id10 のみ受理、他は 90 チェックへ
        id10: 0x5C2730(this) → 0x5C30C0(this) → 0x5CC8BA へ直行
```

* **`0x5C30C0` の実体 [確定(命令)]**: `0x580120([esi+0x8EC])` の結果を **`SetString([esi+0x30], 0xFDE, …)`**（`race_name` の string trait）＋ vfunc+0x170 → `0x438DB0`。＝**「テキストボックスの内容を race_name へ同期」する共通前処理**（v561 §2.1 の「共通前処理」の実体）。
* **`0x5C2730` [確定(命令)]**: `[esi+0x30]` の **0xFD3 / 0xFD4 trait を GetFloat→f2i** して `0x57D2D0`/`0x57D2F0` 系で入力ウィジェットへ反映（名前欄の位置/状態同期。trait の XML 名は残点 V-570-4）。

---

## 3. dispatch の完全表（28 ブロック）[確定(命令)]

各ブロックの形:

```
strA = ページ/要素名（GMST 値）, strB = 親ページ名, prep = 準備 flag
tile = 0x5C3440(this, strA, strB, prep)   ; ret 0x10 = 4引数。中身 = GetPage(strB)(0x5C3110) → 0x589930(検索) [部分確定]
v    = GetFloat(tile, 0xFA8)              ; 0x588bd0 ret 4。0xFA8 = `<id>` trait
if (id >= v) → ACT else → 次ブロック
```

| No | strA | strB | prep | 分岐（less→次 / ge→act） | ACT |
|---|---|---|---|---|---|
| 1 | Random `sRandom` | Face | 1 | jp→`0x5CCBF4`（→id98 チェックへ） | **Random 確認**（§4） |
| 2 | Reset `sReset` | Face | 2 | jp→次 | **Reset 確認**（§4） |
| 3 | Age `sAge` | Main | 3 | jp→次 | `0x5C7070(ebx=id)` → `0x5C2F20` → tail |
| 4 | Red `sRed` | Hair | 4 | **jnp→`0x5CD9BC`** | **`0x5C5C30(1)`＝Style ページ act** → tail |
| 5 | Green `sGreen` | Hair | 5 | 同上（jnp→0x5CD9BC） | 同上 |
| 6 | Blue `sBlue` | Hair | 6 | 同上 | 同上 |
| 7 | Length `sLength` | Hair | 7 | jp→次 | `0x5CCFB7`（§6.2） |
| 8 | Gender `sGender` | Race | 8 | jp→次 | 0x5CCFB7 |
| 9 | Face `sFace` | Main | 9 | jp→次 | 0x5CCFB7 |
| 10 | Shape `sShape` | Face | 11 | jp→次 | 0x5CCFB7 |
| 11 | Tone `sTone` | Face | 13 | jp→次 | 0x5CCFB7 |
| 12 | Hair `sHair` | Main | 15 | jp→次 | 0x5CCFB7 |
| 13 | Race `sRace` | Main | 17 | jp→次 | 0x5CCFB7 |
| 14 | General(Face) | Shape | 19 | jp→次 | 0x5CCFB7 |
| 15 | Brow | Shape | 21 | jp→次 | 0x5CCFB7 |
| 16 | Cheeks | Shape | 23 | jp→次 | 0x5CCFB7 |
| 17 | Chin | Shape | 25 | jp→次 | 0x5CCFB7 |
| 18 | Eyes | Shape | 27 | jp→次 | 0x5CCFB7 |
| 19 | Forehead | Shape | 29 | jp→次 | 0x5CCFB7 |
| 20 | Jaw | Shape | 31 | jp→次 | 0x5CCFB7 |
| 21 | Mouth | Shape | 33 | jp→次 | 0x5CCFB7 |
| 22 | Nose | Shape | 35 | jp→次 | 0x5CCFB7 |
| 23 | General(Skin) | Tone | 37 | jp→次 | 0x5CCFB7 |
| 24 | Beard | Tone | 39 | jp→次 | 0x5CCFB7 |
| 25 | Eye shades `sEyesTex` | Tone | 41 | jp→次 | 0x5CCFB7 |
| 26 | Eyebrows | Tone | 43 | jp→次 | 0x5CCFB7 |
| 27 | Mouth(Lips) | Tone | 45 | jp→次 | 0x5CCFB7 |
| 28 | Nose shades `sNoseTex` | Tone | 47 | jp→`0x5CDA11`（**epilogue＝全滅・act なし**） | 0x5CCFB7 |

* **prep flag の並び**: 1..9 は連番、**10 以降は 2 刻み（11,13,...,47）**。act 時に `prep+1`（10,12,...,48）を `[esp+0x3c]` として `0x5CCFB7` へ渡す → **act ID = 1..9（特別 act 用）と 10,12,..,48**（その意味は残点 V-570-1）。
* **比較の方向 [部分確定]**: `test ah,0x44` + `jp`（id<tile）で次へ、fall-through（id>=tile）で act。**block4-6 だけ命令が反転（`jnp`→act / fall→次）だが意味は同一**。降順に並んだ id の線形探索として読むと整合（**gt で act が誤爆しない理由は id の一意性依存 [未確定]** → V-570-5）。
* **ブロック内の strA/strB は v567 のページ遷移表の親子関係そのもの**（Face→Main で Face を下げる、詳細→Shape/Tone、Race→Main...）＝ **「ボタン id → 親ページへ移動」が act の本体**（§6.2）。

---

## 4. Random / Reset の確認ダイアログ [確定(命令)]

```
act(Random): cmp [0xB3B4C8],0 → 既に1なら 0x5C9CD0(Random 実行) へ即実行
           未確認なら 0x579C10(text=[0xB39450]="Randomize face?", yes=[0xB38CF8], no=[0xB38D00],
                            flag=1, callback=0x5C9FC0) を表示、[0xB3B4C8]=1 → tail
act(Reset):  cmp [0xB3B4C9],0 → 既に1なら 0x5C9C70(Reset 実行) へ
           未確認なら 0x579C10(text=[0xB39448]="Reset face?", yes/no, flag=1, cb=0x5C9F70)
                     → [0xB3B4C9]=1 → 0x5CD9D3
```

**コールバックの実体 [確定(命令)]**（どちらも共通の骨格）:

```
0x5C9F70 (Reset cb): 0x589B70(0x40C) で RaceSexMenu を取得 → 0x5898F0 根探索 → 0x9832E6 キャスト
                   → 0x578D70() 応答を読む（al を返し [オブジェクト+0xB0]=0xFF にリセット＝読取と同時に消去）
                   al==2 (Yes) → jmp 0x5C9C70 (Reset 実行) / else → [0xB3B4C9]=0 → ret
0x5C9FC0 (Random cb): 同型 → al==2 → jmp 0x5C9CD0 (Random 実行) / else → [0xB3B4C8]=0 → ret
```

* **実行本体 `0x5C9CD0`（Random）[確定(命令)の骨格]**: RaceSexMenu 取得 → `vfunc+0x170(player)` → `0x5273A0(0,0,0)` → `0x5C91E0(esi)` → 顔データ取得 `0x5221C0`/`0x553B30` で **`[esi+0x880]`（age float）を計算保存** → user3(0xFB1) へ3系統 SetFloat → 終端で `0x578D70()==2` を再確認し Yes なら先頭へ jmp（**ポップアップ後の2段確認ループ**）、`[0xB3B4C8]=0` を保証して ret。`0x5C9C70`（Reset）は同型（別本体）。
* `0x578D70` [確定(命令)]: `0x582160(1,0)` から `[+0xB0]` を読み、再取得して `[+0xB0]=0xFF` にクリアし**旧値を返す**（al=2 が Yes）。

---

## 5. id98 と id10 [確定(命令)＋[未確定]の発生源]

* **id10 [確定(XML+命令)]**: XML `<text name="race_name"> <id> 10 </id>`（L49）＝**名前入力の通知**。ゲート `al==0` 時のみ `0x5C2730`→`0x5C30C0` で**入力文字列を `race_name`(0xFDE) に同期**してから dispatch へ。→ **「テキスト入力中の id10 は特別扱い、それ以外は通常経路」**という v561 の「共通前処理」の全容。
* **id98 [確定(命令)の act / 発生源は [未確定]]**: `cmp ebx,0x62` は block1 の less 側（`0x5CCBF4`）から到達。act = `vfunc+0x170(player)` → `0x526F00`（`[esi+0x1FA]>0` のとき `[esi+0x1F4]` の末尾要素＋`[esi+0xE8]+0x29C` を `0x5528F0` へ、vfunc+0x128 呼出）→ `0x5C91E0(esi)` → tail（Random 実行と `0x5C91E0` を共有する**顔フォーム再構築系**）。**XML に id98 の `<id>` は存在しない [確定(XML)]** → コード生成 pane/行の id 由来と推定（V-570-3）。
* id 1, 2, 20, 100-104 はこのハンドラに `cmp` が無く、**どのブロックとも一致しなければ epilogue（act なし）で抜ける**。

---

## 6. ACT 種別とテール [確定(命令)]

### 6.1 5 種の ACT

| act | 経路 | 内容 |
|---|---|---|
| Random 確認 | block1 ge → `0x5CC93C` | §4 |
| Reset 確認 | block2 ge → `0x5CCC90` | §4 |
| `0x5C7070` | block3 (Age/Main) ge → `0x5CCD2F` | `0x5C7070(ebx)` → `0x5C2F20` → tail（本体は残点 V-570-2） |
| Style 切替 | block4-6 ge → `0x5CD9BC`: `0x5C5C30(1)` | head 実測: `[0xB38FC0]="Style"` を str で GetPage → 0xFAE へ SetTrait ＝**Style ページへ切替** |
| ページ切替 | block7-28 ge → **`0x5CCFB7`**: str(strB)+act ID → **`0x5C9650(this, strB, …)`** → `jmp 0x5CD9C5` | **親ページ（strB）へ移動**（v567 の遷移表と同型。block28 の一致時 `0x5CD993` で実測、他は同一パターン [部分確定]） |

### 6.2 テール2系統と flg

```
0x5CD9C5: cmp [esp+0x13],0 / [0xB3B4C9]=0（無条件）
          [esp+0x13]==1（ダイアログ act）→ 0x5CD9DA（[0xB3B4C8] は維持＝1回限りflgを保持）
          ==0（ページ切替 act 系）→ 0x5CD9D3: [0xB3B4C8]=0 → 0x5CD9DA
0x5CD9D3（Reset act 直行）: [0xB3B4C8]=0 → 0x5CD9DA
0x5CD9DA 共通後半: player vfunc+0x154 → 0x7B4280(idx)=配列[0xB42F54]参照 → 0x7C5E70 / 0x7C7EB0(リスト走査→0x7C5F60) → epilogue ret 8
0x5CDA11: epilogue 直行（全ブロック不一致時・act なし）
```

* `[esp+0x13]` はダイアログ act（`0x5CC975`/`0x5CCBEA`）でのみ 1（初期化箇所は [部分確定]）。
* **`[0xB3B4C9]` のクリアは `0x5CD9C5` 経路のみ** → Reset act（`0x5CD9D3` 直行）では `0x5CD9CA` を素通りせず flg が残る可能性 → 実行本体 `0x5C9C70` 内のクリア有無は [未確定]（V-570-6）。

---

## 7. シグネチャ実測（ret N）[確定(命令)]

| 関数 | ret | 役割 |
|---|---|---|
| `0x5C3440` | `0x10`（4引数） | (this, strA, strB, prep) → tile。中身は GetPage(strB)→`0x589930` [部分確定] |
| `0x5C3110` | `8` | GetPage(this, str) |
| `0x588bd0` | `4` | GetFloat(tile, trait)（ST0 戻り） |
| `0x419B10` | `4` | 文字列 Set（src 1引数版・0x4028D0 の弟分） |
| `0x4028D0` | `8` | 文字列 Set(this, src, flag)（v556） |
| `0x578D70` | `0` | ダイアログ応答の読取＋クリア（al==2=Yes） |
| `0x589B70` | `8` | メニュー id→オブジェクト（`[0xB13974][id*4-0xFA4]`） |
| `0x5DA8C0` 等 | — | （v569 §4 参照） |

---

## 8. 移植への示唆

1. **Done(90) 以外の通知は「id の線形照合 dispatch」**で実装されている（比較は float 化した id vs tile の `<id>`）。移植の UI バスは同構造（switch）で足りるが、**act が「親ページへ戻る」型の大半（block7-28）**である点が v567 の遷移表と表裏一体。
2. **Random/Reset は 1 回限り flg の確認ダイアログ**（Yes=2）→ flg は act の成否で戻す設計。移植でモーダルを共通化するならこの flg の寿命（§6.2 の規則）を写す必要がある。
3. **入力中の id10 は「内容同期」act** に置き換えられる（`al` ゲート）。移植の IME 処理と原版の「テキスト通知」を対応させるなら `0x5C30C0` 相当（textbox→表示名の同期）が要点。
4. id98 の発生源が XML に無い以上、**移植側で 98 を発生させる場面を原版準拠で定義するには V-570-3 の確定が前提**。

---

## 9. 残点

| ID | 内容 | 重要度 |
|---|---|---|
| V-570-1 | act ID（`[esp+0x3c]` の 1..9／10,12,..,48）と `0x5C9650(this, str, id)` の引数契約・意味（ページ遷移 index か） | **中** |
| V-570-2 | block3 の `0x5C7070`/`0x5C2F20`（Age/Main 一致時）と block4-6 の `0x5C5C30(1)` の本体詳細 | 中 |
| V-570-3 | **id98 の発生源**（race_sex_menu.xml に該当 `<id>` なし。AddPane 生成ページ pane の id 設定値か行通知か） | **中** |
| V-570-4 | `0xFD3`/`0xFD4`（名前欄 trait）の XML 名と `0x57D2D0`/`0x57D2F0`/`0x57DD90` の詳細（テキスト入力の状態モデル） | 低 |
| V-570-5 | 比較の `gt` 経路で act が誤爆しない理由（通知 id と tile id の一意性・コード生成ページ pane の id 実値） | 低 |
| V-570-6 | `[0xB3B4C9]` のクリア経路（Reset act 直行時）・`[esp+0x13]` の初期化箇所 | 低 |

---

## 10. probe 一覧

`v570_1`（handler 全1713 insn・id cmp 3種と non-90 decode）／`v570_2`（entry ゲート・id10・tails・GMST 文字列13件・flags）／`v570_3`（`0x5C9CD0` 実行本体 400 insn・cb `0x5C9F70/0x5C9FC0` の完全実体・helper heads 12関数）／`v570_4`（ret N 実測・`0x5C3440` head・flag4・`0x5CCFB7`・`0x578D70`）／`v570_5・v570_6`（**28 ブロック機械抽出表**・push 0xfa8 基準の strA/strB 判別と next/ACT 分類）／`v570_7`（XML `<id>` 全列挙＝10個）／`v570_8`（XML name 36件・「98」検索 0 件・ページ名要素なし確認）。全 8 本 EXIT=0。
