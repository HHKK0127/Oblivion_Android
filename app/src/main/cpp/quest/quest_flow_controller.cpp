#include "quest_flow_controller.h"
#include "../game/player_controller.h"
#include "../game/inventory_manager.h"
#include "../game/npc_manager.h"
#include "../world/world_manager.h"
#include "../script/script_manager.h"
#include <cstring>

// ============================================================================
// QuestFlowController
// ============================================================================

QuestFlowController::QuestFlowController() {
    LOGD("QuestFlowController created");
}

QuestFlowController::~QuestFlowController() {
    cleanup();
    LOGD("QuestFlowController destroyed");
}

bool QuestFlowController::initialize(QuestManager* questMgr,
                                       weave::EventBus* eventBus,
                                       oblivion::script::ScriptManager* scriptMgr,
                                       Player* player,
                                       PlayerController* playerCtrl,
                                       InventoryManager* invMgr,
                                       NpcManager* npcMgr,
                                       WorldManager* worldMgr) {
    if (!questMgr) {
        LOGE("Cannot initialize QuestFlowController with null QuestManager");
        return false;
    }

    questManager_ = questMgr;
    eventBus_ = eventBus;
    scriptManager_ = scriptMgr;
    player_ = player;
    playerController_ = playerCtrl;
    inventoryManager_ = invMgr;
    npcManager_ = npcMgr;
    worldManager_ = worldMgr;

    // Initialize sub-systems
    if (!stageManager_.initialize(questMgr, scriptMgr, npcMgr, worldMgr)) {
        LOGE("Failed to initialize QuestStageManager");
        return false;
    }

    if (!objectiveTracker_.initialize(questMgr, eventBus, nullptr, worldMgr, npcMgr)) {
        LOGE("Failed to initialize QuestObjectiveTracker");
        return false;
    }

    if (!rewardManager_.initialize(player, invMgr)) {
        LOGE("Failed to initialize QuestRewardManager");
        return false;
    }

    // Set up callbacks
    stageManager_.setStageTransitionCallback(
        [this](const StageTransition& t) { onStageTransition(t); });

    objectiveTracker_.setObjectiveCompletionCallback(
        [this](uint32_t qid, uint32_t oid, bool s) { onObjectiveCompleted(qid, oid, s); });

    rewardManager_.setRewardGrantedCallback(
        [this](uint32_t qid, const QuestRewardPackage& r) { onRewardsGranted(qid, r); });

    // Subscribe to EventBus quest events
    if (eventBus_) {
        eventBus_->subscribe("QUEST_TRIGGER", [this](const weave::Event& event) {
            // Parse quest FormID from payload
            if (!event.payload.empty()) {
                uint32_t questFormID = 0;
                try {
                    questFormID = std::stoul(event.payload, nullptr, 16);
                } catch (...) {
                    return;
                }
                activateQuest(questFormID);
            }
        });
    }

    LOGI("QuestFlowController initialized");
    return true;
}

void QuestFlowController::cleanup() {
    stageManager_.cleanup();
    objectiveTracker_.cleanup();
    rewardManager_.cleanup();

    quests_.clear();
    questManager_ = nullptr;
    eventBus_ = nullptr;
    scriptManager_ = nullptr;
    player_ = nullptr;
    playerController_ = nullptr;
    inventoryManager_ = nullptr;
    npcManager_ = nullptr;
    worldManager_ = nullptr;

    LOGD("QuestFlowController cleaned up");
}

void QuestFlowController::update(float deltaTime) {
    // Update sub-systems
    stageManager_.update(deltaTime);
    objectiveTracker_.update(deltaTime);

    // Check for automatic stage advances
    for (auto& [formID, entry] : quests_) {
        if (entry.state == QuestFlowState::ACTIVE) {
            stageManager_.evaluateStageConditions(formID);
        }
    }
}

// ============================================================================
// Quest Registration
// ============================================================================

void QuestFlowController::registerQuest(const QuestRecord& record) {
    QuestFlowEntry entry;
    entry.questFormID = record.formID;
    entry.record = record;
    entry.state = QuestFlowState::REGISTERED;
    entry.currentStage = 0;

    quests_[record.formID] = entry;

    // Register with sub-systems
    stageManager_.registerQuest(record);
    objectiveTracker_.registerQuestObjectives(record);

    // Create default rewards if none specified
    if (!rewardManager_.hasRewards(record.formID)) {
        auto rewards = createDefaultRewards(record);
        rewardManager_.registerReward(record.formID, rewards);
    }

    LOGD("Registered quest flow: FormID=0x%08X, EDID=%s",
         record.formID, record.editorID.c_str());
}

void QuestFlowController::registerQuests(const std::vector<QuestRecord>& records) {
    for (const auto& record : records) {
        registerQuest(record);
    }
    LOGI("Registered %u quest flows", static_cast<uint32_t>(records.size()));
}

// ============================================================================
// Quest Lifecycle
// ============================================================================

bool QuestFlowController::activateQuest(uint32_t questFormID) {
    auto it = quests_.find(questFormID);
    if (it == quests_.end()) {
        LOGW("Quest 0x%08X not registered", questFormID);
        return false;
    }

    auto& entry = it->second;
    if (entry.state != QuestFlowState::REGISTERED &&
        entry.state != QuestFlowState::INACTIVE) {
        LOGW("Quest 0x%08X cannot be activated from state %d",
             questFormID, static_cast<int>(entry.state));
        return false;
    }

    // Activate in QuestManager
    if (questManager_) {
        // Create quest in QuestManager if not already there
        auto existingQuest = questManager_->getQuest(questFormID);
        if (!existingQuest) {
            questManager_->createQuest(0, entry.record.fullName,
                                        entry.record.description);
        }
        questManager_->acceptQuest(questFormID);
    }

    changeState(questFormID, QuestFlowState::ACTIVE);
    entry.activationTime = 0; // TODO: Get current game time

    // Activate initial objectives
    for (const auto& obj : entry.record.objectives) {
        objectiveTracker_.activateObjective(questFormID, obj.objectiveIndex);
    }

    // Set initial stage
    if (!entry.record.stages.empty()) {
        auto sortedStages = entry.record.getSortedStages();
        if (!sortedStages.empty()) {
            entry.currentStage = sortedStages[0]->stageIndex;
            stageManager_.setStage(questFormID, entry.currentStage);
        }
    }

    LOGI("Quest activated: FormID=0x%08X, Name=%s",
         questFormID, entry.record.fullName.c_str());
    return true;
}

bool QuestFlowController::completeQuest(uint32_t questFormID) {
    auto it = quests_.find(questFormID);
    if (it == quests_.end()) return false;

    auto& entry = it->second;
    if (entry.state != QuestFlowState::ACTIVE) {
        LOGW("Quest 0x%08X not active, cannot complete", questFormID);
        return false;
    }

    // Grant rewards
    grantRewards(questFormID);

    // Complete in QuestManager
    if (questManager_) {
        questManager_->completeQuest(questFormID);
    }

    changeState(questFormID, QuestFlowState::COMPLETED);
    entry.completionTime = 0; // TODO: Get current game time

    // Check for quest chain
    checkQuestChain(questFormID);

    LOGI("Quest completed: FormID=0x%08X, Name=%s",
         questFormID, entry.record.fullName.c_str());
    return true;
}

bool QuestFlowController::failQuest(uint32_t questFormID) {
    auto it = quests_.find(questFormID);
    if (it == quests_.end()) return false;

    auto& entry = it->second;
    if (entry.state != QuestFlowState::ACTIVE) {
        LOGW("Quest 0x%08X not active, cannot fail", questFormID);
        return false;
    }

    // Fail in QuestManager
    if (questManager_) {
        questManager_->failQuest(questFormID);
    }

    changeState(questFormID, QuestFlowState::FAILED);

    LOGI("Quest failed: FormID=0x%08X, Name=%s",
         questFormID, entry.record.fullName.c_str());
    return true;
}

// ============================================================================
// Stage Management
// ============================================================================

bool QuestFlowController::advanceStage(uint32_t questFormID, int32_t newStage) {
    auto it = quests_.find(questFormID);
    if (it == quests_.end()) return false;

    if (it->second.state != QuestFlowState::ACTIVE) return false;

    return stageManager_.advanceStage(questFormID, newStage);
}

bool QuestFlowController::setStage(uint32_t questFormID, int32_t stage) {
    auto it = quests_.find(questFormID);
    if (it == quests_.end()) return false;

    return stageManager_.setStage(questFormID, stage);
}

int32_t QuestFlowController::getCurrentStage(uint32_t questFormID) const {
    return stageManager_.getCurrentStage(questFormID);
}

// ============================================================================
// Objective Management
// ============================================================================

bool QuestFlowController::activateObjective(uint32_t questFormID, uint32_t objectiveIndex) {
    return objectiveTracker_.activateObjective(questFormID, objectiveIndex);
}

bool QuestFlowController::completeObjective(uint32_t questFormID, uint32_t objectiveIndex) {
    return objectiveTracker_.completeObjective(questFormID, objectiveIndex);
}

// ============================================================================
// Reward Management
// ============================================================================

bool QuestFlowController::grantRewards(uint32_t questFormID) {
    return rewardManager_.grantRewards(questFormID);
}

// ============================================================================
// Query
// ============================================================================

const QuestFlowEntry* QuestFlowController::getQuestFlow(uint32_t questFormID) const {
    auto it = quests_.find(questFormID);
    if (it == quests_.end()) return nullptr;
    return &it->second;
}

QuestFlowState QuestFlowController::getQuestState(uint32_t questFormID) const {
    auto it = quests_.find(questFormID);
    if (it == quests_.end()) return QuestFlowState::INACTIVE;
    return it->second.state;
}

std::vector<uint32_t> QuestFlowController::getActiveQuests() const {
    std::vector<uint32_t> result;
    for (const auto& [formID, entry] : quests_) {
        if (entry.state == QuestFlowState::ACTIVE) {
            result.push_back(formID);
        }
    }
    return result;
}

std::vector<uint32_t> QuestFlowController::getCompletedQuests() const {
    std::vector<uint32_t> result;
    for (const auto& [formID, entry] : quests_) {
        if (entry.state == QuestFlowState::COMPLETED) {
            result.push_back(formID);
        }
    }
    return result;
}

std::vector<uint32_t> QuestFlowController::getFailedQuests() const {
    std::vector<uint32_t> result;
    for (const auto& [formID, entry] : quests_) {
        if (entry.state == QuestFlowState::FAILED) {
            result.push_back(formID);
        }
    }
    return result;
}

// ============================================================================
// Internal State Management
// ============================================================================

void QuestFlowController::changeState(uint32_t questFormID, QuestFlowState newState) {
    auto it = quests_.find(questFormID);
    if (it == quests_.end()) return;

    QuestFlowState oldState = it->second.state;
    it->second.state = newState;

    LOGD("Quest 0x%08X: State changed from %d to %d",
         questFormID, static_cast<int>(oldState), static_cast<int>(newState));

    // Notify callback
    if (flowCallback_) {
        flowCallback_(questFormID, newState);
    }

    // Emit EventBus event
    if (eventBus_) {
        weave::Event event;
        event.type = "QUEST_STATE_CHANGED";
        event.sender = questFormID;
        event.payload = std::to_string(static_cast<int>(newState));
        eventBus_->emit(event);
    }
}

void QuestFlowController::onStageTransition(const StageTransition& transition) {
    auto it = quests_.find(transition.questFormID);
    if (it == quests_.end()) return;

    it->second.currentStage = transition.toStage;

    // Handle completion stage
    if (transition.isCompletion) {
        completeQuest(transition.questFormID);
    }
    // Handle failure stage
    else if (transition.isFailure) {
        failQuest(transition.questFormID);
    }
}

void QuestFlowController::onObjectiveCompleted(uint32_t questFormID,
                                                 uint32_t objectiveIndex,
                                                 bool success) {
    auto it = quests_.find(questFormID);
    if (it == quests_.end()) return;

    // Check if all required objectives are complete
    if (objectiveTracker_.areAllRequiredObjectivesComplete(questFormID)) {
        it->second.allObjectivesComplete = true;
        LOGI("All objectives complete for quest 0x%08X", questFormID);
    }
}

void QuestFlowController::onRewardsGranted(uint32_t questFormID,
                                             const QuestRewardPackage& rewards) {
    auto it = quests_.find(questFormID);
    if (it == quests_.end()) return;

    it->second.rewardsGranted = true;
    LOGI("Rewards granted for quest 0x%08X", questFormID);
}

void QuestFlowController::checkQuestChain(uint32_t completedQuestFormID) {
    auto it = quests_.find(completedQuestFormID);
    if (it == quests_.end()) return;

    // Check if this quest has a next quest in the chain
    uint32_t nextQuest = it->second.record.nextQuestFormID;
    if (nextQuest != 0) {
        LOGI("Quest 0x%08X chains to quest 0x%08X", completedQuestFormID, nextQuest);
        // Auto-activate the next quest in the chain
        activateQuest(nextQuest);
    }
}

QuestRewardPackage QuestFlowController::createDefaultRewards(const QuestRecord& record) const {
    QuestRewardPackage rewards;
    rewards.questFormID = record.formID;

    // Default rewards based on quest type
    if (record.isMainQuest()) {
        rewards.addExperience(500);
        rewards.addGold(1000);
    } else if (record.isGuildQuest()) {
        rewards.addExperience(250);
        rewards.addGold(500);
    } else if (record.isSideQuest()) {
        rewards.addExperience(100);
        rewards.addGold(200);
    } else {
        rewards.addExperience(50);
        rewards.addGold(100);
    }

    return rewards;
}

// ============================================================================
// Save/Load Support
// ============================================================================

bool QuestFlowController::saveQuestStates(std::vector<uint8_t>& outData) const {
    // Simple serialization: questFormID + state + currentStage
    struct QuestStateEntry {
        uint32_t formID;
        uint8_t state;
        int32_t stage;
    };

    size_t count = quests_.size();
    outData.resize(sizeof(uint32_t) + count * sizeof(QuestStateEntry));

    uint8_t* ptr = outData.data();
    std::memcpy(ptr, &count, sizeof(uint32_t));
    ptr += sizeof(uint32_t);

    for (const auto& [formID, entry] : quests_) {
        QuestStateEntry state;
        state.formID = formID;
        state.state = static_cast<uint8_t>(entry.state);
        state.stage = entry.currentStage;
        std::memcpy(ptr, &state, sizeof(QuestStateEntry));
        ptr += sizeof(QuestStateEntry);
    }

    // Also save sub-system states
    auto stageStates = stageManager_.exportStageStates();
    auto objectiveStates = objectiveTracker_.exportObjectiveStates();
    auto grantedQuests = rewardManager_.exportGrantedQuests();

    // Append stage states
    size_t stageCount = stageStates.size();
    size_t stageDataSize = sizeof(uint32_t) + stageCount * (sizeof(uint32_t) + sizeof(int32_t));
    size_t offset = outData.size();
    outData.resize(offset + stageDataSize);
    ptr = outData.data() + offset;
    std::memcpy(ptr, &stageCount, sizeof(uint32_t));
    ptr += sizeof(uint32_t);
    for (const auto& [fid, stage] : stageStates) {
        std::memcpy(ptr, &fid, sizeof(uint32_t));
        ptr += sizeof(uint32_t);
        std::memcpy(ptr, &stage, sizeof(int32_t));
        ptr += sizeof(int32_t);
    }

    LOGD("Saved %u quest states", static_cast<uint32_t>(count));
    return true;
}

bool QuestFlowController::loadQuestStates(const uint8_t* data, size_t size) {
    if (!data || size < sizeof(uint32_t)) return false;

    const uint8_t* ptr = data;
    uint32_t count = 0;
    std::memcpy(&count, ptr, sizeof(uint32_t));
    ptr += sizeof(uint32_t);

    struct QuestStateEntry {
        uint32_t formID;
        uint8_t state;
        int32_t stage;
    };

    size_t expectedSize = sizeof(uint32_t) + count * sizeof(QuestStateEntry);
    if (size < expectedSize) return false;

    for (uint32_t i = 0; i < count; ++i) {
        QuestStateEntry state;
        std::memcpy(&state, ptr, sizeof(QuestStateEntry));
        ptr += sizeof(QuestStateEntry);

        auto it = quests_.find(state.formID);
        if (it != quests_.end()) {
            it->second.state = static_cast<QuestFlowState>(state.state);
            it->second.currentStage = state.stage;
        }
    }

    // Load stage states
    if (ptr + sizeof(uint32_t) <= data + size) {
        uint32_t stageCount = 0;
        std::memcpy(&stageCount, ptr, sizeof(uint32_t));
        ptr += sizeof(uint32_t);

        std::unordered_map<uint32_t, int32_t> stageStates;
        for (uint32_t i = 0; i < stageCount; ++i) {
            if (ptr + sizeof(uint32_t) + sizeof(int32_t) > data + size) break;
            uint32_t fid;
            int32_t stage;
            std::memcpy(&fid, ptr, sizeof(uint32_t));
            ptr += sizeof(uint32_t);
            std::memcpy(&stage, ptr, sizeof(int32_t));
            ptr += sizeof(int32_t);
            stageStates[fid] = stage;
        }
        stageManager_.importStageStates(stageStates);
    }

    LOGI("Loaded %u quest states", count);
    return true;
}
