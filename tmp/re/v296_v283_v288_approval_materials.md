# v296_v283_v288_approval_materials.md — ai-todo #47 承認材料（v283〜v288 正典・台帳追記候補 before→after ドラフト）

- 作成: 2026-11-05（L1 ドラフト専用サブエージェント・**本ファイルは適用しない**）
- 入力: `tmp/re/v283_v62_residuals4.md` / `v284_v61_headless_relabel.md` / `v285_v72_b333c4_residuals.md` / `v286_v75_annotation_residuals.md` / `v287_v76_data_map_residuals.md` / `v288_misc_residuals.md`（+ 参考 `v290_copilot_bundle_v2_proposal.md` §訂正候補）
- 台帳: ai-todo **#47**（TODO.md L26-27）。#51 照合済み・#48/#50 は移植側の別タスク

## 0. 実測基準（陳腐化リスク注記）

以下すべて 2026-11-05 現在 HEAD で**実測**（読み取りのみ）。`docs/RE_EXE_REPORT.md` は過去に +111 行シフトの実績（v284 §6-1）があるため、**適用セッションは行番号でなく引用文（before）の全文照合で再特定すること**。

| ファイル | 実測状態 |
|---|---|
| `docs/RE_EXE_REPORT.md` | 全 12,564 行。末尾 =「## 73. SetStage 0x1039 頻度カノン」(L12555–12564)。**§74 は未使用**（L12459/L12494/L12555 に 71/72/73 を確認） |
| `tmp/re/v5_ghidra_gaps_report.md` | 全 599 行。「## 9」なし・`V61` 語 0 件（v284 §6-1 と一致） |
| `tmp/re/v10_impl_decompile_join.csv` | 全 528 行（ヘッダ1+データ527）・14 列・LF・`#` 行 0・`v61_status` 列なし。`in_decompile`: no 362 / yes 165。`gap_kind`: ghidra_missing 260 / present 165 / absorbed_suspect 102 |
| `tmp/re/v62_v59_residuals.md` | 全 226 行。§6 未解決点 = L206–211（4項） |
| `tmp/re/v261_clas_service_flags.md` | 全 121 行。L59 に opid 誤記・L94 に fame/infamy 記述（v290 §訂正候補と一致） |
| `TODO.md` | #47 = L26–27（未クローズ） |
| バックアップ番号 | `.bakNNN` の実測最大使用値 = **128**。次 = bak129〜（§6 の手順雛形参照） |

## 1. サマリ

| 区分 | 件数 | 内訳 |
|---|---|---|
| **適用候補 [確定]** | **9** | docs(RE_EXE) 4（A1–A4）/ tmp/re md 3（B1・B2・D1）/ CSV 1（C1）/ 台帳 1（F1・TODO.md） |
| ドラフトのみ（本契約編集禁止） | 1 | F2（STATE.md＝ループセッションが通常運用で実施） |
| 適用不可・非推奨 | 2 | C2（CSV へ `#` コメント行＝互換性破壊で**不可**）/ C3（CSV `gap_kind` 書換え＝非推奨） |
| 材料として保留 [部分]/[推定] | 9 | H1–H9（§7） |
| 検証のみ・正典変更不要 | 6 | G1–G6（§6） |
| GT 追記候補 | 0 | v283〜v288 は GT ファイルを名指しせず（NiDX9Renderer 98スロット表は §74 経由で素材化・§7 注） |
| 参考（v290 §訂正候補・別承認） | 2 | E1（確定事実だが対象 v261 が既存 v2xx 成果物＝本契約編集禁止）/ E2（v295 確定待ちで保留） |

---

## 2. A — docs/RE_EXE_REPORT.md（追記 4 候補）

### A1. §70.10 未解決疑問点 — 解消注の挿行 [確定]

- **追記先**: §70.10「未解決疑問点（主要）」L12096–12098（L12098 が疑問点24項の単一行）。**L12098 の直後（現 L12099 位置）に1行挿入**
- **before**（L12098 冒頭・150字要約、実体は全文照合）:
  > attribute `s*Description` 8 件が ESM で置換されるか / `FUN_00478780` のオブジェクト型 / AV コード名テーブル `0xB0A1A8` の消費者 / `0x66C580` の caller 数 21 vs 22 / …（全24項の単一行）
- **after**（挿入案）:
  ```markdown
  → **[2026-11-05 v283–v288 で解消]**：s*Description ESM置換=あり（12件中10件・sSpeed/sRace は ESM 無し）[v283-1] / FUN_00478780 の this = ref+0x104 の非多形 0x154 B 装備アタッチ管理オブジェクト [v283-2] / 0xB0A1A8 消費者 = 直接3件+境界走査1件（`FUN_004FDE30` は「全AVダンプ」ではなく**コンパイラ引数パーサ**＝v62 訂正・opcode 表 +0x1C の 162/162）[v283-3] / 0x66C580 callers = 22 件確定（21 vs 22 解消）[v283-4] / バイナリのみ 195 関数 = decompiler の this エリジョン（未解析ではない）[v285-2] / フラグ `+0x10`・`+0x11` の意味 = flagA（参照必須）+ `+0x11`=0 未使用 + `+0x12` argn [v288-9] / `B43070=0/1` の意味 = bDoHighDynamicRange∧bDoImageSpaceEffects∧CheckDeviceFormat(SRGBREAD) [v287-3] / `0xB42754` = **NiDX9Renderer**（vtable `0xA88EA4` 98スロット全列挙）[v287-5] / §70.8-2 ハーネス2件 = 解消済（(a) V78・(b) V77）[v287-6] / `DAT_00B36510` の所有経路 = Topic_Resolve `FUN_0052F010`（6 site）+ 解放 `FUN_00530050`（3 site）[v286-7] / `0x446CB0`=Push（既載 L11840）・`0x46CB60`=リスト空判定（96 callers）役割確定 [v286-5]。**継続未決**: `+0x588` の全意味 / 0x804 型付き状態オブジェクトの正式クラス名・配列A/B/C 役割名（構造は確定）/ `0xB3CAC0`(C-7) / `FUN_007B4010` 埋め込みマップの帰属(C-8) / runtime count（静的不可能）/ GMST map teardown 順序 / decompiled L324692 の SSA store / アロケータ `0x401F00` / EVMP 516 外部検証。
  ```
- **出典**: v283 §1–4、v284 §1–5、v285 #2/#3/#6、v286 §5/§7、v287 §3/§5/§6、v288 §9
- **形式リスク**: 低（行挿入のみ）。**L12098 自体の in-place 書き換えは非推奨**（24項単一行・破損時リカバリが重い）
- **判定強度**: [確定] の解消分のみ記載（部分確定は「継続未決」側に回す）

### A2. §70.15 フォローアップ表 — 0xB42754 全スロット [確定]

- **追記先**: L12360（表行・§70.15）
- **before**:
  > `| 中 | `0xB42754` vtable 全スロット同定（+0x54/0xC8/0xD0/0xE0/0x104） | Renderer 節 |`
- **after（置換案）**:
  ```markdown
  | **済** | `0xB42754` vtable 全スロット同定（+0x54/0xC8/0xD0/0xE0/0x104） | **§74**（v287 §5：RTTI `.?AVNiDX9Renderer@@`・主 vtable `0xA88EA4` 98スロット全列挙・`+0x054`=フラグクエリ bit3/bit2 判定・埋め込み NiTMap vtable `0xA88A3C` 7スロット）|
  ```
- **出典**: v287 §5
- **形式リスク**: 低（1行置換。同表の「済」行 L12357 と同一書式）
- **判定強度**: [確定]

### A3. §70.8-2 ハーネス既知バグ — 解消マーカー [確定]

- **追記先**: L12074–12076（§70.8 item 2 の2小項）
- **before**（L12074）:
  > 2. **V76 ハーネスの既知バグ 2 件**（解析ツール側。doc の値には未波及と判断）。
- **after（item 2 見出し行の語尾に追記）**:
  > 2. **V76 ハーネスの既知バグ 2 件**（解析ツール側。doc の値には未波及と判断）。**→ [解消 2026-11-05] (a) `rd()` セクションシフト = V78、(b) `dec_index` ヘッダ正規表現 = V77（`^// (FUN_[0-9a-fA-F]+) @ 0x`・lowercased 20,465 件、実装コード読みで確認。v287 §6）**
- **出典**: v287 §6
- **形式リスク**: 低（インライン追記。既知バグの歴史的記録は削除しない）
- **判定強度**: [確定]

### A4. 新章「## 74. opencode v60v76-bundle 残件（v283〜v288 レポート・2026-11-04/05）」文末追記 [確定]

- **追記先**: L12564（§73 最終行）の**直後**へ文末追記。§71（L12459）/§72（L12494）/§73（L12555）の章様式（`(1) 到達点表`(2) 確定事項`証跡`(本節の新規確定)`(未確定)`）を踏襲
- **before**: なし（純粋な追記）。直前行 = L12564「(本節の新規確定): SetStage `0x1039` = 3,479 発生 / 4.00%…」
- **after 本文**: **付録 1 に全文ドラフト済み**（コピペ適用可）。§70.10/§70.15/§70.8 の注（A1–A3）と二重記載になる事実も「詳細は §74」という参照関係で整合
- **出典**: v283〜v288 全6本
- **形式リスク**: 低（追記のみ）。**番号衝突リスク**: 並行バンドル（v294/v295・v290 起案分）の後続が先に §74 を占有する可能性 → 適用時 HEAD 実測で採番し、内部参照（A2 after の「§74」表記）も追随させる
- **判定強度**: [確定] 事項のみ本文化、[部分]/[推定] は `(未確定)` 行に退避

---

## 3. B — tmp/re/v5_ghidra_gaps_report.md（追記 2 候補・v284 §6-2 判定）

### B1. 案A — §3 区分（L59–60）直後の1行追記 [確定]

- **追記先**: L60「デコンパイル完全欠落 (missing): 319 件」の直後（L61 空行位置に挿入）
- **before**（L58–60）:
  > \- **ギャップ (欠落)**: **362** 件 / - 前関数に吸収 (absorbed): 43 件 / - デコンパイル完全欠落 (missing): 319 件
- **after（挿入案）**:
  ```markdown
  - **[V61 再計測 2026-10-24・v284 で追記]** ギャップ 362 行は headless 再解析（21,240 関数）で全数 `NO_FUNCTION`（contained=0）。`prev_exists`=362/362 true（前関数は実在）→ 「前関数への吸収」機構説は否定され、43 件区分を含む全集合は「独立関数なのに Ghidra が FUN ラベルを生成しなかった」に再分類（詳細 §9）
  ```
- **出典**: v284 §1/§5/§6-2（案A「適用可・記載値に誤りなし」）
- **形式リスク**: 低
- **判定強度**: [確定]

### B2. 案B — 新章「## 9」文末追記（**改訂版**＝V70/V73/V79 反映）[確定]

- **追記先**: L599（フッター行）の直後。現状「## 9」なし（実測・陳腐化注記参照）
- **before**: なし（追記）。直前行 = L599「*Report generated by v5_ghidra_gaps_analysis. All hex evidence from direct binary reads.*」
- **after（改訂版ドラフト）**:
  ```markdown
  ## 9. V61/V70/V73/V79 の後続結論（2026-11-05 追記・v284 §6-2 改訂推奨に基づく）

  1. **prev_exists = 362/362 true**（V70 前 21,240・V70 後 21,597 の双方で）。`contained=0` は「前関数不在」ではなく「前関数本体がギャップを含まない」を意味。吸収説の機構主張は決定的否定（v284 §1）
  2. **21,240 の内訳**: `FUN_*` 20,473 / `__*`・`_*`（CRT）465 / その他特殊 152 / `thunk_FUN_*` 100 / scalar deleting dtor 50。V73 会計（decompiled 20,465＋非 FUN 775）と完全両立（v284 §2）
  3. **362(opid) = 357(VA) + 5**: 差は同一実装を共有するエイリアス opcode 2 組×5（OnEquip⇔OnActorEquip / OnUnequip⇔OnActorUnequip / AdvancePCSkill⇔ModPCSkill / ShowSubtitle⇔ShowDialogSubtitles / OutputMemContexts⇔ResetMemContexts）。運用規則＝集合比較 VA 357・台帳報告 opid 362（v284 §3）
  4. **isExecute 362/362 true**（`.text` Characteristics 0x60000020 全数実測）→ data 誤分割説は本集合に適用なし。「実行可能ブロックに関数 seed が無い」が確定（v284 §4）
  5. **cc2–cc3 59 件（57 ユニーク）の最終分類 = V5「欠落」が正**: 完全独立 51 / 薄片 5 / ガベージ 1。V70 で全数 create 成功（357/357）＋ V79 χ²=0.611 と重ねて決着。V10 の `absorbed_suspect` 102 件の原分類は据え置き、再分類情報は `v10_impl_decompile_join.csv` の **v61_status 列**で持つ（v284 §5・C1 参照）
  6. **関数総数の併記規則**（V73 維持）: `Ghidra解析世代=21,240 → V70後21,597` / `decompiled本文=20,465` / `dec_index=20,465`
  ```
- **出典**: v284 §1–5、§6-2（案B「適用可（改訂付き推奨）」。v284 の必須条件2「吸収説の弱化→仮説不成立＋357/357 作成成功に更新」を反映済み）
- **形式リスク**: 低（追記のみ・数値は v284 の実測値を転記）
- **判定強度**: [確定]
- （案C「区分表に V61 列」= 任意・適用可。B2 で代替されるため**保留**）

---

## 4. C — tmp/re/v10_impl_decompile_join.csv（追記 1 候補＋不可 1＋非推奨 1）

### C1. 案B — `v61_status` 列の末尾追加 [確定]

- **追記先**: ヘッダ行 L1 + データ 527 行の各行末。現在ヘッダ（14 列・実測）:
  - **before**（L1・134 字）:
    > `opid,name,impl,in_decompile,fun_label,decompile_line,cc_prefix,prologue16,prev_fun,prev_va,next_fun,next_va,gap_kind,note`
  - **after**:
    > `opid,name,impl,in_decompile,fun_label,decompile_line,cc_prefix,prologue16,prev_fun,prev_va,next_fun,next_va,gap_kind,note,v61_status`
  - データ行: `in_decompile=no` の **362 行**に `,NO_FUNCTION` を追記 / `in_decompile=yes` の **165 行**には `,""`（空欄・V61 未走査）→ 実務上は行末に `,""` を追記して 15 列に揃える
- **出典**: v284 §6-2（案B「適用可。362行 NO_FUNCTION / 165行 空欄の仕様は事実と一致」）
- **形式リスク**（**中**・適用時に遵守）:
  - 実測で**改行を含むフィールド 0 件・`#` 行 0・LF 統一**を確認 → 各行末尾への**生文字列追記**が安全。`csv` モジュールでの round-trip 再書出しは引用符スタイルが全行に波及する可能性があり回避する
  - 14 列固定・列索引（特に最終列 `note`）を前提とする下流スクリプトがあれば影響 → `note` は 14 列目（後方互換）、新列は末尾 15 列目なので索引参照は不変。名前列参照（`DictReader`）は互換
- **判定強度**: [確定]（内容）、運用ルールは v284 §3（opid 362 単位で台帳報告）

### C2. 案A — `#v61_relabel` コメント行追記 → **適用不可**

- **理由**: `#` 開始行は `csv.DictReader` にデータ行として読まれ 528→529 行となり後続解析を破壊する（v284 §6-2・v61 自身の留保）。**列追加（C1）で代替**。件数: 本ドラフトで唯一の明確「不可」判定

### C3. `gap_kind` 列の in-place 書換え（absorbed_suspect→ghidra_missing）→ **非推奨**

- 動機は v284 §5（59 件の再分類）だが、`gap_kind` の原分類は V10 台帳の一次記録であり、書き換えると V5/V10/V61/V79 の会計追跡（例: `ghidra_missing` 260 / `present` 165 / `absorbed_suspect` 102 の HEAD 実測）が失われる。**C1 の追加列で上書き表現とする**（B2 §9-5 と連動）

---

## 5. D / E / F — 一次資料注記・参考・台帳

### D1. tmp/re/v62_v59_residuals.md — §6 解消注の追記 [確定]

- **追記先**: §6 未解決点（L206–211）の末尾・L211（4項）の直後、L213 `---` の前
- **before**（L208・150字要約）:
  > 属性8件 `s*Description` の placeholder が **ESM GMST 読込時（V55 の key→ptr 上書き機構）で実行時に差し替えられるか**は本タスク範囲外（…ESM 側の該当 GMST 有無は未検証）
- **after（挿入案）**:
  ```markdown
  ### 6.5 解消状況（2026-11-05・v283 / v296 承認追記）
  - §6-1 → **[解決・確定]** ESM 置換=あり（12件中10件実在・文字列型も `FUN_004A7D60` type6 ブランチでコピー転送）。`sSpeedDescription`/`sRaceDescription` は ESM 不在→EXE 既定維持。綴り違い `sIntellegence`/`sWillPower` は**両側同一綴り**でヒット（v283 項目1）
  - §6-2 → **[解決・確定]** this は HighProcess でも TESObjectREFR 本体でもない。参照の `+0x104` に遅延生成される非多形 0x154 B の装備アタッチ/スケルトンボーン管理オブジェクト（vtable なし・`+0x150`=ref 双方向リンク）（v283 項目2）
  - §6-3 → **[解決・確定＋本節の誤り訂正]** 消費者=直接3件（`0x4F4610`/`0x4F60BC` のデバッグログ・`0x4FEA26`）＋境界走査1件（`FUN_004FCA70`）。「全AVダンプ」は stricmp 走査（0..71）の誤認で、`FUN_004FDE30` は**スクリプトコンパイラの汎用引数パーサ**（opcode 表 base `0xB0AF48` stride 0x28 の `+0x1C`=162/162）。正式呼び出し元 = `FUN_00501A00`/`FUN_00501670`/テーブル間接（v283 項目3）
  - （関連）`0x66C580` callers = **22 件全数分類**（A群12+B群10・linked-list 誤認残滓なし）（v283 項目4 / v288 §8）
  ```
- **出典**: v283 §1–4、v288 §8
- **形式リスク**: 低（章追記・既存4項は無編集）
- **判定強度**: [確定]。注: v62 は tmp/re 一次資料（v2xx 成果物ではない）だが、**本サブエージェント契約では読み取りのみ**（適用は承認後セッション）

### E1. tmp/re/v261_clas_service_flags.md L59 opid 誤記 → **参考・別承認**

- **before**（L59・150字要約）:
  > 3. **財布額**: `FUN_005FAA70`（GetBarterGold 実装本体、@0x4F5A80 経由 opid 0x10F8）: `word[base+0x30] + player 効果分(0x41E980@actor+0x44, 0x5F1910(0x1D)=…
- **after 案**（L59 直下に注1行）:
  > `> 訂正注記（v290 §訂正候補1）: GetBarterGold の正 opid は **0x1108**（index 427）。0x10F8 は modpcfame。impl 0x505D10 / cond 0x4F5A80 / 本体 FUN_005FAA70 は正しい`
- **出典**: v290 §訂正候補 1（事実自体は [確定]）
- **適用可否**: **本バンドルでは適用しない** — 対象が既存 v2xx 成果物で本契約の編集禁止対象。人間が別途適用範囲を明示した場合のみ
### E2. v261 L59/L94「fame/infamy 系（0x5F1910@0x1D）」注記 → **保留**

- v290 §訂正候補 2。v279/v280 の「0x1D = Mercantile AV」確定と要突合で、**v295（番号予約済・未産出）の再検証結果待ち**。現状 L94 の「fame/infamy 系」表記は未修正のまま保留

### F1. TODO.md（正本台帳）#47 注 — 適用完了記録テンプレ

- **追記先**: L26–27 の #47 注ブロック直下
- **before**（L27・150字）:
  > `#47`「v283〜v288のRE正典・台帳追記（承認待ち）」注: `docs/RE_EXE_REPORT.md`、v5 gaps report、v10 join CSV等への追記候補。v284でCSV互換性を壊すコメント行追加は行わず…
- **after**（適用後に追記する行の雛形・角括弧は実測値へ）:
  ```markdown
  > [2026-11-05 適用] v296 材料で A1–A4（RE_EXE §74 新設ほか）/ B1–B2（v5 §9）/ C1（CSV v61_status 列）/ D1（v62 §6.5）を適用。bak[129–133] 使用。C2（#行）不可・C3 非推奨で未実施。H1–H9 保留、E1/E2 は別承認。詳細は tmp/re/v296_…md と §74
  ```
- **形式リスク**: ai-todo 経由なら `complete_task` + 直接編集なら `accept_tamper`（AGENTS.md の台帳運用）

### F2. STATE.md — **編集禁止のためドラフト行のみ**（適用セッションの通常運用に委譲）

- before（L3・150字要約）:
  > `Last run: 2026-11-05 (a) — RE loop: V261-V277 landed (v268-v286), V277 done in-session after dispatch outages; …`
- after 案（適用セッションが次走時に Last run を更新）:
  > `Last run: 2026-11-05 (b) — v296 承認材料に基づく v283–v288 正典追記適用（RE_EXE 新§74・§70.8/10/15 注・v5 §9・v10 CSV v61_status・v62 §6.5・bak129〜）。STATE.md 追記はループ通常運用で実施`

---

## 6. G — 検証のみ・正典変更不要（確認した6件）

| ID | 内容 | 検証結果 | 出典 |
|---|---|---|---|
| G1 | RE_EXE §67.6.1 の v61 案A/B | **既適用**。現行 L11526–11530 に同一文面を確認（案由来番 +111 シフト）。追加適用不要（将来注は H9） | v284 §6-1 |
| G2 | band-6 vs total-7（§68） | **原典 L11602 が両方正しく変更不要**（7=新規 opcode・6=新規 FunctionID、StopQuest は既存 0x007A）。ROADMAP L256・STATE.md L87 とも整合 | v286 §2 |
| G3 | Push `0x446CB0` の名前欠落 | 正典 §69.1 L11840 に「347 callers 汎用リスト挿入」と記載済み。欠落は annotate_db のみ → §74 で補記、正典表は不変 | v286 §5 |
| G4 | v60(a) startup-init 走査コード | `0x981C6B` は V93 経由で正典化済み。**v60 が探した境界値（0xA28860/0xA2D184）が誤り**だった事実のみ §74 に記録 | v288 §1 |
| G5 | v69#2（0x66C580 callers） | v283 項目4 へ統合済み（成果物 v288 §8）。単独追記なし | v288 §8 |
| G6 | v63#1 の正式 FUN 名 | decompiled に定義なし → 正式 `FUN_*` 付与不可。命名案 `MapCompare_StrICase` は §74 でのみ記載可 | v288 §3 |

## 7. H — 材料として保留（[部分確定]/[推定] — 適用候補に含めない）

| ID | 内容 | 強度 | 出典 |
|---|---|---|---|
| H1 | 帯 `+0x200..+0x56C` の配列A/B/C 役割名（base/current/modifier 等）・idx8/9/10 リマップ理由。構造（float 72×3＋特殊3・getter/setter）は確定で §74 に載る | 構造[確定]/意味[推定] | v285 #3 |
| H2 | `+0x7A4..` 係数表の用途「`+0x130` 配列との正規化」までの意味づけ・`0x7FA/7FB` 未初期化・`+0x7FC/800` float の用途 | [部分] | v285 #6 |
| H3 | 132/39 = Supermemory 旧エントリ bUP3gq…（forget 済・手順喪失）/ 124/37 = 一次出典喪失。結論「両数値使用不可・v75a A=344/126 が正」は §74 (2) に記録可 | [部分] | v286 §1 |
| H4 | `fun_00530020` の arg1/arg2 正式意味名（Topic_Resolve 側フィールド名依存） | [部分] | v286 §6 |
| H5 | C-7 `0xB3CAC0` の所属クラス・初期化元（identity 番兵までは支持） | [部分] | v287 §1 |
| H6 | C-8 `FUN_007b4010` 埋め込みマップ所有オブジェクト（`param_4`）のクラス名 | [部分] | v287 §2 |
| H7 | v63#4 起動直後 runtime count の厳密値（静的不可能。近似 ≤2,175〜2,176） | [部分] | v288 §5 |
| H8 | v283 残点4件: owner（0x154 B）クラス正式名 / formType 0x21/0x22/0x1A クラス名 / opcode `+0x1C` 残り366エントリのテーブル境界 / `FUN_0066C580` の vcall `+0x198`・float 群の意味 | [推定/未了] | v283 残点 |
| H9 | RE_EXE §67.6.1 の L11528/11530（V61 時点記述）に V70/V73/V79 後続結果が未反映。v284 §6-2 は「将来の追記候補」として in-place 編集を推奨せず → **新 §74 の追記で代替**（A4）。§70.10 への注（A1）とも整合 | 判断=保留 | v284 §6-2 |

※ NiDX9Renderer 98スロットの完全表（v287 §5-2）は GT/台帳素材として価値があるが、v287 は追記先を名指ししていない。適用時は §74 に「全表は v287 §5-2 参照」のポインタのみ置き、表本体の転記是否は人間判断に委ねる。

---

## 8. 適用手順の雛形（承認後セッション用・**本ドラフトでは実行しない**）

前提: ユーザー承認済み。ai-todo #47 を `start_task`。**全行番号は適用直前に再実測**（§0 の陳腐化注）。

1. **バックアップ**（連番は実測最大 128 の次＝bak129〜。衝突時は空番号へ）:
   ```
   copy /Y docs\RE_EXE_REPORT.md                 docs\RE_EXE_REPORT.md.bak129
   copy /Y tmp\re\v5_ghidra_gaps_report.md       tmp\re\v5_ghidra_gaps_report.md.bak130
   copy /Y tmp\re\v10_impl_decompile_join.csv    tmp\re\v10_impl_decompile_join.csv.bak131
   copy /Y tmp\re\v62_v59_residuals.md           tmp\re\v62_v59_residuals.md.bak132
   copy /Y TODO.md                               TODO.md.bak133
   ```
2. **RE_EXE_REPORT.md は下から適用**（先行編集が行番号を潰さない順）: A4（文末 §74 追記）→ A2（L12360 行置換）→ A1（L12098 の直後挿入）→ A3（L12074 語尾追記）。A2 after の「§74」参照は実際の採番に合わせる
3. **v5_ghidra_gaps_report.md**: B2（文末 `## 9`）→ B1（L60 直後挿入）の順（同様に下から）
4. **v10 CSV（C1）**: 生行追記方式で実施（round-trip 再書出し禁止・C2/C3 は行わない）。例:
   ```python
   rows = open(p, newline='').read().split('\n')  # LF・528要素想定
   header = rows[0] + ',v61_status'
   for r in data_rows: append ',NO_FUNCTION' if in_decompile=='no' else ',""'
   # 検証: 行数528不変・全行15列・#行0・gap_kind集計不変（260/165/102）
   ```
5. **v62（D1）**: §6 末尾に 6.5 章を挿入
6. **TODO.md（F1）**: #47 注に適用完了行。ai-todo 経由なら `complete_task` → supermemory に完了記録（方式: 要点+証拠+簡潔タグ）
7. **STATE.md（F2）**: ループセッションの通常運用（「Update STATE.md after every loop run」）で更新。本テンプレの適用手順では触らない
8. **事後検証**: `python -c` で行数照合（RE_EXE 12,564 → +追記行数）、grep マーカー（`## 74`・`6.5 解消状況`・`v61_status`・`[2026-11-05 v283–v288 で解消]`）、CSV は `csv.DictReader` で 15 列×527 行ラウンドトリップ、`git diff --stat` を人間に提示（**コミットは別承認**）。

---

## 付録 1 — 新 §74 本文ドラフト（A4 の after 全文・コピペ適用可）

※ 見出し番号は適用時 HEAD で再採番（§74 占有時は §75 ほかへ）。

```markdown
## 74. opencode v60v76-bundle 残件（v283〜v288 レポート・2026-11-04/05）

依頼バンドル `tmp/re/handoff_20261105_v60v76_residuals_bundle.md`（依頼1〜6・P1>P2>P3>P4>P5>P6）の成果6本
（`tmp/re/v283_v62_residuals4.md`〜`v288_misc_residuals.md`）による追記統合。全セッション harness selftest 4/4 PASS。
承認材料＝`tmp/re/v296_v283_v288_approval_materials.md`（#47 適用記録と連番bak対応）。

### (1) 依頼の到達点

| 依頼 | 主題 | レポート | 判定 |
|---|---|---|---|
| 1 [P1] | v62 残件4点 | `v283_v62_residuals4.md` | **確定×4**（4/4） |
| 2 [P2] | v61 headless relabel 残件6点 | `v284_v61_headless_relabel.md` | 確定×5＋部分×1（§5 relabels 適用判断） |
| 3 [P3] | v72 0xB333C4 マップ残件5点 | `v285_v72_b333c4_residuals.md` | 確定×3＋部分×2 |
| 4 [P4] | v75a/v75b アノテーション・来歴7点 | `v286_v75_annotation_residuals.md` | 確定×5＋部分×2 |
| 5 [P5] | v76 データ関係マップ残件6点 | `v287_v76_data_map_residuals.md` | 確定×4＋部分×2 |
| 6 [P6] | 小物9点（v60/v63/v64/v69） | `v288_misc_residuals.md` | 確定×7＋部分×1＋統合×1（v283 へ） |

### (2) 確定事項（正典新規）

- **v283**: `s*Description` 12件（EXE 中央マップ `0xB35574` 登録）のうち **10件が ESM に実在・文字列型も転送**
  （`FUN_004A7D60` type6 ブランチ。`sSpeedDescription`/`sRaceDescription` 不在）。綴り違い `sIntellegence`/`sWillPower` は両側一致。
  `FUN_00478780` の this = ref+0x104 の**非多形 0x154 B 装備アタッチ管理オブジェクト**（`new(0x154)` 3箇所・`+0x150`=ref・
  Bone テーブル9点・vtable なし）。`0xB0A1A8`（72件）消費者 = 直接3件＋境界走査1件。**訂正**: `FUN_004FDE30` は全AVダンプでなく
  コンパイラ引数パーサ（opcode 表 `0xB0AF48`・stride 0x28・`+0x1C`=162/162＝全体528エントリ）。`0x66C580` callers **22件全数**
  （A群12＝ecx `[0xB333C4]` / B群10＝this 引き渡し・呼出パターン5型）。
- **v284**: `prev_exists`=**362/362 true**（吸収機構説を決定否定）。21,240 内訳 = FUN_* 20,473/CRT 465/特殊152/thunk 100/dtor 50
  （V73 会計 20,465+775 と完全両立・`FUN_*` 差8と thunk 差13は名寄せ方式差）。362(opid)=357(VA)+5（エイリアス opcode 5組）。
  isExecute 362/362（`.text` 0x60000020）。cc2-3 59行（57U）最終分類=**V5「欠落」が正**（独立51/薄片5/ガベージ1）。
  relabels 適用判断: v5 案A/B 適用可（案B は V70/V73/V79 併記に改訂）・**v10 案A（#行）不可**／案B（列追加）適用可。
- **v285**: binary-only 195 関数=**100% this エリジョン**（分類 B195/A0/C0）。`FUN_006600D0`(38)/`FUN_00660110`(79) の this=**常に `[0xB333C4]`**
  （分岐解析で境界2件解消）。帯 `+0x200..+0x56C` 構造確定 = **float 72×3 配列＋特殊 float 3（+0x444/448/44C）**・getter `FUN_0065D270`/
  setter `FUN_0065D310`（`FUN_0065BC70` 経由）。`+0x7A4..+0x7F4` = **formType 0x0C..0x20（21種）型別係数表**（索引 `FUN_00565C50(2,formType)`・
  `+0x130` 配列との比算に使用）。`+0x7F8` byte フラグ write-only（`0x5B5EF1`）。近傍 bss 230本 = 同定177/**未同定53**（完全リスト v285 §#5・
  `FUN_00445A10`/`FUN_009D9FA0` 集中・移植時は binary アドレス直接使用）。
- **v286**: ESM 全体再走査（`v286_p4c.py`・新規独立証跡）で **QUST 388/INFO 1,700 を 9/9 全一致で証跡化**。新規確定
  **QUST 命令 10,326 / INFO 命令 10,962**。50=SetStage 最終命令の SCPT 数・987=原文センテンス数・**220=opcode 表エントリ通し番号**
  （`(opid&0xFF)+163`）。annotate_db 名前欠落5件確定: `0x4526E0`=リストクリア（240 callers）/`0x446CB0`=Push（正典 L11840 既載）/
  `0x46CB60`=リスト空判定（96）/`0x530020`=Topic_Resolve 薄ラッパ（`ret 8`・caller `FUN_006B8E00`）/`0x50A120`=**ScriptEffectUpdate 実装**
  （.data テーブル `0xB0B240`・opid 0x13・predicate 型）。`DAT_00B36510` ユーザー = Topic_Resolve 6 site＋解放 `FUN_00530050` 3 site（正典 NO HIT 解消）。
  band-6/total-7 は §68 原典どおり両正（G2）。
- **v287**: `B43070` = **bDoHighDynamicRange:BlurShaderHDR ∧ bDoImageSpaceEffects:Display ∧ CheckDeviceFormat(0x16,0x80000=SRGBREAD,0x71) 成否**
  （`FUN_00498340`・鍵は静的 .data Setting `0xB06DE4`/`0xB06CC4` 既定1）。teardown `FUN_007B7250` = E8 caller 1件＝CRT 初期化テーブルスタブ
  `0xA10EB0`（`u32@0xA2EFF0` 実測・起動時1回・atexit 先は空 `ret`）。**`0xB42754` = NiDX9Renderer**（RTTI `.?AVNiDX9Renderer@@`・
  `new(0xB00)`・主 vtable `0xA88EA4` **98スロット全列挙**・`+0x054`=フラグクエリ・埋め込み NiTMap vtable `0xA88A3C` 7スロット）。
  ハーネス: (a) 解消済 V78 / (b) 解消済 V77（dec_index 20,465・lowercased）。entry162 区分確定: idx0..30 イベント（0x0000–0x001E 31件）/
  idx31..162 console（0x0100–0x0183 132件・末尾マーカー）/ idx163..531 スクリプト関数（0x1000–0x1170 369件）＝**帯分離完全**（31+132+369=532）。
- **v288**: v60(a) 走査コード=`0x981C6B` インラインループ（abs32 境界 2/2 実測・v60 の探索アドレス誤り）。v60(c) first-match 機構完全再現
  （0xA5D55D 部分文字列→abs32 0件・独立文字列 **0xA5E194**・push imm32 `0x9F0D56`＝4非整列）。`0x584D10` = stricmp==0 ラッパ
  （vtable `0xA40454`+0x08・命名案 `MapCompare_StrICase`・decompiled 定義不在が NO HIT 理由）。`0xA3BF1C`/`0xA53920`/`0xA64800` =
  **3つとも vtable**（install: `FUN_0046C430`＋グローバル `0xB06164` 直書 / `FUN_00521450`（14 callers）/ `FUN_00551370` 系）。
  `sRHand:DEFAULT`（slot `0xB11B94`）既定 = vanilla タイポ `"1.0f, 1.0"`。読手 `FUN_0053A720` の `sscanf("%f, %f")` は第1値まで読めた所で
  中断→第2値は既定 1.0（結果偶然正しい。移植時は `"1.0, 1.0"` 正規化推奨＝移植側課題）。EH `0x9AB578` ファミリ5件に **string 型 runtime
  登録 thunk 不在**（float/dword のみ）。opcode entry+0x10 = {flagA u8=**参照必須**（V99）・+0x11=0・argn u16@+0x12}、サブテーブル
  （`0xB0C8C0` 369件: flagA 1=235/0=134・argn 0:166/1:139/2:53/3:5/4:3/5:3）。

証跡（Git 管理外）: `tmp/re/v283_*`〜`v288_*` と `%TEMP%\opencode\v283_p1*.py`〜`v288_p6c.py`（selftest 4/4 PASS・内部整合は各レポート末尾）。

(本節の新規確定): s*Description ESM 10/12・FUN_00478780 の this・0xB0A1A8 消費者＋FUN_004FDE30 再同定・0x66C580 callers 22・
prev_exists 362/362・21,240 内訳・362=357+5（alias 5組）・isExecute 362/362・cc2-3 最終分類・195 エリジョン・this 100% DAT・
帯構造 3×72 float＋72×3 型別係数表（+0x7A4）・bss 未同定53リスト・QUST 10,326/INFO 10,962（9/9 再検証）・50/987/220 出所・
名前欠落5件（ScriptEffectUpdate 0x50A120 含む）・DAT_00B36510 消費者・B43070 判定式・teardown 起動経路・NiDX9Renderer vtable 98・
entry162 三分離・first-match 再現・0x584D10／vtable 3件・sRHand タイポ・string thunk 不在・flagA/argn 分布。
(未確定): 配列A/B/C 役割名・idx8/9/10 リマップ理由・+0x7FC/800 と 0x7FA/7FB・0xB3CAC0 の帰属（C-7）・FUN_007b4010 の param_4 クラス（C-8）・
runtime count 厳密値（静的不可能・近似≤2,175〜6）・owner 0x154B と formType 0x21/22/1A の正式名・opcode +0x1C 残り366エントリの境界・
`FUN_0066C580` vcall+0x198 と時間系 float・fun_00530020 引数意味名・124/37 の来歴（喪失）。
```

---

*了 — 本ファイルのみ書き込み。docs/・既存 v2xx・TODO_re49.md・STATE.md への適用は一切行っていません。*
