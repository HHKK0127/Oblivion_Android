# v84: sync_memory_to_supermemory 冪等キー破損の調査・ガード適用・復旧プラン

- 日付: 2026-10-25
- 対象: `tools/sync_memory_to_supermemory.py`（台帳 `.github/memory/session-memory.json` → Supermemory 単方向同期ツール）
- 実施者: opencode
- 前提: **実 run（非 dry-run）は本調査で一度も実行していない**。API は read-only の list 系のみ使用。
- 証跡スクリプト: `%TEMP%\opencode\diag_triage_v84.py`（7ページ全走査・entryId 分布・日付ヒストグラム・40-gram 突合）、`%TEMP%\opencode\diag_triage2_v84.py`（サブセット同定・コンテンツ長分布・forgotten 可視化プローブ・台帳ガード entry 確認）。既存 `diag_sync_list.py` / `diag_entryid_match.py` / `diag_fullscan.py`（2026-10-24）の再現・拡張。

---

## §1 症状と実測数値

`python tools/sync_memory_to_supermemory.py --dry-run`（read-only）の出力:

```
ledger entries: 669
already in Supermemory: 0
to push: 669
to update: 0
```

そのまま実行すれば **台帳 669 件すべてが「未同期」と判定され、全件が重複 push される**。ツールは `metadata.entryId` を冪等キーとして `existing_memories()` で既存照合するが、Supermemory（SM）側で台帳由来の `entryId` が 1 件も残っていない。

SM 空間 `opencode_project_470935b226aa5f24` の実測（`POST /v4/memories/list`、desc/asc 双方向・全7ページ、672 件全数取得）:

| 項目 | 実測値 |
|---|---|
| 空間内 totalItems | 672（desc/asc 両方で一致、重複・欠落なし） |
| `metadata.entryId` を持つ item | **21 件のみ、すべて `copilot-*`（source=copilot-direct）** |
| entryId のうち台帳 id と一致 | **0 件** |
| entryId 欠落 item | 651 件 |
| entryId 欠落 item の metadata キーセット | 623 x `(temporalContext, temporalEventStartMs, temporalEventEndMs, type)` / 22 x `(type,)` / 6 x `(sm_source=supermemory-mcp, temporalContext, ...)` |
| createdAt 日別（全 672 件） | 10-21: 56 / 10-22: 393 / 10-23: 109 / 10-24: 114 |
| isForgotten | 全 672 件 `False`、forgetReason 全 `None` |
| isLatest | 全 672 件 `True` |
| isInference=True | 2 件のみ |
| version（copilot-* 21 件） | v1: 19 / v2: 2（PATCH 後も entryId 保持） |

台帳側: 669 エントリ・id 一意 669・本文ハッシュ一意 669。ガード entry `opencode-sync-idempotency-broken-20261024`（2026-10-24T20:15:40Z、category=reference）を確認。10-22 バックフィルは 580 件を entryId 付きで push 済み、当時空間 992 件 → 現在 672 件。

---

## §2 判定: (b) — バックフィル分の forgetting/消失が主因。(a) metadata 削落は主因として棄却

### 判定 1 行

**10-22 のバックフィル 580 件（entryId 付き・逐語本文）は SM 空間から完全に消失しており（残存 item に台帳本文・台帳 entryId のいずれも検出不可）、SM 側の forgetting/削除による (b)。POST /v4/memories 経路自体は entryId を保持する（copilot-* 21 件が 1-2 日経過後も metadata 8 キー完全保持）ため (a) は主因ではない。**

### (a) を棄却する証拠

1. **copilot-* 21 件の metadata は供与した 8 キー構造を完全保持**: `(category, compressed, entryId, ledger, priority, recordedAt, source, tags)`、source 値はすべて `copilot-direct`。これらは `tools/push_copilot_memory.py` で 10-23..25 に `POST /v4/memories` で直接 push されたもの。**同一エンドポイント・同一形状の payload が entryId を保持している**ので、「SM の推論/抽出パイプラインが metadata を書き換えて entryId を落とす」なら copilot-* も対象になるはずが、1-2 日経過後（version=2 の PATCH 2 件を含む）も保持されている。
2. **entryId 欠落 651 件は台帳由来コピーではなく SM 自前の抽出ファクト**:
   - metadata は SM 側生成スキーマ（`temporalContext` / `type` / `sm_source=supermemory-mcp`）で、台帳の 8 キー構造と完全不同。
   - 本文は極端に短い: 日別 med 63-113 文字（10-21: n=56 min27/med63/max246、10-22: n=393 min30/med83/max591、10-23: n=107 med92、10-24: n=95 med113）。台帳エントリは長い逐語セッション記録。
   - `metadata.type` 値分布: architecture 511 / error-solution 61 / project-config 32 / conversation 26 / learned-pattern 9 / preference 6 / None 6 —— SM の分類スキーマそのもの。
   - 具体例（10-22, type=architecture）: `Menu ID 0x40d maps to SpellPurchaseMenu.`（40 文字）—— LLM 抽出ファクトの典型。
3. **逐語本文の残存なし（40-gram 突合）**: whitespace 正規化後、台帳 669 件から 40 文字 n-gram 索引（34,149 エントリ、step 10）を作り、SM 672 件の全スライド窓と突合。**一致したのは 8 件のみ**:
   - 3 件は copilot-*（台帳と内容が重複する copilot 記録 = 正常な交叉一致）。
   - 残りは 1-2 グラムの偶然一致（`plan: create c:\...\tmp\re\v76_data_relation...` 等の短い事実文）。
   - 台帳の逐語本文が大半で SM に残っていない = バックフィルコピーは無傷では生存していない。

### (b) を支持する証拠

1. **日付ヒストグラムの算術制約**: バックフィルは 10-22 に 580 件 push。in-place 変形（本文書き換え+entryId 削除）で生存しているなら 10-22 バケットは ≥580 件必要だが、実際は 393 件かつ全て短文（max 591）。→ 変形生存も最大 ~37 件（10-22 の pre-existing 推定 ~356 を差し引いた残り）に限定され、decisive でない。
2. **空間カウントの整合**: 992（10-22 push 後）− 580（forget）+ ~260（10-23/24 新規 223 件 + 10-22 追加 ~37 件）= 672 = 現在の totalItems。forget 580 件仮説と数字が合う。
3. **forgotten は list から不可視**: 全 672 件が `isForgotten=False`。`includeForgotten: true` / `status: forgotten` / `status: all` の 3 種のプローブ（limit=3 の read-only list）いずれも totalItems=672 のまま（未知パラメータは無視される）。→ 忘却された item は list に現れず、992→672 の減少（net −320、forget 累計 ~580 と新規追加で相殺）は「forget で list から消えた」説明と整合。list 上の `isForgotten`/`forgetReason` フィールドは残存 item 全員が False のため分布分析には寄与せず（計数のみ記録: isForgotten False=672 / forgetReason None=672 / isInference False=670, True=2）。

### push_copilot_memory.py との差分（なぜ copilot-* だけ生き残ったか）

`tools/push_copilot_memory.py` を精読した結果、同期ツールとの差は以下のみ:

| 観点 | sync_memory_to_supermemory.py | push_copilot_memory.py |
|---|---|---|
| エンドポイント | `POST /v4/memories`（batch 100） | **同一**（`sync.request_json` を共有） |
| payload/metadata | `metadata_of()`: entryId/category/priority/recordedAt/source/ledger/compressed/tags | **同一形状**（source/ledger ラベルが `copilot-direct` のみ差異） |
| 冪等照合 | `sync.existing_memories()` | **同一関数を import して使用** |
| 入力 | 台帳 JSON | tmp/copilot_memories.json（ledger-schema 配列） |
| push 日 | 10-22（バックフィル 580 件） | **10-23..25**（21 件） |

**コードパス・payload 形状に本質的差はない**。差は push 日と件数（580 の bulk vs 21 の小規模）のみ。→ (a) の「POST 経路の metadata 削落」をコード diff では説明できず、SM 側の時間経過に伴う forgetting（10-22 時点の 992 件過密状態からの compaction、あるいは bulk push 自体を低関連と判定した忘却）が最も整合的な説明。forget の exact trigger（容量？ bulk？ 定期バッチ？）は本調査では特定できていない（§5）。

---

## §3 適用したガードと検証

### 変更内容（`tools/sync_memory_to_supermemory.py`、+15 行、`.bak` は編集前に `tools/sync_memory_to_supermemory.py.bak` として保存）

1. モジュール docstring に Safety rail 段落を 1 追加（+6 行）。
2. `main()` の統計 print 直後にガード 1 ブロックを追加（+9 行）。push ループ到達前に `return 2`。

```python
    if len(missing) == len(entries) and len(entries) > 100:
        if args.dry_run:
            print("WARNING: idempotency key missing — a real run would push all %d entries; run is BLOCKED by the guard" % len(entries))
        else:
            print("ERROR: idempotency key missing: 0 of %d ledger entries were found in Supermemory" % len(entries))
            print("       (recognized ledger ids=0, space entryIds=%d, to push=%d). Running now would push the whole ledger as duplicates." % (len(known), len(missing)))
            print("       See the ledger guard entry 'opencode-sync-idempotency-broken-20261024' in .github/memory/session-memory.json")
            print("       and the recovery plan in tmp/re/v84_sync_idempotency.md. Refusing to push anything.")
            return 2
```

### 仕様からの意図的な逸脱（重要）

タスク指定の条件は `not known and len(entries) > 100` だったが、**実測では `known` は空ではない**（foreign な `copilot-*` entryId が 21 件混入し、`existing_memories()` が拾う）。この literal 条件ではガードが一度も発火せず、dry-run の `already=0 / to push=669`（＝危険そのもの）を検知できない。そこで条件を本質である **「台帳エントリが 1 件も recognized されない（`len(missing) == len(entries)`）∧ 台帳 > 100 件」** に置いた。`not known` の場合（known が本当に空）も本条件は含意する（台帳 id 一意 669 件を実測済み）。挙動変更はガード追加のみで、push/PATCH ロジックには手を触れていない。

### 検証（実 run は一切実行していない）

(a) 実 API に対する dry-run（read-only: list 系 7+1 リクエストのみ）:

```
$ python tools/sync_memory_to_supermemory.py --dry-run
ledger entries: 669
already in Supermemory: 0
to push: 669
to update: 0
WARNING: idempotency key missing — a real run would push all 669 entries; run is BLOCKED by the guard
exit=0
```

統計が先行出力され、警告 1 行が続き、exit code 0。

(b) 実 run 経路のオフライン検証（スタブで `load_entries`/`resolve_api_key`/`existing_memories`/`request_json` を差し替え、**ネットワーク・API key を一切使わず**、`request_json` 呼出し回数を計測）:

```
--- simulated REAL run (stubs, no network) ---
ledger entries: 669
already in Supermemory: 0
to push: 669
to update: 0
ERROR: idempotency key missing: 0 of 669 ledger entries were found in Supermemory
       (recognized ledger ids=0, space entryIds=1, to push=669). Running now would push the whole ledger as duplicates.
       See the ledger guard entry 'opencode-sync-idempotency-broken-20261024' in .github/memory/session-memory.json
       and the recovery plan in tmp/re/v84_sync_idempotency.md. Refusing to push anything.
rc = 2 | request_json calls = 0
--- simulated DRY-RUN (stubs, no network) ---
... (統計 4 行) ...
WARNING: idempotency key missing — a real run would push all 669 entries; run is BLOCKED by the guard
rc = 0 | request_json calls = 0
```

スタブの `known` には foreign entryId 1 件（copilot 系）を入れており、実測状況（foreign 21 件混入）を再現。**rc=2・request_json 呼出し 0 件** = 何の API 書き込みも発生していない。`python -m py_compile` PASS、警告文字列の em-dash は U+2014 として UTF-8 で正しく保存されていることをバイト確認済み。

ガードは dry-run/実 run 双方の統計出力後に判定する。known が回復すれば（`len(missing) < len(entries)`）自動的に解除され、従来どおりの冪等同期が機能する。

---

## §4 復旧プラン（推奨: pilot 付き段階的再 push。実行は本報告では一切していない）

### 前提となる事実

- SM は 10-22 に 580 件の bulk 逐語 push を forgetting で消失させた前例がある（992→672）。forget の trigger は未特定。
- copilot-* 21 件は 1-2 日経過でも生存。逐語 push 経路（entryId 付き metadata）自体は機能している。
- 冪等キー（entryId）が戻れば、既存の dry-run→push フローがそのまま「差分同期」に復活する。

### 選択肢比較

| # | 選択肢 | 効果 | 重複リスク見積 | Effort | 判定 |
|---|---|---|---|---|---|
| 1 | **pilot 付き段階的再 push**（POST /v4/memories、entryId 付き、小バッチ） | SM 検索が台帳 669 件で全面復旧、冪等性も回復 | 直後 dup=0（現状台帳一致 0）。再 forgetting 後の再実行時 dup=全件（ガードが捕捉して全量 dup は防止、部分 loss は残る） | 小（ガード条件が self-heal するため解除作業不要。pilot 20-30 件 → 7 日間生存確認 → 残りを ≤100 件バッチ+sleep 延長で） | **推奨** |
| 2 | 既存 672 件に PATCH で entryId を付与して救済 | — | **高**: 既存 651 件は台帳由来コピーではなく SM 抽出ファクト（本文不一致・短文）。別物に台帳 id を紐付けると drift 比較が誤発火し、以後の台帳編集で誤 PATCH を生む | 中 | **却下** |
| 3 | 損失受容、ledger（grep）+ MCP `supermemory add` 直写に依存 | 台帳由来の長文レコードは SM に存在しないため、SM 上の台帳検索は復旧しない（現状の W 軸訂正 3 件の運用がこれ） | 0（新規追加のみ） | ゼロ追加コストだが二重管理負荷が永続 | フォールバック |
| 4 | 選択肢 1 + forget 対策（batch 10、sleep 5s、バッチ識別 metadata） | 選択肢 1 相当＋ trigger 原因不明への保険、効果測定可能 | 選択肢 1 と同一 | 小〜中 | 選択肢 1 に統合推奨 |

### 推奨パス（選択肢 1 + 4）

1. **pilot**: 台帳先頭から 20-30 件だけを再 push（`--limit` 等は現ツールに無いため、一時的な部分台帳 or 別入力ファイルで実施。実装は数行）。
2. **生存ゲート**: 7 日後に `--dry-run` を実施し、pilot 件の `already` カウントが pilot 件数のまま維持されることを確認（entryId 回復後はこの照合が正当に機能する）。**このゲートを通過しない限り本体 669 件は展開しない**。
3. **本体展開**: 残りを ≤100 件バッチ・sleep 延長で push。各バッチ後に dry-run で `already` 増加を確認。
4. **フォールバック**: pilot 消失を観測した時点で選択肢 3 へ切り替え、SM は抽出ファクト層＋MCP 直写、台帳は ledger という現行二層運用を公式化。

重複リスクの内訳: 再 push 直後は台帳一致 0 件のため dup 新規発生なし。最大リスクは「再 push 成功後、再度 forgetting が起き、その後に同期を再実行する場面」で、そのとき entryId 失効が全件なら本ガードが `return 2` で止める（§3）。部分的に entryId が残る混合丧失の場合は残存分だけが dup push されるため、pilot ゲートとバッチ毎 dry-run で検知する。

**本報告者は push / PATCH / forget を一切実行していない。実行はユーザーの承認後。**

---

## §5 残リスク

1. **forget の trigger 未特定**: bulk push 自体が低関連判定された可能性があり、その場合 pilot でも消失しうる。→ pilot 生存ゲート必須（§4-2）。
2. **消失の検知手段が限定的**: list エンドポイントは forgotten を可視化しない（`includeForgotten` 等のパラメータは無視される、§2 実測）。消失は dry-run の `already` カウント下降でしか観測できない。→ 同期ツール実行の無い期間は SM 側の台帳検索力が静かに劣化しうる。
3. **ガードは「全件 missing」のみ detect**: 部分的な entryId 欠落（669 中 N 件だけ消失）はガードを通過し、その N 件が dup push される。全量 dup は防ぐが部分 dup は防がない。→ 定期 dry-run で `already` の推移を監視するのが唯一の早期検知。
4. **(a) の再発可能性**: copilot-* の生存は 1-2 日の観測。SM が古いメモリを再処理して metadata を再生成する機能を将来有効化する場合、entryId は再度消える（本報告の (b) 判定は 2026-10-25 時点の証拠に基づく）。
5. **二重表示**: 選択肢 1 で再 push する場合、10-22 以来 MCP add で直接入れた W 軸訂正等（ガード entry 記載の `E2hRTTcHhczd2gs6tEmfaP` 等）と台帳 entry が同一話題として SM 検索に並びうる（本文非同一のため dup 判定にはならないが、検索結果としては重複表示）。台帳側の該当 entry と MCP 追加分の重複整理は再 push 前に行っておくのが望ましい。
6. **ソース文字セット**: ガードの WARNING 文字列に em-dash（U+2014）を導入した。非 UTF-8 の Windows コンソール環境でも Python の cp1252 エンコードに em-dash は存在するため通常は安全だが、`PYTHONIOENCODING` を cp932 に固定した極めて限定的な環境では UnicodeEncodeError になりうる（その場合もガードは print 前に条件判定済み、raise は push 関係なし）。

---

## 付記: 実行コマンド一覧（再現手順）

```
# triage（read-only）
python %TEMP%\opencode\diag_triage_v84.py
python %TEMP%\opencode\diag_triage2_v84.py

# ガード検証（dry-run のみ。実 run は禁止）
python tools/sync_memory_to_supermemory.py --dry-run

# 台帳検証（変更していないが終了確認として実施）
python tools/validate_memory_ledger.py
```
