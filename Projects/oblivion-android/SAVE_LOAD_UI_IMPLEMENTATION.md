# SaveLoadUI 実装ガイド - Phase 4D-UI 完了

## 概要

Phase 4D-UIが完了しました。SaveLoadUIは以下の機能を提供します：

- **セーブメニュー**: 5つのセーブスロット選択
- **ロードメニュー**: 既存セーブの読込
- **セーブ名入力**: カスタム名でセーブ
- **オートセーブ表示**: オートセーブの確認と読込
- **スロット削除**: セーブスロットの削除
- **メタデータ表示**: キャラ名、レベル、場所、プレイ時間

---

## ファイル構成

### 新規ファイル
- **ui/save_load_ui.h** (280行) - SaveLoadUIクラス定義
- **ui/save_load_ui.cpp** (600行) - SaveLoadUI実装

### 更新ファイル
- **CMakeLists.txt** - `ui/save_load_ui.cpp`を追加
- **localization_manager.cpp** - セーブ/ロード関連の日本語/英語翻訳を追加

---

## SaveLoadUI API

### 基本的な使用方法

```cpp
// 初期化
SaveLoadUI saveLoadUI;
saveLoadUI.initialize(gameManager, localizationManager);

// メニュー表示
saveLoadUI.showLoadMenu();    // ロードメニュー表示
saveLoadUI.showSaveMenu();    // セーブメニュー表示
saveLoadUI.showAutoSave();    // オートセーブ確認

// ユーザー入力処理
saveLoadUI.onTouchEvent(x, y);
saveLoadUI.onKeyPress(key);

// 描画とアップデート
saveLoadUI.update(deltaTime);
saveLoadUI.render();

// 表示/非表示
saveLoadUI.toggle();
bool isVisible = saveLoadUI.isVisible();
```

### パネルの種類

```cpp
enum class SaveLoadPanel {
    SAVE_MENU,           // セーブスロット選択
    LOAD_MENU,           // ロードスロット選択
    SAVE_CONFIRM,        // セーブ確認ダイアログ
    SAVE_NAME_INPUT,     // セーブ名入力
    LOAD_CONFIRM,        // ロード確認ダイアログ
    DELETE_CONFIRM,      // 削除確認ダイアログ
    MAIN_MENU            // メニュー戻る
};
```

---

## UI フロー

### セーブフロー
```
showSaveMenu()
    ↓
[セーブスロット一覧表示]
    ↓
selectSlot(slotId) → SAVE_NAME_INPUT
    ↓
[セーブ名入力]
    ↓
confirmSave() → SAVE_CONFIRM
    ↓
[確認ダイアログ]
    ↓
confirmSave() → GameManager::saveGame()
    ↓
toggle() [UIを閉じる]
```

### ロードフロー
```
showLoadMenu()
    ↓
[ロード可能なセーブ一覧表示]
    ↓
selectSlot(slotId) → LOAD_CONFIRM
    ↓
[確認ダイアログ]
    ↓
confirmLoad() → GameManager::loadGame()
    ↓
toggle() [UIを閉じる]
```

### 削除フロー
```
[ロード/セーブメニュー]
    ↓
deleteSlot(slotId) → DELETE_CONFIRM
    ↓
[削除確認ダイアログ]
    ↓
confirmDelete() → GameManager::deleteSaveSlot()
    ↓
back() [メニューに戻る]
```

---

## UI パネル詳細

### 1. セーブメニュー (SAVE_MENU)

```
┌─────────────────────────────────────┐
│         ゲームを保存               │
│                                     │
│ [スロット 0 - 冒険者          ]    │ ← クリック可能
│  Lv.15 | 場所: Cyrodiil | 12時間  │
│                                     │
│ [スロット 1 - [空]            ]    │
│                                     │
│ [スロット 2 - 別のキャラ      ]    │
│  Lv.20 | 場所: Skingrad  | 8時間   │
│                                     │
│ [スロット 3 - [空]            ]    │
│                                     │
│ [スロット 4 - [空]            ]    │
│                                     │
│ [戻る]                              │
└─────────────────────────────────────┘
```

### 2. ロードメニュー (LOAD_MENU)

```
┌─────────────────────────────────────┐
│      ゲームをロード                │
│                                     │
│ [スロット 0 - 冒険者          ]    │ ← 非空スロットのみ
│  Lv.15 | 場所: Cyrodiil | 12時間  │
│                                     │
│ [スロット 2 - 別のキャラ      ]    │
│  Lv.20 | 場所: Skingrad  | 8時間   │
│                                     │
│ [オートセーブ]                      │
│  Lv.15 | 場所: Cyrodiil | 1時間   │
│                                     │
│ [戻る]                              │
└─────────────────────────────────────┘
```

### 3. セーブ名入力 (SAVE_NAME_INPUT)

```
┌─────────────────────────────────────┐
│      セーブ名を入力                │
│                                     │
│ セーブ名を入力してください（最大20文字）
│ スロット 0                         │
│                                     │
│ ┌─────────────────────────┐        │
│ │ _新しいセーブ          │        │ ← テキスト入力
│ └─────────────────────────┘        │
│                                     │
│ [決定]              [キャンセル]    │
└─────────────────────────────────────┘
```

### 4. セーブ確認 (SAVE_CONFIRM)

```
┌─────────────────────────────────────┐
│      セーブ確認                     │
│                                     │
│ セーブ名: 新しいセーブ             │
│ このスロットに上書きしますか？      │
│                                     │
│ [はい]              [いいえ]        │
└─────────────────────────────────────┘
```

### 5. ロード確認 (LOAD_CONFIRM)

```
┌─────────────────────────────────────┐
│      ロード確認                     │
│                                     │
│ キャラ: 冒険者                     │
│ Lv.15 | 場所: Cyrodiil | 12時間   │
│ セーブ: 2026-05-03 14:30:45        │
│                                     │
│ このセーブをロードしますか？        │
│                                     │
│ [はい]              [いいえ]        │
└─────────────────────────────────────┘
```

### 6. 削除確認 (DELETE_CONFIRM)

```
┌─────────────────────────────────────┐
│      削除確認                       │
│                                     │
│ キャラ: 冒険者                     │
│ Lv.15 | 場所: Cyrodiil | 12時間   │
│ セーブ: 2026-05-03 14:30:45        │
│                                     │
│ このセーブを削除してもよろしいですか？
│                                     │
│ [はい]              [いいえ]        │
└─────────────────────────────────────┘
```

---

## UI 要素の寸法

```cpp
// ボタン
BUTTON_WIDTH = 400.0f    // 幅
BUTTON_HEIGHT = 60.0f    // 高さ
BUTTON_SPACING = 20.0f   // ボタン間隔

// 位置
START_X = 50.0f          // X開始位置
START_Y = 100.0f         // Y開始位置
MAX_VISIBLE_SLOTS = 5    // 表示最大スロット数

// アニメーション
ANIMATION_SPEED = 0.3f   // アニメーション速度（秒）
```

---

## 翻訳キー

SaveLoadUIで使用される翻訳キー：

### メニュータイトル
```
save_menu_title          セーブメニュー
load_menu_title          ロードメニュー
autosave_title           オートセーブ
```

### 入力・確認メッセージ
```
save_name_input_title    セーブ名入力
save_name_instruction    セーブ名入力指示
save_confirm_title       セーブ確認
save_confirm_message     上書き確認メッセージ
load_confirm_title       ロード確認
load_confirm_message     ロード確認メッセージ
delete_confirm_title     削除確認
delete_confirm_message   削除確認メッセージ
```

### ボタンラベル
```
button_yes      はい
button_no       いいえ
button_ok       決定
button_cancel   キャンセル
button_back     戻る
button_delete   削除
button_load     ロード
button_save     保存
```

### スロット情報
```
slot_empty       [空]
slot_character   キャラ:
slot_level       Lv.
slot_location    場所:
slot_playtime    プレイ時間:
slot_save_time   セーブ:
```

---

## ユーザー入力処理

### タッチ入力

```cpp
// スロットボタンクリック
onTouchEvent(x, y) {
    // スロットボタン領域をチェック
    // selectSlot(slotId)を呼び出し
}

// 確認ボタン
// Yes ボタン領域: (100, 350) - 250, 400
// No ボタン領域: (280, 350) - 430, 400
```

### キーボード入力

```cpp
onKeyPress(key) {
    8   → バックスペース (テキスト削除)
    13  → Enter (確認)
    27  → Escape (戻る)
    32-126 → 文字入力 (テキスト追加)
}
```

---

## 入力検証

### セーブ名の検証

```cpp
bool isValidSaveName(const std::string& name) {
    // 長さチェック: 1-20文字
    if (name.empty() || name.length() > 20)
        return false;
    
    // 文字チェック: ASCII 32-126のみ
    for (char c : name) {
        if (c < 32 || c > 126)
            return false;
    }
    
    return true;
}
```

### デフォルト名

セーブ名が空の場合: `"セーブ" + slotId`

```cpp
std::string saveName = textInput.empty() ?
    ("セーブ" + std::to_string(selectedSlotId)) : textInput;
```

---

## GameManager統合

SaveLoadUIは以下のGameManagerメソッドを使用：

```cpp
// セーブ
gameManager->saveGame(slotId, saveName);

// ロード
gameManager->loadGame(slotId);

// スロット削除
gameManager->deleteSaveSlot(slotId);

// スロット情報取得
gameManager->getAllSaveSlots();
gameManager->getSaveSlot(slotId);

// オートセーブ確認
gameManager->hasAutoSave();
gameManager->loadAutoSave();
```

---

## TitleScreen統合例

TitleScreenに「ゲーム読込」オプションを追加：

```cpp
class TitleScreen {
private:
    std::unique_ptr<SaveLoadUI> saveLoadUI;
    
public:
    void initialize(GameManager* gm, LocalizationManager* lm) {
        saveLoadUI = std::make_unique<SaveLoadUI>();
        saveLoadUI->initialize(gm, lm);
    }
    
    void update(float deltaTime) {
        // ... existing code ...
        if (shouldShowLoadMenu) {
            saveLoadUI->showLoadMenu();
            saveLoadUI->toggle();
        }
        
        saveLoadUI->update(deltaTime);
    }
    
    void render() {
        // ... render menu ...
        saveLoadUI->render();
    }
    
    void onTouchEvent(float x, float y) {
        if (saveLoadUI->isVisible()) {
            saveLoadUI->onTouchEvent(x, y);
        } else {
            // ... existing menu handling ...
        }
    }
};
```

---

## 本ゲーム内統合例

ゲームプレイ中のポーズメニュー：

```cpp
class PauseMenu {
private:
    std::unique_ptr<SaveLoadUI> saveLoadUI;
    
public:
    void onSaveButtonPressed() {
        saveLoadUI->showSaveMenu();
        saveLoadUI->toggle();
    }
    
    void onLoadButtonPressed() {
        saveLoadUI->showLoadMenu();
        saveLoadUI->toggle();
    }
};
```

---

## オートセーブ統合

セル遷移時のオートセーブ：

```cpp
void WorldManager::onCellTransition(uint32_t oldCell, uint32_t newCell) {
    // ... existing transition code ...
    
    // オートセーブ作成
    if (gameManager) {
        gameManager->createAutoSave();
        LOGI("Auto-save created on cell transition");
    }
}
```

---

## エラーハンドリング

SaveLoadUIは以下をハンドル：

### セーブ失敗
```cpp
if (!gameManager->saveGame(slotId, saveName)) {
    // エラーメッセージ表示
    // 前のメニューに戻る
    back();
}
```

### ロード失敗
```cpp
if (!gameManager->loadGame(slotId)) {
    // エラーメッセージ表示
    // メニューに留まる
}
```

### 無効なセーブ名
```cpp
if (!isValidSaveName(saveName)) {
    // エラー表示
    // 入力画面に戻る
}
```

---

## レンダリング実装ノート

現在の実装では、実際のOpenGL描画は以下のメソッドにプレースホルダーとして記載されています：

```cpp
void renderButton(const std::string& label, float x, float y, float w, float h, bool highlighted);
void renderText(const std::string& text, float x, float y, float scale);
void renderSlotMetadata(const SaveSlot& slot, float x, float y);
```

実装時には以下を追加：
1. OpenGL矩形描画（背景）
2. テキストレンダラー統合
3. ボタンのホバー/クリック視覚効果
4. スクロール機能（スロット数が多い場合）

---

## 次フェーズ: Phase 4E - テスト

実装完了後の予定：

1. **ユニットテスト**
   - JSON シリアライゼーション
   - SaveManager ファイルI/O
   - GameState 検証

2. **統合テスト**
   - SaveLoadUI と GameManager 連携
   - セーブ/ロード サイクル
   - MOD データ永続化

3. **デバイステスト**
   - 実機での Save/Load
   - クラッシュ復旧
   - パフォーマンス測定

---

## ステータス

✅ **Phase 4D-UI 完了**
- SaveLoadUI クラス実装
- 全パネル (6種類) 実装
- テキスト入力機能
- ユーザー入力処理
- ローカライゼーション統合
- GameManager 連携

📋 **コンパイル状態**
- CMakeLists.txt 更新完了
- ローカライゼーション翻訳追加完了
- すべてのファイルが配置完了

🎯 **次のステップ**
1. Phase 4E - ユニット/統合テスト
2. デバイステスト
3. NPC AI 拡張 (Post-Phase 4)

