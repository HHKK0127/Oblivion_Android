# V208: s50 GMST dump 網羅性の再走査 — 「`<設定名>:<セクション名>` 混在名」全数抽出と登録経路判定

- 日付: 2026-10-30 / 対象: `Oblivion.exe` v1.2.0416 Steam（7,898,624 B, D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe）
- 出典依頼: `tmp/re/relcheck_20261029/rel6_gmst3way.md` §6-2/§5-(d)（読取のみ）
- 手法: EXE 全域バイト走査（PE セクションマップ: .text VA 0x401000/raw 0x400, .rdata 0xA28000/0x627200, .data 0xB02000/0x700C00, .tls/.rsrc/.bind も解析に含む）+ capstone 5.0.7 (skipdata) 逆アセンブル。ESM/ESP 本体は不使用。
- スクリプト: `%TEMP%\opencode\v208_scan.py`（主走査）/ `v208_b.py` `v208_c.py` `v208_d.py` `v208_e.py` `v208_f.py` `v208_g.py` `v208_h.py` `v208_i.py`（登録経路・残務の裏取り）
- 抽出結果 CSV: `%TEMP%\opencode\v208_mixed_names.csv`（604 行 + ヘッダ, 299,205 B）

## 0. 結論サマリ（先に読むところ）

1. **`<Name>:<Section>` 混在名は EXE 内に 620 件が実登録**（主走査 600 + 命名規則外 20）。既存 s50 の 2,304 bare 名との重複（別名）は **0 件**。
2. **`fGlobalTimeMultiplier:General` は (b) 新規混在名に含まれる**（登録確認済み、node 0xB06704 / 文字列 0xA3D150）。
3. **登録経路は GameSetting map ではない。** 混在名はすべて **INI 既定値レジストリ（FUN_004040E0 "INI_REG_ADD" 系）** に登録される。GameSetting map（コンテナ 0xB35574, insert FUN_00412D30, 経路 FUN_00419A70/FUN_0041BAE0）には **':' を含むキーが 1 件も登録されない**（登録呼び出し 1,312 + 992 箇所を全数走査、':' 名 0 件）。':' は分割されず、"Key:Section" 全文が 1 つのノード名として保持される。
4. **したがって rel6 §6 の「s50 の網羅性は 2,304+1」は前提誤り。** `fGlobalTimeMultiplier:General` は GameSetting-map GMST ではなく INI レジストリノードであり、s50（GameSetting dump, 2,304 件）は **訂正不要・2,304 のまま**。訂正が必要なのは rel6 §6 / `RE_EXE_REPORT.md` §43 側の分類（INI レジストリを GMST と誤称）。

## 1. 混在名の全数抽出結果

### 1.1 主走査（名前が `^[fibs][A-Z]` 命名規則、または既知 editor-ID）

EXE 全域の `:` について左右を [A-Za-z0-9_] で拡張し、前半が既知 s50 名 2,304 件または `^[fibs][A-Z]`、後半が既知 INI セクション名（registries/defaults 成果物の 39+54 セクション統合）に一致するものを抽出 → **604 件**。全件について文字列 VA の dword 参照・ノード構造（value + name* の 8B ペア）・逆アセンブルで calls を収集。

| 分類 | 件数 | 内容 |
|---|---:|---|
| (a) 既存 2,304 件の別名（重複） | **0** | name_in_s50 = False が 604/604。bare 名集合と混在名集合は素の部分集合ですら交わらない（名前空間が別） |
| (b) 新規の混在名（登録あり） | **600** | 599 件は自動分類 b-ini（FUN_004040E0 呼び出し検出）+ `fAlpha:Water` 1 件を wrapper 解析で再分類（下記 2.3） |
| (c) 登録外 | **4** | 下記 1.3 |

### 1.2 (b) 600 件の内訳（要点）

- **全件が INI 既定値レジストリ経由**（FUN_004040E0 直接、または float wrapper FUN_00404850 / dword wrapper FUN_0044060 経由）。node は `.data` の静的 8B 構造 `{union value; char* "Key:Section"}`、または wrapper による実行時初期化構造。
- `fGlobalTimeMultiplier:General` を含む **583 件は `copilot_ini_registries.txt` ∩ `copilot_ini_defaults.txt` に既出**（既知 INI キー）。6 件は registries 成果物のみ、10 件は成果物に無いが EXE では登録確認（後述 bLightAttenuation）。
- セクション分布は INI 既定値と同一（General, Audio, GamePlay, Display, LOD, Menu, Interface, Controls, Fonts, LoadingBar, Combat, HAVOK/Havok, Weather, Water, SpeedTree, BackgroundLoad, PathFinding/Pathfinding, DistantLOD, Debug, Landscape, Grass 等）。
- **bLightAttenuation セクション 12 件**（v130 A-1 で「セクション白名簿外」とされた群）: 主走査 10 件（bUseConstant/bUseLinear/bUseQuadratic/bOutQuadInLin/fConstantValue/fFlickerMovement/fLinearRadiusMult/fLinearValue/fQuadraticRadiusMult/fQuadraticValue, VA 0xA4332C–0xA434A0, node 0xB08138–0xB08190）+ 命名規則外 2 件（uLinearMethod 0xA433B0 / uQuadraticMethod 0xA433D0, node 0xB0815C/0xB08164）。全件 INI_REG_ADD 呼び出し確認済み。`copilot_ini_registries.txt` に無いのは同成果物が "skipped: bad section: bLightAttenuatio 12" と明記して **あえて除外**したため（実登録あり）。

### 1.3 (c) 登録外 4 件（全件 dword 参照 0 = 未参照リテラル）

| 名 | VA | 判定 |
|---|---|---|
| `sToLoad:General` | 0xA3D7C9 | **文字列境界汚染**: `uGridsToLoad:General`（0xA3D7C4, 登録済）の +5 B スタート地点を拾っただけ |
| `iFaceGenMaxEGMDataSize:General` | 0xA64989 | 未参照。直前 byte が 'u'（`uiFaceGenMaxEGMDataSize:General` 0xA6499F 派生も未参照）。INI ファイル解析用の残骸リテラル |
| `iFaceGenMaxEGTDataSize:General` | 0xA649A9 | 同上（派生 `uiFaceGenMaxEGTDataSize:General` 0xA649BF も未参照） |
| `sS:R2` | (0xBDC324) | `.bind` セクションのバイナリ（AVX2 データ）内で偶然成立した ASCII。前後は 'None' + ガーベジ。偽陽性 |

### 1.4 命名規則外の混在名（`^[fibs][A-Z]` に該当せず主走査から漏れる群）

`.rdata`/`.data` から後半が既知セクションのものを全数列挙 → **37 件**。内訳:

- **実登録 INI キー 20 件**（すべて既存成果物に収載、または bLightAttenuation 除外分）:
  `uGridsToLoad` `uGridDistantCount` `uGridDistantTreeRange` `uGridDistantCountCity` `uGridDistantTreeRangeCity`:General /
  `uVideoDeviceIdentifierPart1`〜`Part4`:Display /
  `uSurfaceTextureSize` `uSurfaceFrameCount` `uSurfaceFPS` `uNearWaterRadius` `uNearWaterPoints` `uNumDepthGrids` `uDepthRange`:Water /
  `iOpenMPLevel` `iThreads`:OPENMP /
  `uLinearMethod` `uQuadraticMethod`:bLightAttenuation
  （`uSurfaceTextureSize:Water` は dword 参照 0xA3F4C4→0x49DD5E で FUN_00444060 wrapper 経由の登録を実確認）
- **未参照・汚染・ノイズ 17 件**: `W` `H` `Screen` `X` `Y` `Shaders`:Display（dword 参照 0）、`Joystick` `Keyboard` `Mouse`:Controls（0）、`Buffer` `Enable`:General（0）、`AbUseMultiThreadedTrees`（bUse... の 'A' 汚染）、`CfGlobalTimeMultiplier`（'C' 汚染 — defaults.txt 成果物自体がこの汚染variant "CfGlobalTimeMultiplier:General va=0x00A3D14F refs=0" を記録しており、実体は 0xA3D150 の fGlobalTimeMultiplier に 1 B 先行する残骸文字）、`HbDisplayLODBuildings`（'H' 汚染）、`AfLandFriction`（'A' 汚染）、`uiFaceGenMaxEGM/EGTDataSize`（'u' 汚染）、`rDebugTextColor`（'r' 汚染）、`zDbDialogueSubtitles`（'z' 汚染）、`TtWatchDog`（'Tt' 汚染）、`Z:GBn`（.data バイナリノイズ）→ すべて (c) 登録外。

**混在名の実登録総数 = 600 + 20 = 620 件**（うち bLightAttenuation 12、既知成果物収載 583+20−12 ほか）。

## 2. 登録経路の判定（EXE コード根拠）

### 2.1 GameSetting map 側（s50 2,304 件の正体）— ':' は絶対に入らない

- s50 の static GMST スタブ実体（例 0x9D9D60 `sMagicEffectItemFor`）:
  ```
  push 0xA325E8  ; default value
  push 0xA325D4  ; name (bare)
  mov  ecx, 0xB33460      ; Setting スロット
  call 0x419A70           ; FUN_00419A70
  ```
- **FUN_00419A70**（int/dword+string 版 Setting 登録）: `*this = arg1; this[1] = name; test name; mov ecx, 0xB35574; call 0x55E000`（重複検査）→ 重複時 "Setting key '%s' already used..." (0xA32490) → `mov ecx, 0xB35574; call 0x412D30`（MAP_INSERT）。
- **FUN_0041BAE0**（float 版, v137 記載）: `fstp [esi]; [esi+4]=name; 同様に 0xB35574 へ`。v137 の記述は float 版のみで、s50 static スタブ群は実際には **FUN_00419A70** を呼ぶ点が新事実（両者とも 0xB35574 挿入で同系統）。
- **全数走査**: CALL rel32 検出で FUN_00419A70 callers **1,312 箇所**（push された name 1,714 ユニーク）、FUN_0041BAE0 callers **992 箇所**（1,091 ユニーク）。**両者に ':' を含む name は 0 件**。FUN_00412D30 直接 callers 47 箇所も全て bare 名（GLOB 群 GameYear/TimeScale 等、FootSound 群）。
- GameSetting Get/Set（0x514068/0x5141A0）の型解決 FUN_004A78D0 は**先頭文字**（f/i/s/b/S…）の jump table で型を判定するのみで、**':' の分割処理は存在しない**。

→ **GameSetting map（0xB35574）のキーは bare 名のみ。':' を含むキーは登録されない。**

### 2.2 混在名側（INI 既定値レジストリ）— ':' は分割されない

- **FUN_004040E0 (INI_REG_ADD)** 本体: `cmp [ecx+0x110],0; lea edi,[ecx+0x10C]; … node[4]（name*）同士を FUN_0098262D で比較（重複時 0xA2FAB0 のメッセージ）→ 重複なければ FUN_00446CB0 でリスト追加。` **name は"Key:Section" 全文のまま比較・保持され、':' でのセクション分離は行わない**（INI 書込時にのみ文書形式へ展開される設計）。
- 登録 site パターン: `push <node> (8B 構造) ; mov ecx, <registry 0xB07BF0/0xB11C40/0xB07CFC…> ; mov [esp+10],0 ; call 0x4040E0`（CRT 初期化テーブル経由）。`mov ecx,0xB07BF0` 出現は **606 箇所**。
- **fGlobalTimeMultiplier:General の登録経路（確定）**:
  - 静的ノード **0xB06704** = `{0x3F800000 (1.0f); 0xA3D150 → "fGlobalTimeMultiplier:General"}`（[0xB06708]=name* を実測）。
  - CRT 初期化スタブ 0x9DC2D0: `push 0xB06704; mov ecx,0xB07BF0; mov [esp+10],0; call 0x4040E0`（=INI_REG_ADD, site 0x9DC302 — registries.txt 成果物の node=0x00B06704 site=0x009DC302 と一致）。
  - 副経路: 生成スタブ 0xA18690 経由で REG2（0x982FB4）→ 別レジストリ 0xB07CFC にも登録。0x9AF430 にも `MOV ECX,0xB06704; JMP 0x403BC0`（name 分類ヘルパ: [ECX+4] の先頭が 'S' なら free）。
  - 消費: 0x47D247 `fmul [0xB06704]`（dt 乗算、直接読み出し）。直接書き込み命令は無い（rel6/§43 の観測と一致）。
- **注意すべき wrapper 系**: 静的ノードを持たず実行時初期化する例 — `fAlpha:Water` は 0x49D534 で `push 0xA3F43C; mov ecx,0xB3524C; call FUN_00404850`（float wrapper: `[esi]=f32; [esi+4]=name; mov ecx,0xB07BF0; call 0x4040E0`）。`uSurfaceTextureSize:Water` は 0x49DD5D で FUN_00444060（dword 版 wrapper、同様に 0xB07BF0 へ）。**site-pattern 走査（push node; mov ecx,reg; …）だけの `copilot_ini_registries.txt` は wrapper 経由の登録を取りこぼす**（fAlpha:Water が registries.txt に無く defaults.txt にあるのはこのため）。

### 2.3 rel6 §6 / RE_EXE_REPORT §43 との相違（文档 vs コードの不整合）

| 文書の主張 | EXE コードの実態 |
|---|---|
| §43/rel6§6: `fGlobalTimeMultiplier` は「**GMST**（Game Setting）」 | INI 既定値レジストリ（FUN_004040E0, registry 0xB07BF0）のノード。GameSetting map (0xB35574) には未登録。Oblivion.ini の `[General] fGlobalTimeMultiplier` と同系統（gmst_defaults.h / oblivion_ini_defaults.cpp の「双系統」は、実体としては **INI 単系統 + 移植側が GMST 扱い**）|
| §43.3:「GMST 名は `<設定名>:<セクション名>` 形式」 | `Key:Section` 形式は **INI レジストリのノード命名規則**。GameSetting map のキーは bare 名のみ（1,312+992 登録点で ':' 名 0 件）|
| rel6 §6: 「s50 の網羅性は **2,304+1**」 | 前提誤り。`fGlobalTimeMultiplier:General` は GameSetting dump (s50) の母集団外。**s50 は 2,304 のまま訂正不要** |
| rel6 §6 引用 §43.2 disasm: `CALL 0x004000E0` | バイト列 `e8 d9 7d a2 ff` @0x9DC302 の実ターゲットは **0x004040E0**（文書の 0x4000E0 は誤記）|
| v137:「FUN_0041BAE0 = Setting 登録」 | float 版のみ正确。s50 static スタブの実体は **FUN_00419A70**（dword+string 版）。両者 0xB35574 挿入で同系統・重複なし |

## 3. 網羅性の最終値

| 母集団 | 最終値 | 訂正要否 |
|---|---:|---|
| **s50 = GameSetting map 登録（bare 名）** | **2,304 件（変更なし）** | **訂正不要**。':' 混在名は GameSetting map に 1 件も無いため、「2,304+1」は成立しない。s50_gmst_full.txt の網羅性（GameSetting 系として）は完全 |
| INI 既定値レジストリ混在名（EXE 実登録） | **620 件**（主走査 600 + 命名規則外 20） | s50 系成果物には追加不要。`copilot_ini_registries.txt`（607 キー + 明示スキップ 12+3）/ `copilot_ini_defaults.txt`（623 キー, Standalone 版）が既存カバレッジ。**追加が必要なら registries.txt 側への「bLightAttenuation 12 件 + wrapper 経由分」補遺**（本タスクでは成果物編集禁止のため未実施）|

## 4. 成果物

- 本ファイル: `tmp/re/v208_gmst_mixed_names.md`（新規）
- CSV: `%TEMP%\opencode\v208_mixed_names.csv`（604 行 + ヘッダ / 299,205 B。列: classification, name, sect, full, str_va, name_in_s50, sect_known, in_ini_reg/def, ini_path, gmst_path, node_va, node_val, refs, calls, 汚染フラグ等）
- 中間物: `%TEMP%\opencode\v208_scan.py` `v208_b.py`〜`v208_i.py` / `v208_cands.json` `v208_rows.json` / `v208_b_disasm.txt` `v208_b_callers.txt` `v208_c_disasm.txt` `v208_c_419A70callers.txt` `v208_c_coloncmp.txt` `v208_d_final.txt` `v208_e_final.txt` `v208_f_final.txt` `v208_g_final.txt` `v208_h_final.txt` `v208_i_final.txt`

## 5. 新規残点

1. **rel6 §6 / RE_EXE_REPORT §43 の訂正ドラフト**（GMST→INI レジストリノードへの再分類、「2,304+1」の撤回、「CALL 0x4000E0」誤記修正）— 本タスクは docs 編集禁止のため未着手。
2. `copilot_ini_registries.txt` の補遺候補: bLightAttenuation 12 件（実登録 node 0xB08138–0xB08190）+ wrapper 経由登録（fAlpha:Water 等）。走査パターンの改善提案（site-pattern ではなく node 参照 + call target で検出）。
3. `W/H/Screen/X/Y/Shaders:Display` 等の未参照リテラル 17 件は INI ファイル読込時の動的マッチングに使われる可能性（読込パス側でキー照合するか）— INI パーサ（v51/v160 系）との接続は未調査。
4. GameSetting map 0xB35574 の実ランタイム内容（ESM 読込時の上書き後）と 2,304 static の対応は本タスク範囲外（s50 は static 登録表）。
