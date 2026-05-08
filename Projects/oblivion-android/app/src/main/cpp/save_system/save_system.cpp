#include "save_system.h"
#include "../game/npc.h"
#include "../game/quest.h"
#include "../game/game_state.h"
#include <android/log.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "SaveSystem", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "SaveSystem", __VA_ARGS__)

namespace SaveSystem {

// ============================================================================
// Character Status Serialization
// ============================================================================

json serializeCharacterStatus(const CharacterStatus& status) {
    json j;

    j["currentHealth"] = status.currentHealth;
    j["maxHealth"] = status.maxHealth;
    j["currentMana"] = status.currentMana;
    j["maxMana"] = status.maxMana;
    j["stamina"] = status.stamina;
    j["maxStamina"] = status.maxStamina;

    // Serialize attributes map
    json attributesJson = json::object();
    for (const auto& [key, value] : status.attributes) {
        attributesJson[key] = value;
    }
    j["attributes"] = attributesJson;

    // Serialize skills map
    json skillsJson = json::object();
    for (const auto& [key, value] : status.skills) {
        skillsJson[key] = value;
    }
    j["skills"] = skillsJson;

    // Equipment & Combat
    j["equippedWeaponId"] = status.equippedWeaponId;
    j["weaponDamage"] = status.weaponDamage;
    j["armorRating"] = status.armorRating;

    // Magic System
    j["knownSpells"] = status.knownSpells;
    j["equippedSpells"] = status.equippedSpells;

    return j;
}

CharacterStatus deserializeCharacterStatus(const json& j) {
    CharacterStatus status;

    status.currentHealth = j.value("currentHealth", 100.0f);
    status.maxHealth = j.value("maxHealth", 100.0f);
    status.currentMana = j.value("currentMana", 50.0f);
    status.maxMana = j.value("maxMana", 50.0f);
    status.stamina = j.value("stamina", 100.0f);
    status.maxStamina = j.value("maxStamina", 100.0f);

    // Deserialize attributes
    if (j.contains("attributes") && j["attributes"].is_object()) {
        for (auto& [key, value] : j["attributes"].items()) {
            status.attributes[key] = value.get<float>();
        }
    }

    // Deserialize skills
    if (j.contains("skills") && j["skills"].is_object()) {
        for (auto& [key, value] : j["skills"].items()) {
            status.skills[key] = value.get<float>();
        }
    }

    // Equipment & Combat
    status.equippedWeaponId = j.value("equippedWeaponId", 0U);
    status.weaponDamage = j.value("weaponDamage", 0.0f);
    status.armorRating = j.value("armorRating", 0.0f);

    // Magic System
    if (j.contains("knownSpells") && j["knownSpells"].is_array()) {
        status.knownSpells = j["knownSpells"].get<std::vector<uint32_t>>();
    }
    if (j.contains("equippedSpells") && j["equippedSpells"].is_array()) {
        status.equippedSpells = j["equippedSpells"].get<std::vector<uint32_t>>();
    }

    return status;
}

// ============================================================================
// NPC Serialization
// ============================================================================

json serializeNPC(const NPC& npc) {
    json j;

    j["npcId"] = npc.npcId;
    j["name"] = npc.name;
    j["race"] = npc.race;
    j["class"] = npc.class_;

    // Position & Movement
    j["position"] = serializeVec3(npc.position);
    j["rotation"] = serializeVec3(npc.rotation);
    j["moveSpeed"] = npc.moveSpeed;

    // AI & Behavior
    j["aiState"] = static_cast<int>(npc.aiState);
    j["targetPosition"] = serializeVec3(npc.targetPosition);
    j["wanderRadius"] = npc.wanderRadius;

    // Combat System
    j["status"] = serializeCharacterStatus(npc.status);
    j["lastDamageTime"] = npc.lastDamageTime;
    j["inCombat"] = npc.inCombat;
    j["combatEngagementTime"] = npc.combatEngagementTime;

    // Quest System
    j["availableQuests"] = npc.availableQuests;
    j["givenQuests"] = npc.givenQuests;

    // Magic System
    j["lastSpellCastTime"] = npc.lastSpellCastTime;

    return j;
}

NPC deserializeNPC(const json& j) {
    uint32_t npcId = j.value("npcId", 0U);
    std::string name = j.value("name", "Unknown");

    NPC npc(npcId, name);

    npc.race = j.value("race", "");
    npc.class_ = j.value("class", "");

    // Position & Movement
    if (j.contains("position")) {
        npc.position = deserializeVec3(j["position"]);
    }
    if (j.contains("rotation")) {
        npc.rotation = deserializeVec3(j["rotation"]);
    }
    npc.moveSpeed = j.value("moveSpeed", 5.0f);

    // AI & Behavior
    npc.aiState = static_cast<AIState>(j.value("aiState", 0));
    if (j.contains("targetPosition")) {
        npc.targetPosition = deserializeVec3(j["targetPosition"]);
    }
    npc.wanderRadius = j.value("wanderRadius", 50.0f);

    // Combat System
    if (j.contains("status")) {
        npc.status = deserializeCharacterStatus(j["status"]);
    }
    npc.lastDamageTime = j.value("lastDamageTime", 0.0f);
    npc.inCombat = j.value("inCombat", false);
    npc.combatEngagementTime = j.value("combatEngagementTime", 0.0f);

    // Quest System
    if (j.contains("availableQuests") && j["availableQuests"].is_array()) {
        npc.availableQuests = j["availableQuests"].get<std::vector<uint32_t>>();
    }
    if (j.contains("givenQuests") && j["givenQuests"].is_array()) {
        npc.givenQuests = j["givenQuests"].get<std::vector<uint32_t>>();
    }

    // Magic System
    npc.lastSpellCastTime = j.value("lastSpellCastTime", 0.0f);

    return npc;
}

// ============================================================================
// Quest Objective Serialization
// ============================================================================

json serializeQuestObjective(const QuestObjective& obj) {
    return json{
        {"objectiveId", obj.objectiveId},
        {"description", obj.description},
        {"state", static_cast<int>(obj.state)},
        {"currentProgress", obj.currentProgress},
        {"targetProgress", obj.targetProgress}
    };
}

QuestObjective deserializeQuestObjective(const json& j) {
    uint32_t id = j.value("objectiveId", 0U);
    std::string desc = j.value("description", "");
    uint32_t target = j.value("targetProgress", 1U);

    QuestObjective obj(id, desc, target);
    obj.state = static_cast<QuestObjectiveState>(j.value("state", 0));
    obj.currentProgress = j.value("currentProgress", 0U);

    return obj;
}

// ============================================================================
// Quest Reward Serialization
// ============================================================================

json serializeQuestReward(const QuestReward& reward) {
    return json{
        {"goldAmount", reward.goldAmount},
        {"experiencePoints", reward.experiencePoints},
        {"itemRewards", reward.itemRewards}
    };
}

QuestReward deserializeQuestReward(const json& j) {
    QuestReward reward;
    reward.goldAmount = j.value("goldAmount", 0U);
    reward.experiencePoints = j.value("experiencePoints", 0.0f);

    if (j.contains("itemRewards") && j["itemRewards"].is_array()) {
        reward.itemRewards = j["itemRewards"].get<std::vector<std::string>>();
    }

    return reward;
}

// ============================================================================
// Quest Serialization
// ============================================================================

json serializeQuest(const Quest& quest) {
    json j;

    j["questId"] = quest.questId;
    j["giverNpcId"] = quest.giverNpcId;
    j["title"] = quest.title;
    j["description"] = quest.description;
    j["longDescription"] = quest.longDescription;
    j["state"] = static_cast<int>(quest.state);

    // Serialize objectives
    json objectivesJson = json::array();
    for (const auto& obj : quest.objectives) {
        objectivesJson.push_back(serializeQuestObjective(obj));
    }
    j["objectives"] = objectivesJson;

    // Serialize reward
    j["reward"] = serializeQuestReward(quest.reward);
    j["isRepeatable"] = quest.isRepeatable;

    return j;
}

Quest deserializeQuest(const json& j) {
    Quest quest;

    quest.questId = j.value("questId", 0U);
    quest.giverNpcId = j.value("giverNpcId", 0U);
    quest.title = j.value("title", "");
    quest.description = j.value("description", "");
    quest.longDescription = j.value("longDescription", "");
    quest.state = static_cast<QuestState>(j.value("state", 0));

    // Deserialize objectives
    if (j.contains("objectives") && j["objectives"].is_array()) {
        for (const auto& objJson : j["objectives"]) {
            quest.objectives.push_back(deserializeQuestObjective(objJson));
        }
    }

    // Deserialize reward
    if (j.contains("reward")) {
        quest.reward = deserializeQuestReward(j["reward"]);
    }

    quest.isRepeatable = j.value("isRepeatable", false);

    return quest;
}

// ============================================================================
// Player State Serialization
// ============================================================================

json serializePlayerState(const PlayerState& state) {
    json j;

    j["position"] = serializeVec3(state.position);
    j["rotation"] = serializeVec3(state.rotation);
    j["currentCell"] = state.currentCell;

    // Character Status
    j["status"] = serializeCharacterStatus(state.status);
    j["playerLevel"] = state.playerLevel;
    j["experiencePoints"] = state.experiencePoints;

    // Inventory
    json inventoryJson = json::array();
    for (const auto& [itemId, quantity] : state.inventory) {
        inventoryJson.push_back({{"itemId", itemId}, {"quantity", quantity}});
    }
    j["inventory"] = inventoryJson;
    j["currentWeight"] = state.currentWeight;

    // Equipment
    j["equippedWeapon"] = state.equippedWeapon;
    j["equippedSpell"] = state.equippedSpell;

    return j;
}

PlayerState deserializePlayerState(const json& j) {
    PlayerState state;

    if (j.contains("position")) {
        state.position = deserializeVec3(j["position"]);
    }
    if (j.contains("rotation")) {
        state.rotation = deserializeVec3(j["rotation"]);
    }
    state.currentCell = j.value("currentCell", 0U);

    // Character Status
    if (j.contains("status")) {
        state.status = deserializeCharacterStatus(j["status"]);
    }
    state.playerLevel = j.value("playerLevel", 1U);
    state.experiencePoints = j.value("experiencePoints", 0.0f);

    // Inventory
    if (j.contains("inventory") && j["inventory"].is_array()) {
        for (const auto& item : j["inventory"]) {
            uint32_t itemId = item.value("itemId", 0U);
            uint32_t quantity = item.value("quantity", 0U);
            state.inventory.push_back({itemId, quantity});
        }
    }
    state.currentWeight = j.value("currentWeight", 0.0f);

    // Equipment
    state.equippedWeapon = j.value("equippedWeapon", 0U);
    state.equippedSpell = j.value("equippedSpell", 0U);

    return state;
}

// ============================================================================
// World State Serialization
// ============================================================================

json serializeWorldState(const WorldState& state) {
    json j;

    j["timeOfDay"] = state.timeOfDay;
    j["dayCount"] = state.dayCount;
    j["currentWeather"] = state.currentWeather;
    j["weatherIntensity"] = state.weatherIntensity;
    j["weatherTransitionTime"] = state.weatherTransitionTime;

    // Loaded Cells
    j["loadedCells"] = state.loadedCells;

    // World Items
    json itemsJson = json::array();
    for (const auto& [itemId, position] : state.worldItems) {
        itemsJson.push_back({
            {"itemId", itemId},
            {"position", serializeVec3(position)}
        });
    }
    j["worldItems"] = itemsJson;

    return j;
}

WorldState deserializeWorldState(const json& j) {
    WorldState state;

    state.timeOfDay = j.value("timeOfDay", 12.0f);
    state.dayCount = j.value("dayCount", 0);
    state.currentWeather = j.value("currentWeather", "clear");
    state.weatherIntensity = j.value("weatherIntensity", 0.0f);
    state.weatherTransitionTime = j.value("weatherTransitionTime", 0.0f);

    // Loaded Cells
    if (j.contains("loadedCells") && j["loadedCells"].is_array()) {
        state.loadedCells = j["loadedCells"].get<std::vector<uint32_t>>();
    }

    // World Items
    if (j.contains("worldItems") && j["worldItems"].is_array()) {
        for (const auto& item : j["worldItems"]) {
            uint32_t itemId = item.value("itemId", 0U);
            glm::vec3 position = deserializeVec3(item.value("position", json::object()));
            state.worldItems.push_back({itemId, position});
        }
    }

    return state;
}

// ============================================================================
// NPC State Serialization
// ============================================================================

json serializeNPCState(const NPCState& state) {
    json j;

    j["npcId"] = state.npcId;
    j["position"] = serializeVec3(state.position);
    j["rotation"] = serializeVec3(state.rotation);
    j["status"] = serializeCharacterStatus(state.status);
    j["aiState"] = static_cast<int>(state.aiState);
    j["wanderRadius"] = state.wanderRadius;
    j["currentCell"] = state.currentCell;
    j["availableQuests"] = state.availableQuests;
    j["givenQuests"] = state.givenQuests;

    return j;
}

NPCState deserializeNPCState(const json& j) {
    NPCState state;

    state.npcId = j.value("npcId", 0U);
    if (j.contains("position")) {
        state.position = deserializeVec3(j["position"]);
    }
    if (j.contains("rotation")) {
        state.rotation = deserializeVec3(j["rotation"]);
    }
    if (j.contains("status")) {
        state.status = deserializeCharacterStatus(j["status"]);
    }
    state.aiState = static_cast<AIState>(j.value("aiState", 0));
    state.wanderRadius = j.value("wanderRadius", 50.0f);
    state.currentCell = j.value("currentCell", 0U);

    if (j.contains("availableQuests") && j["availableQuests"].is_array()) {
        state.availableQuests = j["availableQuests"].get<std::vector<uint32_t>>();
    }
    if (j.contains("givenQuests") && j["givenQuests"].is_array()) {
        state.givenQuests = j["givenQuests"].get<std::vector<uint32_t>>();
    }

    return state;
}

// ============================================================================
// Quest Progress State Serialization
// ============================================================================

json serializeQuestProgressState(const QuestProgressState& state) {
    json j;

    j["questId"] = state.questId;
    j["giverNpcId"] = state.giverNpcId;
    j["state"] = static_cast<int>(state.state);

    // Objective Progress
    json progressJson = json::array();
    for (const auto& [objId, progress] : state.objectiveProgress) {
        progressJson.push_back({{"objectiveId", objId}, {"progress", progress}});
    }
    j["objectiveProgress"] = progressJson;

    j["timeAccepted"] = state.timeAccepted;
    j["timeCompleted"] = state.timeCompleted;

    return j;
}

QuestProgressState deserializeQuestProgressState(const json& j) {
    QuestProgressState state;

    state.questId = j.value("questId", 0U);
    state.giverNpcId = j.value("giverNpcId", 0U);
    state.state = static_cast<QuestState>(j.value("state", 0));

    // Objective Progress
    if (j.contains("objectiveProgress") && j["objectiveProgress"].is_array()) {
        for (const auto& obj : j["objectiveProgress"]) {
            uint32_t objId = obj.value("objectiveId", 0U);
            uint32_t progress = obj.value("progress", 0U);
            state.objectiveProgress.push_back({objId, progress});
        }
    }

    state.timeAccepted = j.value("timeAccepted", 0U);
    state.timeCompleted = j.value("timeCompleted", 0U);

    return state;
}

// ============================================================================
// MOD Support: Companion State Serialization
// ============================================================================

json serializeCompanionState(const CompanionState& state) {
    return json{
        {"companionNpcId", state.companionNpcId},
        {"companionName", state.companionName},
        {"isActive", state.isActive},
        {"relationship", state.relationship},
        {"joinedTime", state.joinedTime}
    };
}

CompanionState deserializeCompanionState(const json& j) {
    CompanionState state;
    state.companionNpcId = j.value("companionNpcId", 0U);
    state.companionName = j.value("companionName", "");
    state.isActive = j.value("isActive", false);
    state.relationship = j.value("relationship", 0.0f);
    state.joinedTime = j.value("joinedTime", 0U);
    return state;
}

// ============================================================================
// MOD Support: Pet State Serialization
// ============================================================================

json serializePetState(const PetState& state) {
    return json{
        {"petNpcId", state.petNpcId},
        {"petName", state.petName},
        {"petType", state.petType},
        {"isActive", state.isActive},
        {"lastKnownPosition", serializeVec3(state.lastKnownPosition)}
    };
}

PetState deserializePetState(const json& j) {
    PetState state;
    state.petNpcId = j.value("petNpcId", 0U);
    state.petName = j.value("petName", "");
    state.petType = j.value("petType", "");
    state.isActive = j.value("isActive", false);
    if (j.contains("lastKnownPosition")) {
        state.lastKnownPosition = deserializeVec3(j["lastKnownPosition"]);
    }
    return state;
}

// ============================================================================
// MOD Support: NPC Relationship State Serialization (CSR)
// ============================================================================

json serializeNPCRelationshipState(const NPCRelationshipState& state) {
    return json{
        {"npcId", state.npcId},
        {"dispositionValue", state.dispositionValue},
        {"conversationTopics", state.conversationTopics},
        {"hasBeenGreeted", state.hasBeenGreeted}
    };
}

NPCRelationshipState deserializeNPCRelationshipState(const json& j) {
    NPCRelationshipState state;
    state.npcId = j.value("npcId", 0U);
    state.dispositionValue = j.value("dispositionValue", 0.0f);
    state.hasBeenGreeted = j.value("hasBeenGreeted", false);

    if (j.contains("conversationTopics") && j["conversationTopics"].is_array()) {
        state.conversationTopics = j["conversationTopics"].get<std::vector<std::string>>();
    }

    return state;
}

// ============================================================================
// MOD Support: Game Balance State Serialization
// ============================================================================

json serializeGameBalanceState(const GameBalanceState& state) {
    return json{
        {"carryCapacityMultiplier", state.carryCapacityMultiplier},
        {"damageMultiplier", state.damageMultiplier},
        {"healthRegenRate", state.healthRegenRate}
    };
}

GameBalanceState deserializeGameBalanceState(const json& j) {
    GameBalanceState state;
    state.carryCapacityMultiplier = j.value("carryCapacityMultiplier", 1.0f);
    state.damageMultiplier = j.value("damageMultiplier", 1.0f);
    state.healthRegenRate = j.value("healthRegenRate", 1.0f);
    return state;
}

// ============================================================================
// Complete Game State Serialization (with MOD Support)
// ============================================================================

json serializeGameState(const GameState& state) {
    json j;

    // Metadata
    j["metadata"] = {
        {"saveName", state.saveName},
        {"saveTimestamp", state.saveTimestamp},
        {"version", state.version},
        {"checksum", state.checksum}
    };

    // Core States
    j["playerState"] = serializePlayerState(state.playerState);
    j["worldState"] = serializeWorldState(state.worldState);

    // NPC & Quest States
    json npcStatesJson = json::object();
    for (const auto& [npcId, npcState] : state.npcStates) {
        npcStatesJson[std::to_string(npcId)] = serializeNPCState(npcState);
    }
    j["npcStates"] = npcStatesJson;

    json questStatesJson = json::object();
    for (const auto& [questId, questState] : state.questStates) {
        questStatesJson[std::to_string(questId)] = serializeQuestProgressState(questState);
    }
    j["questStates"] = questStatesJson;

    // MOD Metadata
    j["activeMods"] = state.activeMods;
    j["modVersions"] = state.modVersions;

    // MOD: Companions
    json companionsJson = json::array();
    for (const auto& comp : state.companionStates) {
        companionsJson.push_back(serializeCompanionState(comp));
    }
    j["companionStates"] = companionsJson;

    // MOD: Pets
    json petsJson = json::array();
    for (const auto& pet : state.petStates) {
        petsJson.push_back(serializePetState(pet));
    }
    j["petStates"] = petsJson;
    j["petHappiness"] = state.petHappiness;

    // MOD: Spells
    j["learnedModSpells"] = state.learnedModSpells;

    // MOD: NPC Relationships (CSR)
    json relationshipsJson = json::array();
    for (const auto& rel : state.npcRelationships) {
        relationshipsJson.push_back(serializeNPCRelationshipState(rel));
    }
    j["npcRelationships"] = relationshipsJson;

    // MOD: Game Balance
    j["gameBalance"] = serializeGameBalanceState(state.gameBalance);

    return j;
}

GameState deserializeGameState(const json& j) {
    GameState state;

    // Metadata
    if (j.contains("metadata")) {
        auto& meta = j["metadata"];
        state.saveName = meta.value("saveName", "");
        state.saveTimestamp = meta.value("saveTimestamp", "");
        state.version = meta.value("version", "0.6.0");
        state.checksum = meta.value("checksum", 0U);
    }

    // Core States
    if (j.contains("playerState")) {
        state.playerState = deserializePlayerState(j["playerState"]);
    }
    if (j.contains("worldState")) {
        state.worldState = deserializeWorldState(j["worldState"]);
    }

    // NPC & Quest States
    if (j.contains("npcStates") && j["npcStates"].is_object()) {
        for (auto& [key, value] : j["npcStates"].items()) {
            uint32_t npcId = std::stoul(key);
            state.npcStates[npcId] = deserializeNPCState(value);
        }
    }
    if (j.contains("questStates") && j["questStates"].is_object()) {
        for (auto& [key, value] : j["questStates"].items()) {
            uint32_t questId = std::stoul(key);
            state.questStates[questId] = deserializeQuestProgressState(value);
        }
    }

    // MOD Metadata
    if (j.contains("activeMods") && j["activeMods"].is_array()) {
        state.activeMods = j["activeMods"].get<std::vector<std::string>>();
    }
    if (j.contains("modVersions") && j["modVersions"].is_object()) {
        state.modVersions = j["modVersions"].get<std::map<std::string, std::string>>();
    }

    // MOD: Companions
    if (j.contains("companionStates") && j["companionStates"].is_array()) {
        for (const auto& compJson : j["companionStates"]) {
            state.companionStates.push_back(deserializeCompanionState(compJson));
        }
    }

    // MOD: Pets
    if (j.contains("petStates") && j["petStates"].is_array()) {
        for (const auto& petJson : j["petStates"]) {
            state.petStates.push_back(deserializePetState(petJson));
        }
    }
    state.petHappiness = j.value("petHappiness", 50.0f);

    // MOD: Spells
    if (j.contains("learnedModSpells") && j["learnedModSpells"].is_array()) {
        state.learnedModSpells = j["learnedModSpells"].get<std::vector<uint32_t>>();
    }

    // MOD: NPC Relationships (CSR)
    if (j.contains("npcRelationships") && j["npcRelationships"].is_array()) {
        for (const auto& relJson : j["npcRelationships"]) {
            state.npcRelationships.push_back(deserializeNPCRelationshipState(relJson));
        }
    }

    // MOD: Game Balance
    if (j.contains("gameBalance")) {
        state.gameBalance = deserializeGameBalanceState(j["gameBalance"]);
    }

    return state;
}

// ============================================================================
// Validation Helpers
// ============================================================================

bool validateHealth(float current, float max) {
    return current >= 0.0f && current <= max && max > 0.0f;
}

bool validateMana(float current, float max) {
    return current >= 0.0f && current <= max && max >= 0.0f;
}

bool validateStamina(float current, float max) {
    return current >= 0.0f && current <= max && max > 0.0f;
}

bool validateAttributes(const std::unordered_map<std::string, float>& attrs) {
    // Attributes should be in range 0-100 typically
    for (const auto& [key, value] : attrs) {
        if (value < 0.0f || value > 100.0f) {
            LOGE("Invalid attribute %s: %f", key.c_str(), value);
            return false;
        }
    }
    return true;
}

bool validateSkills(const std::unordered_map<std::string, float>& skills) {
    // Skills should be in range 0-100 typically
    for (const auto& [key, value] : skills) {
        if (value < 0.0f || value > 100.0f) {
            LOGE("Invalid skill %s: %f", key.c_str(), value);
            return false;
        }
    }
    return true;
}

// ============================================================================
// Error Messages
// ============================================================================

std::string getValidationErrorMessage(const std::string& fieldName) {
    return "Validation failed for field: " + fieldName;
}

}  // namespace SaveSystem
