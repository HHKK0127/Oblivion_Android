# v331 — #106 v327由来次束R（6残件）裁定

- 作成: 2026-11-09 / L1 report-only / 静的解析のみ
- 入力: `tmp/re/v327_v324_residuals_ruling.md`、`tmp/re/v324_crash_rootcause.md`、`docs/RE_EXE_REPORT.md`、`docs/Oblivion_decompiled.c`、既存 `tmp/re` 成果物、原本 EXE の読み取り専用確認
- 禁止事項: オリジナル EXE/ESM/.ess/解析原本、移植側ソースは変更していない。実行時プロセス操作、バイナリ書換え、Design C の適用は行っていない。
- 深解析境界: `0x4C0000-0x4E0000` は #106 専有帯として必要範囲のみ確認。帯外は既存成果物・docs・既存 census の照合、および新規のアドレス列挙に限定。
- 判定語: `[確定] / [部分確定] / [推定] / [未確定] / [確定負]`。観測・推論・未知を分離する。

## 1. 判定表

| 残件 | 判定 | 裁定 |
|---|---|---|
| R1 slot(1,1) 読みの全数走査 | **[部分確定]** | 既存の `FUN_00482150` 経由 census は完了し、定数 `(1,1)` は `0x49A77A` の1件のみ。新規の `0x4C0000-0x4E0000` 全文命令確認では `[reg+0x30]` 形を検出しなかった。ただし全帯域のインライン式を意味単位で完全閉包した証拠ではなく、残余ゼロは未確定。 |
| R2 FUN_007dc9a0系 / INI `DepthOffset` | **[部分確定]** | `DepthOffset` は `+0x30` constant table の slot 10、4要素バッファ `&DAT_00B45EE4` への登録まで確定。`FUN_007dc9a0` は同バッファ4要素を0初期化し `DAT_00B45DCC=this` を設定する ctor。map中心座標の書込系と同一バッファを共有するが、DepthOffset の実行時上書き値・優先順位は未確定。 |
| R3 実行時ゲート遷移 | **[未確定・#82委任]** | `[0xB33A98]`、`World+0x74`、cell state `{3,6}` の実発火順序は静的だけでは決着しない。#82 の動的経路・実行時観測へ委任し、本束では判定を更新しない。 |
| R4 NULLスロット直接経路 | **[未確定]** | 新規配列のNULL充填は既存証拠と整合。`FUN_00482170(obj=0)` は caller 側で `iVar2+0xC` を先に読むため通常の直接到達は困難という [推定] が得られる一方、vtable `+0x1C` の間接クリア caller と寄与率は未同定。 |
| R6 Design C callee側 `jecxz` 前提3件 | **[未確定・適用なし]** | ①`0x4C9A80`本体窓への外部分岐ターゲット全数、②`0x4C9AA0`双子への同型適用可否、③92/94 caller の戻り0安全性、の3件をL2前提として残す。実装・パッチ適用はしていない。 |
| R7 v324/v327 range数基準差 | **[確定]（差分の事実） / [未確定]（旧値の基準）** | v324の「814〜980」は概数・記述値、v327 parser の同一4 dump別 `MINIDUMP_MEMORY_LIST` range count は `792/814/813/952`。差は実測基準で再現したが、v324の旧値をどの stream/フィルタで数えたかは旧 probe 不在のため未確定。 |

## 2. R1 — slot(1,1) の読み経路

### 2.1 既存 census の確定部分

- v327 §2 の全 `.text` E8 census: `FUN_004C9A80` は **92 site / 56 caller**、`FUN_004C9AA0` は **94 site / 56+2 caller**（`v327_2.out`、`v327_2b.out`）。
- `FUN_00482150` 経由は X/Y とも3 site。X の定数 `(1,1)` は **`0x49A77A` の1 siteのみ**。直後は `0x49A77F mov esi,eax; 0x49A780 mov ecx,[esi]; 0x49A782 call 0x4C9A80` で、NULL guard はない（`v327_2c.out` 29, 43–50行相当）。Y は `0x49A79B` の同一 tail。
- `0x49A77A` の `FUN_00482150(1,1)` は `World+8` の5×5配列における offset `(5*1+1)*8 = 0x30` と一致し、既存 v327 §5 の `base=0x30009960 / ESI=0x30009990` 検算も成立。

### 2.2 #106専有帯の追加確認

- 原本 EXE `.text` の `0x4C0000-0x4E0000` を capstone で全文線形走査した。
- 同帯では `mov/lea/cmp` 等の一般的な `[reg+0x30]` 形は検出されず、少なくともこの形の slot(1,1) 直読み候補は **[確定負]**。ただし、`base + index*8 + 0x30` の算術合成、間接ポインタ、別命令分割を同じ意味へ正規化する全 `.text` data-flow 証明は今回の境界外であり、R1全体の残余ゼロとは言わない。

### 2.3 裁定

**[部分確定]**。クラッシュと同じ定数 `(1,1)` の window-indexer 経路は全数走査済みで、無ガード site は `0x49A77A` tail に閉じる。一方、v327 §9 が残した「FUN_00482150 非経由の直接アドレスコード」については、#106帯での bounded negative までであり、全 `.text` の意味正規化を完了したとは扱わない。

## 3. R2 — `FUN_007dc9a0` / `DepthOffset`

### 3.1 観測事実

- `docs/Oblivion_decompiled.c:697792-697811` の `FUN_007db4b0` は `param_1+0x30` の constant table を初期化し、`"DepthOffset"` を **constant slot 10**、型 `0x10000007`、4要素、出力先 `&DAT_00B45EE4` に登録する。
- 同じ `+0x30` table には `WorldViewProjTranspose`、`WorldTranspose`、`QPosAdjust` も登録される。したがって `DepthOffset` は単独のスカラーではなく、4 dword の shader constant（ベクトル相当）として扱われることが [確定]。
- `docs/Oblivion_decompiled.c:698716-698798` の `FUN_007dc9a0` は object ctor。`0xB45EE4/0xB45EE8/0xB45EEC/0xB45EF0` を順に0へ戻し、`DAT_00B45DCC = param_1` を設定する。これは v327 §4 の BSS初期値・書手 census と整合する。
- v327 §4 の EXE側 shallow evidence は、`FUN_007dc9a0` による `0xB45EE4..EF0` への dword write、`FUN_007dcbd0` の float write、`FUN_0049A200` tail の `0xB45EEC/EF0` write を区別している。従って「DepthOffset登録先」と「map中心座標の実行時書込」が同じ4 dword bufferを共有することは [部分確定] だが、用途が完全に同一だとは言えない。

### 3.2 未確定部分

- `FUN_007dc9a0` の ctor reset 後、どの call が `0xB45EE4..EF0` を最終値へ設定するか、`FUN_007dcbd0` の dispatch 条件、shader側で slot 10 がどの register 名へ対応するかは、現静的資料だけでは閉じない。
- よって「`DepthOffset` = cell座標そのもの」と断定しない。確定なのは **登録先・4要素・slot 10・reset writer** までである。

## 4. R3 — 実行時ゲート（#82委任）

- v327 §3 は、`FUN_00444340` の同期25-slot pass、`FUN_00444FB0` 後の `FUN_0049A200` 呼出し、WinMain idle/Sleep 経路を静的に確定した。
- しかし `[0xB33A98]==0` の early-exit、`[World+0x74]`、cell state `{3,6}` skip が実行時にどの順序・頻度で発火するかは heap と runtime state に依存する。
- 判定は **[未確定] のまま #82へ委任**。本束では runtime claim、移植側対策、ゲート変更案を追加しない。

## 5. R4 — NULLスロットの直接経路

- `FUN_004441A0` の slot投入が `0x4443EC` の1 E8 callerに閉じること、`FUN_00482200` の直接E8 callerが0件であることは v327 §3 の既存 [確定]。
- v327 §3-2 の `FUN_004441A0` は投入前に cell object の `iVar2+0xC` を読むため、`obj=0` を通常の直接引数として通ることは困難であり、これは [推定]。ただし vtable `+0x1C` の dispatch は間接で、全 caller が静的に列挙できたわけではない。
- **裁定 [未確定]**: NULLの主源を「新規配列の memset/NULL充填」とするのは整合的だが、clear/release経路との寄与比、vtable間接caller、再構築途中の状態は観測できない。

## 6. R6 — Design C の3前提（パッチ適用なし）

Design C は `FUN_004C9A80` の先頭で ECX==NULL を `xor eax,eax; ret` 側へ逃がす案だが、本束では**設計・適用とも行わない**。L2前提は次の3件。

1. **外部分岐**: `[0x4C9A80,0x4C9A95)` 内部の命令境界へ入る外部 branch target がないこと。既存 v327 は未実施。
2. **双子関数**: `FUN_004C9AA0` の同型 body、内部 branch、NULL時の戻り契約を別途確認すること。Xだけの変更では Yの非対称を残す。
3. **caller安全性**: X 92 caller / Y 94 caller が戻り0を有効な cell coordinate として安全に扱うかを個別にレビューすること。これは caller blast radius が大きく、静的に「全caller安全」とは推測しない。

現判定は **[未確定]**。L2承認なしの実装・バイナリ変更・移植側変更はしていない。

## 7. R7 — v324/v327 range 数の基準差

- v324 §1-3/§2 は Memory list を「**814〜980 range**」と記述し、v324 §1-3では「grid/World はその外」と判定した。
- v327 `v327_5b.out` の同一 parser 再検証は、dumpごとに次を出力した。

  | dump pid | `MINIDUMP_MEMORY_LIST` range count | EEC/EF0 | DCC |
  |---:|---:|---:|---:|
  | 28964 | 792 | 0 / 0 | `0x18900A80` |
  | 9936 | 814 | 0 / 0 | `0x18900A80` |
  | 39788 | 813 | 0 / 0 | `0x18900A80` |
  | 32488 | 952 | 0 / 0 | `0x1894FC4C` |

- v327 §5c は `MINIDUMP_MEMORY_DESCRIPTOR` のフィールド順 `[start u64][DataSize u32][Rva u32]` を再検証し、v322_9 と image globals が4/4一致した。従って `792/814/813/952` は同じ4 dumpを機械的に数えた値として [確定]。
- 旧「814〜980」は、最小値/最大値の概数、別 stream の件数、または旧 parser のフィルタ結果のいずれかを示す可能性はあるが、旧 probe の計数式が現成果物から復元できないため基準そのものは [未確定]。
- これは **数え上げ基準の差**であり、`0x30009960` ページが4 dumpすべて未採取という不在判定を覆さない。R7の材料は確定、旧値の由来のみ未確定とする。

## 8. 既存成果物・docsとの照合と差異

- v327 §9 R1/R2/R3/R4/R6/R7 の残件定義を維持し、R3を#82委任として明記した。R6は「3前提」のまま未確定、Design Cを適用していない。
- `docs/Oblivion_decompiled.c` は `FUN_007db4b0` の `DepthOffset` 登録（L697811）と `FUN_007dc9a0` の4 dword reset（L698789-792）を含む。`docs/RE_EXE_REPORT.md` は `FUN_007db4b0` の登録名（L1643）を記録するが、`DepthOffset` の slot 10・4要素・`&DAT_00B45EE4` の行は記録していない。この文書間の粒度差は docs側の補足候補であり、本束では既存 docs を編集しない。
- v324 の range 数表現「814〜980」と v327 の実測値は同一意味の単一値として統合せず、基準差として併記した。

## 9. 帯・変更・再現性

- 原本 EXE は読み取りのみ。`.ess`、ESM、解析原本、移植ソース、既存 `tmp/re/*.md` は変更なし。
- 新規補助 probe: `%TEMP%\opencode\v331_r1_scan.py` / `.out`。`.text` 全体の `[reg+0x30]` 形を列挙し、#106帯を別途確認した。#106帯の該当形は0件。解析成果物は本 Markdown のみ。
- Ghidra MCP は起動インスタンスなし（`list_instances` 結果）。既存 probe、EXE bytes、docsを用いた静的判定のみで、実行時観測を主張しない。

## 10. 判定内訳

- 主判定を5段階で数えると、**[確定] 0、[部分確定] 2（R1/R2）、[推定] 0、[未確定] 4（R3/R4/R6/R7の旧値由来部分）**。
- R7は「差分の存在・新parserの4値」を `[確定]`、「v324旧値の計数基準」を `[未確定]` とする複合判定。R3は **#82依存**、R6は **3前提未検証・実装なし**。
