# v250_port_saveslot_residuals4.md — 移植セーブ経路残点第4束（N-37..N-40 裁定）

- 作成: 2026-11-02 / TODO serial 243（V243・低）/ **分析専任（docs/GT/TODO/ソース無編集・EXE 読取のみ）**
- 出典本体: `tmp/re/v248_port_saveslot_residuals3.md` §6-2 新規残点 N-37..N-40（本体は読取のみ・無編集）。
- 入力（読取のみ）: `tmp/re/v248_port_saveslot_residuals3.md`, `tmp/re/v246_port_saveslot_residuals2.md`,
  `tmp/re/cache/vtable_index.tsv`, `tmp/re/cache/named_funcs.txt`, `tmp/re/cache/call_targets`,
  `docs/Oblivion_decompiled.c`（部分検索）。
- 手法: capstone 5.0.7 再逆アセンブル＋PE 手動解析（`v248_scanM.py` と同じ va2off/rd32/cstr/COL デコード枠を再利用）。
  ESM 本体は未アクセス。git 不使用。新規スクリプトは `%TEMP%\opencode\v250_*.py`。
- selftest: 着手時 `python "%TEMP%\opencode\probe_harness.py" selftest` → **ALL PASS (4/4)**。

---

## §1 項目1（N-37）: `0xB13970` の RTTI `NiTArray<PAVTile>` と登録側 `FUN_00585330`（vptr `0xA696DC`=Menu）の対応

**判定: 【解明】要素型名と格納実体は矛盾しない。配列に格納されるのは `TileMenu`（RTTI `.?AVTileMenu@@`、
vtable `0xA6AF14`）で、その RTTI 継承チェーンは `TileMenu : TileRect : Tile`（primary base 連なり、各 offset 0）。
すなわち `NiTArray<PAVTile>` に `TileMenu*` を入れるのは正当な基底クラスポインタ格納であり、テンプレート共有でも別名でもない。
一方 `Menu`（vtable `0xA696DC`）は RTTI 上 **基底の無い独立ルートクラス**で、要素として格納されたことは一度もない。
v248 が「登録側」と読んだ `FUN_00585330` は実際には **`Menu::~Menu()`（デストラクタ本体）**であり、
`mov dword[esi],0xA696DC` は MSVC デストラクタ定型の **vptr ダウンキャスト（ダイシング）書き換え**（要素の vptr 上書きではない）。
この関数は続いて `SetAt(this[+0x20]-999, &NULL)` でスロットをクリアする＝**登録解除**。
真の登録側は `FUN_00591C50`（`TileMenu` vtable slot5）で `SetAt((int)引数-999, &this)` に **TileMenu の this** を格納する。**

### §1-1 RTTI 継承チェーンの実測（`v250_scan1` S1 / `v250_scan2` A）

MSVC COL/CD/BCD を手デコード（本イメージの COL signature は 0＝VC6 系レイアウト。
BCD = `{pTypeDesc, numContained, PMD(mdisp,pdisp,vdisp), attr}`、CD baseArray は先頭に自己記述を含む）。

| 型 | COL | CD | baseArray | 内容 |
|---|---|---|---|---|
| `Menu` | `0xAC152C` | `0xAC1540` | 1 個 | **自己のみ**（基底ゼロ＝ルートクラス） |
| `Tile` | `0xAC132C` | `0xAC1340` | 1 個 | 自己のみ（ルート） |
| `TileMenu` | `0xAC1D98` | `0xAC1DAC` | 3 個 | 自己(nContained=2), **`TileRect`** (PMD mdisp=0), **`Tile`** (PMD mdisp=0) |
| `NiTArray<PAVTile>` | `0xAC140C` | `0xAC1420` | 1 個 | 自己のみ |

- `TileRect` の vtable は `0xA69138`（cache id 1541）、`TileMenu` は `0xA6AF14`（id 1540、プライマリ 7 スロット）。
- TileMenu の基底連鎖は全部 mdisp=0（primary base 経路）→ **`TileMenu*` と `Tile*` はポインタ値同一**。
  要素 4B のポインタ配列にそのまま格納でき、.upcast の調整は不要（v248 §2-2「要素サイズ 4B 固定」と整合）。

### §1-2 配列への書き込み箇所は全 3 箇所のみ（`v250_scan1` S3/S4）

.text 全体を 0xB13970 のリテラル（LE 4B）で走査 → **ヒット 6 箇所**（SetSize×2 / SetAt×3 / dtor thunk×1）で
配列オブジェクトの接触点は閉じている（v248 §2 の結論を再確認）。SetAt の引数順は再実測で確定
（`FUN_0042BB90`: `[esp+4]=index`、`[esp+8]=void* valuePtr`、`base[index]=*valuePtr`、`ret 8`）:

| 箇所 | 関数 | 動作 |
|---|---|---|
| `0x591D24` | `FUN_00591C50` = **TileMenu vtable[5]** の ctor 形末尾 | `SetAt(id-999, &esi)` — `mov [esp+0x18],esi` の計算から**格納値=this（TileMenu 実体）が確定** |
| `0x585372` | `FUN_00585330` = **Menu::~Menu** | `SetAt(this[+0x20]-999, &local(=0))` — NULL で**登録解除** |
| `0x591BCE` | `FUN_00591B70` = **TileMenu::~TileMenu**（slot0 thunk `0x5900F0` の実体） | メンバ `[esi+0x44]`（後述の内部 Menu）の virtual slot13 返値−999 に NULL を書く**登録解除** |

- `FUN_00585330` の中身（デコンパイル :290895-290939 と一致）:
  1. `*this = &PTR_00a696dc` — **デstructor 進入時の vptr を自分のクラス（Menu）へ差し替え**（C++ 基底デストラクタ定型。
     派生 ctor/dtor が先に自分の vptr を書くのに対し、基底 dtor は基底名義に戻すだけ）。
  2. `this[+0x14]` 退避 → `FUN_00582160`/`FUN_0057CFE0` で解放。
  3. `SetAt(this[+0x20] - 0x3E9, &NULL)`（`this[+0x20]` = メニュー ID。§1-4 参照）。
  4. `this[+4]` の所有参照を slot0 virtual dtor(1) で解放、`+8..`/`+0xC` のリスト走査解放（TileMenu 側の member リンク）。
- したがって **v248 の「登録側 FUN_00585330 が vptr 0xA696DC の実体を渡す」は読み方誤り**で、正しくは
  「`Menu` 系オブジェクトのデストラクタが self の vptr を Menu に戻しながら、自身の ID スロットを NULL 化して退会する」。
  要素の vptr が上書きされることはない（要素は値として入っているだけ。退会表現は NULL 化＝v248 §2-4 の想定どおり）。

### §1-3 `Menu`（0xA696DC）は要素ではなく `TileMenu+0x44` のメンバ（`v250_scan2` C / `v250_scan3` F1/F2 / `v250_scan4` G2）

- `FUN_00591C50`（登録関数）の流れ:
  1. `[esp+4] == 0xFA2`(4002) のときだけ処理（他型は return 0）。`fld [esp+0x10]` → `call 0x9828C0`（float→int）→ ebx = 初期 ID。
  2. `push 1; push ebx; call 0x587B20` —FUN_00587B20 は **初回のみ** 1B マーカ確保＋`0xB13970.SetSize(0x33=51)` する
     登録表イネーブラ（`[0xB3A708]` にマーカ保存。v248 §2-3 の 25 呼出し元の一つの姿）。
  3. `call 0x587D20`（ecx=マーカ、引数 ebx）= **メニュー ID→実体ファクトリ**。
     `0x587D44: add eax,-0x3E9; cmp eax,0x32; ja 失敗; jmp [eax*4+0x5887B8]` —
     **ジャンプテーブル 0x5887B8 は 0x33=51 エントリ**（case 0..50 = ID 999..1049。v248 §2-3 の SetSize(51) と厳密一致）。
     ケース例: `0x587D59: push 0x64; new; ctor 0x5BC450`（→ vptr `0xA6CE5C` の Menu 派生、100B）、
     別ケース `0x587BC0`: 192B を new し vptr `0xA6A09C` = **VideoMenu** を書く等、**51 種の Menu 派生具象クラス**を生成する。
  4. 生成物の vtable slot13（Menu vtable `[13] = 0x725DA0`、基底既定は `xor al,al; ret 8` の false スタブ。
     具象派生がオーバーライドして ID を返す）を呼んで ID を取得し、**`[inner+0x20] = id`**（＝~Menu が読むのと同じスロット）、
     `[inner+0x14]` にも派生値を退避。
  5. 浮動小数の境界比較（qword `0xA69768`/`0xA69778`）で ID を 0xFA5/0xFAB 系の代替へ補正後、
     `SetAt((int)ebx - 0x3E9, &this)` → **TileMenu 自身を登録**、`[esi+0x44] = 内部 Menu`。
- 裏取り: `TileMenu::~TileMenu`（0x591B70）は进入時に `[esi]=0xA6AF14`（自己 vptr 書込＝dtor 定型）→
  `[esi+0x44]` の slot13 で ID−999 を計算し SetAt(NULL) → 内部 Menu を slot0 dtor(1) で delete。
  `Menu::~Menu`（FUN_00585330）も同 ID で SetAt(NULL) する（二重解除で安全側の重複）。
- よって **v248 §2 が「menu コンテナ」と呼んだ用途推定は正しい**（メニュー ID 登録表）。
  RTTI 名が `PAVTile` なのは要素の静的型が `Tile*`（基底）だからで、実体は `TileMenu*`。

### §1-4 読み手（消費者）の実体（`v250_scan3` D / `v250_scan4` G4）

base `[0xB13974]`・size `[0xB1397A]` へのリテラル参照は .text で 5 箇所（＋dtor thunk）:

| 箇所 | 内容 |
|---|---|
| `0x589B70` | **`GetMenu(id)` 相当の正体**: `idx=id-0x3E9; if (idx<=0x32 && size>idx) return base[idx]; else return 0`（`ret` 2 分）。配列が **ID 999..1049 → TileMenu\*** の登録表であることの決定的証拠 |
| `0x57F35B` | size>6 を検査し `base[6]` の要素を `call 0x58FBA0` へ渡す／さもなくば `0x5903E0` |
| `0x5836A7`,`0x5836E6` | size>43 を検査し `base[43]+0xAC == 0` を条件判定（要素を TileMenu 構造としてフィールド直読み） |
| `0x5ACB43` | size>26 なら `base[26]` の vtable slot0 を `push 1` で呼ぶ（= scalar deleting dtor によるメニュー破棄） |

- 要素は基底 `Tile` のメソッド（slot0 dtor 等）＋派生フィールドオフセット込みで使われており、
  「`Tile*` 配列だが実際にはメニュー登録表」という v246/v248 の運用理解と矛盾しない。

### §1-5 裁定と [PORT] 含意

- **裁定**: 要素型名 `PAVTile` と格納型 `TileMenu*` は `TileMenu : TileRect : Tile` の基底連鎖で対応。
  `Menu`（ルート、無基底）は配列要素ではなく `TileMenu+0x44` の所有オブジェクト。
  「vptr 上書き」は発生していない（v248 観測の `C7 06 DC 96 A6 00` は ~Menu のダウンキャスト書込）。
  v248 §2-1 の「対応は本項の範囲外」は**本節で閉じる**。残点 N-37 は消滅。
- [PORT]: 移植側は「メニュー ID 999..1049 → `TileMenu*`（NULL=空）の 51 固定配列」として実装すれば足りる。
  格納型は基底ポインタ `Tile*` 相当の void ポインタで可。退会＝NULL 書込（~TileMenu/~Menu の二重解除相当を 1 回にまとめても可）。
  ID は浮動小数のしきい補正（0xA69768/0xA69778 と 0xFA5/0xFAB 置換）が伴うため、
  「整数 ID 直接 index」ではなく「登録時に変換した index を保存し、解除時同一スロットを NULL 化」の対で保つこと。

---

## §2 項目2（N-38）: console `save` だけが LoadFormHeader 表（param_4=true の 8B 表）を作る設計動機

**判定: 【解明（機構・動機の帰属まで）／小訂正あり】`param_4=true` の呼び出し元は retail 実バイナリで確定 3 箇所、
すべて開発者/QA 経路のみ。(1) console `save`（FUN_005023D0 の呼出し 0x5024BA）、(2) console `load`
（FUN_005025xx 系の呼出し 0x5025F7）、(3) **新発見: `TestAllCells`（TAC）全セル走破テストの定期保存**
（FUN_00466BE0 の呼出し 0x466D21、50 セルごとに `"Test All Cells %i.ess"` を保存）。
ゲーム機軸の保存（quicksave/autosave/メニュー保存）は全て param_4=0。
表は「読み出し消費者ゼロ」が全域スキャンで確定: マップ Find `FUN_004527A0` の call 命令は .text 全体で
**1 箇所（0x45AD39）のみ**、かつそれは登録関数 `FUN_0045AD00` の**内側**（挿入重複検査）だけ。
よって v248 §3-5 の候補 (b)「診断／レイアウト記録」が正で、その運用主体が QA ハーネス（TAC）であることが特定できた。
ただし表を**出力する**コードは retail に存在せず（登録と破棄のみ）、消費側は出荷されなかった internal ビルド用
スナップショットと読むのが妥当。v248 §3-4「console 経路のみ 1」は**「console と TAC のみ 1」に訂正**。**

### §2-1 param_4 の配線（`v250_scan5` K1）

`FUN_00465130`（保存）の push 順は `[esp+4]=param_1(=0)`, `[esp+8]=param_2(ファイル名)`, `[esp+0xC]=param_3(=0)`,
`ecx=this`, **先押し=`param_4`(bool) ＝「正常解釈フラグ」**。全 6 呼出し元の push 実測:

| 呼出し元 | 归属 | push 列（先頭=param_4） | param_4 |
|---|---|---|---|
| `0x5024BA`（FUN_005023D0） | console `save` | `setne cl`（引数解析成功フラグ）, &名前, 0 | **1**（通常） |
| `0x466D21`（FUN_00466BE0） | **TestAllCells 定期保存** | `push 1`, &"TestAllCellsNN.ess"（0x98208B で整形）, 0 | **1**（固定） |
| `0x466AF6`（FUN_00466AD0） | quicksave | `push 0`, "quicksave"(0xA3B144), 0 | 0 |
| `0x466BD0`（FUN_00466B70） | autosave | `push 0`, "autosave"(0xA3AAD4), 0 | 0 |
| `0x5D3246`（SaveMenu） | メニュー保存 | `push 0`, `push 0`, `push 0` | 0 |
| `0x5D3438` | メニュー保存別系統 | `push 0`, `push 0`, 名前 | 0 |

`FUN_00465860`（読込）の全 5 呼出し元: `0x5025F7`（console `load`、解析結果から bool 生成）= **1**、
`0x466B31`（quickload、"quicksave"）=0、`0x466D5F`/`0x5AE1E6`/`0x5B5DA8` = 0。
⇒ **param_4=true は「console save / console load / TAC save」の 3 経路に限定**。

### §2-2 TAC 経路の全容（`v250_scan6` L1/L2 / `v250_scan7` N1 / decompiled :52139-52215）

- コマンド表（v248 scanI 73 行目）: `TestAllCells / TAC`、flags=`0x10000`（argc=1）、handler `FUN_00507960`。
- `FUN_00507960`: `FUN_004FAE80` で整数引数 1 個を parse（0 なら −1 に正規化）→
  `mov ecx,[0xB333A0]`（ゲームシングルトン）→ `call 0x445DF0(0, mode)` でテストモード移行。
  モードフラグ `[x+0x51]/[x+0x52]` を見る分岐が続く（テスト状態機械）。
- セル移動完了コールバック `FUN_004433A0`（decompiled :52143）:
  `"Moving to exterior cell %08X (%i,%i) %s"` / `"… finished loading in %.02f seconds. Total test time: %02i:%02i:%02i"`
  を `FUN_0040FEC0`（コンソール/ログ出力）へ書き出し、末尾で `if (3 < param_2 && param_2 < 6) FUN_00466BE0(param_2)`。
- `FUN_00466BE0`（0x466BE0、L2 実測）: モード 5 で [0xB333A0]+0x34 のセル配列を一周（call 0x4D5A90 で次セル）、
  プラグイン 5 本分のセーブデータを `0x44A7D0` 経由で生成、グローバル `[0xB33C00]` をインクリメントし
  **`% 50 == 0` のセル通過時に限り** `_snprintf(buf, "Test All Cells %i.ess", n)`（call 0x98208B）→
  **`FUN_00465130(singleton, 0, buf, param_4=1)`**。
- ⇒ TAC は「全セルを巡回し、50 セルごとに詳細保存を打つ」出荷同梱の自動負荷テスト。
  **詳細表（formType→LoadFormHeader 索引＋レコード配置ログ）が付けられるのはこの QA 保存と手動 console 保存だけ**。

### §2-3 消費者ゼロの再証明（`v250_scan7` N2 / decompiled 全域 grep）

- マップ `Find`（`FUN_004527A0`、v248 §3-2 で特定）の `.text` 全体 E8 呼出し走査: この関数への call は
  **1 箇所（`0x45AD39`）のみ**、かつその所在関数は登録関数 `FUN_0045AD00`（挿入時の重複検査）。
  decompiled の該当行 :71772 `cVar2 = FUN_004527a0(param_1,&param_1);` も 1 件だけ。
- レコード配置ログ（表+4 の BSSimpleList）への書込口 `FUN_004531B0` の呼出し 28 箇所は全て
  `mov ecx,[ctx+0x40]; test ecx,ecx; je skip; push 名前; push サイズ; call 0x4531B0` の定型で、
  `[ctx+0x40]`（表ポインタ）が NULL の通常経路では一行も入らない。**読み出し（walk/print）は 0 箇所**
  （走査は破棄 `FUN_004530A0` のみ）。
- ログ実名のサンプル（`v250_scan6` L1）: `Plugin List`(0xA3A680) / `Global Variables`(0xA3AAE8) /
  `Temp Effects List`(0xA3AB5C) / `TES Class`/`Process Lists Class`/`Spectator Events`/`Sky/Weather`/
  `Quick Keys`/`HUD Reticle`/`Interface`/`Regions`(0xA3AF5C..0xA3AEFC) / `Save Game Header`(0xA3B1A0) —
  **セーブファイルのセクション名そのもの**であり、保存／読込の各書込ヘルパが自分の書いた区間を記録している。

### §2-4 裁定と [PORT] 含意

- 裁定: 設計動機は「**開発者 QA 経路专用の診断スナップショット**」（v248 §3-5 の候補 (b) の実証）。
  console 手動保存・console ロード・TAC 巡回テストだけが詳細記録を有効化し、
  retail には表を出力する消費者が同梱されていない（消費コードは未出荷と推定）。TAC 経路の発見により
  「進行中のゲーム状態をそのまま保存するため索引が要る」（候補 a）は否定された（TAC も console も
  保存内容自体は同一の書込経路で、索引の有無は出力バイトに差を作らない）。
- 訂正（v248 に対する差分）: §3-4「console 経路のみ param_4=1」→ **console save / console load / TAC
  定期 save の 3 経路**（TAC は `save` 経由ではなく `FUN_00465130` 直呼出し、名前 `Test All Cells %i.ess`）。
- [PORT]: 移植では `bDetailedSave` は**いかなるゲーム保存でも false 固定**で機能的に等価。
  TAC 相当を移植する場合は「50 セル間隔で詳細保存を打つ」フックだけ残し、表そのものは
  {formType→header リンク列表, (size, section name) ログ} の書込専用リングとして実装、消費側は不要。

---

## §3 項目3（N-39）: `save <name> [integer]` の第 2 引数（`0xB0A684 = {'Integer (Optional)',1,1}`）の意味

**判定: 【解明】第 2 引数は retail で**消費されない構文上の残骸**である。
(1) SaveGame と LoadGame は同一 ParamInfo ブロック `0xB0A678` を共有（2 エントリが同じポインタを指す）し、
(2) コマンド help 文字列は `SaveGame <filename>` / `LoadGame <filename>` と**単一引数しか宣言していない**、
(3) handler 両者（`FUN_005023D0`/`FUN_00502500`）はパース後に第 2 引数の値を一切読まない
（読まれるのは文字列バッファと「パース結果 dword」だけ。後者は param_4 bool の材料になるが、
それは整数値そのものではない）、(4) スクリプト関数としても opid 0x147/0x148・argn=2 で登録されるが
impl は同じ handler を指す。つまり `save foo 5` は `save foo` と挙動同一で、整数は全経路で破棄される。**

### §3-1 エントリ実体と help（`v250_scan9` Q2）

コマンド表 1 エントリ = `{+0x00 表示名, +0x04 エイリアス, +0x08 opid, +0x0C help, +0x10 flags, +0x14 ParamInfo*, +0x18 handler}`。

| エントリ | help 実測 | flags | ParamInfo | handler |
|---|---|---|---|---|
| `SaveGame`/`save` (0xB0BF38) | **`SaveGame <filename>`** | 0x20000 (argc=2) | `0xB0A678` | `0x5023D0` |
| `LoadGame`/`load` (0xB0BF60) | **`LoadGame <filename>`** | 0x20000 (argc=2) | **同じ `0xB0A678`** | `0x502500` |
| `TestAllCells`/`TAC` (0xB0BF88) | `Test All Cells (0 - stop, 1 - start, 2 - interiors, 3 - current world)` | 0x10000 (argc=1) | `0xB0A6CC {'Integer',1,0}` | `0x507960` |

- **対照実験として決定的**: TAC は argc=1 の Integer 引数を実際に消費する
  （`FUN_00507960` が parse 値を `FUN_00445DF0(0, mode)` の mode として渡す＝help の 0/1/2/3）。
  同じ parser・同じ ParamInfo 機構を使う SaveGame の整数だけが消費者ゼロ。
- ParamInfo 12B = `{const char* displayName, u32 kind, u32 optional}`（`v250_scan8` P3 実測、
  第 3 フィールドは名前に "(Optional)"/"(optional)" を持つエントリのみ 1: `{'String',0,0}` /
  `{'Integer (Optional)',1,1}` / `{'Float(optional)',2,1}` / `{'Integer',1,0}`。整合 8/8）。

### §3-2 パーサ FUN_004FAE80 の引数処理（`v250_scan8` P5 / decompiled :194575-194750）

- ループ回数は**ストリーム内の u16「実際に与えられたトークン数」**（`uVar2 = u16[*cursor]`）で、
  declared argc ではない（`save foo` なら 1 ループで終わり、optional int スロットは読み進めない＝
  v248 §4-3 の「トークン数で打ち切られる」を機構的に再確認）。
- kind 分岐（`*puVar13 = ParamInfo[i].kind`）:
  - `case 0` String: トークン文字列を `*puVar12`（呼び出し側のバッファポインタ配列要素）へ memcpy して NUL。
  - `case 1/0x17` Integer: `FUN_004FAB20`（式評価器）で解釈 → `FUN_009828C0`（float→int）→ `*puVar12` へ書く。
  - `case 2` Float: 同様の float 版。kind 0x5/0xa/0x12/0x1c/0x8/0x21/0x23... は form/ref/bool 等の特殊系。
  - 書込は全て `(*(param_6+8) >> 3 & 1)`（コンテキスト bit3「書込許可」フラグ）ゲート。
- kind 属性表 `0xB0A54C`（`v250_scan8` P4）: stride 8、byte0 = 式評価要否（String=0 / Integer=1 / Float=1）、
  dword@+4 = 内部トークン ID（kind+1 の連番 01..10）。v248 §4-3 の `mov bl,byte[kind*8+0xB0A54C]`
  （コンパイラ FUN_004FDE30 の型検査）が読むのもこの byte0。

### §3-3 handler が消費するもの（`v250_scan8` P1/P2 全文字追跡）

`FUN_005023D0`（save、0x130 バイト全体を逆アセ）が parse 後に触るスタック局所は:
1. `[esp+0x118..]` の 288B 文字列バッファ（`0x502422: lea ebp,[esp+0x120]` で parse へ渡した arg1 出力）—
   `strchr(buf,'.')`（0x982EC0）で拡張子位置を探し、dot があれば dort 手前で切り
   `lstrcatA(out, name)` → `lstrcatA(out, ".ess")`（0xA3A0D4 実測）、dot がなければそのまま結合
   （`lstrcpyA(out, "")` 0xA2F7EC＝空文字初期化 → v248 §1-4 の「空初期化→連結」実証）。
2. `cmp dword[esp+0x10],0; setne cl` — parse の**成否/個数 dword**（arg1 とは別スロット）。これだけが
   `FUN_00465130` の param_4（詳細表ゲート、§2）に化ける。
3. 結果文字列で `'Game Saved'`(0xA4BB18) / `'You cannot save right now'`(0xA4BAFC) を `0x579B60` へ。

**整数用のスロットは読まれない**（P1 のコードに 2 番目 parse 出力への参照が存在しない）。
`FUN_00502500`（load）も同一構造: 消費するのは文字列バッファ・bool スロット・
`'Game Loaded'`(0xA4BB34)/`'File not found.'`(0xA4BB24) のみ。
→ ユーザーが `save foo 5` と入力すると 5 はパースはされる（トークン数 2 なら Integer 分岐が走る）が、
handler が読み捨てた上でファイル名 `foo.ess` に保存される。**意味なし（inert）**。

### §3-4 スクリプト側の配線（cache opcode_table / v248 §4-3）

- opcode 表 idx 102/103: `SaveGame`(opid 0x147)・`LoadGame`(opid 0x148)、**argn=2**、
  `argdesc=0xB0A678`（＝コマンド表と同一 ParamInfo）、`impl=0x5023D0/0x502500`（＝同一 handler）。
- コンパイラ（FUN_004FDE30）は argc=2 をバイトコードへ埋め、kind 表で型検査するので
  `SaveGame "name" 5` というスクリプト句は**受理される**（Integer 式として）。
  実行時 VM も同一 impl を呼ぶため、§3-3 と同じく整数は捨てられる。
- idx 104 `TestAllCells`(opid 0x149, argn=1, impl 0x507960) — console とスクリプトで引数消費挙動が一致する
  正常例との比較材料になる。

### §3-5 裁定と [PORT] 含意

- 裁定: **解明**。第 2 引数 `{'Integer (Optional)',1,1}` は
  「LoadGame と共有された ParamInfo ブロックに宣言された、任意整数の**死んだスロット**」。
  retail のどの経路（console 実行・console parse・スクリプト実行）でも値は消費されない。
  由来の推測（**未確認**・断定しない）: 旧エンジン（Morrowind 系）の `SI`/スロット指定系の残構文、
  またはスロット番号/フォーマット版の将来用プレースホルダ。help が単一引数しか宣言しないことから、
  設計上も「想定 usage は `save <filename>`」で確定してよい。
- 訂正なし（v248 §4-3 の観察はすべて再現）。補強: ParamInfo 第 3 フィールドの意味＝ optional フラグ、
  handler 側では未参照（parse ループはトークン数で回り optional を見ない）＝ optional は宣言情報専用。
- [PORT]: 移植コマンド表では `save`/`load` を **argc=1（String 必須）** とするか、argc=2 を保つなら
  「2 番目は無視」と明記する（現物互換を取りたいなら宣言だけ同一 `{String,0,0},{Integer,1,1}` を残す）。
  整数引数がファイル名や詳細フラグへ影響すると誤移植しないこと（実測で全て inert）。

---

## §4 項目4（N-40）: `0x982D85` の正確な同定と vtable 末尾 2 連の意味

**判定: 【解明】`0x982D85` = 静的リンク CRT の **`_purecall`（純粋仮想関数呼び出しスタブ、R6025 報告口）**。
Ghidra FID の `__access` 衝突同定は誤り（命令形・参照先グローバル・レポート文字列の三点で否定）。
`0xA6969C`/`0xA696A0` の 2 連は **vtable `0xA69688`（NiTMapBase<char const*, XMLStorage*>、7 スロット）の末尾スロット [5]/[6]**
であり、独立 2 スロット vtable ではない。両スロットが `_purecall` であることは「基底抽象 NiTMapBase の純粋仮想 2 本」の
意味で、派生具象 `NiTPointerMap<u8, BSSimpleList<LoadFormHeader*>*>`（vtable `0xA3AA98`）では同じスロットに
実装（+0xC 調整 thunk）が入っていることを対照実測で確認済み。**

### §4-1 スタブの実体（`v250_scan11` M1/M3・`v250_scan12` T1/T3・`v250_scan13` U1）

0x982D85 の全命令（12 命令・末尾は別関数 0x982DAF に続く）:
```
push dword [0xBAA5EC]        ; ハンドラ変数（エンコード済みポインタ）
call 0x98BEEB                ; = __decode_pointer（cache named_funcs 一致。内部で IAT[0xA28150]=DecodePointer）
pop ecx / test eax,eax / je  ; ハンドラ無登録ならスキップ（[0xBAA5EC] 初期値 0 実測）
call eax                     ; 登録ハンドラ呼び出し
push 0x19 / call 0x98D5B8     ; = __NMSG_WRITE(25)
push 1 / push 0 / call 0x98BD5E  ; [0xB310A8]（__abort_behavior 相当の global）の bit 書込（and/not/or/ret の最小関数）
jmp 0x98BC6B                 ; = _abort（内部で [0xB310A8] bit0 → msg 0xA、fallthru で _raise/_exit 経路）
```
- **決定的証拠（メッセージ文字列）**: `__NMSG_WRITE` はテーブル `[0xB311E8]`（23 エントリ `{msgnum, strptr}`）を
  線形検索する（`cmp ebx, [edi*8+0xB311E8] … cmp edi,0x17`）。エントリ [9] = `num=0x19, str=0xAA4CE4` の実文字列が
  **`'R6025\r\n- pure virtual function call\r\n'`**（ascii 実読出）。VC CRT の R6025 は purecall 専用番号。
- `[0xBAA5EC]` への書込み箇所は .text 全体で **1 箇所だけ**（`0x98DDEC: mov eax,[esp+4]; mov [0xBAA5EC],eax; ret`）＝
  `_set_purecall_handler` の本体形。読み手は本スタブ（0x982D87）と当該 setter の 2 箇所（ cmp edi,0xbaa5ec は
  near-data false positive 寄りだが、いずれにせよ purecall handler の单一所有が成立）。
- 「735 箇所中 722 箇所が直前 dword が .text ポインタ/同スタブ = vtable スロット列の内側」（U2）。
  汎用ファイルアクセス関数 `__access` が vtable スロットに 722 回も登場するのは不可能で、
  **純粋仮想スロット専用スタブの分布そのもの**。FID 衝突は関数先頭バイト列の偶然一致と判断する（`__access` 説は棄却）。

### §4-2 2 連スロットの構造的位置（`v250_scan11` M5）

| VA | 値 | 帰属 |
|---|---|---|
| 0xA69684 | `0xAC1454` | COL_B（NiTMapBase<...,XMLStorage*>） |
| 0xA69688..0xA69698 | dtor/hash/keyeq/setter/noop | vtable_B slot0..4 |
| **0xA6969C / 0xA696A0** | `0x982D85` ×2 | **vtable_B slot5/slot6** |
| 0xA696A4 | `0xAC149C` | COL_C ＝ 0xA69688 + 7×4（隙間ゼロ） |
| 0xA696A8..0xA696B8 | dtor/hash/keyeq/setter/noop | vtable_C slot0..4 |
| **0xA696BC / 0xA696C0** | `0x982D85` ×2 | **vtable_C slot5/slot6** |
| 0xA696C4 | `0xAC14E4` | COL_D（0xA696C8 の vtable の前置詞） |

- cache vtable_index のスロット数（`0xA69688`＝7、`0xA696A8`＝7、`0xA3904C` 系＝7）と境界が完全一致し、
  「末尾共有か独立 2 スロットか」の問いは **末尾共有（slot5/6）** で確定。
- 抽象/具象の対照（`v250_scan11` M5 / `v250_scan12` T2）:
  - 基底段階 vtable `0xA3904C`（NiTMapBase<..., XMLStorage*> 抽象側）: [5]=[6]=`0x982D85`（purecall）。
  - 派生具象 `0xA3AA98`（NiTPointerMap<u8, BSSimpleList<LoadFormHeader*>*>、§2 で登場）:
    **[5]=`0x4F0F60` = `add ecx,0xC; jmp 0x44D810`、[6]=`0x4B2ED0` = out 初期化 + `add ecx,0xC; jmp 0x44D880`** —
    +0xC のメンバ副物体へ adjust する thiscall thunk。purecall は上書きされ、この時点で抽象スロットの意味が
    「具象側が実装を持つ 2 とおりの純粋仮想」であることが動作形で確認できる。
  - したがって 0xB13948/0xB1395C が使う vtable_B/C の [5]/[6] は「未実装の純粋仮想枠」であり、
    実バイナリで呼ばれたことは無い（呼べば R6025 で abort）。意味（関数名）は RTTI にも残らない → 軽微な残点 R-1 として降ろす。

### §4-3 [PORT] 含意

- 移植側は _purecall 相当を呼ぶ機構そのものを持たない（仮想表を源码から起こすなら slot5/6 は
  「未実装＝起動時 assert」で十分）。v246 N-36 系譜の誤読（slot+4/+0x1C）に続く「slot5/6 の中身」問題も
  **vtable 境界・purecall ともに閉じた**。
- 保存セーブ経路（v248 §2〜§5）の全スレッドで残っていた「.text 後方 CRT 帯のポインタが vtable に現れる」不審は、
  全て MSVC 静的 CRT の purecall / decode_pointer / NMSG_WRITE / abort の部品として説明が揃った。

---

## §5 まとめ・新規残点・自己検証

### §5-1 N-37..N-40 の裁定まとめ

| ID | 判定 | 要点 | 実装要否 |
|---|---|---|---|
| N-37 | **解明** | 要素は `TileMenu*`（RTTI チェーン `TileMenu : TileRect : Tile`、mdisp すべて 0）。`Menu` は無基底ルートで**要素ではない**（`TileMenu+0x44` の所有内部 Menu。ID は内部+0x20）。`FUN_00585330` は `Menu::~Menu` の**退会＋vptr ダウンキャスト**で、要素 vptr の上書きは存在しない。読み手 `FUN_00589B70(id)=base[id-999]`（境界 0..50） | 要（登録表の意味論: ID999..1049→Tile、NULL=空） |
| N-38 | **解明** | param_4=true は console save / console load / **TestAllCells（TAC）50 セルごと定期 save（"Test All Cells %i.ess"）** の 3 QA 経路のみ。表は読み出し消費者ゼロ（Find 1 箇所＝登録内、ログ 28 箇所＝書込のみ、walk は dtor だけ）→ 診断スナップショット（候補 b）の実証。v248 §3-4「console のみ」は訂正 | 任意（bDetailedSave=false 固定で等価） |
| N-39 | **解明** | 第 2 引数は LoadGame と共有の ParamInfo `0xB0A678` 内の死んだスロット。help は `SaveGame <filename>`（単一引数）。両 handler は parse 後に文字列バッファと成否 dword のみ消費、整数は読まずに破棄（console/スクリプト両経路）。opcode 表 argn=2・impl=同一 handler で補強 | 要（ argc 宣言は現物互換で可／無視と明記） |
| N-40 | **解明** | `0x982D85` = `_purecall`（R6025「pure virtual function call」文字列・handler setter 唯一・722/735 件が vtable スロット列）。2 連 = vtable_B/C の slot5/6（7 スロット境界と COL 位置が厳密一致）。派生 `0xA3AA98` で同スロットが実 thunk に上書きされることを対照確認 | 不要（CRT 機構は移植不要） |

### §5-2 新規残点（本束で発生。すべて低）

| ID | 内容 | 優先度 |
|---|---|---|
| R-1 | `NiTMapBase` slot5/slot6 の純粋仮想 2 本の意味名（具象側実装は `0x4F0F60/0x4B2ED0` の +0xC adjust thunk とまで判明。関数意味は RTTI 非掲載） | 低 |
| R-2 | TileMenu 登録時の ID しきい補正（qword `0xA69768`/`0xA69778` と代替 ID `0xFA5(4005)`/`0xFAB(4011)`・`0x588BD0`/`0x58CEB0` の呼出）の業務的意味。登録/解除の整合は取れているため転用不可解なし | 低 |
| R-3 | console parser が optional Integer を書き込む out スロットの実位置（パース引数配列 `&stack0x20` の所属）。handler が読まないことは証明済みで挙動不変 | 低 |

### §5-3 自己検証

- **selftest**: 着手時（本ファイル冒頭節）と完了時（下記）の 2 回実行、いずれも **ALL PASS (4/4)**（`probe_harness.py selftest`）。
- **手法**: capstone 5.0.7 逆アセンブル＋PE 手動解析（セクション表/RVA⇔VA）＋MSVC RTTI（COL/CD/BCD）手デコード＋
  `docs/Oblivion_decompiled.c` 部分読取＋ `tmp/re/cache/`（vtable_index / named_funcs / opcode_table / call_targets.json）突合。
  ESM 本体は未アクセス。git 不使用。
- **v248 記述の訂正点（2 件）**:
  1. §3-4「param_4=1 は console 経路のみ」→ **console save / console load / TAC 定期 save の 3 経路**（§2-1/§2-2 で実測）。
  2. §2-1「登録側 FUN_00585330」→ 実体は `Menu::~Menu`（退会側）。真の登録側は `FUN_00591C50`（TileMenu ctor 系 slot5）。
     ただし §2 の結論（型・構造・SetSize(51)・SetAt セマンティクス）はすべて維持・再確認。
- **機械語アンカー（再現可能）**:
  - `C7 06 DC 96 A6 00` @0x58533D（~Menu の self vptr ダウンキャスト書込。v248 アンカーの再解釈）。
  - `81 C3 17 FC FF FF / 05 17 FC FF FF` @0x591D14/0x591BC3（ID−0x3E9）、`B9 70 39 B1 00 / E8 ..42BB90` 登録/解除 3 箇所。
  - `8D 4C 24 10 / 51`＋`89 74 24 18` @0x591D0F..0x591D20（格納値=this の stack 算術）。
  - `83 EC 0C ...` 6 呼出し元の param_4 push 列（§2-1 表）。`68 18 B5 A3 00` @0x466D08（"Test All Cells %i.ess"）。
  - `FF 35 EC A5 BA 00 / E8 5B 91 00 00 / 6A 19 / E8 1A A8 00 00 / E9 BC 8E 00 00` @0x982D85..（purecall 列）。
  - `C7 40 08 00 00 00 00 / 83 C1 0C` @0x4B2ED4/0x4B2EDF（slot6 具象 thunk）。`'52 36 30 32 35'` = "R6025" @0xAA4CE4。

### §5-4 使用証拠一覧（すべて `%TEMP%\opencode\` 配下、`_out.txt` 付き）

- `v250_scan1.py`（RTTI チェーン初デコード・Menu vtable 15/Tile vtable 7・FUN_00585330 全文字・0xB13970 リテラル 6 箇所・0x591B9x 系登録部）
- `v250_scan2.py`（BCD 正規デコード：Menu 無基底/TileMenu=自己+TileRect+Tile・0xA6AF14=TileMenu・0x591B40..0x591D40 全域・0xB13974/0xB1397A リテラル走査）
- `v250_scan3.py`（FUN_00587B20=初回イネーブラ全文字・FUN_00587D20=ID switch・0x725DA0=false スタブ・消費者 4 帯・0xA696DC/0xA6AF14 リテラル）
- `v250_scan4.py`（TileMenu slot0・0x5BC450 ctor→vptr 0xA6CE5C・ジャンプテーブル 0x5887B8=51 エントリ・GetMenu(id) 0x589B70・COL 名引き）
- `v250_scan5.py`（FUN_00465130 6 呼出し元/FUN_00465860 5 呼出し元の push 列全文脈・FUN_004531B0 28 箇所文脈）
- `v250_scan6.py`（セクション名/ファイル名文字列ダンプ・FUN_00466BE0 の 0x466C00..0x466D60 実測）
- `v250_scan7.py`（FUN_00507960 TAC handler 頭・FUN_004527A0 全域 call 走査＝1 箇所）
- `v250_scan8.py`（save/load handler 全文字・ParamInfo 0xB0A660..0xB0A6E0・kind 属性表 0xB0A54C・parser 頭）
- `v250_scan9.py`（help/別名/opid/handler エントリ実体・'Game Saved'/'You cannot save right now'/'.ess' 等）
- `v250_scan10.py`（opcode_table.json から SaveGame/LoadGame/TAC の argn/impl/argdesc）
- `v250_scan11.py`（0x982D85/0x98BEEB/0x98D5B8/0x98BD5E/_abort・[0xBAA5EC] 参照走査・0x982D85 の .rdata 出現 735・vtable 0xA3904C/0xA3AA98・0xA69684..C8）
- `v250_scan12.py`（0x98DDEC=set_purecall_handler・slot5/6 具象 thunk・メッセージテーブル [0xB311E8]）
- `v250_scan13.py`（msg25="R6025 - pure virtual function call" 実読・スロット列内出現 722/735・COL sig=0）
- 入力（読取のみ）: `tmp/re/v248_port_saveslot_residuals3.md`, `tmp/re/v246_port_saveslot_residuals2.md`,
  `tmp/re/cache/{vtable_index.tsv,named_funcs.txt,call_targets.json,opcode_table.json}`,
  `docs/Oblivion_decompiled.c`（:52139-52215 TAC コールバック / :71756- FUN_0045AD00 / :290895-290953 FUN_00585330/0x5853F0 /
  :194575-194750 parser FUN_004FAE80）。既存中間物 `v248_scan*.py/_out.txt`（13 本）は文脈確認のみで再利用。

### §5-5 不変条件の再確認

- ESM 本体は未アクセス。git 不使用。**docs/GT/TODO/session-memory/plan/cache/ソースの編集なし**。
- 既存 `tmp/re` の上書きなし。**新規に書いたのは本ファイル（v250）のみ**。
- スクリプトは `%TEMP%\opencode\v250_*.py` の新規 13 本のみ（v248 既存スクリプトは読取参照）。
- 中断対策遵守: §1〜§4 を各項目完了の直後に追記（書き溜めなし）。
