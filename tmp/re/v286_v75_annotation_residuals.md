# v286_v75_annotation_residuals.md — v60v76-bundle 依頼4 [P4]: v75a/v75b アノテーション・来歴残件（2026-11-04）

- 依頼元: `tmp/re/handoff_20261105_v60v76_residuals_bundle.md` 依頼4 [優先度 P4]（出典 `v75a_findings.md` / `v75b_findings.md`）
- 対象: Oblivion.exe v1.2.0416 (Steam) / Oblivion.esm 277,504,985 B
- ハーネス: `probe_harness.py selftest` → **4/4 PASS**（本セッション実行）
- **判定: 7点中 5点 [確定] / 2点 [部分確定]**

---

## 1. v75a N12「132/39」「124/37」の来歴 → **[部分確定] 132/39 は特定済み・124/37 は一次記録喪失**

| 数値 | 来歴（特定結果） |
|---|---|
| **132 / 39** | **Supermemory 旧エントリ `bUP3gq7DeSyw53Mdw49kmY`**（「REFR+0x58 統合」エントリ）に記録された計測値 **`vt+0x284=132回` / `vt+0x288=39回`**。出典は `tmp/re/memory_contradictions_20261024.md:34` の引用行（「統合エントリで保持: …旧 bUP3 の計測値…」）。**当該エントリは同調査で forget 処理済み**（同 L36「処置: …bUP3gq7DeSyw53Mdw49kmY（統合で吸収）を forget」）→ **計測手順の一次記録は失われ、引用経路のみ残存**。 |
| **124 / 37** | **一次出典がリポジトリ・Supermemory ともに残存せず → 出所特定不能**。初出は `tmp/re/contradictions_TU_verify_20261024.md:53`（N12 行の「vs」相手）のみで、それ以前の母資料なし（`plan.md` / `docs/*` / `GT_CHEATSHEET` / 両 session-memory とも該当 0 件）。以降 v75a・TODO へ転記された二次引用のみ。 |

- **数字の意味（基準）**: 対象は **`vt+0x284` / `vt+0x288`（vtable/オブジェクト displacement）の使用回数**。いずれの方式でも .text 全体では再現不可の**過小計**（v75a が全 .text capstone 2,257,987 命令で実測: 妥当値 **A_object = 344（0x284）/ 126（0x288）**、B_stack = 38/25、`call [mem+…]` = 0）。**両数値は使用不可**。
- 判定: 来歴のうち 132/39 はエントリ ID・forget 履歴まで特定 → **部分確定**。124/37 は記録喪失により特定不能（＝これ以上追跡不可、v75a の A=344/126 が正）。

---

## 2. v75a 小a「band-6 vs total-7」の原典照合 → **[確定] 両方正しい（対象が異なる）**

原典 = `docs/RE_EXE_REPORT.md` **L11602**（§68）:

> 「新規 FunctionID は **0x009A〜0x009F の 6 件**を割り当てる。`native_scda_decoder.cpp` は**全 7 opcode** の名前を返却済み、`native_scda_bridge.cpp:33` に `{0x1037, FunctionID::StopQuest}` が既存。」

- **total-7 = Phase-68 の新規 opcode 7 件**（StopQuest 0x1037 / Evp 0x105E / PlaySound 0x1026 / ModCrimeGold 0x1076 / SetCrimeGold 0x1075 / KillActor 0x108B / ResurrectActor 0x108C）。
- **band-6 = うち新規に割り当てた FunctionID 6 件 = 0x009A〜0x009F**（7 つ目の StopQuest は **既存 FunctionID** を使うため帯域外）。
- 整合の旁証: `RE_ANALYSIS_ROADMAP.md` L256「§68 …追加仕様 **7 件** (StopQuest 0x1037 / … / 新規 enum 0x009A〜0x009F)」＝同じ 7 件/6 帯の構造。実装ログ `STATE.md` L87（2026-11-04）「**six** Phase-68 handlers … FunctionID 0x9A-0x9F, **V75a band canon**」＝実装側も6件で整合。
- → 「7」と「6」は**どちらも正**（新規 opcode 数 vs 新規 enum 数）。v75a の「帯域内6件・7つ目 StopQuest=0x7A が帯域外」は原典と一致。

---

## 3. v75b「QUST 388 / INFO 1,700 の証跡なし」→ **[確定] ESM 全体再走査で 9/9 全一致（新規 artifact）**

`v286_p4c.py`（本タスク新規）で **GRUP 感知の ESM レコードウォーク＋SCDA 命令ウォーク**を全数実施:

- 手法: レコードヘッダ20B / GRUP 20B（`dataSize==0` の1件を数える）/ 圧縮レコード（`flags&0x40000`）はサブレコード走査スキップ（QUST/INFO/SCPT は非圧縮 = plan.md の保証どおり）/ SCDA 内は `[u16 op][u16 argLen][args]`、**Marker 0x001C は固定4B**（`s57_svm_5_bytecode.py` の規則を踏襲）。

| 検証項目 | 本タスク実測 | 期待値（plan.md / 既知 artifact） | 結果 |
|---|---|---|---|
| records | **1,167,017** | 1,167,017（plan.md:464） | **OK** |
| GRUPs | **85,079** | 85,079（同） | **OK** |
| zero-size records | 1 | 1（STAT 0x0008050C） | **OK** |
| walk 終端 | leftover_stack=0（余剰0） | zero remainder | **OK** |
| SCDA blocks | SCPT **2,393** / INFO **5,552** / QUST **1,701** | 同（plan.md:464） | **OK** |
| SCPT 命令数 | **65,628** | 65,628（s59 ヘッダ） | **OK** |
| SCPT SetStage | **1,391** | 1,391（scda_opcode_table_out.txt:39） | **OK** |
| 総命令数 | **86,916** | 86,916（plan.md:502） | **OK** |
| 総 SetStage | **3,479** | 3,479（plan.md:521） | **OK** |
| **QUST SetStage** | **388** | 388（plan.md:546） | **OK** |
| **INFO SetStage** | **1,700** | 1,700（同） | **OK** |

- 追加の整合: INFO 内比率 = 1,700 / 10,962 = **15.51%**（plan.md:548 の "15.51% of all INFO instructions" と一致）。SCPT 独立 opcode 数 = **141**（scda テーブルヘッダ `141 distinct opcodes` と一致）。
- **QUST 命令数 10,326 / INFO 命令数 10,962** も本タスクの新規確定値。
- → 「証跡なし（plan.md prose のみ）」は**解消**。本 probe が独立 artifact。

---

## 4. 紛らわしい数字 50 / 987 / 220 の出所整理 → **[確定]**

| 数字 | 単位 | 出所（実測確認） |
|---|---|---|
| **50** | **SetStage で「最終命令」になる SCPT の本数**（STOP 欠落系） | `tmp/re/copilot_archive/scda_opcode_table_out.txt:158` の "last instruction of each script" セクション `0x1039 50` |
| **987** | **名寄せコーパス上の SetStage ソース文（センテンス）数** — 命令発生軸ではない | `plan.md:473`。`docs/RE_EXE_REPORT.md` L12561（§73）でも「原文センテンス数（命令発生軸ではない）」と正典化済み |
| **220** | **opcode テーブルのエントリ通し番号（0-based index）** — 出現数ではない | `tmp/re/s57_svm_8_opcode_table.txt:39` の列（`0x1036→217` / `0x1037→218` / **`0x1039→220`** / `0x1052→245` / `0x1053→246`）と `s69_scda_b.md:157` の2列目 `| 0x1039 \| 220 \| SetStage \|` に出現。連番式 **`(opid & 0xFF) + 163`** が 5 行すべてで一致（0x1036→54+163=217 ✓）→ **表の行番号** |

---

## 5. annotate_db 名前欠落 5 件の関数名・役割 → **[確定] 5/5**

`tmp/re/annotate_db.json`（functions 368 件）に**5件とも不在**を確認（lookup 0 件）。役割は decompiled＋caller＋テーブルで特定:

| VA | 判定 | 根拠 |
|---|---|---|
| **0x4526E0** | **リストクリア（命名案 `List_Clear`）** — head を辿って `FUN_00401F20`（free）で全解放し `*list=0`。**callers 240**、本文18行 | decompiled: `while (param_1[1]) { iVar1 = param_1[1]->next; FUN_00401f20(param_1[1]); … } *param_1 = 0;`。正典名なし（§69 の Cleanup 0x4FA020 は別関数） |
| **0x446CB0** | **`Push`（汎用リスト挿入）— 正典で既に同定済み** | `docs/RE_EXE_REPORT.md` **L11840**「\| Push \| 0x446CB0 \| +0x00 / +0x04 \| **347** \| 汎用リスト挿入 \|」・L11880「347 callers の汎用リスト挿入」。本文: `malloc(8)`→ノード生成→head/tail 保守。**annotate_db 欠落だが正典に記載あり** |
| **0x46CB60** | **リスト空判定（命名案 `List_IsEmpty`）** — head/tail とも 0 なら 1 を返す。**callers 96**、本文12行 | `if ((param_1[1]==0) && (*param_1==0)) return 1; return 0;`。Topic_Resolve からも 0x52F1DD で呼ばれる |
| **0x530020** | **Topic_Resolve の薄いラッパ**（結果マスク付き） | disasm 全文: `push ecx`(out 用ローカル) → `push 0,0,0 / push arg2 / push arg1 / lea out` → `mov byte[out],0` → **`call 0x52F010`** → `neg/sbb/not` で **out フラグ立時のみ結果を 0 マスク** → `ret 8`（**2 スタック引数**）。caller **1 件 = `FUN_006B8E00`**（結果構築系: `*(param_3+0xC)==0xD7` 判定を含む78行関数） |
| **0x50A120** | **`ScriptEffectUpdate` の実装** — **.data のオペコードテーブルが正式名を保持** | abs32 参照 = `.data` 1 件（VMA **0xB0B240** の fn フィールド、fn2=0x4FDE30）。レコード内容 `{"ScriptEffectUpdate", "", id=0x13, …, fn=0x50A120}`。本文 [0x50A120,0x50A147) 48B: `fldz`→出力 double に 0.0 を書込み、引数 obj が非null かつ `byte[obj+0x29]!=0` なら **1.0**、常時 `al=1` を返す **predicate**（`ret` は 0x50A143/0x50A146、以降 int3×9 → 0x50A150=Script_Enable） |

---

## 6. `fun_00530020` の引数意味 → **[部分確定]**

- **thiscall・スタック引数2本（`ret 8`）**。呼び出し形: `Topic_Resolve(this, out_byte*, arg1, arg2, 0, 0, 0)`（this は ecx のまま転送、out はローカルの byte、第4〜6 は定数0）。
- **arg1 / arg2 = Topic_Resolve の第1/第2スタック引数へそのまま転送**（本ラッパ自身の解釈は行わない）。戻り値は「**out フラグが立っていたら 0 にマスク**」（`neg cl; sbb ecx,ecx; not ecx; and eax,ecx`：フラグ0 → 結果そのまま / フラグ≠0 → 0）。
- caller は `FUN_006B8E00` の1件のみ（会話/トピック結果の構築系）。**arg1/arg2 の正式な意味名は Topic_Resolve 側の未確定フィールド名（`[this+0x24]` 型値・`[cand+0x25]` bit 等）に依存** → そこまでは特定せず。

---

## 7. `DAT_00b36510` のユーザー（正典 NO HIT）→ **[確定] 2 関数・9 サイト**

`.text` abs32 参照 = **9 件**（正典 docs には記載なし → 本レポートが初記載）:

| 関数 | サイト数 | 役割 |
|---|---|---|
| **`FUN_0052F010`（Topic_Resolve）** | **6**（0x52F03A/061/067/253/2A1/2FB） | 8B 保持子 `{dword, head}` の遅延確保（`malloc(8)`）・クリア（`call 0x4526E0`）・保留候補リストの走査/抽選（v75b §2 で確定済み） |
| **`FUN_00530050`** | **3**（0x530142/16B/17A） | **保持子の解放者**: L69-83 `piVar6 = DAT_00b36510; … FUN_00401f20(DAT_00b36510); DAT_00b36510 = …`（先頭を free して全体解放）。関数は vtable 2本設定（`PTR_FUN_00A5555C` / `param_1[6]=PTR_FUN_00A55548`）→ 要素走査・カウンタ減算（`*(iVar2+0x14) -= 1`）→ 保持子解放 → `param_1[7]` free → u16 フィールド群更新 → `FUN_0046C280` → ret（101行）。**caller 1 件 = `FUN_00530340`** |

- 判定: `DAT_00b36510` は **Topic/会話系の「保留候補保持子」で、作成・走査は Topic_Resolve、破棄は `FUN_00530050`（後始末系）**。正式クラス名は未特定 = **部分確定**（正典 NO HIT は解消）。

---

## 残点（記録）

1. `124/37` の一次出典（記録喪失のため追跡不能 — v75a の A=344/126 を正として扱う）。
2. `FUN_00530050` の正式なクラス名（dtor か reset かの区別）、`FUN_006B8E00` の arg 体系。
3. `0x530020` の arg1/arg2 の意味名（Topic_Resolve 側フィールド名の確定に依存）。
4. `ScriptEffectUpdate` の `byte[obj+0x29]` フィールドの正式名（ActiveEffect 系か）。

## 証跡（すべて `%TEMP%\opencode\` または `tmp/re\`）

| スクリプト / 入力 | 内容 |
|---|---|
| `v286_p4.py` + `_out.txt` | annotate_db lookup（5件 0ヒット）/ 名前欠落5件の本文・caller / FUN_00530020 / DAT_00b36510 refs |
| `v286_p4b.py` + `_out.txt` | 0x50A120 disasm＋.data abs32（→VMA 0xB0B240 テーブル解決）/ FUN_00530050 の保持子解放行・caller |
| **`v286_p4c.py` + `_out.txt`** | **ESM 全体 GRUP-walk ＋ SCDA 命令ウォーク（QUST 388 / INFO 1,700 の新規再検証、9/9 一致）** |
| `tmp/re/s57_svm_8_opcode_table.txt` / `s69_scda_b.md` | 220 = 表エントリ通し番号の確認 |
| `tmp/re/copilot_archive/scda_opcode_table_out.txt` | 50 / 1,391 / scripts 2393 の出所 |
| `docs/RE_EXE_REPORT.md` L11602 / L12555-12564 | Phase-68 7件 vs 6件の原典 / §73 SetStage カノン |
| `tmp/re/memory_contradictions_20261024.md` L34, L36 | 132/39 = bUP3 エントリ（forget 済み）の引用 |

検証日: 2026-11-04。ハーネス: `probe_harness.py selftest` 4/4 PASS。内部整合: ESM records/GRUPs = plan.md と一致 / SCDA blocks 3,5,552+1,701+2,393=9,646 ✓ / 命令 65,628+10,326+10,962=86,916 ✓ / SetStage 1,391+388+1,700=3,479 ✓ / INFO 比率 15.51% ✓ / opcode 数 141 ✓。
