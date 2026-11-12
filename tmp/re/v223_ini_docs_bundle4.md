# V223: INI/文書整備束第4 — wrapper 経由ノード全数化・GECK 消費者不在裁定・Controls 判断材料・crosscheck2 正規化方針・0xB08164 name* 注記

- 日付: 2026-11-01 / 対象: `Oblivion.exe` v1.2.0416 Steam（7,898,624 B, `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`）
- 出典（残点）: `tmp/re/v218_ini_docs_bundle3.md` §8 新規残点5件（本体読取済）。確定済前提: §43 訂正3件・Controls 正=空白なし・母集団差=同一バイナリ
- 手法: capstone 5.0.7（skipdata）＋ `probe_harness`（selftest 着手時 **4/4 PASS**・変更なし）＋ `v172_common`。ESM 本体不使用（禁止遵守）。既存 `tmp/re` 成果物・docs/GT/TODO/cache は読取のみ・無編集。
- スクリプト: `%TEMP%\opencode\v223_1.py` `v223_1b.py` `v223_1c.py` `v223_3.py` `v223_4.py`（各 `_out.txt` 付き）
- PE マップ: image_base 0x400000 / .text VA 0x401000 raw 0x400 / .rdata VA 0xA28000 raw 0x627200 / .data VA 0xB02000 raw 0x700C00（VA−file_off = 0x400C00(.text) / 0x400E00(.rdata) / 0x401400(.data)）。**.bss 領域（例 0xB3524C）は file 非 backed＝静的読取不可**（va2off が None を返す）
- 環境注記: `copilot_ini_registries.txt` / `copilot_ini_crosscheck2.md` / `v91_ini_defaults.csv` は**本作業環境に不在**（プロジェクト内 glob 0 件。v209 が registries.txt の不在を既報）。v218 が引用する数値の裏取りは EXE 再測で行った。

---

## 項1. wrapper 経由 INI ノードの全数化 — **解明（補遺は既知 2 件で全数確定・拡張不要）**

### 1-1 wrapper の caller 全数（`v223_1.py` — E8 直接呼出し全走査＋cache 突合）

| wrapper | E8 caller 数 | caller site | node（ecx 入力） | name（"Key:Section"） | 既定値 | 備考 |
|---|---|---|---|---|---|---|
| `FUN_00404850`(float) | **1**（cache/call_targets も 1） | 0x0049D546 | **0x00B3524C** | `fAlpha:Water`(name* 0xA3F43C) | **0x3F000000 (0.5f)**、fld src = [0x00A3D65C] 実読 | v218 補遺2ドラフトの「0x00000000」は**誤り。正 = 0.5f** |
| `FUN_00444060`(dword) | **1**（cache/call_targets も 1） | 0x0049DD72 | **0x00B35264** | `uSurfaceTextureSize:Water`(name* 0xA3F4C4) | 0x00000080 (128) | v218 補遺2ドラフトと一致 |

- caller のパラテン: `push <value>` → `push <name文字列>` → **`mov ecx,<node固定VA>`** → `call wrapper`。wrapper 本体は `[esi]=value / [esi+4]=name`（esi=ecx=node）→ `push esi; mov ecx,0xB07BF0; call 0x4040E0`(INI_REG_ADD)。
- **「node は実行時生成」（v218 §2-1）の精緻化**: node の VA は**コンパイル時固定**（0xB3524C / 0xB35264）で **.bss 領域**（file 非 backed・静的イメージに内容無し、name* は wrapper 実行時に書かれる）。"実行時生成" は ".data 静的ノードが無い" 意味であり、VA 自体は静的。したがって **`mov ecx,<node>; call wrapper` パタテンで静的検出可能**（`push node; call 0x4040E0` パタテンでは引き続き検出不能）。

### 1-2 完全性証明 — INI_REG_ADD(0x4040E0) 全 637 呼出しの分類（`v223_1b.py`）

| 分類 | 件数 | 内訳 |
|---|---|---|
| 静的 node（直前 `push imm32`(>=0xB00000)） | **635** | `mov ecx` 容器分布: **0xB07BF0=572 / 0xB11C40=62 / 0xB07D28=1**（v91 CSV の 635 行 = 572+62+1 と**一致**） |
| レジスタ経由（wrapper 内部 `push esi`） | **2** | 0x404893（FUN_00404850 内）/ 0x4440A3（FUN_00444060 内）— 上記 2 caller の到達点 |

→ **INI 既定値レジストリの総ノード数 = 637（635 静的 + 2 wrapper）**。wrapper 経由系統は **既知 2 件（fAlpha:Water / uSurfaceTextureSize:Water）以外に存在しない**。
→ **v91 抽出漏れはこの 2 件で全数確定**（v210 §1-3 の指摘と一致）。`copilot_ini_registries.txt` への補遺は v218 補遺1（bLightAttenuation 12 件）＋補遺2（wrapper 2 件）の**現行構成のままで正当・拡張不要**。ただし補遺2の fAlpha 既定値は上記のとおり 0.5f に訂正すること。

### 1-3 既知 12 件（bLightAttenuation）との突合（`v223_1c.py`）

- CRT 初期化帯 0x9E2200–0x9E2700 で容器 0xB07BF0 を呼出す site = **12 件ちょうど**（site 0x9E2262..0x9E2682、node 0xB08138..0xB08190 を 8B 構造で push）。キー・値・name* は v218 §2-1 表と**全項目一致**（再実読）。
- wrapper 群の node（0xB3524C/0xB35264）は .bss の別アドレス領域で**重複なし**。wrapper 経由が 12 件群を説明することもない（12 件は全て静的 `push node` site）。
- v208 §1.2 の「node 0xB08138–0xB08190」表記問題（0xB0815C/0xB08164）は**項5**に譲る。

### 1-4 判定

**解明。** wrapper 系統は 2 件で全数確定（637 = 635 + 2 の恒等式で裏固め）。補遺対象の拡張は不要。付随訂正: 補遺2ドラフトの `fAlpha:Water` 既定値は **0x00000000 → 0.5f (0x3F000000)**。

---

## 項2. 未登録キーを消費する外部ツールの所在 — **解明（本作業環境内に消費者なし。GECK 本体は不在）**

### 2-1 検索実績（`shell` 実行・2026-11-01）

| 範囲 | 結果 |
|---|---|
| `D:\` 直下＋再帰深3 / `D:\Cargo` 全配下（BSA_Browser, Ghidra×2, ghidra-build, GTAVSP, Image, Index, jdk-21, Oblivion_Android, Other, ReaProj, ReverseEngineering, Schwarz, WS 等） | `GECK*.exe` / `CreationKit*.exe` **0 件**。名前 hit は `D:\Cargo\Other\Oblivion\winlator-11.0.0\wine_addons\wine-gecko-2.47.4-x86_64.msi` のみ（Mozilla Gecko ＝ 無関係） |
| Steam: `C:\Program Files (x86)\Steam\steamapps\common` | ゲームディレクトリは **Oblivion のみ**。同フォルダ内 exe = `Oblivion.exe` / `OblivionLauncher.exe` の2件のみ |
| `D:\SteamLibrary` / `C:\Games` | **存在せず** |

### 2-2 同梱バイナリの消費キー走査（`OblivionLauncher.exe` 1,662,976 B、ASCII リテラル数え上げ）

`ShockBolt` **0** / `Absorb` **0** / `Xenon` **0** / `Controls` **0** / `Slide` **0** / `GetPrivateProfile` **1** / `Oblivion.ini` **1**
→ ランチャーは `Oblivion.ini` を**何か1箇所で読む**（`[Launcher]` 系と推定）が、**[ShockBolt]/[Absorb]/Xenon 閾値/Controls 系のキーは消費しない**。

### 2-3 裁定

1. **エンジン（Oblivion.exe）**: 未登録キーを読まない（v218 項3: registry-driven 読込・`GetPrivateProfileSectionA` 未インポート・ファイル側キー列挙経路なし）— 本タスクでも変化なし。
2. **環境内の他のバイナリ（OblivionLauncher.exe）**: 上記のとおり [ShockBolt]/[Absorb]/Xenon/Controls を消費しない。
3. **GECK/CS 側**: **本作業環境に GECK.exe / CreationKit 系バイナリが存在しない**（＝エンジン以外の消費者なし）。したがって同梱 `Oblivion_default.ini` の未登録セクション・Xenon 閾値は**現環境では「ファイルのみ保持」で完結**する。
4. **限定**: GECK 本体を入手した場合の挙動は**未検証**（RE 範囲外＝ソフト不在のため）。 Bethesda 配布の GECK は本バイナリ群に含まれない。

### 2-4 判定

**解明（環境内不在裁定）**。「エンジン以外の消費者なし」を現環境の全バイナリについて確定。GECK 入手時の追加検証のみ残点（項6）。

---

## 項3. Controls 表記差の製品判断材料 — **解明（[PORT] 決定用に 1 セクションへ集約）**

> RE 側の事実認定は完了（正 = 空白なし・v218 項4）。本項は [PORT] 側が「互換の完全再現（空白付き無視）」か「両表記許容」かを判断するために必要な情報をすべてここに集約する。**製品判断自体は [PORT] 側の決定事項**。

### 3-1 エンジン側の確定事実（本タスク再実読 `v223_3.py`、v218 §4-1 / v210 §3.2 と一致）

1. **既定バインディング名テーブル .data 0xB02BB8 = 29 エントリを全数再実読**（スロット [29] は 0x1 でテーブル終端）:

   | # | 名前 | # | 名前 | # | 名前 |
   |---|---|---|---|---|---|
   | 0 | `Forward` | 10 | `Run` | 20 | `Quick3` |
   | 1 | `Back` | 11 | `Always Run`（空白あり） | 21 | `Quick4` |
   | 2 | **`SlideLeft`**（0xA2FA58・空白なし） | 12 | `Auto Move`（空白あり） | 22 | `Quick5` |
   | 3 | **`SlideRight`**（0xA2FA4C・空白なし） | 13 | `Jump` | 23 | `Quick6` |
   | 4 | `Use` | 14 | `Toggle POV`（空白あり） | 24 | `Quick7` |
   | 5 | `Activate` | 15 | `Menu Mode`（空白あり） | 25 | `Quick8` |
   | 6 | `Block` | 16 | `Rest` | 26 | `QuickSave` |
   | 7 | `Cast` | 17 | `Quick Menu`（空白あり） | 27 | `QuickLoad` |
   | 8 | `Ready Item`（空白あり） | 18 | `Quick1` | 28 | `Grab` |
   | 9 | `Crouch/Sneak`（空白あり） | 19 | `Quick2` | | |

2. **読込パス 0x404610**（再逆アセンブル確認）: `mov edx,[esi*4+0xB02BB8]` → `GetPrivateProfileStringA("Controls"(0xA2FAF8), edx, buf, 0x40, 0, …)`（call edi）→ 戻り値 0 ならスキップ → `call 0x982B96`(strtoul base16) → **3 平面へ復元**: `[this+esi+0x1B7E]=(v>>0x10) / +0x1B9B]=(v>>8) / +0x1BB8]=v&0xFF` → ループ `add esi,1; cmp esi,0x1D(29); jl`。**使用名はテーブル由来のみ・空白付き文字列は一切現れない**。
3. **書出パス 0x404400**: 同テーブル 29 件を `WritePrivateProfileStringA("Controls", …)` へ（v210 §3.2）。**書出も必ず空白なし**。

### 3-2 空白付き `Slide Left` / `Slide Right` の正体（GMST との関係整理・再確認）

- 文字列 `Slide Left`(0xA6283C) / `Slide Right`(0xA62820) の **.text 内 imm32 参照は各 1 件のみ**（実測: 0xA6283C → 0x9F60EF の 1 箇所 / 0xA62820 → 0x9F6127 の 1 箇所）。
- その 1 箇所ずつが **GameSetting(GMST) 既定値の登録**である:
  - `0x9F60EF`: `push 0xA6283C("Slide Left"); push 0xA6282C("sUActnSldleft"); call 0x419A70`（GameSetting map 登録＝v218 項7 の系統）
  - `0x9F6127`: `push 0xA62820("Slide Right"); push 0xA62810("sUActnSldright"); call 0x419A70`
- **関係整理**: INI キー `SlideLeft`(テーブル 0xB02BC0) と GMST `sUActnSldleft`（値 = 表示名 `"Slide Left"`）は**別系統の別物**。GMST 側はアクションの**表示名**（UI 用文字列）、INI 側は**バインディングキー**であり、エンジンは GMST 値から INI キーを導出しない（逆方向もない）。したがって「同梱 ini の `Slide Left=` が GMST 経由で拾われる」こともない。

### 3-3 同梱ファイル側の事実（`C:\Program Files (x86)\Steam\steamapps\common\Oblivion\Oblivion_default.ini` 実読）

- 行 133: `Slide Left=001EFFFF` / 行 134: `Slide Right=0020FFFF`（**空白付き**）。
- [Controls] セクションの他 27 バインディング名（`Ready Item` `Always Run` `Menu Mode` `Quick Menu` 等、空白を含む名も含む）は**テーブルと完全一致**。表記差は**この 2 キーのみ**。
- **元 Oblivion での実効**: エンジンは `SlideLeft` でしか叩かないため、この 2 行は**無視され**、ユーザーは既定バインディングのまま動作する（コピーしても壊れないが設定が効かない）。空白付きキーに値を書いても次回以降読まれない。

### 3-4 [PORT] 決定オプション（判断材料の一覧）

| # | 方針 | 内容 | 元ゲームとの差 | 実装コスト | リスク |
|---|---|---|---|---|---|
| A | **互換の完全再現（空白付き無視）** | 読込キーはテーブル 29 件（空白なし）のみ。同梱 ini の `Slide Left`/`Slide Right` 行は元版同様に無視 | **なし（1:1 再現）** | 最小（正規化コード不要） | ユーザーが同梱 ini をコピーすると 2 キーが効かない＝**元版と同じ挙動**（バグ再現） |
| B | **両表記許容（読込時正規化 1 本）** | 読込時に `"Slide Left"→"SlideLeft"` / `"Slide Right"→"SlideRight"` のマッピングを追加。書出は空白なしのまま | **差異あり**（元版が無視する 2 キーを有効化するバグフィックス寄りの拡張） | 小（2 キーの正規化テーブル） | 「元版の意図的挙動かバグか」の解釈次第。正規化を実装した場合のみ、ユーザー設定が実際に効く |
| C（非推奨） | 空白付きで書出 | 書出時も空白付きにする | 大きく逸脱 | — | **次回読込で無視され設定が消える**。エンジン parity に反する。不可 |

### 3-5 判断のポイント（要約）

1. 影響範囲は **29 キー中 2 キーのみ**。他の 27 キーは表記一致で問題なし。
2. 書出の正 = **空白なし一択**（エンジンと同じ。空白付きを書くと自壊する）。
3. A/B の選択は RE 事実ではなく**製品判断**: 「元版の癖（無視）まで再現するか」「同梱 ini が実際に効くようにするか」。RE 側からの推奨は**忠実再現なら A**（実装も最小）。B を選ぶ場合は差異（元版からのおかし修正）として changelog に明記すること。
4. GMST `sUActnSldleft/right` は表示名系統として**別途維持**（INI 方針の影響を受けない）。

### 3-6 判定

**解明（判断材料を本項に集約）。** [PORT] 側の A/B 選択のみ未決（項6 残点として記録）。

---

## 項4. crosscheck2 再生成時のセクション正規化方針 — **解明（方針文書化・crosscheck2 本体は無編集・再生成なし）**

### 4-1 事実: ':' 無し名（セクションレス）ノードの全数確定（`v223_4.py` — 637 件全ノード名実読）

INI_REG_ADD 全 637 件の node name* を実読して分類した結果:

| 分類 | 件数 |
|---|---|
| 混在名 `Key:Section`（':' あり） | **631（静的）+ 2（wrapper: fAlpha:Water / uSurfaceTextureSize:Water）= 633** |
| **bare 名（':' 無し）** | **4** |
| 静的 name* 未解決（wrapper の .bss node 2 件。名前は caller 側で確定済） | 2 |

**':' 無しノード 4 件の全数リスト**（エンジン側 ReadOne の section 分割ヘルパ 0x4A8430 は ':' 無し name に既定 **MAIN**(0xA404C4) を割り当てる — v218 項5-2）:

| bare キー | node | INI_REG_ADD site | 容器 |
|---|---|---|---|
| `fLowPerfCombatantVoiceDistance` | 0x00B14BAC | 0x009FD542 | 0xB07BF0 |
| `iDetectionHighNumPicks` | 0x00B14904 | 0x009FD272 | 0xB07BF0 |
| `iLastHDRSetting` | 0x00B06F54 | 0x009DEEF2 | 0xB07BF0 |
| `sBasePath` | 0x00B0472C | 0x009DAC82 | **0xB07D28**（同容器唯一ノード） |

### 4-2 既知アーティファクト（v218 項5-2）

`fQuestScriptDelayTime`（node 0xB09E28・name は ':' 無し → エンジン側 MAIN 扱い）は、同梱 `Oblivion_default.ini` では `[General]` 配置。crosscheck2 のキー結合（セクション名一致が条件）では **only-in-A(MAIN) と only-in-B([General]) に同時に出現**し、同一キーが二重計上される（crosscheck2 L271 と L406、v218 引用）。

### 4-3 正規化方針（crosscheck2 再生成時の必須要件）

1. **結合キーを正規化する**: `(norm_key, norm_section)` で結合する。`norm_section` = name に ':' があれば **':' の後ろ**、':' 無しなら **`MAIN`**（エンジン 0x4A8430 の既定割当に合わせる）。`norm_key` = ':' 前半（bare 名はそのまま）。
2. これにより、上記 4 bare キー + `fQuestScriptDelayTime` は A/B 両側で**同一キーとして正しく突合**され、only-in-A/only-in-B の二重計上が消える。
3. A 側ノード側も正規化をかけること（bare 名を MAIN 扱いにしてから結合）。片側だけ正規化してもアーティファクトは残る。
4. 実害評価（v218 §8-3 と同じ結論を维持）: 4 bare キーのうち同梱ファイルに載るのは該当箇所が確認されておらず、**現時点で実害なし**。再生成時のデータ品質のための正規化である。
5. 付記: `copilot_ini_registries.txt` のヘッダ「skipped: unresolvable name 3」は、本タスクの E8 起点走査では**635 静的 node すべての name* が実読可能だった**（＝スキャン方式差）。registries.txt 側の 3 件の実体特定は、同ファイルが本環境に不在のため未突合（項6 残点）。

### 4-4 判定

**解明（方針を文書化）。** crosscheck2 本体は読取のみ・再生成は本タスクでは行っていない。

---

## 項5. `0xB08164` = name* スロット注記 — **解明（確定メモ）**

> **確定メモ**: v208 の「node 0xB08138–0xB08190」は **「13 スロット中 12 ノード」**の注記付きで読むこと。0xB0815C / 0xB08164 は独立ノードではなく **name* スロット**（uLinearMethod / uQuadraticMethod ノードの +4）である。

### 5-1 実測（`v223_1c.py`）

| 検査 | 結果 |
|---|---|
| `dword[0x00B08164]` 実読 | `0x00A433D0` → `"uQuadraticMethod:bLightAttenuation"`（＝**node 0xB08160 の name* スロット**） |
| `dword[0x00B0815C]` 実読 | `0x00A433B0` → `"uLinearMethod:bLightAttenuation"`（＝**node 0xB08158 の name* スロット**） |
| 12 登録 site（0x9E2262..0x9E2682）の `push imm32` 演算子 | node 0xB08138..0xB08190（8B 構造 × 12）のみ出现。**0xB0815C / 0xB08164 を push する INI_REG_ADD site は 0 件** |
| 登録 site の実例 | 0x9E23D0: `push 0xB08158; mov ecx,0xB07BF0; call 0x4040E0`(0x9E23E2) / 0x9E2430: `push 0xB08160; …`(0x9E2442) |
| `.text` 内 imm32 参照: 0xB0815C / 0xB08164 | **各 1 件のみ**（0xA1B48F / 0xA1B4BF）。実体は name 参照 thunk: `0xA1B48F: mov eax,[0xB0815C]; test eax,eax; je …; cmp byte ptr [eax],0x53; …`（name* 経由で文字列を読む経路）。ノード参照（6 件 each）とは別系統 |
| `.text` 内 imm32 参照: 0xB08158 / 0xB08160 | 各 6 件（登録 push・getter thunk `0xA1B480`/`0xA1B4B0` 帯 `push node; mov ecx,0xB07CFC; call 0x65C620` 等） |

### 5-2 解釈

- v208 §1.2 の「命名規則外 2 件（uLinearMethod / uQuadraticMethod, **node 0xB0815C/0xB08164**）」表記は **name* スロットの誤認**。正しい node は **0xB08158 / 0xB08160**（v218 §2-1 表が正しい。本タスクで再確認）。
- 「13 スロット中 12 ノード」の意味: 0xB08138–0xB08190 帯には 8B 構造ノードが 12 個（8B × 12 = 0xB08138..0xB08197）並ぶうち、v208 が数えた 13 個目の「ノード」0xB08164 は 0xB08160 ノードの name* スロット（+4）でありノードではない。
- 補足観測（未調査・項6）: getter thunk `0xA1B480` 帯は `push <node>; mov ecx,0xB07CFC; call 0x65C620` しており、bLightAttenuation ノードが**容器 0xB07CFC（0xB07D28 近傍の別系統）にも二次登録される経路**が存在する。

### 5-3 判定

**解明（確定メモとして本項に記載）。**

---

## 6. 新規残点（本タスクで新たに発生・または引継ぎ）

1. **GECK 本体入手時の消費検証**（項2）: 本作業環境に GECK.exe/CreationKit 系が不在のため、「[ShockBolt]/[Absorb]/Xenon 閾値を読む外部ツール」は現環境では不存在で確定。GECK を入手した場合のみ追加検証（優先度 低）。
2. **registries.txt「skipped: unresolvable name 3」の実体突合**（項4-3 付記）: 本タスクの E8 起点走査では 635 静的 node 全 name* が実読できた。registries.txt 側が「3 件」とした内訳との突合は、**同ファイルが本環境に不在**のため未実施（優先度 低）。
3. **getter thunk 帯の二次登録経路**（項5-2 補足）: `0xA1B480` 帯 `push <node>; mov ecx,0xB07CFC; call 0x65C620`（bLightAttenuation 群を容器 0xB07CFC へも登録）の機能・対象範囲は未調査（優先度 低）。
4. **[PORT] Controls 表記差の A/B 選択**（項3-4）: 判断材料は整備済。製品判断は [PORT] 側（優先度 中）。
5. **v218 補遺2ドラフトの fAlpha 既定値訂正**（項1-1）: `0x00000000` → **0.5f (0x3F000000)**。`copilot_ini_registries.txt` への補遺適用時に反映すること（本タスクでは成果物編集禁止のため未適用）。
6. **v218 §2-1 wording の精緻化候補**（項1-1）: 「node は実行時生成（静的 node 無し）」は「node VA は固定・.bss 領域（file 非 backed）」が正確。docs/GT への反映は指示待ちのドラフト扱い。

---

## 7. 検証情報

- EXE: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Oblivion.exe`（image_base 0x400000, x86 PE32, 7,898,624 B）。Steam 配下 `C:\Program Files (x86)\Steam\steamapps\common\Oblivion\` の `Oblivion.exe` / `Oblivion_default.ini`（12,766 B）/ `OblivionLauncher.exe`（1,662,976 B）は**読取のみ**。
- capstone 5.0.7（skipdata）、.text VA 0x401000/file 0x400、.rdata VA 0xA28000/file 0x627200、.data VA 0xB02000/file 0x700C00。
- selftest: 着手時 **4/4 PASS**（`probe_harness.py selftest`、変更なし）。
- スクリプト（新規のみ）: `%TEMP%\opencode\v223_1.py` `v223_1b.py` `v223_1c.py` `v223_3.py` `v223_4.py` ＋ 各 `_out.txt`。
- ESM 本体: **未アクセス**（禁止遵守）。
- 読取のみ（無編集）: `tmp/re/v218_ini_docs_bundle3.md`（本体）/ `v210_84keys_origin.md` / `v208_gmst_mixed_names.md` / `cache/named_funcs.txt` / `cache/call_targets.json` / `docs/RE_EXE_REPORT.md`（参照のみ・該当節の再確認に使用せず v218 引用分で足够）/ Steam 配下 ini/exe。
- 本環境で不在を確認: `copilot_ini_registries.txt` / `copilot_ini_crosscheck2.md` / `v91_ini_defaults.csv`（プロジェクト内 glob 0 件、Steam 配下・D:\Cargo 主要配下の探索でも未発見。数値引用は v218/v210 のものを出典付きで用い、EXE 再測で裏取り）。
- 編集禁止事項遵守: git / docs / GT / TODO / session-memory / plan / cache / ソース / 既存 tmp/re は一切変更せず。成果物は `tmp/re/v223_ini_docs_bundle4.md` **新規 1 ファイルのみ**（本ファイル）。
