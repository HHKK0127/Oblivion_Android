# V164: ESM 接点の事前整備 — 解禁時走査仕様（TODO serial 154 / V154・低）

作成: 2026-10-27（opencode / 分析専任サブエージェント）
対象成果物: 本ファイル（**新規・1 ファイル**）
目的: ESM 再スキャン解禁時に打つ「3 発 +1 発」（CTDA 生バイト / SCDA 22 回所在 / SCDA 頻度 368 再計測 / SCHR 突合）の**走査仕様を確定する**。**走査自体は本タスクでは一切実施しない。**

---

## §0 手法・制約・入力

### 0.1 制約遵守

| 制約 | 本タスクの状態 |
|---|---|
| `Oblivion.esm`（277,504,985 B / 1,167,017 レコード）・DLC ESP 本体への**読み取り走査** | **未実施（0 回）**。パス所在とファイルサイズ（メタデータ）のみ確認 |
| docs / TODO / session-memory / plan 編集 | 未実施。`plan.md` は**読取のみ** |
| 既存 `tmp/re` 成果物の上書き | 未実施。本ファイルは新規 |
| スクリプト置き場 | `%TEMP%\opencode\v164_probe1.py` / `v164_probe2.py`（新規 2 本） |
| `probe_harness.py selftest` | 着手時に実行 → **4/4 PASS**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl=0x504560） |

### 0.2 参照した既存成果物（すべて読取のみ）

| ファイル | 本仕様での用途 |
|---|---|
| `tmp/re/v_data_relmap.md` §F2（F2 範囲 (a)–(e)、推定工数 2–3 人日、順位5） | 骨格・工数の一次出典 |
| `tmp/re/b21_speculative_backing.md` §1.1/§4/§7-8/§8.2 | TrapUpdate 22 回・2 ソース一致・残点 8 |
| `tmp/re/b22_getter_types.md` §2 / 残点 V143-1 | 0x1035/0x104F の impl=0・cond=0x4F6710・判定式の前提 |
| `tmp/re/b20_impl_5071d0.md` §4/§6-3 | CTDA +8 = X 形式の論理帰結と一次証未実施の残点 |
| `tmp/re/v141_b19_quest_residuals.md` §4 | SCHR imm32 5 箇所・3 読込経路・保存側対称・ctor 5 呼出し元 |
| `tmp/re/copilot_archive/scda_opcode_table.py` / `scda_opcode_table_out.txt` | SCDA walk 仕様・141 distinct 表（Σ=63,235） |
| `tmp/re/s57_svm_8_opcode_table.txt` / `s57_script_vm_table.txt:443` | 2 ソース目の 141 表・TrapUpdate 行 idx431 |
| `tmp/re/s58_qst_condtable.txt` | CTDA[+8] u16 → 関数帯の**対応式**（基底 0xB0C8C0 / stride 0x28 / N=369 / range check <0x171） |
| `docs/RE_EXE_REPORT.md` §67.6（480 行）/ §67.7 / §18.2 | 台帳 112 正 / 368 `-`、PACK CTDA 6,172 |
| `tmp/re/plan.md`（読取のみ） | GRUP-aware walk の罠 3 点・SCHR オンディスク GT・corpus 規模・owner 別頻度 |
| `tmp/re/copilot_archive/content_inventory_final.txt` / `esp_inventory.txt` | ESM/DLC レコード数 GT（**errs=0 の final 版を GT とする**） |
| `tmp/re/copilot_archive/schr_field_probe3_out.txt` | SCHR 既知 GT（2,393/2,393 一致系） |
| `tmp/re/s50_gmst_*.txt` / `s50_gmst.md` / `v60_esm_only_keys.md` / `v93_gmst_gt_residuals.md` | **GMST / INI キー専用**（CTDA・SCDA・SCHR は非カバー = §1.1 の判定根拠） |

### 0.3 ファイル所在（メタデータ確認のみ・内容未読）

- `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Data\Oblivion.esm` = **277,504,985 B（存在）**
- `D:\Cargo\Other\Oblivion\Oblivion_Steam版\Data\Oblivion.esm` = **277,504,985 B（存在、同一サイズ）**
- DLC ESP 10 件（`D:\Cargo\Other\Oblivion\Oblivion_Steam版\Data`）: Knights 2,211,935 / DLCMehrunesRazor 728,780 / DLCBattlehornCastle 676,894 / DLCSpellTomes 625,246 / DLCThievesDen 233,735 / DLCVileLair 257,153 / DLCFrostcrag 191,358 / DLCHorseArmor 97,179 / DLCOrrery 47,424 / **DLCShiveringIsles 85（ヘッダのみスタブ = 走査対象外**、`content_inventory_final.txt:35`）
- **中間キャッシュ不在を確認**: `%TEMP%\async_race\corpus.pkl`（plan.md 記載の 2,646 (SCDA,SCTX) ペア cache）および `%TEMP%\scro_probe\` は**存在しない** → 再走査なしに SCDA/SCTX ペアは復元不可。`copilot_archive\scda_cache.pkl` も欠損（b21 §0.2）。
- 走査コストの目安: **ESM 全体 walk 約 300 s**（plan.md L823 の記載）、DLC は 10 ファイル合計約 4.3 MB で数秒。

---

## §1 仕様①: CTDA 生バイト（0x1035 / 0x104F の ESM 実在確認）

出典: `b22_getter_types.md` 残点 **V143-1** ＋ `b20_impl_5071d0.md` §6-3。

### 1.1 既存キャッシュのカバー範囲（確定）

| 既存キャッシュ | 0x1035 / 0x104F の扱い | CTDA バイトの扱い | カバー判定 |
|---|---|---|---|
| `copilot_archive/scda_opcode_table_out.txt`（141 行） | **不在（0 件）** — 本回実測で absent | **不含**（SCDA = コンパイル済み命令列のみ） | **0%**（条件帯は構造上カバー外） |
| `s57_svm_8_opcode_table.txt`（141 行） | **不在** — 本回実測で absent | 不含 | 0%（同一走査の表結合） |
| `s50_gmst_*`（`s50_gmst_esm2.txt` = ESM GMST 382 件） | n/a | 不含 | 0%（GMST レコードのみ） |
| `s57_*`（vm 表 / opcode 表 / ctormap / script_vm） | EXE 側表に**実在** | 不含 | EXE 側のみ |
| `s58_qst_condtable.txt`（369 エントリ） / `s64_ai_cond_*` | **EXE 側に実在**: `53 1035 GetScriptVariable pCnt=2 impl=0 cond=004F6710 isCond=YES` / `79 104F GetQuestVariable 同` | 不含 | EXE 表のみ（ESM バイト非参照） |
| `plan.md` / `docs/RE_EXE_REPORT.md` §18.2 | n/a | **件数のみ**: PACK CTDA **6,172** + CTDT 18 / QUST CTDA **3,784** / INFO CTDA **48,531** + CTDT 72 | **opid 分布 = 0%** |

> **結論 [確定]**: 0x1035/0x104F の「命令としての出現」は 2 ソースで 0 件（= b22 の記載を再現）、
> 「**条件（CTDA）としての出現**」は**どの既存キャッシュも一切カバーしていない**。
> よって解禁時は **ESM 全レコードの CTDA 走査が必須**（部分キャッシュによる代替不可）。

### 1.2 走査仕様（レコード範囲・オフセット・判定式）

**走査方式**: GRUP 降下型 record-boundary walk。`plan.md` の既知罠 3 点を**必ず踏襲**する
（この 3 点を外すとレコード数・サブレコード境界が崩れる = plan.md L463）:

1. **圧縮レコード**（flags & `0x00040000`）は **20 B ヘッダを維持**し、dataSize の先頭 4 B が展開後サイズ前置（追加ヘッダではない）。圧縮は `NPC_` 全 2,482 / `LAND` 31,083 / `PGRD` 8,224 のみ、**`SCPT`/`QUST`/`INFO` は圧縮 0 件**。
2. **ゼロサイズレコード**（ESM に 1 件のみ: `STAT` FormID `0x0008050C` @ offset 7,984,486）は `p += 20` で**スキップ（ただし件数には数える）**。件数 GT = **1,167,017 レコード / 85,079 GRUP / 残余 0 B**。
3. **GRUP は降下する**（`p += 20` して end を push）。トップレベルのみ走査すると PACK が 0 件になる（RE_EXE_REPORT §18.1 の教训）。

**対象範囲**: `Oblivion.esm` 全レコード（親レコード種を**問わない**）＋ DLC ESP 9 件（§0.3 のスタブ除外）。
サブレコード **tag ∈ {`CTDA`, `CTDT`}** を全数収集する（=「レコード範囲」は特定 owner 3 種に絞らない。既知 owner は PACK/QUST/INFO の 3 種のみで、**他 owner の有無自体が未特定** = §7-N4）。

**オフセット・判定式**（すべて EXE 側確定済みの対応を使う）:

```
payload  = CTDA payload（想定 24 B。実測 size ヒストグラムを必ず出力）
i        = u16le(payload[8:10])                      # b20 §4.3: FUN_0056A9C0 が +8 へ書くのは X のみ
opid     = 0x1000 + i                if i <= 0x170    # s58_qst_condtable.txt:3 mapping
         = i                          if i <= 0x1170  # b20 §4.3 の「X 入力 → canonical 復元」対偶（防御分岐）
         = OUT_OF_BAND                                # それ以外 → invalid として別集計
target   = (opid == 0x1035) or (opid == 0x104F)       # i == 0x0035 / i == 0x004F
```

**補助一致式（EXE 表との突合）**:

```
entry    = 0xB0C8C0 + i * 0x28            # 関数帯基底 0x00B0C8C0 / stride 0x28 / N=369（s58:2,5）
cond_ptr = u32le(EXE_image[entry + 0x20])  # 0x56AC50 が読む condfunc（b22 §2.3-2）
i == 0x35  -> entry 0xB0D108, cond 0x4F6710（imm32 参照 0xB0D128 = idx216+0x20, b22）
i == 0x4F  -> entry 0xB0D518, cond 0x4F6710（imm32 参照 0xB0D538 = idx242+0x20, b22）
```

**一次証としての副産物（b20 §6-3 を同時に消す）**: `i` の**全ヒストグラム**を出力する。
`i` の max ≤ `0x170` が全件成立すれば「**ファイルの CTDA は X 形式**」という b20 §4.4 の論理帰結が
**生バイト一次証**として成立する（`i >= 0x1000` が 1 件でもあれば反証 = 台帳・b20 の裁定見直し案件）。

### 1.3 出力スキーマ

| ファイル | 列 / 内容 |
|---|---|
| `v164_ctda_hist.tsv` | `owner_tag  i  opid  name_or_empty  count`（owner × i の全ヒストグラム） |
| `v164_ctda_targets.tsv` | `file  rec_offset  formid  owner_tag  edid  sub_offset  i  opid  cond_ptr  payload_hex_24`（0x1035/0x104F の**全出現行**。0 件なら空ファイル + 0 件明記） |
| `v164_ctda_sizes.json` | `total_ctda / total_ctdt / size_histogram / owner_counts / new_owner_tags` |

### 1.4 検証式（受理 / 棄却）

| # | 検証 | 受け入れ値（既知 GT） |
|---|---|---|
| C1 | CTDA 総数 | **≥ 58,487**（PACK 6,172 + QUST 3,784 + INFO 48,531）。既知 3 owner の内訳が**個別に一致**すること |
| C2 | CTDA size | 全件 **24**（逸脱は全列挙。`b20` のロードは 0x18 生読み = `FUN_00450C20(record,dest,0x18)`） |
| C3 | `i` の範囲（X 形式証） | `max(i) <= 0x170`（`OUT_OF_BAND` 0 件） |
| C4 | 0x1035 / 0x104F 出現 | 実測値を報告（**0 件でも「バニラ未使用」として確定値**になる。現状の予測は「存在し得る」のみ、頻度は未知） |
| C5 | cond_ptr 一致 | `i=0x35` / `i=0x4F` の全出現で `cond_ptr == 0x4F6710` |
| C6 | CTDT の扱い | size と `i` を**別集計**（plan.md: INFO CTDT = 20 B。20 B で `payload[8:10]` は読めるが、b20 の 0x18 生読みとの整合は未確認 = §7-N3） |

---

## §2 仕様②: TrapUpdate `0x110C` の ESM 22 回の所在特定手順

出典: `b21_speculative_backing.md` §4（発火元ゼロ・impl=`0x977C50` no-op・**ESM 22 回はソース由来**）＋ 残点 **§7-8**（`scda_cache.pkl` 欠損・DLC ESP 未走査）。

### 2.1 前提（b21 §4.6 の確定分を踏襲）

- EXE 内に 0x110C を**生成・分岐・発火**するコードは存在しない（name ptr xref = .data の name フィールドのみ、行 VA xref = 0、imm32 8 件は TreeView `TVM_GETITEM` 3 + バイト偶発 5）。
- 生成経路は「**スクリプトソースに `TrapUpdate` と書かれた箇所**」のみ（表の汎用走査経由）。
- 実行時は共有スタブ `0x977C50` = `mov al,1; ret`。
- → 所在特定の価値 = **どのスクリプトが 22 回呼んでいるか（EDID + ソース行）** を確定し、b21 §7-8 を閉じる。

### 2.2 走査対象・キー・walk 規則

**対象（SCDA ブロック全数）**:

| ファイル | ブロック | 備考 |
|---|---|---|
| `Oblivion.esm` | **9,646** = SCPT 2,393 + INFO 5,552 + QUST 1,701 | plan.md L455 の GT。**SCPT 2,393 だけでは 22 回の再現に十分だが、QUST/INFO/DLC は未知**（= 新規計測） |
| DLC ESP 9 件 | SCPT **653** / INFO **1,055** / QUST **26**（`content_inventory_final.txt` の合計） | 一切未走査。**`esp_inventory.txt` は errs>0 の旧走査のため GT に使わない** |

**walk 規則**（`copilot_archive/scda_opcode_table.py` を流用、**tail 例外の扱いを §3-3 の慣例に合わせる**）:

```
instruction = [u16 op][u16 argLength][argLength bytes]   # advance = 4 + argLength
exception   : op == 0x001C -> 固定 4 B（argLength 無視）
prologue    : 先頭 4 B = 1d 00 00 00（0x001D、SCPT のみ）
terminator  : 末尾 4 B = 11 00 00 00（0x0011 STOP）
split block : レコード内の複数 SCDA サブレコードは**連結せずブロック単位**で走査
              （QUST は 1 レコード最大 11 ブロック、INFO は最大 1、SCPT は常に 1。210 分割は非 SCPT のみ = plan.md L813）
```

**キー（1 出現 = 1 行）**:

```
(file, rec_offset, formid, owner_tag, edid, block_ordinal, instr_index, byte_offset)
```

### 2.3 出力スキーマ

| ファイル | 列 / 内容 |
|---|---|
| `v164_trap_sites.tsv` | `file  rec_offset  formid  owner_tag  edid  block_ordinal  instr_index  byte_offset  op  arglen  prev_op  next_op  sctx_line_no  sctx_line_text` |
| `v164_trap_summary.json` | `{"esm_scpt": n, "esm_qust": n, "esm_info": n, "dlc": {"<file>": n}, "arglen_hist": {}, "sctx_keyword_hits": n, "walk_fail": 0, "sources": {"scda_table_out": 22, "s57_join": 22}}` |

### 2.4 検証式

| # | 検証 | 受け入れ値 |
|---|---|---|
| T1 | **walker 回帰ゲート**（最初に必ず通す） | ESM SCPT で `0x110C` 出現 = **22**（`scda_opcode_table_out.txt` と `s57_svm_8_opcode_table.txt` の 2 ソース一致値、b21 §8.2） |
| T2 | arglen | **全 22 件が argLength=0**（histogram `0:22`。→ 命令長 4 B 固定） |
| T3 | walk 完走 | SCPT 2,393/2,393、全体 9,646/9,646、framing fail 0（plan.md L503 の 0 misalignment 再現） |
| T4 | ソース突合 | 同一レコードの SCTX に `TrapUpdate` キーワードが在るか。**コメント混入で SCTX 側が多い場合は許容し差分を報告**（`§7-N6`） |
| T5 | DLC・QUST/INFO | 既知値なし → **新規計測値として確定**（0 件なら「未使用」として記録） |
| T6 | 慣例非依存 | tail は必ず `0x0011` 固定のため `0x110C` は §3-3 の両慣例で**同一値**（=22 は不変） |

---

## §3 仕様③: SCDA 頻度 368 の再計測仕様

出典: `docs/RE_EXE_REPORT.md` §67.6（480 行 = 正 **112** / `-` **368**、本回パースで再現）＋ §67.7-1 ＋ b21 §1.3・§7-9。

### 3.1 本回で確定した前提（既存 2 ソースの一致状況）

| 検査（本回 `v164_probe1/2.py` 実測） | 結果 |
|---|---|
| `scda_opcode_table_out.txt` 行数 / Σcount | **141 distinct / Σ = 63,235** |
| `s57_svm_8_opcode_table.txt` 行数 / Σcount | **141 /63,235**、2 ソースの per-op 差分 **mismatch 0** |
| §67.6 を 480 行パース | 正 **112** / `-` **368** / 他 **0** / unique opid 480 |
| 台帳 112 正頻度 vs SCDA 表 | **mismatch 0（112/112）**（b21 §8.2 の「22/22」はその部分集合） |
| `-` 368 件のうち SCDA 表に在る opid | **0 件**（= 368 件はすべて **SCPT では 0 回**） |
| 台帳に載る 0x110C / 0x1035 / 0x104F | すべて **不在**（0x110C/0x1035/0x104F は §67.9 の非掲載 52 件側） |

### 3.2 「63,235 vs 65,628」の慣例差 — 本回で**算術的に解明**

`plan.md` は SCPT 命令数を 2 箇所で別値で書く（L454 = **63,235** / L507 = **65,628**、差 = 2,393 = ブロック数）。
`scda_opcode_table.py` の walk は `stop = n - 4` のため**末尾 4 B（tail STOP）を数えず**、4 B 単独ブロック（**72 件**）は `n == 4` 早期 return で **0 件**扱い。

```
63,235 (tail 除外 + prologue-only 0 件)
  + 2,321 (SCPT の tail STOP = plan.md L541「2,321 は 0x0011 で終端」)
  +    72 (prologue-only ブロックの prologue 1 命令)
  = 65,628  == plan.md L507 の SCPT 値            ... 一致
0x0011: 2,023 (表) + 2,321 (tail) = 4,344 == plan.md 全体表の 0x0011 値   ... 一致
0x001D: 2,321 (表) +    72        = 2,393 == plan.md「2,393/2,393 に 1 個」... 一致
```

**裁定案 [確定・算術 3 系統一致]**: 正しい SCPT 命令数は **65,628**（= tail を含む慣例）、
`scda_opcode_table_out.txt` / `s57_svm_8` / 台帳 §67.6 の値は**すべて tail 除外慣例**。
**両慣例の差を受ける台帳行は 112 のうち 2 行のみ**（`0x0011`: 2,023 → 4,344、`0x001D`: 2,321 → 2,393）、
他 110 行は両慣例で同一（tail が 0x0011 固定のため）。

### 3.3 再計測で検証すべきこと（順序付き）

1. **walker 回帰ゲート**: ESM SCPT で distinct = **141**、per-op が 2 ソースと **mismatch 0**、walk 完走 2,393/2,393、framing fail 0。
2. **慣例列の二重出力**: `tail_excl` / `incl` の両列を出し、台帳に更新する列を裁定する（推奨: `incl` を GT とし、既存 112 のうち 2 行のみ差分を明示。**台帳編集は本タスク権限外** = F1 反映バッチへ送る）。
3. **全体突合**: ESM 全体 **9,646 ブロック / 86,916 命令 / 156 distinct**、owner 別 **SCPT 65,628 / QUST 10,326 / INFO 10,962**（plan.md L503–510）と一致すること。
4. **台帳 join**: §67.6 を 480 行パースし、**キーは OpID のみ**（名称列は `0x0015 = OnLoad` 等の誤記多数 = 本回確認。名称キーは禁止）。112 正頻度が SCPT 列と **mismatch 0** を再現。
5. **368 `-` の内訳確定**（本回の集合論 + 走査で確定）:
   - 368 件は**すべて 141 表に不在 = SCPT では 0 回**（本回実測）。
   - SCPT 外で出現する distinct は plan.md 上 **156 − 141 = 15 種**（`9 opcodes exclusive to QUST` + `8 exclusive to INFO` = 17 と 15 の差 2 は「QUST/INFO 双方に現れる SCPT 外 opcode」で調和するはず → **走査時に確定** = §7-N2）。
   - → **ESM 全体で正になるのは最大 15 件のみ**、残 **353 件は ESM で 0 回確定**（DLC 由来の正は別列）。
6. **DLC 列の追加**: ESM と DLC を**別列で保持**（`Freq` 列の定義を混ぜない。推奨スキーマ §3.4）。
7. **TrapUpdate の不変性確認**: `0x110C` は両慣例で同一値（§2.4 T6）。

### 3.4 出力スキーマ

`v164_scda_freq.tsv`:

```
opid  name  table_idx  esm_scpt_tail_excl  esm_scpt_incl  esm_qust  esm_info  esm_total  dlc_total  ledger_67_6  status
status ∈ {matched_scpt, filled_dash_positive, zero_confirmed_esm, new_outside_ledger}
```

付随: `v164_scda_owner_matrix.json`（owner × band のブロック数/命令数/distinct）。

---

## §4 仕様④: SCHR 供給源の ESM 突合仕様

出典: `v141_b19_quest_residuals.md` §4（EXE 側確定）＋ `plan.md` L921（ESM 側既知 GT）。

### 4.1 EXE 側で確定済みの対応（突合の相手）

| 区分 | 内容 |
|---|---|
| imm32 `'SCHR'` = `0x52484353` | 画像内 **5 箇所**: `0x4FA81F` / `0x4FC23D` / `0x52A745` / `0x52B21F` / `0x531370` |
| **読込 3 経路（すべて `Script+0x18` へ 20 B 直書き）** | A = Script vtable slot7 `FUN_004fc0d0`（`lea ecx,[ebx+0x18]`）／ B = SCPT ローダ `FUN_00531290`（dest `0xB36548` = 全域 `0xB36530` + 0x18）／ C = QUST/INFO 段階 `FUN_0052b1f0`（コンテナ `+0x24` = 埋込 Script + 0x18） |
| **保存側対称** | slot9 `FUN_004fa810`: `lea eax,[esi+0x18]; push eax; push 0x52484353; push 0x14` → **SCHR(0x14) ≡ `Script+0x18..+0x2B`** |
| ctor 5 呼出し元（v141 §4.4） | `FUN_00531290`（SCPT ローダ）/ `FUN_0052acc0`（QUST コンテナ）/ `FUN_00530db0`（破棄・キャッシュクリア）/ `FUN_00447df0`（formType ファクトリ）/ `FUN_009e4120`（静的初期化） |
| 内部規約 | `FUN_004fc7a0` が **`mov dword [esi+0x18],0`（unused 強制 0）** + 先頭 5 dword 複写 |

**フィールド対応（EXE ↔ ESM は同一物）**: `+0x18`=unused(0) / `+0x1C`=refCount / `+0x20`=compiledLength / `+0x24`=lastVarIndex / `+0x28`=**scriptType = SCHR[+0x10]**。

### 4.2 ESM 側の走査仕様

**対象**: `SCHR` サブレコード**全数**（既知の母数: SCPT **2,393** / QUST **2,978**（QSDT と 1:1）/ INFO **19,231** = **24,602**、+ DLC）。QUST/INFO の SCHR も **20 B**（plan.md L476 の全レコード走査で確定）。
**方式**: §1.2 の同一 walker（1 本の pass で §1〜§4 の全出力を得る）。

**検証式**:

| # | 検証 | 受け入れ値（既知 GT / 新規） |
|---|---|---|
| S1 | size | 全件 **20**（逸脱 0） |
| S2 | `unused (+0)` | **全件 0**。1 件でも非 0 なら EXE の `mov [esi+0x18],0` がロード時に情報を消し、**保存でファイルと不一致（round-trip 破壊）**になる → 移植上の欠陥情報 |
| S3 | `compiledLength (+8)` vs `len(SCDA)` | SCPT: **2,393/2,393 一致**（既知 GT）。QUST/INFO: SCDA 有 = 一致、SCDA 無（QUST 1,277 / INFO 13,679）は `compiledLength == 0` かを**実測**（plan.md L496 の「header without body」構造の裏付け） |
| S4 | `refCount (+4)` | SCPT: `== len(SCRV) + len(SCRO)` **2,393/2,393**（既知 GT）。QUST/INFO は SCRO のみ（QUST SCRO 6,119 / INFO SCRO 8,405）で同式が成立するかを**実測** |
| S5 | `lastVarIndex (+12)` | SCPT: `>= max(SLSD index)`（既知: equal 2,250 / above・below 0 件、`lt = 0`）。QUST/INFO に SLSD は無し（plan.md のサブレコード集合）→ N/A 記録 |
| S6 | `scriptType (+16)` ヒストグラム | SCPT: **{0: 2,031, 1: 265, 256: 97}**（既知 GT、`static_cast` 不可）。**QUST / INFO / DLC は新規計測**（値の予想は立てず「測定」扱い。`0/1/256` 以外が出た場合に EXE のマッピング（Object/Quest/Magic）が不足する） |
| S7 | owner × loader 対応表 | SCPT → 経路 B / QUST → 経路 C / INFO → 経路 C（または A）を、**各 owner の SCHR が実際に 3 経路のどれで読まれるか**の対応表として記録（EXE 側の `formType` 表 `0xB05E00` の 13=SCPT / 58=INFO / 59=QUST を根拠に） |
| S8 | walker 回帰 | SCPT の S1–S6 が既知 GT と **mismatch 0**（= walker の ESM パーサが正しいことのゲート） |

### 4.3 出力スキーマ

`v164_schr_invariants.tsv`:

```
owner_tag  n_schr  size_ok  unused_zero  cl_eq_scda  cl_zero_no_scda  refcount_ok  lastvar_ok  scriptType_hist  loader_path  dlc_delta
```

---

## §5 解禁時チェックリスト（優先順・推定工数・エントリポイント）

**共通エントリポイント**

| 項目 | 内容 |
|---|---|
| walker | 新規 `%TEMP%\opencode\v164_pass1.py`（**1 本の GRUP-aware walker が 4 出力**を同時生成: CTDA hist / SCDA freq / Trap sites / SCHR invariants）。`copilot_archive/scda_opcode_table.py` の `walk()` を流用し、**tail 除外・含むの両モード**を実装する |
| DLC walker | 同ループの `*.esp` 分岐（`copilot_archive/esp_scan.py` のロジック流用、**GT は `content_inventory_final.txt`**） |
| 回帰ハーネス | `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（着手時・完了時の 2 回。本回は着手時に PASS 確認済み） |
| 台帳 join | `docs/RE_EXE_REPORT.md` §67.6 を 480 行パース（**OpID キーのみ**）。編集は**禁止**（出力は `tmp/re` 配下の新規ファイルへ） |
| 禁止 | ESM/ESP 本体の**読取走査以外の目的での**開示、git、docs/TODO/session-memory/plan 編集、既存 `tmp/re` 上書き |

| 順 | タスク | 消去する残点 | 推定工数 | 受け入れゲート |
|---|---|---|---|---|
| **P0** | **walker 回帰ゲート**: selftest 4/4 + ESM SCPT の SCDA 表を 2 ソースと突合（mismatch 0）+ SCHR SCPT 既知 GT 再現（S1–S6） | （後続全件の足場） | **0.25 人日** | distinct 141 / 112 正頻度 mismatch 0 / SCHR mismatch 0 |
| **P1** | **CTDA 生バイト**（§1）: 全レコード CTDA/CTDT walk → `i` ヒストグラム + 0x1035/0x104F 全出現 | **b22 V143-1** ＋ **b20 §6-3**（X 形式の一次証） | **0.75 人日** | C1–C6 |
| **P2** | **TrapUpdate 22 回の所在**（§2）: 9,646 + DLC ブロックの 0x110C 行特定 + SCTX 行対照 | **b21 §7-8** | **0.25 人日** | T1–T6（T1 = 22 再現が最初の合図） |
| **P3** | **頻度 368 再計測**（§3）: owner 別 freq + 慣例両列 + 台帳 join + 368 内訳 | **§67.7-1** / **b21 §7-9** | **0.5 人日** | 1–7（特に「353 件は ESM 0 回」の確定） |
| **P4** | **SCHR 突合**（§4）: QUST/INFO/DLC 拡張 + owner × loader 対応表 | **b19 §5-3 の ESM 突合半分**（EXE 側は v141 で確定済） | **0.5 人日** | S1–S8 |
| **P5** | **DLC ESP pass**（P1–P4 の DLC 列）: SCPT 653 / INFO 1,055 / QUST 26 ブロック + SCHR/CTDA | b21 §7-8 の「DLC SCDA 未走査」 | **0.25 人日** | content_inventory_final の件数と一致 / errs 0 |
| | **合計** | F2 解禁分（a)(b)(c) の 3 発 + 頻度補完 | **2.5–3 人日**（`v_data_relmap` F2「解禁後 2–3 人日」と整合） | |

**推奨実行順の根拠**: P0 を先に置くと、以降の P1–P4 は**同じ 1 回の ESM pass（約 300 s）**の副産物として得られ、
走査コストを「1 回 + DLC 1 回」に固定できる。P1 を P2 より前にするのは、**P1 が 2 つの独立残点（V143-1 + b20-3）を同時に消し、
判定式が既に EXE 側で確立済み（= 追加分析ゼロ）だから**。P3 は台帳反映（F1）の入力になるため、P4 より前に回す。

---

## §6 本タスクの実測（自己検証）

| # | 検査 | 結果 |
|---|---|---|
| 1 | `probe_harness.py selftest` | **4/4 PASS**（bytes@0x504050 `518b4c24188b5424` / bytes@0x4FB580 `578b7908` / string@0xA7035C `.\AI\CombatController.cpp` / opid0x0015 impl `0x504560`） |
| 2 | `scda_opcode_table_out.txt` パース | distinct **141** / Σ **63,235** / `0x110C` = **(22, arglen 0..0)** / `0x1035`・`0x104F` **absent** / `0x1039` = 1,391 |
| 3 | `s57_svm_8_opcode_table.txt` パース | distinct **141** / Σ **63,235** / per-op **mismatch 0** / `0x110C` = 22（tblIdx 431, impl 00977C50, dispatch 004FDE30） |
| 4 | §67.6 480 行パース | 正 **112** / `-` **368** / 他 0 / unique 480 / 正頻度 vs SCDA 表 **mismatch 0** / `-` 行で SCDA 表に在るもの **0 件** |
| 5 | 台帳の 0x110C / 0x1035 / 0x104F | すべて**不在**（§67.9 の非掲載 52 件側） |
| 6 | 63,235 vs 65,628 の分解 | `63,235 + 2,321 + 72 = 65,628`、`0x0011: 2,023+2,321 = 4,344`、`0x001D: 2,321+72 = 2,393` の **3 系統一致** |
| 7 | `s58_qst_condtable.txt` の対応式 | `53 1035 GetScriptVariable pCnt=2 impl=0 cond=004F6710 isCond=YES` / `79 104F GetQuestVariable 同`（基底 0xB0C8C0 / stride 0x28 / range check <0x171） |
| 8 | CTDA 既知件数の突合 | PACK **6,172**（+CTDT 18、RE_EXE_REPORT §18.2）/ QUST **3,784** / INFO **48,531**（+CTDT 72、plan.md）= **58,487** |
| 9 | 中間キャッシュ不在 | `%TEMP%\async_race\corpus.pkl` / `%TEMP%\scro_probe` / `copilot_archive\scda_cache.pkl` — **いずれも不在**（再走査必須の根拠） |
| 10 | ESM/ESP パス（メタデータのみ） | ESM 2 パスとも **277,504,985 B 存在**、DLC ESP 10 件のサイズ取得（**内容未読**） |
| 11 | `tmp/re` 内の CTDA 言及ファイル | 31 件。**ESM 側 opid 分布を持つものは 0 件**（すべて EXE 側表・件数・文章） |

証跡: `%TEMP%\opencode\v164_probe1.py` / `v164_probe2.py`（既存成果物の読取のみ、ESM 非接触）。

---

## §7 新規残点

| # | 残点 | 内容 | 解消タイミング |
|---|---|---|---|
| N1 | **台帳 Freq 列の定義裁定** | tail 除外（既存 112）か tail 含む（GT 65,628 系）か、または `Freq_scpt` / `Freq_esm_total` / `Freq_dlc` の 3 列化か。既存 112 のうち**差が出るのは `0x0011`・`0x001D` の 2 行のみ** | P3 完了時（反映は F1 バッチへ） |
| N2 | **SCPT 外 15 種の実在確認** | plan.md は `exclusive9 + 8 = 17` と `156 − 141 = 15` を併記（差 2 = QUST/INFO 双方出現の推定）。368 `-` のうち正になる実数（上限 15）は走査時確定 | P3 |
| N3 | **CTDT の size と b20 の 0x18 生読みの整合** | plan.md は INFO CTDT = **20 B**、b20 のロードハンドラは `CTDA`/`CTDT` とも `FUN_00450C20(...,0x18)`。20 B サブレコードで 0x18 読みが成立するのか（超過読み or size 判定あり）が未確認 | P1（size ヒストグラムで観測） |
| N4 | **既知 3 owner 以外の CTDA 保有レコード種** | PACK/QUST/INFO の 3 種の合計 58,487 が既知。他 owner（IDLE/FACT/PACK 外等）の有無・件数は未特定 | P1（`new_owner_tags` で確定） |
| N5 | **DLC ESP の SCDA/SCHR/CTDA 全未走査** | 既存 `esp_inventory.txt` は errs>0 の旧走査（内容未検証）。DLC 側の TrapUpdate/頻度/scriptType は未知値 0 件 | P5 |
| N6 | **SCTX キーワードと SCDA 出現の差分** | 「ソース由来」の裏付けとして SCTX の `TrapUpdate` 出現数と 22 の対照を行うが、コメント内キーワードで不一致になり得る（許容し差分報告） | P2 T4 |
| N7 | **QUST/INFO/DLC の scriptType 分布** | SCPT の {0:2031,1:265,256:97} 以外は未測定。`0/1/256` 以外が出れば EXE の 3 値マッピングが不足 | P4 S6 |
| N8 | **慣例差の docs/GT への未反映** | §3-2 の解明（63,235 ↔ 65,628）は本成果物のみ。`RE_EXE_REPORT` / `plan.md` / b21 §0.2 の「65,628」引用との突合記述は**本タスクの編集禁止により未反映**（F1 反射バッチへ送る） | F1（別タスク） |
| N9 | **`0x1000` 帯の tail 例外の一般性** | walk の「tail は必ず 4 B の `0x0011`」は SCPT で検証済み。QUST/INFO/DLC でも tail が常に 4 B かは P3 の walk 完走で间接確認するのみ（明示的検証式は未設計） | P3 T3 |

---

## §8 付記: 1 行まとめ

> **「解禁時に打つ 4 発はすべて既存成果物で判定式と受け入れ値が揃っている。残るのは ESM/DLC を 1 本の walker で 1 回走らせ、CTDA の `i` ヒストグラム・TrapUpdate 22 行・owner 別頻度（慣例 2 列）・SCHR 不変量の 4 出力を吐き、既知 GT（141 表 / 112 台帳 / SCHR 2,393 件系 / CTDA 58,487）と mismatch 0 を確認することだけ」** — 工数 2.5–3 人日、走査は ESM 約 300 s + DLC 数秒。

---

## §9 追記（2026-10-28 / V154 再試行）

> 前回セッションの §0–§8（352 行）を維持し、本節のみ追記する（上書きしない）。
> 追記の動機: スコープ記載のうち (i)「CTDA の s\*Description / Op 相当確認（v60/v93 再利用手順の具体化）」、(ii)「解禁後の正確なコマンド/スクリプト仕様」、(iii) 契約 §6 の必須 section（結論 3 行・Android 移植への示唆）が未記載だったため。

### 9.0 スコープ文言の分解（曖昧さの裁定）

| スコープ記載 | 本節での担当 | 裁定 |
|---|---|---|
| CTDA の「Op 相当」確認 | §9.1 | CTDA payload の **byte0 = flags(低5) \| op<<5(高3)** の実測仕様。EXE 側で確定済みのレイアウトに突き合わせる |
| `s*Description` 置換の v60/v93 再利用手順 | §9.2 | F2(d)（= TODO #142 / `v142`）の残り。既存 GMST キャッシュで **7/8 が既確認**、1 件のみ解禁時確認 |
| TrapUpdate 22 件の所在特定手順 | 既存 §2 ＋ §9.4 | 既存節で確定済み。追記は実行コマンド形のみ |
| SCDA 頻度 368 の再計測手順 | 既存 §3 ＋ §9.3 | 既存節で確定済み。追記は**解禁後の正確なコマンド仕様** |
| SCHR 供給源（EXE 側確定）の ESM 突合 | 既存 §4 | 追加記述なし（突合仕様は §4.2 で確定済み） |

### 9.1 仕様①補: CTDA の Op（byte0）相当確認

#### 9.1.1 EXE 側で確定した byte0 レイアウト（本回裏取り・証拠）

- **評価側**: `FUN_0056AC50`（条件評価本体、`docs/Oblivion_decompiled.c:270510`）は `:270547` で `FUN_0056a3b0(*param_1 >> 5,(float)local_8,uVar1)` を呼ぶ。逆アセンブル実測でも `0x56ACFC movzx eax,cl` → `0x56AD03 shr eax,5` → `0x56AD16 call 0x56A3B0`（`call 0x56A3B0` は `.text` 全域で **1 件のみ** = この経路が唯一）。
- **比較ヘルパ**: `FUN_0056A3B0`（`:269929`）は `switch(op)` の **case 0..5 の 6 分岐**。どの case にも該当しないと default なしで `return 1`（**常に真**）→ **op 6/7 は未定義動作**。各 case の不成立時 `return 0` から逆算した意味: 0=`==` / 1=`!=` / 2=`>` / 3=`<` / 4=`>=` / 5=`<=`。
- **生成側**: `FUN_0056A9C0`（`:270357`）は byte0 を `bVar1 & 0x1f | 0x80`（0x1001: op=4）/ `bVar1 & 0x1f | 0x20`（0x106B: op=1）/ `*param_1 & 0x1f`（op=0 系）と書き換える → **byte0 = flags[4:0] | op[2:0]<<5**。同関数の `param_2 - 0x1000 < 0x171` / `0x170 < param_2` の正規化は本書 §1.2 の判定式と同一（本回 6/6 パターン実在確認）。
- **flags の実消費**: `:270541-270546`（asm `0x56ACE9 fld dword ptr [eax+0x24]` / `0x56ACEE fld dword ptr [esi+4]`）で **bit2(0x4)=1 かつ +4≠0 なら +4 をオブジェクトポインタとして辿り `+0x24` を比較値にする**。bit2=0 なら +4 を float のまま比較値として使う。
- **ロード側（無変換の根拠）**: CTDA は `FUN_00450C20(record,dest,0x18)`（`:61751`、`.text` call **239 件**）で 0x18 生読みされ、byte0 の正規化コードは生成側 `FUN_0056A9C0` にしかない → **ファイル byte0 == 実行時 byte0** が成立。
- **既存キャッシュの非カバー**: 既存の CTDA キャッシュは件数のみ（§1.1）、SCDA 表（141 行）は SCDA 専用 → **byte0 を持つ既存成果物は 0 件**。byte0 分布は解禁時計測が必須。

#### 9.1.2 計測仕様（§1.2 の walker を 1 本で兼用）

出力 `v164_ctda_op_hist.tsv`:

```
owner_tag  op_raw_hex  op  flags  count
op = op_raw >> 5   /   flags = op_raw & 0x1F
```

| # | 検証 | 受け入れ値 / 逸脱時の意味 |
|---|---|---|
| O1 | op ∈ {0..5} | 全件。**6/7 が出れば** evaluator が常に真を返す未定義ケース（原作でも実質「常に真」扱い） |
| O2 | `op_raw >> 5 == op` かつ `op_raw & 0x1F == flags` | 恒等式（スキーマ誤りの検出用） |
| O3 | flags 分布 | 未知 → **新規計測値として確定**（N11） |
| O4 | bit2（0x4） | **想定は 0 のみ**。1 が出た場合、+4 の float ビットがポインタとして辿られる → 読込後 fixup の有無を別途調査（b20 前提の見直し案件） |
| O5 | 0x1035 / 0x104F 出現行（§1.2 C4）との join | 同一行の op/flags を `v164_ctda_targets.tsv` に追加列で保持 |
| O6 | GetDisposition 特例との整合 | `:270551` `+8 == 0x4C` かつ `:270554` `op ∈ {2,3}` で `*param_4 = 1`。X=0x4C の出現行で op が 2/3 以外なら特例非発火（記録のみ） |

> **留意**: payload[+4] は「比較値」で、§1.2 の `i`（+8 u16 = 関数 X）とは別物。idx/opid の罠と同型に混同しないこと。

#### 9.1.3 `s*Description` 相当（= F2(d)）は §9.2 に分離

スコープ文言「s\*Description / Op 相当」は 2 件の統記と解釈した。`s*Description` は GMST キーであって CTDA フィールドではないため、§9.2 で扱う。

### 9.2 仕様①補: `s*Description` 8 件の v60/v93 再利用手順

#### 9.2.1 既存キャッシュで確定した 7/8（本回計測）

`s50_gmst_esm2.txt`（既存 ESM キャッシュ）を本回パース:

- **382 行 / 382 unique / 先頭文字 f=243, s=108, i=31** → `v60_esm_only_keys.md` §5（`GRUP @ file 0x2FC` / size `0x6B43` / 終端 `0x6E3F` / **382 = f243/s108/i31**）と**完全一致（3/3）**。→ esm2 のキー集合は **Oblivion.esm の GMST キー集合そのもの**と判断してよい。
- 8 件のうち **7 件が実在**（行番号併記）: `sStrengthDescription` L228 / `sWillPowerDescription` L229 / `sPersonalityDescription` L230 / `sLuckDescription` L231 / `sIntellegenceDescription` L232 / `sEnduranceDescription` L233（値 `MESSAGE`）/ `sAgilityDescription` L246。残り 6 件の値は `YOU ARE USING WRONG GAMESETTING`。
- **`sSpeedDescription` のみ esm2 に不在**（キー集合に `sSpeed` 始まり 0 件、部分文字列 `SpeedDesc` の esm2 内ヒット 0）。他キャッシュの `SpeedDesc` ヒット 11 件はすべて **EXE 側**（`s50_gmst_rows*.json` の registrar 行 = `value: "Need a gamesetting description."`、`s50_gmst_full.txt:1360` 等）で、**ESM 側の値を持つキャッシュは 0 件**。
- → `v142_s7010_residuals.md` §1-3 の表は「8 件とも ESM 実在（値 junk）」としているが、**出典（esm2）で裏付けられるのは 7 件のみ**。残る 1 件は訂正候補（N10）。

#### 9.2.2 解禁時の手順（再スキャン範囲を GMST GRUP ≒ 27.5 KB に限定）

1. `python "$env:TEMP\opencode\probe_harness.py" selftest` → 4/4 PASS。
2. `Oblivion.esm` の先頭 **`0x2FC .. 0x6E3F`（GRUP GMST, size 0x6B43）** のみを読む（§5 の P0 全体 walk の先頭に同置できる。単独なら数 ms）。
3. EDID 一覧（382）を `s50_gmst_esm2.txt` と join → **mismatch 0** を確認（= 既存キャッシュの妥当性ゲート D1、v60 の GRUP 実測の再現）。
4. 8 キーの有無と DATA 値を v142 §1-3 の表と突合（D2）。特に **`sSpeedDescription` の在不在確定**（D3: 在 → esm2 が欠落＝キャッシュ側の欠落 / 不在 → vanilla が欠落＝v142 の 1 行訂正）。
5. 出力 `v164_sdesc_gmst.tsv`: `key  in_esm  esm_value  exe_default  replaced  delta`。
6. 本手順は §5 の P1 と**重複しない**（P1 は CTDA/SCDA/SCHR 軸、GMST 軸は本手順のみ）。

### 9.3 解禁時の正確なコマンド仕様

PowerShell 7 の禁止事項（heredoc / 複数行 `python -c` / `head`）を踏まえた実行形。**スクリプトは必ず先にファイルへ書いてから実行する**。

```powershell
# 0) 前提確認（契約 §3-3）
python "$env:TEMP\opencode\probe_harness.py" selftest

# 1) walker 本体（新規・1 本で 5 出力: CTDA i/Op / SCDA freq / Trap sites / SCHR / GMST-8）
#    ファイルは %TEMP%\opencode\v164_pass1.py（中間物は v164_* 接頭辞で継続）
python "$env:TEMP\opencode\v164_pass1.py" `
  --esm    "C:\Program Files (x86)\Steam\steamapps\common\Oblivion\Data\Oblivion.esm" `
  --dlc    "C:\Program Files (x86)\Steam\steamapps\common\Oblivion\Data" `
  --outdir "$env:TEMP\opencode\v164_out" `
  --tails both --owner-cols

# 2) 台帳 join（§67.6 480 行 / OpID キーのみ。名称キーは禁止 = §3-3-4）
#    %TEMP%\opencode\v164_join.py を先に書き、次を実行
python "$env:TEMP\opencode\v164_join.py" `
  --ledger "C:\Users\hiroki.kogarumai\Oblivion_Android\docs\RE_EXE_REPORT.md" `
  --freq   "$env:TEMP\opencode\v164_out\v164_scda_freq.tsv"

# 3) ゲート判定（P0: distinct 141 / 112 正頻度 mismatch 0 / SCHR 既知 GT mismatch 0）
python "$env:TEMP\opencode\v164_gate.py" --outdir "$env:TEMP\opencode\v164_out"

# 4) 完了後にもう一度
python "$env:TEMP\opencode\probe_harness.py" selftest
```

- **ESM パスの注意**: `probe_harness.py:16` の既定 `ESM` 定数は Steam インストール版（本回メタデータ実測で **277,504,985 B 実在**）だが、同一サイズのコピーが `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Data` と `D:\Cargo\Other\Oblivion\Oblivion_Steam版\Data` の **2 箇所**にも有る。pass1 は**必ず `--esm` 明示**（暗黙既定への依存を禁止）。3 実体は同サイズ（SHA 未照合 = N13）。
- **DLC**: 上記 3 ディレクトリのいずれも `*.esp` **10 件**（内 `DLCShiveringIsles.esp` = **85 B スタブ = 走査対象外**）で、§0.3 のサイズ一覧と一致（本回 **3/3** 再確認、内容未読）。
- **出力先**: 解禁タスクの成果物 1 ファイル制約を守るため、中間 TSV/JSON は `%TEMP%\opencode\v164_out\` に置き、`tmp/re/` へは要約のみ追記する（§5 P0–P5 の受け入れ値がゲート）。
- **頻度 368 の再計測**は上記 1) の `--owner-cols` と `--tails both`（`tail_excl` / `incl` の両列）で §3.4 のスキーマを満たす。台帳への反映は F1 バッチ（本タスクの権限外 = §7-N1/N8）。

### 9.4 TrapUpdate 22 件: 追記はコマンド形のみ

- 手順本体は既存 §2（walk 規則・キー・T1–T6）で確定済み。**`scda_cache` 欠損の代替は「9,646 ブロック + DLC の再 walk 1 回」で、それが §9.3 pass1 の副産物**（§5 P2）。
- SCTX 側の `TrapUpdate` キーワード照合（T4）は同一 pass で SCTX サブレコードを保持して行い**差分のみ報告**（N6）。追加走査は不要。

### 9.5 結論（3行）

1. 解禁時に打つ 4 発（CTDA 生バイト / TrapUpdate 22 件 / 頻度 368 再計測 / SCHR 突合）はいずれも判定式・受け入れ値・出力スキーマが揃っており、**1 本の walker（約 300 s）+ DLC 1 回**で完結する。
2. 本追記で **CTDA の byte0（flags | op<<5）** という未カバーの計測軸を追加し、EXE 側（`0x56ACFC movzx eax,cl` → `shr eax,5` → `call 0x56A3B0`、生成側 `FUN_0056A9C0` のマスク）でレイアウトを確定した。既存キャッシュに byte0 を持つ成果物は 0 件。
3. `s*Description` 8 件は**既存キャッシュで 7/8 が確定済み**（esm2 の 382 キー = v60 の GRUP 実測と一致）。残る `sSpeedDescription` は GMST GRUP 27.5 KB の単独読みで確定でき、v142 §1-3 の 1 行が訂正対象になる可能性を検出した。

### 9.6 Android 移植への示唆

移植側は「条件の比較演算子」を **byte0 の高 3 ビット（op<<5）** として読み、低 5 ビットをフラグとして保持すること（生成 API は `FUN_0056A9C0` のマスク形をそのまま再現し、ファイル I/O は 0x18 verbatim で通す）。op ∈ {0..5} は evaluator（`FUN_0056A3B0` の 6 分岐）に 1:1 で対応し、6/7 は「常に真」として扱っても原作と差が出ない。flags bit2 が立った CTDA が ESM 実在する場合は、+4 を float ではなくオブジェクトポインタ（`+0x24` 参照）として解決する経路を移植側に用意する必要がある（O4 の計測結果次第）。GMST 軸では `s*Description` のうち **ESM に在るキーだけ値を上書きし、無いキー（候補: `sSpeedDescription`）は EXE 既定値を保持する**再現を行う。

### 9.7 本追記の自己検証（内部整合）

| # | 検査 | 結果 |
|---|---|---|
| 1 | `probe_harness.py selftest` | **4/4 PASS**（着手時・追記時の 2 回） |
| 2 | capstone 全文走査（`.text` raw `0x400` / size `0x626E00`） | 命令数 **2,258,215**（GT §4 と一致） |
| 3 | `call` 先カウント vs `cache/call_targets.json` | `0x56A3B0`=1 / `0x56A9C0`=1 / `0x450C20`=239 / `0x56AC50`=3 → **4/4 一致** |
| 4 | decompiled アンカー行番号（Select-String 実測） | 61751 / 269929 / 270357 / 270510 / 270547 / 270554 → **6/6** |
| 5 | `FUN_0056A9C0` 本体の正規化パターン | `&0x1f\|0x80` / `&0x1f\|0x20` / `&0x1f` / `*(short*)(p+8)=X` / `param_2-0x1000<0x171` / `0x170<param_2` → **6/6 実在** |
| 6 | `s50_gmst_esm2.txt` | 382 行 / 382 unique / f243+s108+i31 = **v60 §5 と 3/3 一致** |
| 7 | 8 キーの esm2 実在 | **7/8**（不在 = `sSpeedDescription`、esm2 内 `SpeedDesc` ヒット 0） |
| 8 | ESM/ESP 実体のメタデータ | ESM 3 パスとも 277,504,985 B、DLC `*.esp` 10 件 × 3 ディレクトリ = **3/3 一致**（内容未読 = 制約遵守） |

中間スクリプト: `%TEMP%\opencode\v154_check.py` / `v154_probe.py`（ESM/DLC 本体への読み取りは一切なし）。
