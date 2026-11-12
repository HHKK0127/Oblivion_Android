# V350: NiKeyframeData::Load 同定再裁定 — walk.rs 正典昇格と v349 §2 訂正（#122 反証依頼への回答）

- 日付: 2026-11-10 / タスク: ai-todo **#122**（V350 反証依頼・main 起票） / 作成: RE レーン（opencode 直接解析）
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 Steam / 7,898,624 B / ImageBase 0x400000）
- 手法: capstone 5.0.7（v307_common.py — section map は v305 検証契約。`probe_harness.py` は %TEMP% に**不存在**のため v121 系と同じ v307_common 読取形で代替）、factories/COL/RTTI 走査、main 実測（tmp/re/n5b_samples 5本・walk.rs）との三方突合
- 中間証跡: `%TEMP%\opencode\v350_1..v350_8.py/.out`（**全 EXIT=0**。v350_8 初回のみ .bss 読取例外 → ガード追加で再実行成功）
- 制約: git / docs / GT / TODO / session-memory / plan / 既存 `tmp/re/*.md` は**無編集**（本ファイル新規1件のみ）、ESM 未アクセス、L1

---

## §0 判定サマリ（依頼依頼①②③）

| # | 依頼 | 判定 |
|---|---|---|
| ① | 0x6DFDE0（v349 が「NiKeyframeData vtbl+0x1C=Load」とした関数）の同定再裁定 | **[確定] 0x6DFDE0 = NiLookAtInterpolator::Load（file-load 経路）**。実行時変換側 = 0x6D3BD0（同じく NiLookAtInterpolator）。**どちらも NiKeyframeData の Load ではない** |
| ①-b | NiKeyframeData の正体 | **[確定] 本バイナリに NiKeyframeData 独立 C++ クラスは存在しない**。NIF 型名 "NiKeyframeData" と "NiTransformData" は**同一 create fn 0x6E1C00 → NiTransformData クラス（vtable 0xA7B474）へエイリアス登録** |
| ② | walk.rs レイアウトの正典昇格 or 両経路の切り分け | **[確定] walk.rs skip_keyframe_data を正典昇格してよい** — EXE 実 Load 0x6E2180 と指令単位で1:1、main の .kf バイト精算とも完全一致 |
| ②-b | v349 §2 の注記訂正 | **[確定] 訂正（7件・§3 訂正表）** — クラス同定・帰属・スロット数すべて誤り。台帳 row326/V326 は row327 で追補 |
| ③ | .keyf family（NiFloatData 等）単独 body の命令確認 | **[確定] 全 family Load が先頭 u32 count**（4B read → cmp0 → jbe）。NiBoolData のみ追加バージョンゲート。NiKeyframeManager は別形式（key-data body 非該当） |

---

## §1 依頼①: 0x6DFDE0 の同定再裁定 [確定]

### 1.1 ファクトリ登録の決定的証拠（name → create → ctor → vptr 鎖）

NIF ブロックファクトリ `0x6C2DB0`（登録）/ `0x6C31F0`（解除）は `push create_fn / push name_str / call 0x712590` の対で登録する（arg1 = name = top）。**ペア方向は NiTransformData 事例で検証済**（下記 c の RTTI 一致）。

| name 文字列 | VA | create fn | 出典（v350_5/6） |
|---|---|---|---|
| `NiKeyframeData` | 0xA791F8 | **0x6E1C00** | `6C2FDD: push 0x6e1c00 / 6C2FE2: push 0xa791f8` |
| `NiTransformData` | 0xA79024 | **0x6E1C00**（同一！） | `6C3130: push 0x6e1c00 / 6C3135: push 0xa79024` |
| `NiLookAtInterpolator` | 0xA79188 | **0x6DFD70** | `6C302B: push 0x6dfd70 / 6C3030: push 0xa79188` |
| `NiKeyframeManager` | 0xA791E4 | 0x6E1A00 | `6C2FEC: push 0x6e1a00 / 6C2FF1: push 0xa791e4` |
| `NiKeyframeController` | 0xA79208 | 0x6C3F00 | `6C2FCE: push 0x6c3f00 / 6C2FD3: push 0xa79208` |

### 1.2 create → ctor → vtable

- **create 0x6E1C00**（NiKeyframeData/NiTransformData 共通）: SEH prologue → `push 0x2c; call 0x401F00`（**new(0x2C=44B)**）→ `call 0x7005D0`（base ctor: vptr 0xA3FCF8→**0xA7D61C**）→ **`mov [esi], 0xA7B474`** → フィールド初期化3組:
  `{+8 word, +0x20 ptr, +0x10 ptr, +0x1c byte}` / `{+0xa, +0x24, +0x14, +0x1d}` / `{+0xc, +0x28, +0x18, +0x1e}`
  ＝**3 KeyGroup スロット**（rot/trans/scale 用の型情報群。setter 小関数 0x6E1A80/0x6E1AC0 が各組をまとめて書き）
- **create 0x6DFD70**（NiLookAtInterpolator）: `push 0x44; new`（**new(0x44=68B)**）→ `push 0/push 0/push 0; call 0x6DFB70`
- **ctor 0x6DFB70**: `mov [esi], 0xA7B0D4`（vptr 書）→ +0x10=arg / word[+0xC]=0 / +0x14=0 / +0x18..+0x20=既定グローバル(0xB24260..68) / +0x24..+0x30=0xB3CBA4..CBB0 / +0x34=fld[0xA79E10] / +0x38..+0x40=3スロット初期化（0x984BCF ヘルパ、dtor 0x7016A0、count=3、stride=4）→ **vtable 0xA7B0D4 = NiLookAtInterpolator**

### 1.3 RTTI 補正 COL 走査（v121_1 旧 walk の誤りを補正）

旧 v121_1 は `rd32(col+12)` を**name 文字列 VA** と直接比較（不一致で0件）。本ラウンドは **TD 構造体 = name_va − 8**（MSVC TypeDescriptor: pVFTable/spare/name[+8]）＋ pTD@col+12（VS2003/2005 形5フィールド COL）で再走査 → 22 COL 候補から:

| クラス | vtable | vt+0x1C (Load) |
|---|---|---|
| **NiTransformData** | **0xA7B474** | **0x6E2180** |
| NiFloatData | 0xA7A2EC | 0x6E3690 |
| NiRotData | 0xA7AB3C | 0x6D8F60 |
| NiPosData | 0xA7A624 | 0x6D9F90 |
| NiBoolData | 0xA7C2AC | 0x6E8A30 |
| NiControllerSequence | 0xA79A34 | 0x6C8E40 |
| NiTextKeyExtraData | 0xA7AA0C | 0x6D7780 |
| NiKeyframeManager | 0xA7B3FC | 0x6E12C0 |
| NiTimeController | 0xA7EB24 | 0x715F40 |
| NiGeomMorpherController | 0xA7A23C | 0x6D0D30 |

- **`.?AVNiKeyframeData@@` RTTI TD はバイナリに存在しない**（rdata+data 全走査・`.?AVNi*` 388 名中0件）＝**独立クラス不在の直接裏付け** [確定負]
- 0xA7B0D4 の RTTI TD も存在しないが、ファクトリ鎖（1.1/1.2）で **NiLookAtInterpolator** と同定 [確定]

### 1.4 0x6DFDE0 本体の実読み順（v350_1/2）

- 範囲: **0x6DFDE0..0x6DFE8F（ret 4 @6DFE8F、size 0xB0）**。`func_bounds` の end=0x6E0C00 は **linear 過走の誤測**（int3 塗り 0x6DFE92.. + 次 fn@0x6DFEA0 が実境界 — v313 教訓の再発、linear 手動再確認で是正）
- 読み順（実命令）:
  1. **2B read → this+0xC**（`push 2` / dest `esi+0xC` / `[stream+0x21C]` の vtable+4 reader 経由）＝u16 フラグ
  2. **`call 0x712A90` → this+0x10** — 実体は **ResolveLink**: u32 リンク読（size=4）→ `-1` なら 0、否则 `[stream+0x1F0]` リンクテーブルのインデクス解決 → **オブジェクトポインタ**（v349 の「count」は誤り）
  3. **`call 0x713620` → this+0x14** — 実体は **ReadString**: u32 長さ → `new(len+1)` → len バイト読 → **NUL 終端** → char*（v349 の「float[n] times」は誤り。dtor 0x6DF0D0 が `[esi+0x14]` を free するで整合）
  4. **`call 0x6CB990`（ecx=this+0x18, arg=stream）**: 0x709430 → 0x715420 → u32 読→+0x34 → `[stream+0xD8] ≥ 0x0A01006E` ゲート → 1B 読（LookAt 用サブレコード）
  5. **3回ループ**（counter=3 は arg1 スロット `[esp+0x14]` 再利用）: 各回 ResolveLink + refcount swap（InterlockedDecrement 旧/Increment 新）→ **+0x38 / +0x3C / +0x40 の3スロットのみ**（v349 の「+0x38..+0x54」は誤り）
- **file-load 経路と確定**（[stream+0x21C]=reader / [stream+0x1F0]=リンクテーブル / [stream+0xD8]=バージョン — NiStream のみ該当）[確定]

### 1.5 実行時変換側 = 0x6D3BD0 [確定]（v348§6 の裏付け）

v350_2 B1 実測: RTTI チェーン探索（`0xB3D91C` ヒットで継続）→ `push 0x40; new` → `call 0x6C3E50` → `push 0x44; new` → **`call 0x6DFB70`（ctor＝NiLookAtInterpolator 生成）** → `[ebp+0x40] → [edi+0x10]` → **`byte[ebp+0x3C] bit0 → or word[edi+0xC], 1` / `(byte>>1)&3 → +0xC bits1-2`（packed フラグ）** → `movzx eax, word[eax+0xA]`（v348§6 の +0xA u16 ＝**メモリ側ソースオブジェクトの語**）。**ストリーム読みゼロ**＝実行時変換確定。
- 0x6D42D0/0x6D4300 = RTTI チェーン型別ランタイム変換 dispatcher（0xB3FA80 → 0x6D3EB0 / **0x6D3BD0** / …へ分岐）[部分確定]（本ラウンドは LookAt 分岐のみ実測）

### 1.6 依頼①への回答（要約）

> **0x6DFDE0 は file-load 経路（NiLookAtInterpolator::Load）、0x6D3BD0 は実行時変換（同じく NiLookAtInterpolator の runtime 生成）。設問の二分法は正しいが、どちらも NiKeyframeData の Load ではない。NiKeyframeData の実 Load = 0x6E2180（= NiTransformData::Load、§2/§3）。**

---

## §2 NiKeyframeData の正体 [確定]

1. ファクトリが **"NiKeyframeData" と "NiTransformData" の両名を同一 create 0x6E1C00 に登録**（§1.1）
2. create 0x6E1C00 は **new(0x2C) + vptr 0xA7B474**（= RTTI NiTransformData、COL 走査一致でペア方向も検証済）
3. `.?AVNiKeyframeData@@` RTTI TD がバイナリに**存在しない**（独立クラスが無いことの直接証拠）
4. → **NIF ファイル内の "NiKeyframeData" ブロックは、実行時には NiTransformData クラスのオブジェクトとして読み込まれる**（Gamebryo 側の型エイリアス／同族実装）。walk.rs の `"NiTransformData" | "NiKeyframeData" => skip_keyframe_data()` 共通ハンドラは**EXE のファクトリ設計と完全一致** [確定]

---

## §3 依頼②: walk.rs 正典昇格 [確定] ＋ v349 §2 訂正表

### 3.1 EXE 実 Load 0x6E2180（NiTransformData/NiKeyframeData）の読み順（v350_6/7/8）

```
call 0x7008A0                     ; NiObject::Load base（全 family 共通 prologue）
u32 count  (4B read → cmp 0 → jbe → Translations へ)     ← walk.rs: num_rotation_keys
if count≠0: u32 type (edi へ)                              ← walk.rs: rotation_type
  flag  = byte [type + 0xB3D3F4]
  reader= [type*4 + 0xB3D0B8] → call   (キー読み)          ← walk.rs: skip_quat_keys/skip_keys
  fn2   = [type*4 + 0xB3D440] → call   (後続処理)
  count ≤ 0xFFFF ゲート（超過時 0x60D0A0 へ警告 string 0xA7B534 — 警告のみで形式差ではない）
  call 0x6E1E90                    ; type-4 euler/Order 系後処理候補
Translations KeyGroup:                                  ← walk.rs: skip_key_group("Vector3")
  u32 count → cmp0 → jbe
  if≠0: u32 interp → reader [type*4+0xB3D0A0] → call → fn2 [type*4+0xB3D428] → call
  count ≤ 0xFFFF ゲート
Scales KeyGroup:                                        ← walk.rs: skip_key_group("float")
  （同型: u32 count → interp → dispatch → …）
```

- **指令単位で walk.rs と1:1**（count→type→dispatch→KeyGroup×2 の順序・count==0 で interp 未読の分岐も一致）
- dispatch テーブル4本（**0xB3D0B8 / 0xB3D0A0 / 0xB3D440 / 0xB3D428**）は **0xB32C00 超＝.bss（ファイル非バックド・実行時初期化）** [確定] — 静的読取不可は仕様（初期化元の登録関数は未同定＝残点4）
- **main の .kf バイト精算と完全一致**（検算）:
  - `goblin blk73=116B` = u32 count(5) + u32 type + 5×20(quat key: time4+value16) + u32 Trans(0) + u32 Scale(0) = 4+4+100+4+4 = **116** ✓
  - `goblin blk2=48B` = u32 count(**0** → type 未読) + u32 Trans(2) + u32 interp + 2×16(time4+vec312) + u32 Scale(0) = 4+4+4+32+4 = **48** ✓（walk.rs の「count==0 で interpolation unread」分岐と一致）
  - `swim blk4` = type4 euler[2,2,2] + trans 87 — walk.rs の type4 分岐（Order + 3×KeyGroup(float)）と整合

### 3.2 v349 §2 訂正表（旧記述 → 新裁定・根拠付き）

| # | v349 §2 の記述 | 裁定 | 根拠 |
|---|---|---|---|
| 1 | NiKeyframeData **実 vftable = 0xA7B0D4** | **誤り** — 0xA7B0D4 = **NiLookAtInterpolator**。NiKeyframeData 独立クラス不存在 → NiTransformData(0xA7B474) へエイリアス | §1.1 ファクトリ鎖 / §1.3 RTTI 欠落 |
| 2 | **Load = slot+0x1C = 0x6DFDE0** | **帰属誤り** — NiKeyframeData/NiTransformData::Load = **0x6E2180**。0x6DFDE0 = NiLookAtInterpolator::Load | §1.2/§1.4/§3.1 |
| 3 | 読み順 **u16 kind → +0xC** | 命令は正しいが**帰属誤り** — 0x6DFDE0 の 2B 読は LookAt の u16 フラグ（+0xC bits0-2 packed；ctor 0x6DFC20..42 と 0x6D3BD0 の両書きで確認）。NiKeyframeData に kind フィールドは無い | §1.4-1 / §1.5 |
| 4 | **u32 count → +0x10（0x712A90）** | **誤り** — 0x712A90 = **ResolveLink**（u32 リンク → [stream+0x1F0] 解決 → ポインタ、-1→0）。+0x10 = リンク先オブジェクト | v350_2 A1 全文 |
| 5 | **float[n] times → +0x14（0x713620）** | **誤り** — 0x713620 = **ReadString**（u32 長さ → new(len+1) → 読 → NUL 終端）。+0x14 = char*（dtor が free で整合） | v350_2 A2 全文 |
| 6 | **8×u32 counts + refcount swap（+0x38..+0x54）** | **誤り** — **3×ResolveLink + refcount swap、3 スロットのみ（+0x38/+0x3C/+0x40、ループ counter=3）**。+0x44 は存在しない | v350_1 本体全文 |
| 7 | 値充填は **0x6D4300 → 0x6D42D0 fixup 路** | **[部分確定] 修正** — 0x6D42D0/0x6D4300 は RTTI チェーン型別**ランタイム変換** dispatcher（0x6D3EB0/0x6D3BD0 等へ分岐）であり NiKeyframeData ファイル読込とは無関係。全型の網羅は未実施 | v350_2 B2/B3 |

- **walk.rs skip_keyframe_data は本裁定をもって正典 [確定]**（EXE 0x6E2180 ＋ main .kf 実測 ＋ walk.rs の三方一致）
- 台帳 **row326/V326 への追補**は本ファイルが訂正記録（row327 に反映・訂正番号付き）

---

## §4 依頼③: .keyf family 単独 body の命令確認 [確定]

全 family Load は prologue で `call 0x7008A0`（base vtable 0xA7D61C の vt+0x1C = NiObject::Load）→ **先頭が u32 count（4B read → cmp 0 → jbe skip）**:

| クラス | vtable (RTTI) | Load | 先頭読み | 追加ゲート |
|---|---|---|---|---|
| NiTransformData / NiKeyframeData | 0xA7B474 | 0x6E2180 | u32 count → u32 type → 型 dispatch | count≤0xFFFF |
| NiFloatData | 0xA7A2EC | 0x6E3690 | u32 count → cmp0 | — |
| NiRotData | 0xA7AB3C | 0x6D8F60 | u32 count → cmp0 | — |
| NiPosData | 0xA7A624 | 0x6D9F90 | u32 count → cmp0 | — |
| NiBoolData | 0xA7C2AC | 0x6E8A30 | u32 count → cmp0 | **[stream+0xD8] ≥ 0x0A010068**（追加読あり） |
| NiKeyframeManager | 0xA7B3FC | 0x6E12C0 | **別形式**（SEH・0x715F40 呼び・0x4010003 ゲート）＝manager 本体で key-data body ではない | 範囲外・記録のみ |

- → walk.rs の **count + interp + interleaved キー**パターンは family 全般と整合 [確定]。NiBoolData のバージョンゲートは walk.rs 側の該当 arm で要確認（残点）

---

## §5 自己検証

| # | 検査 | 結果 |
|---|---|---|
| 1 | probe 一覧（%TEMP%\opencode） | v350_1(1,860行) / v350_2(516) / v350_3(29) / v350_4(657) / v350_5(117) / v350_6(200) / v350_7(255) / v350_8(97) — **全 EXIT=0**（v350_8 初回の .bss 例外はガード追加で是正） |
| 2 | probe_harness selftest | **実行不可（%TEMP% に probe_harness.py 不存在）** → v307_common.py の section map（v305 検証契約: .text/.rdata/.data の VA↔file）で代替。v121 系と同じ読取形 |
| 3 | ファクトリペア方向 | NiTransformData → 0x6E1C00 → vptr 0xA7B474 = **RTTI NiTransformData と一致**で方向検証 [確定] |
| 4 | vtable slot 実読 | [0xA7B0D4+0x1C] = 0x6DFDE0 MATCH / [0xA7B474+0x1C] = 0x6E2180（COL 走査） |
| 5 | main バイト精算の突合 | blk73=116 / blk2=48 / swim euler[2,2,2] の3例すべて §3.1 の読み順と**算術一致** |
| 6 | RTTI 欠落の網羅性 | `.?AVNi*`/`.?AVBS*` 全388名を rdata+data 全走査 → NiKeyframeData/NiLookAtInterpolator TD とも0件（独立クラス不在/未動的キャストの裏付け） |
| 7 | func_bounds 過走 | 0x6DFDE0 で再発（0xE20 誤測）→ int3 塗り+次 fn で linear 手動是正（v313 教訓どおり） |
| 8 | 旧 v121_1 COL walk の誤り | name VA 直接比較＝不一致0件を再現 → name−8 補正で22候補取得（訂正根拠を §1.3 に記録） |

---

## §6 新規残点（次束用）

| ID | 内容 | 深刻度 |
|---|---|---|
| V-350-1 | [部分確定] `0x6E1E90`（type-4 euler/Order 系後処理）の内部と Order フィールド位置 — walk.rs は version≤0x0A010000 で skip4 だが EXE 側の該当読は 0x6E1E90 内未転記 | 中 |
| V-350-2 | [未確定] dispatch テーブル4本（0xB3D0B8/0xB3D0A0/0xB3D440/0xB3D428）の初期化元（登録関数）— .bss のため静的読取不可、実行時登録元の同定が未実施 | 低 |
| V-350-3 | [部分確定] 0x6D42D0/0x6D4300 型別ランタイム変換 dispatcher の全型網羅（本ラウンドは LookAt 分岐 0x6D3EB0/0x6D3BD0 のみ実測） | 低 |
| V-350-4 | [未確定] NiLookAtInterpolator のサブレコード 0x6CB990 のバージョン 0x0A01006E 以降の追加フィールド（LookAt 範囲・本依頼外） | 低 |
| V-350-5 | [未確定] NiBoolData のバージョンゲート 0x0A010068 以降の追加読 — walk.rs の該当 arm と突合要 | 低 |
| V-350-6 | v121 の「11 クラス vtbl/TD map」は本裁定で NiKeyframeData エントリが誤りと判明 — **他エントリの再監査は未実施**（COL walk 補正の影響範囲確認） | **中** |

---

## §7 main への報告事項

1. **増分1（commit 6992263e・walk.rs 準拠 KeyframeData reader・workspace 180 passed/0 failed）は本裁定で実測検証済み** — walk.rs レイアウトは EXE 正典 [確定]。main の「実データ整合側の walk.rs 準拠」判断は正しかった
2. **増分2（nif-ffi C ABI keyframe getter + C++ shim + oracle 突合）のゲート解除**
3. v349 §2 の訂正（§3.2 の7件）は docs/GT 反映時に適用 — 反映は #47/#20 の一括バッチ（承認待ち）へ送る（本タスクは docs 編集なし = L1）
4. #122 description の typo（「v349=v349」）とタグ重複は、本 note の証跡（v350_1..8・§3.2 訂正表）を正とする

---

### 参照（本タスクの証跡）

- `%TEMP%\opencode\v350_1.py/.out`（0x6DFDE0 全文・vtable slot・callers=0・E 検索）
- `%TEMP%\opencode\v350_2.py/.out`（0x712A90=ResolveLink / 0x713620=ReadString / 0x6CB990 / 0x6D3BD0 実行時変換 / ctor 0x6DFB70・0x6DF0D0 / ファクトリ文字列）
- `%TEMP%\opencode\v350_3.py/.out`（RTTI TD 発見: NiKeyframeData TD なし / COL walk 旧法0件）
- `%TEMP%\opencode\v350_4.py/.out`（RTTI 全388名 / ファクトリ登録表 0x6C2DB0 / ctor wrapper 0x6DFD70 / vcall スキャン）
- `%TEMP%\opencode\v350_5.py/.out`（**ファクトリペア決定打**: NiKeyframeData→0x6E1C00・NiLookAtInterpolator→0x6DFD70 / 補正 COL 走査22候補）
- `%TEMP%\opencode\v350_6.py/.out`（create 0x6E1C00→vptr 0xA7B474 実読 / NiTransformData::Load 0x6E2180 冒頭 / factory name 一覧）
- `%TEMP%\opencode\v350_7.py/.out`（family各 Load 先頭読み5種 / ctor 0x7005D0）
- `%TEMP%\opencode\v350_8.py/.out`（0x6E2180 tail: KeyGroup dispatch / テーブル4本=.bss 判定）
- 読取のみ: `rust/crates/nif-core/src/walk.rs`（skip_keyframe_data L631 / dispatch L1662）・`tmp/re/n5b_samples/`（main 実測・本タスクは未再解析）
