# Oblivion Android - アセット統合ガイド

**最終更新**: 2026-08-27  
**対象**: BSA抽出素材、ISO抽出素材のAndroidプロジェクト統合

---

## 目次

1. [アセット概要](#アセット概要)
2. [ISOからの抽出方法](#isoからの抽出方法)
3. [BSAからの抽出方法](#bsaからの抽出方法)
4. [Androidプロジェクトへの配置](#androidプロジェクトへの配置)
5. [C++からのアクセス方法](#cからのアクセス方法)
6. [カテゴリ別統合方法](#カテゴリ別統合方法)
7. [APKサイズ影響](#apkサイズ影響)
8. [トラブルシューティング](#トラブルシューティング)

---

## アセット概要

### ファイル統計

| 拡張子 | ファイル数 | 合計サイズ | 種類 |
|--------|-----------|-----------|------|
| `.wav` | 2,090 | 435.66 MB | 音声（効果音・アンビエント） |
| `.nif` | 1,556 | 234.81 MB | 3Dメッシュ（NetImmerse形式） |
| `.mp3` | 692 | 25.00 MB | 音声（音楽・ボイス） |
| `.kf` | 614 | 22.23 MB | アニメーションキーフレーム |
| `.egm` | 13 | 6.46 MB | FaceGenメッシュデータ |
| `.tex` | 5 | 4.25 MB | フォントテクスチャ |
| `.lip` | 691 | 3.41 MB | リップシンクデータ |
| `.xml` | 89 | 1.16 MB | UIメニュー定義 |
| `.txt` | 6 | 1.07 MB | テキストデータ |
| `.fnt` | 5 | 0.07 MB | フォント定義 |
| **合計** | **5,756** | **734 MB** | |

### ディレクトリ構成

```
bsa_Extraction/
├── distantlod/     # 遠距離LODデータ（.lod）
├── facegen/        # キャラクター顔データ（.egm）
├── fonts/          # ゲームフォント（.fnt + .tex）
├── menus/          # UIメニュー定義（.xml）
│   ├── main/       # メインメニュー（HUD, インベントリ, マップ, 魔法）
│   ├── generic/    # 汎用UI要素
│   ├── dialog/     # ダイアログUI
│   ├── options/    # オプションメニュー
│   └── prefabs/    # UI部品テンプレート
├── meshes/         # 3Dモデル（.nif）
│   ├── architecture/   # 建築物
│   ├── armor/          # 防具
│   ├── characters/     # キャラクターベースメッシュ
│   ├── clothes/        # 衣服
│   ├── clutter/        # 小物
│   ├── creatures/      # クリーチャー
│   ├── dungeons/       # ダンジョン部品
│   ├── effects/        # エフェクト
│   ├── landscape/      # 地形オブジェクト
│   ├── weapons/        # 武器
│   └── ...
└── sound/          # 音声データ
    ├── fx/         # 効果音（.wav）
    │   └── ambient/    # 環境音
    └── voice/      # ボイス（.mp3 / .wav）
```

---

## ISOからの抽出方法

### 前提条件

#### 必要なソフトウェア
1. **7-Zip** or **WinRAR** or **PowerISO** - ISOファイルのマウント/展開用
   - Download: https://www.7-zip.org/ (無料)
   - Alternative: Windows 10+はネイティブでISOマウント可能

### Step 1: ISOをマウント

#### 方法A: Windows ネイティブマウント（Windows 10+推奨）
1. エクスプローラーでISOファイルを見つける
2. 右クリック → マウント
3. 新しいドライブレター（例：D:\）が表示される

#### 方法B: 7-Zip展開（全Windowsバージョン対応）
1. ISOファイルを右クリック
2. 「7-Zip」→「展開」を選択

### Step 2: ゲームアセットの場所

マウント/展開したISO内の構造：
```
[ISO Root]/
├── Data/
│   ├── meshes/           ← NIFファイル（3Dモデル）
│   ├── textures/         ← DDSファイル（テクスチャ）
│   └── [other game data]
```

### Step 3: サンプルアセットの抽出

```powershell
# PowerShell使用例
cd "C:\Users\hiroki.kogarumai\Oblivion_Android\app\src\main\assets"

# ディレクトリ作成
mkdir -p meshes
mkdir -p textures

# 特定のモデルをコピー（マウントドライブがD:\の場合）
copy "D:\Data\meshes\architecture\doors\door01.nif" "meshes\"
copy "D:\Data\meshes\furniture\table01.nif" "meshes\"
copy "D:\Data\textures\architecture\doors\door01.dds" "textures\"
copy "D:\Data\textures\furniture\table01.dds" "textures\"
```

### 推奨テストアセット

| カテゴリ | サイズ | 用途 |
|---------|--------|------|
| Furniture（家具） | 10-50 KB/個 | テスト用に最適 |
| Architecture（建築） | 100-500 KB/個 | ドア、窓 |
| Weapons（武器） | 50-200 KB/個 | 中程度の複雑さ |
| Creatures（クリーチャー） | 200 KB-2 MB/個 | 動物、モンスター |

推奨テストセット合計: 50-100 MB

---

## BSAからの抽出方法

BSAファイルはOblivionのアーカイブ形式です。

### 抽出済みデータの場所

```
E:\Cargo\Oblivion_Android\Other\BSA\bsa_Extraction\
```

### 主要カテゴリ

| カテゴリ | ファイル数 | サイズ | 統合容易度 |
|---------|-----------|--------|----------|
| UIメニュー（XML） | 89 | 1.16 MB | ★★☆☆☆（中） |
| フォント | 10 | 0.07+4.25 MB | ★★★☆☆（やや高） |
| 3Dメッシュ（NIF） | 1,556 | 234.81 MB | ★★★★★（非常高） |
| 音声（WAV） | 2,090 | 435.66 MB | ★☆☆☆☆（低） |
| 音声（MP3） | 692 | 25.00 MB | ★☆☆☆☆（低） |

---

## Androidプロジェクトへの配置

### 推奨ディレクトリ構造

```
app/src/main/assets/
├── meshes/
│   ├── architecture/
│   │   └── doors/
│   │       ├── door01.nif
│   │       └── door02.nif
│   ├── furniture/
│   │   ├── table01.nif
│   │   └── chair01.nif
│   └── creatures/
│       └── rat.nif
├── textures/
│   ├── architecture/
│   │   └── doors/
│   │       ├── door01.dds
│   │       └── door02.dds
│   ├── furniture/
│   │   ├── table01.dds
│   │   └── chair01.dds
│   └── creatures/
│       └── rat.dds
├── audio/
│   ├── music/
│   │   ├── explore.mp3
│   │   ├── dungeon.mp3
│   │   └── battle.mp3
│   ├── sfx/
│   │   ├── ambient/
│   │   ├── combat/
│   │   ├── magic/
│   │   └── ui/
│   └── voice/
└── menus/
    ├── inventory_menu.xml
    ├── map_menu.xml
    └── hud_main_menu.xml
```

### アセットカタログ（オプション）

```
app/src/main/assets/catalog.txt:
architecture/doors/door01
furniture/table01
furniture/chair01
creatures/rat
```

---

## C++からのアクセス方法

### Android Asset Manager使用

```cpp
#include <android/asset_manager.h>

// メッシュの読み込み例
AAsset* asset = AAssetManager_open(assetManager, 
    "meshes/furniture/table01.nif", 
    AASSET_MODE_STREAMING);

if (asset) {
    off_t size = AAsset_getLength(asset);
    const void* buffer = AAsset_getBuffer(asset);
    
    // NIFデータの解析
    // ...
    
    AAsset_close(asset);
}
```

### Java連携

```java
AssetManager assetManager = context.getAssets();
InputStream input = assetManager.open("meshes/furniture/table01.nif");

// JNI経由でネイティブコードに渡す
nativeLoadAsset(buffer, size);
```

### パス変換

```cpp
std::string constructMeshPath(const std::string& meshName) {
    // Input: "furniture/table01"
    // Output: "meshes/furniture/table01.nif"
    return "meshes/" + meshName + ".nif";
}
```

---

## カテゴリ別統合方法

### 1. UIメニュー（menus/*.xml）

**難易度**: ★★☆☆☆（中）  
**優先度**: 🔴 高

#### 確認済みXMLファイル

| XMLファイル | 内容 | 既存コードとの対応 |
|------------|------|------------------|
| `inventory_menu.xml` | インベントリUI完全定義 | `ui/inventory_ui.cpp` に統合 |
| `map_menu.xml` | マップUI（ワールド/ローカル） | `ui/ui_map_panel.cpp` に統合 |
| `hud_main_menu.xml` | HUD（HP/MP/コンパス/武器） | `ui/hud_renderer.cpp` に統合 |
| `magic_menu.xml` | 魔法メニュー | `ui/` に新規作成 |
| `stats_menu.xml` | ステータスメニュー | `ui/` に新規作成 |

#### テクスチャ参照パス

```
Menus\Shared\shared_main_background_1.dds    # メイン背景
Menus\Shared\shared_main_background_2.dds    # 背景装飾
Menus\Shared\all_tabs_icons.dds              # タブアイコン
Menus\Shared\all_small_icons.dds             # 小アイコン
Menus\Inventory\inv_border_*.dds             # インベントリ枠
Menus\HUD\hud_ribbon_*.dds                   # HUDリボン
Menus\Map\map_border_*.dds                   # マップ枠
```

#### 統合ステップ

1. **XMLパーサー作成**（新規）
   - ファイル: `engine/xml_menu_parser.h/cpp`
   - 対応要素: `<rect>`, `<image>`, `<text>`, `<template>`

2. **既存UIとの連携**
   - `UISystem::registerComponent()` を使用して動的生成

### 2. フォント（fonts/*）

**難易度**: ★★★☆☆（やや高）  
**優先度**: 🟡 中

#### フォーマット

| ファイル | 形式 | 内容 |
|---------|------|------|
| `*.fnt` | バイナリ | フォント定義（文字マッピング、UV座標） |
| `*_0_lod_a.tex` | バイナリ | フォントテクスチャ（DDS圧縮） |

#### 統合方法

**案A: 既存フォントを継続使用（簡易）**
- 現在の `arial.ttf` / `NotoSans.ttf` を継続使用
- メリット: 実装不要、すぐに動作

**案B: Daedric/Kingthingsフォントを変換して使用（本格的）**
- `.tex` → `.png` に変換
- `.fnt` バイナリを解析してグリフ情報を抽出
- 作業量: 2-3日

**推奨**: フェーズ9では案A、フェーズ10で案Bを検討

### 3. 3Dメッシュ（meshes/*.nif）

**難易度**: ★★★★★（非常高）  
**優先度**: 🟢 低（長期的）

#### NIFフォーマット

- バージョン: 10.0.1.0〜20.3.0.9（Oblivionは20.0.0.4）
- 内容: 頂点、インデックス、UV、法線、ボーン、マテリアル

#### 統合方法

**案A: NIFローダー自作**
- 作業量: 2-3週間
- リスク: 高

**案B: OpenMWのNIFローダーを移植**
- GPLライセンスのため注意
- 作業量: 1週間

**案C: 変換ツールで事前に変換（推奨）**
- Blender + NIFプラグインで `.nif` → `.obj` / `.fbx` に変換
- 作業量: 変換のみ（半日）

**推奨**: フェーズ9では対象外。フェーズ10で案Cを採用

### 4. 音声データ（sound/*）

**難易度**: ★☆☆☆☆（低）  
**優先度**: 🔴 高

#### フォーマット

| カテゴリ | 形式 | 統合容易度 |
|---------|------|----------|
| `sound/fx/*.wav` | PCM WAV | ✅ OpenAL-Softで直接再生可能 |
| `sound/fx/ambient/*.wav` | PCM WAV | ✅ 同上 |
| `sound/voice/*.mp3` | MP3 | ⚠️ デコードが必要 |

#### 統合ステップ

**ステップ1: WAVファイル統合（1日）**
1. `sound/fx/` 内のWAVファイルを `app/src/main/assets/audio/sfx/` にコピー
2. `AudioManager::loadWavFile()` を実装
3. 効果音トリガーをゲームイベントに接続

**ステップ2: MP3→WAV変換（1日）**
```bash
ffmpeg -i input.mp3 -acodec pcm_s16le -ar 44100 -ac 2 output.wav
```

**ステップ3: 音声定義JSON作成（2日）**
```json
{
  "categories": {
    "ui": [
      {"id": "click", "file": "ui/click.wav"},
      {"id": "hover", "file": "ui/hover.wav"}
    ],
    "ambient": [
      {"id": "cave", "file": "fx/ambient/amb_cavehowl_lp_01_3d.wav", "loop": true}
    ],
    "combat": [
      {"id": "swing", "file": "fx/combat/swing.wav"},
      {"id": "hit", "file": "fx/combat/hit.wav"}
    ]
  }
}
```

#### 推奨ファイル一覧（統合優先度順）

| 優先度 | ファイルパターン | 用途 |
|--------|----------------|------|
| 🔴 高 | `sound/fx/ui/*.wav` | UIクリック音 |
| 🔴 高 | `sound/fx/ambient/*.wav` | 環境音（ループ） |
| 🟡 中 | `sound/fx/combat/*.wav` | 戦闘SE |
| 🟡 中 | `sound/fx/magic/*.wav` | 魔法SE |
| 🟢 低 | `sound/voice/*.mp3` | NPCボイス（変換後） |

---

## APKサイズ影響

| 素材カテゴリ | サイズ | 圧縮後（推定） | 統合時期 |
|-------------|--------|--------------|---------|
| 既存プロジェクト | 8.8 MB | - | 現在 |
| UIテクスチャ | ? | ~10 MB | Phase 9 |
| 効果音WAV（選択的） | ~50 MB | ~25 MB | Phase 9 |
| ボイスWAV（変換後） | ~200 MB | ~100 MB | Phase 10 |
| 3Dメッシュ（変換後） | ~235 MB | ~120 MB | Phase 10+ |
| **合計（Phase 9）** | **~85 MB** | **~45 MB** | |
| **合計（Phase 10）** | **~500 MB** | **~260 MB** | |

---

## 即座に統合可能な素材

### 優先度A（今すぐ統合）

| # | 素材 | 理由 | 工数 |
|---|------|------|------|
| 1 | **UIレイアウトXML** | 既存UIコードと直接連携可能 | 3-5日 |
| 2 | **WAV効果音** | OpenALでそのまま使用可能 | 1-2日 |
| 3 | **メニュー文字列** | ローカリゼーションに統合 | 0.5日 |

### 優先度B（Phase 9後半）

| # | 素材 | 理由 | 工数 |
|---|------|------|------|
| 4 | **MP3ボイス** | WAV変換後に統合 | 1-2日 |
| 5 | **フォント** | 変換/実装に時間がかかる | 2-3日 |

### 優先度C（Phase 10以降）

| # | 素材 | 理由 | 工数 |
|---|------|------|------|
| 6 | **3Dメッシュ（NIF）** | パーサー/変換が必要 | 2-3週間 |
| 7 | **アニメーション（KF）** | メッシュと同時に対応 | 1-2週間 |
| 8 | **リップシンク（LIP）** | ボイス統合後に対応 | 1週間 |

---

## リスクと対策

| リスク | 影響 | 確率 | 対策 |
|--------|------|------|------|
| NIFフォーマットが複雑すぎる | Phase 10遅延 | 高 | 変換ツール使用を前提とする |
| .texフォントが読めない | フォント統合不可 | 中 | 既存フォントフォールバック |
| WAVファイルサイズが大きい | APKサイズ肥大 | 高 | 圧縮OGG-Vorbisに変換 |
| XMLレイアウトが複雑 | UI統合に時間がかかる | 中 | 主要メニューのみ先に対応 |

---

## トラブルシューティング

### ISOがマウントできない場合
- Windows 7以前: 7-Zipで抽出
- Windows 10/11: システムの更新が必要な場合あり

### ファイルがAndroid APKで見つからない場合
- `app/src/main/assets/` フォルダにファイルがあるか確認
- Gradleでプロジェクトをリビルド
- `gradle clean` → `gradle build` を実行

### NIF解析失敗
- NIFバージョンの不一致（異なるOblivionパッチ）
- 抽出時のファイル破損
- パーサーが特定のNIFバリアント未対応

---

## 参考資料

- [UESP File Formats Wiki](https://en.uesp.net/wiki/Oblivion:File_formats)
- [NIF Format Specification](https://en.uesp.net/wiki/Oblivion:File_formats)
- [DDS Format Reference](https://docs.microsoft.com/en-us/windows/win32/direct3ddds/dx-graphics-dds-pguide)
- [OpenMW Asset Reference](https://github.com/OpenMW/openmw/)

---

**最終更新**: 2026-08-27
