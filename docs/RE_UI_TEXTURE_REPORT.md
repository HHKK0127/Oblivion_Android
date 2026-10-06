# Oblivion Steam版 リバースエンジニアリングレポート：UIテクスチャとメニューXML

解析日: 2026-10-04
対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版\Data\Oblivion - Textures - Compressed.bsa`（BSA103、1161.4 MB）
関連: [RE_FONT_REPORT.md](RE_FONT_REPORT.md)

## 1. 概要

Oblivion のUIテクスチャを Android 移植へ取り込むため、`Oblivion - Textures - Compressed.bsa` の `textures\menus*` を解析・抽出した。

**主要な結論**

1. BSA のUIテクスチャは **3段階の品質ティア**（`menus` / `menus80` / `menus50`）で格納されている。
2. **完全なセットは `textures\menus` のみ**。`menus80` / `menus50` はLOD用の部分セットであり、カテゴリ単位で丸ごと欠落するものがある。
3. 非アイコンUIテクスチャ **814枚** を抽出・PNG化した（非圧縮合計 92.2 MiB、ユニーク784枚）。
4. Oblivion のUIは **完全にデータ駆動のXML**（BSA内 `menus\` に104ファイル）。Android版にはこのXMLインタプリタが存在しない。

## 2. 品質ティア（新規解明）

BSA のフォルダマップ（1090フォルダ）から `textures\menus\d*` を集計した結果。

| ティア | フォルダ | DDS数 | 圧縮サイズ | 役割 |
|--------|----------|-------|-----------|------|
| `menus` | `textures\menus\` | 1,975 | 約 40.6 MB | **完全版（最高品質）** |
| `menus80` | `textures\menus80\` | 1,712 | 約 17.0 MB | 80% ティア（部分セット） |
| `menus50` | `textures\menus50\` | 1,789 | 約 9.6 MB | 50% ティア（部分セット） |

### ティア別カテゴリ収録数（非アイコン）

| カテゴリ | menus | menus80 | menus50 |
|----------|------:|--------:|--------:|
| icons | 1161 | 1137 | 1161 |
| stats | 133 | 133 | 133 |
| misc | **85** | 4 | 4 |
| spell effect timer | **81** | 0 | 0 |
| shared | 77 | 77 | 84 |
| book | 58 | 58 | 58 |
| map | 57 | 56 | 56 |
| loading | **53** | 5 | 53 |
| class | 43 | 43 | 43 |
| level_up | 39 | 39 | 39 |
| dialog | 28 | 28 | 28 |
| lockpicking | **14** | 2 | 2 |
| persuasion | **11** | 1 | 1 |
| credits | 4 | 4 | **0** |
| quick-keys | **4** | 1 | 1 |
| faders | **2** | 0 | 2 |

- `menus80` に存在しないカテゴリ: `spell effect timer`(81), `faders`(2)
- `menus50` に存在しないカテゴリ: `spell effect timer`(81), `credits`(4)
- `menus80`/`menus50` のみに存在するカテゴリ: なし（上位互換の関係）

**含意**: `menus80` / `menus50` は「常時フル解像度で必要なテクスチャ」を意図的に間引いたLODセットである。
移植で必要なUIテクスチャを揃える場合は `textures\menus` を正典とすること。

## 3. DDS フォーマット

展開後データは 128 バイトの NetImmerse DDS ヘッダ + ピクセルデータ。

| fourcc | 形式 | 用途例 |
|--------|------|--------|
| `DXT1` | BC1 | 汎用UI（アルファ1bit） |
| `DXT3` | BC2 | 明示アルファ付きUI |
| `DXT5` | BC3 | 主要UI背景（`center_background` 等） |
| `A` / `\0\0\0\0` + 32bpp | BGRA32 | 非圧縮UI |
| `\0\0\0\0` + **16bpp** | **RGB565** (masks R=0xF800 G=0x07E0 B=0x001F) | `misc\healthbar3dbw.dds` |
| `\0\0\0\0` + **16bpp** | **ARGB4444** (masks R=0x0F00 G=0x00F0 B=0x000F A=0xF000) | `map\world\world_map_icon_*.dds`, `world_map_marker_*.dds` |

16bpp 形式は既存デコーダで未対応だったため `tmp/dds_decode.py` に `decode_rgb565` / `decode_argb4444` / `decode_16bit` を追加した。
16bpp DDS は mip チェーンを含む場合がある（例: `healthbar3dbw.dds` 64x16 で payload 2734 B）。デコーダは先頭レベル（= ヘッダの w×h）のみを読む。

## 4. 抽出結果

| 項目 | 値 |
|------|-----|
| 抽出対象 | `textures\menus\`（アイコン除く） |
| 抽出数 | **814 PNG** |
| 非圧縮PNG合計 | **92.2 MiB** |
| ユニーク（SHA1） | 784（24重複グループ） |
| 出力先 | `tmp/extracted_menus_full/menus/` |
| マニフェスト | `tmp/ui_texture_manifest.json`（3ティア 5,476エントリ） |
| 分析レポート | `tmp/ui_texture_report.json` / `.txt` |

### 解像度分布（上位）

| 解像度 | 枚数 | 代表カテゴリ |
|--------|-----:|--------------|
| 128x128 | 147 | icons/stats/shared |
| 64x64 | 107 | shared/hud |
| 512x512 | 86 | shared/stats/book |
| 128x32 | 81 | spell effect timer |
| 256x256 | 66 | shared/misc |
| 1024x512 | 47 | loading（背景画） |
| 32x32 | 47 | map_world アイコン |
| 1024x1024 | 39 | 主要UI背景（DXT5） |
| 2048x2048 | 1 | `map\world\cyrodiil_resized.dds` |

### 最大テクスチャ

| サイズ | 解像度 | ファイル |
|--------|--------|----------|
| 5,329.2 KB | 2048x2048 | `map_world__cyrodiil_resized.png` |
| 1,299.0 KB | 1024x1024 | `loading__loading_background.png` |
| 1,190.6 KB | 1024x1024 | `genericbackground__center_background.png` |
| 1,153.1 KB | 1024x1024 | `lockpicking__base.png` |
| 1,034.9 KB | 1024x1024 | `container__cont_background.png` |

### 重複例

- `container__cont_button_long_off.png` = `shared__shared_button_long_off.png`
- `hud__hud_ribbon_fatigue_empty.png` = `hud__hud_ribbon_health_empty.png` = `hud__hud_ribbon_magic_empty.png`
- `inventory__inv_equiped_marker_{1,3,5}.png` / `{2,4,6}.png`
- `shared__shared_border_horizontal_1.png` = `stats__stat_border_horizontal_1.png` = `stats__stat_fact_border_horizontal_1.png`

## 5. Android版の現状とギャップ

`app/src/main/assets/textures/ui/` 配下は **40 PNG**（うち git 追跡は 33 枚）。

| 内訳 | 数 |
|------|----:|
| トップレベル（logo / loading / dialog_selection） | 7（**全て未追跡**） |
| `icons/` | 7（BSAに存在しない自作プレースホルダ） |
| `inventory/` | 26 |

`textures\menus` の非アイコン814枚に対して **814枚が未統合**。
`tmp/extracted/` に抽出済みのアイコン934枚（`icons_*` 839枚）も未統合。

### 品質ティアの混在（新規解明・画素照合で確定）

既存40枚について、BSA内の同名 DDS を**全ティア分デコードして画素単位で照合**した
（`tmp/android_tier_origin.py` / 結果 `tmp/android_tier_origin.json`）。

| 判定 | 枚数 | ファイル |
|------|-----:|---------|
| `menus80` と画素完全一致 | 29 | `inventory/*` 全26枚, `dialog_selection_cut.png`, `dialog_selection_full.png`, `loading_symbol.png` |
| `menus` と画素完全一致 | 3 | `load_in_game_default.png`, `tes_oblivion_logo_bink.png`, `tes_oblivion_logo_final.png` |
| **どのティアとも一致しない** | 1 | `loading_background.png` |
| BSAに同名なし | 7 | `icons/icon_*.png` |

#### `loading_background.png` の出自（重要な訂正）

`loading_background.dds` は `menus`(1024x1024, DXT3, 625,418 B) と `menus50`(512x512) に存在し、
`menus80\loading\` には存在しない（`_loading screens here must be 100%.txt` の指定どおり）。

Android版 `loading_background.png` は **どちらの `loading_background.dds` とも一致しない**
（menus と max diff 214 / 平均 19.8(R) 22.5(G) 25.1(B)、menus50 を1024へ拡大しても同程度）。

一方で **`textures\menus\genericbackground\center_background.dds` とは max diff 0（完全一致）**。
SHA256 も一致（`4B04343F...`）。

```
app/src/main/assets/textures/ui/loading_background.png
  == tmp/extracted_menus_full/menus/genericbackground__center_background.png  (max diff 0)
  != tmp/extracted_menus_full/menus/loading__loading_background.png           (max diff 214)
```

両者は **同一の絵柄**（正規化相互相関 NCC = 0.888）で、トーンだけが異なる
（`center_background` 平均輝度 213.0 / `loading_background` 192.1）。単純なガンマ変換では一致しない
（center^1.22 でも平均差 18.0）ため、別々にグレーディングされた2バリアントと推定。

| 参照元 | 参照先 |
|--------|--------|
| `menus\loading_menu.xml`, `menus\options\main_menu.xml`, `master_menu_file.txt` | `Menus\Loading\loading_background.dds` |
| `menus\prefabs\generic_background.xml` | `Menus\GenericBackground\center_background.dds` |

**結論**: Android版は「ローディング画面用」ではなく「汎用メニュー背景用」のバリアントを流用している。
絵柄は同じなので実害は小さいが、原作のローディング画面に忠実にしたい場合は
`Menus\Loading\loading_background.dds` から再抽出して差し替える。

#### git 追跡状況

`icons/` 7枚と `inventory/` 26枚の計33枚のみコミット済み。
トップレベル7枚（`dialog_selection_cut.png`, `dialog_selection_full.png`, `load_in_game_default.png`,
`loading_background.png`, `loading_symbol.png`, `tes_oblivion_logo_bink.png`, `tes_oblivion_logo_final.png`）は**未追跡**。

#### 付随発見

- `textures\menus\loading\load_troll.psd`（**4,075,177 B**）— BSA に Photoshop 元データが混入。`load_troll.dds` の制作元。
- `menus\loading_menu.xml` は `loading_background.dds` を1枚だけ参照する単純な構成（4,856 B）。
- BSA 内に Bethesda の作業メモ `_loading screens here must be 100%.txt` / `_loading screens here should be 50%.txt`（80 B）が同梱。

### 品質ティアの性質（3ティアを全数デコードして確定）

各ティアの同名 DDS ヘッダと画素を全数照合した結果、3ティアの関係は次のとおり。

| ティア | 非アイコン枚数 | 解像度 | 格納形式 | PNG合計 | 実体 |
|--------|--------------:|--------|----------|--------:|------|
| `menus` | 814 | 原寸 | 無圧縮 BGRA32 / ARGB4444 / DXT5 | 92.21 MiB | **最高品質（マスター）** |
| `menus80` | 575 | 原寸 | DXT3 | 38.74 MiB | 原寸のまま DXT3 化した別物 |
| `menus50` | 628 | 長辺 1/2 | DXT3 | 25.00 MiB | **`menus` と同一絵柄の忠実な縮小版** |

ヘッダ実測例:

| ファイル | `menus` | `menus80` | `menus50` |
|----------|---------|-----------|-----------|
| `shared\general_scrollbar_up.dds` | 32x32 **DXT5** | 32x32 **DXT3** | 16x16 DXT3 |
| `dialog\dialog_button_training_hit.dds` | 64x64 **DXT5** | 64x64 **DXT3** | 32x32 DXT3 |
| `map\world\world_map_icon_elven_ruin.dds` | 32x32 **ARGB4444(16bpp)** | 32x32 **DXT3** | 16x16 DXT3 |
| `dialog\dialog_selection_full.dds` | 2048x64 **BGRA32(32bpp)** | 2048x64 **DXT3** | 1024x32 DXT3 |
| `container\cont_icon_inv_on.dds` | 128x128 DXT5 | **128x64** DXT5 | 64x64 DXT5 |

#### 重要な訂正: `menus80` は `menus` の単純な低品質版ではない

`menus` と `menus80` の画素を比較すると、差が量子化誤差に収まらないファイルがある。

`shared\general_scrollbar_up.dds`（両者 32x32）:

| ティア | R | G | B | A |
|--------|--:|--:|--:|--:|
| `menus` | 167.7 | 142.6 | 95.3 | 49.8 |
| `menus80` | 199.0 | 181.8 | 152.8 | 31.8 |
| `menus50` | 168.1 | 143.9 | 99.4 | 50.7 |

`menus` と `menus50` はチャンネル平均がほぼ一致する（＝同一絵柄の縮小版）のに対し、
`menus80` は **原寸のまま別の絵**になっている。両者とも不透明な画素だけを比べても
RGB 差 max 215 / mean 43.5（`dialog\dialog_button_training_hit.dds`）に達し、
アルファのエッジ処理も異なる（`menus` は 2値、`menus80` はアンチエイリアス済み）。

デコーダ自体は Pillow のネイティブ DDS デコードと max diff 0 で一致することを確認済み
（`tmp/dds_decode.py`）。したがってこの差はデータ側の性質である。

#### ティアの包含関係

- `menus80` が `menus` / `menus50` に対して新規に追加するテクスチャは **0枚**。
  → `menus80` を採用する理由がない。
- `menus` に存在せず下位ティアにのみ存在するテクスチャが **8枚**ある。

| ファイル | 存在するティア |
|----------|----------------|
| `container__cont_box_background` | menus80, menus50 |
| `shared__general_scrollbar_hor` | menus50 |
| `shared__general_scrollbar_hor_left` | menus50 |
| `shared__general_scrollbar_hor_marker` | menus50 |
| `shared__general_scrollbar_hor_right` | menus50 |
| `shared__general_scrollbar_line` | menus50 |
| `shared__general_scrollbar_marker_bottom` | menus50 |
| `shared__general_scrollbar_marker_middle` | menus50 |

**完全な集合 = `menus` 814枚 + `menus50` の上記8枚 = 822枚。**

#### サイズ試算（822枚、PNG、LANCZOS縮小）

| 方針 | MiB |
|------|----:|
| `menus` そのまま | 92.21 |
| `menus` 長辺1024上限 + loading 1024x512→512x256 | 66.55 |
| `menus` 長辺512上限 | 48.13 |
| `menus` 全辺1/2 | 36.17 |
| `menus50`（Bethesda製1/2、628枚のみ） | 25.00 |
| `menus` 長辺256上限 | 19.94 |

圧縮テクスチャ（ETC2/BC3 = 8bpp）で持つ場合の理論値:

| 解像度 | 画素数 | 8bpp | 4bpp(BC1/ETC2 RGB) |
|--------|-------:|-----:|-------------------:|
| 原寸 | 111.67 Mpix | 111.67 MiB | 55.83 MiB |
| 1/2 | 26.87 Mpix | 26.87 MiB | 13.43 MiB |

#### サイズの偏り

`menus` 92.21 MiB の内訳は上位に極端に偏る。

| 解像度 | 枚数 | 合計 |
|--------|-----:|-----:|
| 1024x512 | 47 | 37.43 MiB |
| 1024x1024 | 39 | 18.73 MiB |
| 512x512 | 86 | 13.72 MiB |
| 256x256 | 66 | 5.66 MiB |
| 2048x2048 | 1 | 5.20 MiB |
| 128x128 | 147 | 1.51 MiB |

累積では上位100枚で 73.9%、上位200枚で 89.7% を占める。
最大は `map_world__cyrodiil_resized.png`（2048x2048、5.20 MiB）、
次いで `loading__loading_background.png`（1024x1024、1.27 MiB）。
1024x512 の47枚はほぼ全て `loading__load_*.png`（ローディング画面の全画面写真）である。

#### Android側の実需

Android の C++ が現在参照している UI テクスチャは **18枚のみ**（`assets/textures/ui/` 40ファイル、3.24 MiB）。

```
textures/ui/loading_background.png          textures/ui/tes_oblivion_logo_final.png
textures/ui/load_in_game_default.png        textures/ui/dialog_selection_full.png
textures/ui/dialog_selection_cut.png        textures/ui/main_background.png
textures/ui/inventory/inv_equiped_marker_%u.png
textures/ui/inventory/inv_icon_tab_{all,weapons,apparel,alchemy,misc}.png
textures/ui/icons/icon_{iron_sword,iron_cuirass,health_potion,mana_potion,iron_ore,leather,scroll_shield}.png
```

`TextureLoader`（`engine/texture_loader.h`）は stb_image による **PNGのみ**対応で、
ミップマップ生成も圧縮テクスチャ（ETC2/ASTC）読み込みも行わない。
圧縮テクスチャを使うにはローダの拡張が必要。

**含意**:
- `menus80` は新規テクスチャを1枚も追加しないため、統合対象から除外してよい。
- `menus` が唯一のマスターであり、`menus50` は同一絵柄の忠実な 1/2 版である。
  したがって「`menus` から縮小」すれば `menus50` 相当を再現できる。
- 822枚すべてを積む必要はない。現状の実需は18枚であり、
  未実装メニューを実装するたびに必要な分だけ追加するのが最も効率的。
- 現状の `menus80` 由来29枚は `menus` 版に差し替えると高品質化できる
  （例: `dialog\dialog_selection_full.dds` は menus 103,135 B / menus80 14,830 B。解像度は同じで格納形式のみ異なる）。

### 採用方針（確定）: 方針A — `menus` を単一ソースとするオンデマンド統合

3ティアの実測を踏まえ、次の方針を採用する。

| 項目 | 決定 |
|------|------|
| 単一ソース | `textures\menus`（最高品質マスター） |
| 除外 | `textures\menus80` は**使用しない**（新規テクスチャを1枚も追加しないため） |
| フォールバック | `textures\menus50` のみに存在する8枚はそこから取得 |
| 統合範囲 | 全822枚を一括投入せず、**Android側が実際に参照する分だけ**オンデマンドで追加 |
| 解像度上限 | 長辺 1024 px。ただし画素数 262,144（=1024x256）以下の細長いUIパーツは上限を適用しない |
| 格納形式 | PNG（`TextureLoader` が stb_image で PNG のみ対応） |

上限の適用条件を「長辺 > 1024 かつ 画素数 > 262,144」とした理由:
`dialog_selection_full.dds`（2048x64 = 131,072 px）のような細長いUIストリップは
縮小しても容量削減効果が小さく、品質劣化だけが残るため。実際に上限が作用するのは
`map_world__cyrodiil_resized`（2048x2048 → 1024x1024）のような大判のみとなる。

#### 方針Aの初期適用結果（2026-10-05 実施）

`menus80` 由来だった29枚を `menus` 版へ差し替えた。

| 対象 | 枚数 | 前 | 後 | 差 |
|------|-----:|----:|----:|----:|
| `inventory/*` | 26 | 638,000 B | 1,084,533 B | +446,533 B |
| `dialog_selection_cut.png` | 1 | 5,301 B | 11,246 B | +5,945 B |
| `dialog_selection_full.png` | 1 | 41,338 B | 75,100 B | +33,762 B |
| `loading_symbol.png` | 1 | 242,032 B | 396,755 B | +154,723 B |
| **合計** | **29** | **926,671 B (0.88 MiB)** | **1,567,634 B (1.50 MiB)** | **+640,963 B (+0.61 MiB)** |

差し替え後の出自は全29枚が `menus` と画素完全一致。`menus80` 由来のアセットは
`assets/textures/ui/` から消滅した。作業は `tmp/apply_tier_policy.py` で再現でき、
旧アセットは `tmp/ui_assets_backup/` に退避済み。

未適用（別途判断が必要）:

- ~~`loading_background.png` — BSA上のどのティアとも画素一致せず、内容は
  `textures\menus\genericbackground\center_background.dds` と同一。
  原作のローディング画面（`textures\menus\loading\loading_background.dds`）へ
  差し替えるかは別途判断とする。~~ → **完了（2026-10-05）**
- `icons/icon_*.png` 7枚 — `textures\menus*` に同名・同形状の元テクスチャが存在せず、
  BSA由来ではない（コミット `52f6a2c4` で追加）。出自の再確認が必要。

#### `loading_background.png` 原作差し替え（2026-10-05 実施）

`tmp/extracted_menus_full/menus/loading__loading_background.png` を
`app/src/main/assets/textures/ui/loading_background.png` へ差し替え済み。

- 適用先は5画面（タイトル/ランチャー/設定/セーブロード/マップ）。タイトル画面は
  videoBackgroundActive 時のみ動画背景を優先し、通常時は本テクスチャを汎用背景として使用。
- BSAから直接再抽出（`tmp/verify_loading_background.py`）して画素完全一致を確認の上で適用。
- SHA256 `0461d4cc...`（原作 `Menus\Loading\loading_background.dds` と一致）。
- 旧アセット（`center_background` と一致、SHA `4b04343f...`）は
  `tmp/ui_assets_backup/textures/ui/loading_background.png` に退避済み。
- APKサイズは約 +230 KB（1,219,199 B → 1,330,140 B）。

#### 参照整合性の全数照合（2026-10-05 実施）

既存 `assets/textures/ui/` 40枚すべてを C++/Java の `textures/ui/*.png` リテラル参照と照合した
（`tmp/ui_asset_reference_check.py`。リテラル19件 + 動的フォーマット11件 を収集し、アセット側を3分類）。

| 分類 | 枚数 | 対象 |
|------|-----:|------|
| **REF**（リテラル参照あり） | 17 | `dialog_selection_cut/full`, `icons/icon_*` 7枚, `inventory/inv_icon_tab_*` 5枚, `load_in_game_default`, `loading_background`, `tes_oblivion_logo_final` |
| **DYN**（`%u`/`%d` 動的フォーマット） | 6 | `inventory/inv_equiped_marker_1..6.png`（`ui_inventory_panel.cpp`） |
| **ORPHA**（コード未参照・孤児） | **17** | `inventory/inv_` ヘッダー4 / ボーダー3 / hilite 1 / icon_small 7（計15枚）+ `loading_symbol.png` + `tes_oblivion_logo_bink.png` |

**孤児17枚（1.18 MiB）の重要ファクト**:

- 現状インベントリUI（`ui_inventory_panel.cpp`）は `PlaceholderAssets::drawPanel` の
  塗りつぶし描画のため、`inventory/inv_` 装飾15枚（ボーダー・ヘッダー・ヒライト・小アイコン）は未使用。
- `loading_symbol.png` はタイトルのランチャーアイコンの生成源（`scripts/generate_launcher_icons.py`）であり、
  ゲーム内描画には未参照。`tes_oblivion_logo_bink.png` もタイトル画面は `logo_final` を使用しており未参照。
- 内訳（サイズ）: `inventory\inv_border_*` 3枚で 489,993 B、`loading_symbol` 396,755 B、
  `tes_oblivion_logo_bink` 242,521 B が主要。

**決定（2026-10-05, ユーザー判断）**: 孤児17枚は**削除せず保持する**。
すでに GitHub 管理外（`.gitignore` の Bethesda 資産扱い）であり、削除は不要。
今後メニューUIを本実装する際に `inventory` 装飾・ローディング画面を利用する想定で温存する。
オンデマンド統合の基準としては「**参照が追加された時点で、そのメニューXMLが指す DDS を menus から抽出して追加**」とし、
未参照アセットを先回りで追加しない。

## 6. メニューXML（新規解明）

`Oblivion - Textures - Compressed.bsa` には `menus\` フォルダがあり、**104ファイルのメニュー定義XML** が格納されている。

| パス | 内容 |
|------|------|
| `menus\main\` | inventory_menu / magic_menu / map_menu / hud_main_menu / hud_info_menu / hud_reticle / stats_menu 等 |
| `menus\chargen\` | race_sex_menu / class_menu / skills_menu / birthsign_menu |
| `menus\dialog\` | alchemy / enchantment / enchantmentsetting / persuasion / sigilstone / spell_purchase / spellmaking |
| `menus\generic\` | quest_added / skill_perk |
| `menus\` 直下 | loading_menu / message_menu / quantity_menu / repair_menu / sleep_wait_menu / training_menu / lockpick_menu / negotiate_menu / recharge_menu / levelup_menu / container_menu / book_menu / breath_meter_menu |
| `menus\strings.xml` | 11,024 B（UI文字列） |
| `menus\master_menu_file.txt` | 1,083,306 B（全メニューの連結） |

### XMLフォーマット

Bethesda 独自の宣言的UI記法。要素と演算子で構成される。

```xml
<menu name="LoadingMenu">
  <class> &LoadingMenu; </class>
  <stackingtype> &no_click_past; </stackingtype>
  <user1> Menus\Loading\load_in_game_default.dds </user1>
  <rect name="black">
    <red> 0 </red> <green> 0 </green> <blue> 0 </blue> <alpha> 255 </alpha>
    <width> <copy src="screen()" trait="width"/> </width>
    <height> <copy src="screen()" trait="height"/> </height>
  </rect>
</menu>
```

- 要素: `menu` / `rect` / `image` / `nif` / `text`
- 演算子: `copy` / `add` / `sub` / `mul` / `div` / `eq` / `gt` / `onlyif` / `onlyifnot`
- trait参照元: `screen()` / `parent()` / `me()` / `strings()`
- 画像参照は `Menus\Misc\hud_reticle.dds` のように DDS パスを直接指定

**含意**: オリジナルのUIは完全にデータ駆動であり、XMLインタプリタを実装すれば全メニューがピクセル単位で再現できる。
Android版は約90個の手書きC++ファイル（`app/src/main/cpp/ui/`）で個別実装されており、このXMLは未使用。

## 7. 成果物とスクリプト

| ファイル | 役割 |
|----------|------|
| `tmp/extract_menus_full.py` | ティア指定抽出 + マニフェスト生成（`--tier` / `--skip-icons` / `--only-missing` / `--manifest-only`） |
| `tmp/dds_decode.py` | DXT1/DXT3/DXT5/BGRA32/RGB565/ARGB4444 デコーダ |
| `tmp/ui_texture_report.py` | 解像度・重複・Android差分の分析 |
| `tmp/bsa_folder_map.json` | BSAフォルダマップ（1090フォルダ） |
| `tmp/ui_texture_manifest.json` | 全3ティア 5,476エントリのマニフェスト |
| `tmp/apply_tier_policy.py` | 方針Aの適用スクリプト（`--dry-run` / `--only-menus80` / `--backup-dir`） |
| `tmp/ui_asset_reference_check.py` | C++/Java リテラルと assets の参照照合（REF/DYN/ORPHA 分類） |
| `tmp/extracted_menus_full/menus/` | 非アイコン814 PNG（tier=menus） |
| `tmp/extracted_menus/` | 先行抽出915 PNG（menus80ベース、不完全） |
| `tmp/extracted/` | アイコン934 PNG |
| `tmp/bsa_misc_full/menus/` | メニュー定義XML 104ファイル + strings.xml + master_menu_file.txt |

## 8. 次のステップ

1. 品質ティア選択は**方針Aで確定**（`menus` 単一ソース・オンデマンド統合・長辺1024px上限）。→ 完了
2. 参照整合性の全数照合を実施し、孤児17枚（1.18 MiB）は**保持**と決定。→ 完了（2026-10-05）
3. 抽出済み814枚のAndroid統合: **参照が生じた分だけ** `menus` から抽出・縮小・PNG変換して追加。
4. メニューXMLインタプリタの実装検討（要素・演算子・traitのサブセットから着手）。
5. `loading_background.png` の原作差し替え。→ 完了（2026-10-05）
6. `icons/icon_*.png` 7枚の出自再確認（BSA非由来）。
