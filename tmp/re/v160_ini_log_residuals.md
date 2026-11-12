# V160: INI/Setting/Log 残点束の解明（Oblivion.exe v1.2.0416 Steam）

- 作成: 2026-10-26 着手 / 2026-10-27 完成（分析専任サブエージェント。本ファイル新規作成のみ。docs・TODO・既存 tmp/re・session-memory・plan・cache は無変更、git なし）
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（7,898,624 B / ImageBase 0x400000 / capstone 5.0.7 skipdata）
- 出典（残点）: `tmp/re/v130_ini_residuals.md` N-1・N-2・N-3・N-4・N-5 ＋ `b24_misc_residuals.md` 残点1・2・3 ＋ `v137_v80_global_residuals.md` 残点6 ＋ `v158_slot_opcode_map.md` §3-7（残点6 の裁定確認）
- 手法: `python ".../probe_harness.py" selftest` → **4/4 PASS**（着手時）。`re_util`（raw E8 callers / find_dword / u32）＋ `probe_harness.Ctx`（rd/cstr/dec_index/ledger 21,597 件）＋ PE インポート記述子手動パース。中間成果物は `%TEMP%\opencode\v160_1..19.py` / `v160_1..19.txt` / `v160_names992.txt` / `v160_names2304.txt`（新規のみ）
- ESM 未アクセス（s60 照合は既存成果物のみ）。既存 tmp/re ファイルは読取のみ。

---

## §0 判定サマリ（9 残点）

| # | 残点 | 判定 | 一句话 |
|---|---|---|---|
| 1 | type 2 (`h`) の語源・用途（v130 N-1） | **解明（死コード確定・語源はバイナリ証拠なし）** | `h…:Section` 形文字列は EXE 全域 **0 件**、v91 CSV の h キー **0 件**、GameSetting 登録名 2,304 件にも `h` **0 件** → 登録し得る name 自体が存在しない。Dump の byte 読みは type-1 ハンドラ共有（表付け間違い）。語源候補 hword/hex はいずれも実挙動（4 B・`%u` = type 4 と同一）と不整合 → 推定も立てず「証拠なし」でクローズ（§1） |
| 2 | GameSetting コンテナ 0xB35574 のノード全数と type 分布（v130 N-2） | **解明** | 正体 = **name→Setting\* ハッシュマップ**（8 B `{u32/f32 value, char\* name}`、**type フィールド無し**）。登録は CRT 初期化の**登録関数 2 系**のみ（FUN_0041BAE0=float 992 件 / FUN_00419A70=raw32 1,312 件）→ **静的 2,304 件・実行時 insert 経路 0 件**。name 先頭分布 **f=992 / s=1,085 / i=227（他 0）** → console は name[0] で type 判定するため **type 1/2/8 は構造上発生不可能**（§2） |
| 3 | v91 §1.2 のクラス名 DefaultCombatStyle/Atmosphere の出所（v130 N-3） | **解明** | 出所 = **v91 自身の「obj+0x114 直後の TD」読み法**（§1.2 表に「(+0x114)」と自認）。実測で 4 行すべて一致（0xB07D28+0x114 = `.?AVDefaultCombatStyle@@`、0xB11C40+0x114 = `.?AVAtmosphere@@`、0xB07AD8+0x114 = ゼロ→「RTTI 無し」）。TD 直前配置規則（TD+0x24 = そのクラスの obj）により直後 TD は**隣接する別クラス**のもの。2 クラスは実在・vtable 付き（0xA4109C / 0xA56B40）・インスタンスあり（§3） |
| 4 | `fDifficultyMinValue:GamePlay` の孤立（v130 N-4） | **解明（前提訂正）** | 「孤立 INI 文字列」では**なかった**。`fDifficultyMinValue:GamePlay` リテラルは EXE 内 0 件（docs マニフェスト由来のキー表記）。bare 名 0xA5B0E8 は GMST 名クラスタに実在し、**既に GameSetting 登録済み**（0x9ED6F0: `fld [0xA30634]=-1.0f` → push 名 → `mov ecx,0xB37B70; call 0x41BAE0`）。INI ノードではないため v91 の INI 照合から漏れたのは構造上の必然（§4） |
| 5 | path オーバーフローの起動順: head 破壊 vs cookie abort（v130 N-5・移植重要） | **解明: head 破壊が先** | 3 つの構造証明: (a) 長 path の唯一の書手 bss 0xB3F280 = **FUN_0040D4D0 内 2 箇所のみ**（0x40E109・0x40E12E）; (b) cookie 経路 3 系（0x404400/0x404540/RefreshINI）の call site は **FUN_0040D4D0 内 0x40E826・0x40F580（head 後）＋ runtime 間接のみ**; (c) head 前のウィンドウ 0x40E180..0x40E710 の全 call 62 件に cookie 経路 0 件。→ folder ≥ 243 の起動では **0x40E710 で head(+0x10C) が先に破壊**され、その後 0x40E826 → FUN_00404A00 → FUN_00404540（frame 0x14C）が cookie abort。移植時は head 保護が必須で cookie abort に頼れない（§5） |
| 6 | MessageHandler ライタ 0x494730 の [0xB34D8C]（b24 残点1） | **裏づけ（v158 裁定と矛盾なし）** | abs32 参照・`.text` raw disp32 とも**実測 1 件 = 0x494768（読手のみ）**、書手不在。bss 恒 NULL → 到達不能死コード。v158 §3-7 の裁定を独立再測で支持（§6） |
| 7 | ctor が書く [0xB27E60]:=0x6F7070 / [0xB40608]:=0x60D0A0（b24 残点2） | **解明（b24 の crash 推定は誤り）** | 両方とも**フック関数ポインタ**。[0xB27E60] の**ファイル画像値 = 0x7485D0 = MessageBoxA ラッパ FUN_007485d0**（legacy 警告ダイアログ sink）で、ctor（唯一の呼出し元 = CRT 初期化 0x9DC337）が **no-op `xor eax,eax; ret`(0x6F7070) に上書き = retail で警告 sink を無効化**。[0xB40608] は bss NULL → ctor が `ret`(0x60D0A0) を設定（テーブル後処理フック、読手 1 件は NULL チェック付き）。crash/exception 系ではない（§7） |
| 8 | sev3/9/Confirm フラグ 4 件の設置者（b24 残点3） | **解明（2 グループに分裂）** | `[0xB06B30]`/`[0xB06B38]` = **INI 登録ブール** `bSkipInitializationFlows:MESSAGES`（既定 1）/ `bBlockMessageBoxes:MESSAGES`（既定 0）→ 設置者 = CRT 既定値 + INI [MESSAGES] LoadAll。`[0xB06B15]`/`[0xB06B16]` = **INI 非連動の内部 1-byte ラッチ**（.data 初値 1/1、sev9/Confirm ハンドラ自身が操作）。b24 の「INI 連動と推定」は半分のみ正（§8） |
| 9 | Setting 登録スタブ 0x9F8E00–0x9F9050 全数＋0xA235xx 対応表（v137 残点6） | **解明** | レンジ内 Setting 登録（call 0x41BAE0）= **8 件**（call site 0x9F8F04..0x9F9050）＋レンジ直後に 2 件 = ファミリ**計 10 件**（v137 記載 7 件から +3: obj 0xB39E40/E48/E50）。レンジ内には Setting 以外の CRT スタブも 7 件混在（CS 0xB39D00 初期化 + BSFaceGen 系 6 件）を全数分離。atexit 対応表 0xA232E0 / 0xA23550–0xA235F0 = CS 2 + Setting 10。全 10 件は GameSetting map 登録であり **§70.2 の INI 既定値テーブル（635）とは別系統**（§9） |

---

## §1 残点1: type 2 (`h`) の語源・用途

### 1-1 検証（v130 N-1 の試行経路どおり）

| 検査 | 結果 |
|---|---|
| EXE 全域 `h…:Section` 形 NUL 終端文字列（正規表現走査） | **0 件** |
| `v91_ini_defaults.csv`（INI ノード 635）の h 始まりキー | **0 件** |
| GameSetting 登録名 2,304 件（§2 実測）の `h` | **0 件**（分布は f/s/i のみ） |

### 1-2 判定

- **用途**: type 2 は name[0]='h' のときのみ選ばれるが、そのような name は **INI ノード・GameSetting 名・`.rdata` 文字列のいずれの層にも存在しない** → 実行時到達は不可能な**完全デッドコード**（ReadOne/WriteOne の実装は存在するが呼出し不可）。
- **Dump の byte 読み**（v130 の不整合点）: Dump 整形 switch 表 `0x4A7C7C` の [2]（type 2）は [1]（type 1）と**同一ハンドラ 0x4A7BBC** を指す。type 1 が 1 バイト型である以上、これは type 2 に専用実装を与えなかった**表付け間違い（コピー・アーティファクト）**であり、type 2 固有の挙動は存在しない。
- **語源**: バイナリからの判定は**不能**。候補 `hex` / `hword`(16bit) はいずれも実挙動（ReadOne で 4 B 書込・WriteOne で `%u` = type 4 と完全同一、Dump は type 1 共有）と不整合。証拠を持たないため推定を記さない。

**判定: 解明（未使用確定・語源はバイナリ証拠なし）。**

---

## §2 残点2: GameSetting コンテナ 0xB35574（lookup 0x55E000）

### 2-1 正体: Setting 名前→Setting\* ハッシュマップ（.bss）

`lookup 0x55E000` の逆アセンブル（`v160_3.txt`）でレイアウト確定:

```
0xB35574 +0x00  KeyTraits vptr（+4 = hash(name) / +8 = compare / +0x10,+0x18 = ノード解放）
          +0x08  bucket 配列（dword ポインタ → ノード）
          +0x0C  登録カウンタ（remove で -1）
ノード:  +0x00 next / +0x04 key(char* name) / +0x08 value(Setting*)
Setting オブジェクト: +0x00 u32/f32 value / +0x04 char* name（8 B 固定・type フィールド無し）
```

- 操作系: **lookup = 0x55E000**（E8 308 件）、**remove = 0x4524C0**（76 件）、**insert = 0x412D30**（47 件、うち **ecx=0xB35574 は 2 件のみ**）。
- コンテナへの abs32 xref = **19 件**（全数分類）: 登録関数内の dup-check/insert 4 件（0x419AB8/0x419AF1・0x41BB28/0x41BB61）、Setting dtor の remove（0x405621 = FUN_004055E0）、シャットダウン一括 remove ループ 8 件（0x40C54F..0x40C7DB）、コンソール Get/SetGameSetting 2 件（0x51404B/0x514183）、文字列→Setting インライン lookup 4 件（0x575CF9..0x575D33）。

### 2-2 ノード全数（確定）

| 区分 | 件数 | 根拠 |
|---|---:|---|
| 登録系 A: **FUN_0041BAE0**（float 默认 `fstp [this]` → dup-check → insert） | **992** | callers = 992、VA 範囲 0x9DA4D4..0x9FD314 = **CRT 初期化スタブ帯のみ**（`v160_9.txt`） |
| 登録系 B: **FUN_00419A70**（raw32 默认 → dup-check → insert、同レイアウト） | **1,312** | callers = 1,312、0x9D9D6F..0x9FCF2F = CRT 初期化帯のみ。独自実装（0x419A80-: value/name 代入 → 0x55E000 dup-check → 0x412D30 insert） |
| 名前重複 | **0** | distinct names 992 ∩ 1312 = 0（`v160_7.txt`） |
| 動的 insert（実行時） | **0** | insert 0x412D30 の ecx=0xB35574 は上記 2 登録関数内サイトのみ。0xB35574 の abs32 全 19 件に insert は存在しない（§2-1 分類） |
| **実行時合計** | **2,304** | |

- 重複エラー文字列: 0xA32490 `"Setting key '%s' already used in map.\nSetting keys must be unique.\n"`（両登録系が 0x4A7A60 へ出力）。
- **ESM GMST 適用**は lookup+値上書き（insert 経路が静登録の 2 サイトのみ）と推定 — ESM 未検証（§10 R-N2）。

### 2-3 type 分布（= name 先頭文字実測 2,304 件）

コンテナ自体に type フィールドは無い。type は **console Get/SetGameSetting（0x51404B/0x514183）が lookup 後 name[0] を FUN_004A78D0 で判定**する方式（`v160_5.txt` で call 0x4A78D0 を直接確認）。

| 先頭文字 | type() 戻り | 件数 | 備考 |
|---|---|---:|---|
| `f` | 5 (float) | **992** | 登録系 A（fstp で float 設定） |
| `s` | 6 (string) | **1,085** | 登録系 B（value = 既定文字列ポインタ） |
| `i` | 3 (int) | **227** | 登録系 B |
| `b`/`c`/`h`/`u`/`r`/`a` | 0/1/2/4/7/8 | **0 件** | |

- **v130 N-2 の懸念「type 1/2/8 が存在しうる残る唯一の系統」は否定**: 構造上（type フィールド無し・type 判定は name[0] 依存）＋実測（c/h/a 名 0 件）の二重に不可能。
- 整合検算: v130 §7 の「GameSetting G/S は type 3/5/6 のみ実装」⇔ 実在 name も f/s/i のみ → **実装と実在が完全対応**（type 9/UNKNOWN に落ちる name は存在しない）。
- INI ノード系（635 件・container 0xB07BF0 等）とは**別系統**（v130 既記の通り。本タスクで 2 系統の対応表を完成）。

---

## §3 残点3: DefaultCombatStyle / Atmosphere の出所

### 3-1 v91 の読み法（v91 自身の注記で確定）

`v91_ini_defaults.md` §1.2 行 60 は **`0xB07BF0 | .?AVRegSettingCollection@@（+0x114）`** と明記（行 73 にも「+0x114 直後から RTTI TypeDescriptor が続く」）。すなわち **v91 = 「オブジェクト +0x114 直後に続く TD をそのクラスの RTTI として読む」方法**。この方法を 4 行すべてに適用した結果が §1.2 の表:

| v91 行の obj | obj+0x114 の中身（本タスク実測） | v91 が記した RTTI | obj の実 RTTI（v130 の COL 連鎖） |
|---|---|---|---|
| 0xB07BF0 (0x114 B → 0xB07D04) | `.?AVRegSettingCollection@@` | RegSettingCollection | `.?AVINISettingCollection@@` |
| 0xB07D28 (→ 0xB07E3C) | **`.?AVDefaultCombatStyle@@`** | DefaultCombatStyle | `.?AVRegSettingCollection@@` |
| 0xB11C40 (→ 0xB11D54) | **`.?AVAtmosphere@@`** | Atmosphere | `.?AVBlendSettingCollection@@` |
| 0xB07AD8 (→ 0xB07BEC) | **全ゼロ** | （RTTI 無し） | `.?AVLipSynchroSettingCollection@@` |

4 行すべて一致 → **出所は完全に解明: obj+0x114 隣接 TD の誤帰属**（v130 §4.1 の「オブジェクト直後 TD 誤読」仮説の実証）。誤読機構の根本: 実布局は **`[TD (0x24/0x2C)][obj (0x114)]` の順（TD が自分の obj の直前に来る）**（例: TD 0xB07D04 +0x24 = obj 0xB07D28、TD 0xB07AAC +0x2C = obj 0xB07AD8）→ 「obj の直後」に来るのは**次のクラスの TD**。

### 3-2 2 クラスは実在クラス（本タスクの新事実 — 孤立 TD ではない）

| 項目 | DefaultCombatStyle | Atmosphere |
|---|---|---|
| TD | 0xB07E3C（name 0xB07E44） | 0xB11D54（name 0xB11D5C） |
| **COL** | **0xABA100**（sig=0, pTD, pCD=0xABA114 CD numBase=**4**） | **0xABFBDC**（sig=0, pTD, CD numBase=**2**） |
| **vtable** | **0xA4109C**（COL は 0xA41098 = vtable[-1]） | **0xA56B40**（COL は 0xA56B3C） |
| vtable の .text 参照 | **1 件 = 0x9E1228（CRT 初期化帯 → 静的インスタンスが起動時に構築される）** | **2 件 = 0x53AE94 / 0x53AF8D（実行時コード）** |
| vtable 先頭スロット | 0x60D0A0(`ret`), 0x60D0A0, 0x46B800, 0x46B870 | 0x53B3D0, 0x4B2460, 0x543D30, 0x53B0E0 |
| 隣接 TD（同じクラスタ） | — | 直後に `.?AVSkyObject…@@`（0xB11D70 帯 = sky クラスタ） |

- b24 §6.2 の「`.text` 参照ゼロ vtable 10 行」に 0xA4109C/0xA56B40 は**不在**（＝参照あり）→ 孤立ではなく**通常クラス**。
- v91 の vtable 列（0xA40590 / 0xA566A4）は COL 連鎖と一致する**正しい値** → v91 の誤りは name 列のみ。
- 補足: `.?AVDefaultCombatStyle@@` は `.?AVTESCombatStyle@@`（0xB053E8、`__RTDynamicCast` の src/dst として .text 参照 10 件）とは別 TD。

**判定: 解明。出所 = v91 の obj+0x114 読み法。対象オブジェクトは RegSettingCollection / BlendSettingCollection（v130 の訂正を支持）、2 クラス名は実在する隣接 TD 由来。**

---

## §4 残点4: fDifficultyMinValue:GamePlay の「孤立」

### 4-1 実測

| 検査 | 結果 |
|---|---|
| `fDifficultyMinValue:GamePlay` リテラル | **0 件**（EXE 全域 find = -1） |
| bare `fDifficultyMinValue` | **0xA5B0E8**（GMST bare-name クラスタ内: fSplashScale1..3 / fGrabPower / fHostileActor{Interior,Exterior}Distance / **fDifficulty{Min,Max,DefaultValue,DamageMultiplier}** / fClothing*） |
| bare 名への imm32 参照 | **1 件 = 0x9ED6FB** |
| 参照先の実行時識別 | **既に GameSetting 登録済み** |

```
0x9ED6F0:  fld dword [0xA30634]          ; 既定値 = -1.0f（生 000080bf、実測）
0x9ED6F6:  push ecx / fstp [esp]
0x9ED6FA:  push 0xA5B0E8                 ; "fDifficultyMinValue"
0x9ED6FF:  mov ecx, 0xB37B70             ; Setting オブジェクト（.bss 8 B）
0x9ED704:  call 0x41BAE0                 ; GameSetting 登録（map 0xB35574、§2 の float 系 992 件の 1 つ）
0x9ED709:  push 0xA1FE00 / call 0x981FB4 ; atexit（Setting 破棄シンク）
```

### 4-2 `:GamePlay` 表記の出所

v91 §7（行 164・293）: 既知キー 33 件は **docs（RE_EXE_REPORT / v80 / v81）を正規表現 `\b[fbisur]\w+:Section\b` で抽出したマニフェスト**との照合であり、`fDifficultyMinValue:GamePlay` は docs 由来のキー表記。v91 自も「連接文字列として不存在（bare 直接のみ）→ 既存登録ノードではない」と記載済み。本タスクでその正体を確定: **INI ノードではなく GameSetting 登録済み bare 名**。

### 4-3 判定

- INI ノードではない（v91 CSV 非一致を実測 ✓）ため INI 照合から漏れたのは構造上の必然。
- Android 移植示唆: このキーは **INI 既定値テーブルに載せず GameSetting/GMST 側**へ（既定 -1.0f は EXE 静的値、ESM GMST が上書きする可能性は未検証）。
- 既定値の生値: [0xA30634] = `00 00 80 bf` = **-1.0f**（実測。GMST 実行時値との関係は ESM 未検証のため保留）。

**判定: 解明（前提訂正つき）。INI 孤立文字列ではなく GameSetting 登録済み float。**

---

## §5 残点5: path オーバーフローの起動順（head 破壊 vs cookie abort）【移植重要】

### 5-1 FUN_0040D4D0 の呼出し網（実測）

- E8 呼出し元 = **5 件**（+ E9 tail-jump 1 件）:

| site | 宿主 | 直前の文脈 |
|---|---|---|
| 0x40DF54 | FUN_0040D4D0 **自前** | `mov ecx,ebp; call 0x40d4d0`（内部再帰） |
| 0x4456B3 | FUN_004455E0 | `mov ecx,[0xB33398]; call`（ガード 2 経路通過後） |
| 0x44EB24 | FUN_0044E720 | sprintf+0x579B60 報告後、`mov ecx,[0xB33398]` |
| 0x50D567 | FUN_0050D4C0 | `mov ecx,[0xB33398]`（宿主の E8 callers = 0 → 間接/未到達） |
| 0x5BDD53 | FUN_005BDCD0 | タイマー比較ループ内、`mov ecx,[0xB33398]` |
| **E9 0x440C78** | FUN_00440C40 | `mov ecx,[0xB33398]; mov byte [0xB35228],1; jmp 0x40d4d0` |

- 全外部エントリが `ecx = [0xB33398]`（グローバル単例。Confirm の UI スレッド判定 `+0x10 == GetCurrentThreadId()` でも参照される Main 系オブジェクト）経由。imm32 参照 0xB35228 = 3 件（0x440C73 の書出 + 0x49E8BD/0x49E8D6 読手）。
- **5 件は線形の「呼び出し順」を持たない**（相互再帰クラスタの再入ポイント群）: FUN_0040D4D0 → 0x40ED72 → FUN_004455E0 → 0x4456B3 へ戻り / FUN_0040D4D0 → 0x40EC6B → FUN_00445A10 → FUN_004455E0・FUN_0044E720 → 戻り / FUN_0040D4D0 → 0x40DD31・0x40DD98 → FUN_00444FB0 → E9 0x440C78 → 戻り。上位根は FUN_00465860 / FUN_004B7B40 / FUN_00508C30 / FUN_005821F0 等（いずれも opcode/メニュー系ランタイム呼び出しで到達）。
- **制限**: PE AddressOfEntryPoint = 0xBAF2ED（.bind セクション内の保護系スタブ。`rep movsd` + 暗号定数 → 実 CRT start は間接遷移）のため、**最初の外部エントリの厳密な一次導出は静的不能**。ただし後述の構造証明には不要。

### 5-2 構造証明（順序の確定 — 呼出し順に依存しない）

**証明 A: 長 path の唯一の書手は FUN_0040D4D0 内**
bss 0xB3F280 の全セクション abs32 参照 = **20 件を全数分類**（`v160_19.txt`）:

| 書込み（store） | 読み/引数（load/push） |
|---|---|
| **0x40E109**（copy#2 ループ 0x40E100..0x40E112: stack→bss） | 0x404422・0x404562（cookie 経路内読み）/ 0x40E142（copy#3）/ 0x45D4B4・0x45D52D・0x45DB12・0x45E79D・0x45F39C（push 引数）/ 0x45F3C2・0x45F454（byte コピー読み）/ 0x494492・0x494502・0x494572・0x4945F2（path builder 0x4944xx = **bss から 0xB34C80 static へコピー**、bss 非改変を実測）/ 0x498A84（push 引数）/ 0x513BD5（RefreshINI 読み）/ 0x53ACA2・0x53ADB2（byte コピー読み） |
| **0x40E12E..0x40E134**（`mov word [0xB3F280],ax` + `mov byte [0xB3F282],cl` = 終端初期化） | |

→ **bss への書込みは 2 箇所とも FUN_0040D4D0 内（0x40E100 系・0x40E12E 系）**。起動直後（bss = ゼロ = 空文字列）から長 path が出現するのはこの関数内のみ。空 path ならいかなる後続コピーも溢れない → **オーバーフローの前提成立自体が FUN_0040D4D0 内 0x40E100 の後**。

**証明 B: cookie 経路の全 call site は head コピーの後**
v130 の cookie 制限 3 系について raw E8 全数（callers の網羅性）:

| cookie 経路 | 全 call site | head 後か |
|---|---|---|
| FUN_00404540（frame 0x14C・local 252 B） | **1 件: 0x404AA8（FUN_00404A00 内）** | FUN_00404A00 の唯一の呼出し = **0x40E826（FUN_0040D4D0 内・head 0x40E710 の後）** ✓ |
| FUN_00404400（frame 0x148・local 252 B） | 2 件: 0x40C37F（FUN_0040C350 内）/ 0x59C694（FUN_0059C5E0 内） | FUN_0040C350 の唯一の呼出し = **0x40F580（FUN_0040D4D0 内・head 後）** ✓。FUN_0059C5E0 は E8 callers **0 件**、`0x59C5E0` の abs32 は 0xA6A000 の 1 件のみだがこれは文字列データ（`…ing you to sink.' 'Alchemy\0You cannot create potio…`）中の偶然 4 バイトで **readers 0 件** → データ経路不在・ランタイム register 間接のみ（起動パス不在） |
| RefreshINI 0x513BB0（frame 0x108） | E8 **0 件**、abs32 は opcode テーブル 0xB0BDE8 のみ | コンソール `refreshini` のランタイム間接のみ ✓ |

**証明 C: head 前のウィンドウに cookie 経路の呼出し 0 件**
bss 設定後（0x40E100）から head コピー（0x40E710）まで — ウィンドウ 0x40E180..0x40E710 の**全 call 指令 62 件を列挙**（`v160_18.txt`）: いずれも {0x404400, 0x404540, 0x404A00, 0x40C350, 0x59C5E0, 0x513BB0} に**非該当**。内訳: path builder 0x4944xx ×3（→ static 0xB34C80 へコピー・スタックバッファ無しを実測）、IAT 系（DeleteFileA・GetLogicalDriveStringsA 等）、**FUN_00404940**（frame 0x20C/0x200 buf = GetLogicalDriveStringsA ドライブ種別チェック、OS 入力 bounded・path 非関係）、`0x4A7980` ×4（ヒープ文字列格納・stack コピー無し）、MessageBoxA 0x40E2C0（ダイアログのみ）、setting-collection 仮想呼び出し（Begin/LoadAll 等・内部 bounded）。

### 5-3 結論（folder ≥ 243 の起動実行順）

```
(1) 0x40E0C4..0x40E160  folder→bss 0xB3F280→stack→"Oblivion.ini"（frame 0x6A8・余裕あり・cookie 無被害）
(2) 0x40E180..0x40E708  builder/ドライブチェック/文字列格納（§5-2C、全て無害）
(3) 0x40E710..0x40E722  stack → 0xB07BF4 無検査コピー  ⇒ ★ head(+0x10C) 破壊  …… 最初の破壊現象
(4) 0x40E826            call FUN_00404A00 → 0x404AA8 call FUN_00404540（local 252 B）⇒ cookie abort
(5) 0x40F580            call FUN_0040C350 → 0x40C37F call FUN_00404400（local 252 B）⇒ cookie abort（(4) が fail-fast なら到達しない）
```

- **head 破壊が先、cookie abort は後**。(4)(5) の fail-fast 時点では head は既に破壊されている。
- 閾値算術（flag +0x108 = folder ≥ 239、head = folder ≥ 243、cookie = folder ≥ 243、窓 [239,243) = flag のみ無害）は **v130 §5.2/5.3 の値を踏襲**（本タスクは順序を確定した）。
- **移植示唆**: folder ≥ 243 では cookie abort（MSVC の fail-fast）に依存した「止まり方」は期待できない。連結リスト head(+0x10C) の破壊が先に成立するため、**path 長ガード（folder/dir/L の境界検査）を 0x40E710 コピーの手前に置く**ことが唯一の防衛。

### 5-4 試行経路

`v160_4.txt`（callers 5 件 + cookie 系 census）→ `v160_5.txt`（E9・宿主チェーン・FUN_0040C350/00404A00 呼出し元）→ `v160_6.txt`（E9 宿主・[0xB33398]）→ `v160_8.txt`（0x40E6F0..0x40E840 直線フロー実証）→ `v160_16/17.txt`（エントリ追跡:.bind 保護スタブで断念を記録）→ `v160_18.txt`（head 前 call 62 件全列挙 + 0xA6A000 判定）→ `v160_19.txt`（bss 20 ref の store/load 全分類 + builder 実体）。

---

## §6 残点6: [0xB34D8C] — v158 裁定の裏づけ

独立再測（`v160_1.txt` §R6）:

| 検査 | 実測 | v158 §3-7 |
|---|---|---|
| `0xB34D8C` の全セクション abs32 参照 | **1 件 = 0x494768**（0x494730 内の読手） | 1 件 = 0x494768 |
| `.text` raw disp32 `8C 4D B3 00` 出現（mov-immediate 書手も含む全バイト走査） | **1 件 = 0x494768** | 書手 0 件 |

- 0xB34D8C は .data bss 域（raw 未バック）→ 初期値 0、構築経路なし。
- v158 §3-7 の「恒 NULL bss・MessageHandler ライタの context 付き本文書きは到達不能（死コード）」裁定と**完全に整合。矛盾なし・新事実なし**。

**判定: 裏づけ完了。**

---

## §7 残点7: [0xB27E60]:=0x6F7070 / [0xB40608]:=0x60D0A0 の用途

### 7-1 値と設置者の確定（`v160_1.txt` / `v160_10.txt` / `v160_12..15.txt`）

| 項目 | [0xB27E60] | [0xB40608] |
|---|---|---|
| **ファイル画像値（ロード直後）** | **0x7485D0 = FUN_007485d0**（実測 `d0857400`） | **0（bss / raw backing 無し）** |
| ctor が書く値 | 0x6F7070 = `xor eax,eax; ret`（return-0 共用、imm32 refs **2,752**） | 0x60D0A0 = `ret`（void 共用、**957**） |
| 設置者 | MessageHandler ctor `0x494650` 末尾（0x494693）。**callers(0x494650) = 1 件 = 0x9DC337（CRT 初期化 FUN_009dc2d0）のみ** | 同（0x49469D） |
| 読手 | **3 件**（全員 `test; je; call` NULL チェック付き） | **1 件**（NULL チェック付き） |

**0x7485D0 の実体（v160_12）**: `mov eax,[esp+8]; mov ecx,[esp+4]; push 0x40000; push eax; push ecx; push 0; call [0xA28258]; xor eax,eax; ret` — IAT **0xA28258 = USER32!MessageBoxA**（手動インポート解析で確定）→ **`MessageBoxA(0, text, caption, 0x40000)` ラッパ = legacy 警告ダイアログ sink**。imm32 参照 VALUE 0x7485D0 = **1 件（0xB27E60 スロット自身）** → 他用途なし。

### 7-2 読み手の全文脈（3 件）

| 読手 | 宿主 | 整形文脈 | 呼出し形 |
|---|---|---|---|
| 0x72F5A7 | FUN_0072f420 | "%s contains old skin info and needs to be re-exported."（title "Old Version" 0xA80470） | sprintf 後 `push 0; push title; push buf; call sink` = sink(buf, title, 0) → ファイル画像では MessageBox(NULL, buf, title, 0x40000) |
| 0x74E82D | FUN_0074e390 | "File %s contains a mesh particle system! %s remove it."（"Unsupported Object" 0xA86044） | 同型 |
| 0x88C4CD | FUN_0088c3d0 | "%d Havok picks (%d path, %d LOS, %d ViewCaster) used this frame."（"WARNING" 0xA96190） | 同型 |
| 0x748591（[0xB40608]） | FUN_00748530 | 0xB40610 の 12 B/entry フラグテーブル走査 → フラグ立ちは `call ecx(idx, 0xB40408)`、末尾 `LeaveCriticalSection(0xB40790)`（IAT 0xA28074 を手動解析で確定） | テーブル後処理フック |

### 7-3 判定

- **起動直後の実効値**: ロード時 [0xB27E60]=MessageBox sink → CRT 初期化で **no-op に上書き** → 実行時は常に no-op。[0xB40608]: NULL（読手はスキップ）→ ctor が `ret` no-op を設定。
- **用途の確定**: 両方とも「legacy のデバッグ出力/警告表示フック」。**MessageHandler ctor の役割 = retail でこれらを no-op 化して警告を沈黙する**（MessageHandler 自身の sev ディスパッチへ集約、かつ b24 §3.4 の通り retail で sev1/7 も no-op → 警告は全面的に出力されない）。ファイル画像に MessageBox sink が残っているのは「MessageHandler 導入前の旧経路の既定値」。
- **b24 残点2 の推定「crash/exception 前提の可能性」は誤り**。crash 系コードには一切接続しない。

**判定: 解明。**

---

## §8 残点8: sev3/9/Confirm フラグ 4 件の設置者

### 8-1 INI 登録ブール 2 件（b24 の「INI 連動」はこれだけ正しい）

| アドレス | INI キー（name@+4） | .data 既定値 | 登録スタブ（CRT 初期化） | atexit | 設置者 |
|---|---|---:|---|---|---|
| 0xB06B30 | `bSkipInitializationFlows:MESSAGES`（0xA3DAA8） | **1** | FUN_009dc8c0: `push 0xB06B30; mov ecx,0xB07BF0; call 0x4040E0` @0x9DC8E5 | 0xA18A20 @0x9DC8FC | **CRT 既定値 1 ＋ INI [MESSAGES] の LoadAll(ReadOne)** |
| 0xB06B38 | `bBlockMessageBoxes:MESSAGES`（0xA3DB8C） | **0** | FUN_009dca60: 同型 @0x9DCA85 | 0xA18AE0 @0x9DCA9C | 同（既定 0） |

- **読み手とゲート方向（実測）**:
  - sev3 ハンドラ 0x4948A8: `cmp byte [0xB06B30],0; jne 0x494933` — **0x494933 = epilogue（cookie チェック → ret 4）**（`v160_13.txt`）→ 既定 1 では **sev3（ProgramFlow.txt 書込）はスキップ**、INI で 0 にしたときのみ有効。名前の "Skip" と既定 1 が整合。
  - Confirm ハンドラ 0x4949E3: `cmp byte [0xB06B38],0; je 実ダイアログ` → 立つとスタイル依存の既定応答（style nibble==2 → 5、他 → 6）に置換。

### 8-2 内部 1-byte ラッチ 2 件（INI 非連動 — b24 推定の訂正）

| アドレス | .data 初値 | 書き手（全数） | 読み手 | 意味 |
|---|---:|---|---|---|
| 0xB06B15 | **1** | sev9 ハンドラ 0x494981 の `mov byte,0` のみ（clear 書手 = 1 箇所） | sev9 先頭 0x494968 `cmp;je` | **one-shot**: 初回 sev9 時のみ既存 StackTrace.txt を DeleteFileA（0x494975-0x49497B）→ 以降クリア済みで追記 |
| 0xB06B16 | **1** | Confirm FUN_004949E0（範囲 0x4949E0..0x494E90）内 2 箇所: **0x494E02 の `mov,1`**（0x5732d0 呼出し直後）/ **0x494A67 の `mov,0`**（UI スレッド検証通過時の消費） | 0x494A24 `cmp;je 0x494E0B` | **Confirm 表示中/再入待ちラッチ**: 消費時は `[0xB33398]+0x20`（ダイアログ状態）と `GetCurrentThreadId()==[0xB33398]+0x10`（IAT 0xA2808C、UI スレッド判定）を通過してから 0 に |

- **配置（新事実）**: 2 フラグは **MessageHandler TypeDescriptor（0xB06AF8、.data）の name NUL（0xB06B14）と初ノード 0xB06B18 の間 3 バイト隙間（0xB06B15..17 = `01 01 00`）に配置された 1-byte 変数**（実測 image `v160_1.txt`）。セクション実測: .rdata は 0xB018D2 終端 → 0xB06AF8 以降は**すべて .data**（`v160_16.txt`）= 書込可能 ✓。
- **b24 が refs に挙げた 0x9B0121 / 0x9B0151 の正体（分類完了）**: `mov ecx,<node>; jmp 0x403BC0` の **name 解放シンク**（0x9B0090..0x9B0150 に B18/B20/B28/B30/B38 の 5 個。0x403BC0 = name 先頭 == 'S'(0x53) のとき free）。参照はリンカ組込 CRT レコード列（0xADC480..、各レコード `{1, 0xFFFFFFFF, thunk, 0x19930522(MSVC C++ EH マジック), 1, self}`、cookie 付きハンドラ尾）のみで **.text に登録コードなし = 静的テーブル**。書手ではない。
- 補足（'S' タグ機構）: `0x4A7980`（string 値のヒープ格納）は name コピー先頭を **`mov byte [esi],0x53` で 'S' に強制改変**し、旧 name が 'S' 先頭なら free してから block へ再設置（`v160_13.txt`）。's'→'S' は type() で type 6 不変、INI キー照合は大文字小文字無視と両立するとみられる（**推定**）→ dtor 系の「'S' のときだけ free」判定と対応する**ヒープ所有権タグ機構**と整合。Dump 等の読み手が改変後 name を如何に扱うかは未検証（§10 R-N3）。

**判定: 解明。B30/B38 = INI [MESSAGES] ブール（CRT 既定値 + ReadOne が設置）、B15/B16 = ハンドラ自身が操作する内部ラッチ（INI 非連動）。**

---

## §9 残点9: Setting 登録スタブ 0x9F8E00–0x9F9050 全数＋0xA235xx 対応表

### 9-1 レンジ内の全スタブ（非 Setting 7 件 + Setting 8 件 = 15 件、全数分離）

`0x9F8E00..0x9F9050`（host 帯 FUN_009f8d70 / CRT 初期化帯）:

| 0x30 スタブ | 内容 | 判定 |
|---|---|---|
| 0x9F8E00 | `ret`（前関数の末尾） | 無収 |
| 0x9F8E10 | `mov ecx,0xB39D00; call 0x401000`（CS 初期化）+ `push 0xA232E0; call 0x981FB4` | **CS 0xB39D00 初期化 + atexit**（Setting ではない） |
| 0x9F8E30 | `call 0x70E220`: name **BSFaceGenModelExtraData**(0xA64C1C), obj 0xB39D88, arg 0xB3FD44 | クラス登録（Setting ではない） |
| 0x9F8E50 | 同: **BSFaceGenBaseMorphExtraData**(0xA64EF4), 0xB39D90, 0xB3FD44 | 同 |
| 0x9F8E70 | 同: **BSFaceGenMorphData**(0xA64F10), 0xB39D98, arg 0 | 同 |
| 0x9F8E90 | 同: **BSFaceGenMorphDataHead**(0xA64F24), 0xB39DA0, parent 0xB39D98 | 同 |
| 0x9F8EB0 | 同: **BSFaceGenMorphDataHair**(0xA64F3C), 0xB39DA8, parent 0xB39D98 | 同 |
| 0x9F8ED0 | 同: **BSFaceGenNiNode**(0xA65164), 0xB39DB8, arg 0xB3FAB0 | 同 |

### 9-2 Setting 登録スタブ全数（call 0x41BAE0 = GameSetting map 登録）

レンジ内 8 件 + レンジ直後 2 件 = **ファミリ計 10 件**（v137 の既記 7 件から **+3 件を新規発見**: obj 0xB39E40/E48/E50）:

| stub | call site | name | 既定値 | Setting obj | atexit スランク |
|---|---|---|---:|---|---|
| 0x9F8EF0 | 0x9F8F04 | fTreeNearDistanceBase | **2048.0** ([0xA2FF44]) | 0xB39E08 | 0xA23560 |
| 0x9F8F20 | 0x9F8F34 | fTreeFarDistanceBase | **16384.0** ([0xA65258]) | 0xB39E10 | 0xA23570 |
| 0x9F8F50 | 0x9F8F64 | fTreeSizeConversion | **10.0** ([0xA31C80]) | 0xB39E18 | 0xA23580 |
| 0x9F8F80 | 0x9F8F94 | fTreeWindVariance | **0.05** ([0xA43328]) | 0xB39E20 | 0xA23590 |
| 0x9F8FB0 | 0x9F8FC4 | fLeafRockTimeScale | **2.0** ([0xA379B4]) | 0xB39E28 | 0xA235A0 |
| 0x9F8FE0 | 0x9F8FF4 | fLeafRustleTimeScale | **0.5** ([0xA3D65C]) | 0xB39E30 | 0xA235B0 |
| 0x9F9010 | 0x9F9020 | fLeafRockSpeedSwayInfluence | **1.0** (fld1) | 0xB39E38 | 0xA235C0 |
| 0x9F9040 | **0x9F9050**（レンジ境界） | fLeafRustleSpeedSwayInfluence | 1.0 (fld1) | 0xB39E40 | 0xA235D0 |
| 0x9F9070 | 0x9F9080（レンジ外直後） | fLeafRockAmountSwayInfluence | 1.0 (fld1) | 0xB39E48 | 0xA235E0 |
| 0x9F90A0 | 0x9F90B0（同） | fLeafRustleAmountSwayInfluence | 1.0 (fld1) | 0xB39E50 | 0xA235F0 |

- 既定値は v137 の既記 7 件と**完全一致**（検算 ✓）。name は push 即値から直接デコード（0xA652B0 = fLeafRockSpeedSwayInfluence は v137 未記載分を補完）。
- obj 間隔は 8 B だが **0xB39E20 → 0xB39E28 で 0xB39E24 相当の空き**（未使用スロット）。

### 9-3 0xA235xx / 0xA232xx atexit 対応表（全数）

| スランク | 実体 | 対象 |
|---|---|---|
| 0xA232E0 | `mov ecx,0xB39D00; jmp 0x401010`（DeleteCriticalSection） | CS 0xB39D00 |
| 0xA23550 | `mov ecx,0xB39E80; jmp 0x401010` | CS 0xB39E80（v137 既記） |
| 0xA23560 / 570 / 580 / 590 / 5A0 / 5B0 / 5C0 / 5D0 / 5E0 / 5F0 | 各 `mov ecx,<obj>; jmp 0x4055E0`（FUN_004055E0 = Setting 破棄） | 0xB39E08 / E10 / E18 / E20 / E28 / E30 / E38 / E40 / E48 / E50 = **上表 10 Setting と 1:1** |

（0xA23540 は直前スランク末尾の途中バイトで decode は無意味。0xA235E0/5F0 は probe5/9/10 で実測。）

### 9-4 §70.2 の INI 既定値テーブルへの接続（判定）

- 全 10 件は **FUN_0041BAE0 経由 = GameSetting map 0xB35574 の float 登録**（§2 の 992 件の一部）。**INI ノードではない** — 11 名すべて `v91_ini_defaults.csv`（635 ノード）に**非一致**を実測（`v160_6.txt`）。
- → **§70.2 の INI 既定値テーブル（635）には入れない**。接続先は **GameSetting/GMST 既定値テーブル**（§2 の 2,304 件中の f 系サブセット）。ESM の同名 GMST が実行時に値を上書きする形式（推定、§10 R-N2）。
- Android 移植示唆: INI 既定値 = v91 の 635、GameSetting 既定値 = 2,304（本ファミリ 10 件の既定値は上表）の**二層構造**として保持すること。

---

## §10 新規残点（V143+ 起票粒度）

| ID | 残点 | 深刻度 | 試行経路 |
|---|---|---|---|
| **R-N1** | **bss パス読手の残余**: 0x53AC60 / 0x53ADxx の bss→stack **無検査 byte コピー 2 箇所**（`v160_19.txt`: `mov [esp+eax]` / `mov [esp+eax+0x40]` — v130 の「8 箇所/5 経路」インベントリ**外**）＋ 0x45D4xx クラスタ（push bss ×5 と byte コピー ×2）と 0x498A84（push bss）。frame / cookie / 呼出時期が未監査。0x53AC60 は head 後 0x40E759 から呼出されるため §5 結論には影響しないが、追加の cookie abort 経路になり得る | 中 | `callers(0x53AC60)` + 各宿主の frame 探索 + `callers` of 0x45D4B4 群の宿主 |
| **R-N2** | **ESM GMST 適用経路の未検証**: map 0xB35574 に insert 経路が静登録の 2 サイトのみ ⇒ 実行時は lookup+上書きと推定。ESM の GMST ロードが新規 name を insert するか（＝2,304 が実行時も固定か）は ESM スキャン禁止のため未確定。fDifficultyMinValue の既定 -1.0f と ESM 値の関係も含む | 中 | ESM GMST GRUP の読取許可が出た場合に `Ctx.gmst_group()` + FUN_00517xxx 系 GMST ローダの decompiled 突合 |
| **R-N3** | **'S' タグ後 name の読み手挙動**: 0x4A7980 が string 値格納時に name[0] を 'S' へ強制改変（§8-2 補足）→ Dump/WriteOne/LoadAll が改変後 name を如何に扱うか未検証（INI キー照合は大文字小文字無視で成立する推定のみ。保存時 INI のキー表記への影響も未確認） | 低 | Dump 系 0x4A7B10 と LoadAll の name 使用箇所 + 0x4A7980 全 callers（`v160_18.txt` の 0x40E26F 等 4 site）の実体 |
| R-N4 | `FUN_0059C5E0` の実行時到達経路（E8 callers 0 件・唯一の abs32 0xA6A000 は文字列中の偶然 4 バイトで readers 0 件 → register 間接の可能性のみ） | 低 | ホスト 0x59C5E0 の vtable 登録探索（`find_dword` 0xA6A000 周辺の真正 vtable 所在） |

---

## §11 使用証拠一覧

### 11-1 中間スクリプト（すべて `%TEMP%\opencode\`・新規・非 git 管理）

| スクリプト | 出力 | 内容 |
|---|---|---|
| `v160_1.py` | `v160_1.txt` | R1 h-文字列 0 件 / R4 孤立調査 / R6 [0xB34D8C] 再測 / R7 両グローバル xref・ctor 全文 / R8 フラグ 4 件 census・MESSAGES ノード dump |
| `v160_2.py` | `v160_2.txt` | R2 コンテナ xref 19 件・lookup 0x55E000 本体・abs32 分類 / R3 DefaultCombatStyle・Atmosphere 文字列実在調査 |
| `v160_3.py` | `v160_3.txt` | R2-deep: remove 0x4524C0・登録関数 callers・FUN_0041BAE0 本体 / R3-deep: TD 周辺ダンパ / R4-deep: bare 参照 1 件 = 登録スタブ / R7 文字列・宿主 / R8 TD 画像・sev9/Confirm 逆アセンブル |
| `v160_4.py` | `v160_4.txt` | R9 スタブ群（初回・統合 decode）/ R2-name 992 件 first-char 分布 / R4 既定値 / R5 callers 5 件 + cookie 系 census + range 0x40D4D0..0x40F640 |
| `v160_5.py` | `v160_5.txt` | R9-precise: レンジ内 call site 10 件を個別分解 / R9b スランク表 / R2 insert 0x412D30 census（47 件・ecx 分類）/ R5 E9・宿主チェーン・FUN_0040C350/00404A00 呼出し元 |
| `v160_6.py` | `v160_6.txt` | R5 E9 宿主 FUN_00440c40 と上位 / [0xB33398] decompiled 言及 / R2 登録系 B FUN_00419A70 本体（insert ecx=0xB35574 確認）/ v91 CSV 11 名非一致クロスチェック |
| `v160_7.py` | `v160_7.txt` + `v160_names2304.txt` | 登録系 A/B 全 callers の name 分布実測: f=992 / s=1,085 / i=227、overlap 0、h=0 |
| `v160_8.py` | `v160_8.txt` | R5 流動実証: 0x40E6F0..0x40E840（head コピー → FUN_00404A00 call の直線フロー）/ 0x40F560..0x40F5A0 / FUN_00445A10 チェーン |
| `v160_9.py` | `v160_9.txt` | 登録関数 callers の VA 範囲（全 CRT 初期化帯）/ 0x9B00xx スタブ正体（mov ecx,node; jmp 0x403BC0）/ スランク尾部 / site 0x9F9020 name 解決 / FUN_0059C5E0 callers=0 |
| `v160_10.py` | `v160_10.txt` | 0x403BC0 本体（'S' 判定 free）/ CRT レコード列 0xADC480 / 0x9F8E00 前段（CS+FaceGen 6 件）/ file image [0xB27E60]=0x7485D0・[0xB40608]=bss / ctor callers=1 |
| `v160_11.py` | `v160_11.txt` | **孤立 TD 2 個の COL/vtable 確定**（0xABA100→0xA4109C、0xABFBDC→0xA56B40）+ obj+0x114 隣接 4 行の実測一致 |
| `v160_12.py` | `v160_12.txt` | 0x7485D0 実体（MessageBoxA ラッパ）/ 0x4A7980 全文（'S' 改変機構）/ thunk 表 / 0xABFBE8 ctx / cookie 経路 abs32 到達性 / FaceGen prelude names / ctor callers |
| `v160_13.py` | `v160_13.txt` | sev3 ゲート先 0x494933 = epilogue 確定 / 0x4A7980 尾部（'S' 書込 0x4A7A11） |
| `v160_14/15.py` | `v160_14/15.txt` | PE インポート記述子手動パース: 0xA28258=USER32!MessageBoxA / 0xA28074=LeaveCriticalSection / 0xA28068=DeleteCriticalSection / 0xA2808C=GetCurrentThreadId |
| `v160_16..19.py` | `v160_16..19.txt` | セクション表（.rdata 0xA28000..0xB018D2、.data 0xB02000..・bss >0xB32C00）/ エントリ追跡（.bind 保護スタブで断念を記録）/ **head 前 call 62 件全列挙** / **bss 20 ref の store/load 全分類 + builder 0xB34C80 実体 + FUN_00404940 実体** |
| shell inline | （出力は会話） | v91 §7 の出所確認（docs 正規抽出）、v158/v160 突合、IAT grep |

### 11-2 主要実測値（抜粋）

- GameSetting map: insert census 47 件中 ecx=0xB35574 = **2**（両登録系内）/ 登録 A 992（0x9DA4D4..0x9FD314）+ B 1,312（0x9D9D6F..0x9FCF2F）/ 分布 f 992・s 1,085・i 227・他 0 / distinct overlap **0**
- COL/vtable: 0xABA100(numBase=4)→vtable 0xA4109C（refs 1: 0x9E1228）/ 0xABFBDC(numBase=2)→0xA56B40（refs 2: 0x53AE94/0x53AF8D）
- R5: callers(0x40D4D0)=5+E9 1 / cookie 経路全 site = {0x40C37F, 0x59C694, 0x404AA8}+opcode 0xB0BDE8 / bss 0xB3F280 refs 20 件中 store = **0x40E109・0x40E12E の 2 件のみ** / head 前 call = 62 件中 cookie 経路 0
- R6: 0xB34D8C abs32 = **1**（0x494768）、raw disp32 = **1**
- R7: [0xB27E60] file image = 0x7485D0（refs to VALUE = 1）、readers 3、ctor callers = 1（0x9DC337）; 0x6F7070 refs 2,752 / 0x60D0A0 refs 957
- R8: B15 refs = 2（0x49496A/0x494983）、B16 refs = 3（0x494A26/0x494A69/0x494E04、全員 FUN_004949E0 内）、B30 refs = 4、B38 refs = 4
- R9: レンジ内 call 8 + 直後 2 = 10、スランク 0xA232E0 + 0xA23550..5F0（CS 2 + Setting 10）、CSV 非一致 11/11

### 11-3 自己検証

| 検査 | 期待 | 結果 |
|---|---|---|
| `probe_harness.py selftest`（着手時） | 4/4 PASS | **4/4 PASS** |
| v137 既記スタブ 7 件（名前・既定値・obj・atexit） | 一致 | **7/7 完全一致**（残 3 件を追加発見） |
| v158 §3-7（[0xB34D8C] 1 件） | 一致 | **1 件で一致** |
| 2,304 = 992 + 1,312・overlap 0 | — | **成立** |
| xref 19 件の内訳合計 | 19 | 4 + 1 + 8 + 2 + 4 = **19** ✓ |
| bss refs 20 件の分類 | store 2 + load/push 18 | **2 + 18 = 20** ✓ |
| v91 §1.2 の obj+0x114 4 行 | 一致 | **4/4 一致**（0xB07BEC 全ゼロ = 「RTTI 無し」行含む） |
| 既定値 6 種（2048/16384/10/0.05/2/0.5） | v137 と一致 | **一致** |
| IAT 手動パース（5 VA） | 名前解決 | MessageBoxA / LeaveCriticalSection / DeleteCriticalSection / GetCurrentThreadId / **4/4+ 成功** |
| 制約遵守 | 新規 = 本ノート + %TEMP% v160_* のみ | **遵守**（docs・TODO・session-memory・plan・cache・既存 tmp/re・git・ESM は未編集/未アクセス） |
