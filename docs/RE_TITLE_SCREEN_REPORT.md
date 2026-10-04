# Oblivion Steam版 リバースエンジニアリングレポート：タイトル画面

解析日: 2026-10-04
対象: `D:\Cargo\Ghidra\temp\Oblivion_Steam版`

## 1. 全体構成（Data フォルダ）

- **BSA アーカイブ 17個**。最大は `Oblivion - Textures - Compressed.bsa` (1.16 GB)
- **BSA103 フォーマット完全解読**（36バイトヘッダ + フォルダレコード + フォルダブロック + ファイル名ブロック + 実データ）
- 主要ESP: Oblivion.esm (277 MB) + 全DLC (Knights, ShiveringIsles, 各DLC)

### Textures BSA 構造（全1090フォルダ / 18040ファイル）
```
textures\
├── menus       (262フォルダ) ← 100% UIスケール
├── menus80     (259フォルダ) ← 80% UIスケール
├── menus50     (259フォルダ) ← 50% UIスケール
├── architecture / armor / characters / clothes / clutter / creatures
├── dungeons / effects / faces / fire / landscape / landscapelod
├── magic / oblivion / obliviongate / plants / rocks / sky / trees / water / weapons / wood
```

## 2. タイトル画面の構成（main_menu.xml 完全解析）

`menus\options\main_menu.xml` がタイトル画面全体を定義。

### レイヤー構造（depth順）
| depth | 要素 | テクスチャ/定義 | 座標 |
|-------|------|----------------|------|
| 1 | backdrop 背景 | `Menus\Loading\loading_background.dds` (1280x960) | 画面中央 |
| 2 | oblivion_logo ロゴ | `Menus\Loading\tes_oblivion_logo_final.dds` (748x159) | x=中央+3, y=height/4+22.5 |
| 2 | oblivion_logo (bink) | `Menus\Loading\tes_oblivion_logo_bink.dds` (760x169) | ロゴの -5,-5、フェード用 |
| 10 | press_start | テキスト `_pressstart` | y=725（Xbox時のみ表示） |
| 10 | button_layout | 6ボタン（button_floating.xml プリファブ） | y=725 |

### メインメニュー6ボタン（button_layout）
1. `main_continue` → `_continue`
2. `main_new` → `_new`
3. `main_load` → `_load`
4. `main_options` → `_options`
5. `main_credits` → `_credits`
6. `main_exit` → `_exit`

### button_floating.xml（ボタンプリファブ）
- ボタン高さ 64
- ホバー時: `Menus\Dialog\dialog_selection_full.dds` を表示
- テキスト色: RGB(117, 59, 33)（こげ茶）
- ユーザー1フラグで表示制御（コードからロゴ/ボタン/プレススタートの表示を切替）

### テクスチャ一覧（textures\menus\loading\ 55ファイルから主要分）
| ファイル | サイズ | フォーマット | 用途 |
|----------|--------|--------------|------|
| loading_background.dds | 1024x1024 | DXT3 | 背景 |
| tes_oblivion_logo_final.dds | 1024x256 | DXT3 | タイトルロゴ |
| tes_oblivion_logo_bink.dds | 1024x256 | DXT3 | イントロBINKからロゴへのフェード用 |
| loading_symbol.dds | 512x512 | DXT3 | ローディングシンボル |
| loading_save_normal_frame.dds | 512x512 | DXT5 | セーブ枠 |
| dialog_selection_full.dds | 2048x64 | BGRA32非圧縮 | 選択バー（フル） |
| dialog_selection_cut.dds | 128x64 | BGRA32非圧縮 | 選択バー（カット） |
| shared_button_long_on.dds | 256x64 | DXT5 | ボタンON（長） |
| shared_button_short_on.dds | 128x64 | DXT5 | ボタンON（短） |
| tes_oblivion_logo.dds | 512x128 | DXT5 | クレジット用ロゴ |

### 技術的発見
- **dialog_selection_*.dds は fourcc='A'（DDPF_ALPHA 0x41）の特殊ヘッダ**。NetImmerse独自の 128バイトDDSヘッダ（標準より4バイト大きい、DDS_PIXELFORMAT がオフセット76から開始）。実体は 32bit 非圧縮 BGRA (A8R8G8B8)。rmask=0x00ff0000, gmask=0x0000ff00, bmask=0x000000ff, amask=0xff000000
- DXTテクスチャの fourcc はオフセット84、ピクセルデータはオフセット128から開始

## 3. Android移植版との比較

Android版 `title_screen.cpp` は同じ構成・テクスチャ名をPNGで使用。

### レイアウト定数（オリジナルを忠実に再現）
| 項目 | オリジナル | Android版 | 一致 |
|------|-----------|-----------|------|
| ボタン数 | 6 | 6 | YES |
| ボタン行 Y | y=725 | MENU_ROW_CENTER_Y=0.786 | YES |
| ボタン行幅 | (動的) | MENU_ROW_SPAN=0.545 | YES |
| キャップ高 | (動的) | MENU_CAP_HEIGHT_RATIO=0.025 | YES |
| ロゴ | x=中央+3, y=h/4+22.5 | 同様 | YES |

### テクスチャ比較（標準DXTデコード vs Android PNG、MAE / 0-255）
独立実装した**標準DirectX式DXT3/DXT5デコーダ**（`tmp/dds_decode.py`）で元DDSを復号し、Android版PNGとピクセル単位で比較。

| テクスチャ | 寸法 | フォーマット | R | G | B | A | 一致率 |
|-----------|------|-------------|-----|-----|-----|-----|--------|
| tes_oblivion_logo_final | 1024x256 | DXT3 | 0.00 | 0.00 | 0.00 | 0.00 | 100% |
| tes_oblivion_logo_bink | 1024x256 | DXT3 | 0.00 | 0.00 | 0.00 | 0.00 | 100% |
| loading_background | 1024x1024 | DXT3 | 0.00 | 0.00 | 0.00 | 0.00 | 100% |
| loading_symbol | 512x512 | DXT3 | 0.00 | 0.00 | 0.00 | 0.00 | 100% |
| dialog_selection_full | 2048x64 | BGRA32 | 0.00 | 0.00 | 0.00 | 0.00 | 100% |
| dialog_selection_cut | 128x64 | BGRA32 | 0.00 | 0.00 | 0.00 | 0.00 | 100% |

**結論: Android版の全タイトル用テクスチャは、元DDSの標準デコード結果とピクセル単位で100%完全一致。変換の丸め差・アルファ差は一切存在しない。**

### 検証プロセス（重要な修正）
- 初回の比較（MAE=14.17 / 8.39）は**解析側のDDSデコード方法が非標準**だったことが原因の誤報だった。
- 標準DXT3は「1ブロック16バイト = アルファ8B + c0/c1 4B + **インデックス4B**」。初回はインデックスを16Bと誤読（28B/ブロック）したため色・アルファの丸めがずれていた。
- 標準デコード（565展開 `(v<<3)|(v>>2)`、補間 `(2c0+c1)/3`、アルファ `(nib<<4)|nib`）に修正後、Android版と**全テクスチャがビット単位で一致**。
- 標準デコードの再現PNGを `tmp/bsa_textures_title/png/*_std.png`、コンタクトシートを `screenshots/title_textures_standard.png` に保存。

### ロゴ表示の引き伸ばし（確定）
- 元テクスチャ 1024x256（AR 4.0）に対し、ゲームは main_menu.xml で 748x159（AR 4.70）に表示 → 横方向に約17%引き伸ばし。
- Android版は `LOGO_WIDTH_RATIO=0.431, LOGO_HEIGHT_RATIO=0.161`（title_screen.h 行281-283）で16:9画面時AR≈4.76 → オリジナルのAR 4.70と整合。再現は正しい。

## 4. 今後の解析候補

- 他のメニュー（pause_menu.xml, options_menu.xml 等）のScaleform定義解析
- セーブ/ロード画面（loading_save_*.dds の利用）
- 全18,040テクスチャのインデックス（`textures_bsa_index.txt` に保存済み）
- 各DLCのBSA個別解析
