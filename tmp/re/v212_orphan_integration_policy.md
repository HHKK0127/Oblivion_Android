# v212 — 正本未参照の孤児成果物 統合方針と handover アーカイブ明示（TODO V203 の答申ドラフト）

- 作成: 2026-10-31（分析専任サブエージェント / TODO serial 203・V203・低優先）
- 出典: `tmp/re/relcheck_20261029/rel3_reference_drift.md`（§1-C / §2 / §3 / §4、スナップショット 2026-10-29 14:22:48）
- 入力（読取専用）: `docs/RE_EXE_REPORT.md`（見出し索引抽出のみ）、`tmp/re/*.md` 279件（top-level）、`tmp/re/TODO_re49.md`（V168・V203 行）、`tmp/re/handover_20261025/`（構成のみ）
- 再計算スクリプト: `%TEMP%\opencode\v212_scan.py` / `v212_scan2.py`（出力 `%TEMP%\opencode\v212_orphans2.txt`、見出し索引 `v212_headings.txt` / `v212_headings70.txt`）
- **本ファイルは政策ドラフト。docs/・GT・TODO・handover・既存 tmp/re ファイルへの適用は行わない（適用は親セッションの承認後）。**

---

## §0. 前提と数値の扱い

| 項目 | 値 | 出所 |
|---|---|---|
| rel3 時点の孤児数 | 173/263（65.8%、計 4,352,931 B） | rel3 §3（実測） |
| 本日（2026-10-31 22:11）の再計算孤児数 | **189/279** | v212_scan 再計算（実測） |
| 差分 +16 の内訳 | スナップショット後（10-29 14:22 以降）に生成された実新規 **約16件**（v192・v193・v193b・v195・v197-v208 系）＋ 就地更新による境界ずれ約1件の誤差。正本側の新規参照化は 0 件相当（正本の最終更新は 10-31 の V206 1文字修正のみ） | [推定] 差分解（189-173=16 と mtime>スナップショット 17件からの按分） |
| 分類対象 | rel3 ベースライン 173 件を代表する **再計算 189 件**（超集合。rel3 当時の 173 件は本分類の内側にほぼ含まれる） | — |

以降、件数は再計算 189 件ベースで記載し、rel3 ベースラインとの対応を明示する。

---

## §1. 173件の3分類（政策提案）

### §1.1 分類ルール（機械適用・再現可能）

1. **(c) アーカイブ化** — ①完了/重複スナップショット（`*completed_archive*` / `*.A_completed_*` / `*duplicate_override*`）、②同一系統（接頭辞 v/s/b + トピック語が同一）で新版が存在する旧版（superseded）、③旧TODO断片・時代差（`TODO_re49_new.md` / `v4v9_unresolved_report.md`）。
2. **(b) 一次資料（現状維持）** — ①監査・照合・台帳・報告系の命名（crosscheck / audit / scan / inventory / ledger / contract / cheatsheet / relmap / verify / contradictions / review_batch / ci_fail / scope / design_spec / findings / *_report 等）、②2026-10-26 以前生成のノート（V136–V167 適用バッチ〔v167_v144_apply_log.md 等〕で内容の大半が正本・GT・b系へ反映済み。証跡として保持、引用は TODO 行経由で足りる）。
3. **(a) 正本へ統合すべき** — 上記以外（＝2026-10-26 以降生成の、残差・仕様・対応表など固有知見を含む未統合ノート）。正本の該当節への追記候補。

※ (b)② の「反映済み」判定は適用ログの存在に基づく **[推定]**（ファイル単位の全数突合は未実施。反証があれば (a) へ繰り上げ）。

### §1.2 分類件数（再計算 189 件）

| 分類 | rel3当時相当（mtime≤10-29 14:22） | スナップショット後の新規 | 合計 | 実測サイズ |
|---|---|---|---|---|
| (a) 正本へ統合 | 64 | 16 | **80** | 2,647,316 B（約2.5MB） |
| (b) 一次資料 現状維持 | 94 | 1 | **95** | 1,679,796 B（約1.6MB） |
| (c) アーカイブ化 | 14 | 0 | **14** | 414,253 B（約0.4MB） |
| 合計 | 172（≒rel3 の 173） | 17 | 189 | 4,741,365 B（rel3 時点 4,352,931 B から成長） |

### §1.3 (a) 統合候補 上位20件と節番号案

正本は現状 §1〜§70（§70 = v60〜v76 統合節、最終小節 §70.16 (5) 残作業 = L12342）。§71 以降は未整備。節番号案は「既存節への追記」か「新設 §70.x / §71」を明示する。

| # | ファイル | size(B) | mtime | 統合先 節番号案 | 備考（V168 管轄か） |
|---|---|---|---|---|---|
| 1 | `b13_nonrtti_vtable_classes.md` | 83,348 | 10-28 | §20 に新小節「非 RTTI クラスの vtable 同定表」（§20/§23 の補強） | b13 本体行は V144 で b系訂正済み。表の本体統合は未着手 |
| 2 | `v161_quest_extra_residuals.md` | 79,379 | 10-28 | §58（クエスト）＋ §69.5（残課題）への追記 | 未着手 |
| 3 | `v126_bind_crt_bootstrap.md` | 66,557 | 10-26 | §44（CRT 数値ヘルパ）に新小節、または新設 §70.17 | §35.5 atexit 訂正は V144 で適用済み・残余が対象 |
| 4 | `v121_gt_corrections.md` | 62,580 | 10-28 | §41.3（GT 訂正）残余＋ GT §4.1.1 | **V168 管轄**（A–D 裁定は V158 で確定、GT/docs 確定反映待ち） |
| 5 | `v157_runtime_residuals.md` | 53,236 | 10-27 | 新設 §70.17「v12x–v19x ランタイム残差統合」 | 未着手 |
| 6 | `v139_b17_table2_residuals.md` | 53,131 | 10-27 | §70.13／§70.14（opcode 正典表・b17 Table2） | 未着手 |
| 7 | `b23_evaluator_sizes.md` | 51,675 | 10-27 | §9（Expression::Eval）／§51–§52（式評価器仕様） | 未着手 |
| 8 | `v158_slot_opcode_map.md` | 48,921 | 10-27 | §67（MISS 台帳）／§70.14（slot–opcode 対応表） | 台帳反映分は **V168 管轄**（H1–H7）、文書化は未着手 |
| 9 | `b21_speculative_backing.md` | 48,419 | 10-26 | §20（RTTI 証拠・推定裏付けの整理） | 未着手 |
| 10 | `v134_forwarder_residuals.md` | 46,536 | 10-27 | §70.16（forwarder / FUN ラベルなし） | 未着手 |
| 11 | `v190_graphics_residuals.md` | 45,544 | 10-29 | §11（シェーダ）／§62（描画パイプライン） | 未着手 |
| 12 | `v138_b16_alarm_residuals.md` | 45,168 | 10-27 | §59.3（b16 mask 訂正）残余 | §59.3 2件は V144 で訂正済み・残余が対象 |
| 13 | `v159_renderer_residuals.md` | 43,532 | 10-27 | §61／§62（レンダラ） | 未着手 |
| 14 | `v164_esm_gate_spec.md` | 43,494 | 10-28 | `docs/RE_ESM_REPORT.md`（ROADMAP L42 の予定節）へ | N8 台帳2行+慣例注記は **V168 管轄** |
| 15 | `b26_nontt_class_names.md` | 43,287 | 10-26 | §20／§23（クラス名同定） | 未着手 |
| 16 | `v184_engine_small_residuals.md` | 43,151 | 10-29 | §70.x 小残差（または新設 §70.18） | §4 訂正12件は **V168 管轄**・残余のみ |
| 17 | `v160_ini_log_residuals.md` | 42,808 | 10-27 | §12（Oblivion_default.ini 解析） | 未着手 |
| 18 | `v183_renderer_stream_residuals.md` | 42,763 | 10-29 | §61（ストリーミング） | §11-4(s61/s81/v129) は **V168 管轄**・残余のみ |
| 19 | `v162_gap_residuals.md` | 42,517 | 10-28 | §70.16 (4c) 周辺（gap 実例） | 未着手 |
| 20 | `v177_gt_slot_residuals2.md` | 42,473 | 10-28 | GT §4.1.1／§2 ＋ docs §41.3 | GT ドラフト4点は **V168 管轄**（②）・正本転記は適用後の残余 |

(a) 全80件のうち **14件は V168 の追記拡充リストに既出**（v164・v166・v170・v171・v175・v176・v177・v181・v182・v183・v184・v186・v187・v188 — TODO_re49.md L581 の追記 ①② に対応）。残り約66件が V168 とは無関係の新規統合束である。

### §1.4 (b) 一次資料 95件（現状維持・上位のみ）

`v133_full_dup_scan.md`(71KB)・`v180_letter_rows_audit.md`(38KB)・`memory_inventory.md`(37KB)・`v_data_relmap.md`/`v_data_relmap2.md`(33/29KB)・`v189_esm_ledger_residuals.md`(32KB)・`GT_CHEATSHEET.md`(19KB)・`v170_ledger_granularity.md`(21KB)・`v166_eh_funclet_ledger.md`(20KB)・`v179_csv_crosscheck.md`(13KB)・`SUBAGENT_CONTRACT.md`・`ci_fail_analysis_20261028.md`・`v_review_batch1_20261027.md`、および 10-26 以前の s49–s102 系統合ノート（`s69_hc92.md` 79KB ほか約45件）と v13–v97 系旧ノート（`v97_forwarder_family_map.md` 51KB ほか約30件）。
→ **方針: 移動も削除もしない。正本からのリンクは増やさず、TODO_re49.md の各行が出典ポインタとして機能する現状を維持する。**（例外検討: `GT_CHEATSHEET.md` は正本 §70.11 証跡級の1行リンクを受ける価値あり＝次回 docs バッチの任意項目）

### §1.5 (c) アーカイブ化 14件（全数）

| ファイル | size(B) | 理由 |
|---|---|---|
| `TODO_re49_completed_archive.md` | 193,654 | 完了タスクの旧スナップショット（正本は現行 `TODO_re49.md`） |
| `s56_section.md` / `s50_section.md` / `s49_section.md` / `s55_section.md` / `s57_section.md` / `s58_section.md` / `s59_section.md` / `s60_section.md` | 12–39KB | section 型旧ノート。後続の s56_*/s57_*/s58_* 個別ノートおよび正本 §56–§60 統合済み（superseded） |
| `s75_camera.md` | 8,820 | `s102_camera.md`（10-21）に置換済み |
| `v184_engine_small_residuals.A_completed_1200.md` | 35,936 | subagent 完了報告の重複スナップショット（本体 v184 と同内容） |
| `v182_asset_integration.A_completed_0748.md` | 26,322 | 同上（本体 v182 と同サイズ・同時刻） |
| `v182_duplicate_override_note.md` | 7,443 | 重複上書きの説明メモ（v182 本体に吸收済み） |
| `TODO_re49_new.md` | 5,171 | 旧TODO断片（現行 TODO_re49.md へ吸収済み） |
| `v4v9_unresolved_report.md` | 10,596 | v4/v9 期の未解決一覧（v5x–v20x で大半解消・時代差） |

→ **方針: 物理移動はしない（正本・GT からのリンクは元々無い）。各ファイル冒頭に「アーカイブ: 現行正本は〜」の1行を追記する（tmp/re 配下のため編集可・要親承認）。** s*_section 系・s75_camera は本文改編ではなく本方針書 §1.5 の一覧で宣言するだけで足りる（低価値のため1行追記は任意）。

---

## §2. handover_20261025/ のアーカイブ明示方針

### §2.1 裁定: **ディレクトリ内 README を主とし、正本側は次回 docs 承認バッチで1行注記を追加する（二段構え）**

| 手段 | 裁定 | 理由 |
|---|---|---|
| ① handover ディレクトリ内 README 追加（`handover_20261025/README.md`） | **主・先行実施（ドラフト §2.3）** | handover を直接開く作業者に警告が届くのはディレクトリ内注記のみ。docs 外（tmp/re 配下）のため承認ゲートが軽い。rel3 §2 で判明した「対応先なし13件」の所在リストもここでしか示せない |
| ② 正本 `docs/RE_EXE_REPORT.md` §1 概要への注記 | **不採用** | 正本冒頭は成果物全体の概要であり、単一スナップショットの寿命注記には不釣り合い。正本の潔癖性を損なう |
| ③ 正本の該当箇所（§70.11 証跡、および L11923 `tmp/re/probe_harness.py` 参参照行の隣）への1行注記 | **採用・次回 docs バッチ（V168 相当）で追加** | 正本経由で handover を知る読者に「handover は 10-25 スナップショット・live tmp/re を優先」を届ける最小手段。docs 編集は要承認のため本タスクではドラフトのみ |

### §2.2 補足事実（rel3 §2 実測）

- handover 41件のうち live と MD5 一致は 26件（63.4%）、陳腐化（不一致+対応先なし）15件（36.6%）。
- **live に実体がない13件**: `MANIFEST.txt`（疑似対応・別用途ファイル）、`ledgers/v70_force_create.csv`、`ledgers/v79_bodies.json`、`ledgers/v79_classified.csv`、`scripts/probe_harness.py`、`scripts/probe_last_report.txt`、`scripts/round_verify.py`、`scripts/scan_new_artifacts.py`、`scripts/v_todo_round14.py`、`scripts/v_todo_round15.py`、`scripts/v_todo_round16.py`、`session_A_summary.md`、`session_B_summary.md`。
- うち **`scripts/probe_harness.py` は正本 L11923 が参照する手法資料**で、handover 配下にしか存在しない（真の dangling と同一の構造的欠落）。アーカイブ宣言と併せてこの1件の live への復元（handover→tmp/re 直下へのコピー、親承認後）を推奨する。
- handover は静的スナップショットではない（`scripts/probe_last_report.txt` の handover 側 mtime が 10-27 に更新されている＝並行セッションの書き込み痕跡）。したがって README 文言は「凍結宣言」ではなく「**正本照合には live `tmp/re/` を直接参照せよ（照合基準: rel3 2026-10-29 14:22 スナップショット）**」とする。

### §2.3 README ドラフト（実装は親セッション承認後。配置先 `tmp/re/handover_20261025/README.md`）

```markdown
# handover_20261025 — セッション引き継ぎアーカイブ（2026-10-25 時点スナップショット）

**これは 2026-10-25 時点の逆転工程セッション引き継ぎ資料のスナップショットです。**

- **正本照合には live `tmp/re/` を直接参照してください。** handover ⇔ live の全数 MD5 照合
  （rel3 / 2026-10-29 14:22 スナップショット）では 41件中 26件のみ一致（63.4%）。
  陳腐化 15件（36.6%）の内訳は `tmp/re/relcheck_20261029/rel3_reference_drift.md` §2。
- 照合時点で **live に実体がない13件**（本ディレクトリのみに生存）:
  MANIFEST.txt（※別用途の `lodnif/manifest.txt` とは無関係）/ ledgers: v70_force_create.csv,
  v79_bodies.json, v79_classified.csv / scripts: probe_harness.py, probe_last_report.txt,
  round_verify.py, scan_new_artifacts.py, v_todo_round14-16.py / session_A_summary.md,
  session_B_summary.md。
- 特に `scripts/probe_harness.py` は `docs/RE_EXE_REPORT.md` L11923 が参照する手法資料です
  （正本↔live の二重に見える欠落の同一原因。復元候補: V203 参照）。
- 本ディレクトリは「凍結」ではありません。並行更新の痕跡（probe_last_report.txt mtime 10-27）
  が確認されており、以降の差分は追跡していません。
- 統合方針全体: `tmp/re/v212_orphan_integration_policy.md`（V203）。
```

正本側1行注記ドラフト（§70.11「証跡」または L11923 近傍への挿入案）:

```markdown
- 注記【訂正 2026-10-XX】: `tmp/re/handover_20261025/` は 2026-10-25 時点のセッション引き継ぎ
  スナップショット（陳腐化 36.6%・rel3 §2）。照合・引用は live `tmp/re/` を優先すること。
  handover 内のみ生存の `scripts/probe_harness.py` を除く。
```

---

## §3. 優先順位付き実行計画（0.5–1人日。docs 編集は要承認のためドラフト先行）

| Step | 内容 | 規模 | 承認 |
|---|---|---|---|
| 1 | 本方針の承認取得（§1 3分類 / §2 handover 方針 / §4 V168 裁定） | 0.05人日 | **ユーザー承認** |
| 2 | handover README 配置（§2.3 ドラフトを `handover_20261025/README.md` へ）＋ 正本1行注記ドラフトを V168 の docs バッチ項目へ1行追加 | 0.1人日 | README: tmp/re 配下のため軽承認可 / 正本注記: 要承認・次回バッチ |
| 3 | (c) 14件のアーカイブ明示: ①`TODO_re49_completed_archive.md` / `*.A_completed_*` / `v182_duplicate_override_note.md` / `TODO_re49_new.md` の冒頭に「アーカイブ: 現行正本は `TODO_re49.md`」1行追記（.bak 付き・round_verify）、②`s*_section` 8件+`s75_camera` は本方針書 §1.5 の一覧宣言のみ（改編しない） | 0.15人日 | tmp/re 配下のため軽承認可（既存 tmp/re 上書きに当たるため親承認推奨） |
| 4 | (a) 残約66件（V168 管轄14件を除く）の TODO 起票: トピック束ごとに V-番号を1件ずつ割り、§1.3 の節番号案・出典行を付けたドラフト行を作成（docs 本体は無編集） | 0.3人日 | TODO_re49.md 追記は親承認後 |
| 5 | (b) 95件は方針書 §1.4 の一覧を正とするのみ。TODO 行は既に出典として機能しており追加作業なし（`GT_CHEATSHEET.md` の1行リンクのみ任意） | 0.05人日 | 不要 |
| 6 | 任意: `probe_harness.py` の handover→`tmp/re/` 復元（正本 L11923 の dangling 解消） | 0.05人日 | 要承認（正本参照パスへの追加ファイル配置） |

合計: 約 **0.7人日**（Step 1–5 の必須分）。Step 6 と s*_section の1行追記を含めても 1人日以内。
(a) 80件そのものの docs 本体への統合は本計画の対象外 — V168 適用後、§71 新設（「v9x–v20x 統合節」）または既存節追記を 2–3 バッチに分割して別途起票する（1バッチあたり 20–30件、要承認）。

---

## §4. V168【要承認】反映バッチとの関係 — 裁定: **合流させない（分離維持）**

V168（TODO_re49.md L581）= 「台帳追記反映バッチ（12,000行超）＋GT/docs 反映第2バッチ」。既に v181/v182 docs 転記ドラフト、GT/docs 訂正第2（v177・v171・v175・v176・v184・v186・v183・v188・v187）、台帳 H1–H7 裁定待ちを拡充含む。

1. **V203 の成果（本方針書・handover README・(c) 宣言・(a) 起票）を V168 に混載しない。** V168 は「用意済みドラフトを docs/GT/台帳へ一括適用する承認ブロック」（分析ゼロ・アンカー検証済み）であり、173件のトリアージは新規ドラフト生成を伴う別種の作業。混載すると単一承認の爆発半径が肥大しレビュー不能になる。
2. **V168 既出の (a) 14件（v164・v166・v170・v171・v175・v176・v177・v181・v182・v183・v184・v186・v187・v188）は V168 管轄のまま据え置き** — 本方針書はそれらを「V168 適用済み/適用予定」として節番号案の備考に記載するにとどめ、二重起票しない。
3. **本タスク（V203・低優先）は V168（高優先・承認待ち）をブロックしない。** Step 1–5 は V168 の承認可否と独立に進められる。
4. V168 の台帳追記（H1–H7）とは領域が重ならない（本タスクは docs 引用構造の政策であり台帳内容には触れない。V195/V207 の session-memory バックフィルとも重複しない — TODO L689 の注記と同じ関係）。

---

## §5. 制約・既知の限界（事実と推定の区別）

- **[実測]** 孤児判定は rel3 と同じ「ベースネーム部分文字列一致」方式（大文字小文字非区別）。本文中にファイル名が言及されない場合は孤児に残る（過剰気味）。
- **[推定]** (b)②「10-26 以前のノートは反映済み」は V136–V167 適用ログの存在根拠。ファイル単位の全数突合は未実施。反証があれば (a) へ繰り上げ。
- **[推定]** rel3 ベースライン 173 件と再計算 189 件の差 +16 は、ほぼ全件がスナップショット後の新規生成（v192–v208 系）だが、就地更新による境界ずれ約1件を含み得る（正本側の新規参照化は 0 件相当）。
- **[実測]** handover は並行更新が続く流動的なディレクトリ（probe_last_report.txt mtime 10-27）。§2.3 README は「凍結」ではなく「live 優先」を宣言する。
- 本ファイルは新規作成のみ。既存 `tmp/re/`・docs・GT・TODO・handover・session-memory への書き込みは一切行っていない（スクリプトは `%TEMP%\opencode/` に配置）。

— 以上。承認後、Step 2–4 を親セッションで実施すること。
