#include "quest_stage_manager.h"
#include "../game/npc_manager.h"
#include "../world/world_manager.h"
#include <algorithm>

// ============================================================================
// StageConditionEvaluator
// ============================================================================

bool StageConditionEvaluator::evaluate(const QuestCondition& cond,
                                        uint32_t questFormID,
                                        int32_t currentStage,
                                        QuestManager* questMgr,
                                        NpcManager* npcMgr,
                                        WorldManager* worldMgr) {
    switch (cond.functionIndex) {
        case 29: // GetQuestRunning
            return evalGetQuestRunning(cond, questMgr);
        case 30: // GetStage
            return evalGetStage(cond, questFormID, questMgr);
        case 31: // GetStageDone
            return evalGetStageDone(cond, questFormID, questMgr);
        case 23: // GetDead
            return evalGetDead(cond, npcMgr);
        case 24: // GetItemCount
            return evalGetItemCount(cond);
        case 39: // GetInCell
            return evalGetInCell(cond, worldMgr);
        case 0:  // GetDistance
            return evalGetDistance(cond, worldMgr);
        case 78: // GetQuestCompleted
            return evalGetQuestCompleted(cond, questMgr);
        default:
            LOGD("Unhandled condition function: %u (%s)",
                 cond.functionIndex, QuestCondition::getFunctionName(cond.functionIndex));
            return false;
    }
}

bool StageConditionEvaluator::evaluateAll(const std::vector<QuestCondition>& conditions,
                                            uint32_t questFormID,
                                            int32_t currentStage,
                                            QuestManager* questMgr,
                                            NpcManager* npcMgr,
                                            WorldManager* worldMgr) {
    if (conditions.empty()) return true;

    bool hasOrFlag = false;
    bool anyTrue = false;

    for (const auto& cond : conditions) {
        bool result = evaluate(cond, questFormID, currentStage, questMgr, npcMgr, worldMgr);

        if (cond.flags & 0x01) {
            // OR condition
            hasOrFlag = true;
            anyTrue = anyTrue || result;
        } else {
            // AND condition - if any AND condition fails, whole thing fails
            if (!result) return false;
        }
    }

    return hasOrFlag ? anyTrue : true;
}

bool StageConditionEvaluator::evalGetStage(const QuestCondition& cond,
                                            uint32_t questFormID,
                                            QuestManager* questMgr) {
    if (!questMgr) return false;
    auto quest = questMgr->getQuest(questFormID);
    if (!quest) return false;

    // Compare current stage with condition value
    int32_t targetStage = static_cast<int32_t>(cond.comparisonValue);
    // We need to get the current stage from the quest manager
    // For now, check if the quest is in the expected state
    switch (cond.comparisonOp) {
        case 0: return quest->state == QuestState::IN_PROGRESS; // Equal
        case 1: return quest->state != QuestState::IN_PROGRESS; // Not equal
        default: return false;
    }
}

bool StageConditionEvaluator::evalGetStageDone(const QuestCondition& cond,
                                                uint32_t questFormID,
                                                QuestManager* questMgr) {
    if (!questMgr) return false;
    auto quest = questMgr->getQuest(questFormID);
    if (!quest) return false;

    // Check if the quest has reached a certain stage
    // This is a simplified check - in full implementation, we'd track stage history
    return quest->state == QuestState::COMPLETED;
}

bool StageConditionEvaluator::evalGetQuestRunning(const QuestCondition& cond,
                                                   QuestManager* questMgr) {
    if (!questMgr) return false;
    uint32_t targetQuest = cond.param1;
    return questMgr->isQuestActive(targetQuest);
}

bool StageConditionEvaluator::evalGetQuestCompleted(const QuestCondition& cond,
                                                     QuestManager* questMgr) {
    if (!questMgr) return false;
    uint32_t targetQuest = cond.param1;
    return questMgr->isQuestCompleted(targetQuest);
}

bool StageConditionEvaluator::evalGetDead(const QuestCondition& cond,
                                           NpcManager* npcMgr) {
    if (!npcMgr) return false;
    uint32_t npcFormID = cond.param1;
    auto npc = npcMgr->getNPC(npcFormID);
    if (!npc) return false;
    return npc->status.currentHealth <= 0;
}

bool StageConditionEvaluator::evalGetItemCount(const QuestCondition& cond) {
    // TODO: Check player inventory for item count
    // Requires InventoryManager integration
    return false;
}

bool StageConditionEvaluator::evalGetInCell(const QuestCondition& cond,
                                             WorldManager* worldMgr) {
    if (!worldMgr) return false;
    // Check if player is in the specified cell
    // This is a simplified check
    return false;
}

bool StageConditionEvaluator::evalGetDistance(const QuestCondition& cond,
                                               WorldManager* worldMgr) {
    if (!worldMgr) return false;
    // Check distance between player and target
    // This is a simplified check
    return false;
}

// ============================================================================
// QuestStageManager
// ============================================================================

QuestStageManager::QuestStageManager() {
    LOGD("QuestStageManager created");
}

QuestStageManager::~QuestStageManager() {
    cleanup();
    LOGD("QuestStageManager destroyed");
}

bool QuestStageManager::initialize(QuestManager* questMgr,
                                    oblivion::script::ScriptManager* scriptMgr,
                                    NpcManager* npcMgr,
                                    WorldManager* worldMgr) {
    if (!questMgr) {
        LOGE("Cannot initialize QuestStageManager with null QuestManager");
        return false;
    }

    questManager_ = questMgr;
    scriptManager_ = scriptMgr;
    npcManager_ = npcMgr;
    worldManager_ = worldMgr;

    LOGI("QuestStageManager initialized");
    return true;
}

void QuestStageManager::cleanup() {
    questRecords_.clear();
    currentStages_.clear();
    questManager_ = nullptr;
    scriptManager_ = nullptr;
    npcManager_ = nullptr;
    worldManager_ = nullptr;
    LOGD("QuestStageManager cleaned up");
}

void QuestStageManager::update(float deltaTime) {
    // Evaluate stage conditions for all active quests
    for (auto& [formID, stage] : currentStages_) {
        evaluateStageConditions(formID);
    }
}

void QuestStageManager::registerQuest(const QuestRecord& record) {
    questRecords_[record.formID] = record;

    // Initialize stage to 0 if not already set
    if (currentStages_.find(record.formID) == currentStages_.end()) {
        currentStages_[record.formID] = 0;
    }

    LOGD("Registered quest: FormID=0x%08X, EDID=%s, Stages=%zu",
         record.formID, record.editorID.c_str(), record.stages.size());
}

bool QuestStageManager::advanceStage(uint32_t questFormID, int32_t newStage) {
    auto it = currentStages_.find(questFormID);
    if (it == currentStages_.end()) {
        LOGW("Quest 0x%08X not registered for stage management", questFormID);
        return false;
    }

    int32_t oldStage = it->second;
    if (newStage <= oldStage) {
        LOGD("Quest 0x%08X: Cannot advance backwards from %d to %d",
             questFormID, oldStage, newStage);
        return false;
    }

    return setStage(questFormID, newStage);
}

bool QuestStageManager::setStage(uint32_t questFormID, int32_t stage) {
    auto it = currentStages_.find(questFormID);
    if (it == currentStages_.end()) {
        LOGW("Quest 0x%08X not registered for stage management", questFormID);
        return false;
    }

    int32_t oldStage = it->second;
    it->second = stage;

    LOGI("Quest 0x%08X: Stage changed from %d to %d", questFormID, oldStage, stage);

    // Create transition event
    StageTransition transition;
    transition.questFormID = questFormID;
    transition.fromStage = oldStage;
    transition.toStage = stage;

    // Check if this is a completion or failure stage
    auto recordIt = questRecords_.find(questFormID);
    if (recordIt != questRecords_.end()) {
        const auto* stageEntry = recordIt->second.findStage(stage);
        if (stageEntry) {
            transition.logText = stageEntry->logText;
            transition.isCompletion = stageEntry->isCompletionStage();
            transition.isFailure = stageEntry->isFailStage();
        }
    }

    onStageTransition(transition);
    return true;
}

int32_t QuestStageManager::getCurrentStage(uint32_t questFormID) const {
    auto it = currentStages_.find(questFormID);
    if (it == currentStages_.end()) return -1;
    return it->second;
}

bool QuestStageManager::evaluateStageConditions(uint32_t questFormID) {
    auto recordIt = questRecords_.find(questFormID);
    if (recordIt == questRecords_.end()) return false;

    const auto& record = recordIt->second;
    int32_t currentStage = currentStages_[questFormID];

    // Check all stages that are ahead of current stage
    for (const auto& stage : record.stages) {
        if (stage.stageIndex <= currentStage) continue;

        // Evaluate conditions for this stage
        if (StageConditionEvaluator::evaluateAll(
                stage.conditions, questFormID, currentStage,
                questManager_, npcManager_, worldManager_)) {
            // Conditions met - advance to this stage
            LOGD("Quest 0x%08X: Stage %d conditions met, advancing",
                 questFormID, stage.stageIndex);
            setStage(questFormID, stage.stageIndex);
            return true;
        }
    }

    return false;
}

bool QuestStageManager::tryAdvanceToStage(uint32_t questFormID, int32_t targetStage) {
    auto recordIt = questRecords_.find(questFormID);
    if (recordIt == questRecords_.end()) return false;

    const auto& record = recordIt->second;
    const auto* stageEntry = record.findStage(targetStage);
    if (!stageEntry) {
        LOGW("Quest 0x%08X: Stage %d not found", questFormID, targetStage);
        return false;
    }

    // Evaluate conditions for the target stage
    if (StageConditionEvaluator::evaluateAll(
            stageEntry->conditions, questFormID, currentStages_[questFormID],
            questManager_, npcManager_, worldManager_)) {
        return setStage(questFormID, targetStage);
    }

    return false;
}

const QuestRecord* QuestStageManager::getQuestRecord(uint32_t questFormID) const {
    auto it = questRecords_.find(questFormID);
    if (it == questRecords_.end()) return nullptr;
    return &it->second;
}

bool QuestStageManager::isQuestRegistered(uint32_t questFormID) const {
    return questRecords_.find(questFormID) != questRecords_.end();
}

std::unordered_map<uint32_t, int32_t> QuestStageManager::exportStageStates() const {
    return currentStages_;
}

void QuestStageManager::importStageStates(const std::unordered_map<uint32_t, int32_t>& states) {
    for (const auto& [formID, stage] : states) {
        currentStages_[formID] = stage;
    }
    LOGI("Imported %zu quest stage states", states.size());
}

void QuestStageManager::onStageTransition(const StageTransition& transition) {
    // Trigger scripts for the new stage
    triggerStageScripts(transition.questFormID, transition.toStage);

    // Check for quest completion
    if (transition.isCompletion) {
        checkCompletionStage(transition.questFormID, transition.toStage);
    }

    // Check for quest failure
    if (transition.isFailure) {
        checkFailStage(transition.questFormID, transition.toStage);
    }

    // Notify callback
    if (transitionCallback_) {
        transitionCallback_(transition);
    }
}

void QuestStageManager::triggerStageScripts(uint32_t questFormID, int32_t stageIndex) {
    if (!scriptManager_) return;

    // In Oblivion, quest stages can trigger scripts
    // The script is typically associated with the quest record
    auto recordIt = questRecords_.find(questFormID);
    if (recordIt == questRecords_.end()) return;

    // Look for script references in the quest record
    // For now, we log the stage change
    LOGD("Quest 0x%08X: Stage %d script trigger (ScriptManager integration pending)",
         questFormID, stageIndex);
}

bool QuestStageManager::checkCompletionStage(uint32_t questFormID, int32_t stageIndex) {
    if (!questManager_) return false;

    LOGI("Quest 0x%08X: Completion stage %d reached", questFormID, stageIndex);
    return questManager_->completeQuest(questFormID);
}

bool QuestStageManager::checkFailStage(uint32_t questFormID, int32_t stageIndex) {
    if (!questManager_) return false;

    LOGI("Quest 0x%08X: Failure stage %d reached", questFormID, stageIndex);
    return questManager_->failQuest(questFormID);
}
