# v193_opcode_gt_residuals.md — opcode/GT slot 残点 第4束（V181）

- 作成日: 2026-10-29 / 担当: TODO serial 181 (V181) — **親セッションによる直接解析**（サブエージェントのサーバ再起動による中断を受け、inline 継続。前回子のスケルトン＋selftest を踏襲）
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 Steam / 7,898,624 B / ImageBase 0x400000）
- スコープ: `TODO_re49.md` serial 181（V181）行
  - v178 `V-178-r1..r5`（%変数 +0x10 フラグと 'l' emit 元 / `[0xB33B00]` クラス命名 / `0x4FBE00` レート制御 / fetch opcode 0x1C スペシャル / 盗難馬 type0x3D 付与経路）
  - v177 `V-177a..e`（+0x11D 値5 / ExtraStartingPosition node+8・type0x53 / base+0x58 bit7,22,31 / MagicItem vt+0x18 の 1,4 / +0xE4..F0 リスト族の元素型）
  - v179 台帳未登録 29 opid の EXE 側実体調査（名称・役割）
- 契約: `tmp/re/SUBAGENT_CONTRACT.md`（読取のみ・成果物は本ファイルのみ・`TODO_re49.md`/`docs/`/台帳は編集禁止＝他セッション保留依頼を遵守）

## 1. 手法 / 入力

- capstone 5.0.7（detail 付き Cs）で逆アセンブル、`tmp/re/re_util.py`（callers/find_dword/cstr）＋ `cache/`（opcode_table.json 533行 / call_targets.json / vtable_index.tsv / named_funcs.txt）。
- decompiled（29 MB）は該当行の部分読取のみ（GT §2 規則）。既存中間物 `v178_p5_out.txt`（0x4FBE00 全文・fetch prologue）・`v189_opid29.csv`・`v189_esm_ledger_residuals.md` §3.2 を再利用。
- 中間スクリプト: `%TEMP%\opencode\v193_1..3_*.py` + 出力 `v193_1..3_out.txt`。
- `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（着手時・完成直前の 2 回）。

## 2. 証拠・分析

### V-178-r1: %変数レコード +0x10 フラグの語義と 'l' emit 元 — [確定(式) / 残(初回書手)]

- **'f'/'s' 分岐式を decompiled で直接確認**: `FUN_004faa90`(L194376-194385) は `this+0x48` リストを歩き、名前 `puVar2[6]`（+0x18, char*）で照合 → `return ('f' + (record[+0x10] != 0 ? 0x0D : 0))`。`FUN_004FD5F0`(L196967-196975) も `this+0x3c` リストで同一式（`param_2[0x204] = (-(*(char*)(puVar2+4) != 0) & 0xd) + 0x66`）。
  → **record[+0x10] の byte フラグが 0 なら 'f'(float)、非 0 なら 's'(string)** [確定]。
- **レコード実体**: `FUN_004FA910`（リストの clone、実測）は 0x18B セルを確保し `+0 = value(dword)` / `+8 = f64(8B)` / **`+0x10 = byte フラグ（`al=[src+0x10]` をコピー）`** を複写。名前 +0x18 はソース側ノード（`puVar2[6]`）に属す（clone 先の size 0x18 とは別レイアウト）。
- **'l'(0x6C) emit**: 本タスクでコンパイラ帯 **0x4F0000..0x503000 を独立に再走査（C6 disp=imm8 0x6C、バイト/ディスペイト両形式）→ 0 件**。v178 の「リテールに不在＝consume 側のみ受付」を独立確認 [確定]。
- **意味推定**: 's' = 文字列型（エンジン変数/スクリプト変数の型区別。'f'QuestDelayTime 等の INI 系グローバルは float='f'）。**+0x10 の初回書手（型宣言側）は未検出 [残]**。

### V-178-r2: `[0xB33B00]` シングルトンの命名と [+0x18] 書手 — [確定]

- **ctor の唯一 caller 0x441BB0 の同定**: `FUN_00441970`（ゲーム初期化関数）内: `push 0x1C4; call 0x401F00`（malloc）→ null チェック → **`call 0x45B300`（ctor）→ `mov [0xB33B00], eax`**（ctor 内公開 0x45B3C8 と二重）。以降 `[0xB06A2C]` による後続初期化が続く大関数の一部 [確定]。
- **[+0x18] bit0/bit18 の書手 = `FUN_0045A530`（thiscall, ret 4, bool 引数）** [確定]:
  ```
  0x45A530: eax=[0xB33398] → edi=[eax+0x10]（メインスレッド ID キャッシュ）
            call GetCurrentThreadId (IAT 0xA2808C) / cmp eax,edi
   メインスレッド時: arg≠0 → or dword [esi+0x18], 1      ; bit0 = メインスレッド文脈
   他スレッド時    :        → or dword [esi+0x18], 0x40000 ; bit18 = 他スレッド文脈（0x45A566）
  ```
- **this = シングルトンの裏付け**: callers 21 件のうち確認した全 site が `mov ecx,[0xB33B00]` 直近（または直後に同ロード）＋ `push 0/1` で呼ぶ — enter/leave ペア構造（例: 0x40F3B0 `push 1` → 0x40F43B `push 0`）。呼び出し元帯: 0x40D4D0（初期化）、0x444340、0x4471D0、0x45F2A2、0x4645BC、0x466605、0x4D43xx、0x4DFBxx、0x4E36xx、0x4F17xx、0x500Cxx、0x6633A8/0x6634B6（ワーカースレッド帯）。
- これで v178 §3 の構造表が完結: **[0xB33B00] = スレッド文脈フラグ＋formID マップ＋formType 0x7C 等を持つ非仮想 POD シングルトン（命名は未、RTTI 無し）**、述語 0x45A500 の対になる setter が確定。
- （0x45EC50/0x465860 の同型 or 命令は別オブジェクト用の同形フラグと判明: 0x45EC50 は [0xB333A0]=NavMeshManager 系。混同注意。）

### V-178-r3: `0x4FBE00` レート制御の仕様 — [確定]

v178_p5 の全文 disasm（94 命令）から仕様を確定:

| 要素 | 内容 |
|---|---|
| `[esi+0x38]` | 残りディレイ f32（実行までのカウントダウン） |
| `[esi+0x3c]` | 待機累積 f32（ディレイ中の経過時間） |
| `[0xB33E9C]` | フレームデルタ f32（毎フレーム `timer -= delta` / `accum += delta`） |
| `[esi+0x20]` | arg flag==0 のとき ≤4 なら早期 return 0（実行不可ゲート） |
| `[esi+0x28]` | レート制御有効 byte（0 なら gate を飛ばして実行） |
| 執行 | `0x516970`（準備）→ **`0x517810`（RunScript ラッパ）** → al を bl に保存 |
| 再スケジュール | `0x4FAA90(this, "fQuestDelayTime"@0xA49E80, &rec)` で変数解決 → `0x4FA110(rec)` で f32 取得 → **>0.0 なら timer += fQuestDelayTime、そうでなければ timer += `[0xB09E28]` = 実測 5.0f** |
| リセット | 実行後 `[esi+0x3c]=0`（累積クリア）→ `0x4FB430`（後始末） |
| 定数 | `[0xA2FAA8]` = **0.0f**（実測、IsHorseStolen と共有の 0.0 リテラル） |

→ **既定実行間隔は 5 秒**（fQuestDelayTime が未設定/0 のとき）。fQuestDelayTime > 0 で上書き可 [確定]。

### V-178-r4: fetch `0x516830` の opcode 0x1C スペシャルケース — [部分確定]

実行時パス（prologue＋継続 disasm 実測）:

1. i16 をバイト列から fetch（cursor += 2）し、**`cmp [out], 0x1C`** で分岐。
2. 0x1C 時: 続きの i16 を変数 id として読み、`bl` = 引数 seta フラグ。`bl==0` なら **`0x4FA9C0(ctx, id, [edx+8])` で変数レコード解決** → 見つかったら **`seta [esp+0x18] = (record[+0xC] > 0)`**（unsigned 比較 → 負値も 1）。
3. `rec[+8]` を **`0x9832E6`（RTTI キャスト、型表 0xB03054/0xB02F9C）** で型変換し `[out]` へ格納。
4. 未解決/非該当経路: **`0x4FCA30(opcode)` で表行取得 → vcall `[edi]+0xD4` → `0x5167A0`（メッセージ 0xA4DB08 / 0xA4DAC8）** でイベント/診断系処理 → return 0。

→ opcode 0x1C = **「%変数参照フェッチ」**: id → レコード解決 → 型キャスト → 出力、失敗時は表行＋vcall ベースのエラー/イベントパス。**seta フラグ（[esp+0x18]）の後続読手は未特定 [残]**。record[+0xC] が double(+8..+0xF) の高位ワードである場合、seta は「値が [0,1) 外」相当になる点は解釈時に要注意 [推定]。

### V-178-r5: 盗難馬 type0x3D 付与経路 — [部分]

- マーカ機構本体は v189/v178 §4 で確定済み（`0x4269E0` アキュムレータ、OnAlarmVictim 0x50B3E0 内 0x50B558 が被害者 ref +0x44 へ加算、警報報道 0x426A90 内 0x426F91、IsHorseStolen 0x4F8C20 が >0 判定）。
- 本タスクの追加: **`0x426A90` の caller は 2 件のみ: `FUN_004858D0`（0x485A61）/ `FUN_004E00F0`（0x4E0324）** [確定] — 警報報道の発火元候補がこれに絞られた。
- 文字列 `HorseStolen` は 0xA4E50E に実在（0xA4E500="ellFullName" の次 string）だが **abs32 参照は 0 件** → デバッグ/未使用リテラル。
- **馬 ref を victim とする警報の成立条件（ゲーム側トリガ）は [残]**（0x4858D0/0x4E00F0 の深部解析が次手）。

### V-177a: ExtraStartingPosition node+8 と type0x53 +0xC — [大部分確定]

- **`FUN_0041F7F0` 全文実測**（11B）: `push 0x53; call 0x41E210; test; je; mov eax,[eax+0xC]; ret` → **type 0x53 extra の +0xC getter（セル系）** [確定]。callers 4 件: **0x45BCA7（teleport 適用 FUN_0045BB30 内）** / 0x45EB10 / 0x49014A / 0x490C51。`push 0x53` 全走査では他に 0x8884BC/0x954B58 の 2 件（0x53=83 の別用途候補）— 実用ユーザーは 0x41F7F0 のみ。
- **ExtraStartingPosition ctor `0x42AC40` 実測**: `[esi+4]=0x14`（type byte）、**`[esi+8]=0`（destCell ptr 初期化）**、vtable 0xA35A70、vcall `+0x174`（GetPos）→ 3 float を +0xC..+0x14 へ。
- **node+8 の書手**: `FUN_0045C4F0`（0x45C75B `[ecx+8]=eax` / 0x45C781）と `FUN_004DD1A0`（0x4DD1F8/0x4DD21A/0x4DD240）で store 実測 → v177 の「load 時配置解決／slot+0xF8 setter」に整合 [確定(書手)]。**node+8 の読手（copy flag 経路）は [残]**。

### V-177b: MagicItem::vt+0x18 と +0x5C 埋め込みコントローラ — [部分]

- vtable_index row 802: `MagicItem vtable 0xA322C4 / 16 slots`。**vt+0x18 = slot[6] = `0x982D85`** [確定]。
- 0x982D85 の本文: `[0xBAA5EC]` チェック → `push 0x19` → `push 1; push 0` → **`jmp 0x98BC6B`**。0x98BC6B は **C++ ランタイム/例外系の巨大ハンドラ**（sub esp,0x328、セグメントレジスタ退避、0x98DACB/0x98DAD8 呼出し）。slot[4..6] が同一アドレス = **未実装（pure-call 系）共有スタブ** [推定]。
  → v177 の「戻り値 1/4」は**派生クラス（SpellItem 等）の override** 由来と解される。派生の slot[6] 実装特定は [残]。
- **`FUN_00699190` 実測**: `lea ebx,[arg+0xC]` → `call 0x415180`（bit22 リストチェッカ、V-177c 参照）→ 真なら `[esi+8]=5; return 0` → **エフェクト/SpellItem 資格検証（失敗コード 5）** [確定(構造)]。+0x5C 埋め込みコントローラの class 名は [残]。

### V-177c: `[base+0x58]` bit7/22/31 の各読手 — [構造確定 / フラグ名は残]

3 つの reader を全文実測:

| bit | reader | 本文 |
|---|---|---|
| **bit7 (0x80)** | `FUN_004133C0`（tail 0x4133D1 含む全文） | `base=[ecx+0x1C]; edx=[base+0x58]; shr 7; test 1` → 立ちなら **0**、否则 **[this+0xC]**（setter 0x4133E0 と対） |
| **bit22 (0x400000)** | `FUN_00415180`（全文 38B） | this+0x4/+0x8 のリストを歩き（`add ecx,-4` 歩行＝V-177e の同型）、各要素 form の `[form+0x1C]→[+0x58]` の **bit22** が立つ要素が在れば al=1。= **「リスト内に bit22 フォームが在るか」チェッカ**（V-177b の 0x699190、V-180/V-188 の毒判定 0x414EB0 と同型走査） |
| **bit31 (0x80000000)** | `FUN_004AE5A0` | `test dword [ecx+0x58], 0x80000000; setne al; ret` の 3 命令 |

- +0x58 は **dword 値として読まれる**（非 deref）→ v177 の「formID/flags デュアリティ（未解決 formID モード）」仮説と整合 [確定(構造)]。
- **フラグ名は [残]**: bit7=フォーム無効化ゲート [推定] / bit31=unresolved マーク [推定] / bit22=エフェクトリスト走査時の除外・特殊状態 [推定]。

### V-177d: `+0x11D` 値5 と `+0x11C` KnockedState — [残を縮小]

- decompiled 全走査（assign 行のみ）: process 系 +0x11C 書手は **0 初期化（多数）+ setter 引数（L285538 `= param_2`）+ `0x80000000` センチネル（L286800）+ player 特例（L306048 `DAT_00b333c4+0x11c`）** を検出。L97658 は **vtable デリファレンス経由**（別クラスの同名オフセット）＝混在注意。
- byte 幅の {1,3,5,6} 書手は v177 の走査（byte writer = FUN_0064B400 初期化 1 件のみ）を本タスクでも傍証（immediate walk 不要と整合）。
- → **KnockedState の値名と {1,3,5,6} 遷移書手は [残] を維持**。`0x80000000` センチネル（L286800）が Knocked 系の特殊値である可能性は [推定]。

### V-177e: `+0xE4..+0xF0` リスト族の元素型 — [残を維持]

- decompiled 全走査: `+0xE4/E8/EC/F0) = FUN_00401f00(...)` 形式の行 **0 件** → リストヘッドへの alloc 直書きは存在せず、**ctor ゼロ初期化＋挿入ヘルパ経由**（v177 の process band disp8 書手 0 件と整合）。
- 元素型決定には挿入ヘルパ（または HighProcess ctor の decompiled 全読）が必要で **[残]**。次手: 0x64B240 系ゲッタの呼び出し元での要素 deref パターン、`FUN_0064B400` 帯の ctor 全読。

### 29 opid（台帳 §67.6 未登録）の EXE 側実体 — [確定(一覧)]

v189 §3.2 の 29 件すべてに impl 先頭フィンガープリントを付与（opcode_table.json 突合 29/29）。名称は 532 表と完全一致（＝「未登録」は §67.6 付録表のみ、v189 裁定と再整合）。

| 分類 | opid（impl） | 実測フィンガープリントの特徴 |
|---|---|---|
| ブロックイベントハンドラ | 0x0016 OnMagicEffectHit (0x5045E0) / 0x001E OnReset (0x5045A0) | FAE80 型 gate 前段（b16 の 22 check サイト系） |
| 共有スタブ（`mov al,1; ret`） | 0x110C TrapUpdate (0x977C50) / 0x114A AddAchievement (0x977C50) | v192 §3-TrapUpdate 裁定と一致（意図的 no-op） |
| メニュー開閉（[0xB333C4]=player ゲート） | 0x10D9 ShowClassMenu (0x505810→`call 0x5E3320`) / 0x10DA ShowRaceMenu (0x505850→0x57B7E0) / 0x10DB ShowBirthSignMenu (0x505870→0x57A9B0) | b21 の menu xml 経由と整合 |
| b17 既知（dedup キュー） | 0x1021 Enable (0x50A150) / 0x1022 Disable (0x50A240) | v192 §3-v173 と同一 |
| SEH cookie 大物コマンド impl | 0x1002 AddItem (0x507320) ほか | `push -1; push 0x9B6DC8; fs:[0]` 型 |
| 標準コマンド impl（gate+delegate） | 0x100D/0x100F/0x1010/0x1013/0x101E/0x1026/0x1036/0x1037/0x1039/0x105E/0x1071/0x1072/0x1073/0x1075/0x1076/0x1089/0x108B/0x1154/0x115B | null チェック（`test esi/edi,esi`）→ delegate。0x105E EvaluatePackage は 0x9832E6 キャスト前段 |

- scda 上位 0x0016（10,480 回）を含む頻度上位（SetStage 1,391 / Activate 888 / PlayGroup 824）はすべて**実体有効な impl** を持つ（v189 の 27/27 実測有効と矛盾なし）。

## 3. 結論（3行）

1. **runtime 残点3件が確定**: [0xB33B00] の [+0x18] 書手は **`FUN_0045A530`（GetCurrentThreadId 比較で bit0/bit18 を enter/leave セマンティクスで設定、callers 21 件は全件シングルトンロード＋push 0/1）**、0x4FBE00 のレート制御は **「timer[+0x38] をフレームデルタでカウントダウンし、既定 5.0f 秒（[0xB09E28] 実測）または fQuestDelayTime で再スケジュール」**、%変数 +0x10 フラグは **0='f'/非0='s' の文字列型判定 byte**（decompiled L194383/L196975 で式直接確認）。
2. **GT/slot 残点は構造確定**: base+0x58 の 3 reader（bit7=formID 無効化ゲート 0x4133C0 / bit22=エフェクトリスト内該当チェッカ 0x415180 / bit31=setne 3 命令 getter 0x4AE5A0）、type 0x53 = **セル系 +0xC getter `0x41F7F0`**、ExtraStartingPosition node+8 の書手（0x45C4F0/0x4DD1A0）まで実測。フラグ名・node+8 読手・+0xE4..F0 元素型・MagicItem 派生 override・Knocked 値名は残。
3. **29 opid は全件が実体有効な impl を保持**（共有スタブ2・menu 3・b17 既知2・残り20=gate+delegate 型）、「台帳未登録」は §67.6 付録表のみという v189 裁定を EXE 側から再確認。

## 4. 残留不明点

1. r1: %変数レコード +0x10 の**初回書手**（型宣言側）。
2. r4: opcode 0x1C の seta フラグ（record[+0xC]>0）の後続消費箇所と +0xC の実体。
3. r5: 馬 ref を victim とする警報の成立条件（FUN_004858D0 / FUN_004E00F0 の深部）。
4. 177a: ExtraStartingPosition node+8 の読手（copy flag 経路）と type 0x53 の ctor/書手。
5. 177b: MagicItem 派生クラス（SpellItem 等）の slot[6] override 本体と、+0x5C 埋め込みコントローラの class 名。
6. 177c: bit7/22/31 の正式なフラグ名（+0x58 の formID/flags デュアリティ確定）。
7. 177d: KnockedState の値名と {1,3,5,6} 遷移書手、L286800 の 0x80000000 センチネルの意味。
8. 177e: +0xE4..+0xF0 リスト族の元素型（HighProcess ctor 全読 or 挿入ヘルパ特定が次手）。
9. r2: `[0xB33B00]` クラスの正式命名（非仮想・RTTI 無し）。

## 5. 自己検証

- **selftest 4/4 PASS**（着手時・完成直前の 2 回: bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl=0x504560）。
- 内部整合:
  1. `[0xA2FAA8]`=0.0f / `[0xB09E28]`=5.0f（u32=0x40A00000）を生バイト実測 → v178 の IsHorseStolen/0.4FBE00 記載と一致。
  2. 0x45A530 の disasm（or 1 / or 0x40000）が述語 0x45A500（v178 §3.1）の bit0/bit18 と**互いに排他的対**で一致。
  3. MagicItem vtable 0xA322C4 = vtable_index row 802 と一致（typedesc 0xAB4090 列とも整合）。
  4. 'f'/'s' 式は decompiled 2 箇所（L194383/L196975）で同一、'l' emit 0 件は本タスクの独立走査と v178 の記載が一致。
  5. 29 opid: opcode_table 29/29 マッチ、impl 先頭バイトは v189 の 27/27+stub2 と矛盾なし。
  6. `push 0x53` 全走査 3 件・callers(0x41F7F0)=4 件・callers(0x426A90)=2 件・callers(0x45A530)=21 件 — いずれも独立再計測。
- 制約遵守: 新規ファイルは本 1 件のみ（前回子のスケルトンを継続）。docs/・TODO_re49.md・GT・台帳・git・Supermemory・sync は**編集 0 件**（他セッション保留依頼遵守）。ESM 読取 0 回。中間物は `%TEMP%\opencode/v193_1..3_*`。

## 6. Android 移植への示唆（実装しない）

- **スレッド文脈フラグ**: `SetThreadContextFlag(bool)`（GetCurrentThreadId 比較、非メインは bit18 パス）を移植版でも 1 関数に集約し、enter/leave ペア（push 1 → push 0）を RAII 化する。述語側（0x45A500）は「現在スレッドがメインか」の 2 通りを返す設計を再現。
- **スクリプト実行間隔**: quest/スクリプトの実行は「残りディレイをフレームデルタで減算 → 0 で実行 → fQuestDelayTime（既定 5 秒）で再アーム」の単純モデル。fQuestDelayTime 未設定時は 5.0f を既定とする。
- **%変数レコード**: 値セル {u32/f64/byte-flag}＋名前ノードの 2 層構造とし、byte-flag が「文字列型か」を表す。'l' は実装不要（リテール未使用）。
- **opcode 0x1C**: %変数参照は「レコード解決 → RTTI キャスト → 失敗時は診断イベント」の 3 段。seta 相当フラグは「レコード値の大小判定」を持つため、移植時は意味を実測してから再現すること。
- **extra 0x53 / ExtraStartingPosition**: teleport は type0x53 の +0xC（セル）＋type0x14 の位置/回転を組み合わせて解決する 2-extra 構造。
