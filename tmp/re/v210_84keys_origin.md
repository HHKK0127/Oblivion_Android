# V210: 移植のみ84キーの出所特定（rel1 §5-2 の追跡）

- 日付: 2026-10-30 / 対象: `Oblivion.exe` v1.2.0416 Steam（7,898,624 B, `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`）
- 出典依頼: `tmp/re/relcheck_20261029/rel1_ini_type_agreement.md` §5-2（読取のみ）
- 手法: EXE バイト走査 + capstone 5.0.7 (skipdata) 逆アセンブル + 既存成果物との機械突合。**ESM 本体は不使用**（GMST 判定はすべて EXE 内 GameSetting 登録表 `s50_gmst_full.txt` を使用）。
- スクリプト: `%TEMP%\opencode\v210_match.py` `v210_exe_strings.py` `v210_gmst_match.py` `v210_exe84.py` `v210_value84.py` `v210_value_cpp.py` `v210_disasm.py` `v210_gpp.py` `v210_iat.py` `v210_controls_fn.py` `v210_gpp_read.py` `v210_bindtable.py` `v210_flags.py`
- CSV: `%TEMP%\opencode\v210_84keys.csv`（84行 + ヘッダ, 3,639 B, 列 `key,type,section,group`）

## 0. 結論サマリ

1. **84件すべてが同梱 `Oblivion_default.ini`（shipped defaults file）の「エンジン未登録・ファイルのみ」85キーに含まれ**（交差84 / 移植のみ0 / ファイルのみ1）、**値も実質全件一致**（85件 vs 移植cpp: 機械一致84 + 残り1件は C++ エスケープ見かけ差で実体同一 = 差分実質0）→ 出所の主系統は **(c) INI ファイル由来**。
2. **26キー（Absorb 12 + ShockBolt 14）の内訳は系統が2つに割れる**:
   - `Absorb` 12 = **EXE の GameSetting(GMST) マップに bare 名で登録済み**（s50 台帳と12/12 同名・同既定値）。INI レジストリ混在名としては0件。→ **(b) GMST 由来と (c) ファイル由来の二重系統**（値は両者一致で識別不能）。
   - `ShockBolt` 14 = **EXE に文字列が1件も存在しない**（bare 名0件 / `ShockBolt` 独立文字列0件 / `[ShockBolt]` 0件 / 混在名0件）。GMST は `fShockBolt*` 5件＋`iShockDebug` 1件のみで名前も値も異なる。→ **(c) 同梱ファイル由来のみ**。
3. **残り58件**は rel1 の分類がおおむね正しいが、例外が2件: `fAlpha`(Water) と `uSurfaceTextureSize`(Water) は **EXE に実登録あり（wrapper 経由）＝ v91 抽出漏れ**。Controls 33 の内訳は「キー割当29 + Xenon 閾値4」で、29件は EXE コードで読出/書出を**確定**。
4. 移植表の出所フラグ（`oblivion_ini_defaults.h` の `kFromFile=0x01 / kFromEngine=0x02 / kFromBoth=0x03`）を84件に機械照合した結果、**84/84 が `kFromFile`** = 移植側は既に「ファイル由来」と正しく分類済み（§3-4）。

---

## 1. 84件の全数リスト

rel1 はセクション内訳のみ記載のため、本タスクで全数を機械抽出した（`rel1_data2.json` の `only_port` = rel1 本体の生成物と同一。件数・内訳は rel1 §5-2 と一致: 84件 / `Controls 33 / ShockBolt 14 / Absorb 12 / Archive 6 / Water 4 / General 3 / Pathfinding 2 / Audio 2 / Display 2 / (空) 1 / LOD 1 / GamePlay 1 / MESSAGES 1 / HAVOK 1 / SpeedTree 1`）。全行は CSV（`v210_84keys.csv`）を参照。要点は以下。

| # | セクション | 件数 | キー |
|---|---|---:|---|
| 1 | Controls | 33 | バインディング名29: `Forward Back Slide Left Slide Right Use Activate Block Cast Ready Item Crouch/Sneak Run Always Run Auto Move Jump Toggle POV Menu Mode Rest Quick Menu Quick1..8 QuickSave QuickLoad Grab` ＋ 閾値系4: `fXenonLookMult fXenonLookXYMult iXenonMenuStickThreshold iXenonMenuStickSpeedThreshold` |
| 2 | ShockBolt | 14 | `bDebug fBoltGrowWidth fBoltSmallWidth fBoltsRadius fCastVOffset fCoreColorR/G/B fGlowColorR/G/B fSegmentVariance fTortuosityVariance iNumBolts` |
| 3 | Absorb | 12 | `fAbsorbBoltGrowWidth fAbsorbBoltSmallWidth fAbsorbBoltsRadius fAbsorbCoreColorR/G/B fAbsorbGlowColorR/G/B fAbsorbSegmentVariance fAbsorbTortuosityVariance iAbsorbNumBolts` |
| 4 | Archive | 6 | `SMasterMeshesArchiveFileName SMasterMiscArchiveFileName SMasterSoundsArchiveFileName SMasterTexturesArchiveFileName1 SMasterVoicesArchiveFileName1 SMasterVoicesArchiveFileName2` |
| 5 | Water | 4 | `fAlpha SNearWaterIndoorID SNearWaterOutdoorID uSurfaceTextureSize` |
| 6 | General | 3 | `SMainMenuMusicTrack bNewAnimation bUseThreadedBlood` |
| 7 | Pathfinding | 2 | `bBackgroundPathing bSmoothPaths` |
| 8 | Audio | 2 | `bDSoundHWAcceleration bUseSoftwareAudio3D` |
| 9 | Display | 2 | `bHighQuality20Lighting fFarDistance` |
| 10–15 | LOD / GamePlay / MESSAGES / HAVOK / SpeedTree / (空) | 各1 | `fLODNormalTextureBlend` / `iDifficultyLevel` / `iFileLogging` / `iResetCounter` / `iTreeClonesAllowed` / `bEnableBorderRegion` |

**母集団の対応（重要）**: 同梱 `Oblivion_default.ini` のみキーは `copilot_ini_crosscheck2.md` §3 の **85件**。移植84件との突合結果は **交差84 / 移植のみ0 / ファイルのみ1**（差分は `fQuestScriptDelayTime`(General)）。これは Steam 版 v91 には `fQuestScriptDelayTime` が登録済み（`v91_ini_defaults.csv` 行: `MAIN, 0x00B09E28, reg=FUN_009e39c0@0x009E39F2`）＝移植表・RE 両側キーであるためで、**整合する**（crosscheck2 の母集団は `OblivionStandalone.exe` 572 node で v91＝Steam 595 と僅差、下記 §5-4）。

---

## 2. 26キー（Absorb 12 + ShockBolt 14）の出所判定

### 2.1 (a) EXE の INI レジストリ混在名620件との突合 → **該当0件**

- `v208_mixed_names.csv`（620 実登録混在名）を全行走査: **`Absorb` / `ShockBolt` を含む行 = 0件**（列 `classification,name,sect,full,...` 全域を走査）。
- v208 の分類とも矛盾しない（v208 §1.4 の命名規則外20件にも Absorb/ShockBolt は無い）。
- → **26キーは EXE の INI 既定値レジストリ（FUN_004040E0 系）に一切登録されていない**（rel1「抽出範囲外」の表現を確認）。

### 2.2 (b) GMST 由来か — **Absorb 12 = YES / ShockBolt 14 = NO**

`s50_gmst_full.txt`（EXE 内 GameSetting map 登録表 2,304 bare 名）と名前一致照合（ESM 本体は不使用）:

| 群 | s50 突合 | EXE 内文字列（bare 完全一致） | 判定 |
|---|---|---|---|
| Absorb 12 | **12/12 exact ヒット** | 12/12 実在（`.rdata` 0xA5C4E0–0xA5C5E8, .text 参照1本ずつ） | **GMST 登録あり** |
| ShockBolt 14 | **0/14** | **0/14 実在**（`bDebug` は他セクション混在名6件のみ、`ShockBolt` 付きの bare 名は0件） | **GMST でもない** |

- Absorb の登録経路は逆アセンブルで確定: `fAbsorbBoltsRadius` @0x9EFB20 → `push 0xA5C4E0; mov ecx,0xB381F8; call 0x41BAE0`（float 版 Setting 登録＝GameSetting map 0xB35574 へ）、`iAbsorbNumBolts` は `call 0x419A70`。既定値も移植値と一致（例: `fAbsorbBoltSmallWidth` 既定 7.0 = 移植 7.0000 / `fAbsorbSegmentVariance` 7.0 = 7.0000 / `iAbsorbNumBolts` 1 = 1）。
- ShockBolt の GMST 近縁は **別名・別値**: `fShockBoltGrowWidth`(1.0) `fShockBoltSmallWidth`(3.0) `fShockBoltsRadius`(**48.0**) `fShockBoltsRadiusStrength`(3.0) `fShockBoltsLength`(1024.0) `iShockDebug`(int) の6件のみ。移植 `[ShockBolt] fBoltsRadius` の値は **24.0** で GMST の 48.0 と**不一致** → GMST 由来とは言えない。
- 「`Absorb` 単独文字列」の .text 参照は1本（0x9D9F61）だけだが、これは `push 'Absorb'; push 'sMagicEffectItemAbsorb'; mov ecx,0xB334E0; call 0x419A70` ＝ **GMST `sMagicEffectItemAbsorb` の既定値登録**であり、INI セクション参照ではない。
- → **(b) 判定: Absorb 12 は GMST 系統と一致（ただしファイルにも同名同値で実在）。ShockBolt 14 は GMST 非該当。**

### 2.3 (c) Oblivion.ini ファイル由来か — **26/26 該当（これが本判定）**

- `copilot_ini_crosscheck2.md` §3「Only in the shipped file (engine does not register these)」85件に、**Absorb 12・ShockBolt 14 とも全件実在**（`[Absorb] (12)` / `[ShockBolt] (14)` ブロック、行番号558–569 / 259–272 付き）。
- **値の機械照合（移植 cpp 直パース, 657 エントリ）**: 85件中 **EQ=84 / DIFF=1 / MISSING=0**。唯一の DIFF は `SMainMenuMusicTrack` で、cpp の `"special\\tes4title.mp3"` とファイル値 `special\tes4title.mp3` の**C++ エスケープ差（実体は同一）** → **実質 85/85 一致**。
- 26キーの例（移植 = 同梱ファイル、完全一致）: `[ShockBolt] fBoltsRadius 24.0000 = 24.0000`、`fCastVOffset -10.0000 = -10.0000`、`iNumBolts 7 = 7`、`[Absorb] fAbsorbBoltSmallWidth 7.0000 = 7.0000`。
- **EXE 側がこの2セクションを読めないことの裏取り**（バイト走査, 全 PE セクション対象）:
  - `[ShockBolt]` リテラル **0件** / 独立 C 文字列 `ShockBolt` **0件**（ヒット5件はすべて `fShockBoltsLength` 等の内部部分列）。
  - `[Absorb]` リテラル **0件**。独立 `Absorb` は3件存在するが、参照先1件は上記 GMST 登録、残り2件は参照0（未参照リテラル）。
  - 26キーの bare 名で `Key:Section` 形式の混在名も **0件**（`fBoltsRadius:ShockBolt` 等を直接走査して0）。
  - IAT には `GetPrivateProfileStringA`(0xA280B4) / `GetPrivateProfileIntA`(0xA280E4) / `WritePrivateProfileStringA`(0xA280B0) が存在するが、**それらの呼出し点に Absorb/ShockBolt 用のセクション引数は現れない**（セクション名リテラルが存在しない以上、実行時に渡せない）。
- → **(c) 判定: 26キーの出所は同梱 `Oblivion_default.ini`（＝INI ファイル由来）。エンジン本体はこの2セクションを読まない。**

### 2.4 (d) CS/GECK ドキュメント由来か — **補強証拠なし（現時点で不要な仮説）**

- 84件（85件）すべてが同梱ファイルの実キー・実値と1:1で一致するため、「ドキュメントを見て推定した」必然性は生じない。
- ただし `[Absorb]`/`[ShockBolt]` が**誰のために存在するセクションか**（GECK/CS 側が読む・単なる記載・過去の残骸）は本調査では未確認 → §5 残点。

### 2.5 26キー判定表（結論）

| 群 | 件数 | INI レジストリ混在名 | GMST(s50) | 同梱 ini | EXE 読込 | **判定** |
|---|---:|---:|---|---|---|---|
| Absorb | 12 | 0 | **12/12 同名同値** | 12/12 同値 | なし（セクション文字列なし） | **GMST 系統と同名同値 ＋ 同梱ファイル由来（二重系統）**。INI セクションとして扱う根拠はファイル側のみ |
| ShockBolt | 14 | 0 | 0/14（近縁 `fShockBolt*` 5件は別名・値相違） | 14/14 同値 | なし（`ShockBolt` 文字列自体が不在） | **同梱 `Oblivion_default.ini` 由来（確定）** |

---

## 3. 残り58件の分類サマリ

### 3.1 EXE 内在走査（84件全件、bare 完全一致 / セクション単独文字列）

`v210_exe84.py` で84キーを全 PE セクション走査（結果: `%TEMP%\opencode\v210_exe84.txt`）:

| 判定 | 件数 | 内訳 |
|---|---:|---|
| **EXE に bare 名実在** | **42** | Controls **27**（`Forward` … `Grab` のうち空白付き2件を除く群, .rdata 0xA2F960–0xA2FA6C）＋ Controls 空白付き2（`Slide Left` 0xA6283C / `Slide Right` 0xA62820 — 但し .data テーブル上の登録名は `SlideLeft`/`SlideRight`、下記 3.2）＋ Absorb 12（0xA5C4E0–0xA5C5E8）＋ `fAlpha`（0xA91DD8 単独 + 混在名 `fAlpha:Water` 0xA3F43C） |
| **EXE に不在** | **42** | ShockBolt 14 ＋ 残り28（下表 D 列） |

58件を系統別に分類（合計 29+4+2+23 = 58）:

| 区分 | 件数 | 内容 | 出所判定 | 根拠 |
|---|---:|---|---|---|
| **A. Controls バインディング** | **29** | `Forward Back Slide Left Slide Right Use Activate Block Cast Ready Item Crouch/Sneak Run Always Run Auto Move Jump Toggle POV Menu Mode Rest Quick Menu Quick1..8 QuickSave QuickLoad Grab` | **実行時の INI 読み書きキー（コード確定）** | 下記 3.2 |
| **B. Controls Xenon 閾値** | 4 | `fXenonLookMult fXenonLookXYMult iXenonMenuStickThreshold iXenonMenuStickSpeedThreshold` | 同梱ファイルのみ（EXE 不在） | v210_exe84 exact=0 / substr=0 |
| **C. Water（wrapper 登録）** | 2 | `fAlpha` `uSurfaceTextureSize` | **EXE 実登録あり ＝ v91 抽出漏れ** | 下記 3.3 |
| **D. 同梱ファイルのみ** | 23 | Archive 6 / `SNearWaterIndoorID` `SNearWaterOutdoorID` / General 3（`SMainMenuMusicTrack` `bNewAnimation` `bUseThreadedBlood`）/ Audio 2 / Pathfinding 2 / Display 2 / LOD 1 / GamePlay 1 / MESSAGES 1 / HAVOK 1 / SpeedTree 1 / (空)1（`bEnableBorderRegion`） | 同梱ファイルのみ（EXE 文字列不在） | exact=0 / substr=0 |

### 3.2 区分 A: Controls 29件は「読込・書出ともに EXE コードで特定」（rel1 の [推定] を確定）

- **既定バインディング名テーブル**: `.data 0xB02BB8` に **29エントリ**（`Forward, Back, SlideLeft, SlideRight, Use, Activate, Block, Cast, Ready Item, Crouch/Sneak, Run, Always Run, Auto Move, Jump, Toggle POV, Menu Mode, Rest, Quick Menu, Quick1..8, QuickSave, QuickLoad, Grab`）。
- **書出側** `0x404400`:
  ```
  fld  [0xA2FB0C] (=1.8f); fstp [0xB02C4C]     ; fVersion node へ既定値 1.8 を設定
  mov  ebp, [0xA280B0]                          ; IAT WritePrivateProfileStringA
  lea  esi, [ecx+0x1B9B]; mov edi, 0xB02BB8; mov ebx, 0x1D   ; ebx=29
  loop: sprintf(buf, "%08X", 3平面バイト列から組立値)          ; 例 "0011FFFF"
        mov edx, [edi]                          ; キー名
        push buf0; push buf; push edx; push 0xA2FAF8 ("Controls"); call ebp
        add esi,1; add edi,4; sub ebx,1; jnz loop
  ```
  → **既定29バインディングを `%08X` 形式で `Oblivion.ini [Controls]` に書き出す**。値は `this+0x1B5D / +0x1B7E / +0x1B9B` の3平面（29バイト×3）から構成され、移植値 `Forward=0011FFFF` 等と一致。
- **読込側** `0x404610`:
  ```
  mov edi, [0xA280B4]                            ; IAT GetPrivateProfileStringA
  loop: mov edx, [esi*4+0xB02BB8]                ; キー名テーブル
        push ...; push 0x40; push buf; push 0; push edx; push "Controls"; call edi
        call 0x982B96 (strtoul base 16) → 3平面へ復元
  ```
  → **同テーブル29件を `GetPrivateProfileStringA("Controls", key, …)` で読み、16進→3平面へ復元**（書出の逆変換）。
- なお29件中多数は GMST でも既定値文字列として共有される（`push "Forward"; push "sUActnForward"; call 0x419A70` 等）が、それは表示名用であり INI キーとしては上記テーブルが本体。
- **一致範囲の内訳**: 移植の29キーのうち **27キー**は .data テーブルの登録名と**完全一致**（`Forward` … `Grab` で空白違いを除く）。
- **表記差（要注意）**: エンジンテーブルは **`SlideLeft`/`SlideRight`（空白なし, 0xA2FA58/0xA2FA4C）**、同梱 `Oblivion_default.ini` と移植表は **`Slide Left`/`Slide Right`（空白あり, ファイル行133/134）**。空白付き文字列は 0xA6283C/0xA62820 に別系統（GMST `sUActnSldleft` 既定値）で存在し、.data テーブルには無い → **エンジンは空白付きキーを読み/書かない可能性が高い**（移植が空白付きを採用している点は要判断）。

### 3.3 区分 C: `fAlpha` / `uSurfaceTextureSize` は EXE 登録あり（v91 抽出漏れ）

- v208 §2.2 の登録実確認: `fAlpha:Water`（0xA3F43C）= 0x49D534 `push 0xA3F43C; mov ecx,0xB3524C; call FUN_00404850`（float wrapper → registry 0xB07BF0 へ）、`uSurfaceTextureSize:Water`（0xA3F4C4）= 0x49DD5D `FUN_00444060`（dword wrapper → 同上）。本タスクの走査でも両混在名の実在を確認（`v210_bindtable.txt`）。
- `v91_ini_defaults.csv`（637行）の `reg=` はすべて CRT 初期化スタブ系（`FUN_009Dxxxx`/`FUN_009Fxxxx`）で、**`FUN_00404850`・`FUN_00444060` 由来の行は0件** → v91 の抽出手法が wrapper 経由を取りこぼす（v208 §2.2 の指摘と同一機構）。
- → rel1 §5-2「移植のみ84件は RE 登録既定値と系統が異なる」には **例外2件**（逆に言えば移植側が正しい）。

### 3.4 値照合と出所フラグ（84件全件）

- **値**: `oblivion_ini_defaults.cpp`（657エントリ直パース）× 同梱ファイル値（crosscheck2 §3, 85件）→ **EQ 84 / DIFF 1 / MISSING 0**。唯一の DIFF は `SMainMenuMusicTrack` の `"special\\tes4title.mp3"` vs `special\tes4title.mp3` の**C++ エスケープ見かけ差（実体同一）** → **実質84/84一致**。
- **出所フラグ**（`IniSource` = `kFromFile 0x01 / kFromEngine 0x02 / kFromBoth 0x03`）: **84/84 が `kFromFile`**（`v210_flags.txt`）。移植側は既に「同梱ファイル由来」と正しく分類している。

### 3.5 rel1 §5-2 記述の検証結果

| rel1 の記述 | 検証結果 |
|---|---|
| Absorb(12)/ShockBolt(14) は RE の39セクションに無い → 26件は EXE の INI 登録既定値の抽出範囲外、GMST 由来の可能性・出所は [要調査] | **正しい**（v208 混在名620件に該当0）。ただし GMST 由来は **Absorb 12 のみ成立**、ShockBolt 14 は GMST・INI レジストリ・EXE 文字列すべて非該当で **同梱ファイル由来に確定** |
| Controls 33件はキー割当。RE 側に Controls セクションはあるがこのキー群は未登録 → 実行時のユーザー設定であり登録既定値に載らない系統と推定 [推定] | **29件はコードで裏付け（[推定] → 確定）**。残り4件（Xenon 閾値）はキー割当ではなく float/int で、かつ EXE 不在＝同梱ファイルのみ → 「33 = 全件キー割当」は**部分訂正** |
| Archive の `SMaster*FileName`（BSA ファイル名7件中6件）等は、移植が実運用値として補った系統 | **7件中6件の数え方は正しい**（移植の BSA 系7件 = `sArchiveList` ＋ `SMaster*` 6件、うち port-only は6件）。ただしそれは「実運用値の補完」ではなく**同梱ファイル由来**（値は `Oblivion - Meshes.bsa` 等で完全一致）。RE 側は単一キー `sArchiveList`（カンマ連結, node 0xB04458）で保持しており、移植は `sArchiveList`(kFromBoth) と `SMaster*`×6(kFromFile) を**両方**保持している |

---

## 4. 移植側への示唆（出所が確認できないキーの扱い方針案）

**出所が確認できないキーは0件**（84/84 が同梱 `Oblivion_default.ini` 由来で、値まで一致）。よって「保持/削除/要調査」の分類は次のとおり。

| 群 | 件数 | 方針案 | 理由 |
|---|---:|---|---|
| 全84件 | 84 | **保持**（`kFromFile` のまま） | 値は同梱ファイルと100%一致。削除根拠なし。既定テーブルから外すとユーザーの `Oblivion.ini` に実在するキーの既定値解決が壊れる |
| うち `Absorb` 12 | 12 | **保持 ＋ 注記「GMST としても登録済み（別系統）」** | EXE の GameSetting map に同名同値で存在（s50 12/12）。将来 ESM 読込による GMST オーバーライドを再現する場合、この12件は **INI 経路と GMST 経路の双方**を実装しないと上書きが再現できない → **要調査（優先度 中）** |
| うち `ShockBolt` 14 | 14 | **保持 ＋ 注記「EXE 未使用（読込経路なし）」** | セクション名リテラルが EXE に存在せず、エンジンは読めない。既定テーブルに在るだけで副作用はないため**削除より保持**を推奨。ただし移植のロジックがこれを消費する設計にしない限り**機能しない**前提を明示 |
| Controls 29 | 29 | **保持（正）＋ 表記差を要判断** | エンジンは `SlideLeft`/`SlideRight`（空白なし）で読出/書出する一方、移植と同梱ファイルは `Slide Left`/`Slide Right`。忠実再現するなら**両表記を許容する正規化**、またはエンジン名への統一を要判断（優先度 中） |
| `fAlpha` / `uSurfaceTextureSize` | 2 | **保持（むしろ RE 側の補遺候補）** | EXE 実登録（wrapper 経由）＝ v91 抽出漏れ。移植側は正しい |
| その他 D 区分23 + Xenon 4 | 27 | **保持**（EXE 非使用の可能性あり、要確認は任意） | 同梱ファイルに実在し値一致。EXE は文字列すら持たないため本体では未使用（レンダラ/オーディオ等の外部設定を除く） |
| 「要調査」に残すもの | — | **(i) ShockBolt/Absorb の真の消費者（GECK/CS/その他）**(ii) `bDebug`(ShockBolt) と GMST `iShockDebug` の対応 (iii) Controls 表記差の扱い | 出所自体は特定済み。残るのは「誰が読むか」と表記整合 |

---

## 5. 新規残点

1. **`[ShockBolt]` / `[Absorb]` の消費者未特定**: EXE 本体は読まない（セクション文字列0件）と確定したが、**同梱ファイルに載っている理由**（GECK/CS が読む・旧版からの残骸・将来用）は未調査。GECK 側バイナリ（存在すれば）の確認は別タスク。ESM 本体の GMST レコードに Absorb 系が含まれるかも未確認（本タスクは ESM 禁止）。
2. **v91 抽出漏れ（wrapper 経由）の全数化**: `fAlpha:Water` `uSurfaceTextureSize:Water` の2件を確認。v91 は CRT 初期化スタブ由来のみ抽出しているため、**他の wrapper 経由ノードも同様に漏れている可能性**がある（v208 §5-2 の提案「site-pattern ではなく node 参照 + call target で検出」の再実施が有効）。成果物編集は本タスク禁止のため未実施。
3. **Controls の表記差2件**（`Slide Left`/`SlideLeft`, `Slide Right`/`SlideRight`）: エンジン側は空白なし。移植の読込互換性方針が未決定。
4. **母集団差の明文化**: `copilot_ini_crosscheck2.md` は `OblivionStandalone.exe`（572 node / 38 セクション）、`v91_ini_defaults.csv` は Steam 版（595 キー / 39 セクション）。`fQuestScriptDelayTime` が Steam では登録済み・Standalone では未登録であることなど、**2成果物の突合には母集団差の注記が要る**。
5. **rel1 の未対象残件（今回対象外）**: (a) `rDebugTextColor` の packed 色バイト順裏取り、(b) RE のみ39件の移植要否 — rel1 §6 の記述をそのまま引き継ぐ。
6. 既存文書との不整合（AGENTS.md 指示による報告）: `RE_EXE_REPORT.md.pre80b/83/84` §「`[Pathfinding]`・`[ShockBolt]`・`[Absorb]` などはデバッグや演出用で、移植の優先度は低い」は **`[ShockBolt]`/`[Absorb]` が EXE 非使用である本調査結果と整合**するが、同段落がこれらを「INI 登録済みセクション」のごとく扱う点は `v91`/`v208` の実測（登録0件）と食い違う → 文書側の分類訂正候補（docs 編集は禁止のため未実施）。

---

## 6. 成果物

| パス | サイズ |
|---|---|
| `C:\Users\hiroki.kogarumai\Oblivion_Android\tmp\re\v210_84keys_origin.md`（本ファイル, 新規） | 約 24.4 KB（204行, UTF-8） |
| `%TEMP%\opencode\v210_84keys.csv`（84行 + ヘッダ） | 3,639 B |
| `%TEMP%\opencode\v210_match.txt`（84 vs 同梱ファイル85 突合） | 709 B |
| `%TEMP%\opencode\v210_exe_strings.txt`（26キー + セクション/混在名走査） | 5,506 B |
| `%TEMP%\opencode\v210_gmst_match.txt`（26キー × s50 GMST 台帳） | 7,771 B |
| `%TEMP%\opencode\v210_exe84.txt`（84キー EXE 内在走査） | 8,359 B |
| `%TEMP%\opencode\v210_value_cpp.txt`（cpp × 同梱ファイル値照合） | 195 B |
| `%TEMP%\opencode\v210_value84.txt` / `v210_value_cmp.txt`（manifest 版値照合, 参考） | 171 B / 6,494 B |
| `%TEMP%\opencode\v210_disasm.txt`（'Absorb' 参照点 + GMST stub） | 10,225 B |
| `%TEMP%\opencode\v210_gpp.txt` / `v210_gpp_read.txt`（GPP 呼出し） | 3,090 B / 6,424 B |
| `%TEMP%\opencode\v210_iat.txt`（IAT + 文字列参照） | 4,890 B |
| `%TEMP%\opencode\v210_controls_fn.txt`（0x404400 曲出関数） | 6,299 B |
| `%TEMP%\opencode\v210_bindtable.txt`（既定バインディング表29件 + 混在名） | 3,009 B |
| `%TEMP%\opencode\v210_flags.txt`（84件の出所フラグ） | 3,572 B |

中間物: `%TEMP%\opencode\v210_*.py`（13本）。既存 `tmp/re` 成果物は一切更新していない（読取のみ）。
