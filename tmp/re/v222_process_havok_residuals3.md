# v222_process_havok_residuals3.md — process/Havok 残点第3束（V-217-1..7 ＋ v204 継続1件）

- 日付: 2026-11-01 / 作成: opencode サブエージェント（TODO serial 216 / V216・中）
- 対象: `Oblivion.exe`（Steam v1.2.0416 / 7,898,624 B）
- 出典: `tmp/re/v217_process_havok_residuals2.md` §11 新規残点 V-217-1..7 ＋ `v204_process_residuals2.md` 継続1件
- 手法: capstone 5.0.7（`v172_common.py` / `probe_harness.py Ctx`／`v217_util.py` 再利用）＋ `cache/{named_funcs,vtable_index,call_targets,opcode_table}`（参照のみ）＋ `docs/Oblivion_decompiled.c`（部分読取）
- 中間物: `%TEMP%\opencode\v222_util.py` および `v222_1..N_*.py` / `v222_*_out.txt`
- selftest: 着手時 **4/4 PASS**（`probe_harness.py selftest`）
- 制約遵守: git / docs / GT / TODO / session-memory / plan / cache / 既存 `tmp/re/*.md` は**無編集**。新規は本 1 ファイル＋`%TEMP%\opencode/v222_*` のみ。ESM 読取 0 回
- **VA 換算の罠（v217 §ヘッダ継承）**: `.rdata+.data` を連続走査する imm32 リストは .data 部分が 0x600 過小。本タスクでは最初からセクション個別変換の `imm32_refs3()` のみを使用

---

## §1 item1（V-217-1）LifeState=3（Unconscious）の書手 — **解明: script command `SetUnconscious` 経由の tail-jmp で確定**

### 証拠A: オペコード表に `SetUnconscious` が存在（opcode_table 参照）

| opid | name | cond | impl |
|---|---|---|---|
| **0x10F1** | **SetUnconscious** | — | **`FUN_00505AC0`** |
| 0x10F2 | GetUnconscious | `FUN_004F7370` | `FUN_00505B50` |
| 0x10F3 | SetRestrained | — | `FUN_00505B70` |
| 0x10F4 | GetRestrained | `FUN_004F73F0` | — |

- **GetUnconscious cond `FUN_004F7370`**: IsActor ガード（vt+0x190）→ `call 0x5E0DC0`（LifeState getter）→ **`cmp eax,3; jne; fld1`** ＝ **LifeState==3 を "Unconscious" と定義する公式読手**（トレース文字列 0xA49148/0xA49160、`v222_1b_out.txt`）。

### 証拠B: SetUnconscious impl `FUN_00505AC0` の経路（`v222_1b_out.txt`）

1. `call 0x4FAE80`（script 引数/ターゲット解決ガード）→ 真で続行。
2. `FUN_009832E6(ref, 型フィルタ 0xB03054/0xB03070)` で**Actor 限定解決**。
3. flag≠0 → **`push 1; call FUN_005E8E30`** → `call 0x5E02B0`（process vt+0x194 更新要求）。
   flag==0 → **`push 0; call FUN_005E8E30`**。

### 証拠C: 真の値3 生成点 = **`FUN_005E8E30`（SetUnconscious 専用ヘルパ）**（`v222_1c_out.txt`）

```
FUN_005E8E30:  cmp [esp+4],0 ; je clear
               mov dword [esp+4], 3 ; jmp FUN_005E6680     ; == SetLifeState(this,3)（tail-call）
clear:         cmp [ecx+0xB0],3 ; jne ret
               mov [esp+4], 0 ; jmp FUN_006680系==0x5E6680 ; == SetLifeState(this,0)
               ret 4
```

- **値3 は `push 3` ではなく `mov [esp+4], 3`＋`jmp`（スタックスロット上書き＋テールジャンプ）**で SetLifeState へ渡される。これが v217 の「E8 呼出し21件の即値 census で 3 が不在」の**原因**（E8/即値の2 条件のどちらにも該当しない）。
- **E9 tail-jmp 全数走査**（`v222_1d_out.txt`）: `0x5E6680` への E9 は **`0x5E8E3F` / `0x5E8E55` の 2 件のみ**（ともに FUN_005E8E30 内、値 3 と 0）。E8 呼出し 21 件（{0,1,2,4,5,6}）＋ E9 2 件（{3,0}）で **SetLifeState の全入力経路が閉じる**。
- 副証: `FUN_005E8E30` の caller は `0x505B2D`/`0x505B40`（＝SetUnconscious impl 内の2 分岐）のみ。

### 裁定

> **LifeState=3 の書手は実在する**: script command **`SetUnconscious`（opid 0x10F1）** → `FUN_00505AC0` → **`FUN_005E8E30`** → `SetLifeState(actor,3)`（tail-jmp）。解除は SetUnconscious(0)（LifeState==3 のときのみ 0 へ）。Save 復元（`FUN_006022F0` の byte ロード）も値 3 を受け得るが、エンジン内の生成源は本経路に限定される（他に E9/E8 の書手なし）。

**判定: 解明**（証拠 `v222_1_out.txt` / `v222_1b_out.txt` / `v222_1c_out.txt` / `v222_1d_out.txt`）。

---

## §2 item2（V-217-2）Actor vt+0x1A0 述語 / vt+0x284(0x30) の id 意味 — **解明: `IsParalyzed`（AV 0x30 = Paralysis）**

### 前提（既存同定の再確認）

- Actor vtable `0xA6E074` 実測: `+0x1A0 = 0x5E17E0`、`+0x284 = 0x5F1970`、`+0x288 = 0x5F1A60`（`v222_2_out.txt` §A）。
- b14 §1.2 既確定: **vt+0x284 = `GetAV(int)`**（Actor/Character → `0x5F1970`、AV==9 のとき vt+0x288(GetAVFloat) に `push 0x28` 委譲＋`fild`）。id は **ActorValue コード**。

### 証拠A: `FUN_005F1970`（GetAV(int)）本文（`v222_2_out.txt` §B）

- `[this+0x58]`(process) なし → `FUN_005F1910`（base フォールバック）。
- AV==9（Magicka）専用枝: GetAVFloat(0x28)/100 クランプ → vt+0x170(level) → IsActor ガード → **process vt+0x268(this, 9, level)** → `fild × scale`。
- 一般枝（AV≠9 を含む）: vt+0x170 → IsActor ガード → **`process->vt+0x268(this, av, 0)`** の戻り値を int で返す。
  ⇒ id 0x30 も同経路（level 0）で処理される。

### 証拠B: AV 名前テーブル `0xB0A1A8`（.data、`v222_2b/2c_out.txt`）

- テーブル基底 `0xB0A1A8` = idx0 'Strength'（検証済み。idx7 'Luck' / idx8 'Health' / idx0xB 'Encumbrance' も v217/v163 の既存値と一致）。
- **idx 0x30（48）= `'Paralysis'`**（隣接: 0x2F 'Invisibility' / 0x31 'Silence'。終端 0x48 'Begin'）。
- 副証: b14 の「HighProcess `0x628CA0` は AV 0xB/0x30 を特別扱い」＝Encumbrance と **Paralysis** の 2 special-case と整合。

### 裁定

> **vt+0x1A0（`0x5E17E0`）の正式名は `IsParalyzed`**（本タスク裁定）:
> `return this->GetAV(Paralysis) != 0`（`push 0x30; call vt+0x284; test eax,eax; setne al`）。
> **vt+0x284 の id 0x30 の意味 = ActorValue コード 48 = Paralysis** で確定。opcode 表に Paralysis 系公開コマンドは無く（実測 0 件）、純粋な内部述語（AI/アニメ/FSM ガードの 30 箇所超で使用）。魔法効果 Paralysis がこの AV を積む側（magic 系）と突き合わせれば三辺一致するが、那是本タスク範囲外。

**判定: 解明**（証拠 `v222_2_out.txt` / `v222_2b_out.txt` / `v222_2c_out.txt`、既存 `b14_slot_method_map.md` §1.2）。

---

## §3 item3（V-217-3）[0xB3BCF0] 増分パス1 の単位 — **解明: T = TimeScale（ゲーム秒/実秒、ESM 既定 30.0）＝ fTimeScale 相当で確定**

### 証拠A: `FUN_004029E0`（時計進行）の代数と実引数（`v222_3d/3g_out.txt`）

- `hours_delta = T · x / 3600.0`（T=`[[0xB332E0+0x14]+0x24]`、3600 は double `0xA2F938`）→ **x が実秒のとき T はゲーム秒/実秒**（fTimeScale）以外に整合しない。
- **x の実体**: 毎フレーム更新 `FUN_0040D4D0@0x40DB8B` が `fld [0xB33E9C] → push → call FUN_004029E0`。[0xB33E9C] は**実時間フレームデルタ（秒）**:
  - リセット点 `FUN_009DC2A0@0x9DC2BC`: `fstp [0xB33E9C]`（0 初期化、計時器構造体 0xB33E80..0xB33EA4 の一部）。
  - 他利用点は全て `int × 定数 × [0xB33E9C]` の**フレーム数→秒変換**（`0x5106BA`/`0x6341CB`/`0x678147`、定数 0xA31C78=π/180・0xA3C770=0.25 等の角速度/係数合成）。
- **増分パス1 の内部整合**: `FUN_0065F770` は「**待機（Wait）の1時間ステップ**」処理（末尾 `add [esi+0x590],-1; jg` の残り時間カウントダウン、`[esi+0x6A8]/[+0x6AC]` 加算）。パス1 は同時に実行される **`FUN_004029E0(d)`（x = d = 3600/T）** を行う → `hours_delta = T·(3600/T)/3600 = **1.0 時間ちょうど**`。**d = 3600/T = 「1 ゲーム時間の実秒 duration」**（T=30 なら 120 実秒）。
- **パス2**（`FUN_00678510`、caller `FUN_0040D4D0@0x40DC2A`・`FUN_004433A0`）: `[0xB3BCF0] += [0xB33E9C]` ＝毎フレームの実秒 delta を加算。**パス1/パス2 とも単位は「実秒」で一致**。

### 裁定

> **T は fTimeScale 相当（ゲーム秒/実秒、ESM 既定 30.0）で確定**（フレーム長ではない。フレーム長なら FUN_004029E0 の代数が x に依存しないことと矛盾）。
> **[0xB3BCF0] = 累積実時間秒**（経過実秒のクロック）: パス2 が毎フレーム [0xB33E9C]（フレーム実秒）を、パス1 が待機1時間あたり 3600/T（＝1ゲーム時間の実秒）を加算。副产物: `[0xB3BCF4]` はフェーズマーカ（0/0x32/0x64/0xC8、`FUN_0040D4D0@0x40DBE6..` で切替）で v217 の「隣接カウンタ」呼びは訂正。

**判定: 解明**（証拠 `v222_3d_out.txt` / `v222_3g_out.txt` / `v222_3i_out.txt`、v217 §6 継続）。

---

## §4 item4（V-217-4）0xB332E0 ゲーム時計 singleton のクラスとサブオブジェクト配線 — **解明: vptr 無し POD ＋ サブオブジェクト＝TESGlobal 6 本（formID 0x35..0x3A）**

### 証拠A: 初期配線 `FUN_00402860`（`v222_3d_out.txt`）

```
FUN_00402860: for i in 0..5: [this + i*4] = FUN_0046B250(0x35 + i)
```
- **`FUN_0046B250(id)` = グローバル・フォームレジストリ `0xB0613C` からの formID 検索**（`NiTPointerMap` 型 API: Lookup `0x55E000` / Remove `0x4524C0` / Insert `0x452570`、ハッシュ配列先 `[0xB06144]`、キー=formID、値=TESForm*）。key 挿入例: `FUN_0046C460`=**TESForm ctor**（内部で一度きりのシグネチャ69種検証テーブル 0xB05E04＝NONE/TES4/GRUP/GMST/GLOB/.../TOFT）＋`FUN_00448D40`=**実行時 formID 割当器**（`[obj+0x8C0]` カウンタ、low24 満杯で +0x800＝modIndex 進行）。
- 呼出し元は `0x44F8FF`（`FUN_0044F3D0`＝ESM 読込後処理）1 箇所のみ。

### 証拠B: 6 サブオブジェクトの正体（`FUN_0044AC20` = **DataHandler の engine globals 確保ルーチン**、`v222_3n/3o_out.txt`）

各ブロックは「レジストリに formID で無ければ作成」パターン: `malloc(0x28)` → **ctor `FUN_004F95E0`**（**vptr `0xA4981C` = `.?AVTESGlobal@@`** を書く＝**TESGlobal**、value float @+0x24、type byte @+0x20、`FUN_0046C460`＝TESForm 基底 ctor 呼出し）→ `FUN_0046C300(obj,1,formID)`（レジストリ再登録、[obj+0xC]=formID）→ **vt+0xD8 = SetEditorID(name)** → value 初期値書込み → 名前マップ `0xB06164` へ `push obj; push name; call 0x412D30`。

| formID | 名前 | 既定値 | singleton スロット | スロット用途（getter 実測） |
|---|---|---|---|---|
| **0x35** | **GameYear** | 427.0 (`0xA2F940`) | +0x00 | `FUN_00402B40` = floor 値（年） |
| **0x36** | **GameMonth** | 7.0 (`0xA37CFC`) | +0x04 | `FUN_00402B80` = int（月、既定7） |
| **0x37** | **GameDay** | 17.0 (`0xA2F944`) | +0x08 | `FUN_00402BA0` = int（日） |
| **0x38** | **GameHour** | 12.0 (`0xA2F918`) | +0x0C | `FUN_00402BD0` = float（現在時刻 0..24、既定12:00） |
| **0x39** | **GameDaysPassed** | 1.0 | +0x10 | `FUN_00402C00`＝int；`FUN_00402C40/90`＝**mod 7 → 曜日**（テーブル 0xB06FD4、名前 0xA30A6C 'Sunday' 帯） |
| **0x3A** | **TimeScale** | **30.0** (`0xA37CC8`) | +0x14 | `FUN_004029D0` = **T**（`FUN_004029E0` が x·T/3600 で時間を進める） |

- 名前文字列は .rdata に連続配置（`0xA37CC0 'Player' / 0xA37CCC 'TimeScale' / 0xA37CD8 'GameDaysPassed' / 0xA37CE8 'GameHour' / 0xA37CF4 'GameDay' / 0xA37D00 'GameMonth' / 0xA37D0C 'GameYear'`、`v222_3l/3m_out.txt`）。
- `FUN_004029E0` は時計進行時に **GameHour(+0xC) へ新時刻を fstp し、日跨ぎループで GameDaysPassed(+0x10) を加算・GameDay/Month/Year(+8/+4/+0) へ繰上げ伝播**（`[0xB333C4]`(player) の `[+0x6B0]` カウンタ加算を伴う）。

### 裁定

> **`0xB332E0` は vptr を持たない POD 構造体（6 × TESGlobal*）**であり、「クラス正式名」は C++ 意味では存在しない（RTTI/文字列とも無し）。エンジン呼称は名前マップ登録名から **`TimeClock` 相当の6変数束**とし、仮称 **"GameTimeGlobals"**。
> **サブオブジェクトは NiTimeController ではない（v217 の推定を訂正）**: 正体は **TESGlobal インスタンス6 本**（GameYear/GameMonth/GameDay/GameHour/GameDaysPassed/TimeScale、DataHandler が ESM に存在しない場合にランタイム生成、formID 0x35..0x3A）。+0x24 は TESGlobal::value float。**TimeScale（既定30.0）が item3 の T の正体**（GECK の fTimeScale GMST と対になる runtime 変数。コンソール/スクリプトの "Set TimeScale to" 経路でもある）。

**判定: 解明**（証拠 `v222_3b_out.txt` / `v222_3c_out.txt` / `v222_3d_out.txt` / `v222_3f_out.txt` / `v222_3h_out.txt` / `v222_3i_out.txt` / `v222_3j_out.txt` / `v222_3k_out.txt` / `v222_3l_out.txt` / `v222_3m_out.txt` / `v222_3n_out.txt` / `v222_3o_out.txt`）。

---

## §5 item5（V-217-5）hk3AxisSweep vtable `0xA9A710` slot2-9/11-13 の名称 — **大幅縮小（slot2-6/8/9 は根拠付き命名、7/11/13 は[残]）**

### 補足実測（本タスク）

- **RTTI 無しを再確認**: `[vtable-4] = 0`（COL 不在）。
- **プロファイル文字列 → 参照関数の対応**（`v222_5c_out.txt`）: `'LtquerySingleAabb'`/`'marker'`/`'Styz-Axis'`/`'StScanBitfield'` は slot10(`0x8E1E20`)/slot16(`0x8E5950`) 内、`'Lthk3AxisSweep'`/`'memory'` は slot14(`0x8E24A0`)/slot17(`0x8E2F20`) 内、`'StWalk'`/`'StStartOverlaps'`/`'Stbitfield'` は slot14 内、**`'3AxisSweep'`/`'Nodes'`/`'Markers'`/`'Axis'`(0xA51AFC) は slot1(`0x8E3960`) 内**。
- **量子化定数の実値**: `0xA9A650 = 32768.0f` / `0xA9A660 = 0.0f` / `0xB2FC70 = 65532.0f` ＝ v205 の「16bit 量子化」を数値で裏付け（+32768 シフト、0..65532 クランプ）。
- **slot impl への E8 直接呼出しは0件**（全て仮想ディスパッチ経由）。
- データ構造（ctor `0x8E4730` と impl 逆算）: `+0x10/+0x20/+0x30` = 軸毎 offset/scale (hkVector4)、`+0x40` = **Nodes** 配列（stride 0x10）、`+0x44` = マーカ総数、`+0x48` = フリーリスト（flags 0x3FFFFFFF マスク）、`+0x4C/+0x58/+0x64` = **3軸ソート配列**（stride 4、'Axis'）、`+0x50/+0x5C/+0x68` = 各軸カウント、`+0x70` = 削除済カウント。ハッシュテーブルは `[+0x44]` 系ではなく slot15 の式 `([+0x44]-[+0x70])*3*8+0x24` が aabb 構造体サイズ算出。

### スロット表（本タスク裁定・確度付き）

| slot | impl | 名称 | 確度 | 根拠 |
|---|---|---|---|---|
| 0 | `0x8E6460` | dtor（scalar、refcount リリース） | 高 | 既存 |
| 1 | `0x8E3960` | **内部状態ダンプ**（アーカイブ風オブジェクトへ '3AxisSweep' + 'Nodes' + 3×'Axis' 配列の (ptr,start,end) 範囲を出力）— v217 の「ノード/登録操作系」を**訂正** | 中 | 2回の引数オブジェクト仮想呼出し＋4配列出力ループ（`v222_5d_out.txt`） |
| **2** | `0x8E1660` | **addEntity(handle, aabb)**（float AABB を16bit量子化→マーカ/3軸配列へ挿入） | 中 | `[ebp+0xC]`=AABB、量子化定数 3 セット、`[edi+0x40]` へ `lea`（`v222_5c_out.txt`） |
| **3** | `0x8E5130` | **addChunk(entities[], numEntities, ...)**（一括登録: `[arg+4]≥1` ガード、マーカ配列(stride0x10)と軸配列(stride4)を `0x8A6E40`(resize) で一括拡張） | 中 | 2重 resize・count ガード |
| **4** | `0x8E19C0` | **removeEntity(handle)**（handle→node（×16）→内部ヘルパ `FUN_008E0E90` で3軸リストから除去→マーカビットフィールド走査） | 中 | `v222_5e_out.txt` 全文 |
| **5** | `0x8E5630` | **removeChunk(entities[], numEntities)**（ハンドル配列ループ＋ビット演算（`and 0x1F;sar 5;shl`）でビットフィールド再構築） | 中 | `v222_5c_out.txt` |
| **6** | `0x8E4590` | **castAabb(aabb, input, collector)**（aabb 量子化＋ハンドル配列走査ループ、第3引数 collector） | 中 | 定数3セット再利用・`[ebp+0x18]` 引数 |
| **7** | `0x8E61E0` | **collide 系（用途[残]）**: pool から `(m_max+1)*16` / `m_max*4` ×2 の3バッファを確保 | 低 | `v222_5c_out.txt`（末尾未読） |
| **8** | `0x8E1D90` | **getAabbs（全ノード一括エクスポート）**: 出力 hkArray(stride 0x20) を m_max へ resize→`FUN_008E1060` でノード(0x10)を複写 | 中 | `v222_5d_out.txt` |
| **9** | `0x8E1E00` | **getAabb（handle→node 変換の薄いラッパ）**: `[arg]×16+this+0x40` を組み `FUN_008E1060` へ tail | 中 | 7命令 |
| **10** | `0x8E1E20` | querySingleAabb（v205 確定、'LtquerySingleAabb'） | 高 | 既存 |
| **11** | `0x8E14E0` | **updateEntity 除去側 or マーク系 [残]**（`FUN_008E0E90` 呼出し＋ビットフィールド一時バッファ確保、slot4 と同型の入口） | 低 | `v222_5d_out.txt` |
| **12** | `0x8E3A90` | **updateEntity(handle, hkpAabbUint32*)**（16B packed AABB をコピー→軸毎 scale で量子化→3軸ソート配列をバイナリサーチして位置更新） | 中 | `[eax..+0xC]` 16Bコピー・`this+0x50` 軸配列探索ループ |
| **13** | `0x8E1160` | **setWorldAabb 系（world AABB 再設定）[確度低]**: 引数ベクトルを `xorps` 符号反転→`vt+0x30`(=slot12) 呼出し→`this+0x10/0x20` へ反映（`1.0/[+0x30..+0x38]` 割） | 低 | `v222_5d_out.txt` |
| 14-17 | — | castRay / getAabbCount / querySingleAabb 本体 / クラス名・トレース（v195/v205 確定） | 高 | 既存 |

### 裁定

> v217 の残点 **slot2-6/8/9 は「addEntity/addChunk/removeEntity/removeChunk/castAabb/getAabbs/getAabb」系として根拠付きで命名**（ Havok 4.x/2005 `hkpBroadPhase` のメソッド群と挙動が対応）。**slot7/11/13 は[残]**（slot7=3バッファ確保系、slot11=slot4 同型入口、slot13=world AABB 再設定系）。slot1 は登録操作ではなく**ダンプ系**だった（v217 訂正）。

**判定: 大幅縮小（7/11/13 は[残]）**（証拠 `v222_5_out.txt` / `v222_5b_out.txt` / `v222_5c_out.txt` / `v222_5d_out.txt` / `v222_5e_out.txt`）。

---

## §6 item6（V-217-6）`0xA96B78` の4書手と `0xA96B64/68/70` view 対応 — **解明（＋v217 の「4 書手」を1件訂正）**

### 実測A: ラン全体と view の切り出し関係（`v222_6_out.txt` §A）

`0xA96B58` 起点の10連続コードポインタ（b13 #73）は **1つのクラス vtable ＋ そこから切り出したネスト view** である:

| vtable | slots（実装） |
|---|---|
| `0xA96B58`（クラスA 本体） | {`0x899300`(scalar dtor、`[+0]=hkBaseObject 0xA45E5C`), `0x68F970`(no-op `ret 4`), `0x8992F0`(`lea eax,[ecx+8]; ret 0xC` = +8 のサブインターフェイス返却), `0x899380`, `0x899390`, `0x8993E0`, `0x927A70`, `0x8993D0`, `0x8993A0`, `0x68F970`} |
| **`0xA96B64`** | {`0x899380`, `0x899390`, `0x8993E0`, `0x927A70`, `0x8993D0`, `0x8993A0`, `0x68F970`}（= ラン +0xC 以降の7スロット view） |
| **`0xA96B68`** | {`0x899390`, `0x8993E0`, `0x927A70`, `0x8993D0`, `0x8993A0`, `0x68F970`} |
| **`0xA96B70`** | {`0x927A70`, `0x8993D0`, `0x8993A0`, `0x68F970`} |
| **`0xA96B78`**（クラスB 本体） | {`0x8993A0`(deleting dtor), `0x68F970`(no-op)} |

- インターフェイス実装の意味（`v222_6b_out.txt`）:
  - `0x899380` = `*(byte*)arg1 = 1; ret 0xC` / `0x899390` = 同 `ret 0x10` / `0x927A70` = 同 `ret 0x18` ＝ **全衝突許可（always-true）フィルタ判定**。
  - `0x8993E0` = `sub ecx,0x10; jmp 0x8993A0` / `0x8993D0` = `sub ecx,0xC; jmp 0x8993A0` ＝ **this 調整付き scalar dtor ソンク**（+0x10/+0xC サブオブジェクト用）。
  - `0x8993A0` = deleting dtor（`call FUN_008DA1D0`（scalar dtor）→ refcount 0 なら `[0xBA7D98]` アロケータで size `[+4]=0x18` を free）。

### 実測B: クラスB（vtable `0xA96B78`）の ctor とレイアウト（`v222_6_out.txt` §D）

```
FUN_00899330 (ctor, this=ecx, 0x18B):
  [+6]=1(refcount)
  [+8]=0xA95C60 (.?AVhkCollidableCollidableFilter@@)   ← 第1段: 既定フィルタ vtable
  [+0xC]=0xA95C3C (.?AVhkShapeCollectionFilter@@)
  [+0x10]=0xA95C48 (.?AVhkRayShapeCollectionFilter@@)
  [+0x14]=0xA95C68 (.?AVhkRayCollidableFilter@@)
  [+0]=0xA96B78, [+8]=0xA96B64, [+0xC]=0xA96B70, [+0x10]=0xA96B68, [+0x14]=0xA96B64  ← 第2段: concrete へ上書き
```

- **view 対応の確定**（RTTI index の typedesc 照合、`v222_6_out.txt` §B）:

| オフセット | 既定（第1段） | concrete（第2段） | インターフェイス |
|---|---|---|---|
| +0 | — | `0xA96B78` | クラス自身（dtor + no-op） |
| +8 | `hkCollidableCollidableFilter` | **`0xA96B64`** | collidable×collidable フィルタ |
| +0xC | `hkShapeCollectionFilter` | **`0xA96B70`** | shape collection フィルタ |
| +0x10 | `hkRayShapeCollectionFilter` | **`0xA96B68`** | ray×shape collection フィルタ |
| +0x14 | `hkRayCollidableFilter` | **`0xA96B64`**（+8 と同一 view 共用） | ray×collidable フィルタ |

### 実測C: 「4 書手」の再検（v217 §9 訂正）

1. **`FUN_00899330`（standalone ctor）**: caller は **`0x89D363` の1件のみ**（`FUN_0089D340`＝**ahkWorld のフィルタ差替/setter**: `[world+0x78]` の旧オブジェクトを refcount デクリメント＆解放 → 新オブジェクトを格納 → `return &obj->+0xC`（hkShapeCollectionFilter メンバのインターフェイスポインタ））。
2. **`FUN_0089A230`（ahkWorld ctor）内 @`0x89A802-0x89A831`**: ctor の**完全インライン展開**（byte 列実測: push0x24/push0x18 → allocator call → `[+4]=0x18` → 既定4 vtable → concrete4 vtable → `[esi+0x78]=eax`、refcount inc 付き）。＝ ahkWorld は **+0x78 に本フィルタのインスタンスを内蔵**。
3. **`FUN_008DA1D0` @`0x8DA23C`**: **scalar dtor の派生部**（caller = `0x8993A3`＝クラスB primary vtable slot0 の中）。本文: `+8/+0xC/+0x10/+0x14` に concrete view 復元 → `+0x10=0xA95C48`、`+0xC=0xA95C3C`（既定へ戻す）→ `[+0]=0xA45E5C`（**hkBaseObject**）＝ **dtor 時の vtable フェーズ降下**。書手というより dtor 連鎖。
4. **`FUN_00A12BC0` @`0xA12BD2`**: **vptr 書手ではない（v217 の誤検出を訂正）**。`mov eax,0xA96B78` → stack へ4バイト分割（`[esp+0xC..0xF]` に逆順格納）＝ **シリアライズ型スタンプ構築**（メモリ書込みなし）。

### 裁定

> `0xA96B78` クラス = **Havok の allow-all フィルタ集約体（`hkpNullCollisionFilter` 相当）[確度中]**: 4つの Havok フィルタインターフェイス（collidable×collidable / shapeCollection / ray×shapeCollection / ray×collidable）を単一0x18B オブジェクトに嵌め、全判定を「許可(1)」で返す。**ahkWorld が +0x78 に内蔵**し、実行時 setter `FUN_0089D340` で差替え可能。**RTTI 版 `bhkCollisionFilter`（行1603-1607、vtable 0xA95CBC..0xA95CE4、sub-vptr オフセット +0/+8/+0xC/+0x10/+0x14 が完全同一レイアウト、ctor `FUN_0088A570`）とは別クラス**（同一レイアウトの系統関係 [確度高]、継承の向きは[残]）。クラス正式名は非 RTTI のため EXE に露出せず [残]（仮称 "ahkNullFilter"）。

**判定: 解明（view 対応・役割）＋ 書手4件目を訂正（FUN_00A12BC0 は非書手）**（証拠 `v222_6_out.txt` / `v222_6b_out.txt`、`b13_nonrtti_vtable_classes.md` §3.5 #73、`vtable_index.tsv` 行1603-1607/1678/1686-1707）。

---

## §7 item7（V-217-7）補助 process（0x154・ctor `FUN_00478730`）のクラス名と +0x104/+0x5C8 割当 — **解明（前提訂正: MiddleLowProcess ではない）**

### 証拠A: 0x154 オブジェクトは**非ポリモーフ**（vptr 書手不在）

1. ctor `FUN_00478730` 本文（`v222_7_out.txt` §A）: `memset(this,0,0x154)` ＋ `memset(g_0xB33C80,0,0x100)`（グローバル記録領域クリア）→ `[+0x150]=arg1(owner ref)` → arg2 ありなら `call FUN_00478070(this,form)`（アドオン付着）→ return。**vptr 書込なし**。
2. **ctor の caller は3件のみ**（`v222_7_out.txt` §B）: `FUN_00521A40+0x92` / `FUN_00523170+0x79` / `FUN_00667BE0+0x236`。いずれの後続も **vptr 書込なし**（malloc(0x154)+ctor → Character/Player vtable の setter（+0x16C/+0x14C）へ渡すのみ、`v222_7b_out.txt`）。
3. 破棄系も仮想 dtor 経由ではなくレコードループ（`FUN_0047AB80`＝`FUN_004796F0`）＝非ポリモーフ整合（v217 §1 確定）。

### 証拠B: MiddleLowProcess は**別クラス・別サイズ**（本タスクの新事実）

- **MiddleLowProcess（RTTI、vtable `0xA72D64`）の ctor = `FUN_00658690`**（`v222_7_out.txt` §C の vptr 書手2件のうち1つ目）: `call FUN_006438F0`（**LowProcess ctor**、vptr `0xA720A4` を `0x643922` で書く）→ `[esi]=0xA72D64` → `+0x94` 埋め込みメンバの ctor（`FUN_0065BE10`／別形 `FUN_006586F0` は `FUN_0065CCC0`+`FUN_00648CB0`）。
- **ctor 連鎖の実証**: **`FUN_0064B400`（MiddleHighProcess ctor）@`0x64B42A` が `call 0x658690` の直後に `[esi]=0xA72684`（MidHigh vtable）** ＝ LowProcess→MiddleLowProcess→MiddleHighProcess の継承チェーン。
- **MiddleLowProcess の確保サイズ = `0xA8`（168B）**: `FUN_005EBB30@0x5EBCE3` と `FUN_00659C90@0x659E05` が `push 0xA8; call malloc` → `call 0x658690`（`v222_8c/8d_out.txt`）。
- ⇒ **0x154 ≠ 0xA8: 補助コンテナは MiddleLowProcess ではない**。v217 起票時の「LowProcess/MiddleLowProcess のどれか」二択は**両方とも否定**（LowProcess=0x90B・MidLow=0xA8B・補助=0x154B の三者三様）。補助コンテナの正式クラス名は非ポリモーフのため存在せず（仮称 **"AttachRecordContainer"**、v142 の「アタッチブロック」と同物）。

### 証拠C: +0x104/+0x5C8 の「1st/3rd person」割当の実証

- getter `FUN_006600D0(flag)` = `flag ? [this+0x5C8] : [this+0x104]`（既存確定）。本タスクで **caller 38 件**を実測（`v222_7c_out.txt`）:
  1. **flag = byte `[g_thePlayer+0x588]` の直接読取**: `0x4DCD34`（`mov ecx,[0xB333C4]; mov al,[ecx+0x588]; push eax; call 0x6600D0`）＝ v142 の decompiled L173563 と同一経路。
  2. **flag = 即値1**: アドオン系（`FUN_00478780`/`FUN_00478CE0`/`FUN_0047BC40` 帯）が `push 1` で「プレイヤーの第2ブロック」を直接参照（すべて `owner([+0x150])==ref` 検査を併走）。
- **`[Player+0x588]` = Show1stPerson フラグ**（既存確定: `v69_show1stperson_link.md`/`v72_global_b333c4_map.md`/`s102_camera.md`: console `S1ST`（opid 0x113）ハンドラ `0x50A050` が 0/1 書込み、エンジン共通 setter `FUN_0066C580`（22 呼出）、`FUN_00671620` が xor トグル）。

### 裁定

> - **+0x104 = 通常時（3人称/Show1stPerson=0）に使用される第1アタッチブロック**、**+0x5C8 = Show1stPerson=1（1人称モデル表示）時に使用される第2ブロック** — getter の flag 源が byte `[Player+0x588]` であることで**割当実証が確定**（v204 の「厳密な割当は未証明 [残]」への回答）。
> - ただし両者が指すのは「process」ではなく **0x154B の非ポリモーフアドオン記録コンテナ**（v217 §1）。「1st/3rd person **process**」という通称は**誤り**（真正の process ヒエラルキーは [REFR+0x58] の Low(0x90)/MidLow(0xA8)/MidHigh(0x18C)/High 系）。

**判定: 解明（前提訂正つき）**（証拠 `v222_7_out.txt` / `v222_7b_out.txt` / `v222_7c_out.txt` / `v222_8c_out.txt` / `v222_8d_out.txt`、既存 `v142_s7010_residuals.md` / `v69_show1stperson_link.md`）。

---

## §8 item8（v204 継続）外部 setter `0x629440` 経由の KnockedState 値1 生成 — **解明: 在らず（値3 のみ実証）**

### 証拠A: setter への経路は E8 直接0件・仮想ディスパッチ1件（`v222_8b/8c_out.txt`）

- `E8 call 0x629440` = **0 件**（本文再確認: `mov al,[esp+4]; mov [ecx+0x11C],al; ret 4`）。
- disp `0x2E8` の .text 全出現58件を分類: **`mov r32,[reg+0x2E8]`（load）は2件のみ**。残りは他クラスの byte/dword/float フィールド書込・x87 命令・`lea`・スタックオフセット（`cmp edi,0x2E8` の1件はアニメ軌道ループのバイト長定数＝偶然一致）。
  1. **`0x5F6142`（`FUN_005F6020+0x122`）: `mov ecx,[esi+0x58]`(process) → `mov edx,[eax+0x2E8]` → `push 3` → `call edx`** — 死亡/消滅系ヘルパ内で **KnockedState=3（GettingUpPrep）** を外部 setter で設定。
  2. `0x5FD4A8`（`FUN_005FCAB0+0x9F8`）: `mov ecx,esi` の esi は **Actor**（同関数内で vt+0x284 GetAV を使う actor this）。**Actor vt+0x2E8 = `0x60D0A0` = `ret`（no-op・引数なし）** であり process setter ではない。
- vtable 側の整合: High `0xA71814`/MidHigh `0xA72684` の +0x2E8 = `0x629440`（実 setter）、**MidLow `0xA72D64`/Low `0xA720A4` の +0x2E8 = `0x68F970`（no-op `ret 4`）**（`v222_8d_out.txt`）＝ v200 の D5 行と一致。

### 裁定

> **KnockedState 値1 を生成する外部 setter 経路は存在しない**。`0x629440` への実際のディスパッチは `FUN_005F6020`（push 3）の1件のみ。値1 の生成源は v204 確定どおり **FSM `FUN_006545E0` 内部**（Lead-In→Main 映射 `(old==4)*2+1`、`0x65471F` 帯の直接書込）に限定される。v200 §4-1 の留保（「setter 経由でも生成し得る」）は**否定**で決着。

**判定: 解明（在らず）**（証拠 `v222_8_out.txt` / `v222_8b_out.txt` / `v222_8c_out.txt` / `v222_8d_out.txt`）。

---

## §9 新規残点（本タスクから起票）

| ID | 内容 | 優先 |
|---|---|---|
| **V-222-1** | hkp3AxisSweep vtable slot**7/11/13** の用途（slot7=pool 3バッファ確保系、slot11=slot4 同型入口、slot13=world AABB 再設定系）— slot 本文の末尾未読 | 低 |
| **V-222-2** | `0xA96B78` クラス（ahkWorld+0x78 の allow-all フィルタ集約体）の正式名と RTTI 版 `bhkCollisionFilter`（ctor `FUN_0088A570`）との継承/並立関係 | 低 |
| **V-222-3** | （v161 から継承）`FUN_00667BE0`（Player vt+0x14C、+0x5C8 第2アタッチブロック生成）の喚起元18 vcall サイトの受信クラス照合 | 低 |
| **V-222-4** | [0xB33E9C]（フレーム実時間 delta）の実際の書手（0xB33E80 帯タイマー構造体の更新関数 — `FUN_009DC2A0` はリセットのみで、進行時の書手は構造体経由で未特定） | 低 |

---

## §10 自己検証・証跡

- **`probe_harness.py selftest` 4/4 PASS**（着手時）。
- 内部整合（独立一致 10 件）:
  1. SetUnconscious 経路: `FUN_005E8E30` の tail-jmp 値3 と GetUnconscious cond `FUN_004F7370` の `cmp eax,3` が一致。E9 全数走査（2 件）で SetLifeState 入力経路が閉じる。
  2. AV 名前テーブル基底 `0xB0A1A8`=idx0 'Strength'（idx7/8/0xB の既存値と一致）→ idx0x30='Paralysis' の帰結が安全。
  3. TimeScale 既定値 30.0（.rdata `0xA37CC8`）＝ GECK fTimeScale 既定と一致。`T·x/3600` 代数と待機1時間ステップ（`hours_delta==1.0`）が整合。
  4. 6 グローバルの ctor `FUN_004F95E0` が書く vptr `0xA4981C` = vtable_index 行1324 `.?AVTESGlobal@@` と一致。value float +0x24 も TESGlobal レイアウトと整合。
  5. GameHour 既定 12.0 / GameDaysPassed 既定 1.0 / GameYear 427.0 / GameDay 17.0 / GameMonth 7.0 が各 getter のフォールバック定数（`0xA2F918`/`fld1`/`0xA2F940`/`0xA2F944`/`0xA37CFC`）と1対1で一致。
  6. MiddleLowProcess サイズ 0xA8（malloc 2 箇所）と補助コンテナ 0x154 の不一致＝別クラス。MidHigh ctor の MidLow 連鎖（`0x64B42A`→`[esi]=0xA72684`）と vtable_index 行828/829 が整合。
  7. process vt+0x2E8 = High/MidHigh のみ `0x629440`、Low/MidLow は `0x68F970`(no-op) — v200 D5 行と一致。Actor vt+0x2E8 は `ret` no-op で呼び出し無害。
  8. 量子化定数 `0xA9A650`=32768.0f / `0xA9A660`=0.0f / `0xB2FC70`=65532.0f が v205「16bit 量子化」と整合。
  9. `0xA96B78` クラスのレイアウト（+0/+8/+0xC/+0x10/+0x14 の5 vptr）が bhkCollisionFilter RTTI 行1603-1607 の sub-vptr オフセット（0/8/0xC/0x10/0x14）と完全一致。
  10. `FUN_0089D340` の refcount 操作（[+4]=size、[+6]=refcount、0 で deleting dtor 呼出）と ahkWorld ctor のインライン ctor 展開（byte 列一致）が、同一クラスの2生成経路として整合。
- 制約遵守: 新規ファイルは本 1 件＋`%TEMP%\opencode/v222_*`（スクリプト 18 本＋出力 18 本）。docs/・既存 tmp/re・GT・TODO・session-memory・plan・cache・git は編集 0 件。ESM 読取 0 回。

---

## §11 Android 移植への示唆（実装しない）

- `SetUnconscious`（opid 0x10F1）→ LifeState 3 の tail-call 経路を再現する場合、setter 呼出しを E8 直呼として実装すると opcode 経路が漏れる。ヘルパ `SetLifeStateConditional(ref, flag, value_if_set, guard_value)` 形（SetRestrained 0x5E8E60 も同型）として実装すると原文の契約（解除は「現在値==対象値のときのみ0」）を再現できる。
- vt+0x1A0 = `IsParalyzed`（GetAV(Paralysis)!=0）。AV 名前テーブル `0xB0A1A8`（0..0x48='Begin'）は enum→文字列表として移植可能。Paralysis AV(0x30) を積む側（magic）と合わせて三辺一致させること。
- ゲーム時計は **TESGlobal 6 本（GameYear/GameMonth/GameDay/GameHour/GameDaysPassed/TimeScale）へのポインタ束**として実装し、GMST/GLOB レジストリ（formID→form）から解決する原文の構造を再現すると、コンソール `Set TimeScale to` やスクリプト経由の変更が自動的に時計へ反映される。時間進行は `hours += TimeScale·Δt/3600`、24h wrap 時に日跨ぎ（GameDay/Month/Year 繰上げ＋GameDaysPassed 加算）。
- [0xB3BCF0] 相当（経過実秒）は、毎フレーム Δt(実秒) を加算し、待機1時間ステップで `3600/TimeScale`（＝1ゲーム時間の実秒）を加算する二経路設計を再現。
- ahkWorld は作成時に allow-all フィルタ（+0x78）を内蔵する。移植版ではフィルタを差替可能インターフェイス（4 フィルタの集約）として保持すること。
- 補助コンテナ（0x154、アドオン記録16個）は process ヒエラルキーとは**別物**として分離実装。Player の第2コンテナは Show1stPerson フラグ（Player byte +0x588）で選択。
