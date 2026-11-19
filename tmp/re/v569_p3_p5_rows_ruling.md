# v569 — V-559-2 クローズ: StatsMenu p3（Skills）と p5（Misc）の行生成全分岐の完全確定（ai-todo #157 フォロー）

対象: `Oblivion.exe` v1.2.0416 (Steam)。根拠は命令実読（probe **v569_1〜v569_18**・18本 全 EXIT=0、うち試行・補助分あり）＋ `bsa_menus\stats_menu.xml`（BSA 直抽出）。
先行: `tmp/re/v559_statsmenu_tabs_ruling.md`（U3・V-559-2 の起票元）
判定: `[確定(命令)]` / `[確定(XML+命令)]` / `[部分確定]` / `[未確定]`

---

## 1. 結論

| 問い | 判定 | 答え |
|---|---|---|
| p3 = Skills の行生成（`0x5DC630`）全分岐 | **[確定(命令)]** | 21 スキル行を 1 ループで生成。**listindex はプリディケート `0x51C090` で 2 群**（真→0..、偽→C+1..、C=真の件数・**番号 C は空白**）。テンプレート instantiate 失敗時は `'Error creating skill item in Stats menu. Template...'`（`0xA6DB10`→`0x4A7A60`）をログ。§2。 |
| p3 の行フィールド対応 | **[確定(XML+命令)]** | §2.2 の表。**user1=1.0（`fld1`）／user2=進度 0..1（`0x65FCA0`）／user3=現在値／user4=名前／user5=アイコン（`_small` 挿入後）／user6=AV 索引（fild）／user7=base**。進行度バーは XML `temp_fill_bar` が `parent().user2/user1` を読む＝**EXE が上書きして初めて表示が動く**。 |
| p5 = Misc の行生成（`0x5DA930`）全分岐 | **[確定(命令)]** | **44 行**（helper 5＋ランク別 5（ループ）＋犯罪 6（ループ）＋単発 28）を完全列挙（§3）。**唯一の条件生成は SE Bounty 行**（`0x46B280("SEWorld")` != 0 のときのみ）。 |
| p5 の listindex 機構 | **[確定(命令)]** | helper 行は**明示引数 0,1,2,4,5**（**3 番は空白**）。以降は `add edi,1`=次行連続／`add edi,2`=**1 スロット空白**。SE 行を含むパスで **44 行＋15 空白＝59 スロット（0..58）＝全スロット数と完全整合**。§3.3。 |
| 列挙の整合検証 | **[確定(命令)]** | 44＋15＝59＝最大 index 58＋1。ループ trip 数（bucket 5＝`mov dword,[esp+0x14],5`、犯罪 6＝`0xB11E50..0xB11E68` を 4 刻み）と一致。 |

---

## 2. p3 Skills 行生成 `0x5DC630`（head = 0x5DC630・末尾 0x5DC947 ret）

### 2.1 全体構造 [確定(命令)]

```
[前パス] ebp=0..20:  form = 0x565BE0(2, ebp)            ; AV 索引（12..32、§4）
        P = 0x51C090( 0x5E3320([0xB333C4]) , form )     ; 恒等は [未確定] V-569-1
        P が真なら C++  (C = 真の件数)
        builder+0x30 の user3 ← float(C)                (0x5DC6BE-5DC6D2)
        C>0 なら C++（→ 偽群の開始番号 = C+1）
[行ループ] ebp=0..20 (cmp ebp,0x15 / jl 0x5DC6F0):
        slot = [builder+0x24 のカーソル]                ; 初期 &slot0、行毎 +4
        if slot が旧行を保持 → 旧行を vtable+0 (arg1=1) で解放  (0x5DC739-5DC745)
        row = 0x585410(builder, [builder+0x3c], "stat_skill_template", 0)   (0x5DC72A)
        row ← slot に保存                                (0x5DC74A)
        form = 0x565BE0(2, ebp) → [esp+0x28] へ保存      (0x5DC74C-5DC75D)
        icon = 0x446AF0([0xB33A98], ebp) → +0x24 の文字列をコピーし、
               '.' の直前に "_small" を挿入（strchr→tail+6→memcpy6）  (0x5DC761-5DC7C7)
        P 判定（前パス同型）で listindex カウンタを選択:
           真 → [esp+0x30]（初期 0）      偽 → [esp+0x14]（初期 C+1/C）
        SetFloat(row, listindex=0xFAA, カウンタ++)
        SetFloat(row, user3=0xFB1, vfunc+0x284([0xB333C4], form))     ; 現在値
        SetString(row, user4=0xFB2, 0x565CC0(form))                    ; スキル名
        SetString(row, user5=0xFB3, [esp+0x34] icon 文字列)
        SetFloat(row, user6=0xFB4, fild([esp+0x28]) = form（AV 索引）) ; internal
        SetFloat(row, user7=0xFB5, 0x5F1910([0xB333C4], form))         ; base
        0x5DA1A0(builder, form) → slot[form-12] の行に user1/user2/user3/user7（§2.3）
        [esp+0x1c] のテンプレ文字列を free (0x401F20)、slot カーソル +4、ebp++
[失敗経路] row==0 → 0x4A7A60('Error creating skill item in Stats menu. Template...')  (0x5DC909)
```

**listindex の 2 群（[確定(命令)]・v559 の未答）**: 行は元のスキル順ではなく「P が真の行 → 偽の行」に**グループ化**され、番号 C が**空白**になる。最大番号 = C+(21−C) = 21 → **22 スロット（0..21・空白 1）**。C=0 のときは偽群が 0..20（空白なし）。

### 2.2 行フィールド対応（XML 交差検証）[確定(XML+命令)]

| trait | id | EXE の書込み | XML（`stat_skill_template` :3099-3300） | 表示 |
|---|---|---|---|---|
| listindex | 0xFAA | 2 群カウンタ（§2.1） | y=70×listindex、listclip、**`stat_item_line` の filename=listindex mod 3** | 行位置＋罫線 3 型交互 |
| user1 | 0xFAF | **`fld1` → 1.0 定数**（`0x5DA715`） | `temp_fill_bar` の分母 `parent().user1` | 進行度バー |
| user2 | 0xFB0 | **`0x65FCA0(player, form)` float 0..1**（進度） | 分子 `parent().user2` | 同上 |
| user3 | 0xFB1 | vfunc+0x284(player, form)＝現在値（行ループ内＋`0x5DA1A0` の 2 重設定） | `stat_item_value` の文字列と、user7 との比較で色（lt/eq/gt → under/even/over） | 数値＋色 |
| user4 | 0xFB2 | `0x565CC0(form)` 文字列 | `stat_item_label` | スキル名 |
| user5 | 0xFB3 | icon 文字列（`_small` 挿入後、`[esp+0x34]`） | `stat_item_icon.filename` | 64×64 アイコン |
| user6 | 0xFB4 | fild(`[esp+0x28]`)＝**AV 索引そのもの**（`0x5DC75D` が form を保存） | `<!-- internal use only -->` 既定 0・**テンプレ内で未参照** | （表示なし） |
| user7 | 0xFB5 | `0x5F1910(player, form)`＝base | 色判定の基準 | 数値色 |

> user6 の記法はスタックスロット解析の帰結（§5）。EXE は AV 索引を 1 スロットに退避して使うが、XML は user6 を読まないため表示影響なし。

### 2.3 `0x5DA1A0`（共有 AV/進行度フィラー・6 呼出元）[確定(命令)]

* シグネチャ: `ret 4`（this=ecx＋AV 索引引数 1 個）。呼び出し元 6 箇所: `0x57A78F`(p1 系)／`0x5DC8CB`(本 p3 行ループ内・毎行)／`0x5DCC9C`・`0x5DCE34`・`0x5DCEE5`(タブ系 `0x5DCD70`)／`0x5DCEFC`。
* ルーティング: `arg==-1` → 全体リフレッシュ経路（ヘッダ AV ペア user3..user16 を builder メンバタイルへ＋全 21 スロット再設定）。`arg==8` → health（`0x5DA227`）、`9` → magicka（user8/user9/user13: `0xFB6/0xFB7/0xFBB` on `[esi+0x28]`）、`0xA` → fatigue（user10/user11/user14）、`0..7` → 属性別（`[esi+0x2c]`）、**`12..32` → `ebx=arg-12` で `row=[esi+ebx*4+0x60]`（＝p3 のスロット配列）を取り出し null チェック後に user1=1.0／user2=進度／user3=現在値／user7=base を設定**（`0x5DA6F4-0x5DA78F`）。
* **タイミング整合**: p3 行ループは row を slot に保存（`0x5DC74A`）した**後**に本関数を呼ぶため、初回ビルドでも `slot[form-12]`＝当該行が必ず存在（null スキップは発生しない）。
* `0x5DA794: cmp edi,-1; jne 0x5DA8AE(ret 4)` ＝単一スキル指定時は当該行のみ更新して即終了。

---

## 3. p5 Misc 行生成 `0x5DA930`（head = 0x5DA930・末尾 0x5DBAF4 ret・1336 命令）

### 3.1 行の作り方 3 系統 [確定(命令)]

1. **helper `0x5DA8C0`（`ret 0xc`・this=builder, 引数= listindex, value, label の順に push）** — 5 行。値は quest 系カウントと player フィールド。
2. **ループ 2 本** — ランク別 5 行（trip=`mov dword,[esp+0x14],5`）／犯罪 6 行（trip=`0xB11E50→<0xB11E68` を 4 刻み）。
3. **単発 28 行** — `push 0; push 'stat_misc_template'; push list; call 0x585410` 直後に label=GMST 文字列（`SetString user1=0xFAF`）、value=player フィールド（`SetFloat user2=0xFB0`）、listindex（`SetFloat 0xFAA`）。

### 3.2 44 行の完全表（SEWorld ありパスの listindex）[確定(命令)]

| # | idx | ラベル（GMST） | 値の出所 |
|---|---|---|---|
| 1 | 0 | Days Passed: `sMiscGameDaysPlayed` | `0x402C00([0xB332E0])` float（時計オブジェクト） |
| 2 | 1 | Active Quests: `sMiscActiveQuests` | `0x52A8A0` クエリ1（引数 1,0,0,buf）の計数 |
| 3 | 2 | Quests Completed: `sMiscQuestsCompleted` | `0x52A8A0` クエリ2（引数 1,1,0,buf）の計数 |
| 4 | 4 | Skill Increases: `sMiscSkillAdvances` | player+0x660 |
| 5 | 5 | Training Sessions: `sMiscTrainingSessions` | player+0x664 |
| 6-10 | 6-10 | Novice／Apprentice／Journeyman／Expert／Master Skills（`sMiscNoviceSkills`..`sMiscMasterSkills`・表 `0xB11E3C`） | 各ランク件数（`0x565BE0(2,idx)` → `0x5F23B0` のランク 0..4 を 21 回集計） |
| 11 | 12 | Bounty: `sMiscBounty` | `0x41FC90(player+0x44)` float（x87 経由 fistp） |
| 12 | 13 | Shivering Isles Bounty `sMiscSEBounty` | player+0x700 float・**条件行（§3.4）** |
| 13 | 14 | Fame: `sMiscFame` | vfunc+0x204(player) |
| 14 | 15 | Infamy: `sMiscInfamy` | vfunc+0x208(player) |
| 15 | 17 | Days Jailed: `sMiscDaysJailed` | player+0x658 |
| 16-21 | 18-23 | Items Stolen／Items Pickpocketed／Trespasses／Assaults／Murders／Horses Stolen（表 `0xB11E50..0xB11E64`） | player+0x6C8+4k（k=0..5） |
| 22 | 24 | Largest Bounty: `sMiscLargestBounty` | player+0x668 |
| 23 | 26 | Creatures Killed: `sMiscNumKills` | player+0x66C |
| 24 | 27 | People Killed: `sMiscNumPersonKills` | player+0x670 |
| 25 | 29 | Places Discovered: `sMiscNumPlacesDiscovered` | player+0x674 |
| 26 | 31 | Locks Picked: `sMiscNumLocksPicked` | player+0x678 |
| 27 | 32 | Lockpicks Broken: `sMiscNumPicksBroken` | player+0x67C |
| 28 | 34 | Souls Trapped: `sMiscSoulsTrapped` | player+0x680 |
| 29 | 36 | Ingredients Eaten: `sMiscIngredientsEaten` | player+0x684 |
| 30 | 37 | Potions Made: `sMiscPotionsMade` | player+0x688 |
| 31 | 39 | Oblivion Gates Shut: `sMiscOblivionGatesShut` | player+0x68C |
| 32 | 41 | Horses Owned: `sMiscHorsesOwned` | player+0x690 |
| 33 | 42 | Houses Owned: `sMiscHousesOwned` | player+0x694 |
| 34 | 43 | Stores Invested In: `sMiscStoresInvestedIn` | player+0x698 |
| 35 | 45 | Books Read: `sMiscNumBooksRead` | player+0x69C |
| 36 | 46 | Skill Books Read: `sMiscNumSkillBooksRead` | player+0x6A0 |
| 37 | 47 | Artifacts Found: `sMiscArtifactsFound` | player+0x6A4 |
| 38 | 49 | Hours Slept: `sMiscHoursSlept` | player+0x6A8 |
| 39 | 50 | Hours Waited: `sMiscHoursWaited` | player+0x6AC |
| 40 | 52 | Days as a Vampire: `sMiscDaysAsAVampire` | player+0x6B0 |
| 41 | 53 | Necks Bitten: `sMiscPeopleFedOn` | player+0x6B8 |
| 42 | 55 | Jokes Told: `sMiscJokesTold` | player+0x6BC |
| 43 | 56 | Diseases Contracted: `sMiscDiseasesContracted` | player+0x6C0 |
| 44 | 58 | Nirnroots Found: `sMiscNirnrootsFound` | player+0x6C4 |

* 行 1-5 は helper の**明示 listindex 引数**（`push` 0／1／2／4／5・`0x5DA95F`/`5DAA04`/`5DAA18`/`5DAA34`/`5DAA50`）＝**index 3 は空白**。
* `mov edi,6`（`0x5DAA5B`）以降は加算式（§3.3）。値が player フィールドの行は byte オフセットをそのまま `SetFloat user2`（int→float 変換）。

### 3.3 listindex 前進ルール [確定(命令)]

* **ループ**: bucket は trip 5×`+1` → 6..10 後に edi=11、その直後に `+1`（`0x5DAB52`）→ **Bounty=12（idx 11 空白）**。犯罪ループは trip 6×`+1` → 18..23。
* **`add edi,1` = 連続**／**`add edi,2` = 1 スロット空白**（y=70×listindex の XML なら空行）。
* **空白スロット（SE ありパス）**: 3, 11, 16, 25, 28, 30, 33, 35, 38, 40, 44, 48, 51, 54, 57（計 15）。**44 行＋15 空白＝59＝最大 58＋1 で完全整合**。
* **行 44（Nirnroots）だけは** create（`0x5DBA4F`）→ `add edi,2`（`0x5DBA54`）→ SetFloat の順で、**SetFloat 側が +2 を踏む**ため idx=58（57 空白）になる。他 43 行は「加算→create→SetFloat」または「create→SetFloat→次行へ加算」で、SetFloat 値＝create 時の edi と一致。

### 3.4 唯一の条件生成 [確定(命令)]

`push 'SEWorld'; add edi,1; call 0x46B280; test eax,eax; je 0x5DACA1`（`0x5DABE7-0x5DABF9`）— **`SEWorld` クエスト判定が偽なら、SE Bounty 行の create（`0x5DAC22`）とその `+1`（`0x5DAC9E`）がともにスキップ**され、以降の全 listindex が **−1** にシフト（空白位置も連動）。**非 SE パスの idx = 上表 −1（行 12 より後）**。他の 43 行は無条件生成。

---

## 4. シグネチャ実測（`ret N` ＝ callee-pop バイト）[確定(命令)]

| 関数 | ret | 用途（本裁定での判明分） |
|---|---|---|
| `0x565BE0` | `ret`（cdecl・caller `add esp,8`） | `(2, skillIdx)` → **AV 索引（12..32）**（`0x5DA6F4` の `av-12` 判定と 21 連番で確定） |
| `0x5E3320` | `ret`（0 引数） | `[0xB333C4]` から単体取得（null チェック用） |
| `0x51C090` | `ret 4` | プリディケート P（this＋form）。**意味は [未確定] V-569-1** |
| `0x5F23B0` | `ret 4` | スキルランク 0..4（bucket 集計） |
| `0x5F1910` | `ret 4` | AV の base |
| `0x446AF0` | `ret 4` | `(global 0xB33A98, skillIdx)` → icon 元文字列（`+0x24`） |
| `0x402C00` | `ret` | 時計系グローバルから Days Passed float |
| `0x52A8A0` | `ret` | フォームリスト列挙（quest 2 種の計数） |
| `0x65D270` | `ret 8` | AV の max 系 float |
| `0x5DA8C0` | `ret 0xc` | misc 行 helper（listindex, value, label） |
| `0x585410` | `ret 0xc` | instantiate（this, list, name, flag=0） |
| `0x58CEB0` / `0x58CED0` | `ret 8` | SetFloat / SetString |
| `0x5DA1A0` | `ret 4` | 共有 AV/進行度フィラー（§2.3） |
| `0x4028D0` | `ret 8` | 文字列 Set（v556）: `(this, src, flag)`・len/cap は `+4/+6` |

---

## 5. 手法メモ（ハーネスの罠・再発型）

1. **`capstone` の `op_str` に mnemonic は無い** — `re.match(r"mov +eax,...")` は 0 件。オペランドのみでマッチさせ、hex 桁数は `{3,6}`（`0xb38f70`=6 桁、`0xb1`=2 桁等を混在させる場合）。**レジスタ長は `{2}` 固定だと `edi` を逃す**（`[a-z]+` に）。
2. **push の保留（esp-shift）を無視するとスロット読み替えが起きる** — `push xxx` 直後の `[esp+K]` は実体 `[normal+K-4n]`。本件では:
   * テンプレ文字列オブジェクトは **normal `[esp+0x1c..0x23]`**（2 push 直後の `[esp+0x24..]`）、
   * **slot カーソル = normal `[esp+0x24]`**（`+4` は 1 push 保留中の `0x5DC8E5 [esp+0x28]+4` が実体 normal 0x24）、
   * **form/AV 索引 = normal `[esp+0x28]`**（1 push 保留の `0x5DC75D`）、
   * builder this = normal `[esp+0x2c]`（変化なし）。
   線形デコードだけでは `push ecx`（`0x5DC8D4`, 1 byte）が行表示から落ちることがある → **size 付き逐語ダンプ（v569_18）で必ず確認**。
3. **線形デコード開始点は必ず head か命令境界に**（`0x5DB700` 等の任意アドレス開始は mid-instruction リスク。本件は luck で整合したが、末尾は `ret` 位置で検証した）。
4. **静的登録バンドの逆引きは `push value; push name; mov ecx,g` の「g より前の最後の 2 push」**を取る（前後 0x40 の窓だと前後レジスタの登録が混入する）。デコードずれを避けるため **`push imm32`（68）のバイト走査**が最強。
5. **「行数×空白＝スロット数」のアリタナイ検証** — 列挙表の算術整合（44+15=59）を取ると、listindex 表の取りこぼしを機械的に検出できる。

---

## 6. 移植への示唆

1. **Skills タブは「分類別グルーピング」**（P 群 → 非 P 群・間に 1 空白）。タブ UI を素のスキル順にすると原版と並びが変わる。**P の意味（V-569-1）が出れば並び順を完全再現できる**。
2. 進行度バーは **user1=1.0 定数＋user2=0..1 浮動小数**という正規化構成（XML 既定 100/50 はダミー）。移植が既定値のまま使うと全バー 50% 固定になる。
3. 行の色は **現在値 vs base**（lt/eq/gt の 3 値）— user3/user7 の 2 系統が必須（p2 属性と同じ構造）。
4. **Misc の空白スロット**は「+2 加算＝1 行空け」の機械的規則。移植で行を詰めるなら原版の見た目（間隔）が変わる箇所が 15 箇所。
5. **SE Bounty 行の有無でその後の全行が ±1 シフト**する（Shivering Isles 有無で UI が動く点）。移植では条件を `SEWorld` クエスト判定に合わせるか、固定表示にするかを明示決める。
6. `Days Passed` は `[0xB332E0]` メンバ呼び出し、`Fame/Infamy` は vfunc+0x204/0x208 — **ラベル GMST と値 getter は非対応の別系統**（ラベルは static 登録グローバル、値は player フィールド／vfunc の並置）。1 つずつ対応表（§3.2）を移植のデータ表に転記すること。

---

## 7. 残点

| ID | 内容 | 重要度 |
|---|---|---|
| V-569-1 | プリディケート `0x51C090(0x5E3320([0xB333C4]), form)` の意味＝p3 の 2 群グルーピングと「真 C 件」表示（`builder+0x30.user3` の参照先 XML）の正体 | **中** |
| V-569-2 | p5 の空白スロット 15 箇所が「y=70 空行」で正しいか（`stat_misc_template` の y 公式未確認）と、空白が動的追加行の予約枠か | 低 |
| V-569-3 | `0x52A8A0` の 2 クエリ（引数 1,0,0,buf と 1,1,0,buf）の区別（Active vs Completed の機構） | 低 |
| V-569-4 | `0x565BE0` の第 1 引数 `2` の意味と `0x565CC0`／`0x65FCA0` の正確なシグネチャ群（float 戻りの x87 約定含む） | 低 |
| V-569-5 | `0x5DC8F3-5DC8FC`（`{ptr=builder, len/cap=bx}` をテンプレ文字列スロットへ書く 3 命令）の目的（次イテレーションの zero-init で上書きされるため単体では dead に見える） | 低 |

---

## 8. probe 一覧

`v569_1`〜`v569_18`（**18 本・全 EXIT=0**）。有効本: v569_1（head 特定）／v569_2（p3 全 decode 245 insns）／v569_3（p5 線形 901 行）／v569_5（helper `0x5DA8C0`・`0x5DA1A0` decode＋ラベル厳密化）／v569_7-8-9（行機械抽出・`mov edi` 対応）／v569_10（Nirnroots/0xB38500 特定・caller の E8 走査）／v569_11（**末尾 0x5DBAF4 まで延長・35 create サイト＝44 行・全ラベル同定**）／v569_12（`0x5DA1A0` 分岐ルート）／v569_13（trait push 全走査・caller 6 本）／v569_14（`0x4028D0` len/cap `+4/+6` 確認）／v569_15-16（AV ルーティング＝**`av-12` スロット到達と user1=1.0/user2=進度を確定**）／v569_18（size 付き byte 境界検証）。試行・補助: v569_4, 6, 17。
