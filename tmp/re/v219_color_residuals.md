# v219 — 色/描画系 残点4件の EXE 実測解明

- 日付: 2026-11-01
- 対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe` (Oblivion v1.2.0416 Steam)
- 出典: `tmp/re/v209_rdebugtextcolor.md` §6（残点4件定義）、`tmp/re/v91_ini_defaults.md`（項目4 対象）
- 手法: capstone 5.0.7 (skipdata) による EXE 実バイナリ読解。VA↔fileoff: `.text` = VA-0x400C00 / `.rdata` = VA-0x400E00 / `.data` = VA-0x401400
- selftest: `python "%TEMP%\opencode\probe_harness.py" selftest` → **ALL PASS (4/4)**

---

## 項目1 — 0x571900 の呼び出し元 0x57273E の描画系命名

**判定: 解明（推定名を確定。呼び出し元は DebugText 系テキスト描画）**

### 1-1. 0x571900 の呼び出し元

- `0x571900` の caller は **`0x57273E` の1件のみ**。
- `0x57273E` は **`FUN_005723E0`** の内部（本体 0x5723E0〜、ret 0x1C = 7引数）。

### 1-2. FUN_005723E0 = `DebugText::AddText`（推定名）の根拠

| 証拠 | 内容 |
|---|---|
| RTTI | `.?AVDebugTextExtraData@@`（vtable `0xA687BC`、21スロット、ctor `FUN_00571C50` が vptr 書込、vtable[1] = `FUN_00571CD0` = `mov eax,0xb3a6a8; ret`）。登録シンク `0x9FAD20` が `push 0xb3fd44 / push "DebugTextExtraData" / mov ecx,0xb3a6a8 / call 0x70E220` |
| 文字列 | `"Too many unique debug text items. \n"` (`0xA68810`) → xref **`0x572620`** = FUN_005723E0 内 |
| シングルトン | `FUN_00571F90` が new(0x15F0) → `DAT_00b3a6a4`。ctor `FUN_00571E80` = 0xC8(200)×0x1C の DebugTextData 配列＋末尾 NiTList（+0x15E0 vtable=`0xA687A4`、+0x15E4/E8/EC） |
| NiTList vtable | `0xA68784` / `0xA68794` / `0xA687A4` = `DebugText::PAUDebugTextData` 系 |

### 1-3. FUN_00571900 = `DebugText::CreateTextNode`（推定名）

- `new(0xDC)` → `FUN_0070B780` が vtable `0xA7E38C` = `.?AVNiNode@@` を書込（＝NiNode を生成）。
- 色を **`0xB12DAC` (rDebugTextColor)** から、フォント index を **`0xB12DB4` (iDebugTextFont = 3)** から取得。
- `FUN_00576A30`（`DAT_00b3a6bc`、0x18 alloc）の配列 `[idx*4-4]` でフォント情報取得 → `FUN_00575870` でテキストジオメトリ生成。
- `InterfaceManager`（`FUN_00582160` → `0xB3A6E0`、size 0x134、ctor `0x5802B0`＝v204 と一致）**+0x70 の "DebugText Root" に Attach（vtable+0x84）**。
- 門番: `FUN_00585C10(1)` → +0x31 > 0 または IfaceMgr+0x50。

### 1-4. AddText 签名と呼び出し網羅

- 签名: `(this, text, x, y, mode, -1, -1.0, font=0→DAT_00b12db4)`（ret 0x1C = 7引数）。
- 定数: `0xA4D6FC` = 640.0f、`DAT_00a30634` = -1.0f。
- **callers 24件**: `0x513f89` / `0x57b93b` / `0x57ca8c` / `0x582955` … `0x585799` 等。

### 1-5. 周辺関数群 naming

| アドレス | 推定名 | 根拠 |
|---|---|---|
| `FUN_00572170` | `DebugText::Update` | `FUN_005821F0` の `0x583975` から呼叫 |
| `FUN_00572850` | `DebugText::FloatingText 生成` | `"FloatingText"` (`0xA68834`) xref `0x5729AB`/`0x572A17`。new(0x20) → `FUN_00571C50`(DebugTextExtraData ctor)。callers `0x586495` / `FUN_00586000` |
| `FUN_00575870` | `テキストジオメトリ builder` | callers `0x571AF2` / `0x572ADF` / `0x59D2B0` |
| `FUN_00571E80` | `DebugText ctor`（200スロット配列） | 1-2 参照 |
| `FUN_00571F90` | `DebugText singleton 取得/作成` | new(0x15F0) → `DAT_00b3a6a4` |
| `FUN_00571C50` | `DebugTextExtraData ctor` | RTTI vptr 書込 |
| `FUN_00977c50` | no-op 返1（`mov al,1; ret`） | AddText が push 3 / push 2 で呼叫 |

### 1-6. コンソールコマンド連携（描画系呼び出しの入口）

- `TDT` (0x010E) @`0x501000` = `FUN_0057C200`（InterfaceManager+0x50 フラグ反転、前提 +0x1c != 0）/ `FUN_005792E0`（同フラグ読出）→ 文字列 `"Debug Text -> %s"` (`0xA4B938`, xref `0x501023`)。
- `sdt` (0x0166) @`0x5081F0` = `DAT_00B02E0C` 書込。
- `"InterfaceManager: DebugText Root"` (`0xA691B4`) → xref `0x581EC2` = `FUN_00581CC0`（InterfaceManager 初期化、param+0x70 に DebugText Root NiNode）。
- TU 文字列 `.\Interface\InterfaceManager.cpp` (`0xA69048`) → xref `0x57EE7D` / `0x57EFF2` = `FUN_0057ED20`。

**残点（新規）N-1**: `0x571000-0x574000` band の assert TU 文字列は `BSTempEffectParticle.cpp` のみで、DebugText band 自体に TU 文字列が無い。→ FUN_005723E0 の TU 直接証拠は未取得（命名は RTTI/文字列/xref からの推定に留まる）。

---

## 項目2 — type 8 ('a') の ReadOne 分岐実装と理論的消費者

**判定: 解明（分岐は EXE に実在。ただし INI 登録ノード0件で現状 dead code）**

### 2-1. type dispatch（`0x4A78D0`）

- byte テーブル `0x4A7954`、index = 名前[0] - 'S'。
- **'a' (idx 14) = 0x01 → jump[1] = `0x4A7923` = `mov eax,8`** → **type 8** を確定。

### 2-2. ReadOne（`FUN_004A8800`）jump table `0x4A8AE4`

- **`[8] = 0x4A89F6`**:
  1. `GPPStringA` 默认 `""` (`0xa2f7ec`)
  2. `sscanf` **`"%u,%u,%u,%u"`** @`0x4A8A3F` / 文字列 `0xA40520`
  3. `cmp eax,4; sete bl`（4値そろっていれば成功フラグ）
  4. push 4値 → `call 0x4A83C0`（**pack: `u1<<24 | u2<<16 | u3<<8 | u4`**）
  5. `add esp,0x28` → `jmp 0x4A8AB3` → `mov [esi],eax`、`al=bl`、ret 4

→ **実装は `%u,%u,%u,%u` の4値スキャン＝確定**（RRGGBB を4コンポーネントで読む形式）。

### 2-3. WriteOne（jump table `0x4A87D4`、index = type-1）

- **`[7] = 0x4A8728`**（type 8 → idx 7）: expand **`0x4A83F0`** で4ワード展開 → `sprintf` **`"%u,%u,%u,%u"`** @`0x4A875F`。
- pack `0x4A83C0` / expand `0x4A83F0` のアセンブラ実読で逆対応を確認（書側も type 8 対応）。

### 2-4. 理論上の消費者（type dispatch の呼び出し10件）

| 呼び出し元 | 挙動 |
|---|---|
| `0x4a8859` (ReadOne 0x4A8800) | type 8 読取（2-2） |
| `0x4a8648` (WriteOne 0x4A85F0) | type 8 書出（2-3） |
| `0x4a7b7c` (FUN_004a7b10) | switch default → `FUN_0070fb40` |
| `0x4a7d9b` (FUN_004a7d60) | type 3/5/6 のみ扱い、**8 は無視** |
| `0x4a8c18` (FUN_004a8bc0) | `RegSetValueExA` に **dwType = type を直渡し** |
| `0x4a8d85` (FUN_004a8cf0) | type → サイズ変換 |
| `0x514068` / `0x5141a0` / `0x5142c9` / `0x514460` (FUN_00513e90) | type 3/5/6 のみ。他は `"GameSetting %s >> UNKNOWN TYPE"` |

- `RegSettingCollection` vtable `0xA404F8`: **+0C = `0x4A85F0` (WriteOne)、+10 = `0x4A8800` (ReadOne)**。

### 2-5. INI 登録実態（'a' キー = 0件）

- `GetPrivateProfileStringA` 呼び出しは **`FUN_004a8800` (ReadOne) 内のみ4件**（`0x4a88a7` / `0x4a894a` / `0x4a8999` / `0x4a8a16`）。`GPPIntA` は5件（`0x40dfce` / `0x4a88d9` / `0x4a8a8d` / `0x4a8aad` / `0x53ac86`）。
- v91 CSV 635件の型分布: **f238 / b173 / i103 / s99 / u21 / r1、type8 = 0件**。
- `copilot_ini_registries.txt` に `^\s+a` 行なし、`.rdata` に `a*:Section` 文字列 0件。

→ **type 8 分岐は実装として存在するが、現行 INI 登録では使用されない（dead path）**。

---

## 項目3 — bare hex 6桁経路（移植 sHexColor 拡張）が EXE に無いことの裏取り

**判定: 解明（不在＝EXE に bare hex 経路は構造的に存在しない）**

### 3-1. リテラル/フォーマット走査

| 検索 | 結果 |
|---|---|
| `"sHexColor"` / `"HexColor"` / `"RRGGBB"` | **0 hits** |
| `"%02x"` / `"%06x"` / `"%02X"` / `"%x%x%x"` | **0 hits** |
| INI band `0x4A7800-0x4A8F00` の push-imm 文字列 | `'%d'`, `'%u'`, `'%u,%u,%u,%u'`, `'%u,%u,%u'`, `'%.4f'`, `'%f'`, `'MAIN'`, `'INISettings'`, `'Total INISettings'`, `'RegSettings'`, `'Total RegSettings'` のみ — **`%x` 一切なし** |
| `.text` 全体の `%x` 系フォーマット | 12種のみで全て無関係サブシステム（`FUN_008a9100`, `FUN_008bb6a0`, `FUN_007a9e10`, `FUN_0089fb70`, `FUN_008a0d20`, `FUN_00498340`, `FUN_00947fa0` 等） |
| standalone `'%x'` @`0xA8ABB9` / `0xA9830A` / `0xA97962` | `.text` 参照 **0件** |

### 3-2. パーサ実測

- `sscanf` `%u`: `"00FF80"` → **1変換**（10進解釈、hex 解釈されない）、`"0x00FF80"` → 1変換。
- `GetPrivateProfileIntW`: `"0011FFFF"` → **11**（10進で F で停止）、`"0x00FF80"` → **0xFF80**（OS API は `0x` 接頭辞のみ hex 対応、**bare hex 非対応**）。
- `strtol` / `strtoul` **インポートなし**。
- `"Forward"` 文字列 xref `0x9F607D` = `FUN_009f6040` のアクション登録（`FUN_00419a70("sUActnForward","Forward")`）で **INI 読み取りではない**。

### 3-3. 結論

- 彩色 type 7/8 は `sscanf %u` のみ使用（`GPPIntA` の `0x` 解釈は int/bool 系に限定）。
- → **色値の bare hex 6桁経路は EXE に存在しない**。移植側 sHexColor 拡張は Android移植独自の拡張であり、EXE 互換経路ではない。

---

## 項目4 — v91:50 の 0xAARRGGBB 注記訂正ドラフト

（v91 本体は**読取のみ**。以下は before → after のドラフト提示のみ）

### before（現行 v91 行50）

```markdown
| `r` | 7 | 1 | 色 0xAARRGGBB（`rDebugTextColor:Menu`=0xFFFBE9FF） |
```

- 対象ファイル: `tmp/re/v91_ini_defaults.md:50`（handover コピー `tmp/re/handover_20261025/deliverables/v91_ini_defaults.md:50` も同文）
- 行51（参考・変更なし）: `| `a` | 8 | 0 | 同上 |`

### after（訂正案）

```markdown
| `r` | 7 | 1 | 色 0xRRGGBBAA（`rDebugTextColor:Menu`=0xFFFBE9FF → 下位8bit=AA=FF、上位24bit=RRGGBB=FFFBE9。EXE 側 type7 の pack は `%u` 4値経路と併用され、v209 §4B の RRGGBBAA 判定に一致） |
```

### 訂正根拠

- v209 §4B の既存 after 案（RRGGBBAA）と整合。
- 項目2-2/2-3 の pack/expand 実装（`u1<<24|u2<<16|u3<<8|u4` = 最上位バイトが先頭グループ）および v209 本体の消費側 `0x571900` 実測と矛盾しない。
- **訂正ドラフト件数: 1件**（v91:50 の1行のみ。行51 は「同上」のため訂正不要だが、行50 訂正後は文言追随を推奨）。

---

## 新規残点

| ID | 内容 |
|---|---|
| N-1 | DebugText band (`0x571000-0x574000`) に assert TU 文字列が存在せず、`FUN_005723E0` の TU（ソースファイル名）直接証拠が未取得。命名は RTTI/文字列/xref からの推定に留まる |
| N-2 | type 8 ('a') は実装存在も登録ノード0件で dead。将来 'a' 登録を追加する場合の挙動（RegSetValueExA 直渡し等）は未検証 |

---

## 検証情報

- selftest: `python "%TEMP%\opencode\probe_harness.py" selftest` → **ALL PASS (4/4)**（着手時）
- スクリプト: `%TEMP%\opencode\*.py`（capstone 5.0.7 skipdata、PE 実読）
- 本成果物: `tmp/re/v219_color_residuals.md`（新規・UTF-8）
- v91 本体 / v209 本体 / 既存 tmp/re 成果物: **読取のみ・上書きなし**
