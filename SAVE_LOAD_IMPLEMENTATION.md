# Save/Load システム実装計画

## 概要
ゲーム進行状況（プレイヤーステータス、NPC状態、クエスト進捗、ワールド状態）を JSON フォーマットで永続化し、次回起動時に復元する機能。

## ファイル構造

```cpp
save_system/
├── save_manager.h           // Save/Load マネージャー
├── save_manager.cpp         // 実装
├── game_state.h             // ゲーム状態スナップショット
└── save_slots.h             // セーブスロット管理
```

## SaveManager クラス設計

### 責務
- ゲーム状態のスナップショット取得
- JSON へのシリアライズ
- ファイルへの保存
- ファイルからの読み込み
- JSON からのデシリアライズ

### インターフェース

```cpp
class SaveManager {
public:
    bool saveGame(const std::string& slotName, const GameState& state);
    bool loadGame(const std::string& slotName, GameState& outState);
    bool deleteSave(const std::string& slotName);
    std::vector<std::string> getSaveSlots() const;
    bool hasSave(const std::string& slotName) const;

private:
    std::string getSavePath(const std::string& slotName) const;
    std::string gameStateToJson(const GameState& state) const;
    bool jsonToGameState(const std::string& json, GameState& outState);
};

struct GameState {
    // Player
    uint32_t playerId;
    glm::vec3 playerPosition;
    CharacterStatus playerStatus;
    
    // World
    std::vector<uint32_t> loadedCells;
    std::vector<WorldObject> worldObjects;
    
    // NPCs
    std::map<uint32_t, NPC> npcStates;
    
    // Quests
    std::map<uint32_t, Quest> questStates;
    
    // Timestamp
    uint64_t saveTime;
    std::string saveName;
};
```

## JSON 形式

```json
{
  "version": "0.6.0",
  "saveTime": "2026-04-17T10:30:00Z",
  "player": {
    "position": [100.0, 50.0, 200.0],
    "health": 95.5,
    "mana": 120.0,
    "level": 10,
    "inventory": [...]
  },
  "world": {
    "loadedCells": [1001, 1002, 1003],
    "objects": [...]
  },
  "npcs": {
    "1007": {
      "name": "Hellas",
      "position": [105.0, 50.0, 205.0],
      "health": 100.0,
      "questGiven": [1000]
    }
  },
  "quests": {
    "1000": {
      "state": "IN_PROGRESS",
      "objectives": [...],
      "acceptTime": "2026-04-17T09:00:00Z"
    }
  }
}
```

## 実装ロードマップ

### Step 1: SaveManager ヘッダー (30分)
- GameState 構造体定義
- SaveManager クラス宣言
- JSON ライブラリ統合 (nlohmann/json)

### Step 2: JSON シリアライズ実装 (45分)
- PlayerStatus → JSON
- NPC → JSON  
- Quest → JSON
- ワールド状態 → JSON

### Step 3: ファイルI/O実装 (30分)
- Android ストレージへのパス解決
- セーブファイル書き込み
- セーブファイル読み込み

### Step 4: デシリアライズ実装 (45分)
- JSON → PlayerStatus
- JSON → NPC
- JSON → Quest
- JSON → ワールド状態

### Step 5: テスト & 統合 (30分)
- Save/Load テストシナリオ
- Renderer への統合
- エラーハンドリング

## 統合ポイント

### Renderer での使用例

```cpp
// ゲーム開始時
if (hasSaveGame()) {
    GameState savedState;
    saveManager->loadGame("slot1", savedState);
    restoreGameState(savedState);
} else {
    createNewGame();
}

// ゲーム終了時（Ctrl+S等）
GameState currentState = captureGameState();
saveManager->saveGame("slot1", currentState);
```

## 既存システムへの影響

- **NPC**: serialize() メソッド追加不要（既存メンバで足りる）
- **Quest**: serialize() メソッド追加不要
- **Renderer**: saveGame()/loadGame() メソッド追加

## リスク・対策

| リスク | 対策 |
|--------|------|
| JSON ライブラリの依存性 | nlohmann/json (ヘッダーオンリー) 使用 |
| セーブファイルサイズ | gzip 圧縮で 80% 削減予定 |
| 互換性 | version フィールドで将来対応 |

## 成功基準

✅ Save/Load ボタンで動作確認  
✅ 3つのセーブスロット管理  
✅ ゲーム状態正確に復元  
✅ ファイルサイズ < 1 MB  

---

*実装期間: 約 3-4 時間*  
*優先度: 🔴 最高*
