# v193_subagent_supplement.md — opcode/GT slot 残点 第4束（サブエージェント分析補遺）

- 日付: 2026-10-29 / 作成: opencode サブエージェント（TODO serial 181 / V181・中）
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（Steam v1.2.0416, ImageBase 0x400000）
- **位置づけ**: 本ファイルは `tmp/re/v193_opcode_gt_residuals.md` の**代替ではなく補遺**。同ファイルは本セッション作成途中に**親セッションが直接解析で上書き**しており、制約（既存 `tmp/re` 上書き禁止 / repo 内新規 1 件制）に従い**当該ファイルは変更していない**。本補遺は親セッション版に対して **追加・訂正** を提供する。
- selftest: 着手時・完了時とも **4/4 PASS**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl=0x504560）
- 手法: capstone 5.0.7（`v172_common.py` 経由: dump / e8_callers / imm32_refs3 / raw バイトスキャン）＋ `docs/Oblivion_decompiled.c`（`probe_harness.Ctx` 索引）
- 中間物: `%TEMP%\opencode\v193_p1..p20.py` / `v193_p*_out.txt` / `v193_opid29_detail.{py,csv}`
- 制約遵守: git / docs / GT / TODO / session-memory / plan / cache / 台帳 / 既存 `tmp/re/*.md` は**無編集**、ESM 本体へは**未アクセス**

---

## A. 親セッション版との差分（先に読むこと）

| 項目 | 親版（`v193_opcode_gt_residuals.md`） | 本補遺 | 裁定 |
|---|---|---|---|
| r2 クラス命名 | 「命名は未（RTTI 無し）」＝残点9 | **`TESSaveLoadGame`（実文字列 5 件）** | **本補遺で解消** |
| r2 ctor caller | `FUN_00441970` 内 `push 0x1C4/malloc` | 同。加えて **vtable `0xA37470` = `.?AVTES@@` → `TES::TES()`**、caller `0x40662D` → `[0xB333A0]` | 補強 |
| r2 bit0/bit18 setter | `FUN_0045A530`（ret 4, bool 引数, callers 21） | **`FUN_00465860` にも同一対のセットあり**（`0x4667CA/D0` クリア、`0x4667F8/FE` セット、`0x45B9A0` ストリーム書出しの前後） | **両方あり**（親版は enter/leave 用、本補遺は I/O 前後用）＝併記が正 |
| r2 `[+0x7C]` | 「formType 0x7C」を踏襲 | **セーブ・バージョン番号**（`"Current version is %i"` が `[+0x7C]` を実引数） | **本補遺で要訂正** |
| r4 opcode 0x1C | 「%変数参照フェッチ」＝0x1C が本体、`[out]` へオブジェクト格納 | **0x1C は prefix**：fetch が **opcode-out を i16#2 で上書き**（`0x5168CB mov [ebp],ecx`、ebp=`param_3` をスタック逆算で実証）→ 0x1C は executor に届かない。命令長 8B（`[0x1C][X][opcode][arg]`） | **本補遺が新証拠**。親版の「0x1C が本体」は要修正 |
| r5 `HorseStolen` | 「abs32 0 件 → デバッグ/未使用リテラル」 | **`0xA4E50E` = `"IsHorseStolen"`(0xA4E50C) の末尾**。実参照は `.data 0xB0F4D0`＝**オペコード表 idx445 行の name ポインタ** | **親版の判定は誤り** |
| r5 `0x426A90` | 「警報報道」（v178 踏襲）、callers 2 件 | **`ExtraDataList` の Save/Load**（本文に `"LoadGame Buffer error…"` `..\\TES Shared\\ExtraDataList.cpp` line `0x1B6F`、`0x4534D0` で 4B 読出） | **要訂正**（callers 2 件は保存系として自然） |
| r5 書手 4 件 | 4 件とも有効と仮定 | **`0x60FC54` の所在関数は到達不能**（E8 caller 0・imm32 参照 0・Ghidra 関数登録なし）→ 実質 3 件 | **本補遺で縮小** |
| 177a node+8 | 「`FUN_0045C4F0`/`FUN_004DD1A0` が store＝destCell」 | **`CopyListForReference` が `[node+8]` を「連結先頭/次」として走査**（`esi=[edi+4]` 起点、`esi=[esi+8]` 推進）＝**全 extra 共通の next リンク**。destCell は type0x14 に無し（type0x53 の +0xC 側） | **要仲裁**（下記 §6） |
| 177b | MagicItem slot6=`0x982D85`(pure-call) → 派生 override が [残] | **全 1715 vtable 行の slot6 を走査 → `mov eax,1/4; ret` は 0 件**。派生の主 vtable（SpellItem/EnchantmentItem 55slot）slot6 = `0x418F10`＝dtor 系 → `0x46C2F0`→`0x46B170`（formType switch）。**「slot6=戻り値1/4」説は再現不能** | **親版も本補遺も未解決**（負の証拠を追加） |
| 177c | 3 reader の構造確定・フラグ名 [残] | 加えて **bit21 writer 検出**（Player LoadGame で known MGEF に `\|=0x200000`）、`or [reg+0x58]` の bit7/bit31 **書手 0 件**、**`FUN_004AE590` = bit30 述語**（bit31 の対） | 補強 |
| 177d | 「byte writer は FUN_0064B400 初期化 1 件のみ・非保存」 | **`FUN_006564C0` の `0x65698E mov byte [esi+0x11c], al`（ロード時復元）を実測** → **「セーブ非保存」は誤り**。加えて byte 書手 0/2/reg を追加検出 | **親版・v177 追補とも要訂正** |
| 177d `+0x11D=5` | 値域に含めたまま | **`+0x11D` の decompiled 代入は全 4 件のみ（0 / コピー / 0 / `(cond?9:4)`）→ 値 5 の書手は不在**。`FUN_00654F10` の `5/10` は **`vt+0x370` へ渡す code 引数**であって +0x11D への書込ではない | **v177 の「5=一時値」は未証・誤読候補** |
| 177e | 「alloc 直書き 0 件 → 挿入ヘルパ経由 [残]」 | **setter 3 本と元素型を特定**（§10） | **大幅進展** |
| 29 opid | 分類表のみ | **29 行の prologue/ret/argdesc/flagA を CSV 化**＋重点 2 件の全文 | 補強 |

---

## B. 【V-178-r2】クラス命名 = `TESSaveLoadGame` — **解消**

### B.1 型名実文字列（5 件、v193_p10）

| VA | 文字列 |
|---|---|
| `0xA3A4E0` | `Error: TESSaveLoadGame::SaveGameData() was called while loading.\n` |
| `0xA3A82C` | `TESSaveLoadGame::SaveQueuedHavokData` |
| `0xA3A854` | `TESSaveLoadGame::SaveQueuedAttachedAnimation` |
| `0xA3B06C` | `TESSaveLoadGame::LoadForm` |
| `0xA3B0B8` | `TESSaveLoadGame::UnloadForm` |

既存資料との整合（命名は本プロジェクト初出ではない）: `docs/RE_EXE_REPORT.md`「セーブリスト単体 `DAT_00b33b00`（セーブマネージャのシングルトン）」/ `tmp/re/s58_save_1_raw.txt`「find_dword 0xB33B00 (save manager singleton)」/ `tmp/re/b01_counter.md`「`g_pSaveLoad(0xB33B00)` の thunk → `0x45B9A0`」。

### B.2 ctor caller と起動経路

```
0x40662D (fun_0040xxxx 起動部): malloc(0xAC) → mov ecx,eax / call 0x441970 → mov [0xB333A0],eax (g_pTES) → call 0x43F560
FUN_00441970: *param_1 = &PTR_LAB_00A37470  ← vtable 0xA37470
             DAT_00b3f580=1 / FUN_005350F0 / FUN_00537760 … zero-init …
0x441BB0   : call 0x45B300 (TESSaveLoadGame ctor)
```
**`cache/vtable_index.tsv` #1252: `TES  .?AVTES@@  0x00A37470`** ⇒ **ctor 唯一 caller `0x441BB0` = `TES::TES()`**。

### B.3 `[+0x18]` bit0/bit18 セッター（v193_p9/p10）

`imm32_refs3(0xB33B18) = 0 件`（コードは `[0xB33B00]` をロードしてから触る）。`.text` の `or dword [reg+0x18],imm` 候補 209 件のうち前方 0x40B に `[0xB33B00]` を持つ 7 件を抽出 → **bit0/bit18 を扱うのは `FUN_00465860` の 2 連続サイト**:

```
0x4667B4 mov edi,[0xA2808C] (GetCurrentThreadId IAT)
0x4667BC mov eax,[0xB33398] / 0x4667C1 mov esi,[eax+0x10]  ← メインスレッド ID キャッシュ
0x4667C4 call edi / cmp eax,esi / jne 0x4667d0
0x4667CA and dword [ebp+0x18], 0xFFFFFFFE   ← メイン: bit0 クリア
0x4667D0 and dword [ebp+0x18], 0xFFFBFFFF   ← 他:     bit18 クリア
0x4667D7 push 4 / lea ecx,[esp+0x54] / push ecx
0x4667DE mov ecx,[0xB33B00] / 0x4667E4 call 0x45B9A0   ← 4B ストリーム書出し
0x4667E9 … GetCurrentThreadId 再比較 …
0x4667F8 or dword [ebp+0x18], 1             ← メイン: bit0 セット
0x4667FE or dword [ebp+0x18], 0x40000       ← 他:     bit18 セット
```

- `this` 検証: `FUN_00465860` 呼出し元 5 件中 **3 件で `mov ecx,[0xB33B00]` 直近**（`0x5025EA`/`0x5AE1DB`/`0x5B5D90`）。本文が触る `[ebp+0x14/0x18/0x40/0x50/0xA9]` は**すべて ctor `0x45B300` の零クリア集合と一致**（`mov byte [esi+0xA9],bl` 等）。`FUN_00465860` は `GetTickCount()` 使用・`*(param_1+0xA9)=1`。
- `0x45A500` の `this` 再検証: 直接呼出し 118 件のうちサンプル 14 件すべて `mov ecx,[0xB33B00]`。
- ⇒ **親版の `FUN_0045A530`（enter/leave 型 setter）と本補遺の `FUN_00465860`（I/O 前後型 setter）は別関数で併存**。両者が bit0/bit18 を対にしており、述語 `0x45A500` はそれらの読手。
- 他ビット書手（本文）: `0x4443A0` `|=0x100`(bit8) / `0x45CC0B`,`0x45DDC9` `|=0x10000`(bit16) / `0x4B7D98` `|=0x8000`(bit15) / `0x5439CE` `|=0x400`(bit10)。

### B.4 `[+0x7C]` = セーブ・バージョン（v178 の「formType byte」を訂正）

`FUN_00426A90` 内（v193_p9）:
```
0x426F72 mov ecx,[0xB33B00] / push 4 / lea edx,[esp+0x80] / push edx / call 0x4534D0
L28397 FUN_004a7a60("LoadGame Buffer error: Block Header is incorrect in file %s on line %i.  Current version is %i",
                    "..\\TES Shared\\ExtraDataList.cpp", 0x1b6f, *(byte*)(DAT_00b33b00 + 0x7c))
```
`FUN_0045A170` の `0x20..0x5A` 判定・`L27112 [+0x7C]=[+0x71]`・`0x45A170` が「セーブ互換レンジゲート」とも整合。⇒ **`+0x7C` = 現行セーブデータ・バージョン番号** [有力]。

---

## C. 【V-178-r3】`0x4FBE00` レート制御 — 親版と一致（詳細を追加）

decompiled L195449-195499 ＋ asm 0x4FBE00-0x4FBEF8（v193_p7）:

| 要素 | 語義 | 証拠 |
|---|---|---|
| force 引数 | `mov ecx,[esp+0x14]; test cl,cl` → 強制実行 | 0x4FBE03 |
| `[this+0x20]` | 実行可能状態カウンタ。`force==0 && ≤4` → return 0 | 0x4FBE0B `cmp [esi+0x20],4 / jbe` |
| `[this+0x28]` | レート制御有効 byte。0 なら gate・再アームとも skip | 0x4FBE11 / 0x4FBE74 |
| `[this+0x38]` | 残り遅延 f32：毎フレーム `[0xB33E9C]`(frame dt) 減算 | 0x4FBE26 `fsub [0xB33E9C]` |
| `[this+0x3c]` | 待機累積 f32：同 dt 加算、実行後に **0 リセット** | 0x4FBE32 / 0x4FBEE9 |
| 実行 | `0x516970`（atexit 付き遅延初期化 → `&DAT_00B36208`）→ `0x517810`(RunScript) → `bl=al` | 0x4FBE68/0x4FBE6F |
| 再アーム | `FAA90(this,"fQuestDelayTime"@0xA49E80,&id)` → id 無しなら `[0xB09E28]` 加算 / `FAA110(owner,id)` 値 `> [0xA2FAA8](0.0f)` なら値加算、否则 `[0xB09E28]` 加算 | 0x4FBE8B-0x4FBEE3 |
| `[0xB09E28]` | **既定遅延秒（実測 5.0f / 0x40A00000）**。所有 FUN = `FUN_004FBB60`（setter 候補） | v193_p7 r3-3 |
| `[0xB33E9C]` | **frame dt**（エンジン全域で dt として使用、L5695/L9894/L326773…） | decompiled 横断 |
| `[0xA2FAA8]` | **0.0f**（fQuestDelayTime の無効しきい値） | v178 自己検証 #14 と一致 |
| 後処理 | `call 0x4FB430(this)` → `mov al,bl` | 0x4FBEEE |

**注意（v178 訂正候補）**: `[this+0x3c]` は「**ディレイ待機に費やした累積時間**」であり v178 §6 の「実行時間累積 `[script+0x34]`」（QPC 計測）とは**別系統**。移植時は区別すること。

---

## D. 【V-178-r4】fetch opcode `0x1C` = **参照オブジェクト接頭辞（prefix）**

### D.1 スタック逆算で out-param を確定（asm + decompiled 一致）

`ret 0x18`（6 スタック引数: param_2..param_7）:
```
entry: param_2=+4  param_3=+8  param_4=+0xC  param_5=+0x10  param_6=+0x14  param_7=+0x18
0x516830 push ecx → 0x516831 mov eax,[esp+0x14] = param_5 → mov [eax],0     ; *arg_out = 0
0x516835 push ebp → 0x516836 mov ebp,[esp+0x10] = param_3                     ; ebp = &opcode_out
0x516840 push esi → 0x516841 mov esi,[esp+0x18] = param_4                     ; esi = &PC
0x516855 mov [ebp],ecx        ← 第1 opcode を opcode_out へ
0x51685B cmp dword [ebp],0x1C / jne 0x51694b
0x51686E movsx ecx,word[...]  ← i16 #1（参照 index） / PC+=2
0x516883 jne 0x5168c2         ← param_7≠0 なら解決スキップ
0x51688C call 0x4FA9C0        ← 参照レコード解決
0x516895 cmp dword [eax+0xC],0 / seta byte [esp+0x18]   ; ★ bVar6 = rec[+0xC]≠0
0x5168B4 call 0x9832E6        ← 名前 → form 解決 → *param_6 へ
0x5168CB mov [ebp],ecx        ← ★ i16 #2 を opcode_out へ上書き
0x5168DA cmp [edx],0 (param_6) / jne 0x516955
0x5168DF cmp byte [esp+0x14],0 (bVar6) / jne 0x516955
0x5168E9 call 0x4FCA30        ← 行取得 → エラー文 "Unknown reference object for function '%s'"
0x516955 … i16 #3 を arg_out へ / PC+=2 / mov al,1
```
非 0x1C 経路: `*param_6 = 0` → 直に i16 #3 を arg_out へ（命令長 4B）。

### D.2 裁定

> **opcode `0x1C` は「参照オブジェクト接頭辞」**であり、fetch 自身が `[0x1C:i16][X:i16]`（4B・固定）を消費して `X` を参照レコード表で解決し、**直後の本命令の `(opcode, arg)` を返す**。executor は **0x1C を一切見ない**。
> - **`rec[+0xC] ≠ 0`** → `seta` = 「スクリプト変数にバックされる動的参照」フラグ。`FAA110(ctx, id)` で値を解決し `rec[+8]` へキャッシュ。
> - **`rec[+8] ≠ 0`** → `FUN_009832E6` で名前→form 解決 → `*param_6`（本命令の暗黙オペランド）。
> - **refobj==0 かつ非変数** → `Unknown reference object [for function '<0x4FCA30(param_3) の行名>']. Execution of script '<名>' stopped.` で **fetch=0**（実行停止）。**行名が i16#2 に掛かることが、i16#2 が opcode である傍証**。
> - **`param_7 ≠ 0` では解決スキップ**（FireEvent 走査と整合）。
> - **f framing との整合**: v176 §6.1「`0x001C` のみ固定 4B 例外」＝prefix が常に 4B 固定であることと矛盾しない。QUST 先頭ヒストグラム `0x001C 671 / INFO 516` も「参照付き命令の先頭 prefix」として自然。

**親版との相違**: 親版は 0x1C を「本体＝%変数参照フェッチ、`[out]` へオブジェクト格納」とするが、**opcode_out が i16#2 で上書きされる**（`0x5168CB`）ことを考慮すると成立しない。`*param_6` がオブジェクト out であり、opcode_out とは別。

---

## E. 【V-178-r5】盗難馬 type0x3D 発生経路 — **専用経路なし（ライブ書手は 2 件）**

### E.1 書手 4 件の実バイナ検証（v193_p13/p17）

| site | 実内容 | 判定 |
|---|---|---|
| `0x426F91` | `mov ecx,[0xB33B00] / push 4 / lea edx,[esp+0x80] / push edx / call 0x4534D0`（**ストリーム 4B 読出**）→ `fld [esp+0x7c]` → `mov ecx,ebx / call 0x4269E0`。直前 `test byte [esp+0x254],0x80 / je` | **ExtraDataList の Save/Load における復元**（`..\\TES Shared\\ExtraDataList.cpp` L0x1B6F） |
| `0x50B558` | `call 0x4FAE80` → `vt+0x250`(ecx=esi) → `fild [esp+0x18]-値` → `esi==g_thePlayer && [player+0x116]≠0` なら `[player+0x700]+=delta`／否则 **`lea ecx,[esi+0x44] / call 0x4269E0`** → `vt+0x40` 通知 | **OnAlarmVictim＝被害者 EDL へ犯罪値加算（唯一のライブ創出経路）** |
| `0x60FDA3` | `esi=ecx(this)` / `edi=arg2`。`this==g_thePlayer` とき `lea ebx,[edi+0x44]` → `0x41FC90` 読出 → **>0.0 でなければ即終了** → `[g_thePlayer+0x6DC]++` → `fild [0xB376B0]` → `mov ecx,ebx / call 0x4269E0` → `vt+0x40(0x80)` | **既存マーカへの加算（消化側）** |
| `0x60FC49-0x60FD17` | `lea ecx,[esi+0x44] / call 0x4269E0`、`delta≥1.0` か `this==player && [player+0x668]` 比較で通知可否 | **到達不能**: `e8_callers(0x60FC49)=0`・`imm32_refs3(0x60FC49)=0`（全セクション）・Ghidra 関数登録なし・`0x60FBB4`=int3 |

⇒ **有効書手は実質 3 件**（復元／OnAlarmVictim／既存加算）で、「発生」を担うのは **OnAlarmVictim のみ**。

### E.2 文字列 xref（v193_p12/p13）

| 文字列 | xref | 所在 |
|---|---|---|
| `"%s got away with stealing horse"` `0xA70058` | `.text 0x60FE53` | **`FUN_0060FD20`**（L377164 `sprintf` → `FUN_00579B60`） |
| `"%s tried to pickpocket…"` / `"%s got away with attacking %s"` | 0x6106E5 / 0x610E3F | `FUN_00610540` / `FUN_00610930`（兄弟ハンドラ） |
| **`"IsHorseStolen"` `0xA4E50C`** | `.data 0xB0F4D0` | **オペコード表 idx445 行 name ポインタ**（+0x10 flagA=1, +0x18 impl=`0x505EE0`, +0x20 worker=`0x4F8C20`）。**`0xA4E50E` はその末尾＝別リテラルではない** |
| `"steal this horse?"` `0xA61104` 等 | `.text 0x9F2CE1/CE6/D01/D06` | `FUN_009F1C00`＝**起動時メッセージ登録**（`push 2 文字列 / mov ecx,0xB38DC0 / call 0x419A70`）＝UI 文言 |

### E.3 `FUN_0060FD20` = 馬盗難の犯罪解決ハンドラ（確定）

1. `criminal==player && victim.type0x3D>0` → `[player+0x6DC]++` ＋ `[victim+0x44] += fild[0xB376B0]` ＋ `vt+0x40(0x80)` 通知
2. `FUN_006070B0(5,…)` で**犯罪種別 5 の犯罪オブジェクト**生成 → `FUN_0067A290` で witness/guard リスト取得
3. 目撃者ごと `vt+0x374` / `vt+0x224` / `FUN_0052ED50(2,0xe)`・`(2,3)`（トピック発火）/ `piVar9[0x16]+0x1a4` / `vt+0x310`
4. witness 無し → `sprintf("%s got away with stealing horse")`
5. `IsHorseStolen`(opid `0x111A`) の唯一 caller `0x505EEE` ⊂ impl `0x505EE0` → `0x4F8C20`（`type0x3D>0.0f` 判定）

**残る留保 → V-193-4**: 「馬 ref を victim とする OnAlarmVictim の成立条件（専用トリガ）」は本タスクで特定不能。

---

## F. 【V-177a】ExtraStartingPosition `node+8` / type `0x53` `+0xC`

### F.1 `node+8` = **全 BSExtraData 共通の次ノード・リンクリストポインタ**（本補遺の新読解）

`ExtraDataList::CopyListForReference`（`FUN_00428B20` 起点、v193_p14）:
```
0x428B35 mov ebx,ecx            ; ebx = dest EDL (this)
0x428B46 mov edi,[esp+0x28]     ; edi = source EDL (arg)
0x428B4A mov esi,[edi+4]        ; ★ esi = source EDL のリスト先頭
0x428B55 movzx eax,byte[esi+4]  ; ★ [node+4] = extra type（v178 の node 構造と一致）
          add eax,-0x12 / cmp eax,0x36 / ja …   ; type 0x12..0x48 の jumptable
          … `mov byte[esp+0x28],1`（ローカル flag）…
          case 削除系: mov esi,[edi+4]（先頭へ戻す）/ mov byte[esp+0x28],0
0x428C3C test esi,esi / je end
0x428C40 cmp byte[esp+0x28],0 / je 0x428c4a
0x428C47 mov esi,[esi+8]        ; ★ [node+8] を「次」へ進む
0x428C4A test esi,esi / jne 0x428b55
```
⇒ `esi` は **extra ノード**（type を `[esi+4]` で読む）であり、`[esi+8]` は**走査の推進先**。つまり **`+8` は extra ノード共通の next リンク**（ctor `0x42AC40` の `[esi+8]=0` ＝「新規ノードは末尾」、attach `0x41E0A0` で連結）。

**親版の「`FUN_0045C4F0`/`FUN_004DD1A0` が destCell を store」との対立**: type0x14 の payload は `+0xC..+0x14`(pos)/`+0x18..+0x20`(rot) で**destCell を持たず**、destCell 相当は **type0x53 の `+0xC`**（V-177b）にある。よって `+8` を destCell と解すると type0x53 と二重計上になる。**本補遺の「next リンク」説を推奨**（親版の 0x45C4F0/0x4DD1A0 の store 先がノードか EDL かは要再確認 → V-193-5）。

### F.2 type `0x53` = **単一ポインタ payload の extra**（ctor 実測）

```
FUN_00429950 (ctor, type0x53 用・v193_p17):
  0x429950 mov eax,ecx / xor ecx,ecx
  0x429954 mov byte [eax+4],0x53        ; type
  0x429958 mov dword [eax+8],ecx        ; next = 0
  0x42995B mov dword [eax],0xA357A0     ; ★ vtable 0xA357A0（RTTI 未照合）
  0x429961 mov dword [eax+0xC],ecx      ; payload = 0
  0x429964 ret

FUN_00423C90 (get-or-create・v193_p17):
  arg==0 → push 0x53 / call 0x41E160（破棄） / ret 4
  否则   → push 0x53 / call 0x41E210（取得）
           無ければ malloc(0x10) → call 0x429950 → 0x41E0A0(attach)
           [esi+0xC] = arg          ; ★ +0xC = 与えられた 1 ポインタ
```
- `FUN_0041F7F0`（v193_p14 実測 11B）: `push 0x53 / call 0x41E210 / test / je / mov eax,[eax+0xC] / ret` ＝ **+0xC getter**。callers **4 件**: `0x45BCA7`(teleport 適用 `FUN_0045BB30`) / `0x45EB10`(破損修復 `FUN_0045E990`) / `0x49014A` / `0x490C51`。
- `FUN_00423C90` callers **3 件**: `0x423D57`/`0x423D63`（`FUN_00423D30`）/ **`0x427E15`（`FUN_00426A90`＝Save/Load）**。
- ⇒ **type0x53 は「後方参照（セル等）を 1 ポインタで保持する extra」**、`FUN_0041F7F0` はその参照解決キー。**クラス名は vtable `0xA357A0` の RTTI 未照合 → V-193-6**（親版も同様に [残]）。

---

## G. 【V-177b】`MagicItem::vt+0x18` の 1/4 と `+0x5C` コントローラ

### G.1 「slot6 が戻り値 1/4」は**再現不能**（負の証拠・v193_p17）

`cache/vtable_index.tsv` 全 **1715 行**の `vtable+0x18`（=slot6）を実バイト読みして戻り値を判定:
- `mov eax,1; ret` を返す slot6 → **3 種のみ**（`HavokFileStreambufReader 0x8BC710` / `HavokFileStreambufWriter 0x8B1670` / `Normal,PosGen,Random,Uniform 0x91FEA0`）＝**MagicItem 系に該当なし**
- `mov eax,4; ret` を返す slot6 → **0 件**

MagicItem 系の主 vtable slot6 実測:
| クラス | vtable / slots | slot6 | 本文 |
|---|---|---|---|
| MagicItem | 0xA322C4 / 16 | `0x982D85` | pure-call 系スタブ（親版と一致） |
| **SpellItem / EnchantmentItem** | 0xA34C3C / 0xA33C84, 55 | **`0x418F10`** | `lea ecx,[esi+0x24]; call 0x414C70`(リスト解放) → `jmp 0x46C2F0` → **`0x46B170`** |
| AlchemyItem / IngredientItem | 0xA3217C / 0xA33F5C, 72 | `0x412860` | `lea ecx,[esi+0x30]; call 0x414C70` → 同 |
| MagicItemObject / MagicItemForm | 0xA32364 / 0xA33DBC | `0x46C2F0` | `jmp 0x46B170` |

`0x46B170` は `movzx eax,byte[esi+4]; add -0xD; cmp 0x30; jumptable`＝**formType ベースのディスパッチ（SEH 付き大関数）**で 1/4 を返す形ではない。

⇒ **`param_2` の実 vtable が上記のどれでもない、または `*param_2 + 0x18` の解釈に別要因がある**。**親版・本補遺とも 1/4 の由来は未特定**（→ V-193-7）。手掛かり: `0x414C70` は `+4/+8` を head/tail とする**リスト解放ヘルパ**（要素 `+0` は next、`-4` 調整で先頭復帰）。

### G.2 `this+0x5C` コントローラ — **存在は実証、class 名は未特定**

- `FUN_00699190` 呼出し元 **13 件**: `0x5020D6/0x50212F`(`fun_00501f50` = Cast impl) / `0x514942` / **`0x5E4496`(`fun_005e4440` = StartItemAction)** / `0x601439` / `0x6033F2` / **`0x6174C2`(`fun_00617340` = item-type switch)** / `0x62B3F3` / `0x62B539` / `0x634FAE` / `0x64AE06` / `0x64D786` / `0x6728FC`。
- `fun_005e4440` の decompiled（v193_p17）で **`*(int*)(param_1 + 0x5c)` が vtable として使われる**:
  ```
  (**(code **)(*(int *)(param_1 + 0x5c) + 0x34))(iVar7 + 0x18);
  (**(code **)(*(int *)(param_1 + 0x5c) + 0x3c))(iVar7);
  (**(code **)(*(int *)(param_1 + 0x5c) + 8))(iVar1 + 0x18, param_2, 0);
  (**(code **)(*(int *)(param_1 + 0x5c) + 0x1c))(iVar3 + 0x18, 0,0,0);
  ```
- `fun_00617340` は `*(*(param_1+0x3c) + 0x5c)` 経由 → **`[obj+0x5C]` = コントローラ vtable ポインタ**（v177 の前提を実証）。
- コントローラ契約（`FUN_00699190` 本体、v193_p14）: `*this`=vtable、`this[2]`(=+0x08)=状態 (**5=busy 拒否 / 6=受入不可拒否**)、`vt+0x18()`=種別(1/4)、`vt+4`/`vt+0`=SpellItem 供給、`vt+0x30`=可否、`vt+0x34/0x3c`=受入、`vt+0x10/0x14`=起動、`FUN_00415180()`=bit22 ゲート。
- **class 名は +0x5C の ctor/初期化書手が未特定 → V-193-7 継続**。

---

## H. 【V-177c】`[base+0x58]` bit7/22/31（親版の構造確定へ追加）

### H.1 述語の全実測（v193_p16）

| bit | 述語 | 本文 |
|---|---|---|
| bit7 | `FUN_004133C0` | `base=[ecx+0x1C]; [base+0x58]>>7 &1` → 立ちなら **0**、否则 `[ecx+0xC]` |
| bit22 | `FUN_00415180` | `this+4/+8` のリスト走査（`add ecx,-4`）、各要素 `[[elem+0x1C]+0x58]>>0x16 &1` → 1 つでもあれば 1。**唯一の呼び出し元 `0x69919E` = `FUN_00699190`**（busy ゲート） |
| **bit30** | **`FUN_004AE590`（新規同定）** | `mov eax,[ecx+0x58]; shr eax,0x1e; and al,1; ret` |
| bit31 | `FUN_004AE5A0` | `test dword [ecx+0x58],0x80000000; setne al; ret` |

### H.2 bit31 の呼び出し元文脈（v193_p17）

```
0x4DE9EB (fun_004de980): mov ecx,[esi+0x1C] / call 0x4AE590(bit30) → 真なら return 3
0x4DE9FF                : mov ecx,[esi+0x1C] / call 0x4AE5A0(bit31) → neg/sbb/and 5 → return 5 or 0
                          ⇒ 戻り値集合 {0,3,5}
0x4DBB5A (fun_004dbae0): bit30/bit30 判定 → vt+0x154 へ
0x650698 (fun_006505d0) : bit30/bit31 判定 → FUN_00446CB0 (家具/セーブ帯)
0x6788A2,0x678985 (fun_00678750): vt+0x170(GetBaseForm) → bit31 → 1 をフラグへ
```

### H.3 `+0x58` の書手側証拠（v193_p18/p20）

- **`or dword [reg+0x58], imm` のうち bit7/bit31 を立てるものは .text に 0 件** ⇒ 高ビットは**直接 OR されない＝ESM レコードヘッダの flags 一括インポート由来**。
- **bit21 の書手を検出**: `L15470 (fun_00xxxxx: Player LoadGame 内)` `*(knownEffectSetting + 0x58) |= 0x200000`（`"Player Load: Failed to find known effect setting ID %08X"` の直後）⇒ **`+0x58` は MGEF を含む TESForm 系に存在する flags dword**、bit21 = 「既知の呪文効果（known MGEF）」マーク。
- 既存資料でも **`0x80000000` の意味は未解決として既出**: `tmp/re/s73_bsa_dds_tel.md` / `tmp/re/v182_asset_integration.md`「`0x80000000` の意味**（load-state flag? teleport-type flag…）」。
- ⇒ **フラグ名は未確定を維持**（→ V-193-8）。ただし性質は「**ESM record flags 相当の u32 であり、bit21 のみ実用上の命名が確定**」まで進展。

---

## I. 【V-177d】`+0x11D` 値5 と `+0x11C` 各値・遷移書手

### I.1 `+0x11D` の書手は decompiled 全数で **4 件のみ**（v193_p20 D2）

| L# | FUN | 代入 |
|---|---|---|
| L412317 | `fun_0064b400` | `= 0`（初期化） |
| L412443 | `fun_0064b650` | `= param_2[+0x11d]`（**構造コピー**） |
| L415119 | `fun_00650030` | `= 0`（遷移失敗） |
| L417613 | `fun_00654f10` | `= (-(cVar2!=0) & 5) + 4` → **4 か 9** |

- v177 の asm 既知 immed（`FUN_006564C0` の `0x656F20`→1 / `0x656F29`→6）は Ghidra の別の表現で上記に現れない（親版も同様に再確認できず）。
- **`+0x11D = 5` の書手は 1 件も不在**。`FUN_00654F10` の `case 4/9 → vt+0x370(…, cVar2?10:5, …)` の **5/10 は `vt+0x370` へ渡す code 引数**であって +0x11D への書込ではない。
- ⇒ **v177 の「値 5 = 一時値」は未証（誤読候補）**。値セットは実測で **{0, 1, 4, 6, 9}**（1/6 は遷移中間・4/9 は定常・0 は未使用/失敗）。→ V-193-9。

### I.2 `+0x11C` の byte 書手は **v177/v178 の「1 件のみ・非保存」を訂正**（v193_p19/p20）

| site | 命令 | 意味 |
|---|---|---|
| **`0x65698E`（`FUN_006564C0` 内）** | **`mov byte [esi+0x11c], al`**（al は直前のローカル `[esp+0x6c]`、`test dword [esp+0x64],0x2000000` の直後） | **★ ロード時の復元** → **「セーブ非保存」は誤り** |
| `0x65454D`（`FUN_00654420`/`006545E0`） | `mov byte [edi+0x11c], 2` | 値 **2**（v177 の {0,1,3,5,6} に無い） |
| `0x654A19`（`FUN_006545E0`） | `mov byte [edi+0x11c], bl` | レジスタ値 |
| `0x654CE8`（`FUN_006545E0`） | `mov byte [ebx+0x11c], 0` | 値 **0** |
| `0x623460`（`FUN_006231E0`） | `mov dword [esi+0x11c], 0` | dword 幅（別解釈に注意） |
| `L412405`（`fun_0064b650`） | `*(char*)(param_1+0x11c) = (char)param_2[0x47]` | 構造コピー |
| `L285538`(`fun_0057dc30`) / `L306048`(`DAT_00b333c4+0x11c`) | dword 幅の別クラス混在 | 混同注意 |

- ⇒ **byte 幅の値集合は実測で {0, 2, reg, セーブ値}**。v177 の読手比較値 {1,3,5,6} と**書手側で一致しない** → **KnockedState の値名と遷移書手は未確定を維持**（→ V-193-9）。byte 書手は v177 の「1 件のみ」ではなく**少なくとも 4 件**。

---

## J. 【V-177e】`+0xE4..+0xF0` リスト族 — **setter 3 本と元素型を特定**

### J.1 4 フィールドは**独立した 4 リスト先頭**（v193_p18/p20）

**getter（flag 引数付き条件取得）**:
```
0x64B240 → [this+0xE4] / 0x64B270 → +0xE8 / 0x64B2A0 → +0xEC / 0x64B2D0 → +0xF0
  flag==0 → 生ポインタ返却
  flag≠0  → [head]==0 なら 0、否则 push 0; call 0x484E80(head,0) → 真なら head / 伪なら 0
```

**setter（全体置換・v193_p20 E1）**:
```
FUN_0064AE90 (ret 4): if ([this+0xE8]!=0){ 0x484470(node); free(node); } [this+0xE8] = arg; return 1
FUN_0064AED0 (ret 4): 同様に +0xEC
FUN_0064AF10 (ret 4): 同様に +0xF0
（+0xE4 は 0x64AE50 系に相当を持つはず＝本次で未特定）
```

**クローン**: `FUN_0064B650` が `esi→edi` へ **`+0xE4/+0xE8/+0xEC/+0xF0` を 4 本まとめてコピー**（`0x64B6D9..0x64B703`）＋ `+0xF8` を float でコピー ⇒ **4 フィールドは 1 ファミリ**。

### J.2 **元素型の決定**（`FUN_00484E80` + `FUN_0041DF10` 全文、v193_p18）

```
FUN_00484E80(this_list, flag):          ; ret 4
  esi = [ecx]                            ; ★ リスト先頭ノード
  loop:
    ecx = [esi]                          ; ★ element = node[+0]
    if (ecx == 0) → return 0
    if (FUN_0041DF10(ecx, flag)) → return 1
    esi = [esi + 4]                      ; next
  return 0

FUN_0041DF10(element, flag):            ; ret 4
  if (flag != 0) return ([element + 0x0B] >> 4) & 1        ; u32@+0x08 の bit28
  else          return ([element + 0x0B] & 0x18) != 0      ; u32@+0x08 の bit27|bit28
```

- **ノード構造 = `{+0 element, +4 next}`**、**要素は `+0x08` に u32 フラグ（bit27/28）を持つオブジェクト**。
- これは v171 の「flags bit27/28 検査」と一致。**要素はフォーム系オブジェクト**（`[elem+0x08..0x0B]` が flags）[有力]。
- process 帯 (0x600000-0x6D0000) の `mov [reg+0xE4/E8/EC/F0]` ストアは **11 件のみ**（`fun_006022f0/00603ca0/0062a660/00633cb0/006411e0/0064ae90/0064aed0/0064af10/0064b650/006623d0/00679c10`）＝**挿入ヘルパは上記 setter 群**。
- ⇒ **元素型 = ノード `{+0 elem, +4 next}` で保持され、`elem+0x08` の bit27/28 でフィルタされるフォーム系オブジェクト** [確定(構造)]。**装備/エフェクト/魔法のどれかは bit27/28 のフラグ名（V-177c と同一課題）に依存 → V-193-8 に統合**。

---

## K. 29 opid（台帳 §67.6 未登録）の EXE 側実体

- 詳細: **`%TEMP%\opencode\v193_opid29_detail.csv`**（ヘッダ + 29 行、10,118 B）。列: `opid,name,scda_count_excl,impl,argn,argdesc,flagA,retN,kind,head1,head2,prologue10`。
- 検証: `opcode_table.json` と **29/29 マッチ**、`kind` 判定は v189 の「実 impl 27 + 共有スタブ 2」と**矛盾なし**（`0x110C`/`0x114A` → `mov al,1; ret`）。
- 名称は 532 表と完全一致（＝「未登録」は §67.6 付録表のみという v189 裁定を EXE 側から再確認）。

### K.1 引数契約の型（本タスクの新規整理）

`retN` 列が示すスタック引数数は **`0`（null ゲート＋即時 delegate 型）** と **`0x38`/`0x24` 等の巨大フレーム型** の 2 系統に分かれる:

| 型 | 特徴 | opid（該当） |
|---|---|---|
| **A. null ゲート → 即 delegate** | `push esi; mov esi,[esp+0x10]; test esi,esi; je …` で ref 引数を検証してから委譲。`retN=0` | 0x1072 Lock, 0x1073 UnLock, 0x1075 SetCrimeGold, 0x1076 ModCrimeGold, 0x1021 Enable, 0x1022 Disable |
| **B. 標準 8 引数 impl** | `push ecx / mov edx,[esp+0x1C] / mov ecx,[esp+0x18] …` ＝ `(this, ctx, opcode, pc, arg, out…)` の共通シグネチャ、`FAE80` でイベント引数デコード | 0x0016, 0x001E, 0x100F, 0x1010, 0x1013, 0x101E, 0x1036, 0x1037, 0x1039, 0x1089, 0x108B, 0x1154, 0x115B |
| **C. SEH cookie 大物** | `push -1 / push 0x9B6DC8 / mov eax,fs:[0] / sub esp,…` | 0x1002 AddItem, 0x100D Activate |
| **D. RTTI/名前解決から入る** | 先頭が `call 0x9832E6`（名前→form）または `mov ecx,[0xB333C4]`（player ゲート） | 0x105E EvaluatePackage, 0x10D9/0x10DA/0x10DB メニュー 3 種 |
| **E. 共有スタブ** | `mov al,1; ret` | 0x110C, 0x114A |

### K.2 重点 2 件（v193_p19 B2 全文）

**`0x0016 OnMagicEffectHit`（10,480 回・argn=1・argdesc `0xB0ACE4`・impl `0x5045E0`）**
```
push ecx / ecx=[esp+0x18] / edx=[esp+0x14] / esi=[esp+0x14] / edi=[esp+0x24]
lea eax,[esp+8] → push eax / … 8 push … → call 0x4FAE80 (add esp,0x20)   ← 引数デコード
if (!ok) return (al=0)
*ebx(=arg out) = 0.0
if (esi && edi):
    eax=[esp+0xC]; push 0x2000; push eax; mov ecx,edi; call 0x4FB5F0      ← フラグ 0x2000 のイベント照合
    if (al) *ebx = 1.0
return 1
```
⇒ **band1 ゲート型**（v178 §1 の「結果 0.0=skip / ≠0.0=本文実行」と完全に同じ形）。`0x4FB5F0(owner, form, 0x2000)` が **MGEF ヒット可否**を判定し 1.0/0.0 を返す。頻度 10,480 は「Hit 毎に評価される band1」であることと整合。

**`0x1039 SetStage`（1,391 回・argn=2・argdesc `0xB0AA2C`・impl `0x502CB0`）**
```
push ecx / edx=[esp+0x1C] / fldz / edi=[esp+0x24] / lea eax,[esp+4] / fstp qword [edi]   ← *out = 0.0
… 9 push … → call 0x4FAE80 (add esp,0x24)
if (!ok) return
ecx=[esp+0x24] (quest ref)
if (ecx): eax=[esp+4](stage); push eax; call 0x529AC0 → if(al) *out = 1.0
return 1
```
⇒ **`FUN_00529AC0(quest, stage)` が実体**（成功で 1.0）。argn=2（quest, stage）で v189 の arglen 8..10 と整合。

---

## L. 新規残点（本補遺から起票）

| ID | 内容 | 優先 |
|---|---|---|
| **V-193-1** | `FUN_004FDAF0` の `*(char*)(local_8+4)` type-code 体系（`0x04/05/06/09/0A/20/39/3B` ＋ `FUN_00469520` の `{0x13..0x2B}` 集合）と、§1.1 の boolean `record[+0x10]` が同一フィールドかの判別 | 低 |
| **V-193-2** | `FUN_005097A0` の `{+0x20 type, +0x24 f32}` ノードで **`'l'` と `'f'` が別分岐**（0x509AD7 / 0x509AED）を取る点。emit 不在のため retail 未到達だが、移植時に型を問う箇所 | 低 |
| **V-193-3** | fetch `0x516830` の `param_7` の実引数（FireEvent か別 pass か）と `*refobj_out` の持続範囲（prefix 効果が 1 命令限定か） | 中 |
| **V-193-4** | **盗難馬に type0x3D を付ける OnAlarmVictim の成立条件**（馬を victim とする警報トリガ）。書手は OnAlarmVictim のみ・専用経路なしを確定済み | 中 |
| **V-193-5** | ExtraStartingPosition `node+8` を「destCell」とする親版 store（`0x45C4F0`/`0x4DD1A0`）の store 先がノードか EDL かの仲裁（本補遺は「全 extra 共通 next」説） | 低 |
| **V-193-6** | extra type `0x53` の class 名（ctor `0x429950` の vtable **`0xA357A0`** の RTTI 未照合） | 低 |
| **V-193-7** | `MagicItem::vt+0x18()` 戻り値 1/4 の実装本体（全 1715 vtable の slot6 に `mov eax,1/4;ret` なし＝親版/本補遺とも未解決）＋ `this+0x5C` コントローラの class 名（ctor/初期化書手未特定） | 中 |
| **V-193-8** | `[base+0x58]` の bit7/22/31 正式名（＋ bit30 `FUN_004AE590` 述語の意味・戻り値 {0,3,5} enum）。**bit21 = known MGEF は確定**。既存 `s73`/`v182` の `0x80000000` 未解決と統合 | 中 |
| **V-193-9** | `+0x11D` の値 5 の正体（書手不在＝`vt+0x370` code 誤読候補）／`+0x11C` の byte 値集合 {0,2,reg,セーブ値} と v177 読手値 {1,3,5,6} の不一致、KnockedState 値名 | 中 |
| **V-193-10** | `+0xE4` の setter（`+0xE8/+0xEC/+0xF0` は `0x64AE90/ED0/F10` を特定、`+0xE4` の対は未特定）と bit27/28 フラグ名（＝V-193-8 と統合して 4 リストの用途を確定） | 中 |

---

## M. 自己検証・証跡

| # | 検査 | 結果 |
|---|---|---|
| 1 | `probe_harness.py selftest` | 着手時 **4/4 PASS** / 完了時 **4/4 PASS** |
| 2 | ESM / ESP / DLC アクセス | **0 回**（再スキャン禁止遵守） |
| 3 | git / docs / GT / TODO / session-memory / plan / cache / 台帳 / 既存 `tmp/re` の編集 | **0 件**。`v193_opcode_gt_residuals.md` は親セッションが書いた既存物のため**未変更** |
| 4 | `imm32_refs3(0xB33B18)` | 0 件（コードが `[0xB33B00]` 経由で触ることを確認） |
| 5 | `e8_callers(0x60FC49)` / `imm32_refs3(0x60FC49)` | 0 / 0 → 到達不能判定 |
| 6 | vtable slot6 全走査 | 1715 行 / 735 種、`mov eax,1;ret` 3 種（非 MagicItem 系）、`mov eax,4;ret` 0 種 |
| 7 | `+0x11D` 代入全数 | 4 件（値 5 の書手 0 件） |
| 8 | `+0x11C` byte 書手 | **4 件以上**（`0x65698E` ロード復元を含む） |
| 9 | 29 opid × opcode_table | 29/29 マッチ、stub 2 件は v189 と一致 |
| 10 | `push 0x53` 全走査 / `FUN_0041F7F0` callers | 16 サイト / **4 件** |

**使用証拠ファイル（本タスク生成・すべて `%TEMP%\opencode/`）**:
`v193_p1.py`〜`v193_p20.py` と対応 `v193_p*_out.txt`（20 プローブ）、`v193_opid29_detail.py` / `v193_opid29_detail.csv`、本補遺 `v193_subagent_supplement.md`。

**読取のみ**: `tmp/re/v178_runtime_residuals3.md` / `tmp/re/v177_gt_slot_residuals2.md` / `tmp/re/v189_esm_ledger_residuals.md` / `tmp/re/v171_gt_candidates.md` / `tmp/re/cache/{opcode_table.json,vtable_index.tsv,named_funcs.txt}` / `%TEMP%\opencode\v189_opid29.csv` / `docs/Oblivion_decompiled.c`（部分検索）／親版 `tmp/re/v193_opcode_gt_residuals.md`（読取のみ）。
