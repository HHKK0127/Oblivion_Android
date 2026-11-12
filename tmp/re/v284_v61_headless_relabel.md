# v284_v61_headless_relabel.md — v60v76-bundle 依頼2 [P2]: v61 headless relabel 残件6点（2026-11-04）

- 依頼元: `tmp/re/handoff_20261105_v60v76_residuals_bundle.md` 依頼2 [優先度 P2]（出典 `tmp/re/v61_headless_relabel.md` §7 未解決点）
- 対象: Oblivion.exe v1.2.0416 (Steam) / Ghidra 12.1.4 プロジェクト `OblivionRe`
- ハーネス: `probe_harness.py selftest` → **4/4 PASS**（本セッション実行）
- **判定: 6点中 5点 [確定]、1点 [部分確定]（項目6 = 適用判断）**

---

## 項目1: `prev_exists` 列の意味 → **[確定] 「前関数が新解析に実在するか」を表す列。実測 362/362 = true（V61 §4.3-4 の弱点は解消）**

### 1-1. 意味（仕様の定義）

V61 §6 の V70 仕様で提案された出力列で、**各ギャップ VA の `prev_fun`（v10 CSV の `prev_va` 列）が新解析の FunctionManager に存在するか**を真偽で記録するもの。用途は V61 §4.3-4 の弱点（「`contained=0` が『前関数が在るのに本体がギャップを含まない』のか『前関数自体が不在』なのか区別できない」）の切り分け。

### 1-2. 実測値（本タスクで計算）

入力: `v10_impl_decompile_join.csv` の `in_decompile=no` 362行（`prev_va`/`next_va` 列）× `%TEMP%\opencode\func_list_new.txt`（V70 後 21,597）× `v70_force_create.csv`（`created` 357 件を減算して **V70 前の 21,240 を復元**、件数一致を確認）。

| 列 | V70 前の 21,240 で | V70 後の 21,597 で |
|---|---|---|
| **prev_exists** | **362 / 362 = true**（false 0件） | 362 / 362 = true |
| next_exists | **362 / 362 = true** | 362 / 362 = true |

### 1-3. 判定の帰結

- **前関数は362件すべて旧解析（21,240）に実在する** → `getFunctionContaining(gap)=null` は「前関数が不在」ではなく「**前関数の本体がギャップを含まない**」を意味する。
- → **V61 §4.3-4 の弱点は解消**。吸収説の機構主張（「Ghidra が前関数の CFG に統合した」）は **決定的に否定**（前関数が在り、かつギャップを包含していない）。
- V61 §4.3-4 の留保「前関数が新解析に実在しなければ吸収説を否定できない」は**不成立側に確定**。

---

## 項目2: 21,240 件の内訳 → **[確定] 名前パターン別5分類（合計一致）**

`func_list_new.txt`（21,597）から V70 作成357件を除いて**21,240 を復元**し、Ghidra 名前で分類:

| 分類 | 件数 | 例 |
|---|---|---|
| `FUN_*`（通常名） | **20,473** | `FUN_00480000`, `FUN_00640000` … |
| `__*` / `_*`（CRT/特殊アンダースコア） | **465** | `__cftoe`, `__cftoa_l`, `__cftof2_l` … |
| その他特殊名 | **152** | `CreateObject`, `tidy_global`, `facet_Register`, `~_Fac_node`, `FID_conflict:_CallMemberFunction1` … |
| `thunk_FUN_*` | **100** | `thunk_FUN_0089f8e0` … |
| `` `scalar_deleting_destructor' `` | **50** | （RTTI 由来の削除デストラクタ群） |
| **合計** | **21,240** | ✓ |

### 2-1. 既存記述との会計（V73 と完全整合）

- V73 の会計「decompiled 定義 20,465 ＋ 非 FUN 特殊名 775 = 21,240」と、本分類は**両立**する:
  - Ghidra `FUN_*` 20,473 = decompiled ヘッダ定義 20,465 ＋ **8**（Ghidra は `FUN_` 名だが decompiled.c ヘッダ未収録）
  - 非 FUN 名 767（465+152+50）＋ **8** = 775 ✓（V73 の「Ghidra のみ・非 gap = 775」と完全一致）
- `thunk_FUN_*` は Ghidra 100 件 / decompiled.c の `thunk_FUN_` 定義 87 件 → **差13件は名寄せ・正規化の方式差**（V73 の「thunk 対応は未検証」への部分回答）。
- 参考: V70 後は 21,597（= 21,240 + 357）。

**運用推奨（V73 と同じ結論を維持）**: 「関数総数」は方式を併記する（`Ghidra解析世代=21,240 → V70後21,597` / `decompiled本文=20,465` / `dec_index=20,465`〔V77 で20,465に更新済〕）。

---

## 項目3: gap 362 件 vs 357 件の内訳差 → **[確定] 差5 = 同一実装を共有するエイリアス opcode が2 opid 登録**

`v10_impl_decompile_join.csv` の impl 集合で照合（362行 = 357ユニークVA、重複5 VA × 各2行）:

| 共有 impl | opid / name（2件ずつ） | V10 gap_kind |
|---|---|---|
| `0x00503E10` | `0x0004` **OnEquip** / `0x001C` **OnActorEquip** | ghidra_missing |
| `0x00503E90` | `0x0005` **OnUnequip** / `0x001D` **OnActorUnequip** | ghidra_missing |
| `0x0050D020` | `0x10D4` **AdvancePCSkill** / `0x110F` **ModPCSkill** | ghidra_missing |
| `0x005007D0` | `0x0151` **ShowSubtitle** / `0x1164` **ShowDialogSubtitles** | absorbed_suspect |
| `0x00500890** | `0x0155` **OutputMemContexts** / `0x0169` **ResetMemContexts** | absorbed_suspect |

- **台帳（opid）単位 = 362 行 / VA 単位 = 357 件**。差5 は「1つの実装関数に2つの opcode 名が登録されている（エイリアス）」ことで、**実装の一覧側の重複ではなく opcode 側の多対一**。
- V79 §6-6 の残点「opid を変えて2行登録されている原因」→ **上表で確定**（OnEquip⇔OnActorEquip 等の別名群＋スキル系・字幕系・メモリ文脈系の2名共通実装）。
- 運用ルール（V61 §7-3 を維持）: **集合比較は VA 単位（357）、台帳報告は opid 単位（362）**。

---

## 項目4: `isExecute` フラグの用途 → **[確定] 「コードブロックだが seed 不在」か「data 誤分割」かの切り分け軸。実測は362/362 が実行可能ブロック**

### 4-1. 用途（V70 仕様での位置づけ）

`getMemoryBlock(va).isExecute()` は、ギャップが **実行可能ブロック（コード）** に属するか **data ブロック**（=オペコードテーブル等のデータを関数と誤分割した結果）に属するかを切り分ける判定軸。共通原因の仮説（V5 行595「オペコードテーブル由来で CALL 到達不能＝関数作成の seed が無い」）の検証が目的。

### 4-2. 実測値（PE セクション属性で全数判定）

ギャップ VA 範囲 = `0x500450 .. 0x514500`。全362件は **`.text` セクション（VA 0x401000 / size 0x626C39 / Characteristics `0x60000020` = IMAGE_SCN_CNT_CODE | EXECUTE | READ）** の内側:

| 判定 | 件数 |
|---|---|
| isExecute = **true**（.text / EXECUTE） | **362 / 362** |
| isExecute = false（data ブロック） | **0** |

- → **「実行可能ブロックに在るのに関数 seed が無い」が確定**。data 誤分割説は本集合には適用されない（= V5 の seed 不在説に有利、V79 の「オペコードハンドラ領域 0x50xxxx への集中」とも整合）。
- 補足: Ghidra 側の `getMemoryBlock(va)` は .text を1ブロックとしてマップするため、ブロック単位の再照会でも同一結果になる（本判定は PE ヘッダの Characteristics による全数実測。Ghidra 再実行は行っていない = 方法の注記）。

---

## 項目5: cc2〜cc3 の 59 件の最終分類 → **[確定] V5「欠落」が正（完全独立関数 51 / 薄片 5 / ガベージ 1）**

入力: `v79_classified.csv`（357行、`cc` 列 = V5 の CC前置バケット、`cls`/`reason` 列 = V79 分類）。`cc=2` / `cc=3` を抽出:

| バケット | 行数 | class1 完全独立 | class2 薄片 | class3 ガベージ |
|---|---|---|---|---|
| cc×2 | 43行 → 42U | 39 | 2 | 1 |
| cc×3 | 16行 → 15U | 12 | 3 | 0 |
| **計（= 依頼の59件）** | **59行 / 57ユニーク** | **51（89.5%）** | **5（8.8%）** | **1（1.8%）** |

- reason 内訳: `full-body` 50 / `tiny` 4 / `call+return-forwarder` 1 / `small-with-cf` 1 / `trivial-garbage` 1。
- **最終分類 = V5 の「欠落（missing）」が正**。57件中51件が本文200字超・分岐/複数呼び出しを備えた完全な独立関数。薄片5件（`0x00500E40`/`0x005064A0` tiny、`0x00508180` 等）とガベージ1件（`0x005087A0`、`DAT_00b42f3c = … != 1` の最小反転）も V70 で create 成功済み＝**自己完結した実在関数**。
- → **V10 の `absorbed_suspect`（吸収の可能性）はこの59件について再分類すべき**（`ghidra_missing` 側へ）。V61 §7-6 の「最終分類が未決着」→ **決着**。
- 根拠の重ね: V61 の `contained=0`（本項目1で前関数実在を確認し決定化）＋ V70 の `created=357/failed=0` ＋ V79 の χ²=0.611（cc と薄片化は無相関）。

---

## 項目6: §5 relabels の適用判断 → **[部分確定] 適用可（v5 報告案A/B・v10 案B）／v10 案A は不可。ただし RE_EXE_REPORT 分は既適用**

### 6-1. 現状の棚卸し（実測）

| 対象 | v61 §5 の案 | 現状 |
|---|---|---|
| `docs\RE_EXE_REPORT.md` §67.6.1 | 案A（行11417置換）・案B（行11415直後挿入） | **適用済み** — 現行 **L11530**（案A文）/ **L11528**（案B文）に同一文面を確認。行番号は案から **+111 シフト**（後続の追記による） |
| `tmp\re\v5_ghidra_gaps_report.md`（599行） | 案A（行60直後に1行）・案B（§9 新章） | **未適用** — 「V61」語0件・599行のまま |
| `tmp\re\v10_impl_decompile_join.csv`（528データ行） | 案A（末尾 `#v61_relabel` 行）／案B（列 `,v61_status` 追加） | **未適用** — マーカー0件 |

### 6-2. 適用可/不可の判断

| 案 | 判定 | 根拠 |
|---|---|---|
| v5 報案A（1行追記） | **適用可** | 記載値（362/362 NO_FUNCTION、defined=0/contained=0、containing_name 全空）は V61 の gap_scan.csv 実測と一致し、本タスクでも再確認。数値の誤りなし |
| v5 案B（§9 新章） | **適用可（改訂付き推奨）** | 同上。ただし V61 以降に **V70（created=357/failed=0・総数21,597）／V73（会計解消）／V79（吸収仮説=不成立）** が確定済み → 旧案のままでは「吸収説の弱化材料」止まりで最新結論を過小評価する。**V70/V73/V79 を併記した改訂版にしてから適用すべき** |
| v5 案C（区分表に V61 列） | 任意・適用可 | 数値矛盾なし |
| v10 案A（`#` 行追記） | **不可** | v61 自身の留保どおり `#` 行は `csv.DictReader` に1データ行として読まれ、後続の解析を壊す。**案B を採用** |
| v10 案B（列追加） | **適用可** | 362行に `NO_FUNCTION`、`in_decompile=yes` の165行は空欄（V61 未走査）という仕様は事実と一致 |
| （参考）RE_EXE 分 | 追加適用不要 | 既適用。**ただし L11528/11530 は V61 時点の記述で、V70/V73/V79 の後続結果が未反映** → 将来の追記候補（人間承認が必要） |

### 6-3. 適用時の必須条件（3点）

1. **行番号の再特定**: 案は 2026-10-24 時点の行番号。RE_EXE は既に +111 シフトを経験しており、v5/v10 も後続編集でずれている可能性がある → 適用直前に文面照合で再特定する。
2. **V70/V73/V79 への統合**: 「吸収説の弱化」→「**仮説不成立（V79 §5.1）＋357/357 作成成功（V70）**」に更新した文面にする。
3. **編集権限**: `docs/` および `tmp/re/` の正本編集は当方の運用上【要承認】（本依頼は判断の提示まで）。**適用は人間の承認後**。

---

## 残点（記録）

1. Ghidra ブロック単位の `isExecute` 再照会（本判定は PE セクション属性による。結果は同一と見込まれるが未実施）。
2. `thunk_FUN_*` の Ghidra100 vs decompiled87 の13件差の個別同定。
3. class3 3件（`0x00500820`/`0x00501A60`/`0x005087A0`）のバイナリ決着（V79 §6-2 のまま）。

## 証跡

| スクリプト / 入力 | 内容 |
|---|---|
| `%TEMP%\opencode\v284_p2.py` + `v284_p2_out.txt` | 本タスクの全計算（prev_exists / 21,240 分類 / PE Characteristics / cc2-3 分布 / エイリアス対） |
| `%TEMP%\opencode\func_list_new.txt` | Ghidra 21,597 関数（start,name） |
| `%TEMP%\opencode\v70_force_create.csv` | created 357 件（21,240 の復元に使用） |
| `%TEMP%\opencode\v79_classified.csv` | 357 行の cc/cls/reason |
| `tmp\re\v10_impl_decompile_join.csv` | 362 gap 行の prev_va/next_va/gap_kind |
| `docs\RE_EXE_REPORT.md` L11526-11530 | V61 案A/B 適用済みの確認 |

検証日: 2026-11-04。ハーネス: `probe_harness.py selftest` 4/4 PASS。内部整合: 21,240 復元一致 / 分類合計21,240一致 / 362行=357ユニーク再現 / prev&next 362/362 / isExecute 362/362 / cc2-3 = 59行=57U。
