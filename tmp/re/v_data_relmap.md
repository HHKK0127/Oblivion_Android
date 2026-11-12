# v_data_relmap.md — RE 成果物データ関係性マップ（棚卸し）

- 作成日: 2026-10-26 / 作成者: データ関係性マップ担当サブエージェント
- 編集範囲: **本新規 1 ファイルのみ**。docs/ / GT_CHEATSHEET.md / TODO_re49.md / 台帳 / ledger / plan は一切変更していない（読取のみ）。
- 対象: `tmp/re/` 配下の最新完了分 10 件 + 前期分 11 件 + 台帳 2 件 + `docs/RE_EXE_REPORT.md`（見出しのみ抽出）。
- 方法: 各成果物の見出し構造・手法/入力節・残留不明点節を全文抽出、成果物間の相互参照を住所/識子子で機械計数、共有資産の参照マトリクスを機械計数。

---

## 0. 棚卸し範囲

### 0.1 成果物 ↔ V-ID ↔ 台帳行（対応表）

| 成果物 | V-ID | 台帳行 (v_open_rows) | 状態 |
|---|---|---|---|
| `b20_impl_5071d0.md` | V116 | 114 | 完了（残点は pool V163 等へ起票済） |
| `b21_speculative_backing.md` | V117 | 115 | 完了（pool 記載済） |
| `b25_cc_residuals.md` | V122 | 120 | 完了（残留 R1–R5 = pool V161） |
| `b26_nontt_class_names.md` | V123 | 121 | 完了（残り R-1〜R-6） |
| `b27_slot_residuals.md` | V124 | 122 | 完了（V-27a〜h） |
| `b28_push_caller_residuals.md` | V125 | 123 | 完了（残点 9 項） |
| `v121_gt_corrections.md` | V121 | 119 | 完了（**編集は承認待ち**） |
| `v126_bind_crt_bootstrap.md` | V126 | 126 | 完了 |
| `v127_labelless_cat3.md` | V127 | 127 | 完了 |
| `v137_v80_global_residuals.md` | V137 | 137 | 完了 |
| `v90_b350d8_triangle.md` | V90 | （前期完了） | 完了（残点8 = V126/V127/V128 へ分配済） |
| `v91_ini_defaults.md` | V91 | — | 完了（§8 残点7 = V130） |
| `v92_shader_renderer_bundle.md` | V92 | — | 完了（§a-5/b-7/c-5 = V129） |
| `v93_gmst_gt_residuals.md` | V93 | — | 完了（残点5 のうち 1/2 は V126 へ） |
| `v94_boundary_dups.md` | V94 | — | 完了（§e R-a〜R-g = V131/V133 等） |
| `v96_gap_instances_crosscheck.md` | V96 | — | 完了（残点 → V127/V136） |
| `v97_forwarder_family_map.md` | V97 | — | 完了（§6 = V132/V134） |
| `v80_partB_globals.md` | V80 | — | 完了（§5/§6 → V90/V91/V92/V137） |
| `b14_slot_method_map.md` | V110 | — | 完了（§5 残点 → V121/V124） |
| `b16_alarm_masks.md` | V112 | — | 完了（§4 残点 → V138） |
| `b19_quest_byte_writer.md` | V115 | — | 完了（§5 残点 → V141） |

- 未着手/成果物未生成: V118–V120（b22–b24）、V128–V136、V138–V142（`glob` で **0 件確認**）。
- 台帳: `%TEMP%\opencode\v_open_rows.txt`（28 行 = 行31 + 行114–142 の 27 行）、`%TEMP%\opencode\v143_residual_pool.md`（残点統合プール、A群=GT/docs 訂正 6 件 / B群=分析継続 16 件）。

### 0.2 `docs/RE_EXE_REPORT.md` の節構造（`^## ` のみ抽出）

- 1,029,892 B / 12,318 行（v121 §2.2 実測）。
- §1–§49: 基盤・メニュー・ESM/BSA・戦闘/魔法等の個別解析（Copilot 主導期）。
- §50–§66: opencode/Copilot 統合波（Wave 4a–8、魔法パイプライン）。
- **§67**: Script VM MISS ハンドラ台帳 480 件（§67.6 台帳 / §67.7 残課題 / §67.9 UNDET15）。
- **§68**: FunctionID 追加仕様 7 件。
- **§69**: リスト/イベント・コンテナ構造（§59.3 事件ビット表の後継）。
- **§70**: opencode 解析ノート v60–v76 の統合。**§70.10（CRT/起動: V126 と領域重複）、§70.15(6)（v80 残不明点: V137 が解明）、§70.16（ラベルなし関数 UNLABELED: V86/V96/V127 の着地点、(4c) 追記案は V136 で承認待ち）**。
- → v/b 系成果物の docs 着地点は §67–§70 に集中。**§70.16 と §67.7 が未解消残課題の主要バッファ**。

---

## 1. 共有データ・GTマップ

### 1.1 共有資産カタログ

| # | 資産 | 実体 | 規模/定数 | 管理 GT |
|---|---|---|---|---|
| A1 | **オペコード表 0xB0AF48** | `Oblivion_decompiled.c`/PE .data 内の静的表（`cache/opcode_table.json` に一次ダンプ） | 基底 `0xB0AF48` / stride `0x28` / **533 行**（idx532=フェンス）/ 全 11 列確定（V78/V83/b17） | GT_CHEATSHEET §4 |
| A2 | **GT_CHEATSHEET.md** | `tmp/re/GT_CHEATSHEET.md` | 11,458 B / 152 行 / **LF** / §1 セクション定数・§2 台帳行数+ラベル欠落・§4 クラス/vtable・§5 訂正履歴・§7 handover | 自身＝GT（**訂正適用待ち 11 件**、§2 は §3 未反映） |
| A3 | **func_list_new.txt**（= `cache/named_funcs.txt`） | Ghidra FunctionManager 台帳 | 21,597 行 `00401000,FUN_00401000` | GT_CHEATSHEET §1 |
| A4 | **dec_index** | `docs/Oblivion_decompiled.c` の `// FUN_* @ 0x` ヘッダ索引（probe_harness Ctx） | **20,465 件**（+ 名前付きヘッダは非収録） | GT_CHEATSHEET §1（件数のみ） |
| A5 | **call_targets.json** | `.text` 全域 E8 呼出し先カウント | 13,946 ターゲット / 137,421 呼出し | `cache/README.md` |
| A6 | **vtable_index.tsv** | RTTI vtable 索引 | 1,715 行（primary 1,481 / secondary 234 / slots 29,063） | `cache/README.md` |
| A7 | **b13 系 JSON** | `b13_secondary.json`（mdisp 階層 1,715 COL）/ `b13_final.json` / `b13_blocks.json` / `b13_evidence.json` / `b13_names2.json` | b13 節で生成 | b13 本体（`b13_nonrtti_vtable_classes.md`） |
| A8 | **probe_harness.py** | `%TEMP%\opencode\` の読取ハーネス（selftest 4/4 = 会話開始の必須儀式） | rd()/cstr()/dec_index/enclosing_fun | GT_CHEATSHEET（前提） |
| A9 | **gap/台帳系中間台帳** | `v86_unlabeled_scan.txt`(10,175) / `v87_region_class.txt` / `v92_e8_214_triage.txt`(214) / `v96_e8_candidates.txt`(14,186/台帳外565) / `v79_classified.csv` / `v79_bodies.json` / `v70_force_create.csv` | Copilot/opencode 共用（読取専用） | RE_EXE_REPORT §70.16 |
| A10 | **docs/RE_EXE_REPORT.md** | 正本書（CRLF・.bak15 あり） | 1,029,892 B | 親セッション（要承認） |
| A11 | **GT スロット語彙（分散）** | GT_CHEATSHEET §4 / RE_EXE_REPORT §17.2・§41.3・§20.2 / `handover_20261025/session_B_summary.md` L43–49 / b14 §1 / b27 §4 | **5 箇所に分散・互いに不一致あり** | 未統合（V121 §5 V149 が「GT §4.1 常設」提案） |
| A12 | **ESM 側成果物** | `v60_esm_only_keys.md` / `re_leveledlist_system.txt` / `re_crime_system.txt` / `s50_gmst.md` 等 | ESM 再スキャン禁止下の既存引用元 | なし（統合 GT 未形成） |

### 1.2 資産 × 成果物 参照マトリクス（機械計数、値 = 参照出現数）

| 成果物 | A1 opcode表/-json | A2 GT | A3 func_list | A4 dec_index | A5 call_targets | A6 vtable | A7 b13 JSON | A8 harness | A10 report |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| b20 (V116) | 2 | 5 | 3 | — | 2 | — | — | 4 | 9 |
| b21 (V117) | 7 | — | — | — | — | 2 | — | 3 | 3 |
| b25 (V122) | 2 | 1 | — | — | 2 | 2 | — | 3 | 1 |
| b26 (V123) | — | — | — | — | — | 2 | **10** | 2 | — |
| b27 (V124) | — | — | — | — | **7** | 1 | — | — | — |
| b28 (V125) | — | 1 | 2 | — | 1 | 2 | — | 2 | — |
| v121 (V121) | — | **20** | — | — | — | — | — | 2 | **24** |
| v126 (V126) | — | — | — | — | — | — | — | 1 | 3 |
| v127 (V127) | — | — | 3 | **7** | — | — | — | 5 | 1 |
| v137 (V137) | — | — | 2 | **7** | — | — | — | 1 | 3 |
| v90 (V90) | — | — | — | 7 | — | — | — | 2 | — |
| v91 (V91) | — | — | — | 1 | — | — | — | 3 | 1 |
| v92 (V92) | 1 | — | 2 | 3 | — | — | — | 3 | 3 |
| v93 (V93) | — | 1 | 1 | — | — | — | — | 2 | — |
| v94 (V94) | 1 | — | 2 | 2 | — | — | — | 3 | — |
| v96 (V96) | — | — | 4 | **18** | — | — | — | 4 | 3 |
| v97 (V97) | 1 | 2 | 1 | — | — | — | — | 3 | — |
| v80 (V80) | — | — | — | 6 | — | — | — | 2 | 1 |
| b14 (V110) | — | 4 | 1 | — | 4 | **6** | — | 2 | 2 |
| b16 (V112) | 2 | — | — | — | 2 | — | — | 2 | 1 |
| b19 (V115) | 5 | 1 | — | — | 3 | 1 | — | 2 | — |

- **全成果物共通の足場**: `probe_harness` selftest 4/4（**20/21**、b27 のみ明示記載なし）、`docs/Oblivion_decompiled.c`（16/21）。
- **最重度共有資産**: A1 オペコード表（**8 成果物**が直接読取: b20/b21/b25/b16/b19/v92/v94/v97）、A4 dec_index（v 系ラベル分析 8 成果物）、A5 call_targets（b 系の裏取り基盤 7 成果物）。
- **単独依存資産**: A7 b13 系 JSON は b26 専用（b13→b26 の一気通貫）、A6 vtable_index は b14/b26 系が主消費者。

### 1.3 「ラベル/台帳」レイヤ（最重要の共有データ断面）

住所ベースで 6 層が並走しており、**どの成果物も全層の突合はしていない**:

| 層 | 台帳 | 件数 | 収録条件 | 主な利用者 |
|---|---|---:|---|---|
| L1 | `dec_index`（decompiled ヘッダ） | 20,465 | `// FUN_* @ 0x` のみ（名前付きヘッダ 767 を**無視**） | v80/v90/v96/v127/v137 |
| L2 | `func_list_new.txt`（Ghidra 台帳） | 21,597 | FUN 名義 20,830 + 非 FUN 名義 767 | 全体 |
| L3 | V86 UNLABELED（ret+int3 規則） | 10,175 (A214/B9,809/C152) | terminator+CC ラン | v96, v127 |
| L4 | **Cat-3**（デコンパイル無領域、V127 新設） | **10,261 / 2,272 領域** | 全ヘッダ 21,232 使用・誤検出 0 件設計 | v127（みずから） |
| L5 | Cat-3b（台帳登録済・ヘッダ無し） | 365（339 が規則通過） | L2 ∩ ¬L1 | v127 → pool V152 |
| L6 | 個別発見分 | V96 10 / V92 214 / V90 5 / V137 1,132（L2−L1 の構造差） | 各タスク散在 | 未統合 |

- **L1 と L2 の構造差 1,132 関数**が「dec_index 欠落の罠」（v137 §2 で実害実証 = v80 の誤帰属）、**L3–L6 は互いに全突合未実施**。
- 台帳追記提案（v96 (d)-1/-2、func_list 相当の追記行）は **未適用**（V136・承認待ち）。

---

## 2. 成果物間の依存・矛盾

### 2.1 依存チェーン（前身 → 継承・訂正）

```
b01–b12 (b スコープ群)
 ├─ b05 ────────────→ b20 (§9 突合、GT §2 vs §67.9 の裁定)
 ├─ b09/b12 ────────→ b21 (D類35件を3軸で再判定 → 「裏取り不可」を部分訂正)
 ├─ b04 ────────────→ b28 (「45件」不一致を裁定、b15 の arg 判定を訂正)
 ├─ b15 ────────────→ b28 (no-local-def 判定 3 件を 1 誤り/2 補正)
 ├─ b18 ────────────→ b25 (CC 残留 5 点を深掘り)
 ├─ b13 ────────────→ b26 (b13_secondary.json 等を引き、§6-3 の 2 仮説を誤りと裁定)
 ├─ b14 ──┬─────────→ b27 (残留 slot 5 点を実測、b14 §1.2/§5-6 の誤りを検出)
 │        ├────────→ v121 (§1.3/§4/§5 の GT 不整合を V121 訂正 5 件へ)
 │        └─(b21 が 8 箇所引用、b16/b25 が slot 名を引用)
 └─ b11 ────────────→ b21, b16

v80 ──┬→ v90 (B350D8 三角、dec_index 偏り 684 を発見)
      ├→ v91, v92 (§6 未解決案を各タスク化)
      └→ v137 (§5 残不明点 4 件を解明、うち 1 件は v80 の前提を否定)

v90 ──┬→ v126 (§5-1/5-2 = .bind→CRT 接続)
      ├→ v127 (§5-3 = ラベルなし第3カテゴリ)
      └→ v128 (§5-4〜8、未生成)

v93 ──┬→ v126 (未解決残点 1/2 + §c CRT テーブル)
      └ (v81 の .rdata 定数誤りを裁定)
v96 ──┬→ v127 (残点2 = gap 内ブロック独立性を 10/10 判定)
      └→ V136 (台帳追記提案、未適用)
v94/v97 ─→ v79/v70 系中間台帳 (v79_classified.csv 等) を共有
v121 ────→ b27 §4 が整合確認（+0x2D0 実測一致 / タスク要約に 1 誤り）
v127 ────→ v90 の「684」を 767 に訂正実測（+83 乖離）
```

### 2.2 検証・訂正関係（A の主張を B が検証/訂正）

| # | 主張元 | 検証/訂正元 | 結果 |
|---|---|---|---|
| C1 | v80 §5-3「svs→FUN_00441850 経路」 | **v137 §2** | 経路不存在 = **v80 の前提誤り**（dec_index 欠落 FUN_005101E0 への帰属バイアス）。docs §70.15(6) 側も未訂正 |
| C2 | v90 §1-2「名前付きヘッダ 684 / 全 21,149」 | **v127 (0)** | 実測 **767 / 21,232**（+83 乖離、台帳 non-FUN 767 と VA 完全一致）。docs 訂正要否は裁定待ち |
| C3 | b14 §1.3/§4/§5 の GT 名 4+1 件 | **v121** | GT 訂正 5 件に確定（+8/0xA0/0x2D0/0x36C/0x190）、**GT/report へ未反映** |
| C4 | v121 の GT 訂正文書 | **b27 §4** | V121 §1.3 の実測値は**全件一致 = 訂正不要**。ただしタスク要約「Actor 版 0x5E4440 経由」が字義通り不成立（兄弟実装） |
| C5 | b09/b12「D類35件は裏取り不可・推定174件」 | **b21 §3** | 25 確定 / 9 準確定 / 1 維持 → 帯内推定 168→112、**b09/b12 台帳は未更新（V156）** |
| C6 | b04「コンテナ入手経路 45 件未特定」 | **b28 §5** | **再現できる計測なし**（33 件が実測）→ b04 記載側が誤り |
| C7 | b15 の no-local-def 3 件 / arg17 | **b28 §2–3** | 1 件は誤り（実体=ローカル）、2 件は引数番号訂正 → b15 未編集 |
| C8 | b13 §6-3「ICF 統合で 4 件不一致」「suffix 共有」 | **b26 §3.4/§4.1** | いずれも**検証側の罠/誤り**（base phase 誤認・float 隣接）→ `b13_final_table.md` 訂正 4 箇所が未実施 |
| C9 | b14 §1.2「+0x154 も 0x4B3750」 | **b27 §1.3** | 登録は `+0xEC` のみ 30 件、**+0x154 行は誤り**（29/30 が oor） |
| C10 | b14 §5-6「静的キャスト=1 の分布」 | **b27 §5** | 1 = **0 件**（capstone 10 進表示 bug）、0 = 1,498 / 復元不能 320 → b14 表現が誤り |
| C11 | RE_EXE_REPORT §67.9 vs GT_CHEATSHEET §2（0x5072B0） | **b20 §9** | §67.9 は正しく ghidra_missing 掲載済、**GT §2 のみ欠落** → GT 追記提案 |
| C12 | RE_EXE_REPORT §35.5「0x981FB4 = 初期化完了通知」 | **v126 §10-4** | 実体は **atexit ラッパ** → docs 訂正 + §70.10 一覧から 3 項目除去、**未反映** |
| C13 | v81「GMST 登録 377 件」「.rdata 定数 0x4009D0」「0x7B14BE」 | **v93 §a / v96 残1** | 378 が正（377 は VA 誤り起因の過小計数）、定数誤りは確定・**原因未特定**、0x7B14BE は実測 0x7B14C0（2B 差） |
| C14 | handover `session_B_summary.md` L43/44/49（slot GT） | **v121 §4** | L43/L44 誤り候補・L49 は既に GT §5 で誤り判定済 → **GT §7 に未注記** |
| C15 | V5 表の見出し「opid」/ CC 前置列 `cc×8` | **v94 §e R-f/R-g** | 第1列は実体=表 idx、`cc×8` は飽和表記（実数保持せず）→ V5 ファイル未訂正 |
| C16 | cache `call_targets.json` = 1,819 | **b14/b27 線形実測 = 1,818** | **1 サイト差・要因未特定**（decode 境界か非整列か） |

### 2.3 未解消矛盾リスト（**18 件**）

**(a) 真に未解消（要因未特定・裁定待ち）= 4 件**

| # | 対立 | 内容 | 出典 |
|---|---|---|---|
| U1 | 名前付きヘッダ **684 vs 767** | v90 の 684 は再現せず（+83）。docs の 684 表記の訂正要否が未裁定 | v127 R1 |
| U2 | `call 0x9832E6` **1,819 vs 1,818** | キャッシュと線形実測の 1 サイト差、要因未特定 | b27 §5, V-27g |
| U3 | `0xa9d798` slot0/1 = 描画系仮想関数 | 「slot0/1 は dtor」vftable 想定と矛盾、ctor 不在と合わせ未解 | b26 R-2 |
| U4 | v81 の `.rdata` 定数 0x4009D0 誤導 | 結果（誤 VA）は確定・**原因**（ハーネス由来か手動か）未特定 | v93 残4 |

**(b) 誤り確定済みだが**編集禁止で未反映**（= 成果物→GT/docs/台帳の断層の中身）= 14 件**

| # | 誤りが確定した主張 | 反映先（未適用） | 検出者 |
|---|---|---|---|
| R1 | GT slot 5語（+8/0xA0/0x2D0/0x36C/0x190） | GT_CHEATSHEET §4.1 + RE_EXE_REPORT §17.2/§41.3 | v121 §1–3 |
| R2 | GT 新規不整合 A–F（`+0x170`=GetBaseForm 等、report L7503 vs L1669/L1071 の内部矛盾） | RE_EXE_REPORT（裁定待ち） | v121 §4 |
| R3 | v80 の svs 経路（帰属バイアス） | RE_EXE_REPORT §70.15(6) | v137 §2 |
| R4 | §35.5 `0x981FB4` = atexit、§70.10 の 3 項目 | RE_EXE_REPORT §35.5/§70.10 | v126 §10-4 |
| R5 | b14 §1.2「+0x154 → 0x4B3750」 | b14_slot_method_map.md §1.2 | b27 V-27b |
| R6 | b14 §5-6「静的キャスト=1」 | b14 §5-6 + castType 列 | b27 V-27g |
| R7 | b13 §6-3 の 2 仮説（suffix 共有/ICF 不一致） | `b13_final_table.md` 行1/74/76 + b13 §6-3 | b26 R-4 |
| R8 | b12 §4「D類35件は裏取り不可」 | b12/b09 台帳（確度帯の更新 = pool V156） | b21 §3 |
| R9 | b09「推定 174 件」→ 残 127 | b09 台帳 | b21 §3 |
| R10 | b04「45 件未特定」 | b04 §8-2 | b28 §5 |
| R11 | b15 の arg17 / param 判定 3 件 | b15 §7 | b28 §2–3 |
| R12 | handover session_B L43/44/49 の slot 記載 | GT_CHEATSHEET §7 注記 | v121 §4 |
| R13 | GT §2 のラベル欠落に `0x5072B0` 不在 | GT_CHEATSHEET §2 | b20 §6-5 |
| R14 | v81 の 0x7B14BE（2B差）/ V5 表の見出し `opid` / `cc×8` 飽和 | v81, V5 の該当表記 | v96 残1, v94 R-f/R-g |

---

## 3. データ断層（未解析の交差領域）— **最重要**

### F1. 成果物 → GT_CHEATSHEET / RE_EXE_REPORT / 台帳 の反映断層（最大の横断断層）

- **範囲**: §2.3(b) の 14 件 + pool A群（V143–V146）。v121/v126/b14/b26/b27/b28/b21/b20 が「分析完了・編集なし」で終了し、GT/正本側が旧値のまま。
- **なぜ未解析か**: サブエージェントの編集禁止（docs/GT/TODO/ledger は承認制）＋ 承認手順（.bak 採番 → 編集 → round_verify ALL OK）が未実行。GT_CHEATSHEET は .bak 0 件のまま。
- **何を読めば埋まるか**: `v121_gt_corrections.md` §2（所在特定）＋ §3（適用手順チェックリスト）／ `v126` §10-4 ／ `b27` V-27b/g ／ `b26` R-4 ／ `b21` §3 帯表 ／ `b20` §6-5 — **すべて反映テキストが成果物内に用意済み**で、追加分析ゼロ。
- **推定工数**: 小〜中（承認が前提、編集+検証のみ）**0.5–1 人日**。

### F2. ESM 側成果物 ⇔ EXE 側成果物の接点

- **範囲**: (a) b20: CTDA `+8` X 形式の**生バイト一次証**（ESM 再スキャン禁止で未実施）、(b) b21: TrapUpdate `opid 0x110C` の ESM 22 回の**所在スクリプト**（`scda_cache.pkl` 欠損・DLC ESP の SCDA も未走査）、(c) b19 §5-3: ESM ロードされる Script の `+0x28` 供給源（SCHR 書込先 5 呼び出し）、(d) V142: `s*Description` 8 件の ESM 置換、(e) V160: TrapUpdate 名称の外部資料照合。
- **なぜ未解析か**: **ESM 再スキャン禁止**（本ラウンド）＋ 中間キャッシュ `scda_cache.pkl` 欠損。EXE 側成果物は ESM を「既存成果物からの引用」でしか参照できない。
- **何を読めば埋まるか**: 既存 ESM 成果物（`v60_esm_only_keys.md` / `re_*_system.txt` / `s50_gmst.md` / `copilot_ini_*.txt`）で代替可能な (d) を先行消化 → 禁令解禁時に (a)(b)(c) の 3 件へタスクを絞る（b21 §7-8, b20 §6-3 に検証仕様あり）。
- **推定工数**: 先行分 小（**0.5 人日**）／解禁後 大（**2–3 人日**、走査 1 本 + 判定）。

### F3. renderer 系（v92/v90/v80/v137）⇔ Cat-3 tail 帯域（v127）

- **範囲**: v92 の **renderer gap 50 / state gap 54**（tiny getter 0x769F00–0x769F80 クラスタ、台帳未登録）と v127 の Cat-3（**全 10,261 件の 63% = 6,475 件が 0x980000+ に集中**、`[0x9A9D80,0x9D8020)` 189KB/668、`[0xA001F0,0xA0A290)` 41KB/2,294）が**住所単位で一度も突合されていない**。加えて v137 の dec_index 欠落 1,132 関数の列挙（手順のみ提示）も未実施。
- **なぜ未解析か**: 時系列の断層 — v92（Wave2）完了時は Cat-3 概念が未定義、v127 完了時は v92 の gap 一覧を住所集合として取り込まなかった。tail 帯域の**正体（D3DX / CRT / 16B 揃えスロット）自体が未同定**（v127 R5）。
- **何を読めば埋まるか**: v127 の Cat-3 エントリ一覧（`%TEMP%\opencode\v127_*.py` 出力）× v92 の gap 一覧（§b 表）× v90 ラベルなし 5 件 × v137 欠落 2 件を**住所集合の突合スクリプト 1 本**で統合。tail 帯域の識別は `.rdata` の RTTI 名 1,529 件（`.?AV`、v137 §2-4 で判明）と v92 付録 A の vtable 表で判定。
- **推定工数**: 中（**0.5–1 人日**、追加走査なし・集合演算＋判定中心）。

### F4. CC/combats 系（b18/b25/b27）⇔ オペコード dispatcher（b05/b19/b20/v94/v97）

- **範囲**: (i) `+0x18` の**実行時到達経路は band1（opid ≤ 0x1E）のみ証明済**で、band2/band3 のアンカーは 0 件（v94 R-a → V131 未着手）、(ii) `+0x1C` 例外 dispatcher 4 件（V77 以来保留 → V132 未着手）、(iii) b20 の dispatcher **arg8≠0 経路は到達不能（デッドコード）・arg9 経路の呼び出し元未特定**、(iv) **CC のフィールド（`+0x1BC` / slot58 連鎖 / `+0x6C` 状態 0..0xF）へ書き込む script opcode の逆引きが未走査**（b25 は「CC 側の読手」のみ、opcode 側は b05/b21 の頻度表のみで接続点なし）。
- **なぜ未解析か**: b 系（slot/CC 軸）と opcode 表軸の成果物が**別タスクとして並行生成**され、両者の交点（opcode impl ↔ CC/process slot 書手）を担当したタスクが一度も無い。+0x18 アンカー走査（v94）が band2/band3 で 0 件だったため、到達経路の立証方法自体が途切れている。
- **何を読めば埋まるか**: `cache/opcode_table.json`（533 行 impl 列）× `cache/call_targets.json` × `cache/vtable_index.tsv`（CC vtable 0xA70A14）を突き合わせる双向走査（3 つの共有資産のみで足りる）。dispatcher 側は b20 §4/§6 の `0x5169A0` 呼び出し元 2 件（間接）の追跡を継続。
- **推定工数**: 中〜大（**1.5–2 人日**、V131/V132 とは別個タスクとして）。

### F5. クラス/slot 成果物（b13/b14/b26/b27）⇔ GT slot 表（分散した 5 箇所）

- **範囲**: slot 語彙が (1) GT_CHEATSHEET §4（1 行のみ）、(2) RE_EXE_REPORT §17.2/§41.3/§20.2、(3) handover session_B L43–49（誤記含む）、(4) b14 §1/§3（未同定 9 種）、(5) b27（V-27h 追記候補）に分散し、**統合表が存在しない**。b26 の未同定クラス 9 件（R-3）も同じ表に載る運命。
- **なぜ未解析か**: V121 は「GT §4.1 常設」を V149 として提案しただけで承認未了。b14/b26/b27 はいずれも GT 編集禁止。
- **何を読めば埋まるか**: v121 §3 手順（GT §4.1 新設）に b27 V-27h の追記 3 行と b14 §3 の未同定 9 種を同枠で流し込む。**追加分析なし**。
- **推定工数**: 小（F1 の反映バッチに**同梱**、+0.25 人日）。

### F6. forwarder/opcode カテゴリ系（v97/b09/b12/b21）⇔ 実行時消費経路

- **範囲**: v97 §4.4 で「`+0x18` と `+0x20` は同一 helper 共有」と確定した一方、(i) **帯域 A（0x00–0x1E）/帯域 B（0x100–0x183）で `+0x20` を読むコードが未発見**（既知はバンド C の FUN_0056AC50/0056AE20 のみ）、(ii) R6 アクション本体 16 件（StartQuest/ShowRaceMenu 等）の中身未精査、(iii) R1/R2 helper の a2/a3 読手未特定。
- **なぜ未解析か**: v97 は「骨格重複族」の調査であり、実行時消費側（CTDA 経路・opcode 実行経路）は b03/b10 と別タスク。帯域 A/B のアンカーが表基址の負インデックス式で見つからなかった（v94 同様のアンカー 0 件問題）。
- **何を読めば埋まるか**: 表全体 533 行の `+0x20` 非ゼロ 169 件を起点に、`.text` の imm32 参照（表範囲 [0xB0AF48,0xB10268) の走査、b03/b06 の手法を再利用）で読手を全数列挙。
- **推定工数**: 中（**1–1.5 人日**、走査スクリプトは b03 の流用可）。

### F7. ラベルなし/gap 台帳の多層断層（L1–L6 の未統合）

- **範囲**: dec_index 20,465 / Ghidra 台帳 21,597 / V86 10,175 / **Cat-3 10,261** / Cat-3b 365 / seed-B 新規 133 / V96 新实例 10 / V92 214 / v137 欠落 1,132 が**別々のタスクで別々に計測**され、統合住所表が無い。台帳追記提案（v96 (d)・v127 R3）は **V136 で承認待ちのまま未適用**、V140（§70.16 (5)-4 の新規 66 件）も未着手。
- **なぜ未解析か**: 各タスクが「成果物 1 ファイル制約」で他台帳を読むだけで書き込めない。統合作業の担当タスクが成立していない。
- **何を読めば埋まるか**: 住所キーの**統合テーブルを 1 本**作れば L1–L6 全層が突合可能（各層の入力はすべて既存: `func_list_new.txt`, dec_index はヘッダ走査, `v86_unlabeled_scan.txt`, v127 の Cat-3 出力, `v96_e8_candidates.txt`）。v137 §6 の「1,132 関数列挙はスクリプト 1 本」も同テーブルで副産物化。
- **推定工数**: 大（**1.5–2 人日**）だが**効果は最大**（F3・F7・v137 残点を同時に消す）。

### F8. CRT/起動系（v90/v93/v126）⇔ CRT ラベルなし関数群

- **範囲**: v126 で `.bind → steamdrm → 0x98787C` の経路は確定したが、**`0x98xxxx` 帯の 12 関数（0x98765B/0x98717A/0x987886/0x981C6B/0x981F78/0x981FB4/0x98D8A2/0x98D805/0x982897 等）は台帳・decompiled 双方に名称なし**（v126 §4-3）。v93 の初期化テーブル表 `[0xA283D0,0xA2F77C)` 7,403 スロットは 4,681 件のみ監査（残 2,722 件）。
- **なぜ未解析か**: v126 の目的は「経路確定」であり名称取得はスコープ外。v93 の全数監査は表の規模が大きく分割された。
- **何を読めば埋まるか**: CRT 文字列（`GetStartupInfoA`/`__getmainargs`/`mscoree.dll` 近傍）と呼び出し文脈、v126 §4-3 の呼び出し元一覧を再利用。残区間監査は v93 の既存スクリプト（`v93_step23/24.py`）のレンジ拡張のみ。
- **推定工数**: 小〜中（**0.5–1 人日**）。

### F9. opencode 系（v*/b*）⇔ Copilot 系（s*/sec*）成果物の突合

- **範囲**: `tmp/re/` は 2 エージェント系統の成果物が混在（s50–s69/sec37 系 = Copilot、v*/b* 系 = opencode）。数値 GT を持つ主要 s*（`s50_gmst.md`, `s57_script_vm.md`, `s60_*`, `s58_section.md`）と v/b 系の突合は**個別案件のみ**（例: b20 が `s57_script_vm.md` Wave5 と整合確認済 / v93 が `s50_gmst.md` を 378 で完全再現 / **v91 §8-7「s60 の 615 キーと件数差 20 の内訳未特定」が未照合**）。
- **なぜ未解析か**: 系統間の成果物索引が存在せず、相互参照は偶発。
- **何を読めば埋まるか**: 数値 GT を持つ s* 成果物の主要表（GMST / opcode 表 / INI キー / セクション定数）を機械抽出して v/b 系の実測値と突合する**突合表 1 本**。
- **推定工数**: 小〜中（**0.5–1 人日**）。

---

## 4. 推奨解析順（断層解消の高 → 低。番号は付けない＝採番は親セッション）

| 順位 | 断層 | 断層 | 推奨タスクの骨子 | 理由 |
|---|---|---|---|---|
| **1** | F1 + F5 | 成果物 → GT/docs/台帳の反映バッチ | v121 §3 の手順書に沿い、§2.3(b) 14 件 + pool A群（V143–V146）+ GT §4.1 常設（F5 同梱）を**まとめて**適用。.bak 採番 → 編集 → round_verify | 追加分析ゼロ・即実行可。反映を先送りすると**以後の全タスクが古い GT を前提に作られる**（既に b27 が b14 の誤りを検出するなど誤差が波及中） |
| **2** | F7（+ F3, v137 残点） | ラベル/台帳の統合住所テーブル | L1–L6 の 6 層を 1 テーブルに統合 → Cat-3 × renderer gap 突合（F3）・1,132 欠落列挙・seed-B 133 の triage・V136 追記候補の確定までを**1 タスクで完結** | 残存する最大の「住所ベースの未突合」。スクリプト 1 本で複数断層を同時に塞げ、後続タスク全般の足場になる |
| **3** | F4 前半 + F6 | オペコード実行時到達経路の解明 | band2/band3 の `+0x18` 到達（V131 相当）、`+0x1C` 例外 4 件（V132 相当）、帯域 A/B の `+0x20` 消費者全数、b20 dispatcher arg8/arg9 | 「オペコード表は読めたが実行時経路が half 未証明」という表全体の信頼性問題。§67.7 残課題 3 件の裏側でもある |
| **4** | F4 後半 | CC/slot ⇔ opcode 書手の双方向マップ | b18/b25/b27 の CC/process slot（`+0x1BC`/slot58/`+0x6C` 等）をキーに、opcode impl 側から書手を逆引き | 移植（Android）時に「スクリプト命令が戦闘状態へ書く」経路の仕様が必要。共有資産 3 点のみで構成でき実行コストが低い |
| **5** | F2 先行分 | ESM 接点の事前整備 | 既存 ESM 成果物で代替可能な確認（s*Description 置換・SCHR 供給源の一次走査なし確認等）を先に消化し、**解禁時に打つ 3 発（CTDA 生バイト / SCDA 22 件 / SCHR）のタスク仕様を確定** | 禁止事項により実行不可のため「仕様確定」までが今回可能な上限。解禁時に無調整で入ると走査コストが膨らむ |

- 補助順位: F8（CRT 命名、小）→ F9（s* 突合、小）は上記 1–3 の実行と並行して着手可能なスモールタスク。

---

## 5. 成果物パス＋サイズ（読取対象の実在確認済み）

| パス | サイズ (B) |
|---|---:|
| `C:\Users\hiroki.kogarumai\Oblivion_Android\tmp\re\b20_impl_5071d0.md` | 41,353 |
| `C:\Users\hiroki.kogarumai\Oblivion_Android\tmp\re\b21_speculative_backing.md` | 48,419 |
| `C:\Users\hiroki.kogarumai\Oblivion_Android\tmp\re\b25_cc_residuals.md` | 34,228 |
| `C:\Users\hiroki.kogarumai\Oblivion_Android\tmp\re\b26_nontt_class_names.md` | 38,198 |
| `C:\Users\hiroki.kogarumai\Oblivion_Android\tmp\re\b27_slot_residuals.md` | 27,853 |
| `C:\Users\hiroki.kogarumai\Oblivion_Android\tmp\re\b28_push_caller_residuals.md` | 27,995 |
| `C:\Users\hiroki.kogarumai\Oblivion_Android\tmp\re\v121_gt_corrections.md` | 36,744 |
| `C:\Users\hiroki.kogarumai\Oblivion_Android\tmp\re\v126_bind_crt_bootstrap.md` | 66,557 |
| `C:\Users\hiroki.kogarumai\Oblivion_Android\tmp\re\v127_labelless_cat3.md` | 24,126 |
| `C:\Users\hiroki.kogarumai\Oblivion_Android\tmp\re\v137_v80_global_residuals.md` | 30,878 |
| `C:\Users\hiroki.kogarumai\Oblivion_Android\tmp\re\v90_b350d8_triangle.md` | 26,769 |
| `C:\Users\hiroki.kogarumai\Oblivion_Android\tmp\re\v91_ini_defaults.md` | 26,901 |
| `C:\Users\hiroki.kogarumai\Oblivion_Android\tmp\re\v92_shader_renderer_bundle.md` | 33,099 |
| `C:\Users\hiroki.kogarumai\Oblivion_Android\tmp\re\v93_gmst_gt_residuals.md` | 18,668 |
| `C:\Users\hiroki.kogarumai\Oblivion_Android\tmp\re\v94_boundary_dups.md` | 31,509 |
| `C:\Users\hiroki.kogarumai\Oblivion_Android\tmp\re\v96_gap_instances_crosscheck.md` | 30,844 |
| `C:\Users\hiroki.kogarumai\Oblivion_Android\tmp\re\v97_forwarder_family_map.md` | 51,811 |
| `C:\Users\hiroki.kogarumai\Oblivion_Android\tmp\re\v80_partB_globals.md` | 20,340 |
| `C:\Users\hiroki.kogarumai\Oblivion_Android\tmp\re\b14_slot_method_map.md` | 29,283 |
| `C:\Users\hiroki.kogarumai\Oblivion_Android\tmp\re\b16_alarm_masks.md` | 21,237 |
| `C:\Users\hiroki.kogarumai\Oblivion_Android\tmp\re\b19_quest_byte_writer.md` | 25,970 |
| `C:\Users\hiroki.kogarumai\AppData\Local\Temp\opencode\v_open_rows.txt`（台帳 28 行） | 24,835 |
| `C:\Users\hiroki.kogarumai\AppData\Local\Temp\opencode\v143_residual_pool.md`（残点プール） | 4,935 |
| `C:\Users\hiroki.kogarumai\Oblivion_Android\docs\RE_EXE_REPORT.md`（見出しのみ抽出） | 1,029,892 |

- 参照のみで読んだ共有資産: `tmp/re/cache/{opcode_table.json, call_targets.json, named_funcs.txt, vtable_index.tsv, README.md}`、`tmp/re/GT_CHEATSHEET.md`、`tmp/re/handover_20261025/{session_A_summary.md, session_B_summary.md, deliverables/v76_data_relationship_map.md}`、`tmp/re/{b18_cc_field_semantics.md, b13_nonrtti_vtable_classes.md, v60_esm_only_keys.md, SUBAGENT_CONTRACT.md}`。
- 本タスクで生成した補助物: `%TEMP%\opencode\vrelmap_extract.py` / `vrelmap_extract.txt` / `vrelmap_xref.py`（スクリプト置き場の規定遵守、repo 外）。

---

## 6. 付記: 断層の全体像（1 行まとめ）

> **「分析は 21 成果物で済んでいるのに、訂正の反映（F1）・住所の統合（F3/F7）・実行時経路の接続（F4/F6）・ESM との一次証（F2）の 4 方向に、成果物どうしをつなぐ横断タスクが 1 件も入っていない」** — これが現状のデータ断層の本質。特に F1（反映）は追加分析ゼロで即着手でき、後続全タスクの前提精度を決めるため最優先。
