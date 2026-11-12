# V75B 調査 findings — SetStage 頻度の単位突合 / 0x52F010 本体 / 0x50A050 サニティ

- 調査日: 2026-10-24
- 対象: Oblivion.exe (Steam v1.2.0416, ImageBase 0x400000, capstone 5.0.7 / 32bit)
- ガード: `python probe_harness.py selftest` → **ALL PASS**（bytes@0x504050, bytes@0x4FB580, string@0xA7035C, opid 0x0015→0x504560）、v75b probe 冒頭バイトゲート 2/2 一致
- 前提: 本ファイルのみ新規作成。既存ファイルの変更なし。

---

## 1. SetStage（0x1039）頻度 3 数字の単位突合

### 判定

| 数字 | 単位（判定） | コーパス / 分母 | 出典（検証済み） |
|---|---|---|---|
| **1,391** | **命令出現数（SetStage opcode の実行回数）** — 「SetStage を含有するスクリプト数」**ではない** | SCPT 専用: 2,393 SCPT スクリプト / 65,628 命令 | `tmp/re/copilot_archive/scda_opcode_table_out.txt:39`（`0x1039 1391 8 10 8:8, 10:1383`、ヘッダ `scripts: 2393` / `complete: 2393/2393`、7,546 B）ほか 4 箇所 |
| **3,479** | **命令出現数、全コーパス合計**（SCPT 1,391 + QUST 388 + INFO 1,700 = 3,479、86,916 命令中 4.00%、INFO 内 15.51%） | SCPT+QUST+INFO: 9,646 ブロック / 86,916 命令 | `plan.md:521`（頻度表の行）、`plan.md:546`（内訳 prose）。旧「出典不明」は**解消** |
| **2,000-3,000** | **推定値（artifact なし）— 使用不可** | — | `tmp/re/contradictions_TU_verify_20261024.md:55` のセル内に「（推定）」ラベル付きで唯一出現。生成元の計測ファイル無し |

単位判定の中核証拠:

1. **1,391 はヒストグラムの合計と一致する（命令単位）**: `8:8, 10:1383` = 8 + 1,383 = **1,391**。argLen ごとの命令カウントの和であって、スクリプト数の和ではない。`s69_scda_b.md:100` も `- 出現数: 1391、argLen 8/10` と**「出現数」**と明記。
2. **ヘッダ `scripts: 2393` は分母**（全 SCPT デコード成功数）。`s59_svm_r7_bytecode.txt:2` `scripts=2393  decoded_ok=2393  total_insns=65628`。矛盾点: contradictions doc :55 は「1,391（scda=含有スクリプト数）」と注記したが、含有スクリプト数ならヒストグラム和と一致しない。**該当注記は誤り**。
3. **3,479 = 1,391 + 388 + 1,700 の和**、`plan.md:546` が明示（"3,479 instructions (4.00%), split SCPT 1,391 / QUST 388 / INFO 1,700 - 15.51% of all INFO instructions"）。`plan.md:521` の表行 `| 0x1039 | 3,479 | 4.00% | 71.53% | SetStage |` と矛盾なし（3,479 / 86,916 = 4.00%）。

1,391 の出現箇所（すべて同源 = scda 全体集計の転記）:

- `tmp/re/copilot_archive/scda_opcode_table_out.txt:39`（原本。RE_ANALYSIS_ROADMAP.md:137 と s69_scda_b.md:7 が参照するファイル。**ファイルは実在する**）
- `tmp/re/s59_svm_r7_bytecode.txt:101`（`  0x1039 1391`、同 :2 にヘッダ）
- `tmp/re/s57_svm_8_opcode_table.txt:39`（`0x1039 function 1391 8..10 8:8, 10:1383 ... SetStage 00502CB0 004FDE30`）
- `tmp/re/s69_scda_a_03_optable.txt:83`（`0x1039 SetStage SetStage 0x502CB0 0 (1391) 8:8,10:1383`）
- `tmp/re/s69_scda_b.md:100`（`出現数: 1391、argLen 8/10`）

3,479 の出典:

- `plan.md:502`（`9,646 blocks / 86,916 instructions`）、`plan.md:509`（Total 行 `9,646 | 86,916 | 156`）＝分母
- `plan.md:521`（頻度表の 0x1039 行 = 3,479 / 4.00%）
- `plan.md:546`（owner 分割 prose = 1,391 / 388 / 1,700）

RE_EXE_REPORT 側の確認（contradictions :55 の指摘を再確認）:

- `docs/RE_EXE_REPORT.md` の `SetStage` 出現は **1 行のみ（:10022、QUST プロズ）**、`0x1039` は **0 件** → 「RE_EXE_REPORT に SetStage の pipe 行なし」は事実、MISS 判定の根拠は維持。

### 手法

- `rg -n` で `plan.md` / `tmp/re/*` / `docs/RE_EXE_REPORT.md` / `docs/RE_ANALYSIS_ROADMAP.md` を横断し、3 数字の全出現行を列挙・原文引用。
- argLen ヒストグラムの加算（8 + 1,383 = 1,391）と owner 分割の加算（1,391 + 388 + 1,700 = 3,479）、割合検算（3,479 / 86,916 = 4.00%）で単位を確定。
- `Test-Path` で `tmp/re/copilot_archive/scda_opcode_table_out.txt` の実在を確認（7,546 B、`scripts: 2393` / `complete: 2393/2393` / `141 distinct opcodes`）。
- ESM 再スキャンは行わず（禁止事項）、既存 artifact の突合のみ。

### 残課題

- QUST 388 / INFO 1,700 の独立 artifact（owner別セサス表）は存在せず、`plan.md:546` の prose が唯一の出典（scda 全体集計は SCPT 専用）。
- 別単位の周辺数字（混同注意）:
  - **50** = 最終命令が SetStage の SCPT 本数（STOP 欠落; `scda_opcode_table_out.txt:158` `0x1039 50`、"last instruction of each script" セクション）。
  - **987 samples** = 名寄せコーパス上の SetStage ソース文サンプル数（`plan.md:473`、別軸）。
  - s57:39 / s69_scda_b.md:157 の列 `220` は単位未確認（今回は不使用）。
- `tmp/re/contradictions_TU_verify_20261024.md:55` の「1,391（scda=含有スクリプト数）」注記は実態と逆。要訂正（本ファイルがその訂正）。

### 推奨ドキュメント記述

> **SetStage（0x1039）頻度 = 命令出現数**：全コーパス 3,479 / 86,916 命令（4.00%）＝ SCPT 1,391 + QUST 388 + INFO 1,700（INFO 内 15.51%）。SCPT 単体のセサス値 1,391 は「SCPT 2,393 スクリプト・65,628 命令中の 0x1039 出現命令数」であり、**SetStage を含有するスクリプト数ではない**（分母 `scripts: 2393` は全 SCPT 数）。「2,000-3,000」は推定値のため使用しない。

---

## 2. 0x52F010 本体（+ 0x50A050 サニティ）

### 判定 — 0x52F010

- **用途: Conversation 選択（Topic 解決）関数。** annotate_db の正式名 **`Topic_Resolve`**（`functions["0x52F010"] = {"name": "Topic_Resolve"}`）と一致。this（ecx）の `this+0x28` Topic リストを走査し、各 Conversation について `Topic_CanUse`（0x530830）で条件判定 → 最初の合格候補を優先返却、bit1 付き候補はグローバル保持子に保留して走査終了時に **`rand() % count + 1`** で 1 つ抽選、何も無ければガード付き再帰（型 {2,3,4,6} かつ param5≠0 のとき）で再試行。
- **範囲: [0x52F010, 0x52F330)**（線形デコード 269 命令が末尾 `ret 0x18`@0x52F32D + 3B = 0x52F330 で**正確に一致**。次関数は `FUN_0052f330`（decompiled L225369 `// FUN_0052f330 @ 0x52f330`、db 名 Topic_SortByPriority））。出口 5 本すべて `ret 0x18`（0x52F035/0x52F107/0x52F2C8/0x52F2D6/0x52F32D）、分岐 60 本すべて内部ターゲット、外部ジャンプ無し。呼び出し 7 本すべて内部。
- **旧 findings の訂正（重要）: decompiled.c に FUN_0052f010 ラベルは実在する** — コメント L225202 `// FUN_0052f010 @ 0x52f010`、定義 L225204-225206 `int __thiscall FUN_0052f010(int param_1, undefined1 *param_2, ... int param_6, int *param_7)`、本体 L225208-225365。「Ghidra が見逃し＝ラベル無し」という従来記録は抽出バグ由来: (a) dec_index の定義行 regex が `FUN_...(` で始まる行を拾わない、(b) `enclosing_fun` が case-sensitive `FUN_` で lowercase キー `fun_0052efe0` に一致せず失敗 → 全 caller の「def=?」も本バグの産物。
- **旧 findings の訂正その2: 呼び出し元 0x52F0EE は fun_0052efe0 内ではなく 0x52F010 自身の中**（自己再帰）。`.text` 全域 E8 スキャンの結果、0x52F010 の呼び出しは **4 ブロック**: 0x52F0EE（自己再帰）+ 外部 3 本 = **0x52F785（Topic_EvaluateAny）/ 0x52F7D3（Topic_FindOrCreateConversation）/ 0x53003B（fun_00530020）**。E9 ジャンプ 0 本、絶対 imm32 参照 0 本。
- **旧 findings の訂正その3: 「rand なし / idiv なし」は半分正しい。** `idiv` は無いが **`0x52F2F7 div esi` が実在**し、直前の `push 0; call 0x47DF80`（= `_rand()` ラッパ、time seed; decompiled L100820）→ `xor edx,edx; div esi` → `add edx,1` → 保留リスト走査で n 番目を返す、つまり **`rand() % count + 1` 選択はこの関数内にある**（Ghidra の decompiled も L225274-225277 `uVar7 = FUN_0047df80(0); iVar9 = uVar7 % uVar10 + 1;` で一致）。ただし即時 return 経路（最初の合格候補を返す）が主で、抽選は「保留候補が積まれた場合」の経路。

本体の構成（先頭部はアセンブラと decompiled が逐語一致）:

| 項目 | 内容 |
|---|---|
| プロローグ | `sub esp,0x14` / `mov eax,[esp+0x18]`（第1スタック引数 = out byte*）/ `mov ebp,ecx`（this）/ `mov byte[eax],0`（out 初期化） |
| 早期退出 | `mov ecx,[ebp+8]; shr ecx,5; test cl,1` → **bit5 SET なら `ret 0x18` で return 0**（無効化フラグ; decompiled L225227-225229） |
| 保持子初期化 | `DAT_00b36510 == NULL` なら `push 8; call 0x401F00`（malloc ラッパ）で 8B `{dword, head}` を遅延確保・ゼロ化（0x52F049）／非 NULL なら `call 0x4526E0` でノード全解放＆先頭ゼロ（リストクリア; decompiled L63215 `FUN_004526e0`: `while (param_1[1]) { free head; follow +4 } *param_1 = 0;`） |
| リスト走査 | `this+0x28` の Topic リストを辿り、Conversation ごとに `call 0x530830` = **Topic_CanUse**（db 名、0x52F186）で条件判定 |
| 候補処理 | 条件通過時 `bit1 of [cand+0x25] == 0` → 即 return；`bit1 set` → `call 0x446CB0`（0x52F270）で保留リストへ追加。`bit5 of [cand+0x25]` → priority-stop（走査打ち切り）。`cand+0x30` リスト・`call 0x46CB60`（0x52F1DD）で追加照合 |
| 抽選 | 走査終了時 保留リストが非空 → リスト長を数えて `rand() % len + 1` 番目のノードを return（0x52F2D9-0x52F32D）／空 → `local_8`（最後の合格候補）を return |
| 再帰 | ガード `DAT_00b333c4+0x116` が立っていて `this+0x24 ∈ {2,3,4,6}` かつ param5≠0 ならガードを 0 にして `call 0x52F010`（0x52F0EE）→ 復元 → return（L225258-225264） |
| 署名 | thiscall（ecx = this）+ 6 スタック引数 `ret 0x18`。caller 側 push 数は全 4 経路で 6 と一致。decompiled 呼び出し: L225768 `FUN_0052f010(param_1,param_2,param_3,0,0,0)` / L225794 `FUN_0052f010(&param_4,param_1,param_2,1,param_3,param_4)`（&param_4 = out byte*）/ L226296 / L225262（再帰） |

グローバル `DAT_00b36510`（.bss ポインタ、8B 保持子）の参照は `.text` 内 9 箇所: 0x52F010 内 6 箇所（0x52F03A/061/067/253/2A1/2FB）+ 0x530142/0x53016B/0x53017A（別関数、decompiled L226370 でも出現）。

### 判定 — 0x50A050 サニティ

- **関数開始としての成立は確認**（ただし「手前に int3 パディング」という V69 前提は**不成立**）:
  - 手前 16B = `83 c4 08 5f 5e b0 01 5b c3 b0 01 5b c3` → 前関数の 2 連続エピローグ（`... pop esi; mov al,1; pop ebx; ret` ×2）で前関数の最終 `ret`@0x50A04F が 0x50A050 に**正確に接続**。**直前の int3 は 0 バイト。**
  - 0x50A050 は新規コード開始: `mov ecx,[0xb333c4]; cmp byte ptr [ecx+0x588],0; je 0x50a0fb`。
  - 終端: `ret`@0x50A11A → **int3 ×5**（0x50A11B-0x50A11F）→ 次関数 0x50A120（`fldz` 始まり、名無）→ int3 ×9 → 0x50A150 = **`Script_Enable`**（db 名）。つまり関数範囲 = **[0x50A050, 0x50A11B)**。
- **両ライトはともにこの範囲内**: 0x50A075 `mov byte ptr [eax+0x588], 0`（V69 の +0x588 クリア）✓、0x50A0FB `mov byte ptr [ecx+0x588], 1`（0x50A0FA ret の後 = 別分岐ターゲット）✓。
- **呼び出し方式: 直接 E8 呼び出し 0 本 / E9 0 本 → .data テーブル経由の間接呼出。** 絶対参照は 1 本のみ: file off 0x70A330 = **VA 0xB0B730（.data、レコード幅 0x28）**、レコード内容 = `{"Show1stPerson", "S1ST", id=0x113, "Show the 1st person Model from the 3rd person camera. If in 3rd person mode it will show both.", 0, 0, fn=0x50A050, fn2=0x4FDE30}`。
- **結論: V69「0x50A050 = Show1stPerson setter」は確認**（テーブル名まで一致）。int3 パディングの根拠のみ差し替え = 「前関数のエピローグ終端 + 後続 int3 ×5 + テーブル参照」。

### 手法

- ハーネス `probe_harness.py`（`Ctx.rd/cstr/text_imm32/dec_search/dec_index/dec_lines`）+ capstone 5.0.7（32bit 線形デコード、window [0x52F010,0x52F330) / [0x50A050,0x50A649)）。実行ゲートは selftest = ALL PASS。
- PE ヘッダを struct で直接解析し `.text`（VA 0x401000 / raw 0x400 / 0x626E00 B）と `.data`（VA 0xB02000 / raw 0x700C00）を特定 → **.text 全域を E8 / E9 rel32 スキャン**（ターゲット 0x52F010・0x50A050、disasm 2 命令で検証）と**ファイル全体の絶対 imm32 スキャン**（リトル endian 4B）。
- decompiled.c を行範囲読込: FUN_0052efe0 L225177 / **FUN_0052f010 L225204-225365** / FUN_0052f330 L225371（境界確認）/ FUN_0047df80 L100820（RNG）/ FUN_004526e0 L63215（リストクリア）/ 呼び出し L225262・L225768・L225794・L226296。
- annotate_db は `{"functions","structs","constants"}` の 3 キー構造（旧 probe は `entries` 前提で 0 件 → 本回で修正）。0x52F010=Topic_Resolve / 0x52F330=Topic_SortByPriority / 0x52F770=Topic_EvaluateAny / 0x52F790=Topic_FindOrCreateConversation / 0x530830=Topic_CanUse / 0x50A150=Script_Enable を取得。0x4526E0・0x401F00・0x401F20・0x446CB0・0x46CB60・0x47DF80・0x50A050 は db 名無し（null）。
- 0x50A050 は context バイト列を dwords に分解し、前後レコード（id 0x113 / 0x114）の stride 0x28 からフィールド位置を確定 → 文字列ポインタを `cstr` で解決。

### 残課題

- `FUN_0047df80` は decompiled で `void` 型（返り値なし扱い）だが実体は `_rand()` ラッパで EAX が生き残る（Ghidra 型付けの限界）。rand 判定は asm（`div esi`）と decompiled（`% uVar10 + 1`）の一致で裏付け済みだが、返り値型の明示は未修復。
- db 名が付いていない: 0x4526E0（リストクリアと確定）、0x401F00（malloc ラッパと確定）、0x446CB0（保留候補への追加と推定）、0x46CB60（状態/条件確認と推定）、0x530020（外部 caller の 1 つ）、0x50A120。
- `fun_00530020` の上下文と、`param_6` / `param_7`（`cand+0x30` リスト照合）の正式な意味は未確定。`[cand+0x25]` bit1/bit5、`[this+8]` bit5、`[this+0x24]` 型値の正式フィールド名は未確定。
- `DAT_00b36510` の 3 箇所（0x530142/16B/17A、decompiled L226370）を使う側の関数が未特定。
- 0x50A050 の dispatch テーブル全体（VA 0xB0B730 近傍、stride 0x28、id フィールドの意味）は未走査。id 0x113 の正式な意味（オペコードかコマンド索引か）は未確認。
