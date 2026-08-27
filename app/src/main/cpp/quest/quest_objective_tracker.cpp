#include "quest_objective_tracker.h"
#include "../game/combat_manager.h"
#include "../world/world_manager.h"
#include "../game/npc_manager.h"
#include <algorithm>

// ============================================================================
// QuestObjectiveTracker
// ============================================================================

QuestObjectiveTracker::QuestObjectiveTracker() {
    LOGD("QuestObjectiveTracker created");
}

QuestObjectiveTracker::~QuestObjectiveTracker() {
    cleanup();
    LOGD("QuestObjectiveTracker destroyed");
}

bool QuestObjectiveTracker::initialize(QuestManager* questMgr,
                                        weave::EventBus* eventBus,
                                        CombatManager* combatMgr,
                                        WorldManager* worldMgr,
                                        NpcManager* npcMgr) {
    if (!questMgr) {
        LOGE("Cannot initialize QuestObjectiveTracker with null QuestManager");
        return false;
    }

    questManager_ = questMgr;
    eventBus_ = eventBus;
    combatManager_ = combatMgr;
    worldManager_ = worldMgr;
    npcManager_ = npcMgr;

    // Subscribe to EventBus events
    subscribeToEvents();

    LOGI("QuestObjectiveTracker initialized");
    return true;
}

void QuestObjectiveTracker::cleanup() {
    unsubscribeFromEvents();
    objectives_.clear();
    questManager_ = nullptr;
    eventBus_ = nullptr;
    combatManager_ = nullptr;
    worldManager_ = nullptr;
    npcManager_ = nullptr;
    LOGD("QuestObjectiveTracker cleaned up");
}

void QuestObjectiveTracker::update(float deltaTime) {
    // Periodic checks for location-based objectives
    // For now, location checks are event-driven via onCellEntered
}

void QuestObjectiveTracker::registerQuestObjectives(const QuestRecord& record) {
    auto& questObjectives = objectives_[record.formID];

    for (const auto& objEntry : record.objectives) {
        ObjectiveProgress progress;
        progress.questFormID = record.formID;
        progress.objectiveIndex = objEntry.objectiveIndex;
        progress.type = objEntry.type;
        progress.targetFormID = objEntry.targetFormID;
        progress.isOptional = objEntry.isOptional;
        progress.description = objEntry.description;
        progress.isActive = false;
        progress.isCompleted = false;
        progress.currentCount = 0;
        progress.requiredCount = 1;

        questObjectives[objEntry.objectiveIndex] = progress;
    }

    // Also register targets
    for (const auto& target : record.targets) {
        auto it = questObjectives.find(target.objectiveIndex);
        if (it != questObjectives.end()) {
            it->second.targetFormID = target.targetFormID;
        }
    }

    LOGD("Registered %zu objectives for quest 0x%08X",
         record.objectives.size(), record.formID);
}

bool QuestObjectiveTracker::activateObjective(uint32_t questFormID, uint32_t objectiveIndex) {
    auto questIt = objectives_.find(questFormID);
    if (questIt == objectives_.end()) {
        LOGW("Quest 0x%08X not registered for objective tracking", questFormID);
        return false;
    }

    auto objIt = questIt->second.find(objectiveIndex);
    if (objIt == questIt->second.end()) {
        LOGW("Objective %u not found in quest 0x%08X", objectiveIndex, questFormID);
        return false;
    }

    objIt->second.isActive = true;
    LOGD("Objective %u activated for quest 0x%08X", objectiveIndex, questFormID);
    return true;
}

bool QuestObjectiveTracker::completeObjective(uint32_t questFormID, uint32_t objectiveIndex) {
    auto questIt = objectives_.find(questFormID);
    if (questIt == objectives_.end()) return false;

    auto objIt = questIt->second.find(objectiveIndex);
    if (objIt == questIt->second.end()) return false;

    if (objIt->second.isCompleted) return true;

    objIt->second.isCompleted = true;
    objIt->second.currentCount = objIt->second.requiredCount;
    onObjectiveCompleted(questFormID, objectiveIndex);
    return true;
}

bool QuestObjectiveTracker::updateObjectiveProgress(uint32_t questFormID,
                                                     uint32_t objectiveIndex,
                                                     uint32_t progress) {
    auto questIt = objectives_.find(questFormID);
    if (questIt == objectives_.end()) return false;

    auto objIt = questIt->second.find(objectiveIndex);
    if (objIt == questIt->second.end()) return false;

    auto& obj = objIt->second;
    obj.currentCount = progress;

    if (obj.currentCount >= obj.requiredCount && !obj.isCompleted) {
        obj.isCompleted = true;
        onObjectiveCompleted(questFormID, objectiveIndex);
    }

    return true;
}

const ObjectiveProgress* QuestObjectiveTracker::getObjectiveProgress(
        uint32_t questFormID, uint32_t objectiveIndex) const {
    auto questIt = objectives_.find(questFormID);
    if (questIt == objectives_.end()) return nullptr;

    auto objIt = questIt->second.find(objectiveIndex);
    if (objIt == questIt->second.end()) return nullptr;

    return &objIt->second;
}

std::vector<ObjectiveProgress*> QuestObjectiveTracker::getActiveObjectives(uint32_t questFormID) {
    std::vector<ObjectiveProgress*> result;
    auto questIt = objectives_.find(questFormID);
    if (questIt == objectives_.end()) return result;

    for (auto& [idx, obj] : questIt->second) {
        if (obj.isActive && !obj.isCompleted) {
            result.push_back(&obj);
        }
    }
    return result;
}

std::vector<ObjectiveProgress*> QuestObjectiveTracker::getAllObjectives(uint32_t questFormID) {
    std::vector<ObjectiveProgress*> result;
    auto questIt = objectives_.find(questFormID);
    if (questIt == objectives_.end()) return result;

    for (auto& [idx, obj] : questIt->second) {
        result.push_back(&obj);
    }
    return result;
}

bool QuestObjectiveTracker::areAllRequiredObjectivesComplete(uint32_t questFormID) const {
    auto questIt = objectives_.find(questFormID);
    if (questIt == objectives_.end()) return true;

    for (const auto& [idx, obj] : questIt->second) {
        if (!obj.isOptional && !obj.isCompleted) {
            return false;
        }
    }
    return true;
}

// ============================================================================
// EventBus Event Handlers
// ============================================================================

void QuestObjectiveTracker::onEnemyKilled(uint32_t enemyFormID) {
    // Find KILL objectives targeting this enemy
    auto* obj = findObjectiveByTarget(enemyFormID, ObjectiveType::KILL);
    if (obj && obj->isActive && !obj->isCompleted) {
        obj->currentCount++;
        LOGD("Kill objective progress: Quest=0x%08X, Obj=%u, Count=%u/%u",
             obj->questFormID, obj->objectiveIndex,
             obj->currentCount, obj->requiredCount);

        if (obj->currentCount >= obj->requiredCount) {
            obj->isCompleted = true;
            onObjectiveCompleted(obj->questFormID, obj->objectiveIndex);
        }
    }
}

void QuestObjectiveTracker::onItemFound(uint32_t itemFormID) {
    auto* obj = findObjectiveByTarget(itemFormID, ObjectiveType::FIND);
    if (obj && obj->isActive && !obj->isCompleted) {
        obj->currentCount++;
        LOGD("Find objective progress: Quest=0x%08X, Obj=%u, Count=%u/%u",
             obj->questFormID, obj->objectiveIndex,
             obj->currentCount, obj->requiredCount);

        if (obj->currentCount >= obj->requiredCount) {
            obj->isCompleted = true;
            onObjectiveCompleted(obj->questFormID, obj->objectiveIndex);
        }
    }
}

void QuestObjectiveTracker::onLocationDiscovered(uint32_t locationFormID) {
    auto* obj = findObjectiveByTarget(locationFormID, ObjectiveType::DISCOVER);
    if (obj && obj->isActive && !obj->isCompleted) {
        obj->isCompleted = true;
        LOGD("Discover objective completed: Quest=0x%08X, Obj=%u",
             obj->questFormID, obj->objectiveIndex);
        onObjectiveCompleted(obj->questFormID, obj->objectiveIndex);
    }
}

void QuestObjectiveTracker::onNpcTalkedTo(uint32_t npcFormID) {
    auto* obj = findObjectiveByTarget(npcFormID, ObjectiveType::TALK);
    if (obj && obj->isActive && !obj->isCompleted) {
        obj->isCompleted = true;
        LOGD("Talk objective completed: Quest=0x%08X, Obj=%u",
             obj->questFormID, obj->objectiveIndex);
        onObjectiveCompleted(obj->questFormID, obj->objectiveIndex);
    }
}

void QuestObjectiveTracker::onCellEntered(uint32_t cellFormID) {
    // Check REACH objectives
    for (auto& [questFormID, questObjs] : objectives_) {
        for (auto& [idx, obj] : questObjs) {
            if (obj.type == ObjectiveType::REACH && obj.isActive && !obj.isCompleted) {
                if (obj.targetFormID == cellFormID) {
                    obj.isCompleted = true;
                    LOGD("Reach objective completed: Quest=0x%08X, Obj=%u",
                         obj.questFormID, obj.objectiveIndex);
                    onObjectiveCompleted(obj.questFormID, obj.objectiveIndex);
                }
            }
        }
    }
}

// ============================================================================
// EventBus Integration
// ============================================================================

void QuestObjectiveTracker::subscribeToEvents() {
    if (!eventBus_) return;

    // Subscribe to combat events (enemy killed)
    eventBus_->subscribe("ENTITY_DEATH", [this](const weave::Event& event) {
        if (event.targetId != 0) {
            onEnemyKilled(event.targetId);
        }
    });

    // Subscribe to item pickup events
    eventBus_->subscribe("ITEM_PICKUP", [this](const weave::Event& event) {
        if (event.targetId != 0) {
            onItemFound(event.targetId);
        }
    });

    // Subscribe to location discovery events
    eventBus_->subscribe("LOCATION_DISCOVERED", [this](const weave::Event& event) {
        if (event.targetId != 0) {
            onLocationDiscovered(event.targetId);
        }
    });

    // Subscribe to dialogue completion events
    eventBus_->subscribe("DIALOGUE_COMPLETE", [this](const weave::Event& event) {
        if (event.targetId != 0) {
            onNpcTalkedTo(event.targetId);
        }
    });

    // Subscribe to cell entry events
    eventBus_->subscribe("CELL_ENTERED", [this](const weave::Event& event) {
        if (event.targetId != 0) {
            onCellEntered(event.targetId);
        }
    });

    LOGD("Subscribed to EventBus events");
}

void QuestObjectiveTracker::unsubscribeFromEvents() {
    if (!eventBus_) return;

    eventBus_->unsubscribe("ENTITY_DEATH");
    eventBus_->unsubscribe("ITEM_PICKUP");
    eventBus_->unsubscribe("LOCATION_DISCOVERED");
    eventBus_->unsubscribe("DIALOGUE_COMPLETE");
    eventBus_->unsubscribe("CELL_ENTERED");

    LOGD("Unsubscribed from EventBus events");
}

// ============================================================================
// Internal Methods
// ============================================================================

void QuestObjectiveTracker::onObjectiveCompleted(uint32_t questFormID, uint32_t objectiveIndex) {
    LOGI("Objective %u completed for quest 0x%08X", objectiveIndex, questFormID);

    // Update QuestManager
    if (questManager_) {
        questManager_->updateObjectiveProgress(questFormID, objectiveIndex, 1);
    }

    // Notify callback
    if (completionCallback_) {
        completionCallback_(questFormID, objectiveIndex, true);
    }

    // Check if all objectives are complete
    checkQuestCompletion(questFormID);
}

void QuestObjectiveTracker::checkQuestCompletion(uint32_t questFormID) {
    if (areAllRequiredObjectivesComplete(questFormID)) {
        LOGI("All required objectives complete for quest 0x%08X", questFormID);
        // The QuestManager will handle actual quest completion
    }
}

ObjectiveProgress* QuestObjectiveTracker::findObjectiveByTarget(uint32_t targetFormID,
                                                                 ObjectiveType type) {
    for (auto& [questFormID, questObjs] : objectives_) {
        for (auto& [idx, obj] : questObjs) {
            if (obj.targetFormID == targetFormID && obj.type == type) {
                return &obj;
            }
        }
    }
    return nullptr;
}

// ============================================================================
// Save/Load Support
// ============================================================================

std::vector<ObjectiveProgress> QuestObjectiveTracker::exportObjectiveStates() const {
    std::vector<ObjectiveProgress> result;
    for (const auto& [questFormID, questObjs] : objectives_) {
        for (const auto& [idx, obj] : questObjs) {
            result.push_back(obj);
        }
    }
    return result;
}

void QuestObjectiveTracker::importObjectiveStates(const std::vector<ObjectiveProgress>& states) {
    for (const auto& state : states) {
        objectives_[state.questFormID][state.objectiveIndex] = state;
    }
    LOGI("Imported %zu objective states", states.size());
}
