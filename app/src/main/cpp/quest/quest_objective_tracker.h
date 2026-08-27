#pragma once

#include "quest_record.h"
#include "../game/quest_manager.h"
#include "../engine/imperial_weave.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>
#include <android/log.h>

#define LOG_TAG "QuestObjTracker"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
#define LOGD(...) do {} while(0)
#endif
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ============================================================================
// Quest Objective Tracker
// Phase 39: Tracks objective progress via EventBus integration
// ============================================================================

// Forward declarations
class CombatManager;
class WorldManager;
class NpcManager;

// ============================================================================
// Objective Progress State
// ============================================================================
struct ObjectiveProgress {
    uint32_t questFormID = 0;
    uint32_t objectiveIndex = 0;
    ObjectiveType type = ObjectiveType::NONE;
    uint32_t targetFormID = 0;
    bool isActive = false;
    bool isCompleted = false;
    bool isOptional = false;
    uint32_t currentCount = 0;
    uint32_t requiredCount = 1;
    std::string description;

    float completionPercent() const {
        if (requiredCount == 0) return 100.0f;
        return (static_cast<float>(currentCount) / static_cast<float>(requiredCount)) * 100.0f;
    }
};

// ============================================================================
// Objective Completion Callback
// ============================================================================
using ObjectiveCompletionCallback = std::function<void(uint32_t questFormID,
                                                        uint32_t objectiveIndex,
                                                        bool success)>;

// ============================================================================
// Quest Objective Tracker
// ============================================================================
class QuestObjectiveTracker {
public:
    QuestObjectiveTracker();
    ~QuestObjectiveTracker();

    // Initialize with game systems
    bool initialize(QuestManager* questMgr,
                    weave::EventBus* eventBus,
                    CombatManager* combatMgr,
                    WorldManager* worldMgr,
                    NpcManager* npcMgr);

    void cleanup();
    void update(float deltaTime);

    // Register objectives from a quest record
    void registerQuestObjectives(const QuestRecord& record);

    // Objective operations
    bool activateObjective(uint32_t questFormID, uint32_t objectiveIndex);
    bool completeObjective(uint32_t questFormID, uint32_t objectiveIndex);
    bool updateObjectiveProgress(uint32_t questFormID, uint32_t objectiveIndex,
                                  uint32_t progress);

    // Query
    const ObjectiveProgress* getObjectiveProgress(uint32_t questFormID,
                                                   uint32_t objectiveIndex) const;
    std::vector<ObjectiveProgress*> getActiveObjectives(uint32_t questFormID);
    std::vector<ObjectiveProgress*> getAllObjectives(uint32_t questFormID);
    bool areAllRequiredObjectivesComplete(uint32_t questFormID) const;

    // Callbacks
    void setObjectiveCompletionCallback(ObjectiveCompletionCallback callback) {
        completionCallback_ = std::move(callback);
    }

    // EventBus event handlers
    void onEnemyKilled(uint32_t enemyFormID);
    void onItemFound(uint32_t itemFormID);
    void onLocationDiscovered(uint32_t locationFormID);
    void onNpcTalkedTo(uint32_t npcFormID);
    void onCellEntered(uint32_t cellFormID);

    // Save/Load support
    std::vector<ObjectiveProgress> exportObjectiveStates() const;
    void importObjectiveStates(const std::vector<ObjectiveProgress>& states);

private:
    // Objective storage: questFormID -> (objectiveIndex -> progress)
    std::unordered_map<uint32_t, std::unordered_map<uint32_t, ObjectiveProgress>> objectives_;

    // Game system pointers
    QuestManager* questManager_ = nullptr;
    weave::EventBus* eventBus_ = nullptr;
    CombatManager* combatManager_ = nullptr;
    WorldManager* worldManager_ = nullptr;
    NpcManager* npcManager_ = nullptr;

    // Callback
    ObjectiveCompletionCallback completionCallback_;

    // EventBus subscriptions
    void subscribeToEvents();
    void unsubscribeFromEvents();

    // Internal objective completion logic
    void onObjectiveCompleted(uint32_t questFormID, uint32_t objectiveIndex);
    void checkQuestCompletion(uint32_t questFormID);

    // Find objective by target FormID
    ObjectiveProgress* findObjectiveByTarget(uint32_t targetFormID, ObjectiveType type);
};
