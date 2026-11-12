# v313 — Save/Load RE Follow-up（実 .ess 境界・残点9件の静的解析）/ ai-todo #76

- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 Steam、ImageBase 0x400000、7,898,624 B）
- 日付: 2026-11-07 / 担当: 静的解析サブエージェント（v313 lane）
- **正式契約**: `tmp/re/SAVE_LOAD_RE_FOLLOWUP_v313.md`（2026-11-07 ユーザー提供・旧プロンプトより優先）。帯分離・5段階判定・クリーンルーム等は旧プロンプトを踏襲。
- **帯（自帯）**: 0x459000-0x460000 / 0x464000-0x466000 / 0x430000-0x431000 / 0x400000-0x402000・0x981F00-0x982100 / 0x4D8000-0x4F2000 / 0x5E1000-0x5E2800。**v314/v315 禁止帯（0x44F000-0x452000・0x51B000-0x51C000・0x570000-0x574000・0x404000-0x406000・0x452000-0x459000・0x460000-0x464000）は非侵入**。
  - 自帯延長として扱った範囲: **0x466000-0x467920**（調査対象 FUN_465130 本体 0x465130..0x465856 と、その callers quicksave 0x466AD0 / autosave 0x466B70 / テスト保存 0x466D21、および 0x5E1FD0 の tail 先 0x4677F0 がここに存在するため。v314/v315 禁止帯と非重複）。
  - 帯外 VA は浅層記録のみ: 0x416650（本文のみ）、0x441BB9（store 1件）、0x854D84（文脈のみ）、0x5CDC07/0x5F153E/0x608C66 等（直近文脈のみ）。v313_01 の生スキャンは .text 全体（命令境界検証）で、FORBID 帯の3サイト（0x40467C/0x44FE22/0x44FE3F/0x4516E7＝計4）は **VA+命令のみ記録し深解析せず**。
- 判定タグ: **[確定] / [部分確定] / [推定] / [未確定] / [確定負]**
- セクション: `.text` VA 0x401000→file 0x400 (0x626E00) / `.rdata` VA 0xA28000→file 0x627200 (0xD9A00) / `.data` VA 0xB02000→file 0x700C00 (0x30C00)。`F(va)=foff+(va-sva)`（v307_common.py。`pe.get_offset_from_rva(VA-0x400000)` と一致 = v305/v307 で +0x400 ずれ統一済み [確定]）。
- ツール: python 3.11 + capstone 5.0.7（`md.detail=True`）+ pefile（import 目録解決）。raw E8 caller=`va+5+rel==target`。IAT 実名は pefile の import ディレクトリで解決。
- クリーンルーム: OpenTES4 / xOBSE / COEF 等の逆コンパイル由来コードは参照・流用なし。実 .ess データ・B13 実機ダンプなし（§1）→ 動的照合は未実施、証拠不足箇所は [未確定]/[部分確定] で明示。

---

## 1. 解析対象・目的・実 `.ess` サンプルの有無と影響

**目的**: `tmp/re/SAVE_LOAD_RE_FOLLOWUP_v313.md` §優先解析項目の 9 項目を命令証拠で裁定し、正典反映候補を人間確認用に整理する。推測実装の材料を作らない（v309 §6 完了条件④の踏襲）。

**先に読んだ資料（重複調査なし）**: `tmp/re/SAVE_LOAD_RE_FOLLOWUP_v313.md`（正式契約）/ `tmp/re/v309_saveheader_followup.md`（§1 各項目の既確定事項・§3 新規残点9件）/ `tmp/re/OPENCODE_REVERSE_ENGINEERING_INSTRUCTION.md` / `tmp/re/re_util.py`（読取専用）/ `tmp/re/v307_common.py` 依存の既存 probe 類。

### 1.1 実 `.ess` サンプルの有無 — **[確定] 不在**

| # | 検索範囲 | 方法 | 結果 |
|---|---|---|---|
| 1 | ワークスペース `Oblivion_Android` 全深さ（隠し含む・filesize>0） | `**/*.ess` glob + `Get-ChildItem -Force -Recurse` | **0 件**（`tmp/` 領域も包含して 0） |
| 2 | システム %TEMP%（`...\AppData\Local\Temp` 全深さ・隠し含む） | 同上 | **0 件** |
| 3 | ユーザープロファイル `C:\Users\hiroki.kogarumai` 全深さ | `Get-ChildItem -Recurse -Filter *.ess` | **0 件** |
| 4 | `D:\`（Depth 8）＋ `Documents` 直下 | 同上 | **0 件** |
| 5 | `Documents\My Games`（Oblivion セーブ既定先） | `Test-Path` | **ディレクトリ不在** |
| 6 | ゲームインストール先 `D:\Cargo\Ghidra\temp\Oblivion_Steam版` 全再帰 | `*.ess`/`*.bsa` 列挙 | **.ess = 0**（.bsa 17 のみ） |
| 7 | BSA 展開先 | リポジトリ内 `**/*.nif` 等の展開物 | **0 件 → 展開先不在** |
| 8 | `TES4SAVEGAME` 横断（バイナリ含む） | `rg -a -l "TES4SAVEGAME"` ワークスペース | テキスト10件のみ（docs/TODO/STATE）→ **バイナリ .ess なし** |

（親の depth4 予備調査 0 件を、全深さ・隠し・D:・Documents・インストール先・BSA 展開先まで拡張して最終確認。）

**影響**: 契約書 §1 の**動的照合5項目はすべて未実施** — ①ヘッダ size↔実ファイル総長照合 ②offset 38 以降のフィールド境界照合 ③圧縮データ物理位置特定 ④change form/global/quest/cell/AI 境界推定 ⑤FUN_45D5F0 の name/thumb 読取順と実データ照合。**バイナリ構造を推測で確定しない**方針を維持し、本成果物は静的解析のみ。

---

## 2. 使用した probe/script 一覧

実行形: `chcp 65001` → `python $env:TEMP\opencode\v313_<n>.py *> $env:TEMP\opencode\v313_<n>.out`。全本 EXIT=0。依存は既存の `%TEMP%\opencode\v307_common.py`（読取専用・変更なし）。

| script | 内容 | 主な結果（EXIT=0） |
|---|---|---|
| v313_01 | .text 全体 disp32 0x154..0x15F スキャン（命令境界検証897サイト）+ push 0x154/0x160 + vtable 0xA39094 xref + vtable diff | byte +0x154 参照=0x45D57D/0x459472/0x4594CF/0x4594D8/0x459526 のみ、push 0x160=2件、vtable xref=1件、vtable+0x00..0x3C=base と同一 |
| v313_02 | FUN_464320 全 dump + callers + writer/reader 文脈 + 帯内 +0x88 スキャン | FUN_464320=0x464320..0x46443A、max+1 算出、callers 2件（0x45DA88/0x464E4B）、+0x88 サイト14 |
| v313_03 | FUN_465130 prologue/edi 追跡/callers push 列 | edi=定数0（0x4651AB）、callers 6件全 push 序 |
| v313_04 | FUN_45D5F0 全 dump + FUN_465860 の call 文脈 + call targets | 10引数 `ret 0x28`、読取順 F1..F10、0x465A17..0x465A3F の push 列 |
| v313_05 | 0x5E1FD0 / 0x4DE8D0 dump + callers + strings | 5E1FD0=30B→tail jmp 0x4677F0、4DE8D0=0x4DE8D0..0x4DE974 `ret 4`、callers 31/3 |
| v313_06 | FUN_45E6A0 全 dump（0x45E6A0..0x45E934）+ 分岐/call/new | 2 ctor の flag 0/1、seek(0xD000)=esi、チャンク 0xA9000/0xAA000 |
| v313_06b | FUN_45E6A0 の jcc/jmp 全先デコード | 2 jmp は new 成敗スキップと確定 |
| v313_07 | FUN_45D220 全 dump + callers / FUN_4E0F80 bounds+構造 | D220=0x45D220..0x45D383、Havok キュー投入、4E0F80 呼出目録 |
| v313_08 | 0x401000/0x981FB4 once-init | 0x401000=14B CS ラッパ、0x981FB4 callers 2944、0xA17B60 トランポリン |
| v313_09 | IAT 解決 / 0x4677F0 / 459450 / 416650 callers / strings / 465130 epilogue / 45B38A / 459570 / 430970 ctor / 4E0F80 first-ret / seek 定数 / 4DA2A0 / 0xB333C4 生成 / 0x854D84 | IAT 実名12種、4677F0 本体、ctor の [esi+4]/[esi+8]、FUN_465130=`ret 0xC`@0x465856 |
| v313_10 | TD 名 / FUN_459570 文字列 / 0x45B350 帯 head / 'BSX'・'REF' 文脈 / 4E0F80 callers / 4301A0 | TESFullName 同定、'Save '/'-'/'Playing Time' 等、callers 文脈 |
| v313_11 | IAT 追加名 / FUN_45ADC0 範囲 / [0xB33B00] store / BSX 線形境界 | FileTimeToSystemTime・SystemTimeToTzSpecificLocalTime、45ADC0=`ret 4`@0x45B2FD、store=0x441BB9 の1件 |
| v313_12 | 0x45B300 prologue+終端+strings / [0xB33B00] store 再走査 | **FUN_45B300=0x45B300..0x45B668（SaveGame ctor）**、off-by-pad 第5例 |
| v313_13 | FUN_00416650 dump | 整列挿入（3-way compare でノード間挿入）`ret 8` |
| v313_14 | FUN_00459400 dump + callers | スロット全 deleting-dtor 破棄、callers 8件 |

---

## 3. 各項目の結果（9項目・VA/命令/5段階）

### 項目1: 実 `.ess` サンプルの有無 — **[確定]（不在）**

§1.1 の表のとおり全8系統 0 件。**動的照合5項目は未実施**と明記（契約書 §1 条件充足）。バイナリ構造の確定はすべて静的命令証拠に基づく。

### 項目2: `+0x154` byte の意味 — **[確定]（FILETIME 更新時刻キャッシュの valid flag）**

**生成経路（単一）**:
- `FUN_0045D450` @0x45D543 `push 0x160 / call 0x401F00`（new）→ ctor `0x430970(path, flag=0, 0x20000, 0)` @0x45D572 → `mov [esi],0xA39094`（vtable 差替 @0x45D577）→ **`mov byte [esi+0x154],bl`（bl=0）@0x45D57D** → `call 0x416650(list=[edi+0x6C], item=slot, compare=0x459450)` @0x45D58A-0x45D59B。
- .text 全体の `push 0x160` は2件のみ。もう1件 0x854D84 は `call 0x7E2370` への **size 実引数**（`push 0x160 / push ecx / push eax / call 0x7e2370 / add esp,0x18`）で new 導線ではない → **[確定] FUN_45D450 以外の生成経路なし**。
- vtable 0xA39094 の imm32 xref = **0x45D579 の1件のみ** → 差替は1経路 [確定]。

**参照全サイト（命令境界検証済）**: byte +0x154 は **0x45D57D（=0 初期化）/ 0x459472（cmp）/ 0x4594CF（=1）/ 0x4594D8（cmp）/ 0x459526（=1）の5件のみ**（他897検証サイトは全て esp 相対ローカル＝別オブジェクト）。隣接 dword: +0x158（0x45948A/0x4594C3/0x4594E1/0x45951A）/ +0x15C（0x459490/0x4594E7）— すべて `FUN_00459450` 内。→ **参照は FUN_45D450（初期化）と FUN_459450（唯一の使用）の2関数のみ** [確定]。

**意味（FUN_00459450 = 0x459450..0x459561、`ret`、cdecl 2引数）**:
```
459467: mov esi,[esp+0x164]        ; arg1 = slot A
459472: cmp byte [esi+0x154],0 / je 0x459498
45948A: mov ebx,[esi+0x158] / mov esi,[esi+0x15C]      ; キャッシュ済: low/high
459498: lea eax,[esp+0x20] / push eax / lea ecx,[esi+0x3c] / push ecx
4594A1: call [0xA2812C] (FindFirstFileA)               ; パターン = slot のパスバッファ +0x3C
4594A7: cmp eax,-1 / je ... / mov edx,[esp+0x38](=fd.ftLastWriteTime.high)
4594B0: mov ebx,[esp+0x34](=ftLastWriteTime.low) / call [0xA28120](FindClose)
4594C3: mov [esi+0x158],ebx / mov [esi+0x15C],eax / mov byte [esi+0x154],1
（slot B は 0x4594D8..0x459526 で同様）
45952F: cmp esi,edi / jbe / jae ... / cmp ebx,ebp / ja / sbb eax,eax / neg eax  ; 64bit 3-way
459561: ret
```
→ **+0x154 = FILETIME（ftLastWriteTime）キャッシュ valid flag、+0x158 = low、+0x15C = high。0x459450 = 2スロットのファイル最終更新時刻による3-way比較関数（-1/0/1）** [確定(命令)]。

**一覧との関係**:
- `FUN_00416650`（0x416650..0x416703、`ret 8`、thiscall）本文: ノード走査中 `push eax(既存item) / push ebp(新item) / call [esp+0x24](arg2=compare)` → `test eax / jg` で降下、≤ で新規ノード(8B)を `edi` と `esi` の間に挿入（0x4166A2-0x4166BF）、末尾なら追加 → **compare の3-way結果を使う整列挿入** [確定(命令)]。
- → **SaveGame+0x6C のスロットリストは最終更新時刻順に整列保持** [確定]。セーブ一覧の並び順に使用 [推定]（一覧生成 FUN_45D450=v309 確定との接続は命令上直接）。
- 真の dtor は別: `FUN_00459400`（0x459400、tail-jmp 0x5AE430）が各 item に **`mov eax,[ecx] / mov edx,[eax] / push 1 / call edx`（deleting dtor）** を叩き全スロット解放（callers 8件: 0x45D494 / 0x464417 / 0x5AE1F5 / 0x5AE9CD / 0x5AEF4E / 0x5B5914 / 0x5B5DB7 / 0x5D3929）→ **一覧破棄→再構築毎に new(0x160) でキャッシュは0から再生成** [確定]。

**vtable 配下メソッド**: 0xA39094 の +0x00..+0x3C は 0xA3636C と全スロット同一（base 実体）→ base 実装は size 0x154 のオブジェクトしか参照しないため **+0x154 に到達しない** [確定負]。ロード/削除/バックアップ系の関数（FUN_45F2E0/FUN_465860/FUN_45D450 等）に +0x154 参照なし（帯内スキャン）[確定]。

**残る不確定点**: +0x155..0x157（pad、参照なし [確定負]）、+0x15D..0x15F は +0x15C dword の上位（=FILETIME.high に包含）。

### 項目3: `[esi+0x88]` counter のライフサイクル — **[確定]**（dry/real 両パス同居時の増加挙動のみ [未確定]）

**ヘッダ内対応フィールド = offset38（size フィールド直後）の u32** [確定]（writer 0x464E74 の 4B 書出が v309 item6 表の offset38 行、reader は F1）。

**① 初期化 0 — SaveGame ctor を新同定**:
- **`FUN_0045B300` = 0x45B300..0x45B668（`ret`）** — **off-by-pad 第5例**: 直前 FUN_0045ADC0 の `ret 4` が 0x45B2FD..0x45B2FF で終了し、**0x45B300 に CC パディング0で新関数が開始**（`func_head(0x45B300, minrun≥1)=False`。教訓の4例 0x466D4D/0x5AE18D/0x44F3C1/0x51B484 に続く第5例）。head 探索時は必ず直前バイト列を確認すること。
- 本文: SEH prologue → `mov esi,ecx / xor ebx,ebx` → フィールド0初期化（+0x04..+0x40, +0x4C, +0x50, +0x64..+0x6C, +0x70=0, +0x71=0, **+0x7D=1**, +0x80, +0x84, **+0x88=0 @0x45B38A**, +0x8C, +0x90）→ sub-container 生成（new×11 + ctor 0x45A860/0x45A940/0x45AA80/0x45ABE0、vtable 0xA39044 ×2）→ 尾部 **[esi+0x94..0xA3]=0（SYSTEMTIME 0クリア @0x45B630）**、`mov al,0x7D / [esi+0x71]=al / [esi+0x7c]=al / [esi+0x70]=0`、`mov eax,esi`（this を返す）。
- 生成点: **0x441BB0 `mov ecx,eax / call 0x45b300` → 0x441BB9 `mov dword [0xb33b00],eax`**（失敗時 `xor eax,eax` で0）。[0xB33B00] の store 形（A3 / ModRM05）全走査で **この1件のみ** → **singleton はここでのみ生成/書込** [確定]。
- → **`[esi+0x88]` の初期化0 = SaveGame ctor FUN_0045B300 @0x45B38A** [確定]。

**② 保証初期化（counter==0 時の再計算）— callers 2件**:
- writer: `FUN_00464AB0` @0x464E40 `cmp dword [esi+0x88],0 / je 0x464e50` 直前 @0x464E49 `mov ecx,esi / call 0x464320`（実際は `cmp`→`je`→`mov ecx,esi; call` @0x464E4B）。
- 名前生成: `FUN_0045D920` @0x45DA7D `cmp [ebx+0x88],0 / jne / mov ecx,ebx / call 0x464320`。

**③ `FUN_00464320`（0x464320..0x46443A、`ret`）の算出式**:
```
464327: mov ebx,[edi+0x6c]（スロット一覧）/ 無ければ call 0x45d450 で構築（自前フラグ [esp+0xB]）
ループ（各スロット）:
 464363: push 0 / push 0 / lea eax,&F+0x10 / push eax / push ebp(slot) / call 0x459570
 464374: test al,al / jne 0x4643f3          ; 成功 = ファイル名から連番取得 → F+0x10 に値
 464378: push 2 / push 0 / push ebp / call 0x45f2e0   ; mode2 既存再 open
 46438A: cmp byte [esi+0x24],0 / je ...     ; ctor の exists フラグ
 464395: push 0 / push esi / call 0x45dbc0  ; ヘッダ読（size 返）
 4643D6: call 0x45d5f0（10引数, arg3=&F+0x10）  ; F1 = offset38 counter u32 を F+0x10 へ
 4643F3: mov eax,[esp+0x18](=F+0x10) / cmp eax,[esp+0x1c](=F+0x14) / mov [esp+0x1c],eax  ; max 追跡
終了後:
 46441C: [edi+0x71]=[edi+0x7c]=0x7D / [edi+0x70]=0
 464424: mov eax,[esp+0x14](=F+0x14) / add eax,1 / mov [edi+0x88],eax   ; **max + 1**
 46440E: 自前構築フラグなら call 0x459400（破棄）
```
- **初期値 = max(既存 .ess のファイル名連番, ヘッダ offset38 値) + 1** [確定(命令)]。`FUN_00459570`（0x459570..0x45977F、`ret 0x10`）= `strrchr(path,0x5C)`（0x983FD0= strrchr 相当 [推定]）で末尾ファイル名を得、`strncmp(filename,"Save ",5)`（0x984166）＋ `strstr(filename,"-")`（0x9840E0）、`memcpy` で "Save " 直後〜'-' の数値部を抽出 → `0x983479`（atoi 相当 [推定]）→ `mov [ebp],eax`（*arg2）→ bool 戻り [確定(命令)]。
- 同型の利用がもう1件: `FUN_00463EC0` @0x46408A `call 0x459570` → `test al,al / jne 0x46430e` else mode2 再open（0x46409E）→ ヘッダ読 — **「ファイル名解析優先・失敗時はヘッダ読取」の2段階** [確定]。`FUN_459570` callers = 0x46408A / 0x46436F の2件のみ。

**④ writer での保存と増加（FUN_00464AB0）**:
```
464E14: mov ebx,[esp+0x78]（arg2 = セーブ名）
464E1C: push 'quicksave'(0xA3B144) / push ebx / call 0x9840E0 / test / jne 0x464e99
464E2E: push 'autosave'(0xA3AAD4)  / push ebx / call 0x9840E0 / test / jne 0x464e99
464E40: cmp dword [esi+0x88],0 / je → call 0x464320
464E53: mov ecx,[esi+0x88] / mov [esp+0x34],ecx / test dry / je 0x464e74
（dry） 464E65: add [esi+0x90],4 / **add [esi+0x88],ebp(1)** / jmp 0x464ed1
（real）464E74: ...4B 書出（値=[esp+0x34]） / 464E91: **add [esi+0x88],ebp(1)**
（quicksave/autosave/dry 分岐先 0x464E99）: mov [esp+0x34],0 → 0 または size計上のみ、**counter 増加なし**
```
- → **通常保存: ヘッダ offset38 に現在値を書き出し、書き出し毎に +1** [確定(命令)]。**quicksave/autosave: offset38 に 0 を書出し counter は不変** [確定(命令)]（strcmp/strstr 一致で 0x464E99 経由）。dry（[this+0x18] bit9）時も counter+=1 が生じる。
- **[未確定] dry→real の2パスが同一セーブで連続する場合の増加挙動**（動態証拠・実 .ess 無し。2パスなら +2 となり実ファイルの連番と乖離しうるが、実行序列は静的に確定できない）。

**⑤ 復元（reader 側）**: FUN_464320 の ②（上記 F1 読取）＝ writer と**同一フィールド（offset38 u32）**を扱う [確定]。

**⑥ 使用**: `FUN_0045D920` @0x45DA7D-0x45DA8D `mov ebx,[ebx+0x88]` → sprintf `0xA3A8A4 = 'Save %i - %s - %s, Level %i, Playing Time %02i.%02i.%02i'` の先頭 `%i`（arg3=ebx）に counter を渡す → **自動生成保存名の連番 "Save N"** [確定]。ファイル名とヘッダの双方に N が載るため、次回の FUN_459570 がファイル名から復元できる構造 [確定]。

**⑦ オブジェクト数/change form 数等との関係**: **[確定負]** — FUN_464320 の計算に計数処理は存在せず、「ファイル名連番 or ヘッダ同値」の max+1 のみ。セーブ対象数・change form 数・オブジェクト数は関与しない。

### 項目4: `FUN_00465130` の mode 引数 — **[確定]（mode = 恒 0）**

- **範囲（v309 補正）**: **0x465130..0x465856（`ret 0xC` @0x465856、epilogue 0x465843〜）**。v309 時点の probe は func_bounds が 0x467920 まで過走（linear first-ret で訂正）。直後 0x465860 = FUN_465860（v309 確定と整合）。
- **edi の出所（引数/ローカル/定数の判定）**:
```
465144: push edi（レジスタ退避）
465174: mov edi,0xA3AAD4   （'autosave' 比較用・この先は reject 経路または je 0x4651a5）
465184: je 0x4651a5         （一致で merge）
4651AB: **xor edi,edi**     （merge 経由の全経路が必ず通過: 0x465165 jne / 0x46516E jne / 0x465184 je の三つ）
4651B1..0x4651C3: call 0x683490 / 0x432860 / 0x461680（edi callee-保存）
4651D9: push edi            ← FUN_45F2E0 の arg3 = mode へ
4651DE: call 0x45f2e0
```
→ **edi は prologue 退避＋定数化（xor edi,edi）のみ。mode は常に 0** [確定]。v309 の「arg3 = edi [部分確定]」を解消。mode0 = FUN_45F2E0 の「書モード新規」パス（.bak ローテーション・name 生成、v309 item10 表）と整合 [確定]。
- **引数契約（補足）**: arg1=[esp+0x6C]=既存オブジェクト or 0、arg2=[esp+0x70]=セーブ名(basename) or 0、arg3=[esp+0x74]=名前新設フラグ。FUN_45F2E0 へは (arg1, arg2, mode=0) を転送 @0x4651D5-0x4651DE。
- **呼び手6件（push 順で確定）**:

| 呼び手 | 文脈 | arg1 / arg2 / arg3 |
|---|---|---|
| 0x466AF6 | quicksave（head 0x466AD0） | 0 / **'quicksave'(0xA3B144)** / 0 |
| 0x466BD0 | autosave（head 0x466B70） | 0 / **'autosave'(0xA3AAD4)** / 0 |
| 0x466D21 | 'Test All Cells %i.ess'（0xA3B518）テスト保存 | 0 / sprintf 済みテスト名 / 1 |
| 0x5024BA | console SaveGame | 0 / &namebuf / setne flag |
| 0x5D3246 | 通常セーブ | 0 / 0 / 0 |
| 0x5D3438 | 既存スロット再保存（item=[esi]） | slot / 0 / 0 |

- **ガード**: dry フラグ or `call 0x452330` 真 → 続行。偽なら arg2 を 9B 比較（`repe cmpsb` vs 'autosave'@0xA3AAD4）一致のみ続行、他は reject（0x57acc0 メッセージ）→ **通常保存不可時の autosave 例外** [部分確定]（0x452330 の厳密な意味は未追跡＝残件）。
- 全 caller が mode=0 → **autosave/quicksave/通常save のいずれも FUN_45F2E0 mode0（書新規）経由** [確定]。

### 項目5: `FUN_0045D5F0` の name/thumb 読取順 — **[確定]**（thumb tail の最終同一照合のみ [部分確定]）

- **範囲（v309 補正）**: **0x45D5F0..0x45D919（`ret 0x28` = 10引数）**。v309 の probe bounds 0x45F6AC は func_bounds 過走 → 訂正。`[this+0x14]` = バッファ/カーソル、終端で `call 0x401E40`（free）＋ [this+0x14]=0。
- **前処理（バッファ化読取）**: arg2(size) を容量に `new[]`（thiscall 0x401E20、ecx=0xB02020、失敗時ログ 0xA390D4 = 'Could not create save buffer, out of memory.'）→ `[this+0x14]=buffer` → **`mov edx,[ebx+4]（ebx=arg1=slot、[+4]=ctor 登録の read ファンクタ 0x430050）/ push 1 / lea ecx,&arg2 / push ecx / push edi(size) / push eax(buffer) / push ebx / call edx / add esp,0x14`** = **slot から size バイトを buffer へ fill** → カーソル [this+0x14]=buffer 先頭から走査 [確定]。
- **引数表（10個・push 順で確定）**: arg1=slot/stream、arg2=size、arg3=F1出力、arg4=F2出力、arg5=F3出力、arg6=F4出力、arg7=F5出力、arg8=F7出力、arg9=F6出力、arg10=F8-10出力。
- **読取順（全 gate・命令）**:

| # | gate | 読取形式 | 出力 | 対応する writer 列（v309 item6） |
|---|---|---|---|---|
| F1 | ver≥0x38（<0x38 なら 0） | u32 | *arg3 | **offset38 counter**（0x464E74） |
| F2 | 常時 | u8 len + len B memcpy（arg4==0 なら len 分のみ進める） | *arg4 | name1（len=strlen+1、0x464ED1/0x464F00） |
| F3 | ver≥0x34（else 0） | u16 | *arg5 | u16 = Level（0x464F34） |
| F4 | 常時（len0 は既定文字列 [0xB38728] をコピー） | u8 len + len B | *arg6 | name2（0x464F65/0x464F97） |
| F5 | 常時 | u32（`fld`/`fstp` float） | *arg7 | 日数 float（0x464FCF） |
| F6 | ver≥0x1D（else 0） | u32 | *arg9 | 遊戯時間 ms（0x465000） |
| F7 | ver≥0x38: ファイル16B / <0x38: `call 0x4301A0(slot,&local)` の返り16B | 16B | *arg8 | SYSTEMTIME（GetLocalTime、0x465044） |
| F8 | 常時 u32（==0 で終了） | 詳下記 | — | thumb size（0x465075） |
| F9/F10 | F8≠0 かつ arg10≠0 のみ | u32,u32（第2値→*arg10、ver<0x2E で +0x24 skip、new(0x70)+ctor 0x70E560+`imul edi,ebx; lea edi,[edi+edi*2]` の ×3B memcpy+`0x701FC0`+InterlockedIncrement、**戻り=配列**） | *arg10/返値 | thumb ブロック（4B+4B+pixels、0x4650A5/0x4650B4/0x4650C3） |

- **型・位置は writer 列と完全1:1**（u32/len-str/u16/len-str/float/u32/16B/4B+block）→ **name1→u16→name2 の順は writer の書き出し列と一致** [確定]。既存 ledger の header_field_reader ノート（「ver>0x37:u32 / u8長+文字列 / ver>=0x34:u16 / u8長+文字列(氏名) / u32 / ver>0x1c:u32 / ver>0x37:16B / 0x70確保構造 iVar16*iVar5*3B, ver<0x2e で+0x24skip」）とも一致 [確定]（本件は命令列からの独立再導出）。
- **`0x465A3B` の size 引数の使われ方 [確定]**: push ebx(size) = arg2 = **new[] の容量であり、fill の読取バイト数**（&arg2 で実読量が上書きされうる）。
- **呼び手2件の対比 [確定]**:
  - `FUN_00464320` @0x4643B3-0x4643D6: arg3=&F+0x10（counter）、arg5=&F+0x0C、arg7/8/9=frame 内 temp、arg4/arg6/arg10=0（name1/name2/thumb スキップ）。
  - `FUN_00465860`（ロード実行）@0x465A17-0x465A3F: arg2=ebx(size)、**arg4=&[SaveGame+0xB0]（name1）、arg5=&+0x1B4（u16）、arg7=&+0x1B8（float）、arg9=&+0x1BC（playtime u32）**、arg3=arg6=arg8=arg10=0 → **ロード時は counter/name2/SYSTEMTIME/thumb を本関数で読まない** [確定]（counter は次保存時に FUN_464320 がファイル群から再計算）。
  - v309「12引数」→ **10引数**（push 数え上げ、`ret 0x28` と一致）に訂正（旧記録は残す）。
- **F7 の旧版フォールバック = `FUN_004301A0`（0x4301A0..0x430233、`ret 4`）**: [this+0x3C] パスで `FindFirstFileA` → ftLastWriteTime → **`FileTimeToSystemTime`（IAT[0xA28138]）→ `SystemTimeToTzSpecificLocalTime`（IAT[0xA28134]、lpTZ=NULL）** → 16B ローカル SYSTEMTIME [確定]。**ver<0x38 の SYSTEMTIME = ファイル最終更新時刻から生成** [確定]。
- **thumb tail の対応 [部分確定]**: writer の thumb size は `mov ecx,[end]-[begin]; add ecx,8`（=pixels+8）、ブロックは 4B+4B（初期値 0x100,0x100 = 256×256 [推定]）+ pixels、reader F8 のスキップ値一致・×3B コピー（RGB24 w×h×3）・0x70E560 画像 ctor と整合 → **F8=u32 size(=8+w*h*3), u32 w, u32 h, RGB24 pixels** [推定〜部分確定]。実 .ess 照合なしで最終確定は不可（契約書 §1）。F8 以降より後の領域（0x4650EE の `[SaveGame+0x40]` 経由 0x4531B0 呼出等）は**本関数では読まれない**（F8 が最後のフィールド）[確定]。

### 項目6: 名前取得関数 `0x5E1FD0`・`0x4DE8D0` の正体 — **[確定]**

**`0x5E1FD0` = 0x5E1FD0..0x5E1FFF（30B、`jmp 0x4677F0` テールジャンプ、スタック引数なし）**:
```
5E1FD0: mov esi,ecx（this=[0xB333C4]）
5E1FD4: mov eax,[esi] / mov edx,[eax+0x170] / call edx   ; vfunc+0x170(this) → サブオブジェクト
5E1FE1: test / je 0x5e1ff9
5E1FE7: vfunc+0x190(this) → bool 必須 / 否なら ebx=0
5E1FFB: lea ecx,[ebx+0x24]（obj==0 なら ecx=0x24）/ pop / jmp 0x4677f0
```
**`0x4677F0` = 0x4677F0..0x467879（`ret`、スタック引数0・[esp+4] は push ecx のローカル）**:
```
4677F7: movzx eax,word [esi+0xE]                 ; base 値
4677FB: shr [esi+4],7 の bit1 判定
467807: vfunc+0x170([0xB333C4]) → lea ecx,[eax+0x24] / add ax,word [ecx+0xE]  ; bit7 立ちなら加算
467832: word [esi+0x10]（>0）より小さい → ax に上限クランプ
467851: word [esi+0x12]（>0）より大きい → ax に下限クランプ
467864: cmp ax,1 / jge → ax<1 なら ax=1
```
→ **u16 の意味 = 'Level %i'**（`FUN_0045D920` の sprintf 0xA3A8A4 へ `movzx edx,ax` を arg5=Level として渡す @0x45DAAA-0x45DAB1）→ **ヘッダ F3 u16 = レベル（旧版は base 加算＋上下限クランプ＋最低1）** [確定]。**2B フィールドとの対応: F3 u16 = 0x5E1FD0 戻り** [確定]。callers=31件（保存帯 0x45DAA1/0x464C2D/0x41AE31、他は汎用 getter）。

**`0x4DE8D0` = 0x4DE8D0..0x4DE974（`ret 4`、thiscall、arg1=&TString スタック）** — callers **3件のみ**: 0x45D9AB（FUN_45D920 名前生成）/ 0x464C58（FUN_464AB0 writer）/ 0x5B9065。
```
(1) [this+0x40]≠0 かつ call 0x4C97F0 真 → edi=[this+0x40]+0x1C（NULL なら ""(0xA2F7EC)）
    → push 0 / push edi / mov ecx,arg1 / call 0x4028D0（thiscall 文字列代入）→ ret 4
(2) else → [this+0x40] or vfunc([this+0x18]) → call 0x4C9CF0 → ebx / vfunc+0x174(this) の位置情報等を
    sub esp,0xC で積み → [ebx+0xDC] の vfunc へ3引数 call → ret 4
(3) else → 0x4028D0(&arg1, ""(0xA2F7EC), 0) → ret 4
```
→ **name2 = [0xB333C4] のサブオブジェクト由来文字列（取得不能時は空文字）を arg1 の TString へ代入して返す getter** [確定(命令)]。呼び出し側（writer 0x464C58）は `mov eax,[esp+0x4c] / cmp eax,ebx(0)` → NULL 時は `call 0x6ecc80` → `vfunc+0xD4` の名前フォールバック [確定]。

**name1 = `0x4DA2A0`（0x4DA2A0..0x4DA2CA、`ret`）**:
```
4DA2A0: mov ecx,[ecx+0x1C]
4DA2B2: call 0x9832E6 = __RTDynamicCast(arg, 0, TESForm@0xB02F9C, **TESFullName@0xB03298**, 0)
4DA2BE: 成功時 mov eax,[eax+4]（char* name）/ 失敗時 eax=0xA2F7EC（""）
```
→ **name1 = [this+0x1C] が TESFullName を実装していればその name、非該当は空** [確定]。

**writer/reader 双方の関係 [確定]**: writer name1=0x464C0A、u16=0x464C2D、name2=0x464C58 → 書出し列 name1(F2)→u16(F3)→name2(F4)。reader は F2/F3/F4（項目5表）。**長さ/終端**: writer は `strlen+1`（NUL 含み）を長さ byte に（0x464C13-0x464C29 / 0x464C8D-0x464C9D）、reader は u8 len + len B（len0 は既定文字列コピー）→ **NUL 終端バイト列・ANSI バイト** [確定]（符号化はシステム ANSI [推定]）。

**`[0xB333C4]`（getter 系の this）**: `push 0x804 / call 0x401F00（new）` → ctor `0x66A740` → `mov [0xb333c4],eax`（0x46451D..0x464553、`call 0x46C300(eax,0x14,1)` 初期化）→ name1/name2/level/遊戯時間（+0x710/+0x714 累積 @0x45DA23-0x45DA3F）等の getter の this → **プレイヤー相当単一オブジェクト** [部分確定]（正確なクラス名の RTTI 同定は未実施＝残件）。

**引数矛盾の解消 [確定(命令)]**: 0x4677F0 はスタック引数を読まないため、writer 呼び出し（push 0 件）と整合。FUN_45D920 の call 前4push（&buf＋ハッシュ3個）は本体未使用で、後続 `add esp,0x30`（= 残余16 + sprintf 6×4 + strchr 2×4 = 48）で一括後始末される（4push は sprintf の `%02i.%02i.%02i` に渡されるプレイタイム分解値の退避位置を兼ねる）。

### 項目7: `FUN_0045E6A0` の ctor 分岐 — **[確定]**

**範囲**: 0x45E6A0..0x45E933（`ret` @0x45E933）= v309 と一致。

**2つの jmp の正体（契約 §7-2）**:
- `0x45E60 je 0x45E77C`（new 失敗→esi=0）/ **`0x45E77A jmp 0x45E77E` = ctor 成功時のみ esi を維持して合流**（失敗分岐 0x45E77C の `xor esi,esi` を飛ばす）。
- `0x45E7D1 je 0x45E7F2`（2つ目の new 失敗→[esp+0x18]=0）/ **`0x45E7F0 jmp 0x45E7F6` = 成功時のみ合流**。
- → **2 jmp はいずれも「new 成功/失敗のスキップ」で、分岐実体はアロケーション成否のみ** [確定]。v309 の「細部未追跡（新規残点6）」を解消。

**各 ctor と seek の紐づき [確定]**:
| | 生成 | ctor 引数 | 開く方式 |
|---|---|---|---|
| ctor#1 | 0x45E746 `push 0x154`（XBoxSaves\%s @0xA3A9E4） | `0x430970(path, **flag=0**, 0x20000, 0)` @0x45E773 | ctor は exists チェックのみ（+0x24）→ **0x45E77E で vtable+0x18 open 明示呼出** |
| ctor#2 | 0x45E7B3 `push 0x154`（%s%s%s.ess @0xA3A88C） | `0x430970(path, **flag=1**, 0x20000, 0)` @0x45E7E5 | **ctor 内で即 open**（`cmp [esp+0x2c],1 → call 0x42FE80`） |

- **seek(0xD000) の対象 = ESI = ctor#1（XBoxSaves ストリーム）**: 0x45E7F6-0x45E81D `mov eax,[0xA853D0] / mov edx,[esi] / mov edx,[edx+0xC] / push eax / push 0xD000 / mov ecx,esi / call edx` [確定]。ctor#2 は EDI（.ess 書出先）で seek されない。
- ctor 0x430970 の flag 契約（v309 item2 補完）: flag==1 → 即 open（書モード導線）、flag==0 → `GetFileAttributes` 相当（0x982D71、結果 -1 で `setne`）で **[esi+0x24]=exists フラグ** のみ設定 [確定]（実名は内部 CRT のため [推定]）。path は +0x3C へ MAX_PATH(0x104) 未満のみコピー、長さ超過なら空 [確定]。
- **0xAA000 / 0xA9000 の意味 [確定]**: コピーループの**チャンク読みサイズ**（1回目 bl=1 で 0xA9000、以降 0xAA000 @0x45E830-0x45E83E）。read は `[esi+4]` ファンクタ（ctor 登録 0x430050）で `(&bigbuf(0xAA468 chkstk スタック), chunk, &got, 1)`、write は `[edi+8]` ファンクタ（0x430070）。read 後に **seek ステップ: 1回目 0x4000 / 以降 0x2000、origin=[0xA853D4]=1（SEEK_CUR）**、残サイズ（WIN32_FIND_DATA nFileSize − 0xD000 @0x45E80C `add ebp,0xFFFF3000`）から got+step を減算して `jg loop`（0x45E8B8）。
- 後始末: 両ストリーム deleting dtor（vfunc+0,1）、`DeleteFileA`（[0xA280AC]）、`FindNextFileA` ループ、`FindClose` [確定]。
- **'CON ' 検査なし（v309 確定）と整合**: 本関数は XBoxSaves ディレクトリ列挙＋0xD000 前提で、**CON→.ess 変換（インポート）経路** [確定]。new は両方 0x154（0x160 スロットではない）[確定]。

### 項目8: `FUN_004E0F80` と `FUN_0045D220` — **[部分確定]（D220 は [確定]、4E0F80 は構造まで）**

**`FUN_0045D220` = 0x45D220..0x45D383（`ret 4`）全体像 [確定]**:
```
45D220: thread ゲート: GetCurrentThreadId([0xA2808C]) vs [0xB33398]+0x10 → [this+0x18] bit0 で early-out
45D24E: esi = arg1 = [esp+0x20]
45D256: __RTDynamicCast(arg1, 0, TESObjectREFR@0xB03054, Actor@0xB03070, 0) → edi
45D271: [SaveGame+0] マップ lookup（0x55E000、キー=[arg1+0xC]=formID）
         REFR 経路: lookup 値 → push → call 0x5F0310(edi,…) → al 真で続行 / 偽なら第2系統へ
45D29F: 第2系統: 同 lookup → 値の bit3（test al,8）必須 / 偽なら out(0x45D37C)
45D2C9: call 0x4E0970(arg1, &local) → ax = u16 サイズ（+ local 構造）
45D2F0: 非 REF R時は [local+?]+[local+?] の和==1 で除外 / ax==0 で除外
45D30C: push 0xA3A82C = 'TESSaveLoadGame::SaveQueuedHavokData' / mov ecx,0xB33B80 / call 0x401020（ログ）
45D31B: vfunc+0x40(edi, 8)（REFR フラグ設定）
45D32A: new(u16+2) / mov word [edi],bp（u16 ヘッダ）/ [SaveGame+0x14] へ一時設置
45D34C: call 0x4E0A40（シリアライズ）→ 0x4211E0 で [arg1+0x44] へ積む / [SaveGame+0x14]=0
45D364: vfunc+0x40(arg1, 0x1000000) / call 0x401040（ログ終了）
```
→ **D220 = SaveQueuedHavokData（物理/Havok 状態データの保存キュー投入: マップゲート → サイズ取得 → u16 付きバッファ生成 → [arg1+0x44] へ格納 → フラグ）** [確定(命令)]。

**`FUN_0045D390`（v309 確定の SaveQueuedAttachedAnimation）との役割分担 [確定]**:
| | FUN_45D220 | FUN_45D390 |
|---|---|---|
| キュー | **SaveQueuedHavokData**（ログ 0xA3A82C） | **SaveQueuedAttachedAnimation**（ログ 0xA3A854） |
| サイズ取得 | 0x4E0970 | 0x4E0840 |
| 生成/格納 | 0x4E0A40 → **0x4211E0** | 0x452310 → 0x4E08D0 → **0x4210E0** |
| ゲート | SaveGame マップ lookup bit3 ＋ 0x5F0310 | マップ値 `test dword [eax],0x2000000` |
| flag | vfunc+0x40(...,8) + 0x1000000 | vfunc+0x40(...,0x1000000) |
→ **2つは別種（Havok vs AttachedAnimation）の保存キューを扱う対関数** [確定]。v309 項目11 の訂正（D390=キュー保存チェック）は維持し、D220 側を対として追補。

**呼び手構文（0x4E0F80 内）**: 0x4E1160 `[ebp+8]>>5&1 → jne 0x4E11A7`（bit5 で全体スキップ）、0x4E116F `vfunc+0x198(edi)` 判定、**0x4E1181/0x4E1188 D220（片方は `jmp 0x4E11A7` 終了経路）→ 0x4E118F/0x4E1196 D220 → 0x4E119B D390 の連鎖**、0x4E11A7 `lea ecx,[ebp+0x44] / call 0x41E650` [v309 の踏襲＋jmp 経路を追補]。

**`FUN_004E0F80` 全体像 = 0x4E0F80..0x4E1571（`ret 4` @0x4E156F、506命令）** [部分確定]:
- **callers 7件**: 0x5CDC07 / 0x5F153E / 0x608C66 / 0x60E45F / 0x6121EF / 0x62505B / 0x6251EC。0x5CDC07 の直前 `mov ecx,[0xB333C4] / mov edx,[ecx+0x3C] / mov ebp,[edx+0x1C] / push ebx / call 0x4e0f80`（プレイヤー経路）、0x608C66 は `push 0 / mov ecx,esi`。
- **内容の分類**:
  - リフカウント: `call [0xA28078]=InterlockedIncrement` / `[0xA2807C]=InterlockedDecrement`（0x4E1077/0x4E10D2/0x4E11D7/0x4E132E/0x4E136C、Dec 後 `test eax,eax / jne / vfunc+0(...,1)` deleting dtor）、`new(0x401F00)` @0x4E14CC。
  - vfunc 使用: +0x380（×2）/+0x164/+0x188（×2）/+0x470/+0xF0/+4/+8、flag push 0x2000000 等。
  - **種別キー: `push 0xA7D0EC('BSX') / mov ecx,ebx / call 0x6FF9C0`（0x4E128A-0x4E1291）、`push 0xA3CEB0('REF') / mov ecx,ebx / call 0x6FFAC0`（0x4E131E-0x4E1325）** → 名前キーによる追加データ検索/操作 [部分確定]（'BSX'=ExtraBSXFlags 系 [推定]、'REF' 実体 [未確定]）。
  - container/リスト系 0x7B4280/0x7C5E70/0x7C76D0/0x7C7DC0、active-ref リスト 0x41E630/0x41E650/0x41E6B0/0x41F590/0x41F5B0、その他 0x64FFF0/0x6FF9C0/0x4DE1C0/0x65A2C0/0x4D9A50/0x6AC420/0x678D90/0x607B90/0x6FFAC0/0x977C50(=timeGetTime 相当 [推定]) 等。
  - **保存キュー連鎖の実行**: 内部で 0x45D220 ×2 → 0x45D390（上記）。
- → **全体像 = 保存処理中の per-object 後処理ルーチン（リフカウント管理・'BSX'/'REF' 種別分岐・active-ref 操作・Havok/AttachedAnimation キュー投入）** [部分確定]。残: 各枝の厳密な責務と 'BSX'/'REF' の実体名（＝残件）。
- **v309 訂正**: 「0x4E0F80 文字列参照0件」→ **imm32 push で 'BSX'/'REF' の2件を検出**（rdata 0xA28000..0xB01A00 全走査）。

### 項目9: `FUN_004302B0` once-init（0x401000 / 0x981FB4）— **[確定]**

- **`0x401000` = 0x401000..0x40100D（14B）**: `push esi / mov esi,ecx / push esi / call IAT[0xA28064] / mov eax,esi / pop esi / ret`。**IAT[0xA28064] = KERNEL32.dll!InitializeCriticalSection**（pefile import 目録で実名解決）→ **0x401000 = InitializeCriticalSection(ecx) ラッパ（this を返す）** [確定]。callers 21件（`mov ecx,global / call` または `lea ecx,[obj+0x80/0x200] / call` = CS 初期化パターン）。
- **`0x401010` = 0x401010..0x401017**: `push ecx / call IAT[0xA28068] / ret`、**IAT[0xA28068] = DeleteCriticalSection** → **dtor トランポリン本体** [確定]。
- **`0x981FB4` = atexit**: **callers 2944件**、全形が `push <handler> / call 0x981FB4 / add esp,4`（cdecl 1引数）。handler は .text 尾部の静的 dtor トランポリン群（例 **0xA17B60 = `mov ecx,0xB33980 / jmp 0x401010`** = DeleteCriticalSection(&0xB33980)）[確定]。v309 の [推定「0x981FB4=atexit 系」] を [確定] に昇格。
- **FUN_4302B0 の once-init の実体**（v309 項目1の [部分確定] を解消）:
```
4302D8: test byte [0xB33A00],1 / jne 0x43030d      ; ガード bit0
4302E0: or dword [0xB33A00],1                       ; 二重 init 防止（先行設定）
4302E6: mov ecx,0xB33980 / call 0x401000            ; InitializeCriticalSection(&グローバル CS)
4302F8: push 0xA17B60 / call 0x981FB4 / add esp,4   ; atexit(DeleteCriticalSection トランポリン)
```
→ **CRT 起動処理ではなく、関数内遅延初期化（グローバル CRITICAL_SECTION 0xB33980 の初回 InitCS）＋ atexit による終了時解放** [確定]。read/write 経路への影響: 初回 read/write の前に必ず CS 初期化が走るガード（ストリーム層の排他ロック）で、init は1度のみ・失敗時は例外送出のみ（関数は戻り値なし）[確定(命令)]。他のグローバルも同型（例 0x6B0CCC `or [0xB3C180],1 / mov ecx,0xB3C100 / call 0x401000 / push 0xA26270 / call 0x981FB4`）[確定]。

---

## 4. 主判定表（9行）

| 項目 | 判定 | 根拠VA | 残る不確定点 |
|---|---|---|---|
| 実 `.ess` サンプル | **[確定]（不在）** | §1.1 の8系統（rg -a / Get-ChildItem 全深さ等） | 動的照合5項すべて未実施（サンプル入手後に再実施要） |
| +0x154 byte | **[確定]** | 0x45D57D（初期化0）、0x459450..0x459561（FindFirstFileA＋FILETIME キャッシュ＋3-way 比較）、0x416650..0x416703（整列挿入）、0x459400（dtor 破棄）、0x45D543/0x45D579/0x854D84（生成経路否定） | +0x155..0x157 は pad（参照 [確定負]）。UI 表示への接続は [推定] |
| +0x88 counter | **[確定]** | 初期化: **FUN_0045B300 @0x45B38A**（0x441BB9 singleton store）、算出: 0x464320（0x464424 max+1、F1 読取 0x4643D6、ファイル名解析 0x459570@0x46436F）、書出/増加: 0x464E40-0x464E97、quicksave/auto=0: 0x464E1C/0x464E2E→0x464E99、使用: 0x45DA7D-0x45DABF | dry→real 両パス同居時の増加回数 [未確定]（動態/実 .ess 無し） |
| FUN_00465130 mode | **[確定]** | 0x4651AB `xor edi,edi` → 0x4651D9 `push edi`、範囲 0x465130..0x465856 `ret 0xC`@0x465856、callers 6件 push 列 | ガード FUN_452330 の厳密な意味 [部分確定] |
| name/thumb 読取順 | **[確定]** | FUN_45D5F0 = 0x45D5F0..0x45D919 `ret 0x28`（F1..F10: 0x45D67E/0x45D6AC/0x45D6DF/0x45D714/0x45D763/0x45D789/0x45D7AF/0x45D7F8/0x45D80E）、fill=[arg1+4]=0x430050、0x4301A0 フォールバック（FileTimeToSystemTime IAT0xA28138） | thumb tail の最終同一照合（F8=8+w*h*3）[部分確定]、実データ照合は未実施 |
| name 取得関数 | **[確定]** | 0x5E1FD0=0x5E1FD0..0x5E1FFF→0x4677F0=0x4677F0..0x467879（Level クランプ）、0x4DE8D0=0x4DE8D0..0x4DE974（3系統）、name1=0x4DA2A0（RTDI TESFullName@0xB03298）、u16='Level %i'@0x45DAB1 | [0xB333C4] の正確なクラス名 [部分確定]、符号化 ANSI は [推定] |
| FUN_0045E6A0 ctor 分岐 | **[確定]** | 2 jmp=0x45E77A/0x45E7F0（new 成敗スキップ）、ctor#1 flag=0 @0x45E773 / ctor#2 flag=1 @0x45E7E5、seek(0xD000)=ESI @0x45E7F6-0x45E81D、チャンク 0xA9000/0xAA000・seek ステップ 0x4000/0x2000、[0xA853D4]=1 | seek ステップの存在理由（CON チャンク間隔）は解釈 [推定] |
| FUN_004E0F80/FUN_0045D220 | **[部分確定]** | D220=0x45D220..0x45D383（SaveQueuedHavokData、log 0xA3A82C、0x4E0970/0x4E0A40/0x4211E0）、対 D390、4E0F80=0x4E0F80..0x4E1571（callers 7、'BSX'/'REF'、Interlocked） | 4E0F80 各枝の厳密責務・'BSX'/'REF' 実体 [未確定/部分確定] |
| once-init | **[確定]** | 0x401000=IAT KERNEL32!InitializeCriticalSection、0x401010=DeleteCriticalSection、0x981FB4=atexit（2944 callers）、0xA17B60 トランポリン、0x4302D8-0x4302FD ガード列 | — |

---

## 5. 判定サマリ

| 判定 | 件数 | 該当項目 |
|---|---|---|
| **[確定]** | **8** | 実 .ess サンプル（不在）, +0x154, +0x88 counter, FUN_465130 mode, name/thumb 読取順, name 取得関数, FUN_45E6A0 ctor 分岐, once-init |
| **[部分確定]** | **1** | FUN_4E0F80/FUN_45D220（D220 本体は [確定]、4E0F80 は構造・callers・キュー連鎖まで） |
| [推定] | （付帯） | 0x983FD0/0x9840E0/0x984166/0x983479/0x982D71 = strrchr/strstr/strncmp/atoi/GetFileAttributes 相当（内部 CRT 実名不可）、thumb w/h=256×256、'BSX'=ExtraBSXFlags 系、符号化 ANSI、977C50=timeGetTime 相当、一覧 UI 接続 |
| [未確定] | （付帯） | dry/real 両パスの counter 増加挙動、4E0F80 各枝/'BSX'/'REF' 実体、[0xB333C4] クラス名、FUN_452330 の意味、thumb tail 実照合 |
| [確定負] | （付帯） | +0x88 へのオブジェクト計数の関与なし、vtable 0xA39094 経由の +0x154 参照なし、FUN_45D450 以外の new(0x160) 経路なし、+.ess サンプルの存在（全8系統） |

---

## 6. v309 既知事項との整合性（矛盾と訂正・旧結論は不削除）

| # | v309 の記載 | 本件の裁定 | 裁定理由（証拠） |
|---|---|---|---|
| 1 | §1-9「dtor 0x459450」 | **訂正: 比較関数（FILETIME 3-way）** | 0x459450 は2引数・int 3-way 戻り・0x416650 の整列挿入内で `call [fn]; test/jg` 使用。真の dtor は 0x459400 内 `vfunc+0(item,1)` @0x459410-0x45941C |
| 2 | §3-3「FUN_465130 arg3 = edi [部分確定] | **[確定] mode=0（定数）** | 0x4651AB `xor edi,edi`（全 merge 経路通過）→ 0x4651D9 push |
| 3 | §1-10「callers 0x4651DE mode arg3=edi [部分確定] | 同上に [確定] 昇格 | 同上＋callers 6件の push 列 |
| 4 | §1-6「FUN_45D5F0 へ12引数」 | **訂正: 10引数** | 0x4643B3-0x4643D6 / 0x465A17-0x465A3F の push 数＝10、`ret 0x28` と一致 |
| 5 | §1-3 probe bounds FUN_45D5F0=0x45F6AC | **訂正: 0x45D919（ret 0x28）** | linear dump で 0x45D919 ret、以降 int3 パディング→0x45D920 |
| 6 | §1-4「2 jmp 分岐細部未追跡（残点6）」 | **解消: new 成敗スキップ** | 0x45E760/0x45E7D1 の je と 0x45E77A/0x45E7F0 の jmp の対応 |
| 7 | §1-1「0x401000/0x981FB4 正体 [部分確定]」 | **[確定] InitCS ラッパ / atexit** | IAT 実名（0xA28064/0xA28068）＋0x981FB4 callers 2944＋0xA17B60 トランポリン |
| 8 | §1-11「0x4E0F80 文字列参照0件」 | **訂正: 'BSX'/'REF' 2件** | 0x4E128A / 0x4E131E の imm32 push（rdata 全走査） |
| 9 | §3-1「+0x154 byte 意味 [未確定]」 | **解消: FILETIME キャッシュ valid flag** | 0x459450 全文（FindFirstFileA→ftLastWriteTime→flag=1） |
| 10 | §3-2「FUN_464320 役割 [未確定]」 | **解消: max+1 算出＋version リセット** | 0x464320 全文＋0x459570/F1 の2系統入力 |
| 11 | 項目9 new(0x160) メモリの「dtor 0x459450」 | #1 と同じ裁定 | 同上 |
| 12 | （新）off-by-pad 教訓4例 | **第5例: 0x45B300** | FUN_45ADC0 `ret 4` @0x45B2FD 直後に CC0 で FUN_45B300 開始（minrun≥1 で func_head=False） |
| 13 | （新）func_bounds 過走の実例 | FUN_465130→0x467920 / FUN_4E0F80→0x4E8FD6 / FUN_45D5F0→0x45F6AC はいずれも linear first-ret で訂正 | 過走時は ret/CC 境界を linear で再確認すること |
| 14 | （新）FUN_45B300 = SaveGame ctor は v309 未同定 | **本件で新同定** | 0x441BB0/0x441BB9 の singleton 生成列＋フィールド初期化内容 |

**矛盾なし（踏襲確認）**: FUN_45DBC0 が size をローカルに読むのみ／name 読取実体=FUN_45D5F0、version gate=[0xB33B00]+0x7c（F1/F3/F6/F7 で全再確認）、0x465860=ロード実行、0x45D450=一覧生成、new(0x154)=I/O ストリーム、size 式=0x24+len+len+thumb（writer 列と reader 順の1:1で再確認）。

---

## 7. 新規残件（次束用・低優先）

1. **[未確定] +0x88 counter の dry→real 2パス挙動** — dry 時にも 0x464E6C で counter+=1 が生じるが、同一セーブで dry→real が連続するか（実行時動態・実 .ess 無し）。連続なら +2 でファイル連番と乖離。
2. **[部分確定] FUN_004E0F80 の各枝の厳密な責務** — 506命令・callers 7件（0x5CDC07 等）まで同定済だが、vfunc +0x188/+0x470/+0xF0 の対象と 'BSX'/'REF'（0x6FF9C0/0x6FFAC0）の実体名が未同定。
3. **[部分確定] [0xB333C4] の正確なクラス名** — new(0x804)+ctor 0x66A740+`call 0x46C300(0x14,1)`、getter this（+0x1C/+0x40/+0x710/+0x714）。RTTI TD 同定未実施。
4. **[部分確定] thumb tail（F8-F10）の最終同一** — writer の size=pixels+8・4B+4B+pixels 列と reader のスキップ一致・×3B memcpy で整合するが、実 .ess 照合なし。arg10 の受け先（FUN_465860 は0、ロード側の thumb 読取担当関数）が未特定。
5. **[部分確定] FUN_00459570 の arg3/arg4** — 数値抽出（arg2）は確定、arg3（比較対象?）と arg4（整形名バッファ? 0x4596F6-0x459760 の '.'/':' 变換部）の厳密な契約が未追跡。
6. **[部分確定] FUN_00465130 ガード FUN_0452330（0x452330）の意味** — 「保存可否判定→false 時は autosave のみ許可」との整合だが、dry 判定との関係が未追跡。
7. **[未確定] FUN_0045B300（SaveGame ctor）の残フィールド** — +0x7D=1 の意味、+0x44/+0x48/+0x54..0x60/+0x74..0x7C の初期化有無（0x45B396-0x45B630 の間の未表示区間）。
8. **[部分確定] name1/name2 の表示上の意味** — name1=[this+0x1C] の TESFullName、name2=4DE8D0 出力（既存 ledger の「氏名」と整合も、[this+0x1C] の実クラス未同定）。
9. **[未確定] 旧版（F1/F3/F6/F7 ゲート未満）の実挙動** — 静的には0/代替値を確定したが、実 .ess での照合が必要（契約書 §1 ②⑤）。
10. **[未確定] 0x416650 呼び手群（16件）の compare 関数カタログ** — 帯外多し（0x452150 等は v315 禁止帯）。保存帯の0x459450 のみ確定。
11. **[推定] seek ステップ 0x4000/0x2000 の CON チャンク構造上の理由** — 命令は確定、レイアウト解釈は .ess 実データ待ち。

（新規残件 = **11 件**）

---

## 8. 再現手順

1. 環境: Windows / python 3.11（capstone 5.0.7・pefile）/ EXE `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`。
2. 依存ヘルパー（読取専用・変更禁止）: `%TEMP%\opencode\v307_common.py`（F/func_head/find_head/func_bounds/disasm/range/callers_of/resolve_imm32_refs/str_at）。`tmp/re/re_util.py` は読取のみ。
3. 実行（各 probe）:
   ```powershell
   chcp 65001
   python $env:TEMP\opencode\v313_<n>.py *> $env:TEMP\opencode\v313_<n>.out
   ```
   `<n>` = 01..14（§2 の表）。
4. 判定は全て出力の VA/命令列に基づく。生スキャン（v313_01）は命令境界検証（window 内 disasm で disp32 を含む命令を特定）を通過したサイトのみ採用。

## 9. selftest 結果（再実行記録）

- **初回実行**: v313_01..v313_14 全14本 EXIT=0。
- **再実行（決定的検証）**: 全14本を再実行 → **全 EXIT=0**、実行前後の全 `.out` の **MD5 比較 = 差分0（hash-differences=0）** → 出力は決定的で再現可能。
- 内容別 selftest ポイント:
  - v313_01: raw disp32 0x154=687 件 → 境界検証897サイト → byte 参照5件（save 帯のみ）。
  - v313_04: FUN_45D5F0 `ret 0x28`＝10引数一致（push 数カウントと一致）。
  - v313_09: IAT 実名 12 種解決（InitializeCriticalSection / DeleteCriticalSection / FileTimeToSystemTime / SystemTimeToTzSpecificLocalTime / Interlocked* / FindFirstFileA 等）。
  - v313_11: FUN_45ADC0 `ret 4`@0x45B2FD → FUN_45B300 head（off-by-pad）を linear first-ret で特定。
  - v313_12: [0xB33B00] store = 0x441BB9 の1件（A3/ModRM05 絞り込みで確定）。

---

## 10. docs/RE_EXE_REPORT.md 等への反映要否

**本指示のみでは docs/移植コード/CI/TODO/STATE は変更しない**（契約書 §解析制約）。人間確認後の反映候補（正典 `docs/RE_EXE_REPORT.md` を正とし、矛盾時は §55 補遺→§50 の優照合。矛盾点は旧結論を削除せず裁定理由を記録 = §6）:

1. **SaveGame ctor = FUN_0045B300（0x45B300..0x45B668、singleton store 0x441BB9、+0x88=0 初期化）— docs 未出（本件が初出）**
2. **スロット +0x154/+0x158/+0x15C = FILETIME キャッシュ＋比較関数 0x459450（v309「dtor」訂正）と整列挿入 0x416650 — docs 未出**
3. **+0x88 counter = max(ファイル名連番, ヘッダ offset38)+1、quicksave/autosave は offset38=0、使用先=自動保存名 'Save %i' — docs 未出**
4. **FUN_465130 mode=0 恒（0x4651AB）＋呼び手6件の name 引数表 — docs 未出（mode 表の補強）**
5. **FUN_45D5F0 の読取順 F1..F10 表（gate/型/arg）と fill=[arg1+4]、F7 フォールバック=FileTimeToSystemTime — docs 未出**
6. **name1=TESFullName(0x4DA2A0)/u16=Level(0x5E1FD0→0x4677F0)/name2=0x4DE8D0、書式 'Save %i - %s - %s, Level %i, Playing Time %02i.%02i.%02i' — docs 未出**
7. **FUN_45E6A0 = CON→.ess 変換（ctor#1 読 flag0/ctor#2 書 flag1、seek(0xD000)=ctor#1、チャンク 0xA9000/0xAA000＋seek 0x4000/0x2000 SEEK_CUR）— docs は v309 の「XBoxSaves 列挙＋0xD000」のみ**
8. **FUN_45D220 = SaveQueuedHavokData（対: D390 = AttachedAnimation、ログ 0xA3A82C）— docs 未出**
9. **once-init = InitializeCriticalSection(0x401000)+atexit(0x981FB4)、CS 0xB33980 — docs 未出**
10. **off-by-pad 第5例 0x45B300 と func_bounds 過走3例（0x467920/0x4E8FD6/0x45F6AC）— 解析手順ノートへ**

**変更すべきでないもの**: GT・ソース（app/**）・CI・TODO.md・STATE.md・他 lane（v314/v315）の成果物・既存 tmp/re 成果物（上書き禁止）。

---

帯境界遵守: 自帯＋ FUN_465130 本体延長（0x466000-0x467920）のみ。v314/v315 禁止帯は非侵入（v313_01 の生スキャンで現れた FORBID 帯4サイトは VA+命令のみ記録・深解析なし）。ESM 読取・git 操作なし。成果物は本ファイルのみ新規作成（v304〜v312 成果物・台帳・STATE・TODO・GT・ソース・.github/**・docs/ は未編集）。

## 10. 追補（次束F＝v316 統合時・2026-11-07）

`v316_v313_residuals.md`（49,357B/547行・probe 16 本・全 EXIT=0・MD5 決定的）により §7 残件 11 件を処理し、本 artifact の該当記述を以下で**訂正・拡張**する（旧記述は削除せず・詳細と根拠は v316 §4/§5・訂正 12 行は v316 §5）:

1. **項目4 訂正: `FUN_45D5F0` の callers は 4 件**（本 artifact の 2 件から拡張）。**ロード側 `FUN_465860` は arg10=0 で thumb を読まない**（読むのは `FUN_004610F0` のみ）→ 本 artifact の thumb 読取順の「ロード側受け先」部分を差替。[確定]
2. **項目1 拡張・一部 [未確定] へ**: counter は `FUN_464AB0` 1 呼出し＝**+1 のみ（dry/real は `test/je` で排他）**・呼出しは 0x465228 の 1 件。ただし bit9(0x200) は `[reg+0x18]` 演算 530 件・SaveGame+0x18 書込 18 件・RMW 31 件に **0 件＝dry 発火経路は未検出**（実 .ess/動的待ち）。
3. **項目6 確定＋off-by-pad 第 6 例**: 継続条件＝**`dry ‖ 0x452330() ‖ name=="autosave"`**（dry が短絡先行）・`0x452330`＝プレイヤー状態＋メニュー ID {0,3,0x3F5,0x40F} 述語。**`0x452330` の関数先頭同定で off-by-pad 第 6 例を記録**（既知 5 例に追加）。
4. **項目2 拡張**: `FUN_004E0F80` ＝ **REFR SetModel（`[+0x3C]` 差し替え）**・15 系統の分岐カタログ（bit5 ゲート→D220/D390、'BSX'/'REF' の refcount 解放、0x4D7D10 で `[+0x3C]` 代入）。
5. **項目3 新確定**: **`[0xB333C4]` ＝ `.?AVPlayerCharacter@@`**（vtable 0xA73A0C→COL 0xAC3D80→TD 0xB080A4・store 0x4067A6/0x4497A9/0x464553 の 3 件）。
6. **項目5/7/8/9/10/11 新確定**: arg3＝`sprintf("%s%s",[0xB38720],中央)` 出力先・arg4＝'.'→':' 変換時刻文字列 ／ `+0x44/+0x48` は **ctor 未初期化**・`+0x7D` は `[+0x74]` formID 解決テーブル適用フラグ ／ 名前書式 `'Save N - <name1> - <name2>, Level L, Playing Time HH.MM.SS'`（サニタイズ＋重複時 `' #%d'`）／ F1=明示0・F3=0・F6=0・**F7=0x4301A0（FileTimeToSystemTime→SystemTimeToTzSpecificLocalTime）**・F8 は ver<0x2E で +0x24 skip ／ 比較関数 16 件特定（11[確定]/4[部分確定]/1[未確定]・0x5D8FD0 ミドルエントリ）／ seek＝初回 **SEEK_SET 0xD000**・step **SEEK_CUR 0x4000/0x2000**・定数表 {0,1,2}＝入力 got+step・出力 got。
7. **動的不足（実 .ess 待ち）**: dry 発火経路・+0x44/+0x48 意味・[0xB38720] 実内容・FUN_004610F0 引数契約/'BSX'·'REF' 実体・compare 0x5D8FD0・seek レイアウト実証・メニュー ID 名称・[+0x74] 投入元・name2 語彙・0x45BA00/0x45F7F0 所属 — いずれも**実 .ess/動的実行が不足証拠**と記録（#84→#82 依存として起票）。

## 12. 動的照合 Phase A 反映（v320 統合時・2026-11-07）

`v320_ess_dynamic_phaseA.md`（34,178B/293行・probe 12 本・検体 MD5 FC32EFAE…）による本 artifact 該当項目の**動的確定**:

1. **項目5 thumb: [部分確定]→[確定] 昇格** — thumb = **8 + 256×192×3**（w=256/h=192・pixels @103）。v313 の「256×256」推定は **256×192 = 実測差**として訂正（256 幅のみ一致、高さは実測）。
2. **指示書 §1-1（ヘッダ size↔総長照合）の解決**: size field ＝ **u32@34**・`38+size`＝ヘッダ+thumb 終端と完全一致（総長一致 u32 は 0 件＝payload 非連鎖を [確定負]）。観測ツールの dow 異常は field 順誤りで解除（v312 §9 参照）。
3. **項目9 関連の E2 解消**: 保存レコード `+9` byte ＝ **`0x7D` が 5,527/5,527 全件一致 [確定]**（version byte 説を動的追認・v319 の [推定] を裏付け）。`bit31` 立ち 0 件 → **Δ=0x2C 実 entry 該当なし [確定負]**、Δ0x1C の中身は観測不能 [未確定]。
4. **項目11 seek の実レイアウト面**: `0xD000/0x4000/0x2000（+0x11000/0x13000）はすべて thumb pixels 内で構造切替なし [確定(本サンプル)]**（PC .ess は CON seek 帯非該当＝'CON ' 分岐とは無関係を実測補強）。
5. **歩行の全体像**: `[12B entry][tail payload]` で 150,364..822,976 ＝ **5,527 records 連続歩行・崩れ 0**、count 欄 u32@147,757＝5,527 完全一致。**無圧縮 [確定]**（zlib 44 候補全滅・raw deflate 4,273 試行 0・エントロピー ≥7.0 ゼロ・'Oblivion.esm' 平文）。quest=274/cell=186・AI 系 0 [確定負]・global 候補 2 [推定]。
