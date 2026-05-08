#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

// Forward declarations
struct CharacterStatus;
struct NPC;
struct Quest;
struct QuestObjective;
struct QuestReward;
struct GameState;
struct PlayerState;
struct WorldState;
struct NPCState;
struct QuestProgressState;
struct CompanionState;
struct PetState;
struct NPCRelationshipState;
struct GameBalanceState;

using json = nlohmann::json;

namespace SaveSystem {

// ============================================================================
// JSON Converters for Basic Types
// ============================================================================

// Convert glm::vec3 to JSON
inline json serializeVec3(const glm::vec3& v) {
    return json{
        {"x", v.x},
        {"y", v.y},
        {"z", v.z}
    };
}

// Convert JSON back to glm::vec3
inline glm::vec3 deserializeVec3(const json& j) {
    return glm::vec3(
        j.value("x", 0.0f),
        j.value("y", 0.0f),
        j.value("z", 0.0f)
    );
}

// ============================================================================
// Character Status Serialization
// ============================================================================

json serializeCharacterStatus(const CharacterStatus& status);
CharacterStatus deserializeCharacterStatus(const json& j);

// ============================================================================
// NPC Serialization (without shared_ptr combatTarget)
// ============================================================================

json serializeNPC(const NPC& npc);
NPC deserializeNPC(const json& j);

// ============================================================================
// Quest Serialization
// ============================================================================

json serializeQuest(const Quest& quest);
Quest deserializeQuest(const json& j);

json serializeQuestObjective(const QuestObjective& obj);
QuestObjective deserializeQuestObjective(const json& j);

json serializeQuestReward(const QuestReward& reward);
QuestReward deserializeQuestReward(const json& j);

// ============================================================================
// Player & World State Serialization
// ============================================================================

json serializePlayerState(const PlayerState& state);
PlayerState deserializePlayerState(const json& j);

json serializeWorldState(const WorldState& state);
WorldState deserializeWorldState(const json& j);

json serializeNPCState(const NPCState& state);
NPCState deserializeNPCState(const json& j);

json serializeQuestProgressState(const QuestProgressState& state);
QuestProgressState deserializeQuestProgressState(const json& j);

// ============================================================================
// MOD Support Serialization
// ============================================================================

// Companion System (CM Partners MOD)
json serializeCompanionState(const CompanionState& state);
CompanionState deserializeCompanionState(const json& j);

// Pet System (Maigrets House Cats, More Female Servants MODs)
json serializePetState(const PetState& state);
PetState deserializePetState(const json& j);

// NPC Relationship System (CSR MOD)
json serializeNPCRelationshipState(const NPCRelationshipState& state);
NPCRelationshipState deserializeNPCRelationshipState(const json& j);

// Game Balance System (Carry Capacity MOD)
json serializeGameBalanceState(const GameBalanceState& state);
GameBalanceState deserializeGameBalanceState(const json& j);

// ============================================================================
// Complete Game State Serialization (with MOD support)
// ============================================================================

json serializeGameState(const GameState& state);
GameState deserializeGameState(const json& j);

// ============================================================================
// Validation Helpers
// ============================================================================

bool validateHealth(float current, float max);
bool validateMana(float current, float max);
bool validateStamina(float current, float max);
bool validateAttributes(const std::unordered_map<std::string, float>& attrs);
bool validateSkills(const std::unordered_map<std::string, float>& skills);

// ============================================================================
// Error Messages
// ============================================================================

std::string getValidationErrorMessage(const std::string& fieldName);

}  // namespace SaveSystem
