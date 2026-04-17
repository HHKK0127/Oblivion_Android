#include "quest_manager.h"
#include <algorithm>

// Quest implementation
void Quest::accept() {
    state = QuestState::ACCEPTED;
    for (auto& obj : objectives) {
        obj.state = QuestObjectiveState::ACTIVE;
    }
}

void Quest::complete() {
    state = QuestState::COMPLETED;
    for (auto& obj : objectives) {
        obj.state = QuestObjectiveState::COMPLETED;
    }
}

void Quest::fail() {
    state = QuestState::FAILED;
    for (auto& obj : objectives) {
        obj.state = QuestObjectiveState::FAILED;
    }
}

void Quest::updateObjective(uint32_t objectiveId, uint32_t progress) {
    for (auto& obj : objectives) {
        if (obj.objectiveId == objectiveId) {
            obj.currentProgress = progress;
            if (obj.isCompleted()) {
                obj.state = QuestObjectiveState::COMPLETED;
            }
            break;
        }
    }
}

bool Quest::allObjectivesCompleted() const {
    if (objectives.empty()) return false;
    for (const auto& obj : objectives) {
        if (!obj.isCompleted()) return false;
    }
    return true;
}

uint32_t Quest::getCompletedObjectiveCount() const {
    uint32_t count = 0;
    for (const auto& obj : objectives) {
        if (obj.isCompleted()) count++;
    }
    return count;
}

// QuestManager implementation
QuestManager::QuestManager()
    : npcManager(nullptr), nextQuestId(1000), nextObjectiveId(1) {
    LOGD("QuestManager created");
}

QuestManager::~QuestManager() {
    cleanup();
    LOGD("QuestManager destroyed");
}

bool QuestManager::initialize(NpcManager* nm) {
    if (!nm) {
        LOGE("Cannot initialize QuestManager with null NpcManager");
        return false;
    }

    npcManager = nm;
    LOGI("QuestManager initialized with NpcManager");
    return true;
}

void QuestManager::cleanup() {
    quests.clear();
    npcQuests.clear();
    activeQuests.clear();
    completedQuests.clear();
    npcManager = nullptr;
    LOGD("QuestManager cleaned up");
}

uint32_t QuestManager::generateQuestId() {
    return nextQuestId++;
}

uint32_t QuestManager::generateObjectiveId() {
    return nextObjectiveId++;
}

uint32_t QuestManager::createQuest(uint32_t npcId, const std::string& title,
                                   const std::string& description) {
    uint32_t questId = generateQuestId();
    auto quest = std::make_shared<Quest>(questId, npcId, title, description);

    quests[questId] = quest;
    npcQuests[npcId].push_back(questId);

    LOGD("Quest created: ID=%u, NPC=%u, Title=%s", questId, npcId, title.c_str());
    return questId;
}

bool QuestManager::addObjective(uint32_t questId, const std::string& objectiveDesc,
                               uint32_t targetProgress) {
    auto it = quests.find(questId);
    if (it == quests.end()) {
        LOGW("Quest ID %u not found", questId);
        return false;
    }

    uint32_t objectiveId = generateObjectiveId();
    QuestObjective objective(objectiveId, objectiveDesc, targetProgress);
    it->second->objectives.push_back(objective);

    LOGD("Objective added to quest %u: ID=%u, Desc=%s, Target=%u",
         questId, objectiveId, objectiveDesc.c_str(), targetProgress);
    return true;
}

bool QuestManager::setQuestReward(uint32_t questId, const QuestReward& reward) {
    auto it = quests.find(questId);
    if (it == quests.end()) {
        LOGW("Quest ID %u not found", questId);
        return false;
    }

    it->second->reward = reward;
    LOGD("Reward set for quest %u: Gold=%u, Exp=%.1f", questId, reward.goldAmount,
         reward.experiencePoints);
    return true;
}

bool QuestManager::acceptQuest(uint32_t questId) {
    auto it = quests.find(questId);
    if (it == quests.end()) {
        LOGW("Quest ID %u not found", questId);
        return false;
    }

    auto quest = it->second;
    if (quest->state != QuestState::PENDING) {
        LOGW("Quest %u is not in PENDING state", questId);
        return false;
    }

    quest->accept();
    activeQuests.push_back(questId);

    LOGI("Quest accepted: ID=%u, Title=%s", questId, quest->title.c_str());
    return true;
}

bool QuestManager::completeQuest(uint32_t questId) {
    auto it = quests.find(questId);
    if (it == quests.end()) {
        LOGW("Quest ID %u not found", questId);
        return false;
    }

    auto quest = it->second;
    if (quest->state == QuestState::COMPLETED) {
        LOGW("Quest %u is already completed", questId);
        return false;
    }

    quest->complete();
    completedQuests.push_back(questId);

    auto activeIt = std::find(activeQuests.begin(), activeQuests.end(), questId);
    if (activeIt != activeQuests.end()) {
        activeQuests.erase(activeIt);
    }

    LOGI("Quest completed: ID=%u, Title=%s, Reward Gold=%u", questId,
         quest->title.c_str(), quest->reward.goldAmount);
    return true;
}

bool QuestManager::failQuest(uint32_t questId) {
    auto it = quests.find(questId);
    if (it == quests.end()) {
        LOGW("Quest ID %u not found", questId);
        return false;
    }

    auto quest = it->second;
    if (quest->state == QuestState::FAILED) {
        LOGW("Quest %u is already failed", questId);
        return false;
    }

    quest->fail();

    auto activeIt = std::find(activeQuests.begin(), activeQuests.end(), questId);
    if (activeIt != activeQuests.end()) {
        activeQuests.erase(activeIt);
    }

    LOGI("Quest failed: ID=%u, Title=%s", questId, quest->title.c_str());
    return true;
}

std::shared_ptr<Quest> QuestManager::getQuest(uint32_t questId) const {
    auto it = quests.find(questId);
    if (it == quests.end()) {
        return nullptr;
    }
    return it->second;
}

std::vector<std::shared_ptr<Quest>> QuestManager::getQuestsByNpc(uint32_t npcId) const {
    std::vector<std::shared_ptr<Quest>> result;
    auto it = npcQuests.find(npcId);
    if (it != npcQuests.end()) {
        for (uint32_t questId : it->second) {
            auto quest = getQuest(questId);
            if (quest) {
                result.push_back(quest);
            }
        }
    }
    return result;
}

std::vector<std::shared_ptr<Quest>> QuestManager::getActiveQuests() const {
    std::vector<std::shared_ptr<Quest>> result;
    for (uint32_t questId : activeQuests) {
        auto quest = getQuest(questId);
        if (quest) {
            result.push_back(quest);
        }
    }
    return result;
}

std::vector<std::shared_ptr<Quest>> QuestManager::getCompletedQuests() const {
    std::vector<std::shared_ptr<Quest>> result;
    for (uint32_t questId : completedQuests) {
        auto quest = getQuest(questId);
        if (quest) {
            result.push_back(quest);
        }
    }
    return result;
}

bool QuestManager::updateObjectiveProgress(uint32_t questId, uint32_t objectiveId,
                                          uint32_t progress) {
    auto it = quests.find(questId);
    if (it == quests.end()) {
        LOGW("Quest ID %u not found", questId);
        return false;
    }

    auto quest = it->second;
    quest->updateObjective(objectiveId, progress);

    LOGD("Objective progress updated: Quest=%u, Objective=%u, Progress=%u", questId,
         objectiveId, progress);

    if (quest->allObjectivesCompleted() && quest->state == QuestState::IN_PROGRESS) {
        return checkAndCompleteQuest(questId);
    }

    return true;
}

bool QuestManager::isQuestActive(uint32_t questId) const {
    auto it = quests.find(questId);
    if (it == quests.end()) {
        return false;
    }
    return it->second->state == QuestState::ACCEPTED ||
           it->second->state == QuestState::IN_PROGRESS;
}

bool QuestManager::isQuestCompleted(uint32_t questId) const {
    auto it = quests.find(questId);
    if (it == quests.end()) {
        return false;
    }
    return it->second->state == QuestState::COMPLETED;
}

void QuestManager::update(float deltaTime) {
    for (uint32_t questId : activeQuests) {
        auto quest = getQuest(questId);
        if (quest && quest->allObjectivesCompleted() &&
            quest->state == QuestState::IN_PROGRESS) {
            checkAndCompleteQuest(questId);
        }
    }
}

bool QuestManager::checkAndCompleteQuest(uint32_t questId) {
    auto quest = getQuest(questId);
    if (quest && quest->allObjectivesCompleted()) {
        return completeQuest(questId);
    }
    return false;
}

void QuestManager::logQuestStatus() const {
    LOGD("========== Quest Manager Status ==========");
    LOGD("Total quests: %zu", quests.size());
    LOGD("Active quests: %zu", activeQuests.size());
    LOGD("Completed quests: %zu", completedQuests.size());

    for (uint32_t questId : activeQuests) {
        auto quest = getQuest(questId);
        if (quest) {
            LOGD("  Quest: %s (ID=%u, Objectives=%u/%u)", quest->title.c_str(),
                 questId, (unsigned int)quest->getCompletedObjectiveCount(), (unsigned int)quest->objectives.size());
        }
    }

    LOGD("==========================================");
}
