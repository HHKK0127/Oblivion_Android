# V211: graphify グラフ資産の網羅性方針（FUN 46.7%・後半欠落の裁定）

- 作成: 2026-10-30 / タスク: serial 202 相当（TODO V202・**低**）/ 役割: **分析専任 — 方針裁定のみ（実行しない）**
- 出典: `tmp/re/relcheck_20261029/rel4_graphify_join.md`（rel4、2026-10-29 14:44）、`graphify-out/graph.json`（読取・集計のみ）、`%TEMP%\opencode\func_list_new.txt`（21,597 行）、`tmp/re/v155_label_ledger_union.md`（6 層統合表）、`docs/Oblivion_decompiled.c`（読取のみ）
- 手法: Python 集計スクリプト `%TEMP%\opencode\v211_agg.py` / `v211_spot.py` / `v211_quick.py`（graph.json 全ロード・台帳・ヘッダ走査）、`v211_tsp.py`〜`v211_tsp4.py`（`tree_sitter_c` 単体解析。**graphify 本体は起動していない**、graphify 同梱 venv の Python と tree-sitter ライブラリのみ使用）
- 制約遵守: git 不使用 / docs・GT・TODO・session-memory・plan・graphify-out 編集なし（読取のみ）/ 既存 `tmp/re` 上書きなし（本ファイルのみ新規）/ **graphify 再生成コマンドの実行なし**
- 集計 JSON: `%TEMP%\opencode\v211_summary.json`

---

## 1. 事実確認（rel4 ⟷ 本実測の突合）

### 1-1 graph.json の現状（実測・2026-10-30 読取）

| 項目 | 実測値 |
|---|---:|
| nodes | 47,722 |
| links | 122,672 |
| communities | 1,153 |
| hyperedges | 0 |
| `built_at_commit` | `19fafb7f0117ea6332fc6d8f8015e37111e7f65a`（rel4 記載 HEAD と一致） |
| FUN ノード（label `FUN_<va>()` 口径） | **9,734**（ID `docs_oblivion_decompiled_fun_<va>` 口径とも一致） |
| `docs/Oblivion_decompiled.c` 由来ノード | 9,843（rel4 と一致） |
| port ノード（`app/src/**`・third_party 除く） | 18,582（rel4 の移植 C++ 18,391 + JVM 191 = 18,582 と一致） |
| third_party ノード | 12,277（rel4 と一致） |
| FUN ノードの `source_location` 最大 | L983,548（`FUN_00981db6()`） |
| decompiled 由来全ノードの行最大 | L983,600（`type_info()` ノード） |

### 1-2 rel4 記載との突合表

| 検査項目 | rel4 記載 | 本実測 | 判定 |
|---|---|---|---|
| nodes / links / communities | 47,722 / 122,672 / 1,153 | 同値 | ✅ 一致 |
| FUN ノード数 | 9,734 | 9,734（label・ID 双口径） | ✅ 一致 |
| 台帳行数 / 名前ユニーク | 21,597 / 21,489（重複名 22 種） | 21,597 行・VA ユニーク 21,597 / 名前ユニーク 21,489・重複名 22 種（108 行過剰） | ✅ 一致（口径差の正体＝重複名。下記 1-3） |
| 台帳 FUN パターン名 | 20,830 | 20,830 | ✅ 一致 |
| カバレッジ（台帳全名 / FUN 名） | 45.30% / 46.73% | 45.3%（9,734/21,489）/ 46.73%（9,734/20,830） | ✅ 一致 |
| グラフのみの FUN（台帳に無い） | 0 | **0**（graph ⊂ ledger） | ✅ 一致 |
| VA 整合 | 9,734 match / 0 mismatch | 9,734 match / 0 mismatch | ✅ 一致 |
| 抽出行の上限 | 983,600（FUN ノード loc 最大 983,548） | 行最大 983,600・FUN 最大 983,548 | ✅ 一致 |
| FUN 定義行（1 行 regex） | 19,593 | 19,593 | ✅ 一致 |
| L983,600 以降の定義 | 3,200 件が 0 収録 | 3,200 件・収録 0（追補: FUN ヘッダ口径では **3,211**） | ✅ 一致（口径差を追記） |
| L983,600 以前の収録 | 9,346 / 16,393（57.0%） | 9,346 / 16,393（57.0%） | ✅ 一致 |
| EXE⇔移植の構造エッジ | 0 本（888 跨ぎは型名衝突・`exit` 1 本は INFERRED 0.85） | 自前 port ⇄ decompiled は **1 本のみ**（`_exit()`⇄`.handler()` crash_handler.h、calls・**INFERRED 0.85**）、third_party 887（`uint`/`uchar` マージ）、doc 430 → AST/EXTRACTED 由来の意味的エッジ **0 本** | ✅ 一致（887+1=888 も整合） |
| ハブのコミュニティ分離 | FunctionID=36 / esm_reader.cpp=91 / Jolt.h=269 / oblivion_ini.cpp=2 | 同値 | ✅ 一致（`Renderer` は同名ノードが複数あり本検査では別ノードに一致したため対象外） |
| アドレス四分位カバレッジ | Q0 84.3% / Q1 59.8% / Q2 4.1% / Q3 0.0%（幅不等バケット 6,632/6,008/3,965/2,988） | VA 等件数四分位: **82.0% / 87.4% / 18.1% / 3.3%**（256KB VA 窓では 0x740000 境界で 90%→5% に急落、0x900000 以降 0% 帯） | ⚠️ 数値は非再現（バケット境界の定義が本再現と一致せず [推定]。rel4 の境界特定は未達）。「高 VA 帯で急落」の傾向自体は ✅ 一致 |

**突合総括**: rel4 の記載は本実測とほぼ全項目一致。唯一の非再現は四分位のバケット境界のみで、傾向判定には影響しない。

### 1-3 台帳の口径（21,597 vs 21,489）

- `func_list_new.txt` は `VA,FUNCNAME` 形式 **21,597 行**、**VA は全行ユニーク（21,597）**。
- **名前**ユニークは **21,489**。差 108 行の正体は**同名別 VA の重複名 22 種**（`` `scalar_deleting_destructor' `` 46 行、`CreateObject` 38 行、`FID_conflict:*` 系、`thunk_FUN_006ce350` 等）。
- これは v155 の「L1 = 21,597（VA ユニーク 21,597）」および rel4 の「重複名 22」と完全整合。
- **口径の結論**: 「21,597」＝**VA 行数**、「21,489」＝**名前ユニーク数**。どちらも正しく、混用しないこと（FUN パターン名は 20,830、残り 659 は import 等の固有名）。

### 1-4 欠落帯の実測（再確認）

- `docs/Oblivion_decompiled.c` = **1,066,496 行**（Python 実測。※PowerShell `Measure-Object -Line` は 956,447 と大きく出るため行数計測には使わないこと [差分は空行等、要因は未特定]）。
- FUN ヘッダ `// FUN_* @ 0x` = **20,465**（**v155 L2 と完全一致**）。うち **L983,600 以降は 3,211 件**、以前は 17,254 件。
- 1 行 regex の FUN 定義 = 19,593（rel4 と一致。multi-line シグネチャ定義が読み取れないためヘッダ数より少ない。graph 内 FUN のうち 388 件が本 regex で未捕捉 → シグネチャ行改行型と判明、抽出漏れではない）。
- 収録: L983,600 以前 16,393 定義中 **9,346（57.0%）**、以後 **0 / 3,200**（ヘッダ口径では 0 / 3,211）。
- つまり欠落は「後半 3,200 の全数欠落」＋「前半の約 43%（7,047 件）の散布欠落」の**二層構造**。

### 1-5 EXE⇔移植エッジの実測（再確認）

| 接続先カテゴリ | 本数 | 内容 |
|---|---:|---|
| third_party ノード | 887 | `uint`/`uchar` 等の Ghidra 型名が Jolt/doctest 側とマージ（context: parameter_type / return_type）。**型名衝突** |
| port（自前コード） | **1** | `_exit()` ⇄ `.handler()`（`app/src/main/cpp/engine/crash_handler.h`）、relation=`calls`、**INFERRED 0.85**（libc `exit` 名衝突） |
| doc（`docs/**` 等） | 430 | RE ドキュメント経由の参照（INFERRED が大半） |
| **AST/EXTRACTED 由来の意味的跨ぎ** | **0** | rel4 統計と一致 |

INFERRED エッジ総数は 431（doc 430 + port 1）。rel4 の結論「EXE⇔移植の構造エッジは 0 本」を**確認**。

### 1-6 【新規所見】欠落の真因は「抽出上限設定」ではなく tree-sitter 解析失敗（rel4 の [推定] を否定）

rel4 は欠落要因を「graphify の 1 ファイルあたりノード/トークン予算（~10k 上限）[推定]」としていたが、本タスクの実測で**否定**された。

1. **graphify 自身が解析エラーを記録している**: キャッシュ `graphify-out/cache/ast/v0.9.95-s25-g7154a062ba57/a7cf1082e914….json`（content-hash キー・23.4 MB・nodes 9,881 / edges 48,042）に `parse_errors = {"first_error_line": 1, "multiline_error": true}` が入っている。
2. **tree_sitter_c 単体再解析**（graphify 本体非起動）:
   - `function_definition` は **10,668 件のみ**、開始行の最大は **L983,600**、**L983,606 以降は 0 件**。
   - root node は **L1–EOF を覆う単一 ERROR ノード（28,995,463 B 全域）**。ERROR ノード計 13,961、`root.has_error = true`。
   - L983,600 は `void __thiscall type_info::~type_info(...)`、直後に `` // `scalar_deleting_destructor' @ 0x981e65 `` と続く RTTI/バッククォート帯で解析状態が失動 [解釈は推定]。
3. **つまり**: Ghidra デコンパイル構文（`__thiscall`、バッククォート RTTI 名、`undefined8` 等の Ghidra 型）を tree-sitter の C 文法が解析できず、エラー回復が L983,600 で限界を迎えている。解析は**決定論的**なので、**設定変更・再実行・キャッシュ無効化のいずれでも復旧しない**（同じ入力 → 同じ 10,668）。graphify の設定面を確認したが、この上限に相当する knob は存在しない（§2-A 参照）。
4. **認識率の全体像**: ヘッダ 20,465 に対し認識 10,668 = **52.1%**。うち graphify がノード化した FUN は 9,734（認識分の約 91%）。→ 欠落は「解析器能力の問題」として、**設定ではなく入力（ソース）側の扱い**でしか解決しない。

**分割解析実験**（本タスク・parse のみ／graphify 非使用。回収可能性の定量）:

| 分割サイズ | 認識関数 / ヘッダ 20,465 | 比率 |
|---|---:|---:|
| 分割なし（現状） | 10,668 | 52.1% |
| 50,000 行帯 | 17,655 | 86.3%（帯境界で切断される関数を損する） |
| 100,000 行帯 | 18,524 | 90.5%（ただし L700,001–800,000 帯のみ 0/1,002 で全滅 [要因未特定]） |
| **200,000 行帯（6 帯）** | **19,460** | **95.1%** |
| 尾部 L983,601–EOF 単独 | 3,679 | 尾部ヘッダ 3,715 の約 99% |

→ **「ファイルを分割すれば欠落帯はほぼ回収できる」ことが実測で成立**（ただし graphify のノード化率 ~91% を掛えると FUN ノード期待値は ~17,000 程度、全体カバレッジ ~82% [推定]。現状 9,734 から約 1.7 倍）。

---

## 2. 選択肢の比較

### 2-0 比較の前提となる技術事実（graphify 0.9.95 の設定面調査・読取のみ）

インストール先: `%APPDATA%\uv\tools\graphifyy\Lib\site-packages\graphify`（uv tool `graphifyy` 0.9.95）。

| 調査対象 | 結果 |
|---|---|
| rcfile（`.graphifyrc` 系）の許可キー | **`viz_node_limit` と `language.*` のみ**（`rcfile.py`）。抽出上限キーは**存在しない** |
| 環境変数 | `GRAPHIFY_VIZ_NODE_LIMIT`（HTML 表示上限）、`_MERGE_MAX_NODES=100_000`・`_MERGE_MAX_BYTES=50MB`（merge 時 size guard）、`_HINT_GRAPH_MAX_BYTES=24MB`（警告のみ）→ **いずれも抽出ではない** |
| LLM 文書パスの `_FILE_CHAR_CAP=20,000`（`llm.py`） | document 専用。`file_slice.py` に明記「*Only plain-text documents are sliced: code files need whole-symbol context*」→ **code ファイル（.c）は分割・上限対象外** |
| CLI フラグ | file size / limit 系の抽出制御フラグなし |

**結論**: 「抽出上限を解除する設定」は graphify に存在しない。rel4 の [推定]「1 ファイルあたりノード/トークン予算 ~10k」は §1-6 の実測により否定され、真因は tree-sitter 解析失敗。よって (A) の成り立ちは「設定見直し」ではなく「**入力（ソース分割・正規化）の見直し＋再生成**」として裁定する。

### 2-1 比較表

| 観点 | **(A) 再生成設定の見直し**（→ 実体は入力前処理＋再生成） | **(B) 台帳直読みを正とし graph.json はサブセット** | **(C) FUN↔移植シンボル明示エッジの新規設計** |
|---|---|---|---|
| 実体 | 設定変更のみは**効果なし**（実測で knob 不存在）。実効化には **A-1 ソース分割**（200k 行帯・6 分割）か **A-2 構文正規化**＋再生成が必要 | `func_list_new.txt`（21,597 VA 行）＋ `v155` 6 層統合表 ＋ decompiled.c ヘッダ走査（20,465）を**正本**と定義し、graph.json を「呼出し関係の視覚化サブセット」として文書化 | 対応表（`fun_va` ⇄ 移植シンボル）のデータソース選定と格納方式（参照レイヤ / sidecar）の設計 |
| 期待効果 | A-1: FUN ノード 9,734 → **~17,000 [推定]**（認識率 52.1%→95.1% は実測）、後半 3,211 FUN ヘッダを回収。A-2: 前半の散布欠落（7,047 件）も含めて理論上限へ | 「何が正本か」の口径問題を解消。全量・後半 VA 高域の照合は**既に可能**（VA 結合 100%・mismatch 0 が実測済み） | EXE⇔移植エッジ 0 本 → 対応表ペア数ぶんの明示エッジ（RE_EXE_REPORT 由来なら数百規模、人手なら数十〜数百） |
| 実行コスト | **中〜大**。A-1: 分割スクリプト＋**行番号 Lxxxx 参照との互換設計**＋再生成＋再突合。A-2: 大（Ghidra 構文の行-preserving 正規化マスクの実装・検証） | **小**（文書 1 枚＋既存成果の引用のみ。再生成不要） | **中**（データソース棚卸し＋対応表作成。既存 RE 文書の列挙を再利用） |
| 保守性 | **低**。原寸 `Oblivion_decompiled.c` の行番号は v155/v127 等の RE 全文相互参照の基盤。分割・置換は Lxxxx 参照と衝突。原寸保持＋parts 追加なら二重収録・ノード二重生成を抑える対策が別途必要 | **高**。再生成の有無・graphify バージョン変化・`built_at_commit` 遷移に依存しない | **中〜高**（C-3 参照レイヤなら再生成耐性あり。C-1 graph.json 直書きは再生成で消失＝非推奨） |
| 再現性 | 中。帯境界・graphify バージョン・二重収録抑制条件に依存（`parse_errors` スタンプは消えるが測定手順は再利用可） | **高**。実測キー結合（9,734 match / 0 mismatch・graph ⊆ ledger）に裏付け | 高。VA は台帳が正典、port ノード ID は `source_file`+`label` から決定的に再計算できる |
| リスク | 行番号体系の破壊、再生成によるノード ID 遷移、二重収録。**EXE⇔移植エッジは依然 0 本**（跨ぎ問題は解決しない） | 「graph = 正本」と誤解する利用者の残存（文書化で吸収）、graph 検索の網羅性は 45.3% のまま | 手作りエッジの陳腐化（移植側改名で dangling）、INFERRED 0.95 と curated の信頼度区別 |
| graphify 再生成 | **必須**（本タスクでは実行禁止 → 裁定後の別タスク） | 不要 | C-1/C-2 のみ必要、C-3 は不要 |

### 2-2 (A) の技術的手順（裁定後の実行計画。**本タスクでは実行しない**）

1. 前提確認（本タスクで完了）: 設定 knob 不存在・真因は tree-sitter 解析失敗（§1-6）、現行 52.1%。
2. **A-1 分割案**: `docs/Oblivion_decompiled.c` を **200,000 行帯で 6 分割**（認識率 95.1% を実測済み。50k 帯は境界切断損失で 86.3%、100k 帯は 90.5% かつ L700,001–800,000 帯が 0/1,002 で全滅＝要因未特定）。形態の選択:
   - (a) **原寸保持＋`docs/decompiled_parts/*.c` 追加**: Lxxxx 参照は不変。ただし原寸側の未解析尾部と parts 側で二重収録になるため、indexing 対象の調整（ignore / 原寸除外）が必要。
   - (b) 原寸置換: Lxxxx 参照の一斉更新が必要（RE 文書全域に影響・**非推奨**）。
3. 再生成（記述のみ・別タスク）: graphify の再 index/build → `graph.json` の FUN ノード数・`source_location` 分布・台帳カバレッジを `%TEMP%\opencode\v211_*.py` 相当で再測 → rel4 形式の突合表を更新。
4. 検証ゲート: FUN カバレッジ ≥80% [推定]・VA 整合 mismatch 0 維持・**EXE⇔移植の構造エッジが依然 0 であることを明記**（A は跨ぎを解決しない）。

### 2-3 (C) の概要設計（案・実装はしない）

- **キー**: 台帳 VA（正）⇄ graphify ノード ID。port 側 ID は `app_src_*` 系（`source_file`+`label` から決定的に算出）。ラベルの `()` 正規化（rel4 §5-1）を必須手順に。
- **データソース候補**: ① `docs/RE_EXE_REPORT.md`「Android 移植への示唆」節（現行 INFERRED 426 本のソース）② `tmp/re/v_data_relmap.md` / `v76` 関係表 ③ `v155` 統合表（VA）＋移植シンボル対応 ④ 人手ペアリング（RE 判定済み）。
- **レコード案**: `{fun_va, fun_name, port_symbol, port_source_file, relation (ported_from|mirrors|implements), provenance (doc_ref|manual|heuristic), confidence, checked_at}`。
- **格納方式 3 案**:
  - C-1 `graph.json` 直接追記 — **非推奨**（再生成で消失・手書き破壊リスク）。
  - C-2 sidecar JSON を graphify の merge 経路へ — 再生成耐性はあるが、**`graphify merge` は `_MERGE_MAX_BYTES=50MB` を持ち、現行 graph.json は 58,195,634 B で cap 超え**（`cli.py` L2958–2970）→ 現状の merge 経路では失敗する可能性が高く、要事前検証。
  - **C-3 参照レイヤ（対応表 CSV ＋ join スクリプト）— 推奨**: graphify 非依存・再生成耐性・VA 正典と直結。graph クエリ時は都度 join。
- **運用**: 移植シンボル改名による dangling を定期検査に追加。provenance/confidence で INFERRED(0.95) と curated を区別する。

---

## 3. 推奨案と実行チェックリスト

### 3-1 推奨: **B（第一段・即実）＋ C-3（第二段）／ A-1 は条件付きパイロット、A-2 は見送り**

| 判定 | 案 | 理由（実行コスト・保守性・再現性） |
|---|---|---|
| **採用推奨（第一段）** | **(B) 台帳直読みを正とする明文化** | 実測済みのキー結合（9,734 match / 0 mismatch・graph ⊆ ledger・graph only = 0）に裏付けがあり、**コスト最小・保守性最高・再現性最高**。「21,597＝VA / 21,489＝名前 / 20,830＝FUN 名」の口径まで含めて文書化すれば、45.3% か 46.7% かの数え方争いも止まる。rel4 §5 の実務示唆とも一致。 |
| **採用推奨（第二段）** | **(C-3) FUN↔移植対応表（参照レイヤ）** | EXE⇔移植の構造エッジ 0 本問題への**唯一の実効手段**（A では跨ぎは生まれない）。graphify 非依存で再生成耐性が高く、既存 RE 文書（RE_EXE_REPORT 等）の列挙を再利用できる。VA 正典＋決定的ノード ID 再計算で整合検証が機械化できる。 |
| **条件付きパイロット** | **(A-1) ソース分割＋再生成** | 有効性は実測済み（認識率 52.1%→**95.1%**、FUN ノード ~17,000 [推定]）。ただし原寸行番号は RE 全文の相互参照基盤のため、「原寸保持＋parts 追加＋二重収録抑制」の条件が揃うまで着手しない。再生成は本タスク範囲外。 |
| **見送り** | **(A-2) 構文正規化** | 行-preserving マスクの実装・検証コストが大きく、A-1 で 95% 台に届く以上、費用対効果が悪い。L700,001–800,000 帯の全滅要因が特定でき次第、必要性を再評価。 |
| **非推奨** | (C-1) graph.json 直書き | 再生成で消える・58MB で merge 経路も不安定。 |
| — | 「設定を変えて再生成」 | **前提が実測で否定済み**（設定 knob 不存在・解析は決定論的）。再生成の反復では 9,734 のまま。 |

### 3-2 実行チェックリスト（方針採用後に実施する際）

**B（明文化）**
- [ ] 口径の明記: 21,597＝VA 行数（全行一意）/ 21,489＝名前ユニーク（重複名 22 種・108 行過剰）/ 20,830＝FUN パターン名（v155 L1・rel4 と整合）
- [ ] 「graph.json ＝ FUN 45.3%（FUN 名で 46.7%）のサブセット。全量照合は `func_list_new.txt` ＋ `Oblivion_decompiled.c` ヘッダ走査（20,465）直読み」を明記
- [ ] 追記先の裁定: README / AGENTS.md / `graphify-out/GRAPH_REPORT.md` への反映位置（本タスクでは未編集）
- [ ] rel4 の [推定]「~10k ファイル上限」を「tree-sitter 解析失敗（`first_error_line=1`・L983,600 失動）」に訂正する方針を決定（rel4 は履歴保存か訂正 rel か）

**A-1（実施する場合）**
- [ ] 分割帯 200,000 行を採用（95.1% 実測）し、帯境界で切断される関数の損失を確認
- [ ] L700,001–800,000 帯が単独 parse で 0/1,002 になる要因を調査
- [ ] 原寸保持＋parts 追加の方針を採用し、二重収録の抑制（indexing 対象調整）を設計
- [ ] 再生成後: FUN ノード数・カバレッジ（≥80% [推定]）・VA 整合 mismatch 0・`source_location` 分布を再測し rel4 形式で突合
- [ ] `built_at_commit` と HEAD の一致を再確認（rel4 §4 相当）
- [ ] **EXE⇔移植エッジが 0 のままであることを再確認し、跨ぎは C で対応することを明記**

**C-3（実施する場合）**
- [ ] データソース優先順位の裁定（① RE_EXE_REPORT「移植への示唆」→ ② v_data_relmap/v76 → ④ 人手）
- [ ] レコードスキーマ確定（`provenance`・`confidence` で INFERRED 0.95 と curated を区別）
- [ ] port ノード ID の決定的算出規則を確認（`source_file`+`label`、ラベル `()` 正規化込み）
- [ ] dangling 検査（移植側改名）を定期チェックに追加
- [ ] `graphify merge` を使う方式（C-2）を選ぶ場合のみ、50MB cap（現行 58.2MB）の事前検証

### 3-3 残る人間判断

1. **正本の裁定**: 「台帳＋ヘッダ直読みを正、graph.json はサブセット（B）」を正式に採用するか。
2. **A-1 の可否と配置**: 分割 parts を `docs/` 置きとするか graphify 対象外の場所に置くか（行番号参照との共存ルール込み）。
3. **C の方式とソース**: C-3（参照レイヤ）で確定か、C-2（sidecar merge・50MB cap 要検証）を検討するか。対応表の source 優先順位。
4. **rel4 [推定] の訂正方法**: rel4 は履歴として残し訂正 rel を回すか、要約文書側だけ直すか。
5. **同名ノード衝突の最終確認**: `Renderer` 等の同名 label が複数存在し、rel4 のハブ照合は本再現で 4/5 ノードのみ一致確認（FunctionID / esm_reader.cpp / Jolt.h / oblivion_ini.cpp 一致、Renderer は別ノードに一致）。残り 1 件のノード特定を実施するか。

---

## 付録: 実測環境・スクリプト一覧

- 集計: `%TEMP%\opencode\v211_agg.py` → `%TEMP%\opencode\v211_summary.json`（graph.json 全ロード・台帳・decompiled.c 定義走査・クロスエッジ分類・四分位）
- スポット: `v211_spot.py`（388 件の multi-line シグネチャ定義の実体確認・port エッジ特定・256KB VA 窓カバレッジ）、`v211_quick.py`（FUN loc 最大 983,548 / 台帳重複名 22 種）、`v211_cache.py` / `v211_cache2.py`（stat-index・AST キャッシュ `parse_errors` 読取）
- 解析（graphify 本体非起動・`tree_sitter_c` の parse のみ、graphify 同梱 venv `…\graphifyy\Scripts\python.exe`）: `v211_tsp.py`（全体）/ `v211_tsp2.py`（parse_errors・L983,600 帯・ヘッダ 20,465/3,211）/ `v211_tsp3.py`（頭尾分割）/ `v211_tsp4.py`（帯分割 50k/100k/200k/全体）
- 設定面調査（読取のみ）: `rcfile.py`（許可キーは `viz_node_limit`/`language.*` のみ）、`file_slice.py`（code は分割対象外の明記）、`llm.py`（`_FILE_CHAR_CAP=20_000`）、`cli.py`（`_MERGE_MAX_BYTES=50MB`）、`GRAPH_REPORT.md`（抽出上限の記述なし＝設定の記録自体が無い）
- 未実施（禁止事項の遵守）: git 操作、docs/GT/TODO/session-memory/plan・ソース・graphify-out の編集、既存 `tmp/re` の上書き、**graphify 再生成コマンドの実行**、STATE.md の更新（L1 report-only・本タスクは成果物 v211 のみ）
