# V79: `absorbed_suspect`（cc前置≥2 / 319件）の V70 デコンパイル追跡

- 日付: 2026-10-25
- 対象: Oblivion.exe (Steam v1.2.0416, ImageBase 0x400000) のギャップ362行 = 357ユニークVA
- 入力:
  - `tmp\re\v5_ghidra_gaps_report.md` §3.1 全ギャップリスト（cc前置バケット / 判定 列）
  - `tmp\re\v10_impl_decompile_join.csv`（gap_kind / cc_prefix 列、header確認済）
  - `%TEMP%\opencode\v70_force_create.csv`（362行, va/pre_status/entry/name/decomp_ok/decomp_chars）
  - `%TEMP%\opencode\v70_decomp_dump.txt`（228,917 bytes, ブロック `/* ===== VA entry name ===== */` 362件, 全件 decomp_ok）
- 生成物（本レポート以外はすべて %TEMP%\opencode\ 配下の中間物）:
  - `v79_classify.py`（初版）, `v79_classify2.py`（最終版）, `v79_classified.csv`（357行）, `v79_summary.json`, `v79_bodies.json`, `v79_c23_list.md`

## §1 手法（判定規則と閾値）

### 1.1 `absorbed_suspect`（cc≥2, 319件）の再現

V5 報告 §3.1 の `CC前置` 列を正として再現した（V5 表の判定と完全一致を確認: cc≥2 ⇔ 判定「欠落」= 319行 / cc<2 ⇔ 判定「吸収」= 43行、全362行）。

- `CC前置` 列の値: `(0)`=24行, `(1)`=19行, `cc×2`=43行, `cc×3`=16, `cc×4`=16, `cc×5`=15, `cc×6`=19, `cc×7`=37, `cc×8`=173（`cc×8` は「≥8」の飽和表記と解される）
- **タスク定義の `absorbed_suspect` = cc前置≥2 = 319行**（＝V5 の判定「欠落」）。本レポートではこの定義を採用する。タスク本文の仮説（ICF / 分割解析による「吸収された薄片」）はこの319件に対して検証する。
- 319行のうち重複VAが5件含まれる（`0x005007d0`, `0x00500890`, `0x00503e10`, `0x00503e90`, `0x0050d020` — gap_vas.txt の362行=357ユニーク+重複5の正体）。**重複5件は全て cc≥2側**のため、ユニークVAベースでは **cc≥2 = 314 / cc<2 = 43 / 計 357**。
- 注意（データ解釈の罠）: `v10_impl_decompile_join.csv` の `cc_prefix` 列は **yes/no の2値**であり、その yes 260件は V5 の **cc≥4 集合**と一致する（V61 記載どおり）。`gap_kind=absorbed_suspect`（102件）= cc≤3 の残りで、**V5 意味（43件）ともタスク定義（319件）とも異なる**ため、本レポートでは v10 の gap_kind 列は参照せず、V5 表の cc前置バケットのみを用いた。

### 1.2 指標（dump 1ブロック毎）

- `L`: dump 本文長（署名行含む、CRLF の `\r` 除去後）。V70 CSV の `decomp_chars` と数字符の差あり（改行カウントの扱い）。判定は `L` に統一。全体 min=63 / med=434 / max=4222。
- `exec文`: 署名行・空行・変数宣言行を除外した実行文（`;` 数）。
- `n_call`: `FUN_xxxxxxxx(` / `func_0x...(` の出現数。
- `has_cf`: `if/else/while/for/switch/case/goto` の有無 — **文字列リテラル除去後**に判定（初期版は文言 "…switch to COMBAT INFO page." 中の `switch` で誤検出し、0x00500770 等を誤分類した。修正済）。
- `exact_dup`: ホワイトスペース正規化後の本文完全一致グループ。
- `skel`: アドレス（`FUN_*` / `func_0x*` / 16進定数）を `@` に潰した骨格。骨格一致グループ = 「同じ型の定型コード族」。

### 1.3 判定規則（優先順、閾値はタスク指定の ~120 / ~200 字を軸に exec 文で補正）

| 優先 | 条件 | クラス | 理由ラベル |
|---|---|---|---|
| 1 | L<120 かつ 呼び出し0 かつ CF無 かつ exec文≤2 | **3 データ/無意味** | trivial-garbage |
| 2 | L<120 | **2 薄片/断片** | tiny |
| 3 | CF無 かつ 呼び出し≤2 かつ exec文≤3（署名長に依存せず） | **2** | call+return-forwarder |
| 4 | 骨格重複≥2 かつ CF無 かつ L<200 かつ 呼び出し≤2 | **2** | skeleton-dup-thin |
| 5 | L≥200 かつ (CF有 or exec文≥4) | **1 完全な独立関数** | full-body |
| 6 | CF有 かつ exec文≥3 | **1** | small-with-cf |
| 7 | exec文≥4 かつ L≥150 | **1** | straightline |
| 8 | それ以外 | **2** | residual-thin |

### 1.4 検証

- `probe_harness.py selftest` → **PASS**
- `ctx.rd(0x504050, 8)` → `518b4c24188b5424`（期待値と一致 ✓。V5 表の OnHit prologue 先頭8B とも一致）
- 分類本体は harness 非依存の raw CSV / dump パースのみ（`v79_classify2.py`）。

## §2 クラス集計 + クロス表

### 2.1 全体集計（357ユニークVA）

| クラス | 件数 | 比率 | L 範囲 (min/med/max) |
|---|---|---|---|
| 1 完全な独立関数 | **256** | 71.7% | 165 / 574 / 4222 |
| 2 薄片/断片 | **98** | 27.5% | 69 / 182 / 374 |
| 3 データ/無意味 | **3** | 0.8% | 63 / 84 / 90 |

class2 の内訳（reason）: call+return-forwarder 73 / tiny(L<120) 22 / residual-thin 3。
class3（3件）: 0x00500820, 0x00501a60, 0x005087a0。

### 2.2 クロス表: cc前置≥2 vs <2 × クラス（ユニークVA）

| | class1 | class2 | class3 | 計 | class2+3 率 |
|---|---|---|---|---|---|
| **cc≥2（absorbed_suspect, 319行→314U）** | 223 (71.0%) | 89 (28.3%) | 2 (0.6%) | 314 | **29.0%** |
| **cc<2（V5「吸収」, 43件）** | 33 (76.7%) | 9 (20.9%) | 1 (2.3%) | 43 | **23.3%** |
| 計 | 256 | 98 | 3 | 357 | 28.3% |

- χ²適合度（2×2, class2+3 vs class1）= **0.611, p ≈ 0.43 → 有意な相関なし**。cc≥2 であっても薄片化する傾向は cc<2 とほぼ同じ（むしろ比率はわずかに低い側と高い側が交錯する水準）。
- class2+3 の理由内訳: cc≥2 = forwarder 68 / tiny 19 / residual-thin 2 / garbage 2（計91）。cc<2 = forwarder 5 / tiny 3 / residual-thin 1 / garbage 1（計10）。

### 2.3 cc前置バケット別 × クラス

| ccバケット | 件数 | class1 | class2 | class3 |
|---|---|---|---|---|
| cc=0 | 24 | 19 | 5 | 0 |
| cc=1 | 19 | 14 | 4 | 1 |
| cc≥2 | 42 | 39 | 2 | 1 |
| cc≥3 | 15 | 12 | 3 | 0 |
| cc≥4 | 15 | 9 | 6 | 0 |
| cc≥5 | 15 | 8 | 7 | 0 |
| cc≥6 | 19 | 15 | 4 | 0 |
| cc≥7 | 37 | 21 | 15 | 1 |
| cc≥8 | 171 | 119 | 52 | 0 |

（ユニーク化で cc≥2/≥3/≥4 が各−1、cc≥8 が−2。合計357。）

### 2.4 補助観測

- **exact_dup（本文完全一致）= 0組**。357本の本文は全て互いに異なる → **MSVC /OPT:ICF が折り畳んだ identical コピーの直接証拠はギャップ集合内に存在しない**。
- **骨格重複 = 13組 / 115件**。最大族は **47件**（すべて class2）で骨格は `func_0x@(param_3,0,0,in_stack_0000001c); return;` 型の単一呼び出しフォワーダ。ただし**呼出先 helper は一件ずつ全て別**（0x004f4640 / 0x004f46e0 / 0x004f6010 …）→ 同一コードの折り畳みではなく、オペコードハンドラの定型コード生成パターン。次いで20件族（class1）、12件（class1×2組）、10件（class2×2組）等。
- **空間的集中**: class2+3 の101件は全て **0x00500770–0x0050d8c0** に収まる（ギャップ全体は 0x00500450–0x00514500）。薄片はスクリプト・オペコードハンドラ領域（0x50xxxx帯）の現象であり、EXE全域に散発する「誤分割データ」の挙動ではない。

## §3 クラス2/3 の全VAリスト（101件、1行1件、cc 付き）

- 列: VA | name | class | L | cc前置（V5表記） | reason
- `(0)`/`(1)` = cc前置 0/1（**cc<2 = V5「吸収」側**）、`cc ×N` = cc前置 ≥N（**cc≥2 = absorbed_suspect 側**）
- class2 = 98件、class3 = 3件（うち cc≥2 が 91件、cc<2 が 10件）

| VA | name | cls | L | cc前置 | reason |
|---|---|---|---|---|---|
| 0x00500770 | FUN_00500770 | 2 | 127 | cc ×8 | call+return-forwarder |
| 0x00500780 | FUN_00500780 | 2 | 116 | (0) | tiny |
| 0x005007A0 | FUN_005007a0 | 2 | 110 | cc ×4 | tiny |
| 0x005007C0 | FUN_005007c0 | 2 | 72 | cc ×4 | tiny |
| 0x00500820 | FUN_00500820 | **3** | 63 | (1) | trivial-garbage |
| 0x00500C60 | FUN_00500c60 | 2 | 112 | cc ×3 | tiny |
| 0x00500E30 | FUN_00500e30 | 2 | 72 | cc ×7 | tiny |
| 0x00500E40 | FUN_00500e40 | 2 | 114 | cc ×2 | tiny |
| 0x00500F60 | FUN_00500f60 | 2 | 70 | cc ×8 | tiny |
| 0x005013E0 | FUN_005013e0 | 2 | 69 | (1) | tiny |
| 0x00501650 | FUN_00501650 | 2 | 72 | cc ×4 | tiny |
| 0x00501660 | FUN_00501660 | 2 | 72 | cc ×8 | tiny |
| 0x005019E0 | FUN_005019e0 | 2 | 182 | cc ×8 | call+return-forwarder |
| 0x00501A60 | FUN_00501a60 | **3** | 90 | cc ×7 | trivial-garbage |
| 0x00501C00 | FUN_00501c00 | 2 | 124 | cc ×8 | call+return-forwarder |
| 0x00501C20 | FUN_00501c20 | 2 | 173 | cc ×8 | call+return-forwarder |
| 0x00501E20 | FUN_00501e20 | 2 | 182 | (0) | call+return-forwarder |
| 0x005022D0 | FUN_005022d0 | 2 | 207 | cc ×8 | call+return-forwarder |
| 0x00502370 | FUN_00502370 | 2 | 236 | (1) | call+return-forwarder |
| 0x00502390 | FUN_00502390 | 2 | 236 | cc ×6 | call+return-forwarder |
| 0x005023B0 | FUN_005023b0 | 2 | 236 | cc ×6 | call+return-forwarder |
| 0x00502630 | FUN_00502630 | 2 | 182 | (1) | call+return-forwarder |
| 0x00502650 | FUN_00502650 | 2 | 182 | cc ×8 | call+return-forwarder |
| 0x00502670 | FUN_00502670 | 2 | 182 | cc ×8 | call+return-forwarder |
| 0x005027B0 | FUN_005027b0 | 2 | 190 | cc ×7 | call+return-forwarder |
| 0x005027D0 | FUN_005027d0 | 2 | 190 | cc ×5 | call+return-forwarder |
| 0x005027F0 | FUN_005027f0 | 2 | 190 | cc ×5 | call+return-forwarder |
| 0x00502870 | FUN_00502870 | 2 | 182 | cc ×7 | call+return-forwarder |
| 0x005029C0 | FUN_005029c0 | 2 | 182 | cc ×7 | call+return-forwarder |
| 0x005029E0 | FUN_005029e0 | 2 | 182 | cc ×8 | call+return-forwarder |
| 0x00502A00 | FUN_00502a00 | 2 | 182 | cc ×8 | call+return-forwarder |
| 0x00502A20 | FUN_00502a20 | 2 | 182 | cc ×8 | call+return-forwarder |
| 0x00502B00 | FUN_00502b00 | 2 | 179 | cc ×7 | call+return-forwarder |
| 0x00502E90 | FUN_00502e90 | 2 | 182 | cc ×7 | call+return-forwarder |
| 0x00502EB0 | FUN_00502eb0 | 2 | 182 | cc ×8 | call+return-forwarder |
| 0x00502ED0 | FUN_00502ed0 | 2 | 182 | cc ×8 | call+return-forwarder |
| 0x00502EF0 | FUN_00502ef0 | 2 | 182 | cc ×8 | call+return-forwarder |
| 0x00502F10 | FUN_00502f10 | 2 | 182 | cc ×8 | call+return-forwarder |
| 0x00502F30 | FUN_00502f30 | 2 | 182 | cc ×8 | call+return-forwarder |
| 0x00502F50 | FUN_00502f50 | 2 | 182 | cc ×8 | call+return-forwarder |
| 0x00502FD0 | FUN_00502fd0 | 2 | 182 | cc ×7 | call+return-forwarder |
| 0x00502FF0 | FUN_00502ff0 | 2 | 182 | cc ×8 | call+return-forwarder |
| 0x005033F0 | FUN_005033f0 | 2 | 182 | cc ×8 | call+return-forwarder |
| 0x00503710 | FUN_00503710 | 2 | 182 | cc ×8 | call+return-forwarder |
| 0x00503730 | FUN_00503730 | 2 | 182 | cc ×8 | call+return-forwarder |
| 0x005039F0 | FUN_005039f0 | 2 | 182 | cc ×7 | call+return-forwarder |
| 0x00503A10 | FUN_00503a10 | 2 | 182 | cc ×8 | call+return-forwarder |
| 0x00503A30 | FUN_00503a30 | 2 | 182 | cc ×8 | call+return-forwarder |
| 0x00503C40 | FUN_00503c40 | 2 | 182 | cc ×8 | call+return-forwarder |
| 0x00504010 | FUN_00504010 | 2 | 374 | cc ×8 | call+return-forwarder |
| 0x005047E0 | FUN_005047e0 | 2 | 70 | cc ×8 | tiny |
| 0x005047F0 | FUN_005047f0 | 2 | 70 | (0) | tiny |
| 0x00504800 | FUN_00504800 | 2 | 132 | (0) | call+return-forwarder |
| 0x005051D0 | FUN_005051d0 | 2 | 124 | cc ×5 | call+return-forwarder |
| 0x005051F0 | FUN_005051f0 | 2 | 124 | cc ×8 | call+return-forwarder |
| 0x00505210 | FUN_00505210 | 2 | 124 | cc ×8 | call+return-forwarder |
| 0x00505230 | FUN_00505230 | 2 | 124 | cc ×8 | call+return-forwarder |
| 0x00505460 | FUN_00505460 | 2 | 190 | cc ×4 | call+return-forwarder |
| 0x00505660 | FUN_00505660 | 2 | 190 | cc ×5 | call+return-forwarder |
| 0x005056F0 | FUN_005056f0 | 2 | 104 | cc ×8 | tiny |
| 0x00505850 | FUN_00505850 | 2 | 104 | cc ×8 | tiny |
| 0x00505870 | FUN_00505870 | 2 | 107 | cc ×8 | tiny |
| 0x00505890 | FUN_00505890 | 2 | 107 | cc ×8 | tiny |
| 0x005058B0 | FUN_005058b0 | 2 | 104 | cc ×8 | tiny |
| 0x00505930 | FUN_00505930 | 2 | 182 | cc ×7 | call+return-forwarder |
| 0x00505950 | FUN_00505950 | 2 | 182 | cc ×8 | call+return-forwarder |
| 0x00505970 | FUN_00505970 | 2 | 182 | cc ×8 | call+return-forwarder |
| 0x005059F0 | FUN_005059f0 | 2 | 182 | cc ×7 | call+return-forwarder |
| 0x00505A10 | FUN_00505a10 | 2 | 182 | cc ×8 | call+return-forwarder |
| 0x00505B50 | FUN_00505b50 | 2 | 182 | cc ×6 | call+return-forwarder |
| 0x00505C00 | FUN_00505c00 | 2 | 182 | cc ×6 | call+return-forwarder |
| 0x00505C20 | FUN_00505c20 | 2 | 182 | cc ×8 | call+return-forwarder |
| 0x00505C40 | FUN_00505c40 | 2 | 182 | cc ×8 | call+return-forwarder |
| 0x00505C60 | FUN_00505c60 | 2 | 182 | cc ×8 | call+return-forwarder |
| 0x00505D10 | FUN_00505d10 | 2 | 182 | cc ×8 | call+return-forwarder |
| 0x00505D30 | FUN_00505d30 | 2 | 182 | cc ×8 | call+return-forwarder |
| 0x00505D50 | FUN_00505d50 | 2 | 182 | cc ×8 | call+return-forwarder |
| 0x00505D70 | FUN_00505d70 | 2 | 182 | cc ×8 | call+return-forwarder |
| 0x00505D90 | FUN_00505d90 | 2 | 182 | cc ×8 | call+return-forwarder |
| 0x00505DB0 | FUN_00505db0 | 2 | 179 | cc ×8 | call+return-forwarder |
| 0x00505EE0 | FUN_00505ee0 | 2 | 190 | cc ×3 | call+return-forwarder |
| 0x00505F00 | FUN_00505f00 | 2 | 190 | cc ×7 | call+return-forwarder |
| 0x00505F20 | FUN_00505f20 | 2 | 190 | cc ×7 | call+return-forwarder |
| 0x00505F40 | FUN_00505f40 | 2 | 190 | cc ×7 | call+return-forwarder |
| 0x00505FC0 | FUN_00505fc0 | 2 | 190 | cc ×5 | call+return-forwarder |
| 0x005062B0 | FUN_005062b0 | 2 | 156 | (0) | residual-thin |
| 0x005062D0 | FUN_005062d0 | 2 | 156 | cc ×8 | residual-thin |
| 0x005062F0 | FUN_005062f0 | 2 | 190 | cc ×8 | call+return-forwarder |
| 0x00506370 | FUN_00506370 | 2 | 182 | cc ×5 | call+return-forwarder |
| 0x00506390 | FUN_00506390 | 2 | 182 | cc ×8 | call+return-forwarder |
| 0x005063B0 | FUN_005063b0 | 2 | 182 | cc ×8 | call+return-forwarder |
| 0x005064A0 | FUN_005064a0 | 2 | 72 | cc ×2 | tiny |
| 0x00506AD0 | FUN_00506ad0 | 2 | 110 | cc ×5 | tiny |
| 0x00506DC0 | FUN_00506dc0 | 2 | 182 | cc ×7 | call+return-forwarder |
| 0x005077E0 | FUN_005077e0 | 2 | 104 | cc ×3 | tiny |
| 0x00507B90 | FUN_00507b90 | 2 | 133 | (1) | call+return-forwarder |
| 0x00508180 | FUN_00508180 | 2 | 180 | cc ×4 | residual-thin |
| 0x005087A0 | FUN_005087a0 | **3** | 84 | cc ×2 | trivial-garbage |
| 0x0050A3E0 | FUN_0050a3e0 | 2 | 107 | cc ×8 | tiny |
| 0x0050BAA0 | FUN_0050baa0 | 2 | 221 | cc ×7 | call+return-forwarder |
| 0x0050D8C0 | FUN_0050d8c0 | 2 | 101 | cc ×4 | tiny |

## §4 代表例10（dump 引用）

引用は空行を圧縮して掲げる（本文は v70_decomp_dump.txt のまま）。

### 例1: 0x00504050（OnHit）— class1 full-body / cc≥2(×2) / L=573
```c
undefined1
FUN_00504050(undefined4 param_1,undefined4 param_2,int param_3,undefined4 param_4,undefined4 param_5
            ,int param_6,undefined8 *param_7,undefined4 param_8)
{
  char cVar1;
  undefined4 uStack_4;
  uStack_4 = 0;
  cVar1 = FUN_004fae80(param_1,param_2,param_8,param_3,param_4,param_5,param_6,&uStack_4);
  if (cVar1 == '\0') {
    return 0;
  }
  *param_7 = 0;
  if ((param_3 != 0) && (param_6 != 0)) {
    cVar1 = FUN_004fb5f0(uStack_4,0x80);
    if (cVar1 != '\0') {
      *param_7 = 0x3ff0000000000000;
    }
  }
  return 1;
}
```
→ ゲート関数 FUN_004fae80 呼び出し + 入子条件分岐。完全な独立関数。（V66 のゲート0x004fae80 と同一の関数を呼ぶ）

### 例2: 0x00503ce0 — class1 small-with-cf / cc≥2(×8) / L=183
```c
undefined1 FUN_00503ce0(void)
{
  if (DAT_00b43070 != '\0') {
    DAT_00b431e4 = DAT_00b431e4 == '\0';
    return 1;
  }
  DAT_00b4610c = DAT_00b4610c == '\0';
  return 1;
}
```
→ L<200 だが実分岐あり。シェーダ系フラグの二択トグルで、これ自体が完結した小関数。

### 例3: 0x00506e90 — class1 straightline / cc≥2(×6) / L=182
```c
undefined1 FUN_00506e90(void)
{
  FUN_00579b60("Clearing Facegen Model Data");
  FUN_00442630(1,0);
  FUN_0043fc20(0);
  FUN_00404c60(1);
  func_0x0054fe90();
  return 1;
}
```
→ コンソールコマンド（Facegen データ削除）の実処理本。分岐なしでも5呼び出しの直列実装で独立関数。

### 例4: 0x005022d0（GameMode）— class2 call+return-forwarder / cc≥2(×8) / L=207
```c
undefined4 __thiscall FUN_005022d0(undefined4 param_1)
{
  char cVar1;
  double *in_stack_0000001c;
  cVar1 = FUN_00578f60(param_1);
  *in_stack_0000001c = (double)(cVar1 == '\0');
  return 1;
}
```
→ **L≥200 でも薄片**の例。実行文は3個（1呼び出し+変換+return）で分岐なし。サイズ閾値単独では class1 誤判定になる（署名/宣言が長さを稼ぐ）ため exec 文補正が必要だった。

### 例5: 0x00502370 — class2 call+return-forwarder / **cc<2 (1)** / L=236
```c
void FUN_00502370(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,
                 undefined4 param_5,undefined4 param_6,undefined4 param_7)
{
  func_0x004f60e0(param_3,param_6,0,param_7);
  return;
}
```
→ 7引数の完全転送フォワーダ。本文は実質1呼び出し。ただし 0x00502390/0x005023b0 は同じ骨格で**別 helper（0x004f4760 / 0x004f47b0）**を呼ぶ → 折り畳みではなく定型ハンドラ族。

### 例6: 0x005013e0 — class2 tiny / **cc<2 (1)** / L=69（全体最小）
```c
undefined1 FUN_005013e0(void)
{
  FUN_006669f0();
  return 1;
}
```
→ 最小の「単一呼び出し+return」。thunk 的だが完全に自己完結。

### 例7: 0x0050d8c0 — class2 tiny / cc≥2(×4) / L=101
```c
void FUN_0050d8c0(void)
{
  if (DAT_00b0703c != '\0') {
    DAT_00b3522a = 1;
  }
  return;
}
```
→ L<120 のtiny枠。ガード付き1ストア。分岐はあるが本体1文。

### 例8: 0x005062b0 — class2 residual-thin / **cc<2 (0)** / L=156
```c
undefined4 FUN_005062b0(undefined4 param_1,undefined4 param_2,int param_3)
{
  if (param_3 != 0) {
    func_0x004e51f0(param_3,1);
  }
  return 1;
}
```
→ ゼロガード+単一呼び出し。0x005062d0 は同一骨格で第2引数が 0（on/off 対）。境界判例だが薄い本としては自己完結。

### 例9: 0x00500820 — **class3** / cc<2 (1) / L=63
```c
void FUN_00500820(void)
{
  DAT_00b42cd8 = 1;
  return;
}
```
→ 定数書込のみ。データ誤分割の可能性と「最小実装の INI フラグ setter」の可能性の両方が残る（→ §6-2）。

### 例10: 0x00501a60 — **class3** / cc≥2(×7) / L=90
```c
undefined1 FUN_00501a60(void)
{
  DAT_00b09ae4 = DAT_00b09ae4 == '\0';
  return 1;
}
```
→ フラグ反転のみ。0x005087a0（`DAT_00b42f3c = DAT_00b42f3c != '\x01';`）と同じ型。呼び出しゼロの最小関数群だが、直前/直後に同じ DAT を触る INI ハンドラ帯に並ぶため「実在する最小ハンドラ」と解するのが自然。

## §5 判定（absorbed_suspect 仮説の成立/一部成立/不成立）と V5 表への反映案

### 5.1 判定: **不成立**（大半が完全な独立関数、薄片も自己完結型）

`absorbed_suspect`（cc前置≥2 = 319行 / 314ユニークVA）を「MSVC /OPT:ICF 等で吸収・合併された薄片（独立関数ではない）」という仮説で検証した結果:

1. **71.0%（223/314）が class1（完全な独立関数）**。cc<2 側の class1 比率（76.7%）と差がなく、cc≥2 は薄片化を予測しない（class2+3 率 29.0% vs 23.3%、**χ²=0.611, p≈0.43**）。
2. **同一本文の完全一致グループ = 0組**。/OPT:ICF が identical な COMDAT を折り畳んだ場合に観測される「複数VAが同じ本体を指す」状態が存在しない（本バイナリの各VAは独立したバイト列）。
3. class2 の中心的パターン（73件のフォワーダ、うち47件は単一骨格族）は **骨格は同一でも呼出先 helper が一件ずつ全て別**。これはオペコードハンドラの定型コード生成であって、吸収・重複排除の残骸ではない。全98件が「単一呼び出し転送」「tiny 実装」「ガード付き単一操作」といった**自己完結した最小実装**であり、前関数の内部断片（複数VAにまたがる不完結なコード）は1件も観測されなかった。
4. class2+3 の101件はすべてオペコードハンドラ領域（0x00500770–0x0050d8c0）に収束し、EXE 全域のデータ境界に散在する誤分割パターンとは異なる。
5. これは V70 の確定結果（ForceCreateGaps で created=357/failed=0、全て実在関数）および V61 の「前関数に吸収」機構説 0/102 観測と整合する。

補足: クラス3 の3件（0x00500820 / 0x00501a60 / 0x005087a0）のみ「呼び出し0・定数/反転のみ」で、データ誤分割の可能性が完全には消えない。ただし cc<2 に1件・cc≥2 に2件と cc と無相関で、いずれも他の INI ハンドラと同一帯に並ぶため、現時点では「最小実装ハンドラ」と暫定する（バイナリ検証は §6-2）。

**結論（1行）**: absorbed_suspect（cc≥2, 319件）は薄片・断片の指標ではなく、cc前置は「Ghidra のラベル境界検出可否」のシグナルにすぎず、319件は実在する完全な独立関数群（薄い98件も自己完結型ハンドラ）である → **仮説は不成立**。

### 5.2 V5 表への反映案（**適用はしない**。案の提示のみ）

1. **§6 の分類命名の修正案**: 「前関数に吸収(absorbed) / 欠落(missing)」を事実記述に改称 — 「cc前置<2: gap VA が前関数コードに連続（int3 パディング無し = ラベル境界未検出）」「cc前置≥2: int3 分離済（独立境界）」。V70 で357/357すべて実在関数と確定済みのため、「吸収」の語は誤誘導。
2. **`absorbed_suspect` 仮説欄への追記案**: 「cc≥2 = 319件。V79 で分類 → class1 223 / class2 89 / class3 2（314U）。ICF 合併の直接証拠（同一本体）0組、cc≥2 と薄片化は有意相関なし（χ²=0.61）。仮説不成立」。
3. **§3.1 表への `class` 列追加案**: VA毎の 1/2/3 は `v79_classified.csv`（357行、reason 付き）を正とする。
4. **§6.1 P3追跡への追記案**: 「吸収(cc前置<2) 43件のうち33件(76.7%)が class1 で、『前関数に吸収』機構説は V61 の 0/102 に続き V79 でも弱い」。
5. **class3 3件の注記案**: 「呼び出し0・定数書込/反転のみ。最小 INI/コンソールハンドラの可能性、データ誤分割の可能性が未確認（V79 §6-2）」。

## §6 残課題

1. **近傍重複照合の範囲**: 本判定の exact_dup / 骨格重複はギャップ357件の内部のみ。`decompiled.c` の全21,597関数（ギャップ外21,240 + V70新規357）との本文・骨格重複照合は未実施。前関数/後続関数との重複（タスクの "duplicates of neighboring logic" の厳密版）はこれに含む。
2. **class3 3件のバイナリ確認**: 0x00500820 / 0x00501a60 / 0x005087a0 の実体サイズ、直前の int3 パディング、オペコードテーブル（0xB0AF48系）からの参照有無を確認し、「最小実装 vs データ誤分割」を決着。
3. **境界判例の目視**: residual-thin 3件（0x005062b0 / 0x005062d0 / 0x00508180）と L≥200 の class2 6件（0x005022d0, 0x00502370/90/b0, 0x00504010, 0x0050baa0）は機械規則の境界にいる。手動で「完全関数」へ格上げすべきか要判断。
4. **フォワーダ族の呼出先マップ**: 47件骨格族・20件族等の呼出先 helper（0x004f4xxx / 0x004f6xxx 系）を役割毎に一覧化し、V69/V70 のオペコードハンドラ表と接続する。
5. **cc前置値の出典**: 本判定は V5 表の記載値を正として複写（バイナリからの再計算は行っていない）。再計算する場合は probe_harness の rd() を使うが、.rdata末/.data 開始の 0x600 ギャップ跨ぎで −0x600 シフトする既知バグ（→V77）に注意。
6. **V5 表の重複5件**: 0x005007d0 / 0x00500890 / 0x00503e10 / 0x00503e90 / 0x0050d020 が opid を変えて2行登録されている原因（実装一覧側の重複）の確認。
7. **サイズ指標の微差**: V70 CSV の `decomp_chars` と本文長 L は数字符ずれる（改行計上の差）。本判定は L に統一したが、V70 報告との突合時に混乱しないこと。
8. **（スコープ外）** タスク定義と v10 CSV の `gap_kind=absorbed_suspect`(102件=cc≤3) の呼称衝突。将来のタスクで v10 の gap_kind を使う場合は、どちらの定義かを必ず明示すること。
