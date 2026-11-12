# V81: シェーダ / INI / GMST 残課題5件の閉鎖（2026-10-25）

対象: `tmp\re\TODO_re49.md` 行81 `| 81 | V81:`（V76 Part C の残り4本 + V66 残件2本）。
手法: `probe_harness.py selftest` **4/4 PASS**（bytes@0x504050 / bytes@0x4FB580 / string@0xA7035C / opid0x0015）→ raw PE 直読（.text const 0x400C00 = VA−raw 0x400）+ capstone 線形デコード検証 + `docs/Oblivion_decompiled.c`（ヘッダ 20,465 件、行→関数はヘッダ VA/行番号の二分探索）+ Ghidra 台帳 `%TEMP%\opencode\func_list_new.txt`（21,597 件）。
本ファイルが V81 の正本（docs / TODO / memory / git は一切変更しない）。生成物: `tmp\re\v81_scan.txt` 〜 `v81_scan10.txt`（%TEMP%\opencode 側スクリプト v81_scan.py〜v81_scan10.py）。

---

## 1. ① b43070 (0xB43070) のセマンティクス

### 1.1 型: byte boolean（0/1 のみ）。writer は **4 行 / 2 関数**

| 行 | 関数 | 代入 |
|---|---|---|
| L122081 | `FUN_00498340` | `DAT_00b43070 = '\0';` |
| L122085 | `FUN_00498340` | `DAT_00b43070 = '\x01';` |
| L670320 | `FUN_007b45f0` | `DAT_00b43070 = 0;`（b42f48=0 経路） |
| L670324 | `FUN_007b45f0` | `DAT_00b43070 = 0;`（b42f48=1 経路） |

- raw .text 内 `70 30 B4 00` = **55 site**、capstone 線形デコードで全 55 件が `MEM[0xb43070]` オペランド（偽陽性 **0**）。
- decompiled 参照 = **49 行 / 31 関数**（内訳は §1.5）。enum ではなく **bool**（到達値 0/1 のみ、V40 §2.2 の逆アセンブル `0x7B46C3 mov byte [0xB43070], 0` / `0x7B46DE mov byte [0xB43070], dl` (dl=0) でも定数 0 のみ）。

### 1.2 FUN_00498340 の書き込み条件 = 3 条件の AND（今回確定）

逆アセンブル 0x4987AE–0x4987C9（`docs/Oblivion_decompiled.c` L122080–122085 と 1:1 一致）:

```
0x4987AE  cmp byte ptr [0xb06de4], 0      ; L122080 第1条件
0x4987B5  mov al, byte ptr [0xb06cc4]     ; 第2条件を al へ
0x4987BA  je  0x4987d2                    ;   -> 0
0x4987BC  test al, al
0x4987BE  je  0x4987d2                    ;   -> 0
0x4987C0  cmp byte ptr [0xb350d6], 0      ; 第3条件
0x4987C7  je  0x4987d2                    ;   -> 0
0x4987C9  mov byte ptr [0xb43070], 1      ; 成立 -> 1
```

**入力3つの正体（今回 raw 読取で確定）**:

| グローバル | 正体 | 証拠 |
|---|---|---|
| `DAT_00b06de4` | INI named setting **`bDoHighDynamicRange:BlurShaderHDR`**（既定 1） | Setting 構造 {+0 値, +4 名前} として `rd(0xB06DE4)=01000000`、`[0xB06DE8]=0xA3E810` → `cstr = 'bDoHighDynamicRange:BlurShaderHDR'` |
| `DAT_00b06cc4` | INI named setting **`bDoImageSpaceEffects:Display`**（既定 1） | `rd(0xB06CC4)=01000000`、`[0xB06CC8]=0xA3E434` → `'bDoImageSpaceEffects:Display'`（base−8 の名前 0xA3E418 = `'bDoCanopyShadowPass:Display'` も確認、構造の整合性の裏付け） |
| `DAT_00b350d6` | **IDirect3D9::CheckDeviceFormat の成否** | L122071 `iVar10 = (**(code **)(*DAT_00b42154 + 0x28))(DAT_00b42154, 0, 1, 0x16, 0x80000, 3, 0x71);` → L122072 `DAT_00b350d6 = -1 < iVar10;`。vtable **+0x28 = CheckDeviceFormat**（IDirect3D9: RegisterSoftwareDevice +0x0C … CheckDeviceType +0x24 / **CheckDeviceFormat +0x28** / CheckDeviceMultiSampleType +0x2C） |

隣接の同族（同一関数内で同時に書かれる byte フラグ群、まとめて「caps ブロック」として確認）:

- L122073 同形式で `Usage=0x20000` → **L122074 `DAT_00b43071 = -1 < iVar10;`**（別フォーマット照合）
- L122075 `DAT_00b43072 = (byte)(caps+0x44 >> 10) & 1`
- L122082/L122087 `DAT_00b43073`（b43070 の補数系）
- L122188–122191: `Usage=0` の CheckDeviceFormat を **13 形式**（0x17,0x18,0x19,0x1a,0x51,0x14,0x15,0x16,0x72,0x71,0x24,0x74,0x32）で行い `(&DAT_00b42e98)[iVar9] = -1 < iVar10` = 対応フォーマット bool 表

→ **b43070 = byte bool「HDR(bDoHighDynamicRange) かつ 画像空間効果(bDoImageSpaceEffects) が INI で有効 かつ D3D9 の追加フォーマット照合(Usage 0x80000) に合格」**。V40 §2.4 の「追加シェーダーフォーマット対応フラグ = `DAT_00b06de4 && DAT_00b06cc4 && DAT_00b350d6`」を**独立に再導出できた**（V40 の結論と一致し、前駆の `b42154(IDirect3D9*) → b43070` エッジ Y7 も成立）。

### 1.3 FUN_007b45f0 側（GPU 検出）— 与件と書き込み

- **呼出し元 = E8 2件（今回全走査で確定、v76 Part C-2 の「GPU 検出呼び出し元」に回答）**:
  1. **`FUN_00498340` 0x49885B / L122098**: `FUN_007b45f0(DAT_00b06c44, DAT_00b06c48, DAT_00b06db4, iVar9 + 0x204, _Str1, uVar5);` — **実機情報パス**。`_Str1 = iVar9 + 4` = **GetAdapterDesc のデバイス名**（L122068、失敗時 `"GetAdapterDesc() failed."` L122043）、`uVar5` は L122066 `caps+0x11c` を L122094–122097 で 0x60 上限/0x20 下限調整した値（= RendererInfo.txt の `maxPS20inst` L122210 と同値）。直後 L122099 `if (DAT_00b42f48 < 5)`。
  2. **`FUN_004980D0` 0x4982BF / L121863**: `FUN_007b45f0(0, 0, DAT_00b06db4, 0, 0, 0);` — **フォールバックパス**（デバイス名・caps を全て0で渡す）。逆アセンブル 0x4982A6–0x4982C4: `[0xb06db4]` を読む以外は `[0xb06c44]`/`[0xb06c48]` を0化して6引数 push → `call 0x7b45f0` / `add esp,0x18`。両サイトとも各関数の ret+CC 内に実在（`FUN_004980D0` end 0x498338 > 0x4982BF）。
  - 参考: `FUN_007b4590` の E8 caller は `FUN_007b45f0` 内 0x7B468F（L670374 `FUN_007b4590(DAT_00b42f48);`）1件のみ = v76 Y6 と一致。
- 書き込み: **level 0 / level 1 経路でのみ 0**（L670319–670324、V40 §2.2 の実命令 `0x7B46C3` / `0x7B46DE` と一致）。**level ≥ 2 では b43070 を触らない** → FUN_00498340 が先に立てた 1 を level<2 のときだけ潰す、という合成動作。

### 1.4 読手（raw imm32 全数）と b42f48 との関係

- raw 55 site / header-bisect 40 関数、うち **46 site / 31 関数**が decompiled 本文内（= decompiled 49 行）、**9 site は本文外**（§1.5）。上位読手: `FUN_007dab80` ×5（variant コード生成、L697244–697270 で `(b42ea5, b43070, b43071)` の組合せから返り値 `\x03..\x13` を算出）、`FUN_00498340` ×3、`FUN_006a6de0` ×3、`FUN_007ff4a0` ×3、`FUN_007e7f70`/`FUN_007e8e10`/`FUN_007ed6c0`/`FUN_007edcd0` 各 ×2、他 ×1。
- **b42f48 との複合条件（4 箇所、decompiled 行で特定）**:
  - L668750 `FUN_007b2800`: `if ((DAT_00b42f48 < 2) || (DAT_00b43070 != '\0'))`
  - L680532 `FUN_007c3240`: 同型
  - L707959 `FUN_007e8e10`: `if ((DAT_00b42f48 < 2) || (DAT_00b43070 == '\0'))`
  - L732510 `FUN_008129f0`: 同型
  → **レベル < 2（PS1.x 系）なら b43070 を問わず通過、レベル ≥ 2 では b43070 が判定主体**という役割分担。
- `FUN_007b4290`（28 スロット遅延生成）**case 8 で `DAT_00b43070 != 0` をゲート**（L670157、Y5 成立）、case 7 は `DAT_00b42f48 < 2`（L670148）。
- 例: `FUN_007b18c0` L668005 `if (DAT_00b43070 == '\0') { … sepia.dds … }`、`FUN_007dab80` の variant は `(level, b42ea5, b43070, b43071) → 0..19`（V40 §5 L226 と本調査の読手一覧が一致）。

### 1.5 raw 55 / decompiled 49 の差の分解（新規）

| 区分 | site 数 | 内容 |
|---|---|---|
| decompiled 本文内 | 46 | 31 関数（decompiled は 49 行） |
| **無名 gap 関数内** | 6 | 0x405588（`FUN_004053e0` ret 0x405439 の後）、0x40E922（**gap [0x40DF74, 0x40F640) = 0x16CC B**、§3 参照）、0x6A6AA2 / 0x6A6D39（各 ret+CC の後）、0x7B1513（**0x7B14BE で始まる無名関数**、`FUN_007b13a0` は ret 0x7B14BA+3CC で終了）、0x7C53B2（**0x7C53B0 で始まる無名関数**、`FUN_007c5340` は `ret 8` @0x7C53AB で終了） |
| **台帳にあり decompiled に無い関数** | 3 | 0x503CE2 → 台帳 `FUN_00503ce0`、0x506AD2 → `FUN_00506ad0`、0x5088DF → `FUN_00508860`（いずれも dec ヘッダ欠落） |

逆に decompiled 行が raw site より1多い関数が3件（`FUN_007ed6c0` 2/3、`FUN_007edcd0` 2/3、`FUN_00848c40` 1/2）→ 単一命令の複数行展開と推定（§7 残点）。

**判定: b43070 = byte bool、writer は FUN_00498340（INI 2 条件 + CheckDeviceFormat）と FUN_007b45f0（level<2 でクリア）の2系統のみ、読手 55 site は全て実メモリ参照。意味論は「HDR + 画像空間効果 + 追加フォーマット対応 && シェーダレベル≥2」= 拡張ポストプロセス系シェーダーバリアント可用。[確定]**

---

## 2. ② b42f48 / FUN_007b7250 teardown 経路

### 2.1 呼出しは **E8 = 1 件のみ**（E9=0、imm32=0）

- raw E8 → 0x7B7250 = **1 site: 0xA10EB5**（capstone 検証済: `call 0x7b7250`）。E9 tail jmp → 0 件。`7B 72 7B 00` の絶対参照は **.text/.rdata/.data 全て 0 件**（関数ポインタ登録も無し）。
- decompiled 側の `FUN_007b7250(` 出現は**定義行 L671878 のみ**（= この関数はどの decompiled 本文からも呼ばれていなかった）。

### 2.2 唯一の呼出し元 = **無名の CRT 初期化スタブ 0xA10EB0**

```
0xA10EA4..0xA10EAF  int3 ×12                 ; FUN_00a10e50 は ret @0xA10EA3 で終了
0xA10EB0  mov ecx, 0xb430d8
0xA10EB5  call 0x7b7250                      ; ★唯一の呼出し
0xA10EBA  push 0xa27150
0xA10EBF  call 0x981fb4                      ; atexit 相当（0x981FB4: push [esp+4]; call 0x981f78 … ret）
0xA10EC4  pop ecx
0xA10EC5  ret
```

- 台帳: `prev=FUN_00a10e50 (0xA10E50) / next=FUN_00a12380 (0xA12380)` → **0xA10EB0 の関数は台帳 21,597 件にも無い無名 gap 関数**（ret+int3 境界規則の新实例）。decompiled の `FUN_00a10e50` 本文は L1038642–1038658 の16行で完結し、このスタブを含まない。
- **登録先テーブル**: .rdata のコードポインタ連続 run **0xA2ED04..0xA2F1B0（299 エントリ、全て 0xA10000–0xA12000 の小関数）** のうち **0xA2EFF0 = 0xA10EB0**。
- **隣接エントリも atexit 登録スタブ**（0xA10F00 / 0xA10F10 を確認、0x10 刻み）: `0xA10F00: push 0xa271f0 / call 0x981fb4 / pop ecx / ret`、`0xA10F10: push 0xa27220 / …` → このテーブルは **CRT 静的初期化テーブル（.CRT$XCU 相当）**と判断。よって **FUN_007b7250 はプロセス起動時（main より前）に1回だけ実行**される。
- **登録された atexit ハンドラ 0xA27150 は空関数**: `0xA27150: ret` のみ（直前は int3）。対照的に同じ帯の `0xA27120` は実処理を持つ: `push 0xb2c35c / mov ecx, 0xb07cfc / call 0x65c620（マップ erase）/ mov eax,[0xb2c360] / cmp byte[eax],0x53 / je / free`。

### 2.3 FUN_007b7250 の本体（L671874–672076、203 行）

- **b42ec0 〜 b42f2c を全ゼロ**（L671885–671912 = **28 スロット全部**。タスク前提の「b42ec0..b42ed8」は部分集合で、実際は v76 Hub3 の「28 スロット遅延確保表」全域）。
- 既定値セット: `DAT_00b42f40 = 0x2f`(L671920)、**`DAT_00b42f48 = 0`(L671929)**、`DAT_00b42d74 = 0`(L671930)、`DAT_00b42e87 = 1`(L671931)、`_DAT_00b42d70 = &LAB_007b4150`(L671921)、`DAT_00b42ea5 = 0`(L672071) 等。
- **COM singleton の refcount リリース**: `DAT_00b430ec`(L671933–671939) を筆頭に b430dc / b430f0 / b430e0 / b43100 / b430e4 / b4311c / b43120 / b430f8 / b43110 / b430d4 / b43114 / b430fc / b43128 / b43124 … を `InterlockedDecrement → 0 なら vtable+0 呼び出し → ポインタ = 0`。
- `param_1` は本文中未使用（末尾 `return param_1;` L672072 のみ）→ `mov ecx, 0xb430d8` の値は結果的に捨てられる（Ghidra の `__fastcall` 表記はこの引数を拾ったもの）。
- **b43070 は本体中に出現しない**（writer 4 行にこの関数は含まれない、§1.1 で確認）。

### 2.4 生命周期判定

| 観点 | 結果 |
|---|---|
| 実行時に呼ぶ経路 | **無し**（E8 1 件 = CRT 初期化スタブのみ、E9 0、imm32 ポインタ登録 0） |
| 起動時 | **1回実行**（.rdata 初期化テーブル 0xA2EFF0 経由）→ この時点では singleton は全て NULL なので release 系は no-op = **事実上の「リセット/初期化」** |
| 終了時 | 登録先ハンドラ 0xA27150 は **`ret` のみ = 空** → 終了時 teardown は行われない |
| 既知の別経路 | （参考）V80 で `0x7B8530` = シェーダ系シングルトン一括リセットと同定済み（E8 は `FUN_00497B20` 1 件）— 本タスク範囲外 |

**判定: FUN_007b7250 の呼出し元は「無名 CRT 初期化スタブ 0xA10EB0（.rdata 初期化テーブル 0xA2EFF0 エントリ）」1件のみで、実行時 teardown 経路は存在しない。名は teardown だが実体は「起動時の既定化リセット」。[確定]**（ただし初期化テーブル自体の走査コードは §7 残点）

---

## 3. ③ GMST マップ 377/382 の寿命側検証

### 3.1 FUN_004055e0 の実体（逆アセンブル 0x4055E0–0x40564B、L4267–4293 と一致）

```
0x405602  mov esi, ecx              ; esi = this = Setting*
0x405608  test esi, esi / je
0x405614  mov eax, [esi+4]          ; name
0x405617  test eax, eax / je
0x40561B  push eax                  ; key = name
0x40561C  mov ecx, 0xb35574         ; ★中央 GMST レジストリ
0x405621  call 0x4524c0             ; map.erase(name)
0x405626  mov esi, [esi+4]
0x40562D  cmp byte [esi], 0x53      ; 'S'
0x405630  jne skip
0x405632  push esi / call 0x401f20  ; free(name)
0x40564B  ret
```

- `FUN_004524c0` = **map::erase 本体**（L63085–63118: `vtable+4` で hash → `vtable+8` で compare → 先頭/連結の unlink → `vtable+0x10/+0x18` で node free → `param_1[3]--`）。E8 caller **76 件**。
- **FUN_004055e0 の E8 caller = 1 件（0x40C51C = FUN_0040c350 内）**。

### 3.2 FUN_0040c350 = 「動的 Setting* 配列 9 本」の一括破棄

decompiled L7859–8003（9 ループ、それぞれに `*pcVar2 == 'S'` 検査が1つずつ = 全8 inline + FUN_004055e0 経由1):

| 配列基址 | 上限 | スロット数 | 備考 |
|---|---|---|---|
| `DAT_00b39548` | `< 0xC` | 3 | FUN_004055e0 経由（L7863） |
| `DAT_00b39578` | `< 0x3B8` | **238** | 逆アセンブル 0x40C581 `cmp edi,0x3b8` で確認 |
| `DAT_00b39554` | `< 0x24` | 9 | |
| `DAT_00b39930` | `< 0x20` | 8 | |
| `DAT_00b399D0` | `< 0x74` | 29 | |
| `DAT_00b39A44` | `< 0x18` | 6 | |
| `DAT_00b39A60` | `< 0x50` | 20 | |
| `DAT_00b39950` | `< 0x7C` | 31 | |
| `DAT_00b39530` | `< 0x18` | 6 | |
| **合計** | | **350** | 各要素: erase → `cmp 'S'` → free name → free Setting |

- 各ループの erase はすべて `push name / mov ecx,0xb35574 / call 0x4524c0`（例 0x40C54E, 0x40C5AA）。
- **0xB35574 に対する `mov ecx, 0xb35574` は全 17 サイト**: erase **9**（FUN_004055e0 ×1 + FUN_0040c350 ×8）/ insert（registrar `FUN_00419A70` ×2, `FUN_0041BAE0` ×2）/ find（`FUN_00513E90` ×2）/ 手動 walk（`FUN_00575B40` ×2）。→ **破壊系は FUN_004055e0 と FUN_0040c350 のみ**（v76 の "eraser" 主張を全数で裏付け）。
- **FUN_0040c350 の E8 caller = 1 件 0x40F580** — ただしこのサイトは `FUN_0040d4d0` の decompiled 本文（L8522–8662 の141行、末尾 `return`）の**外側**であり、台帳にも **[0x40DF74, 0x40F640) の 0x16CC バイトに関数0件** → **無名 gap 関数が唯一の呼出し元**（header-bisect は fun_0040d4d0 に誤帰属させる）。
- 参考: `FUN_0040d4d0` 自体の E8 caller は5件（自己 0x40DF54 / `FUN_004455e0` 0x4456B3 / `FUN_0044E720` 0x44EB24 / gap 0x50D567 / `FUN_005BDCD0` 0x5BDD53）で、V80 の「`0xB42E8C` 既定 callback を FUN_0040D4D0 が set」= **初期化/リセット系関数**という既存同定と整合。

### 3.3 `'S'` プレフィックスの意味（今回確定）

- **静的登録名に `S` 始まりは0件**: B コレクション登録パターン `push <Setting*>; mov ecx,0xb07bf0` を全走査 → 606 サイト中587件で名前解決、先頭文字ヒストグラム = **b:185 / f:227 / i:103 / r:1 / s:50 / u:21、`S`: 0**。
- **`S` の書き手は `FUN_004a7980` のみ**（decompiled 内 `'S'` 出現は計12行、うち書き込みは L133740 の1行）:
  - L133722 ヒープ確保 → L133734–133739 旧文字列をコピー → **L133740 `*pcVar3 = 'S';`**（先頭バイトを 'S' に上書き）→ **L133741–133743 `if (*param_1[1] == 'S') free(param_1[1]);`** → L133744 ポインタ差し替え。
  - L133740 と L133741 が対で現れる =「新しくヒープ所有した文字列は 'S' でマークし、旧文字列も 'S' のときだけ free」。
- `FUN_004a7a80` L133778–133781 `switch (*(param_1[1])) { case 0x53: case 0x73: …}` = **先頭バイトは型タグ**（'S' と 's' はともに文字列型）。
- → **`cmp byte [reg], 'S'` は「エンジンがヒープ所有する文字列かどうか」の所有権マーカ判定**。f/i/s/b/u/r 始まりの静的 .rdata 名前は **free されない**（`= 対象外`）。
- `cmp byte [eax],'S'` の全 .text 出現は **647 件**（`FUN_0040c350` 内8、`FUN_004055e0` は `cmp byte [esi],'S'` 1 件のみ = 0x40562D、他は `FUN_00A16420`/`FUN_00A166D0` 系の連続 teardown 帯・`FUN_00403B80`・`FUN_004A7980` 等）。

### 3.4 「377/382」と「5件 ESM-only」の出所と再検証

- **リポジトリ内に `377` の一次資料は存在しない**（`rg "377"` を docs/tmp\re 全 md/txt に走査、GMST/382/登録 の文脈ヒットは `v76_data_relationship_map.md` Part C-5 と `TODO_re49.md` 行81 の**主張側のみ**）。
- **一次資料は Supermemory エントリ `6270a504e69619e8581808`**（mem-only）:
  > 「**包含検証 377/382 が .rdata文字列+push で EXE 登録を確認**、**未検出5件** = `fRegionGenTexPlacedNotMatch` / `PlacedMatch` / `GenNotMatch` / `GenMatch`（**.rdata に文字列自体なし=ESM専用キー→V55経路なら上書き棄却**）＋ `sSpecialization`（文字列あり・直接pushなし=間接登録の可能性）＝**V60**」
  - 旁証エントリ `LKVaUgjXaRENZzBpFrjSnT`: "Five ESM GMST keys are not present in the executable: … and sSpecialization."
- **リポジトリ側の対応成果物 `s50_gmst.md`**:
  - L245「**名前一致 378 / 382**（型別: f 239/243、i 31/31、s 108/108）。EXE 未登録の 4 件は `fRegionGenTex*`」
  - L146–147「ESM にのみ存在する名前は **4 件**… これらの文字列は **EXE 内に一切存在しない**（`fRegionGenTex` バイト走査 0 件）ため、隠れた第 3 の registrar は存在しない **[確定]**」
  - L33「382 レコード中 378 が EXE 名と一致、226 件が値相違」
- **本セッションの独立確認（ESM 277MB は再走査せず、EXE 側のみ）**:
  - `fRegionGenTex` / `fRegionGenTexGenMatch` → **EXE 内0 件**（s50 の主張を再現）✓
  - `sSpecialization` → EXE 内 **2 箇所**（VA 0xA5D12D: 直前 byte 0x73 の長文字列の一部 / VA 0xA5DD64: 独立な null 終端文字列）だが、**この VA を指す絶対ポインタは全セクション0 件** → 直接登録（push / lea）は未確認。
- → **377 と 378 の差 = `sSpecialization` の1件**（「.rdata 文字列 + 直接 push」基準 vs「文字列一致」基準の差）。基準の異なる2つの数値は両立する。

### 3.5 寿命判定（377 を erase が維持するか / 5件は dead か）

- **ESM 上書き経路（`v55_gmst_override_path.md` [完了]）**: ESM GMST ハンドラ `0x44E326` → manager `0xB35468` slot9 `0x4A8040` が EDID を **`0xB35574` へ lookup のみ（insert なし）**、命中時 `0x4A7D60` が DATA を**既存変数オブジェクトへ4バイト転送**。insert `0x412D30` は汎用メソッド47呼出元だが **本マップへの runtime 経路0件**。
- したがって:
  1. **登録済み（377〜378）**: 静的スロット（`s50_gmst.md` §3: 静的スロット領域 0xB33460–0xB3B7C8 の4206セルのうち登録済み GMST **2056**）と map キー2,175件に存在し、**`FUN_0040c350` が歩く9本の動的配列（350 スロット）には1件も入らない** → **この erase 経路では一切消されない（= 377 系は保持される）**。`'S'` 判定も静的名前には発火しない（§3.3）ので名前の free も無い。
  2. **ESM 専用5件**: lookup が必ずミスし、insert 経路も runtime に無い → **マップに一度も入らない = never-erased だがその理由は「そもそも未登録」= dead**。ESM 側の DATA 値も破棄される。4件（`fRegionGenTex*`）は文字列自体が EXE に無いため決定的。`sSpecialization` は文字列のみ存在（絶対ポインタ0件）で**登録未確認 = 377/378 差分の残点**。

**判定: 『`'S'`-prefix erase が377を維持する』は成立（erase は動的350スロットのみ・静的分は経路外）。5件 ESM-only は never-erased だが真因は「lookup-only・insert 経路なしで未登録（dead）」。ただし 377 という数値の一次資料はリポジトリに無く mem-only、リポジトリ基準では 378/4。[確定（機構）／ 377 の数値出所は mem-only のまま]**

---

## 4. ④ FUN_007b4010 の埋め込みマップ owner（`this = *(param_4+8)` の基点）

### 4.1 呼出しは2件（今回 E8 全走査で判明、既存記載の1件から増加）

| site | 所属（header-bisect → 実体） | 検証 |
|---|---|---|
| 0x4BAC51 | `FUN_004baa80` | decompiled L146275 に実在 |
| 0x437658 | header-bisect=`FUN_00437590` → **実体は無名関数 0x437600** | `FUN_00437590` は `ret` @0x4375FE + int3 で終了、0x437600 に thiscall prologue（`sub esp,0xc; push esi; mov esi,ecx`）。台帳 `prev=FUN_00437590 / next=FUN_004376a0` → 無名 gap |

（decompiled を探しても L146275 と定義行 L669963 のみ — 0x437658 側は無名関数のため decompiled に無い。）

### 4.2 `*(param_4+8)` の供給元（主経路 FUN_004baa80）

`FUN_004baa80` 本文 L146207–146285 より、渡り物の構造体は **スタック上12バイト**:

```
L146271  FUN_007b20b0();
L146272  uStack_220 = *(undefined4 *)(param_1 + 0xc);   ; ← 構造体 +8
L146273  local_228 = 0;                                 ; ← 構造体 +0
L146274  puStack_224 = puVar4;                          ; ← 構造体 +4
L146275  FUN_007b4010(param_6, param_7, param_8, &local_228, param_2, local_22c, param_5);
```

- `param_4 = &local_228`（デコンパイラの `local_228` は宣言 L146221、0 代入 L146273、アドレス渡し L146275 の3点のみ）。
- **`*(param_4+8)` = `uStack_220` = `*(FUN_004baa80::this + 0x0C)`**（`param_1` は `__thiscall` の this）。
- 同一本文 L146244 `FUN_007b2a00(*(undefined4 *)(param_1 + 0xc))` = **同じポインタがシェーダ系述語にも渡される**。

### 4.3 `this` の正体 = **TESObjectTREE** のバーチャルメソッド

- `0x4BAA80` の絶対参照は .rdata **1 件のみ `0xA450D0`**（= vtable スロット）。E8 直接呼び出し **0 件** → バーチャルディスパッチのみ。
- vtable 逆引き: 0xA450D0 から前方向にコードポインタ run を辿った基点 **0xA44FB4**、その直前 `dword[0xA44FB0] = 0xABBB28` = **Complete Object Locator**。COL 構造 `{sig=0, offset=0, pTypeDescriptor=0xB051B0, pCDN=0xABBB3C}`、TypeDescriptor `0xB051B0` = **`.?AVTESObjectTREE@@`**（`.rdata` に1件、raw 検証済）。
- 既存成果物と一致: `tmp\re\rtti_vtables.txt:444` / `tmp\re\vtable_map.txt:444` = `0x00A44FB4 93 .?AVTESObjectTREE@@ TESObjectTREE`。
- → **スロット index = (0xA450D0 − 0xA44FB4)/4 = 71（+0x11C）= TESObjectTREE の仮想関数 #71**。

### 4.4 第2経路（無名関数 0x437600）も同じ「+0x0C マップポインタ」

```
0x437604  mov esi, ecx                 ; esi = this (thiscall)
0x437606  mov eax, [esi+0x30]
0x437609  mov ecx, [eax]               ; -> FUN_004ba780(this=*[this+0x30], 1)
0x437622  mov eax, [esi+0x30]
0x437625  mov ecx, [eax]
0x437627  mov edx, [ecx+0xc]           ; ★ edx = *(*([this+0x30]) + 0x0C)
0x43762A  mov [esp+0x10], edx          ; 構造体 +8 に格納
0x43762E  mov [esp+0xc], edi           ; +4
0x437632  mov [esp+8], 0               ; +0
0x437647  mov ecx, [eax+0xc]           ; param_3 にも同値
0x43764A  lea edx, [esp+0x14]          ; param_4 = &構造体
0x437658  call 0x7b4010
```

→ 2経路とも **「オブジェクトのフィールド +0x0C に格納されたポインタ」がマップの `this`**。

### 4.5 マップの使い方（FUN_007b4010 本文 L669961–670029）

- L669988 `FUN_0055e000(*(undefined4 *)(param_4 + 8), &param_7);` = **map find**（find 実体 0x55E000、v76 Hub2 の `find = 0x55e000` と一致）。直後 L669989 `piVar3 = *(param_7 + 0x24)` = ノード連結（v76 の node 構造 `{next@0, key@4, value@8}` + bucket +0x24 と整合）。
- L669986 `(**(code **)(*param_3 + 0x78))()`、L669980 `DAT_00b42d60 += param_7 & 0xffff`、L669907 `FUN_007b3be0(param_4, param_1, &param_7, param_3)` → **シェーダ/テクスチャ束縛系**（`FUN_007b4010` の残り2つの呼出し元も同系統: `FUN_007b20b0` / `FUN_007b2a00` / `FUN_004ba780`）。
- **v76 N2 判定（GMST マップ ↔ シェーダ init = NO）を裏付け**: このマップはグローバル `0xB35574` ではなく、TESObjectTREE（または `*[this+0x30]`）の **+0x0C が指すオブジェクトに埋め込まれた同一クラスのインスタンス**（vptr+0 / bucket +8、vtable 0xA40454 系）。

**判定: `*(param_4+8)` の基点は「TESObjectTREE vtable#71 レシーバ（FUN_004baa80 の this）のフィールド +0x0C」（第2呼出しは `*(*(this+0x30)+0x0C)`）。そこが指す同一クラスの map インスタンスが FUN_0055e000 の this。[確定（経路）] ただし +0x0C の指す先オブジェクトのクラス名は未特定**

---

## 5. ⑤ V66 残件: 0x66C580 の呼出し差 + EVMP 一次資料調査

### 5.1 呼出しの全数（EXE raw、capstone 検証）

- **E8 → 0x66C580 = 22 site（22/22 が実 `call` 命令と線形デコードで検証済、偽陽性0）**。
- **E9 → 0x66C580 = 2 site**: `0x6331AE`（owner `FUN_00633130`）、`0x63323E`（owner `FUN_006331c0`）= **tail jmp**（`jmp 0x66c580`、直後に `ret 4`）。
- decompiled 側: `FUN_0066c580(` 出現は定義行 L431340 を除き **21 行 / 15 呼出し元関数**（v66_v68_findings.md の「21 行 / 16 関数」の16は定義行込みで、実呼出し元は15）。

### 5.2 差の完全分解（「1件差」の正体）

| 内訳 | 件数 | 内容 |
|---|---|---|
| EXE E8 にあって decompiled に無い | **3 site / 2 関数** | `0x656F80`, `0x656FAF` → **無名 gap 関数 A = [0x656C82, 0x657340)**（`FUN_006564c0` は ret 0x656C7F、台帳は次の `FUN_00657340` まで0件）<br>`0x6710DA` → **無名 gap 関数 B = [0x6710A5, 0x671170)**（`FUN_00670f50` は ret+CC 0x6710A4、次は台帳 `FUN_00671170`） |
| decompiled にあって E8 に無い | **2 site / 2 関数** | `FUN_00633130` L399314 `FUN_0066c580();` ← **E9 tail jmp 0x6331AE**<br>`FUN_006331c0` L399341 `FUN_0066c580();` ← **E9 tail jmp 0x63323E** |
| 純一致 | 19 site | 残り19 E8 は decompiled 行と1:1 |

検算: 22（E8）− 3（gap のため decompiled に無い）+ 2（E9 は E8 走査に含まれない）= **21 = decompiled 行数** ✓
関数ベース: raw owners **17** − gap 2 = 15 = decompiled 呼出し元15 ✓

**gap 関数内コードの実在確認（逆アセンブル）**:
```
0x656F80: call 0x66c580
0x656F85: mov ecx, dword ptr [0xb333c4]
0x656F8B: cmp byte ptr [ecx+0x588], 0     ; +0x588 byte フラグ（V66-2 の本体と同一）
0x656F92: je 0x656fa7 …
0x6710DA: call 0x66c580
0x6710DF: cmp byte ptr [esi+0x589], 0
0x6710E6: je 0x6710ef
0x6710E8: mov ecx, esi / call 0x66c600    ; 近接フラグ群
```

- **「欠けた呼出し元」は1つではなく2つの無名 gap 関数**（合計3サイト）。元の「21 vs 22 の1件差」は、gap 由来の3件と E9 tail-jmp 由来の−2件が相殺した**純粋なネット差**だった。
- v66_v68_findings.md の Residual uncertainty「decompiled 21 vs EXE 22 の呼出件数1件差（デコンパイラの統合・重複展開いずれかの可能性。未特定）」→ **[解消] 統合でも重複展開でもなく、(a) 無名 gap 関数に呼出し3件（decompiled 非収録）、(b) tail jmp 2件（E8 非計上）**。

### 5.3 EVMP 正体の一次資料調査（plan / s69_scda* / tmp\re 横断）

**リポジトリ rg（`rg -i "evmp"`、docs + tmp 全ファイル）** — 一次資料 **0 件**。出現はすべて矛盾追跡・引継ぎ文書:

| ファイル | 件数 | 性質 |
|---|---|---|
| `docs/RE_EXE_REPORT.md` | 4（L11876, L11951 ほか） | 「「EVMP 516」は Supermemory の外部ノートにのみ由来し、リポジトリ成果物に裏付けがなく、実測は **532**。以後は根拠として扱わない」 |
| `docs/RE_ANALYSIS_ROADMAP.md` | 1 | 同上の引用 |
| `tmp\re\TODO_re49.md` | 2（L254 V68 行 / L266 本行） | 主張側 |
| `tmp\re\v66_v68_findings.md` | 9 | V68 の調査記録（本件の前駆） |
| `tmp\re\memory_contradictions_20261024.md` / `memory_structure_map_verify_20261024.md` | 1 / 3 | 自ら「出所未確認(mem-only)」と自認 |
| `tmp\supermemory_diff\sm_documents.json` / `diag2.txt` | 1 / 1 | メモリダンプ |
| 各 `.bak` / `.pre*` | 計20超 | 上記の複製 |

**`plan.md`**: `rg -i "evmp"` → **0 件（exit 1）**。`rg -i "evmp|516"` → 8 件だが全て `516` の部分一致（アドレス等の偶発、EVMP 無関係）。
**`s69_scda*`**（`s69_scda_a.md`, `s69_scda_b.md`, `s69_scda_a_01/02/03`, `s69_scda_b_1_probe.txt`, `s69_scda_b_2_handlers.txt` ほか）: `rg -i "evmp"` → **0 件（exit 1）**（= V68 の「引用元 `s69_scda_b_2_handlers.txt` に 516/158/358 の計数記述0件」を再確認）。

**supermemory 検索（`"EVMP VM table 516"`, scope=project, 1回）** →5件:
1. `8UC2xGaoFU4e8dD94K5cw9` — 既往ノート「「EVMP 516」は Supermemory の外部ノートにのみ由来…実測は 532」
2. `98v4sWUmZjfQcwJbACFXLG` — **"MISS denominators verified: EXE table 532, 531, 527, 495, 480, 516, 368."** ← 516 を含む唯一の载体（V68 の分母系列と同一）
3. `6GkErhgC8rtfwF7cArzCs1` — "FunctionID **Evp** is mapped to opcode 0x105E."
4. `W3nw6zhipcUrRX41ihe7Gb` — "**Evp** function address is 0x50AF90."
5. `fb1830f242c81add3015b2` — V18 の indirect call 調査（480-opid 台帳、EVMP 記述なし）

- **`Evp`（opcode 0x105E / 0x50AF90）は別語**で EVMP の完全一致語ではない（語源候補として記録、因果は未証）。

**判定: EVMP = メモリ（Supermemory）のみに存在する語で、リポジトリ（docs / tmp\re / plan.md / s69_scda* / ダンプ以外）に一次資料は0件。EXE 実測のオペコード表サイズは532で、516 は"分母系列"エントリ `98v4sWUm…` 経由の未検証値。→ EVMP の正体は undeterminable（ツール名/セッション名/誤記のいずれか判別不能）。[未解決（判別不能と確定）]**

---

## 6. 判定まとめ

| # | 項目 | 判定 | 主要証拠 | 状態 |
|---|---|---|---|---|
| ① | b43070 semantics | **byte bool = `bDoHighDynamicRange:BlurShaderHDR` && `bDoImageSpaceEffects:Display` && `IDirect3D9::CheckDeviceFormat(+0x28, Usage 0x80000)` 成功**、さらに `FUN_007b45f0` が level<2 で0に潰す → 実効は「HDR+画像空間画像空間効果+追加フォーマット対応 && シェーダレベル≥2」。writer 4行/2関数、reader raw 55 site/40関数（decompiled 49行/31関数）。b42f48 とは `(b42f48<2) \|\| (b43070!=0)` の4箇所複合 + variant 算出 | **確定** |
| ② | b42f48 / FUN_007b7250 teardown 経路 | E8=1件のみ（0xA10EB5）= **無名 CRT 初期化スタブ 0xA10EB0**（.rdata 初期化テーブル 0xA2ED04..0xA2F1B0 の 0xA2EFF0 エントリ、隣接は全て atexit 登録スタブ）。起動時1回の**既定化リセット**であり実行時 teardown は不存在（E9=0 / imm32=0）。登録 atexit ハンドラ 0xA27150 は `ret` の空関数。本体は b42ec0..**b42f2c 28スロット全消去** + b430ec 以下 singleton release | **確定** |
| ③ | GMST 377/382 寿命 | `FUN_004055e0` = `0xB35574.erase(name)` + `name[0]=='S'` のときのみ free（E8 caller は FUN_0040c350 の1件のみ）。`FUN_0040c350` は**動的 Setting* 配列9本=350スロットのみ**を erase → **静的登録377/378件は経路外で保持**。`'S'` はヒープ所有マーク（静的名先頭ヒストグラム587件中 'S'=0、書手は FUN_004a7980 L133740）。5件 ESM-only は V55 の lookup-only・insert 経路0件により**未登録=dead** | **確定（機構）／377 の数値は mem-only（リポジトリ基準は 378/4）** |
| ④ | FUN_007b4010 埋込 map owner | `*(param_4+8)` = **TESObjectTREE（vtable 0xA44FB4、slot#71 = 0xA450D0）のレシーバ +0x0C**（L146272 `uStack_220 = *(param_1+0xc)`）。第2呼出し（無名関数 0x437600）は `*(*(this+0x30)+0x0C)`。いずれも「+0x0C の指す同一クラス map インスタンス（bucket +8）」で find = `FUN_0055e000`（L669988）→ v76 N2（GMST グローバル非参照）を裏付け | **確定（経路）／+0x0C 先のクラス名は未特定** |
| ⑤a | V66 0x66C580 呼出し差 | EXE E8=**22（全検証済）** + E9=**2（tail jmp）**、decompiled=**21行/15呼出し元**。差は「無名 gap 関数2つ（3サイト: 0x656F80/0x656FAF/0x6710DA）が decompiled 未収録」−「E9 tail-jmp 2件が E8 非計上」の相殺 → **欠落呼出し元は1つではなく2つ** | **確定（解消）** |
| ⑤b | EVMP 一次資料 | `plan.md` 0件 / `s69_scda*` 0件 / docs・tmp\re の出現は全て矛盾追跡文書。supermemory 5件中、516 の载体は `98v4sWUm…`（分母系列）、`Evp`(0x105E/0x50AF90) は別語。EXE 実測 532 | **未解決（undeterminable と確定）** |

---

## 7. 未解決の残点

1. **CRT 初期化テーブルの走査コード**: .rdata run `0xA2ED04..0xA2F1B0`（299 エントリ）の**絶対アドレス参照は全セクションで0件**（run 端点・中央エントリ・末尾直後すべて試行、`va−0x401000` 等の相対形も0件）。走査側は相対算出か PE セクション境界経由と推定。**[未特定]**
2. **b43070 の decompiled 行数過剰3件**: `FUN_007ed6c0`(raw2/decomp3)、`FUN_007edcd0`(2/3)、`FUN_00848c40`(1/2) — 単一機械命令の複数行展開（goto/ラベル重複）と推定、命令単位での較正は未実施。**[残]**
3. **`sSpecialization` の登録有無**: 文字列は .rdata に2箇所（0xA5D12D / 0xA5DD64）だが絶対ポインタ参照0件。377（直接 push 基準）と 378（文字列一致基準）の差そのもの。間接登録の有無は未判定。**[未特定]**
4. **`FUN_0040c350` 唯一の呼出し元（無名 gap [0x40DF74, 0x40F640) の関数）の呼び出し元・到達時期**: 起動時かリロード時か（`FUN_0040d4d0` 自身は `FUN_004455E0`/`FUN_0044E720`(ESM ディスパッチ帯)/`FUN_005BDCD0` から呼ばれ、初期化/リセット系と整合するが、gap 関数側の caller は無名のため未追跡）。**[未特定]**
5. **`TESObjectTREE +0x0C` が指すオブジェクトのクラス/用途**、および vtable slot#71 の実行時レシーバ（E8 直呼び0件、`call [reg+0x11C]` 形のディスパッチ元は未走査）。**[未特定]**
6. **シェーダテーブル (b42ec0) の実行時再初期化経路**: FUN_007b7250 に実行時呼出しは無いが、V80 の `0x7B8530`（シングルトン一括リセット）等との役割分担は本タスク範囲外。**[調査外]**
7. **無名 gap 関数の新实例**（台帳 21,597 件にも不在）: `0xA10EB0`（FUN_007b7250 呼出し）、`0x40F5xx`（FUN_0040c350 呼出し）、`0x437600`（FUN_007b4010 第2呼出し）、`0x656C82..`（FUN_0066c580 ×2）、`0x6710A5..`（FUN_0066c580 ×1）、`0x7B14BE`・`0x7C53B0`・`0x40543A..`・`0x6A6A9B..`（b43070 読手）。ret+1..3×int3 の細かい境界では既存の "ret+4×CC" 規則（V85/V96）が取り逃すため、台帳との突合が必要。**[記録のみ]**
8. **EVMP 正体**: メモリ外に判別材料なし（§5.3）。**[undeterminable]**

---

検証日: 2026-10-25。ハーネス: `probe_harness.py selftest` 4/4 PASS。本ファイルが V81 の正本（`TODO_re49.md` / `docs/` は未変更）。
