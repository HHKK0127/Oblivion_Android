# V61 — analyzeHeadless 再解析とギャップ362件の再ラベル検証（実測レポート）

- 作成日: 2026-10-24
- 区分: **新規成果物（1ファイルのみ）／既存ドキュメントは一切編集していない**
- 本レポート内の「更新案」はすべて **未適用の文面提案**（行番号は現行ファイル基準）
- 作成スクリプトは `%TEMP%\opencode\` 配下のみ（`v61_scan_decompiled*.py` / `v61_parse_v5.py` / `v61_parse_v10.py` / `v61_band_join.py` / `v61_crosscheck.py` / `v61_case_check.py` ほか）

---

## 1. 実行環境とログ検証

### 1.1 環境

| 項目 | 実測値 |
|---|---|
| Ghidra | **12.1.4 PUBLIC**（`application.version=12.1.4` / `application.release.name=PUBLIC`、`application.java.min=21`） |
| インストール先 | `C:\Users\hiroki.kogarumai\.copilot\chats\2026-10-09\improved-waffle-2dbf7451\ghidra_12.1.4_PUBLIC` |
| プロジェクト | `D:\Cargo\Ghidra\temp\ghidra_projects\OblivionRe`（`OblivionRe.gpr` + `OblivionRe.rep`、10ファイル / **126 MB**、作成 19:38:32） |
| インポート対象 | `D:\Cargo\Ghidra\temp\OblivionStandalone.exe`（7,898,624 B、ASCII パス複製） |
| SHA256 | `A8F313845C1545E9A60E1E995961EEF4C033115DA9443F6D756341DF3C2B7DC6`（**`D:\Cargo\Other\Oblivion\Oblivion_Steam版\Oblivion.exe` と一致**＝再検証済み） |
| 実行時刻 | 2026-10-24 19:38:29 – 19:44:52（バックグラウンド） |

### 1.2 成果物（`C:\Users\hiroki.kogarumai\AppData\Local\Temp\opencode\`）

| ファイル | サイズ | 行数 | 最終更新 |
|---|---|---|---|
| `gap_scan.csv` | 8,433 B | **364**（ヘッダ1 + データ362 + サマリ1） | 19:44:47 |
| `gap_vas.txt` | 4,344 B | **362** | 19:37:35 |
| `headless_stdout.log` | 20,609 B | 241 | 19:44:52 |
| `headless_analysis.log` | 22,848 B | 250 | 19:44:52 |
| `headless_stderr.log` | **0 B** | 0 | 19:38:17 |
| `ghidra_scripts/ExportGaps.java` | 3,065 B | — | — |

### 1.3 タイムライン（`headless_analysis.log` のタイムスタンプ実測）

| 時刻 | 所要 | 出口 |
|---|---|---|
| 19:38:29 | — | プロセス開始（LoggingInitialization） |
| 19:38:31 | +2 s | `HEADLESS: execution starts` |
| 19:38:32 | +1 s | `Creating project` / `IMPORTING:` |
| 19:38:47 | +15 s | `IMPORTING: Loaded 0 additional files` |
| 19:38:48 | +1 s | `ANALYZING all memory and code:` |
| 19:44:41 | **+353 s** | `REPORT: Analysis succeeded`（アナライザ内訳の `Total Time 352 secs` と整合） |
| 19:44:41 | +0 s | `REPORT: Execute script: ExportGaps.java` |
| 19:44:47 | +6 s | `ExportGaps: input=362 totalFunctions=21240 DEFINED=0 CONTAINED=0 NO_FUNCTION=362` |
| 19:44:47 | +0 s | `REPORT: Post-analysis succeeded` |
| 19:44:52 | +5 s | `REPORT: Save succeeded` → `REPORT: Import succeeded` |

- **合計ウォール時間: 383 秒（6分23秒）**（19:38:29 → 19:44:52）
- **純分析時間: 353 秒（5分53秒）** / Ghidra 自己申告 **352 秒**
- **インポート: 16 秒 / スクリプト+後分析+保存: 11 秒**

アナライザ別 上位（`Total Time 352 secs` 内）: Decompiler Switch Analysis 74.234 s、x86 Constant Reference Analyzer 66.473 s、Stack 61.910 s、Function Start Search 38.187 s、Disassemble 18.509 s、Function ID 14.480 s、X86 Function Callee Purge 10.874 s。

### 1.4 3点確認（必須）

| 確認項目 | 結果 | 行（analysis log / stdout） |
|---|---|---|
| `Import succeeded` | **OK** | 250 / 241 |
| `Post-analysis succeeded` | **OK** | 247 / 238 |
| `Save succeeded` | **OK** | 249 / 240 |
| （補）`Analysis succeeded` | **OK** | 242 / 233 |

### 1.5 エラー / 警告

- **ERROR レベル行 = 0**（大文字小文字区別での `ERROR` 照合。初回の区別なし照合で4件拾ったのは `DEBUG (WEVTResourceDataType) Error processing Provider Element.` のメッセージ本文であり、レベルは DEBUG）
- **例外・中断類（`Exception` / `ERROR:` / `Aborted` / `FAILED`）= 0**
- **stderr = 0 B**
- **WARN = 11 件（固有メッセージ。stdout と analysis log の両方に書かれるため出現は22回）**

| # | 件数 | 内容 | 影響 |
|---|---|---|---|
| 1 | 5 | `DecompileCallback: Decompiling 00xxxxxx: Unable to read bytes at ram:…`（00555a80 / 00402dc0 / 005661e0 / 007c9f30 / 005c0440） | 単関数のデコンパイル可否のみ。ギャップ判定に無関係 |
| 2 | 4 | `ExportDataDirectory: Invalid or missing function at 100754dc / 105f0c6c / 100dcd7c / 52e820e8` | PE エクスポート領域の不正 VA。対象外 |
| 3 | 2 | `NoReturnFunctionAnalyzer: Ignoring leading '_' chars on … ___raise_securityfailure / ___report_rangecheckfailure` | 名寄せの正規化警告。解析結果に影響なし |

結論: **分析は正常完走**。致命的エラーなし、3点の成功 REPORT すべて取得。

---

## 2. `gap_scan.csv` の全件集計

### 2.1 構造

```
行1    : va,status,containing_name,containing_start   ← ヘッダ
行2-363: 362 データ行
行364  : #total_functions=21240 defined=0 contained=0 none=362 input=362   ← サマリ行
```

### 2.2 status 分布（362行全件読解）

| status | 件数 | 割合 |
|---|---|---|
| `NO_FUNCTION` | **362** | 100.0% |
| `DEFINED` | 0 | 0% |
| `CONTAINED` | 0 | 0% |

### 2.3 `containing_name` 列の実測

- **`containing_name` 空 = 362/362（100%、全件空）**
- **`containing_start` 空 = 362/362（全件空）**

### 2.4 VA の重複（台帳単位と VA 単位の差）

- **行数 362 / ユニーク VA = 357**（重複5 VA × 各2行 = 10行）
- 重複 VA と該当 opid:

| 重複 VA | 行 | opid / name | gap_kind |
|---|---|---|---|
| `0x503e10` | 2 | 0x0004 OnEquip / 0x001C OnActorEquip | ghidra_missing |
| `0x503e90` | 2 | 0x0005 OnUnequip / 0x001D OnActorUnequip | ghidra_missing |
| `0x50d020` | 2 | 0x10D4 AdvancePCSkill / 0x110F ModPCSkill | ghidra_missing |
| `0x5007d0` | 2 | 0x0151 ShowSubtitle / 0x1164 ShowDialogSubtitles | absorbed_suspect |
| `0x500890` | 2 | 0x0155 OutputMemContexts / 0x0169 ResetMemContexts | absorbed_suspect |

→ `ghidra_missing` 260行 = **257ユニークVA**、`absorbed_suspect` 102行 = **100ユニークVA**、計 **357ユニークVA**。

### 2.5 入力整合（タスク6）

- `gap_vas.txt` = **362 行**（非空362 / ユニーク357）
- `gap_scan.csv` データ行 = **362 行**（ユニーク357）
- 両者の VA 集合を 10進正規化して比較 → **多重集合完全一致（差分0）**
- サマリ行の `input=362` と一致

### 2.6 スクリプトの判定意味（`ExportGaps.java` 実読確認）

| status | 判定式 |
|---|---|
| `DEFINED` | `getFunctionAt(addr) != null` |
| `CONTAINED` | `getFunctionAt` が null のとき `getFunctionContaining(addr) != null`（`containing_name` / `containing_start` はこのときのみ埋まる） |
| `NO_FUNCTION` | 上記いずれも null |

→ **`containing_name` 全行空 ＝ 「どの既存関数の本体にも含まれない」という直接的な実測値**（「スクリプトが埋めていない」ではない）。

---

## 3. 新旧解析の対比（本レポートの核心・すべて実測）

### 3.1 `decompiled.c` の 1パス走査（lowercased 実測）

対象: `docs\Oblivion_decompiled.c`（**28,995,463 B / 1,066,496 行**、mtime 2026-10-20 17:38）

| 指標 | 実測値 |
|---|---|
| 定義行数（型+`FUN_…(` かつ `;` 無し・非インデント） | 20,565 |
| **`FUN_xxxx` 定義（ユニーク・lowercased）** | **20,465** |
| `thunk_FUN_xxxx` 定義（ユニーク） | 87 |
| **`FUN_` 系定義 合計** | **20,552** |
| **`FUN_` トークン出現（ユニーク・lowercased）** | **20,558**（純 `FUN_` 20,471 + thunk 87） |
| 定義なし＝参照のみ（トークン差集合） | **6**: `fun_00815db0`, `fun_00820c00`, `fun_00828280`, `fun_0082d990`, `fun_00836810`, `fun_0085e660` |
| hex 部分が大文字を含むトークン | **0 / 20,471**（本ファイルは全件すでに小文字） |

- **20,465 は `v5_ghidra_gaps_report.md` 行5 の「(1,066,496行 / 20,465関数)」と**厳密一致**（独立実測での再現）。
- 参照のみ6件は同 行586 の「`ref_only` の6件 (FUN_00815DB0 等)」と一致。

### 3.2 差分

| 比較 | 新解析 | decompiled.c | 差 |
|---|---|---|---|
| `FUN_` 定義（純） | 21,240 | 20,465 | **+775** |
| `FUN_` 系定義（thunk 含む） | 21,240 | 20,552 | **+688** |
| `FUN_` トークン全体 | 21,240 | 20,558 | **+682** |

※ 新解析の 21,240 は `FunctionManager.getFunctions(true)` の全走査件数（メモリ関数）。`decompiled.c` は **2026-10-20 生成の既存成果物＝別セッションの解析結果**であり、同一解析の比較ではない。差は「新解析が新規に関数化した分」と「decompiled.c 側の生成条件差」の合算で、**内訳は本 V61 では未切り分け**。

### 3.3 ギャップ集合の相互検証

| 検証 | 結果 |
|---|---|
| 357ギャップVA のうち decompiled.c に `FUN_` 定義があるもの | **0 / 357**（統一定義と整合） |
| V10 `in_decompile=yes` 165行（158ユニーク）の `fun_label` が decompiled.c に定義あり | **158 / 158**（100%） |
| 新解析で 362行すべて `NO_FUNCTION` | **362 / 362** |

→ **新解析でも旧解析（decompiled.c）でも、362件はどちらの自動解析でも関数化されない**（総関数が増えているにもかかわらず）。

---

## 4. V10・V5 分類との突合と「吸収説」の緊張点

### 4.1 突合の手順と整合

- `v5_ghidra_gaps_report.md` の §3.1 表（**行66–427 = 362行**）を、`v10_impl_decompile_join.csv` の `in_decompile=no` 362行と **位置対で結合 → impl 一致 362/362**（同一順序を確認）
- V5 の `CC前置` 数値（`cc×N` / `(N)`）を抽出し閾値を再計算

### 4.2 クロス表（実測）

| CC前置バンド | 件数 | V5 判定（閾値≥2） | V10 `gap_kind`（閾値≥4） | **V61 新解析** |
|---|---|---|---|---|
| `cc ≥ 4` | **260** | 欠落 | `ghidra_missing` | `NO_FUNCTION`（defined=0 / contained=0） |
| `cc 2〜3` | **59** | **欠落** | **`absorbed_suspect`** | `NO_FUNCTION` |
| `cc ≤ 1` | **43** | 吸収 | `absorbed_suspect` | `NO_FUNCTION` |
| 合計 | **362** | 319 / 43 | 260 / 102 | **362 / 0 / 0** |

- V5: 362行中 `欠落` 319・`吸収` 43（`cc≥2` = 319 / `cc<2` = 43）→ **タスク記載の 319/43 を再現**
- V10: `ghidra_missing` 260 / `absorbed_suspect` 102 → **タスク記載の 260/102 を再現**
- **V10 の `ghidra_missing` 260行（257ユニークVA）と V5 の `cc≥4` 260行（257ユニークVA）は集合として完全一致（対称差0）**
- **V5 の P1 impl 集合（357ユニーク）と `gap_vas.txt`（357ユニーク）は完全一致（対称差0）**
- **差59件 ＝ `cc×2`〜`cc×3` のバンド**（`cc≥2` で319、`cc≥4` で260、差59）。**この59件のみ小分類が食い違い、集合「362」自体は V5/V10 で一致**

### 4.3 「吸収説」対 `contained=0` の整理（本節が核心）

**吸収説の機械的主張（原文）**

- V5 行564: 「**吸収 (absorbed)**: CC前置 < 2 → gap VA が前関数のコード本体に連続。**Ghidra が前関数の一部と誤認識**」
- V5 行593: 「43件が『前関数に吸収』… **Ghidra が前関数の CFG に統合**」
- V10 `note`（`absorbed_suspect`）: 「cc前置なし＝**前関数に吸収の可能性**」

**その予測**: gap VA が前関数の本体に含まれるなら、`getFunctionContaining(gapAddr)` は前関数を返し、`gap_scan.csv` は `CONTAINED` + `containing_name=前関数名` を出すはず。

**実測結果**: **`CONTAINED = 0 / 362`、`containing_name` は 362/362 が空。**
特に吸収説の対象である `absorbed_suspect` 102件（内訳: V5=吸収と一致する43件 ＋ V5=欠落と V10=吸収が食い違う59件）は、**102/102 が `contained=0`**。

**結論（誠実な整理）**

1. **吸収説の「Ghidra が前関数に統合している」という機械的主張は、V61 新解析では 0件も観測されない → 吸収説の弱化・再検討材料。**
2. `cc 2〜3` の **59件は V5=欠落 / V10=吸収 で分類が逆**だが、V61 は両者とも `contained=0` → **この59件については V5（欠落）側の読みに軍配**。V10 の `absorbed_suspect` 59件は再分類の候補。
3. `cc≤1` の **43件は V5/V10 とも「吸収」だが、`contained=0` はその機構説と衝突する。** ただし V5 の観測事実そのもの（＝gap VA 直前に連続 int3 が無い、というバイト並び）は V61 では**反証も確認もされていない**（V61 はバイト列を見ていない）。よって「吸収＝Ghidra が統合」は弱い／「バイト連続」は未判定、と切り分ける必要がある。
4. **`contained=0` の替代説明（逃げ道）**: `getFunctionContaining` が null になるのは「前関数そのものが新解析に存在しない」場合にも起こる。362件の `prev_fun` / `next_fun` は**旧解析（decompiled.c）では 362/362 とも定義あり**だが、**新解析での存在有無は V61 で未照会**。→ 前関数が新解析にも実在することが確認できれば `contained=0` は決定的となり、逆に前関数が不在なら V61 だけでは吸収説を否定できない。**この一点が本検証の残る弱点（§7-1）。**
5. なお新解析は**総関数21,240 ＞ decompiled 20,465/20,552** であり、旧解析で定義済みだった前関数が新解析で消えている蓋然是低いが、**蓋然性は証拠ではない**。

**補助的観察（共通原因の仮説）**: 362件の `prev_fun`・`next_fun` は旧解析で全て定義済みなのにギャップだけ未定義 → 「オペコードテーブル（データ）由来で CALL 到達不能＝関数作成の種（seed）が無い」という V5 行595 の説明が、新旧両解析に共通する原因としてもっとも整合的。これは吸収説とは独立に成立する。

---

## 5. 更新案（**未適用・既存ファイルは一切編集していない**）

以下はすべて **提案文面**。行番号は 2026-10-24 時点の現行ファイル基準（挿入後は後続の行番号がシフトする）。

### 5.1 `tmp\re\v5_ghidra_gaps_report.md`（現行 599 行）

**案A（推奨・追記1）**: 行60 の直後（新行61）に、§3 サマリの子項目として1行挿入

```
  - （V61 実測 2026-10-24）新解析 Ghidra 12.1.4 でも **362/362 が NO_FUNCTION**（defined=0 / contained=0、containing_name 全行空）。小分類（43/319）に依らず「Ghidra が関数化しない」ことは確定。
```

**案B（推奨・追記2）**: 行595（§8 終端）と 行597（`---`）の間に新章を挿入

```
## 9. V61 headless 再解析による検証（2026-10-24 追記・未適用案）

Ghidra **12.1.4 PUBLIC** の `analyzeHeadless` で `D:\Cargo\Ghidra\temp\OblivionStandalone.exe`（SHA256 `A8F31384…C2B7DC6`＝`Oblivion_Steam版\Oblivion.exe` と一致）を新規解析し、本報告 §3.1 の362行を `getFunctionAt` / `getFunctionContaining` で走査した（`%TEMP%\opencode\gap_scan.csv`、8,433B）。

- 解析: 19:38:48–19:44:41（**353秒**、Total Time 352秒）。`Import succeeded` / `Post-analysis succeeded` / `Save succeeded` の3点取得、ERROR 0件、WARN 11件（いずれも無害）。
- 関数総数 **21,240**。対象362行: **defined=0 / contained=0 / NO_FUNCTION=362**、`containing_name` は **362/362 が空**。
- CC前置バンド別: `cc≥4` 260件 / `cc2〜3` 59件 / `cc≤1` 43件、**全バンド NO_FUNCTION**。
- **§6 行564 の「吸収＝Ghidra が前関数の一部と誤認識」という機構説は V61 で 0/43 が観測されず＝吸収説の弱化材料**（ただし gap VA 直前の int3 なし＝バイト連続という観測事実は未検証。さらに前関数自体の新解析での存在有無も未照会のため、決定的否定には至らない）。
- `cc2〜3` の59件は本報告=欠落 / V10=absorbed_suspect で食い違うが、`contained=0` は本報告（欠落）側に有利。
- `docs\Oblivion_decompiled.c`（1,066,496行）の lowercased `FUN_` 定義は **20,465**（thunk 含む 20,552、トークン 20,558、参照のみ6件）。新解析 21,240 との差は **+775 / +688**。
- 重複: 362行 = 357ユニークVA（重複5 VA: 0x503e10 / 0x503e90 / 0x50d020 / 0x5007d0 / 0x500890）。

出典: `%TEMP%\opencode\gap_scan.csv`, `headless_stdout.log`, `headless_analysis.log`。本章は**提案のみで未適用**。
```

**案C（任意）**: 行569・570 の区分表に「V61 contained」列を追加する場合は、表ヘッダ（行567）と2行（569/570）をそれぞれ `| 区分 | P1 | P2 impl | P2 call | 計 | V61 contained |` 形式に置換し、行569 に `| 0 |`、行570 に `| 0 |` を末尾付加。

### 5.2 `tmp\re\v10_impl_decompile_join.csv`（現行 528 データ行、末尾改行で行529は空）

**案A（推奨）**: 行528 の直後、**新行529** として追記（`gap_scan.csv` のサマリ行と同じ `#` 慣例）

```
#v61_relabel,ghidra_12.1.4_PUBLIC,2026-10-24,src=%TEMP%/opencode/gap_scan.csv,input=362,defined=0,contained=0,none=362,total_functions=21240,uniq_va=357,dup_va=5,ghidra_missing=260(NO_FUNCTION),absorbed_suspect=102(NO_FUNCTION),v5_cc_ge4_set_symdiff=0,note=contained=0 weakens absorbed theory (102/102); prev_fun existence in new run unverified
```

- ⚠️ 留意: `#` 行は `csv.DictReader` 等では1データ行として読まれる（`gap_scan.csv` と同じ取扱い）。厳密な CSV パーサを避けたい場合は **案B** を推奨。

**案B（代替）**: ヘッダ（行1）に列 `,v61_status` を追加し、行2–528 に各行の impl を対応付けて `NO_FUNCTION` を埋める（362行は `NO_FUNCTION`、`in_decompile=yes` の165行は V61 未走査のため空欄）。

### 5.3 `docs\RE_EXE_REPORT.md` §67.6.1（見出し 行11404、表 11408–11412、注記 11414–11415、旧記述 11417、次節 11419）

**案A（必須・旧記述の置換）**: 行11417 を以下の文に置換

```
Ghidra 本体は `C:\Users\hiroki.kogarumai\.copilot\chats\2026-10-09\improved-waffle-2dbf7451\ghidra_12.1.4_PUBLIC`（**12.1.4 PUBLIC** / Java21）で利用可能となり、2026-10-24 19:38:29–19:44:52 に `analyzeHeadless` を完走した（分析353秒・自己申告352秒、`Analysis succeeded` / `Post-analysis succeeded` / `Save succeeded` / `Import succeeded` の4点取得、ERROR 0件・WARN 11件）。ギャップ362件の再ラベル化検証は**実施済み**で、**362/362 が `NO_FUNCTION`（`defined=0` / `contained=0`、`containing_name` 全行空）**＝新旧どちらの自動解析でも関数化されない（出典 `%TEMP%\opencode\gap_scan.csv`、V61）。旧記述「Ghidra 本体不在・未実施（TODO V23）」は**失効**。
```

**案B（推奨・追記）**: 行11415 の直後（新行11416）に1段落挿入

```
**V61 実測（2026-10-24）**: 新解析の関数総数 **21,240** に対し `docs\Oblivion_decompiled.c`（1,066,496行）の lowercased `FUN_` 定義は **20,465**（`thunk_FUN_` 87 を含め 20,552、`FUN_` トークン全体 20,558、定義なし参照のみ6件）。差は **+775 / +688**。上表260件（`ghidra_missing`）は **V5 の cc前置≥4 セットと完全一致**（257ユニークVA・対称差0）。`absorbed_suspect` 102件は「V5=欠落（cc2〜3）59件」＋「V5=吸収（cc≤1）43件」の合算であり、**この59件のみ V5/V10 で小分類が逆**。**新解析では362件すべて `contained=0`** のため、`absorbed_suspect` の「前関数に統合されている」という機構説は V61 では 0/102 で観測されず＝**吸収説の弱化・再検討材料**（ただし前関数自体の新解析での存在有無は未照会のため決定的否定ではない）。
```

**案C（任意）**: 行11411・11412 の「意味」列末尾に `／ V61: NO_FUNCTION 260/260`・`／ V61: NO_FUNCTION 102/102` を付記。

---

## 6. 次の一手 — V70（強制 createFunction + デコンパイル）仕様の骨子

※ V70 は別途並行実行予定。ここでは**仕様の骨子**にとどめ、実装・実行は行わない。

**実行形態**

```
analyzeHeadless D:\Cargo\Ghidra\temp\ghidra_projects\OblivionRe OblivionRe
  -process OblivionStandalone.exe -noanalysis
  -scriptPath %TEMP%\opencode\ghidra_scripts
  -postScript ForceCreateGaps.java %TEMP%\opencode\gap_vas.txt %TEMP%\opencode\v70_force_create.csv
```

- `-process` + `-noanalysis`（既存DBを開き、再解析・再インポートを回避）。スクリプトは変更を加えるので `-readOnly` は使わない（保存は許可）。
- 入力は `gap_vas.txt`（362行 / 357ユニークVA）。**V61 の `gap_scan.csv` 結果を `getFunctionAt` で再現確認してから**作成に入る（不一致があれば V61 の再現性エラーとして即中断）。

**スクリプト処理（1 VA ごと）**

1. 事前状態記録: `getFunctionAt(va)` / `getFunctionContaining(va)` / `prev_va`（`prev_fun`）の `getFunctionAt` / `next_va` の `getFunctionAt`
2. 強制作成: `CreateFunctionCmd(addr, monitor)` または `FunctionManager.createFunction(name, addr, body, true)`（例外・失敗理由を文字列で捕捉）
3. 作成成功時のみ: `DecompInterface.decompileFunction(f, timeout, monitor)` を実行し、成功/失敗/`DecompileError` を記録
4. 既存関数と衝突した場合（本体が前関数を跨ぐ等）は、拡張前後の `Function.getBody()` を記録

**出力列（`v70_force_create.csv`）**

```
va,opid,name,cc_band,v5_judge,v10_gap_kind,
contained_before,prev_exists,next_exists,
create_ok,create_err,body_start,body_end,
decompile_ok,decompile_err,decompile_head
```

**分類マトリクス（362件 × 作成可否 × デコンパイル成否）**

| create | decompile | 意味 / 対応 |
|---|---|---|
| 成功 | 成功 | **強制ラベル化可能**。V5/V10 の「missing（独立関数だがラベル未生成）」を補強。実装対象 |
| 成功 | 失敗 | ラベルは作れるが本文不可（CFG境界/データ混入）。個別調査 |
| 失敗 | — | 既存 CFG と衝突、またはアドレスが関数境界でない → **`absorbed_suspect` 102件の帰結判定に直結** |

**追加判定軸（V61 の弱点の解消）**

- `prev_exists` 列 → §4.3-4 の弱点を一掃。前関数が新解析に実在するなら `contained=0` は決定的。
- `absorbed_suspect` 102件を `cc2〜3`（59）と `cc≤1`（43）に分けて create 成功率を比較 → 吸収説の妥当性を数値で判定。
- 362件が「コードブロックに在るのに seed 不在」なのか「ブロック属性が data」なのかを `getMemoryBlock(va).isExecute()` で切り分け（共通原因の仮説検証）。

---

## 7. 未解決点

1. **前関数（`prev_fun` 362件）の新解析での存在有無が未照会。** `contained=0` が「前関数が存在し、かつ本体がギャップを含まない」のか「前関数自体が不在」なのかを区別できない。吸収説の判定がここに悬かっている（V70 の `prev_exists` 列で解消可）。
2. **21,240 と 20,465/20,552 の差（+775/+688）の内訳が未特定。** 新解析の新規検出分と、`decompiled.c`（2026-10-20 生成・別セッション）の生成条件差の寄与が分離できていない。
3. **「362」と「357」の使い分け。** 台帳（opid）単位は362行、VA単位は357件（重複5 VA）。集合比較は VA 単位、台帳報告は opid 単位で行う運用が必要。
4. **362件のブロック属性（`isExecute`）未確認。** 「テーブル由来＝seed 不在」という共通原因の仮説は、V61 では間接的（prev/next は定義済み・ギャップのみ未定義）な証拠にとどまる。
5. **新解析21,240の内訳（thunk / external / 本体関数の別）未取得。** `getFunctions(true)` はメモリ関数のみを走査するため、`decompiled.c` 側の `thunk_FUN_` 87件との対応関係は未検証。
6. **`cc2〜3` の59件の最終分類が未決着**（V5=欠落 / V10=吸収）。V61 は `contained=0` により V5 に有利な証拠を与えたが、V10 側の再分類は提案止まり（§5 の更新案に反映していない＝`absorbed_suspect` の文言は温存）。

---

## 8. 検証チェックリスト

| タスク | 結果 |
|---|---|
| `gap_scan.csv` 全件読解（362行） | ✅ status `NO_FUNCTION` 362 / サマリ行 行364 |
| `containing_name` 全件空か | ✅ 362/362 空（`containing_start` も同様） |
| ログ正常完走 | ✅ ERROR 0 / WARN 11（無害）/ stderr 0 B |
| 所要時間 | ✅ 383秒ウォール / 353秒分析（自己申告352秒） |
| `Import` / `Post-analysis` / `Save` succeeded | ✅ 3点すべて取得 |
| `decompiled.c` FUN_ ラベル数 1パス実測（lowercased） | ✅ 20,465（thunk 含む 20,552、トークン 20,558） |
| 新解析 21,240 との差 | ✅ +775 / +688 / +682 |
| V10 260/102 との突合 | ✅ 位置対362/362一致、V10 ghidra_missing ≡ V5 cc≥4（対称差0） |
| V5 319/43 の再現 | ✅ 319 / 43、差59 = cc2〜3 |
| 吸収説 vs `contained=0` の整理 | ✅ §4.3（弱化材料・ただし前関数存在未照会の留保付き） |
| 更新案（未適用・行番号・文面） | ✅ §5（v5 2案＋任意1、v10 2案、RE_EXE 3案） |
| V70 仕様骨子 | ✅ §6 |
| `gap_vas.txt` = 362行 / csv = 362データ行 | ✅ 両方362行・多重集合完全一致（ユニーク357） |
| 既存編集禁止 / 新規1ファイル / スクリプトは `%TEMP%` | ✅ 本ファイルのみ新規作成。既存3ファイルは読み取りのみ |
