# V155: ラベル/台帳の 6 層統合住所テーブル（VA キー 1 本）

- 作成: 2026-10-27 / タスク: V145 相当（serial 145・**高**）/ 出典: `v_data_relmap.md` §F7・v137 §6・v127 R3（統合住所テーブルの不在＝最大の住所ベース未突合）。
- 前提: `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl）。
- 検証対象 EXE: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（7,898,624 B。.text = VA 0x401000–0xA27C39 / 6,450,233 B、VA=0x400C00+file_off）。capstone 5.0.7。
- 編集範囲: 既存ファイルは**すべて読取専用**。成果物は本新規 1 ファイル + `%TEMP%\opencode\v155_union.csv`（新規）のみ。新規スクリプトは `%TEMP%\opencode\v155_*.py`。git / docs / TODO / session-memory / plan / 共有台帳（`func_list_new.txt` / `v133_dup_results.csv`）は**無編集**。
- 検出ロジック: `v127_scan.py` を**逐語再実装**（seed A/B・5 条件フィルタ・accept600・pad_before すべて同一）。再現カウントが v127 既知値と **完全一致**（§0・§1 で検証）。

---

## (0) 全層の清点（本タスク実測・v127 既知値と一致）

| 層 | 名称 | 件数 | 入力ソース | 検証 |
|---|---|---:|---|---|
| **L1** | Ghidra FunctionManager 台帳 | **21,597** | `func_list_new.txt` | VA ユニーク 21,597 |
| **L2** | dec_index（`// FUN_* @ 0x` ヘッダ） | **20,465** | `docs/Oblivion_decompiled.c` ヘッダ走査 1 回 | v127/v133 と一致 |
| **L3** | Copilot 全数列挙 V86 UNLABELED | **10,175** | `tmp/re/v86_unlabeled_scan.txt` A=214 / B=9,809 / C=152 | 3 節合計一致 |
| **L4** | Cat-3（デコンパイル無領域エントリ） | **10,261** | v127 検出の逐語再実装 | 10,261 / seed 内訳も一致 |
| **L5** | Cat-3b（台帳のみ・全ヘッダ無し） | **365** | L1 − (FUN∪named ヘッダ 21,232) | v127 の 365 と一致 |
| **L6a** | seed-B 新規（pad 非依存） | **133** | L4 のうち seedB ∧ ¬seedA | v127 §1-3 の 133 と一致 |
| **L6b** | V96 gap 新实例 I1–I10 | **10** | `v136_doc_append.md` §3 | L3/L4 に 10/10 収録 |

**ヘッダ清点（decompiled.c 1,066,496 行を 1 回走査）**: `// FUN_* @ 0x` = **20,465** / 名前付き（非 FUN）= **767** / 全ヘッダ = **21,232**。台帳 non-FUN 名義 767 と名前付きヘッダ 767 は **VA 完全一致**（named-only 0 / ledger-only 0）。名前付き 767 の内訳: plain 597 / thunk_* 100 / RTTI `` `name' `` 50 / FID_conflict 20。

---

## (1) 6 層の集合演算（exclusive / overlap）

### 1-1 層サイズ

| 層 | L1 | L2 | L3 | L4 | L5 | L6a | L6b |
|---|---:|---:|---:|---:|---:|---:|---:|
| 件数 | 21,597 | 20,465 | 10,175 | 10,261 | 365 | 133 | 10 |

**6 層の和集合 = 31,905 VA**（= 21,597 + 10,175 + 10,261 − 重複。L5/L6a/L6b は既存層の部分集合）。

### 1-2 pairwise overlap（|A ∩ B|）

|  | L1 | L2 | L3 | L4 | L5 | L6a | L6b |
|---|---:|---:|---:|---:|---:|---:|---:|
| **L1** ledger | 21,597 | 20,465 | 0 | 0 | 365 | 0 | 0 |
| **L2** dec_index | 20,465 | 20,465 | 0 | 0 | 0 | 0 | 0 |
| **L3** v86 | 0 | 0 | 10,175 | 10,128 | 0 | 0 | 10 |
| **L4** cat3 | 0 | 0 | 10,128 | 10,261 | 0 | 133 | 10 |
| **L5** cat3b | 365 | 0 | 0 | 0 | 365 | 0 | 0 |
| **L6a** seedB | 0 | 0 | 0 | 133 | 0 | 133 | 0 |
| **L6b** I1–I10 | 0 | 0 | 10 | 10 | 0 | 0 | 10 |

### 1-3 membership-pattern 分布（重 exclusive / overlap 実体）

| パターン（属する層） | 件数 | 意味 |
|---|---:|---|
| L1+L2（台帳 & FUN ヘッダ） | 20,465 | 通常の Ghidra=dec_index 一致関数 |
| L3+L4（V86 & Cat-3） | 10,118 | ラベルなし＝両方式が捕捉 |
| **L1 のみ**（台帳 & 全ヘッダ無し = 名前付きヘッダあり） | **767** | named-only 関数（Cat-2。dec_index が無視） |
| L1+L5（台帳 & FUN 名義だがヘッダ無し） | 365 | **Cat-3b**（decompiled 出力のみ欠落） |
| L4+L6a（Cat-3 & seed-B 新規） | 133 | pad 非依存で V86 が検出不能だった分 |
| **L3 のみ**（V86 あるが Cat-3 採用せず） | **47** | 偽境界 43 + interior 4 = **未収録 VA**（§5） |
| L3+L4+L6b（gap instance） | 10 | I1–I10 |

### 1-4 per-layer exclusive（その層にのみ属する VA）

| 層 | exclusive | 層サイズ | shared |
|---|---:|---:|---:|
| L1 ledger | 767 | 21,597 | 20,830 |
| L2 dec_index | **0** | 20,465 | 20,465（L2 ⊂ L1 严格被覆） |
| L3 v86 | **47** | 10,175 | 10,128 |
| L4 cat3 | 0 | 10,261 | 10,261（L4 ⊂ L3∪L6a、完全連合） |
| L5 cat3b | 0 | 365 | 365（L5 ⊂ L1） |
| L6a seedB | 0 | 133 | 133（L6a ⊂ L4） |
| L6b I1–I10 | 0 | 10 | 10（L6b ⊂ L3∩L4） |

**解釈**: L2 は L1 に厳密被覆（exclusive 0）。L5/L6a/L6b はそれぞれ L1/L4/(L3∩L4) の真部分集合で独立 VA を新設しない。**実質的に新しい住所を供給するのは L1（767 named）・L3（47 未採用）・L4（L3 に無い 133）**で、6 層の「住所の実体」は L1 ∪ L3 ∪ (L4−L3) = 21,597 + 10,175 + 133 − (10,128 重複) の 31,905 に収束。

---

## (2) 名前付きヘッダ 767（v127/v133）vs 684（v90）の口径裁定

**裁定: 数え方差（正規表現の網の広さの差）。更新差ではない。docs の 684 表記は訂正要（実数 767）。**

同一の `decompiled.c` に対し v90 と v127 の**両方のヘッダ正規表現を走らせた**:

| 正規表現 | 名前付き（非 FUN）検出 | 出典 |
|---|---:|---|
| v127 catch-all `^// (.+?) @ 0x([0-9a-fA-F]+)\s*$` | **767** | `v127_scan.py` HDR（本タスク再現一致） |
| v90 identifier-only `^// ([A-Za-z_][A-Za-z0-9_]*) @ 0x` | **684** | `v90_hdr2.py` ANY（本タスク再現一致） |
| 差（v127 のみ / v90 のみ） | **+83 / 0** | ― |

**+83 の実構成**（v90 の識別子専用正規表現 `[A-Za-z_][A-Za-z0-9_]*` が拾えなかった名前）:

| サブタイプ | 件数 | 名前例（v90 漏れの原因文字） |
|---|---:|---|
| RTTI `` `name' `` | 50 | `` `scalar_deleting_destructor' ``（`` ` `` `'`） |
| `FID_conflict:*` | 20 | `FID_conflict:CreateContainer`（`:`） |
| デコレータ/特殊文字つき plain | 13 | `operator*`（`*`）/ `operator==`（`=`）/ `~_Fac_node`・`~type_info`・`~exception`（`~`）/ `__CxxThrowException@8`・`__CallSettingFrame@12`・`_LanguageEnumProc@4` など stdcall 接尾辞（`@`）/ `$I10_OUTPUT`（`$`） |

- **83 件すべてが識別子外文字を含む**ため、`[A-Za-z_][A-Za-z0-9_]*` 型の v90 正規表現は構造上一致しない。v90 の 684 は「名前が C 識別子として妥当なもの」の計数で、「名前付きヘッダ全数」ではない。
- **更新差でない証跡**: FUN ヘッダ（dec_index）は **20,465** が v90 / v127 / v133 / 本タスク v155 の 4 回の独立カウントで完全一致。decompiled.c が再生成・差し替えされていれば FUN 数も動くはずだが不変 = **同一ファイル・同一行数 1,066,496**。よって +83 は純粋な計数口径の差。
- **docs 訂正要否**: `docs/RE_EXE_REPORT.md` 等の「名前付きヘッダ 684 / 全ヘッダ 21,149」表記は**実数（767 / 21,232）へ訂正要**。dec_index 20,465・台帳 21,597 は正しいので波及は名前付き件数の記述のみ。**docs 編集は本タスクの禁止事項のため未実施**（親セッション承認制。v127 R1/V143 に対応）。

---

## (3) I1–I10（V96/V136 gap 新实例）の裏づけ

`v136_doc_append.md` §3 の 10 件を独立再検証（本タスクの逐語再実装パイプラインで L3/L4 収録を機械照合）:

| id | entry VA | L1 台帳 | L2 dec_index | L3 V86 | L4 Cat-3 | seed | pad | accept |
|---|---|---|---|---|---|---|---:|---|
| I1 | 0x00A10EB0 | 0 | 0 | ✓ | ✓ | A | 12 | ok |
| I2 | 0x0040DF80 | 0 | 0 | ✓ | ✓ | **A+B** | 12 | ok |
| I3 | 0x00437600 | 0 | 0 | ✓ | ✓ | A | 1 | ok |
| I4 | 0x00656C90 | 0 | 0 | ✓ | ✓ | A | 14 | ok |
| I5 | 0x006710B0 | 0 | 0 | ✓ | ✓ | A | 11 | ok |
| I6 | 0x007B14C0 | 0 | 0 | ✓ | ✓ | A | 3 | ok |
| I7 | 0x007C53B0 | 0 | 0 | ✓ | ✓ | A | 2 | ok |
| I8 | 0x00405440 | 0 | 0 | ✓ | ✓ | A | 6 | ok |
| I9 | 0x006A6AA0 | 0 | 0 | ✓ | ✓ | A | 5 | ok |
| I10 | 0x006A6D10 | 0 | 0 | ✓ | ✓ | A | 8 | ok |

- **収録裏づけ: L4 Cat-3 10/10・L3 V86 10/10・L1/L2 不在 10/10**（全件が「無名 gap」= 台帳も dec_index も無し）。pad 値 12/12/1/14/11/3/2/6/5/8 も v136 表と完全一致。
- **独立性**: gap 内ブロックの独立性（I1=140 独立スタブ / I3・I5・I6=単一独立 / I2・I4・I7・I8・I9・I10=同一関数 multi-block・multi-exit）は `v127_gap_indep.py`・`v127_gap_validate.py` 出力（%TEMP%）で既に判定済みで、本タスクの entry 収録・pad 一致と矛盾しない。外部 E8/E9 が中間・末尾ブロックを参照する例 0/10 も v127 出力と一致。
- **I2 の特殊性**: 唯一 seed A+B（CRT 0x98780E からの E8 呼出し = WinMain entry）。他 9 件は pad 依存 seed A のみで V86 に A/B 節として収録。
- **台帳追記単位**: 独立追記が必要なのは I1（本体）+ I3・I5・I6 + I7・I9・I10（entry 登録）で、I2・I4・I8 は witness ブロックが同一関数内部（v136 補正 C4 と整合）。

---

## (4) v137「decompiled 欠落 1,132 関数」候補との整合

**統合表から機械抽出可。定義 = L1 台帳 − L2 dec_index = 1,132 行**（本タスク実測 1,132 で一致）。

内訳（1,132 の分解）:

| 部分集合 | 件数 | 対応層 | 実体 |
|---|---:|---|---|
| 名前付きヘッダあり台帳関数 | **767** | Cat-2（L1 のみ exclusive） | dec_index が無視する非 FUN 名義（thunk/RTTI/FID/plain） |
| FUN 名義だがヘッダ無し | **365** | **L5 Cat-3b** | decompiled 出力自体が欠落（例 FUN_00500450 系） |
| **合計** | **1,132** | ― | v137 §6 の「台帳差分で機械抽出可」＝本表 L1&~L2 |

- v137 の典型例 `FUN_005101E0`（0x5101e0）は台帳に存在・dec_index に無い = 1,132 に含まれる。E8 site の宿主帰属を狂わせる「dec_index 欠落の罠」は、この 1,132 のうち **365（ヘッダ完全無し）が特に危険**（767 は名前付きヘッダがあるので enclosing 帰属は名前付きヘッダで救える）。
- **結論**: v137 §7 残点「1,132 関数の列挙（スクリプト 1 本）」は本統合表の `L1&~L2` 行フィルタで**既に充足**。抽出式 = CSV の `func_list_name 非空 ∧ dec_header が FUN_* でない`（＝名前付きヘッダ 767 + ヘッダ空 365 の和）で **1,132 行**。うち `Cat3b=1` が 365（ヘッダ完全無し）。

---

## (5) 未収録 VA の所在（L3 exclusive 47）

**未収録 VA = V86（L3）にあるが Cat-3（L4）に採用されなかった 47 件**。V127 の厳密 terminator（term_kind＝命令長整合）フィルタで V86 の粗い `is_terminator` による誤一致を除外した結果。

- **構成**: 偽境界 **43**（V86 C 節＝無参照。x87/即値バイトが ret/jmp に見えた）+ interior **4**（前台帳から <16B。0x73863C / 0x73EF3B / 0x990D10 / 0x9A9D50 = V87 interior 4 と同一集合）。うち v86-C 45 / v86-A 1（0x990D10）/ v86-B 1（0x9A9D50）。
- **帯域分布**:

  | 帯域 | 47 中 |
  |---|---:|
  | 0x401000–0x600000 | 8 |
  | 0x600000–0x800000 | 27 |
  | 0x800000–0x980000 | 10 |
  | 0x980000–0x9C0000 | 2 |

- **判定**: これらの 47 は **V86 が自己申告していた boundary false positives の実体**であり、「本物の関数エントリだが Cat-3 が取りこぼした」VA ではない（= 実質的に未検出エントリは 0）。Cat-3 側が正しい。seed-B 133（pad 非依存）は逆に **V86 が構造上検出不能**だった分で、L4 ⊃ L6a として補完している。

---

## (6) 成果物・証跡（docs 編集なし）

| 種別 | パス / 内容 |
|---|---|
| **本体（新規）** | `C:\Users\hiroki.kogarumai\Oblivion_Android\tmp\re\v155_label_ledger_union.md`（UTF-8） |
| **統合 CSV（新規・全行）** | `%TEMP%\opencode\v155_union.csv`（31,905 行 + ヘッダ。列 = `VA,func_list_name,dec_header,V86,Cat3,Cat3b,seedB,I_no,notes`） |
| スクリプト（新規・%TEMP%） | `v155_build_union.py`（本体パイプライン）/ `v155_named83.py`（767vs684 明細）/ `v155_locate47.py`（未収録 47 帯域） |
| 中間出力 | `%TEMP%\opencode\v155_union_summary.txt` |
| 読取のみの参照 | `func_list_new.txt`・`v133_dup_results.csv`（無改変）、`docs/Oblivion_decompiled.c`（ヘッダ 1 回走査のみ）、`tmp/re/v86_unlabeled_scan.txt`、`tmp/re/v127_labelless_cat3.md`、`tmp/re/v136_doc_append.md`、`tmp/re/v96_gap_instances_crosscheck.md`、`tmp/re/v137_v80_global_residuals.md`、`tmp/re/v_data_relmap.md`（§F7）、`%TEMP%\opencode\v127_scan.py`（逐語参照）、`%TEMP%\opencode\probe_harness.py` |
| 検証 | seed A=33,098 / seed B=17,921 / raw union=38,375 / Cat-3=10,261（seed 9,761/367/133）/ Cat-3b=365 / V86=10,175 / I1–I10 L4=10/10 が v127 既知値と**完全一致** |
| 未実施（禁止事項） | docs 本体編集、git、TODO/session-memory/plan/func_list/v133 CSV の編集、既存 tmp/re 上書き |

再現: `python "%TEMP%\opencode\probe_harness.py" selftest`（4/4）→ `python "%TEMP%\opencode\v155_build_union.py"`。

---

## (7) 新規残点（V156+ 起票粒度）

| # | 提案 V-id | 優先 | 内容 |
|---|---|---|---|
| S1 | **V156: docs の名前付きヘッダ件数の訂正（684→767 / 21,149→21,232）** | 中 | 本タスクで数え方差と確定（v90 identifier-only 正規表現が backtick RTTI 50 + FID_conflict 20 + デコレータ 13 = 83 を漏れ）。`RE_EXE_REPORT.md` 等の該当表記を訂正。docs 編集を伴うため承認制（v127 R1/V143 と同根） |
| S2 | **V157: L1&~L2 = 1,132 の E8 宿主帰属シミュレーション** | 中 | Cat-3b 365 が enclosing 帰属を狂わせる実害を定量化（v137 典型例 0x5101E0 型。dec_index-only enclosing を使った E8 辺の誤帰属率を全 1,132 で測る）。統合表の `L1&~L2` フィルタが入力 |
| S3 | **V158: 6 層統合テーブルを round_verify / 台帳クロスチェックへ接続** | 低 | 本テーブル（31,905 VA）を今後の func_list 追記候補（V136 §3・V140 66 件・seed-B 133・I1–I10）の単一ソース化。追記候補の VA が必ず本表に在ることを assert 化 |

---
生成物: `tmp/re/v155_label_ledger_union.md`（新規）+ `%TEMP%\opencode\v155_union.csv`（新規・31,905 行） / 生成スクリプト: `%TEMP%\opencode\v155_build_union.py`, `v155_named83.py`, `v155_locate47.py` / 前提: `probe_harness.py selftest` 4/4 PASS

---
---

# 追記（V145 継続セッション・2026-10-27 12:32〜）: 独立再検証と V140/V136 突合

既存本体（14,883 B / 187 行）を**読取のみ**して継続。本節は上書きせず追記する。再走査ではなく**既存成果物（統合 CSV / v86 scan / v133 CSV / v127 出力 / cache）を入力にしたマージ検証**（ESM 再スキャンなし・decompiled.c はヘッダ正規表現 1 回のみ）。

## (8) 独立再検証（本ラウンドの機械チェック全件）

### 8-1 統合 CSV の全項目再計算 — **61/61 PASS**（`%TEMP%\opencode\v145_verify_union.py` → `v145_verify_out.txt`）

統合 CSV を**一切信頼せず**6 層集合を列から再構築し、本体 md の全数値と照合:

- 行数/VA 一意 **31,905 / 31,905**、層サイズ 7 件（21,597 / 20,465 / 10,175 / 10,261 / 365 / 133 / 10）全一致。
- pairwise overlap **21 組全一致**（L1∩L2=20,465、L3∩L4=10,128、L1∩L5=365、L4∩L6a=133、L3∩L6b=10、他ゼロ）。
- membership パターン **7 種全一致**（L1+L2=20,465 / L3+L4=10,118 / L1 のみ=767 / L1+L5=365 / L4+L6a=133 / L3 のみ=47 / L3+L4+L6b=10）。
- 部分集合関係: **L2 ⊂ L1（差 0）/ L6a ⊂ L4（差 0）/ L5 = L1&~L2 の Cat3b=1（365/365）**。

### 8-2 名前付きヘッダの完全再走査 — **11/11 PASS**（`v145_named_x.py` → `v145_named_xcheck.txt`）

`docs/Oblivion_decompiled.c` をヘッダ正規表現で 1 回再走査（本文走査なし）:

| 検査 | 実測 | 期望 |
|---|---:|---:|
| `// FUN_*` ヘッダ（L2） | 20,465 | 20,465 |
| 名前付き（非 FUN）ヘッダ | **767** | 767 |
| 全ヘッダ | 21,232 | 21,232 |
| 台帳 non-FUN 名義 | **767** | 767 |
| named-only / ledger-only / 対称差 | 0 / 0 / **0** | 0 |
| FUN ヘッダ ⊄ 台帳 | 0 | 0 |
| 台帳 FUN でヘッダ全無し（L5） | **365** | 365 |
| v90 identifier-only 正規表現の名前付き | **684** | 684 |

→ **767 vs 684 は本ラウンドでも独立再現**（§2 の裁定が再確認）。

### 8-3 V86 層と A 節 214 の突合（`v145_v86_set.py` / `v145_supp.py` → `v145_v86_setcheck.txt` / `v145_supp_out.txt`）

- `v86_unlabeled_scan.txt` の A∪B∪C 10,175 VA と統合 CSV の `V86=1` 行集合は **対称差 0（完全一致）**。
- **A 節 214 件: L3 収録 214/214・L4（Cat-3）収録 213/214・台帳混入 0/214**。L4 に落ちる 1 件は `0x990D10`（dist<16 の interior = §5 の interior-4 の 1 件）のみ。
- L3-exclusive 47 の内訳を再現: **V86 節 C=45 / A=1（0x990D10）/ B=1（0x9A9D50）**、interior-4 は **4/4 すべて 47 の中に居る**、帯域 8/27/10/2 — 本体 §5 の記載と**完全一致**。

### 8-4 probe 証跡（`v145_probe.py` → `v145_probe_out.txt`）

| VA | 台帳 | 先頭命令（capstone 実測） |
|---|---|---|
| `0x00446CB0` | **有** | `push esi ; push edi ; mov edi,[esp+0xc] ; test edi,edi` |
| `0x005101E0` | **有** | `sub esp,0x208 ; mov eax,[0xb30aac] ; xor eax,esp`（= v137 欠落典型例だが台帳には存在） |
| `0x00A10EB0`（I1） | 無 | `mov ecx,0xb430d8 ; call 0x7b7250`（= v136 I1 の根拠 `call 0x7b7250` と一致） |
| `0x00990D10` | 無 | `fld xword ptr [esp+0x10] ; fld xword ptr [esp+4] ; …` |
| `0x00983DDB` | 無 | `push esi ; push [esp+8] ; mov esi,ecx ; call 0x983daa` |

---

## (9) V140「新規 66 件」と統合表の突合（本ラウンド新規）

`v140_e8_66_finalization.md` §1-3 の 66 行（確定 60 / 保留 6）を統合 CSV に照合（`v145_cross_v140.py` → `v145_cross_out.txt`）:

- **55/66 が統合表に収録**（全件 `Cat3=1`・L1/L2 は 0 = すべて無名 gap であることを再確認）。
- **11 件が統合表に無い**。拒否理由を境界条件で実測（`v145_reject_probe.py` → `v145_reject_reasons.txt`）:

| 拒否理由 | 件数 | 該当 VA |
|---|---:|---|
| `no-boundary-term`（pad 直前に terminator 無し） | **8** | `0x4672D0` `0x4F65B0` `0x4F7750` `0x4F7800` `0x4F78B0` `0x4F7EF0` `0x983DC3` `0x983DDB` |
| `dist<16`（前台帳開始点から 16B 未満 interior） | **3** | `0x98347E`(dist5) `0x988837`(dist10) `0x99C8B1`(dist13) |

- **うち 6 件は V140 の「保留 6 件」と完全一致**（0x4672D0 / 0x4F65B0 / 0x4F7750 / 0x4F7800 / 0x4F78B0 / 0x4F7EF0 = V144 の対象）。残る 5 件（`0x98347E` `0x983DC3` `0x983DDB` `0x988837` `0x99C8B1`）は V140 で**確定**だが Cat-3 の 5 条件（boundary-term / dist≥16）では落ちる。
- **全 11 件が E8 ターゲット**（`tmp/re/cache/call_targets.json` で 11/11 `calls>=1`）→ つまり seed-B では生じるが `is_cat3()` のフィルタで落ちる群である。**統合表の既知限界**（L4 は Cat-3 採用分のみを収録するため、V96/V140 の E8 直接列挙系は L4 に含まれない）を実測で特定した。
- V86 節との重複: **66 件中 V86 収録 0 件**（66 件は `adjacent`/`MID-INSN` 形状で `ret+CC` 境界規則に掛からない）— v140 の「現行規則では不可視の新規候補」記述と整合。

→ 解釈: 統合表は「6 層の突合」を**定義通り**完成させているが、**第 7 の入力源（V96/V140 の E8 直接列挙 = 規則非依存）は層として未統合**。既知の未収録は **11 件**（6 保留 + 5 確定）で、これが本タスクで特定された唯一の実質ギャップ。

## (10) V136 別紙（RE_EXE_REPORT §70.16 (4c)）の裏づけ [docs 未編集・提出のみ]

`v136_doc_append.md` §2/§3 の (4c) 表・台帳追記 10 行を統合 CSV で機械照合:

- **台帳 0/10・dec_index 0/10**（I1–I10 の `func_list_name` / `dec_header` は全行空）・**V86 10/10・Cat-3 10/10** — (4c) 表の記載と完全一致。
- **pad 値 10/10 一致**: 12/12/1/14/11/3/2/6/5/8（I1→I10、CSV `notes` 列 vs v136 §3-B 表）。
- **V86 節 10/10 一致**: I2 のみ A 節、他 9 件は B 節（CSV `notes` の `v86-A`/`v86-B` vs v136 §3-B「V86」列）。
- seed は I2 のみ `A+B`（CRT `0x98780E` からの E8）、他 9 件は `A` — v136/本体 §3 と一致。
- → **(4c) 根拠データの提出は以上。docs 編集（(4c) 挿入）は本タスク禁止事項のため未実施**（V144 の適用待ちステータスに変更なし）。

---

## (11) 成果物の更新（本ラウンド）

| 種別 | パス / 内容 |
|---|---|
| **本体（追記のみ・上書きなし）** | `C:\Users\hiroki.kogarumai\Oblivion_Android\tmp\re\v155_label_ledger_union.md` |
| **統合 CSV（同名成果物として tmp/re に配置・新規）** | `C:\Users\hiroki.kogarumai\Oblivion_Android\tmp\re\v155_label_ledger_union.csv`（**1,507,687 B / 31,905 行 + ヘッダ**、`%TEMP%\opencode\v155_union.csv` と **SHA256 完全一致** `750b3607…`） |
| **1,132 件の実列挙（v137 残点の充足）** | `%TEMP%\opencode\v145_missing1132.csv`（**1,132 行 + ヘッダ** / 42,997 B。列 `VA,func_list_name,dec_header,Cat3b`。内訳 = 名前付きヘッダ 767〔thunk 100 / rtti-backtick 50 / FID_conflict 20 / plain 597〕+ ヘッダ全無し 365。`FUN_00510150` `FUN_005101e0` を収録） |
| 中間スクリプト（%TEMP%） | `v145_verify_union.py`（61 checks）/ `v145_named_x.py`（11 checks）/ `v145_v86_set.py` / `v145_cross_v140.py` / `v145_supp.py` / `v145_reject_probe.py` / `v145_probe.py` |
| 中間出力 | `v145_verify_out.txt`（5,239 B）/ `v145_named_xcheck.txt`（1,205 B）/ `v145_v86_setcheck.txt` / `v145_cross_out.txt` / `v145_supp_out.txt` / `v145_reject_reasons.txt`（1,742 B）/ `v145_probe_out.txt` |
| 読取のみ | `v155_union.csv`（前セッション生成）・`v86_unlabeled_scan.txt`・`v136_doc_append.md`・`v140_e8_66_finalization.md`・`v137_v80_global_residuals.md`・`cache/call_targets.json`・`cache/named_funcs.txt`・`func_list_new.txt`・`docs/Oblivion_decompiled.c`（ヘッダ 1 回）・`TODO_re49.md`（serial 145 行） |
| 禁止事項の遵守 | docs / TODO / 台帳 / git / Supermemory / plan / 既存 tmp/re の上書き **すべて未実施**。ESM 再スキャン **なし** |

## (12) 結論（3行）

1. 6 層（func_list 21,597 / dec_index 20,465 / V86 10,175 / Cat-3 10,261 / Cat-3b 365 / seed-B 133）の統合住所テーブル **31,905 VA** を 1 本の CSV に確定し、本ラウンドの独立再計算 **61/61・ヘッダ再走査 11/11・V86 集合差 0** で全数値を再現した。
2. 名前付きヘッダ **767 vs 684 は「数え方差」**（v90 の identifier-only 正規表現が backtick RTTI 50 + FID_conflict 20 + デコレータ 13 = 83 を漏れる）と、再走査でも確定。**v137 の欠落 1,132 件は「名前付き 767 + ヘッダ全無し 365」**として実ファイル列挙した。
3. V136 (4c) の 10 件（pad・V86 節・0/10 台帳）を 10/10 突合して提出。唯一の実質ギャップは **V96/V140 の E8 直接列挙系 11 件（確定 5 + 保留 6）が層として未統合**である点で、これは本タスクで初めて定量特定した。

## (13) 残留不明点

- 上記 11 件（第 7 入力源）を層 L7 として統合するか否か — V144（保留 6 件裁定）/ V152（Cat-3 三層差分）と論点が重なるため起票のみ。→ `S4` として (7) に下記追記。
- `0x983DDB` / `0x983DC3` は `pad=0x00 len=1` で `term=None`（直前が `00` ラン + terminator 判定不能）— 2 件のみ Cat-3 の boundary-term 条件の副作用で落ちており、偽陽性か真の gap かは未裁定。
- 名前付き 767 のうち `thunk_*` 100 件と `FID_conflict` 20 件の実体（Ghidra 側の命名規約）は本タスクのスコープ外。

## (14) 自己検証

- `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl。着手時・完了時の 2 回確認）。
- 内部整合: 統合 CSV 再計算 **61/61 PASS** / ヘッダ完全再走査 **11/11 PASS** / V86 集合 **対称差 0** / A 節 214 の L3 収録 **214/214**・L4 **213/214**（欠落 1 は interior-4 の `0x990D10` のみ）/ V140-66 収録 **55/66**（欠落 11 = 保留 6 + 確定 5、拒否理由 8+3 を全件実測）/ I1–I10 の台帳 0/10・pad 10/10・V86 節 10/10 / 1,132 列挙 **1,132 = 767 + 365 = 1,132 行**。
- `python "%TEMP%\opencode\round_verify.py"` → **ALL OK**（成果物 UTF-8 / 編集対象 docs 無改変 / TODO 432 行のまま）。

## (15) Android 移植への示唆

移植層では「関数エントリの正」を 1 本の住所キーで引けることが E8 辺の宿主帰属・ログ対応表・スタブ実装のレビュー全般を安定させる。本統合テーブルを単一ソースとすれば、ラベル無し gap（10,308 VA）由来の外部呼び出しは「台帳無し = 未実装」ではなく「Ghidra 未ラベル（V86/Cat-3 で実在証明済み）」として区別でき、ポート時は func_list 追記候補（I1–I10・seed-B 133・V140 確定分）を優先スタブ化してから残りを `unimplemented(name, va)` ログに落とせる。特に dec_index 欠落 1,132 のうちヘッダ全無し 365 件はデコンパイル本文すら無いので、移植の参照元は本 CSV の `dec_header` 空列をマーカーにできる（実装は行わない）。

### (7)-追記 S4（本ラウンド起票）

| # | 提案 V-id | 優先 | 内容 |
|---|---|---|---|
| S4 | **V159: 第 7 入力源（V96/V140 E8 直接列挙）の層化 L7** | 中 | 統合表に未収録の 11 件（確定 5: `0x98347E` `0x983DC3` `0x983DDB` `0x988837` `0x99C8B1` / 保留 6 = V144 対象）を `L7=V96E8` 列として CSV に追加し、31,905 → 31,916 へ。全 11 件は `call_targets.json` で E8 ターゲット実測済み。V144/V152 と論点共有 |

---
追記生成物: `tmp/re/v155_label_ledger_union.md`（追記・合計 **26,996 B / 313 行**）+ `tmp/re/v155_label_ledger_union.csv`（新規・1,507,687 B / 31,905 行）+ `%TEMP%\opencode\v145_*.py`（7 本）・`v145_missing1132.csv`（1,132 行） / 検証: selftest 4/4 PASS・61/61・11/11・round_verify ALL OK
