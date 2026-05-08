#pragma once

#include "npc.h"
#include <functional>
#include <vector>
#include <unordered_map>
#include <memory>
#include <android/log.h>

#define LOG_TAG "AIStateMachine"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ============================================================
// AIEvent: Event types that trigger state transitions
// ============================================================
enum class AIEvent {
    PLAYER_APPROACHED,      // プレイヤーが接近
    TOOK_DAMAGE,           // ダメージを受けた
    ENEMY_DETECTED,        // 敵を検出
    HEALTH_LOW,            // HP低下（30%以下）
    HEALTH_CRITICAL,       // HP危機的（20%以下）
    MANA_LOW,              // マナ不足（20%以下）
    TARGET_DEFEATED,       // 敵撃破
    QUEST_OBJECTIVE,       // クエスト達成
    CONVERSATION_START,    // 会話開始
    CONVERSATION_END,      // 会話終了
    CONFUSED,              // 混乱状態
    PANICKED,              // パニック状態
    TIMEOUT,               // タイムアウト（状態滞在時間超過）
    ALERT,                 // アラート状態
    ALLY_ATTACKED          // 味方が攻撃された
};

// ============================================================
// AITransition: State transition definition
// ============================================================
struct AITransition {
    AIState fromState;                                      // 遷移元の状態
    AIState toState;                                        // 遷移先の状態
    AIEvent trigger;                                        // 遷移トリガー
    std::function<bool(std::shared_ptr<NPC>)> condition;   // 遷移条件（オプション）
    float timeout = -1.0f;                                  // タイムアウト時間（秒）

    AITransition() = default;

    AITransition(AIState from, AIState to, AIEvent evt,
                 std::function<bool(std::shared_ptr<NPC>)> cond = nullptr,
                 float timeoutVal = -1.0f)
        : fromState(from), toState(to), trigger(evt),
          condition(cond), timeout(timeoutVal) {}
};

// ============================================================
// AIStateMachine: Manages NPC state transitions and events
// ============================================================
class AIStateMachine {
public:
    AIStateMachine();
    ~AIStateMachine();

    // Transition Management
    void addTransition(const AITransition& transition);
    void addTransition(AIState from, AIState to, AIEvent trigger,
                      std::function<bool(std::shared_ptr<NPC>)> condition = nullptr,
                      float timeout = -1.0f);

    // Event Processing
    void processEvent(std::shared_ptr<NPC> npc, AIEvent event);

    // Update (called each frame)
    void update(std::shared_ptr<NPC> npc, float deltaTime);

    // State Query
    AIState getCurrentState(std::shared_ptr<NPC> npc) const;
    float getStateTime(std::shared_ptr<NPC> npc) const;

    // Clear all transitions (for reset)
    void clearTransitions() { transitions.clear(); }

    // Initialize default transitions
    void initializeDefaultTransitions();

private:
    std::vector<AITransition> transitions;

    // State tracking per NPC (using NPC ID as key)
    struct StateInfo {
        AIState currentState = AIState::IDLE;
        float stateTime = 0.0f;
        uint32_t npcId = 0;
    };

    // Find state info for an NPC
    StateInfo* findOrCreateStateInfo(uint32_t npcId);

    // Check if transition is valid
    bool canTransition(std::shared_ptr<NPC> npc, const AITransition& trans) const;

    // All state tracking for all NPCs (key = npcId, value = StateInfo)
    // Using unordered_map instead of vector to avoid pointer invalidation on reallocation
    std::unordered_map<uint32_t, StateInfo> npcStates;
};
