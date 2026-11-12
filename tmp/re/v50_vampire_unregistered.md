# V50: `vampire` 未登録の理由と影響の解明（Oblivion.exe v1.2.0416 / Steam）

作成: 2026-10-24 / 対象EXE: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`
作業dir: `C:\Users\hiroki.kogarumai\Oblivion_Android`
証拠スクリプト（すべて `%TEMP%\opencode\`）: `v50_selftest.py` / `v50_dis.py` / `v50_analyze.py`

---

## 0. 結論（先に要点）

1. **「11本の CRT dtor thunk が atexit 等で登録されており vampire のみ未登録」という前タスク(V47)の受け継いだ前提は誤り。**
   `0x9C0450..0x9C04BD`（11本 × 10B、stride 0x0A）は **CRT dtor thunk ではなく、`FUN_009FBF20` の C++ EH（例外巻き戻し）用 UnwindMap の action thunk** である。
   各 thunk はファイル内で **ちょうど1箇所** だけ、`.rdata` の `0xAE9754` 起点の UnwindMap から参照される（atexit テーブル等からは0件、`call`/`jmp` rel32 での参照も0件）。
2. **終了時の破棄は「atexit 登録1本 = `0xA24B60`」で行われ、そこでは `count = 0xC (12)` がハードコードされている。**
   つまり **`vampire`@`0xB3B3C8` も含めた12件すべてが終了時に解放される**（逆順、先頭が vampire）。
3. **vampire に thunk が無い本当の理由** = `FUN_009FBF20` は完全に直線的なコード（条件分岐0件）で、12個目の `Set` の直後に **例外を投げうる呼び出しが無い** ため、コンパイラが **EH state を 11 に更新するストアを死コードとして削除**した。state 11 が一度も書き込まれないため、`maxState = 11`（= state 0..10 用）の UnwindMap に **UnwindMap[11] が生成されない** → 12本目 thunk が存在しない。
4. **影響: 正常終了時のリーク・クラッシュ・二重解放はいずれも発生しない。** 11本 vs 12件のズレは EH 巻き戻し機構内部の話であり、`vampire` を含む12件は終了時にちょうど1回ずつ解放される。

---

## 1. セルフテスト 4件（全PASS）

実行: `python "%TEMP%\opencode\v50_selftest.py"` (workdir=`Oblivion_Android`)

```
PASS 1 bytes@0x504050 -> 518b4c24188b5424 .text
PASS 2 bytes@0x4FB580 -> 578b7908 .text
PASS 3 string@0xA7035C -> .\AI\CombatController.cpp .rdata
PASS 4a opid 0x0015 impl==0x504560 -> 0x504560
PASS 4b 532 entries + sentinel@532 -> ('0x507620', '0x1171', 'ADD NEW FUNCTIONS BEFORE THIS ONE!!!')
PASS 4c idx monotonic -> ('0x0', '0x1171')
ALL PASS
```

補足（セルフテスト4の内訳）:

- オペコードテーブル基底 `0xB0AF48`、stride `0x28`。
  レイアウト = `{+0x00 name ptr, +0x04 ptr, +0x08 u32 opid, +0x0C ptr, +0x10 u16…, +0x14 ptr, +0x18 u32 impl, +0x1C 0x4FDE30}`。
- `opid = 0x0015` のエントリは index 21、name = `"OnLoad"`、`impl (+0x18) = 0x504560` ✓
- 有効エントリは index `0..531` = **532件**、index 532（`opid=0x1171`, name=`"ADD NEW FUNCTIONS BEFORE THIS ONE!!!"`, impl=0）がセンチネル。opid は全区間単調増加。

---

## 2. `FUN_009FBF20` の完全デコード

### 2-1. 位置づけ

- VA `0x9FBF20`（.text, size `0x213` → `ret` @ `0x9FC132`）、SHE付き cdecl だが実体は **C++ EHフレーム付きの初期化関数**。
- **CRT静的初期化配列（.rdata）から呼ばれる**: 参照は `0xA2ADEC` のみ。
  隣接エントリ: `0x9FBE90 / 0x9FBEF0 / **0x9FBF20** / 0x9FC140 / 0x9FC170`（並列する初期化関数群）。
- Ghidra デコンパイル: `docs\Oblivion_decompiled.c` line **1035440–1035515**（`// FUN_009fbf20 @ 0x9fbf20`）。

### 2-2. 全命令リスティング（capstone 5.0.7, ret 終端, 上限 0x400）

```
0x009FBF20: push     -1                 ; EH state 初期値 = -1（未構築）
0x009FBF22: push     0x9c04be           ; EH ハンドラ（trampoline）
0x009FBF27: mov      eax, dword ptr fs:[0]
0x009FBF2D: push     eax
0x009FBF2E: push     esi                ; esi は後段で xor esi,esi（=0）に使う
0x009FBF2F: mov      eax, dword ptr [0xb30aac]   ; __security_cookie
0x009FBF34: xor      eax, esp
0x009FBF36: push     eax
0x009FBF37: lea      eax, [esp + 8]
0x009FBF3B: mov      dword ptr fs:[0], eax       ; __ehrec 安装
0x009FBF41: xor      esi, esi                    ; esi = 0（以降の第2引数）
0x009FBF43: push     esi                         ; arg2 = 0
0x009FBF44: push     0xa2f7ec                    ; arg1 = ""
0x009FBF49: mov      ecx, 0xb3b370               ; this = entry#0
0x009FBF4E: call     0x4028d0                    ; String::Set   ← 1件目
0x009FBF53: push     esi
0x009FBF54: push     0xa6c0c8                    ; "item_pickup"
0x009FBF59: mov      ecx, 0xb3b378
0x009FBF5E: mov      dword ptr [esp + 0x18], esi ; EH state = 0
0x009FBF62: mov      dword ptr [0xb3b378], esi   ; entry#1 {buf,len,cap} = 0
0x009FBF68: mov      word ptr [0xb3b37c], si
0x009FBF6F: mov      word ptr [0xb3b37e], si
0x009FBF76: call     0x4028d0                    ; ← 2件目
0x009FBF7B: push     esi
0x009FBF7C: push     0xa6c0b8                    ; "open_container"
0x009FBF81: mov      ecx, 0xb3b380
0x009FBF86: mov      byte ptr [esp + 0x18], 1    ; EH state = 1
0x009FBF8B: mov      dword ptr [0xb3b380], esi
0x009FBF91: mov      word ptr [0xb3b384], si
0x009FBF98: mov      word ptr [0xb3b386], si
0x009FBF9F: call     0x4028d0                    ; ← 3件目
0x009FBFA4: push     esi
0x009FBFA5: push     0xa6c0ac                    ; "chair_sit"
0x009FBFAA: mov      ecx, 0xb3b388
0x009FBFAF: mov      byte ptr [esp + 0x18], 2    ; state = 2
0x009FBFB4: mov      dword ptr [0xb3b388], esi
0x009FBFBA: mov      word ptr [0xb3b38c], si
0x009FBFC1: mov      word ptr [0xb3b38e], si
0x009FBFC8: call     0x4028d0                    ; ← 4件目
0x009FBFCD: push     esi
0x009FBFCE: push     0xa6c098                    ; "activate_pull_push"
0x009FBFD3: mov      ecx, 0xb3b390
0x009FBFD8: mov      byte ptr [esp + 0x18], 3    ; state = 3
0x009FBFDD: mov      dword ptr [0xb3b390], esi
0x009FBFE3: mov      word ptr [0xb3b394], si
0x009FBFEA: mov      word ptr [0xb3b396], si
0x009FBFF1: call     0x4028d0                    ; ← 5件目
0x009FBFF6: push     esi
0x009FBFF7: push     0xa6c08c                    ; "bed_sleep"
0x009FBFFC: mov      ecx, 0xb3b398
0x009FC001: mov      byte ptr [esp + 0x18], 4    ; state = 4
0x009FC006: mov      dword ptr [0xb3b398], esi
0x009FC00C: mov      word ptr [0xb3b39c], si
0x009FC013: mov      word ptr [0xb3b39e], si
0x009FC01A: call     0x4028d0                    ; ← 6件目
0x009FC01F: push     esi
0x009FC020: push     0xa6c078                    ; "book_scroll_read"
0x009FC025: mov      ecx, 0xb3b3a0
0x009FC02A: mov      byte ptr [esp + 0x18], 5    ; state = 5
0x009FC02F: mov      dword ptr [0xb3b3a0], esi
0x009FC035: mov      word ptr [0xb3b3a4], si
0x009FC03C: mov      word ptr [0xb3b3a6], si
0x009FC043: call     0x4028d0                    ; ← 7件目
0x009FC048: push     esi
0x009FC049: push     0xa6c06c                    ; "npc_talk"
0x009FC04E: mov      ecx, 0xb3b3a8
0x009FC053: mov      byte ptr [esp + 0x18], 6    ; state = 6
0x009FC058: mov      dword ptr [0xb3b3a8], esi
0x009FC05E: mov      word ptr [0xb3b3ac], si
0x009FC065: mov      word ptr [0xb3b3ae], si
0x009FC06C: call     0x4028d0                    ; ← 8件目
0x009FC071: push     esi
0x009FC072: push     0xa6c060                    ; "open_door"
0x009FC077: mov      ecx, 0xb3b3b0
0x009FC07C: mov      byte ptr [esp + 0x18], 7    ; state = 7
0x009FC081: mov      dword ptr [0xb3b3b0], esi
0x009FC087: mov      word ptr [0xb3b3b4], si
0x009FC08E: mov      word ptr [0xb3b3b6], si
0x009FC095: call     0x4028d0                    ; ← 9件目
0x009FC09A: mov      byte ptr [esp + 0x10], 8    ; state = 8（この時点では push 前 → offset が 8 小さい）
0x009FC09F: mov      dword ptr [0xb3b3b8], esi
0x009FC0A5: mov      word ptr [0xb3b3bc], si
0x009FC0AC: push     esi
0x009FC0AD: push     0xa6c058                    ; "horses"
0x009FC0B2: mov      ecx, 0xb3b3b8
0x009FC0B7: mov      word ptr [0xb3b3be], si
0x009FC0BE: call     0x4028d0                    ; ← 10件目
0x009FC0C3: push     esi
0x009FC0C4: push     0xa6c050                    ; "crown"
0x009FC0C9: mov      ecx, 0xb3b3c0
0x009FC0CE: mov      byte ptr [esp + 0x18], 9    ; state = 9
0x009FC0D3: mov      dword ptr [0xb3b3c0], esi
0x009FC0D9: mov      word ptr [0xb3b3c4], si
0x009FC0E0: mov      word ptr [0xb3b3c6], si
0x009FC0E7: call     0x4028d0                    ; ← 11件目 (crown)
0x009FC0EC: push     esi
0x009FC0ED: push     0xa6c048                    ; "vampire"
0x009FC0F2: mov      ecx, 0xb3b3c8               ; this = entry#11 (vampire)
0x009FC0F7: mov      byte ptr [esp + 0x18], 0xa  ; state = 10  ← ここで止まる
0x009FC0FC: mov      dword ptr [0xb3b3c8], esi   ; entry#11 {buf,len,cap} = 0（事前ゼロ）
0x009FC102: mov      word ptr [0xb3b3cc], si
0x009FC109: mov      word ptr [0xb3b3ce], si
0x009FC110: call     0x4028d0                    ; ← 12件目 (vampire)
0x009FC115: push     0xa24b60                    ; ★この関数で唯一の関数ポインタ push
0x009FC11A: call     0x981fb4                    ; atexit(0xA24B60)
0x009FC11F: add      esp, 4
0x009FC122: mov      ecx, dword ptr [esp + 8]
0x009FC126: mov      dword ptr fs:[0], ecx       ; __ehrec 解除
0x009FC12D: pop      ecx
0x009FC12E: pop      esi
0x009FC12F: add      esp, 0xc
0x009FC132: ret
```

確認済みの機械的性質:

- **条件分岐（`jcc`）: 0件** — 完全に直線コード。
- **`FUN_009FBF20` 内に現れる `0x9C04xx` 系 imm32 は1件のみ = `0x9C04BE`（EH ハンドラ trampoline、prologue の `push 0x9c04be`）。** `0x9C0450..0x9C04B4`（dtor thunk 群）は **1回も push / call されない（出現 0件 × 全12エントリ）**。
- **関数内で唯一 push される関数アドレス = `0xA24B60`（1件）** = 終了時破棄用コールバック。
- `FUN_004028D0` は thiscall（`ret 8`）: `Set(this=ecx, src=[esp+4], minLen=[esp+8])`。`minLen` は全12件で `0`。

### 2-3. Ghidra デコンパイル引用（`docs\Oblivion_decompiled.c` L1035444–1035515）

```c
void FUN_009fbf20(void)
{
  int *unaff_FS_OFFSET;
  int local_c;                 // saved fs:[0]
  undefined1 *puStack_8;       // = &LAB_009c04be  (EH handler)
  undefined4 local_4;          // EH state（trylevel）

  local_4 = 0xffffffff;
  puStack_8 = &LAB_009c04be;
  local_c = *unaff_FS_OFFSET;
  *unaff_FS_OFFSET = (int)&local_c;
  FUN_004028d0(&DAT_00a2f7ec,0);      // Set(entry0, "")
  local_4 = 0;
  _DAT_00b3b378 = 0; _DAT_00b3b37c = 0; _DAT_00b3b37e = 0;
  FUN_004028d0("item_pickup",0);       // Set(entry1)
  local_4._0_1_ = 1;
  ...（以下 10 ブロック同型）...
  FUN_004028d0("crown",0);             // Set(entry10) @0xB3B3C0
  local_4 = CONCAT31(local_4._1_3_,10);
  _DAT_00b3b3c8 = 0; _DAT_00b3b3cc = 0; _DAT_00b3b3ce = 0;
  FUN_004028d0("vampire",0);           // Set(entry11) @0xB3B3C8
  _atexit((_func_4879 *)&LAB_00a24b60);   // ← atexit は **1回のみ**
  *unaff_FS_OFFSET = local_c;
  return;
}
```

> 注: Ghidra は thiscall の `mov ecx, <global>` を引数に含めず `FUN_004028d0(src, 0)` の2引数で表示する。`this` は capstone の実命令（`mov ecx, imm32`）で確定させた。

### 2-4. 12件の登録表（セット順 / thunk 有無）

**①セット順: `0xB3B370 → 0xB3B378 → … → 0xB3B3C8` の昇順12件（降順・分岐なし）。**
**②`init` 内で dtor thunk（`0x9C0450` 系）が push されている件数 = 全12件とも 0 件**（この関数に現れる `0x9C04xx` は `0x9C04BE` = EH handler の1件のみ）。

| # | Set VA | String オブジェクト | .rdata リテラル | 文字列 | 事前ゼロ init | state (Set実行中) | init内 thunk push 件数 | EH UnwindMap → thunk | 終了時破棄 |
|---|--------|--------------------|------------------|--------|----------------|-------------------|------------------------|----------------------|-----------|
| 0 | `0x9FBF4E` | `0xB3B370` | `0xA2F7EC` | `""` | なし（ローダがゼロ化） | -1 | **0** | UnwindMap[0] → `0x9C0450` | 有 |
| 1 | `0x9FBF76` | `0xB3B378` | `0xA6C0C8` | `item_pickup` | `0x9FBF62..` | 0 | **0** | UnwindMap[1] → `0x9C045A` | 有 |
| 2 | `0x9FBF9F` | `0xB3B380` | `0xA6C0B8` | `open_container` | `0x9FBF8B..` | 1 | **0** | UnwindMap[2] → `0x9C0464` | 有 |
| 3 | `0x9FBFC8` | `0xB3B388` | `0xA6C0AC` | `chair_sit` | `0x9FBFB4..` | 2 | **0** | UnwindMap[3] → `0x9C046E` | 有 |
| 4 | `0x9FBFF1` | `0xB3B390` | `0xA6C098` | `activate_pull_push` | `0x9FBFDD..` | 3 | **0** | UnwindMap[4] → `0x9C0478` | 有 |
| 5 | `0x9FC01A` | `0xB3B398` | `0xA6C08C` | `bed_sleep` | `0x9FC006..` | 4 | **0** | UnwindMap[5] → `0x9C0482` | 有 |
| 6 | `0x9FC043` | `0xB3B3A0` | `0xA6C078` | `book_scroll_read` | `0x9FC02F..` | 5 | **0** | UnwindMap[6] → `0x9C048C` | 有 |
| 7 | `0x9FC06C` | `0xB3B3A8` | `0xA6C06C` | `npc_talk` | `0x9FC058..` | 6 | **0** | UnwindMap[7] → `0x9C0496` | 有 |
| 8 | `0x9FC095` | `0xB3B3B0` | `0xA6C060` | `open_door` | `0x9FC081..` | 7 | **0** | UnwindMap[8] → `0x9C04A0` | 有 |
| 9 | `0x9FC0BE` | `0xB3B3B8` | `0xA6C058` | `horses` | `0x9FC09F..` | 8 | **0** | UnwindMap[9] → `0x9C04AA` | 有 |
| 10 | `0x9FC0E7` | `0xB3B3C0` | `0xA6C050` | `crown` | `0x9FC0D3..` | 9 | **0** | UnwindMap[10] → `0x9C04B4` | 有 |
| **11** | **`0x9FC110`** | **`0xB3B3C8`** | **`0xA6C048`** | **`vampire`** | **`0x9FC0FC..`** | **10** | **0** | **なし（UnwindMap に該当なし）** | **有（`0xA24B60` / count=0xC）** |

- `state` の実値は prologue の `push -1` から始まり、`0..10` の11回だけ書き戻される（`0x9FBF5E` … `0x9FC0F7`）。**`state = 11` のストアは存在しない。**
- `0x9FC09A` のみ offset が `[esp+0x10]` なのは、この箇所だけ `push esi; push imm`（2回 = 8バイト）が **前** に入っていないため（同一の state 変数）。

---

## 3. dtor thunk 群（`0x9C0450..0x9C04BD`）の正体と対応表

### 3-1. 各 thunk のデコード（capstone、全11本）

各 thunk は 10 バイト固定（**stride = 0x0A**、V47 記載の「0x10 程度」は近似）: `B9 <imm32=global>` (`mov ecx, imm32`, 5B) + `E9 <rel32>` (`jmp`, 5B)。
thunk 群の実占有範囲 = `0x9C0450 .. 0x9C04BD`（11本 × 10B = 0x6E）。
rel32 先は **全11本とも `0x402DA0` = `String` Clear/dtor**（`free(buf)` → `buf=0, cap=0, len=0`）。

```
0x009C0450: mov ecx, 0xb3b370 ; jmp 0x402DA0      (b970b3b300 e94629a4ff)
0x009C045A: mov ecx, 0xb3b378 ; jmp 0x402DA0      (b978b3b300 e93c29a4ff)
0x009C0464: mov ecx, 0xb3b380 ; jmp 0x402DA0
0x009C046E: mov ecx, 0xb3b388 ; jmp 0x402DA0
0x009C0478: mov ecx, 0xb3b390 ; jmp 0x402DA0
0x009C0482: mov ecx, 0xb3b398 ; jmp 0x402DA0
0x009C048C: mov ecx, 0xb3b3a0 ; jmp 0x402DA0
0x009C0496: mov ecx, 0xb3b3a8 ; jmp 0x402DA0
0x009C04A0: mov ecx, 0xb3b3b0 ; jmp 0x402DA0
0x009C04AA: mov ecx, 0xb3b3b8 ; jmp 0x402DA0
0x009C04B4: mov ecx, 0xb3b3c0 ; jmp 0x402DA0      ← 最後 = crown（vampire ではない）
```

`FUN_00402DA0`（dtor 本体）:

```
0x00402DA0: push esi
0x00402DA1: mov  esi, ecx
0x00402DA3: mov  eax, dword ptr [esi]     ; buf
0x00402DA5: push eax
0x00402DA6: call 0x401f20                 ; free
0x00402DAB: xor  eax, eax
0x00402DAD: add  esp, 4
0x00402DB0: mov  dword ptr [esi], eax      ; buf = 0
0x00402DB2: mov  word ptr [esi + 6], ax    ; cap = 0
0x00402DB6: mov  word ptr [esi + 4], ax    ; len = 0
0x00402DBA: pop  esi
0x00402DBB: ret
```

### 3-2. thunk の参照元（＝「登録先」）= **C++ EH UnwindMap だけ**

| thunk | 参照箇所（imm32、ファイル全体） | 参照数 |
|-------|-------------------------------|--------|
| `0x9C0450` | `0xAE9758` (.rdata) | 1 |
| `0x9C045A` | `0xAE9760` | 1 |
| `0x9C0464` | `0xAE9768` | 1 |
| `0x9C046E` | `0xAE9770` | 1 |
| `0x9C0478` | `0xAE9778` | 1 |
| `0x9C0482` | `0xAE9780` | 1 |
| `0x9C048C` | `0xAE9788` | 1 |
| `0x9C0496` | `0xAE9790` | 1 |
| `0x9C04A0` | `0xAE9798` | 1 |
| `0x9C04AA` | `0xAE97A0` | 1 |
| `0x9C04B4` | `0xAE97A8` | 1 |

- **`.text` 内からの参照は0件**、**E8/E9 rel32 でこの領域を指す転移も全ファイル0件**（`call`/`jmp` スキャン実施）。
- つまり **atexit/onexit テーブルへの登録は存在しない** → 「CRT dtor thunk が11本登録されている」という前タスクの解釈は、UnwindMap を登録テーブルと誤認したものと判定できる。

### 3-3. FuncInfo（`FUN_009FBF20` の EH メタデータ）

```
0xAE9730: 22 05 93 19   magic  = 0x19930522   (C++ EH FuncInfo)
0xAE9734: 0b 00 00 00   maxState = 11          ← state 0..10 用
0xAE9738: 54 97 ae 00   UnwindMap = 0xAE9754
0xAE973C: 00 00 00 00   TryBlockMap = 0
0xAE9740: 00 00 00 00   ESTypeList  = 0
0xAE9744: 00 00 00 00   EHFlags     = 0
```

参照: `0xAE9730` は `0x9C04D0`（= `mov eax, 0xAE9730` @ `0x9C04CF` のハンドラ）からのみ。

UnwindMap レイアウト `{int toState; void* action}` × 11:

| index | toState | action (= thunk) | 破棄対象 |
|-------|---------|------------------|----------|
| 0 | -1 | `0x9C0450` | `0xB3B370` |
| 1 | 0 | `0x9C045A` | `0xB3B378` |
| 2 | 1 | `0x9C0464` | `0xB3B380` |
| 3 | 2 | `0x9C046E` | `0xB3B388` |
| 4 | 3 | `0x9C0478` | `0xB3B390` |
| 5 | 4 | `0x9C0482` | `0xB3B398` |
| 6 | 5 | `0x9C048C` | `0xB3B3A0` |
| 7 | 6 | `0x9C0496` | `0xB3B3A8` |
| 8 | 7 | `0x9C04A0` | `0xB3B3B0` |
| 9 | 8 | `0x9C04AA` | `0xB3B3B8` |
| 10 | 9 | `0x9C04B4` | `0xB3B3C0` (crown) |
| **11** | — | **存在しない** | **`0xB3B3C8` (vampire) は巻き戻し対象外** |

意味: 「state = k の状態から巻き戻すときは `UnwindMap[k].action()` を実行して state = `UnwindMap[k].toState` へ」。
`state = -1` は未初期化なので action 不要（0番が toState=-1 で終わる）。**state 11 が一度も書かれないため index 11 は生成されない。**

### 3-4. 「11本 vs 12件」対応表 / vampire 用 thunk の存否判定

| String オブジェクト | 対応 thunk | 備考 |
|--------------------|-----------|------|
| `0xB3B370` `""` | `0x9C0450` | 有 |
| `0xB3B378` `item_pickup` | `0x9C045A` | 有 |
| `0xB3B380` `open_container` | `0x9C0464` | 有 |
| `0xB3B388` `chair_sit` | `0x9C046E` | 有 |
| `0xB3B390` `activate_pull_push` | `0x9C0478` | 有 |
| `0xB3B398` `bed_sleep` | `0x9C0482` | 有 |
| `0xB3B3A0` `book_scroll_read` | `0x9C048C` | 有 |
| `0xB3B3A8` `npc_talk` | `0x9C0496` | 有 |
| `0xB3B3B0` `open_door` | `0x9C04A0` | 有 |
| `0xB3B3B8` `horses` | `0x9C04AA` | 有 |
| **`0xB3B3C0` `crown`** | **`0x9C04B4`** | **有（11本目に含まれることを確定）** |
| **`0xB3B3C8` `vampire`** | **—** | **欠落（下記で「どこにも存在しない」ことを確定）** |

**vampire 用 thunk の存否 — 判定: 「隣接アドレスは int3 ではなく、ファイル全体にも存在しない」**

1. `0x9C0450..0x9C04BD` が thunk 群（11本 × 10B）。最終 thunk `0x9C04B4..0x9C04BD` のうち **`0x9C04B9` はその `jmp` 命令の先頭バイト**（V47 記載の範囲 `0x9C0450〜0x9C04B9` はこの末尾 jmp 先頭を指したものと解される）。
   **`0x9C04BE` は int3 ではなく EH ハンドラ本体**:
   ```
   0x009C04BE: mov edx, dword ptr [esp + 8]
   0x009C04C2: lea eax, [edx - 4]
   0x009C04C5: mov ecx, dword ptr [edx - 8]
   0x009C04C8: xor ecx, eax            ; ← 0x9C04C9 はこの命令の ModRM(0xC8) = 手続きの途中
   0x009C04CA: call 0x9811e2           ; __security_check_cookie
   0x009C04CF: mov eax, 0xae9730       ; FuncInfo
   0x009C04D4: jmp 0x980ea4            ; SEH/C++EH ディスパッチ
   ```
2. int3 塗りつぶしは **`0x9C04D9..0x9C04DF` の7バイトのみ**（10バイトの thunk は入らない）。
3. **ファイル全体の imm32 スキャン**: `0xB3B3C8` を指すのは `0x9FC0F3`（`mov ecx, 0xb3b3c8`）と `0x9FC0FE`（`mov dword [0xb3b3c8], 0`）の2箇所 = **すべて `FUN_009FBF20` 内**。`0x9C04C9` 型の thunk は **どこにも存在しない**。
4. `0x9C04E0` / `0x9C04E8` に続く `mov ecx,[ebp-0x10] ; jmp 0x402DA0` 型 thunk は **別の関数（ローカルオブジェクト用、参照元 `0xAE97B0`/`0xAE97B8`）** であり、本テーブルとは無関係。

### 3-5. 終了時破棄: 実際に12件を潰す関数

`FUN_009FBF20` の末尾が `atexit(0xA24B60)`（`call 0x981fb4` → `0x981F78 = _onexit`、戻り値変換からも `atexit` と確定: 成功時 0 を返す C 規約）。

```
0x00A24B60: push 0x402da0      ; dtor  = String Clear
0x00A24B65: push 0xc           ; count = 12   ★ vampire を含む
0x00A24B67: push 8             ; stride = 8
0x00A24B69: push 0xb3b370      ; base  = 先頭
0x00A24B6E: call 0x98124f      ; __stdcall 反復破棄ヘルパ（ret 0x10）
0x00A24B73: ret
```

`FUN_0098124F`（ループ部分）:

```
0x0098125F: mov esi, dword ptr [ebp + 0xc]   ; esi = stride (8)
0x00981262: mov eax, esi
0x00981264: imul eax, dword ptr [ebp + 0x10] ; eax = 8 * 12 = 96
0x00981268: add dword ptr [ebp + 8], eax     ; ptr = 0xB3B370 + 96 = 0xB3B3D0 (末尾直後)
0x0098126F: dec dword ptr [ebp + 0x10]       ; count--
0x00981272: js  0x98127f                     ; count < 0 で終了
0x00981274: sub dword ptr [ebp + 8], esi     ; ptr -= 8
0x00981277: mov ecx, dword ptr [ebp + 8]     ; this = ptr
0x0098127A: call dword ptr [ebp + 0x14]      ; dtor(this)
0x0098127D: jmp 0x98126f
```

- 12回転。1回目 = `0xB3B3D0 - 8` = **`0xB3B3C8` (vampire)**、最終回 = **`0xB3B370`**。
- **→ 終了時、vampire を含む12件すべてが逆順でちょうど1回破棄される。**
- `FUN_0098124F` は汎用ヘルパで、EXE 内に **458 箇所**の呼び出し（アレイ/配列の逆順破棄用）が存在する。

---

## 4. vampire エントリの特殊性: 原因分類（(a)〜(d)）

| 候補 | 判定 | 根拠（実命令・実データ） |
|------|------|--------------------------|
| **(a) 末尾エントリだけ別関数で登録** | **非該当** | Set は `FUN_009FBF20` 内 `0x9FC110`（`push 0xa6c048; mov ecx,0xb3b3c8; …; call 0x4028d0`）。この関数の後ろに続くのは `push 0xa24b60 / call 0x981fb4 / epilogue` のみで、別関数や後続コードでの登録は無い。`FUN_009FBF20` の外部から `0xB3B3C8` を触るコードも存在しない（imm32 スキャン 2件すべて本関数内）。 |
| **(b) タイムリーに未初期化（レイズ/遅延初期化）** | **非該当** | CRT静的初期化配列 `0xA2ADEC` → `FUN_009FBF20` で **`0x9FC0FC..0x9FC110`（事前ゼロ3ストア + Set）が必ず実行される**。遅延/条件分岐は無し（`jcc` 0件）。初期化後に `state=10` まで到達済み。 |
| **(c) thunk が存在するが登録が条件付き** | **非該当** | **thunk 自体が存在しない**（§3-4 判定1〜4: `0x9C04C9` は `xor ecx,eax` の途中、int3 は `0x9C04D9..` の7バイトのみ、ファイル全体で `0xB3B3C8` を `mov ecx` する箇所ゼロ、rel32 転移ゼロ）。登録を分岐するコードも無い（`jcc` 0件）。 |
| **(d) 単なる欠落（バグ）** | **「欠落」は事実だが「バグ」ではない** | ① EH state は `-1 → 0 … → 10` の11回のみ書かれる（`0x9FBF5E`…`0x9FC0F7`）。12件目の Set 直後に例外を投げうる呼び出しが無いため **`state = 11` のストアが死コードとして削除**される。② その結果 `maxState = 11` の FuncInfo に `UnwindMap[11]` が生成されず、12本目 thunk が **無くなる**（コンパイラの正常な出力）。③ 終了時破棄は UnwindMap とは独立に `atexit(0xA24B60)` 1本で **count=0xC** がハードコードされており、vampire を含む12件を破棄する。 |

**分類: (d) 系「見かけ上の欠落」— ただしバグではなく、C++ EH の state 機構に対するコンパイラの正当な省略。**
さらに前提そのものについて: 「11本 = CRT dtor thunk」という認識は誤りで、**終了時破棄の観点では vampire を含む12件が未登録という事態は存在しない**。

### 4-1. `vampire` 文字列の値と参照関係

| 対象 | アドレス | 実読 | 参照（imm32、ファイル全体） |
|------|----------|------|------------------------------|
| .rdata リテラル | `0xA6C048` | `"vampire"`（7文字 + NUL = 8B、`0xA6C048..0xA6C04F`、次は `0xA6C050="crown"` と連続配置） | **1件のみ**: `0x9FC0EE`（`push 0xa6c048` = Set の引数） |
| String オブジェクト（初期化先） | `0xB3B3C8` | `.data` の BSS 領域（`0xB02000 + 0x30C00 = 0xB32C00` より先 → **ファイル非バックド = ローダが0クリア**） | 2件のみ: `0x9FC0F3`（`mov ecx,0xb3b3c8`）、`0x9FC0FE`（`mov dword[0xb3b3c8],0`）= すべて init 内 |
| 初期化元 | `FUN_009FBF20 @ 0x9FC110` | `Set(this=0xB3B3C8, src="vampire", minLen=0)` → ヒープへコピー | — |

比較（crown `0xB3B3C0`）: リテラル `0xA6C050` 参照 1件（init）、オブジェクト参照 3件（`0x9C04B5` thunk / `0x9FC0CA` / `0x9FC0D5` init）。
→ **crown は「init + thunk」、vampire は「init のみ」**。差分は thunk 1本だけ。

**補足（副次的だが確定した事実）**: テーブル12件の String オブジェクト（`0xB3B370..0xB3B3C8`）と、その入力リテラル（`0xA2F7EC` の `""` を除く11本）は、**この EXE 内で `FUN_009FBF20`（初期化）と破棄コード以外から参照されない**（`item_pickup` は別に `"item_pickup.dds"` が `0xA3D982` に存在する別物）。他の絶対参照もデータポインタも存在しない（`0xB3B300..0xB3B410` の全 imm32 スキャン、285件を精査）。つまり **この12件は本 EXE では「書くだけ」** で、読み手が無い。

---

## 5. 影響評価（事実ベース）

1. **終了時のリーク: 発生しない。** `atexit(0xA24B60)` が `count = 0xC = 12` で逆順破棄するため、`vampire` の `buf` も含め12件すべてが `free` される。前タスク前提の「CRT dtor 未了＝解放されない String buf」は該当しない。
2. **クラッシュ・二重解放: 正常経路では発生しない。** thunk 群は `FUN_009FBF20` の例外巻き戻し時のみ走り、その場合その関数は終了しないため `atexit` が未登録のまま（登録は12件目の Set と epilogue の間の1箇所のみ）。逆に正常完了時は thunk が1回も走らないため、二重破棄の経路が無い。
3. **理論上の残余リスク（実行時は未観測）**: atexit 登録直後〜`ret` の間に /EHa 由来の非同期例外が入ると、`state = 10` のまま巻き戻されて entry0..10 が破棄され、その後 `0xA24B60` でもう一度破棄されるウィンドウがある。ただしこれは **state 11 が書かれていても同様に起きる**（登録後に全12件が巻き戻され、終了時にもう一度破棄される）ため、**11本 vs 12件のズレ自体はこのリスクに寄与しない**。静的初期化中の例外は通常 `std::terminate` に到達する。
4. **機能への影響: 皆無。** そもそも12件の String オブジェクトとリテラルは初期化・破棄以外から参照されておらず（§4-1）、`vampire` の「thunk 欠落」はどのコードパスからも観測されない。

---

## 6. 未解決点

1. **なぜ12件の String が「書くだけ」なのか**: アニメーションイベント名らしき文字列だが、読み手が EXE 内に存在しない。別のモジュール/DLL や実行時のヒープ経由（コンテナへの退避）を疑うが、`0xB3B370` を指すデータポインタも存在しないため、本 EXE 単体では説明できない。要: 他 V での別タスク（例: アニメーションイベント配信系の追跡）。
2. **`FUN_004028D0` の第2引数 `minLen`（全12件で0）の意味論**: `max(strlen(src), arg2)` として cap 比較に使われる実装（`0x4028D0..0x402944`）は確認済みだが、`arg2 != 0` の呼び出しが存在するか（= 実際の用途）は未調査。
3. **事前ゼロ（`buf/len/cap = 0`）を entry0 以外にしか出さない理由**: entry0 のみローダのゼロ化に依存する。ソースが `String arr[12] = {...}` の動的初期化なのか個別グローバルなのかは、Ghidra の他関数（例 `FUN_009FBEF0` が `0xB3B348` を同型操作）と併せて未確定。
4. **`0x9C04D9..0x9C04DF` 直後の `0x9C04E0`/`0x9C04E8` thunk 群の所有関数**: 参照元 `0xAE97B0`/`0xAE97B8`（隣接 FuncInfo）は未解析。本テーブルと無関係と判断したが、厳密には別 V で確認要。
5. **前タスク V47 の `tmp\re\v47_string_type_spec.md` の該当記述（「11本の CRT dtor thunk」「vampire のみ dtor 未登録」）は本調査で反証**。既存ドキュメントは編集禁止のため未修正 — 訂正は別タスク/別成果物で。

---

## 付録: 使用コマンド（再現手順）

```
python "%TEMP%\opencode\v50_selftest.py"                     # セルフテスト4件
python "%TEMP%\opencode\v50_dis.py" dis  0x9FBF20 0x400       # FUN_009FBF20 全デコード
python "%TEMP%\opencode\v50_dis.py" dis  0x402DA0 0x60        # String Clear/dtor
python "%TEMP%\opencode\v50_dis.py" dis  0xA24B60 0x40        # atexit ターゲット
python "%TEMP%\opencode\v50_dis.py" dis  0x98124F 0x80        # 反復破棄ヘルパ
python "%TEMP%\opencode\v50_dis.py" dump 0x9C0440 0x1B0       # thunk 群 + EH handler
python "%TEMP%\opencode\v50_dis.py" dump 0xAE9730 0x90        # FuncInfo + UnwindMap
python "%TEMP%\opencode\v50_dis.py" xrefs 0xB3B3C8            # vampire 参照
python "%TEMP%\opencode\v50_analyze.py"                        # 12件登録表 + UnwindMap 表
```
