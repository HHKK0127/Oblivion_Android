# V96: 無名 gap 新实例と Copilot 全数列挙台帳の突合

- 作成: 2026-10-25 / タスク: V96 照合（優先度: 低）/ 前提: `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**。
- 規約遵守: `rd()` はセクション対応（.text const 0x400C00 / .rdata 0x400E00 / .data 0x401400 / bss=None）、`dec_index` = **20,465 件**、`enclosing_fun()` 相当の header-bisect で VA→ラベル。
- 対象 EXE: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（手動パース、7,898,624 B）。Copilot 台帳の解析対象 `OblivionStandalone.exe` と **SHA256 完全一致**（`A8F313845C1545E9A60E1E995961EEF4C033115DA9443F6D756341DF3C2B7DC6`）＝同一バイナリのため台帳との突合は有効。
- 編集範囲: 既存ファイルは一切読取のみ。成果物は本新規 1 ファイルのみ（スクリプトは `%TEMP%\opencode\v96_gap_xcheck.py`）。git / memory 追加 / sync ツールは未実行。

## (a) Copilot 台帳の特定

TODO_re49.md 行 84「V85〜V89(Copilot): FUN ラベルなし関数の全数列挙（ret+int3 境界検出）| **[x] 完了 (2026-10-25)**」に対応する成果物は単一ファイルではなく、**ground truth 1 件 + 系列成果物**で構成される。件数・列構成は下表のとおり。

| 役割 | パス | サイズ(B) | 件数 / 列構成 |
|---|---|---:|---|
| ground truth: Ghidra FunctionManager 台帳 | `%TEMP%\opencode\func_list_new.txt` | 477294 | **21,597 行** / 列 2 = `<addr>,<name>`（例 `00401000,FUN_00401000`） |
| V85 prologue 走査（選別力不足で不採用） | `tmp/re/v85_unlabeled_scan.txt` | 919300 | 候補 18,947（E8 10,401 / 非 E8 8,546）/ 列 = `- 0xADDR pad= prev_ret= strict\|loose` |
| **V86 本体 = UNLABELED 全数 10,175** | `tmp/re/v86_unlabeled_scan.txt` | 413267 | A( E8 呼出し先 ) **214** / B( imm32 参照のみ ) **9,809** / C( 無参照 ) **152** / 列: A 節 `- 0xADDR pad= insns= first= prev_term=`、B・C 節 `- 0xADDR refs= insns= first=` |
| V87 位置分類 | `tmp/re/v87_region_class.txt` | 738941 | gap 10,171 / interior 4 / beyond 0 / 列 = `- 0xADDR <V86 の節名>` |
| V88 caller 証跡（SHOW=15 のみ） | `tmp/re/v88_e8_called_evidence.txt` | 1990 | サンプル 15 件 / 列 = `prev= next= body= insns= callers=` |
| V92 triage（A 節全 214） | `tmp/re/v92_e8_214_triage.txt` | 19616 | **214 行** / 列 = `candidate \| prev start \| next start \| body \| insns \| end \| pad \| callers` |
| V95b 形状再現率（集計のみ・アドレス列なし） | `tmp/re/v95_recall_gap.txt` | 1398 | 9 行 / 実数は L17-L25（現行規則 19,395/21,597 = 89.8%） |
| V96 規則非依存 E8 全列挙 | `tmp/re/v96_e8_candidates.txt` | 159306 | E8 ターゲット 14,186 / 台帳外 565 / 「all rows」**565 行** / 列 = `0xADDR CLASS padclass accept insns= sites= good= ext= at= fall= host= \| asm \| sites` |
| 正典叙述 | `docs/RE_EXE_REPORT.md` §70.16 | 1026048 | (1) 手法 4 手順 / (2) A 節 214 / (3) B 節 9,809 / (4) 限界 / (4b) 派生 B / (5) 残作業 |

**判定: 台帳は特定できた。** 本突合では ground truth 21,597 + V86 10,175 + V85 18,947 + V87 + V92 214 + V96 565 を独立ロードし、各实例を 6 重で照合した。

## (b) 判定表

列: `dec_index` = decompiled ヘッダ `// FUN_xxx @ 0xVA` に **entry の VA** が存在するか / `台帳` = `func_list_new.txt` 21,597 開始点 / `V86` = UNLABELED 節 / `境界` = entry 直前のパディングと終端 / `判定` = Copilot 全数列挙（V86）に対する収録結果。

| # | v81 記載 | 実測 entry | dec_index | 台帳21,597 | V86 | V85 | V92 | V96 | 直前境界 | accept(600) | 判定 |
|---|---|---|---|---|---|---|---|---|---|---|---|
| I1 | `0xA10EB0`（call at 0xA10EB5） | `0x00A10EB0` | **無** | **無** | **B** | 無 | 無 | 無 | 12×`CC` + ret | ok | **カバー済** |
| I2 | `0x40F5xx`（gap [0x40DF74, 0x40F640) 内） | `0x0040DF80` | **無** | **無** | **A** | 有 | 有 | 有 | 12×`CC` + ret | ok | **カバー済** |
| I3 | `0x437600`（call at 0x437658） | `0x00437600` | **無** | **無** | **B** | 有 | 無 | 無 | 1×`CC` + ret | ok | **カバー済** |
| I4 | `0x656C82..`（sites 0x656F80 / 0x656FAF） | `0x00656C90` | **無** | **無** | **B** | 有 | 無 | 無 | 14×`CC` + ret imm16 (c2 iw) | ok | **カバー済** |
| I5 | `0x6710A5..`（site 0x6710DA） | `0x006710B0` | **無** | **無** | **B** | 有 | 無 | 無 | 11×`CC` + ret | ok | **カバー済** |
| I6 | `0x7B14BE`（FUN_007b13a0 ret@0x7B14BA + 3CC の後） | `0x007B14C0` | **無** | **無** | **B** | 有 | 無 | 無 | 3×`CC` + ret imm16 (c2 iw) | ok | **カバー済** |
| I7 | `0x7C53B0`（FUN_007c5340 `ret 8` @0x7C53AB の後） | `0x007C53B0` | **無** | **無** | **B** | 無 | 無 | 無 | 2×`CC` + ret imm16 (c2 iw) | ok | **カバー済** |
| I8 | `0x40543A..`（FUN_004053e0 ret@0x405439 の後） | `0x00405440` | **無** | **無** | **B** | 有 | 無 | 無 | 6×`CC` + ret | ok | **カバー済** |
| I9 | `0x6A6A9B..` | `0x006A6AA0` | **無** | **無** | **B** | 無 | 無 | 無 | 5×`CC` + ret imm16 (c2 iw) | ok | **カバー済** |
| I10 | `0x6A6A9B..` 系の第 2 読手（v81 本文 0x6A6D39） | `0x006A6D10` | **無** | **無** | **B** | 有 | 無 | 無 | 8×`CC` + ret imm16 (c2 iw) | ok | **カバー済** |

**件数**: 実測 entry 10/10 ・ dec_index ラベル **0 件**（すべて無し）・ Ghidra 台帳 21,597 収録 **0 件**（すべて無し）・ Copilot V86 UNLABELED 収録 **10 件（A=1 / B=9 / C=0）**・ V85 **7**・ V92 **1**・ V96 **1**・ 現行規則（terminator + 1..31×CC + accept600）成立 **10/10**。


**読み方**: 上表の「判定」は **Copilot 全数列挙（V86 UNLABELED）に対する収録有無** であり、Ghidra 台帳 21,597 と `dec_index` 20,465 に対しては **全 10 件が漏れ**（= それらの台帳にとって本实例はそのまま「無名 gap」である）。段階別の内訳は (c)-3。

### (b)-1. witness を含むブロック（全 10 件。gap 先頭 entry と別ブロックに載るのは I2 / I4 / I8）

gap 内は `ret`/`jmp` で複数ブロックに分割される。witness（call サイト / b43070 disp32）が載るブロックの先頭を、gap 先頭 entry と別に実測した。

| # | gap 先頭 entry | gap 内ブロック数 | witness ブロック先頭 | その直前 | 規則一致 | V86 収録 | entry と同一 |
|---|---|---:|---|---|---|---|---|
| I1 | `0x00A10EB0` | 140 | `0x00A10EB0` | 12×`CC` + ret | OK | B | 同一 |
| I2 | `0x0040DF80` | 30 | `0x0040F488` | パディングなし（直前 = jmp rel32 (e9) @`0x0040F483`） | **NG** | **無** | **別ブロック** |
| I3 | `0x00437600` | 1 | `0x00437600` | 1×`CC` + ret | OK | B | 同一 |
| I4 | `0x00656C90` | 9 | `0x00656F29` | パディングなし（直前 = jmp rel8 (eb) @`0x00656F27`） | **NG** | **無** | **別ブロック** |
| I5 | `0x006710B0` | 1 | `0x006710B0` | 11×`CC` + ret | OK | B | 同一 |
| I6 | `0x007B14C0` | 1 | `0x007B14C0` | 3×`CC` + ret imm16 (c2 iw) | OK | B | 同一 |
| I7 | `0x007C53B0` | 2 | `0x007C53B0` | 2×`CC` + ret imm16 (c2 iw) | OK | B | 同一 |
| I8 | `0x00405440` | 2 | `0x00405550` | パディングなし（直前 = ret @`0x0040554F`） | **NG** | **無** | **別ブロック** |
| I9 | `0x006A6AA0` | 2 | `0x006A6AA0` | 5×`CC` + ret imm16 (c2 iw) | OK | B | 同一 |
| I10 | `0x006A6D10` | 4 | `0x006A6D10` | 8×`CC` + ret imm16 (c2 iw) | OK | B | 同一 |

**件数**: witness ブロック先頭が gap 先頭 entry と同一 = **7/10**、別ブロック = **3 件**（I2、I4、I8）。
別ブロックの直前はいずれも **パディングなし（adjacent = 直前 terminator または jmp 直後）**であり、CC-pad 規則では候補に上がらない（= v95b の `adjacent` クラス）。

### (b)-2 個別実測（probe_harness `rd()` + capstone 5.0.7 / skipdata 不使用・詳細 decode）

#### I1. CRT 初期化スタブ（FUN_007b7250 の唯一実行経路）

- v81 記載: `0xA10EB0`（call at 0xA10EB5） ｜ witness: `call 0x7b7250` @0xA10EB5（.rdata 登録表 0xA2EFF0 経由で起動。E8 呼出し先としては未登録）
- gap 下端 `0x00A10EA4`（前台帳 `FUN_00a10e50` @`0x00A10E50` の terminator 直後）→ gap 上端（次台帳開始）`0x00A12380` (`FUN_00a12380`)
- **実測 entry `0x00A10EB0`**: 直前に **12 B の `0xCC` パディング**、その直前 terminator = **ret @`0x00A10EA3`**。entry 直前 32 B = `ff83c4048b4c240464890d000000005983c40cc3cccccccccccccccccccccccc`
- capstone: accept(600B) = **ok** / 命令数 = 6 / 終端 `0x00A10EC5`
- `dec_index`（20,465 件）に entry のラベル: **無** ｜ header-bisect での帰属 = `FUN_00a10e50` @`0x00A10E50`（= 誤帰属。entry は decompiled 未収録）
- 収録: 台帳21,597 **無** ／ V86 **B** ／ V85 **無** ／ V87 **有** ／ V92 **無** ／ V96 **無**
- V86 台帳の行（原文）: `- 0x00A10EB0  refs=1 insns=6 first=mov`
- gap 内の E8 呼出し先（外部から呼ばれることの証拠）: 0 件（.rdata / ポインタ参照のみ）

#### I2. FUN_0040c350 の唯一の E8 呼出し元

- v81 記載: `0x40F5xx`（gap [0x40DF74, 0x40F640) 内） ｜ witness: `call 0x40c350` @0x40F580（E8 cb cd ff ff）
- gap 下端 `0x0040DF74`（前台帳 `FUN_0040d4d0` @`0x0040D4D0` の terminator 直後）→ gap 上端（次台帳開始）`0x0040F640` (`FUN_0040f640`)
- **実測 entry `0x0040DF80`**: 直前に **12 B の `0xCC` パディング**、その直前 terminator = **ret @`0x0040DF73`**。entry 直前 32 B = `0be80a9f0800881da74fb3005f5e5d5b83c410c3cccccccccccccccccccccccc`
- capstone: accept(600B) = **ok** / 命令数 = 55 / 終端 `0x0040E047`
- `dec_index`（20,465 件）に entry のラベル: **無** ｜ header-bisect での帰属 = `FUN_0040d4d0` @`0x0040D4D0`（= 誤帰属。entry は decompiled 未収録）
- 収録: 台帳21,597 **無** ／ V86 **A** ／ V85 **有** ／ V87 **有** ／ V92 **有** ／ V96 **有**
- V86 台帳の行（原文）: `- 0x0040DF80  pad=12 insns=55 first=sub prev_term=0x0040DF74`
- gap 内の E8 呼出し先（外部から呼ばれることの証拠）: `0x0040DF80`←0x0098780E
- witness ブロック: 先頭 `0x0040F488`（gap 先頭 entry とは別）/ 117 命令 / 終端 `0x0040F637` / 直前 = パディングなし（直前 = jmp rel32 (e9) @`0x0040F483`）

#### I3. FUN_007b4010 の第 2 呼出し（+0x0C map owner）

- v81 記載: `0x437600`（call at 0x437658） ｜ witness: `call 0x7b4010` @0x437658
- gap 下端 `0x004375FF`（前台帳 `FUN_00437590` @`0x00437590` の terminator 直後）→ gap 上端（次台帳開始）`0x004376A0` (`FUN_004376a0`)
- **実測 entry `0x00437600`**: 直前に **1 B の `0xCC` パディング**、その直前 terminator = **ret @`0x004375FE`**。entry 直前 32 B = `c7442418ffffffffe823fbffff8b4c241064890d00000000595f5e83c410c3cc`
- capstone: accept(600B) = **ok** / 命令数 = 55 / 終端 `0x00437691`
- `dec_index`（20,465 件）に entry のラベル: **無** ｜ header-bisect での帰属 = `FUN_00437590` @`0x00437590`（= 誤帰属。entry は decompiled 未収録）
- 収録: 台帳21,597 **無** ／ V86 **B** ／ V85 **有** ／ V87 **有** ／ V92 **無** ／ V96 **無**
- V86 台帳の行（原文）: `- 0x00437600  refs=1 insns=55 first=sub`
- gap 内の E8 呼出し先（外部から呼ばれることの証拠）: 0 件（.rdata / ポインタ参照のみ）

#### I4. FUN_0066c580 の gap 呼出し元 A（2 サイト）

- v81 記載: `0x656C82..`（sites 0x656F80 / 0x656FAF） ｜ witness: `call 0x66c580` @0x656F80 と @0x656FAF
- gap 下端 `0x00656C82`（前台帳 `FUN_006564c0` @`0x006564C0` の terminator 直後）→ gap 上端（次台帳開始）`0x00657340` (`FUN_00657340`)
- **実測 entry `0x00656C90`**: 直前に **14 B の `0xCC` パディング**、その直前 terminator = **ret imm16 (c2 iw) @`0x00656C7F`**。entry 直前 32 B = `64890d00000000595f5e5d5b83c44cc20c00cccccccccccccccccccccccccccc`
- capstone: accept(600B) = **ok** / 命令数 = 40 / 終端 `0x00656D02`
- `dec_index`（20,465 件）に entry のラベル: **無** ｜ header-bisect での帰属 = `FUN_006564c0` @`0x006564C0`（= 誤帰属。entry は decompiled 未収録）
- 収録: 台帳21,597 **無** ／ V86 **B** ／ V85 **有** ／ V87 **有** ／ V92 **無** ／ V96 **無**
- V86 台帳の行（原文）: `- 0x00656C90  refs=2 insns=40 first=mov`
- gap 内の E8 呼出し先（外部から呼ばれることの証拠）: 0 件（.rdata / ポインタ参照のみ）
- witness ブロック: 先頭 `0x00656F29`（gap 先頭 entry とは別）/ 35 命令 / 終端 `0x00656FB4` / 直前 = パディングなし（直前 = jmp rel8 (eb) @`0x00656F27`）
- 第 2 witness `0x00656FAF`: 同一ブロック = **同一**

#### I5. FUN_0066c580 の gap 呼出し元 B（1 サイト）

- v81 記載: `0x6710A5..`（site 0x6710DA） ｜ witness: `call 0x66c580` @0x6710DA
- gap 下端 `0x006710A5`（前台帳 `FUN_00670f50` @`0x00670F50` の terminator 直後）→ gap 上端（次台帳開始）`0x00671170` (`FUN_00671170`)
- **実測 entry `0x006710B0`**: 直前に **11 B の `0xCC` パディング**、その直前 terminator = **ret @`0x006710A4`**。entry 直前 32 B = `5cdaffff6a008bcfe8c37de6ff5f5e5d5b83c42cc3cccccccccccccccccccccc`
- capstone: accept(600B) = **ok** / 命令数 = 57 / 終端 `0x0067116C`
- `dec_index`（20,465 件）に entry のラベル: **無** ｜ header-bisect での帰属 = `FUN_00670f50` @`0x00670F50`（= 誤帰属。entry は decompiled 未収録）
- 収録: 台帳21,597 **無** ／ V86 **B** ／ V85 **有** ／ V87 **有** ／ V92 **無** ／ V96 **無**
- V86 台帳の行（原文）: `- 0x006710B0  refs=1 insns=57 first=mov`
- gap 内の E8 呼出し先（外部から呼ばれることの証拠）: 0 件（.rdata / ポインタ参照のみ）

#### I6. b43070 読手（0x7B1513）

- v81 記載: `0x7B14BE`（FUN_007b13a0 ret@0x7B14BA + 3CC の後） ｜ witness: `cmp byte ptr [0xb43070],0` の disp32 @0x7B1513（命令先頭 0x7B1511）
- gap 下端 `0x007B14BD`（前台帳 `FUN_007b13a0` @`0x007B13A0` の terminator 直後）→ gap 上端（次台帳開始）`0x007B1670` (`FUN_007b1670`)
- **実測 entry `0x007B14C0`**: 直前に **3 B の `0xCC` パディング**、その直前 terminator = **ret imm16 (c2 iw) @`0x007B14BA`**。entry 直前 32 B = `20518bcfe8b795fcff8b4c241064890d00000000595f5e83c410c20400cccccc`
- capstone: accept(600B) = **ok** / 命令数 = 136 / 終端 `0x007B1667`
- `dec_index`（20,465 件）に entry のラベル: **無** ｜ header-bisect での帰属 = `FUN_007b13a0` @`0x007B13A0`（= 誤帰属。entry は decompiled 未収録）
- 収録: 台帳21,597 **無** ／ V86 **B** ／ V85 **有** ／ V87 **有** ／ V92 **無** ／ V96 **無**
- V86 台帳の行（原文）: `- 0x007B14C0  refs=1 insns=136 first=sub`
- gap 内の E8 呼出し先（外部から呼ばれることの証拠）: 0 件（.rdata / ポインタ参照のみ）

#### I7. b43070 読手（0x7C53B2）

- v81 記載: `0x7C53B0`（FUN_007c5340 `ret 8` @0x7C53AB の後） ｜ witness: `cmp byte ptr [0xb43070],0` の disp32 @0x7C53B2（命令先頭 0x7C53B0）
- gap 下端 `0x007C53AE`（前台帳 `FUN_007c5340` @`0x007C5340` の terminator 直後）→ gap 上端（次台帳開始）`0x007C5440` (`FUN_007c5440`)
- **実測 entry `0x007C53B0`**: 直前に **2 B の `0xCC` パディング**、その直前 terminator = **ret imm16 (c2 iw) @`0x007C53AB`**。entry 直前 32 B = `d986800000005fd99b800000008b8e880000005e898b880000005bc20800cccc`
- capstone: accept(600B) = **ok** / 命令数 = 58 / 終端 `0x007C5434`
- `dec_index`（20,465 件）に entry のラベル: **無** ｜ header-bisect での帰属 = `FUN_007c5340` @`0x007C5340`（= 誤帰属。entry は decompiled 未収録）
- 収録: 台帳21,597 **無** ／ V86 **B** ／ V85 **無** ／ V87 **有** ／ V92 **無** ／ V96 **無**
- V86 台帳の行（原文）: `- 0x007C53B0  refs=1 insns=58 first=cmp`
- gap 内の E8 呼出し先（外部から呼ばれることの証拠）: 0 件（.rdata / ポインタ参照のみ）

#### I8. b43070 読手（0x405588）

- v81 記載: `0x40543A..`（FUN_004053e0 ret@0x405439 の後） ｜ witness: `cmp byte ptr [0xb43070],0` の disp32 @0x405588（命令先頭 0x405586）
- gap 下端 `0x0040543A`（前台帳 `FUN_004053e0` @`0x004053E0` の terminator 直後）→ gap 上端（次台帳開始）`0x004055E0` (`FUN_004055e0`)
- **実測 entry `0x00405440`**: 直前に **6 B の `0xCC` パディング**、その直前 terminator = **ret @`0x00405439`**。entry 直前 32 B = `da57008bf083c40885f675d85e8ac35bc35eb0015bc38ac35bc3cccccccccccc`
- capstone: accept(600B) = **ok** / 命令数 = 96 / 終端 `0x0040554F`
- `dec_index`（20,465 件）に entry のラベル: **無** ｜ header-bisect での帰属 = `FUN_004053e0` @`0x004053E0`（= 誤帰属。entry は decompiled 未収録）
- 収録: 台帳21,597 **無** ／ V86 **B** ／ V85 **有** ／ V87 **有** ／ V92 **無** ／ V96 **無**
- V86 台帳の行（原文）: `- 0x00405440  refs=2 insns=96 first=push`
- gap 内の E8 呼出し先（外部から呼ばれることの証拠）: 0 件（.rdata / ポインタ参照のみ）
- witness ブロック: 先頭 `0x00405550`（gap 先頭 entry とは別）/ 43 命令 / 終端 `0x004055D9` / 直前 = パディングなし（直前 = ret @`0x0040554F`）

#### I9. b43070 読手（0x6A6AA2）

- v81 記載: `0x6A6A9B..` ｜ witness: `cmp byte ptr [0xb43070],0` の disp32 @0x6A6AA2（命令先頭 0x6A6AA0）
- gap 下端 `0x006A6A9B`（前台帳 `FUN_006a6920` @`0x006A6920` の terminator 直後）→ gap 上端（次台帳開始）`0x006A6AF0` (`FUN_006a6af0`)
- **実測 entry `0x006A6AA0`**: 直前に **5 B の `0xCC` パディング**、その直前 terminator = **ret imm16 (c2 iw) @`0x006A6A98`**。entry 直前 32 B = `e8eb23fdff8b4c241464890d00000000595f5e5d5b83c40cc20400cccccccccc`
- capstone: accept(600B) = **ok** / 命令数 = 5 / 終端 `0x006A6AB1`
- `dec_index`（20,465 件）に entry のラベル: **無** ｜ header-bisect での帰属 = `FUN_006a6920` @`0x006A6920`（= 誤帰属。entry は decompiled 未収録）
- 収録: 台帳21,597 **無** ／ V86 **B** ／ V85 **無** ／ V87 **有** ／ V92 **無** ／ V96 **無**
- V86 台帳の行（原文）: `- 0x006A6AA0  refs=1 insns=5 first=cmp`
- gap 内の E8 呼出し先（外部から呼ばれることの証拠）: 0 件（.rdata / ポインタ参照のみ）

#### I10. b43070 読手（0x6A6D39）

- v81 記載: `0x6A6A9B..` 系の第 2 読手（v81 本文 0x6A6D39） ｜ witness: `cmp byte ptr [0xb43070],0` の disp32 @0x6A6D39（命令先頭 0x6A6D37）
- gap 下端 `0x006A6D08`（前台帳 `FUN_006a6af0` @`0x006A6AF0` の terminator 直後）→ gap 上端（次台帳開始）`0x006A6DE0` (`FUN_006a6de0`)
- **実測 entry `0x006A6D10`**: 直前に **8 B の `0xCC` パディング**、その直前 terminator = **ret imm16 (c2 iw) @`0x006A6D05`**。entry 直前 32 B = `3bb30080797c71720b6a0483c63856e8ccc7daff5ec20400cccccccccccccccc`
- capstone: accept(600B) = **ok** / 命令数 = 26 / 終端 `0x006A6D67`
- `dec_index`（20,465 件）に entry のラベル: **無** ｜ header-bisect での帰属 = `FUN_006a6af0` @`0x006A6AF0`（= 誤帰属。entry は decompiled 未収録）
- 収録: 台帳21,597 **無** ／ V86 **B** ／ V85 **有** ／ V87 **有** ／ V92 **無** ／ V96 **無**
- V86 台帳の行（原文）: `- 0x006A6D10  refs=1 insns=26 first=mov`
- gap 内の E8 呼出し先（外部から呼ばれることの証拠）: 0 件（.rdata / ポインタ参照のみ）

## (c) 規則検証（"ret+4×CC" と ret+1..3×int3）

### (c)-1 "ret+4×CC" という規則は成果物に存在しない

`tmp/re/` の V85/V86/V92/V95/V96 の `.py` / `.txt` / `.md` 一括と `docs/RE_EXE_REPORT.md` §70.16 を全文走査した（パターン `ret+4×CC|4×0xCC|>= ?4 ?x ?0?CC` 等）。`ret+4×CC` 相当の記述・実装は **0 件**。唯一の出現は `tmp/re/v81_shader_ini_residual.md` L362（本検証の対象となった記述そのもの）のみ。実装は一貫して **pad 1..31**:

| 成果物 | 実装 / 記述された規則 | 原文 |
|---|---|---|
| V85 `v85_unlabeled_funcs.py` | `ret`/`ret imm16` + **>=1 × 0xCC** + prologue | `- heuristic: ret ([0xC3] | [0xC2 iw]) + >=1 x 0xCC + prologue` |
| V86 `v86_unlabeled_diff.py` | **0xCC x1..31** + 直前 terminator | コード: `if j < n and 1 <= j - i <= 31 and is_terminator(data, i)` / 出力: `- boundary rule: 0xCC x1..31 preceded by a terminator (ret/jmp/iret/hlt/ud2)` |
| 正典 §70.16 (1) | 同上 | `0xCC ラン（長さ 1〜31）を検出し、その直前が terminator` |
| V95b `v95b_recall_fix.py` | `pads=CC` + term（`padCC(1..31)+term`） | `R1 current pads=CC min3 len600`（`min3` は **min_insn=3** であり pad 最小ではない） |
| V96 `v96_e8_candidates.py` | `1 <= plen <= 31` | `if c in PAD_CHARS and 1 <= plen <= 31 and t >= 0: return "pad%02X(%d)+term"` |

**判定**: v81 §7-7 の「`"ret+4×CC" 規則（V85/V96）」は成果物上に存在しない（想起違い）。実在規則は `terminator + 1..31×CC`（+ capstone accept: 600 B 窓・int3 等で棄却・ret/jmp 到達・命令数 >=3）。以下はこの実在規則に対する検証。

### (c)-2 定量: 10 实例の拾い漏れ

| # | entry | 直前 pad | pad∈[1..31]+term | accept600 | 判定 | 原因 |
|---|---|---:|---|---|---|---|
| I1 | `0x00A10EB0` | 12×`CC` | OK | ok | **カバー済** | ― |
| I2 | `0x0040DF80` | 12×`CC` | OK | ok | **カバー済** | ― |
| I3 | `0x00437600` | 1×`CC` | OK | ok | **カバー済** | ― |
| I4 | `0x00656C90` | 14×`CC` | OK | ok | **カバー済** | ― |
| I5 | `0x006710B0` | 11×`CC` | OK | ok | **カバー済** | ― |
| I6 | `0x007B14C0` | 3×`CC` | OK | ok | **カバー済** | ― |
| I7 | `0x007C53B0` | 2×`CC` | OK | ok | **カバー済** | ― |
| I8 | `0x00405440` | 6×`CC` | OK | ok | **カバー済** | ― |
| I9 | `0x006A6AA0` | 5×`CC` | OK | ok | **カバー済** | ― |
| I10 | `0x006A6D10` | 8×`CC` | OK | ok | **カバー済** | ― |

- **gap 先頭 entry の pad 実測分布**: 1B:1, 2B:1, 3B:1, 5B:1, 6B:1, 8B:1, 11B:1, 12B:2, 14B:1（pad 1..3 = **3 件**、pad 0 = **0 件**）。
- **`ret + 1..3×int3` の細かい境界で取り逃す实例は 0 件**: 規則の pad 範囲が 1..31 であるため、pad=1..3 の实例（I3=1B / I7=2B / I6=3B）はすべて形状規則を満たし、accept600 でも `ok`。v81 の「ret+1..3×int3 では境界以前に帰着せず」は実在規則に該当しない。
- **pad=0（隣接）で落ちる gap 先頭 entry: 0 件**。
- 一方、**witness を載せるブロック**（(b)-1）では 3 件が「直前 terminator / jmp 直後でパディングなし」= 規則不可視。これが実際の漏れであり、v95b の `adjacent`（1,633 件 = 7.6%、再現率 89.8%→97.3% の差分要因）と同一クラス。

### (c)-3 台帳レベルの突合結果（3 段階）

| 段階 | 台帳 | 規模 | 10 实例の収録 | 判定 |
|---|---|---:|---:|---|
| L1: Ghidra FunctionManager | `func_list_new.txt` | 21,597 | **0 / 10** | **全件 漏れ**（= 本来の「無名 gap」） |
| L2: decompiled ヘッダ index | `dec_index` | 20,465 | **0 / 10** | **全件 漏れ**（header-bisect では前の関数へ誤帰属） |
| L3: Copilot 全数列挙 | V86 UNLABELED | 10,175 | **10 / 10**（A=1, B=9, C=0） | **全件カバー済**（ただし 9 件は B 節 = imm32 参照のみ・確定リスト外） |
| L3': Copilot 確定扱い（V92 A 節 triage 214） | `v92_e8_214_triage.txt` | 214 | **1 / 10** | 0x40DF80 のみ確定扱い、他 9 件は B 節保留 |
| L4: V96 規則非依存 E8 全列挙 | `v96_e8_candidates.txt` | 565 ターゲット | **1 / 10** | gap 内に E8 呼出し先があるのは 0x40DF80 のみ |
| L5: witness ブロック（CC-pad 規則の可視性） | （規則適用） | ― | **7 / 10** | **3 件は漏れ**（adjacent / jmp 直後） |

V85 収録 7 / 10・V87 収録 10 / 10（V86 と同集合の位置分類）。

## (d) 台帳側への追記提案文（**提案のみ・実編集はしない**）

### (d)-1 `docs/RE_EXE_REPORT.md` §70.16 へ追加する新小節

```markdown
#### (4c) 無名 gap 新实例の台帳突合（V96・2026-10-25）[検証済]

V81 §7-7 の残点 7 に挙がった無名 gap 関数の新实例 10 件（親関数単位）を probe_harness で実測し、
dec_index 20,465 / Ghidra 台帳 21,597 / V86 UNLABELED 10,175 / V85 18,947 / V92 214 / V96 565 と突合した。

- **L1 Ghidra 台帳 21,597: 0/10 収録**（全件が無名 gap）。**L2 dec_index 20,465: 0/10**。
  header-bisect はいずれも前の関数へ誤帰属する（例: 0x437600 → FUN_00437590、0xA10EB0 → FUN_00a10e50）。
- **L3 Copilot 全数列挙 V86: 10/10 収録**（A 節 1 = 0x40DF80 / B 節 9 = imm32 参照のみ）。
  したがって §70.16 (3) の「B 節は確定リストに含めない」方針のままでは、この 9 件は**確定扱いにならない**。
- 実測 entry と直前境界:

  | id | v81 記載 | entry | pad | terminator | accept600 | V86 |
  |---|---|---|---:|---|---|---|
  | I1 | 0xA10EB0（call at 0xA10EB5） | `0x00A10EB0` | 12×CC | ret | ok | B |
  | I2 | 0x40F5xx（gap [0x40DF74, 0x40F640) 内） | `0x0040DF80` | 12×CC | ret | ok | A |
  | I3 | 0x437600（call at 0x437658） | `0x00437600` | 1×CC | ret | ok | B |
  | I4 | 0x656C82..（sites 0x656F80 / 0x656FAF） | `0x00656C90` | 14×CC | ret imm16 (c2 iw) | ok | B |
  | I5 | 0x6710A5..（site 0x6710DA） | `0x006710B0` | 11×CC | ret | ok | B |
  | I6 | 0x7B14BE（FUN_007b13a0 ret@0x7B14BA + 3CC の後） | `0x007B14C0` | 3×CC | ret imm16 (c2 iw) | ok | B |
  | I7 | 0x7C53B0（FUN_007c5340 ret 8 @0x7C53AB の後） | `0x007C53B0` | 2×CC | ret imm16 (c2 iw) | ok | B |
  | I8 | 0x40543A..（FUN_004053e0 ret@0x405439 の後） | `0x00405440` | 6×CC | ret | ok | B |
  | I9 | 0x6A6A9B.. | `0x006A6AA0` | 5×CC | ret imm16 (c2 iw) | ok | B |
  | I10 | 0x6A6A9B.. 系の第 2 読手（v81 本文 0x6A6D39） | `0x006A6D10` | 8×CC | ret imm16 (c2 iw) | ok | B |

- **規則検証**: 成果物に `ret+4×CC` 規則は存在しない（実装は terminator + 1..31×CC）。
  `ret + 1..3×int3` の細境界で取り逃す实例は **0 件**（pad 1..3 は規則の範囲内）。
  実際の可視性損失は **witness を載せるブロックが adjacent / jmp 直後（pad=0）**である 3 件
（I2、I4、I8）＝ v95b の `adjacent` クラスと同型。
- 台帳へ追記する際は「gap 先頭 entry のみを 1 関数として登録」では不足し、
  gap 内の後続ブロック（パディングなしで続く実コード）も対象に含める必要がある。
```

### (d)-2 `func_list_new.txt` 相当の追記行（別紙・本体は未編集）

| addr | proposed name | pad | terminator | evidence | V86 |
|---|---|---:|---|---|---|
| `0x00A10EB0` | `FUN_00a10eb0__crt_init_stub` | 12×CC | ret @`0x00A10EA3` | `call 0x7b7250` @0xA10EB5（.rdata 登録表 0xA2EFF0 経由で起動。E8 呼出し先としては未登録） / accept=ok | B |
| `0x0040DF80` | `FUN_0040df80__gap_A (0x40c350 caller lives in a later block)` | 12×CC | ret @`0x0040DF73` | `call 0x40c350` @0x40F580（E8 cb cd ff ff） / accept=ok | A |
| `0x00437600` | `FUN_00437600__map_owner` | 1×CC | ret @`0x004375FE` | `call 0x7b4010` @0x437658 / accept=ok | B |
| `0x00656C90` | `FUN_00656c90__0x66c580_caller_A` | 14×CC | ret imm16 (c2 iw) @`0x00656C7F` | `call 0x66c580` @0x656F80 と @0x656FAF / accept=ok | B |
| `0x006710B0` | `FUN_006710b0__0x66c580_caller_B` | 11×CC | ret @`0x006710A4` | `call 0x66c580` @0x6710DA / accept=ok | B |
| `0x007B14C0` | `FUN_007b14c0__b43070_reader` | 3×CC | ret imm16 (c2 iw) @`0x007B14BA` | `cmp byte ptr [0xb43070],0` の disp32 @0x7B1513（命令先頭 0x7B1511） / accept=ok | B |
| `0x007C53B0` | `FUN_007c53b0__b43070_reader` | 2×CC | ret imm16 (c2 iw) @`0x007C53AB` | `cmp byte ptr [0xb43070],0` の disp32 @0x7C53B2（命令先頭 0x7C53B0） / accept=ok | B |
| `0x00405440` | `FUN_00405440__b43070_reader` | 6×CC | ret @`0x00405439` | `cmp byte ptr [0xb43070],0` の disp32 @0x405588（命令先頭 0x405586） / accept=ok | B |
| `0x006A6AA0` | `FUN_006a6aa0__b43070_reader` | 5×CC | ret imm16 (c2 iw) @`0x006A6A98` | `cmp byte ptr [0xb43070],0` の disp32 @0x6A6AA2（命令先頭 0x6A6AA0） / accept=ok | B |
| `0x006A6D10` | `FUN_006a6d10__b43070_reader` | 8×CC | ret imm16 (c2 iw) @`0x006A6D05` | `cmp byte ptr [0xb43070],0` の disp32 @0x6A6D39（命令先頭 0x6A6D37） / accept=ok | B |

追記時は **witness ブロックの先頭**（(b)-1 表の `gap 先頭 entry と別ブロック` の行）も併記すること。

## 残点

1. **v81 記載と実測のずれ**: `0x7B14BE` は実測 entry `0x7B14C0`（ret imm16 は 0x7B14BA-0x7B14BC + 3×CC = 0x7B14BD-0x7B14BF）で **2 B 差**。`0x40543A..` / `0x6A6A9B..` / `0x656C82..` / `0x6710A5..` は **gap 下端（最初の CC）**であり entry ではない（entry = 0x405440 / 0x6A6AA0 / 0x656C90 / 0x6710B0）。
2. **gap は 1 関数ではない**: gap 内ブロック数は I1=140 / I2=30 / I3=1 / I4=9 / I5=1 / I6=1 / I7=2 / I8=2 / I9=2 / I10=4。
   v81 の「無名 gap 関数 A = [0x656C82, 0x657340)」型の 1 関数仮定と、CC-pad 規則が gap 先頭 1 点しか可視化しない事実は両立しない。ブロックが同一関数の multi-exit か独立関数かは**未判定**（E8 呼出し先は gap 内に 0x40DF80 のみで、外部呼出し証拠が無い）。
3. **I9 / I10 の前台帳**: I9 prev = `FUN_006a6920`、I10 prev = `FUN_006a6af0`。v81 は両者を `0x6A6A9B..` の一括記載にしているが、**間に台帳関数が存在する別 gap** である（v81 記載の粒度不足）。
4. **accept600（600 B 窓）の影響**: 本 10 件はすべて `ok` だったが、gap 内後続ブロックは未測定。V86 の MAX_LEN=600 は v95b で「min_insn/max_len を振っても等値」と報告されており、長大関数での落ちは今回未検証。
5. **b43070 読手の witness は disp32 位置**: `cmp byte ptr [0xb43070],0` の即値バイト位置のため、命令境界チェック（`site in bounds`）では検証できない。本突合は [entry, 終端] の範囲包含で判定した。
6. **未実施**: Ghidra 側への関数登録、`docs/`・`tmp/re/` 既存ファイルの編集、git commit、memory 追加、sync ツール（すべて禁止事項のため未実施）。

---
生成物: `tmp/re/v96_gap_instances_crosscheck.md`（新規 1 ファイル） / 生成スクリプト: `%TEMP%\opencode\v96_gap_xcheck.py` / 前提: `probe_harness.py selftest` 4/4 PASS
