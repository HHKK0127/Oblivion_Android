# V58: Script VM MISS ハンドラ台帳（§67）生成スクリプト／選択規則の復元調査

- 作成日: 2026-10-24
- 調査対象: `docs\RE_EXE_REPORT.md` §67「Script VM MISS ハンドラ台帳（480件）」の原表 `tmp\re\svm_495_miss_addrs.md` を生成したスクリプトと選択規則
- 制約遵守: 既存 DB/ファイルは一切読み取りのみ（`sqlite3` は `file:...?mode=ro` URI で接続）。新規成果物は本ファイルのみ。作業スクリプトは `%TEMP%\opencode\v58_*.py` にのみ作成。

## 0. 結論サマリ

1. **生成主体の訂正**: 台帳は **GitHub Copilot セッションではなく、opencode セッション（2026-10-22 JST 昼〜夕）が生成**した。Copilot セッション `9427ac74` は 10/22 夜に成果物を**受領・確認**し、`append_s67_69.py`（10/22 19:59 作成）で §67〜§69 として RE_EXE_REPORT へ追記した側である（証跡 §3.3/§4.2）。
2. **生成スクリプト本体は未復元**（判定 c の要素）。`%TEMP%\opencode\` から 10/21〜10/22 分が失われ、`.opencode\messages|parts` は各セッション最終メッセージのみ残存、Copilot `session.db` は会話を保持しない DB 構成のため。ただし**選択規則の記述は復元できた**（§3.3 引用）。
3. **UNDET15 の判定は (b)**: 復元できた規則（annotate_db 既登録除外／impl≠0／capstone シグネチャ判定／SCDA 頻度降順ソート）では `0x0016` OnMagicEffectHit が説明できない。実機逆アセンブルで **`0x0016` の impl `0x5045E0` は台帳掲載済み `0x0017`/`0x0018`/`0x0019` と同一プロローグ**であり、シグネチャ判定で落ちる理由が無い（§5）。ただし v1=495 件 → v2=480 件の差が**ちょうど 15** であり、UNDET15 は v2 化の際に落とされた可能性が高い（理由の記録はなし）。

## 1. Copilot session.db 一覧（`C:\Users\hiroki.kogarumai\.copilot\session-state\` 配下・全 44 件）

全件を `Get-ChildItem -Recurse -Filter session.db` で列挙し、`sqlite_master`（read-only）でテーブル実名を取得した。テーブル構成は DB により異なり、下記のスキーマ族 A〜F に分かれる。族の内容（実名）は下表の下に記載。

| # | session（先頭8桁） | LastWriteTime | Size(B) | 族 | todos |
|---|---|---|---|---|---|
| 1 | 3f3a594e | 2026-07-02 17:47:51 | 28,672 | A | 0 |
| 2 | 6d0dea6c | 2026-07-07 17:07:30 | 36,864 | A' | 0 |
| 3 | 8f155833 | 2026-07-20 20:15:18 | 28,672 | A | 0 |
| 4 | 1e48fe58 | 2026-07-21 00:18:08 | 49,152 | A | 117 |
| 5 | a5827467 | 2026-07-22 10:50:15 | 28,672 | A | 4 |
| 6 | 77b40097 | 2026-07-22 11:12:50 | 28,672 | A | 0 |
| 7 | b4c5551c | 2026-08-04 14:51:39 | 135,168 | B | 237 |
| 8 | 5e83ebd4 | 2026-08-04 17:16:31 | 45,056 | A+tui_flutter_porting | 0 |
| 9 | 55a908b1 | 2026-08-05 12:55:01 | 28,672 | A | 12 |
| 10 | 820f6d2e | 2026-08-17 15:38:59 | 28,672 | A | 0 |
| 11 | 6a130466 | 2026-08-19 08:46:22 | 49,152 | A | 87 |
| 12 | 0b71c8cf | 2026-08-31 16:25:30 | 28,672 | A | 0 |
| 13 | ca32e5d2 | 2026-09-01 12:07:54 | 28,672 | A | 8 |
| 14 | 6b221fc2 | 2026-09-17 11:26:14 | 20,480 | C | 17 |
| 15 | f5aa54cb | 2026-09-22 15:15:23 | 20,480 | C | 8 |
| 16 | fe7f23f7 | 2026-09-22 19:20:13 | 28,672 | C+session_state | 0 |
| 17 | d8f3cd86 | 2026-09-24 00:59:18 | 36,864 | C+session_state | 16 |
| 18 | 0c8d192d | 2026-09-24 10:47:12 | 20,480 | C | 3 |
| 19 | 964d16fb | 2026-09-24 18:51:27 | 77,824 | D | 64 |
| 20 | 44d8c597 | 2026-10-02 15:00:19 | 90,112 | E | 0 |
| 21 | 69cd8aff | 2026-10-02 18:17:41 | 69,632 | E | - |
| 22 | 05b6cb3a | 2026-10-03 09:35:31 | 69,632 | E | - |
| 23 | ac1128ee | 2026-10-03 14:01:38 | 69,632 | E | - |
| 24 | 2dbf7451 | 2026-10-09 09:42:38 | 192,512 | F | - |
| 25 | 90201b97 | 2026-10-09 09:49:22 | 192,512 | F | - |
| 26 | 3b2971dc | 2026-10-11 11:25:16 | 192,512 | F | - |
| 27 | 7edd4ba2 | 2026-10-11 11:48:19 | 212,992 | F+ | 6 |
| 28 | e94e5522 | 2026-10-11 11:53:29 | 212,992 | F+ | 5 |
| 29 | 30cbb1e4 | 2026-10-11 12:14:01 | 212,992 | F+ | 14 |
| 30 | f37ebdc7 | 2026-10-11 12:34:42 | 212,992 | F+ | 0 |
| 31 | 1102928b | 2026-10-15 16:57:25 | 192,512 | F | - |
| 32 | 352e8aad | 2026-10-18 16:28:15 | 212,992 | F+ | 2 |
| 33 | 4d967590 | 2026-10-19 15:03:23 | 212,992 | F+ | 0 |
| 34 | a16dd3d0 | 2026-10-19 18:15:19 | 212,992 | F+ | 0 |
| 35 | **c494221b** | 2026-10-20 17:42:53 | 282,624 | F+ | 140 |
| 36 | 4d94e632 | 2026-10-21 14:00:11 | 212,992 | F+ | 0 |
| 37 | 868318a4 | 2026-10-21 14:09:03 | 212,992 | F+ | 17 |
| 38 | 9e3d376b | 2026-10-21 14:21:06 | 212,992 | F+ | 4 |
| 39 | d01b8d5b | 2026-10-21 15:37:25 | 212,992 | F+ | 6 |
| 40 | **2c1ee76c** | 2026-10-22 08:46:07 | 192,512 | F | - |
| 41 | **4199c9bc** | 2026-10-22 13:27:03 | 192,512 | F | - |
| 42 | **c6209d0e** | 2026-10-22 14:30:41 | 192,512 | F | - |
| 43 | 524919fc | 2026-10-23 13:56:13 | 212,992 | F+ | 3 |
| 44 | **9427ac74** | 2026-10-23 23:28:12 | 245,760 | F+ | 43 |

（件数は `Measure-Object` 実測 44 件。上表 1〜44 が全件）

**テーブル族（実名）**

- A: `inbox_entries, todo_deps, todos`
- A': A + `session_memory`
- B: `claw_subtasks, inbox_entries, integration_analysis, integration_roadmap, poc_setup_plan, todo_deps, todos, warp_tasks`
- C: `todo_deps, todos`（+ 一部 `session_state`）
- D: `inbox_entries, remaining_tasks, review_items, todo_deps, todos, user_prefs`
- E: `factory_agents, factory_credit_charges, factory_journal, factory_phases, factory_progress, factory_runs`(+`todo_deps,todos` が混在)
- F: `factory_agents, factory_credit_charges, factory_journal, factory_phases, factory_progress, factory_runs, workflow_agents, workflow_credit_charges, workflow_journal, workflow_phases, workflow_progress, workflow_runs, workflow_store_schema`
- F+: F + `todo_deps, todos`

**重要**: 全 44 件に会話ログテーブル（`messages` / `conversation` / `commands` 等）は**存在しない**。F/F+ 系の `factory_journal` / `workflow_journal` / `*_progress` は候補 10 DB 全件で **0 行**（スキーマ実測）。会話・スクリプト断片は session.db に保存されない設計である。

## 2. 台帳生成セッションの特定（候補と絞り込み）

生成実施日時の外部時刻 anchors: `append_s67_69.py` = 2026-10-22 19:59:04（JST、repo 側）。台帳受領報告 = Copilot turn 3395 = **2026-10-22T10:53:30Z（UTC）＝ 19:53 JST**（`session-store.db` の turns.timestamp は UTC、ファイル mtime は JST である点に注意）。

### 候補 1（本命・生成実施）: **opencode セッション `ses_e9d892175ffe08jjw24I31ycFX`**

- 所在: `C:\Users\hiroki.kogarumai\.opencode\messages\ses_e9d892175ffe08jjw24I31ycFX\`（最終＝唯一の残存メッセージ `msg_1a1627eedbbfir5ynfqiy5.json`、10/22 16:24:07 JST）＋ `parts\msg_1a1627eedbbfir5ynfqiy5\prt_1a1627eedbdm67sew7j.json`（273,561 B）。
- 根拠: この part はコンパクション用コンテキスト注入であり、次を含む（§3.3 引用 A）:
  - 「(1) **svm_495_miss_addrs.md v2: 480 entries, 133 named, sorted by frequency**」
  - 「Script VM: **495 MISS handlers extracted via capstone from binary (520 impl addrs, 10 annotated)**. Handler pattern: push ecx→call 0x4FAE80→test/jne→ret.（旧サマリ＝v1 時点）」
  - 「Gate analysis complete. … 489 unnamed handlers: 327 command-type (helper=0), 162 expression-getters (unique helper per opcode)」
- v1（495件）→ v2（480件）の履歴とファイル名 `svm_495` の由来（v1 の 495 が残った）がこの 1 メッセージで説明される。**台帳生成の主体はこの opencode セッションと特定**できる。

### 候補 2（受領・追記側・生成ではない）: **Copilot セッション `9427ac74-c35e-4e3d-b47e-79c1c4621727`**

- `session-state\9427ac74\session.db`（10/23 23:28:12、245,760 B、F+ 族、todos=43）。
- `session-store.db` の turns に 2,348〜3,406 が記録され、**turn 3395（2026-10-22T10:53:30Z）**で「`tmp/re/svm_495_miss_addrs.md` | 44 KB。**Script VM MISS ハンドラ 480 件の完全台帳**（頻度降順、Impl VA / Size / Freq / 外部呼出）| 今回の最大成果」と報告（§3.3 引用 B）。直前の turn 3389/3392「Opencodeの解析結果を受け取ってもらえますか」により、**受け取り側**と確定。
- 台帳の RE_EXE_REPORT §67 への追記は同セッションへの引き渡し後、`tmp\re\append_s67_69.py`（10/22 19:59:04）が実施（§4.2）。

### 候補 3（10/22 の Copilot session-state、棄却）: `2c1ee76c` / `4199c9bc` / `c6209d0e`

- 3 件とも F 族（todos テーブル自体が無い）で factory/workflow 全テーブル 0 行。`session-store.db` の sessions で内容確認: `2c1ee76c`=「supermemory.aiってGitHubCopilotに対応していますか」（cwd は `.copilot\chats\...`）、`4199c9bc`=「Rustで置き換えられる所はありますか」、`c6209d0e`=「Supermemory mcp設定の確認」。いずれも台帳生成と無関係で**棄却**。
- `c494221b`（10/20 17:42、todos=140、最大サイズ 282KB）は RE 解析長寿命セッションだが、最終更新が 10/20 で台帳生成日（10/22）に届かない。todos 140 件・todo_deps 42 件を含むが、キーワード走査で生成痕跡なし（§3.1）。

## 3. スクリプト断片・選択規則の検索結果

### 3.1 Copilot session.db 全 44 件のテキスト列横断走査

走査対象: 全 DB・全テーブルの TEXT 系列（`todos`/`todo_deps`/`inbox_entries`/`session_state`/`session_memory`/`factory_*`/`workflow_*`/`remaining_tasks`/`review_items`/`user_prefs` 等）。検索語: `miss_addrs` / `svm_495` / `annotate_db` / `s57_svm_8` / `cc_prefix` / `absorbed_suspect` / `0x977C50` / `0xB0AF48` / `台帳` / `scda_opcode` / `Script VM` / `MISS ハンドラ`。

結果（11 ヒット、すべて todos の説明文。生成スクリプトは無し）:

| DB | テーブル | 行キー(pk) | 語 | 先頭200文字付近 |
|---|---|---|---|---|
| 0c8d192d | todos | preserve-script-vm-batches | Script VM | "Preserving Script VM batches / Commit the existing validated Script VM expansion changes before integrating master. / done" |
| 0c8d192d | todos | integrate-master-changes | Script VM | "Merge origin/master while preserving Script VM work and honoring renderer ownership." |
| 0c8d192d | todos | restore-host-script-tests | Script VM | "Restoring host Script VM tests / Await host test runner update to link Quest Flow implementations…" |
| 6b221fc2 | todos | phase39-script | Script VM | "Phase 39: Script VM ゲームシステムへの接続 / done" |
| 9427ac74 | todos | combat-damage-apply-reimpl | annotate_db | "CombatDamage_Apply (0x5FEBF0) の完移植確認 / annotate_db.json に Copilot high-priority とマーク…" |
| 9427ac74 | todos | annotate-system | annotate_db | "annotate.py + annotate_db.json の保守と拡張 …【OPENCODE 2026-10-21 コミット済み】… 25関数 + 1386構造体オフセット…" |
| 9427ac74 | todos | phase-67 | Script VM | "Phase 67: Full Script VM / スクリプトVM完全互換: ~1,080 関数実装… hhkk0127-script-vm-expansion ブランチ…" |
| 9427ac74 | todos | re-ghidra-headless-acquire | absorbed_suspect | "…impl 527 件中 165 件のみデコンパイル済みで 362 件が欠落（Ghidra_missing 260 / absorbed_suspect 102 等）。突合可否表は tmp/re/v10_impl_decompile_join.csv…" |
| 964d16fb | todos | step6-phase67 | Script VM | "Achieving full Script VM compatibility / Implement ~1,080 additional script functions (118 of ~1,200 done)…" |
| d8f3cd86 | todos | ws-b-script-vm | Script VM | "Expanding Script VM function coverage / Grow the 118 implemented script functions toward ~1,200…" |
| d8f3cd86 | todos | phase67-dialogue | Script VM | "…QUST 416 progression via Script VM. Phase 71…" |

`miss_addrs` / `svm_495` / `s57_svm_8` / `0xB0AF48` / `0x977C50` / `台帳` / `MISS ハンドラ` は **session.db で 0 件**。

### 3.2 `.copilot\session-store.db`（会話ログ本体。本調査の最大発見）

スキーマ（read-only 実測）: `sessions`(635) / **`turns`(3,421: session_id, turn_index, user_message, assistant_response, timestamp)** / `checkpoints`(3,193) / `session_files`(3,096) / `search_index`(FTS 18,528) / `assistant_usage_events`(120,129) 他。

- `turns.assistant_response` の turn **3395**（session 9427ac74, 2026-10-22T10:53:30Z）:

  > `tmp/re/svm_495_miss_addrs.md` | 44 KB。**Script VM MISS ハンドラ 480 件の完全台帳**（頻度降順、Impl VA / Size / Freq / 外部呼出）| 今回の最大成果 … 1. **Script VM MISS 台帳 480 件** — VM 関数テーブル `0xB0AF48`（532 エントリ、stride 0x28）から既登録 367 を除外。上位は OnLoad 12,098 / OnStartCombat 10,530 / OnActorEquip 5,647

- `checkpoints` 3477/3478/3479（追記セグメント）に「§67/§68/§69 の追記を実施」「`tmp/re/svm_495_miss_addrs.md` の統計を算出: 480 行、freq 数値 112 件（合計 46,104）、`-` 368 件、サイズ 153B、160B…」。追記作業の記録であり生成ではない。
- `turns` 全 3,421 行・`checkpoints` 全 3,193 行を `miss_addrs`/`台帳`/`annotate_db` 等で走査したが、**生成 python スクリプトの本文（`open(`/`write`/`480` を含むコード行）は 0 件**。残存しているのは要約・報告文のみ。

### 3.3 選択規則の記述（復元できたもの。引用）

**引用 A**（`.opencode\parts\msg_1a1627eedbbfir5ynfqiy5\prt_1a1627eedbdm67sew7j.json` 内 /text、コンパクション注入文、≤600字）:

> 5 tasks completed. (1) svm_495_miss_addrs.md v2: 480 entries, 133 named, sorted by frequency. … Script VM: 495 MISS handlers extracted via capstone from binary (520 impl addrs, 10 annotated). Handler pattern: push ecx→call 0x4FAE80→test/jne→ret. Top priority: SetStage, Evp, Enable. DB: 351 funcs total. … Gate 0x4FAA80 confirmed as arg decoder… 489 unnamed handlers: 327 command-type (helper=0), 162 expression-getters (unique helper per opcode).

**引用 B**（`tmp\re\svm_495_miss_addrs.md` 先頭 7 行＝原表自身の規則明記）:

> Source: Oblivion.exe VM function table @ 0xB0AF48 (532 entries, stride 0x28)
> opcode名: s57_svm_8_opcode_table.txt (141 function opcodes) + block-type (0x00-0x1E)
> 頻度: SCDAスキャン実測値 (copilot_archive/scda_opcode_table_out.txt)
> gate呼出: 0x4FAE80 (trampoline) の有無 [capstone detail mode]
> annotate_db.json 既登録 (367 funcs) は除外済み。MISS total: 480

**引用 C**（`tmp\re\append_s67_69.py` L21-26 以降＝§67 への追記時に同じ規則を転記）:

> Android版 Script VM の移植率を定量化するため、`Oblivion.exe` の Script VM 関数テーブルを全数走査し、Android版で未実装のハンドラ（MISS）を **480 件** 特定・台帳化した。`tmp/re/annotate_db.json` に登録済みの 367 関数は除外している。
> - 対象: Script VM 関数テーブル `0xB0AF48`（532 エントリ、stride 0x28）
> - 判定: 各エントリの実装関数を capstone で逆アセンブルし、共通シグネチャ（gate `0x4FAE80` 呼出）を確認

**復元された選択規則（確定）**:
1. 全走査対象 = テーブル `0xB0AF48` の 532 エントリ（stride 0x28、impl は +0x18）。
2. impl == 0 は対象外。
3. `tmp/re/annotate_db.json` 既登録（367 funcs。現物は 368）を除外。
4. capstone 逆アセンブルで共通シグネチャ（gate 呼出、`0x4FAE80`=mid-function continuation、実体は `0x4FAA80` 引数デコーダ）を確認して MISS と判定。
5. SCDA 実測頻度（`copilot_archive/scda_opcode_table_out.txt`、`0x0016`=10,480 等）を結合し**頻度降順**でソート。頻度未観測は `-`。
6. opcode 名は `s57_svm_8_opcode_table.txt` + block-type 0x00-0x1E から付与（v1 は 133 named、v2 以降は全件 named）。

## 4. 別系統の手掛かり（パス＋該当箇所）

### 4.1 `%TEMP%\opencode\`（生成スクリプトの本来の保管場所）

- **2026-10-20〜10-22 作成の .py は 0 本**（`Get-ChildItem` 実測。10/23 11:42 以降のみ残存）。生成スクリプトは失われた。
- 関連既存物:
  - `%TEMP%\opencode\v10_join.py`（10/23 17:22:54、4,945 B）— **存在する**。cc_prefix 実装は §6 参照。
  - `v5_dump_optable.py`（10/23 14:59）、`verify_vs_optable.py`（10/23 12:47）、`svm_cl_00〜05`（10/23 13:41〜13:55）— 台帳の検証・修復系。生成ではない。
  - `analyze_p2.py` / `per_elem_oracle.py` / `p2_impl_gaps.json` 等 — 台帳を**読む**側（gap 分析、10/23）。

### 4.2 `tmp\re\`（repo 側）

- `append_s67_69.py`（10/22 19:59:04、472 行）: **§67 追記スクリプトを完全回収**。原表 `tmp/re/svm_495_miss_addrs.md` を読み、`assert len(rows) == 480`（L9）、top50 と全 480 行を §67.4/§67.6 に埋め込み。生成ではないが、規則文の一次的転記元。
- `tmp\re\*.py` 全 400 本超を `miss_addrs|0xB0AF48|480` で grep — **生成コードは 0 件**（`0xB0AF48` 参照は `s57_svm_7_table.py` / `s57_svm_8_opcode_table.py` / `s69_scda_a_01_bt.py` 等のテーブル読取系のみ）。
- `tmp\re\copilot_archive\`: `scda_opcode_table.py`（09/24 12:26）+ `scda_opcode_table_out.txt`（09/24 12:27、`0x0016 10480` を含む）＝**頻度ソースの実体を回収**。`_copy_from_copilot_state.py` は session-state からのアーカイブ用。
- `tmp\re\opencode\ses_*.txt` 9 本 — すべて 10/16 作成で台帳生成（10/22）より前。無関係。
- 原表のバックアップ（生成直後の v2 系を保全）:

| ファイル | mtime | Size | 行数(データ) | 0x0016 |
|---|---|---|---|---|
| svm_495_miss_addrs.md.bak.prev | 2026-10-22 16:01:18 | 44,208 | 480 | 掲載なし |
| .bak / .bak2 / .bak4〜.bak8 | 10/23 10:24〜14:26 | 44,208 | 480 | 掲載なし |
| .bak9 / 現行 | 10/23 16:48:51 | 40,868 / 40,866 | 480 | 掲載なし |

  → **v1（495 行）の実ファイルは保存されていない**。最も古い `.bak.prev`（10/22 16:01）ですでに 480 行（v2）。v1 の記憶は引用 A のサマリのみ。

### 4.3 `.opencode\`（opencode のメッセージ保管。本調査で新たに判明）

- `messages\<session>\msg_*.json`（336 B、メタ）+ `parts\<msg>\prt_*.json`（本文）。**残存は 10/22 の 3 セッション×最終メッセージのみ**（`ses_e9d892175` 16:24 / `ses_ead2cf2b` 19:14 / `ses_e9ccb319` 19:42）。
- `ses_e9ccb319`（cwd=C:\Users\hiroki.kogarumai）は現在の調査用セッションであり無関係。`ses_ead2cf2b` の part（43 KB）は opcode テーブル layout の context（`Field +0x18 contains the runtime implementation pointer…`）で、Project Knowledge 注入。

### 4.4 その他の Copilot 側 DB

- `.copilot\data.db`（144 sessions / activity_items 2,612 / session_checkpoints 3,108 等）— 会話本文は無くメタのみ。`session-store.db`（§3.2）が会話本文の実体。

## 5. UNDET15 の選択規則判定: **(b) 規則は見つかるが説明できない**

### 5.1 検証したこと（すべて read-only）

1. `annotate_db.json`（現物 368 funcs、mtime 2026-10-22 16:09:49）に `0x5045E0`（0x0016 impl）/ `0x5045A0`（0x001E impl）は**不在**（json 全走査で 0 件）。→ 「annotate_db 除外」では落ちない。
2. impl ≠ 0: `0x0016` impl = `0x005045E0`、`0x001E` impl = `0x005045A0`（テーブル実読み）。→ 「impl=0 除外」では落ちない。
3. 共有スタブ `0x977C50` でもない（監査側確定と整合）。
4. **capstone 実逆アセンブル（EXE `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe` 読取、`%TEMP%\opencode\v58_disasm_0016.py`）**:

| OpID | 名称 | impl | 先頭プロローグ | 台帳 |
|---|---|---|---|---|
| 0x0015 | OnLoad | 0x504560 | `cmp [esp+0xc],0 / fldz / push esi …` | **掲載** |
| **0x0016** | **OnMagicEffectHit** | **0x5045E0** | `push ecx / mov ecx,[esp+0x18] / mov edx,[esp+0x14] / push esi / … / lea eax,[esp+8] / push eax` | **不掲載** |
| 0x0017 | OnSell | 0x503D90 | `push ecx / mov ecx,[esp+0x18] / mov edx,[esp+0x14] / push esi / …`（0x0016 と同一型） | **掲載** |
| 0x0018 | OnTrigger | 0x504660 | 同一型 | **掲載** |
| 0x0019 | OnStartCombat | 0x5041D0 | 同一型 | **掲載** |
| 0x001E | OnReset | 0x5045A0 | `cmp [esp+0xc],0 / fldz …`（0x0015 と同一型） | **不掲載** |

  → **`0x0016`/`0x001E` は「共通シグネチャ（gate 系 prologue）」を満たす正常 MISS ハンドラ**であり、復元規則 1〜5 のいずれからも落ちない。シグネチャ判定の失敗では説明できない。

### 5.2 補助的事実（v1→v2 の算術対応）[事実＋推定の混在に注意]

- 事実: 引用 A にて v1 = **495** MISS / v2 = **480** entries。差 = **15**。
- 事実: 監査 `v15_v20_v22_ledger_audit.md` の台帳不掲載 52 件の内訳は impl=0:5 / annotate_db:25 / 共有スタブ:7 / **UNDET:15**。
- [推定] v1→v2 の更新で**ちょうど 15 件**が除外されており、この 15 件が現在の UNDET15 と同一集合である可能性が高い。除外理由の記録（スクリプトも会話ログも）は現存せず。
- 反証留意点: 引用 A の v1 は「10 annotated / DB 351 funcs」時点の数値であり、annotate_db が 351→367/368 に増えた分（+16/+17）との関係が単純な差引きで説明できるかは未検証。495−480=15 が annotate_db 増加分と一致するかは、v1 時点の annotate_db 具体内容の記録がないため**本調査では確定しない**。

### 5.3 判定

**(b)** — 復元した選択規則（§3.3）は有用だが、UNDET15（特に `0x0016` OnMagicEffectHit、SCDA 頻度 10,480 で rank2 相当）を説明できない。機械検証により「annotate_db 除外」「impl=0」「gate シグネチャ不一致」のいずれでもないことを**反証済み**。残る説明候補は「v2 化時に人手/別フィルタで 15 件を落とした」のみだが、その規則本文は現存しない。

## 6. cc_prefix 閾値実装の所在

`%TEMP%\opencode\v10_join.py` は **存在する**（10/23 17:22:54、4,945 B）。該当行:

```python
# v10_join.py L68-69
def cc_prefix(va):
    return rb(va - 4, 4) == b"\xcc" * 4
# L92-94 gap_kind 分類
#   "present" if hit else
#   ("ghidra_missing" if cc_prefix(impl) else "absorbed_suspect")
```

- 閾値は**数値比較ではなく「impl 直前 4 バイトが連続 0xCC」の完全一致判定**（＝"cc≥4" 相当。V5 の "cc≥2" 案とは別物）。
- 出力先 `tmp/re/v10_impl_decompile_join.csv`（`OUT` 定義 L6）。cc_prefix=yes 集合＝ghidra_missing（関数先頭実在だが FUN_ 未生成）、no 集合＝absorbed_suspect（前関数への吸収疑い）。
- 注: 台帳 §67 の生成とは**独立の軸**（impl↔decompiled 突合）。UNDET15 判定にはこの cc_prefix は使われていない（台帳生成規則に cc_prefix の記述は無い）。

## 7. annotate_db.json の時系列（367→368）

| 時点 | 値 | 根拠 |
|---|---|---|
| 2026-10-22 午前（v2 台帳生成時） | **367** funcs | 原表ヘッダ「既登録 (367 funcs)」＋ §67.1 |
| 2026-10-22 16:09:49（JST） | ファイル mtime。現物 **368** funcs / structs 7 | json 実カウント（本調査）＋ turn 3395「functions 368 …「既登録367」と整合（+1）」 |
| §67.8 証跡（10/22 夜の追記） | 「functions 368 / structs 7」 | `append_s67_69.py` L106 |

+1 の内容（どの関数が 368 目か）は本調査の範囲外。台帳の「367 除外」記述と現物 368 の +1 差は、受領報告時点（19:53 JST）から既に認識されていた。

## 8. 未解決点

1. **生成スクリプト本文は未回収**。保管候補（%TEMP%\opencode 10/21-22 分、.opencode メッセージ全履歴）は削除済み。残存する最有力の手掛かりは Supermemory project スコープ `opencode_project_470935b226aa5f24`（70 docs / 1124 memories、turn 3395 報告）— 生成手順の要約が保存されている可能性が高い。次回は `supermemory`（mode: search, scope: project）で「svm_495 / MISS 台帳 / 480」を照会するのが最短。
2. **v1（495 行版）のファイルは不在**。`.bak.prev`（10/22 16:01）が既に v2。v1→v2 で落とした 15 件の実体比較は不可能（サマリ文のみ）。
3. **UNDET15 の除外理由**（§5）: 現存証存証拠では説明不能。確定させるには (i) Supermemory 1 の照会、(ii) 残る 13 件（0x0016/0x001E 以外）も本調査と同様に capstone でプロローグ比較し「全 15 件がシグネチャ適合だった」ことを示した上で、v2 生成直後のstdout/会話断片を探す、のいずれかが必要。
4. `tmp\re\svm_495_miss_addrs.md` の 10/23 16:48:51 更新（44,208→40,866 B、504 行化）は 10/23 の opencode 監査セッション（`vv_fix_1078.py` 16:48:29 等）による修復系と同期。台帳の行内容（480 件・上位頻度）は不変。監査適用（.bak14 系）は `RE_EXE_REPORT.md` 側であり原表には波及していない。
5. `session-store.db` の `data.db` 側は WAL アクティブ（app 常時起動中）。本調査は read-only URI 接続のみで無傷。今後の追加照会も同方針であること。

## 9. 証跡ファイル（本調査で作成、すべて %TEMP%\opencode\）

`v58_db_scan.py`（45 DB 列挙）/ `v58_db_search.py`（キーワード横断）/ `v58_schema.py` / `v58_copilot_dbs.py`（data.db 等スキーマ）/ `v58_turns_search.py` / `v58_turn3395.py` / `v58_turn_full2.py` / `v58_opencode_scan.py` / `v58_extract_part.py` / `v58_part_ctx.py` / `v58_bak_scan.py` / `v58_disasm_0016.py` / 抽出テキスト `v58_turn3395_user.txt`・`v58_turn3395_asst.txt`・`v58_part_273k.txt`
