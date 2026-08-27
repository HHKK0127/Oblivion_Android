#pragma once

#include "quest_record.h"
#include "../game/quest_manager.h"
#include "../script/script_manager.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>
#include <android/log.h>

#define LOG_TAG "QuestStageMgr"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
#define LOGD(...) do {} while(0)
#endif
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ============================================================================
// Quest Stage Manager
// Phase 39: Manages quest stage transitions, condition evaluation,
//           and script integration
// ============================================================================

// Forward declarations
class QuestManager;
namespace oblivion { namespace script { class ScriptManager; } }

// ============================================================================
// Stage Transition Event
// ============================================================================
struct StageTransition {
    uint32_t questFormID = 0;
    int32_t fromStage = -1;
    int32_t toStage = 0;
    bool isCompletion = false;
    bool isFailure = false;
    std::string logText;
};

// ============================================================================
// Stage Condition Evaluator
// ============================================================================
class StageConditionEvaluator {
public:
    // Evaluate a single condition against current game state
    static bool evaluate(const QuestCondition& cond,
                         uint32_t questFormID,
                         int32_t currentStage,
                         class QuestManager* questMgr,
                         class NpcManager* npcMgr,
                         class WorldManager* worldMgr);

    // Evaluate all conditions for a stage (AND logic, unless OR flag set)
    static bool evaluateAll(const std::vector<QuestCondition>& conditions,
                            uint32_t questFormID,
                            int32_t currentStage,
                            class QuestManager* questMgr,
                            class NpcManager* npcMgr,
                            class WorldManager* worldMgr);

private:
    // Individual condition evaluators
    static bool evalGetStage(const QuestCondition& cond, uint32_t questFormID,
                             class QuestManager* questMgr);
    static bool evalGetStageDone(const QuestCondition& cond, uint32_t questFormID,
                                 class QuestManager* questMgr);
    static bool evalGetQuestRunning(const QuestCondition& cond,
                                    class QuestManager* questMgr);
    static bool evalGetQuestCompleted(const QuestCondition& cond,
                                      class QuestManager* questMgr);
    static bool evalGetDead(const QuestCondition& cond, class NpcManager* npcMgr);
    static bool evalGetItemCount(const QuestCondition& cond);
    static bool evalGetInCell(const QuestCondition& cond, class WorldManager* worldMgr);
    static bool evalGetDistance(const QuestCondition& cond, class WorldManager* worldMgr);
};

// ============================================================================
// Quest Stage Manager
// ============================================================================
class QuestStageManager {
public:
    using StageTransitionCallback = std::function<void(const StageTransition&)>;

    QuestStageManager();
    ~QuestStageManager();

    // Initialize with game systems
    bool initialize(QuestManager* questMgr,
                    oblivion::script::ScriptManager* scriptMgr,
                    NpcManager* npcMgr,
                    WorldManager* worldMgr);

    void cleanup();
    void update(float deltaTime);

    // Register a quest record for stage management
    void registerQuest(const QuestRecord& record);

    // Stage operations
    bool advanceStage(uint32_t questFormID, int32_t newStage);
    bool setStage(uint32_t questFormID, int32_t stage);
    int32_t getCurrentStage(uint32_t questFormID) const;

    // Stage evaluation (check if conditions are met to advance)
    bool evaluateStageConditions(uint32_t questFormID);
    bool tryAdvanceToStage(uint32_t questFormID, int32_t targetStage);

    // Get quest record
    const QuestRecord* getQuestRecord(uint32_t questFormID) const;

    // Callbacks
    void setStageTransitionCallback(StageTransitionCallback callback) {
        transitionCallback_ = std::move(callback);
    }

    // Query
    size_t getRegisteredQuestCount() const { return questRecords_.size(); }
    bool isQuestRegistered(uint32_t questFormID) const;

    // Save/Load support
    std::unordered_map<uint32_t, int32_t> exportStageStates() const;
    void importStageStates(const std::unordered_map<uint32_t, int32_t>& states);

private:
    // Quest records (FormID -> QuestRecord)
    std::unordered_map<uint32_t, QuestRecord> questRecords_;

    // Current stage per quest (FormID -> stage index)
    std::unordered_map<uint32_t, int32_t> currentStages_;

    // Game system pointers
    QuestManager* questManager_ = nullptr;
    oblivion::script::ScriptManager* scriptManager_ = nullptr;
    NpcManager* npcManager_ = nullptr;
    WorldManager* worldManager_ = nullptr;

    // Callback
    StageTransitionCallback transitionCallback_;

    // Internal
    void onStageTransition(const StageTransition& transition);
    void triggerStageScripts(uint32_t questFormID, int32_t stageIndex);
    bool checkCompletionStage(uint32_t questFormID, int32_t stageIndex);
    bool checkFailStage(uint32_t questFormID, int32_t stageIndex);
};
