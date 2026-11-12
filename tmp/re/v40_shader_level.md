# V40: シェーダレベル enum (DAT_00b42f48, 0..7) の完全語義と Android/GLES 移植方針

対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe` (v1.2.0416, Steam)
解析: 自前 PE パーサ (struct) + capstone 5.0.7 + Ghidra デコンパイル `docs\Oblivion_decompiled.c` ラベル照合 (lowercase)
作成日: 2026-10-24 / 新規ファイル (既存ドキュメント無改変)

---

## 1. セルフテスト 4 件（実測・全 PASS）

PE セクション（自前パーサ実測）:
| sect | VA | vsize | PointerToRawData | rawsize |
|---|---|---|---|---|
| .text | 0x401000 | 0x626C39 | 0x400 | 0x626E00 |
| .rdata | 0xA28000 | 0xD98D2 | 0x627200 | 0xD9A00 |
| .data | 0xB02000 | 0xA9C1C | 0x700C00 | 0x30C00 |

| # | 検査 | file_off | 実測値 | 期待値 | 結果 |
|---|---|---|---|---|---|
| ① | bytes@0x504050 | 0x103450 | `518b4c24188b5424` | `518b4c24188b5424` | **PASS** |
| ② | bytes@0x4FB580 | 0x0FA980 | `578b7908` | `578b7908` | **PASS** |
| ③ | string@0xA7035C | 0x66F55C | `.\AI\CombatController.cpp` | 同左 | **PASS** |
| ④ | opid 0x0015 impl | table 0xB0AF48/stride 0x28 | entry#21 @0xB0B290, impl=`0x504560` | `0x504560` | **PASS** |

---

## 2. FUN_007b45f0（シェーダレベル検出本体）の完全解析

### 2.1 シグネチャ（デコンパイル L670306 / 呼出元 L121863, L122098 実測）

```
void FUN_007b45f0(param_1, param_2, param_3(char), param_4(char), param_5(char*), param_6(int))
```
| param | 実体（呼出元 L122098 実測） |
|---|---|
| param_1 | `DAT_00b06c44` = D3D 頂点シェーダーバージョン dword（ah=major） |
| param_2 | `DAT_00b06c48` = D3D ピクセルシェーダーバージョン dword（ch=major） |
| param_3 | `DAT_00b06db4` = 「シェーダー必須」フラグ（INI/起動オプション由来の byte） |
| param_4 | `*(char*)(adapterDesc + 0x204)` = アダプタ名前バッファの先頭 byte（存在判定ガード） |
| param_5 | `adapterDesc + 4` = GetAdapterDesc 由来のベンダ文字列（例: "ati"/"nv" を含む Description） |
| param_6 | `uVar5` = `*(ushort*)(GetDeviceCaps構造体 + 0x11C)` を、`(>0x60 かつ +0x114 の DWORD < 0x20)` のとき **0x60 にクランプ** した値（L122066-122097）。第2呼叫出し (FUN_004980d0) では全0で呼ばれる |

### 2.2 逆アセンブリ（全経路・実命令）

```
; === 入口: シェーダーバージョン判定 ===
0x7B45F0  mov  eax, [esp+4]            ; param_1 (VS ver)
0x7B45F4  cmp  ah, 2
0x7B45F7  mov  ecx, [esp+8]            ; param_2 (PS ver)
0x7B45FB  push esi
0x7B45FC  jb   0x7B46AE                ; VS major < 2
0x7B4602  cmp  ch, 2
0x7B4605  jb   0x7B46AE                ; PS major < 2
0x7B460B  cmp  dword [esp+0x14], 0     ; param_4 (ベンダ文字列あり?)
0x7B4610  mov  edx, 2
0x7B4615  mov  [0xB42F48], edx         ; ★ level = 2
0x7B461B  mov  [0xB42F40], 0x2F        ; flags = 0x2F (bit4=0x10 ON)
0x7B4625  je   0x7B468E                ; param_4==0 → epilogue
0x7B4627  mov  esi, [esp+0x18]         ; param_5 (vendor str)
0x7B462B  test esi, esi
0x7B462D  je   0x7B468E
; === "ati" 判定 ===
0x7B462F  push 3
0x7B4631  push 0xA3E274                ; = "ati"
0x7B4636  push esi
0x7B4637  call 0x9864D9                ; __strnicmp
0x7B463C  add  esp, 0xC
0x7B463F  test eax, eax
0x7B4641  jne  0x7B4702                ; ati でない → nv 判定へ
0x7B4647  mov  eax, [esp+0x1C]         ; param_6 (caps word)
0x7B464B  cmp  eax, 0x60
0x7B464E  mov  edx, [0xB42F48]
0x7B4654  jle  0x7B46E6                ; ati & param_6 <= 0x60
0x7B465A  cmp  edx, 2
0x7B465D  jne  0x7B466A
0x7B465F  mov  edx, 6
0x7B4664  mov  [0xB42F48], edx         ; ★ ati & >0x60 → level = 6
0x7B466A  mov  [0xB42D74], 6           ; ★ alt-level = 6
0x7B4674  cmp  eax, 0xFF               ; LAB_007b4674 (>0xFF チェック)
0x7B4679  jle  0x7B4682
0x7B467B  mov  byte [0xB42EA5], 1      ; ★ param_6 > 0xFF → フラグ = 1
0x7B4682  cmp  edx, 2
0x7B4685  jge  0x7B468E
0x7B4687  and  [0xB42F40], ~0x10       ; (防御的・到達不能: この時点で level>=2)
; === epilogue（全経路共通） ===
0x7B468E  push edx
0x7B468F  call 0x7B4590                ; FUN_007b4590(level) → DAT_00B42E88 設定
0x7B4694  add  esp, 4
0x7B4697  cmp  byte [esp+0x10], 0      ; param_3 (シェーダー必須)
0x7B469C  pop  esi
0x7B469D  je   0x7B46AD
0x7B469F  test edx, edx                ; level != 0 ? (7B4590 経由の edx)
0x7B46A1  je   0x7B46AD
0x7B46A3  mov  [0xB42F48], 1           ; ★ 必須 && 任意シェーダー可 → level = 1 にクランプ
0x7B46AD  ret
; === VS/PS major < 2 ブロック ===
0x7B46AE  cmp  ah, 1
0x7B46B1  jb   0x7B46D6
0x7B46B3  cmp  ch, 1
0x7B46B6  jb   0x7B46D6
0x7B46B8  mov  edx, 1
0x7B46BD  mov  [0xB42F48], edx         ; ★ VS/PS 両方 1.x → level = 1
0x7B46C3  mov  byte [0xB43070], 0
0x7B46CA  mov  [0xB42F40], 0xF         ; flags = 0x0F (bit4=0x10 OFF)
0x7B46D4  jmp  0x7B4687
0x7B46D6  xor  edx, edx
0x7B46D8  mov  [0xB42F48], edx         ; ★ VS or PS major == 0 → level = 0
0x7B46DE  mov  byte [0xB43070], dl
0x7B46E4  jmp  0x7B4687
; === ati & param_6 <= 0x60 ===
0x7B46E6  cmp  edx, 2
0x7B46E9  jne  0x7B46F6
0x7B46EB  mov  edx, 4
0x7B46F0  mov  [0xB42F48], edx         ; ★ ati & <=0x60 → level = 4
0x7B46F6  mov  [0xB42D74], 6           ; ★ alt-level = 6
0x7B4700  jmp  0x7B4682
; === "nv" 判定 ===
0x7B4702  push 2
0x7B4704  push 0xA3E24C                ; = "nv"
0x7B4709  push esi
0x7B470A  call 0x9864D9                ; __strnicmp
0x7B470F  add  esp, 0xC
0x7B4712  test eax, eax
0x7B4714  jne  0x7B4763                ; ati/nv 以外
0x7B4716  mov  eax, [esp+0x1C]         ; param_6
0x7B471A  cmp  eax, 0x60
0x7B471D  mov  edx, [0xB42F48]
0x7B4723  jle  0x7B4744                ; nv & param_6 <= 0x60
0x7B4725  cmp  edx, 2
0x7B4728  jne  0x7B4735
0x7B472A  mov  edx, 5
0x7B472F  mov  [0xB42F48], edx         ; ★ nv & >0x60 → level = 5
0x7B4735  mov  [0xB42D74], 5           ; ★ alt-level = 5
0x7B473F  jmp  0x7B4674                ; >0xFF チェックへ
0x7B4744  cmp  edx, 2
0x7B4747  jne  0x7B4754
0x7B4749  mov  edx, 3
0x7B474E  mov  [0xB42F48], edx         ; ★ nv & <=0x60 → level = 3
0x7B4754  mov  [0xB42D74], 5           ; ★ alt-level = 5
0x7B475E  jmp  0x7B4682
; === ati/nv 以外 ===
0x7B4763  mov  edx, 2
0x7B4768  mov  [0xB42F48], edx         ; ★ その他ベンダ → level = 2
0x7B476E  mov  [0xB42D74], edx         ; ★ alt-level = 2
0x7B4774  jmp  0x7B468E
```

### 2.3 GPU 検出分岐表（閾値 → レベル。実命令より確定）

| 条件（param_1..6） | DAT_00b42f48 (level) | DAT_00b42d74 (alt) | その他 |
|---|---|---|---|
| VS major=0 または PS major=0 | **0** | （不変） | b43070=0, flags &= ~0x10 |
| (VS<2 または PS<2) かつ 両方 >=1 | **1** | （不変） | b43070=0, flags=0x0F (&=~0x10) |
| VS>=2 かつ PS>=2（基底） | **2** | （不変） | flags=0x2F |
| 〃 + ベンダ文字列なし/NULL | 2（維持） | （不変） | — |
| 〃 + "ati" (`__strnicmp`3文字) + param_6 **<= 0x60** | 2→**4** | **6** | — |
| 〃 + "ati" + param_6 **> 0x60** | 2→**6** | **6** | param_6>0xFF → b42ea5=1 |
| 〃 + "nv" (`__strnicmp`2文字) + param_6 **> 0x60** | 2→**5** | **5** | param_6>0xFF → b42ea5=1 |
| 〃 + "nv" + param_6 **<= 0x60** | 2→**3** | **5** | — |
| 〃 + ati/nv 以外 | 2 | **2** | — |
| 上記いずれか + param_3(必須)!=0 かつ level!=0 | 1 にクランプ | — | level==0 なら 0 のまま |

- レベル昇格は常に `if (level == 2) level = N` のガード付き（既に 1 へクランプ済みの再呼び出しでは昇格しない）。実代入は計 9 箇所（9x `mov [0xB42F48], ...`）+ 初期化 1 箇所。
- **昇格なしで level が 0/1 のまま epilogue に達した場合のみ** `and [0xB42F40], ~0x10`（bit4 クリア）が実行される（0x7B4687）。基底=2 で始まる上位経路では jge が常に成立し不通。

### 2.4 派生グローバル（実測）

| グローバル | 書き | 意味（実測） |
|---|---|---|
| DAT_00b42d74 (0xB42D74) | 6x(7b45f0) + 1x(7b7250=0) = **6書き/2読み** | 「ベンダ最大レベル」: ati→6 / nv→5 / その他→2 / リセット→0。読むのは FUN_00404f00 と FUN_007b4780 の param!=0 分岐のみ |
| DAT_00b42e88 (0xB42E88) | 3x(FUN_007b4590) | **level→定数マッピング**: level{1,2}→**2**, level{3,4,5}→**6**, level{6,7}→**8**, default→2。読側 10 箇所（FUN_007cd090 / 007cefd0 / 008453f0×2 / 00845870×2 / 0085a200 / 008627c0×2 / 008637d0） |
| DAT_00b42f40 (0xB42F40) | level>=2 で 0x2F、level1 で 0x0F、&=~0x10（level<2 時）+ FUN_004826f0/005df160 の bit5(0x20) 切替 + FUN_004073d0 の bit4 切替 | フラグ群。**bit4(0x10) = 「ps_2_0 相当以上」**（level>=3 と併用で7箇所 `test [0xB42F40], 0x10`）、bit3(0x08) は FUN_007ff4a0 がテスト。計 41 参照 |
| DAT_00b43070 (0xB43070, byte) | 7b45f0 が 0（1.x/0 経路）/ FUN_00498340 L122081-122085 が 0/1 | 「追加シェーダーフォーマット対応」フラグ（`DAT_00b06de4 && DAT_00b06cc4 && DAT_00b350d6` の結果）。計 55 参照（cmp 47 / read 4 / write 4） |
| DAT_00b42ea5 (0xB42EA5, byte) | 7b45f0 が 1（param_6>0xFF 時）/ 7b7250 が 0 | 「高 device-id GPU」フラグ。計 10 参照（cmp 8 / write 2）。FUN_007dab80 の variant 選択に使用 |
| DAT_00b43071 | FUN_00498340 L122074（CheckDeviceFormat 相当の成否） | FUN_007dab80 が variant コード計算に使用 |

### 2.5 FUN_007b4590（epilogue ヘルパー、0x7B4590）

`switch(level-1)`（テーブル 0x7B45C8, 7件）: level1-2 → `[0xB42E88]=2`、level3-5 → `=6`（ecx=6 を代入）、level6-7 → `=8`、default → `=2`。FUN_007b45f0 からのみ呼出（E8 直接 1 件）。

### 2.6 呼出元（E8 直接 scan 実測）

| 関数 | 呼出 | 呼び出し内容 |
|---|---|---|
| FUN_004980d0 @0x4982BF | 1 | `FUN_007b45f0(0,0,DAT_00b06db4,0,0,0)`（L121863。VS/PS=0 → level 0。強制フラグ時のみ level 1） |
| FUN_00498340 @0x49885B | 1 | `FUN_007b45f0(DAT_00b06c44, DAT_00b06c48, DAT_00b06db4, iVar9+0x204, _Str1, uVar5)`（L122098。本検出。直後で `if (DAT_00b42f48 < 5) DAT_00b06f14 = 0`、`__strnicmp(_Str1,"ati",3)` → LoadLibraryA("ATIMGPUD.dll")/AtiQueryMgpuCount、"nv" → NVCPL.dll/NvCplGetDataInt） |

FUN_007b7250（1件、FUN_00a10e50 から）: `DAT_00b42f48 = 0; DAT_00b42d74 = 0;` ほかシェーダーグローバル約60個をゼロ化 + flags=0x2F、refcount 付き COM オブジェクト解放（= シェーダーシステムの既定化/終了処理。L671878-672073）。

---

## 3. レベル 0..7 → ps_* / vs_* 対応表（実アドレス・実内容つき）

### 3.1 FUN_007b4780（→ps プロファイル名, L670385）と FUN_007b47e0（→vs, L670416）

- FUN_007b4780(`char param`): param==0 → `DAT_00b42f48`、param!=0 → `DAT_00b42d74`。`add eax,-1; cmp eax,6; ja default` で 1..7 を switch（ジャンプテーブル 0x7B47C4, 7件）。default(0/8+) → `xor eax,eax` = **NULL 返却**。
- FUN_007b47e0(void): `DAT_00b42f48` を同様 switch（テーブル 0x7B480C, 7件）。default → NULL。

### 3.2 対応表（8段階 + 未使用値）

| level | ps 名 (FUN_007b4780) | 文字列アドレス | vs 名 (FUN_007b47e0) | 文字列アドレス | BSSM_SV マクロ名 (FUN_007b7070) | マクロ文字列アドレス | 検出器での到達 |
|---|---|---|---|---|---|---|---|
| 0 | *(NULL)* | — | *(NULL)* | — | `BSSM_SV_NONE` | 0xA8F230 | ○ (VS/PS major=0) |
| 1 | `ps_1_3` | 0xA8CF1C | `vs_1_1` | 0xA8CDC4 | `BSSM_SV_1_X` | 0xA8F224 | ○ (1.x または必須クランプ) |
| 2 | `ps_2_0` | 0xA8CDB0 | `vs_2_0` | 0xA8D0BC | `BSSM_SV_2_0` | 0xA8F218 | ○ (基底/他ベンダ) |
| 3 | `ps_2_a` | 0xA8D348 | `vs_2_0` | 0xA8D0BC | `BSSM_SV_2_A96` | 0xA8F208 | ○ (nv & param_6<=0x60) |
| 4 | `ps_2_b` | 0xA8D340 | `vs_2_0` | 0xA8D0BC | `BSSM_SV_2_B96` | 0xA8F1F8 | ○ (ati & param_6<=0x60) |
| 5 | `ps_2_a` | 0xA8D348 | `vs_2_0` | 0xA8D0BC | `BSSM_SV_2_A` | 0xA8F1EC | ○ (nv & param_6>0x60) |
| 6 | `ps_2_b` | 0xA8D340 | `vs_2_0` | 0xA8D0BC | `BSSM_SV_2_B` | 0xA8F1E0 | ○ (ati & param_6>0x60) |
| 7 | `ps_3_0` | 0xA8D338 | `vs_3_0` | 0xA8D350 | `BSSM_SV_3_0` | 0xA8F1D4 | **× 未使用**（後述） |

- 上記8文字列は .rdata 内の全 `ps_*/vs_*` プロファイル文字列を網羅走査した結果（重複なし・全8件）。
- **3/4 と 5/6 の差**: ps/vs プロファイル名は同一（ps_2_a/ps_2_b）だが、**BSSM_SV マクロ名が `*_96` か否か**で別物（3/4 = 96 スロット制限版）。FUN_007b4590 の DAT_00B42E88 も {3,4,5}→6 で同一グループ。
- **level 7 は検出器が決して代入しない**: 代入9箇所の値は {2,6,4,1,0,1,3,5,2} だけで、7b7250 のリセットも 0。書き10件すべてを実測済み（4章）。よって **ps_3_0 / vs_3_0 / BSSM_SV_3_0 は出荷版で到達不能**（dead mapping）。
- 呼出実測: FUN_007b4780 = 15 件（FUN_007bb280×8, FUN_00498340×2, FUN_007eee40, FUN_007fd260, FUN_00801c90, FUN_00803e40×2）、FUN_007b47e0 = 14 件（FUN_007bb280×10, FUN_00498340, FUN_007eee40, FUN_007fd260, FUN_00803e40）。主消費者はシェーダーコンパイル群。

### 3.3 補助: FUN_007b7070（BSSM_SV マクロ名 getter, L671769）
`switch(DAT_00b42f48)` 0..7（テーブル 0x7B70B8）。上表のとおり。default → `"???"`。

### 3.4 補助: FUN_00404f00（レベル getter, L4056）
`return param ? DAT_00b42d74 : DAT_00b42f48;`（alt 選択付き単純 getter）。

### 3.5 補助: FUN_007dab80（variant コード生成, L697225）
`switch(level)`:
- level0/default → 0、1→1、2→2、3→5、4→8
- 5 → ea5==0&&7070==0: **3** / ea5==0&&7070: 10+((b43071)!=0) → 10/11 / ea5&&7070==0: **4** / ea5&&7070: 12+((b43071)!=0) → 12/13
- 6 → 同4分岐: **6** / 14/15 / **7** / 16/17
- 7 → 7070? 18/19 : **9**
= (level, b42ea5, b43070, b43071) → 0..19 のシェーダーバリアント ID。

---

## 4. DAT_00b42f48 参照 137 件の分類

### 4.1 生スキャン（.text 内 raw bytes `48 2F B4 00`、149 ヒット）

| 種別 | 件数 |
|---|---|
| cmp（即値/レジスタ） | 123 |
| read (`mov eax, [0xB42F48]`) | 16 |
| **write (`mov [0xB42F48], ...`)** | **10**（7b45f0×9 + 7b7250×1） |

デコンパイル側で `DAT_00b42f48` を含む行 = **137**（V25 の GT「137参照」と一致。残り12は検出器内部の自読み）。

### 4.2 レベル閾値別ヒストグラム（137 参照。デコンパイル実抽出）

| 条件形 | 件数 | 意味 |
|---|---|---|
| `1 < DAT` (level>=2) | 45 | 「シェーダーパス可否」の主判定 |
| `DAT < 2` (level<=1) | 26 | 上の逆（1.x 以下フォールバック） |
| `((DAT<2)-1 & MASK)+base` 定数選択 | 13 | level<2 か否かで定数ペア切替 |
| `DAT < 5` (level<=4) | 8 | 「ps_2_a/b 本番版 (5/6) 未満」判定 |
| `2 < DAT` (level>=3) | 7 | 「ps_2_a 以上」判定（大抵 flags bit4 と併用） |
| `DAT == 0` | 6 | 固定機能のみ |
| `DAT == 1` | 4 | ps_1_3 専用経路 |
| `DAT == 2` ガード | 4 | 検出器内部の昇格ガード |
| `switch(DAT)` | 4 | 7b4590 / 7b4780 / 7b7070 / 7dab80 |
| `DAT < 3` (level<=2) | 3 | |
| `DAT < 7` (level<=6) | 1 | **level7 のみ生きる唯一の比較**（FUN_007fac40 L720141: `if (level<7) DAT_00b2dcfc=8`） |
| 値読み（getter/引数/退避） | 6 | 404f00, 7b45f0→7b4590, 7b4780, 7d1170(iStack), 80d110(iVar8), 85ae20(iStack) |
| **write** | **10** | |

### 4.3 上位の参照箇所（レベル閾値別・代表 20 系統）

**level>=2 系（45件）主な所属関数と文脈:**
| VA | 関数 (L) | 文脈 |
|---|---|---|
| 0x49BDFF | FUN_0049b930 (L124230) | `push 0x14` へ分岐（デバイス/初期化系） |
| 0x691DE2 / 0x691F1F / 0x6920D8 / 0x6921ED | FUN_00691da0/691ee0/6920b0/6921d0 (L459861-460035) | 光/被写界深度系シェーダー群（`b42f3e&&b42e84` と合成条件） |
| 0x693F5F / 0x69406D / 0x6940DD / 0x6941A0 / 0x69426F | FUN_00693e70/694050/6940c0/694120 (L461456-461598) | 同上系統 |
| 0x7E2D69 等 setl×12 | FUN_007e2d60/7e2d90/7e3160/7e3e00/7f3760/7f3790/7f3890/7f3c10/7f4790/7f4970 (L703194-715663) | `DAT_00b4600c / DAT_00b468fc = ((level<2)-1 & MASK)+base` の**定数ペア選択**（0x28/0x78, 0x4B/0xEB）＝ レンダーステート定数切替 |
| 0x7D8AAC | FUN_007d85d0 (L695630) | `level<3 || flags&0x10` パターン |
| 0x80D78F/0x80D7B8/0x80D7D2 | FUN_0080d110 (L729722) | 動的閾値（レジスタ比較, 後述） |
| 0x7FAED3 | FUN_007fac40 (L720005) | level 7 比較の唯一箇所 |
| 0x7C8763 | FUN_007c8710 (L684981) | **シャットダウン本体でも level>=2 で経路分岐** |
| 0x7D12A0 | FUN_007d1170 (L690873) | 第2系統 shutdown でも分岐 |

**level<=1 系（26件）代表:** 0x406B09(FUN_00406950) / 0x507E7E・0x50800E・0x508192(FUN_00507e60) / 0x4D01DF・0x4D0A27・0x4D0A8A(FUN_004d0190/4d06c0) / 0x5EE1CA・0x5F37F2(FUN_005ee1b0/5f3140) / 0x668738→L668750(FUN_007b2800) / 0x7C3295・0x7C33E0(FUN_007c3240) / 0x7E97AF 等（FUN_007e8e10）/ 0x812B53(FUN_008129f0)。

**`<5` 系（8件）:** FUN_00498340×3（L121444 `DAT_00b430b0=(level<5)-1&2`、L122099、L122309）/ FUN_005dde20・005deb80×2（L344036/344468/344542）/ FUN_007da220（L697150: `level<5 → *(u16*)(x+0x18)&=0xfffe; *(u8*)(x+0x1a)=0x80`）/ FUN_007dab80 間接（L697150）等。

**`>=3` 系（7件）:** 0x405A80 / 0x407484 / 0x7C33E0 / 0x7D3385 / 0x7D4702 / 0x7D64BA / 0x7D8AAC / 0x882EFF の大半が `level>=3 && (flags & 0x10)` の合成条件。

**レジスタ閾値比較（16件・閾値は局所変数）:** 0x406B0B(edi), 0x40CD0A(esi), 0x51028A/0x5102FB/0x51043D(esi, FUN_0050ff70), 0x7B2AE6(ebp), 0x7C3297(edi), 0x7C3C96(esi), 0x7E839B/0x7E8653(ebx, FUN_007e7f70), 0x7E91B4/0x7E9216(esi, FUN_007e8e10), 0x80B7D0(esi), 0x80D78F/0x80D7BA/0x80D7D4(edi, FUN_0080d110)。閾値は直近24byteの即値解決に失敗（後述 §7）。

### 4.4 まとめ
- 参照の中心は「**level>=2 でシャーディング/シェーダーパスを有効化**（45）」「**level<=1 で固定機能/低品質フォールバック**（26+13）」の二分岐であり、閾値 5（ps_2_a/b 本番版）が8件、閾値 3（ps_2_a 以上+flags）が7件、閾値 7 が1件。
- INI/メニュー等からの直接書込は **0**（書き10件すべて検出器+リセット）— V25 の GT と再一致。

---

## 5. 0x7FAA90（ShadowLightShader の二段間接トラポリン）と vtable 0xA90634

### 5.1 FUN_007faa90 本文（デコンパイル L719889-719897、引用）

```c
void __fastcall FUN_007faa90(int *param_1)
{
  (**(code **)(*param_1 + 0xac))();      /* WARNING: Could not recover jumptable at 0x007faaa8. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (**(code **)(*param_1 + 0xb0))();
  return;
}
```
逆アセンブリ（0x7FAA90）:
```
push esi / mov esi,ecx / mov eax,[esi] / mov edx,[eax+0xAC] / call edx
mov eax,[esi] / mov edx,[eax+0xB0] / mov ecx,esi / pop esi / jmp edx
```
= **vt+0xAC（slot43）を通常呼び出し → vt+0xB0（slot44）を this 引き継ぎの tail-call** する二段連鎖。Ghidra の "jumptable" 警告は末尾 `jmp edx` の誤検出。

### 5.2 vtable 0xA90634（ShadowLightShader, 48 スロット、.rdata 初期値実測）

| slot | +off | target | slot | +off | target |
|---|---|---|---|---|---|
| 00 | +0x00 | 0x7D1EB0 | 24 | +0x60 | 0x77A450 |
| 01 | +0x04 | 0x7C8B20 | 25 | +0x64 | 0x7C90B0 |
| 02-05 | +0x08..14 | 0x6F7070 (同値×4) | 26-29 | +0x68..74 | 0x77A4F0/77A520/77A550/77A580 |
| 06 | +0x18 | 0x80B190 | 30 | +0x78 | 0x76C780 |
| 07 | +0x1C | 0x91FEA0 | 31 | +0x7C | 0x7790C0 |
| 08 | +0x20 | 0x7790B0 | 32 | +0x80 | 0x77AA10 |
| 09 | +0x24 | 0x76C7A0 | 33 | +0x84 | 0x7C8F50 |
| 10 | +0x28 | 0x7C9110 | 34 | +0x88 | 0x7CC280 |
| 11 | +0x2C | 0x7C9F30 | **35** | **+0x8C** | **0x7C8710** ← FUN_007c8710 が呼ぶ slot |
| 12 | +0x30 | 0x77A1B0 | 36 | +0x90 | 0x7D1C60 |
| 13 | +0x34 | 0x7C9230 | 37 | +0x94 | 0x801430 |
| 14 | +0x38 | 0x77A1F0 | 38 | +0x98 | 0x977C50 |
| 15 | +0x3C | 0x77A310 | 39 | +0x9C | 0x801380 |
| 16 | +0x40 | 0x7790E0 | 40 | +0xA0 | 0x977C50 |
| 17 | +0x44 | 0x76C7D0 | 41 | +0xA4 | 0x7AEF60 |
| 18 | +0x48 | 0x77A0B0 | **42** | **+0xA8** | **0x7FAA90** ← 二段トラポリン |
| 19 | +0x4C | 0x77A0F0 | **43** | **+0xAC** | **0x7CD090** |
| 20 | +0x50 | 0x77A3F0 | **44** | **+0xB0** | **0x7CEFD0** |
| 21 | +0x54 | 0x77A400 | 45 | +0xB4 | 0x7D1170 |
| 22-23 | +0x58..5C | 0x60D0A0 (同値×2) | 46 | +0xB8 | 0x977C50 |
| | | | 47 | +0xBC | 0x83ABE0 |

### 5.3 既存マップ照合

- `tmp\re\vtable_map.txt` L1511: `0x00A90634  48  .?AVShadowLightShader@@  ShadowLightShader` → **アドレス・48スロット・クラス名すべて一致（2/2 ファイル）**
- `tmp\re\rtti_vtables.txt` L1511: `0x00A90634  .?AVShadowLightShader@@  ShadowLightShader` → **一致**
- 本タスクの直接 COL 解析（[0xA90630]=0xACFE38）では pTypeDesc が 0 を返し name 再現には至らず（§7）。クラス同定は既存マップ 2 件を採用。

### 5.4 シャットダウン時（FUN_007c8710 経由）の振る舞い（実測）

1. `FUN_007c8710`（slot35, 0x7C8710）入口: `mov esi,ecx / mov eax,[esi] / mov edx,[eax+0xA8] / call edx` → **0x7FAA90 を呼ぶ**（0x7C8715 実オペコード確認）。その後 `mov ecx,esi / call 0x820910`。
2. 0x7FAA90 は vt+0xAC = **FUN_007cd090**（スタック 0x2868B、入口 `mov ecx,[0xB42E88]` = **level由来定数 2/6/8 を読み**、`push 0xA` + 大きな char** ローカル配列群を構築）を call。
3. 続けて vt+0xB0 = **FUN_007cefd0**（スタック 0x2B2C B、`fld [0xB2C2BC]` + `mov eax,[0xB42E88]` 同様に level由来定数を読み、`push 0xC`）を **tail-call**。
4. したがって **シャットダウン経路でも level 由来の DAT_00B42E88（2/6/8）が両関数に入力される**（0x7CD0B9 / 0x7CF029 の実オペコード）。slot45 = 0x7D1170 = V25 で同定済みの第2系統 shutdown。
5. FUN_007c8710 内部にも `cmp [0xB42F48],2; jl` が1件（0x7C8763）— shutdown 本体も level>=2 で経路分岐。

---

## 6. Android/GLES3 移植方針 a/b/c 比較（実測件数ベース・推奨は留保）

前提となる実測: level 依存の分岐点は **137 参照 = 比較100件 + 定数選択習語13件 + switch 4件 + 値読み6件 + 書き10件**。到達可能な値は **0..6 の7種**（7は出荷版到達不能）。付随グローバル: b42d74(8) / b42e88(13) / b42f40(41) / b43070(55) / b42ea5(10) — 合計 **264 参照（6グローバル）**。

| 観点 | (a) level=2 固定（ps_2_0/vs_2_0 相当） | (b) GLES capabilities から level 算出 | (c) enum 廃止（グローバル削除+全参照書き換え） |
|---|---|---|---|
| 実装 | DAT_00b42f48 の初期値/検出器を level=2 に固定（b42d74=2, b42e88=6, b42f40=0x2F, b43070/ea5=0） | GLES3 → level 6（または 7）、GLES2 → level 2 など2値〜多値を capability から算出（検出器の param1/2 相当=シェーダーバージョン、param3=必須フラグのみ残す形） | DAT_00b42f48 他6グローバルごと削除し、137+127=264 参照を手書きで定数化/除去 |
| 定数化される比較 | **100 件全部**（≥2系45・≤1系26・<5系8・≥3系7・==0系6・==1系4・<3系3・<7系1・ガード4） | 2値を出す限り、**level=2 のとき 100 件中 92 件が死に**（≤1系26+==0系6+==1系4+<3系3+<5系8+≥3系7+<7系1+ガード4… の片側）、**level=7 のとき 91 件が死に**。≥2系45 は level2/7 の両方で真のため生存（=2値間で分岐するコードは残る） | 100 件すべて消去（分岐自体が存在しない） |
| switch | 4件が case2 のみに縮退（7b4590→6, 7b4780→"ps_2_0", 7b7070→"BSSM_SV_2_0", 7dab80→2） | 4件が実行時 switch のまま（使用 case は2値分のみ。level7 到達時は case7 が初めて生きる） | 4件削除 |
| 定数選択習語13件 | 全13件が level>=2 側の定数に固定（0x50→0xE4, 0x28→0x78, 0x4B→0xEB 等） | 2値分岐のまま | 13件削除 |
| 値読み6件 | 定数を返す（無害） | 生存 | 削除 |
| ps/vs 名 getter 呼出 29件（15+14） | すべて "ps_2_0"/"vs_2_0" を得る | GLES3 側が ps_3_0/vs_3_0（level7 固定時）または ps_2_b/vs_2_0（level6 固定時） | 呼出29件を直接定数へ（getter 削除可） |
| level 7 経路（到達不能だったもの） | 依然死んだまま（<7 の1件・case7 が死に码） | **level7 を出す方針なら初めて生存**（<7 の1件 + case7 + FUN_007fac40 の `DAT_00b2dcfc=8` 非設定経路）。逆に GLES2=2 で動かす端末では死に码 | 全削除 |
| shutdown 連鎖への影響 | FUN_007c8710 の level>=2 分岐(1件)が常に真、slot43/44 へは b42e88=6 が入る（元の ATI/NV 中位機と同じ値） | 端末依存値が入る（2 or 6/8）。slot43/44 のループ境界 push 0xA/0xC と組み合わせ要確認 | b42e88 を定数化、slot43/44 は定数引数化 |
| 追加作業 | ほぼゼロ（検出器の param 到達を諦めるだけ） | GLES caps → level マッピング表の設計（どの GLES 機能を param1/2/6 相当に対応させるか） | 264 参照の1件ずつ書き換え（手動・大規模。検証コスト大） |
| リスク | ps_2_0 相当固定は「ps_2_a/b パス」（=3/4/5/6 で生きる≥3系7件+<5系8件の内側）を ps_2_0 側に倒す。元の PC では ati/nv 全機が level>=3 だったため、**シェーダー実行経路の多くが低配路に固定**される | 元 EXE に 7 を出す経路は存在しないため、level7 は**新規経路**（ps_3_0/vs_3_0 マッピングは実在するが未使用）。GLES3 相当に寄せるなら level6（ps_2_b, b42e88=8）が「実機で存在した最高値」 | 破壊的。以降の RE との対応（EXE アドレス参照）が切れる |

各案の影響件数まとめ:
- **(a)** 分岐死 = 比較100 + switch4縮縮 + 習語13 = **117/137**（残り20は定数読み+検出器内部）。付随グローバル5種は全参照が定数解決（合計死数 ≒ 264 中 254）。
- **(b)** level2 と level7 の2値運用時: 各端末で **92 件（level2）/ 91 件（level7）** が死に、level 間で分岐する比較 **約 8〜9 件**（≥2系45のうち実際に両値で結果が変わるもの）+ switch4 が生き残る。
- **(c)** **264 参照すべて**が消去対象（b42f48 137 + b42d74 8 + b42e88 13 + b42f40 41 + b43070 55 + b42ea5 10）。

---

## 7. 判定できなかった点

1. **param_6 の正確なフィールド名**: `*(ushort*)(caps+0x11C)`（NiAdapterDesc 経由の GetDeviceCaps 構造体）を閾値比較するが、この構造体のレイアウトは EXE 内に型情報がなく未同定。`>0x60 && +0x114<0x20` なら 0x60 にクランプする挙動から「instruction slots 系カウンタ」の可能性があるが**推定どまり**（+0x114/+0x11C の実体未確定）。
2. **レジスタ閾値比較16件**（§4.3末尾）: `cmp [0xB42F48], reg` の reg 値は直前24byteに即値がなく解決不能（スタック引数/遠距離ロード）。代表例の一部はデコンパイルで `DAT_00b42f48 < uVar5` 形になるが、全16件の実閾値は未確定。
3. **FUN_007cd090 / FUN_007cefd0 の関数全体の意味**: 入口の実測（b42e88 読取、push 0xA/0xC、巨大ローカル配列）のみ。本体（各数千行相当）の完全解析は未実施。level→2/6/8 がループ上限/フォーマット数のどれかは未確定。
4. **FUN_00404f00 / FUN_007dab80 の全呼出元**: E8 scan 未実施（getter/variant 生成の利用箇所は未網羅）。
5. **RTTI の直接再現**: COL(0xACFE38) 直読では pTypeDesc=0 となり name が取れず（vtable-4 以外の場所に COL がある可能性）。クラス名は既存 vtable_map.txt / rtti_vtables.txt（両方 L1511 一致）で確定とした。
6. **GLES 機能レベルと 0..6 の対応の当否**: 本レポートは事実（分岐・件数）のみ提示し、どの GLES 機能を level 何に対応させるかは未判断（ユーザー決定事項）。

---
（本レポートは新規作成1ファイル。既存ドキュメント・%TEMP% 外への書込みなし。）


## 訂正（V42, 2026-10-24）: 「レジスタ閾値16件」は本編に既に内包

本稿 §4.3 末尾の「未解決: レジスタ経由の閾値16件」は**誤解**だった。デコンパイルは定数伝播済みで、
当該16件は上記ヒストグラム（**≤1系26 / 定数選択習語13 / ==0系6**）に**既に内包**されている。
漏れていたのは生バイナリスキャン段階の閾値特定のみ。V42 で16 VA を実命令VAへ補正のうえ全件解決し、
閾値由来を `mov reg,2` 即値14件（独立ロード8箇所が14cmp共有）＋`xor reg,reg` 2件と確定、
**移植3案 (a)/(b)/(c) への追加影響 = ±0件**（(b) で level 0/1 を出す新設計採時のみ最大16件が新規生存）。
詳細: `tmp/re/v42_register_thresholds.md`。

## 移植方針の決定（V43, 2026-10-24 ユーザー判断）

**案 (b) GLES caps から算出 を採用**。端末の capabilities から `FUN_007b45f0` 相当のロジックで
レベルを算出し、(a) 固定・(c) 廃止は不採用。V42 の追加調査により「レジスタ経由16件」は本編
ヒストグラムに既に内包されており、3案の追加影響は ±0 件で確定済み。移植時の実装は、
算出に用いる比較（vendor "ati"/"nv"、caps 閾値 0x60/0xFF）と対応表（§表B・ps/vs 対応）
をそのまま移植し、GLES3 実機では算出結果が単一値に収束する点を許容する。
