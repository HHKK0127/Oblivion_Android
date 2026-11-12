# V204: Supermemory の古い記述の整理（要ガード解除判断）— 分析レポート

- 作成: 2026-10-31 / 担当: TODO serial 204
- 出典: `tmp/re/relcheck_20261029/rel5_ledger_freshness.md`（一次実測・読取専用）＋ 本セッションでの補足確認
- **制約**: 本タスクは**整理提案まで**。forget/更新の実行と `tools/sync_memory_to_supermemory.py` ガード解除の判断は **[HUMAN]**（現行: V84 により真の実行 FORBIDDEN・pilot 保留）。
- **API 制約の明記**: 2026-10-31 時点、本セッションのツールカタログに **supermemory は露出していない**（top-level ツール・Code Mode カタログともに不在）。したがって **Supermemory API 側の直接検証（list/search での ID 特定）は未実施**。以下の整理は rel5 が 2026-10-29 に実施した list/search 実測と台帳解析に基づく。

## 1. 陳腐化の現状（rel5 実測の要点）

| 層 | 最新記録 | 鮮度差（対 10-29） |
|---|---|---|
| 台帳 session-memory.json | 2026-10-25T10:51Z（670件） | **−4日** → 2026-10-31 に V195 バッチで 685 件へ、V205 で 689 件へ**解消済み** |
| Supermemory | 2026-10-26T21:40Z（V138 完了） | **−3日**（10-27 以降 search ヒット 0・maxMemories 上限の影響で下限値 [留意]） |

## 2. 陳腐化記述の特定と整理提案

### 2-1. 「session B 残件11件（B-0 から）」（更新 or forget）
- **陳腐化内容**: V98〜V108 は **2026-10-25 に完走済み**（台帳 `opencode-report-corrections-v98-v108-20261025` が記録）。「残件」として列挙された状態は既に過去形。
- **提案**: ① forget（後続の完走記録が台帳と Supermemory 両方に存在するため情報損失なし）**または** ②「完了（2026-10-25・台帳参照）」への更新。**①を推奨**（重複記録の分岐防止）。
- **実行時の手順**: Supermemory 検索で `session B 残件` を突き合わせ、元 ID を特定（rel5 の API 制約下では本作業はツール復旧後・または人間の管理画面で実施）。

### 2-2. TODO 行数メモ類（更新）
- 陳腐化内容: 「334行 / 53 V-rows」「281行 / 30行」等のスナップショット。**実際は 2026-10-31 時点で 700行 / V系 92行（done 84 / open 8）**。
- **提案**: 行数スナップショット系のエントリは**参照価値が低い**ため forget を推奨。行数に依存する記録は常に `tmp/re/TODO_re49.md` を正とし、その旨の1本を新規 add してもよい（例: 「TODO 正本 = tmp/re/TODO_re49.md・行数スナップショットは参照しない」）。

### 2-3. 10-27 以降の記録欠落（追記の要否）
- Supermemory 側に 10-27〜10-30 の RE 成果（V169–V192 バッチ等）の記録が無い。
- **ただし**: 台帳（canonical store）側は V195 バッチで 6エントリ（V116–V142 / V143–V167 / V169–V190 / letter-row 監査 / 9値連託 / 現況サマリ）が 2026-10-31 に追記済み。
- **提案**: Supermemory への個別 add は行わず、**台帳を正とする**（AGENTS.md の canonical store 規約: 同一内容の二重記録はドリフトの温床）。Supermemory は「ポインタ付き要約」1本に留めるか、現状維持。

## 3. sync_memory_to_supermemory.py ガード解除の判断材料

| 選択肢 | 内容 | 評価 |
|---|---|---|
| **A. 維持（推奨）** | V84 ガード（FORBIDDEN）を維持。Supermemory は人間/明示指示でのみ更新 | canonical 規約と整合・二重ドリフトリスク最小。台帳 689 件が唯一の正本 |
| B. pilot 再開 | ガードを「pilot 20–30件」に変更し再実行 | rel5 時点の鮮度差（−3日）は台帳側で解消済み。**現時点での再開理由は薄い** |
| C. sync 廃止 | スクリプトをアーカイブし Supermemory 同期方針を事棄却 | 運用単純化できるが、他セッション/Copilot 側の Supermemory 利用（例: 別プロジェクト連携）の将来性次第 |

- **推奨: A（維持）**。根拠: ①canonical 規約（AGENTS.md）が台帳を正に指定、②V195/V205 で台帳側の鮮度・訂正が双双解消、③Supermemory の list 表示は content:null クイランク（2026-10-26 に search 実証・data は保持されている）と併せ、運用上の信頼性が台帳より低い、④V84 当時の目的（バックフィル）は台帳追記で達成済み。
- 人間が B を選ぶ場合: まず `list`/`search` で §2-1/2-2 の対象 ID を確定 → forget → guard 変更 → pilot 実行（20–30件）→ 7日生存確認、の順を推奨。

## 4. 本セッションで確認済みの補足事実

- 2026-10-26 に本セッションから Supermemory へ 2件追記済み（workflow ルール・V116–V192 実績サマリ）。当時の `list` で新規エントリが content:null 表示されるが **search では全文取得可能**（data 保持の実証）— 表示クイランクでありデータ喪失ではない。
- 台帳の参照欠損2件（`tools/adb.execurrent` / `docs/PHASE_PLAN.md`）は **V205 で 2026-10-31 に訂正エントリ追記済み**（689件・validator 0 errors）。

## 5. 人間判断チェックリスト

1. §2-1 の forget（session B 残件11件）— 実行可否
2. §2-2 の TODO 行数メモ forget ＋「正本=TODO_re49.md」エントリ追加の要否
3. §3 のガード方針（A 維持 / B pilot / C 廃止）— **推奨 A**
4. supermemory ツールが復旧した際の実行主体（人間 or 次回セッション）

## 6. 証拠
- rel5_ledger_freshness.md 全文（§1–§5 の実測値）
- 台帳の現況: `tools/validate_memory_ledger.py` = 689 entries / 0 errors（V205 時点）
- 本セッションの Supermemory 追記/検証記録（2026-10-26、search での content 取得実証）
- スクリプト実行なし（解析・提案のみ）／git・docs・GT・ソース編集なし
