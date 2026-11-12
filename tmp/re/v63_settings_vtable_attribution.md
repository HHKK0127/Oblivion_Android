# V63: 設定マップ `0xB35574` の真の vtable 帰属確定（string hash/compare 実証）

- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe` (v1.2.0416, Steam)
- 日付: 2026-10-24 / ラウンド: V63
- 方法: PE 自前 struct 解析 + capstone 5.0.7 実逆展開 + `docs\Oblivion_decompiled.c`（lowercased 照合）の三重証拠。**本ファイルは新規成果物。既存ドキュメントは無変更。**
- スクリプト: `%TEMP%\opencode\v63_p1.py`〜`v63_p12.py`（probe_harness.py の Ctx を流用）

---

## 0. セルフテスト4件（ground truth）

| # | 検査 | 実測 | 結果 |
|---|------|------|------|
| ① | `bytes@0x504050` | `518b4c24188b5424` | **PASS** |
| ② | `bytes@0x4FB580` | `578b7908` | **PASS** |
| ③ | `string@0xA7035C` | `.\AI\CombatController.cpp` | **PASS** |
| ④ | テーブル opid `0x0015` impl | `0x504560`（`[0xB0AF48+0x15*0x28+0x18]`） | **PASS** |

`SELFRESULT ALL PASS`（v63_p1.py / v63_p11.py で2回実施）。PE セクション実測: `.text VA=0x401000/RSz=0x626E00/raw=0x400`、`.rdata VA=0xA28000/RSz=0xD9A00/raw=0x627200`、`.data VA=0xB02000/RSz=0x30C00/raw=0x700C00`（ground truth と一致）。

---

## 1. 結論（先出し）

| 項目 | 確定値 |
|------|--------|
| **設定マップ `0xB35574` の真の vtable** | **`0xA40454`**（文字列キー版・全7スロット） |
| **文字列 hash** | **`FUN_004A80A0`**（`_tolower` フォールド djb2: `h = h*0x21 + c`、`bucket = h % [this+4]`＝`% 401`） |
| **文字列 compare** | **`0x584D10`**（`__stricmp(a,b) == 0`。CRT 本体 `0x98262D=__stricmp` → `0x982525=___ascii_stricmp`） |
| `0xA403A8` / `0xA403F0` の正体 | 泛用（pointer/integer キー）マップ基底クラスの vtable。**ctor `FUN_004A7E60` 内の一時値 `0xA403A8` → 同 ctor 末尾 `0xA403F0`**（＋破棄時の2段階入れ替え）。**実行時にこれを保持する生存オブジェクトは存在しない** |
| `0xA40474` / `0xA4049C` の正体 | シングルトン `0xB35468` 本体（＝GMST/Setting マネージャ）の vtable（base ctor 値 `0xA40474` / 最終値 `0xA4049C`） |
| V49 記述 | **正**（「hash/compare は vtable 経由」＝正しい。未解決点1「個別 vtable 未特定」は本ラウンドで**解消**） |
| V55/V57 記述 | **訂正必要**（「マップ vtable = 0xA403A8/0xA403F0」は誤り。訂正文は §6） |

---

## 2. `0xB35468` / `0xB35574` の初期化経路（vtable 書き込み元の特定）

### 2.1 どちらも bss（ground truth の再確認）

- `0xB35468`: `.data` file-backed 範囲 `0xB02000〜0xB32C00` の**外** → `va2off=None`（bss・実行時充填）
- `0xB35574`（= `0xB35468 + 0x10C`）: 同じく bss。
- したがって vtable は**実行時コードが書く**。`.text` に `imm32=0xB35574` を含む「書込み命令」は存在しない（19 件は全て `mov ecx,0xB35574`/`mov eax,[0xB35574]` 等の thiscall 設定・読取）。→ **書込みは `this+0x10C` ディスプレースメント経由**であることが判鍵。

### 2.2 初期化チェーン（実測）

```
CRT 初期化配列 .rdata VA 0xA2840C = 0x9E06B0        (file off 0x62760c, 実読で確認)
  └ 0x9E06B0 (静的初期化 thunk):
       push 0x191                       ; bucket 数 = 401
       mov  ecx, 0xB35468
       call 0x4A8330                    ; FUN_004a8330 = シングルトン ctor
       push 0xA1AA00                    ; dtor thunk
       mov  dword [0xB35468], 0xA4049C  ; 最終( most-derived ) vtable へ書換
       call 0x981FB4                    ; _atexit (Ghidra 名確認済)
  └ 0x4A8330 (FUN_004a8330, ctor) の中で:
       [this+0x108] = 0 ; byte[this+4] = 0
       [this]       = 0xA40474                        ; base vtable
       lea  edi, [this+0x10C]   ; edi = 0xB35574
       push 0x191 ; mov ecx, edi ; call 0x4A7E60      ; 埋め込みマップ ctor(401)
       byte [edi+0x10] = 0                            ; map+0x10 = ownsKey 0
       [edi]           = 0xA40454   ★ 真の vtable をここで確定 ★
```

生逆展開（抜粋, v63_p3.py）:

```
0x4a8369: lea      edi, [esi + 0x10c]
0x4a8370: mov      ecx, edi
0x4a8376: mov      dword ptr [esi], 0xa40474
0x4a837c: call     0x4a7e60
0x4a8381: mov      byte ptr [edi + 0x10], bl
0x4a8384: mov      dword ptr [edi], 0xa40454      ; ★
```

Ghidra デコンパイル（`docs\Oblivion_decompiled.c` L134311-134333、lowercased 照合）:

```c
// FUN_004a8330 @ 0x4a8330
undefined4 * __thiscall FUN_004a8330(undefined4 *param_1,undefined4 param_2)
{
  ...
  *param_1 = &PTR_FUN_00a40474;      /* L134327 */
  FUN_004a7e60(param_2);             /* L134328 = 埋め込みマップ ctor (this = param_1+0x43) */
  *(undefined1 *)(param_1 + 0x47) = 0;      /* L134329 = map+0x10 (ownsKey) = 0 */
  param_1[0x43] = &PTR_FUN_00a40454;        /* L134330 ★ = 0xB35574+0 の vtable */
  ...
}
```

`param_1[0x43]` = offset `0x43*4 = 0x10C`。生逆展開とデコンパイルが一致。

### 2.3 終了時（整合性の副証）

- dtor thunk `0xA1AA00`: `mov ecx,0xb35468; jmp 0x4a8180`
- `FUN_004a8180`（L134197-134221）: `[param_1]=0xA40474` → 埋め込みオブジェクト破棄 → `param_1[0x43] = &PTR_FUN_00a40454; FUN_004a7f20();` → `[param_1]=&PTR_LAB_00a403c8`。**マップの vtable は破棄中も `0xA40454` に復帰する**（=0xA40454 がそのクラスの最終 vtable である裏付け）。

### 2.4 シングルトン `0xB35468` 自体の vtable（参考・V55 既述の確認）

| 段階 | vtable | 出所 |
|------|--------|------|
| base ctor が書いた瞬間 | `0xA40474` | `0x4A8376` / L134327 |
| CRT 初期化 thunk が最終確定 | **`0xA4049C`** | `0x9E06C4: mov [0xb35468], 0xa4049c` |
| complete dtor 中 | `0xA40474` | `0x4A81A9` / L134212 |
| deleting dtor 後 | `0xA403C8`（全スロット `0x982D85`=purecall 系の「未構築」仮想表） | `0x4A81D6` / L134218 |

`0xA4049C` 主要スロット実読（V55 記述と一致）: `+00=0x412E20`(Add)、`+04=0x404FF0`(Remove: `add ecx,0x10c; jmp 0x4524C0`)、`+08=0x4A7FB0`、`+0C=0x6FE080`、`+10=0x4A7D60`(ESM "DATA" 値適用)、`+14=0x4A7CA0`、`+18=0x4A7CB0`(record ptr クリア)、`+1C=0x4A81F0`、`+20=0x4A8290`、`+24=0x4A8040`(GMST 適用入口)。`+28` 以降は vtable 外（文字列 "MAIN"）。

---

## 3. 真の vtable `0xA40454` の全スロット実読と各 VA の正体

`.rdata` 実読（v63_p4.py）。この vtable は 7 スロット（`0xA40454..0xA40473`）で完結し、直後 `0xA40474` から次 vtable が続く（クラスタ密結合）。

| スロット | VA | 正体（証拠） |
|---|---|---|
| `+0x00` | `0x4A8150` | scalar deleting dtor: `[esi]=0xA40454; call 0x4A7F20; if(flag&1) free(esi); ret 4`（L134182-134193） |
| `+0x04` | **`0x4A80A0`** | **文字列 hash**（下 §4）。`FUN_004A80A0`（L134124-134178） |
| `+0x08` | **`0x584D10`** | **文字列 compare**（下 §4） |
| `+0x0C` | `0x6C4F80` | add-entry: `ownsKey([ecx+0x10])!=0` のとき `strlen+1` 確保(`FUN_00401f00`=`NiMalloc`) + `_strcpy_s(0x984B6A)` で **key をコピー**、`[node+8]=value`、`ret 0xC`。`ownsKey==0` なら末尾 `0x6C4FC5`: `node->key=key; node->value=value`（ポインタ格納、コピーなし） |
| `+0x10` | `0x584D30` | key 释放: `if([ecx+0x10]) free([node+4]); ret 4`（ownsKey のときのみ） |
| `+0x14` | `0x46C0D0` | node 確保: `NiMalloc(0xC); [eax+8]=0; ret`（node = {next@0, key@4, value@8} = 12B — V49 の node 定義と一致） |
| `+0x18` | `0x573D00` | node 释放: `[node+8]=0; free(node); ret 4` |

**runtime 使用経路の直接証拠**（slot +4/+8 が char* キーで間接呼び出しされる箇所, 0x575CF9-0x575D45）:

```
0x575cf8: mov  eax, dword ptr [0xb35574]   ; vtable = 0xA40454
0x575cfd: lea  edx, [esp + 0x60]           ; char* key（スタック上に構築した文字列）
0x575d01: push edx
0x575d02: mov  edx, dword ptr [eax + 4]    ; hash = vt[+4] = 0x4A80A0
0x575d05: mov  ecx, 0xb35574
0x575d0c: call edx                          ; bucket = hash(key)
0x575d0e: mov  ecx, dword ptr [0xb3557c]    ; buckets（map+8 = 0xB3557C）
0x575d14: mov  esi, dword ptr [ecx + eax*4]
   ... リンク走査 ...
0x575d20: mov  edx, dword ptr [esi + 4]    ; node->key (char*)
0x575d23: push edx
0x575d2a: lea  eax, [esp + 0x64]           ; key (char*)
0x575d2e: push eax
0x575d2f: mov  eax, dword ptr [edx + 8]    ; compare = vt[+8] = 0x584D10
0x575d32: mov  ecx, 0xb35574
0x575d37: call eax                          ; bool = compare(key, node->key)
0x575d39: test al, al / jne found / mov esi,[esi](=next)
```

→ **スロット +4/+8 はどちらも `char*` を受け取る文字列処理**。整数版スロットではない。

---

## 4. hash / compare の判定（証拠つき・整数版との対比）

### 4.1 文字列 hash = `FUN_004A80A0`（`0xA40454` スロット +4）

生逆展開（v63_p5.py）+ デコンパイル L134124-134178 の突合:

1. **strlen**: `mov cl,[eax]; add eax,1; test cl,cl; jne` によるバイト走査ループ → 長さ `ebx`
2. **ケースフォールドコピー**: `lea eax,[ebx+1]; call 0x983FA0`（`__chkstk`/`_alloca_probe` 系＝**スタック上に len+1 を確保**）、各バイト `movsx eax, byte[...]; push eax; call 0x984E85; mov [dst], al`。`0x984E85` は Ghidra 名 **`_tolower`**（L987678）。デコンパイル L134161: `iVar6 = _tolower(...)`
3. **djb2 積算**:
   ```
   0x4a8117: mov edx,eax / shl edx,5 / add edx,eax   ; edx = h*33
   0x4a811e: movsx eax, cl                            ; eax = c (signed char)
   0x4a8127: add eax, edx                             ; h = h*0x21 + c
   ```
   デコンパイル L134173: `uVar7 = (int)cVar1 + uVar7 * 0x21;`
4. **バケット化**: `div dword ptr [ecx+4]`（this+4 = bucket 数 = 401）→ `mov eax, edx`（剰余）を返す。L134176: `... uVar7 % *(uint *)(param_1 + 4)`

→ **「バイト走査 + 乗算型（*33 累積）+ トワールフォールド + `% bucketCount`」の文字列ハッシュ**。`ret 4`（thiscall、key は1つのみ）。

### 4.2 文字列 compare = `0x584D10`（`0xA40454` スロット +8）

```
0x584d10: mov  eax, [esp+8]     ; arg2 = node->key (char*)
0x584d14: mov  ecx, [esp+4]     ; arg1 = key (char*)
0x584d18: push eax / push ecx
0x584d1a: call 0x98262d         ; __stricmp  (Ghidra 名, L984304)
0x584d1f: add  esp, 8
0x584d22: neg  eax / sbb eax,eax / add eax,1   ; return (__stricmp(...)==0)
0x584d29: ret  8
```

- `0x98262D` = **`__stricmp`**（Ghidra シンボル `__stricmp` @ L984304。NULL 検査＋`_errno=0x16(EINVAL)`→ invalid-parameter ハンドラ経由で `jmp 0x982525`）
- `0x982525` = **`___ascii_stricmp`**（L984215）: 両オペランドを A-Z→+0x20 に折りたたみながら比較する ASCII ループ（生逆展開 v63_p8.py で実証）

→ **大文字小文字を区別しない文字列等値判定**。hash の `_tolower` フォールドと意味的に整合（同一クラスが両方を提供）。

> 補足: `0x584D10` は Ghidra の decompiled.c に**関数定義が存在しない**（前後の `FUN_00584CC0`/`FUN_00584D50` の間に未関数化ギャップとして残存）。よって出典は生逆展開＋vtable スロット参照のみ。命名上の正式 `FUN_*` は付与されていない。

### 4.3 整数版（同胞 `0xB33508` ほか）との対比表

| | 文字列版（本マップ, vtable `0xA40454`） | 整数版（vtable `0xA32C34`/`0xA403A8`/`0xA403F0`） |
|---|---|---|
| hash スロット `+4` | **`0x4A80A0`** | **`0x6A9060`** |
| hash 中身 | `_tolower` フォールド djb2: `h=h*0x21+c`（バイト走査ループ＋スタック退避コピー） → `% [this+4]` | `mov eax,[esp+4]; xor edx,edx; div dword[ecx+4]; mov eax,edx; ret 4` = **`key % bucketCount`（即値剰余・バイト走査なし）** |
| compare スロット `+8` | **`0x584D10`** | **`0x763E80`** |
| compare 中身 | `__stricmp(a,b)==0`（CRT 文字列比較） | `cmp ecx,[esp+8]; sete al; ret 8` = **u32 ポインタ/整数の同一性比較** |
| add-entry `+0xC` | `0x6C4F80`（ownsKey 時は key を malloc+`_strcpy_s` で**コピー**） | `0x67F130`（`node->key=a; node->value=b; ret 0xC`、**コピーなし**） |
| freeKey `+0x10` | `0x584D30`（ownsKey 時のみ `free(node->key)`） | `0x68F970`（`ret 4` の no-op） |
| node alloc/free `+0x14`/`+0x18` | `0x46C0D0` / `0x573D00` | `0x46C0D0` / `0x573D00`（**共通**＝同一基底クラス由来） |
| bucket 数（実例） | 401（`0x191`、`0xB35574+4`） | 37（`0x25`、`0xB33508+4`） |

整数版が V62 までに確定していた `hash=0x6A9060 (key%37)` / `compare=0x763E80 (==)` は、**あくまで整数キー地図の実装**であり、文字列キー地図には使われない。

### 4.4 「文字列キーなのに整数 hash」の矛盾は**存在しない**

V62 で観測された「`0xA403A8`/`0xA403F0` のスロット +4/+8 が整数版と同一」は矛盾ではなく **vtable の取り違え**である:

- それらは**基底クラス（pointer/integer キー）の vtable**であり、ctor `FUN_004A7E60` が self に一時的に書く値。
- 文字列キー地図 `0xB35574` は、基底 ctor の直後に**上位 ctor `FUN_004A8330` が `param_1[0x43] = 0xA40454` と上書き**するため、実行時には常に `0xA40454`（+4/+8 = 文字列版）が載っている。
- 「hash は文字列ポインタではなく先頭文字/長しさせ？」等の変則説は**不要**。正真正銘の `char*` ハッシュ／`__stricmp` である（§3 の runtime 呼び出し証拠参照）。

---

## 5. `0xA403A8` / `0xA403F0` の正体（タスク4）

### 5.1 実体の所在

- **`.data` file-backed 範囲 `0xB02000〜0xB32C00` に、`0xA403A8`/`0xA403F0`（および `0xA40454`/`0xA40474`/`0xA4049C`/`0xA32C34`）を vtable ポインタとして持つオブジェクトは 0 件**（バイト列スキャン、v63_p5.py）。全インスタンスは bss（実行時構築）。
- `.text` での `imm32` 代入箇所は **ctor/dtor 系4関数のみ**:
  - `0xA403A8`: `0x4A7E15`(FUN_004A7E10) / `0x4A7E35`(FUN_004A7E30) / `0x4A7E78`(FUN_004A7E60) / `0x4A7EF7`(FUN_004A7EB0 内 `0x4A7EF5`)
  - `0xA403F0`: `0x4A7EA5`(FUN_004A7E60 末尾) / `0x4A7EDA`(FUN_004A7EB0 先頭)
  - decompiled 照合も同6箇所（L133997/134010/134029/134035/134056/134060）＋他なし。

### 5.2 生成源 = 泛用マップ ctor `FUN_004A7E60`

```c
// FUN_004a7e60 @ 0x4a7e60 (L134021-134037)
undefined4 * __thiscall FUN_004a7e60(undefined4 *param_1, uint param_2)  /* param_2 = bucket count */
{
  param_1[1] = param_2;                      /* this+4 = bucket 数 */
  *param_1   = &PTR_FUN_00a403a8;            /* ★ 構築中の一時 vtable */
  param_1[3] = 0;                            /* this+0xC = count */
  _Dst = FUN_00401f00(param_2 * 4);          /* NiMalloc(buckets) */
  param_1[2] = _Dst;                         /* this+8 = bucket 配列 */
  _memset(_Dst, 0, param_1[1] * 4);
  *param_1   = &PTR_FUN_00a403f0;            /* ★ 構築完了後の vtable */
  return param_1;
}
```

破棄時も2段階で入れ替わる: `FUN_004A7EB0`（L134041-134065）`[esi]=0xA403F0` → `[esi]=0xA403A8` → `free(buckets)`。`FUN_004A7F20` は `[edi]=0xA40410`（文字列版の破棄中 vtable、+4=`0x7DAED0`=**ケース非依存 djb2**）へ一度移してから `FUN_004A7EB0` へ tail-call。

### 5.3 「これらを使うオブジェクト」は実行時に存在しない

- `FUN_004A7E60` の**呼び出し元は `FUN_004A8330`（シングルトン ctor）の1箇所のみ**（decompiled.c 全文1パス走査で2件＝定義＋その1呼出し）。
- その呼び出し元は**帰る前に即座に `0xA40454` へ特化**する（§2.2）。したがって **`0xA403F0` を最終値として保持するオブジェクトは本バイナリに存在しない**（`0xA403A8` は構築中・破棄中の一時値）。
- `0xA403A8`/`0xA403F0` の用途は「同一ヘッダ構造を持つ map クラス階層の**基底クラス実装**（hash=`key%[this+4]`、compare=`==`、add-entry=コピーなし、freeKey=no-op）」であり、派生（文字列）クラスは +4/+8/+0xC/+0x10 のみ差し替え、+0x14/+0x18（node alloc/free）を継承している。

### 5.4 同胞・派生 vtable の系譜（本ラウンドの全数把握）

| vtable | 種別 | dtor `+0` | hash `+4` | cmp `+8` | 採用オブジェクト |
|---|---|---|---|---|---|
| `0xA403A8` | 基底（構築中/破棄中） | `0x4A7E30` | `0x6A9060`(int) | `0x763E80`(int) | 一時値のみ |
| `0xA403F0` | 基底（構築完了） | `0x4A7F70` | `0x6A9060`(int) | `0x763E80`(int) | なし（すぐ派生化） |
| **`0xA40454`** | **文字列キー派生** | `0x4A8150` | **`0x4A80A0`(str)** | **`0x584D10`(str)** | **`0xB35574`（設定マップ）** |
| `0xA40410` | 文字列キー派生（破棄中） | `0x4A7F90` | `0x7DAED0`(str, fold なし) | `0x584D10`(str) | 破棄処理中の一時値 |
| `0xA32C34` | 整数キー派生 | `0x417090`(`scalar_deleting_destructor`) | `0x6A9060`(int) | `0x763E80`(int) | `0xB33508`（fourcc マップ）、他1实例（ctor `0x416A20`） |
| `0xA3BF1C` / `0xA53920` / `0xA64800` | 文字列キー派生（別クラス） | `0x46C430` / `0x521590` / `0x551370` | `0x4A80A0`(str 共有) | `0x584D10`(str 共有) | 別系統の文字列 map（ハッシュ実装を共有） |

`0x4A80A0`（文字列 hash）は `.rdata` 中で **4 本の vtable**（`0xA3BF1C`+4=`0xA3BF20`、`0xA40454`+4=`0xA40458`、`0xA53920`+4=`0xA53924`、`0xA64800`+4=`0xA64804`）から参照される＝同一実装の共有。

### 5.5 補助的に確定した map ヘッダ／使用経路

- ヘッダ: `+0x00` vtable / `+0x04` bucket 数(401) / `+0x08` bucket 配列 ptr(`0xB3557C`) / `+0x0C` count(`0xB35580`) / `+0x10` **ownsKey フラグ byte(`0xB35584`) = 0**。
- **`+0x10` は「重複許可フラグ」ではなく key 所有権フラグ**: `0x6C4F80` は `!=0` のときのみ key を malloc+`_strcpy_s(0x984B6A)` でコピー、`0x412D30` は `==0` のとき既存 node の key ポインタを差し替え、`0x4A7F20` は `!=0` のときのみ node key を free。本マップは 0 → **キーは `.rdata` の静的文字列を直接指す**（＝V49 の「f\*/s\*/i\* 等の文字列キー」と整合）。
- Setting プールは map 直後に埋め込み: `0xB35588` から 8B ストライド（`{value@+0, key@+4}`）。CRT thunk `0x9E0710/0x9E0730/0x9E0750/0x9E0770` … = `push <value>; push <.rdata key>; mov ecx, 0xB35588+8*i; call 0x419A70`。
- setter `FUN_00419A70`（L16470-16501, 生逆展開 `0x419A70`）: `Setting+4 = key`（`[esp+0x1c]`）、`Setting+0 = value`（`[esp+0x20]`）→ 重複検査 `mov ecx,0xB35574; call 0x55E000`（lookup）→ 命中なら警告文字列 `0xA32490`（"Setting key '%s' already used in map.\n..."）を `FUN_004A7A60` で出力 → 未命中なら `mov ecx,0xB35574; call 0x412D30`（insert, key=`Setting+4`, value=`Setting*`）。**V49 の {value@+0,key@+4} は正しい**。
- lookup `FUN_0055E000`（L259920-259942）は **完全に vtable 経由**（`(**(**param_1+4))(key)` → buckets[...] → `(**(**param_1+8))(key,node->key)` → `*out = node->value`）。insert `FUN_00412D30`（L11092-11121）も +4/+8/+0xC/+0x14 を間接呼び出し。**よって同じ lookup/insert 関数が文字列地図・整数地図の双方で使われる**（整数地図側は `FUN_0055E000(0x1a,&out)` 等の整数キー呼び出しが多数存在＝L57578 以下）。
- Add ラッパ `0x412E20`（`0xA40474`/`0xA4049C` スロット +0）: `lea edi,[ecx+0x10c]; call 0x55E000; dup 警告; else insert` — V57§7-#11 の「vtable 先頭スロット」事実と完全整合。

---

## 6. 既存記述の可否判定と訂正文（未適用の文面案）

### 6.1 判定表

| 資料・箇所 | 既存記述 | 判定 |
|---|---|---|
| V49 §2.4/§2.6 | 「キー比較・ハッシュは**仮想関数（vtable 経由）**でパッチされ、キー種別ごとに多重インスタンス化されたテンプレート的ハッシュマップ」「中央マップ `0xb35574` は**文字列キー**なので別 vtable を使うはずだが `mov ecx,0xb35574; call ctor` 形式が無く、親オブジェクト経由の実行時書込と推定。文字列 hash/compare の具体 VA は未特定」 | **正**（vtable 経由＝正しい。親オブジェクト経由＝`FUN_004A8330` で完全に実証された。未解決点1は本ラウンドで**解消**：vtable=`0xA40454`、hash=`0x4A80A0`、compare=`0x584D10`） |
| V55 §1（`v55_gmst_override_path.md` L27） | 「マップ構造 (ctor `0x4A7E60`): `[this+0]`=**vtable(0xA403A8/0xA403F0)**、`[this+4]`=bucket数401、`[this+8]`=bucket配列(`0xB3557C`)、`[this+0xC]`=count、`[this+0x10]`=**重複許可フラグ**」 | **訂正必要**。`0xA403A8`/`0xA403F0` は**基底クラス vtable（ctor 内の一時値）**であり、実行時の値は `0xA40454`。また `+0x10` は**キー所有権フラグ（ownsKey）**であり重複許可ではない（重複は常に拒否＋警告） |
| V57 §1 表（`v57_settings_unified_spec.md` L116） | 「…マップ vtable = **0xA403A8/0xA403F0**」「文字列 hash/compare 関数の具体 VA は未特定のまま（§7-5）」 | **訂正必要**（前半）＋**解消**（後半）。V55 の誤りを採用している |
| V57 §5-223（L223） | 「ctor `0x4A8330`（静的 init `0x9E06B0`, push 401）→ 埋め込みマップ `0x4A7E60(401)` = `0xB35574` 構築、**vtable `0xA4049C` へ書換**」 | **正**（`0xA4049C` はシングルトン `0xB35468` 本体の最終 vtable として正確。ただし同文は埋め込みマップ自体の vtable には触れていない＝欠落であって誤りではない） |
| V57 §7-5（L266） | 「中央マップの文字列 hash/比較関数の具体 VA — V55 でマップ vtable = 0xA403A8/0xA403F0 まで判明したが、そのスロットに入る文字列 hash/compare の VA は未特定」 | **解消**（hash=`FUN_004A80A0`、compare=`0x584D10`）。ただし前提の「vtable = 0xA403A8/0xA403F0」は誤り |
| V49/V55/V57「hash/compare は vtable 経由の間接呼び出し」 | — | **正**（`FUN_0055E000`/`FUN_00412D30`/`0x575CF9` 局所インライン化の3経路すべてで `mov edx,[vtable+4/+8]; call edx` を実測） |

### 6.2 訂正文（未適用の文面案 — 既存ドキュメントには適用しない）

**（案1）`v55_gmst_override_path.md` L27 の置換**

> - マップ構造 (ctor `0x4A7E60` + 上位 ctor `0x4A8330`): `[this+0]`=vtable（**実行時の最終値 `0xA40454`（文字列キー版）**。`FUN_004A7E60` 内では基底クラス vtable が `0xA403A8`（構築中）→`0xA403F0`（完了）と2段階で入れ替わるが、`FUN_004A8330` が `param_1[0x43] = &PTR_FUN_00a40454`（`0x4A8384: mov [edi],0xA40454`）で上書きするため、実行時には存在しない）、`[this+4]`=bucket数401、`[this+8]`=bucket配列(`0xB3557C`)、`[this+0xC]`=count(`0xB35580`)、`[this+0x10]`=**キー所有権フラグ ownsKey**（byte、本マップは0＝キーは `.rdata` 静的文字列を直接参照）。スロット `+4`=`FUN_004A80A0`（`_tolower` フォールド djb2、`h=h*0x21+c`、`% [this+4]`）、`+8`=`0x584D10`（`__stricmp==0`）。

**（案2）`v57_settings_unified_spec.md` L116 セルの置換**

> V55 の確定を採用（ただし vtable 帰属は V63 で訂正）: 埋め込みマップ `0xB35574`（`0xB35468+0x10C`、ctor `0x4A8330` → `lea edi,[this+0x10c]; call 0x4A7E60(401)`）。**マップ vtable = `0xA40454`（文字列キー版）**。文字列 hash = `FUN_004A80A0`、compare = `0x584D10`（§7-5 解消）。

**（案3）`v57_settings_unified_spec.md` L266（§7-5）の置換**

> 5. **（解消・V63）中央マップの文字列 hash/比較関数** — hash = **`FUN_004A80A0`**（`_tolower` フォールド djb2、`h=h*0x21+c`、`bucket=h%401`）、compare = **`0x584D10`**（`__stricmp(a,b)==0`、CRT `0x98262D`→`0x982525`）。いずれも vtable `0xA40454` の `+4`/`+8`。同胞の整数版 `0x6A9060`(`key%[this+4]`)/`0x763E80`(`==`) とは別実装。旧記載「vtable = 0xA403A8/0xA403F0」は**基底クラスの一時 vtable であり誤り**（訂正は §1 表参照）。

---

## 7. 未解決点（本ラウンドでは確定できなかった事項）

1. **`0x584D10` の Ghidra 関数化名** — decompiled.c に定義が存在しない（`FUN_00584CC0` と `FUN_00584D50` の間に未関数化ギャップ）。実体は生逆展開＋vtable 参照で確定済みだが、`FUN_*` ラベルは付いていない。V61 analyzeHeadless 再解析で埋まる可能性あり。
2. **`0xB35468` のクラス名** — 機能（埋め込み Setting マップ、Add/Remove/AddRecord 系仮想、GMST レコード適用経路 `0x44E326`→slot+0x24=`0x4A8040`→`0x4A7D60`）は確定したが、公式 C++ クラス名（`DataHandler` 系か `SettingCollection`/`GameSettingManager` 系か）は RTTI/型名未照合。V55 の「GMST マネージャ」という呼称は機能的には正しい。
3. **`0xA3BF1C` / `0xA53920` / `0xA64800`（文字列 hash を共有する別 vtable 3本）の採用オブジェクト** — 本タスクのスコープ外のため未追跡（いずれも bss 実体）。
4. **登録 2,304 件・bucket 401 の実行時 count 検証** — bss のため静的には検証不能（V49 の数え上げ是正ではなく、実行時 `[0xB35580]` の観測が必要）。
5. **`0x412E20`（Add）の vtable 経由 runtime 呼出元** — V57§7-#11 の残課題（E8 直呼び0件）は本ラウンドでも変化なし。

---

## 8. 使用ツール/証跡

- スクリプト（すべて `%TEMP%\opencode\`）: `v63_p1.py`(selftest+raw refs)、`v63_p2.py`/`v63_p3.py`(CRT thunk/ctor 逆展開)、`v63_p4.py`(vtable 実読)、`v63_p5.py`(hash/compare 本体＋.data/.text スキャン)、`v63_p6.py`/`v63_p7.py`(decompiled 索引/呼出し)、`v63_p8.py`(setter/insert/sibling 逆展開)、`v63_p9.py`〜`v63_p12.py`(CRT 配列・系譜・最終確認)
- decompiled 出典行: L11092(insert)、L16470(setter)、L133992-134344(map/ctor/dtor 系)、L134124(hash)、L259920(lookup)、L983731(`_atexit`)、L984215(`___ascii_stricmp`)、L984304(`__stricmp`)、L987678(`_tolower`)
