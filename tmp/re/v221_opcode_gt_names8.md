# v221_opcode_gt_names8.md — opcode/GT 残点 第8束（V216 継続）

- 作成日: 2026-11-01 / 担当: TODO serial 215 (V215・中)  spawning / 分析: サブエージェント
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 Steam / ImageBase 0x400000）
- スコープ: `tmp/re/v216_opcode_gt_names7.md` §6 新規残点 **V-216-1..6**
- 契約: 分析専任（docs/GT/TODO・cache・ソース編集禁止・既存 tmp/re 上書き禁止）。成果物は本ファイルのみ（新規）
- 手法: capstone 5.0.7（v172_common.py 経由・セクション個別変換）＋ `tmp/re/cache/named_funcs.txt` 窓
- selftest: 着手時 `probe_harness.py selftest` → **4/4 PASS**

## 1. 【V-216-1】{0,3,5} は表示単位 enum である件 — **[確定]**

### 1.1 表示単位の語彙テーブル＝**GameSetting 群 `sMagicEffectItem*`（既定値は .rdata に埋込）**

- .rdata 0xA325D4..0xA32808 に **GMST 名 → 既定文字列** のペア列を実読み（`v221_6_out.txt`）:
  `sMagicEffectItemFor`="for" / `On`="on" / `In`="in" / `SecondsSingular`="sec" / `Percent`="%" / `PointsSingular`="**pt**" / `SecondsPlural`="secs" / `PointsPlural`="**pts**" / `sMagicEffectItemFeet`="**ft**" / `Lock`="Lock" / `CreateLock`="Create" / **`sMagicEffectItemUpToLevel`="up to level"** / `Fortify`/`Restore`/`Drain`/`Damage`/`Absorb` / `sMagicRangeStrike`="Strike"。
- 各 GMST は 0x9D9D60..0x9D9F7F の **static initializer スタブ**（`push 既定値; push 名前; mov ecx, GMSTオブジェクト; call 0x419A70`）で登録。グローバル配置は stride 8 で **0xB33460(For) .. 0xB334E8(RangeStrike)**、`Feet=0xB334A0` / `UpToLevel=0xB334B8` / `PtSg=0xB33488` / `PtPl=0xB33498` / `Percent=0xB33480`（実測・`v221_7/8_out.txt`）。

### 1.2 bit30/bit31 → 単位語の分岐本体＝**`FUN_00413BE0`（EffectItem 記述子組立器）**

- `FUN_00413BE0`（0x413BE0..0x413FB6、`ret 0x18`）が **18 GMST グローバルの大半を直接消費**（`v221_8/9_out.txt`）。 magnitude 部の分岐（asm 逐語）:
  ```
  0x413DBE  mov ecx,[eax+0x58] / shr ecx,0x1e / test cl,1 / je 0x413df2   ; bit30 = kMgefFlag_MagnitudeIsLevel
  0x413DC9  mov ecx,0xb380d0; call 0x403c00            ; レベル倍率 float 取得（fild magnitude × 倍率 → ftol）
  0x413DDE  mov edx,[0xb334b8]                         ; GMST sMagicEffectItemUpToLevel = "up to level"
  0x413DE4  push eax / push edx / push 0xa32580("%s %d") → "up to level %d"
  0x413DF2  ... shr ecx,0x1f / test cl,1 / je 0x413e10 ; bit31 = kMgefFlag_MagnitudeIsFeet
  0x413DFD  mov eax,[0xb334a0]                         ; GMST sMagicEffectItemFeet = "ft"
  0x413E02  push eax / push edx / push 0xa32588("%d %s") → "%d ft"
  0x413E10  既定: magnitude==1 → PtSg("pt") else PtPl("pts")、"%d %s"
  ```
  付随: bit3（shr 3）→ Percent(" %d %s" に %）、mgefCode=='OPEN'/'LOCK' → "Lock" 味、bit8/NoMagnitude・bit7/NoDuration・bit9 は該当部を省略、duration 部は `FUN_004133C0`(GetDuration)→1 なら "sec" else "secs"（v216 §1.3 の GetDuration 確定と整合）。
- → **「bit30=MagnitudeIsLevel→"up to level"（level 表記）／bit31=MagnitudeIsFeet→"ft"（feet 表記）／否则 pt(s)」は EXE 側で文字列まで含めて確定**。v216 §2.2 の CEF フラグ名（表示専用ビット）の帰結は正しい。
- 註: 本関数の単位選択は述語 `FUN_004AE590/0x4AE5A0` を呼ばず **インライン `shr/test` で実装**（callers 一覧に 0x413Bxx が無い理由）。

### 1.3 `FUN_004de980` の {0,3,5} — **「base=EffectSetting のときの表示単位種別コード」** [確定]

- ジャンプテーブル完全解読（`v221_2_out.txt`、byte 表@0x4DEBD0 + dword 表@0x4DEBA8・絶対アドレス）: switch は **`this->vt[0x170]()`（GetBaseForm）の返す form の formType byte（[form+4]）**（0x12..0x2B）。
  - **formType 0x20 → 0x4DE9EB**: `base=[esi+0x1C]`（=EffectSetting*）に bit30 述語 → **3**、否则 bit31 述語 → **5 or 0**。★ 本残点の該当ケース
  - 0x12(TESForm 汎用)→ `FUN_004D9890([esi+0x1C])` で 4/0、0x15→ `[esi+0x1C]+0x64` で 1/6、0x18→8、0x17/0x24→2、0x1A→ `[esi+0x1C]+0x7C` bit1、0x23→ プレイヤ状態連鎖（0xA/0xB/2/7/9/1）、既定 1、0x1C/0x1D/0x1E/0x25/0x29/0x2A→0。
  - formType 実測対応（ctor の `mov byte [esi+4], imm` 走査・`v221_5_out.txt`）: **0x12 = FUN_004B40A0（基底＝TESForm）**、**0x20 = FUN_004AE660**（`call 0x4B40A0` で TESForm 派生・sub-vtable +0x24/+0x30/+0x48＝TESModel/TESDescription/TESFullName/TESIcon 継承構造、`[esi+0x58]=0` 初期化＝**mgefFlags**）→ **formType 0x20 = EffectSetting [確定]**。
- callers は 2 件のみ（`e8` 実測）:
  - **0x5A4ADB（`FUN_005A4980` 内）**: `eax==5`（edi≠0 時）または `eax==3` → プレイヤ `vt[0x174](1,1)` 取得 → `FUN_004DBAE0`（後述の最近効果点探索、level/feet 許容フラグ付き）へ。＝**{3,5} はゲート（表示単位種別の別用途）**。
  - **0x5A8CAE（`FUN_005A8C90`＝メニュ更新系）**: ebx=enum を保持し、`ebx==7||0xA` / `ebx==1` で追加ゲート、`push ebx; call 0x5A8BC0`（カテゴリ処理）、制御コントロール 0xFAF へ float 設定（`0xB3B350` グローバル＝メニュー系）。
- → **{0,3,5} は「表示単位 enum（0=pt系既定 / 3=MagnitudeIsLevel / 5=MagnitudeIsFeet）」であることを EXE 側で確定**。ただし callers は文字列インデックスではなく**種別コード（ゲート/切替）として**使用し、単位文字列の実挿入は `FUN_00413BE0` 側が mgefFlags を直接読んで行う（二経路）。
- **未解明（残）**: `FUN_004de980` 全体（enum 0..11）の正式機能名（=「アイテム/エフェクト文脈の表示・操作カテゴリ選択器」という記述止まり）。enum の 0/1/2/4/6/7/8/9/0xA/0xB の各意味（0x23 帯のプレイヤ状態連鎖）は未同定。`[esi+8]` bit13 早期退出の意味も未解明。

### 1.4 GT ドラフト（反映は要承認）

- (G) `FUN_004AE590/0x4AE5A0` 行: **`EffectSetting::IsMagnitudeIsLevel()/IsMagnitudeIsFeet()`（V221）**: `[this+0x58]` mgefFlags の bit30/bit31 述語。表示単位は **`FUN_00413BE0`（EffectItem 記述子組立器）**が bit30→GMST `sMagicEffectItemUpToLevel`("up to level") / bit31→`sMagicEffectItemFeet`("ft") / 既定→`PointsSingular/Plural`("pt"/"pts") で挿入（GMST グローバル 0xB33460..0xB334E8・既定値埋込は 0x9D9D60.. の initializer）。`FUN_004de980` の戻り {0,3,5} は base=EffectSetting(formType 0x20) 時の同種別コード（0/3/5）。

## 2. 【V-216-6】FUN_004AE5A0 callers 5 件の再分類 — **[確定]**

v193b の「bit31=負センチネル」解釈は不成立（v216 §2.2 訂正）を前提に、5 件すべてを実測分類（`v221_1/10_out.txt`）:

| # | caller | 含まれる関数 | bit31 使用形態 |
|---|---|---|---|
| 1 | 0x4DBB5A | **`FUN_004DBAE0`** | 引数 byte[ebp+0x10]（許容フラグ）と組み合わせ、「base form が formType 0x20(EffectSetting)」「[ebp+0xC]→bit30 / [ebp+0x10]→bit31」で**効果点列挙を feet 型に絞る/含める**。非該当→失敗 ret0 |
| 2 | 0x4DEA02 | `FUN_004DE980` | §1.3: 表示単位種別コード **5** の生成 |
| 3 | 0x650698 | **`FUN_006505D0`** | グローバル byte `0xB3BA80`==0 のとき **bit31 立ちを必須条件**として要求（!=0 のときは bit30 立ちを要求、互斥）。通過時のみ `[0xB3BA9C]` コレクション（contains=0x446C30 / add=0x446CB0）へ esi を追加。esi は [+8] formFlags bit5/11/14 チェック付き TESForm 系 |
| 4 | 0x6788A2 | **`FUN_00678750`** | 候補走査（linked list `[esp+0x14]`）で、採用候補 `edi` の GetBaseForm() に bit31 立ち→**`[esp+0x12]` フラグ=1**（feet 型マーカ）＋ ebp=edi 採用 |
| 5 | 0x678985 | `FUN_00678750` | 同上（2 候補目のルート。bit31 立ち→採用＋フラグ1） |

- 補足: 同関数内に bit30 版（0x678A98 → フラグ 0）が対で存在 → **`[esp+0x12]` = 「選択効果が feet 型か level 型か」の双対マーカ**と整合。
- **結論**: bit31 の別系統利用は存在せず、**5 件すべてが MGEF kMgefFlag_MagnitudeIsFeet の正規用途（表示単位種別・効果選別フィルタ）**。v193b H.2 の「負センチネル」仮説は完全に棄却 [確定]。
- **未解明（残）**: `FUN_006505D0` の `0xB3BA80` モード選択（bit30 必須/ bit31 必須の切替主体）と `0xB3BA9C` コレクションの実体名。`FUN_00678750` の全体機能名（候補エフェクト走査＋0x566DC0/0x5E0F30/0x5E6FA0 ゲートの目的）。

### GT ドラフト（反映は要承認）

- (H) GT「`FUN_004AE5A0` callers」行: 5 件＝`FUN_004DBAE0`（許容フラグ）/`FUN_004DE980`（表示単位コード5）/`FUN_006505D0`（`0xB3BA80` モードで bit31 必須→`0xB3BA9C` 追加）/`FUN_00678750`×2（feet/level 双対マーカ `[esp+0x12]`）。v193b「bit31=負センチネル」は棄却。

## 3. 【V-216-3】`0x450C20` の正式契約 — **[確定]**

### 3.1 本体＝**`TESFile::Read(void* dest, UInt32 size)`**（thiscall / `ret 8` / 戻り=bool）

- asm 全文実測（`v221_12/13_out.txt`、0x450C20..0x450F01）。呼び出し側は 239 件（例: `push 0x200; push esp`＝EDID 文字列 / `push 1` / `push 4`＝formID / `push 0x10; ExtraDataPtr`＝SPIT 一括）。
- **引数契約**: arg1=`dest`（ebp）、arg2=`size`（edi）＝**呼び出し元バッファの容量上限**（読取バイト数指定ではない）。
- **内部状態**: `[this+0x254]` = **現在 chunk（サブレコード）の残余バイト数**、`[this+0x260]+[0x25C]+0x1A` = chunk ファイルオフセット（+0x1A=26=レコードヘッダ長）、`[this+0x414]` = 展開済みバッファ（compressed `[this+0x244]&0x40000` 系）、`[this+0x23C]` = ファイル種別 fourcc（グローバル `[0xB05E20]` と比較）。

### 3.2 分岐と超過時挙動（実測）

| 条件 | 挙動 | 戻り |
|---|---|---|
| 残余==0 | 何も読まず | **1（true）** |
| **残余 <= size**（通常経路） | **残余バイトをちょうど読み切る**（普通: stream vtable[1] 経由 / compressed: 展開バッファ `[0x414]+[0x260]+6` から `memcpy`(0x9812C0)）。**SPIT 16B・formID 4B はこの経路＝バイナリが完全読出される正しい経路** | 読了で **1**、短読なら `"First ReadFile() in GetChunkData failed"`（0xA38DE0 帯）エラー報告で **0** |
| **残余 > size**（超過） | `dest[size-1]=0` → **size-1 バイトのみ読み** → 文字列警告 **`"Chunk size %d too big in chunk %s_ID in form %s_ID"`**（0xA38E18、引数=残余・chunk fourcc 文字列・form fourcc 文字列）を `0x4A7A60` で出力 | **1（true・切り詰め成功扱い）** |
| seek 補正 | 前回 read 後の位置不整合時（`[0x264]!=0`）に chunk オフセット+26 へ `Seek`（stream vtable[C]） | — |

### 3.3 呼称訂正の確定

- **v177 §（ENAM 追補）の「4-byte formID reader」呼称は誤り（呼称訂正を確定に昇格）**: `push 4; call 0x450C20` は「4 バイト読むための reader」ではなく **「容量 4 のバッファへ chunk 残余（4）を読む汎用ブロック reader」**。本関数はサブレコード/チャンク一括読出のエンジン共通ルーチン（`push 0x200` 文字列・`push 1` byte・`push 0x20` 以上の一括構造体まで同一関数）。
- **GT ドラフト (I)**: 「`0x450C20`＝4-byte formID reader」行 → **`TESFile::Read(dest, size)`（V221）**: size=バッファ容量上限。残余<=size なら残余ちょうど読出（true/短読で false）、残余>size なら size-1+NUL 切り詰め＋"Chunk size %d too big" 警告（true）。compressed は展開バッファ `[this+0x414]` 経由。

## 4. 【V-216-4】ENCH/ALCH/INGR の ExtraData 契約 — **[確定]**

### 4.1 クラス同定（ctor 実測・formType / GetMagicType 定数スタブ / 共有 LoadForm）

| クラス | formType | ctor | size | LoadForm | GetMagicType(s6) |
|---|---|---|---|---|---|
| SpellItem | 0x10 | `FUN_0041D420` | 0x44 | `0x41B2D0` | 動的 `0x4D9B40`（SPIT type） |
| **EnchantmentItem** | **0x0F** | `FUN_00419570` | 0x44 | **`0x41B2D0`（SpellItem と共有）** | **const 6**（`0x7AFE90: mov eax,6`）＝kMagicType_Enchantment |
| **AlchemyItem** | **0x28** | `FUN_00412BE0` | 0x80 | **`0x41B5B0`（INGR と共有）** | **const 7**（`0x4059C0`）＝kMagicType_AlchemyItem |
| **IngredientItem** | **0x19** | `FUN_004198F0` | 0x80 | `0x41B5B0` | **const 8**（`0x419750`）＝kMagicType_Ingredient |

- 基底 ctor は SpellItem/ENCH=`FUN_004194E0`（MagicItem 系基底）、ALCH/INGR=`FUN_00412B50`。CEF Magic::MagicTypes（6/7/8）と定数スタブが一致し、v216 §3.1 の値対応表が閉じる。

### 4.2 ExtraData 契約（埋め込み sub-vtable 実読み・`v221_16_out.txt`）

| クラス | sub 位置 / vtable | ChunkType(s9) | Ptr(s10) | Size(s11) | 着地 |
|---|---|---|---|---|---|
| SpellItem | +0x18 / 0xA34BF4 | **'SPIT'**(0x54495053,`0x41D250`) | `lea [ecx+0x1C]`(0x419000) | **0x10**(0x8C4830) | obj+0x34 |
| EnchantmentItem | +0x18 / 0xA33C3C | **'ENIT'**(0x54494E45,`0x419730`) | `lea [ecx+0x1C]`(0x419000) | **0x10**(0x8C4830) | **obj+0x34** |
| AlchemyItem | +0x24 / 0xA32134 | **'ENIT'**(`0x419730`) | `lea [ecx+0x54]`(0x419740) | **8**(`0x419750`) | **obj+0x78** |
| IngredientItem | +0x24 / 0xA33F14 | **'ENIT'**(`0x419730`) | `lea [ecx+0x54]`(0x419740) | **8**(`0x419750`) | **obj+0x78** |

- → **SpellItem と同構造（LoadForm が sub-vtable s9/s10/s11 を叩いて SPIT/ENIT を一括読出）であることを確認** [確定]。v216 の疑問「ENCH fourcc の ExtraData='ENIT'?」は **肯定**（ただし ENCH のみ 16B・ALCH/INGR は 8B）。
- LoadForm 契約: EDID/EFID(効果リスト `FUN_004154B0`)/FULL は分岐処理、未知 fourcc は s15(LoadBases) へ: SpellItem/ENCH=`0x60CF60`(no-op)、**ALCH/INGR=`0x419760`（'DATA'/'MODL'/'MODT'/'SCRI' 処理）**。四cc は sub-vtable で動的照合＝ESM 由来のエンジン共通パターン。
- Compare/Copy（s12/s13）: ENCH=`0x419010/0x419060`（obj+0x38/+0x3C を比較・複写、`_RTDynamicCast` RTTI 0xB033C8）、ALCH=`0x412A00/0x412A40`（+0x78 dword/+0x7C byte、RTTI 0xB0320C）、INGR=`0x419840/0x419880`（同 +0x78/+0x7C、RTTI 0xB0373C）。

### 4.3 ENIT フィールド実測範囲

- **ENCH ENIT (16B @obj+0x34)**: `+0x34 u32 / +0x38 u32 / +0x3C u32 / +0x40 u8`（ctor 既定値 +0x34=2, +0x38=-1, +0x3C=-1, +0x40=0。compare は +0x38/+0x3C 主）。
- **ALCH/INGR ENIT (8B @obj+0x78)**: `+0x78 u32 / +0x7C u32(比較は byte)`。
- **未解明**: 各フィールドの意味論（cost/uses/flags の対応）。

### 4.4 GT ドラフト（反映は要承認）

- (J) GT「ENCH/ALCH/INGR」行: **ENCH=class formType 0x0F（`FUN_00419570`・MT=6）＝ENIT 16B @+0x34（ChunkType `0x419730`/Ptr `0x419000`/Size `0x8C4830`）／ALCH=0x28（`FUN_00412BE0`・MT=7）・INGR=0x19（`FUN_004198F0`・MT=8）＝ENIT 8B @+0x78（同 impl `0x419730`/`0x419740`/`0x419750`）**。LoadForm は SpellItem(0x41B2D0)／ALCH-INGR 共有(0x41B5B0)。SpellItem と同一 vtable パターン。

## 5. 【V-216-2】FUN_00419B30 callers 28 件の分類 — **[確定]**

### 5.1 本体の精査追補（`v221_26_out.txt`）— arg 契約とタイブレークの訂正

- **arg 契約（確定）**: 第2引数 = **最低 MagicRange 閾値**。ループは `[elem+0x10]`（range）が **arg より大きい**要素を採用、同 range はタイブレークで採用。arg=0（23件）= 全 range、**arg=2（5件: 0x614376/0x6143C2/0x61DEC2/0x61DF28/0x62B0EE）= range≥2（Aimed）限定**。初回 best は arg、未採用なら 0 を返す（`[this+0x10]==0 && [this+0x14]==0` でも 0）。
- **タイブレークの訂正（v216 §4.1 訂正候補）**: 同 range 時の比較値は **`ftol(FUN_00413890(elem, 0))`＝スケール済み magnitude（丸め付き・決定的）**。`0x9828C0` は **ftol** であり rand() ではない → 「同値は rand() でランダム」は**誤り**。`FUN_00413890` は [elem+0x18] 有無で `[effect+0x64]` を経由して actor スキル等でスケール（`0x41B770`/`0x548C00`）して float 返し（`v221_27_out.txt`）。
- 併せて v216 §4.2 の callers 記述訂正候補: 返り値の `word [+0x20]`（0xFFFF 比較つき）は **TESFullName::nameLength**（`[+0x1C]`=name ptr の対。「school word」ではなく**名称文字列の長さセマンティクス**）。

### 5.2 callers 28 件の分類表（`v221_25_out.txt`）

| 分類 | 件数 | sites（関数） |
|---|---|---|
| **名称/表示**（`[ret+0x20]` word 0xFFFF → `[ret+0x1C]` name） | **9** | 0x419CF5(`FUN_00419CF0`) / 0x419D96(`0x419D90`) / 0x419E57(`0x419E50`) / 0x419F5E(`0x419F10`) / 0x41A64F(`0x41A610`) / 0x41ABCF(`0x41AB90`) / 0x5EDD18(`0x5EDCE0`) / 0x602CAC(`0x602A70`) / 0x699E4D(`0x699DA0`) |
| **適用/アクティブエフェクト構築/検証ゲート**（`0x4152C0` 適用ヘルパ連携・`[ret+0x78]`→instance+0x34 複写・`[ret+0x80]!=0` ゲート） | **10** | 0x69069C(`0x6904B0`) / 0x690A3E(`0x690A00`) / 0x69144F(`0x691030`) / 0x69A103(`0x69A060`) / 0x69B61E(`0x69AF30`) / 0x6A055F(`0x6A0540`) / 0x6A085B(`0x6A0800`) / 0x69F212(`0x69F1E0`・`[esi+0x74]` へキャッシュ) / 0x5ED675(`0x5ED5A0`・[+0x80] ゲート) / 0x5F40B8(`0x5F3E00`・[+0x80] ゲート) |
| **float 読取**（`[ret+0x74]`・fallback 1.0f＝倍率/コスト系と推定） | **3** | 0x61DEC2 / 0x61DF28（`FUN_0061DDF0`） / 0x62B0EE(`0x62B070`) ※すべて arg=2 |
| **効果記述子生成**（`call 0x416410` →2 文字列出力） | **2** | 0x614376 / 0x6143C2（`FUN_006142D0`・arg=2） |
| **VFX/関連オブジェクト**（`[ret+0x7C]` ptr → new 0x4C / vt0x420 適用） | **2** | 0x60CBAB(`0x60C940`) / 0x6576A1(`0x6575B0`) |
| **フォーム解決**（`[ret+0x64]` → `0x41B820`） | 1 | 0x69BF7D(`0x69BEC0`) |
| **モデル取得**（`[ret+0x18]` TESModel sub vt[5] → +0x2C） | 1 | 0x69E4B7(`0x69E410`) |

### 5.3 「代表 effect 決定はどれに必須か」の回答

- **三方面すべてで使用**される汎用セレクタ: 名称表示 9・適用/構築/ゲート 10・float 倍率 3・記述子 2・VFX 2・フォーム解決 1・モデル 1（計 28）。マジカコストの直接計算（SPIT cost）には使われないが、`[ret+0x74]` float（fallback 1.0）経由の倍率取得と効果記述子生成に必須。代表選択なしでは名称・適用・VFX・記述子が全て成立しない。
- `this` は **MagicItem 埋め込み sub**（callers の一部は `lea ecx,[item+0x18]` で明示、他は `vt[0x30](0)` 経由）。list は sub+0x0C ヘッド（node: +4=elem / +8=next、`add -4` 歩行）。SEFF 解決（`FUN_00412CB0`）・bit22 除外は v216 どおり。

### 5.4 GT ドラフト（反映は要承認）

- (K) GT「`FUN_00419B30`」行 補強: **arg=最低 range 閾値（0=全許容 / 2=Aimed 限定・5件）**。同 range タイブレーク=**ftol(スケール済み magnitude `FUN_00413890`)**（rand ではない＝v216 訂正）。callers 28 件＝名称9/適用10/float倍率3/記述子2/VFX2/フォーム解決1/モデル1。返り値フィールド: name=+0x1C・nameLength(word)=+0x20・formID系=+0x64・float倍率=+0x74・dword=+0x78・VFX ptr=+0x7C・ゲート dword=+0x80。

## 6. 【V-216-5】kMgefFlag_UnknownO(bit23) の EXE 側 reader — **[確定]**

### 6.1 新規検出した bit23 パス（`shr reg,0x17` 全走査・`v221_20/21_out.txt`）

| 帯 | 所在 | 内容 |
|---|---|---|
| **登録時** | `FUN_00417220` @0x41729E | ★ bit23 立ち → **詳細構築スキップ**（'SEFF'・bits4-6(0x70) と並列）。arg6=flags を新規オブジェクト（class formType 0x0C・size 0xA8）の `[+0x58]` へ格納 |
| **汎用述語** | `FUN_00416370` @0x4163D6-0x4163DE | `[esi+0x58]` flags に対し**任意マスク（arg 経由）**で「全ビット一致」検査した後、**bit23 立ちは即不合格**（meta 効果はフラグ問い合わせから除外）。併せて `[edi+0x64]` vs `[esi+0x64]`（=6 含む）比較 |
| **適用帯（9件）** | 0x5ED8E0(`FUN_005ED860`)/0x601F9D(`0x601B80`)/0x62C372(`0x62BA20`)/0x62D49D(`0x62CA80`)/0x630ECA(`0x630D40`)/0x63A74B(`0x63A020`)/0x63BBB0(`0x63BA30`)/0x676021(`0x675FC0`) | 共通形 `[obj+8]→[+0x1C]`（または3重参照）の flags dword bit23 → **立ちなら適用スキップ**（magic 適用/キャンセル決定帯） |
| **伝播書手** | `FUN_0060F250` @0x60F9AD/0x60F9B5 | ★ `[ebx+0x1C]` bit23 → **`or [ebp+0x1C],0x800000`** / 否则 `and [ebp+0x1C],0xFF7FFFFF` — **bit23 は実行時オブジェクト間で複写される**（ESM 静的フラグだけでなくインスタンス伝播あり） |

- 除外候補（非 mgef）: `FUN_004BFAA0`（float 仮数部 bit23）、`FUN_0040D4D0`/`0x451A60` 等の `push 0x800000`（色/マスク定数）、0x8Exxx UI 帯。

### 6.2 POSN/DISE の実行時登録＝CEF コメントの逐語実証（`v221_23/24_out.txt`）

- **`FUN_00417420` = 組み込み EffectSetting 登録群**: `FUN_00417220(code, name, range?, …, **mgefFlags(arg6)**, …, 関連 fourcc…)` を連続呼び出し。エラー文字列は **"Registered EffectSetting contains multiple ASSOCIATED flags!"**(0xA330D4) / **"Registered EffectSetting does not allow any range!"**(0xA33114)。
- **POSN: `push 0x800000`**（name="Poison Info"）/ **DISE: `push 0x800000`**（name="Disease Info"）→ **「POSN,DISE have *only* this bit set」の CEF コメントを EXE 側で逐語実証** [確定]。
- 登録列（抜粋）: Water Breathing(0x52424157)/Water Walking(0x41574157)/Shield(0x444C4853)/Fire Shield("FISH")/Lightning Shield("LISH")/Frost Shield("FRSH")(flags 0x100007A)/Burden("BRDN",関連 FTHR+DSPL)/Feather("FTHR",関連 DSPL)/DUMY(関連 REHE+FOHE+ABHE+DSPL)/INVI/CHML/PARA/SLNC/CHRM/DEMO/RALY/CALM… — **ESM 未定義/特殊効果のハードコード登録**。
- 0x417220 の後続: bits20/19/16/17/18 が1つでも立つと「multiple ASSOCIATED flags」エラー経路 → **bit23 と ASSOCIATED 系ビット群の排他チェック**。

### 6.3 GT ドラフト（反映は要承認）

- (L) GT「kMgefFlag_UnknownO(bit23)」行: **reader は多経路**: 登録時 `FUN_00417220`(0x41729E)・汎用述語 `FUN_00416370`(0x4163D6)・適用帯9件（0x5ED860/0x601B80/0x62BA20/0x62CA80/0x630D40/0x63A020/0x63BA30/0x675FC0）・**伝播書手 `FUN_0060F250`(0x60F9AD or/and 対)**。**POSN/DISE は `FUN_00417420` で mgefFlags=0x800000 のみで実行時登録**（CEF「meta effects」コメントの EXE 実証）。`FUN_00417220`＝組み込み EffectSetting 登録ヘルパ。

## 7. 新規残点（次タスク候補）

| ID | 内容 | 優先 |
|---|---|---|
| V-221-1 | `FUN_004DE980` 全体（enum 0..11）の正式機能名と各値の意味論（特に formType 0x23 ケースのプレイヤ状態連鎖 0x5E0xxx→0xA/0xB/7/9/2/1、`[this+8]` bit13 早期退出、`[esi+0x1C]+0x64`/`+0x7C` ケース）。`FUN_005A4980`/`FUN_005A8C90`（メニュ帯 0x5A4xxx/0x5A8xxx）の所属メニュ同定 | 中 |
| V-221-2 | `FUN_006505D0` のグローバル byte `0xB3BA80`（bit30 必須/ bit31 必須のモード切替主体）と `0xB3BA9C` コレクション（contains=0x446C30 / add=0x446CB0）の実体名 | 中 |
| V-221-3 | `FUN_00678750` の全体機能名（候補エフェクト走査＋`0x566DC0`/`0x5E0F30`/`0x5E6FA0` ゲート＋`[esp+0x12]` feet/level マーカの消費者） | 低 |
| V-221-4 | formType 0x0C クラス（size 0xA8、TESModel/Description/FullName/Icon 継承、DATA 0x40B @+0x58、**'ESCE'** チャンクで [+0x6C] 個の dword 配列 @+0x9C、レコードロード経路 0x44E3B1 ＋ `FUN_00417220` 実行時生成経路の二経路）の正式クラス名と record fourcc | 中 |
| V-221-5 | ENIT フィールド意味論: ENCH 16B @+0x34{u32,u32,u32,u8}（既定 2/-1/-1/0）、ALCH/INGR 8B @+0x78{u32,u32(byte比較)} | 中 |
| V-221-6 | EffectSetting 未命名フィールド: +0x64（formID 系・`0x41B820` 解決／`FUN_00413890` の actor スケール経路でも使用）、+0x74 float（fallback 1.0・倍率/コスト系）、+0x78 dword（アクティブエフェクト instance+0x34 へ複写）、+0x7C ptr（VFX/関連・new 0x4C）、+0x80 dword（2 callers が非ゼロ必須ゲート） | 中 |
| V-221-7 | `FUN_00417420` 登録列の完全枚舉と追加引数の意味（arg7=0x43/0x3F/0x40/0x39/0x37…、arg9-12=関連 fourcc 'DSPL'/'REHE'/'FOHE'/'ABHE'/'CUPA'/'CUPO'(0x4F505543)/'CUDI'(0x49445543)、code 0x414D5453↔"Stunted Magicka"）。'DSPL' 等の fourcc 意味 | 低 |
| V-221-8 | 適用帯 bit23 リーダーの `[obj+8]→[+0x1C]` ホルダ（flags dword 保持オブジェクト）のクラス同定（0x63BA30 帯等） | 低 |

## 8. 本タスクでの訂正まとめ

| 対象 | 訂正内容 |
|---|---|
| v177「`0x450C20`＝4-byte formID reader」 | **誤り（呼称訂正を確定に昇格）** → `TESFile::Read(dest, size)`＝汎用 chunk ブロック reader（size=バッファ容量上限・超過時は size-1+NUL 切り詰め＋警告で true） |
| v193b H.2「bit31=負センチネル」 | **棄却を確定**（callers 5 件すべて MGEF kMgefFlag_MagnitudeIsFeet 正規用途） |
| v216 §4.1「同値は rand()（0x9828C0）でランダム」 | **誤り** → 0x9828C0=ftol、タイブレークは `FUN_00413890` のスケール済み magnitude（決定的） |
| v216 §4.2 callers「[+0x20]（school word）」 | **訂正候補** → TESFullName::nameLength（word・0xFFFF=lazy sentinel）。[+0x1C]=name ptr |
| v216 §6 V-216-1「{0,3,5}=表示単位 enum」 | **確定**（ただし strings 挿入本体は `FUN_00413BE0` が mgefFlags 直読の別経路。{0,3,5} は種別コードとして使用） |
| v216 §3.1 の mgefFlags +0x58 | 追加整備: formType 0x20=EffectSetting（ctor `FUN_004AE660`・`[esi+0x58]=0` 初期化・TESModel/TESDescription/TESFullName/TESIcon 継承）。TESFullName は +0x1C{name,+0x20 len}（vtable 無し） |

## 9. 自己検証・証跡

- **selftest**: 着手時・完成直前とも `probe_harness.py selftest` → **4/4 PASS**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl=0x504560）。
- **内部整合（一致 12 項目）**:
  1. formType 0x20 ctor `FUN_004AE660` の `[esi+0x58]=0` 初期化＝mgefFlags、base ctor `FUN_004B40A0`(formType 0x12=TESForm) 呼び出しと FUN_004DE980 の case 0x12/0x20 が整合。
  2. GMST `sMagicEffectItem*` のグローバル配置 stride 8（0xB33460..0xB334E8）と initializer スタブ（0x9D9D60..0x9D9F7F、`push 値; push 名; mov ecx; call 0x419A70`）が 18 件過不足なく一致。
  3. `FUN_00413BE0` の bit30→UpToLevel("up to level")/bit31→Feet("ft")/既定→pt(s)/bit3→"%" 分岐と文字列テーブルが整合。
  4. ENIT fourcc 0x54494E45/SPIT fourcc 0x54495053 のバイト列が "ENIT"/"SPIT" と一致（v216 の SPIT 実測を継承）。
  5. ENCH(6)/ALCH(7)/INGR(8) の定数スタブ GetMagicType が CEF Magic::MagicTypes と一致。
  6. ENCH/ALCH/INGR の compare/copy フィールド位置（+0x38/+0x3C、+0x78/+0x7C）が ExtraDataPtr の着地（obj+0x34、obj+0x78）と一致。
  7. `FUN_00419B30` 全文（0x419B30..0x419C46、ret 4）の arg 参照（[esp+0x18]）と「range>arg 採用・同値 ftol タイブレーク」が callers の arg=0/2 分布（23/5）と両立。
  8. POSN/DISE 登録の `push 0x800000`（arg6=flags）が FUN_00417220 の `[esi+0x58]` 格納・bit23 スキップと整合。
  9. 適用帯 bit23 リーダー8件が全て `[x+0x1C]` フラグ dword の `shr 0x17` 形で、float 仮数部等の無関係な `shr 0x17`（FUN_004BFAA0 等）を除外して分類。
  10. `FUN_0060F250` の or/and 0x800000 対が bit23 伝播の双対（立→or / 不立→and ~mask）で矛盾なし。
  11. `TESFile::Read` の early-out（残余0→true）・fast path（残余>size→size-1+NUL+警告）・alt path（残余<=size→残余読出）の3経路が callers の用途（EDID 0x200 文字列・SPIT 0x10・formID 4）と整合。
  12. 制約遵守: 成果物は本ファイルのみ（新規1件）。docs/・GT・TODO・session-memory・plan・cache・ソース・既存 tmp/re は編集 0 件。ESM 本体未アクセス。web 検索なし（CEF 記述は v216 引用踏襲）。
- **証跡（すべて `%TEMP%\opencode/`・本タスク生成）**: `v221_1.py`（AE590/AE5A0 callers＋FUN_004DE980 全文）/ `v221_2.py`（DE980 ジャンプテーブル）/ `v221_3.py`（level/feet 文字列走査）/ `v221_4.py`（caller 窓＋文字列コンテキスト）/ `v221_5.py`（FUN_004DBAE0 全文＋formType ctor 走査）/ `v221_6.py`（**sMagicEffectItem* テーブル実読み＝核心証拠**）/ `v221_7.py`（0x9D9Dxx initializer 実測）/ `v221_8.py`（GMST グローバル参照走査）/ `v221_9.py`（**FUN_00413BE0 全文**）/ `v221_10.py`（AE5A0 callers 5 件文脈）/ `v221_11.py`（0x4AE660 確認）/ `v221_12/13.py`（**0x450C20 全文＋alt path＋エラー文字列**）/ `v221_14..17.py`（**ENCH/ALCH/INGR ctor→vtable→LoadForm→ExtraData 実測**）/ `v221_18.py`（ctor callers）/ `v221_19.py`（record dispatch＋FUN_00417220）/ `v221_20/21.py`（**bit23 全走査**）/ `v221_22/23/24.py`（**FUN_00417420 登録列＋名前文字列**）/ `v221_25.py`（**FUN_00419B30 callers 28 件窓**）/ `v221_26/27.py`（FUN_00419B30/0x413890 全文）＋対応 `v221_*_out.txt`。
