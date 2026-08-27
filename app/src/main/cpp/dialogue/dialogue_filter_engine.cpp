#include "dialogue_filter_engine.h"
#include <algorithm>
#include <cstring>

namespace oblivion {
namespace dialogue {

// ============================================================================
// Oblivion Actor Value IDs
// ============================================================================
namespace ActorValue {
    constexpr uint16_t Strength      = 0;
    constexpr uint16_t Intelligence  = 1;
    constexpr uint16_t Willpower     = 2;
    constexpr uint16_t Agility       = 3;
    constexpr uint16_t Speed         = 4;
    constexpr uint16_t Endurance     = 5;
    constexpr uint16_t Personality   = 6;
    constexpr uint16_t Luck          = 7;
    constexpr uint16_t Health        = 8;
    constexpr uint16_t Magicka       = 9;
    constexpr uint16_t Stamina       = 10;
    constexpr uint16_t Blade         = 11;
    constexpr uint16_t Blunt         = 12;
    constexpr uint16_t Block         = 13;
    constexpr uint16_t Armorer       = 14;
    constexpr uint16_t HeavyArmor    = 15;
    constexpr uint16_t Alchemy       = 16;
    constexpr uint16_t Alteration    = 17;
    constexpr uint16_t Conjuration   = 18;
    constexpr uint16_t Destruction   = 19;
    constexpr uint16_t Illusion      = 20;
    constexpr uint16_t Mysticism     = 21;
    constexpr uint16_t Restoration   = 22;
    constexpr uint16_t Marksman      = 23;
    constexpr uint16_t Mercantile    = 24;
    constexpr uint16_t Security      = 25;
    constexpr uint16_t Sneak         = 26;
    constexpr uint16_t Speechcraft   = 27;
    constexpr uint16_t Athletics     = 28;
    constexpr uint16_t Acrobatics    = 29;
}

// ============================================================================
// DialogueFilterEngine
// ============================================================================

DialogueFilterEngine::DialogueFilterEngine() = default;
DialogueFilterEngine::~DialogueFilterEngine() = default;

void DialogueFilterEngine::updatePlayerFromPlayer(const Player& player, uint32_t playerFormID) {
    playerCtx.player = &player;
    playerCtx.playerFormID = playerFormID;
    playerCtx.playerName = player.name;
    playerCtx.playerLevel = player.playerLevel;
    playerCtx.playerGender = 0; // Default male

    // Map skills
    playerCtx.skillValues[ActorValue::Blade] = static_cast<float>(player.skills.Blade);
    playerCtx.skillValues[ActorValue::Blunt] = static_cast<float>(player.skills.Blunt);
    playerCtx.skillValues[ActorValue::Block] = static_cast<float>(player.skills.Block);
    playerCtx.skillValues[ActorValue::Restoration] = static_cast<float>(player.skills.Restoration);
    playerCtx.skillValues[ActorValue::Destruction] = static_cast<float>(player.skills.Destruction);
    playerCtx.skillValues[ActorValue::Alteration] = static_cast<float>(player.skills.Alteration);
    playerCtx.skillValues[ActorValue::Conjuration] = static_cast<float>(player.skills.Conjuration);
    playerCtx.skillValues[ActorValue::Illusion] = static_cast<float>(player.skills.Illusion);
    playerCtx.skillValues[ActorValue::Mysticism] = static_cast<float>(player.skills.Mysticism);
    playerCtx.skillValues[ActorValue::Marksman] = static_cast<float>(player.skills.Marksman);
    playerCtx.skillValues[ActorValue::Athletics] = static_cast<float>(player.skills.Athletics);
    playerCtx.skillValues[ActorValue::Acrobatics] = static_cast<float>(player.skills.Acrobatics);

    // Map attributes
    playerCtx.attributeValues[ActorValue::Strength] = static_cast<float>(player.attributes.Strength);
    playerCtx.attributeValues[ActorValue::Intelligence] = static_cast<float>(player.attributes.Intelligence);
    playerCtx.attributeValues[ActorValue::Willpower] = static_cast<float>(player.attributes.Willpower);
    playerCtx.attributeValues[ActorValue::Agility] = static_cast<float>(player.attributes.Agility);
    playerCtx.attributeValues[ActorValue::Speed] = static_cast<float>(player.attributes.Speed);
    playerCtx.attributeValues[ActorValue::Endurance] = static_cast<float>(player.attributes.Endurance);
    playerCtx.attributeValues[ActorValue::Personality] = static_cast<float>(player.attributes.Personality);
    playerCtx.attributeValues[ActorValue::Luck] = static_cast<float>(player.attributes.Luck);

    // Health/Magicka/Stamina
    playerCtx.attributeValues[ActorValue::Health] = player.health;
    playerCtx.attributeValues[ActorValue::Magicka] = 100.0f; // Default
    playerCtx.attributeValues[ActorValue::Stamina] = player.stamina;
}

void DialogueFilterEngine::updateNPCFromNPC(const NPC& npc, uint32_t npcFormID) {
    npcCtx.npc = &npc;
    npcCtx.npcFormID = npcFormID;
    npcCtx.npcName = npc.name;
    npcCtx.npcLevel = npc.status.level;
}

std::vector<FilterResult> DialogueFilterEngine::evaluateNodes(const DialogueTree& tree) const {
    std::vector<FilterResult> results;
    lastEvalCount = 0;
    lastPassCount = 0;

    const auto& allNodes = tree.getAllNodes();
    results.reserve(allNodes.size());

    for (const auto& [id, node] : allNodes) {
        ++lastEvalCount;
        FilterResult result = evaluateNode(node);
        if (result.passed) {
            ++lastPassCount;
            results.push_back(result);
        }
    }

    // Sort by score descending
    std::sort(results.begin(), results.end(),
        [](const FilterResult& a, const FilterResult& b) {
            return a.score > b.score;
        });

    DFE_LOGD("Evaluated %zu nodes, %zu passed filters", lastEvalCount, lastPassCount);
    return results;
}

FilterResult DialogueFilterEngine::evaluateNode(const DialogueNode& node) const {
    FilterResult result;
    result.node = &node;

    // Check all conditions
    for (const auto& cond : node.conditions) {
        if (!evaluateCondition(cond)) {
            result.passed = false;
            result.score = 0;
            return result;
        }
    }

    // Check script variable conditions
    for (const auto& svc : node.scriptConditions) {
        if (!evaluateScriptCondition(svc)) {
            result.passed = false;
            result.score = 0;
            return result;
        }
    }

    // All conditions passed
    result.passed = true;
    result.score = calculateNodeScore(node);
    return result;
}

const DialogueNode* DialogueFilterEngine::selectBestResponse(
    const std::vector<FilterResult>& results) const {
    if (results.empty()) return nullptr;
    // Results are already sorted by score
    return results[0].node;
}

const DialogueNode* DialogueFilterEngine::selectBestResponseForTopic(
    const DialogueTree& tree, uint32_t topicFormID) const {
    auto nodes = tree.getNodesForTopic(topicFormID);
    if (nodes.empty()) return nullptr;

    std::vector<FilterResult> results;
    for (const auto* node : nodes) {
        FilterResult result = evaluateNode(*node);
        if (result.passed) {
            results.push_back(result);
        }
    }

    return selectBestResponse(results);
}

const DialogueNode* DialogueFilterEngine::selectGreeting(const DialogueTree& tree) const {
    // Try greeting category first
    auto greetingGroups = tree.getTopicGroupsByCategory(TopicCategory::Greeting);
    for (const auto* group : greetingGroups) {
        const DialogueNode* best = selectBestResponseForTopic(tree, group->topicFormID);
        if (best) return best;
    }

    // Fallback to any available node
    const DialogueNode* greeting = tree.getGreetingNode();
    if (greeting) {
        FilterResult result = evaluateNode(*greeting);
        if (result.passed) return greeting;
    }

    return nullptr;
}

bool DialogueFilterEngine::evaluateCondition(const DialogueCondition& cond) const {
    auto func = static_cast<ConditionFunction>(cond.functionIndex);

    switch (func) {
        case ConditionFunction::GetIsID:
            return evaluateGetIsID(cond);
        case ConditionFunction::GetIsSex:
        case ConditionFunction::GetPCIsSex:
            return evaluateGetIsSex(cond);
        case ConditionFunction::GetIsRace:
        case ConditionFunction::GetPCIsRace:
            return evaluateGetIsRace(cond);
        case ConditionFunction::GetIsClass:
        case ConditionFunction::GetPCIsClass:
            return evaluateGetIsClass(cond);
        case ConditionFunction::GetInFaction:
        case ConditionFunction::GetPCInFaction:
            return evaluateGetInFaction(cond);
        case ConditionFunction::GetFactionRank:
            return evaluateGetFactionRank(cond);
        case ConditionFunction::GetLevel:
            return evaluateGetLevel(cond);
        case ConditionFunction::GetActorValue:
        case ConditionFunction::GetBaseActorValue:
            return evaluateGetActorValue(cond);
        case ConditionFunction::GetStage:
            return evaluateGetStage(cond);
        case ConditionFunction::GetQuestRunning:
            return evaluateGetQuestRunning(cond);
        case ConditionFunction::GetQuestCompleted:
            return evaluateGetQuestCompleted(cond);
        case ConditionFunction::GetRelationshipRank:
            return evaluateGetRelationshipRank(cond);
        case ConditionFunction::GetDisposition:
            return evaluateGetDisposition(cond);
        case ConditionFunction::SameFaction:
            return evaluateSameFaction(cond);
        case ConditionFunction::SameRace:
            return evaluateSameRace(cond);
        case ConditionFunction::SameSex:
            return evaluateSameSex(cond);
        case ConditionFunction::GetRandomPercent:
            // Always pass for now (would need RNG)
            return true;
        default:
            // Unknown condition - pass by default
            DFE_LOGD("Unknown condition function: %u", cond.functionIndex);
            return true;
    }
}

bool DialogueFilterEngine::evaluateScriptCondition(const ScriptVariableCondition& cond) const {
    // Check global variables
    auto it = playerCtx.globalVariables.find(cond.variableFormID);
    if (it != playerCtx.globalVariables.end()) {
        return cond.evaluate(it->second);
    }
    // If variable not found, pass by default
    return true;
}

// ============================================================================
// Individual condition evaluators
// ============================================================================

bool DialogueFilterEngine::evaluateGetIsID(const DialogueCondition& cond) const {
    // Check if the subject matches the specified FormID
    uint32_t targetFormID = cond.param1;
    if (cond.runOnType == 0) { // Subject = NPC
        return npcCtx.npcFormID == targetFormID;
    } else if (cond.runOnType == 1) { // Target = Player
        return playerCtx.playerFormID == targetFormID;
    }
    return false;
}

bool DialogueFilterEngine::evaluateGetIsSex(const DialogueCondition& cond) const {
    uint8_t targetSex = static_cast<uint8_t>(cond.comparisonValue);
    if (cond.runOnType == 0) {
        return npcCtx.npcGender == targetSex;
    } else {
        return playerCtx.playerGender == targetSex;
    }
}

bool DialogueFilterEngine::evaluateGetIsRace(const DialogueCondition& cond) const {
    uint32_t targetRace = cond.param1;
    if (cond.runOnType == 0) {
        return npcCtx.npcRaceFormID == targetRace;
    } else {
        return playerCtx.playerRaceFormID == targetRace;
    }
}

bool DialogueFilterEngine::evaluateGetIsClass(const DialogueCondition& cond) const {
    uint32_t targetClass = cond.param1;
    if (cond.runOnType == 0) {
        return npcCtx.npcClassFormID == targetClass;
    } else {
        return playerCtx.playerClassFormID == targetClass;
    }
}

bool DialogueFilterEngine::evaluateGetInFaction(const DialogueCondition& cond) const {
    uint32_t targetFaction = cond.param1;
    const auto& memberships = (cond.runOnType == 0)
        ? npcCtx.factionMemberships
        : playerCtx.factionMemberships;

    for (const auto& membership : memberships) {
        if (membership.factionFormID == targetFaction) {
            return true;
        }
    }
    return false;
}

bool DialogueFilterEngine::evaluateGetFactionRank(const DialogueCondition& cond) const {
    uint32_t targetFaction = cond.param1;
    int32_t requiredRank = static_cast<int32_t>(cond.comparisonValue);
    const auto& memberships = (cond.runOnType == 0)
        ? npcCtx.factionMemberships
        : playerCtx.factionMemberships;

    for (const auto& membership : memberships) {
        if (membership.factionFormID == targetFaction) {
            return cond.evaluate(static_cast<float>(membership.rank));
        }
    }
    return false;
}

bool DialogueFilterEngine::evaluateGetLevel(const DialogueCondition& cond) const {
    float level = (cond.runOnType == 0)
        ? static_cast<float>(npcCtx.npcLevel)
        : static_cast<float>(playerCtx.playerLevel);
    return cond.evaluate(level);
}

bool DialogueFilterEngine::evaluateGetActorValue(const DialogueCondition& cond) const {
    uint32_t avID = cond.param1;
    float value = (cond.runOnType == 0) ? getNPCActorValue(avID) : getActorValue(avID);
    return cond.evaluate(value);
}

bool DialogueFilterEngine::evaluateGetStage(const DialogueCondition& cond) const {
    uint32_t questFormID = cond.param1;
    int32_t requiredStage = static_cast<int32_t>(cond.comparisonValue);

    auto it = playerCtx.questStages.find(questFormID);
    if (it != playerCtx.questStages.end()) {
        return cond.evaluate(static_cast<float>(it->second));
    }
    return false;
}

bool DialogueFilterEngine::evaluateGetQuestRunning(const DialogueCondition& cond) const {
    uint32_t questFormID = cond.param1;
    auto it = playerCtx.questStages.find(questFormID);
    bool isRunning = (it != playerCtx.questStages.end());
    return cond.evaluate(isRunning ? 1.0f : 0.0f);
}

bool DialogueFilterEngine::evaluateGetQuestCompleted(const DialogueCondition& cond) const {
    uint32_t questFormID = cond.param1;
    // Check if quest stage indicates completion (stage >= 100 typically means done)
    auto it = playerCtx.questStages.find(questFormID);
    if (it != playerCtx.questStages.end()) {
        return cond.evaluate(it->second >= 100 ? 1.0f : 0.0f);
    }
    return cond.evaluate(0.0f);
}

bool DialogueFilterEngine::evaluateGetRelationshipRank(const DialogueCondition& cond) const {
    // Use disposition as proxy for relationship rank
    int32_t rank = (npcCtx.disposition - 50) / 10; // Map 0-100 to -5 to 5
    return cond.evaluate(static_cast<float>(rank));
}

bool DialogueFilterEngine::evaluateGetDisposition(const DialogueCondition& cond) const {
    return cond.evaluate(static_cast<float>(npcCtx.disposition));
}

bool DialogueFilterEngine::evaluateSameFaction(const DialogueCondition& cond) const {
    for (const auto& npcFaction : npcCtx.factionMemberships) {
        for (const auto& playerFaction : playerCtx.factionMemberships) {
            if (npcFaction.factionFormID == playerFaction.factionFormID) {
                return true;
            }
        }
    }
    return false;
}

bool DialogueFilterEngine::evaluateSameRace(const DialogueCondition& cond) const {
    return npcCtx.npcRaceFormID == playerCtx.playerRaceFormID;
}

bool DialogueFilterEngine::evaluateSameSex(const DialogueCondition& cond) const {
    return npcCtx.npcGender == playerCtx.playerGender;
}

// ============================================================================
// Score calculation
// ============================================================================

int32_t DialogueFilterEngine::calculateNodeScore(const DialogueNode& node) const {
    int32_t score = node.priority;

    // Bonus for NPC-specific dialogue
    if (node.filterNPCFormID != 0 && node.filterNPCFormID == npcCtx.npcFormID) {
        score += 100;
    }

    // Bonus for quest-linked dialogue
    if (node.questFormID != 0) {
        auto it = playerCtx.questStages.find(node.questFormID);
        if (it != playerCtx.questStages.end()) {
            score += 50;
            // Extra bonus if stage matches
            if (node.questStage >= 0 && it->second == node.questStage) {
                score += 30;
            }
        }
    }

    // Bonus for faction match
    if (node.factionFormID != 0) {
        for (const auto& membership : playerCtx.factionMemberships) {
            if (membership.factionFormID == node.factionFormID) {
                score += 20;
                if (node.factionRank >= 0 && membership.rank >= node.factionRank) {
                    score += 10;
                }
                break;
            }
        }
    }

    // Penalty for already spoken nodes
    if (node.hasBeenSpoken) {
        score -= 200;
    }

    return score;
}

float DialogueFilterEngine::getActorValue(uint32_t actorValueID) const {
    auto it = playerCtx.skillValues.find(static_cast<uint16_t>(actorValueID));
    if (it != playerCtx.skillValues.end()) {
        return it->second;
    }
    auto it2 = playerCtx.attributeValues.find(static_cast<uint16_t>(actorValueID));
    if (it2 != playerCtx.attributeValues.end()) {
        return it2->second;
    }
    return 0.0f;
}

float DialogueFilterEngine::getNPCActorValue(uint32_t actorValueID) const {
    // Map NPC attributes from CharacterStatus
    if (!npcCtx.npc) return 0.0f;

    switch (actorValueID) {
        case ActorValue::Health: return npcCtx.npc->status.currentHealth;
        case ActorValue::Magicka: return npcCtx.npc->status.currentMana;
        case ActorValue::Stamina: return npcCtx.npc->status.stamina;
        default: {
            auto it = npcCtx.npc->status.skills.find(
                std::to_string(actorValueID));
            if (it != npcCtx.npc->status.skills.end()) {
                return it->second;
            }
            return 0.0f;
        }
    }
}

} // namespace dialogue
} // namespace oblivion
