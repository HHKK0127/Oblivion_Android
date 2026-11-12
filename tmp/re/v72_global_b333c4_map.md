# V72 — `DAT_00b333c4` (0x00B333C4) フィールド地図・一次資料

- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe` (Oblivion.exe v1.2.0416, Steam版, 7,898,624 bytes)
- decompiled: `docs\Oblivion_decompiled.c` (1,066,496 行)
- 作成日: 2026-10-24 / スクリプト・中間証跡は `%TEMP%\opencode\` のみ（既存ドキュメント無編集）
- 使用スクリプト（すべて新規・このタスク専用）:
  `v72_selftest.py` / `v72_dec_scan.py`(+`v72_dec_refs.txt`) / `v72_line2func.py` /
  `v72_bin_scan.py`(+`v72_bin_refs.txt`) / `v72_analyze.py`(+`v72_dec_analysis.txt`) / `v72_disasm.py`
- PE は `struct` 自前解析（DOS→COFF→OptionalHeader→SectionTable）。capstone 5.0.7。

---

## 1. セルフテスト 4 件 — **ALL PASS (4/4)**

| # | 検査 | got | exp | 結果 |
|---|------|-----|-----|------|
| 1 | `bytes@VA 0x504050` (8B) | `518b4c24188b5424` | `518b4c24188b5424` | PASS |
| 2 | `bytes@VA 0x4FB580` (4B) | `578b7908` | `578b7908` | PASS |
| 3 | `string@VA 0xA7035C` | `.\AI\CombatController.cpp` | 同左 | PASS |
| 4 | テーブル (rooted `0x00B0AF48`, stride 0x28, id@+8, impl@+0x18) opid `0x0015` ('OnLoad') impl | `00504560` | `00504560` | PASS |

セクション表（自前解析）: `.text` VA=0x401000 Raw=0x400 Size=0x626E00 / `.rdata` VA=0xA28000 Raw=0x627200 /
`.data` VA=0xB02000 VSize=0xA9C1C Raw=0x30C00（＝file-backed は 0xB02000–0xB32C00、**以降 bss**、.data 仮想末 0xBABC1C）/ ImageBase=0x400000 SizeOfImage=0x805000。

---

## 2. 全参照統計

### 2(a) decompiled（1パス走査、lowercased `b333c4`）

- 総行数 1,066,496 / マッチ行 **1,647**（全行が `DAT_00b333c4` を含む。other-label 行 0）
- トークン出現数 **1,732** / 参照関数 **596**
- フォーム別（出現数 / 行数 / 関数数）:

| フォーム | 出現 | 行 | 関数 | 意味 |
|---|---|---|---|---|
| PTR（値用途: null判定・引数渡し・比較等） | 968 | 962 | 478 | ポインタ変数本体の利用 |
| VTABLE `*DAT_00b333c4 + 0xNNN` | 312 | 312 | 152 | **vtable スロット**（ブロック外・hex 311+decimal `+900` 1） |
| SUB `DAT_00b333c4[N]`（数値添字） | 212 | 185 | 82 | フィールド＝**byte 4N**（§3） |
| CAST `*(T*)(DAT_00b333c4 + 0xN)` | 208 | 200 | 95 | フィールド＝**byte N**（§3） |
| CASTINT `*(T*)((int)DAT_00b333c4 + 0xN)` | 18 | 17 | 11 | フィールド＝**byte N** |
| VTDEREF `*DAT_00b333c4`（単独逆参照） | 7 | 7 | 6 | vtable 読み出し |
| ASSIGN `DAT_00b333c4 = …` | 6 | 6 | 4 | 代入（§5） |
| SUBDYN（可変添字） | 1 | 1 | 1 | `DAT_00b333c4[変数]` |
| OTHERLABEL | 0 | 0 | 0 | — |

（前後文脈±2行つき全行は `%TEMP%\opencode\v72_dec_refs.txt`）

### 2(b) .text imm32 `C4 33 B3 00`（=0x00B333C4）— capstone 検証スキャン

- パターン出現 **2,663 箇所**、全数を「**その VA が decompiled ヘッダ由来の関数境界リニアデコードで指令境界に一致する**」条件で capstone 検証 → **UNVERIFIED 0**。
- 読/書別: **R(read)=2,660 / W(write)=3 / IMM(アドレス即値化)=0**
- 参照関数（実所有）: **791**（すべて FUN_）。全 2,663 箇所が絶対 `[…0xB333C4]` のメモリオペランド。
- W 3 箇所（＝ポインタ変数への書込みはこれだけ）:

| VA | 指令 | owner | 対応 decompiled |
|---|---|---|---|
| 0x004067A6 | `mov dword ptr [0xb333c4], eax` | FUN_00406360 | L4996/L4999（null→ctor 結果。片寄り合流で1本化） |
| 0x004497A9 | `mov dword ptr [0xb333c4], ebx` | FUN_004492e0 | L56593（破棄経路 `= 0`） |
| 0x00464553 | `mov dword ptr [0xb333c4], eax` | FUN_00464440 | L79073/L79076（同上） |

- ファイル全体の `C4 33 B3 00` 出現 = 2,663（**.text 以外＝0 件**。ポインタ値がデータとして保存されることは無い）。
- **近傍走査** `[0xB333C4-0x400, 0xB333C4+0x804)`（base 精确値以外）: 検証済み 5,578 箇所 / **270 種の別アドレス**（負方向40＝file-backed .data、正方向230＝bss 近縁グローバル群）。例: `mov [0xb33395]`+47, `[0xb333cc]`, `[0xb333d8]`, `[0xb333a0]`, `0xB33404–0xB33415` …（decompiled 側で `DAT_00b33395 / DAT_00b333cc / DAT_00b333d8 / DAT_00b333a0` 等**別ラベル**として出現＝独立グローバル）。
- ⇒ **`0xB333C4 + X` の絶対アドレスでヒープブロックのフィールドへ到達しているコードは 0 件**（実行時 new 済みアドレスのため原理的に不可能と一致）。

### 2(c) 両統計の関係

- **decompiled 596 関数 ⊂ binary 791 関数**（decompiled-only 0、binary-only 195 関数・450 サイト）。
- 欠落の典型例（this 引数のエリジョン）: FUN_0040c830 の機械語 `0040C92B mov ecx,[0xb333c4] … 0040C945 call 0x660110` に対し decompiled L8076 は `iVar13 = FUN_00660110(1);` と **DAT 引数を出力に残さない**。→ 参照数はバイナリ側が正。

---

## 3. 基準型判定（byte vs 添字）— 実測で確定

### 結論: `DAT_00b333c4` は **bss@0xB333C4 の int\* ポインタ変数1本**であり、実体ブロックは **ヒープ上 0x804 bytes**。decompiled 表記は2系統混在し、**どちらも asm 実測で裏取りした**。

**判定 A（subscript 形）: `DAT_00b333c4[N]` = byte +4N（int 添字 ×4）**

- **決定的例**: FUN_004073d0 / decompiled L5509 `if (((((char)DAT_00b333c4[0x162] != '\0') …`
  ```
  00407508  8b0dc433b300   mov ecx, dword ptr [0xb333c4]
  0040750E  80b98805000000 cmp byte ptr [ecx + 0x588], 0
  ```
  ⇒ `0x162` の実体は **byte +0x588**（+0x162 ではない）。`0xB333C4 + 0x588 = 0xB3394C` ではなく、**ポインタ経由 +0x588**。
- 複数例: `DAT_00b333c4[0x176]` → `0040A147 mov eax,[0xb333c4]` / `0040A15C cmp dword [eax+0x5d8],0`（＝+0x5D8）、`[0x177]` → `0040A1BA mov edi,[eax+0x5dc]`（＝+0x5DC）。
- ctor 内: `param_1[0x1e7] = 0` → `0066A85A mov dword [esi+0x79c], ebx`（×4）。

**判定 B（cast 形）: `*(T*)(DAT_00b333c4 + N)` = byte +N（生ディスプレースメント）**

- `+0x6b0`: FUN_004029e0 / L2010 → `00402A82 mov eax,[0xb333c4]` / `00402A87 add dword ptr [eax+0x6b0], 1`
- `+0x2c`: FUN_00462280 / L77560 → `004623CA mov eax,[0xb333c4]` / `004623CF mov edx, dword ptr [eax+0x2c]`
  （同一 byte0x2C が SUB 形 `DAT_00b333c4[0xb]`（L78008）＝4×0xB でも表記され、**両形が同一 byte オフセットで一致**することを確認)
- `+0x116`: FUN_0052f010 (Topic_Resolve) / L225258 → `0052F09D mov esi,[0xb333c4]` / `0052F0A5 cmp byte ptr [esi+0x116], bl`、`0052F0E8 mov byte [esi+0x116], bl`、`0052F0FC mov byte [ecx+0x116], 1`（L225261/225263 の `=0/=1` と3対3一致）。FUN_00465130 / L79716 → `00465697 mov eax,[0xb333c4]` / `0046569C mov cl, byte ptr [eax+0x116]`。
- **注意（param 側 only ×4）**: ctor の `*(undefined1*)(param_1 + 0x43)` は `0066A8DB mov byte ptr [esi+0x10c], bl` ＝ **×4（0x43→0x10C）**。int* パラメータ上の cast は C 規則どおり ×4、**グローバル `DAT_00b333c4` の cast は raw byte**（上記3サンプルすべて実測）。

**判定 C**: `*DAT_00b333c4 + 0xNNN` は **vtable スロット**（ブロックフィールドではない）。

### 既知2点の再解釈（本判定による）

| 既存記録 | decompiled 表記 | 実測 byte | 判定 |
|---|---|---|---|
| recursion guard `+0x116`（対話 深さ1ガード） | `*(char*)((int)DAT_00b333c4 + 0x116)` / `*(char*)(DAT_00b333c4 + 0x116)` | **byte +0x116**（asm 0x52F0A5/E8/FC, 0x46569C） | 記録どおり byte +0x116 |
| byte0x588 フラグ（Show1stPerson 系） | `(char)DAT_00b333c4[0x162]` | **byte +0x588**（asm 0x40750E） | 「0x162 表記」は int 添字＝0x588。**byte0x162 ではない** |

---

## 4. フィールド地図

### 4-1. 帯域ヒストグラム（byte 換算、DAT 形式 SUB×4 + CAST raw の合算: 出現 438 / 異なり 106 / 最大 +0x7A0）

| 帯域 | 出現 | 異なり | 主なオフセット |
|---|---|---|---|
| +0x000–0x0FF | 119 | 19 | 0x20,0x24,0x28,0x2C,0x30,0x34,0x3C,0x43,0x45,0x49,0x4B,0x58… |
| +0x100–0x1FF | 74 | 15 | 0x10C,0x110,0x114,0x115,0x116,0x117,0x118,0x11C,0x124,0x12C,0x184,0x1DC… |
| +0x200–0x2FF | 1 | 1 | 0x200（＝SUB `[0x80]`, L107150） |
| +0x300–0x3FF | **0** | 0 | （ctor が一括ゼロ埋めする配列帯・named 参照なし） |
| +0x400–0x4FF | **0** | 0 | （同上） |
| +0x500–0x5FF | 122 | 21 | 0x570,0x574,0x578,0x57C,0x584,0x588,0x590,0x594,0x5A8,0x5AC,0x5B4… |
| +0x600–0x6FF | 101 | 46 | 0x600,0x610,0x614,0x61C,0x62C…0x6B0…（最多46種） |
| +0x700–0x7FF | 21 | 4 | 0x700,0x734,0x738,0x7A0 |

※ +0x200〜+0x56C の空き帯は ctor FUN_0066a740 がループ3本で dword 一括ゼロ埋め
（idx 0x81–0xC8 / 0xC9–0x110 / 0x114–0x15B ＝ byte 0x204–0x320 / 0x324–0x440 / 0x450–0x56C）。**動的インデックスで使われる配列格納域**（SUBDYN の named 出現は1件のみ）。

### 4-2. 上位15オフセットと用途分類（判定根拠＝decompiled 行引用）

| # | off | 出現 | 用途分類 | 判定根拠（行） |
|---|---|---|---|---|
| 1 | +0x058 | 61 | **参照カウント付きサブオブジェクトの vtable ポインタ**（ctor で 0x2ec 確保したオブジェクト＝idx0x16。FUN_00489820 に集中） | L110895 `iVar6 = (**(code **)(**(int **)(DAT_00b333c4 + 0x58) + 0xf8))(1);` ＋ ctor L430287 `param_1[0x16]` |
| 2 | +0x588 | 34 | **1-byte フラグ（Show1stPerson 系候補・視点/会話状態）** | L5509 `((char)DAT_00b333c4[0x162] != '\0')`、L173563 `FUN_006600d0((char)DAT_00b333c4[0x162])`（計38行系） |
| 3 | +0x1E0 | 17 | **一時保持ポインタ**（比較一致時に 0 クリア） | L179025 `if (param_1 == *(int **)(DAT_00b333c4 + 0x1e0))` / L179026 `… = 0;` (FUN_004e4690) |
| 4 | +0x5D8 | 13 | **refcount 対象オブジェクトの dword/ptr**（null 判定で分岐、ctor が InterlockedDecrement リリース） | L6600/L6606 `if (DAT_00b333c4[0x176] == 0)`、ctor L430193 |
| 5 | +0x738 | 12 | **戦闘中フラグ（byte）** — StartCombat で 1、戦闘/死亡パイプラインで判定 | L206348 `*(undefined1 *)(DAT_00b333c4 + 0x738) = 1;` (FUN_00514660=StartCombat)、L360730 `*(char *)(…+0x738) != '\0'` (FUN_005f6330) |
| 6 | +0x116 | 10 | **対話再帰ガード（深さ1、byte）** | L225258/225261/225263 (FUN_0052f010=Topic_Resolve)、L433531 (FUN_0066f420) |
| 7 | +0x590 | 10 | **dword 状態/ハンドル**（StartCombat で 0 クリア、`DAT_00b3b730` へコピー転送） | L206293 `*(undefined4 *)(…+0x590) = 0;`、L340251 `DAT_00b3b730 = *(undefined4 *)(DAT_00b333c4 + 0x590);` |
| 8 | +0x5F4 | 10 | **オブジェクト ptr**（null 判定後に vtable+0xD4 呼び出し） | L207234 `if (*(int **)(DAT_00b333c4 + 0x5f4) == 0)` / L207238 `(**(code **)(**(int **)(…+0x5f4) + 0xd4))()` |
| 9 | +0x11C | 9 | **int→float スケール値**（浮動小数式で乗算） | L110442 `((float)*(int *)(DAT_00b333c4 + 0x11c) * (float)_DAT_00a3d8e8 + …` (FUN_00488e50) |
| 10 | +0x5C0 | 9 | **1-byte モード/状態フラグ（idx0x170）** | L289532/L289549 `((char)DAT_00b333c4[0x170] == '\0' …` (FUN_005821f0、FUN_005a4980 等) |
| 11 | +0x12C | 9 | **1-byte フラグ（decimal 表記 300）** — 1/0 切替、`FUN_004dd260(DAT,…)` と連動 | L319033 `*(undefined1 *)(DAT_00b333c4 + 300) = 1;` / L319035 `= 0;` (FUN_005b03b0) |
| 12 | +0x110 | 9 | **dword カウンタ**（上限を bss `DAT_00b37d18` と比較、毎回 +1） | L354158 `*(int *)(…+0x110) <= DAT_00b37d18` / L354169 `… + 1` (FUN_005eb520) |
| 13 | +0x02C | 8 | **Vector3 X（float 連続 +0x2C/+0x30/+0x34 ＝ idx0xB–0xD）** | L77560 `local_c = *(undefined4 *)(DAT_00b333c4 + 0x2c);`、L78008 `FUN_0066eaf0(DAT_00b333c4[0xb],…[0xc],…[0xd],…)` |
| 14 | +0x030 | 7 | Vector3 Y | L77561 / L78008 |
| 15 | +0x034 | 7 | Vector3 Z | L77562 / L78008 |

次点: +0x5C ×6, +0x570 ×6, +0x578 ×6, +0x5E0 ×6, +0x610 ×5。+0x734 は float フィールド（L347461 `*(float *)(DAT_00b333c4 + 0x734) * _DAT_00b37b88`、L311033 引数渡し）。

---

## 5. 初期化元と規模推定

### 5-1. ポインタ変数の初期化（bss@0xB333C4 への書込みは全 .text で3箇所のみ）

- **生成経路（2箇所とも同一パターン）**:
  ```
  push 0x804 ; call 0x401f00        ; ヒープ確保（new/malloc 系ラッパ）
  test eax,eax / mov ecx,eax
  call 0x66a740                     ; FUN_0066a740 = ブロック初期化 ctor
  mov  dword ptr [0xb333c4], eax    ; グローバルへ登録
  ```
  - FUN_00406360: `00406774 push 0x804` → `00406793 call 0x66a740` → `004067A6 mov [0xb333c4], eax`
  - FUN_00464440: `0046451D push 0x804` → `0046453C call 0x66a740` → `00464553 mov [0xb333c4], eax`
- **破棄経路**: FUN_004492e0 `004497A9 mov [0xb333c4], ebx`（vtable+0x10 リリース後 `= 0`、L56593）。
- decompiled ASSIGN 6 行 = 4 関数（0x406360/0x464440 は null+ctor が分岐合流で1ストア化、0x4492e0、0x5ba4d0 L324692 は機械語ストア未検出→§7-1）。

### 5-2. ctor `FUN_0066a740`（ブロック初期化本体、L430157〜）

- vtable 設定: `*param_1 = &PTR_FUN_00a73a0c`（+0x000）、`param_1[6]`（+0x018）、`param_1[0x17]`（+0x05C）、`param_1[0x1a]`（+0x068）、`param_1[0x1e2]`（+0x788）。
- 数値指定初期化 **232 箇所 / 異なり 205 byte オフセット**、最大 **+0x800**（`param_1[0x200] = 0`）。
- 配列帯一括ゼロ: idx 0x81–0x15B（byte +0x204–+0x56C、ループ3本）。
- byte 単位初期化: +0x115=1、**+0x588=0（`*(param_1+0x162)`）**、+0x589/58A/58B=0（隣接 byte）、+0x5A9=1、+0x6E5/6E6=0、+0x71C–0x71F 等。
- サブ確保: 0x94（→+0x790, memset 0x25×4）、0x54（→+0x5B0）、0x2ec（→+0x058）。

### 5-3. 規模上限

| 根拠 | 値 |
|---|---|
| 確保サイズ（2箇所とも `push 0x804`） | **0x804 bytes = 2,052 B = 513 dwords（idx 0..0x200）** |
| ctor 最大アクセス | +0x800（idx0x200、終端 dword） |
| decompiled DAT 形式 最大アクセス | +0x7A0（L412600 `DAT_00b333c4[0x1e8] = DAT_00b333c4[0x1e8] + 1`、FUN_0064bbc0） |
| 近傍 bss 終端（ブロック外） | .data 仮想末 0xBABC1C（このブロックとは無関係） |

⇒ **ブロック規模 = 固定 0x804 bytes（ヒープ、単一オブジェクト）**。named 使用は +0x7A0 まで、ctor は +0x800 まで初期化。

### 5-4. 参照関数数の合計

- decompiled: **596** / binary: **791** / 統合（dec ⊂ bin）: **791 関数**（binary-only 195 関数・450 サイトは this 引数エリジョン起因）。

---

## 6. 移植示唆

1. **一括構造体化を推奨**: `0xB333C4` は「ポインタ1本＋実体=ヒープ 0x804 の単巨大オブジェクト」なので、移植時は `struct Globals { … }` を1つ（static または `std::unique_ptr`）にまとめ、初期化は ctor 2系統（0x406360/0x464440）を単一 `init_globals()` に集約し、書き込み3箇所・null チェック経路をそこで吸収する。
2. **表記正規化**: フィールド定義は **byte オフセット基準で1箇所**に正規化（`[N]`→4N、cast `+N`→N の二重表記を排除）。+0x115/+0x116/+0x588–0x58B/+0x5A9/+0x6E5 等の byte 直アクセス帯は `uint8_t`/ビットフィールド、残りは dword フィールドとして宣言。
3. **帯域の取捨選択**: +0x200〜+0x56C の配列帯は named 参照ゼロでも ctor 一括ゼロ＋動的添字で使われるため**除外不可**（残す）。逆に bss 近傍の独立グローバル230本（0xB333C4+0x4〜+0x7BC の絶対参照群＝Ghidra 別ラベル）はブロックと無関係なので移植対象から分離してよい。

---

## 7. 未解決点

1. **L324692 `DAT_00b333c4 = piVar12`（FUN_005ba4d0）に機械語ストア不在** — 同関数に `mov [0xb333c4],…` は0件（R のみ10件）。Ghidra の SSA join/不要ストア排除の産物の可能性。W は全体で3箇所のみ。
2. **decompiled 欠落195関数の原因**は「呼出しの this 引数（DAT ポインタ）が出力に現れない」例（L8076 vs 0x40C92B）で説明できるが、195 関数すべての目視検証は未実施。
3. **+0x200〜+0x56C 配列帯の中身**（要素型・用途）未特定。named SUBDYN は decompiled で1件のみで、実体は param_1 引数経由アクセス群（本統計の対象外）。
4. **FUN_00660110 / FUN_006600d0 の this が常に DAT_00b333c4 か**は全呼出し未確認（本タスクでは例証のみ）。
5. **近傍 bss cluster（0xB333C8〜0xB33B80 の正方向230アドレス）**の個別ラベルは `DAT_00b33395/98/9c/a0/b4/b8/cc/d8/f6, DAT_00b33404-415` 等の一部のみ確認（他は独立グローバルと推定）。
6. **終端 byte +0x7A4〜+0x803**（約40B）は named アクセス未検出（ctor は +0x800 まで初期化済み）。用途未特定。
7. `0x401f00` の正確な身元（`operator new` か malloc ラッパか）は IDA 的裏付けのみ（引数0x804/0x94/0x54/0x2ec の確保呼び出しとして実測）。
