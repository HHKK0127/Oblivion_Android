# v199_esm_alarm_residuals.md — ESM/警報束（V186: PathGrid ローダ確定・馬警報条件・Extra 残点）

- 作成日: 2026-10-29 / 担当: TODO serial 186 (V186) — 分析専任サブエージェント（移植実装なし）
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 Steam / 7,898,624 B / ImageBase 0x400000）
- スコープ: `tmp/re/TODO_re49.md` serial 186（V186）行
  1. FUN_004E65B0 を PathGrid ESM ローダ本体としての確定（レコード読取ループ・入出力構造・PGRD 内部表現）
  2. 盗難馬警報の成立条件（0x426A90 の caller 2 件 FUN_004858D0 / FUN_004E00F0 の深部）
  3. ExtraStartingPosition node+8 の読手（copy flag 経路）と type 0x53 extra の ctor/書手
- 出典（読取専用）: `tmp/re/v192_havok_magic_residuals.md` §2-R9・§5-6 / `tmp/re/v193_opcode_gt_residuals.md` §2-V-177a・§2-V-178-r5・§4-3/§4-4 / `GT_CHEATSHEET.md` / `cache/*` / `docs/Oblivion_decompiled.c`
- 中間スクリプト: `%TEMP%\opencode\v199_*.py`（+ 既存 `v192b_*`/`v193_*` 再利用）
- 制約: 既存 tmp/re・docs/・TODO・GT・台帳・git・Supermemory・sync は**読取のみ・編集 0 件**。ESM/DLC ファイル読取 0 回（本体再スキャン禁止）。成果物は本新規 1 ファイルのみ。
- 凡例: [確定] = 逆アセンブル/データ直接確認 / [部分] = 構造判明・一部未 / [推定] = 論理推定 / [残] = 未解明。

## 1. 手法 / 入力

- `python "$env:TEMP\opencode\probe_harness.py" selftest` → **4/4 PASS**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl=0x504560）。
- 解析: capstone 5.0.7（detail 付き Cs）+ pefile、`tmp/re/re_util.py` 系ヘルパ、`cache/`（call_targets.json / named_funcs.txt / vtable_index.tsv / opcode_table.json）。
- decompiled（29 MB）は該当関数の部分読取のみ（GT §2 規則）。
- （本節は着手時に先行作成 — §2 以降は判明ごとに追記）

## 2. 証拠・分析

### 2-1. FUN_004E65B0 = PathGrid(PGRD) データ適用本体 — [確定(構造)]

**caller**: 2 件のみ — `0x4E75D0`（FUN_004e74e0 内）/ `0x4E765C`（FUN_004e7610 内）[確定]。ESM レコード層（この 2 関数）から呼ばれる「PGRD サブレコード適用メソッド」である。

**シグネチャ**: thiscall、`ret 4`（スタック引数 1 個 = レコードリーダ）。SEH 大関数（sub esp,0x13C）。

**引数（リーダ = ebx = param_1）**:
- `[reader+0x250]` = 現在サブレコード FourCC（`call 0x4510b0` で取得。0 なら終端）
- `[reader+0x254]` = サブレコードデータ長
- `call 0x450c20(reader, dest, size)` = サブレコード生データ読取
- `call 0x44FEA0(reader)` = 次サブレコードへ進む（`[+0x254]+6` を cursor `[+0x260]` に加算）。ループ条件はこの戻り al。

**FourCC switch（実測）**:

| FourCC(u32) | ASCII | 分岐先 | 処理 |
|---|---|---|---|
| `0x49524750` | **PGRI** | 0x4E68B1 | `[+0x254]/0x10` 件の 16B レコードを読取 → 各 16B を malloc(0x10) へ複写し **`[this+0x28]` リスト**へ 0x446CB0 で push |
| `0x4C524750` | **PGRL** | 0x4E66E8 | `0x40FD70(size)` でバッファ取得→読取、`shr 2` で u32 数。各要素を **0x46BB20 でフォーム解決 → RTTI キャスト 0x9832E6（型表 0xB03054/0xB02F9C）** → `[this+0x24]` 配列（後述）を index で引いて node を得、`0x4E50E0(this, form, node)` で **`[this+0x34]` マップ（form→node）**へ登録。キャスト失敗時のみ `0xA47790 'Could not find reference (%08X) for linked points in pathgrid for cell %s.'` をロガー 0x4A7A60 へ |
| `0x50524750` | **PGRP** | 0x4E6938 | 点列本体。`0x4E4A10([this+0x24], [this+0x30])` で**ノード配列を resize** → `[this+0x30]`*16B を読取 → 各 16B エントリごとに: malloc(0x2C) → **`0x4E7DF0`（node ctor）** → `0x4BEF50(node, entry)` で **entry[+0..+8] の 3 float を node[+0x14..+0x1C] へコピー（位置 XYZ）** → `0x42BB90(nodeArray, i, &node)` で**スパース配列へ index 付き格納** → `[entry+8](z)` と定数 `[0xA3B888]`（**-FLT_MAX**、生バイト FF FF 7F FF 実測）を fcompp 比較 → `0x67ED50(node, flag)`（flag は `[this+0x20]` オブジェクトのセル照合 0x4CBBB0/0x4F0600 経由）→ **`0x4E5380(this, node)` で `[this+0x44]` マップへ node 登録**。**`[esp+0x24] += byte[entry+0xC]`（点毎の接続数を加算）** |
| `0x52524750` | **PGRR** | 0x4E6A60 | 接続列。**前提: `[esp+0x20] != 0`（PGRP で node が生成済み）**。ebp = `[esp+0x24]` = **PGRP で集計した総接続数**。malloc(接続数*2) → **`cmp [reader+0x254], 接続数*2`** → 不一致なら **0x4E6BAB `push 0xA47720` = 'PathGrid (%08X) in cell %s found different connection data than it was expecting: Expected: %i Found: %i'**（Expected=[esp+0x40]経由の集計値、Found=`[+0x254]>>1`。ロガー 0x4A7A60）。一致なら u16 配列として読取し、第2ループで各点の `byte[entry+0xC]` 個ぶんの u16 index を消費: 対象 node が非 null・自己以外なら **`0x4E7DE0(self)` = `lea eax,[ecx+0x20]`（node+0x20 = 接続リスト）→ `0x446CB0(list, other)` で push**、`byte[entry+0xC]--` |
| `0` | — | 0x4E6C5A | ループ終了。`free([esp+0x20])`・`free([esp+0x34])` 後に戻り |

- エラー文脈: `[this+0x20]`（オブジェクト、vtable +0xD4 で名前取得）が null なら "UNKNOWN"(0xA309A4)、非 null なら 0x4C97F0 判定 → 0x4C9CF0 経由で `'%s (%d, %d) in world %s'`(0xA477E8) / `'%s (%d, %d)'`(0xA477DC) を組み立て。`[esp+0x48]`=「エラー報告済み」抑制 byte。
- 戻り値: `byte[esp+0x1B]`。PGRP 終了時に**総接続数==0 なら 1**（0x4E6C42）。

**入出力構造（this = PathGrid 適用コンテキスト）** [確定(フィールド)]:

| offset | 内容 |
|---|---|
| `+0x0C` | **セル名 string**（エラー文 'in cell %s' へ直push） |
| `+0x20` | エラー文脈オブジェクト ptr（0 = "UNKNOWN"） |
| `+0x24` | **ノード配列オブジェクト ptr**（自身に vtable `0xA47590`、+4=data(dword*)、+8=size(u16)、+0xA=capacity(u16)、+0xC=実数(u16)。挿入は 0x42BB90、resize は 0x4E4A10） |
| `+0x28` | PGRI 16B エントリのリスト頭（0x446CB0 push） |
| `+0x30` | **点数 (u16)**（0 なら早期 return。PGRP の resize 引数） |
| `+0x34` | form→node マップ（0x55e000 find / 0x452570 insert、PGRL で使用） |
| `+0x44` | node キー→データ マップ（0x4E5380 が使用） |

**node (malloc 0x2C) の内部表現** [確定]: ctor `0x4E7DF0`（base ctor 0x67EDC0 → `[+0x20]=0/[+0x24]=0`、グローバル 0xB3F9A8.. を `[+0x14..]` へ）。**`[+0x14..+0x1C] = 位置 XYZ float**（0x4BEF50 が entry[0..8] からコピー）/**`[+0x20] = 接続リスト**（0x4E7DE0 = `lea eax,[ecx+0x20]`）/[+0x24] = 第2リスト。ファイル側 16B エントリは `{+0..+8: XYZ float, +0xC: byte 接続数}`。

（§2-2/§2-3 は以下に追記）


### 2-2. 盗難馬警報の成立条件 — [大部分確定]

**前提（v189/v193 済み）**: `0x4269E0` = アキュムレータ本体。本タスクで全読し次を確定:

**`FUN_004269E0` = `ExtraDataList::AddCrimeGold(float)`** [確定]:
```
extra = 0x41E210(EDL, 0x3D)         ; GetByType(0x3D)
if (!extra) { extra = malloc(0x10); 0x42A290(extra, float); 0x41E0A0(EDL, extra); }  ; AddExtra
else        { extra[+0xC] += float; }
if (extra[+0xC] <= 0.0) 0x422E20(EDL, extra, 1);   ; RemoveExtra（0 以下で消去）
ret 4
```
- **type 0x3D extra の ctor = `0x42A290`** [確定]: `fstp [eax+0xC]`(float 引数) / `mov byte [eax+4],0x3D` / `[eax+8]=0` / **vtable `0xA3592C`**。
- **vtable 0xA3592C = `.?AVExtraCrimeGold@@`**（cache/vtable_index.tsv row 662、typedesc 0xB04248）→ **type 0x3D = ExtraCrimeGold** [確定]。
- **getter = `0x41FC90`**（`lea ecx,[ref+0x44]; call` で float 返し。0 以下比較に使用）[確定(使用箇所)]。

**警報値の加算元（callers(0x4269E0) = 4 件）** [確定]:

| site | 関数 | 文脈 |
|---|---|---|
| 0x426F91 | **FUN_00426A90** | **arg1 bit0x80 ゲート**（`test byte [esp+0x254],0x80` = arg1 の bit7）。成立時: `[0xB33B00]` 文脈（セーブ/ストリーム）から 4B float 読み（0x4534d0）→ AddCrimeGold。**= セーブ復元経路** [確定] |
| 0x50B558 | **FUN_0050B4A0** | FAE80 ゲート（script opcode impl、E8 caller 0 件）。`gold = fild([arg]) − [arg2]` → 対象≠player または `[player+0x116]≠0` なら `lea ecx,[esi+0x44]; AddCrimeGold(gold)`、player かつ +0x116 立ちなら `[player+0x700] += gold` [確定] |
| 0x60FC54 | **FUN_0060F250** | E8 caller 0 件（vtable/impl）。`AddCrimeGold(arg)` 後、**arg > 1.0 のとき** player 系処理へ（0x60FC6E 以降: `[player+0x6DC]` 等） [確定(構造)] |
| 0x60FDA3 | **FUN_0060FD20** | E8 caller 0 件。**`esi==g_thePlayer && 0x41FC90(edi+0x44) <= 0`（被害者の crime gold が 0 の初回）** → **`[player+0x6DC] += 1`（窃盗カウンタ）** → `fild [0xB376B0]`（グローバル int = 立て替え gold 値 [推定]）→ `AddCrimeGold(edi+0x44, 値)` → `call [edi]+0x40`（vtable 通知、arg 0x80）。else（actor 非 player）: `call [esi]+0x284(0x1F) == 0x64`（100）なら `0x5E0550(esi)` [確定(構造)] |

→ **馬盗難の runtime 成立条件** [確定(構造)]: **被害者（馬）ref の EDL に ExtraCrimeGold(0x3D) > 0 が載る**こと。初回載せは FUN_0060FD20（player の窃盗カウンタ +1 と同時）。

**`FUN_004F8C20` = IsHorseStolen の本体** [確定]（callers(0x4F8C20) = 1 = 0x505EEE のみ、opcode impl 帯 0x505EE0 の系列 = E8 caller 0 件で vtable/表経由）:
```
result(double*) = 0.0
if (ref && IsActor(ref=vt+0x190)) {
    if (0x5E3290(ref))                 ; 馬/騎乗可能系述語 [推定: IsHorse 系]
        if (0x41FC90(ref+0x44) > 0.0)  ; ExtraCrimeGold > 0
            result = 1.0
}
```
→ **成立条件 = 「IsActor かつ 0x5E3290 述語成立 かつ ExtraCrimeGold > 0」** [確定]。0x5E3290 の命名は [残]。

**`FUN_00426A90` の役割** [確定(構造)]: `ecx` = **ExtraDataList**（vtable 0xA350F8 = `.?AVExtraDataList@@` row 663）。arg1=flags / arg2 / arg3=form。`arg1 & 0x80` 成立時にストリームから float を読んで AddCrimeGold（セーブ復元）。`push arg3; call 0x9832e6`（RTTI キャスト、型表 0xB03054→0xB03070）で ref を解決。callers 2 件:
- **FUN_004858D0（0x485A61）**: **新規 ExtraDataList を ctor `0x422EE0`（vtable 0xA350F8）で生成** → `0x426A90(EDL, 0x20, 0, 0)`（flags=0x20、bit7 なし＝gold 読みなし）→ `0x67B1E0(list, EDL)` でリスト追加。callers 4 件: **FUN_004937e0（唯一 caller = FUN_004E00F0+0x1EA）/ FUN_00614db0 / FUN_0068ffe0×2**。
- **FUN_004E00F0（0x4E0324）**: `lea edi,[esi+0x44]` = **ref 自身の EDL** → `0x426A90(EDL, flags=ebx, [esp+0x20], ref)`。flags マスク `0x177577E0`（`[0xB33B00]+0x7C >= 0x43` なら `0x177577F0`、差分 bit4=0x10）または **IsActor(ref)** で発火。**= TESObjectREFR のセーブ復元で crime gold を再構築する経路** [確定(構造)]。caller は 1 件: FUN_00659C90（callers 3 件: FUN_006022f0 / FUN_00608410 / FUN_0069f800）。

**報道（警報メッセージ）系**: v189 済みの OnAlarmVictim 0x50B3E0 / 警報報道 0x426A90 内 0x426F91 との関係は上記のとおり。**馬盗難パス = 「馬 ref に ExtraCrimeGold が載る（FUN_0060FD20/60F250/50B4A0）→ IsHorseStolen が 1 を返す（0x4F8C20）→ quest 条件/スクリプトが反応」** [確定(構造)]。

### 2-3. ExtraStartingPosition node+8 読手 / type 0x53 ctor — [確定 + v193 訂正]

#### (a) type 0x53 extra の ctor と RTTI 名 — [確定]

- **ctor = `FUN_00429950`**（`.text` 全域で `mov byte [reg+4], 0x53` を **1 件のみ**検出＝0x429954、排他的実証）:
  ```
  0x429950: mov eax,ecx; xor ecx,ecx
            mov byte [eax+4], 0x53      ; type = 0x53
            mov dword [eax+8], 0        ; +8 = 0
            mov dword [eax], 0xA357A0   ; vtable
            mov dword [eax+0xC], 0      ; +0xC = 0（world/cell ptr 初期化）
            ret
  ```
- **vtable `0xA357A0` = `.?AVExtraStartingWorldOrCell@@`**（cache/vtable_index.tsv row 717、typedesc 0xB04114、abs32 参照は ctor 内 0x42995D の 1 件のみ）→ **type 0x53 = ExtraStartingWorldOrCell** [確定]。
- **書手（setter）= `FUN_00423C90`** [確定]: `GetByType(0x53)` → 無ければ malloc(0x10)→`0x429950` ctor→`0x41E0A0`(AddExtra) → **`mov [esi+0xC], ebx`（arg = world/cell form ptr）**。arg が null なら `0x41E160`（RemoveByType）で除去。callers 3 件: **FUN_00423D30×2 / FUN_00426A90+0x1385(0x427E15)**。
- **getter = `FUN_0041F7F0`**（`push 0x53; call 0x41E210; return [eax+0xC]`）。`push 0x53` 全走査は **1 件のみ**（0x41F7F0）＝実用ユーザーはこれだけ。consumers 4 件: **0x45BCA7(FUN_0045BB30) / 0x45EB10(FUN_0045E990) / 0x49014A / 0x490C51**（いずれも teleport/starting-position 適用帯）。

#### (b) ExtraStartingPosition（type 0x14）のレイアウトと pos/rot アクセサ — [確定]

ctor `FUN_0042AC40`（vtable `0xA35A70` = `.?AVExtraStartingWorldOrCell@@` の兄、row 716 `.?AVExtraStartingPosition@@`）の実レイアウト（malloc 0x24）:

| offset | 内容 |
|---|---|
| `+0` | vtable `0xA35A70` |
| `+4` | type byte `0x14` |
| `+8` | **0 で初期化のみ（他に書手・読手なし）** |
| `+0xC..+0x14` | **位置 XYZ**（3 float、vcall `+0x174`=GetPos からコピー） |
| `+0x18..+0x20` | **回転 XYZ**（3 float、`[arg+0x20]` = ref の rotation フィールドから） |

**pos/rot アクセサ 4 本（EDL get-or-create 型）** [確定]:

| VA | 役割 | read/write | caller |
|---|---|---|---|
| `0x41F730` | **GetPosition**（`[+0xC..+0x14]` 読み） | read | **0x4D8230** = REFR vtable slot **+0xF4**（GT §4.1 候補E の位置 getter と一致） |
| `0x41F6F0` | **GetRotation**（`[+0x18..+0x20]` 読み） | read | **0x4D8210** = slot **+0xF0**（回転 getter と一致） |
| `0x41F7B0` | **SetPosition**（`[+0xC..+0x14]` 書き） | write | FUN_0045C4F0 / FUN_004DD130 |
| `0x41F770` | **SetRotation**（`[+0x18..+0x20]` 書き） | write | FUN_0045C4F0 / FUN_004DD130 |

#### (c) 【v193 訂正】「node+8 の書手」は stack temp の誤読 — [確定(訂正)]

- v193 §V-177a は「node+8 の書手: FUN_0045C4F0（0x45C75B `[ecx+8]=eax` / 0x45C781）と FUN_004DD1A0（0x4DD1F8/0x4DD21A/0x4DD240）」と記載したが、**本タスクの全文実測でこれら 5 箇所はすべて stack temporaries** である:
  - 0x45C75B/0x45C781 = `sub esp,0xC; mov ecx/eax,esp; ...mov [ecx/eax+8],...` → **pos/rot ベクトルの by-value 引数構築**（直後に `call 0x41F7B0`(SetPos) / `call 0x41F770`(SetRot)）。
  - 0x4DD1F8/0x4DD21A/0x4DD240 = 同型の `sub esp,0xC` ブロック（FUN_004DD130 内、SetPos/SetRot 呼出前）。
- → **ExtraStartingPosition の +8 は ctor の 0 初期化以外に一切の書手・読手が存在しない**（`.text` 全域で GetByType(0x14) 後の +8 アクセスなし。push 0x14..call 0x41E210 は計 7 箇所＝4 アクセサ + 0x41EA70(get-or-create) + 0x45BC31/0x45EA9B(teleport 帯) で、いずれも +8 へは触れない）。
- → v177 が求めていた「**node+8 の読手（copy flag 経路）は実在しない**」。cell 解決は +8 ではなく**別 extra（type 0x53 = ExtraStartingWorldOrCell）の +0xC** で行われる（getter 0x41F7F0 → teleport 適用 FUN_0045BB30 0x45BCA7）。+8 は retail ビルドで未使用のフィールド。

（§3 以降は以下に追記）



## 3. 結論（3行）

1. **FUN_004E65B0 = PathGrid(PGRD) サブレコード適用本体で確定**: caller 2 件（FUN_004e74e0 / FUN_004e7610）のみ、FourCC switch は **PGRI（16B エントリ→[+0x28] リスト）/ PGRL（form 解決→[+0x34] マップ）/ PGRP（ノード生成 0x4E7DF0・位置コピー 0x4BEF50・[+0x30] 点数で配列 resize・接続数集計）/ PGRR（u16 接続列→node[+0x20] リスト）** の 4 種。PGRP 集計値と PGRR の `[reader+0x254]`（バイト長）が不一致のとき 0x4E6BAC で 'PathGrid (%08X) in cell %s found different connection data...' を出力。this の入出力構造（+0x0C formID / +0x24 ノード配列(vtable 0xA47590) / +0x28 PGRI リスト / +0x30 点数 / +0x34/+0x44 マップ）と node 0x2C レイアウト（+0x14..1C 位置 / +0x20 接続リスト）まで実測確定。
2. **馬盗難警報の成立条件が確定**: **馬 ref の ExtraDataList に ExtraCrimeGold(type 0x3D, ctor 0x42A290, vtable 0xA3592C=`.?AVExtraCrimeGold@@`) が >0 載る**こと。加算元は FUN_004269E0（AddCrimeGold、callers 4 件 = セーブ復元 0x426A90 bit7 / script impl 0x50B4A0 / **初回窃盗 FUN_0060FD20（[player+0x6DC]++ と fild [0xB376B0] を加算）** / 0x60F250）。IsHorseStolen 0x4F8C20 は「IsActor && 0x5E3290 述語 && crime gold > 0」で 1.0 を返す。
3. **Extra 残点2件が確定＋v193 訂正**: **type 0x53 = ExtraStartingWorldOrCell（ctor 0x429950 / vtable 0xA357A0 / 書手 FUN_00423C90＝[+0xC] へ world/cell form / getter 0x41F7F0）**。ExtraStartingPosition(type 0x14) の +8 は **ctor の 0 初期化のみで読手・書手とも実在せず**（v193 の「node+8 書手」5 箇所はすべて `sub esp,0xC` の **stack temporaries**（pos/rot by-value 引数構築）＝誤読を訂正。pos/rot は +0xC..14/+0x18..20、getter 0x41F730/0x41F6F0 → vtable slot +0xF4/+0xF0 と GT 候補E に整合）。

## 4. 残留不明点

1. §2-1: PGRI 16B エントリの +0xC dword（位置とは別ワード）の意味。PGRP の `[entry+8](z)` と定数 -FLT_MAX 比較で `0x67ED00(node,1)` を呼ぶ実務的意味（ practically 発火しないsentinel チェックか）。
2. §2-1: `[this+0x24]` ノード配列 vtable 0xA47590 の RTTI 名（vtable_index 未照合）。`0x4E5380` が使う `[this+0x44]` マップのキー型（0x4BEF40 経由か）。
3. §2-2: **`0x5E3290` の命名**（IsHorseStolen 内の述語。馬/騎乗可能系の可能性が高いが未特定）。`[0xB376B0]`（FUN_0060FD20 が fild するグローバル int）の語義（立て替え gold 値 [推定]）。FUN_0060FD20 / 0x60F250 の vtable slot 配置（E8 caller 0 件）。
4. §2-2: FUN_00426A90 の残りの flag ビット群（0x177577E0 マスク内の各ビットが何を再構築するか）の全数名前化。
5. §2-3: ExtraStartingWorldOrCell +8（ctor で 0 初期化）が本当に未使用か（+0xC のみ使用とする本タスクの読取は ctor/setter/getter の 3 箇所。残りの 0x41E210(0x53) 経由の consumer は 0x41F7F0 のみ）。
6. §2-3: teleport 適用 FUN_0045BB30 / FUN_0045E990 の 0x49014A/0x490C51 帯（0x41F7F0 の残り consumers 2 件）の深部。

## 5. 自己検証

- **selftest 4/4 PASS**（着手時・完成直前の 2 回: bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015 impl=0x504560）。
- 内部整合（`v199_17_consistency.py` で機械検算、**13/13 一致**）:
  1. call 数（cache/call_targets.json vs 本タスクの記載）: 0x4E65B0=2 / 0x426A90=2 / 0x4269E0=4 / 0x4F8C20=1 / 0x41F7F0=4 / 0x42AC40=1 / 0x429950=1 / 0x423C90=3 / 0x41EA70=4 / 0x41F730=1 / 0x41F6F0=1 / 0x41F770=2 / 0x41F7B0=2 — 全一致。
  2. 文字列生実測: 0xA47720='PathGrid (%08X) in cell %s found different connection data...' / 0xA47790='Could not find reference (%08X) for linked points in pathgrid for cell %s.'（v192 R9 の記載と一致）。
  3. FourCC u32 実測: PGRP=0x50524750 / PGRI=0x49524750 / PGRL=0x4C524750 / PGRR=0x52524750（FUN_004E65B0 の switch 4 箇所）。
  4. RTTI 突合: vtable 0xA3592C=`.?AVExtraCrimeGold@@`(row 662) / 0xA35A70=`.?AVExtraStartingPosition@@`(row 716) / 0xA357A0=`.?AVExtraStartingWorldOrCell@@`(row 717) / 0xA350F8=`.?AVExtraDataList@@`(row 663) — cache/vtable_index.tsv と一致。
  5. 排他性実証: `mov byte [reg+4],0x53` は .text 全域 1 件（0x429954） / `push 0x53..call 0x41E210` は 1 件（0x41F7F0） / abs32 vtable 0xA357A0 参照は 1 件（ctor 内）。
  6. GT 整合: IsActor = slot +0x190（0x4F8C20/0x4F8CF0/0x4F8DB0 内で使用）= GT §4.1 と一致。getter 0x41F730→slot +0xF4 / 0x41F6F0→slot +0xF0 = v163 §5.1（候補E）と一致。
- 制約遵守: 新規ファイルは本 1 件のみ。docs/・TODO_re49.md・GT・台帳・git・Supermemory・sync は**編集 0 件**。ESM/DLC 読取 0 回（本体再スキャン 0 回）。中間物は `%TEMP%\opencode/v199_01..17_*` のみ。既存 tmp/re 成果物の上書き 0 件。

## 6. Android 移植への示唆（実装しない）

- **PGRD ローダ**: 移植側は FourCC 4 種（PGRI/PGRL/PGRP/PGRR）を 1 レコードのサブブロックとして処理し、「PGRP で点毎の接続数を合計 → PGRR のバイト長が 合計*2 でないと接続データ不整合」という検証を再現する。ノードは「位置 XYZ float + 接続リスト（ノードへのポインタ）」の 2 フィールドで十分で、PGRI（16B 情報エントリ）と PGRL（ref→node マップ）は ESM 互換のために別コンテナで保持する。
- **馬盗難警報**: 「馬 ref に ExtraCrimeGold > 0 が載る」を単一の真偽条件とし、加算タイミング（窃盗検知 FUN_0060FD20 相当で初回 +1・gold 設定、以降 FUN_0060F250/0x50B4A0 相当で加算、<=0 で extra 削除）をそのまま移植する。IsHorseStolen は「Actor かつ 馬述語 かつ crime gold > 0」の 3 条件 conjunction。
- **Extra データ**: type 0x53(ExtraStartingWorldOrCell) は +0xC に world/cell form ptr を 1 つ持つだけの 0x10 オブジェクト。ExtraStartingPosition(type 0x14) は +0xC..+0x20 に pos/rot を持つ 0x24 オブジェクトで **+8 は使用しない**（v193 の旧記載に注意）。teleport は「pos/rot（type 0x14）＋ world/cell（type 0x53）」の 2-extra 組み合わせで解決する。
- **セーブ復元**: crime gold のセーブ復元は flags bit7 成立時にストリームから float を読んで AddCrimeGold する経路（FUN_00426A90）を再現する。ExtraDataList は vtable 0xA350F8 の 0x14 オブジェクト（ctor 0x422EE0）。
