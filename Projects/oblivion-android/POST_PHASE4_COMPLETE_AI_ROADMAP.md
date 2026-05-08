# Post-Phase 4 Complete AI Roadmap - 全フェーズ統合計画

**作成日**: 2026-05-03  
**ステータス**: 完全計画書（実装前確認用）  
**対象**: NPC AI強化（既存機能保持）  
**予定時間**: 6.5-7時間

---

## 📊 全フェーズ概要マトリックス

```
┌─────────────┬──────────────────┬──────────┬─────────┬─────────────┐
│ フェーズ    │ 目標             │ 時間(h)  │ 優先度  │ 依存関係    │
├─────────────┼──────────────────┼──────────┼─────────┼─────────────┤
│ Phase 1     │ 基本移動AI       │ 1-2      │ 高      │ なし        │
│ (WANDER)    │ WANDER/PATROL    │          │         │             │
│             │ FOLLOW_PLAYER    │          │         │             │
├─────────────┼──────────────────┼──────────┼─────────┼─────────────┤
│ Phase 2     │ 戦闘AI強化       │ 2        │ 高      │ Phase 1 ◄── │
│ (Combat AI) │ HP管理           │          │         │ (同時OK)    │
│             │ スペル優先度     │          │         │             │
├─────────────┼──────────────────┼──────────┼─────────┼─────────────┤
│ Phase 3     │ 状態管理         │ 1.5      │ 中      │ Phase 1,2 ◄ │
│ (State      │ イベント駆動     │          │         │ (順序推奨)  │
│  Machine)   │ 遷移ロジック     │          │         │             │
├─────────────┼──────────────────┼──────────┼─────────┼─────────────┤
│ Phase 4     │ NPC会話          │ 2        │ 中      │ 独立実装OK  │
│ (Dialogue)  │ ダイアログツリー │          │         │ (最後推奨)  │
│             │ クエスト統合     │          │         │             │
├─────────────┼──────────────────┼──────────┼─────────┼─────────────┤
│ テスト      │ Unit/統合/Device │ 1-1.5    │ 高      │ すべて完了後│
├─────────────┼──────────────────┼──────────┼─────────┼─────────────┤
│ 合計        │ 完全なNPC AI     │ 6.5-7    │ -       │             │
└─────────────┴──────────────────┴──────────┴─────────┴─────────────┘
```

---

## 🔄 推奨実装順序と並列化

### オプションA: 順序実装（安定、確認しやすい）
```
Phase 1 (1-2h) → Phase 2 (2h) → Phase 3 (1.5h) → Phase 4 (2h) → テスト
        ↓                ↓                 ↓                ↓
    確認           確認            確認            確認
```
**所要時間**: 8-8.5時間（シリアル）

### オプションB: 並列実装（効率的）
```
Phase 1 & 2 (並列) → Phase 3 → Phase 4 → テスト
   2-3時間で両方      1.5h      2h      1-1.5h
```
**所要時間**: 6.5-7時間（推奨）

**推奨**: **オプションB - 並列実装**
- Phase 1 と 2 は依存関係がない（同時実装可）
- Phase 3 は 1,2 の完了後（状態管理のため）
- Phase 4 は独立実装可（最後に統合推奨）

---

## 📁 ファイル変更マトリックス

### Phase 1: 基本移動AI

| ファイル | 変更 | 行数 | 内容 |
|---------|------|------|------|
| `game/npc.h` | 拡張 | +20 | WANDER/PATROL 用メンバ |
| `game/npc.cpp` | 拡張 | +150 | WANDER/PATROL/FOLLOW 実装 |
| `game/npc_manager.h` | 拡張 | +5 | addPatrolWaypoint() |
| `game/npc_manager.cpp` | 拡張 | +20 | ウェイポイント管理 |

**合計追加**: 195行程度

### Phase 2: 高度な戦闘AI

| ファイル | 変更 | 行数 | 内容 |
|---------|------|------|------|
| `game/npc.h` | 拡張 | +10 | 戦闘AI用メンバ |
| `game/npc.cpp` | 拡張 | +150 | スペル選択強化、HP管理AI |
| `game/combat_manager.cpp` | 拡張 | +100 | AI判断統合 |

**合計追加**: 260行程度

### Phase 3: ステートマシン

| ファイル | 変更 | 行数 | 内容 |
|---------|------|------|------|
| `game/npc.h` | 拡張 | +5 | AIState 拡張（新状態） |
| `game/ai_state_machine.h` | **新規** | 80 | ステートマシン定義 |
| `game/ai_state_machine.cpp` | **新規** | 120 | ステートマシン実装 |
| `game/npc_manager.cpp` | 拡張 | +50 | 状態遷移管理 |

**合計追加**: 255行程度

### Phase 4: 会話システム

| ファイル | 変更 | 行数 | 内容 |
|---------|------|------|------|
| `game/dialogue_system.h` | **新規** | 100 | ダイアログシステム |
| `game/dialogue_system.cpp` | **新規** | 200 | 会話実装、ツリー |
| `game/npc.h` | 拡張 | +5 | 会話状態メンバ |
| `game/npc.cpp` | 拡張 | +30 | 会話メソッド |
| `localization_manager.cpp` | 拡張 | +100 | 会話テキスト（日本語化） |

**合計追加**: 435行程度

### 共通

| ファイル | 変更 | 行数 |
|---------|------|------|
| `CMakeLists.txt` | 拡張 | +10 |
| `game/ai_state_machine.cpp` | 新規 | 120 |
| `game/dialogue_system.cpp` | 新規 | 200 |

---

## 🏗️ コードドメインマップ

```
game/
├── npc.h/cpp
│   ├── Phase 1: WANDER, PATROL, FOLLOW_PLAYER 実装
│   ├── Phase 2: selectSpellForCombat() 強化
│   ├── Phase 3: AIState 拡張（CONFUSED, PANICKED等）
│   └── Phase 4: 会話メソッド追加
│
├── npc_manager.h/cpp
│   ├── Phase 1: addPatrolWaypoint()
│   ├── Phase 3: AIStateMachine 統合
│   └── Phase 4: dialogue 初期化
│
├── combat_manager.h/cpp
│   ├── Phase 2: updateAIDecisions() 追加
│   └── 既存ロジック保持（変更なし）
│
├── ai_state_machine.h ★ Phase 3
│   └── 状態遷移管理
│
├── ai_state_machine.cpp ★ Phase 3
│   └── ステートマシン実装
│
├── dialogue_system.h ★ Phase 4
│   └── ダイアログ構造定義
│
├── dialogue_system.cpp ★ Phase 4
│   └── ダイアログツリー実装
│
└── quest_manager.h/cpp
    └── 既存（会話と連携）
```

**凡例**:
- `h/cpp` = 既存ファイル拡張
- `★` = 新規ファイル

---

## 🔍 詳細実装パス

### Phase 1: 基本移動AI

#### Step 1-1: NPC.h 拡張（10分）
```cpp
// 既存メンバの後に追加:
struct NPC {
    // ... 既存 ...
    
    // Phase 1 用新メンバ:
    std::vector<glm::vec3> patrolWaypoints;
    int currentWaypointIndex = 0;
    float waypointArrivalThreshold = 1.0f;
    float lastWanderTargetTime = 0.0f;
    static constexpr float WANDER_TARGET_UPDATE_TIME = 5.0f;
    
    // ヘルパーメソッド:
    void generateRandomWanderTarget();
    void moveTowardsTarget(float deltaTime);
    bool shouldDetectEnemies();
};
```

#### Step 1-2: NPC.cpp 実装（45分）
```cpp
// update() メソッドの case 文を実装:
// - case AIState::WANDER { ... }
// - case AIState::PATROL { ... }
// - case AIState::FOLLOW_PLAYER { ... }

// ヘルパー関数実装:
// - generateRandomWanderTarget()
// - moveTowardsTarget()
// - shouldDetectEnemies()
```

#### Step 1-3: NPCManager 拡張（15分）
```cpp
// npc_manager.h に追加:
void addPatrolWaypoint(uint32_t npcId, const glm::vec3& waypoint);

// npc_manager.cpp に実装
```

#### Step 1-4: テスト（20分）
- WANDER 目標生成確認
- PATROL ウェイポイント遷移確認
- FOLLOW 距離管理確認

---

### Phase 2: 高度な戦闘AI

#### Step 2-1: NPC.h スペル選択メソッド（5分）
```cpp
// 既存の selectSpellForCombat() を拡張:
uint32_t selectSpellForCombat();  // スケルトンを改善

// 新規ヘルパー:
uint32_t findSpellByType(SpellType type);
uint32_t findMostEffectiveSpell(std::shared_ptr<NPC> enemy);
bool canCastSpell(uint32_t spellId);
bool hasBuffActive(const std::string& buffName);
bool shouldFlee();
```

#### Step 2-2: NPC.cpp スペル選択AI実装（60分）
```cpp
// selectSpellForCombat() の詳細実装:
// 1. HP が 30% 以下 → ヒール優先
// 2. マナ 20% 以下 → 回復スペル
// 3. バッファなし → バッファスペル
// 4. 敵に対してダメージスペル
// 5. スペルなし → 通常攻撃

// ヘルパー関数実装（150行）
```

#### Step 2-3: CombatManager AI判断（30分）
```cpp
// combat_manager.cpp に追加:
void updateAIDecisions(CombatInstance& combat, float deltaTime);

// 戦闘更新ループに統合
```

#### Step 2-4: テスト（25分）
- スペル選択判定テスト
- HP 管理テスト
- マナ管理テスト

---

### Phase 3: ステートマシン

#### Step 3-1: AIStateMachine.h 作成（20分）
```cpp
// 新規ファイル作成:
- enum class AIEvent (10個定義)
- struct Transition
- class AIStateMachine

// 主要メソッド:
- addTransition()
- processEvent()
- update()
```

#### Step 3-2: AIStateMachine.cpp 実装（40分）
```cpp
// ステートマシン実装:
- 状態遷移ロジック
- イベント処理
- 条件判定

// 遷移テーブル初期化
```

#### Step 3-3: NPC.h AIState 拡張（5分）
```cpp
enum class AIState {
    // 既存
    IDLE, WANDER, PATROL, FOLLOW_PLAYER, COMBAT,
    // 新規
    CONFUSED, PANICKED, FLEEING, CONVERSATION, WAITING, QUEST_OBJECTIVE
};
```

#### Step 3-4: NPCManager 統合（20分）
```cpp
// npc_manager.h に AIStateMachine メンバ追加
// update() で状態遷移処理
```

#### Step 3-5: テスト（20分）
- 状態遷移確認
- イベント処理テスト
- 条件判定テスト

---

### Phase 4: 会話システム

#### Step 4-1: DialogueSystem.h 作成（25分）
```cpp
// 新規ファイル作成:
- struct DialogueNode
- struct DialogueTree
- class DialogueSystem

// メンバ:
- dialogueTrees map
- currentNodeId
```

#### Step 4-2: DialogueSystem.cpp 実装（60分）
```cpp
// 実装内容:
- initializeDialogues()
  └─ すべてのNPC会話を定義
- getNode()
- selectOption()
- getNPCResponse()

// 会話定義（各NPCごと）
```

#### Step 4-3: ローカライズ対応（30分）
```cpp
// localization_manager.cpp に追加:
- dialogue_companion_greeting
- dialogue_companion_quest
- dialogue_companion_yes
- ... (各NPC 5-10 キーずつ)
```

#### Step 4-4: NPC.cpp 会話メソッド（15分）
```cpp
// NPC に会話メソッド追加:
void startConversation(uint32_t npcId);
void endConversation();
bool isInConversation() const;
```

#### Step 4-5: テスト（20分）
- ダイアログツリー確認
- NPC 会話インタフェース
- クエスト統合

---

## 🧪 統合テスト戦略

### Unit Tests（ユニットテスト）

#### Phase 1 テスト
```cpp
// test_ai_movement.cpp
- testWanderTargetGeneration()
  └─ 半径内に目標が生成されるか
- testPatrolWaypointTransition()
  └─ ウェイポイント遷移が正確か
- testFollowDistance()
  └─ フォロー距離が維持されるか
```

#### Phase 2 テスト
```cpp
// test_combat_ai.cpp
- testSpellSelectionByHP()
  └─ HP低時にヒール選択か
- testSpellSelectionByMana()
  └─ マナ不足時に回復選択か
- testEffectiveSpellSelection()
  └─ 敵属性に応じたスペル選択か
```

#### Phase 3 テスト
```cpp
// test_state_machine.cpp
- testStateTransition()
  └─ 状態遷移が正確か
- testEventProcessing()
  └─ イベント処理が正確か
- testConditionEvaluation()
  └─ 条件判定が正確か
```

#### Phase 4 テスト
```cpp
// test_dialogue.cpp
- testDialogueTreeStructure()
  └─ ツリー構造が有効か
- testOptionSelection()
  └─ 選択肢処理が正確か
- testNPCResponse()
  └─ 返答が正しいか
```

### 統合テスト

```cpp
// test_ai_integration.cpp
- testMultipleNPCs()
  └─ 複数NPC同時動作
- testCombatInteraction()
  └─ プレイヤーとNPC戦闘
- testDialogueWithCombat()
  └─ 会話と戦闘の相互作用
- testSaveLoadWithAI()
  └─ AI状態の永続化（Phase 4との統合）
```

### デバイステスト

#### 実Android デバイス
```
1. 基本動作テスト
   - WANDER が正常に動作
   - PATROL が正常に動作
   - FOLLOW_PLAYER が正常に動作

2. 性能測定
   - FPS: 60FPS 維持か
   - メモリ: < 50MB 増加
   - CPU: 無視できるレベルか

3. エッジケース
   - 多数NPC（10+個）動作確認
   - 長時間プレイでメモリリーク確認
   - 会話中の戦闘開始確認
```

---

## 📈 品質指標（デリバリー基準）

### コード品質
- [ ] すべての新規メソッドにコメント付き
- [ ] エラーハンドリング完備
- [ ] メモリリークなし（valgrind確認）
- [ ] 既存AIロジック変更なし

### テスト完了
- [ ] ユニットテスト 100% パス
- [ ] 統合テスト完了
- [ ] デバイステスト 2台以上で確認

### パフォーマンス
- [ ] FPS: 60+ fps 維持
- [ ] メモリ: < 50MB 増加
- [ ] CPU 使用率: < 10%

### ドキュメント
- [ ] 各フェーズの実装ドキュメント
- [ ] テスト手順書
- [ ] デプロイメント手順

---

## 📅 タイムラインと依存関係

```
Day 1:
  Phase 1 (1-2h)   │  Phase 2 (2h)    [並列実装推奨]
  ├─ WANDER        │  ├─ スペル選択
  ├─ PATROL        │  ├─ HP管理
  └─ FOLLOW        │  └─ 戦闘判断
  
  Phase 3 準備 (確認)
  
Day 2:
  Phase 3 (1.5h)   [Phase 1,2 確認後]
  ├─ StateMAchine
  ├─ 状態遷移
  └─ イベント処理
  
  Phase 4 (2h) [並列実装可]
  ├─ DialogueSystem
  ├─ ダイアログツリー
  └─ ローカライズ
  
  テスト & デバッグ (1-1.5h)
  ├─ ユニットテスト
  ├─ 統合テスト
  └─ デバイステスト
```

---

## ✅ 実装前チェックリスト

### 環境準備
- [ ] CMakeLists.txt バックアップ作成
- [ ] 現在の npc.cpp/npc.h バージョン確認
- [ ] テストフレームワーク準備（save_system_test.cpp 参考）
- [ ] ログ出力設定確認（LOGD/LOGI/LOGE）

### 既存機能確認
- [ ] CombatManager 現在の実装確認
- [ ] SpellManager と の連携確認
- [ ] WorldManager との連携確認
- [ ] SaveManager との統合方法確認

### 計画確認
- [ ] 各フェーズの依存関係確認
- [ ] テスト戦略確認
- [ ] デバイステスト環境確認

---

## 🎯 実装開始時の注意点

### 重要: 既存AI保持ルール
1. **変更しないもの**
   - `calculateDamage()` ロジック
   - `findNearestEnemy()` 実装
   - `CharacterStatus` 構造

2. **拡張するもの**
   - `selectSpellForCombat()` → より高度な判定に
   - `update()` → 新しいステート処理を追加
   - `AIState` enum → 新しい状態を追加

3. **新規追加するもの**
   - 移動AI（WANDER/PATROL/FOLLOW）
   - ステートマシン
   - ダイアログシステム

### 並列実装時の注意
- Phase 1 と 2 は同時実装可（依存関係なし）
- 毎日マージして動作確認
- コンフリクト早期発見のため

### ブランチ戦略（推奨）
```
main (Phase 4 完了版)
├── feature/phase1-movement
│   └─ WANDER, PATROL, FOLLOW
├── feature/phase2-combat-ai
│   └─ スペル選択, HP管理
├── feature/phase3-state-machine
│   └─ ステートマシン
└── feature/phase4-dialogue
    └─ ダイアログシステム
```

---

## 📝 実装記録テンプレート

各フェーズ完了時に記録：

```markdown
## Phase X 実装完了報告

**完了日**: YYYY-MM-DD
**所要時間**: X時間
**実装内容**:
- [ ] Step X-1: ...
- [ ] Step X-2: ...
- [ ] Step X-3: ...
- [ ] Step X-4: ...

**テスト結果**:
- ユニットテスト: X/X パス
- 統合テスト: ○/○
- デバイステスト: ○/○

**問題・解決**:
- 問題: ...
- 解決: ...

**次フェーズへの引き継ぎ**:
- ...
```

---

## 📞 サポート・参考資料

### 参考ファイル
- `POST_PHASE4_NPC_AI_ENHANCEMENT.md` - 詳細設計書
- `test/save_system_test.cpp` - テストフレームワーク参考
- `game/combat_manager.cpp` - 既存AI実装参考

### トラブルシューティング

| 問題 | 原因 | 解決策 |
|------|------|--------|
| NPC が動かない | aiState が IDLE | setAIState(AIState::WANDER) |
| スペル選択失敗 | SpellManager なし | initialize() 時に SpellManager 渡す |
| メモリリーク | ウェイポイント未解放 | shared_ptr 使用 |
| FPS低下 | NPC多数 | NPC 数制限 or LOD 実装 |

---

## 🏁 最終チェック

実装前に以下を確認：

```
ビルド環境:
□ CMakeLists.txt 更新準備完了
□ コンパイラ設定確認（C++17）
□ 依存ライブラリ確認

実装計画:
□ Phase 1,2 並列実装か順序実装か決定
□ テスト環境準備完了
□ ブランチ戦略確定

チーム確認:
□ 既存AI保持ルール理解完了
□ フェーズ間の依存関係理解完了
□ テスト項目確認完了
```

---

## 🚀 次のステップ

**実装開始前に確認すること**:

1. ✅ この計画書の全体を確認した
2. ✅ 各フェーズの時間見積もり確認
3. ✅ 既存AI保持ルール理解
4. ✅ 並列実装か順序実装か決定
5. ✅ テスト戦略確認

**準備完了時の確認**:
```
全フェーズの実装計画は承認されましたか？
実装順序（並列 or 順序）は決定されましたか？
Phase 1 から開始してもいいですか？
```

---

**Status**: 🟡 準備完了（確認待ち）  
**Ready for**: Phase 1-4 実装開始  
**Estimated Duration**: 6.5-7 hours  
**Quality Gate**: すべてのテスト完了後リリース
