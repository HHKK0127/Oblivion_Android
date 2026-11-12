# V73: 新旧関数リスト突合（2026-10-24）

## 比較対象
| リスト | 件数 | 出所 |
|---|---|---|
| Ghidra 新解析（V61時点） | 21,240 | v61_headless_relabel.md |
| Ghidra 新解析 + V70 作成分 | **21597** | func_list_new.txt（本実行 totalFunctions） |
| decompiled.c `FUN_` ヘッダ定義 | **20465** | `// FUN_... @ 0x` 行カウント（本実行再計測） |
| dec_index（start-line 採用） | 19593 | probe_harness |

## 突合結果（addr集合, Ghidra新 vs decompiled定義）
- **両方にある**: 20465
- **Ghidraのみ（新規=旧解析が欠落していた分+V70分）**: 1132
  - うち V70 gap 由来（entry 一致）: 357
  - うち非 gap（既存解析にもあったが decompiled 未収録=旧側の欠落）: 775
- **decompiledのみ（Ghidra新に無い）**: 0
  - 例:
- dec_index − Ghidra_new = 0（start-line 採用とヘッダ定義の差の内訳）

## 3値（21,240 / 20,465→20465 / 19,593）の説明
1. **件数差は計測方式と解析世代の合成**:   - Ghidra FunctionManager（分析世代） 21,240 → V70 後 21597   - decompiled.c ヘッダ定義 20465 ＝その世代の**実際に本文を出力した関数のみ**（thunk/短小/データ隣接は出力対象外かヘッダ非生成）   - dec_index 19593 ＝本セッションの start-line 正規表現採用（`def` 行直書き型の欠落など regex 依存で最も小さい）2. **完全会計が成立**: decompiled定義 20465 + 非FUN特殊名(thunk/`scalar_deleting_destructor`/FID_conflict等) 775 = **21240 ＝ V61 の旧 Ghidra 総数と一致** ✓。旧 decompiled.c の「欠落」775件は**本文未出力の特殊関数**であって解析欠落ではない。   - V70 で新規作成 357 件を加え **21597 ＝ 現行 Ghidra 総数** ✓。dec_index 19593 は decompiled 定義比で 872 件の regex 欠落（split-signature 型等）＝**3値はいずれも正しいが方式が異なる**。3. V5 の「新規 decompiled にのみ存在=旧 decompiled に無い」362件仮説は本突合で拡張: 非gapの decompiled未収録が 775 件（＝2の775件）で、旧 decompiled.c 側の出力対象（FUN 名のみ）が主因。4. decompiled-only は **0 件**（逆方向ドリフトなし）、dec_index − Ghidra_new も 0 → 新解析は旧 decompiled を完全包含。

## 推奨する以後の参照ルール
- 「関数総数」を語るときは **必ず方式を併記**: `Ghidra(解析世代)=21,597(V70後)` / `decompiled本文=20465` / `dec_index採用=19593`。- V10 の gap 表は decompiled 有無列を `旧世代` 注記付きに。

## 成果物
- 本ファイル / `func_list_new.txt` / `%TEMP%\opencode\v70_v73_report.py`（本突合スクリプト）

## 追記: dec_index 欠損872件の解消 (2026-10-24, V77)
V77 で probe_harness の `dec_index` をヘッダコメント `// FUN_... @ 0x` 起点に再構築 → **19,593 → 20,465（decompiled ヘッダ定義数と正確に一致、旧キーは真部分集合・欠損0）**。本レポートの「dec_index 19,593 = 20,465 − 872」は原因特定（DEF regex の split-signature 型不達）として正しく、**欠損872件はインデックス側の修正で解消された**。伴う変更: dec_index の値はヘッダ行番号基準（`dec_body()` が1〜3行上から開始）、「dec_index採用=19593」表記は以後 **20,465** に更新（本表の1行目も同旨で陳腐化）。
