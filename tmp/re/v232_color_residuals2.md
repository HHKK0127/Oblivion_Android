# v232 — 色/描画残点 第2束（v219 N-1 / N-2）の EXE 実測解明

- 日付: 2026-11-01
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe` (Oblivion v1.2.0416 Steam)
- 出典: `tmp/re/v219_color_residuals.md` §1-6 残点 N-1 / §2 残点 N-2
- 手法: capstone 5.0.7 (skipdata) による EXE 実バイナリ読解＋docs/Oblivion_decompiled.c 部分照合。VA↔fileoff: `.text` = VA-0x400C00 / `.rdata` = VA-0x400E00 / `.data` = VA-0x401400
- selftest: `python "%TEMP%\opencode\probe_harness.py" selftest` → **ALL PASS (4/4)**（着手時）

---

## 項目1 — DebugText band (0x571000-0x574000) に assert TU 文字列が存在しないこと / FUN_005723E0 の TU 直接証拠探索

**判定: 解明（裁定: TU 不在＝release アサート剥落。FUN_005723E0 の TU 直接証拠は取得不可。命名は推定のまま維持）**

### 1-1. band 再走査（push imm32 → .rdata 文字列、全形式列挙）

`0x571000-0x574000` band の `push imm32` 参照 .rdata 文字列は以下の13件のみ（v219 再現確認）:

| 文字列 VA | 文字列 | 参照サイト |
|---|---|---|
| `0xA32DE0` | `LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is %i` | `0x5713c4` |
| `0xA32E38` | `LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i` | `0x5713a7` |
| `0xA32E90` | `LoadGame Buffer underrun ... Currently loading form is %08...` | `0x571374` |
| `0xA32F10` | `LoadGame Buffer overrun ... Currently loading form is %08X` | `0x57132f` |
| `0xA32FD0` | `LoadGame Buffer error: Block Header is incorrect ...` | `0x57109d` |
| `0xA33030` | `LoadGame Buffer error: Block Header is incorrect ...` | `0x57107f` |
| `0xA68734` | `..\\TES Shared\\TempEffects\\BSTempEffectParticle.cpp`（band 内 **唯一の** assert TU） | `0x57107a` / `0x571098` / `0x571327` / `0x57136c` / `0x57139f` / `0x5713bc` |
| `0xA68810` | `Too many unique debug text items. \n` | `0x57261f`（= FUN_005723E0 内） |
| `0xA68834` | `FloatingText` | `0x5729ab` / `0x572a16` |
| `0xA688B0` | `FaderNodeLight` | `0x573160` |
| `0xA688C0` | `FaderNode Above Menus` | `0x57311b` |
| `0xA688D8` | `FaderNode Below Menus` | `0x57310d` |
| `0xA688F0` | `Data\\Textures\\Menus\\Faders\\%s` | `0x573595` |

→ band 内 assert 文字列は **BSTempEffectParticle.cpp のみ**（`0x57107a` 等の xref は LoadGame Buffer エラー報告部。DebugText 部門の TU ではない）。v219 の指摘を再現確認。

### 1-2. .rdata 全体ワイド走査（TU-like 文字列 67 件＋.text xref 全形式）

- .rdata に NUL 終端 TU-like 文字列（`.\path\File.cpp` / `File.cpp` / `*.h` 等）**67 件**（.cpp系 66 / .h系 1。うち `.\` 形式 45 件）。
- 各文字列 VA の **.text 全体 byte-pattern xref**（push / mov imm32 等の命令形式を問わない 4バイト即値走査）:
  - **DebugText band `0x571000-0x574000` への xref: 0 件**
  - **FUN_005723E0 内 (`0x5723E0-0x572850`) への xref: 0 件**
- TU 文字列の実在例（アサート TU はバイナリに残る）: `.\Misc\Main.cpp` (`0xA2FCB8`) / `.\Interface\InterfaceManager.cpp` (`0xA69048`) / `.\AI\Actor.cpp` / `.\AI\CombatController.cpp` (`0xA7035C`) / `.\Magic\ActiveEffect.cpp` 等。→ **TU 文字列機構自体は本バイナリに実在**し、DebugText にだけ無い。

### 1-3. 全ファイル literal 走査（"DebugText" 系・埋め込み ".cpp" 形式）

| 検索 | 結果 |
|---|---|
| `"DebugText.cpp"` / `"FontManager.cpp"` / `"NiNode.cpp"` / `"debugtext"` (v219_dis9 再現) | **0 hits** |
| `debugtext`（大文字小文字無視、EXE 全体） | 20 hits — すべて INI 設定名 (`iDebugText:Display` / `iDebugTextTopBottomOffset:Display` / `iDebugTextLeftRightOffset:Display` / `rDebugTextColor:Menu` / `iDebugTextFont:Menu`)、コンソールコマンド説明 (`sdt`/`TDT`)、RTTI 埋込 (`NiTListBase@V?$DFALL@PAUDebugTextData@DebugText@@...` @`0x7118AE`)、RTTI 名 `DebugTextExtraData`、`InterfaceManager: DebugText Root` — **ソースパス形式は 0 件** |
| `".cpp"` バイト列（EXE 全体・埋め込み形式含む） | 69 hits — 全て Havok 系 (`hkVisualDebugger.cpp` / `.\visualdebugger\viewer\hk*.cpp`) 等。**DebugText 系 TU 0 件** |
| `"DebugText.h"` / `"DebugText.hpp"` / `"DebugTextNode"` / `"NiDebug"` | **absent** |

### 1-4. 裁定

- `FUN_005723E0`（推定 `DebugText::AddText`）の TU 直接証拠（ソースファイル名）は **EXE に存在しない**。
- 本バイナリには assert TU 文字列の機構が実在する（`.\*.cpp` 形式 45 件、Havok 系含め 69 ファイル分）にもかかわらず、DebugText 部門にのみ TU が無い ＝ **release ビルドでのアサート剥落**と裁定。
- → 命名は **推定名 `DebugText::AddText` / `DebugText::CreateTextNode` のまま維持**（RTTI `.?AVDebugTextExtraData@@`・`NiTListBase<DebugText::DebugTextData*>` 埋込・`"Too many unique debug text items. \n"` xref `0x57261f`・シングルトン `DAT_00b3a6a4` 等 v219 の推定根拠に変更なし。新しい反証も無し）。

---

## 項目2 — type8 ('a') の将来挙動検証（代表2経路: FUN_004a8bc0 / FUN_00513e90）

**判定: 解明（下記 2-1〜2-6 で実証。type8 値が将来渡された場合の挙動を両経路で確定）**

### 2-1. FUN_004a8bc0（RegSetValueExA 直渡し）— type8 は `dwType=8` でそのまま登録される

capstone 実読（`0x4A8BC0-0x4A8C46`）＋ decompiled.c L134624-134671 照合:

```
004A8BD4  mov      esi, [ebx+4]        ; esi = key 名
004A8C0C  mov      ecx, ebx
004A8C0E  call     0x4a7a80            ; cbData = size per 先頭文字（'a' → 4）
004A8C13  push     esi
004A8C14  mov      [esp+0x14], eax     ; DStack_48 = cbData
004A8C18  call     0x4a78d0            ; dwType = type dispatch（'a' → 8）
004A8C20  cmp      eax, 1
004A8C23  jne      0x4a8c46            ; type==1 ('c') のみ名前 's' 書換。type8 は素通し
...
RegSetValueExA(hKey, key名, 0, dwType=8, lpData=ノード先頭(&pack値), cbData=4)
```

- `FUN_004a7a80`（cbData）: 先頭文字 jump table `0x4A7AE4`（index = ch-'S'）。`'a'/'f'/'i'/'r'/'u'` → **`mov esi, 4` (`0x4A7AAC`)**、`'b'/'c'/'h'` → 1、`'s'/'S'` → strlen+1（decompiled L133791-133996 と一致）。
- **type8 の挙動: 値ノード先頭の pack DWORD（`u1<<24|u2<<16|u3<<8|u4`、リトルエンディアンメモリ並び `[u4][u3][u2][u1]`）を 4 バイトの生バイナリ blob として、レジストリ値型 8（Win32 定数では `REG_RESOURCE_LIST`）で登録**。型妥当性チェックは無く、`RegSetValueExA` は型8を受理する（失敗しない）。INI 側 WriteOne/ReadOne とは無関係の生バイナリ経路。
- 備考: engine type enum は `b0/c1/h2/i3/u4/f5/s6/r7/a8/unknown9` で、dwType 直渡しの結果として REG 定数と偶発的に重なる（`u`→REG_DWORD=4、`s`→REG_SZ=1 等）。`a`=8 は `REG_RESOURCE_LIST` 相当になる。

### 2-2. FUN_00513e90（GameSetting 表示系コンソールコマンド）— type8 は UNKNOWN TYPE 表示に落ちる

`0x514068` / `0x5141a0`（GameSetting 系2箇所、v219 と同経路）capstone 実読:

```
00514067  push     edx                 ; key 名
00514068  call     0x4a78d0            ; type
00514070  sub      eax, 3
00514073  je       0x5140bf            ; type3 (i) 系
00514075  sub      eax, 2
00514078  je       0x5140a0            ; type5 (f) 系
0051407A  sub      eax, 1
0051407D  je       0x514092            ; type6 (s) 系 → "GameSetting %s >> '%s'"
0051407F  mov      eax, [esi+4]
00514082  push     eax
00514083  push     0xa4d770            ; "GameSetting %s >> UNKNOWN TYPE"
00514088  call     0x579b60            ; コンソール出力
```

- type8 の場合: `8-3=5`(≠0) → `5-2=3`(≠0) → `3-1=2`(≠0) → **フォールスルーして `"GameSetting %s >> UNKNOWN TYPE"` をコンソール出力**（`0x579b60` = コンソール print）。`0x5141a0` 側も同一構造（同一フォーマット文字列 `0xA4D770`）。
- → **type8 値を読ませてもクラッシュせず、値は一切文字列化されず UNKNOWN TYPE メッセージのみ**。`%u,%u,%u,%u` 書式は使われない。

### 2-3. INISetting 系2経路（0x5142c9 / 0x514460）— type8 は jump table 範囲外 default → UNKNOWN TYPE

```
005142C9  call     0x4a78d0
005142D1  cmp      eax, 6
005142D4  ja       0x514366            ; type>6 は jump table 外（type8 はこれに該当）
005142DA  jmp      dword ptr [eax*4 + 0x5143a0]   ; table は type0-6 のみ
...
00514366  lea      ecx, [esp+0x14]
0051436B  push     0xa4d7ac            ; "INISetting %s >> UNKNOWN TYPE"
00514370  jmp      0x51437c
0051437C  call     0x579b60            ; コンソール出力
```

- `0x514460` 側も同一構造（`cmp eax,6 / ja 0x5144a4`、table `0x5144E4`、ja-default `0x5144AC` も同一文字列 `0xA4D7AC`）。
- jump table 実値: `0x5143A0` = `[type0=0x514343, 1=0x514366, 2=0x514366, 3=0x5142e1("INISetting %s >> %i"), 4=0x514366, 5=0x514302, 6=0x514325]`（type1/2/4 も UNKNOWN TYPE 参照先）。`0x5144E4` = `[0x514474(int), 0x5144a4, 0x5144a4, 0x514474, 0x5144a4, 0x514485(float), 0x514496(string)]`。
- → **type8 は INISetting 表示でも `"INISetting %s >> UNKNOWN TYPE"` 出力に確定**（値の解釈・クラッシュなし）。4表示経路すべてで type8 = UNKNOWN TYPE。
- ※ Ghidra の `FUN_00513e90` (0x513E90-) は複数コンソールハンドラを跨ぐ巨大関数として抽出されており、`0x5142c9`/`0x514460` も enclosing = FUN_00513e90（enclosing bisect 再確認）。

### 2-4. FUN_004a7b10（0x4a7b7c default 経路）と FUN_004a8cf0 帯（RegQuery 期待型）— 参考

- `FUN_004a7b10` (`0x4A7B10`, decompiled L133808): switch(type) で `case 0` (b) → `FUN_0070f950`、`case 1/2` (c/h) → `FUN_0070f9b0`、`case 3` (i) → `FUN_0070faf0`、**`default`（type4/5/6/7/**8**/9）→ `FUN_0070fb40`**（浮動小数点書式化系）。type8 は default = 浮動小数点扱いに落ちる（pack DWORD 値を float として解釈する末節経路。実登録0件のため未使用）。
- `0x4A8D78` (`call 0x4a7a80` → `0x4A8D85` `call 0x4a78d0`) 帯: type→REG 期待型変換 `test eax,eax / je +3 / sub eax,6; neg eax; sbb eax,eax; and eax,3; add eax,1` — **type8 の場合の計算結果は 4（REG_DWORD 期待）**。書き込み側が型8 blob で書くのと期待型4が食い違うが、'a' 登録が0件のため実害なし（将来 'a' 登録追加時は読み戻し系で型不一致となり得る — 新規残点 N-3 参照）。※ decompiled.c の `FUN_004a8cf0` は RegCloseKey ラッパのみ抽出されており、`0x4a8d85` 帯は Ghidra 抽出失敗域（capstone 実読で補完）。

### 2-5. 書式 `%u,%u,%u,%u` の帰着（再確認）

- ReadOne type8 head `0x4A89F6`: `GetPrivateProfileStringA` (IAT `0xA280B4`) → `sscanf "%u,%u,%u,%u"` (`0xA40520`) → `cmp eax,4; sete bl` → pack `0x4A83C0`。
- WriteOne type8 `0x4A8728`: expand `0x4A83F0` → `sprintf "%u,%u,%u,%u"` (`0xA40520`)。
- pack `0x4A83C0` = `u1<<24|u2<<16|u3<<8|u4`（実読: `shl/or` 4段）、expand `0x4A83F0` = `(v>>24),(v>>16)&0xff,(v>>8)&0xff,v&0xff`（実読: `shr/movzx`）。
- RegSettingCollection vtable `0xA404F8`: `+0C=0x4A85F0 (WriteOne) / +10=0x4A8800 (ReadOne)`（再現確認）。
- → **INI ファイル経路（WriteOne/ReadOne）でのみ `%u,%u,%u,%u` 4値形式**。レジストリ直書き経路 (2-1) には書式化が介在しない（生4バイト）。ゲーム設定表示経路 (2-2/2-3) には型8の書式化経路が存在しない。

### 2-6. 総括（将来 'a' 登録時の挙動）

| 経路 | type8 値が渡った場合の実挙動 |
|---|---|
| INI WriteOne (`0x4A85F0`→`0x4A8728`) | pack DWORD を expand → `"u1,u2,u3,u4"` 10進4値文字列として INI に書出 |
| INI ReadOne (`0x4A8800`→`0x4A89F6`) | `"%u,%u,%u,%u"` で4値 sscanf → pack DWORD に復元（4値そろわなければ成功フラグ false、値は 0 初期化のまま） |
| レジストリ書出 FUN_004a8bc0 | `dwType=8` (REG_RESOURCE_LIST)・cbData=4 の生 blob で登録。文字列化なし |
| GameSetting 表示 FUN_00513e90 (0x514068/0x5141a0) | `"GameSetting %s >> UNKNOWN TYPE"` コンソール出力のみ。クラッシュなし・値非表示 |
| INISetting 表示 (0x5142c9/0x514460) | jump table (type0-6) 範囲外 default → `"INISetting %s >> UNKNOWN TYPE"` コンソール出力。値解釈なし・クラッシュなし |
| `FUN_004a7b10` default | `FUN_0070fb40`（浮動小数点書式化系）に到達する末節経路 |

---

## 新規残点

| ID | 内容 |
|---|---|
| N-3 | レジストリ書き出し (FUN_004a8bc0) は type8 を `dwType=8` blob で書くが、読み戻し系 `0x4A8D85` 帯の期待型計算は type8 → 4 (REG_DWORD) を算出する。将来 'a' キーを登録した場合の RegQueryValueExA 型不一致時のフォールバック動作（値破棄か生読みか）は未検証 |

---

## 検証情報

- selftest: `python "%TEMP%\opencode\probe_harness.py" selftest` → **ALL PASS (4/4)**（着手時）
- スクリプト: `%TEMP%\opencode\v232_tu_scan.py` / `v232_tu_scan2.py` / `v232_type8_consumers.py` / `v232_type8_extra.py`（capstone 5.0.7 skipdata、PE 実読）。v219 中間物 `v219_dis9.py` 再実行（enclosing/imm32 スキャン再現）
- 証拠出力: `%TEMP%\opencode\v232_tu_scan_out.txt` / `v232_tu_scan2_out.txt` / `v232_type8_out.txt` / `v232_type8_extra_out.txt` / `v232_dis9_out.txt`
- docs/Oblivion_decompiled.c: L133653 (FUN_004a78d0) / L133767 (FUN_004a7a80) / L133808 (FUN_004a7b10) / L134624 (FUN_004a8bc0) / L206159 (FUN_00513e90) / L275830 (FUN_005723e0) — **読取のみ**
- v219 本体 / v209 本体 / 既存 tmp/re 成果物: **読取のみ・上書きなし**（本ファイル v232 は新規）
