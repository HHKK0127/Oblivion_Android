# Phase 9 実装計画書（改定版）

**プロジェクト**: Oblivion Android（プランA - OpenGL ES継続）  
**作成日**: 2026-06-07  
**バージョン**: 0.9.0 → 1.0.0  
**アセット戦略**: 既存アセット活用 + プレースホルダー補完（Option A）  
**予定工期**: 8週間

---

## 1. 既存アセット評価と活用方針

### 1.1 使用可能アセット一覧

| ファイル | サイズ | 用途 | 活用方針 |
|---------|--------|------|---------|
| `oblivion_logo.png` | 241 KB | Oblivion ロゴ | タイトル画面そのまま使用 |
| `main_background.png` | 1,299 KB | 羊皮紙背景 | UIパネル背景、メニュー背景に転用 |
| `shared_button_long_off.png` | 5.7 KB | 長ボタン(未選択) | メインメニューボタン |
| `shared_button_long_on.png` | 4.5 KB | 長ボタン(選択) | メインメニューボタンホバー状態 |
| `shared_button_short_off.png` | 4.0 KB | 短ボタン(未選択) | サブボタン、閉じるボタン |
| `shared_button_short_on.png` | 2.7 KB | 短ボタン(選択) | サブボタンホバー状態 |
| `sky_clouds.png` | 1,202 KB | 雲テクスチャ | タイトル画面背景（オプション） |
| `terrain_grass.png` | 556 KB | 草地 | ゲーム内既存用途（変更なし） |
| `terrain_grass2.png` | 492 KB | 草地2 | ゲーム内既存用途（変更なし） |

### 1.2 ビジュアルスタイル統一性

**確認済み**: すべてのアセットが「羊皮紙/古書風」テーマで統一
- ボタン: ベージュ/茶色系の縁取り
- 背景: 古紙テクスチャ
- ロゴ: 石造り風フォント

→ **新規プレースホルダーも同じ色合いで作成**（#C4A97F, #8B7355, #5C4033）

---

## 2. プレースホルダー（仮アセット）定義

### 2.1 プログラム生成プレースホルダー

```cpp
// ui/placeholder_assets.h
namespace PlaceholderAssets {
    // 色定義（羊皮紙風パレット）
    constexpr glm::vec3 PARCHMENT_LIGHT(0.77f, 0.66f, 0.50f);  // #C4A97F
    constexpr glm::vec3 PARCHMENT_DARK(0.55f, 0.45f, 0.33f);   // #8B7355
    constexpr glm::vec3 BROWN_ACCENT(0.36f, 0.25f, 0.20f);     // #5C4033
    constexpr glm::vec3 GOLD_HIGHLIGHT(0.85f, 0.65f, 0.13f);   // #DAA520
    
    // 描画関数（OpenGL ES）
    void drawSolidRect(float x, float y, float w, float h, glm::vec3 color);
    void drawPanel(float x, float y, float w, float h);  // 縁取り付きパネル
    void drawIconFrame(float x, float y, float size);    // アイコン枠
    void drawStatusBar(float x, float y, float w, float h, float fillRatio, glm::vec3 fillColor);
}
```

### 2.2 必要なプレースホルダー一覧

| カテゴリ | プレースホルダー名 | 実装方法 | 優先度 |
|---------|------------------|---------|--------|
| **パネル** | `panel_default` | 9パッチ風に `main_background.png` をスケーリング | 高 |
| **ステータスバー** | `bar_hp_bg`, `bar_hp_fill` | プログラム生成（緑/赤/青のグラデーション） | 高 |
| **アイコン** | `icon_inventory`, `icon_map`, `icon_quest`, `icon_settings` | プログラム生成（簡易シンボル） | 高 |
| **スクロールバー** | `scroll_track`, `scroll_thumb` | プログラム生成 | 中 |
| **チェックボックス** | `check_off`, `check_on` | プログラム生成 | 中 |
| **スライダー** | `slider_track`, `slider_thumb` | プログラム生成 | 中 |
| **アイテム枠** | `item_slot_empty`, `item_slot_selected` | プログラム生成 | 中 |
| **マップマーカー** | `marker_player`, `marker_quest`, `marker_npc` | プログラム生成（三角/丸） | 低 |
| **NPC肖像** | `portrait_placeholder` | 単色矩形 + 「?」テキスト | 低 |

---

## 3. 開発フェーズ（MVPアプローチ）

### フェーズ9A: UIフレームワーク基盤（Week 1-2）

**目標**: 既存アセットを読み込み、基本的なUIコンポーネントを動作させる

#### Week 1: 基盤実装

**Day 1-2: アセットパイプライン整備**
- [ ] 既存テクスチャを `app/src/main/assets/textures/` にコピー（必要な場合）
- [ ] DDSローダー/PNGローダーの統合確認
- [ ] `TextureManager` にUI用テクスチャ登録機能追加
- [ ] プレースホルダー描画関数実装（`ui/placeholder_assets.cpp`）

**Day 3-4: UIコアフレームワーク**
- [ ] `UIComponent` 基底クラス作成
  - 位置、サイズ、表示/非表示、タッチ判定
  - 親子関係（Panelが子コンポーネントを持つ）
- [ ] `UIPanel` 作成（背景テクスチャ対応）
  - `main_background.png` をスケーリングして使用
- [ ] `UIButton` 作成（既存ボタンテクスチャ対応）
  - 通常/ホバー/押下の3状態
  - テキストラベルオーバーレイ

**Day 5-7: HUD実装**
- [ ] `HUDRenderer` 作成（ゲーム画面上のオーバーレイ）
- [ ] HP/MP/STAバー（プログラム生成プレースホルダー）
- [ ] ミニマップ枠（プログラム生成）
- [ ] クイックスロットバー（プログラム生成）
- [ ] 既存デバッグHUDとの統合（フェーズ8から引継ぎ）

**Week 1 成果物**:
- `ui/ui_component.h/cpp`
- `ui/ui_panel.h/cpp`
- `ui/ui_button.h/cpp`
- `ui/hud_renderer.h/cpp`
- `ui/placeholder_assets.h/cpp`

#### Week 2: メニューUI実装

**Day 8-10: メインメニュー**
- [ ] `MainMenuUI` 作成
  - 背景: `main_background.png`（全画面 or 中央配置）
  - ロゴ: `oblivion_logo.png`（上部中央）
  - ボタン: 「はじめから」「つづきから」「設定」「終了」
  - ボタンテクスチャ: `shared_button_long_off/on.png`
- [ ] ボタンアニメーション（ホバー時スケール1.05倍、フェード0.1秒）
- [ ] タッチ判定精度調整

**Day 11-12: 設定メニュー**
- [ ] `SettingsUI` グラフィカル化
  - 背景パネル: `main_background.png`
  - 戻るボタン: `shared_button_short_off/on.png`
  - トグルスイッチ（プログラム生成）
  - 言語選択ボタン（国旗アイコンはプレースホルダー）

**Day 13-14: セーブ/ロードメニュー**
- [ ] `SaveLoadUI` グラフィカル化（フェーズ7.1から引継ぎ）
  - スロット表示をパネル化
  - 選択中スロットのハイライト表示
  - 空スロットの「EMPTY」表示

**Week 2 成果物**:
- `ui/main_menu_ui.h/cpp`
- `ui/settings_ui.h/cpp`（更新）
- `ui/save_load_ui.h/cpp`（更新）

---

### フェーズ9B: インベントリシステム（Week 3-4）

#### Week 3: データ層 + UIレイアウト

**Day 15-17: アイテムデータ層**
- [ ] `Item` クラス実装
  - 属性: ID, 名前, 種別(Weapon/Armor/Potion/Key), 重量, 価値
  - JSONデータベース読み込み（assets/items/item_database.json）
- [ ] `Inventory` クラス実装
  - グリッド管理（8×5 = 40スロット）
  - 重量制限（プレイヤー筋力依存）
  - スタック対応（ポーション等99個まで）
- [ ] `EquipmentManager` 実装
  - 装備スロット: Head, Body, Hands, Feet, MainHand, OffHand
  - 装備時のステータス反映

**Day 18-19: インベントリUI基盤**
- [ ] `InventoryUI` 作成
  - 背景パネル（羊皮紙風）
  - グリッド表示（40スロット）
  - スロット枠（プログラム生成）
  - 装備品表示エリア
- [ ] ドラッグ＆ドロップ基盤
  - タッチ開始: アイテム選択
  - タッチ移動: アイテムアイコン追随
  - タッチ終了: ドロップ判定

**Day 20-21: アイテム詳細ポップアップ**
- [ ] アイテム長押しで詳細表示
  - アイテム名、説明文、ステータス
  - ボタン: 「装備」「使用」「捨てる」「閉じる」
  - 背景: `main_background.png`（小サイズにスケーリング）

**Week 3 成果物**:
- `game/item.h/cpp`
- `game/inventory.h/cpp`
- `game/equipment_manager.h/cpp`
- `ui/inventory_ui.h/cpp`
- `assets/items/item_database.json`（サンプル10個）

#### Week 4: 機能統合 + セーブ連携

**Day 22-23: ドロップ＆ピックアップ**
- [ ] ワールド内アイテムドロップ
  - インベントリから「捨てる」で地面に出現
  - 3Dモデルは立方体プレースホルダー（色で種別表示）
- [ ] 近接ピックアップ
  - プレイヤーから2m以内で「拾う」ボタン表示
  - ボタン: `shared_button_short_off/on.png`

**Day 24-25: 消費と装備**
- [ ] ポーション使用: HP/MP回復
- [ ] 装備変更: 防御力/攻撃力のリアルタイム反映
- [ ] CombatManager連携: 武器ダメージ参照

**Day 26-28: セーブ/ロード統合**
- [ ] セーブデータにインベントリ状態追加
  - スロット内容、アイテム数
  - 装備状態
- [ ] ロード時の復元処理

**Week 4 成果物**:
- インベントリ完全機能
- セーブ/ロード連携完了
- ドロップ/ピックアップ動作確認

---

### フェーズ9C: マップシステム（Week 5）

#### Week 5: マップ実装

**Day 29-30: マップデータ層**
- [ ] `WorldMap` クラス作成
  - 既存セルデータからマップテクスチャ生成
  - セルごとに色分け（草原=緑、洞窟=灰、都市=茶）
  - プロシージャル生成（セルタイプに応じた単色）
- [ ] `MapMarker` クラス
  - 種別: Player, Quest, NPC, FastTravel
  - プログラム生成アイコン

**Day 31-32: マップUI**
- [ ] `MapUI` 作成
  - 全画面表示（背景: `main_background.png`）
  - ピンチズーム（0.5x ~ 3.0x）
  - ドラッグパン
  - プレイヤー位置中央追従（トグル可能）

**Day 33-34: クエストマーカー統合**
- [ ] アクティブクエストの目標位置をマーカー表示
- [ ] マーカータップでクエスト名表示
- [ ] マーカー色: メインクエスト=金、サイドクエスト=青

**Day 35: ファストトラベル基盤**
- [ ] 発見済み地点の記録
- [ ] マップ上のファストトラベルポイント表示
- [ ] UI実装（次フェーズで完全対応）

**Week 5 成果物**:
- `game/world_map.h/cpp`
- `game/map_marker.h/cpp`
- `ui/map_ui.h/cpp`
- プロシージャルマップテクスチャ生成

---

### フェーズ9D: UIポリッシュ + 統合テスト（Week 6）

#### Week 6: 統合と最適化

**Day 36-37: UIアニメーション**
- [ ] 画面遷移: フェード（0.2秒）
- [ ] パネル表示: スライドイン（下から）
- [ ] ボタン: ホバースケール + 色変化
- [ ] 通知トースト: 上からスライドイン、3秒後消去

**Day 38-39: 通知システム**
- [ ] アイテム取得通知（「○○を手に入れた」）
- [ ] クエスト更新通知
- [ ] レベルアップ通知
- [ ] トーストUI（プログラム生成、上部表示）

**Day 40-41: チュートリアル統合**
- [ ] 初回プレイ時の操作ガイド
- [ ] 画像付きチュートリアル（プレースホルダー図形）
- [ ] 「次へ」「スキップ」ボタン（既存ボタンテクスチャ使用）

**Day 42: 統合テスト準備**
- [ ] 全UI画面の遷移マップ作成
- [ ] タッチ操作の網羅的テスト項目作成
- [ ] メモリ使用量測定（目標: < 120 MB）

**Week 6 成果物**:
- `ui/ui_animation.h/cpp`
- `ui/notification_ui.h/cpp`
- `ui/tutorial_ui.h/cpp`
- 統合テスト計画書

---

### フェーズ9E: テスト・最適化（Week 7-8）

#### Week 7: デバイステスト + 修正

**Day 43-45: マルチデバイステスト**
| デバイス | テスト内容 |
|---------|-----------|
| Xiaomi 24018RPACG (Android 16) | フル機能テスト、パフォーマンス計測 |
| Amazon Fire Tablet (Android 9) | タッチUI検証、メモリ制限確認 |
| エミュレータ (Pixel 7 API 33) | 解像度変更テスト、回転対応 |

**テスト項目**:
- [ ] 全メニュー表示・遷移（テキスト→グラフィカル化確認）
- [ ] インベントリ操作（ドラッグ＆ドロップ、重量制限）
- [ ] マップ表示（ズーム、パン、マーカー）
- [ ] セーブ/ロード（インベントリ状態維持）
- [ ] 長時間プレイ（30分、メモリリーク確認）

**Day 46-47: バグ修正**
- [ ] タッチ判定ズレ修正
- [ ] テクスチャ読み込みエラー対応
- [ ] メモリリーク修正（テクスチャ解放確認）

**Day 48-49: パフォーマンス最適化**
- [ ] UI描画オーバーヘッド計測（目標: < 2ms/フレーム）
- [ ] テクスチャ圧縮（ETC2）適用検討
- [ ] 不要なテクスチャバインド削減

#### Week 8: リリース準備

**Day 50-52: ドキュメント更新**
- [ ] `README.ja.md` 更新（Phase 9機能反映）
- [ ] `CHANGELOG.md` 更新
- [ ] `KNOWN_ISSUES.md` 更新
- [ ] スクリーンショット撮影（5枚）

**Day 53-54: ビルド確認**
- [ ] Releaseビルド（APKサイズ確認、目標: < 15 MB）
- [ ] 署名確認
- [ ] NDK r26.1互換性最終確認

**Day 55-56: Phase 9完了レビュー**
- [ ] コードレビュー（アーキテクチャ一貫性）
- [ ] アセット整理（未使用ファイル削除）
- [ ] クリーンアップ（デバッグログ削減）

---

## 4. ディレクトリ構成（最終形）

```
app/src/main/
├── assets/
│   ├── textures/
│   │   ├── menus/              # 既存ボタン（そのまま）
│   │   ├── ui/                 # NEW: UIテクスチャ（main_background等をコピー）
│   │   ├── items/              # NEW: アイテムアイコン（プレースホルダー）
│   │   └── map/                # NEW: マップ関連（プロシージャル生成）
│   └── items/
│       └── item_database.json  # NEW: アイテム定義
├── cpp/
│   ├── ui/                     # NEW: UIフレームワーク
│   │   ├── ui_component.h/cpp
│   │   ├── ui_panel.h/cpp
│   │   ├── ui_button.h/cpp
│   │   ├── hud_renderer.h/cpp
│   │   ├── main_menu_ui.h/cpp
│   │   ├── settings_ui.h/cpp
│   │   ├── save_load_ui.h/cpp
│   │   ├── inventory_ui.h/cpp
│   │   ├── map_ui.h/cpp
│   │   ├── notification_ui.h/cpp
│   │   ├── tutorial_ui.h/cpp
│   │   └── placeholder_assets.h/cpp
│   ├── game/
│   │   ├── item.h/cpp          # NEW
│   │   ├── inventory.h/cpp     # NEW
│   │   ├── equipment_manager.h/cpp  # NEW
│   │   ├── world_map.h/cpp     # NEW
│   │   └── map_marker.h/cpp    # NEW
│   └── engine/
│       └── texture_manager.h/cpp  # 更新: UIテクスチャ対応
└── res/
    └── ...（既存のまま）
```

---

## 5. リスク管理

| リスク | 確率 | 影響 | 対策 |
|--------|------|------|------|
| ドラッグ＆ドロップ操作が不安定 | 中 | 高 | 早期にDay 18-19でプロトタイプ作成し、Xiaomi端末で即座に検証 |
| メモリ使用量が120MBを超過 | 低 | 高 | Week 7でプロファイリング、テクスチャ解放徹底 |
| 既存ボタンテクスチャの解像度不足 | 低 | 中 | 4倍スケーリングで問題なし（512x512以上の画面ではややぼやける可能性） |
| JSONパーサーの不具合 | 低 | 中 | item_database.jsonは10個から開始し段階的拡張 |
| 納期遅延 | 中 | 中 | マップのファストトラベルはWeek 5で基盤のみ、完全版はWeek 7以降に回せる |

---

## 6. 成功基準

Phase 9完了条件:

- [ ] **UI**: 全メニューがグラフィカル表示（テキストUIゼロ）
- [ ] **インベントリ**: 40スロット、ドラッグ＆ドロップ、装備変更、セーブ連携
- [ ] **マップ**: プロシージャル表示、ピンチズーム、クエストマーカー
- [ ] **パフォーマンス**: 60 FPS維持、メモリ< 120 MB
- [ ] **安定性**: 30分連続プレイでクラッシュゼロ
- [ ] **互換性**: Xiaomi + Fire Tablet で動作確認
- [ ] **APKサイズ**: < 15 MB（アセット追加後も）

---

## 7. 次のステップ（即座に開始可能）

### 本日から開始できるタスク

1. **アセットコピー**（30分）
   ```bash
   cp Projects/oblivion-android/app/src/main/assets/textures/menus/* app/src/main/assets/textures/ui/
   cp Projects/oblivion-android/app/src/main/assets/textures/main_background.png app/src/main/assets/textures/ui/
   cp Projects/oblivion-android/app/src/main/assets/textures/oblivion_logo.png app/src/main/assets/textures/ui/
   ```

2. **UIディレクトリ作成**（5分）
   ```bash
   mkdir -p app/src/main/cpp/ui
   mkdir -p app/src/main/assets/items
   ```

3. **プレースホルダー実装開始**（Day 1-2）
   - `ui/placeholder_assets.h/cpp` 作成
   - パレット定義 + 基本図形描画関数

---

**承認待ち**: 本計画書承認後、Week 1 Day 1 から実装開始

**最終更新**: 2026-06-07
