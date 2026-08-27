#pragma once

#include "quest_record.h"
#include "quest_stage_manager.h"
#include "quest_objective_tracker.h"
#include "quest_rewards.h"
#include "../game/quest_manager.h"
#include "../engine/imperial_weave.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <string>
#include <android/log.h>

#define LOG_TAG "QuestFlowCtrl"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
#define LOGD(...) do {} while(0)
#endif
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ============================================================================
// Quest Flow Controller
// Phase 39: Unified quest lifecycle management
// ============================================================================

// Forward declarations
class PlayerController;
class InventoryManager;
class NpcManager;
class WorldManager;

namespace oblivion { namespace script { class ScriptManager; } }

// ============================================================================
// Quest Flow State
// ============================================================================
enum class QuestFlowState : uint8_t {
    INACTIVE    = 0,  // Not started
    REGISTERED  = 1,  // Registered from ESM but not active
    ACTIVE      = 2,  // Player has accepted/is doing the quest
    STAGE_TRANS = 3,  // In stage transition
    COMPLETED   = 4,  // Successfully completed
    FAILED      = 5,  // Failed
    REWARDS     = 6   // Rewards being granted
};

// ============================================================================
// Quest Flow Entry
// ============================================================================
struct QuestFlowEntry {
    uint32_t questFormID = 0;
    QuestRecord record;
    QuestFlowState state = QuestFlowState::INACTIVE;
    uint32_t activationTime = 0;
    uint32_t completionTime = 0;

    // Stage info
    int32_t currentStage = 0;
    int32_t targetStage = -1;

    // Objective tracking
    bool allObjectivesComplete = false;

    // Reward tracking
    bool rewardsGranted = false;
};

// ============================================================================
// Quest Flow Callbacks
// ============================================================================
using QuestFlowCallback = std::function<void(uint32_t questFormID, QuestFlowState newState)>;

// ============================================================================
// Quest Flow Controller
// ============================================================================
class QuestFlowController {
public:
    QuestFlowController();
    ~QuestFlowController();

    // Initialize with all game systems
    bool initialize(QuestManager* questMgr,
                    weave::EventBus* eventBus,
                    oblivion::script::ScriptManager* scriptMgr,
                    Player* player,
                    PlayerController* playerCtrl,
                    InventoryManager* invMgr,
                    NpcManager* npcMgr,
                    WorldManager* worldMgr);

    void cleanup();
    void update(float deltaTime);

    // Quest registration (from ESM data)
    void registerQuest(const QuestRecord& record);
    void registerQuests(const std::vector<QuestRecord>& records);

    // Quest lifecycle
    bool activateQuest(uint32_t questFormID);
    bool completeQuest(uint32_t questFormID);
    bool failQuest(uint32_t questFormID);

    // Stage management
    bool advanceStage(uint32_t questFormID, int32_t newStage);
    bool setStage(uint32_t questFormID, int32_t stage);
    int32_t getCurrentStage(uint32_t questFormID) const;

    // Objective management
    bool activateObjective(uint32_t questFormID, uint32_t objectiveIndex);
    bool completeObjective(uint32_t questFormID, uint32_t objectiveIndex);

    // Reward management
    bool grantRewards(uint32_t questFormID);

    // Query
    const QuestFlowEntry* getQuestFlow(uint32_t questFormID) const;
    QuestFlowState getQuestState(uint32_t questFormID) const;
    std::vector<uint32_t> getActiveQuests() const;
    std::vector<uint32_t> getCompletedQuests() const;
    std::vector<uint32_t> getFailedQuests() const;
    size_t getRegisteredQuestCount() const { return quests_.size(); }

    // Sub-system access
    QuestStageManager* getStageManager() { return &stageManager_; }
    QuestObjectiveTracker* getObjectiveTracker() { return &objectiveTracker_; }
    QuestRewardManager* getRewardManager() { return &rewardManager_; }

    // Callbacks
    void setQuestFlowCallback(QuestFlowCallback callback) {
        flowCallback_ = std::move(callback);
    }

    // Save/Load support
    bool saveQuestStates(std::vector<uint8_t>& outData) const;
    bool loadQuestStates(const uint8_t* data, size_t size);

private:
    // Sub-systems
    QuestStageManager stageManager_;
    QuestObjectiveTracker objectiveTracker_;
    QuestRewardManager rewardManager_;

    // Quest flow entries (questFormID -> entry)
    std::unordered_map<uint32_t, QuestFlowEntry> quests_;

    // Game system pointers
    QuestManager* questManager_ = nullptr;
    weave::EventBus* eventBus_ = nullptr;
    oblivion::script::ScriptManager* scriptManager_ = nullptr;
    Player* player_ = nullptr;
    PlayerController* playerController_ = nullptr;
    InventoryManager* inventoryManager_ = nullptr;
    NpcManager* npcManager_ = nullptr;
    WorldManager* worldManager_ = nullptr;

    // Callback
    QuestFlowCallback flowCallback_;

    // Internal state management
    void changeState(uint32_t questFormID, QuestFlowState newState);
    void onStageTransition(const StageTransition& transition);
    void onObjectiveCompleted(uint32_t questFormID, uint32_t objectiveIndex, bool success);
    void onRewardsGranted(uint32_t questFormID, const QuestRewardPackage& rewards);

    // Quest chain handling
    void checkQuestChain(uint32_t completedQuestFormID);

    // Helper to create default rewards from quest record
    QuestRewardPackage createDefaultRewards(const QuestRecord& record) const;
};
