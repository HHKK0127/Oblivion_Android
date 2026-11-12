# V141: b19 §5 quest バイト残点束 5 件の確定（シード local_18[10] / SysWindow+0xbc / ESM 供給源 / ラベル欠落 / 派生クラス）

作成: 2026-10-26（opencode サブエージェント、分析専任）
出典: `tmp/re/b19_quest_byte_writer.md` §5 残留不明点 1–5（2026-10-26 更新版で再確認）
入力: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 Steam）/ `docs/Oblivion_decompiled.c` / `tmp/re/cache/{named_funcs.txt,vtable_index.tsv,opcode_table.json}` / 既存成果物 `b19_*.md` `b25_cc_residuals.md` `v138_b16_alarm_residuals.md` / ESM 既存実測（`plan.md` L921、**再スキャンなし**）

## 0. 判定表（5 点）

| # | 残点 | 判定 | 一句话 |
|---|---|---|---|
| 1 | シード `local_18[10]` の正体 | **解明** | 付属 Script の **SCHR 末尾 dword = `scriptType`（+0x28）**。write-back で残るのは上位 2 バイト（= scriptType の byte2/3）で、実データ `{0,1,256}` では**常に 0** |
| 2 | SysWindow `DAT_00b3a6e0+0xbc` の実体 | **解明（「編集中 Script」は反証）** | **TESObjectREFR 系のリファレンス（現在選択中のフォーム）**。Script ではなく、`+0x44`(BaseExtraList) → extra 0x12 → `+0xC` で付属 Script を間接取得する |
| 3 | ESM ロードされる Script の `+0x28` 供給源 | **解明** | **SCHR(20B) を `Script+0x18` へ直書きする 3 経路**を特定 → `+0x28 = SCHR[+0x10] = scriptType`。保存側（vtable slot9）も対称 |
| 4 | 3 impl の decompiled ラベル欠落 | **解明（GT 追加候補と確定）** | `FUN_00509d70/00505120/00508340` は定義・名前・hex 文字列とも 0 件、参照は **.data オペコードテーブル各 1 件のみ・E8 呼び出し 0** → Ghidra が関数化せず直前関数に吞込 |
| 5 | Script の派生クラス有無 | **解明（派生なし）** | RTTI CHD 944 クラス解析 → `.?AVScript@@` を非先頭ベースに持つクラス **0 件**。`Script : TESForm : BaseFormComponent` の末端。vtable imm32 参照も 2 件のみ |

---

## 1. 手法・自己検証

- 着手時・完了時とも `python "$env:TEMP\opencode\probe_harness.py" selftest` → **4/4 PASS**（`bytes@0x504050` / `bytes@0x4FB580` / `string@0xA7035C` / `opid0x0015 impl=0x504560`）。
- 中間スクリプト: 新規 `%TEMP%\opencode\v141_*.py` **20 本** + 出力 `v141_*_out.txt` **19 点**（`v141_dbg.py` は構造デバッグ）。
- 手法:
  1. decompile 部分読取（`Ctx.dec_body` / `dec_index`）+ capstone 5.0.7（skipdata=True）の実アセンブルで突き合わせ。
  2. 画像全体の imm32 / E8 参照センサス（`data.find` のバイト列走査、`e8 rel32` 再計算）。
  3. RTTI は **MSVC x86 形式**で再解析（COL `sig=0`、CHD の `pBaseClassArray` は**ポインタ配列**、`BaseClassDescriptor = {pTypeDescriptor, pDisp, attrs}`）。当初の `sig=1` + インライン構造体想定では COL 0 件 / CHD 解析 0 件という誤判定 → `v141_5d/5e` で訂正。
  4. ESM は**再スキャンせず**既存実測（`plan.md` L921）のみ引用。
- 制約遵守: git / `docs/` / `TODO_re49.md` / `session-memory.json` / `plan.md` / 既存 `tmp/re/*` は**非編集**（読取のみ）。新規スクリプトは `%TEMP%\opencode` のみ。

---

## 2. 残点(1): シード `local_18[10]` の正体 — **解明**

### 2.1 呼び出しチェーンの確定（`v141_1_seed` / `v141_1b_node`）

```
FUN_00500280 (arg2 = SysWindow+0xbc の値)
  0x5002B2: mov esi,[esp+0x80]        ; arg2
  0x5002B9: test esi,esi / 0x5002D7 je 0x500332   ; arg2==0 ならシード塊ごとスキップ
  0x5002DB / 0x5002E6: mov ecx,esi ; call FUN_004d7240
  0x5002E0: test eax,eax / je 0x500332            ; 付属 Script 無しでもスキップ
  0x5002EF..0x500319: obj+0x18/1C/20/24/28 → 5 dword
```

- `FUN_004d7240 @0x4D7240` = **`add ecx,0x44 ; jmp FUN_0041e900`**（thiscall パススルー、Ghidra は引数を落として `void FUN_004d7240(void)` と誤読）。
- `FUN_0041e900 @0x41E900` = `push 0x12 ; call FUN_0041e210 ; test eax ; mov eax,[eax+0xc]` → **BaseExtraList(form+0x44) の extra type 0x12 ノードの `+0xC`**（`FUN_0041e920` は同じノードの `+0x10`）。`FUN_0041e210` = `BaseExtraList::GetExtraData`（`v138_b16_alarm_residuals.md` §2-3 の GT と一致: presence bitmap `this+8..` / ノード指針 `this+4` / 型 byte `node+4`）。
- **`node+0xC` が付属 Script である決定的証拠**（`v141_1b_node`、ノード生成地 `0x428BC5..0x428C3C`）:
  ```
  push 0x12 / mov ecx,edi / call 0x41e210      ; GetExtraData(0x12)
  mov ebp,[eax+0xc]                            ; 旧 node+0xC を継承
  push 0x14 / call 0x401f00                    ; malloc(0x14)
  push ebp / mov ecx,eax / call 0x429f20       ; node ctor(旧 +0xC)
  push eax / mov ecx,ebx / call 0x41e0a0       ; BaseExtraList::AddExtra
  push 0x12 / call 0x41e210 / mov eax,[eax+0xc]
  mov ecx,eax / call 0x4fbdc0                  ; FUN_004fbdc0(this = node+0xC)
  push 0x12 / call 0x41e210 / mov [eax+0x10],ebp   ; node+0x10 = その戻り値
  ```
  `FUN_004fbdc0 @0x4FBDC0` = `malloc(0x14)` → `call 0x4fa910(this=ecx)` → `[new+0] = this` / `[new+0xc] = result` を返す = **ScriptEventList の生成**（`[new]` = owner Script）。v138 の「extra 0x12 の `+0x10` = event list ptr」と完全整合 ⇒ **`node+0xC` は owner Script**。

### 2.2 スタック写像（ctx 基準の確定、`v141_1_seed` / `v141_8`）

`FUN_00500280` のフレームは `sub esp,0x58` → `push ebx/ebp/esi/edi` → `push cookie` の後に `0x5002A9: lea ecx,[esp+0x14] ; call FUN_004fcd60`（ctx 初期化）＝ **ctx 基準 = その時点の esp+0x14**。シード塊の間に **3 push**（`0x50030E push 0` / `0x500314 push edx` / `0x500318 push eax`）が入るため:

| 命令 | 実行時 esp 表記 | ctx 基準換算 | ソース |
|---|---|---|---|
| `0x5002F2 mov [esp+0x3c],ecx` | +0x3C | **ctx+0x28** | obj+0x18 |
| `0x5002F9 mov [esp+0x40],edx` | +0x40 | **ctx+0x2C** | obj+0x1C |
| `0x500300 mov [esp+0x44],ecx` | +0x44 | **ctx+0x30** | obj+0x20 |
| `0x500307 mov [esp+0x48],edx` | +0x48 | **ctx+0x34** | obj+0x24 |
| `0x500319 mov [esp+0x58],ecx`（3 push 済） | =基準+0x4C | **ctx+0x38** | **obj+0x28 = local_18[10]** |

（b19 §2.1 の「ctx+0x28..+0x38 ← 5 dword」「`0x500319` = ctx+0x38 ← `local_18[10]`」の写像を**独立に再現**。Ghidra の `local_18[k]` は `obj+4k`、`[10]` = `+0x28`。）

### 2.3 何が残るか（上位 2 バイトの意味）

- `FUN_00500170 @0x500190`: `mov al,[ebp+0x28] → [esi+0x38]` / `mov cl,[ebp+0x29] → [esi+0x39]` / `mov edx,[ebp+0x24] → [esi+0x34]`
  ⇒ シード直後に **ctx+0x38 の下位 2 バイト = host+0x28/0x29、ctx+0x34 = host+0x24** が上書きされる。
- 書き戻し `FUN_004fc850 @0x4FC8D0: mov [esi+0x28], ecx`（ecx = ctx+0x38）→ host+0x28 へ **dword のまま**。
- ⇒ host に残るシード由来は **obj+0x2A / obj+0x2B（＝ scriptType の byte2/3）のみ**。
- `scriptType` の実分布は既存 ESM 実測（`plan.md` L921、再スキャンなし）: `{0: 2,031 / 1: 265 / 256: 97}` → **byte2/3 は全ケース 0**。
- 副次的に ctx へ残るシード成分: `ctx+0x2C` = 付属 Script の `refCount`、`ctx+0x30` = `compiledLength`（この 2 個は `FUN_00500170` で上書きされない）。`ctx+0x28` は SCHR unused（恒 0）。

### 2.4 結論

> **`local_18[10]` = 「`SysWindow+0xbc` のフォームに extra 0x12 で付いている Script の `+0x28` = SCHR 末尾 dword = `scriptType`」**。write-back で host に残るのはその上位 2 バイトだけで、実データでは常に 0。b19「中身の意味は未同定」は解消。
> なお arg2==0（bat 経路／未選択）や付属 Script が無い場合はシード塊自体が飛ぶ（`0x5002D7` / `0x5002E0` の je）。

---

## 3. 残点(2): SysWindow `DAT_00b3a6e0 +0xbc` が指す実体 — **解明（「編集中 Script」は反証）**

### 3.1 書手インベントリ（`v141_2_syswindow` / `v141_6_syswin2`）

| 場所 | 値 | コンテキスト |
|---|---|---|
| `FUN_005802b0 @0x58033E mov [esi+0xbc], ebx` | **0** | ctor（ebx=0、`0x5802DD xor ebx,ebx`）。`+0xbc/+0xc0/+0xc4/+0xc8/+0xcc` を一括 0 |
| `FUN_005821f0 @0x5828EF mov [esi+0xbc], edi` | **解決済みフォーム** | 直前 `cmp edi,[esi+0xbc] / je 0x58332A`（変化時のみ）。`edi` は `FUN_004dc270(*entry)` の戻り値（`0x582857` / ループ内 `0x582897`） |
| `FUN_005821f0 @0x5833A5 mov [esi+0xbc], 0` | 0 | 同関数の終了側（`cmp byte [esp+0x47],0` 分岐） |
| `FUN_00585720 @0x58580E mov [eax+0xbc], 0` | 0 | `call FUN_00582160(0,1)`（シングルトン取得直後） |
| `FUN_005859c0 @0x585A56 mov [eax+0xbc], ebx` | 0（`ebx=0`, `0x5859C7 xor ebx,ebx`） | 状態 `+0x31` のクリア時 |

⇒ **非 0 を書くのは `FUN_005821f0@0x5828EF` の 1 箇所のみ**（b19 記載の `FUN_005821f0 書手` はこれ）。ctor=0 は b19 と一致。

### 3.2 実体のクラス同定（決定的）

`FUN_005821f0` / `FUN_00586000` の読み手:

- `mov ecx,[esi+0xbc] ; add ecx,0x44 ; call 0x41E900/0x41E860`（BaseExtraList → extra ゲッタ）＝ **TESForm 系フォーム**として扱う。
- `mov edx,[ecx_vtable + 0x154/0x170/0x190]` = **vtable slot 85 / 92 / 100**（`0x582B06`, `0x582DE0`, `0x5828xx`, `0x586444`）。
- スロット値の妥当性（`v141_12_last` / `v141_13_micro`）:

| vtable | slots | slot85 | slot92 | slot100 |
|---|---|---|---|---|
| `TESForm` 0xA3BE3C | 55 | `0x726F6620`（ASCII " for"）**非コード** | `0x73206568`("he s") | `0x70632E6D`("m.pc") |
| `Script` 0xA49DA4 | 55 | `0x6F206465`("ed o") | `0x25272029`(") '%") | `0x20746F6E`("not ") |
| `TESObjectREFR` 0xA46C44 | **105** | `0x422DE0` ✓ | `0x4D9B40` ✓ | `0x69D990` ✓ |
| `MobileObject` 0xA732FC | 129 | `0x422DE0` ✓ | `0x4D9B40` ✓ | `0x69D990` ✓ |
| `Actor` 0xA6E074 | 239 | `0x422DE0` ✓ | `0x4D9B40` ✓ | `0x977C50` ✓ |

⇒ **55 スロット（TESForm / Script）では slot85+ は .rdata の文字列**であり呼び出せない。したがって `+0xbc` の実体は **101 スロット以上 = `TESObjectREFR` 系（REFR/MobileObject/Actor のいずれか）**。**Script（55 スロット）であることは構造的に排除** → 「編集中 Script」推定は**反証**。

### 3.3 用途（2 経路）

1. **コンパイル種別シード**（`0x586FA3`）: `call FUN_00582160` → `mov eax,[eax+0xbc]` → `push eax`（`FUN_004fbf00` の arg3 = b19 の写像どおり）。bat 経路は arg3=0。
2. **コンソールコマンド `tag`**（`0x5863ED __mbsicmp(buf, 0xA69F3C="tag")` → `0x586403 mov ebp,[ebp+0xbc]`）:
   `mov eax,[ebp+0x0C] ; push eax ; mov ecx,ebp ; call FUN_004da2a0` → `push eax ; push 0xA4D700("\"%s\" (%08x)") ; call 0x402E30`（sprintf）→ `mov edx,[ebp] ; mov eax,[edx+0x154] ; call eax` → `push eax ; push 0xB3FAB0 ; call 0x560920`。
   つまり **`+0x0C` = FormID**、名前は `FUN_004da2a0`（`__RTDynamicCast` `FUN_009832e6` 経由の取得）。
3. `FUN_005821f0` 本体は付属 Script を `mov edi,eax ; mov eax,[edi+0xc] ; mov edx,[vtable+0xd4]`（Script vtable **slot53** = `FUN_004129A0`）で叩き、`"USED"/"UNUSED"/"Locked"/"Action Flags: "/"Count %d"` 系の状態を表示。

### 3.4 結論

> **`+0xbc` = 現在選択/照会中のリファレンス（`TESObjectREFR` 系フォーム）**。Script ではない。付属 Script は `form+0x44` → extra 0x12 → `+0xC` で得る（＝残点(1)のシード源）。b19 の「編集中 Script 推定」はここで棄却し、"seed 供給元としての選択オブジェクト" に訂正。

---

## 4. 残点(3): ESM ロードされる Script の `+0x28` 供給源 — **解明**

### 4.1 `SCHR` の参照は画像内 5 箇所のみ（`v141_3_schr` / `v141_8_loaders`）

`'SCHR'` = `0x52484353` の imm32 出現: `0x4FA81F`, `0x4FC23D`, `0x52A745`, `0x52B21F`, `0x531370` の 5 件（`SCTX` は 0 件、`SCDA` 4 件、`SCRO` 5 件、`SLSD` 2 件、`SCVR` 1 件）。

### 4.2 読み込み先 3 経路（すべて **`Script+0x18`**）

| # | 経路 | 証拠（実アセンブル） |
|---|---|---|
| A | **Script vtable slot7 = `FUN_004fc0d0`**（汎用サブレコード読込） | `0x4FC2F0: push 0 ; lea ecx,[ebx+0x18] ; push ecx ; mov ecx,esi ; call 0x450C20` ⇒ **dest = ebx+0x18**（ebx = Script） |
| B | **SCPT ローダ `FUN_00531290`** | `0x53137B: push 0 ; push 0xB36548 ; mov ecx,ebx ; call 0x450C20` ⇒ dest = `0xB36548`。全域 Script は **`0xB36530`**（imm32 参照 9 件: `0x530E25`, `0x5312E1/E1/F9`, `0x5313FE`, `0x531428`, `0x531441`, `0x9E4221`, `0xA1C541`）。**`0xB36548 = 0xB36530 + 0x18`**。後続 `mov ecx,0xB36548 ; call 0x60D0A0` は **vtable slot0/1 impl = 単一 `ret`（ノーオプ）** |
| C | **QUST/INFO 段階スクリプト `FUN_0052b1f0`**（`FUN_0052a570` から 2 呼出し、`0x52A5A7 cmp al,0x3B` = record type **QUST**） | `0x52B2B2: push 0 ; lea edi,[ebx+0x24] ; push edi ; call 0x450C20` ⇒ dest = `ebx+0x24`。`ebx` の実体は `FUN_0052acc0` が **`lea edi,[esi+0x0C] ; mov ecx,edi ; call FUN_004fbaa0`** で構築する＝**コンテナ +0x0C に埋め込み Script** → **コンテナ+0x24 = 埋込 Script + 0x18** ✓ |

- 読込ヘルパ `FUN_00450c20 @0x450C20` = `GetChunkData` 系（`this` = レコード、`dest`、`maxSize`）。現在のサブレコード payload（長さ = `this+0x254`）を `_memcpy(dest, ...)`（decompile L61752 確認）。
- ローダ判定: `FUN_00531290` は `movzx ecx,[esi+4] ; lea edx,[ecx+ecx*2] ; cmp al,[edx*4+0xB05E00]` — **`0xB05E00` は formType → 4 文字レコード名の表**（`0='NONE', 1='TES4', …, 13='SCPT', …, 49='REFR', 50='ACHR', …, 57='DIAL', 58='INFO', 59='QUST'`）。Script ctor の `mov byte [esi+4],0x0D` = **SCPT** と一致。

### 4.3 保存側の対称性（+0x18 の追加証拠）

- **Script vtable slot9 = `FUN_004fa810`**（`rdata 0xA49DC8` 参照、E8 呼び出し 0 = 仮想のみ）:
  `0x4FA818: lea eax,[esi+0x18] ; push eax ; push 0x52484353 ; push 0x14 ; call FUN_0046AE40`
  `FUN_0046AE40` = タグ + u16 サイズ + データを出力バッファへ追記する**書き出し**ルーチン → **SCHR(0x14) ≡ `Script+0x18..+0x2B`**。
- **`FUN_004fc7a0`（b19 の W4）**: `+0x18/+0x1C/+0x20/+0x24/+0x28` を 5 dword コピー → `mov dword [esi+0x18], 0`（unused を強制 0）→ `FUN_004f9df0(this, [dest+0x20], [src+0x30])`（**+0x20 = compiledLength / +0x30 = bytecode ptr**）→ `+0x40` のリスト複写 → `+0x2C` の確保/解放。**b19 の「+0x18..+0x28 一括コピー」を decompile で完全確認**。

### 4.4 5 つの ctor 呼び出し元の性格（`v141_8` / `v141_9`）

| 呼び出し元 | 呼び出し数 | 役割 |
|---|---|---|
| `FUN_00531290` @0x5312D5 | — | **SCPT ローダ**（stack Script を ctor → `FUN_004fc7a0(0xB36530, stack)` でヘッダ初期化 → SCHR/SCDA/SCRO を全域 `0xB36530` へ → `FUN_004fbb60` → 戻り値 `0xB36530`） |
| `FUN_0052acc0` @0x52ACFD | E8 1 件（`0x52A7AF`） | **QUST 段階スクリプト用コンテナ ctor**（埋込 Script = container+0x0C、`[esi+0x34]=1` ほか） |
| `FUN_00530db0` @0x530E1A | E8 1 件（`0x5318B6`） | ロード後の破棄/キャッシュクリア（stack Script ctor → `FUN_004fc7a0` → `FUN_004fc980`、`DAT_00b36528/00b3652c = 0`） |
| `FUN_00447df0` @0x44889A | E8 9 件 | formType 別ファクトリ（`switch(param_1)` で `malloc(0x28/0x34/…)`、1 ケースで Script ctor） |
| `FUN_009e4120` @0x9E4225 | E8 0 件 | 静的初期化（`_atexit` 付き）。`0x9E4221` で `0xB36530` を参照 |

- **ロード → 複写の実例**（`0x531CC5`/`0x531CCD`/`0x531CD4`, `FUN_00531b40`）:
  `call 0x531290`（全域 Script をロード）→ `mov ecx,eax ; call 0x4fc7a0`（先頭 5 dword を複写）→ `lea eax,[ebx+0x18] ; push eax ; call 0x530430`。
  `FUN_00531470`（`0x53147D` 呼び出し）も `call 0x531290 ; mov esi,eax ; … ; cmp dword [esi+0x20],0` と同じ形。

### 4.5 結論

> **ESM ロードされる Script の `+0x28` 供給源 = `SCHR` の +0x10 dword（`scriptType`）**。3 つのロード経路がいずれも **`Script+0x18` に 20 バイトを直書き**し、保存側（vtable slot9）も `Script+0x18` から 0x14 バイトを SCHR として書き出す。フィールド対応は既存 ESM 実測（`plan.md` L921）と一致: `+0x18` unused(0) / `+0x1C` refCount / `+0x20` compiledLength / `+0x24` lastVarIndex / **`+0x28` scriptType**。
>
> **b19 との整合（重要な限定）**: `host+0x28=1 は本 EXE で決して立たない」は**一時 host（スタック Script）に限った話**であり維持される（host は ctor=0、恒常書手は write-back のみ、シード上位バイトも 0）。一方 **ESM 由来の quest script 265 件は `+0x28=1` を持つ**が、それらはコンパイルの host にならない（入口は `FUN_00586000` の 2 site、host 常にスタック Script）ため quest ゲート到達不能という裁定は変わらない。

---

## 5. 残点(4): decompiled ラベル欠落 3 件 — **解明（GT 追加候補）**

### 5.1 欠落の事実確認（`v141_4_labels` / `v141_7_misc` / `v141_14_refs`）

| 検査 | `FUN_00509d70` (ShowAnim) | `FUN_00505120` (AFAN) | `FUN_00508340` (SHT) |
|---|---|---|---|
| `dec_index`（定義ヘッダ `// FUN_xxx @ 0x`） | **不在** | **不在** | **不在** |
| 名前文字列の生出現（`docs/Oblivion_decompiled.c` 全行） | **0 件** | **0 件** | **0 件** |
| hex 文字列 `509d70` / `505120` / `508340` の生出現 | **0 件** | **0 件** | **0 件** |
| 画像中の imm32 参照 | `.data:0xB0B708` の 1 件のみ | `.data:0xB0BDC0` の 1 件 | `.data:0xB0C298` の 1 件 |
| E8 呼び出し | **0** | **0** | **0** |
| 埋め込先（直前の Ghidra 関数） | `FUN_00509b30`（L201933） | `FUN_005050b0`（L200082） | `FUN_00507e60`（L201165） |
| 先頭バイト | `53 8b 5c 24 10`（push ebx） | `81 ec 08 02 00 00`（sub esp,0x208） | `8b 44 24 0c 56`（mov eax,[esp+0xc]） |

### 5.2 構造的な理由

- 8 impl 全て（ラベルありの `0x5097A0`/`0x510C70`/`0x510E60`/`0x513C90`/`0x511C10` を含む）**コード側参照は 0・.data オペコードテーブルの 1 ポインタのみ**（`0xB0B460/B708/B938/B960/BB18/BDC0/C298/C310`）。
- つまり**コード参照の有無では 5:3 の差を説明できない**（= Ghidra の関数境界ヒューリスティクス／本プロジェクトで手動作成した関数の差）。ラベルあり 5 件の先頭は `55 8b ec`（2 件）と `6a ff 68`（SEH プロローグ、3 件）で、欠落 3 件はいずれも該当しない。
- ただし**欠落 3 件は capstone 実アセンブルでボディ証拠が揃っている**（b19 §3.2: ShowAnim = `test ebx,ebx / je 0x50A04C`、AFAN = `call 0x4FAE80 → test al,al; test esi,esi`、SHT = `mov eax,[esp+0xC] → FUN_009832E6 → test esi,esi`）。

### 5.3 結論

> **GT 追加候補として確定**: Ghidra 側に `FUN_00509d70` / `FUN_00505120` / `FUN_00508340` の 3 関数を新規作成（または decompiled.c の該当ラベルを GT として追加）。現状は直前関数のボディに吞まれており、デコンパイル上の唯一の欠落原因は「.data テーブル参照のみで関数化されなかった」こと。**再デコンパイルしない限り capstone 代替（b19 §3.2）を本筋とする**。
> 細部（5:3 の差を生む Ghidra 側の判定要因）は**未解明**だが、コード実行には無影響（opid は .data テーブル経由で解決される）。

---

## 6. 残点(5): Script の派生クラス有無 — **解明（派生なし）**

### 6.1 RTTI 階層の再解析（`v141_5d_chd` / `v141_5e_rtti` / `v141_10_final`）

本バイナリの RTTI は **MSVC x86 形式**であることを実測で確定:

- `vtable[-1]` = **COL**（`sig=0`, `offset=0`, `cdOffset`, `pTypeDescriptor`, `pClassHierarchyDescriptor`）— x64 由来の `sig=1` 想定では **COL 0 件**という誤判定になった（訂正記録）。
- `CHD` = `{sig, attributes, numBaseClasses, pBaseClassArray}`、**`pBaseClassArray` はポインタ配列**（各 4 バイト）で、各 `BaseClassDescriptor = {pTypeDescriptor, pDisp, pAttrs}`（インライン 12 バイト構造体ではない）。

**Script の実データ**:
```
Script vtable 0xA49DA4 → [vtable-4] = COL 0xABD748
  COL   : sig=0 offset=0 cdOffset=0 pTD=0xB03358 (.?AVScript@@) pCHD=0xABD75C
  CHD   : sig=0 attrs=0 n=3 pBaseClassArray=0xABD76C
  bases : [0] .?AVScript@@              disp=2 attrs=0
          [1] .?AVTESForm@@             disp=1 attrs=0
          [2] .?AVBaseFormComponent@@   disp=0 attrs=0
```
⇒ `Script : TESForm : BaseFormComponent`（単一継承、多重継承なし）。

### 6.2 派生クラスの探索

- COL **1,001 件**を検出し、**944 クラス**の CHD を解析成功。
- **`.?AVScript@@` を非先頭ベースに持つクラス = 0 件** → **Script に派生クラスは存在しない（末端クラス）**。
- 検証系が生きていることの裏取り: `.?AVActor@@`（9 base）、`.?AUbhkWorldCinfo@@`（5 base）、`.?AVTESNPC@@`（25 base）などが正しいチェーンで得られた。多継承クラス 874 件。
- 未解析 57 件はすべて `base td missing`（`.?AVAlchemyItem@@` / `.?AVQueued*@@` / `.?AV*SettingCollection@@` 等、1 種の TD 名が解決不能）で **Script 無関係**。
- 「Script を含む名前」で 22 件ヒットするが、**全て `.?AVTESScriptableForm@@`**（vtable 0xA3C490 / 4 slots、TESForm にコンポーネントを足す別クラス）であり Script の派生ではない。`NiTListBase<V::$NiTPointerAllocator@I@@PAVScript@@>` 等は **Script\* を要素とするコンテナ**。

### 6.3 構造的な追加証拠（b19 の「imm32 参照は 2 つのみ」を再現）

- 画像全体で `0x00A49DA4` の imm32 参照 = **`0x4FBAD3`（ctor `FUN_004fbaa0`: `mov dword [esi],0xa49da4`）と `0x4FC9AA`（reset 系）の 2 件のみ**。他クラスが Script vtable を自前 vtable として保存していない。
- Script vtable と先頭 3 スロットが一致する vtable は 0 件（同バイト列の再利用なし。最初のヒットは Script vtable 自身）。

### 6.4 結論

> **派生クラスなし**。`Script` は `TESForm : BaseFormComponent` を継承する末端クラスで、本 EXE 内に `Script` を基底に持つ RTTI クラスは 1 件も無い。b19「RTTI 索引に base 参照列が無く未判定」は、**索引（`cache/vtable_index.tsv`）に列が無いだけで、COL/CHD を直接解析すれば確定できる**こと、および**索引の COLL 列 = `0x00ABD748` / TD 列 = `0x00B03358` は正しいこと**をあわせて確認した。

---

## 7. 新規残点（V143+ 粒度）

| ID | 内容 | 優先度の目安 |
|---|---|---|
| V143a | **SysWindow のクラス同定**: ctor `FUN_005802b0`（範囲 `0x5802B0..0x5806D0`）に **vtable ストアが無い**（`mov [esi],0` のまま。唯一の .rdata imm32 ストアは `mov [edi],0xA3D604` = NiAlphaProperty で別物）→ RTTI 名が取れない。`0xA69E80` 群のコマンド表（`visible / close all menus / stack / front / delete %s / reload %s / cls / clr / usz / dof / tag / iConsole*`）から機能名を確定するか | 中 |
| V143b | コンソールコマンド **`tag` の正確な用途**（`"%s" (%08x)` 出力 → vtable slot85 戻り値 → `push 0xB3FAB0; call 0x560920` の意味） | 中 |
| V143c | **`FUN_004dc270`（選択 → フォーム解決）のアルゴリズム**: `[x+0x10]` 配列と `[x+0x14]` カウンタの走査、`DAT_00b333c4 == FUN_00660110(1)` 分岐（player 系？）、vtable slot92 の戻り値チェックと `'"'` 判定による再帰 | 中 |
| V143d | **コンパイル ctx フィールドの意味論**: `ctx+0x28..+0x38` ← SCHR {unused, refCount, compiledLength, lastVarIndex, scriptType} の対応を b03 の quest ゲート（byte `+0x28` 読み）と突き合わせ、`ctx+0x2C/+0x30` がコンパイル中どう使われるか | 中 |
| V143e | **Script の残フィールド**: `+0x2C`（`FUN_004fc7a0` が確保/解放するポインタ）、`+0x34/+0x38/+0x3C`（ctor が `fldz` で float 0 化）、`+0x40` のリスト構造（`FUN_004fc040` の `{value,next}` 走査）と `+0x44`（BaseExtraList との関係 — Script は TESForm 系だが `+0x40..+0x4c` を一括 0 化しており、TESForm フィールドの実配置は未確定） | 中 |
| V143f | **extra type 0x12 の `+0xC` 初回書手**: `0x429F20`（node ctor）は旧 `+0xC` を継承するだけなので、「誰が最初に Script を extra に載せるか」（`SetScript` 系 / form の script メンバとの対応。`TESScriptableForm` の位置づけも含む） | 中 |
| V143g | RTTI の TD 名 1 種が解決不能（57 クラスの base に入り `.?AV...@@` 正規表現に該当せず）→ 台帳 `cache/vtable_index.tsv` の欠落 1 件を特定 | 低 |
| V143h | ラベル欠落 3 件の **Ghidra プロジェクトへの反映可否**（関数新規作成 → 再デコンパイルするか、capstone 代替のまま運用するか） | 低 |

---

## 8. 使用証拠一覧

### 8.1 実行スクリプト（新規、すべて `%TEMP%\opencode`）

`v141_dbg.py`, `v141_1_seed.py`, `v141_1b_node.py`, `v141_2_syswindow.py`, `v141_3_schr.py`, `v141_3b_schr.py`, `v141_4_labels.py`, `v141_5_rtti.py`, `v141_5b_rtti.py`, `v141_5c_rtti.py`, `v141_5d_chd.py`, `v141_5e_rtti.py`, `v141_6_syswin2.py`, `v141_7_misc.py`, `v141_8_loaders.py`, `v141_9_more.py`, `v141_10_final.py`, `v141_11_verify.py`, `v141_12_last.py`, `v141_13_micro.py`, `v141_14_refs.py`（21 本）／出力 `v141_*_out.txt` 19 点

### 8.2 主要アドレス（本稿の根拠）

- シード: `FUN_004d7240@0x4D7240`(`add ecx,0x44;jmp 0x41E900`) / `FUN_0041e900@0x41E900`(`push 0x12; mov eax,[eax+0xC]`) / `FUN_0041e210@0x41E210` / `FUN_00500280` の `0x5002DB,0x5002E6,0x5002EF..0x500319` / `FUN_00500170@0x500190` / `FUN_004fc850@0x4FC8D0`
- ノード/イベントリスト: `0x428BC5..0x428C3C` / `FUN_00429f20` / `FUN_0041e0a0` / `FUN_004fbdc0@0x4FBDC0`
- SysWindow: ctor `FUN_005802b0`(`0x58033E`) / `FUN_00582160` / `FUN_005821f0`(`0x5828E3,0x5828EF,0x5833A5`) / `FUN_00585720@0x58580E` / `FUN_005859c0@0x585A56` / `FUN_00586000`(`0x586403,0x586FA3`) / `FUN_004dc270` / `FUN_004da2a0` / fmt `0xA4D700` / literal `0xA69F3C`
- SCHR: `0x4FA81F`(slot9 保存) / `0x4FC2F0`(slot7 読込) / `0x53137B`(SCPT) / `0x52B2B2`(QUST) / `FUN_00450c20` / `FUN_004fc7a0@0x4FC7A0` / `FUN_0052acc0@0x52ACF2` / 全域 `0xB36530`・`0xB36548`・`0xB36570` / formType 表 `0xB05E00`
- ラベル: `0x509D70→.data:0xB0B708`, `0x505120→.data:0xB0BDC0`, `0x508340→.data:0xB0C298`（E8=0）
- RTTI: COL `0xABD748` / CHD `0xABD75C` / base array `0xABD76C` / TD `0xB03358` / vtable 参照 `0x4FBAD3`,`0x4FC9AA`
- vtable スロット検証: `TESForm 0xA3BE3C(55)`, `Script 0xA49DA4(55)`, `TESObjectREFR 0xA46C44(105)`, `MobileObject 0xA732FC(129)`, `Actor 0xA6E074(239)` — slot85/92/100

### 8.3 既存成果物（読取のみ）

- `tmp/re/b19_quest_byte_writer.md`（本体・§5 の 5 残点）
- `tmp/re/v138_b16_alarm_residuals.md` §2-3（`0x12` = ExtraData type id、`0x41E900/920/F0B0/130/670` = type 0x12 ラッパ群）
- `tmp/re/b25_cc_residuals.md`（+0x44 / extra 構造）
- `tmp/re/cache/named_funcs.txt`（境界）・`vtable_index.tsv`（Script 行 L1213: `55 slots / COLL 0x00ABD748 / TD 0x00B03358`）・`opcode_table.json`
- `plan.md` L921（SCHR オンディスク実測 `{+0 unused/+4 refCount/+8 compiledLength/+12 lastVarIndex/+16 scriptType}`、`scriptType` 分布 `{0:2031,1:265,256:97}`）— **ESM 再スキャンなし**
- `docs/Oblivion_decompiled.c`（部分検索: `dec_index` / `enumerate` のみ）

---

## 9. Android 移植への示唆（実装はしない）

1. **`scriptType` を `Script` の正規フィールドとして持つ**: SCHR(+16) → `Script+0x28` の対応が EXE 側で三経路とも一致したので、移植側は `Script.header.scriptType` を SCHR デコード時に1箇所で赋值すればよい（EXE は 20 バイト直書き＋後続 `ret` スタブなので、パース処理を入れる余地がある）。
2. **コンパイル ctx のシードは「選択オブジェクトのスクリプトヘッダ」**: 本 EXE は refCount/compiledLength だけが生きて scriptType は下位 2 バイトで潰れる実装。移植ではシード内容（どのフィールドを残すか）を明示仕様化する。
3. **`SysWindow+0xbc` 相当物は「選択リファレンス」型で持つ**: Script への直結ではなく `ref → extra0x12 → Script` の遅延解決なので、参照が変わったときだけ再解決（`cmp + je`）する既存パターンをそのまま採れる。
4. **派生クラスなし**が RTTI で確定したので、`Script` を final として扱ってよい（仮想スロット 55 のうち slot7=読込 / slot9=保存 / slot27=`FUN_004fbb60` / slot53=`FUN_004129A0` は実装済み扱いで固定可能）。

（了）
