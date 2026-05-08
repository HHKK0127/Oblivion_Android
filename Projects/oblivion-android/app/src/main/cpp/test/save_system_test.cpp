#include <iostream>
#include <cassert>
#include <cstring>
#include <android/log.h>
#include "../game/game_state.h"
#include "../game/game_manager.h"
#include "../save_system/save_system.h"
#include "../save_system/save_manager.h"
#include "../save_system/save_validator.h"

#define LOG_TAG "SaveSystemTest"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

using json = nlohmann::json;

// ============================================================================
// Simple Test Framework
// ============================================================================

class TestRunner {
private:
    int totalTests;
    int passedTests;
    int failedTests;

public:
    TestRunner() : totalTests(0), passedTests(0), failedTests(0) {}

    void runTest(const std::string& testName, bool condition, const std::string& message = "") {
        totalTests++;
        if (condition) {
            passedTests++;
            LOGI("✓ PASS: %s", testName.c_str());
        } else {
            failedTests++;
            LOGE("✗ FAIL: %s - %s", testName.c_str(), message.c_str());
        }
    }

    void printSummary() {
        LOGI("===============================================");
        LOGI("TEST SUMMARY");
        LOGI("===============================================");
        LOGI("Total Tests:  %d", totalTests);
        LOGI("Passed:       %d", passedTests);
        LOGI("Failed:       %d", failedTests);
        LOGI("Success Rate: %.1f%%", (totalTests > 0) ? (passedTests * 100.0f / totalTests) : 0.0f);
        LOGI("===============================================");
    }

    int getFailedCount() const { return failedTests; }
};

// ============================================================================
// Test Cases: JSON Serialization
// ============================================================================

void testVec3Serialization(TestRunner& runner) {
    LOGI("Starting Vec3 Serialization Tests");

    // Test serialization
    glm::vec3 original(100.5f, 50.0f, 200.3f);
    json j = SaveSystem::serializeVec3(original);

    // Test deserialization
    glm::vec3 restored = SaveSystem::deserializeVec3(j);

    runner.runTest("Vec3 X coordinate match",
                   original.x == restored.x,
                   "X mismatch");
    runner.runTest("Vec3 Y coordinate match",
                   original.y == restored.y,
                   "Y mismatch");
    runner.runTest("Vec3 Z coordinate match",
                   original.z == restored.z,
                   "Z mismatch");
}

void testCharacterStatusSerialization(TestRunner& runner) {
    LOGI("Starting CharacterStatus Serialization Tests");

    // Create test status
    CharacterStatus original;
    original.currentHealth = 95.0f;
    original.maxHealth = 100.0f;
    original.currentMana = 45.0f;
    original.maxMana = 50.0f;
    original.stamina = 80.0f;
    original.maxStamina = 100.0f;
    original.attributes["Strength"] = 40.0f;
    original.skills["Blade"] = 45.0f;
    original.equippedWeaponId = 1;
    original.weaponDamage = 10.5f;

    // Serialize and deserialize
    json j = SaveSystem::serializeCharacterStatus(original);
    CharacterStatus restored = SaveSystem::deserializeCharacterStatus(j);

    runner.runTest("CharacterStatus health match",
                   original.currentHealth == restored.currentHealth,
                   "Health mismatch");
    runner.runTest("CharacterStatus mana match",
                   original.currentMana == restored.currentMana,
                   "Mana mismatch");
    runner.runTest("CharacterStatus stamina match",
                   original.stamina == restored.stamina,
                   "Stamina mismatch");
    runner.runTest("CharacterStatus weapon ID match",
                   original.equippedWeaponId == restored.equippedWeaponId,
                   "Weapon ID mismatch");
    runner.runTest("CharacterStatus attributes preserved",
                   !restored.attributes.empty(),
                   "Attributes lost");
    runner.runTest("CharacterStatus skills preserved",
                   !restored.skills.empty(),
                   "Skills lost");
}

void testPlayerStateSerialization(TestRunner& runner) {
    LOGI("Starting PlayerState Serialization Tests");

    // Create test player state
    PlayerState original;
    original.position = glm::vec3(100.5f, 50.0f, 200.3f);
    original.rotation = glm::vec3(0.0f, 0.785f, 0.0f);
    original.currentCell = 0;
    original.playerLevel = 15;
    original.experiencePoints = 5000.0f;
    original.inventory.push_back({1, 1});
    original.inventory.push_back({2, 5});
    original.currentWeight = 45.5f;

    // Serialize and deserialize
    json j = SaveSystem::serializePlayerState(original);
    PlayerState restored = SaveSystem::deserializePlayerState(j);

    runner.runTest("PlayerState position match",
                   original.position.x == restored.position.x &&
                   original.position.y == restored.position.y &&
                   original.position.z == restored.position.z,
                   "Position mismatch");
    runner.runTest("PlayerState level match",
                   original.playerLevel == restored.playerLevel,
                   "Level mismatch");
    runner.runTest("PlayerState experience match",
                   original.experiencePoints == restored.experiencePoints,
                   "Experience mismatch");
    runner.runTest("PlayerState inventory preserved",
                   restored.inventory.size() == 2,
                   "Inventory not preserved");
    runner.runTest("PlayerState weight match",
                   original.currentWeight == restored.currentWeight,
                   "Weight mismatch");
}

void testWorldStateSerialization(TestRunner& runner) {
    LOGI("Starting WorldState Serialization Tests");

    // Create test world state
    WorldState original;
    original.timeOfDay = 14.5f;
    original.dayCount = 120;
    original.currentWeather = "rain";
    original.weatherIntensity = 0.7f;
    original.loadedCells.push_back(0);
    original.loadedCells.push_back(1);
    original.loadedCells.push_back(2);

    // Serialize and deserialize
    json j = SaveSystem::serializeWorldState(original);
    WorldState restored = SaveSystem::deserializeWorldState(j);

    runner.runTest("WorldState time match",
                   original.timeOfDay == restored.timeOfDay,
                   "Time mismatch");
    runner.runTest("WorldState day count match",
                   original.dayCount == restored.dayCount,
                   "Day count mismatch");
    runner.runTest("WorldState weather match",
                   original.currentWeather == restored.currentWeather,
                   "Weather mismatch");
    runner.runTest("WorldState loaded cells preserved",
                   restored.loadedCells.size() == 3,
                   "Loaded cells mismatch");
}

// ============================================================================
// Test Cases: MOD Support Serialization
// ============================================================================

void testCompanionStateSerialization(TestRunner& runner) {
    LOGI("Starting CompanionState Serialization Tests");

    // Create test companion
    CompanionState original;
    original.companionNpcId = 1001;
    original.companionName = "Companion Name";
    original.isActive = true;
    original.relationship = 75.5f;
    original.joinedTime = 12345;

    // Serialize and deserialize
    json j = SaveSystem::serializeCompanionState(original);
    CompanionState restored = SaveSystem::deserializeCompanionState(j);

    runner.runTest("CompanionState NPC ID match",
                   original.companionNpcId == restored.companionNpcId,
                   "NPC ID mismatch");
    runner.runTest("CompanionState name match",
                   original.companionName == restored.companionName,
                   "Name mismatch");
    runner.runTest("CompanionState active status match",
                   original.isActive == restored.isActive,
                   "Active status mismatch");
    runner.runTest("CompanionState relationship match",
                   original.relationship == restored.relationship,
                   "Relationship mismatch");
}

void testPetStateSerialization(TestRunner& runner) {
    LOGI("Starting PetState Serialization Tests");

    // Create test pet
    PetState original;
    original.petNpcId = 2001;
    original.petName = "Fluffy";
    original.petType = "cat";
    original.isActive = true;
    original.lastKnownPosition = glm::vec3(150.0f, 30.0f, 180.0f);

    // Serialize and deserialize
    json j = SaveSystem::serializePetState(original);
    PetState restored = SaveSystem::deserializePetState(j);

    runner.runTest("PetState NPC ID match",
                   original.petNpcId == restored.petNpcId,
                   "NPC ID mismatch");
    runner.runTest("PetState name match",
                   original.petName == restored.petName,
                   "Name mismatch");
    runner.runTest("PetState type match",
                   original.petType == restored.petType,
                   "Type mismatch");
    runner.runTest("PetState position match",
                   original.lastKnownPosition.x == restored.lastKnownPosition.x,
                   "Position mismatch");
}

void testGameBalanceStateSerialization(TestRunner& runner) {
    LOGI("Starting GameBalanceState Serialization Tests");

    // Create test balance
    GameBalanceState original;
    original.carryCapacityMultiplier = 2.0f;
    original.damageMultiplier = 1.5f;
    original.healthRegenRate = 1.2f;

    // Serialize and deserialize
    json j = SaveSystem::serializeGameBalanceState(original);
    GameBalanceState restored = SaveSystem::deserializeGameBalanceState(j);

    runner.runTest("GameBalanceState carry capacity match",
                   original.carryCapacityMultiplier == restored.carryCapacityMultiplier,
                   "Carry capacity mismatch");
    runner.runTest("GameBalanceState damage multiplier match",
                   original.damageMultiplier == restored.damageMultiplier,
                   "Damage multiplier mismatch");
    runner.runTest("GameBalanceState health regen match",
                   original.healthRegenRate == restored.healthRegenRate,
                   "Health regen mismatch");
}

// ============================================================================
// Test Cases: Complete GameState
// ============================================================================

void testCompleteGameStateSerialization(TestRunner& runner) {
    LOGI("Starting Complete GameState Serialization Tests");

    // Create complete game state
    GameState original;
    original.saveName = "Test Save";
    original.saveTimestamp = "2026-05-03 14:30:45";
    original.version = "0.6.0";

    // Player state
    original.playerState.position = glm::vec3(100.5f, 50.0f, 200.3f);
    original.playerState.playerLevel = 15;
    original.playerState.inventory.push_back({1, 1});

    // World state
    original.worldState.timeOfDay = 14.5f;
    original.worldState.dayCount = 120;

    // MOD state
    original.activeMods.push_back("cm-partners");
    original.activeMods.push_back("carry-capacity");

    CompanionState comp;
    comp.companionNpcId = 1001;
    comp.companionName = "Companion";
    original.companionStates.push_back(comp);

    // Serialize and deserialize
    json j = SaveSystem::serializeGameState(original);
    GameState restored = SaveSystem::deserializeGameState(j);

    runner.runTest("GameState save name match",
                   original.saveName == restored.saveName,
                   "Save name mismatch");
    runner.runTest("GameState version match",
                   original.version == restored.version,
                   "Version mismatch");
    runner.runTest("GameState player position match",
                   original.playerState.position.x == restored.playerState.position.x,
                   "Player position mismatch");
    runner.runTest("GameState world time match",
                   original.worldState.timeOfDay == restored.worldState.timeOfDay,
                   "World time mismatch");
    runner.runTest("GameState active MODs match",
                   original.activeMods.size() == restored.activeMods.size(),
                   "Active MODs count mismatch");
    runner.runTest("GameState companions match",
                   original.companionStates.size() == restored.companionStates.size(),
                   "Companions count mismatch");
}

// ============================================================================
// Test Cases: Data Validation
// ============================================================================

void testHealthValidation(TestRunner& runner) {
    LOGI("Starting Health Validation Tests");

    // Valid health
    runner.runTest("Valid health (50/100)",
                   SaveSystem::validateHealth(50.0f, 100.0f),
                   "Valid health rejected");
    runner.runTest("Valid health (0/100)",
                   SaveSystem::validateHealth(0.0f, 100.0f),
                   "Zero health rejected");
    runner.runTest("Valid health (100/100)",
                   SaveSystem::validateHealth(100.0f, 100.0f),
                   "Max health rejected");

    // Invalid health
    runner.runTest("Invalid health (negative)",
                   !SaveSystem::validateHealth(-10.0f, 100.0f),
                   "Negative health accepted");
    runner.runTest("Invalid health (over max)",
                   !SaveSystem::validateHealth(150.0f, 100.0f),
                   "Over-max health accepted");
    runner.runTest("Invalid health (max=0)",
                   !SaveSystem::validateHealth(50.0f, 0.0f),
                   "Zero max health accepted");
}

void testManaValidation(TestRunner& runner) {
    LOGI("Starting Mana Validation Tests");

    runner.runTest("Valid mana (25/50)",
                   SaveSystem::validateMana(25.0f, 50.0f),
                   "Valid mana rejected");
    runner.runTest("Valid mana (0/50)",
                   SaveSystem::validateMana(0.0f, 50.0f),
                   "Zero mana rejected");

    runner.runTest("Invalid mana (negative)",
                   !SaveSystem::validateMana(-10.0f, 50.0f),
                   "Negative mana accepted");
    runner.runTest("Invalid mana (over max)",
                   !SaveSystem::validateMana(60.0f, 50.0f),
                   "Over-max mana accepted");
}

void testStaminaValidation(TestRunner& runner) {
    LOGI("Starting Stamina Validation Tests");

    runner.runTest("Valid stamina (80/100)",
                   SaveSystem::validateStamina(80.0f, 100.0f),
                   "Valid stamina rejected");

    runner.runTest("Invalid stamina (negative)",
                   !SaveSystem::validateStamina(-10.0f, 100.0f),
                   "Negative stamina accepted");
}

// ============================================================================
// Test Cases: SaveValidator
// ============================================================================

void testChecksumValidation(TestRunner& runner) {
    LOGI("Starting Checksum Validation Tests");

    SaveValidator validator;

    // Create test game state
    GameState state;
    state.saveName = "Test";
    state.version = "0.6.0";
    state.playerState.status.currentHealth = 100.0f;
    state.playerState.status.maxHealth = 100.0f;

    // Serialize to JSON
    json j = SaveSystem::serializeGameState(state);

    // Compute and add checksum
    validator.addChecksum(j);

    // Verify checksum is present
    runner.runTest("Checksum added to JSON",
                   j["metadata"].contains("checksum"),
                   "Checksum not found in metadata");

    // Verify checksum is valid
    std::string storedChecksum = j["metadata"]["checksum"];
    runner.runTest("Checksum is non-empty",
                   !storedChecksum.empty(),
                   "Checksum is empty");

    // Modify and verify checksum mismatch
    json j_modified = j;
    j_modified["playerState"]["playerLevel"] = 99;

    bool checksumValid = validator.verifyChecksum(j, storedChecksum);
    runner.runTest("Valid checksum verifies",
                   checksumValid,
                   "Valid checksum not verified");

    bool checksumInvalid = validator.verifyChecksum(j_modified, storedChecksum);
    runner.runTest("Modified data fails checksum",
                   !checksumInvalid,
                   "Modified data passed checksum");
}

// ============================================================================
// Main Test Runner
// ============================================================================

extern "C" int runSaveSystemTests() {
    LOGI("==========================================");
    LOGI("Starting Save System Unit Tests");
    LOGI("==========================================");

    TestRunner runner;

    // JSON Serialization Tests
    testVec3Serialization(runner);
    testCharacterStatusSerialization(runner);
    testPlayerStateSerialization(runner);
    testWorldStateSerialization(runner);

    // MOD Support Tests
    testCompanionStateSerialization(runner);
    testPetStateSerialization(runner);
    testGameBalanceStateSerialization(runner);

    // Complete GameState Tests
    testCompleteGameStateSerialization(runner);

    // Data Validation Tests
    testHealthValidation(runner);
    testManaValidation(runner);
    testStaminaValidation(runner);

    // SaveValidator Tests
    testChecksumValidation(runner);

    // Print summary
    runner.printSummary();

    LOGI("==========================================");
    LOGI("Test Execution Complete");
    LOGI("==========================================");

    return runner.getFailedCount();
}

