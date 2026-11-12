# v179: 台帳 CSV 突合（未突合組合せ一括消化）【分析報告・編集なし】

- **serial**: TODO 166 / V166 配下・中優先（`v_data_relmap2.md` §3-③ a/c の未突合組合せを消化）
- **実施日**: 2026-10-28
- **方式**: 既存 CSV/md/台帳は**すべて読取専用**。突合スクリプトは `%TEMP%\opencode\v179_*.py`（新規）、出力は `%TEMP%\opencode\v179_*.txt` / `v179_diff.csv`。ESM 本体スキャンなし。
- **入力**: `v166_eh_entries.csv` / `v170_ledger_append_candidates.csv` / `v155_union.csv` / `v174_xcu_entries.csv` / `v174_xcu_candidates.csv` / `v174_r_f_noterm_spans.csv` / `v174_noterm_list.csv` / `v158_map.csv` / `v91_ini_defaults.csv` / `tmp/re/s60_io_ini_keys.txt` / `%TEMP%\opencode\func_list_new.txt` / `tmp/re/copilot_archive/scda_opcode_table_out.txt` / `docs/RE_EXE_REPORT.md` §67.6（読取）/ `plan.md`（読取）/ `v130_ini_residuals.md` / `v176_esm_gate_spec2.md`（照合基準）

---

## §1 EH×v170 の 128 行差 — 行単位同定（machine突合）

### 1.1 セット基礎

| 集合 | 行数 | unique VA | dup VA |
|---|---|---|---|
| `v166_eh_entries.csv` | 10,742 | 10,742 | 0 |
| `v170_ledger_append_candidates.csv` | 10,870 | **10,866** | **4** |
| 共通（case-insensitive VA） | — | **10,716** | — |
| EH のみ | — | 26 | — |
| v170 のみ | — | 150 | — |

### 1.2 裁定: 「v170 はどちらを採用したか」

**v170 の L7_EH 層 = 10,716 行 = EH∩v170 に完全一致**（L7_EH ⊆ EH、EH-only 26 は L7_EH 層にも載らない）。

> **v170 は v166 全数 10,742 をそのまま採用したのではなく、26 行を除外した 10,716 を L7_EH として採用し、非 EH 層を +154 行（うち 4 行は既存 VA の重複）追加して 10,870 にした。**

内訳: 10,870 = 10,716（L7_EH = EH 由来）+ 21（adopt_priority・非 EH 層）+ 129（backlog_priority・L6a_seedB Cat3）+ 4（重複行）。

### 1.3 128 行差の分解

```
10,870 − 10,742 = 128
  = (+150 unique VA: v170 のみ) + (4 重複行) − (26 unique VA: EH のみ)
```

| 側 | 件数 | 分類 | 内容 |
|---|---|---|---|
| v170 のみ・adopt_priority | **21** | L6b_I 10 + L7_E8 11 | union_layer = `L3+L4+L6b:I1..I10` 10 件 + `absent` 11 件（E8 層）。EH 走査外の新規採用 |
| v170 のみ・backlog_priority | **129** | L6a_seedB | notes に `part_of_Cat3_backlog_10261`、union_layer = `L4+L6a`。Cat3 mass 10,261 束の一部 |
| v170 のみ・重複行 | **4** | L7_EH adopt × L6a_seedB backlog の二重計上 | VA = `009B055A` / `009B319A` / `009B36FF` / `009B7DFF`。adopt 行に `dup_cat3_backlog`、backlog 行に `dup=L7_EH` と**自己申告付き** |
| EH のみ | **25** | kind=`xcu_stub` | `009D7E40`–`009D8010`（init-array ref @A283F0–A28474、notes `union:L3+L4`）。全件 v174 XCU の fn_va として `v155_union` 収録済み → v170 は XCU 裁定済みとして EH 層から除外 |
| EH のみ | **1** | kind=`unref` | `009AACE8`（`region_start;no .rdata EH ref; not a pushed thunk;union:absent`） |

### 1.4 共通行の矛盾検査（10,716 VA）

| 検査 | 件数 | 判定 |
|---|---|---|
| host_guess 表記不一致 | 351 | **347 件は表示形式差**（EH の `FUN_x(+N more)` を v170 が先頭候補のみ記載）。実矛盾は残 4 件 = 上記 dup VA の backlog 側（host 空欄） |
| kind 不一致 | 4 | 同じ 4 dup VA（EH=`seh_thunk`/`unwind` vs backlog=`region_start_seedB`） |

→ **実質矛盾 0**（4 件は自己申告 dup で説明済み）。

### 1.5 差分 CSV

`%TEMP%\opencode\v179_diff.csv`（176 データ行 = eh_only 26 + v170_only 150、VA・side・layer/kind・status・host・reason 列付き）。

---

## §2 v91×s60 キー差 20 の行レベル対応表

### 2.1 セット基礎（再突合）

| 集合 | 件数 |
|---|---|
| v91 unique (key,section) | 635 |
| s60 raw 行 | 615 |
| s60 unique (case-insens) / セクション | 611 / 36 |
| 突合（case-insens） | 604 |
| v91 のみ / s60 のみ(unique) | 31 / 7 |
| 重複行（B-4） | `v:SW` ×5 → 追加 4 行 |

**算術**: `31 − (7 + 4) = 20` ＝ `21+4+6−4−2−1−4` ✓（v130 §6.2 と一致）

### 2.2 分類（機械再分類）と口径差の同定

機械分類（case-insensitive セクション在否 + エイリアス末尾一致）は **A-1=19 / A-2=4 / A-3=8**。v130 の 21/4/6 と**2 キーだけ束ねが異なる**:

| キー | v130 分類 | 本突合分類 | 理由 |
|---|---|---|---|
| `bUseBackgroundPathing:PathFinding` | A-1（セクション外） | **A-3（純漏れ）** | s60 の 36 セクションに `Pathfinding`(7 キー) が**実在**（case 差のみ）。セクションは走査済み・キーのみ漏れ |
| `iHavokSkipFrameCountTEST:Havok` | A-1（セクション外） | **A-3（純漏れ）** | 同上。s60 に `HAVOK`(23 キー) 実在 |

→ 合計 31・差 20・検算式は不変。**A-1 厳密値は 19**（v130 §6.2 の「PathFinding −1 / Havok −1」は case-sensitive セクション名での束ね）。

### 2.3 対応表

**A-1: セクションが s60 に無い（19 = bLightAttenuation 12 + MAIN 5 + OPENMP 2）**

| キー | Section | v91 VA |
|---|---|---|
| bOutQuadInLin / bUseConstant / bUseLinear / bUseQuadratic | bLightAttenuation | 0x00B08180 / 0x00B08138 / 0x00B08140 / 0x00B08148 |
| fConstantValue / fFlickerMovement / fLinearRadiusMult / fLinearValue | bLightAttenuation | 0x00B08178 / 0x00B08150 / 0x00B08188 / 0x00B08168 |
| fQuadraticRadiusMult / fQuadraticValue / uLinearMethod / uQuadraticMethod | bLightAttenuation | 0x00B08190 / 0x00B08170 / 0x00B08158 / 0x00B08160 |
| fLowPerfCombatantVoiceDistance / fQuestScriptDelayTime / iDetectionHighNumPicks / iLastHDRSetting / sBasePath | MAIN | 0x00B14BAC / 0x00B09E28 / 0x00B14904 / 0x00B06F54 / 0x00B0472C |
| iOpenMPLevel / iThreads | OPENMP | 0x00B0524C / 0x00B05254 |

**A-2 ⇔ B-1: 境界汚染エイリアス 4 対（相殺）** — v91 側が正（EXE リテラル実在 = v130 §6.3 証跡）。s60 VA はリテラル所在、v91 VA はレジストリノード所在で**別アドレス空間**（VA 不一致は正常）:

| v91（正） | v91 VA | s60（汚染） | s60 VA | 汚染源 |
|---|---|---|---|---|
| bDialogueSubtitles:GamePlay | 0x00B13200 | zDbDialogueSubtitles | 00A68C0E | float 1000.0f 末尾 `zD` |
| bDisplayLODBuildings:LOD | 0x00B09AE8 | HbDisplayLODBuildings | 00A47B23 | 直前バイト `H` |
| bUseMultiThreadedTrees:BackgroundLoad | 0x00B0523C | AbUseMultiThreadedTrees | 00A379CF | float 100.0f 末尾 `A` |
| fLandFriction:Landscape | 0x00B114A4 | AfLandFriction | 00A5611F | `47 4E 92 41` 末尾 `A` |

**A-3: s60 の純漏れ（8 = v130 の 6 + 上記口径差 2）**

| キー | Section | v91 VA | 備考 |
|---|---|---|---|
| bSoundEnabled / fHeadroomdB / fMinSoundVel | Audio | 0x00B16178 / 0x00B23C58 / 0x00B23C50 | v130 A-3 |
| bInstantLevelUp | GamePlay | 0x00B14E88 | v130 A-3 |
| bUse Joystick | Controls | 0x00B02C2C | v130 A-3（キー名に空白） |
| fGlobalTimeMultiplier | General | 0x00B06704 | v130 A-3 |
| bUseBackgroundPathing | PathFinding | 0x00B15800 | **口径差**（v130 は A-1） |
| iHavokSkipFrameCountTEST | Havok | 0x00B14E44 | **口径差**（v130 は A-1） |

**B-2: 実行時（動的）登録 2**（.bss ノード = v91 静的列挙に不在）: `fAlpha:Water` @00A3F43C / `uSurfaceTextureSize:Water` @00A3F4C4
**B-3: 偽キー 1**: `v:SW` @0079E1D2 ほか計 5 行（`.text` 内バイト走査誤検出）
**B-4: 重複 4 行**: 上記 `v:SW` ×5 のうち 4 行（raw 615 − unique 611）

---

## §3 統合 CSV 相互一意性検査

### 3.1 各 CSV の一意性（内部）

| CSV | 行数 | 対象 VA 列 | unique | 重複 | 判定 |
|---|---|---|---|---|---|
| v155_union | 31,905 | VA | 31,905 | 0 | ✅ |
| v166_eh_entries | 10,742 | VA | 10,742 | 0 | ✅ |
| v170_ledger_append | 10,870 | VA | 10,866 | **4**（自己申告 dup） | 注記付き ✅ |
| v174_xcu_entries | 7,402 | slot_va / fn_va | 7,402 / 7,380 | 0 / 0 | ✅ |
| v174_xcu_candidates | 1,199 | fn_va | 1,199 | 0 | ✅ |
| v174_r_f_noterm_spans | 1,376 | span_va | 1,376 | 0 | ✅ |
| v174_noterm_list | 112 | int3_va | 112 | 0 | ✅ |
| v158_map | 690 | target_va | **54** | fan-in（例 0x41E920×14 = thunk 先共有） | 想定通り ✅ |

### 3.2 跨 CSV 関係・フラグ再検証

| 検査 | 結果 | 判定 |
|---|---|---|
| v174_xcu_entries フラグ（**fn_va** 基準）`in_func_list` / `in_union` / `in_v170` vs 現行 func_list_new(21,597) / u155 / v170 | **mismatch 0 / 0 / 0** | ✅ 完全整合 |
| v174_r_f_noterm_spans フラグ（**cc_va** 基準）`in_union` / `in_v170` | **mismatch 0 / 0** | ✅ 完全整合 |
| xcu_candidates 1,199 = fn_va ∉ (func_list ∪ u155 ∪ v170) の導出集合 | **集合一致**（7,380 − 6,181 = 1,199） | ✅ 内部整合 |
| func_list_new 21,597 ⊆ v155_union | **21,597 / 21,597** | ✅ |
| noterm span_va ∈ u155 | **1,376 / 1,376（100%）** — span 開始は台帳登録済み。cc_va（int3 位置）∈ u155/v170 = 0 | 想定通り ✅ |
| xcu fn_va ∈ u155 / v170 / EH | 6,181 / 1 / 25 | EH の 25 = §1.3 の xcu_stub 26−1 と完全一致 |
| EH ∩ u155 / v170 ∩ u155 | 668 / 782 | EH/v170 は u155 の真部分集合でなく交差集合（`union:absent` マーカーと整合） |
| v158_map target_va ∈ u155 | 25 / 54 unique（∈ EH/v170 = 0） | 別軸（opid→impl）。残 29 は台帳外 thunk 先 = relmap §3-③b の対象 |

**矛盾件数: 0**（表示形式差 347・自己申告 dup 4 は §1.4 で説明済み。v174 フラグは全項目 mismatch 0）。

---

## §4 s* 系成果物の opid 口径突合（scda 表 × §67.6 × plan.md × v176 裁定）

### 4.1 機械検証結果

| 項目 | 検証値 | 期待値（v164/v176） | 判定 |
|---|---|---|---|
| `scda_opcode_table_out.txt` distinct | 141 | 141 | ✅ |
| 同 Σ(count) | **63,235** | 63,235（tail 除外） | ✅ |
| 同 0x0010 / 0x0011 / 0x001D | 4,344 / 2,023 / 2,321 | — | — |
| §67.6 テーブル行数 | 480（unique opid 480） | 480 | ✅ |
| §67.6 数値 Freq 行 / `-` 行 | **112 / 368** | 112 / 368 | ✅ |
| §67.6 112 正頻度 vs scda 表（tail-excl） | **mismatch 0（112/112）** | mismatch 0 | ✅ |
| `-` 行で scda 表に在るもの | 0 | 0 | ✅ |
| scda 141 のうち §67.6 に**行が無い** opid | **29**（112 + 29 = 141） | — | 新事実（§4.3） |

### 4.2 v176 裁定（N1: tail incl GT・2 行更新）との整合

```
tail 数   = 2,393(SCPT) − 72(prologue-only) = 2,321
0x0011 incl = 2,023 + 2,321 = 4,344   ✅ (v176 target)
0x001D incl = 2,321 +  72   = 2,393   ✅ (v176 target)
Σ incl      = 63,235 + 2,321 + 72 = 65,628 = plan.md L507  ✅
```

- plan.md: 63,235（excl）@ L454 / L884 / L916、65,628（incl）@ L507 / L569 / L570 / L815 / L1007 / L1091 / L1122 — v176 §8 の grep 特定と一致。
- `RE_EXE_REPORT.md` 65,628 @ L10126 / L12105 — 一致。
- 台帳 §67.6 の現行 112 行はすべて tail-excl 値（mismatch 0）= **incl GT への更新前状態**。v176 の「110 行不変・2 行（:10932 `0x001D`=2,321 / :10933 `0x0011`=2,023）のみ更新」裁定と完全整合。N8 の 12 ファイル 16 箇所は F1 バッチ待ち（本タスクでは未編集）。

### 4.3 補足

- §67.6 は EXE 側 FunctionID 台帳（480 opid）、scda 表は ESM SCPT 実測（141 opid）。**112 + 29 = 141** は「EXE 台帳に無い 29 opid が SCPT 実測に存在」を意味し、v176 §7 の「SCPT 外 15 種（全体 156 − 141）」とは**逆方向の残差**（台帳未登録 opcode 群）。V176-f（§67.9 理由未特定 15 opid との集合交差）と合わせ、台帳 join 時の確認対象。

---

## §5 成果物

| パス | 内容 |
|---|---|
| `C:\Users\hiroki.kogarumai\Oblivion_Android\tmp\re\v179_csv_crosscheck.md` | 本報告（新規） |
| `%TEMP%\opencode\v179_diff.csv` | 128 行差の全 VA（176 行 + ヘッダ、reason 列付き） |
| `%TEMP%\opencode\v179_t1_t3.py` / `v179_t2_s60.py` / `v179_t4_opid.py` | 突合スクリプト（再実行可能） |
| `%TEMP%\opencode\v179_t1_t3_out.txt` / `v179_t2_out.txt` / `v179_t4_out.txt` | 生出力証跡 |

## §6 新規残点

| # | 内容 | 優先 |
|---|---|---|
| R1 | v170 の dup 4 VA（`009B055A` 等）は自己申告済みだが、台帳反映バッチ（relmap §4-#1）で **backlog 側を drop するか二重登録するか**の裁定が必要 | 中（反映バッチ時） |
| R2 | v130 §6.2 の A-1=21 は case-sensitive セクション束ね。厳密値 19+口径差 2（`bUseBackgroundPathing` / `iHavokSkipFrameCountTEST`）を v130/v91 記述へ反映するかは人間判断（本タスクは編集禁止のため記録のみ） | 低 |
| R3 | scda 141 のうち §67.6 に無い 29 opid（台帳未登録の SCPT 実測 opcode 群）— V176-f と併せ台帳 join 副産物で確認 | 低 |
| R4 | v158_map の target_va 54 unique 中 29 が u155 非収録（台帳外 thunk 先）— relmap §3-③b（v158×GT 突合）のスコープで扱われ想定 | 低 |
| R5 | xcu_entries の `in_v170=1` の fn_va 1 件は個体特定未実施（x 系 CSV 上の唯一の v170 交点） | 低 |

---
*v179（分析専任・L1 報告のみ）。docs/TODO/台帳/既存 CSV/plan は一切編集していない。*
