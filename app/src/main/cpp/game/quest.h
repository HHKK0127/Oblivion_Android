#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <memory>

enum class QuestState {
    PENDING,      // Available from NPC
    ACCEPTED,     // Player has accepted
    IN_PROGRESS,  // Being worked on
    COMPLETED,    // Successfully completed
    FAILED        // Failed/abandoned
};

enum class QuestObjectiveState {
    PENDING,
    ACTIVE,
    COMPLETED,
    FAILED
};

struct QuestObjective {
    uint32_t objectiveId;
    std::string description;
    QuestObjectiveState state;
    uint32_t currentProgress;
    uint32_t targetProgress;

    QuestObjective(uint32_t id, const std::string& desc, uint32_t target)
        : objectiveId(id), description(desc), state(QuestObjectiveState::PENDING),
          currentProgress(0), targetProgress(target) {}

    bool isCompleted() const { return currentProgress >= targetProgress; }
};

struct QuestReward {
    uint32_t goldAmount;
    float experiencePoints;
    std::vector<std::string> itemRewards;

    QuestReward() : goldAmount(0), experiencePoints(0.0f) {}
};

struct Quest {
    uint32_t questId;
    uint32_t giverNpcId;
    std::string title;
    std::string description;
    std::string longDescription;
    QuestState state;

    std::vector<QuestObjective> objectives;
    QuestReward reward;

    uint32_t timeAccepted;
    uint32_t timeCompleted;

    Quest(uint32_t id, uint32_t npcId, const std::string& t, const std::string& desc)
        : questId(id), giverNpcId(npcId), title(t), description(desc),
          state(QuestState::PENDING),
          timeAccepted(0), timeCompleted(0) {}

    void accept();
    void complete();
    void fail();
    void updateObjective(uint32_t objectiveId, uint32_t progress);
    bool allObjectivesCompleted() const;
    uint32_t getCompletedObjectiveCount() const;
    bool isCompleted() const { return state == QuestState::COMPLETED; }
};
