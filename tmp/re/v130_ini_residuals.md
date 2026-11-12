# V130: INI 設定系の残点束（type c/h/a・lpDefault・動的ノード・0xB07AD8・path 衝突・s60 照合）

- 作成日: 2026-10-26 / タスク: V130（優先度: 中） / 出典: `tmp/re/v91_ini_defaults.md` §8 残点 1〜5, 7（6 = `.data` スキャンの原理的限界は本タスク対象外）
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（v1.2.0416 Steam、ImageBase 0x400000）
- 手法: `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（作業前）。capstone 5.0.7 (skipdata=True) で逆アセンブル、PE インポート記述子を手動パースして IAT 解決、`.data`/`.rdata`/`.text` の imm32 xref は `re_util.find_dword`、EXE 文字列は生バイト直検索。
- 成果物は **新規 1 ファイルのみ**（本ノート）。既存 docs / tmp/re / TODO / session-memory / plan.md の編集、git、ESM 再スキャンは一切なし。中間スクリプト・出力はすべて `%TEMP%\opencode\v130_*.py` / `v130_*.txt`。
- **ESM は未使用**（s60 照合は既存成果物 `s60_io_ini_keys.txt` / `s60_io.md` / `s60_io_2_strings_all.txt` のみ）。

---

## §0 結論表（6 項目）

| # | 残点 | 判定 | 一言 |
|---|---|---|---|
| 1 | type 1(c)/2(h)/8(a) の switch 実体 | **解明** | 2 段ディスパッチ（byte 表 0x4A7954 + jump 表 0x4A792C）を全復元。印付き 3 アドレスは `mov eax,N; ret` の epilogue。挙動は全 10 消費者で確定、**語源 (h) のみ未解明** |
| 2 | LoadOne の lpDefault | **解明** | **現行 `node->value`（型別に変換した文字列/整数）**。専用の静的既定文字列は無い → INI 未設定キー時は値が不変 |
| 3 | 動的ノード 0x404850 / 0x444060 | **解明** | 各 **1 回**（once-guard）。**ヒープではなく .bss 静的ノード 2 個**（0xB3524C / 0xB35264）。中身は `fAlpha:Water`(f, 既定 0.5) と `uSurfaceTextureSize:Water`(u, 既定 0x80)。実行時ノード総数 = **637** |
| 4 | 0xB07AD8 の用途 | **解明** | RTTI クラス **`.?AVLipSynchroSettingCollection@@`** の静的インスタンス。参照は **1 件のみ = atexit 終了処理シンク 0xA1AA20 → dtor 0x4A84E0**、登録ノード 0・内部フィールド参照 0 → **未使用（死蔵）** |
| 5 | path バッファ +0x108 衝突可否 | **解明: 衝突可** | 全コピーが境界検査なし（8 箇所）。フルパス ≥ 260 B で +0x108、**≥ 264 B で head(+0x10C) 破壊**。トリガーは folder ≥ 243 文字 |
| 6 | s60 の 615 キーとの全数照合（差 20） | **解明** | 615 行 = 611 ユニーク + 重複 4 行。**20 = 21（セクション白名簿外）+ 6（s60 の純漏れ）+ 4（文字列境界汚染エイリアス）+ 4（重複行）− 4（エイリアス相殺）− 2（動的登録 2 キー）− 1（`v:SW` 偽キー）** |

---

## §1 タスク (1): type 1(c) / 2(h) / 8(a) の switch 実体

### 1.1 型判定関数 `FUN_004A78D0(name)` の完全復元

```c
// 0x4A78D0
int type(const char* name) {                 // cdecl, 1 arg
    if (!name) return 9;
    int c = (signed char)name[0] - 'S';      // 0x4A78E0: add ecx,-0x53
    if (c > 0x22) return 9;                  // 0x4A78E3: cmp ecx,0x22 / ja
    int k = byte_table[0x4A7954 + c];        // 0x4A78E8: movzx ecx,[ecx+0x4a7954]
    return jump_table[k];                    // 0x4A78EF: jmp [ecx*4+0x4a792c]
}
```

**byte 表 `0x4A7954`（index = 先頭文字 − 'S'、有効範囲 0..0x22）— 復元結果**

| index | 文字 | byte → case |
|---|---|---|
| 0x00 | `S` | 0 |
| 0x01..0x0D | `T..`` ` `` | 9（= default） |
| 0x0E | **`a`** | **1** |
| 0x0F | `b` | 2 |
| 0x10 | **`c`** | **3** |
| 0x11,0x12 | `d`,`e` | 9 |
| 0x13 | `f` | 4 |
| 0x14 | `g` | 9 |
| 0x15 | **`h`** | **5** |
| 0x16 | `i` | 6 |
| 0x17..0x1E | `j..r` 前 | 9 |
| 0x1F | `r` | 7 |
| 0x20 | `s` | 0 |
| 0x21 | `t` | 9 |
| 0x22 | `u` | 8 |

**jump 表 `0x4A792C`（index 0..9）— 復元結果**

| k | 実体 | 戻り type |
|---|---|---|
| 0 | `0x4A7917: mov eax,6; ret` | 6 (`s`/`S`) |
| 1 | **`0x4A7923: mov eax,8; ret`** ← **タスク印** | **8 (`a`)** |
| 2 | `0x4A78F6: xor eax,eax; ret` | 0 (`b`) |
| 3 | **`0x4A78F9: mov eax,1; ret`** ← **タスク印** | **1 (`c`)** |
| 4 | `0x4A78FF: mov eax,5; ret` | 5 (`f`) |
| 5 | **`0x4A7905: mov eax,2; ret`** ← **タスク印** | **2 (`h`)** |
| 6 | `0x4A790B: mov eax,3; ret` | 3 (`i`) |
| 7 | `0x4A791D: mov eax,7; ret` | 7 (`r`) |
| 8 | `0x4A7911: mov eax,4; ret` | 4 (`u`) |
| 9 | `0x4A7928: ret`（eax は入口の `mov eax,9` 済み） | 9 (unknown) |

→ **タスク印の 3 アドレスそのものは「型コードを返す epilogue」であり、特殊な分岐ロジックは無い**。意味は「どの入力で到達するか（byte 表）」にあり、上表で完全に確定した。byte 表は 0x4A7954..0x4A7975 の **34 バイト + 0x4A7976 以降は `0xCC`（int3）**、jump 表は 0x4A792C..0x4A7953 の 10 ダブルワード。

### 1.2 type 1/2/8 の実行時挙動（全消費者）

`FUN_004A78D0` の E8 呼出しは **10 件**（`re_util.callers`）: `0x4A7B7C, 0x4A7D9B, 0x4A8648, 0x4A8859, 0x4A8C18, 0x4A8D85, 0x514068, 0x5141A0, 0x5142C9, 0x514460`。

| 消費者 | type 1 (`c`) | type 2 (`h`) | type 8 (`a`) |
|---|---|---|---|
| **ReadOne/LoadOne** `0x4A8800` | case→`0x4A88C1`: `GetPrivateProfileIntA(path,buf,nDefault=node->value)` → **`mov byte [esi],al`（下位 1 バイトのみ書込）** | case→`0x4A8A9C`: 同 API、nDefault=現行値 → **`mov [esi],eax`（4 バイト書込）** | case→`0x4A89F6`: `GetPrivateProfileStringA`(fmt `"%u,%u,%u,%u"` 0xA40520) → `sscanf` → **`0x4A83C0` で 1 u32 にパック** → `mov [esi],eax` |
| **WriteOne** `0x4A85F0` | 表 0x4A87D4[0] → `0x4A8691`: **`sprintf(buf,"%d", value)`** | [1] → `0x4A876F`: **`sprintf(buf,"%u", value)`**（0xA4051C） | [7] → `0x4A8728`: `0x4A83F0` で展開 → **`sprintf(buf,"%u,%u,%u,%u")`** |
| Dump 整形 `0x4A7B10` | 表 0x4A7C7C[1] → `0x4A7BBC`: **`movzx edx,byte[ebp]`（1 バイト読）** + `0x70F9B0` | [2] → `0x4A7BBC`（**type 1 と同一 = 1 バイト読**） | [7]→表範囲外 → `ja 0x4A7C24` → **`mov edx,[ebp]` 生 dword を `0x70FB40` で整形**（無加工） |
| コンソール `GetINISetting` 0x514230 | 表 0x5143A0[1] → `0x514366` = **"INISetting %s >> UNKNOWN TYPE"** | [2] → 同 **UNKNOWN TYPE** | `cmp eax,6; ja` → **UNKNOWN TYPE** |
| コンソール `SetINISetting` 0x5143C0 | 表 0x5144E4[1] → `0x5144A4` = **UNKNOWN TYPE** | [2] → 同 **UNKNOWN TYPE** | `ja` → **UNKNOWN TYPE** |
| GameSetting Get 0x514068 | `sub eax,3/2/1` チェーンで 3/5/6 のみ → else = **`0xA4D770 "GameSetting %s >> UNKNOWN TYPE"`** | 同 | 同（UNKNOWN） |
| GameSetting Set 0x5141A0 | 同（3/5/6 のみ実装、他は UNKNOWN） | 同 | 同 |
| レジストリ書 `0x4A8B90` | **type==1 のときのみ** 名前コピー先頭バイトを `'s'`(0x73) に差し替えてから `RegSetValueExA` | 通常扱い（無変換） | 通常扱い |
| レジストリ読 `0x4A8D30` | 通常扱い | 通常扱い | 通常扱い（buffer size は type6 のときのみ 0x104） |
| 設定ファイル読 `0x4A7D60` | `sub eax,3 / -2 / -1` で 3/5/6 のみ、**else → `xor eax,eal; ret`（false）** | 同（未対応） | 同（未対応） |

補助関数（IAT 解決で確定）:
- **`0xA280B4` = `KERNEL32!GetPrivateProfileStringA`**（6 引数、type 5/6/7/8 で使用）
- **`0xA280E4` = `KERNEL32!GetPrivateProfileIntA`**（4 引数、type 0/1/2/3/4 で使用）
- **`0xA280B0` = `KERNEL32!WritePrivateProfileStringA`**
- **`0xA28000` = `ADVAPI32!RegSetValueExA` / `0xA28010` = `ADVAPI32!RegQueryValueExA`**

> v91 §5.2 は「[0xA280B4] を 4 回呼出」としていたが、実際は **String(6 引数) と Int(4 引数) の 2 種の IAT エントリが分かれている**（v91 の精緻化）。

### 1.3 型の意味判定

| type | 先頭文字 | 実体（挙動） | 意味 | 判定 |
|---|---|---|---|---|
| 1 | `c` | 1 バイト読 / 1 バイト書込 / 書出しは `%d`（符号付き）/ ダンプも 1 バイト | **char（符号付き 1 バイト整数）**と推定 | **解明（推定含む）** |
| 2 | `h` | 読み込みは 4 バイト書込、書出しは `%u`（type 4 と同一）、ダンプのみ 1 バイト読 | 語源 **未解明**（`hword`/`hex` 等の候補はいずれも挙動と不整合。type 4 とほぼ同義） | **未解明（挙動のみ確定）** |
| 8 | `a` | 読み込み `"%u,%u,%u,%u"` → `0x4A83C0` で `(c0<<24)\|(c1<<16)\|(c2<<8)\|c3` にパック、書出しは `0x4A83F0` で逆展開して同じ書式。**pack/unpack は厳密に可逆** | **4 コンポーネントの色（`r`=3 成分 + α固定 0xFF の対になる「α 明示」版）**と推定 | **解明（推定含む）** |

- `0x4A83C0`（pack）: `ret` の直前までで `(arg1<<24)\|(arg2<<16)\|(arg3<<8)\|arg4` を返す（逆アセンブル全文は `%TEMP%\opencode\v130_02_consumers.txt` L68-80）。
- `0x4A83F0`（unpack）: `out0=v>>24, out1=(v>>16)&0xFF, out2=(v>>8)&0xFF, out3=v&0xFF`（L94-110）→ **完全に可逆**。
- type 7 (`r`) は pack をインラインで行い `or eax,0xff`（下位バイト = α を 0xFF 固定）、書出しは 3 成分のみ `"%u,%u,%u"` → 3 成分色。
- **出現件数**: INI 系 3 コンテナの登録ノード（静的 635 + 動的 2 = 637）では **type 1/2/8 はいずれも 0 件**。動的 2 件は `f…`(5) と `u…`(4)。name は登録後に変化しないため **実行時も 0 件 = デッドコード**（ただし ReadOne/WriteOne では実装済み）。
  - ※別系統の `GameSetting` コンテナ（`0xB35574`、lookup `0x55E000`）は本タスクの登録列挙対象外（type 3/5/6 のみ処理）。

### 1.4 試行経路

1. `v130_01_type.py` → 0x4A78D0〜0x4A7980 の逆アセンブル + byte 表 / jump 表ダンプ（`v130_01_type.txt`）。
2. `v130_01b.py` → ReadOne/WriteOne/GetINI/SetINI の各 switch 表を**生ダブルワードで直接読む**（capstone の命令分割に依存しない）（`v130_01b.txt`）。
3. `v130_02_consumers.py` → `callers(0x4A78D0)` で全 10 消費者を網羅、各サイトを前後 0x50〜0x90 B 逆アセンブル、`0x4A83C0`/`0x4A83F0`/getter 3 種の本体を確認（`v130_02_consumers.txt`）。
4. `v130_08_misc.py` → Dump 整形の switch 表 0x4A7C7C とメッセージ文字列群（`v130_08_misc.txt`）。
5. `v130_10_iat.py` → PE インポート記述子を手動パースし IAT を名前解決（`v130_10_iat.txt`）。
6. `v130_07_final.py` → type を消費する残り 6 サイト（0x4A7D9B / 0x4A8C18 / 0x4A8D85 / 0x514068 / 0x5141A0）を確認（`v130_07_final.txt`）。

---

## §2 タスク (2): `LoadOne`（= `FUN_004A8800`）の `lpDefault` 引数

### 2.1 判定: **現行 `node->value` を型に合わせて変換したもの**（専用の静的既定文字列は存在しない）

| type | 呼出し API | 3 引数目（= lpDefault / nDefault）の実体 | 検出時 |
|---|---|---|---|
| 0 `b` | `GetPrivateProfileIntA` | `nDefault = (node->value != 0)`（`0x404DF0` getter → `setne cl`） | `test eax,eax; setne dl` → 0/1 に正規化 |
| 1 `c` | `GetPrivateProfileIntA` | `nDefault = *(u32*)(&node->value)`（`0x403BE0` getter → `mov edx,[eax]`） | `mov byte [esi],al` |
| 2 `h` | `GetPrivateProfileIntA` | `nDefault = node->value`（`mov ecx,[esi]` を直接 push） | `mov [esi],eax` |
| 3 `i` | `GetPrivateProfileIntA` | `nDefault = node->value` | `mov [esi],eax` |
| 4 `u` | `GetPrivateProfileIntA` | `nDefault = node->value` | `mov [esi],eax` |
| 5 `f` | `GetPrivateProfileStringA` | **`sprintf(buf,"%f", node->value)` した直列バッファ**（`0x403C00` getter → `fstp qword[esp]` → `push 0xA40540 "%f"`） | `sscanf(out,"%f", node)` |
| 6 `s` | `GetPrivateProfileStringA` | **`lpDefault = node->value`（現行の char* をそのまま）**、`nSize=0x100` | `0x4A7980(node, outbuf)` |
| 7 `r` | `GetPrivateProfileStringA` | `sprintf(buf,"%u,%u,%u", 現行値を 0x4A83F0 で展開)` | `sscanf(out,"%u,%u,%u")` → α=0xFF 付与 |
| 8 `a` | `GetPrivateProfileStringA` | `sprintf(buf,"%u,%u,%u,%u", 現行値を 0x4A83F0 で展開)` | `sscanf(out,"%u,%u,%u,%u")` → `0x4A83C0` |

### 2.2 意味

- **INI ファイルにキーが無い場合、`GetPrivateProfile*` は lpDefault（= 現行値）を返すため、`node->value` は一切変化しない** = `.data` の既定値（CRT 静的初期化時点の値）が保持される。v91 §5.2 の「初期値で置かれ LoadAll で上書きされる」を **キー有無のレベルで裏付け**。
- type 6 の lpDefault は `.rdata` の既定文字列（読み取り専用）だが、**書き込み先は別バッファ**（`[esp+0x2a4]` → `0x4A7980`）なので `.rdata` は汚染されない。
- **`0x4A7980` の実体**（`v130_09_str.txt`）: `malloc(len(node->name)+1 + len(src)+1)` → ブロック先頭に **name のコピー**、その直後に **src のコピー** を置き、`node->value = &block[len(name)+1]`。つまり **文字列値は必ずヒープへ複製**され、スタックバッファを指し続けることは無い（dangling 回避）。
- 既定値を専用文字列にしている箇所は **1 もない**。type 5/7/8 の「現行値を書式化したローカルバッファ」も **現行値そのものの文字列化**であり、静的定数ではない（`0xA2F7EC = ""` は WriteOne の空既定に使われるが ReadOne では不使用）。

### 2.3 試行経路

`v130_01_type.txt` の ReadOne 全文逆アセンブル（0x4A8800..0x4A8AD0）を push 順で分解 → IAT 名（`v130_10_iat.py`）で引数の意味を確定 → `0x4A7980` 本体（`v130_09_str.py`）で保存先を確認。

---

## §3 タスク (3): 動的ノード `FUN_00404850` / `FUN_00444060` の実行回数と内容

### 3.1 判定

| 項目 | 結果 |
|---|---|
| 呼出しサイト | `FUN_00404850`（float ノード工場）= **1 件 `0x49D546`**（host `fun_0049d2a0`）、`FUN_00444060`（int ノード工場）= **1 件 `0x49DD72`**（host `fun_0049dd00`） |
| host の呼出し回数 | `fun_0049d2a0` = 3 件（`0x49E07F`, `0x49E427`, `0x49E76F`）、`fun_0049dd00` = 1 件（`0x49E6EE`） |
| **工場本体の実行回数** | **各 1 回 / プロセス**（**once-guard**）。float 側: `test byte[0xB35260],2; jne skip; or [0xB35260],2`。int 側: `test byte[0xB3526C],1; jne skip; or [0xB3526C],1` |
| **ノードの格納先** | **ヒープではない**。`mov ecx, 0xB3524C` / `mov ecx, 0xB35264` = **`.bss` の静的 8 B オブジェクト**（`>= 0xB32C00` のため v91 の `.data` 形状スキャン範囲外だった） |
| 登録先 | 両工場とも末尾で `mov ecx,0xB07BF0; call 0x4040E0`（AddSetting）→ **`INISettingCollection` に prepend** |

### 3.2 ヒープ内容の全数（＝生成物の中身）

**ノード 2 個の内容（各 8 B、`.bss`）**

| 格納 VA | 型判定 | name ポインタ → 文字列 | 既定値 | 備考 |
|---|---|---|---|---|
| `0xB3524C` | `f` → type 5 | `0xA3F43C` → **`"fAlpha:Water"`** | `fld dword[0xA3D65C]` = **0.5**（生 `0000003F`） | 工場引数はスタック経由 `fstp dword[esp]` |
| `0xB35264` | `u` → type 4 | `0xA3F4C4` → **`"uSurfaceTextureSize:Water"`** | `push 0x80` = **128** | 工場引数は即値 |

- 両ノードは **v91 §7 で「孤立ではないがノード化されていない」と記録された 2 文字列の正体**である（`.rdata` に存在し `.data` からポインタ参照 0 件だった理由 = **参照元が `.bss`**）。
- 工場は `ret 8`（2 引数）、呼び出し側は `push value; push name` の順で `ecx = ノード格納先` を渡す。**malloc はノードに対して行っていない**（直前の `push 0x30/0x20/0xC/0x40; call 0x401F00` は同一関数内の**別オブジェクト**（float 配列・行列・ワード列・0x47EA50 初期化ブロック）の確保であり、ノードには無関係）。
- 工場直後の `push 0xA1A630 / 0xA1A660; call 0x981FB4` は **atexit 登録**（作成した別オブジェクトの dtor）で、ノード自体の寿命はプロセス全体。

### 3.3 ノード総数（実行時）

| コンテナ | 静的 | 動的 | 実行時 |
|---|---|---|---|
| `0xB07BF0` `INISettingCollection` | 572 | **+2** | **574** |
| `0xB11C40` `BlendSettingCollection` | 62 | 0 | 62 |
| `0xB07D28` `RegSettingCollection` | 1 | 0 | 1 |
| `0xB07AD8` `LipSynchroSettingCollection` | 0 | 0 | 0 |
| **合計** | 635 | **+2** | **637** |

（`E8→0x4040E0` 呼出し 637 件 = 静的 635 + 動的 2 と完全一致。v91 §2.2 の 637 件の内訳確定。）

### 3.4 試行経路

`v130_recon.py`（callers / once-guard / 引数 push の逆アセンブル）→ `v130_recon2.py`（host 名と atexit 確認）→ `v130_07_final.py`（name 文字列と既定値の生読み、`.bss` であることを確認: `rd()` が "no file backing at VA 0xB3524C (bss of .data)" を警告 → **bss 確定の副産物**）。

---

## §4 タスク (4): `0xB07AD8`（vtable 0xA40548、登録 0 件）の用途

### 4.1 判定: **RTTI クラス `.?AVLipSynchroSettingCollection@@` の静的インスタンス。実行時は一切使われない（死蔵）**

**証拠 A — vtable の正体（9 スロット完全同一）**

```
0xA404F8 (0xB07BF0) = 0x4040E0 0x403C20 0x4A8B10 0x4A85F0 0x4A8800 0x4A83B0 0x4A7CB0 0x4A8560 0x4A85A0
0xA40548 (0xB07AD8) = 0x4040E0 0x403C20 0x4A8B10 0x4A85F0 0x4A8800 0x4A83B0 0x4A7CB0 0x4A8560 0x4A85A0   → 同一
```
→ INI backend（Add/Remove/Dump/WriteOne/ReadOne/Begin/End/SaveAll/LoadAll）を完全に共有。

**証拠 B — RTTI（vtable[-1] = Complete Object Locator の正規連鎖で確定）**

| vtable | COL | pTypeDescriptor | TypeDescriptor 内蔵名 |
|---|---|---|---|
| 0xA404F8 (obj 0xB07BF0) | 0xAB9D68 | 0xB07A88 | **`.?AVINISettingCollection@@`** |
| 0xA40548 (obj 0xB07AD8) | 0xAB9DB8 | 0xB07AAC | **`.?AVLipSynchroSettingCollection@@`** |
| 0xA40590 (obj 0xB07D28) | 0xAB9E0C | 0xB07D04 | **`.?AVRegSettingCollection@@`** |
| 0xA566A4 (obj 0xB11C40) | 0xABFB88 | 0xB11C14 | **`.?AVBlendSettingCollection@@`** |

> **v91 §1.2 の訂正**: 「オブジェクト直後の TypeDescriptor をそのクラスの RTTI」とみなしていたが、**正しくは vtable[-1] の COL が指す TD**。`0xB07BF0 + 0x114 = 0xB07D04` の TD は *次の* オブジェクト `0xB07D28` のもの。
> - `0xB07BF0` = **`INISettingCollection`**（INI backend と整合）
> - `0xB07D28` = **`RegSettingCollection`**（V51 の「レジストリ backend」`RegCreateKeyExA` 系と**完全整合**）
> - `0xB11C40` = **`BlendSettingCollection`**（V51 の「BlendSettings.ini」と**完全整合**）
> - オブジェクト占有サイズ `0x114 = 4(vptr) + 0x104(path) + 4(+0x108) + 4(+0x10C) + 4(+0x110)` で **レイアウトが厳密に一致**（v91 の 0x104 path が再確認できた副次効果）。
> - v91 が記した `DefaultCombatStyle` / `Atmosphere` という名前は今回の TD 4 件に存在しない → **§8 新規残点 N-3**。

**証拠 C — 参照は 1 件のみ**

- `find_dword(0xB07AD8)` = **総 1 件 / .text 1 件 = `0xA1AA20`**。
  ```
  0xA1AA20:  B9 D8 7A B0 00   mov ecx, 0xB07AD8
  0xA1AA25:  E9 B6 DA A8 FF   jmp 0x4A84E0
  ```
- その他の固定値参照（`0xB07ADC` = path 先頭、`0xB07AE4` = head、`0xB07AE8`、`0xB07AF0` …）は **すべて 0 件**。→ フィールドを読み書きするコードが存在しない。
- `push 0xB07AD8; call 0x4040E0`（AddSetting 呼出し）も **0 件** → **登録ノード 0 件**が確定。

**証拠 D — 0xA1AA20 は atexit 登録（＝静的デストラクタ）**

CRT 初期化ブロック `%09E06E0`:
```
0x9E06E0:  68 20 AA A1 00    push 0xA1AA20
0x9E06E5:  E8 CA 18 FA FF    call <atexit登録>
0x9E06EA:  59                pop ecx
0x9E06EB:  C3                ret
```
同様に `0xA1AA10`（→ obj 0xB07BF0）は `0x9E06F1`、`0xA1AA30`（→ obj 0xB07D28）は `0x9E0701` から登録。

**証拠 E — デストラクタ `0x4A84E0` の実体**（`v130_04_05.txt`）
```c
void dtor(InsiCollection* self) {
    self->vptr = 0xA404D0;                    // 途中 vtable
    while (self->chain /*+0x110*/) {          // リンク記録を free(0x401F20) して解放
        Rec* n = self->chain->next; free(self->chain); self->chain = n;
    }
    self->head /*+0x10C*/ = 0;
    self->vptr = 0xA403C8;                    // 基底 vtable（スロットは 0x982D85 = purecall 系）
}
```
→ **プロセス終了時にリストを解放するだけ**。vptr を書くコードは実行時に 0xA404F8/0xA40548 を**一度も書き換えない**（`find_dword(0xA404F8)` = ファイルイメージのみ、.text 0 件）= **初期化は完全静的（vptr はローダが .data から書き込む）**。

### 4.2 結論

**`0xB07AD8` は「LipSynchro（リップシンク）用 INI 設定コレクション」のプレースホルダ**であり、
① 静的に vtable だけ敷かれ、② 終了時にのみ 1 回参照される（デストラクタ）、③ そこに設定が 1 件も登録されず、④ パス構築・Begin/LoadAll/SaveAll のどれも呼ばれない。**v91 の「未使用（死蔵候補）」は「未使用（確定）」に昇格**。

### 4.3 試行経路

`v130_recon.py`（xref と vtable スロット比較）→ `v130_04_05.py`（オブジェクト生ダンパ、COL/TD 連鎖、dtor 逆アセンブル）→ `v130_07_final.py`（**TD 内蔵名を `rd(td+8)` で直接読む**。`+8` をポインタとして読む旧コードは解決不能だった）→ `v130_08_misc.py`（CRT 初期化テーブル 0x9E06A0.. の生ダンパで atexit 判明）。

---

## §5 タスク (5): path バッファ `+0x108` 衝突可否

### 5.1 判定: **衝突可（境界検査なしコピー）**

**オブジェクトレイアウト（`0x114` = 276 B、§4 の RTTI 対応で再確認）**

```
0xB07BF0 +0x000  vptr
0xB07BF4 +0x004  char path[0x104 = 260]   ← 容量 260 B（NUL 含む）
0xB07C98 +0x104  ... (path 内)
0xB07CF8 +0x108  u32 flag/handle          ← path の終端から dest offset 260
0xB07CFC +0x10C  Node* head               ← dest offset 264
0xB07D00 +0x110  Rec* chain               ← dest offset 268
0xB07D04 +0x114  (次のクラスの TypeDescriptor)
```

**境界検査なしコピーの全箇所（8 箇所 / 5 経路）**

| # | VA | 移動元 → 移動先 | 上限検査 |
|---|---|---|---|
| 1 | `0x40E0C4..0x40E0F3` | フォルダ文字列の末尾に `"Oblivion\"`（10 B 含 NUL、`0xA31270`）を追記 | **なし** |
| 2 | `0x40E100..0x40E112` | スタック `[esp+0xAC]` → **bss `0xB3F280`** | **なし** |
| 3 | `0x40E140..0x40E152` | bss `0xB3F280` → スタック `[esp+0x1B0]` | **なし** |
| 4 | `0x40E160..0x40E180` | スタック末尾に `"Oblivion.ini"`（`[0xB02C84]`→`0xA2FBA4`、12 B）を追記 | **なし** |
| 5 | **`0x40E710..0x40E722`** | スタック `[esp+eax+0x1B0]` → **`0xB07BF4`（+0x004）** | **なし** |
| 6 | `0x404420..0x40442F` | bss `0xB3F280` → スタック `[esp+eax+0x4C]`（frame `0x148`） | **なし** |
| 7 | `0x404470..0x40447F` | スタック `[esp+eax+0x4C]` → **`0xB07BF4`** | **なし** |
| 8 | `0x4045B0..0x4045BF` / `0x513C20..0x513C2E` | スタック → **`0xB07BF4`**（`0x404540` 系 / `RefreshINI 0x513BB0` 系） | **なし** |

いずれもループ本体は `mov cl,[src+eax]; mov [dst+eax],cl; inc eax; test cl,cl; jne` の 5 命令のみで、**`cmp eax, <limit>` が 1 も存在しない**。

### 5.2 衝突条件（算術）

- 移動先容量 = **260 B**（`+0x004..+0x107`）。書込みはインデックス `0..L`（NUL 含む、`L` = 文字列長）。
- `L ≤ 259` … 安全。
- **`L = 260` … NUL が offset 260 = **`+0x108`（flag）** に着地 → 衝突。ただし直後に `Begin()`（`[ecx+0x108]=ecx`）が来るため **実害は限定的**。
- **`L ≥ 264` … offset 264 = **`+0x10C`（head ポインタ）** を 4 B ともぎる → CRT 静的初期化で構築済みの連結リスト先頭が破壊され、以後の `LoadAll`/`SaveAll`/`Dump` が無効ポインタを辿る。
- `L ≥ 268` … `+0x110`（chain）も破壊。
- `L = len(dir) + 12`（`"Oblivion.ini"`）、`dir = folder + "Oblivion\"(9)`。よって:
  - `+0x108` 衝突: `dir ≥ 248` ⇔ **`folder ≥ 239` 文字**
  - `head` 破壊: `dir ≥ 252` ⇔ **`folder ≥ 243` 文字**

### 5.3 実現可能性

- フォルダ取得は引数 2 個の間接呼出し（`edi = [0xB3F928]`、出力はスタック buf）→ **MAX_PATH(260) 系の制限**を受ける（s60 §? の CSIDL 推定と同一系）。`folder ≤ 259` → `dir ≤ 268` → **`L ≤ 280`** となり、**head(+0x10C) は 4 B、chain(+0x110) は 4 B 突き破り、さらに 0xB07D04 の TD 文字列に到達し得る** → **理論上は衝突可能**。
- ただし **起動時のロード経路 `0x40E090`（frame `0x6A8` = 1704 B）だけはローカルバッファが余裕ある**ため、先に落ちずに **グローバル `0xB07BF4` へ無検査で到達できる唯一の経路**。
- 一方 `0x404400`/`0x404540`（frame `0x148`/`0x14C`、ローカル容量 **252 B**）と `RefreshINI 0x513BB0`（frame `0x108` = 264 B、cookie `0x104`）はローカル段階で `dir ≥ 252` / `dir+12 ≥ 260` になると **スタッフクッキー（`__security_check_cookie`）を破壊 → fail-fast abort**。すなわち `folder ≥ 243` では「head 破壊」と「クッキーアボート」が同一閾値で競合する。
- 実被害としては `folder ∈ [239, 243)` が唯一の「破壊されるが止まらない」窓（flag のみ、Begin が再設定するため無害）。

### 5.4 試行経路

`v130_recon.py`（`0xB07BF4` の 6 xref）→ `v130_recon2.py`（各コピーの逆アセンブル、frame サイズ）→ `v130_04_05.py`（**直近の `sub esp,imm` を各サイトから機械探索**、`0xB02C84` の実体 = ポインタ `0xA2FBA4 = "Oblivion.ini"`、`0xA31270 = "Oblivion\"` を生読み）→ 手計算で閾値導出。

---

## §6 タスク (6): s60 の 615 キーとの全数照合（件数差 20 の内訳）

### 6.1 セットの定義と件数（再集計・ESM 未使用）

| 集合 | 件数 | 出典 |
|---|---|---|
| v91 登録ノード（静的） | **635** | `v91_ini_defaults.csv` |
| s60 行数（raw） | **615** | `s60_io_ini_keys.txt` の `== [sec] ==` 配下の行を全数カウント（`v130_06b`） |
| s60 ユニーク `(key, section)`（大文字小文字無視） | **611** | 同上（**重複 4 行 = `v:SW` が 5 行**） |
| s60 セクション数 | **36** | 同上（v91 は 40） |
| 突合（大文字小文字無視） | **604** | `v130_06_s60diff.py` |
| v91 のみ | **31** | 同上 |
| s60 のみ（unique） | **7** | 同上 |

### 6.2 差 20 の内訳（完全分解）

```
635 − 615 = 20
           = [v91 のみ 31] − [s60 のみ 7 + 重複行 4]
           = 31 − 11
```

**A. v91 のみ 31 件 = 21 + 4 + 6**

| 内訳 | 件数 | 内容 / 原因 |
|---|---|---|
| **A-1. セクションが s60 の 36 セクションに無い** | **21** | s60 が走査しない 5 セクションの全キー = `bLightAttenuation` 12（bOutQuadInLin / bUseLinear / bUseQuadratic / bUseConstant / fConstantValue / fFlickerMovement / fLinearRadiusMult / fLinearValue / fQuadraticRadiusMult / fQuadraticValue / uLinearMethod / uQuadraticMethod）+ `MAIN` 5（**`:` 無し 4**: sBasePath / iLastHDRSetting / iDetectionHighNumPicks / fLowPerfCombatantVoiceDistance + **`fQuestScriptDelayTime:MAIN`**）+ `OPENMP` 2（iOpenMPLevel / iThreads）+ `PathFinding` 1（bUseBackgroundPathing）+ `Havok` 1（iHavokSkipFrameCountTEST） |
| **A-2. 文字列境界汚染エイリアス** | **4** | v91 名と s60 名が別表記で突合失敗。EXE 上の実文字列は v91 側が正しい（後述 6.3）: `bDialogueSubtitles:GamePlay`, `bDisplayLODBuildings:LOD`, `bUseMultiThreadedTrees:BackgroundLoad`, `fLandFriction:Landscape` |
| **A-3. s60 の純漏れ** | **6** | `bSoundEnabled:Audio`, `fHeadroomdB:Audio`, `fMinSoundVel:Audio`, `bInstantLevelUp:GamePlay`, `bUse Joystick:Controls`, `fGlobalTimeMultiplier:General` |

**B. s60 のみ 7 件 + 重複 4 行 = 11 件 = 4 + 2 + 1 + 4**

| 内訳 | 件数 | 内容 |
|---|---|---|
| **B-1. A-2 のエイリアス側** | 4 | `zDbDialogueSubtitles`, `HbDisplayLODBuildings`, `AbUseMultiThreadedTrees`, `AfLandFriction`（`Section` は共通）→ **A-2 と対になって相殺** |
| **B-2. 実行時（動的）登録ノード** | 2 | `fAlpha:Water`, `uSurfaceTextureSize:Water`（§3 で正体確定 = `.bss` ノード）→ v91 の静的列挙にのみ不在 |
| **B-3. 偽キー** | 1 | `v`（`== [SW] ==` 配下、VA `0x6EF5D2` 等 5 箇所 = **`.text` 内**）= 文字列走査の誤検出 |
| **B-4. 重複行** | 4 | `v:SW` の 5 行中 4 行（raw 615 − unique 611） |

**検算**: `31 − 11 = 20` ✓ ／ `16`... ではなく `21 + 4 + 6 − 4 − 2 − 1 − 4 = 20` ✓

**セクション別件数による独立検算**（`v130_06_diff.txt` の差分合計）

```
MAIN −5, bLightAttenuation −12, OPENMP −2, PathFinding −1, Havok −1   = −21  (A-1)
Audio −3, GamePlay −1, Controls −1, General −1                         = −6   (A-3)
LOD 0, BackgroundLoad 0, Landscape 0（±1 が相殺）                       = 0    (A-2/B-1 相殺)
SW +1, Water +2                                                        = +3   (B-2/B-3)
合計 −24 = 611 − 635 ✓     −24 + 重複 4 行 = −20 = 615 − 635 ✓
```

### 6.3 文字列境界汚染（A-2/B-1 の原因）の実証

EXE 生バイト直検索（`v130_07_final.py`）で、**v91 の 31 キーすべてに `Key:Section` リテラル（または bare キー）が実在**することを確認:

| v91 キー | VA | 直前の 4 バイト（s60 が先頭に吸い込んだもの） |
|---|---|---|
| `bDialogueSubtitles:GamePlay` | `0xA68C10` | `00 80 7A 44` = **float 1000.0f** の下位 2 バイト `z D` |
| `bDisplayLODBuildings:LOD` | `0xA47B24` | `00 00 00 48` → 末尾バイト `H` |
| `bUseMultiThreadedTrees:BackgroundLoad` | `0xA379D0` | `00 00 C8 41` = **float 100.0f** の末尾バイト `A` |
| `fLandFriction:Landscape` | `0xA56120` | `47 4E 92 41` の末尾バイト `A` |

→ **s60 のキー名先頭の `zD`/`H`/`A`/`A` は隣接する float/整数のバイトであり、文字列境界の誤検出**。v91 側の表記が正しく、両者は同一キー。
（A-3 の 6 キーもリテラルは実在 = `0xA77240` / `0xA785F0` / `0xA77C00` / `0xA73E88` / `0xA2FB18` / `0xA3D150`。s60 の `s60_io_2_strings_all.txt` とその参照スクリプト側の欠落。）

### 6.4 試行経路

1. `v130_06b_s60count.py` → `s60_io_ini_keys.txt` の**raw 行数 615 / ユニーク 611 / セクション 36 / 重複 `v:SW` ×5** を確定（`v130_06_diff.txt` は 611 を使っており、v91 記載の 615 との 4 行差がこれで解消）。
2. `v130_06_s60diff.py` → 突合 604 / 31 / 7、セクション別差分（既存スクリプトを再実行）。
3. `v130_07_final.py` → **group A/B/C の全 21 キーを EXE 生バイトで直検索**し、旧分類（`s60_io_2_strings_all.txt` 依存の「literal 不在」判定 11 件）が**誤り**であることを訂正。prefixed variant 正規表現で 4 件の汚染エイリアスを特定。

> **注意**: v91 §7 の「既知キー 33 件中 3 件未一致 = `fAlpha:Water` / `uSurfaceTextureSize:Water` / `fDifficultyMinValue:GamePlay`」のうち前 2 件は **本タスクで「動的登録ノード」と判明**（孤立でもノード化されないでもない）。残る `fDifficultyMinValue:GamePlay` は前処理どおり `.rdata` に接頭のみで真の孤立（本タスクでも変化なし）。

---

## §7 補足: type 消費者マトリクス（判定根拠の全体像）

| type | ReadOne | WriteOne | Dump整形 | GetINI | SetINI | GameSetting G/S | Reg Read/Write | FileRead 0x4A7D60 |
|---|---|---|---|---|---|---|---|---|
| 0 `b` | Int, nDef=(v!=0)→byte | `%u` | byte | `%i` | `atoi` | ✗ | – | ✗ |
| **1 `c`** | Int, nDef=v → **byte** | **`%d`** | **byte** | **UNKNOWN** | **UNKNOWN** | ✗ | 書=名前先頭→`'s'` | ✗ |
| **2 `h`** | Int, nDef=v → dword | **`%u`** | **byte** | **UNKNOWN** | **UNKNOWN** | ✗ | – | ✗ |
| 3 `i` | Int, nDef=v → dword | `%d` | dword | `%i` | `atoi` | ✓ | – | ✓ |
| 4 `u` | Int, nDef=v → dword | `%u` | dword(生) | **UNKNOWN** | **UNKNOWN** | ✗ | – | ✗ |
| 5 `f` | Str, nDef=`%f`(v) → `sscanf` | `%.4f` | float | `%.2f` | `atof` | ✓ | – | ✓ |
| 6 `s` | Str, nDef=v → `0x4A7980` | 生文字列 | string | `'%s'` | `0x4A7980` | ✓ | 読=名前先頭→`'s'` | ✓ |
| 7 `r` | Str, nDef=`%u,%u,%u` → pack+α | 3 成分 `"%u,%u,%u"` | – | **UNKNOWN** | **UNKNOWN** | ✗ | – | ✗ |
| **8 `a`** | Str, nDef=`%u,%u,%u,%u` → `0x4A83C0` | 4 成分 `"%u,%u,%u,%u"` | dword(生) | **UNKNOWN** | **UNKNOWN** | ✗ | – | ✗ |

**エンジンの既知の不整合（本タスクの副次発見）**: `GetINISetting`(opid 0x011E) / `SetINISetting`(opid 0x011D) は type 1/2/**4** を **"UNKNOWN TYPE"** として扱うため、**登録済み `u…` 系 21 キーはコンソールから読めず書けない**（`uGridsToLoad:General` 等）。ReadOne/WriteOne（INI 読み書き）側では正しく処理される。

---

## §8 新規残点（V143+ 粒度）

| ID | 残点 | 深刻度 | 試行経路 / 次タスク |
|---|---|---|---|
| **N-1** | **type 2 (`h`) の語源・用途**。挙動は type 4 (`u`) とほぼ同義（ReadOne: Int+dword、WriteOne: `%u`）だが Dump 整形のみ byte 読みで不整合。登録ノード 0 件のため実証不能 | 低 | `.rdata` に `h…:Section` 形の未登録文字列が無いことを `exe.find` で確認するだけ |
| **N-2** | **`GameSetting` コンテナ `0xB35574`（lookup `0x55E000`、vtable 未調査）のノード全数と type 分布**。type 1/2/8 が存在しうる残る唯一の系統 | 中 | `callers(0x55E000)` と `mov ecx,0xB35574` の xref から登録スタブを列挙（`v137` の `FUN_00412D30` 系とは別物の可能性あり、重複注意） |
| **N-3** | **v91 §1.2 のクラス名 2 件 `DefaultCombatStyle` / `Atmosphere` の出所**。COL 連鎖では `RegSettingCollection` / `BlendSettingCollection` に確定したが、別 TD 由来なら対象オブジェクトが別に存在する可能性 | 中 | `.data` 全域を `".?AV"` パターンで走査し TD 総数と対応表を作る |
| **N-4** | **`fDifficultyMinValue:GamePlay`** は本タスクでも未解決（v91 §7 と同一。孤立文字列のまま） | 低 | `.rdata` の前後 64 B をダンプして隣接文字列の断定 |
| **N-5** | **path オーバーフローの実機検証**（folder ≥ 239/243 文字の仮想フォルダで head 破壊 or cookie abort が先か） | 中（移植時に重要） | 動的実行ではなくスタティックに「どの関数が最初に呼ばれるか」= `0x40E090` 経路の起動順（`FUN_0040d4d0` の呼出し元 5 件の呼び出し順）を `callers` で決定 |
| **N-6** | **`s60_io_2_strings_all.txt` の欠落/ずれ**（6 キーが literal 在のに未収録）。s60 系スクリプトの VA 変換か文字列境界条件のバグ | 低 | 同ファイルの 6 VA（`0xA77240` 等）が存在するか目視 |
| **N-7** | **atexit 系シンク `0xA1AAxx` / `0xA235xx` / `0x9E06xx` 初期化テーブルの全数対応表**（`0xB07AD8` 以外にも同型の死蔵オブジェクトがあるか） | 低 | `callers(atexit登録関数)` で全登録点を列挙 |

---

## §9 証拠一覧

### 9.1 中間スクリプト（すべて `%TEMP%\opencode\`、新規・非 git 管理）

| スクリプト | 出力 | 内容 |
|---|---|---|
| `v130_01_type.py` | `v130_01_type.txt` | `0x4A78D0` 本体 + byte 表 + jump 表 + ReadOne/WriteOne 全文 |
| `v130_01b.py` | `v130_01b.txt` | ReadOne/WriteOne/GetINI/SetINI の switch 表（生 dword）+ 書式文字列 |
| `v130_02_consumers.py` | `v130_02_consumers.txt` | 全 switch 表、`callers(0x4A78D0)`=10、`0x4A83C0`/`0x4A83F0`/getter 本体 |
| `v130_03_dyn.py` | `v130_03_dyn.txt` | 動的ノード工場の host・once-guard・引数 push |
| `v130_04_05.py` | `v130_04_05.txt` | `0xB07AD8` 生ダンパ・COL/TD・dtor、frame サイズ探索 |
| `v130_recon.py` / `v130_recon2.py` | `v130_recon.txt` / (`.txt`) | xref / vtable 比較 / path コピー逆アセンブル |
| `v130_06_s60diff.py` | `v130_06_diff.txt` | v91 ↔ s60 突合（604/31/7） |
| `v130_06b_s60count.py` | `v130_06b_s60count.txt` | s60 raw 615 / unique 611 / 36 セクション / 重複 4 |
| `v130_07_final.py` | `v130_07_final.txt` | TD 内蔵名、動的ノード文字列・既定値、残り type 消費者、**EXE 生バイト直検索** |
| `v130_08_misc.py` | `v130_08_misc.txt` | Dump 整形 switch、メッセージ文字列、vtable 比較、CRT 初期化テーブル |
| `v130_09_str.py` | `v130_09_str.txt` | `0x4A7980` 文字列格納本体、Dump type6/default |
| `v130_10_iat.py` | `v130_10_iat.txt` | PE インポート記述子手動パース → IAT 名解決 |

### 9.2 参照した既存成果物

- `tmp/re/v91_ini_defaults.md`（本体・§8 の残点出典）、`v91_ini_defaults.csv`（635 行）
- `tmp/re/s60_io.md`（§2「36 セクション・615 キー」）、`s60_io_ini_keys.txt`、`s60_io_2_strings_all.txt`
- `tmp/re/v137_v80_global_residuals.md`（別系統 `FUN_0041BAE0` Setting 登録スタブ — **本タスクの `FUN_004040E0` 系とは別物**であることを確認、重複なし）
- `tmp/re/s60_*`（path/CSIDL 記述、BlendSettings/Reg の backend 判定）

### 9.3 自己検証

- `python "%TEMP%\opencode\probe_harness.py" selftest` → **4/4 PASS**（作業前）。
- **件数検算**
  - V1: `E8→0x4040E0` 637 = 静的 635 + 動的 2 ✓
  - V2: 動的ノード 2 = once-guard 2 件（float 0xB35260 bit1 / int 0xB3526C bit0）✓
  - V3: コンテナ別 574 + 62 + 1 + 0 = 637 ✓
  - V4: `s60 raw 615 = unique 611 + 重複 4` ✓
  - V5: `635 − 615 = 20 = 31 − 11` ✓
  - V6: セクション別差分合計 `−24 = 611 − 635`、`−24 + 4 = −20` ✓
  - V7: v91-only 31 = A-1(21) + A-2(4) + A-3(6) ✓、s60-only+dup 11 = 4 + 2 + 1 + 4 ✓
  - V8: `0x114 = 4 + 0x104 + 4 + 4 + 4`（オブジェクトレイアウトと RTTI 帰属が整合）✓
- バイナリ・`docs/`・既存 `tmp/re` ファイル・`TODO_re49.md`・`session-memory.json`・`plan.md` は**無変更**。新規作成は本ノート 1 ファイルのみ。
