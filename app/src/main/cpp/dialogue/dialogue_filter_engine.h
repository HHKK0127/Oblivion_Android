#pragma once

#include "dialogue_tree.h"
#include "../game/npc.h"
#include "../game/player.h"
#include "../game/quest_manager.h"
#include "../game/faction_manager.h"
#include <functional>
#include <unordered_map>
#include <android/log.h>

#define DFE_LOG_TAG "DialogueFilterEngine"
#ifdef ENABLE_DEBUG_LOGS
#define DFE_LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, DFE_LOG_TAG, __VA_ARGS__)
#else
#define DFE_LOGD(...) do {} while(0)
#endif
#define DFE_LOGI(...) __android_log_print(ANDROID_LOG_INFO, DFE_LOG_TAG, __VA_ARGS__)
#define DFE_LOGW(...) __android_log_print(ANDROID_LOG_WARN, DFE_LOG_TAG, __VA_ARGS__)
#define DFE_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, DFE_LOG_TAG, __VA_ARGS__)

namespace oblivion {
namespace dialogue {

// ============================================================================
// Player context for filter evaluation
// ============================================================================
struct PlayerContext {
    const Player* player = nullptr;
    uint32_t playerFormID = 0;
    std::string playerName;
    uint32_t playerRaceFormID = 0;
    uint32_t playerClassFormID = 0;
    uint8_t playerGender = 0;       // 0=Male, 1=Female
    uint32_t playerLevel = 1;

    // Skill values (0-100)
    std::unordered_map<uint16_t, float> skillValues;

    // Attribute values
    std::unordered_map<uint16_t, float> attributeValues;

    // Faction memberships
    std::vector<oblivion::FactionMembership> factionMemberships;

    // Active quests
    std::unordered_map<uint32_t, int32_t> questStages; // questFormID -> current stage

    // Disposition toward NPC (0-100)
    int32_t disposition = 50;

    // Global variables
    std::unordered_map<uint32_t, float> globalVariables;
};

// ============================================================================
// NPC context for filter evaluation
// ============================================================================
struct NPCContext {
    const NPC* npc = nullptr;
    uint32_t npcFormID = 0;
    std::string npcName;
    uint32_t npcRaceFormID = 0;
    uint32_t npcClassFormID = 0;
    uint8_t npcGender = 0;
    uint32_t npcLevel = 1;

    // Faction memberships
    std::vector<oblivion::FactionMembership> factionMemberships;

    // Disposition toward player (0-100)
    int32_t disposition = 50;
};

// ============================================================================
// Filter result
// ============================================================================
struct FilterResult {
    const DialogueNode* node = nullptr;
    int32_t score = 0;              // Higher = better match
    bool passed = false;

    FilterResult() = default;
    FilterResult(const DialogueNode* n, int32_t s, bool p)
        : node(n), score(s), passed(p) {}
};

// ============================================================================
// DialogueFilterEngine - Evaluates filter conditions and selects responses
// ============================================================================
class DialogueFilterEngine {
public:
    DialogueFilterEngine();
    ~DialogueFilterEngine();

    // Set game system references
    void setQuestManager(const QuestManager* qm) { questManager = qm; }
    void setFactionManager(const oblivion::FactionManager* fm) { factionManager = fm; }

    // Set player context
    void setPlayerContext(const PlayerContext& ctx) { playerCtx = ctx; }
    const PlayerContext& getPlayerContext() const { return playerCtx; }

    // Set NPC context
    void setNPCContext(const NPCContext& ctx) { npcCtx = ctx; }
    const NPCContext& getNPCContext() const { return npcCtx; }

    // Update player context from Player object
    void updatePlayerFromPlayer(const Player& player, uint32_t playerFormID = 1);

    // Update NPC context from NPC object
    void updateNPCFromNPC(const NPC& npc, uint32_t npcFormID);

    // Evaluate all nodes and return filtered results
    std::vector<FilterResult> evaluateNodes(const DialogueTree& tree) const;

    // Evaluate a single node
    FilterResult evaluateNode(const DialogueNode& node) const;

    // Select the best response from filtered results
    const DialogueNode* selectBestResponse(const std::vector<FilterResult>& results) const;

    // Select the best response for a specific topic
    const DialogueNode* selectBestResponseForTopic(const DialogueTree& tree,
                                                     uint32_t topicFormID) const;

    // Select greeting response
    const DialogueNode* selectGreeting(const DialogueTree& tree) const;

    // Check if a specific condition passes
    bool evaluateCondition(const DialogueCondition& cond) const;

    // Check if a script variable condition passes
    bool evaluateScriptCondition(const ScriptVariableCondition& cond) const;

    // Statistics
    size_t getLastEvaluationCount() const { return lastEvalCount; }
    size_t getLastPassCount() const { return lastPassCount; }

private:
    const QuestManager* questManager = nullptr;
    const oblivion::FactionManager* factionManager = nullptr;

    PlayerContext playerCtx;
    NPCContext npcCtx;

    mutable size_t lastEvalCount = 0;
    mutable size_t lastPassCount = 0;

    // Condition evaluation helpers
    bool evaluateGetIsID(const DialogueCondition& cond) const;
    bool evaluateGetIsSex(const DialogueCondition& cond) const;
    bool evaluateGetIsRace(const DialogueCondition& cond) const;
    bool evaluateGetIsClass(const DialogueCondition& cond) const;
    bool evaluateGetInFaction(const DialogueCondition& cond) const;
    bool evaluateGetFactionRank(const DialogueCondition& cond) const;
    bool evaluateGetLevel(const DialogueCondition& cond) const;
    bool evaluateGetActorValue(const DialogueCondition& cond) const;
    bool evaluateGetStage(const DialogueCondition& cond) const;
    bool evaluateGetQuestRunning(const DialogueCondition& cond) const;
    bool evaluateGetQuestCompleted(const DialogueCondition& cond) const;
    bool evaluateGetRelationshipRank(const DialogueCondition& cond) const;
    bool evaluateGetDisposition(const DialogueCondition& cond) const;
    bool evaluateSameFaction(const DialogueCondition& cond) const;
    bool evaluateSameRace(const DialogueCondition& cond) const;
    bool evaluateSameSex(const DialogueCondition& cond) const;

    // Score calculation
    int32_t calculateNodeScore(const DialogueNode& node) const;

    // Get actor value (skill or attribute) for a given ID
    float getActorValue(uint32_t actorValueID) const;

    // Get NPC actor value
    float getNPCActorValue(uint32_t actorValueID) const;
};

} // namespace dialogue
} // namespace oblivion
