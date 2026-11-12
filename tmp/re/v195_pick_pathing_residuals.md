# v195_pick_pathing_residuals.md — pick/pathing 統合束（LOS/ViewCaster 種別書手・W/O クラス同定・速度配線）

- 作成日: 2026-10-29 / 担当: TODO serial 182 (V182) の分析専任サブエージェント
- 編集範囲: **本新規 1 ファイルのみ**。docs/ / 既存 tmp/re / TODO_re49.md / GT / 台帳 / git / Supermemory / sync は一切変更していない（読取のみ）。ESM 読取 0 回。Ghidra 呼出なし（capstone 静的解析）。
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe` (v1.2.0416 Steam, ImageBase 0x400000, 7,898,624 B)
- 出典（読取専用）: `tmp/re/v192_havok_magic_residuals.md` §2-R1/R2/R7/R8 + §5-1/2/4/5 + §8-2/§8-3 / `tmp/re/v186_havok_ray_pathing.md` §1.5/§1.6 / `b13_nonrtti_vtable_classes.md` / `b26_nontt_class_names.md` / `cache/*` / `docs/Oblivion_decompiled.c`
- 中間スクリプト: `%TEMP%\opencode\v195_1..N_*.py`（再利用含む: v192b_*, v192_b*）
- 凡例: [確定] = 逆アセンブル/データ直接確認 / [部分] = 構造判明・一部未 / [推定] = 論理推定 / [残] = 未解明。

---

## 1. 手法 / 入力

- `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl=0x504560）。
- 解析: `%TEMP%\opencode` の再利用スクリプト（v192 系）＋新規 `v195_*.py`（capstone 5.0.7 detail 付き、VA→off = `pe.get_offset_from_rva(VA-0x400000)`）。
- 参照数は `tmp/re/cache/call_targets.json`、ラベル境界は `cache/named_funcs.txt`、クラス情報は `cache/vtable_index.tsv` を引く（再走査しない）。decompiled は関数ヘッダ部分読取のみ（GT §2 規則）。

---

## 2. スコープ (1): LOS(0x1A)/ViewCaster(0x1D) request 種別書手 — [確定]

### 2.1 即値不使用の正体 = 「レジスタ合成 + スタック request への disp 相対 store」

v192 R1 で「disp 0x24 への 0x1A/0x1D 即値 store = 0 件」とされた理由が判明した。
種別は **(既有ハンドル値 & 0xFFFF001A) | 0x1A** という **レジスタ合成値**で、
request オブジェクト自体が**呼び出し元スタック上**にあるため、store 命令は
`mov [esp+disp], r32`（=スタックベース判定）になる。実際、種別 store は以下 **4 箇所に集まる**（全 .text の `push imm8 0x1A/0x1D` + `or r32,imm8 0x1A/0x1D` + `and r32,0xFFFF001A` の総当たり走査 = v195_8_out.txt、排他性を確認）。

| # | 型 | 関所 | 命令列（実測） | request 位置 |
|---|---|---|---|---|
| A1 | 0x1A | **FUN_004F9120** @0x4F92A0..A8 | `and eax,0xffff001a; or eax,0x1a; mov [esp+0x78],eax` → 直後に vfunc 0x15C 呼出し | R = [esp+0x54]（0x4F9210 `lea ecx,[esp+0x54]; call 0x4F5EA0`）→ 種別 = **R+0x24**（0x54+0x24=0x78） |
| A2 | 0x1A | **FUN_005F2820** @0x5F2B48..51 | `and edx,0xffff001a; or edx,0x1a; mov [esp+0xe8],edx` → `call eax`(vfunc 0x15C) | R = [esp+0xC4]（0x5F2AB2 `lea ecx,[esp+0xc4]; call 0x4F5EA0`）→ 種別 = **R+0x24**（0xC4+0x24=0xE8） |
| B1 | 0x1A | FUN_004F9120 @0x4F922C / FUN_005F2820 @0x5F2ADB | `push <owner>; push 0x1a; lea ecx,<stack>; call 0x535A00`（collector ctor の第1引数） | stack collector |
| C1 | 0x1D | **FUN_00538D10** @0x538D7B..93 | `mov eax,ecx; shl eax,0x10; or eax,0x1d; mov [esp+0x14],eax`（ecx = 引数 [ebp+0xC]） | スタック・ハンドル語（下位16bit=種別 0x1D、上位16bit=引数） |

- decompiled 突合（v195_5/v195_13）: FUN_005F2820 は `L358286 FUN_00535a00(0x1a,piVar7)` → `L358297 uStack_dc = (uint)uStack_17c & 0xffff001a | 0x1a` → `L358298 (**(*local_198 + 0x15c))(afStack_16c)` の順で、**種別合成 → world vfunc 0x15C へ request 送信**であることを指令レベルと一致させて確認 [確定]。
- **request 構築ブロック（FUN_005F2820 内、v192 R1 の「次の手」）** [確定]:
  1. 0x5F2AB2 `lea ecx,[esp+0xc4]; call 0x4F5EA0` — request R を初期化（0x4F5EA0 本文 = `+0x20=0, +0x24=0(種別クリア), +0x44=1.0, +0x50/+0x70/+0x74/+0x78=0, +0x60=const 16B`、v195_7 実測）。
  2. 0x5F2ABE..D0: `vfunc 0x188`（owner 検査）→ 戻り偽なら [esp+0x1C]（=piVar7 の解決先）に差し替え。
  3. 0x5F2ADA `push ebx; push 0x1a; call 0x535A00` — collector を R の近傍スタックに構築（下記 2.2）。
  4. 0x5F2B27 `call 0x4F8840`（NiPoint3 を 0xA39088 倍 = 単位変換）、0x5F2B33 `call 0x65ABE0`（**FormID ハンドル解決**、v195_7 本文実測）、0x5F2B48..51 種別合成 → R+0x24 へ。
  5. 0x5F2B5D `call eax` = owner の **vfunc 0x15C**（以降 0x158 と対で z 値取得 → 距離計算 → 0x8B8800 で AABB 展開 → 0x6ECC80/0x4440C0 で描画/判定）。
- **発射端の request 種別消費**: 種別は合成直後に vfunc 0x15C の**スタック引数構造体の +0x24 フィールド**として渡される（A1/A2 の `mov [esp+disp],r32` が直後の vfunc 呼出しの引数範囲内）。hud/activate 帯に構築は無い（v192 R3 の否定と矛盾なし）。

### 2.2 collector ctor FUN_00535A00（型 0x1A 受け）— [確定]

v195_6 全文実測（size 0xA0、`ret 8`、callers = 2: 0x4F9235 / 0x5F2AE4 の 2 site のみ）:

```
[esi+0x24] = 1.0 (fld1 ×2) / [esi+0x30] = 0 / [esi+4] = 1.0
[esi]      = 0xA562C8   ; vtable（cache vtable_index row 1233 = `.?AVSpecificItemCollector@@`、2 slots — RTTI 突合は §3 で実測）
[esi+0x40] = <arg2 の解決先（0x65ABE0 経由）>
bucket 登録: ecx = <arg2 の FormID 或いは種別> & 0x3F
             eax = ([esp+0x1C] >= 0x18) ? [esp+0x1C] : 0x1C
             [0xBA7DB0 + eax*4] = (1 << ecx) | 0xA277F
```

- 0xBA7DB0 の abs32 参照は FUN_00535A00 の 1 件＋**コマンド/オペコード表レジストリ 0x8A7F20/0x8A7F70/0x8A83C0 群**のみ（v195_7）→ 0xBA7DB0[bucket] は「種別 bit マップ」型のグローバル表。
- ViewCaster 側で対応物として使う FUN_00538D10（size 0x1B0、callers = 1: 0x5808D7 in FUN_005806D0、FUN_005806D0 callers = 0x5D6A40/0x671778、v195_10/16）は 0x401F00(+0x14 size?) → FUN_00532090 → FUN_00532250（0x30 バイト級構造初期化: +0x14=0x80000000, +8=2 等）でスタック request を組み、種別語（(arg<<16)|0x1D）を [esp+0x14] へ、解決ハンドルを [esp+0x18] へ入れる [確定]。
- 種別 id の出所ヘルパ: **FUN_0047D5B0**（文字列→id 0..0x23、0x1A/0x1D を含む switch、callers 0x4F3EE9/0x4FE988）と **FUN_004979E0**（0..0x11 → 0x14..0x1E/0x6F..0x71、callers 0x497D8F/0x4981D4/0x761EDD/0x77546D）（v195_10/11）。即値 store が無い理由は「種別がこれらのヘルパ経由の引数値として流れ、最後に or 合成される」設計であることを示す [確定]。

### 2.3 FUN_004F5EA0 の全 caller と型対応（種別書手の網羅性）— [確定]

callers = **6 件のみ**（v195_12）: 0x4F9210(=0x1A) / 0x539534(FUN_00538EC0、ViewCaster ループ) / 0x5F2AB9(=0x1A) / 0x66D368(=**0x19**) / 0x68168B(=**0x15**) / 0x68662B(=**0x1B**)。
→ **request 種別書手は本 EXE に 6 経路しか存在せず、0x1A 系 = FUN_004F9120 と FUN_005F2820、0x1D 系 = FUN_00538D10（＋ FUN_00538EC0 が再利用）に確定**した。0x19/0x15/0x1B 系の合成形も同一パターン（`shl 0x10; or type`、例 0x66D379-80）で統一 [確定]。

## 3. スコープ (2): castRay 受け手 W/O クラス同定 — [確定(経路・構造) / 部分(クラス名)]

### 3.1 W = [bhkWorld+8] の実体 = ahkWorld [確定]

ctor 経路を全追跡（v195_21/v195_25/v195c_1）:

1. **bhkWorld ctor 0x88AEB0** → 0x88AFDE `call 0x889A70`（bhkWorld vtable slot28 (+0x70)。arg = cinfo の [+0xa0]）。
2. **FUN_00889A70**: `[0xba7d98]` アロケータ（vfunc+0x10, type=0x2c, size=**0x2C0**）で確保 → esi = 新オブジェクト → 0x889AE5 `call 0x89A230`（cinfo を渡す）＝**W の構築本体**。
3. **FUN_0089A230 先頭**: 0x89A247 `mov [esi], 0xA96D6C`（b13 #130 非RTTI vtable・2 slots・近傍 `bhkWindListener@0xA96A40`/`bhkRefObject@0xA96EB4`）＝**base-phase vtable**。
4. 戻り後 0x889AED: `mov [esi], 0xA95C54` に**上書き** ＝ **`.?AVahkWorld@@`（vtable_index row 1565、2 slots、0xA95C54）**＝**own-phase = 完全クラス ahkWorld**。b26 の「非RTTI base → RTTI own」phase パターンと同一構造。
5. 0x889B25-0x889B2D: `mov edx,[edi]; mov eax,[edx+0x4c]; push esi; mov ecx,edi; call eax` — bhkWorld の vfunc **+0x4C（slot19 = 0x889A60 = `jmp 0x89D400`）** を呼ぶ。
6. **FUN_0089D400**（size 0x30）: `cmp [esi+8],edi; je skip` → `[vfunc+0x50](0)` → **`mov [esi+8], edi`**（0x89D41F）→ `[vfunc+0x50](1)` ＝ **`[bhkWorld+8] ← W(ahkWorld)` 格納書手 [確定]**。
7. deleting dtor slot0 = 0x889590（`[esi]=0xA95C54; call 0x89AD80` → size ワード [esi+4] で free）。**FUN_0089AD80 = complete dtor**（0x89AD8B で base 0xA96D6C へ復元 → サブオブジェクト群解放 → 0x89AF0F `mov [esi+0x64], 0`＝**dtor でのクリア**）。

→ **W = ahkWorld（RTTI 名あり）**。v192 §2-R2 の「O および W は non-RTTI 系候補」は W 側について**訂正**: W は RTTI `.?AVahkWorld@@` を持つ（0xA96D6C は base-phase のみ）。

### 3.2 W+0x64 への実注入点 = FUN_0089A230@0x89A6E4 [確定]

- 0x89A6B5-0x89A6E4（v195_25 実測）: cinfo(edi) の +0x30/+0x40/+0x64 から 3 ワード群を esi+0x280/+0x290/+0x2a0 へコピー → `push [edi+0x64]; push esi+0x290; push esi+0x280; call [0xB2FC80]`。
- **[0xB2FC80] = 0x8E5020**（v195_27）: `[0xba7d98]` アロケータ（type 0x1e, size **0x80**）で確保 → `[eax+4]=0x80`（Havok size ワード）→ **call 0x8E4730（O の ctor）** → 戻りを返す。0xB2FC80 の参照は **0x89A6E0 の 1 件のみ** → O の生成はここ一箇所。
- 0x89A6E4 `mov [esi+0x64], eax` ＝ **W+0x64 ← O 注入 [確定]**。
- v192 §2-R2 の候補「FUN_0089AD80@0x89AF0F」は **dtor クリアであり実注入ではない**（3.1-7）— v192 候補の再解釈で確定。

### 3.3 O = non-RTTI vtable 0xA9A710・ctor FUN_008E4730 = hkpBroadPhase 系 [確定(構造) / 推定(名)]

- **vtable 0xA9A710 の imm32 参照は ctor 0x8E4747 と dtor 系 0x8E5065（FUN_008E5050）の 2 件のみ**（v195c_2）→ O の ctor は FUN_008E4730 に一意。b13 #29（**18 slots**、近傍 RTTI `bhkCharControllerShape@0xA99F64`/`hkPointToPathConstraintCinfo@0xA9A8C8`、Havok 帯・「不明」）に突合一致。
- **vtable slot14 (+0x38) = 0x8E24A0**（v195c_1 (C)）＝ v192 R2 の発射端 `call dword [edx+0x38]`（receiver = [W+0x64]）の**スロット番号と完全一致** → **O = vtable 0xA9A710 クラス [確定]**。
- **FUN_008E4730 本体**（v195_28）: `[esi]=0xA9A710`、`[esi+6]=1`（refcount）、+0x40/+0x48/+0x4C/+0x58/+0x64 に {0,0,0x80000000}（hkArray 空・capacity フラグ）、+0x10/+0x20 = 0・+0x30 = 1.0 ベクトル（transform: scale/offset）、`mov ecx,0x17`（**23 エントリ**）のループで 10.0/11.0 定数＋min/max/addps → **AABB 境界テーブル生成**。
- **castRay 実装 0x8E24A0**（v195c_2）: `[ebp+8]` の request（16B ray ベクトル）→ `addps xmm5,[edi+0x10]`（translate）→ `mulps xmm5,[edi+0x30]`（scale）→ min/max/addps → **ワールド座標→quantized ボクセル座標**（4 ワード `shr 7`）→ `[edi+0x40]` ハッシュテーブル（`[edi+0x44]` count、+[edi+0x20] アロケータ）探索 ＝ **broadphase 量子化レイアウト** [確定(構造)]。
- → **O = hkpBroadPhase 系**（Havok の AABB ボクセルハッシュ broadphase。M-BP-I/Mbpi 系が最有力）[推定・確度中〜高]。b13 #29「不明」への回答。

### 3.4 付随: W+0x78 = collectionFilter 系 [推定] + bhkWorld+0x64 混同禁止の再掲

- 0x89A835-0x89A856（v195c_1 (B)）: vtable 0xA96B64/0xA96B70/0xA96B68 を +8/+0xC/+0x10/+0x14 に書いたオブジェクトを `[esi+0x78]` へ（refcount inc 付き）。vtable 帯に **'collectionFilter' 文字列 0xA96B76** が隣接 → **W+0x78 = collectionFilter 系フィルタ** [推定: 中]。v192 R4 の PickObject が渡す arg3（[this+0x78]）＝フィルタと整合。
- **bhkWorld 自身の +0x64 = type 0x14 カウンタ**（0x88AF04 ゼロ化、0x88FE70/0x88FEB0 thunk）は、Gamebryo ラッパ bhkWorld（RTTI 0xA95CF4）のフィールド。W は **[bhkWorld+8] 経由で間接保持される別オブジェクト**（ahkWorld、0x2C0 バイト）であり、**W+0x64 = O ポインタ**。同オフセット・別クラス・別意味で **v192 §2-R2「重要注意」と矛盾しない** [確定]。
- なお b26 #2 の「FUN_0089a230 = hkWorld.cpp 群・名前候補 hkSimulationIsland 系」は、本タスクで **FUN_0089A230 = ahkWorld（物理ワールド全体）の ctor** と確定したため、#2 のブロック（0xA9A588、+0x2C）はその内部サブオブジェクト（シミュレーション系）という整理になる。

## 4. スコープ (3): PathingSystem 速度 → Actor slot[218] 配線 — [確定(不在)]

### 4.1 前提再確認: PathingSystem[+0x3C..] に速度 store は無い（V183 訂正の維持）

v196 §3.4 の全数裁定どおり、PathingSystem[+0x3C/+0x40/+0x44] の書手は **Init ゼロ化（0x684F1F-2D）・ノード位置コピー（0x688569-75）・別クラス 5 件（0x682FE5/0x68CDDD/0x68CE34/0x68CF75/0x68D36C）** のみ。**速度を書く store は 0 件**。よって「速度出力 → slot[218]」の**配線の起点が存在しない**。

### 4.2 slot[218] 実際の入力 = グローバル [0xB3BCF0] [確定]

- vtable_index 実測（v195_20）: slot218 = Actor 族（Actor/Character/Creature/PlayerCharacter, vt=0xA6E074 等、n=239）= **0x5FAAE0**、HighProcess = **0x631D70**、LowProcess = 0x6FE080（`xor eax,eax; ret` スタブ）。
- **slot218 呼出しの全コンテキスト**（v195_20）: 0x605B73/0x65F8C4/0x673478/0x6787EF の 4 件はいずれも
  `mov ecx, 0xB3BD00; call 0x673B00`（**= `fld [0xB3BCF0]`**、v195_21/23 本文実測）→ `fstp [esp]` → `call [vtable+0x368]`。
  **0x673B00 の callers = 9 件全員**（0x6057A6/0x605B6E/0x65F7A4/0x65F8BF/0x66F892/0x66F9A0/0x673473/0x69F069/0x69F3C9、v195_21）が同一パターン → **slot218 の引数は一貫してグローバル [0xB3BCF0]**。
- **0xB3BCF0 の性質**（v195_23）: getter **FUN_00673B00**（`fld [0xB3BCF0]; ret`）／setter **FUN_00673B10**（[esp+4] を書く → **[0xA3F3D8]=100000.0 超なら 0 へ** → 0x984012/0x983ffd（isnan 系）で NaN/Inf なら **0**）＝**異常値ガード付き float**。setter callers 5 件: 0x65F7BE（fSecs 系計算後）/0x66F8AC/0x66F9BA/0x677133（`push 0` = クリア）/0x678556（加算）。0x677060/0x6770F0 は 0xB3BCF0 を **0xB33B00（設定管理）へ登録**（`push 4; push 0xB3BCF0; call 0x45b9a0`）＝**INI/設定バックドの float**。→ **[0xB3BCF0] = 時間系グローバル（fSecsPerFrame/fGlobalTime 系）** [確定(構造)]。

### 4.3 0x5FAAE0 本体 = 「時間差分処理 → [actor+0xBC] 更新」vfunc [確定]

v195c_1 (E) で size 0x3C0 全体を実測:

1. 引数 float（[esp+0x10]）と **[esi+0xBC]** の差分を生成（0x5FAAEC `fsub [esi+0xBC]`）、0xA3744C(=0.3) などでクランプ。
2. 0x5FAB4A-5C: 差分を **vfunc +0x2E4** へ伝搬。以降 0x198/0x330/0x33C/0x288/0x284/0x2C0/0x2D0/0x2D8/0x468 群＋アトリビュート 0xB/0xD/9/0x1C の重み計算（0x547ED0/0x547F40/0x548D60/0x5F1910）、0x5E07D0（推進系）、0x5F2720/0x5E7A60 呼出し。
3. **終端 0x5FAE7F `fstp [esi+0xBC]`** ＝引数（グローバル時間）を [esi+0xBC] へ格納して終了。
- → **[esi+0xBC] は「前回受け取った時間値のバッファ」**であり、slot218 は「毎フレームの時間入力を差分処理して actor へ反映する vfunc」。**速度ベクトル store ではない**。v192 R8「[+0xBC] が速度上限系」は**本実測で裏付けなくなり要訂正**（差分バッファ）。
- **HighProcess の slot218 = 0x631D70 は別物**（v195c_1 (D)）: 引数はオブジェクト（`mov edi,[esp+0x10]; test; call 0x5E6B40` 検証 → `[esi+0x220]` チェック → **al=bool, ret 4**）＝**述語**。スロット意味はクラス毎に異なる（Actor 族=時間更新 / HighProcess=述語）。

### 4.4 v192/v196 との突き合わせ

| 系統 | 主張 | 本タスク裁定 |
|---|---|---|
| v192 R7 残点「速度本体 = move オブジェクト 0xA747BC」 | [推定] | **反証維持**（v196 §3.5: f32 タイマー＋整数カウンタ＋NiPointer） |
| v192 R8「[+0xBC] が速度上限系・配線は残」 | [部分] | **配線 = グローバル [0xB3BCF0] → slot218**（本タスク）、[+0xBC] は時間差分バッファ |
| V183/v196「PathingSystem に速度 store 不存在」 | [確定] | **維持**。配線の起点自体が無い |
| s72 M7「PathingSystem[+0x3C..] = 速度出力」 | 誤 | **再訂正**: ノード位置コピーのみ |

**帰結**: 「PathingSystem 速度 → Actor slot[218]」という配線は**本 EXE に存在しない**（起点・終点とも速度では無かった）[確定]。

---

## 5. 結論（3行）

1. **LOS(0x1A)/ViewCaster(0x1D) の request 種別書手は本 EXE に 6 経路のみ**（0x1A 系 = FUN_004F9120/FUN_005F2820、0x1D 系 = FUN_00538D10、他 0x19/0x15/0x1B）。種別は「レジスタ合成 + スタック request への disp 相対 store」で、即値 store が無い理由は種別 id ヘルパ（FUN_0047D5B0/FUN_004979E0）経由設計である（§2）。
2. **castRay 受け手は 2 段間接**: bhkWorld（Gamebryo ラッパ、+8 で保持）→ **W = ahkWorld**（RTTI 0xA95C54、ctor FUN_0089A230、格納書手 FUN_0089D400）→ **O = [W+0x64]**（non-RTTI vtable 0xA9A710、ctor FUN_008E4730、注入点 0x89A6E4）＝ **hkpBroadPhase 系**（castRay 仮想 +0x38 = slot14 = 0x8E24A0、量子化ボクセルハッシュ）（§3）。
3. **「PathingSystem 速度 → Actor slot[218]」配線は不存在**: PathingSystem[+0x3C..] はノード位置のみ、slot218(0x5FAAE0) の実入力はグローバル時間 [0xB3BCF0]（setter 0x673B10 が NaN/巨大値ガード）で、[actor+0xBC] は時間差分バッファ。両端とも速度では無かった（§4）。

## 6. 残留不明点

1. O の**正確なクラス名**（hkpMbtiBroadPhase 等）は未特定 [残]。構造（AABB→quantize→ボクセルハッシュ＋castRay 仮想）は broadphase で確定。
2. [0xB3BCF0] の**INI 正式名**（fSecsPerFrame / fGlobalTime / fAltDelta 等）は未取得 [残]。設定登録は 0x45B9A0 経由（型 4 = float）。
3. FUN_008E4730 の **23 エントリ境界テーブル**（0x17 ループ）の意味づけは構造推定 [残]（M-BP-I の境界コードが最有力）。
4. W+0x78 フィルタの実クラス名（collectionFilter 文字列隣接まで）は [残]。
5. FUN_0068CD90/CEA0（vtable 0xA75044、+0x3C = NiPointer）のクラス名は v196 由来のまま [残]。

## 7. 自己検証

- **selftest 4/4 PASS**（着手時・完成直前の 2 回、同一結果: bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl=0x504560）。
- 内部整合（本タスク実測と既存記載の一致）:
  1. **slot14 (+0x38) = 0x8E24A0**（v195c_1 vtable 実測）が v192 R2 の発射端 `call [edx+0x38]` と**スロット一致**。
  2. vtable 0xA9A710 の imm32 参照 = **ctor 0x8E4747 / dtor 0x8E5065 の 2 件のみ** → O の ctor 同定の一意性。
  3. [0xB2FC80] 参照 = **0x89A6E0 の 1 件のみ** → O 注入点の一意性。
  4. **0x673B00 callers = 9 件**（E8 生走査）と slot218 呼出し 9 フレームパスの整合（4 件はコンテキスト全文一致）。
  5. v192 の「W+0x64 書手候補 FUN_0089AD80@0x89AF0F」を **dtor クリア**と再解釈（0x89AD80 が base vtable 0xA96D6C 復元 → 0x89AF0F で `[esi+0x64]=0`）＝候補の切り分けが確定。
  6. ahkWorld の「base 0xA96D6C → own 0xA95C54」vtable 書き換えが b26 phase パターン（base phase / own phase）と一致。
  7. bhkWorld+0x64（type0x14 カウンタ）と W+0x64（O ポインタ）が**別オブジェクト**（v192 §2-R2 重要注意と矛盾なし）。
- 中間スクリプト: `%TEMP%\opencode/v195c_1.py`・`v195c_2.py`（出力 v195c_1_out.txt / v195c_2_out.txt。v195c_2 は末尾 [0xBA7D98] 読みが .data 範囲外で例外終了するが、必要な 3 項目（0xA9A710 refs / slot14 本文 / 文字列アドレス）は取得済み）。
- 制約遵守: 編集は**本ファイルの追記のみ**。docs/ / TODO_re49.md / GT / 台帳 / git / Supermemory / sync は編集 0 件。既存 tmp/re 上書き 0 件。ESM 読取 0 回。Ghidra 呼出なし。

## 8. Android 移植への示唆（実装しない）

- **castRay は 2 段間接で移植する**: bhkWorld 相当の thin ラッパ（+8 でワールド保持、+0x64 は type0x14 カウンタとして分離）→ ahkWorld 相当（+0x64 = broadphase、+0x78 = collectionFilter）→ broadphase の castRay 仮想。**bhkWorld の +0x64 と ahkWorld の +0x64 を同一フィールドと誤認しないこと**（v192 の警告を継続）。
- **broadphase（O）は量子化ハッシュで再現**: transform（scale +0x30 / translate +0x10）→ `addps/mulps` → min/max clamp → 座標 `shr 7` でボクセル化 → ハッシュテーブル探索。23 エントリの境界テーブルは固定表として移植すれば足りる（動的生成は不要）。
- **速度フィールドを作らない**: PathingSystem は「追従ノード位置（+0x3C..+0x44）」のみを保持し、Actor への反映は毎フレームのグローバル時間 [0xB3BCF0]（NaN/Inf/上限 100000 ガード付き setter）を slot218 相当（時間差分処理 vfunc）へ渡す設計。**速度はコンシューマ側で位置差分から算出**する（v196 §8 と統一）。
- **slot218 はクラス毎に意味が異なる**: Actor 族 = 時間差分更新（[+0xBC] = 前回時間バッファ）、HighProcess = bool 述語（0x631D70）。マルチディスパッチ移植時はオーバーライド契約を分けること。
- **フィルタ（W+0x78）は vtable 4 本書き換え型**（+8/+0xC/+0x10/+0x14 に基底 vtable 群）で b26 の filter 系構造と同型。PickObject はフィルタを arg3 として渡す（request +0x14/+0x10 へ設定）。
