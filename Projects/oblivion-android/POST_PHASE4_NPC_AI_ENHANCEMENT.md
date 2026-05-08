# Post-Phase 4 - NPC AI Enhancement Plan

**Status**: 新規開始  
**Based On**: Phase 4 Save/Load完了  
**Goal**: 既存AIを保持しながら高度なAI機能を追加

---

## 現在のNPC AI状況

### ✅ 既存実装済み
| 機能 | 場所 | 状態 |
|------|------|------|
| 戦闘ダメージ計算 | CombatManager::calculateDamage() | ✅ 完全実装 |
| スペル選択AI | NPC::selectSpellForCombat() | ✅ 実装済み |
| 敵検出システム | CombatManager::findNearestEnemy() | ✅ 実装済み |
| ステータス管理 | CharacterStatus | ✅ 完全 |
| 戦闘開始/終了 | CombatManager | ✅ 完全 |
| スペルキャスト | CombatManager + SpellManager | ✅ 実装済み |

### ❌ 未実装（強化対象）
| 機能 | 位置 | 優先度 |
|------|------|--------|
| WANDER - ランダム移動 | NPC::update() | 高 |
| PATROL - パトロール経路 | NPC::update() | 高 |
| FOLLOW_PLAYER - プレイヤーフォロー | NPC::update() | 中 |
| 会話システム | 新規 | 中 |
| パスファインディング | 新規 | 低 |
| 高度な戦闘判断 | NPC + CombatManager | 高 |
| ステートマシン | 新規 | 中 |

---

## 強化計画の全体像

### フェーズ1: 基本AI行動（1-2時間）
**目標**: WANDER, PATROL, FOLLOW_PLAYER を実装

1. **WANDER実装** - ランダムな移動AI
   - 半径内でランダムな目標位置を生成
   - 移動速度に従って目標に向かう
   - 目標到達時に新しい目標を生成

2. **PATROL実装** - 経路パトロール
   - ウェイポイント配列を定義
   - ウェイポイント間を移動
   - ループ可能な経路

3. **FOLLOW_PLAYER実装** - プレイヤー追従
   - プレイヤー位置を追跡
   - 一定距離を保つ
   - コンパニオンシステムに対応

### フェーズ2: 高度な戦闘AI（2時間）
**目標**: インテリジェント戦闘判断

1. **HP管理AI**
   - HP低下時にヒール使用
   - 特定のHP%で逃げるか判断
   - バッファスペル使用タイミング

2. **戦闘戦略AI**
   - 敵の属性に応じたスペル選択
   - 距離管理（遠距離vs近距離）
   - チーム戦闘での協力

3. **スペル優先度決定**
   - ダメージスペル vs ヒール
   - マナ管理
   - スペル詠唱時間考慮

### フェーズ3: ステートマシン（1.5時間）
**目標**: 複雑なAI状態遷移

1. **状態の拡張**
   ```cpp
   enum class AIState {
       // 既存
       IDLE, WANDER, PATROL, FOLLOW_PLAYER, COMBAT,
       // 新規
       CONFUSED,      // 混乱状態
       PANICKED,      // パニック状態
       FLEEING,       // 逃走状態
       CONVERSATION,  // 会話中
       WAITING,       // 待機状態
       QUEST_OBJECTIVE // クエスト目標達成中
   };
   ```

2. **状態遷移ロジック**
   - イベント駆動型の遷移
   - タイムアウト管理
   - 条件付き遷移

### フェーズ4: 会話システム（2時間）
**目標**: NPCとの相互作用

1. **会話メニュー**
   - 利用可能なクエスト表示
   - 質問オプション
   - 日本語化対応

2. **ダイアログシステム**
   - NPCの返答
   - 感정システム
   - 報酬配布

---

## 詳細実装仕様

### フェーズ1: 基本AI行動

#### A. WANDER 実装

**ファイル**: `game/npc.cpp`

```cpp
// NPC::update() の WANDER ケース
case AIState::WANDER: {
    // 目標距離を確認
    float distToTarget = glm::distance(position, targetPosition);
    
    if (distToTarget < 0.5f) {
        // 新しい目標を生成
        generateRandomWanderTarget();
    } else {
        // 目標へ移動
        moveTowardsTarget(deltaTime);
    }
    
    // ランダムに周囲の敵を検出
    if (shouldDetectEnemies()) {
        auto enemy = combatManager->findNearestEnemy(
            shared_from_this(), 30.0f
        );
        if (enemy) {
            enterCombat(enemy);
            setAIState(AIState::COMBAT);
        }
    }
    break;
}

// ヘルパー関数
void NPC::generateRandomWanderTarget() {
    float randomAngle = (rand() % 360) * 3.14159f / 180.0f;
    float randomDist = (rand() % (int)wanderRadius);
    
    targetPosition.x = position.x + cos(randomAngle) * randomDist;
    targetPosition.z = position.z + sin(randomAngle) * randomDist;
    targetPosition.y = position.y;  // 高さは固定
}

void NPC::moveTowardsTarget(float deltaTime) {
    glm::vec3 direction = glm::normalize(targetPosition - position);
    position += direction * moveSpeed * deltaTime;
}

bool NPC::shouldDetectEnemies() {
    // 毎フレーム5%の確率でチェック（フレームレート独立）
    return (rand() % 100) < 5;
}
```

**Data**: `game/npc.h` に追加
```cpp
struct NPC {
    // ... 既存メンバー ...
    
    // 新規: WANDER用
    float lastWanderTargetTime = 0.0f;
    static constexpr float WANDER_TARGET_UPDATE_TIME = 5.0f;
    
    // 新規: PATROL用
    std::vector<glm::vec3> patrolWaypoints;
    int currentWaypointIndex = 0;
    float waypointArrivalThreshold = 1.0f;
};
```

#### B. PATROL 実装

```cpp
case AIState::PATROL: {
    if (patrolWaypoints.empty()) {
        setAIState(AIState::IDLE);
        break;
    }
    
    // 現在のウェイポイント
    glm::vec3 currentWaypoint = patrolWaypoints[currentWaypointIndex];
    float distToWaypoint = glm::distance(position, currentWaypoint);
    
    if (distToWaypoint < waypointArrivalThreshold) {
        // 次のウェイポイントへ
        currentWaypointIndex = (currentWaypointIndex + 1) % patrolWaypoints.size();
    } else {
        // ウェイポイントへ移動
        moveTowardsTarget(deltaTime);
        targetPosition = currentWaypoint;
    }
    
    // 敵検出（WANDER と同じ）
    if (shouldDetectEnemies()) {
        auto enemy = combatManager->findNearestEnemy(
            shared_from_this(), 30.0f
        );
        if (enemy) {
            enterCombat(enemy);
        }
    }
    break;
}
```

**実装例** - NPCManager で PATROL ウェイポイント設定:
```cpp
// game/npc_manager.cpp に追加
void NpcManager::addPatrolWaypoint(uint32_t npcId, const glm::vec3& waypoint) {
    auto npc = getNPC(npcId);
    if (npc) {
        npc->patrolWaypoints.push_back(waypoint);
    }
}

// 使用例:
auto guardNpc = createNPC("Guard", glm::vec3(0, 0, 0));
npcManager->addPatrolWaypoint(guardNpc->npcId, glm::vec3(0, 0, 0));
npcManager->addPatrolWaypoint(guardNpc->npcId, glm::vec3(10, 0, 0));
npcManager->addPatrolWaypoint(guardNpc->npcId, glm::vec3(10, 0, 10));
npcManager->addPatrolWaypoint(guardNpc->npcId, glm::vec3(0, 0, 10));
guardNpc->setAIState(AIState::PATROL);
```

#### C. FOLLOW_PLAYER 実装

```cpp
case AIState::FOLLOW_PLAYER: {
    // プレイヤー位置を取得（worldManager から）
    glm::vec3 playerPos = worldManager->getPlayerPosition();
    
    // 一定距離（3-5単位）を保つ
    float distToPlayer = glm::distance(position, playerPos);
    float desiredDistance = 3.0f;
    
    if (distToPlayer > desiredDistance + 1.0f) {
        // プレイヤーについていく
        targetPosition = playerPos;
        moveTowardsTarget(deltaTime);
    } else if (distToPlayer < desiredDistance - 1.0f) {
        // 距離を取る
        glm::vec3 away = glm::normalize(position - playerPos);
        position += away * moveSpeed * deltaTime;
    }
    // それ以外は待機
    
    // プレイヤーが敵と戦闘中なら参加
    if (worldManager->isPlayerInCombat()) {
        auto playerEnemy = worldManager->getPlayerCombatTarget();
        if (playerEnemy && playerEnemy != combatTarget) {
            enterCombat(playerEnemy);
            setAIState(AIState::COMBAT);
        }
    }
    break;
}
```

---

### フェーズ2: 高度な戦闘AI

#### スペル選択の強化

**ファイル**: `game/npc.cpp` に追加

```cpp
// 現在の実装（スケルトン）を改善
uint32_t NPC::selectSpellForCombat() {
    if (status.knownSpells.empty()) {
        return 0;  // スペルなし → 通常攻撃
    }
    
    // HP低いならヒール優先
    if (status.currentHealth < status.maxHealth * 0.3f) {
        uint32_t healSpell = findSpellByType(SpellType::RESTORATION);
        if (healSpell != 0 && canCastSpell(healSpell)) {
            return healSpell;  // ヒールスペル
        }
    }
    
    // マナ低いなら回復スペル
    if (status.currentMana < status.maxMana * 0.2f) {
        uint32_t manaSpell = findSpellByType(SpellType::MANA_RESTORE);
        if (manaSpell != 0 && canCastSpell(manaSpell)) {
            return manaSpell;
        }
    }
    
    // バッファがない場合、バッファ系スペル
    if (!hasBuffActive("Fortify")) {
        uint32_t buffSpell = findSpellByType(SpellType::FORTIFY);
        if (buffSpell != 0 && canCastSpell(buffSpell)) {
            return buffSpell;
        }
    }
    
    // 敵に対してダメージスペル
    uint32_t damageSpell = findMostEffectiveSpell(combatTarget);
    if (damageSpell != 0 && canCastSpell(damageSpell)) {
        return damageSpell;
    }
    
    // スペルなし → 通常攻撃
    return 0;
}

// ヘルパー関数
uint32_t NPC::findSpellByType(SpellType type) {
    for (uint32_t spellId : status.knownSpells) {
        // SpellManager から spellId の属性を取得
        if (spellManager->getSpellType(spellId) == type) {
            return spellId;
        }
    }
    return 0;
}

uint32_t NPC::findMostEffectiveSpell(std::shared_ptr<NPC> enemy) {
    if (!enemy) return 0;
    
    uint32_t bestSpell = 0;
    float bestScore = 0.0f;
    
    for (uint32_t spellId : status.knownSpells) {
        if (!canCastSpell(spellId)) continue;
        
        float damage = spellManager->getSpellDamage(spellId);
        float enemyResistance = enemy->status.getAttributeBonus("Willpower") * 10.0f;
        float effectiveDamage = damage - enemyResistance;
        
        if (effectiveDamage > bestScore) {
            bestScore = effectiveDamage;
            bestSpell = spellId;
        }
    }
    
    return bestSpell;
}

bool NPC::canCastSpell(uint32_t spellId) {
    // マナチェック
    float manaCost = spellManager->getSpellManaCost(spellId);
    if (status.currentMana < manaCost) return false;
    
    // 詠唱間隔チェック
    float castSpeed = spellManager->getSpellCastTime(spellId);
    if (lastSpellCastTime < castSpeed) return false;
    
    return true;
}

bool NPC::hasBuffActive(const std::string& buffName) {
    // バッファ状態を確認（実装は後で）
    return false;  // TODO: バッファシステム統合
}
```

#### HP管理AI

```cpp
// CombatManager::update() に追加
void CombatManager::updateAIDecisions(CombatInstance& combat, float deltaTime) {
    if (!combat.attacker || !combat.defender) return;
    
    auto attacker = combat.attacker;
    float hpPercent = attacker->status.currentHealth / attacker->status.maxHealth;
    
    // HP 30% 以下 → 逃げるか検討
    if (hpPercent < 0.3f && attacker->shouldFlee()) {
        attacker->setAIState(AIState::FLEEING);
        endCombat(combat.defender->npcId);
        return;
    }
    
    // HP 50% 以下 → ヒール優先
    if (hpPercent < 0.5f) {
        uint32_t healSpell = attacker->findSpellByType(SpellType::RESTORATION);
        if (healSpell != 0) {
            // ヒール発動（通常攻撃スキップ）
            return;
        }
    }
    
    // マナ 20% 以下 → マナ回復待機
    float manaPercent = attacker->status.currentMana / attacker->status.maxMana;
    if (manaPercent < 0.2f) {
        // スペルキャスト一時停止
        return;
    }
}

bool NPC::shouldFlee() {
    // 逃げるべきか判定
    // 敵の方が強い場合や、HP がかなり低い場合など
    float strengthDiff = 
        status.getAttributeBonus("Strength") - 
        combatTarget->status.getAttributeBonus("Strength");
    
    float hpPercent = status.currentHealth / status.maxHealth;
    
    return hpPercent < 0.2f || strengthDiff < -20.0f;
}
```

---

### フェーズ3: ステートマシン

**新規ファイル**: `game/ai_state_machine.h`

```cpp
#pragma once
#include "npc.h"
#include <functional>

enum class AIEvent {
    PLAYER_APPROACHED,    // プレイヤーが接近
    TOOK_DAMAGE,         // ダメージを受けた
    ENEMY_DETECTED,      // 敵を検出
    HEALTH_LOW,          // HP低下
    MANA_LOW,            // マナ不足
    TARGET_DEFEATED,     // 敵撃破
    QUEST_OBJECTIVE,     // クエスト達成
    CONVERSATION_START,  // 会話開始
    CONVERSATION_END,    // 会話終了
    CONFUSED,            // 混乱状態
    PANICKED             // パニック状態
};

class AIStateMachine {
public:
    void addTransition(
        AIState from, 
        AIEvent trigger, 
        AIState to, 
        std::function<bool()> condition = nullptr
    );
    
    void processEvent(std::shared_ptr<NPC> npc, AIEvent event);
    
    void update(std::shared_ptr<NPC> npc, float deltaTime);
    
private:
    struct Transition {
        AIState from, to;
        AIEvent trigger;
        std::function<bool()> condition;
    };
    
    std::vector<Transition> transitions;
};
```

**使用例**:
```cpp
// game/npc_manager.cpp での初期化
AIStateMachine stateMachine;

// IDLE → COMBAT (敵検出)
stateMachine.addTransition(
    AIState::IDLE,
    AIEvent::ENEMY_DETECTED,
    AIState::COMBAT
);

// COMBAT → FLEEING (HP低い)
stateMachine.addTransition(
    AIState::COMBAT,
    AIEvent::HEALTH_LOW,
    AIState::FLEEING,
    [](){ return npc->status.currentHealth < npc->status.maxHealth * 0.2f; }
);

// COMBAT → IDLE (敵撃破)
stateMachine.addTransition(
    AIState::COMBAT,
    AIEvent::TARGET_DEFEATED,
    AIState::IDLE
);
```

---

### フェーズ4: 会話システム

**新規ファイル**: `game/dialogue_system.h`

```cpp
#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include "quest_manager.h"

struct DialogueNode {
    std::string nodeId;
    std::string npcSpeak;           // NPC の台詞
    std::vector<std::string> playerOptions;  // プレイヤー選択肢
    std::unordered_map<int, std::string> responses; // 返答
};

struct DialogueTree {
    std::string treeId;
    std::unordered_map<std::string, DialogueNode> nodes;
    std::string rootNodeId;
};

class DialogueSystem {
public:
    void initializeDialogues();
    const DialogueNode* getNode(const std::string& nodeId);
    void selectOption(int optionIndex);
    std::string getNPCResponse(int optionIndex);
    
private:
    std::unordered_map<uint32_t, DialogueTree> dialogueTrees;
};
```

**実装例**:
```cpp
void DialogueSystem::initializeDialogues() {
    // Companion クエスト NPC の会話
    DialogueTree companionDialogue;
    companionDialogue.rootNodeId = "greeting";
    
    DialogueNode greetingNode;
    greetingNode.npcSpeak = "こんにちは、どうしたのですか？";
    greetingNode.playerOptions = {
        "クエストについて教えてください",
        "別の話があります",
        "さようなら"
    };
    greetingNode.responses = {
        {0, "私は村の外で何か大事なものを失いました。探すのを手伝ってくれませんか？"},
        {1, "何かお役に立てることがあれば..."},
        {2, "さようなら。またね"}
    };
    
    companionDialogue.nodes["greeting"] = greetingNode;
    // ... more nodes ...
    
    dialogueTrees[npcId] = companionDialogue;
}
```

---

## 実装スケジュール

### フェーズ1: 基本AI行動（推奨時間: 1-2時間）
- [ ] WANDER 実装（30分）
- [ ] PATROL 実装（30分）
- [ ] FOLLOW_PLAYER 実装（30分）
- [ ] テスト & デバッグ（15分）

### フェーズ2: 高度な戦闘AI（推奨時間: 2時間）
- [ ] スペル選択強化（45分）
- [ ] HP管理AI（45分）
- [ ] 戦闘戦略AI（30分）

### フェーズ3: ステートマシン（推奨時間: 1.5時間）
- [ ] AIStateMachine 実装（1時間）
- [ ] 状態遷移ロジック（30分）

### フェーズ4: 会話システム（推奨時間: 2時間）
- [ ] DialogueSystem 実装（1時間）
- [ ] ダイアログツリー作成（1時間）

**合計予定時間**: 6.5-7時間

---

## 既存AIの保持

✅ **以下の既存機能は変更なし**
- ダメージ計算ロジック
- 敵検出システム（findNearestEnemy）
- ステータス管理
- 戦闘開始/終了
- SpellManager 統合

✅ **以下は拡張（既存機能を保持）**
- selectSpellForCombat() → より高度な判定に
- update() → 新しいステート処理を追加
- AIState enum → 新しい状態を追加

---

## ビルド対応

### CMakeLists.txt に追加予定
```cmake
# Game Systems
game/npc.cpp           # 既存（拡張）
game/npc_manager.cpp   # 既存（拡張）
game/combat_manager.cpp # 既存（拡張）
game/ai_state_machine.cpp  # 新規（フェーズ3）
game/dialogue_system.cpp    # 新規（フェーズ4）
```

### 翻訳キー（localization_manager.cpp）
- AI状態ログメッセージ（日本語化）
- 会話テキスト（日本語化）
- スペル名（日本語化）

---

## テスト戦略

### ユニットテスト
- [ ] WANDER 目標生成テスト
- [ ] PATROL ウェイポイント遷移テスト
- [ ] スペル選択AIテスト
- [ ] ステート遷移テスト

### 統合テスト
- [ ] 複数NPC間の相互作用
- [ ] プレイヤーとの戦闘
- [ ] 会話ツリー完全性

### デバイステスト
- [ ] 実Android デバイスで動作確認
- [ ] パフォーマンス測定（複数NPC）
- [ ] メモリリーク検査

---

## 次のステップ

1. **フェーズ1開始**: 基本AI行動（WANDER/PATROL/FOLLOW）
2. **実装完了後**: テスト & デバイス検証
3. **フェーズ2以降**: 戦闘AI強化

---

**Status**: 準備完了  
**Next**: フェーズ1実装開始？
