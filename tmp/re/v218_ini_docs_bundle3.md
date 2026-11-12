# V218: INI/文書整備束第3 — rel6/§43 訂正ドラフト・registries 補遺・未参照リテラル・Controls 表記差・母集団差・文書訂正・GameSetting map 仕様化

- 日付: 2026-11-01 / 対象: `Oblivion.exe` v1.2.0416 Steam（7,898,624 B, `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`）
- 出典（残点）: `tmp/re/v208_gmst_mixed_names.md` §5-1〜4 ＋ `tmp/re/v210_84keys_origin.md` §5-3/4/6（両ファイル＋`v209_rdebugtextcolor.md` 該当節を読取）
- 手法: capstone 5.0.7（skipdata）＋ `probe_harness`（selftest 着手時 **4/4 PASS**・変更なし）＋ `v172_common`。ESM 本体は不使用（禁止遵守）。既存 `tmp/re` 成果物・docs/GT/TODO/cache は読取のみ・無編集。
- スクリプト: `%TEMP%\opencode\v218_1.py` `v218_1b.py` `v218_2.py` `v218_3.py` `v218_4.py` `v218_5.py` `v218_7.py`（各 `_out.txt` 付き）
- PE マップ: image_base 0x400000 / .text VA 0x401000 raw 0x400 / .rdata VA 0xA28000 raw 0x627200 / .data VA 0xB02000 raw 0x700C00（VA−file_off = 0x400C00(.text) / 0x400E00(.rdata) / 0x401400(.data)、v209 §1.1 と一致）

---

## 項1. rel6 / RE_EXE_REPORT §43 の訂正ドラフト（解明・ドラフト3件）

### 1-0 裏取り実測（すべて再測）

| 検査 | 結果 | 裏取り元 |
|---|---|---|
| node 0xB06704 実読 | value = `0x3F800000`(1.0f) / +4 name* = `0x00A3D150` → `"fGlobalTimeMultiplier:General"` | `v218_1.py` |
| `0x9DC302` の CALL rel32 バイト | `e8 d9 7d a2 ff` → 実ターゲット = 0x9DC307 + (−0x5D8227) = **`0x004040E0`** | `v218_1.py` |
| VA `0x4000E0`（文書表記どおりの場所） | **PE ヘッダ域（file 0xE0 = 全ゼロ）＝コード無し**。誤記確定 | `v218_1b.py` |
| `FUN_004040E0` 本体 | `cmp [ecx+0x110],0 / lea edi,[ecx+0x10C]` → リスト走査 → name 同士を `0x98262D` 比較 → 重複時 `0xA2FAB0` メッセージ / 非重複なら `0x446CB0` prepend。**name は "Key:Section" 全文のまま比較・保持（':' 分割なし）** | `v218_1.py` |
| `FUN_00419A70` | `mov [esi+4],name / mov [esi],value(raw32)` → `test name` → `mov ecx,0xB35574; call 0x55E000`(dup) → `0x412D30`(insert)。**dword+string 版 Setting 登録** | `v218_1.py` |
| `FUN_0041BAE0` | `fld [esp+0x20]; fstp [esi]; [esi+4]=name` → 0xB35574 経路は FUN_00419A70 と同一。**float 版のみ** | `v218_1.py` |
| call 網羅数（cache/call_targets） | `0x419A70` = **1,312** / `0x41BAE0` = **992** / `0x4040E0` = **637** / `0x412D30` = 47 / `0x55E000` = 308 | `v218_1b.py` |

### 1-1 訂正ドラフト (A): `docs/RE_EXE_REPORT.md` §43 見出し＋§43.1（読取のみ・適用は禁止のためドラフト）

before（§43 見出し / L7856・L7867・L7874）:
```
## §43 0x00B06704 = GMST fGlobalTimeMultiplier（dt 乗算係数の正体）
| +0x04 | 0x00B06704 | **1.0f** | **GMST fGlobalTimeMultiplier の値（dt 乗算係数）** |
つまり 0x00B06704 は **GMST（Game Setting）「fGlobalTimeMultiplier」（セクション General）の値**で、
```
after:
```
## §43 0x00B06704 = INI 既定値レジストリノード fGlobalTimeMultiplier:General（dt 乗算係数の正体）
   ※分類訂正（V208/V218）: これは GameSetting map（0xB35574）の GMST ではなく
     INI 既定値レジストリ（container 0xB07BF0）のノードである。旧「GMST」表記は誤り。
| +0x04 | 0x00B06704 | **1.0f** | **INI レジストリノード fGlobalTimeMultiplier:General の値（dt 乗算係数）** |
つまり 0x00B06704 は **INI 既定値レジストリのノード「fGlobalTimeMultiplier:General」の値**で、
   （Oblivion.ini [General] fGlobalTimeMultiplier と同一系統。GameSetting map には未登録）
```

根拠: v208 §2.1/§2.2（GameSetting map への全 2,304 登録点 1,312+992 箇所に ':' 名 0 件 / INI_REG_ADD 経由 637 箇所）、本項 1-0 の再測。

### 1-2 訂正ドラフト (B): §43.2 の `CALL 0x004000E0` 誤記（L7893）

before:
```
009dc302  e8 d9 7d a2 ff       CALL 0x004000E0        ; 名前付き設定の登録ヘルパ
```
after:
```
009dc302  e8 d9 7d a2 ff       CALL 0x004040E0        ; INI 既定値登録ヘルパ INI_REG_ADD（容器 0xB07BF0。旧 0x004000E0 は誤記。rel32 実測 = 0x9DC307−0x5D8227 = 0x4040E0、V208/V218）
```
補足: VA 0x004000E0 は PE ヘッダ域（file 0xE0・全ゼロ）でコードが存在しない（`v218_1b.py`）。§52 周辺の訂正注記（L9642「§43.2 の `CALL 0x004000E0` は `0x4040E0` の誤記 [訂正]」）は**本ドラフトで本文側にも反映**する。

### 1-3 訂正ドラフト (C): rel6 §6 / v137 の 2 点（`tmp/re/relcheck_20261029/rel6_gmst3way.md` L100-119 は読取のみ）

(C-1) rel6 §6-1/§6-2 の分類と網羅性:
before:
```
1. **分類**: EXE 実在の **GMST**。GLOB ではない（§2.2/§4-5 の [推定] は誤り）。
2. **`s50`（2,304件）に無い理由**: `s50` は **bare editor-ID 名**を抽出対象としており、`Key:Section` 形式で格納されたこの1件は**抽出対象外** → **`s50` の網羅性は 2,304 + 1**（§5-(d) へ波及）。
```
after:
```
1. **分類**: EXE 実在の **INI 既定値レジストリノード**（container 0xB07BF0、登録ヘルパ FUN_004040E0=INI_REG_ADD、site 0x9DC302）。GLOB でも GameSetting map の GMST でもない（GameSetting map 0xB35574 への 2,304 登録点に ':' 名 0 件、V208 §2.1/V218 項1）。
2. **`s50`（2,304件）に無い理由**: s50 の母集団＝GameSetting map 登録。本件は GameSetting map に存在しない INI ノードであり、s50 の**母集団外**である → **`s50` の網羅性は 2,304 のままであり、「2,304+1」は撤回する**（s50_gmst_full.txt は GameSetting 系として完全）。
```

(C-2) v137 の FUN_0041BAE0 記述（`tmp/re/v137_v80_global_residuals.md` L174）:
before:
```
- `FUN_0041BAE0(this, name, default)` = **Setting 登録**: `*this = default; this[1] = name; FUN_00412D30(name, this)`（重複時は `"Setting key '%s' already used in map.\nSetting keys must be unique.\n"` を FUN_004a7a60 に出力）。
```
after:
```
- `FUN_0041BAE0(this, name, f32_default)` = **Setting 登録（float 版）**: `fstp [this]; this[1] = name; 0xB35574 dup-check(0x55E000) → insert(0x412D30)`（重複時は `"Setting key '%s' already used in map.\n..."`(0xA32490) を FUN_004a7a60 に出力）。
- `FUN_00419A70(this, name, raw32_default)` = **同系の dword+string 版**（`[this]=raw32; [this+4]=name` → 0xB35574 経路は float 版と同一）。**s50 の static 登録スタブ群（2,056 件）は実体がこちら**（call 1,312 箇所）。float 版 0x41BAE0 は 992 箇所。両者重複なし（V160 §2-2）。
```

### 1-4 判定

**解明。** 3 件の訂正ドラフト（A: 分類再分類 / B: CALL 誤記 / C-1: 2,304+1 撤回＋C-2: v137 float 版限定）を提示。docs/rel6 本体は指示により無編集（読取のみ）。

---

## 項2. `copilot_ini_registries.txt` 補遺候補 — bLightAttenuation 12 件（解明・補遺ドラフト提示）

### 2-0 背景

`copilot_ini_registries.txt`（L6-8）は `registry 0x00B07BF0 : 557 nodes, 36 sections` として
`#   skipped: bad section: bLightAttenuatio 12` と明記して **bLightAttenuation セクション 12 件を
意図的に除外**している（＝欠落は仕様、かつ「セクション白名簿」の外れた群）。
v208 §1.2 が「全件 INI_REG_ADD 呼び出し確認済み」と報告した**12 node の実在を独立再測**した。

### 2-1 実測（`v218_2.py` / `v218_2d.py` — node 実読＋登録 site 特定）

| # | キー（"Key:Section" 全文） | node VA | value（.data 実読） | 文字列 VA | INI_REG_ADD site（call 0x4040E0） | name 分類 thunk（`mov ecx,node; jmp 0x403BC0`） |
|---|---|---|---|---|---|---|
| 1 | `bUseConstant:bLightAttenuation` | 0x00B08138 | 0x00000000 | 0x00A4332C | 0x009E2262 | 0x009B30D0 |
| 2 | `bUseLinear:bLightAttenuation` | 0x00B08140 | 0x00000000 | 0x00A4334C | 0x009E22C2 | 0x009B3100 |
| 3 | `bUseQuadratic:bLightAttenuation` | 0x00B08148 | 0x00000001 | 0x00A4336C | 0x009E2322 | 0x009B3130 |
| 4 | `fFlickerMovement:bLightAttenuation` | 0x00B08150 | 0x41000000 (8.0f) | 0x00A4338C | 0x009E2382 | 0x009B3160 |
| 5 | `uLinearMethod:bLightAttenuation` | 0x00B08158 | 0x00000001 | 0x00A433B0 | 0x009E23E2 | 0x009B3190 |
| 6 | `uQuadraticMethod:bLightAttenuation` | 0x00B08160 | 0x00000002 | 0x00A433D0 | 0x009E2442 | 0x009B31C0 |
| 7 | `fLinearValue:bLightAttenuation` | 0x00B08168 | 0x40400000 (3.0f) | 0x00A433F4 | 0x009E24A2 | 0x009B31F0 |
| 8 | `fQuadraticValue:bLightAttenuation` | 0x00B08170 | 0x41800000 (16.0f) | 0x00A43414 | 0x009E2502 | 0x009B3220 |
| 9 | `fConstantValue:bLightAttenuation` | 0x00B08178 | 0x00000000 | 0x00A43438 | 0x009E2562 | 0x009B3250 |
| 10 | `bOutQuadInLin:bLightAttenuation` | 0x00B08180 | 0x00000000 | 0x00A4345C | 0x009E25C2 | 0x009B3280 |
| 11 | `fLinearRadiusMult:bLightAttenuation` | 0x00B08188 | 0x3F800000 (1.0f) | 0x00A4347C | 0x009E2622 | 0x009B32B0 |
| 12 | `fQuadraticRadiusMult:bLightAttenuation` | 0x00B08190 | 0x3F800000 (1.0f) | 0x00A434A0 | 0x009E2682 | 0x009B32E0 |

- 登録 site パタテンは正規の `push node; mov ecx,0xB07BF0; mov [esp+10],0; call 0x4040E0`
  （CRT 初期化帯 0x9E22xx–0x9E26xx、64 B 間隔で整列）— **通常の静的ノード群であり wrapper 経由ではない**。
- v208 §1.2 の「node 0xB08138–0xB08190」表記は **node が 8 B 間隔で 13 個並ぶうち 12 個**が該当
  （0xB08164 は uQuadraticMethod ノードの name* スロットでありノードではない。実測済）。
- **wrapper 経由 2 件**（v208 §2.2 / v210 §3.3、本タスクで再測 `v218_2c.py`）:
  - `fAlpha:Water`（文字列 0x00A3F43C）: 0x0049D534 `push 0xA3F43C; mov ecx,0xB3524C; call 0x404850`
    → FUN_00404850 は `[esi]=f32; [esi+4]=name; mov ecx,0xB07BF0; call 0x4040E0`（**node は実行時生成**、静的 node 無し）
  - `uSurfaceTextureSize:Water`（文字列 0x00A3F4C4）: 0x0049DD5D `push 0xA3F4C4; mov ecx,0xB35264; call 0x444060`
    → FUN_00444060 は `[esi]=dword; [esi+4]=name; mov ecx,0xB07BF0; call 0x4040E0`（同上）
  - これらは site-pattern（`push <静的 node>`）走査では**原理的に検出不能**（node が .data に存在しない）。

### 2-2 判定

**解明。** bLightAttenuation 12 件はすべて INI 既定値レジストリ 0xB07BF0 への**実登録**（site 0x9E2262–0x9E2682）。
`copilot_ini_registries.txt` の「skipped: bad section」は**取りこぼしではなく意図的除外**だが、実登録がある以上
**補遺1（下記ドラフト）として追加するのが正当**。加えて wrapper 経由 2 件（fAlpha/uSurfaceTextureSize）も
site-pattern 抽出では検出不能＝補遺対象。

### 2-3 補遺1ドラフト（`copilot_ini_registries.txt` 末尾に追記する想定。本タスクでは成果物編集禁止のためドラフトのみ）

```
##############################################################################
# 補遺1 (V218): bLightAttenuation セクション 12 件 — registry 0x00B07BF0 に実登録
#   本体 L7 の "skipped: bad section: bLightAttenuatio 12" に対し、実登録を再測して補遺。
#   登録 site は CRT 初期化帯 0x9E2262..0x9E2682（64B 間隔）、node は .data 静的 8B 構造。
#   name 分類 thunk（mov ecx,node; jmp 0x403BC0）は 0x9B30D0..0x9B32E0 に併設。
[bLightAttenuation]  (12)
  bOutQuadInLin                          = 0x00000000                     ; hex bits=0x00000000 node=0x00B08180 site=0x009E25C2
  bUseConstant                           = 0x00000000                     ; hex bits=0x00000000 node=0x00B08138 site=0x009E2262
  bUseLinear                             = 0x00000000                     ; hex bits=0x00000000 node=0x00B08140 site=0x009E22C2
  bUseQuadratic                          = 0x00000001                     ; hex bits=0x00000001 node=0x00B08148 site=0x009E2322
  fConstantValue                         = 0x00000000                     ; hex bits=0x00000000 node=0x00B08178 site=0x009E2562
  fFlickerMovement                       = 8                              ; f32 bits=0x41000000 node=0x00B08150 site=0x009E2382
  fLinearRadiusMult                      = 1                              ; f32 bits=0x3F800000 node=0x00B08188 site=0x009E2622
  fLinearValue                           = 3                              ; f32 bits=0x40400000 node=0x00B08168 site=0x009E24A2
  fQuadraticRadiusMult                   = 1                              ; f32 bits=0x3F800000 node=0x00B08190 site=0x009E2682
  fQuadraticValue                        = 16                             ; f32 bits=0x41800000 node=0x00B08170 site=0x009E2502
  uLinearMethod                          = 0x00000001                     ; hex bits=0x00000001 node=0x00B08158 site=0x009E23E2
  uQuadraticMethod                       = 0x00000002                     ; hex bits=0x00000002 node=0x00B08160 site=0x009E2442

##############################################################################
# 補遺2 (V218): wrapper 経由登録 2 件 — node は実行時生成（site-pattern 抽出では検出不能）
#   FUN_00404850 (float) / FUN_00444060 (dword) が [esi]=value; [esi+4]=name を作り
#   mov ecx,0xB07BF0; call 0x4040E0 で登録。node VA は実行時値のため記載せず site のみ。
[Water]  (wrapper 経由 2 件)
  fAlpha                                 = 0x00000000                     ; f32 bits=0x00000000 (runtime node) site=0x0049D546 via FUN_00404850
  uSurfaceTextureSize                    = 0x00000080                     ; hex bits=0x00000080 (runtime node) site=0x0049DD72 via FUN_00444060
```

走査パターン改善案（v208 §5-2 の再提示）: site-pattern（`push <node>; mov ecx,<reg>; …`）ではなく
**「call 0x4040E0 の全 637 呼出し点を起点に、直前の `push` 操作数（即値なら静的 node / レジスタなら wrapper）を
後追いで解決する」**方式に変えると静的 node 群＋wrapper 経由を同時にカバーできる（wrapper 群は `mov ecx,0xB07BF0`
が wrapper 内にあり、呼出し点の +0x4 以降で解決する必要がある）。

## 項3. 未参照リテラル 17 件の INI 読込パス動的マッチング有無（解明: 動的マッチング**なし**）

### 3-0 問題設定

v208 §1.4 は `W/H/Screen/X/Y/Shaders`:Display `Joystick/Keyboard/Mouse`:Controls `Buffer/Enable`:General
等の未参照（dword 参照 0）`Key:Section` リテラル 17 件を「INI ファイル解析時の動的マッチングに使われる可能性」
として残点化した（v51/v160 系 INI パーサとの接続未調査）。

### 3-1 EXE 実測（`v218_3.py` / `v218_3b.py`）

| 検査 | 結果 |
|---|---|
| IAT インポート名走査（.rdata） | `GetPrivateProfileStringA`(名前 0xB00B32) / `GetPrivateProfileIntA`(0xB00C0E) / `WritePrivateProfileStringA`(0xB00B14) の 3 件のみ。**`GetPrivateProfileSectionA` / `SectionNamesA` / `StructA` は 0 件 = 未インポート** |
| `GetPrivateProfileStringA`(IAT 0xA280B4) の呼出し（FF15 パターン） | **4 箇所すべて ReadOne 内**（0x4A88A7 / 0x4A894A / 0x4A8999 / 0x4A8A16） |
| `GetPrivateProfileIntA`(0xA280E4) の呼出し | 5 箇所 = ReadOne 内 3（0x4A88D9 / 0x4A8A8D / 0x4A8AAD）＋ 0x40DFCE（INI バックエンド初期化）＋ 0x53AC86（パス読込: `push 0xA566D0` `push 0xA566C8` の固定リテラル） |
| `WritePrivateProfileStringA`(0xA280B0) の呼出し | 2 箇所 = WriteOne(0x4A879D) ＋ パス書出(0x53AD9E、同じく固定リテラル） |
| ReadOne(0x4A8800) の name 処理 | `call 0x4A78D0`(type 判定) → `call 0x4A8430`(section 抽出) → `call 0x4A8480`(key 抽出) → **登録済み entry->name を ':' 分割した (section,key) で GPP を叩く**（v51 §4.4 と一致） |
| LoadAll(0x4A85A0) | `[edi+0x108]`(hkey/セッション) チェック → `lea esi,[edi+0x10C]` → **登録リスト走査**（`[esi]` entry → `vt+0x10`(ReadOne) thiscall → `[esi+4]` 次へ）。**ファイル側キー列挙は存在しない**（SectionNames 未インポートのため構造上不可能） |
| 17 リテラルの imm32 参照 | 全 11 件代表で **0 件**（v208 の refs=0 と一致）。`W:Display` 等の実体は **`iSize W:Display`(0xA3E304) / `iSize H:Display`(0xA3E314) / `bFull Screen:Display`(0xA3E344) / `iLocation X:Display`(0xA3E35C) / `iLocation Y:Display`(0xA3E370) / `bUse Shaders:Display`(0xA3E3C8) 等の登録済み文字列の +N 先頭**（`.rdata` クラスタ 0xA3E2F0–0xA3E434 で実測）。`Joystick:Controls` = `bUse Joystick:Controls`(0xA2FB18)+5、`Keyboard:Controls` = `bBackground Keyboard:Controls`(0xA2FB30)+12、`Mouse:Controls` = `bBackground Mouse:Controls`(0xA2FB50)+12（Controls クラスタ 0xA2FAF8–0xA2FB6C 実測、`v218_4e.py`）。`Buffer:General`(0xA37857/0xA37877)・`Enable:General`(0xA388E5) も上位キーの部分列 |

### 3-2 判定

**解明: 動的マッチングは存在しない。** INI 読込は **「登録済みノードのリストを辿り、各 name を ':' 分割して
GetPrivateProfile*(section,key,default=現行値) を叩く」登録駆動（registry-driven）方式**であり、
ファイル側のキーを列挙する経路（SectionNames 未インポート）を持たない。
したがって:

1. 未登録キー（`[ShockBolt]` `fQuestScriptDelayTime`（Standalone 側）等を含む）は **INI ファイルに実在しても
   エンジンには一切届かない**（ReadOne に来ない）— v210 §2.3 の結論を読込パス側からも裏固め。
2. 17 件の「未参照リテラル」は独立したキーではなく、**登録済み `Key:Section` 文字列の部分列（+N スタート）**。
   INI ファイルのキー照合に関与しない**走査上の副産物**（v208 の `(c) 登録外` 分類は正しい）。
3. フォールバックは **ファイル単位**（`Oblivion_default.ini` → `Oblivion.ini` への CopyFileA、v51 §3）
   であり、キー単位の動的フォールバックは無い。

## 項4. Controls 表記差 2 件の互換方針裁定（解明・エンジンは**空白なし**を正とする）

### 4-1 実測（`v218_4.py` / `v218_4b.py` / `v218_4c.py` / `v218_4d.py`）

- **既定バインディング名テーブル .data 0xB02BB8 = 29 エントリを全数実読**:

| # | VA | 名前 | # | VA | 名前 |
|---|---|---|---|---|---|
| 0 | 0xB02BB8 | `Forward` | 15 | 0xB02BF4 | `Menu Mode` |
| 1 | 0xB02BBC | `Back` | 16 | 0xB02BF8 | `Rest` |
| 2 | 0xB02BC0 | **`SlideLeft`**（0xA2FA58・空白なし） | 17 | 0xB02BFC | `Quick Menu` |
| 3 | 0xB02BC4 | **`SlideRight`**（0xA2FA4C・空白なし） | 18–25 | 0xB02C00–0xB02C1C | `Quick1`…`Quick8` |
| 4–7 | 0xB02BC8–0xB02BD4 | `Use` `Activate` `Block` `Cast` | 26 | 0xB02C20 | `QuickSave` |
| 8 | 0xB02BD8 | `Ready Item`（空白あり） | 27 | 0xB02C24 | `QuickLoad` |
| 9 | 0xB02BDC | `Crouch/Sneak` | 28 | 0xB02C28 | `Grab` |
| 10–14 | 0xB02BE0–0xB02BF0 | `Run` `Always Run` `Auto Move` `Jump` `Toggle POV` | | | |

- **読込側** 0x404610: `mov edx,[esi*4+0xB02BB8]` → `GetPrivateProfileStringA("Controls"(0xA2FAF8), edx, …)`
  → `0x982B96`(strtoul base16) → 3 平面（+0x1B7E/+0x1B9B/+0x1BB8）へ。**ループ `cmp esi,0x1D`(29)。使用名はテーブル由来のみ**。
- **書出側** 0x404400: 同テーブルを `WritePrivateProfileStringA("Controls", …)` へ（v210 §3.2）。
- **空白付き `Slide Left`(0xA6283C) / `Slide Right`(0xA62820) の .text 参照は各 1 件のみ**、
  実体は **GMST `sUActnSldleft`/`sUActnSldright` の既定値登録**（0x9F60EF / 0x9F6127:
  `push 0xA6283C; push 0xA6282C("sUActnSldleft"); call 0x419A70`）＝**表示名（アクション名）であって INI キーではない**。
  INI 読出/書出コードに空白付き文字列は**一切現れない**。

### 4-2 裁定

**エンジン（EXE）が読む/書く INI キーは `SlideLeft`/`SlideRight`（空白なし）に確定。**
`Slide Left`/`Slide Right` は同梱 `Oblivion_default.ini`・移植表・GMST 表示名の側の表記であり、
**INI キーとしてはエンジンに到達しない**。

互換方針の裁定案（移植への推奨・優先度中）:

1. **正 = エンジン表記 `SlideLeft`/`SlideRight`**。既定テーブルは空白なしを正式キーとする。
2. 同梱ファイル互換のため **読込時のみ「`Slide Left`→`SlideLeft`」の正規化を 1 本持つ**（ユーザーが
   同梱 ini をコピーした場合、エンジンは元の Oblivion でもこの 2 キーを無視して既定値のまま動作する
   — コピーしても壊れないが設定が効かない）。移植でこれを再現するか（互換の完全再現）、
   あるいは両表記を許容してユーザー設定を実際に効かせるかは **製品判断**（RE 側の事実認定は上記のとおり）。
3. 書出は必ず空白なしで行う（エンジンと同じ）。空白付きを書き出すと次回のエンジン/移植読込で無視される。

## 項5. 母集団差の明文化（Standalone 572 node vs Steam 595）— 解明・v210 §5-4 の記述を訂正

### 5-1 実測（`v218_5c.py`〜`v218_5g.py`）

| 検査 | 結果 |
|---|---|
| `D:\Cargo\Ghidra\temp\OblivionStandalone.exe` と `Oblivion_Steam版\Oblivion.exe` の SHA-256 | **完全一致**（ともに `a8f313845c1545e9a60e1e995961eef4c033115da9443f6d756341df3c2b7dc6`, 7,898,624 B）＝ **同一バイナリのリネームコピー**。母集団の実体差は**存在しない** |
| `copilot_ini_crosscheck2.md` の "A nodes: 572 / 38 sections" | **単一コンテナ 0xB07BF0 のみ**のノード数。ヘッダ自体が `OblivionStandalone.exe registry 0x00B07BF0` と明記（crosscheck2 L5-8） |
| `copilot_ini_registries.txt` ヘッダ（Standalone 側走査） | `registry 0x00B07BF0 : 557 nodes, 36 sections` ＋ `skipped: bad section: bLightAttenuatio 12` ＋ `skipped: unresolvable name 3` → **557+12+3 = 572**（crosscheck2 の 572 と一致） |
| `v91_ini_defaults.csv`（Steam 側） | 635 行 = **0xB07BF0: 572 + 0xB11C40: 62 + 0xB07D28: 1**（3 コンテナ合計、40 セクション）。コンテナ 0xB07BF0 の 572 は Standalone と**同一数** |
| rel1 の "595 キー" | **635 行の bare キー名ユニーク数**。`fBlurRadius`/`sBody` 等 30 キーが複数セクション（BlurShader/HDR/HDRInterior、DEFAULT/HIT/QUADHIT）に重複出現し 40 行が重複 → 635−40 = **595**（`v218_5c.py` 実測: distinct bare = 595, dup keys = 30, extra rows = 40） |
| 545 共通キーの node VA 突合（Standalone registries.txt vs Steam v91） | **不一致 0 件**（同一バイナリの必然） |

### 5-2 判定（差分の性質）

1. **バイナリ差ではない。** Standalone と Steam はバイト同一。よって「572 vs 595」は**計測対象の定義差**である:
   - **572** = コンテナ 0xB07BF0 のノード数（＝crosscheck2/registries.txt の走査範囲。ヘッダの skipped 12+3 を含む総登録数）
   - **595** = 3 コンテナ 635 ノードから bare キー名のセクション跨り重複 40 行を除いたユニーク名数（rel1 のキー結合母集団）
   - 両者を並べるときは「635 ノード（3 コンテナ）/ 595 ユニーク bare 名 / うち 0xB07BF0=572」と**単位を明示**すること。
2. **`fQuestScriptDelayTime` は Standalone にも登録済み**（registries.txt `[MAIN]` 行: `node=0x00B09E28 site=0x009E39F2`、Steam v91 と同一）。crosscheck2 に現れる「差分」の正体は**セクション帰属のずれ**である:
   - エンジン側 name = `fQuestScriptDelayTime`（**':' 無し** → ReadOne 分割ヘルパ 0x4A8430 の既定セクション **MAIN**(0xA404C4) 扱い）
   - 同梱 `Oblivion_default.ini` 側 = `[General]` セクション
   - → crosscheck2 のキー結合（セクション名一致が条件）では **only-in-A(MAIN) と only-in-B([General]) に同時に出現**（crosscheck2 L271 と L406）＝**同一キーが二重計上される結合アーティファクト**。

### 5-3 訂正ドラフト: `tmp/re/v210_84keys_origin.md` §5-4（読取のみ・本体は無編集）

before:
```
4. **母集団差の明문화**: `copilot_ini_crosscheck2.md` は `OblivionStandalone.exe`（572 node / 38 セクション）、`v91_ini_defaults.csv` は Steam 版（595 キー / 39 セクション）。`fQuestScriptDelayTime` が Steam では登録済み・Standalone では未登録であることなど、**2成果物の突合には母集団差の注記が要る**。
```
after:
```
4. **母集団差の明一部（V218 訂正）**: Standalone と Steam の EXE は **SHA-256 完全一致の同一バイナリ**であり「fQuestScriptDelayTime が Steam では登録済み・Standalone では未登録」は**誤り**（同一 binary なので両側とも登録済み、node 0x00B09E28 / site 0x009E39F2）。数値差の正体は計測単位: crosscheck2 の **572 = 単一コンテナ 0xB07BF0**（557 listed + skipped 12 bLightAttenuation + 3 unresolvable）、v91 の **635 = 3 コンテナ**（572+62+1）、rel1 の **595 = 635 行の bare 名ユニーク数**（セクション跨り重複 40 行を除く）。crosscheck2 の fQuestScriptDelayTime が only-in-A(MAIN) と only-in-B([General]) に両出するのは、エンジン name が ':' 無しで既定 MAIN 扱い・ファイルは [General] 配置という**セクション帰属の結合アーティファクト**。2 成果物を突合するときは「コンテナ範囲」と「bare 名かどうか」の注記が必須。
```

## 項6. 文書不整合の訂正候補（ShockBolt/Absorb「デバッグ・演出用」記述）— 解明・ドラフト提示

### 6-1 事実確認

- 当該記述は `docs/RE_EXE_REPORT.md` **L816**（§12.2 所見）に現行版で実在し、`tmp/re/RE_EXE_REPORT.md.pre80b` / `.pre83` / `.pre84` の **L816 にも同一文**が残存（3 スナップショットとも grep 実測）。文書:
  ```
  - `[Pathfinding]`、`[HAVOK]`、`[Debug]`、`[ShockBolt]`、`[Absorb]` などはデバッグや演出用で、移植の優先度は低い。
  ```
- v210 §2 の実測: `[ShockBolt]`/`[Absorb]` は **EXE にセクション文字列なし・INI レジストリ登録 0 件・混在名 0 件**（ShockBolt は GMST でも非該当、Absorb は 12 bare 名が GMST として登録済み＝INI セクションではない）。v91 セクション分布にも ShockBolt/Absorb は不在（登録 40 セクションに無い）。
- 逆に `[Pathfinding]`(7) / `[HAVOK]`(23) / `[Debug]`(3) は **v91 に実登録セクションとして実在**（Steam 実測）。

### 6-2 判定

文書 L816 は (a) 登録済みセクション（Pathfinding/HAVOK/Debug）と **未登録セクション（ShockBolt/Absorb）を同一平面に並べて「INI 登録済み」の体で記述**しており、V208/V210 の実測と食い違う。「デバッグや演出用」という用途評価自体は Pathfinding/HAVOK/Debug については文脈上妥当だが、**ShockBolt/Absorb を「INI セクション」として扱う部分が誤り**。

### 6-3 訂正ドラフト（`docs/RE_EXE_REPORT.md` L816、docs 本体は読取のみ・適用は指示待ち）

before:
```
- `[Pathfinding]`、`[HAVOK]`、`[Debug]`、`[ShockBolt]`、`[Absorb]` などはデバッグや演出用で、移植の優先度は低い。
```
after:
```
- `[Pathfinding]`、`[HAVOK]`、`[Debug]` はデバッグや演出用の登録済み INI セクションで、移植の優先度は低い（エンジン実登録: Pathfinding 7 / HAVOK 23 / Debug 3、V91 実測）。
- `[ShockBolt]`、`[Absorb]` は**エンジン未登録のセクション**（INI レジストリ 0 件・EXE 内にセクション文字列なし、V208/V210 実測）。記載は同梱 `Oblivion_default.ini` にのみ存在し、エンジンは読まない。`[Absorb]` の 12 キー名は GameSetting(GMST) に bare 名で別系統登録済み、`[ShockBolt]` 14 キーは GMST にも非該当（近縁 `fShockBolt*` は別名・値相違）。優先度の議論は「ファイルのみ保持」の文脈で行う。
```

## 項7. ESM 読込後の GameSetting map 実内容と 2,304 static の対応（仕様化・EXE 構造証拠のみ）

### 7-1 仕様（証拠: v160 §2 / v55 §1-2 ＋ 本タスク再測 `v218_7.py`）

**データ構造**
- GameSetting map = GMST マネージャ `0xB35468` の埋め込みハッシュマップ `this+0x10C = 0xB35574`（401 bucket、hash=djb2 `0x4A80A0`、compare=`__stricmp`）。ノードは `{next, char* name, Setting*}`、Setting オブジェクトは **8B `{u32/f32 value, char* name}`・type フィールド無し**。
- static 登録 2,304 件（FUN_00419A70 raw32 版 1,312 ＋ FUN_0041BAE0 float 版 992、CRT 初期化帯のみ、重複名 0）が **.bss の静的 Setting オブジェクト**として全数挿入される（＝s50 の母集団）。

**ESM 読込時の挙動（構造証拠）**
1. GMST レコードハンドラ `0x44E326` は EDID を取り `slot9 = 0x4A8040` を thiscall。
2. `0x4A8040`（本タスク再測）: `lea ecx,[esi+0x10C]; call 0x55E000`(lookup) → **`cmp eax,ebx(0); je 0x4A8080`**（**未命中なら vtable+0x10 を呼ばずスキップ**）→ 命中時のみ `vtable+0x10 = 0x4A7D60`（値適用）→ 末尾常に `vtable+0x18 = 0x4A7CB0`（record ptr クリア）。**insert(0x412D30) 呼び出しは関数内に存在しない**。
3. `0x4A7D60`: `[this+0x108]`(record) → 次サブレコード前進(`0x44FEA0`) → `cmp eax,"DATA"` → name 先頭文字で型判別(`0x4A78D0`, f→float/i→int/s→string) → int/float は **`0x4510E0` で DATA 4 バイトを Setting オブジェクト本体へ直接転送**、string は `0x450C20`+`0x4A7980`。
4. これ以外の map への入口は CRT 初期化の 2 登録関数のみ（`0x412D30` の ecx=0xB35574 はその 2 サイト内のみ、v160 §2-2）。削除は shutdown ループ（`0x4524C0`, 8 箇所）と Setting dtor thunk のみ。コンソール Get/SetGameSetting(`0x513FC0`/`0x514100`) も lookup + 変数本体への直接書込で insert なし（v55 §3）。

**→ 仕様命題（ESM 読込後も成り立つ不変条件）**
- **キー集合は 2,304 のまま不変。** ESM 由来の新規 GMST（ESM 独有 4 件 `fRegionGenTex*` 等、rel6 §2.1）は **lookup 未命中で無視され map に現れない**。ESM が持つ既存 378 キーは **value のみが上書き**される（同一 8B オブジェクトの value フィールドが ESM DATA で置換）。
- **runtime 値**: 読込直前 = コンパイル既定値 2,304 件。読込後 = 「ESM に在る 378 件は ESM 値（うち 226 件が既定と相違、rel6）／残 1,926 件はコンパイル既定値」。ESM 無し起動なら 2,304 件すべて既定値。
- **s50（2,304 static dump）との対応**: s50 は **静的登録表の値（ESM 読込前時点の既定値）** であり、ランタイムの実値を再現するには rel6 の ESM 上書き 378 件（移植側は `getLive` 経由）を重ねる必要がある。キー集合については s50 = ランタイム map と**完全一致**（増減なし）。
- **コンテナ分離**: 上記は GameSetting map 0xB35574 の話であり、INI 既定値レジストリ（0xB07BF0 等、ノードは `{value, name="Key:Section"}`）とは**別系統・別ノード**。`fGlobalTimeMultiplier:General`（項1）のように「INI ノードのみ」のキーは GameSetting map に存在せず、ESM 経由の上書き対象にもならない。

### 7-2 判定

**仕様化として解明**（ESM 本体は再スキャン禁止を遵守し、EXE 構造証拠 v55/v160 ＋ 本タスクの 0x4A8040/0x4A7D60 再逆アセンブルで確定）。「2,304 static ↔ runtime map」は **キー集合は恒等、値は ESM 378 件のみ差分**という対応。

---

## 8. 新規残点（本タスクで新たに発生）

1. **wrapper 経由 INI ノードの全数化**（項2 補遺2）: `FUN_00404850`(float)/`FUN_00444060`(dword) の全 caller を辿って実行時生成ノードを列挙し、`copilot_ini_registries.txt` の補遺対象を `fAlpha:Water`/`uSurfaceTextureSize:Water` の 2 件以外にも拡げるか（v91 抽出漏れの全数確定）。優先度 中。
2. **未登録キーを読む外部ツールの所在**（項3 の残り): エンジンは未登録キーを読まないと確定したが、同梱 `Oblivion_default.ini` の `[ShockBolt]`/`[Absorb]`/Xenon 閾値等を消費するのは **GECK/CS 側バイナリ**（存在すれば）の可能性。GECK.exe の有無確認は別タスク。優先度 中。
3. **Controls 表記差の製品判断**（項4）: RE 事実認定は完了（正=空白なし）。移植が「互換の完全再現（空白付き無視）」か「両表記許容（設定を実際に効かせる）」かは [PORT] 側の決定事項。優先度 中。
4. **crosscheck2 §3 の二重計上**（項5）: `fQuestScriptDelayTime` が only-in-A(MAIN) と only-in-B([General]) に両出している結合アーティファクトは、他の ':' 無しキー（`sBasePath` 等 4 件）にも理論上起きうるが、同梱ファイルに載っていないため実害なし。crosscheck2 を再生成する場合はセクション正規化（':' 無し→MAIN）を結合キーに含めるべき。優先度 低。
5. **`uQuadraticMethod` 隣の 0xB08164**（項2）: node 0xB08160(uQuadraticMethod) の name* スロットであり独立ノードではないことを実測で確定。v208 の「node 0xB08138–0xB08190」は「13 スロット中 12 ノード」の注記付きで読むこと。優先度 低（メモ）。

## 9. 検証情報

- EXE: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（image_base 0x400000, x86 PE32, 7,898,624 B, SHA-256 `a8f313845c1545e9a60e1e995961eef4c033115da9443f6d756341df3c2b7dc6`）。比較用に `D:\Cargo\Ghidra\temp\OblivionStandalone.exe`（同 SHA-256 = 同一バイナリ）。
- capstone 5.0.7（skipdata）、.text VA 0x401000/file 0x400、.rdata VA 0xA28000/file 0x627200、.data VA 0xB02000/file 0x700C00。
- selftest: 着手時 **4/4 PASS**（`probe_harness.py selftest`、変更なし）。
- スクリプト（新規のみ）: `%TEMP%\opencode\v218_1.py` `v218_1b.py` `v218_2.py` `v218_2b.py` `v218_2c.py` `v218_2d.py` `v218_3.py` `v218_3b.py` `v218_4.py` `v218_4b.py` `v218_4c.py` `v218_4d.py` `v218_4e.py` `v218_5.py` `v218_5b.py` `v218_5c.py` `v218_5d.py` `v218_5e.py` `v218_5f.py` `v218_5g.py` `v218_7.py` ＋ 各 `_out.txt`。
- ESM 本体: **未アクセス**（禁止遵守）。項7 は EXE コードの構造証拠（v55/v160 成果物＋本タスク再逆アセンブル）のみ。
- 読取のみ（無編集）: `docs/RE_EXE_REPORT.md`（§43・L816・§54.3 該当行のみ）/ `tmp/re/v208_gmst_mixed_names.md` / `v210_84keys_origin.md` / `v209_rdebugtextcolor.md` / `v130_ini_residuals.md` / `v137_v80_global_residuals.md` / `v160_ini_log_residuals.md` / `relcheck_20261029/rel6_gmst3way.md` / `copilot_ini_registries.txt` / `copilot_ini_crosscheck2.md` / `v91_ini_defaults.csv` / `v51_inisettings_spec.md` / `v55_gmst_override_path.md` / `tmp/re/RE_EXE_REPORT.md.pre80b/83/84` / `cache/named_funcs` `cache/call_targets`。
- 編集禁止事項遵守: git / docs / GT / TODO / session-memory / plan / cache / ソース / 既存 tmp/re は一切変更せず。成果物は `tmp/re/v218_ini_docs_bundle3.md` **新規 1 ファイルのみ**（本ファイル）。
