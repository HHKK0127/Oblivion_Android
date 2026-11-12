# V170: 台帳追記粒度の統合裁定（EH funclet / Cat-3 三層 / 共有 FI / 追記候補 CSV）

- 作成: 2026-10-28 / タスク: TODO serial 157（V170・高）/ 分析専任サブエージェント（docs / TODO / session-memory / plan / `func_list_new.txt` 本体・既存 CSV の編集禁止）。
- 前提: `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（着手時・完了時の 2 回）。
- 検証対象 EXE: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（capstone 5.0.7、VA 検証に使用可だが本タスクでは**再走査なし**）。
- 編集範囲: **本新規 1 ファイル + `%TEMP%\opencode` 新規のみ**（`v170_ledger_append_candidates.csv` を実際に生成）。docs / TODO / session-memory / plan / `func_list_new.txt` / 既存 `tmp/re/*` / 既存 CSV は無改変。git / ESM 再スキャン / Ghidra 再スキャンなし。
- 中断対策どおり **証拠収集（v170_analyze.py / v170_fi333.py）→ 即 CSV 書込み（v170_build_csv.py）→ 本 md 追記** の順で実施。

---

## (0) 裁定サマリ（3行）

1. **追記粒度は「実 EH エントリ 10,716」**（unwind 7,915 + seh_thunk 2,657 + catch 144）。うち **10,572 を adopt、catch 144 を conditional**（親関数 span 内のため Ghidra body 被覆の確認待ち）。region 開始点 665 と全 10,742 は**却下**（下記 (1) の根拠）。
2. **Cat-3 三層は L3 published 10,261 entry が住所集合としての正**。L1 raw 16,660 / L2 accepted 11,463 は region 単位の中間産物で、統合表では「L1/L2 = region 数・L3 = entry 数」と明記すれば食い違いは再発しない（v162 §3-5 の裁定を追認）。
3. 追記候補 CSV 統合版を **`%TEMP%\opencode\v170_ledger_append_candidates.csv`（1,396,565 B / 10,870 行 + ヘッダ）** として生成。採用層 = L7_EH 10,716（adopt 10,572 / conditional 144）+ L6b 10 + L7_E8 11 + L6a 133。

---

## (1) 裁定事項 1: 追記粒度の採否

### 1-1 三候補の対比（判断軸 4 つの適用）

| 粒度 | 件数 | ①interior 二重化 | ②catch 144 | ③1行=1関数原則 | ④Ghidra 再分析有用性 | 裁定 |
|---|---:|---|---|---|---|---|
| **region 開始点 665** | 665 | 関数化対象が不明（開始点＝region 先頭であり、これが 1 関数か region のみの先頭か混在） | 開始点に 0 件（帯域外）＝拾えない | **不整合**（665 は region 粒度。func_list 21,597 は開始点＝関数粒度で、単位が違う） | 低い（10,572 の funclet を 639+25 に圧縮し、9,933 個の実エントリを取りこぼす） | **却下** |
| **実 EH エントリ 10,716** | 10,716 | 帯域 funclet（U/thunk）は**親関数と物理分離**＝二重化なし（帯域は台帳 gap）。catch のみ conditional | catch 144 を独立行として明示採扱い（conditional） | **整合**（各 funclet・thunk・catch は独立 body/ret を持つ 1 エントリ） | **高い**（Ghidra が E8/E9・push-imm32 で到達不能な EH body を全数可視化） | **採用** |
| **全 10,742** | 10,742 | 同上（10,716 に含む） | 同上 | 不整合（混入 26 = xcu 25 + unref 1 が関数ではない/偽） | 高いが精度低下（初期化スタブ 25 + 偽開始点 1 を無条件採用） | **却下**（10,716 + 却下 26 に分解して採用） |

### 1-2 根拠（本タスク実測・`v170_analyze_out.txt`）

- **帯域 funclet は親 region と物理的に分離している（①の決着）**: unwind 7,915 / seh_thunk 2,657 は**全数が帯域 `[0x9A9D50, 0x9D8020)` 内**（`band membership by kind` = 全 True、catch のみ全 False）。帯域は `D3DXVec4Transform=0x9A9D44` の直後から次台帳 `0x9D8020` までの**台帳 gap**（v166 §1-4 / v162 §3-1）であり、ここを body に持つ既存 Ghidra 関数は存在しない（台帳交差 **0** = v166 自己検証 12 と本タスク再確認一致）。よって帯域 funclet を個別関数化しても**親関数（帯域外）と二重登録にならない**。
  - 「親 region との二重化」は **region 開始点 665 採用時にのみ**生じる論点: region 開始点を 1 関数として登録すると、同一 region 内の残り funclet（開始点以外の interior 9,933）がその関数 span の interior として衝突する。**10,716 採用（全 funclet を個別化）では region を関数として登録しないためこの衝突自体が発生しない**（region はあくまで分類上の帯域呼称）。
- **catch 144 の位置づけ（②）**: 全 144 が帯域外・`.text` 中間で、宿主（FI 所有関数）の enclosing と **143/144 一致**（不一致 1 = `CallUnexpected` 内部、v166 §1-5）。つまり catch は「宿主関数 span の内側にある独立 body」であり、Ghidra が宿主関数を線形に开拓したときに**body が被覆する可能性**がある（本タスクは Ghidra 再スキャン禁止のため body 被覆を未証）。→ **conditional** とし、Ghidra `Function.getBody()` 突合（v133-B / R-f 系）で独立性を確認してから adopt へ昇格させる。
- **1 行 = 1 関数原則（③）**: func_list 21,597 は「開始点 = 関数」粒度。unwind funclet は各々 ret / dtor tail-jmp で完結する独立 body（v166 §1-3）、seh_thunk は cookie + continuation で完結する登録ハンドラ（2,657/2,657 が `jmp 0x980EA4` 終端）、catch は `mov eax, <next>; ret` でランタイムへ復帰する独立 handler（v166 §1-3）。**いずれも 1 エントリ = 1 callable 実体**であり、台帳の関数粒度と単位が一致する。region 開始点 665 はこれと単位が食い違う（665 は region 数、10,716 は entry 数 = Cat-3 三層と同じ region/entry 混在の罠）。
- **Ghidra 再分析有用性（④）**: 台帳交差 0 = 現行 Ghidra は 10,742 エントリを一切関数としていない。うち **interior 10,074**（union 不在）は E8/E9・push-imm32 経由で到達不能（v166 §1-2 の「Ghidra は push-imm32 を関数化しない」）ため、登録しない限りデコンパイル・エッジ解析から恒久的に落ちる。10,716 採用でこれを全数回収できる。

### 1-3 採用・却下の確定表（本 CSV に反映済み）

| kind | 行数 | status | 理由 |
|---|---:|---|---|
| unwind（cleanup funclet） | **7,915** | adopt | 帯域内・独立 body・台帳 gap に所在（二重化なし） |
| seh_thunk（SEH 登録ハンドラ） | **2,657** | adopt | 帯域内・2,657/2,657 が有効 FI 埋込 + `jmp 0x980EA4` 終端 |
| catch（TryBlockMap 由来） | **144** | conditional | .text 中間＝宿主 span 内。143/144 宿主一致だが body 被覆未証 |
| xcu_stub（初期化スタブ） | 25 | **却下**（本層から） | 非 EH。全 25 が union `L3+L4` 既出＝Cat-3 バックログ側の領域。EH 層への混入は単位汚染 |
| unref（偽開始点 `0x9AACE8`） | 1 | **却下** | 帯域内外どの参照もなし（`.text` の「参照」は E8 rel32 偶然一致）。直前 region の cookie チェック中間を指す偽エントリ（v166 §1-4） |
| **合計（採用+条件付）** | **10,716** | | |

---

## (2) 裁定事項 2: Cat-3 三層のどれを正とするか

### 2-1 三層の再掲（v162 §3-5 / v127 出典・本タスクは再走査せず追認）

| 層 | 単位 | 値 | 位置づけ |
|---|---|---:|---|
| L1 raw regions（`v127_cat3_regions.json`） | region | **16,660** | 中間産物（cls 分解: C 10,047 / D 319 / F 4,860 / A 110 / E 987 / B 337） |
| L2 accepted | region | **11,463** | = L1 − F 4,860 − B 337。region 単位の accept |
| **L3 published Cat-3a** | **entry** | **10,261** | **住所集合としての正**（`v155_union.csv` の `Cat3` 列） |

### 2-2 裁定

- **正 = L3 published 10,261（entry 単位）**。理由: 統合表・台帳追記・Ghidra 登録はすべて**住所（entry）キー**で行うため、住所集合として意味を持つのは L3 のみ。L1/L2 は region（連続コード帯）であり、region → entry の多対一関係があるため住所キーとして直接使えない。
- **「accepted 11,463 − published 10,261 = 1,202」は region − entry の単位混在減算で意味を持たない**（v162 §3-5 の裁定を追認）。住所レベルの実差分（L3 内訳 10,197 accepted 領域内 / 11 却下領域内〔すべて cls=F:noncode〕/ 53 どの L1 領域にも不在）と、L2 の 10,177 領域 ≥1 entry / 1,286 領域 0 entry（うち 1,157 = seed なし / 129 = seed ありだが入口フィルタ除外）を以降の突合の正とする。
- **統合表での表し方（確定）**: 統合表の `Cat3` 列は **L3 entry 集合**を指すと定義し、region 数を論じるときは必ず「L1/L2（region 単位）」と明記して L3 と減算しない。v155 統合 CSV は本定義のまま（`Cat3=1` = published entry）とし、変更不要。

---

## (3) 裁定事項 3: seed-B 133 / L3 exclusive 47 / I1–I10 の最終位置づけ

| 集合 | 件数 | 最終位置づけ | 本 CSV での扱い | 根拠 |
|---|---:|---|---|---|
| **seed-B 133（L6a）** | 133 | **Cat-3 published 10,261 の真部分集合**（`L6a ⊂ L4`、差 0）。pad 非依存で V86 が構造上検出不能だった分の補完。独立層だが独立 VA を新設しない | **backlog_priority 133 行**（Cat-3 バックログ内の優先群として標示。EH 層の region_start 4 件と VA 重複 = `dup=L7_EH` で明示） | v155 §1-4（L6a exclusive 0 / shared 133）・本タスク `seedB x Cat3 = 133/133` |
| **L3 exclusive 47** | 47 | **未収録 VA だが「本物の未検出エントリ」ではない**。偽境界 43（V86 C 節・無参照、x87/即値が ret/jmp 誤認）+ interior 4（前台帳から <16B）。Cat-3 側が正しい | **行なし（却下）**。台帳追記候補に**含めない** | v155 §5（Cat-3 が正しい・実質未検出 0）・本タスク `L3-exclusive = 47` 再確認 |
| **I1–I10（L6b）** | 10 | v136 §3 で台帳追記別紙として確定済み（A フォーマット 10 行）。L3∩L4 両収録・台帳 0/10・dec_index 0/10。独立追記は 10 件すべて entry 登録で足りる（I1 の残 139 スタブは V148、I2/I4/I8 の witness は同一関数 internal で不要） | **adopt_priority 10 行**（`FUN_` + 8 桁 hex。v136 §3-A の台帳形式をそのまま採用） | v136 §3 / v155 §3（pad 10/10・V86 節 10/10 一致）・本タスク `I1-I10 in union = 10` |

- **v155 で裁定済みの部分は全面追認**（本タスクは再走査せず、`v170_analyze.py` の集合演算で一致確認のみ）。seed-B 133 は「独立追記候補」ではなく「Cat-3 バックログ内の高信頼サブセット」として backlog_priority に落とし、実際の追記可否は Cat-3 mass 追記（V158 / S3 単一ソース化）の人間判断に委ねる。

---

## (4) 裁定事項 4: 共有 FI/thunk の表記方法（333 件・最大 253 関数）

### 4-1 実測（`v170_fi333_out.txt`）

- multi-host 行 **347**（unwind 201 + seh_thunk 146）、うち distinct `fi=` **150**（md の「333 FI」とは単位が異なる別口径。下記 4-3）。
- host 数分布: 2 関数共有 0 / 3 関数 142 行 / … / 最大 **253 関数**（2 行 = FI `0x00AF2E8C` の unwind + seh_thunk、宿主 `00405870:FUN_00405870(+252 more)`）。v166 §2-2 の「最大 253 関数が同一 FI/thunk `0x9CA7EB`」と一致。

### 4-2 表記方法の裁定（本 CSV の仕様）

1 行 = 1 関数エントリの原則を守り、**複数 host は列を分離して持つ**（1 セルに羅列しない）:

| 列 | 表記 | 例（253 共有 thunk） |
|---|---|---|
| `host_guess` | **primary host のみ**（`VA:NAME`、帯域への push-site / FI 参照順での先頭） | `00405870:FUN_00405870` |
| `host_count` | 共有宿主数（int。単一 = 1、宿主不定 = 0） | `253` |
| （v166 原 CSV） | `host_guess` に `first(+N more)` の旧表記が残存するが、本統合版では primary へ正規化 | ― |

- **却下案**: ①`(+N more)` 略記をそのまま転記（数値のみで host を辿れない）②全 host を `;` 区切りで 1 セルに列挙（最大 253 × 13 B ≈ 3.3 KB/行になり CSV が肥大化し、1 行 = 1 関数の可読性を損なう）。**根拠**: 既存 v166 CSV も `first(+N more)` 略記だが、これは「宿主特定は v166 原 CSV / EXE で再解決する」前提の略記であり、統合版では host 列分離で primary を確実に持ちつつ全体数を保持する。
- 全 host リストが必要な場合の再解決経路 = `v166_eh_entries.csv` の該当 VA 行（`host_guess` の `(+N more)` + EXE の push-site センサス）。統合版は再解決の起点（primary + count）を提供する。

### 4-3 md「333 FI」と本タスク「347 行 / 150 FI」の口径差

- v166 §2-2 の **333** は「複数宿主を共有する FI の数」との記述だが、本タスクの multi-host 行は 347、distinct `fi=` は 150。差の主因: (a) 同一 FI が unwind 行と seh_thunk 行の両方に現れる（FI↔U↔thunk の多対多）、(b) `fi=` を持たない宿主共有行の計上単位。**この差は表記方式の裁定に影響しない**（primary + count 方式はどちらの単位でも成立）。FI 単位の厳密再カウントは v166 残点 R2 に申し送り、本タスクでは **実測値 347 行 / 150 FI / max 253** を記録するにとどめる。

---

## (5) 成果物: 追記候補 CSV 統合版の仕様と採否表

### 5-1 出力先・規模

| 項目 | 値 |
|---|---|
| パス | **`%TEMP%\opencode\v170_ledger_append_candidates.csv`** |
| サイズ / 行数 | **1,396,565 B / 10,870 行 + ヘッダ** |
| 文字コード | UTF-8（BOM なし）・全セル ASCII |
| 生成スクリプト | `%TEMP%\opencode\v170_build_csv.py`（再現: `python v170_build_csv.py`） |
| 生成時 assert | 10,870 行 / EH 10,716 / L6b 10 / L7_E8 11 / L6a 133 / VA 跨層重複 4（= seedB ∩ EH region_start）すべて PASS |

### 5-2 列定義

| 列 | 型 | 内容 |
|---|---|---|
| `VA` | 8 桁小文字 hex | エントリ住所（func_list 書式に合わせ小文字） |
| `suggested_name` | text | `FUN_` + VA 8 桁（現行台帳の自動名規則。役割名は別途人間付与） |
| `layer` | enum | `L7_EH` / `L6b_I` / `L7_E8` / `L6a_seedB`（出典層） |
| `kind` | enum | `unwind` / `seh_thunk` / `catch` / `entry_gap` / `entry_e8` / `region_start_seedB` |
| `status` | enum | `adopt`（10,572）/ `conditional`（144）/ `adopt_priority`（21）/ `backlog_priority`（133） |
| `host_guess` | `VA:NAME` or 空 | primary host（共有時も先頭 1 件のみ。空 = 宿主不定） |
| `host_count` | int | 共有宿主数（1 = 単一、0 = 不定） |
| `scope` | text | EH 行は v166 の try 注記（例 `try1[0..0];catchHigh=3`）、他は `-` |
| `union_layer` | text | `v155_union.csv` での所属（例 `L3+L4+L6b:I1` / `absent`）。**追記前に必ず union との突合が可能** |
| `evidence` | text | 根拠要約（fi= / term= / pad= / landing insns 等） |
| `notes` | text | 補助（`dup_cat3_backlog` = Cat-3 バックログと VA 重複 / `dup=L7_EH` = EH 行と VA 重複 / interior_of_host_span 等） |

### 5-3 採否表（層別・行数）

| 層 / 種別 | 候補 | 採用 | 採用内訳 | 却下 | 却下理由 |
|---|---:|---:|---|---:|---|
| L7_EH unwind | 7,915 | **7,915**（adopt） | 帯域 funclet・独立 body | 0 | ― |
| L7_EH seh_thunk | 2,657 | **2,657**（adopt） | 帯域登録ハンドラ | 0 | ― |
| L7_EH catch | 144 | **144**（conditional） | 宿主 span 内・body 被覆未証 | 0 | （conditional は承認制） |
| L7_EH xcu_stub | 25 | 0 | ― | **25** | 非 EH・L3+L4 既出（Cat-3 バックログ側） |
| L7_EH unref | 1 | 0 | ― | **1** | 偽開始点 `0x9AACE8`（参照なし） |
| L6b I1–I10 | 10 | **10**（adopt_priority） | v136 別紙 A どおり | 0 | ― |
| L7_E8（確定 5 + 保留 6） | 11 | **11**（adopt_priority） | v140 確定 5 / v162 で保留解除 6 | 0 | ― |
| L6a seed-B | 133 | **133**（backlog_priority） | Cat-3 バックログ内優先（実追記は mass 追記判断に委ねる） | 0 | ― |
| （対象外）L3 exclusive 47 | 47 | 0 | ― | **47**（CSV 行なし） | 偽境界 43 + interior 4（Cat-3 側が正） |
| **CSV 合計** | **10,870** | 10,870 行を収録（うち本体追記の直接採用 = adopt + adopt_priority **10,593**、conditional 144、backlog 133） | | 却下 73（26 + 47）は行なし | ― |

### 5-4 適用時の注意（承認後・別タスク）

1. **VA キーで dedup**: 本 CSV 内の VA 跨層重複 4（`009b055a` `009b319a` `009b36ff` `009b7dff`）は `L7_EH` を優先、`L6a_seedB` 行は `dup=L7_EH` フラグで除外。さらに `func_list_new.txt` 既存 21,597 行（本タスク実測で台帳交差 0 を確認済みだが、適用時刻に再 assert）および Cat-3 mass 追記済み VA との dedup 要。
2. **実反映順序の推奨**: `adopt_priority` 21（I1–I10 + E8 11・件数小・根拠強）→ `adopt` 10,572（帯域 EH）→ `conditional` 144（Ghidra body 突合後）→ `backlog_priority` 133（Cat-3 mass 追記と併せて）。
3. `func_list_new.txt` 本体への反映は**本タスクでは未実施**（禁止事項）。本 CSV は反映素材であり、反映可否は人間承認後の別タスク。

---

## (6) 残る人間判断ポイント

| # | 論点 | 内容 | 関連 |
|---|---|---|---|
| H1 | **catch 144 の adopt 昇格** | Ghidra `Function.getBody()` との突合（禁止事項のため未証）で「宿主 body が catch を被覆しない」ことを確認した上で adopt へ。被覆する場合は Ghidra 側で body 分割が必要 | v166 R1 / v133-B（R-f） |
| H2 | **帯域 10,572 の一括追記の可否** | 台帳 21,597 → 約 32,169 への増加。Ghidra 再インポート運用・名前付与方針（`FUN_` 自動名のみか、`__eh_cleanup` 等の役割接尾辞を付すか）の決定 | v166 R1 |
| H3 | **Cat-3 mass 追記（10,261 entry）** | 本 CSV は EH 層と位置づけ確定分のみ。Cat-3 published 全体の台帳化は V158（S3 単一ソース化）・V159（L7 層化 11 件）の論点と併せて人間判断 | v155 S3/S4 |
| H4 | **共有 FI の同一性検証** | 253 関数共有（FI `0xAF2E8C`）等の「フレームレイアウト一致」仮説の検証。本タスクは表記方法のみ裁定（primary + count） | v166 R2 |
| H5 | **thunk 非登録 FI 6 件** | `0xAFF75C`–`0xAFFCF8`（帯域末尾、宿主不明 U ブロック 7 件の源）。登録経路特定は未着手のため本 CSV に該当行なし（unwind 行としては収録済みだが宿主不定） | v166 R3 |
| H6 | **md「333 FI」の口径確定** | 本タスク実測 347 行 / 150 FI との差（4-3）。v166 R2 の再カウント時に FI 単位を定義して決着 | v166 R2 |
| H7 | **suggested_name の役割付与** | 全 10,870 行が `FUN_` 自動名。catch / thunk / cleanup の役割接尾辞（例 `FUN_009a9d50__eh_cleanup`）は可読性向上だが台帳書式変更を伴う | v136 §3-B 拡張ラベル案 |

---

## (7) 自己検証

- `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（着手時・完了時の 2 回）。
- 内部整合（本タスク実測、`v170_analyze_out.txt` / `v170_fi333_out.txt`）:
  1. v166 CSV 10,742 行、kind 内訳 unwind 7,915 / seh_thunk 2,657 / catch 144 / xcu_stub 25 / unref 1 = **10,742**（v166 §2-2 と完全一致）。
  2. region_start 665 = unwind 611 + seh_thunk 28 + xcu_stub 25 + unref 1（v166 §1-4 と完全一致）。
  3. v166 × v155 統合 CSV 突合: union `absent` 10,074 + `L3` 1（`0x9A9D50`）+ `L3+L4` 663 + `L4+L6a` 4 = 10,742、**台帳（L1）交差 0**（v166 自己検証 8・12 と一致）。
  4. band membership: unwind / seh_thunk / xcu_stub / unref は全行帯域 `[0x9A9D50, 0x9D8020)` 内、catch のみ全行帯域外（= 1-2 の物理分離根拠）。
  5. 宿主付与 10,709 / 10,742、catch 144/144（v166 §2-2 と一致）。multi-host 347 行 / distinct FI 150 / max 253（2 行、FI `0xAF2E8C`）。
  6. L3-exclusive = **47**、seedB = **133**（seedB ∩ Cat3 = 133/133）、I1–I10 = **10**、E8 11 件の統合表不在 = **11/11**（v155 §5 / §9 / §10 と一致）。
  7. 出力 CSV: 10,870 行 / 1,396,565 B、VA 跨層重複 **4**（seedB ∩ EH region_start: `009b055a` `009b319a` `009b36ff` `009b7dff`、いずれも EH 行が `dup_cat3_backlog` フラグ持ち）。
- 禁止事項の遵守: docs / TODO / session-memory / plan / `func_list_new.txt` / 既存 CSV / 既存 `tmp/re/*` の編集、git、ESM 再スキャン、Ghidra 再スキャン **すべて未実施**。新規作成物は本 MD（`tmp/re/v170_ledger_granularity.md`）+ `%TEMP%\opencode\v170_*.py`（3 本）+ `v170_ledger_append_candidates.csv` + `v170_*_out.txt`（2 本）のみ。

---

生成物: `tmp/re/v170_ledger_granularity.md`（新規）+ `%TEMP%\opencode/v170_ledger_append_candidates.csv`（新規・1,396,565 B / 10,870 行） / 生成スクリプト: `%TEMP%\opencode/v170_analyze.py`, `v170_fi333.py`, `v170_build_csv.py` / 前提: `probe_harness.py selftest` 4/4 PASS
