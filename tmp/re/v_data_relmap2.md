# v_data_relmap2.md — RE 成果物データ関係性マップ 第2版（v155–v174 反映）

- 作成日: 2026-10-28 / 作成者: データ関係性マップ更新担当サブエージェント
- 編集範囲: **本新規 1 ファイルのみ**。docs/ / GT_CHEATSHEET.md / TODO_re49.md / 台帳 / ledger / plan / 既存 tmp/re 成果物は一切変更していない（読取のみ）。
- 反映対象: 初版 `v_data_relmap.md`（2026-10-26）以降に追加された **20 成果物**（v155–v174 系、適用ログ 3 本を含む）+ 関連 CSV 6 本。初版の F1–F9 断層分類・18 矛盾リスト・6 層台帳論を構成の基礎として継承。
- 方法: 各成果物の見出し構造・判定サマリ・残留不明点節・適用ログの適用/スキップ表を機械抽出、TODO_re49.md のテーブル行（serial 140–164）と `- [ ]` 箇条書き 55 件を照合、CSV はヘッダ+行数を実測。
- 言語: 本文日本語・コード/識別子原文（project-rule 遵守）。

---

## 0. 棚卸し範囲と serial 対応（第2版で更新）

### 0.1 ファイル ↔ TODO serial ↔ 状態

| 成果物ファイル | TODO serial | 状態（TODO_re49.md 実測） |
|---|---|---|
| `v155_label_ledger_union.md`（+同名 CSV） | 145（6層統合テーブル） | [x] 完了（本体は 10-27 追記で独立再検証・V140/V136 突合まで延長） |
| `v156_tail_band_identity.md` | 146（tail 帯域正体） | [x] 完了（§6 追記 = 独立住所集合突合） |
| `v157_runtime_residuals.md` | 147（opcode 実行時残点） | [x] 完了（§12 追記 = 二重実行対応・訂正 6 件） |
| `v158_slot_opcode_map.md` | 148（CC/slot⇔opcode 双方向） | [x] 完了（§9 追記 = 独立再検証 56/56） |
| `v159_renderer_residuals.md` | 149（renderer 束） | [x] 完了（16 件裁定） |
| `v160_ini_log_residuals.md` | 150（INI/Setting/Log 束） | [x] 完了（9 残点） |
| `v161_quest_extra_residuals.md` | 151（quest/Script/extra 束） | [x] 完了（16 点 + §6 追記） |
| `v162_gap_residuals.md` | 152（E8/台帳ギャップ束） | [x] 完了（R-k = read_ptr バグ発見。v169 で前提訂正適用済） |
| `v163_gt_slot_semantics.md` | 153（GT slot 語義） | [x] 完了（6 残点 + GT §4.1.1 構成案） |
| `v164_esm_gate_spec.md` | 154（ESM 接点事前整備） | [x] 完了（§9 追記 = 再試行・コマンド仕様確定） |
| `v165_v140_recompute.md` | 155（read_ptr バグ波及再計算） | [x] 完了 |
| `v166_eh_funclet_ledger.md` | 156（EH funclet 一括台帳化） | [x] 完了（10,742 エントリ CSV 化） |
| `v167_v144_apply_log.md` | 144 の適用ログ | 適用 30 操作成功 / スキップ 11 グループ |
| `v168_v143_apply_log.md` | 143 の適用ログ | GT 8 操作 + docs 6 操作成功。A–D は v171 へ |
| `v169_v140_correction_log.md` | 155 の訂正適用ログ | v140 9 箇所 + v162 2 箇所成功 |
| `v170_ledger_granularity.md` | 157（台帳追記粒度裁定） | [x] 完了（追記候補 CSV 10,870 行） |
| `v171_gt_candidates.md` | 158（GT 裁定待ち A–D） | [x] 完了（A–D 確定・反映は承認待ち） |
| `v172_runtime_residuals2.md` | 159（opcode 実行時 第2束） | [x] 完了（12 件裁定） |
| `v173_small_residuals2.md` | 160（小残点 第2束） | [x] 完了（13 件裁定） |
| `v174_gap_residuals2.md` | 161（gap/E8 + read_ptr 防御束） | [x] 完了 |
| （成果物なし） | **162（V144 スキップ残の最終処理）** | **[ ] 未着手（実行中の可能性 — 本マップは v175+ を無視）** |
| （成果物なし） | **163（ESM 解禁時仕様 N1–N9）** | **[ ] 実行中とされる（除外）** |
| （成果物なし） | **164（GT slot 語義 V-163a..e）** | **[ ] 実行中とされる（除外）** |

- **旧 letter-row 系**: TODO_re49.md の F/G/H/I/J/N/O/P/Q/R/W3 節の `- [ ]` 箇条書き **55 件**（L33–L162、2026-10-17 起票）は**前版以降 1 度も触れられていない**（§3-④）。
- 関連 CSV 実測: `v155_union.csv` 31,905 行（`VA,func_list_name,dec_header,V86,Cat3,Cat3b,seedB,I_no,notes` / 1,507,687 B、tmp/re 配置分は %TEMP% 版と SHA256 一致）/ `v166_eh_entries.csv` 10,742 行（`VA,kind,scope,host_guess,notes`）/ `v170_ledger_append_candidates.csv` 10,870 行（11 列）/ `v174_xcu_candidates.csv` 1,199 行 / `v174_r_f_noterm_spans.csv` 1,376 行 / `v158_map.csv` 690 行（%TEMP%）。

### 0.2 docs / GT の現況（V143/V144 適用後）

- `docs/RE_EXE_REPORT.md`: **1,038,004 B / 12,352 行**（初版 1,029,892 B / 12,319 行から +33 行 = V144 で +28、V143 で +5）。.bak16 / .bak17 あり。
- `tmp/re/GT_CHEATSHEET.md`: **19,734 B / 180 行**（152 → 180 = +28。§4.1「vtable slot 主要対応」新設・§4.1.1 不整合候補 A–F あり・§5 訂正履歴追加・§6 運用ルール追加）。.bak_apply1 あり。

---

## 1. 新成果物マップ（v155–v174）— 主データ資産と相互参照

### 1.1 成果物 ↔ 主資産 ↔ 参照先（★ = 初版からの新規共有資産）

| 成果物 | 主データ資産（入力/出力） | 新規共有資産 ★ | 主な相互参照（前版からの追加関係） |
|---|---|---|---|
| v155（統合テーブル） | 入力: func_list / dec_index / v86 scan / v127 Cat-3 / seed-B / I1–I10。出力: **統合 CSV 31,905 VA** + `v145_missing1132.csv`（1,132 行） | ★ `v155_union.csv`（住所単一ソース） | v127/v133/v137/v140/v96/v136 を**全件突合**。v137 残点「1,132 列挙」を充足。U1（767/684）口径裁定 |
| v156（tail 帯域） | 入力: `v127_cat3_regions.json` / v92 gap 表 / vtable_index / capstone。出力: 帯域 t1–t5 分類 | ―（訂正: v137 .?AV 位置 0xB02008） | **F3 断層を解消**（renderer gap 102 件 × tail 交差 0）。v156 訂正 5 件を §6-5 に明記 |
| v157（opcode 実行時） | 入力: `cache/opcode_table.json` / decompiled / capstone。出力: 裁定 8 点 + 訂正 6 件 | ― | v131/v132/v134 の残点を消化。b20 の arg8/arg9 経路を確定。→ v172 へ V147-r1..r6 を継承 |
| v158（双方向マップ） | 入力: opcode_table × call_targets × vtable_index。出力: **`v158_map.csv`（690 行 11 列）** | ★ `v158_map.csv`（field⇔opcode 690 ペア） | **F4 断層を解消**。b18/b25/b27/v138/v139/b24 を横断。ハーネス罠 4 件（func_range off-by-one 等）を GT 反映候補として記録 |
| v159（renderer 束） | vtable/RTTI/capstone。16 件裁定（A–P） | ― | v129/v128 の残点を全数裁定。§4 残点 5 件 → v173 へ |
| v160（INI/Log 束） | GameSetting map / IAT 手動パース。9 残点裁定 | ― | v130/v120/v137/v91 残点消化。R-N1..N4 → v173 で全解消 |
| v161（quest/Script/extra） | decompiled / vtable / plan.md L921（ESM GT 引用）。16 点 + §6 独立再測 | ― | v141/v139/v142 残点消化。V-161a..i → v173 で大部分解消 |
| v162（E8/gap 束） | v140/v133/v155 を入力。MinHash 初実施 | ― | **read_ptr() セクション定数バグを発見（R-k）** → v165/v169 へ接続。Cat-3 三層差分を文書化 → v170 が裁定 |
| v163（GT slot 語義） | capstone/vtable 全走査 19 ステップ。6 残点裁定 | ― | v121 B 群を消化。+8=CopyFormAttrs / GetFormID=TESForm+0xC / 0x36C=状態 を確定 → v168 が GT/docs へ反映。**GT §4.1.1 構成案**を設計 |
| v164（ESM ゲート仕様） | 既存 ESM 成果物引用（再スキャンなし）。走査仕様①–④ + N1–N9 | ― | relmap F2 を「仕様確定」まで前進。v161 の SCHR 確定分と plan.md L921 を突合 |
| v165（read_ptr 再計算） | v140 系スクリプト全数スキャン。6 本 BUG 認定・10 数値更新・80 行超不変 | ★ `%TEMP%\opencode\v165_recompute.csv` | v162 R-k の波及を全計算。Rs1..Rs6 → v174 §7 で裁定 |
| v166（EH funclet 台帳化） | .rdata FuncInfo 走査。**`v166_eh_entries.csv` 10,742 行** | ★ `v166_eh_entries.csv`（EH 第 7 層） | v156 R2（189KB 帯）を実行。v155 統合表との重複を裁定（§4）。R1..R7 → v170/v174 へ |
| v167（V144 適用ログ） | docs 7 + b 系 23 操作。skip S1–S11 | ★ `.bak16` + b 系 `.bak_apply1` ×8 | **relmap C リスト/R リストの 9 件（R3–R11）を docs/b 系へ適用**。S1 で「docs に 684 表記なし」を実測 |
| v168（V143 適用ログ） | GT 8 操作 + docs 6 操作 | ★ GT `.bak_apply1` | R1/R12/R13 を回収。GT §4.1 新設。E/F は確定反映、**A–D は候補記載のまま v171 へ** |
| v169（v140 訂正ログ） | v140 9 箇所 + v162 2 箇所 | ★ `.bak_apply1` ×2 | v165 の訂正ドラフトを適用。call-origin 222→150、未知 22→3（IAT）へ確定 |
| v170（台帳粒度裁定） | v166/v155/v162 を集合演算。**`v170_ledger_append_candidates.csv` 10,870 行** | ★ 追記候補 CSV（adopt 10,572 / conditional 144 / adopt_priority 21 / backlog_priority 133） | Cat-3 三層の正 = published 10,261 を裁定。共有 FI 表記方法を確定。**H1–H7 人間判断を起票** |
| v171（GT 裁定 A–D） | vtable 実測 9 ステップ + 厳格ディスパッチ 871/214/201/90 箇所 | ― | **R2（GT 不整合 A–D）を確定**（GetBaseForm / GetParentCell / GetScale / 系統別 3 語義）。GT §4.1.1 確定行ドラフト + docs §41.3 訂正ドラフトを用意。**残点 10 件** |
| v172（opcode 第2束） | v157 O-a..O-e + v158 V-158a..g を 12 件裁定 | ― | Begin u32 消費経路・RunScript 8 引数・52 タグ表・R1 111 行一般化を確定。**V-172-r1..r5 を起票** |
| v173（小残点第2束） | v159(5)/v160(4)/v161(4) 計 13 件裁定 | ― | 逆 vtable マップ（1,715 全 vtable 実読み）を新手法として確立。ESM GMST「insert なし」を確定。**V-173a..c を起票** |
| v174（gap/E8 第2束） | v162 R-a..R-m + v165 Rs1..Rs6 を裁定。**`v174_xcu_candidates.csv` 1,199 行 / `v174_r_f_noterm_spans.csv` 1,376 行** | ★ XCU 候補 CSV / R-f スパン CSV | C 群 wrapper = C++ EH unwind action と解明（v162 前提を訂正）。XCU 7,380 のうち 1,199 = **完全未登録層**を特定。Ghidra MCP は OblivionRe プロジェクト不在を実測 |

### 1.2 依存チェーン（第2版で新規に成立した接続）

```
v162(R-k 発見) ──→ v165(波及再計算) ──→ v169(v140/v162 へ訂正適用)
v156(R2 起票) ──→ v166(EH 台帳化 10,742) ──→ v170(粒度裁定+CSV) ──→ [人間承認] 台帳追記
v166(R-l) / v162(R-l) ──→ v174(XCU 候補 1,199)
v121/v120 ──→ v163(slot 6 点確定) ──→ v168(GT/docs 反映) ──→ v171(A–D 確定) ──→ [人間承認] GT §4.1.1
v155(統合 CSV) ──→ v162 / v166 / v170 / v174 が全員共通の突合キーとして利用（住所単一ソース化が浸透）
v157 / v158 ──→ v172（opcode 第2束）    v159 / v160 / v161 ──→ v173（小残点第2束）    v162 / v165 ──→ v174
```

- **前版 F 断層の消化**: F1（反映断層）= V143/V144 適用で **9+3 件回収**（残りは §2/§5）、F3（renderer⇄Cat-3）= v156 §6-1 で**解消（交差 0）**、F4（CC/slot⇄opcode）= v158 双方向マップで**解消**、F7（6 層未統合）= v155 統合表で**解消**（+第 7 層 L7/EH 層の新論点へ進化）、F2（ESM）= v164 で**仕様確定**、F6（+0x20 消費者）= v157/v172 で大部分解消。
- **残る前版断層**: F5（GT slot 表統合）= §4.1 新設済・§4.1.1 の A–D 確定反映待ち、F8（CRT 命名）= v156/v159/v174 で大部分消化（残: __purecall handler 名等の微小件）、F9（s* 突合）= **依然個別案件のみ**（§3-③）。

---

## 2. 矛盾・訂正の消化状況（前版「未解消矛盾 18 件」の内訳）

根拠: v167（V144 適用: 適用 30 / skip S1–S11）、v168（V143 適用: GT 8 + docs 6）、v169（v140/v162 訂正 9+2）、v170（台帳粒度裁定）、および各成果物の追記節。

### 2.1 (a) 真に未解消 4 件の現在

| # | 対立 | 第2版時点の状態 | 根拠 |
|---|---|---|---|
| U1 | 名前付きヘッダ **684 vs 767** | **解消（口径確定）** — 767 正・+83 = v90 の identifier-only 正規表現が backtick RTTI 50 + FID_conflict 20 + デコレータ 13 を漏れ。684 の持主は v90（v127 出所誤りも訂正）。**docs に「684/21,149」表記は不在**（v167 S1 で実測）= docs 訂正は不要と判明 | v155 §2・v162 §3-8・v167 S1 |
| U2 | `call 0x9832E6` **1,819 vs 1,818** | **残存** — 1 サイト差の要因は未特定。v167 B14-2 で b14 へ「母数 1,819 / castType=0 1,499 / =1 0 件」の訂正注記は適用済（記述は整った）が、差の原因解明は未着手 | v167 B14-2 |
| U3 | `0xa9d798` slot0/1 = 描画系仮想関数 | **残存** — v155–v174 のいずれも未着手（b26 R-2 のまま） | （触手なし） |
| U4 | v81 の `.rdata` 定数 0x4009D0 誤導の原因 | **残存** — 原因は未特定。ただし v165 が read_ptr() セクション定数バグ（+0xE00 ズレ）という「ハーネス由来バグの実在」を実証したことは同型仮説の傍証（v81 当時スクリプトの未走査 = v165 Rs3 で範囲外と明記） | v165 §1-2/Rs3 |

### 2.2 (b) 誤り確定済み・編集禁止で未反映だった 14 件（R1–R14）

| # | 誤り確定した主張 | 反映先 | 第2版時点 | 根拠 |
|---|---|---|---|---|
| R1 | GT slot 5 語（+8/0xA0/0x2D0/0x36C/0x190） | GT §4.1 + docs §17.2/§41.3 | **解消**（GT §4.1 新設 G3 + docs D1–D5） | v168 §2/§3 |
| R2 | GT 新規不整合 A–F | RE_EXE_REPORT | **部分解消** — E（+0xF8）/F（+8 レベル 4 値）は反映済（v168 D6/D1）。**A–D は v171 で確定（GetBaseForm/GetParentCell/GetScale/系統別 3 語義）だが、GT §4.1.1 は候補行のまま＝確定ドラフト反映が承認待ち** | v168 §6・v171 §0/§2 |
| R3 | v80 の svs 経路（帰属バイアス） | docs §70.15(6) | **解消**（D4 で「V137 で解決・本項クローズ」注記） | v167 §2.1 |
| R4 | §35.5 `0x981FB4` = atexit + §70.10 3 項目 | docs §35.5/§70.10 | **解消**（D1/D2/D3a/D3b） | v167 §2.1 |
| R5 | b14 §1.2「+0x154 → 0x4B3750」 | b14 | **解消**（B14-1） | v167 §2.2 |
| R6 | b14 §5-6「静的キャスト=1」 | b14 | **解消**（B14-2） | v167 §2.2 |
| R7 | b13 §6-3 の 2 仮説 | b13 + b13_final_table | **解消**（B13-1/B13-2 + FT-1..3、.bak 付き例外適用） | v167 §2.2/§3 |
| R8 | b12「D類 35 件は裏取り不可」 | b12 | **解消**（B12-1..5: 25 確定 / 9 準確定 / DropMe 1） | v167 §2.2 |
| R9 | b09「推定 174 件」 | b09 | **解消**（B09-1/2: 47 件確定昇格 → 残 127） | v167 §2.2 |
| R10 | b04「45 件未特定」 | b04 | **解消**（B04-1: 33 → 残 1、45 撤回） | v167 §2.2 |
| R11 | b15 の arg17 / param 3 件 | b15 | **解消**（B15-1/2） | v167 §2.2 |
| R12 | handover session_B L43/44/49 | GT §7 注記 | **解消**（G5 運用ルール + G6 注記 = 参照禁止宣言） | v168 §2 |
| R13 | GT §2 に `0x5072B0` 不在 | GT §2 | **解消**（G2 追補） | v168 §2 |
| R14 | v81 0x7B14BE（2B 差）/ V5 表 `opid` / `cc×8` | v81, V5 | **残存** — v167 S6 でスキップ（ドラフト元・対象ファイルが B グループ外。「不明ならスキップ」規約適用） | v167 S6 |

### 2.3 消化サマリ

| 区分 | 件数 |
|---|---:|
| **解消**（反映/裁定とも完了） | **13**（U1 + R1/R3/R4/R5/R6/R7/R8/R9/R10/R11/R12/R13） |
| **部分解消**（分析確定・反映承認待ち） | **1**（R2 の A–D：ドラフトは v171 に用意済） |
| **残存**（要因未特定 or 対象外スキップ） | **4**（U2 / U3 / U4 / R14） |

- 補助消化: v169 が v140 系の read_ptr 由来数値（P2/P3 帰属・call-origin 222→150・未知 22→3）を訂正、v162 R-k の前提を「バグ 5 本限定・波及調査完了」へ訂正。これにより前版 F1 の中核（反映断層）は **18 件中 14 件が閉鎖**し、残 4+1 件は §5 の人間ゲートに集約された。

---

## 3. 未解析領域（最重要）— 優先順位付き

### ① 旧 letter-row 系 55 件 — **前版以降まったく触れられていない最大の未着手層**【優先: 高（起票欠如の是正）】

- **実体**: `TODO_re49.md` の F/G/H/I/J/N/O/P/Phase3/Phase3-4/R/Phase4 節の `- [ ]` 箇条書き **55 件**（L33–L162、2026-10-17 起票）。V 系タスク（serial 116–164）のどれもこの 55 件を出典にしていない。成果物の受け渡し経路が**一度も構築されていない**。
- **中身の分布**: 音響 7 項目 / Sleep-Jail 8 / menu 式 4 / Container 8（F 節）、§56–§58 訂正反映・実機確認（G/H 節）、セーブ .ess・KD 移植判断（I 節）、Wave 6b レンダラ/Havok/ストリーミング（J 節）、HavokContext vtable・RayCast・戦闘 AI・PathingSystem（N 節）、エンジン非 Havok 6 系統 O1–O6（O 節）、Havok 92 メソッド・呪文スコアリング・Pathing 移植（P 節）、**ESM/DLC 統合 Q1–Q8**（content_inventory 116 万件/WRLD/MGEF/WATR/DLC SI parse 失敗等）、BSA/NIF/DDS Q9–Q12、アニメ/シェーダ 48 変種/字幕 R2–R5、Wave 3 深掘り W3-1..7（Alchemy/Enchant/SEFF/MGEF+0x68/Poison/Absorb）。
- **接続の断層**: これらの入力は **s\* 系 Copilot 成果物と docs §56–§60 域**。v/b 系（opencode）の成果物手法（probe_harness / vtable 実測 / 統合 CSV）は**一度も投入されていない**。逆に v159/v172/v173 で確定した renderer/GMST/Script 事実の一部（例: O1 のシェーダハンドルテーブル、W3-3 の SEFF VM）は新成果物側の知識で**短縮できる可能性が高い**のに相互参照が 0 件。
- **推定工数**: 起票束化 0.5 人日 + 各束 1–2 人日（Q 群は統合ドキュメント化中心で 0.5 人日×8、O/P/N 群は解析本体）。

### ② 台帳追記の承認・適用フロー未完（提出済み素材が積まれたまま）【優先: 高】

- **素材は揃っている**: v170 `v170_ledger_append_candidates.csv` **10,870 行**（adopt_priority 21 = I1–I10 + E8 11 / adopt 10,572 = 帯域 EH / conditional 144 = catch / backlog_priority 133 = seed-B）+ v174 `v174_xcu_candidates.csv` **1,199 行**（XCU 配列の完全未登録層）+ v155 の L7 起票（11 件）。
- **止まっている理由（H1–H7・v170 §6）**: ①catch 144 の adopt 昇格は Ghidra `Function.getBody()` 突合が必須 → **OblivionRe プロジェクトが現 Ghidra インスタンス（d1tamers_analysis）に不在**（v174 §2 実測）＝ headless 起動か人間の Ghidra 開き直しが必要。②帯域 10,572 の一括追記 = 台帳 21,597 → 約 32,169 への増加は Ghidra 再インポート運用の決定待ち。③Cat-3 mass 追記（10,261）は L7/S4 と併せての人間判断。④共有 FI（最大 253 関数共有）の同一性検証、⑤thunk 非登録 FI 6 件の宿主特定（未着手）、⑥md「333 FI」口径確定、⑦`FUN_` 自動名への役割接尾辞付与の可否。
- **推定工数**: 人間判断 → CSV 適用スクリプト 1 本 0.5–1 人日（Ghidra が動けば conditional 144 も同時解決）。

### ③ まだ突合していない組合せ（6 層台帳 / EH CSV / GT / docs の交差残り）【優先: 中】

| # | 未突合ペア | 内容 | 現状 |
|---|---|---|---|
| a | **4 統合 CSV の完全突合** | `v155_union`（31,905）× `v166_eh_entries`（10,742）× `v170_append`（10,870）× `v174_xcu`（1,199）の 4 者 VA 集合演算 | v174 §3-2 で XCU×(v155/v170) は済。**EH×XCU / EH×v170 差分（10,742 vs 10,870 の 128 行差）は未検証** |
| b | `v158_map.csv`（690 ペア）× GT §4.1 語彙 | field 名（29/35 field）を GT slot 表記へ正規化し、GT 未収録 slot の拾い漏れを検査 | 未実施（v158 Map と GT は別々に成長） |
| c | **s\* Copilot 系 × v/b 系の残未照合**（前版 F9） | v91 §8-7「s60 の 615 キーと件数差 20 の内訳」、G7/H6 節の訂正が s\* 側成果物/docs へ未反映、s57 との opcode 数値 | 未着手（個別案件のみ） |
| d | v161h:「data 参照のみ impl」362 件 × v133「本文なし 365」 | 残 3 件の内訳完全突合 | 未着手（v161 §3 起票のまま） |
| e | v161i: EH スコープテーブル同型クラスタ全数 | 0xAEC730/750/790 のみ確認・全体像未走査（v166 の FuncInfo 走査手法で代替可能） | 未着手 |
| f | U2 の 1 サイト差（1,819/1,818） | `call_targets.json` × 線形実測の decode 境界走査 | 未着手（§2.1） |

### ④ 残点として起票済みだが次ラウンドへ持ち越された束【優先: 中〜低（serial 162–164 は実行中のため除外）】

| 束 | 内容 | 出典 |
|---|---|---|
| opcode 実行時 第3束（**V147-r4/r5/r6 + V-172-r1..r5**） | 0x1E センチネル意味論 / impl false 時エラー文 / CTDA subject 判定 / executor フラグ配列仕様 / **FireEvent 時の Begin 本文実行主体** / 'X' 'n' 'z' タグ emit 元 / 0x45A500 述語 / type0x3D 実データ / slot+0x190 シグネチャ差 | v157 §12.9・v172 §13 |
| GT/docs 反映第 2 バッチ素材 | v171 §2/§3 ドラフト（GT §4.1.1 A–D 確定行 + docs §41.3 4 行）+ §4 残点 10 件（[+0x108] 語義 / [HighProcess+0xE4] / report L1568-1583 訂正候補 / b14 §1.1 転記不一致 / +0x26C 名称 / flags bit 語義 / V-27a 継続 / §41.2 コメント） | v171 §4 |
| quest/Script 小残点 | V-161d（Script+0x40 リスト意味論）/ V-161e（汎用 extra 関数の受信型最終排除）/ V-173a（FUN_004dc270 実クラス名）/ V-173b/c（0x7688D3 instance 同一性・0x53AD60 caller） | v161 §3・v173 §5 |
| gap 残点の「維持」判定群 | R-b（BA0002 634 成員）/ R-c（比較不能 2,771 本）/ R-d/R-e（L3 領域差 53/11 件）/ R-g（/OPT:ICF 永続）/ R-i（V148 未着手）/ R-j（v155 S4=L7 層化 11 件）/ N-b（v162 本文訂正 3 箇所）/ N-d（移植示唆更新） | v174 §6-3/§8 |
| read_ptr 防御の残 | Rs3（v90 未満スクリプト全数走査 = 範囲外）/ Rs4（memabs 6,210 未検証）/ Rs6（バグ版スクリプトの扱い） | v165 §6・v174 §7 |
| INI/GMST 残（ESM 解禁待ち） | v164 N1–N9 の実測パート（台帳 Freq 列定義・SCPT 外 15 種・CTDT 20B・DLC ESP 全走査・scriptType 分布）— **serial 163 が実行中のため起票は不要**（参照のみ） | v164 §7 |

### ⑤ ESM 領域（解禁待ち・仕様のみ整備済み）【優先: 解禁次第】

- v164 で走査仕様①–④（CTDA 生バイト / TrapUpdate 22 件 / SCDA 頻度 368 / SCHR 供給源）+ 解禁時コマンド仕様（walker 1 本で 5 出力 + 台帳 join + ゲート判定 P0）まで確定。**実走査・DLC ESP は未**（N5）。v173 が「ESM GMST は insert しない」を確定したため、走査時の検証仮説が 1 つ確定している。

---

## 4. おすすめ実行順（上位 5）

| # | タスク骨子 | 影響範囲 | 工数見積 |
|---|---|---|---|
| **1** | **台帳追記反映バッチ**（§3-②）: 人間承認を得て v170 CSV を adopt_priority 21 → adopt 10,572 → backlog 133 の順で func_list へ適用、v174 XCU 1,199 の採否併議。conditional 144 は Ghidra 動作確認後 | 台帳 21,597 → 約 32,169。以後の全 E8 帰属・enclosing 分析の精度が上がる。365 の Cat-3b「罠」が実害化するリスクを遮断 | 0.5–1 人日（承認後・Ghidra が止まっていれば headless 別途） |
| **2** | **GT/docs 反映第 2 バッチ**（§2.2 R2 + §5）: v171 ドラフト（GT §4.1.1 A–D 確定行 + docs §41.3 4 行）+ v166 R4（0x980EA4 docs 訂正）+ v164 N8（63,235/65,628 慣例差）+ v174 N-b（v162 訂正 3 箇所）+ v167 S7/S8 残件を一括適用。.bak 採番 → round_verify | GT 正本の確定性。以後のタスクが候補行（断定できない状態）を前提にするのを防ぐ。前版 F1 の完全閉鎖 | 0.5 人日（分析ゼロ・アンカー置換のみ） |
| **3** | **opcode 実行時 第 3 束**（§3-④ 上段 8 件）: FireEvent 本文実行主体（V-172-r1）を筆頭に、0x1E センチネル・subject 判定・フラグ配列仕様 | Android 移植の VM 実装仕様に直結（イベントハンドラ実行モデルが確定しないと Begin ブロックの移植ができない）。§67.7 残課題の裏側でもある | 1–1.5 人日 |
| **4** | **旧 letter-row 系 55 件の V 系束化起票**（§3-①）: テーマ別に Q 群（ESM 統合 8 件 = 統合ドキュメント化中心）/ O+N+P 群（エンジン+Havok）/ W3 群（Wave 3 深掘り 7 件）/ 移植差分群（J6/F7/F8/I7）へトリアージ。**v159/v173 の renderer/GMST 確定分と v158 のハーネス罠注意を出典として添付** | 前版以降無接触の最大未解析層。ESM 統合（Q1–Q8）は走査済み素材の統合のみで高効率 | 起票 0.5 人日 + 解析は束ごとに 0.5–2 人日（Q 群が最短） |
| **5** | **未突合組合せ一括**（§3-③ a/b/d/e/f）: 4 統合 CSV の VA 集合演算 1 本 + v158_map×GT 突合 + v161h/i + U2 の decode 境界走査 | 数値 GT の一貫性確保。台帳反映（#1）の事前検証も兼ねる | 0.5–1 人日（既存 CSV とスクリプト流用のみ） |

- 補助: ESM 解禁（serial 163 完了後）→ v164 仕様での走査 1 本 + 判定 0.5–1 人日。U3（a9d798 slot0/1）・V-27a 等の単発微小件は隙間時間に。

---

## 5. 残る人間ゲート一覧

| # | ゲート | 内容 | 承認後の実施者 |
|---|---|---|---|
| G-A | **台帳本体（func_list_new.txt）追記** | v170 CSV（10,870 行）+ v174 XCU 候補（1,199 行）の適用。H1（catch 144 昇格）/ H2（帯域一括追記の可否と Ghidra 再インポート運用）/ H3（Cat-3 mass）/ H4（共有 FI 同一性）/ H7（役割接尾辞）の判断 | 親セッション（承認後スクリプト） |
| G-B | **Ghidra 環境** | OblivionRe プロジェクトを稼働インスタンスへ接続（現 d1tamers_analysis のみ）。R-f 1,376 スパン突合（v174 N-c）/ conditional 144 / HavokContext 実 vtable（N2 = B13 実機ダンプ） | ユーザー（Ghidra 起動）+ サブエージェント |
| G-C | **GT/docs 編集（第 2 バッチ）** | GT §4.1.1 A–D 確定行（v171 §2）+ docs §41.3（v171 §3）+ §41.2 任意 2 行 + report L1568-1583（v171 残点 5）+ v166 R4 + v164 N8 + v174 N-b + v167 S7/S8 + b14 §1.1（v171 残点 6） | 親セッション（.bak → 置換 → round_verify） |
| G-D | **ESM/DLC 解禁** | serial 163 の仕様確定後、v164 の walker 実行（CTDA/SCDA/TrapUpdate/SCHR/GMST-8）+ DLC ESP（N5） | ユーザー承認 → サブエージェント |
| G-E | **バグ版スクリプトの扱い** | v165 Rs6: v140b/c/d/e/f2 の read_ptr バグ版を upstream 修正するか「訂正版へ誘導する注記のみ」か | 親セッション判断 |
| G-F | **実機確認系（旧 letter-row 由来）** | ContainerMenu 再オープン（G8/H7）、.ess サンプル取得（I6）、B13 ダンプ（N2） | ユーザーのみ |
| G-G | **本番 Supermemory sync** | ガード中・pilot 保留（前版からの継続） | ユーザー |

---

## 6. 成果物パス＋サイズ（読取対象の実在確認済み）

| パス（`C:\Users\hiroki.kogarumai\Oblivion_Android\tmp\re\` 以下） | サイズ (B) |
|---|---:|
| `v155_label_ledger_union.md`（+ 同名 CSV 1,507,687） | 27,000 |
| `v156_tail_band_identity.md` | 36,297 |
| `v157_runtime_residuals.md` | 53,236 |
| `v158_slot_opcode_map.md` | 48,921 |
| `v159_renderer_residuals.md` | 43,532 |
| `v160_ini_log_residuals.md` | 42,808 |
| `v161_quest_extra_residuals.md` | 79,379 |
| `v162_gap_residuals.md`（.bak_apply1 あり） | 42,517 |
| `v163_gt_slot_semantics.md` | 39,341 |
| `v164_esm_gate_spec.md` | 43,494 |
| `v165_v140_recompute.md` | 20,832 |
| `v166_eh_funclet_ledger.md` | 20,782 |
| `v167_v144_apply_log.md` | 20,988 |
| `v168_v143_apply_log.md` | 14,148 |
| `v169_v140_correction_log.md` | 6,678 |
| `v170_ledger_granularity.md` | 21,429 |
| `v171_gt_candidates.md` | 37,863 |
| `v172_runtime_residuals2.md` | 41,931 |
| `v173_small_residuals2.md` | 40,358 |
| `v174_gap_residuals2.md` | 26,724 |
| `v_data_relmap.md`（前版・読取のみ） | 33,235 |
| `v_review_batch1_20261027.md`（前回レビュー・読取のみ） | 4,730 |
| `TODO_re49.md`（読取のみ） | 149,286 |

- %TEMP%\opencode の主要 CSV（ヘッダ+行数実測）: `v155_union.csv`（31,905 行）/ `v166_eh_entries.csv`（10,742 行）/ `v170_ledger_append_candidates.csv`（10,870 行）/ `v174_xcu_candidates.csv`（1,199 行）/ `v174_r_f_noterm_spans.csv`（1,376 行）/ `v158_map.csv`（690 行）。
- 本タスクの生成物: 本ファイル（新規 1 ファイルのみ）+ `%TEMP%\opencode\vrelmap2_extract.py/.txt` / `vrelmap2_extract2.py/.txt`（抽出補助）。docs/・GT/・TODO/・台帳/・既存 tmp/re 成果物は無編集。ESM 再スキャン・Ghidra 再スキャンなし。

---

## 7. 付記: 1 行まとめ

> **反映断層（F1）は 18 件中 14 件が閉鎖し、住所統合（F7）・CC/slot⇔opcode（F4）・tail 帯域（F3）も解消した。次に効くのは「台帳追記の承認適用（素材 12,000 行超が積み上がり待ち）」と「2026-10-17 以来無接触の letter-row 系 55 件を V 系へ束化すること」— 後者は誰も触れていない最大の未解析層であり、起票欠如の是正だけで解消する。**
