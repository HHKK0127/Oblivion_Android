# V76: DATA RELATIONSHIP MAP (Oblivion.exe v1.2.0416 Steam)

- 作成日: 2026-10-24 / 手法: probe_harness.py (self-test **PASS**: bytes@0x504050 = `518b4c24188b5424`) + decompiled.c ライン走査 + バイナリ imm32 スキャン
- 対象: 7ハブの writer/reader/cross-link グラフ (Part A)、未アタッチ高参照グローバル (Part B)、フォローアップ (Part C)
- 注意: decompiled.c は global-this の thiscall を this 引数省略で描画するため (例: `mov ecx,0xb35574; call` → decompiled 上はグローバル表示なし)、インスタンス帰属はバイナリ imm32 直接読みを正とする。下記の site はすべて `rd(va,4)` で直接検証済み。

## Part A: 7ハブの edge list

### Hub 1: 0xB0AF48 オペコード/コンパイラテーブル (532 エントリ × 0x28, [0xB0AF48, 0xB10268))
- **構造 (直接ダンプ証拠)**: entry0 = 0xB0AF48。フィールド: +0x00 name1 / +0x04 name2 / +0x08 dword / +0x10 byte / +0x12 ushort / +0x14 ptr/float / **+0x18 evaluator (コード ptr)** / **+0x1C helper (コード ptr)** / +0x20,+0x24 = 0 (sampled entries)。
- **名前 (cstr 検証)**: entry0=`GameMode`、entry54=`PickRefByID`、entry162=`ADD NEW CONSOLE COMMANDS BEFORE THIS ONE` (マーカ、eval=helper=0)、entry163=`MessageBox`、entry252=`Message`、entry496=`EssentialDeathReload`。
- **テーブル本体への参照**: imm32 値 `0xB0AF48` の参照は**恰好 1 箇所** = `0x4FF906` in **FUN_004ff710** (decompiled L198134, `lea eax,[ebx+0xb0af48]`; rd直接検証=0xb0af48)。境界値参照 (コード内, エントリ整列): `0xB0B420`=entry31 と `0xB0C8C0`=entry163 → fun_004fca30 / fun_004fdaf0 / fun_004ff710 / fun_005003f0 / fun_0056ae20; 単一エントリ参照: entry59 0xB0B880 (fun_005a3d00, fun_005bc6b0)、entry100 0xB0BEE8 (fun_0078c160)、entry228 0xB0D2E8 (fun_00717930)、entry324 0xB0E1E8 (fun_0041ed50)、entry371 0xB0E940 (fun_00980dca)。計 18 site / 11 関数。
- **コンパイラ側読手**: FUN_004ff710 (ブロック型 31 エントリ = entries 0..30, bound 0xB0B420; 修復用グローバル DAT_00B361F8, L198212ff)、式評価器側 (0xb0af58/5a/5c/60 参照, decompiled L207898ff)。
- **不整合 (重要)**: 既存主張は evaluator +0x20 / argflag +0x24 / impl +0x18。バイナリ証拠は **evaluator +0x18 / helper +0x1C**、+0x20/+0x24 は sampled entry で 0。→ Part C-1。

### Hub 2: GMST マップ (vtable 0xA40454) と 0xB35574 → **SAME object と判定**
- **根拠**: `rd(0xB35574)` = vptr、`[0xB3557C]` = this+8 バケット配列; FUN_00575b40 の vtable walk は vptr+4 = hash (0x4a80a0 djb2) / vptr+8 = compare (0x584d10 stricmp) を呼ぶ (decompiled L278489ff, `DAT_00b35574` を明示参照 = decompiled 上の2 site)。vtable 0xA40454 本体: +0x00 0x4a8150(ctor), +0x04 0x4a80a0, +0x08 0x584d10, +0x0c 0x6c4f80, +0x10 0x584d30, +0x14 0x46c0d0, +0x18 0x573d00, +0x1c 0xab9c80, +0x20 0x412e20, +0x24 0x404ff0, +0x28 0x4a7cd0, +0x2c/30 0x982d85, +0x34 0x6fe080, +0x38 0x977c50, +0x3c 0x4a81f0。
- **writer**: **FUN_00419a70** と **FUN_0041bae0** (find 0x55e000 → 既存時エラー文字列 `0xA32490` "Setting key '%s' already used in map." → insert 0x412d30)。バルク登録モジュール = fun_009d9d00..fun_009fce20 圏 (fun_009e62b0 だけで 404+717 call)。
- **eraser**: FUN_004055e0, FUN_0040c350 (×8, erase 0x4524c0)。
- **reader**: **FUN_00513e90** (find ×2, imm32 site `0x51404b`/`0x514183` = 0xB35574 直接検証; header decompiled L206159)、FUN_00575b40 (手動 walk, caller fun_00576570)。
- **オペコード dispatch → GMST マップ = YES**: FUN_00513e90 のコードアドレス `0x513e90` がテーブル **entry54 (`PickRefByID`) の +0x18 (evaluator) 項に格納** = imm32 site `0xB0B7D0` 直接検証 (=0x513e90; 別 code site `0x47010c` も直接検証)。 FUN_00513e90 はゲート **FUN_004FAE80** を呼ぶ。
- **マップメソッド実体**: find = 0x55e000 (E8 308 caller / 202 関数)、insert = 0x412d30 (47)、erase = 0x4524c0。

### Hub 3: シェーダテーブルクラスタ (0xB42EC0 / 0xB42F48 / 0xB43070)
- **0xB42EC0 (28スロット遅延確保表)**: writer = **FUN_007b4290** (33 ref 中 31; `(&DAT_00b42ec0)[param_1] = FUN_...` switch/case 遅延生成)。リセット writer = **FUN_007b7250** (先頭で b42ec0..b42ed8 を全ゼロ + DAT_00B430EC refcount リリース = テーブル teardown)。
- **ゲート edges (同一 decompiled body)**: FUN_007b4290 は case 7/0xb/0x15/0x18 で `DAT_00b42f48 > 1` を要求、case 8 で `DAT_00b43070 != 0`、case 0x1a で `DAT_00b42ea5` → **b42f48→b42ec0 YES, b43070→b42ec0 YES**。
- **0xB42F48 (シェーダレベル) writer**: **FUN_007b45f0** (GPU 検出: ati/nv strnicmp、値 0..6、同時に b42f40=0xf/0x2f, b42d74, b42ea5 を書く; decompiled L670319/670323ff) + **FUN_007b7250** (L671929 `= 0`)。reader 137 ref / 92 関数。派生: FUN_007b4590 が `switch(b42f48) → b42e88 = 2/6/8`。
- **0xB43070 writer**: **FUN_007b45f0** (`= 0`) + **FUN_00498340** (decompiled **L122081/L122085** `= 0`/`= 1`)。reader 49 ref / 31 関数 (top: FUN_007dab80 ×5)。
- **配列アクセッサ**: DAT_00B42F54+i*4 を FUN_007b4270(set)/FUN_007b4280(get)。
- **GMST マップ ↔ シェーダ init = NO (グローバル直接 edge なし)**: FUN_007b4290 は 0xB35574/0x55e000(global this) を参照しない。`0x7b40f5` の map_find は **FUN_007b4010** 内で `FUN_0055e000(*(param_4+8), ...)` = **埋め込みマップインスタンス** (this = オブジェクト+8) を使用 (decompiled body 確認)。

### Hub 4: DAT_00B42154 (IDirect3D9*)
- **writer**: **FUN_00761df0** decompiled **L606519**: `DAT_00b42154 = (*DAT_00b42158)(0x20)` = **Direct3DCreate9(D3D_SDK_VERSION=0x20)**。破棄時 L607997 `= 0`。DAT_00B42158 = API プロキシ ptr。
- **reader**: imm32 21 site / 11 関数: fun_0076a040 ×5, fun_00761df0 ×3, fun_00498340 ×3, fun_005deb80 ×2, fun_007645c0 ×2, fun_007623d0, fun_007637d0, fun_00764770, fun_0076d5e2, fun_0076de00, fun_00497d50。
- **cross edge**: **FUN_00498340 は b42154 を読む (×3) かつ b43070 を書く (L122081/122085)** → D3D9 caps → シェーダフラグ (L122071 近傍で `(*DAT_00b42154+0x28)` = CheckDeviceFormat 系 vtable 呼び出し)。

### Hub 5: DAT_00B333C4 (0x804 ヒープ管理グローバル)
- **writer**: **FUN_00406360** L4996 `= 0` / L4999 `= FUN_0066a740()` (ctor)、L56593 `= 0`、L79073/79076 (`= 0` / `= FUN_0066a740()`)、**FUN_005ba4d0** L324692 `= piVar12`。
- **参照量**: decompiled 1647 (V72 と一致)、バイナリ imm32 2663 site / 736 関数。
- **シェーダレベルとの cross = YES** (decompiled 両立計測): b42f48∩b333c4 = **10 関数**: FUN_00694120(2/1), **FUN_004073d0(2/6)**, FUN_006940c0(1/1), FUN_00694050(1/1), FUN_005f3140(1/3), FUN_005ee1b0(1/1), FUN_005b7550(1/1), FUN_0050e340(1/2), FUN_00507e60(1/1), FUN_0049e880(1/2)。 b43070∩b333c4 = **FUN_006a6de0 (3/3)**。

### Hub 6: FUN_004FDE30 (デフォルト opcode helper)
- **E8 caller = 0**。imm32 参照 = **テーブル +0x1C (helper) 項に 528/532 エントリ**で格納 (ラティス 0xB0AF64 + 0x28n、rd直接検証済み)。custom helper は 4 件のみ: entry163/496 = 0x500130、entry252 = 0x500150 (entry162 = マーカで 0)。
- **シグネチャ**: `FUN_004fde30(ushort param_1, int param_2, int param_3, undefined4 param_4)`、400 行、512B ローカルバッファ (decompiled L197309ff)。
- **判定**: 直接呼び出しは存在せず、テーブル dispatch 経由でのみ実行される「全オペコード共通デフォルト実装」。

### Hub 7: INI settings vtables 0xA403A8 / 0xA403F0 (V63 訂正済み)
- **imm32 site (直接検証)**: 0xA403A8 → `0x4A7E15`(FUN_004a7e10), `0x4A7E35`(FUN_004a7e30), `0x4A7E78`(FUN_004a7e60), `0x4A7EF7`(FUN_004a7eb0)。0xA403F0 → `0x4A7EA5`(FUN_004a7e60), `0x4A7EDA`(FUN_004a7eb0)。0xA40454 → `0x4A8092`(FUN_004a7f90), `0x4A8155`(FUN_004a8150), `0x4A81CD`(FUN_004a8180), `0x4A8386`(FUN_004a8330)。
- **構造 edge (YES, 構造的)**: 0xA403F0 の +0x14..+0x30 スロットは 0xA40454 の +0x08..+0x20 と同一アドレス (0x584d10/0x6c4f80/0x584d30/0x46c0d0/0x573d00) = INI クラスは**同一マップ実装を埋め込みメンバとして保持**。ただし INI インスタンスは GMST グローバル (0xB35574) とは別オブジェクト。
- **0xA40454 安装**: FUN_004a8150 (L134187 `*param_1 = &PTR_FUN_00a40454`)、FUN_004a8180 (L134216 `param_1[0x43] = ...` = +0x10C 埋め込み)、FUN_004a8330 (L134330, 1 caller fun_009e0630)。

### Cross-link 判定サマリ
| # | Edge | 判定 | 証拠 |
|---|------|------|------|
| Y1 | opcode table +0x1C → FUN_004FDE30 | **YES** | 528/532 エントリ, ラティス直接検証 |
| Y2 | opcode dispatch → GMST map | **YES** | entry54 eval@0xB0B7D0=0x513e90, FUN_00513e90 map_find@0x51404b/0x514183 (both direct), gate 0x4FAE80 |
| Y3 | 登録モジュール → GMST map writer | **YES** | fun_009d9d00..9fce20 → FUN_00419a70/0x41bae0 |
| Y4 | b42f48 → b42ec0 | **YES** | FUN_007b4290 case 7/0xb/0x15/0x18 |
| Y5 | b43070 → b42ec0 | **YES** | FUN_007b4290 case 8 |
| Y6 | b42f48 → b42e88 / b42f40 / b42d74 / b42ea5 | **YES** | FUN_007b4590, FUN_007b45f0 body |
| Y7 | b42154 → b43070 | **YES** | FUN_00498340 (L122081/122085, b42154 ×3) |
| Y8 | b333c4 ↔ b42f48 | **YES** | decompiled 両立 10 関数 |
| Y9 | b333c4 ↔ b43070 | **YES** | FUN_006a6de0 (3/3) |
| Y10 | 0xA403F0 ↔ 0xA40454 マップ実装共有 | **YES (構造)** | 同一メソッドアドレス群 |
| N1 | GMST map ↔ INI chain 0x44DCF0 | **NO** | 0x44xxxx に 0xB35574 imm32 ゼロ (FUN_0044ac20 含む)。INI chain callers: fun_0044f0c0/0x45bde0/0x461310/0x4d1340 |
| N2 | GMST map ↔ shader init (グローバル) | **NO** | FUN_007b4290 に global GMST 参照なし。0x7b40f5 は FUN_007b4010 の埋め込みマップ |

## Part B: 未アタッチ高参照グローバル (top 15)
docs 提及 36VA + 7ハブ + 兄弟系列 (0xB3F9AC/0xB3F9B0 = 記載済 0xB3F9A8 の系列、0xB333B4 = V72 隣接) を除外した候補池 (refs≥5: 1065 件) から上位15。

| VA | refs | 最寄り writer | hypothesis (証拠) |
|----|------|--------------|-------------------|
| 0xB43110 | 224 | FUN_007b7250 (L672010=0), FUN_007b8990 (L673641) | COM refcount 式グローバル ptr (InterlockedDecrement(ptr+1) → vtable release) — シェーダ系 |
| 0xB4310C | 200 | FUN_00441850 (L50701), FUN_004425d0 (L51350), FUN_00444340 | 同上パターン — 設定系シングルトン |
| 0xB43108 | 188 | FUN_0055e340 (L260068), FUN_0055f550, FUN_0055fdf0 | 同上パターン — コンソール/メニュー系シングルトン |
| 0xB43104 | 164 | FUN_00406950 (L5122) | 早期 init の COM 式シングルトン (0xB350D8 と比較) |
| 0xB02E24 | 157 | init: FUN_009d9500 → FUN_004040e0(&DAT) (L1023207) | バッファ書位置カウンタ (`0x500 - DAT`, FUN_00407ae0 L5905ff)。0xB02E1C 起点構造体の +8、FUN_00513e90 は +0x14 を参照 |
| 0xB42CDD | 156 | FUN_007abe30 (L664423=0), FUN_007acf40 (L665136=1) | シェーダ/エフェクト有効フラグ (読手 FUN_00848fa0) |
| 0xB42754 | 155 | FUN_00772060 (L618178) | レンダラ側オブジェクト ptr: vtable+0x54 呼び出し + +0xa08 float フィールド (FUN_007716e0/771830) → D3D device/RT ラッパー候補 |
| 0xB42E8C | 125 | (単純代入なし — 構造体経由) | コールバック proc ptr `(*DAT)(local_d8)` (FUN_007b2aa0, FUN_007b7fc0) |
| 0xB39B80 | 92 | FUN_00553550 (L252582 = FUN_00553140(), L252586=0) | アロケート済み大型オブジェクト (≥+0xDB8 訪問) ptr。null-check: FUN_004783a0, FUN_00523d80 |
| 0xB430DC | 84 | FUN_007b7250 (L671946=0), FUN_007b8990 (L673238) | COM 式シングルトン (シェーダ系兄弟) |
| 0xB3FD64 | 82 | InterlockedInc/Dec のみ (FUN_004a24f0 L129033, FUN_004a2970, FUN_0051ab30) | グローバル生存オブジェクトカウンタ |
| 0xB430F0 | 80 | FUN_007b7250 (L671954), FUN_007b8990 (L673282) | COM 式シングルトン (シェーダ系兄弟) |
| 0xB42EB8 | 79 | FUN_007a9820 (L662320), FUN_007aaa30 (L663457/663548) | エフェクトパラメータ値 |
| 0xB35C24 | 76 | FUN_004ca390 (L158507=0), FUN_004d5600 (L167053) | 設定チェーンが返すオブジェクト ptr (return DAT_00b35c24 @ FUN_004440c0, null-check @ FUN_00441d50) |
| 0xB39E04 | 72 | FUN_0055e390 (L260189), FUN_0055f750 (L261159) | 0xB39B00 系の兄弟シングルトン (`*DAT=0` クリア) |

## Part C: フォローアップ候補
1. **オペコードテーブルのフィールド offsets 不整合**: 主張 (evaluator +0x20 / argflag +0x24 / impl +0x18) vs バイナリ証拠 (evaluator **+0x18** / helper **+0x1C**、+0x20/+0x24 = 0)。アンカーずれか別サブテーブルかを再導出し、RE_EXE_REPORT 等の記述を訂正。
2. **b43070 writer の意味論**: FUN_00498340 L122081/122085 の `= 0/1` がどの D3D9 caps (L122071 CheckDeviceFormat 系) の結果か、および b42154 との入出力経路を確定。ついでに FUN_007b45f0 への入力 (GPU 検出呼び出し元) を特定。
3. **b42f48 writer の残余**: FUN_007b7250 L671929 (`= 0`) の呼び出し経路 (teardown シーケンス) を特定 — writer 自体は特定済 (FUN_007b45f0 / FUN_007b7250)。
4. **COM 式シングルトン 4 本 (0xB43104/108/10C/110) の同定**: setter インスペリ (FUN_00406950 / FUN_0055e340 / FUN_00441850 / FUN_007b7250) を追い、各スロットが指すサブシステム名を確定。ついでに 0xB430DC/0xB430F0/0xB430EC グループ。
5. **GMST マップのライフサイクル**: teardown (FUN_004055e0/FUN_0040c350) の順序と、バルク登録モジュール fun_009d9d00..9fce20 が「デフォルト登録」と「ESM オーバーライド (377/382 主張)」のどちらかを再検証。
6. **0xB42754 の正体**: FUN_007716e0 の vtable+0x54 戻り値ビットと FUN_00772060 の setter 呼び出し元から、D3D9 device / swap chain / RT ラッパーのいずれかを確定。
7. **0xB3CAC0 の用途**: FUN_00560920(`&DAT_00b3cac0`, node) は vtable+4 の値とアドレスを比較するループ = アドレス=キーの identity lookup。キー供給元と値の意味 (MGEF/FormID マップ候補) を特定。
8. **FUN_007b4010 の埋め込みマップ**: this = `*(param_4+8)` の所有オブジェクトとキー領域 (GMST 文字列キーを引くか) を確認 — N2 判定の裏取り。
9. **PickRefByID (entry54) 実装の完全復元**: FUN_00513e90 がなぜ GMST map を ×2 参照するか (0x51404b/0x514183)、ゲート 0x4FAE80 との関係、デcompiled body とバイナリ範囲 [0x513e90, 0x514660) の対応を精査。
10. **ハーネス/索引の既知問題**: (a) `rd()` は .rdata 終端 0xB01A00 と .data 開始 0xB02000 の 0x600 ギャップをまたぐ読みでデータ側を 0x600 シフトする (バグ site: 0x513e90 の格納先は 0xB0B1D0 ではなく **0xB0B7D0**) — 修正または既知問題として明記。(b) `dec_index` は decompiled.c ヘッダの全関数を含まない (例: FUN_00513E90, FUN_00514660 欠落) — 関数帰属は decompiled.c ヘッダ正規表現を使う。(c) テーブル entry162 マーカ前後 (entries 0..161 vs 163+) の区分 (script/console) を確定。

---
検証サマリ: ハブ 7/7 カバー / cross-link YES 10 + NO 2 / Part B 未解決 15 / harness self-test PASS。
## 追記: 列GT再検証 (2026-10-24, V78)

- 手法: **raw PE 読取のみ** (struct で PE セクションヘッダ走査 → VA→file_off 変換、probe_harness 不使用)。ハーネス self-test は `H.Ctx()` 導入で **PASS** (bytes@0x504050) を確認済みだが、本検証は raw 読取で自己完結。
- **セクション訂正 (重要)**: テーブル 0xB0AF48–0xB10268 は **.rdata ではなく .data 内** (.data va=0xB02000 rawptr=0x700C00 → file_off = VA − **0x401400**)。.rdata の delta 0x400E00 で読むと −0x600 シフト = Part C-10(a) のハーネス `rd()` バグの原因。entry54 の +0x18 格納先は **0xB0B7D0** (GT 側が正)。

### 1. 列カウント表 (532 エントリ全数、u32、raw 読取)

| 列 | nonzero | zero | 検証手順の期待値 | 判定 |
|----|---------|------|------------------|------|
| +0x00 name1 | 532 | 0 | - | 全エントリ cstr 可 |
| +0x04 name2 | 532 | 0 | - | |
| +0x08 opid | 531 | 1 (idx0=0) | 532 unique | u16 opid = **532 unique** ✓ |
| +0x0C | 531 | 1 (idx162) | - | |
| +0x10 | 408 | 124 | - | flags 列 |
| +0x14 | **281** | 251 | 2 | **期待 2 は誤り** (実測 281。arg-spec ptr 群: 0xB0ABAC=OnHit/OnActivate 系, 0xB0A11C, 0xB0AA20 等) |
| +0x18 impl | **527** | 5 | ≈528 (zero 4?) | zero は **5**: idx162 (マーカ), idx188/189 (UnusedFunction1/2), idx216 (GetScriptVariable), idx242 (GetQuestVariable) |
| +0x1C helper | **531** | 1 (idx162) | 528 (=532−4) | 既定 0x4FDE30 は **528/532**、例外 **ちょうど 4 件** (下表)。nonzero は 531 (例外のうち zero は idx162 のみ) |
| +0x20 eval | **169** | 363 | 169 | **完全一致** ✓ |
| +0x24 argflag | **19** | 513 | 19 (0x1×9, 0x100×10) | **完全一致** ✓ (値の内訳まで一致) |

**+0x1C の既定 0x4FDE30 以外はちょうど 4 件 (GT の記述どおり)**:
| idx | opid | name | +0x1C |
|-----|------|------|-------|
| 162 | 0x0183 | ADD NEW CONSOLE COMMANDS BEFORE THIS ONE!!! (マーカ) | 0x00000000 |
| 163 | 0x1000 | MessageBox | 0x00500130 |
| 252 | 0x1059 | Message | 0x00500150 |
| 496 | 0x114D | EssentialDeathReload | 0x00500130 |

**+0x24 の 19 件 (0x1 = 9 件 / 0x100 = 10 件)**:
- `0x1` (9, すべて +0x20 も nonzero): GetIsClass, GetIsRace, GetIsSex, GetInFaction, GetIsID, GetFactionRank, IsGuard, GetIsReference, GetIsPlayableRace
- `0x100` (10, すべて +0x20 = 0): Activate, PositionWorld, PositionCell, MoveToMarker, CloseCurrentOblivionGate, ForceFlee, ForceTakeCover, ForceCloseOblivionGate, CreateFullActorCopy, DeleteFullActorCopy

### 2. entry54 ダンプと 0x513E90 の正体

| idx | base VA | opid | name1 / name2 | +0x10 | +0x14 | +0x18 (site) | +0x1C | +0x20 | +0x24 |
|-----|---------|------|---------------|-------|-------|--------------|-------|-------|-------|
| 54 | 0xB0B7B8 | 0x0117 | PickRefByID / PRID | 0x00010000 | 0xB0A7EC | **0x00513E90** (site **0xB0B7D0** ✓) | 0x004FDE30 | **0** | **0** |

- **0x513E90 = FUN_00513e90 = PickRefByID のコマンドハンドラ** (decompiled.c L206159 ヘッダ `// FUN_00513e90 @ 0x513e90`, 本体 L206164–206222, 次ヘッダ `// FUN_00514660 @ 0x514660`)。プロローグは SEH + security-cookie (`push -1; push 0x9b7148; mov eax,fs:[0]; mov eax,[0xb30aac]; xor eax,esp`)。8 引数 (esp 直読み) を受け、**ゲート FUN_004FAE80 (arg decoder) で引数デコード → `*(byte*)(obj+4) - 0x31 < 3` 型チェック → `"\"%s\" (%08x)"` 書式化 → コンソール出力 (FUN_00571f90 / FUN_005723e0) → 成功 1 / 失敗 0 を return**。
- **判定: 値 (VM 評価結果) を返す expression evaluator ではなく、ステータス 0/1 を返すハンドラ**。直接 E8 呼び出しは 0 件、imm32 出現は 0xB0B7D0 (テーブル格納) と 0x47010c の 2 箇所のみ → +0x18 経由 dispatch が唯一の通常経路。
- entry54 は **+0x20 = 0 / +0x24 = 0** → v76 の "(sampled)" はまさにこのエントリに基づく観測で、全数では 169 / 19 が nonzero。

### 3. サニティ

- idx0 = `GameMode` opid0x0000 +0x18=0x005022D0 ✓
- **opid 0x0015 → idx21 `OnLoad` +0x18 = 0x00504560** ✓ (vv_v4_scan GT self-test と一致。「idx15」は文字どおり idx=15 ではなく opid=0x15 の意味だった)
- idx15 = `OnPackageStart` opid0x000F +0x18=0x005043E0 (idx と opid は 31 以降で乖離)
- idx50 = `Show1stPerson`/`S1ST` opid0x0113 base 0xB0B718 **+0x18 = 0x0050A050** ✓ → V69 の「console 名 → opcode 0x113 → +0x18 → handler 0x50A050 (this+0x588 setter)」と一致 = **+0x18 = impl の独立裏付け**
- idx164 = `GetDistance` opid0x1001 +0x18=0x005003F0 / +0x20=0x004F43C0 → 式系コマンドも実装本体は +0x18 に置き、+0x20 は別列 (条件/評価系) を持つ

### 4. 判定 (verdict)

**Project GT 成立 / v76 の列ラベルは誤り**: +0x18 = **impl (ハンドラ)** (nonzero 527、per-entry 変動、Show1stPerson=0x50A050 や PickRefByID=0x513E90 は status 返却ハンドラ)、+0x1C = **helper** (既定 0x4FDE30 = 528/532、例外 4 件)、+0x20 = **eval 列** (nonzero 169)、+0x24 = **argflag** (nonzero 19 = 0x1×9 + 0x100×10)。v76 の「+0x18 evaluator」は 0x513E90 の実体がコマンドハンドラであることから**呼称誤り**、v76 の「+0x20,+0x24 = 0 (sampled)」は**サンプリング偏り**で全数カウントにより反証 (169 / 19)。ただし site 値 0xB0B7D0 と「+0x1C = helper」は v76 側も正。

### Part C-1 決着: **[resolved] 説 (v76) はラベル誤り — Project GT 是正なし**

- 既存主張 (impl +0x18 / helper +0x1C / evaluator +0x20 / argflag +0x24) は全数カウントで支持 (527 / 531 / 169 / 19)。RE_EXE_REPORT の「entry+0x20 = 条件関数ポインタ (169 件)」とも件数一致 (呼称 evaluator vs condition の残余は別タスク)。
- v76 の evaluator +0x18 は impl/handler の誤称。+0x20/+0x24 = 0 は entry54 単一サンプルの一般化。
- 追加事実: +0x18 の zero は 4 でなく **5 件** (idx216/242 は impl を +0x20=0x4F6710 に置き +0x18=0)。+0x14 の nonzero は **281** (想定 2 は棄却)。L43/L55 の「528/532」は +0x1C 既定値として**正しい** (nonzero 531・zero は idx162 のみ)。
- 本文中の「+0x18 evaluator」表記 (L10 / L14 / L21 / L56) は本節で impl/handler に訂正済み。Supermemory 追加: **なし** (Project GT 正のため条件未満)。

検証: `python "C:\Users\hiroki.kogarumai\AppData\Local\Temp\opencode\round_verify.py"` → 本追記直後に実行し **ALL OK**。

## 追記: +0x14 列の実体 (2026-10-24, V83)

- **+0x14 = 引数記述子（arg descriptor）配列の先頭ポインタ**（仮説「name2 ペア」「alternate handler」は棄却）。nonzero **281/532**（V78 計測と一致）、ユニーク先頭 **94**、全件 .data の **0xB0A11C–0xB0AF3C**（表基底 0xB0AF48 直下のプール）。.text/.rdata へ落ちる値は 0 件。**+0x14 ≠ 0 ⟺ +0x12（u16 引数個数）> 0 が双方向完全一致**。
- レコードは 12 バイト `{u32 name_ptr(.rdata), u32 type(0x00–0x25 の38種), u32 optional(0/1)}` が argn 個連続（全 281 エントリで argn*12 が有効、0 例外）。型の例: 0=String/Filename, 1=Integer, 2=Float, 4=ObjectReferenceID, 6=Actor, 9=Cell, 13=Topic, 26=Container, 28=Crime Type, 31=Magic Effect。optional=1 は 179 レコード中 52 件（41 件は名前に "(optional)" を含む）。
- 共有例: 0xB0A6CC={'Integer'} x42、0xB0ABAC={'Actor (optional)'} x12（OnHit/OnActivate/OnDeath 等）、0xB0A11C={'String'} x13（MessageBox/Message 等）。OnAlarm(argn=2)=['Crime Type','Actor (optional)'] と引数列が直感的に整合。
- 消費者: スクリプトコンパイラ **FUN_004ff710 @ 0x4FF710**（decompiled.c L198215–198229）。`(*entry[+0x1C])(argn(+0x12), argdesc(+0x14), argbuf, ctx)` で dispatcher **0x4FDE30**（528 件）に渡し、記述子に従い引数をデコード/検証。旧 GT「+0x14 = OnHit/OnActivate の 2 件のみ」は著名 2 件が同一記述子 0xB0ABAC を共有しただけのサンプリング事故。v76 の entry54「+0x14 名前ポインタ」も誤読（0xB0A7EC は {'ObjectReferenceID',4,0}）。
- 連動訂正: **+0x10 は単一フィールドではない**（byte0=flagA 0/1 が 272 件、byte1(+0x11) は全 532 件 0、u16 +0x12=argn のパック。u32 で nonzero=408 は 272∪281 の和集合）。+0x04 は実別名 176 件+空文字列共有 ptr 0xA2F7EC が 356 件、+0x0C は非空説明 132 件・0 は idx162 のみ。詳細は `tmp/re/v83_col14_meaning.md`。
