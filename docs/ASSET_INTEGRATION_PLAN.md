# 全素材統合計画書

**作成日**: 2026-06-07  
**対象**: `E:\Cargo\Oblivion_Android\Other\BSA\bsa_Extraction\`  
**目的**: BSA抽出素材を既存Androidプロジェクトに統合する完全ガイド

---

## 1. 素材全体概要

### 1.1 ファイル統計

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

### 1.2 ディレクトリ構成

```
bsa_Extraction/
├── distantlod/     # 遠距離LODデータ（.lod）
├── facegen/        # キャラクター顔データ（.egm）
├── fonts/          # ゲームフォント（.fnt + .tex）
│   ├── daedric_font.fnt
│   ├── handwritten.fnt
│   ├── kingthings_regular.fnt
│   ├── kingthings_shadowed.fnt
│   └── tahoma_bold_small.fnt
├── menus/          # UIメニュー定義（.xml）
│   ├── main/       # メインメニュー（HUD, インベントリ, マップ, 魔法）
│   ├── generic/    # 汎用UI要素
│   ├── dialog/     # ダイアログUI
│   ├── options/    # オプションメニュー
│   └── prefabs/    # UI部品テンプレート
├── meshes/         # 3Dモデル（.nif）
│   ├── architecture/   # 建築物（橋、壁、建物）
│   ├── armor/          # 防具
│   ├── characters/     # キャラクターベースメッシュ
│   ├── clothes/        # 衣服
│   ├── clutter/        # 小物
│   ├── creatures/      # クリーチャー
│   ├── dungeons/       # ダンジョン部品
│   ├── effects/        # エフェクト
│   ├── landscape/      # 地形オブジェクト
│   ├── rocks/          # 岩
│   ├── sky/            # 空オブジェクト
│   ├── trees/          # 樹木
│   ├── weapons/        # 武器
│   └── ...
└── sound/          # 音声データ
    ├── fx/         # 効果音（.wav）
    │   └── ambient/    # 環境音
    └── voice/      # ボイス（.mp3 / .wav）
```

---

## 2. 各カテゴリ詳細と統合方法

### 2.1 UIメニュー（menus/*.xml）

**難易度**: ★★☆☆☆（中）  
**優先度**: 🔴 高  
**ステータス**: ✅ 解析完了、統合準備OK

#### 確認済み内容

| XMLファイル | 内容 | 既存コードとの対応 |
|------------|------|------------------|
| `inventory_menu.xml` | インベントリUI完全定義 | `ui/inventory_ui.cpp` に統合 |
| `map_menu.xml` | マップUI（ワールド/ローカル） | `ui/ui_map_panel.cpp` に統合 |
| `hud_main_menu.xml` | HUD（HP/MP/コンパス/武器） | `ui/hud_renderer.cpp` に統合 |
| `magic_menu.xml` | 魔法メニュー | `ui/` に新規作成 |
| `stats_menu.xml` | ステータスメニュー | `ui/` に新規作成 |

#### キー情報

**テクスチャ参照パス**:
```
Menus\Shared\shared_main_background_1.dds    # メイン背景
Menus\Shared\shared_main_background_2.dds    # 背景装飾
Menus\Shared\all_tabs_icons.dds              # タブアイコン
Menus\Shared\all_small_icons.dds             # 小アイコン
Menus\Shared\shared_01_tab.dds ~ shared_05_tab.dds  # タブページ
Menus\Inventory\inv_border_*.dds             # インベントリ枠
Menus\HUD\hud_ribbon_*.dds                   # HUDリボン
Menus\HUD\hud_compass_*.dds                  # コンパス
Menus\Map\map_border_*.dds                   # マップ枠
Menus\Map\world\world_map_paper.dds          # マップ紙
```

**注意**: これらは `.dds` テクスチャを参照するが、実際のテクスチャファイルは**抽出されていない**。別途BSAからテクスチャを抽出するか、既存の `main_background.png` 等を代替として使用する必要がある。

#### 統合ステップ

1. **XMLパーサー作成**（新規）
   - ファイル: `engine/xml_menu_parser.h/cpp`
   - OblivionのメニューXMLを解析し、UIコンポーネントに変換
   - 対応要素: `<rect>`, `<image>`, `<text>`, `<template>`

2. **既存UIとの連携**
   - `UISystem::registerComponent()` を使用して動的生成
   - 座標・サイズをXMLから読み取り、`UIComponent::setPosition()` / `setSize()` に適用

---

### 2.2 フォント（fonts/*）

**難易度**: ★★★☆☆（やや高）  
**優先度**: 🟡 中  
**ステータス**: ⚠️ バイナリフォーマット、変換必要

#### フォーマット分析

| ファイル | 形式 | 内容 |
|---------|------|------|
| `*.fnt` | バイナリ | フォント定義（文字マッピング、UV座標） |
| `*_0_lod_a.tex` | バイナリ | フォントテクスチャ（DDS圧縮） |

**問題点**: `.fnt` はOblivion独自のバイナリ形式。AngelCodeのBMFont形式とは異なる。

#### 統合方法（2案）

**案A: 既存フォントを継続使用（簡易）**
- 現在の `arial.ttf` / `NotoSans.ttf` を継続使用
- メリット: 実装不要、すぐに動作
- デメリット: 本家Oblivionの雰囲気から外れる

**案B: Daedric/Kingthingsフォントを変換して使用（本格的）**
- `.tex` → `.dds` または `.png` に変換
- `.fnt` バイナリを解析してグリフ情報を抽出
- FreeType またはビットマップフォントとして読み込み
- **作業量**: 2-3日

#### 推奨

**フェーズ9では案A（既存フォント維持）**、フェーズ10で案Bを検討。

---

### 2.3 3Dメッシュ（meshes/*.nif）

**難易度**: ★★★★★（非常高）  
**優先度**: 🟢 低（長期的）  
**ステータス**: ⚠️ NIFフォーマット、専用パーサー必要

#### フォーマット分析

**NIF（NetImmerse Format）**: Gamebryoエンジン使用の3Dフォーマット
- バージョン: 10.0.1.0〜20.3.0.9（Oblivionは20.0.0.4）
- 内容: 頂点、インデックス、UV、法線、ボーン、マテリアル

#### 既存コード状況

現在のプロジェクトには:
- `test_cube.nif`（40 bytes - おそらく空または破損）
- `geometry/cube.h/cpp`（独自の立方体生成）

NIFローダーは**存在しない**。

#### 統合方法（3案）

**案A: NIFローダー自作（最も本格的）**
- NIFファイルフォーマットをリバースエンジニアリング
- `assets/nif_loader.h/cpp` を新規作成
- **作業量**: 2-3週間
- **リスク**: 高（フォーマットが複雑）

**案B: OpenMWのNIFローダーを移植**
- OpenMWプロジェクトはNIF読み込みに対応済み
- GPLライセンスのため注意が必要
- **作業量**: 1週間

**案C: 変換ツールで事前に変換（推奨）**
- Blender + NIFプラグインで `.nif` → `.obj` / `.fbx` に変換
- または `nifskope` でエクスポート
- 既存のメッシュ読み込み機能で使用
- **作業量**: 変換のみ（半日）

#### 推奨

**フェーズ9では対象外**。フェーズ10で案C（変換ツール使用）を採用し、特定の武器/防具メッシュのみ統合。

---

### 2.4 音声データ（sound/*）

**難易度**: ★☆☆☆☆（低）  
**優先度**: 🔴 高  
**ステータス**: ✅ WAV/MP3形式、即座に統合可能

#### フォーマット分析

| カテゴリ | 形式 | 統合容易度 |
|---------|------|----------|
| `sound/fx/*.wav` | PCM WAV | ✅ OpenAL-Softで直接再生可能 |
| `sound/fx/ambient/*.wav` | PCM WAV | ✅ 同上 |
| `sound/voice/*.mp3` | MP3 | ⚠️ デコードが必要（OpenALはMP3非対応） |

#### 既存コード状況

```cpp
// 既存のオーディオシステム（Phase 8）
#ifdef AUDIO_SYSTEM_ENABLED
    std::unique_ptr<AudioManager> audioManager;
#endif
```

- `AudioManager` はOpenAL-Softを使用
- `loadWavFile()` はスタブ実装（要完了）

#### 統合ステップ

**ステップ1: WAVファイル統合（1日）**
1. `sound/fx/` 内のWAVファイルを `app/src/main/assets/audio/sfx/` にコピー
2. `AudioManager::loadWavFile()` を実装（PCM読み込み）
3. 効果音トリガーをゲームイベントに接続

**ステップ2: MP3→WAV変換（1日）**
1. `sound/voice/` のMP3をWAVに一括変換
   ```bash
   # FFmpeg使用例
   ffmpeg -i input.mp3 -acodec pcm_s16le -ar 44100 -ac 2 output.wav
   ```
2. 変換後のWAVを `app/src/main/assets/audio/voice/` に配置

**ステップ3: 音声定義JSON作成（2日）**
```json
// assets/audio/sound_definitions.json
{
  "categories": {
    "ui": [
      {"id": "click", "file": "ui/click.wav"},
      {"id": "hover", "file": "ui/hover.wav"}
    ],
    "ambient": [
      {"id": "cave", "file": "fx/ambient/amb_cavehowl_lp_01_3d.wav", "loop": true},
      {"id": "wind", "file": "fx/ambient/amb_wind.wav", "loop": true}
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

## 3. 即座に統合可能な素材

### 3.1 優先度A（今すぐ統合）

| # | 素材 | 理由 | 工数 |
|---|------|------|------|
| 1 | **UIレイアウトXML** | 既存UIコードと直接連携可能 | 3-5日 |
| 2 | **WAV効果音** | OpenALでそのまま使用可能 | 1-2日 |
| 3 | **メニュー文字列** | ローカリゼーションに統合 | 0.5日 |

### 3.2 優先度B（Phase 9後半）

| # | 素材 | 理由 | 工数 |
|---|------|------|------|
| 4 | **MP3ボイス** | WAV変換後に統合 | 1-2日 |
| 5 | **フォント** | 変換/実装に時間がかかる | 2-3日 |

### 3.3 優先度C（Phase 10以降）

| # | 素材 | 理由 | 工数 |
|---|------|------|------|
| 6 | **3Dメッシュ（NIF）** | パーサー/変換が必要 | 2-3週間 |
| 7 | **アニメーション（KF）** | メッシュと同時に対応 | 1-2週間 |
| 8 | **リップシンク（LIP）** | ボイス統合後に対応 | 1週間 |

---

## 4. 具体的な統合作業手順

### 4.1 Step 1: 音声統合（最優先・1日）

```bash
# 1. ディレクトリ作成
mkdir -p app/src/main/assets/audio/sfx/ambient
mkdir -p app/src/main/assets/audio/sfx/combat
mkdir -p app/src/main/assets/audio/sfx/magic
mkdir -p app/src/main/assets/audio/sfx/ui
mkdir -p app/src/main/assets/audio/voice

# 2. WAVファイルコピー（効果音）
cp Other/BSA/bsa_Extraction/sound/fx/ambient/*.wav app/src/main/assets/audio/sfx/ambient/
cp Other/BSA/bsa_Extraction/sound/fx/combat/*.wav app/src/main/assets/audio/sfx/combat/

# 3. MP3→WAV変換（ボイス）
for f in Other/BSA/bsa_Extraction/sound/voice/*.mp3; do
    ffmpeg -i "$f" -acodec pcm_s16le -ar 44100 -ac 2 "app/src/main/assets/audio/voice/$(basename $f .mp3).wav"
done
```

**コード変更**:
- `audio/audio_manager.cpp`: `loadWavFile()` を完全実装
- `audio/audio_manager.h`: サウンド定義JSONの読み込み関数追加

### 4.2 Step 2: UI XML統合（3-5日）

**新規ファイル**:
- `engine/xml_menu_parser.h` / `.cpp` - XMLパーサー
- `ui/menu_factory.h` / `.cpp` - UI生成ファクトリ

**既存ファイル修正**:
- `ui/inventory_ui.cpp`: XMLレイアウト読み込み対応
- `ui/ui_map_panel.cpp`: XMLレイアウト読み込み対応
- `ui/hud_renderer.cpp`: HUD XML読み込み対応

**実装例**:
```cpp
// xml_menu_parser.h
class XmlMenuParser {
public:
    bool parseFile(const std::string& xmlPath);
    std::vector<UIComponentDesc> getComponents() const;
    
private:
    bool parseRect(tinyxml2::XMLElement* elem);
    bool parseImage(tinyxml2::XMLElement* elem);
    bool parseText(tinyxml2::XMLElement* elem);
};
```

### 4.3 Step 3: フォント統合（2-3日・オプション）

**新規ファイル**:
- `engine/font_converter.h` / `.cpp` - `.tex` → `.png` 変換
- `ui/bitmap_font.h` / `.cpp` - ビットマップフォントレンダラー

**注意**: `.tex` ファイルはOblivion独自の圧縮形式の可能性あり。ヘッダー解析が必要。

### 4.4 Step 4: メッシュ統合（Phase 10・2-3週間）

**アプローチ**: NIF → OBJ変換 → 既存ローダーで読み込み

```bash
# Blender Pythonスクリプトで一括変換
blender --background --python convert_nif_to_obj.py
```

---

## 5. リスクと対策

| リスク | 影響 | 確率 | 対策 |
|--------|------|------|------|
| NIFフォーマットが複雑すぎる | Phase 10遅延 | 高 | 変換ツール使用を前提とする |
| .texフォントが読めない | フォント統合不可 | 中 | 既存フォントフォールバック |
| WAVファイルサイズが大きい | APKサイズ肥大 | 高 | 圧縮OGG-Vorbisに変換 |
| XMLレイアウトが複雑 | UI統合に時間がかかる | 中 | 主要メニューのみ先に対応 |

---

## 6. APKサイズ影響試算

| 素材カテゴリ | サイズ | 圧縮後（推定） | 統合時期 |
|-------------|--------|--------------|---------|
| 既存プロジェクト | 8.8 MB | - | 現在 |
| UIテクスチャ（必要に応じて） | ? | ~10 MB | Phase 9 |
| 効果音WAV（選択的） | ~50 MB | ~25 MB | Phase 9 |
| ボイスWAV（変換後） | ~200 MB | ~100 MB | Phase 10 |
| 3Dメッシュ（変換後） | ~235 MB | ~120 MB | Phase 10+ |
| **合計（Phase 9）** | **~85 MB** | **~45 MB** | |
| **合計（Phase 10）** | **~500 MB** | **~260 MB** | |

**注意**: Google PlayのAPKサイズ上限は200MB（拡張ファイル使用で2GBまで可能）。

---

## 7. 今すぐ開始できるタスク

### 本日から実行可能:

1. **音声WAVのコピー**（30分）
   - `sound/fx/ambient/` → `app/src/main/assets/audio/sfx/ambient/`
   
2. **AudioManager::loadWavFile()実装**（4-6時間）
   - PCM WAVヘッダー解析
   - OpenALバッファ生成
   
3. **UI XMLパーサーの雛形作成**（2-3時間）
   - tinyxml2を使用した基本構造
   
4. **サウンド定義JSON作成**（1-2時間）
   - 主要効果音のマッピング

---

## 8. まとめ

### ✅ 即座に統合可能
- **WAV音声**: OpenALでそのまま使用
- **UI XML**: 座標・サイズを既存UIに適用
- **メニュー文字列**: ローカリゼーションに追加

### ⚠️ 変換/実装が必要
- **MP3ボイス**: WAV変換後に統合
- **フォント**: `.tex` → `.png` 変換が必要
- **NIFメッシュ**: 変換ツールまたはパーサーが必要

### ❌ フェーズ9では対象外
- **KFアニメーション**: メッシュ統合後に対応
- **EGM/FaceGen**: 顔生成システム実装後に対応
- ** distantlod**: 描画最適化フェーズで対応

---

**次のアクション**: 音声WAVのコピーと `loadWavFile()` 実装を開始しますか？
