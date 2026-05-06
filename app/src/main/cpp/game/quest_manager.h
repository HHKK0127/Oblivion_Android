#pragma once

#include "quest.h"
#include "npc_manager.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <android/log.h>

#define LOG_TAG "QuestManager"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
#define LOGD(...) do {} while(0)
#endif
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

class QuestManager {
private:
    std::unordered_map<uint32_t, std::shared_ptr<Quest>> quests;
    std::unordered_map<uint32_t, std::vector<uint32_t>> npcQuests;
    std::vector<uint32_t> activeQuests;
    std::vector<uint32_t> completedQuests;

    NpcManager* npcManager;
    uint32_t nextQuestId;
    uint32_t nextObjectiveId;

public:
    QuestManager();
    ~QuestManager();

    bool initialize(NpcManager* nm);
    void cleanup();
    void update(float deltaTime);

    uint32_t createQuest(uint32_t npcId, const std::string& title, const std::string& description);
    bool addObjective(uint32_t questId, const std::string& objectiveDesc, uint32_t targetProgress);
    bool setQuestReward(uint32_t questId, const QuestReward& reward);

    bool acceptQuest(uint32_t questId);
    bool completeQuest(uint32_t questId);
    bool failQuest(uint32_t questId);

    std::shared_ptr<Quest> getQuest(uint32_t questId) const;
    std::vector<std::shared_ptr<Quest>> getQuestsByNpc(uint32_t npcId) const;
    std::vector<std::shared_ptr<Quest>> getActiveQuests() const;
    std::vector<std::shared_ptr<Quest>> getCompletedQuests() const;

    bool updateObjectiveProgress(uint32_t questId, uint32_t objectiveId, uint32_t progress);
    bool isQuestActive(uint32_t questId) const;
    bool isQuestCompleted(uint32_t questId) const;
    size_t getActiveQuestCount() const { return activeQuests.size(); }

    void logQuestStatus() const;

private:
    uint32_t generateQuestId();
    uint32_t generateObjectiveId();
    bool checkAndCompleteQuest(uint32_t questId);
};
