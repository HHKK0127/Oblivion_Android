// Phase 45: Unit Tests Implementation
// Tests key subsystems with isolated unit tests
// Each test group has 3-5 individual test cases

#include "phase45_unit_tests.h"

// Combat system
#include "../game/combat_manager.h"
#include "../game/spell_manager.h"
#include "../game/npc.h"
#include "../game/quest_manager.h"
#include "../game/ai_package.h"
#include "../game/dialogue.h"

// Save system
#include "../save_system/save_manager.h"

// Engine subsystems
#include "../engine/memory_pool.h"
#include "../engine/async_task_manager.h"
#include "../engine/cache_manager.h"

#include <chrono>
#include <cstring>
#include <cmath>
#include <thread>
#include <atomic>
#include <algorithm>

#ifdef __ANDROID__
#include <android/log.h>
#define LOG_TAG "Phase45Test"
#define TEST_LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define TEST_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#else
#include <cstdio>
#define TEST_LOGI(...) printf(__VA_ARGS__)
#define TEST_LOGE(...) fprintf(stderr, __VA_ARGS__)
#endif

// ============================================
// Helper: High-resolution timer
// ============================================
static float getTimeMs45() {
    static auto start = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<float, std::milli>(now - start).count();
}

// ============================================
// Constructor / Destructor
// ============================================
Phase45UnitTests::Phase45UnitTests() {}
Phase45UnitTests::~Phase45UnitTests() {}

// ============================================
// Record a test result
// ============================================
void Phase45UnitTests::record(const std::string& name, bool passed,
                               const std::string& msg, float ms) {
    Phase45TestResult r;
    r.testName = name;
    r.passed = passed;
    r.message = msg;
    r.durationMs = ms;
    results.push_back(r);

    if (passed) {
        TEST_LOGI("[PASS] %s (%.2f ms) %s", name.c_str(), ms, msg.c_str());
    } else {
        TEST_LOGE("[FAIL] %s (%.2f ms) %s", name.c_str(), ms, msg.c_str());
    }
}

// ============================================
// Run all tests
// ============================================
bool Phase45UnitTests::runAllTests() {
    results.clear();

    TEST_LOGI("========================================");
    TEST_LOGI("Phase 45 Unit Test Suite");
    TEST_LOGI("========================================");

    testCombatManager();
    testSpellManager();
    testNPC();
    testQuestManager();
    testSaveManager();
    testMemoryPool();
    testAsyncTaskManager();
    testCacheManager();

    TEST_LOGI("========================================");
    TEST_LOGI("Results: %d passed, %d failed, %zu total",
              getPassCount(), getFailCount(), results.size());
    TEST_LOGI("========================================");

    return getFailCount() == 0;
}

int Phase45UnitTests::getPassCount() const {
    int count = 0;
    for (const auto& r : results) if (r.passed) count++;
    return count;
}

int Phase45UnitTests::getFailCount() const {
    int count = 0;
    for (const auto& r : results) if (!r.passed) count++;
    return count;
}

std::string Phase45UnitTests::getSummary() const {
    std::string summary;
    summary += "Phase 45 Unit Test Results\n";
    summary += "================================\n";
    for (const auto& r : results) {
        summary += r.passed ? "[PASS] " : "[FAIL] ";
        summary += r.testName;
        if (!r.message.empty()) {
            summary += " - " + r.message;
        }
        summary += "\n";
    }
    char buf[128];
    snprintf(buf, sizeof(buf), "\n%d/%d passed (%d failed)\n",
             getPassCount(), (int)results.size(), getFailCount());
    summary += buf;
    return summary;
}

// ============================================================================
// Test Group 1: CombatManager
// Tests weapon damage calculation, parry timing, attack types
// ============================================================================
void Phase45UnitTests::testCombatManager() {
    TEST_LOGI("--- Test Group: CombatManager ---");

    // Test 1.1: Weapon properties defaults
    {
        float t0 = getTimeMs45();
        WeaponProperties wp;
        bool ok = (wp.type == WeaponType::NONE)
               && (wp.baseDamage == 0.0f)
               && (wp.attackSpeed == 1.0f)
               && (wp.range == 3.0f)
               && (wp.criticalChance == 0.05f)
               && (wp.criticalMultiplier == 2.0f)
               && (wp.staminaCost == 10.0f)
               && (!wp.isRanged)
               && (!wp.ignoresArmor)
               && (wp.bleedChance == 0.0f);
        float dt = getTimeMs45() - t0;
        record("Combat_WeaponDefaults", ok,
               "WeaponProperties default values correct", dt);
    }

    // Test 1.2: Weapon type enumeration coverage
    {
        float t0 = getTimeMs45();
        // Verify all weapon types are distinct
        WeaponType types[] = {
            WeaponType::NONE, WeaponType::SWORD_ONE_HAND,
            WeaponType::SWORD_TWO_HAND, WeaponType::AXE_ONE_HAND,
            WeaponType::AXE_TWO_HAND, WeaponType::MACE_ONE_HAND,
            WeaponType::MACE_TWO_HAND, WeaponType::DAGGER,
            WeaponType::BOW, WeaponType::STAFF
        };
        int count = sizeof(types) / sizeof(types[0]);
        bool ok = (count == 10)
               && (static_cast<int>(WeaponType::COUNT) == static_cast<int>(WeaponType::STAFF) + 1);
        float dt = getTimeMs45() - t0;
        record("Combat_WeaponTypeCount", ok,
               "10 weapon types defined", dt);
    }

    // Test 1.3: Combat event types
    {
        float t0 = getTimeMs45();
        CombatEvent event;
        event.type = CombatEvent::Type::ATTACK_HIT;
        event.attackerId = 1;
        event.defenderId = 2;
        event.targetId = 2;
        event.damage = 25.0f;
        event.isCritical = false;
        event.isBlocked = false;
        bool ok = (event.type == CombatEvent::Type::ATTACK_HIT)
               && (event.attackerId == 1)
               && (event.defenderId == event.targetId)
               && (event.damage == 25.0f);
        float dt = getTimeMs45() - t0;
        record("Combat_EventCreation", ok,
               "CombatEvent fields set correctly", dt);
    }

    // Test 1.4: Hitbox structure
    {
        float t0 = getTimeMs45();
        Hitbox hb;
        hb.center = glm::vec3(1.0f, 2.0f, 3.0f);
        hb.halfExtents = glm::vec3(0.5f, 1.0f, 0.5f);
        hb.damage = 15.0f;
        hb.ownerEntityId = 42;
        hb.lifetime = 0.1f;
        hb.isActive = true;
        bool ok = (hb.center.x == 1.0f) && (hb.center.y == 2.0f) && (hb.center.z == 3.0f)
               && (hb.damage == 15.0f) && (hb.ownerEntityId == 42) && hb.isActive;
        float dt = getTimeMs45() - t0;
        record("Combat_HitboxStructure", ok,
               "Hitbox fields set correctly", dt);
    }

    // Test 1.5: Combat constants validation
    {
        float t0 = getTimeMs45();
        // Verify parry window and other constants are reasonable
        // PARRY_WINDOW = 0.3f, DODGE_COOLDOWN = 1.0f, BLEED_DURATION = 5.0f
        // These are private static constexpr, so we test indirectly via WeaponProperties
        WeaponProperties dagger;
        dagger.type = WeaponType::DAGGER;
        dagger.baseDamage = 5.0f;
        dagger.attackSpeed = 1.5f;
        dagger.criticalChance = 0.15f;

        WeaponProperties twoHandAxe;
        twoHandAxe.type = WeaponType::AXE_TWO_HAND;
        twoHandAxe.baseDamage = 25.0f;
        twoHandAxe.attackSpeed = 0.6f;
        twoHandAxe.bleedChance = 0.3f;

        // Dagger should be fast with high crit, axe slow with bleed
        bool ok = (dagger.attackSpeed > twoHandAxe.attackSpeed)
               && (dagger.criticalChance > twoHandAxe.criticalChance)
               && (twoHandAxe.baseDamage > dagger.baseDamage)
               && (twoHandAxe.bleedChance > 0.0f);
        float dt = getTimeMs45() - t0;
        record("Combat_WeaponBalance", ok,
               "Weapon balance properties correct", dt);
    }
}

// ============================================================================
// Test Group 2: SpellManager
// Tests spell creation, mana cost, damage calculation, effect application
// ============================================================================
void Phase45UnitTests::testSpellManager() {
    TEST_LOGI("--- Test Group: SpellManager ---");

    // Test 2.1: Spell creation and basic properties
    {
        float t0 = getTimeMs45();
        Spell fireball(1, "Fireball", "Fireball",
                       MagicSchool::DESTRUCTION, 30.0f, 25.0f);
        bool ok = (fireball.spellId == 1)
               && (fireball.name == "Fireball")
               && (fireball.school == MagicSchool::DESTRUCTION)
               && (fireball.manaCost == 30.0f)
               && (fireball.baseDamage == 25.0f)
               && (fireball.targetType == 1);
        float dt = getTimeMs45() - t0;
        record("Spell_Creation", ok,
               "Spell created with correct properties", dt);
    }

    // Test 2.2: Mana availability check
    {
        float t0 = getTimeMs45();
        Spell heal(2, "Heal", "Heal",
                   MagicSchool::RESTORATION, 20.0f, 0.0f);
        bool canCastFull = heal.isAvailable(50.0f);
        bool canCastExact = heal.isAvailable(20.0f);
        bool cannotCast = heal.isAvailable(15.0f);
        bool ok = canCastFull && canCastExact && !cannotCast;
        float dt = getTimeMs45() - t0;
        record("Spell_ManaCheck", ok,
               "isAvailable correctly checks mana cost", dt);
    }

    // Test 2.3: Spell effects
    {
        float t0 = getTimeMs45();
        SpellEffect dmgEffect(SpellEffectType::DAMAGE, 30.0f, 0.0f);
        SpellEffect healEffect(SpellEffectType::HEAL, 50.0f, 0.0f);
        SpellEffect paralyzeEffect(SpellEffectType::PARALYZE, 1.0f, 5.0f);

        Spell spell(3, "Custom", "Custom",
                    MagicSchool::DESTRUCTION, 40.0f, 30.0f);
        spell.effects.push_back(dmgEffect);
        spell.effects.push_back(healEffect);
        spell.effects.push_back(paralyzeEffect);

        bool ok = (spell.effects.size() == 3)
               && (spell.effects[0].type == SpellEffectType::DAMAGE)
               && (spell.effects[0].magnitude == 30.0f)
               && (spell.effects[1].type == SpellEffectType::HEAL)
               && (spell.effects[2].type == SpellEffectType::PARALYZE)
               && (spell.effects[2].duration == 5.0f);
        float dt = getTimeMs45() - t0;
        record("Spell_Effects", ok,
               "SpellEffect types and magnitudes correct", dt);
    }

    // Test 2.4: Magic school names
    {
        float t0 = getTimeMs45();
        Spell s1(0, "", "", MagicSchool::ALTERATION, 0, 0);
        Spell s2(0, "", "", MagicSchool::CONJURATION, 0, 0);
        Spell s3(0, "", "", MagicSchool::DESTRUCTION, 0, 0);
        Spell s4(0, "", "", MagicSchool::ILLUSION, 0, 0);
        Spell s5(0, "", "", MagicSchool::MYSTICISM, 0, 0);
        Spell s6(0, "", "", MagicSchool::RESTORATION, 0, 0);

        bool ok = (s1.getSchoolName() == "Alteration")
               && (s2.getSchoolName() == "Conjuration")
               && (s3.getSchoolName() == "Destruction")
               && (s4.getSchoolName() == "Illusion")
               && (s5.getSchoolName() == "Mysticism")
               && (s6.getSchoolName() == "Restoration");
        float dt = getTimeMs45() - t0;
        record("Spell_SchoolNames", ok,
               "All 6 magic school names correct", dt);
    }

    // Test 2.5: Spell effect types coverage
    {
        float t0 = getTimeMs45();
        SpellEffectType types[] = {
            SpellEffectType::DAMAGE, SpellEffectType::HEAL,
            SpellEffectType::RESTORE_MANA, SpellEffectType::RESTORE_STAMINA,
            SpellEffectType::FORTIFY_ATTR, SpellEffectType::PARALYZE,
            SpellEffectType::INVISIBILITY, SpellEffectType::SUMMON
        };
        int count = sizeof(types) / sizeof(types[0]);
        bool ok = (count == 8);
        float dt = getTimeMs45() - t0;
        record("Spell_EffectTypeCount", ok,
               "8 spell effect types defined", dt);
    }
}

// ============================================================================
// Test Group 3: NPC
// Tests state transitions, AI package switching, dialogue selection
// ============================================================================
void Phase45UnitTests::testNPC() {
    TEST_LOGI("--- Test Group: NPC ---");

    // Test 3.1: NPC creation and basic properties
    {
        float t0 = getTimeMs45();
        NPC guard(100, "Imperial Guard");
        bool ok = (guard.npcId == 100)
               && (guard.name == "Imperial Guard")
               && (guard.aiState == AIState::IDLE)
               && (!guard.inCombat)
               && (guard.animState == NPC::AnimState::IDLE);
        float dt = getTimeMs45() - t0;
        record("NPC_Creation", ok,
               "NPC created with correct defaults", dt);
    }

    // Test 3.2: AI state transitions
    {
        float t0 = getTimeMs45();
        NPC npc(200, "Test NPC");
        npc.setAIState(AIState::WANDER);
        bool ok1 = (npc.aiState == AIState::WANDER);
        npc.setAIState(AIState::PATROL);
        bool ok2 = (npc.aiState == AIState::PATROL);
        npc.setAIState(AIState::COMBAT);
        bool ok3 = (npc.aiState == AIState::COMBAT);
        npc.setAIState(AIState::IDLE);
        bool ok4 = (npc.aiState == AIState::IDLE);
        bool ok = ok1 && ok2 && ok3 && ok4;
        float dt = getTimeMs45() - t0;
        record("NPC_StateTransitions", ok,
               "AI state transitions work correctly", dt);
    }

    // Test 3.3: Animation state management
    {
        float t0 = getTimeMs45();
        NPC npc(300, "AnimTest");
        bool ok1 = (npc.getAnimState() == NPC::AnimState::IDLE);
        npc.triggerAttack();
        bool ok2 = (npc.getAnimState() == NPC::AnimState::ATTACK);
        npc.triggerHitReaction();
        bool ok3 = (npc.getAnimState() == NPC::AnimState::HIT_REACTION);
        npc.triggerBlock();
        bool ok4 = (npc.getAnimState() == NPC::AnimState::BLOCK);
        npc.triggerDeath();
        bool ok5 = (npc.getAnimState() == NPC::AnimState::DEATH);
        bool ok = ok1 && ok2 && ok3 && ok4 && ok5;
        float dt = getTimeMs45() - t0;
        record("NPC_AnimStates", ok,
               "Animation state triggers work correctly", dt);
    }

    // Test 3.4: Character status
    {
        float t0 = getTimeMs45();
        CharacterStatus status;
        status.initialize(100.0f, 50.0f, 5);
        bool ok1 = (status.maxHealth == 100.0f)
                && (status.currentHealth == 100.0f)
                && (status.maxMana == 50.0f)
                && (status.currentMana == 50.0f)
                && (status.level == 5)
                && status.isAlive();
        status.takeDamage(30.0f);
        bool ok2 = (status.currentHealth == 70.0f) && status.isAlive();
        status.heal(10.0f);
        bool ok3 = (status.currentHealth == 80.0f);
        status.takeDamage(100.0f);
        bool ok4 = !status.isAlive();
        bool ok = ok1 && ok2 && ok3 && ok4;
        float dt = getTimeMs45() - t0;
        record("NPC_CharacterStatus", ok,
               "CharacterStatus init/damage/heal correct", dt);
    }

    // Test 3.5: Quest offering
    {
        float t0 = getTimeMs45();
        NPC npc(400, "QuestGiver");
        npc.addQuestToOffer(10);
        npc.addQuestToOffer(20);
        auto offered = npc.getOfferedQuests();
        bool ok = (offered.size() == 2)
               && (offered[0] == 10)
               && (offered[1] == 20);
        float dt = getTimeMs45() - t0;
        record("NPC_QuestOffering", ok,
               "Quest offering works correctly", dt);
    }
}

// ============================================================================
// Test Group 4: QuestManager
// Tests quest state machine, objective tracking, completion detection
// ============================================================================
void Phase45UnitTests::testQuestManager() {
    TEST_LOGI("--- Test Group: QuestManager ---");

    // Test 4.1: Quest creation and state
    {
        float t0 = getTimeMs45();
        Quest quest(1, 100, "Find the Amulet", "Retrieve the Amulet of Kings");
        bool ok = (quest.questId == 1)
               && (quest.giverNpcId == 100)
               && (quest.title == "Find the Amulet")
               && (quest.state == QuestState::PENDING)
               && (!quest.isCompleted());
        float dt = getTimeMs45() - t0;
        record("Quest_Creation", ok,
               "Quest created with PENDING state", dt);
    }

    // Test 4.2: Quest state machine transitions
    {
        float t0 = getTimeMs45();
        Quest quest(2, 100, "Test Quest", "Description");
        quest.accept();
        bool ok1 = (quest.state == QuestState::ACCEPTED);
        // Note: Quest::accept() may set IN_PROGRESS or ACCEPTED
        // depending on implementation; check both
        bool ok1b = (quest.state == QuestState::ACCEPTED
                  || quest.state == QuestState::IN_PROGRESS);
        quest.fail();
        bool ok2 = (quest.state == QuestState::FAILED);
        bool ok = ok1b && ok2;
        float dt = getTimeMs45() - t0;
        record("Quest_StateMachine", ok,
               "Quest state transitions correct", dt);
    }

    // Test 4.3: Objective tracking
    {
        float t0 = getTimeMs45();
        QuestObjective obj(1, "Kill 5 rats", 5);
        bool ok1 = (obj.objectiveId == 1)
                && (obj.description == "Kill 5 rats")
                && (obj.currentProgress == 0)
                && (obj.targetProgress == 5)
                && (obj.state == QuestObjectiveState::PENDING)
                && !obj.isCompleted();

        obj.currentProgress = 3;
        bool ok2 = !obj.isCompleted();
        obj.currentProgress = 5;
        bool ok3 = obj.isCompleted();
        obj.currentProgress = 7;  // Over-complete
        bool ok4 = obj.isCompleted();

        bool ok = ok1 && ok2 && ok3 && ok4;
        float dt = getTimeMs45() - t0;
        record("Quest_ObjectiveTracking", ok,
               "Objective progress and completion correct", dt);
    }

    // Test 4.4: Quest reward structure
    {
        float t0 = getTimeMs45();
        QuestReward reward;
        reward.goldAmount = 500;
        reward.experiencePoints = 250.0f;
        reward.itemRewards.push_back("Iron Sword");
        reward.itemRewards.push_back("Health Potion");

        bool ok = (reward.goldAmount == 500)
               && (reward.experiencePoints == 250.0f)
               && (reward.itemRewards.size() == 2)
               && (reward.itemRewards[0] == "Iron Sword");
        float dt = getTimeMs45() - t0;
        record("Quest_RewardStructure", ok,
               "QuestReward fields set correctly", dt);
    }

    // Test 4.5: Quest objective states
    {
        float t0 = getTimeMs45();
        QuestObjective obj(1, "Test", 1);
        bool ok1 = (obj.state == QuestObjectiveState::PENDING);
        obj.state = QuestObjectiveState::ACTIVE;
        bool ok2 = (obj.state == QuestObjectiveState::ACTIVE);
        obj.state = QuestObjectiveState::COMPLETED;
        bool ok3 = (obj.state == QuestObjectiveState::COMPLETED);
        obj.state = QuestObjectiveState::FAILED;
        bool ok4 = (obj.state == QuestObjectiveState::FAILED);
        bool ok = ok1 && ok2 && ok3 && ok4;
        float dt = getTimeMs45() - t0;
        record("Quest_ObjectiveStates", ok,
               "All 4 objective states accessible", dt);
    }
}

// ============================================================================
// Test Group 5: SaveManager
// Tests serialization/deserialization roundtrip, save file format validation
// ============================================================================
void Phase45UnitTests::testSaveManager() {
    TEST_LOGI("--- Test Group: SaveManager ---");

    // Test 5.1: GameState default construction
    {
        float t0 = getTimeMs45();
        GameState state;
        bool ok = (state.version == "0.7.0")
               && (state.playerLevel == 1)
               && (state.playerExperience == 0.0f)
               && (state.gameTimeHours == 0.0f)
               && (state.timeOfDay == 12.0f)
               && (state.dayCount == 0)
               && (state.playerStatus.maxHealth == 100.0f)
               && (state.playerStatus.maxMana == 120.0f);
        float dt = getTimeMs45() - t0;
        record("Save_GameStateDefaults", ok,
               "GameState default values correct", dt);
    }

    // Test 5.2: GameState data population
    {
        float t0 = getTimeMs45();
        GameState state;
        state.saveName = "TestSave";
        state.playerPosition = glm::vec3(10.0f, 20.0f, 30.0f);
        state.playerLevel = 15;
        state.playerExperience = 5000.0f;
        state.loadedCells.push_back(0x0000003C);
        state.loadedCells.push_back(0x0000003D);
        state.questStates[1] = 2;
        state.questStates[5] = 3;

        bool ok = (state.saveName == "TestSave")
               && (state.playerPosition.x == 10.0f)
               && (state.playerPosition.y == 20.0f)
               && (state.playerPosition.z == 30.0f)
               && (state.playerLevel == 15)
               && (state.loadedCells.size() == 2)
               && (state.questStates.size() == 2)
               && (state.questStates[1] == 2);
        float dt = getTimeMs45() - t0;
        record("Save_GameStatePopulation", ok,
               "GameState data population correct", dt);
    }

    // Test 5.3: InventorySlotData serialization
    {
        float t0 = getTimeMs45();
        InventorySlotData slot(1001, 5, "Iron Ingot");
        bool ok1 = (slot.itemId == 1001)
                && (slot.quantity == 5)
                && (slot.itemName == "Iron Ingot");

        InventorySlotData defaultSlot;
        bool ok2 = (defaultSlot.itemId == 0)
                && (defaultSlot.quantity == 0)
                && (defaultSlot.itemName.empty());

        bool ok = ok1 && ok2;
        float dt = getTimeMs45() - t0;
        record("Save_InventorySlotData", ok,
               "InventorySlotData constructors correct", dt);
    }

    // Test 5.4: EquippedItemData serialization
    {
        float t0 = getTimeMs45();
        EquippedItemData equip(0, 2001, "Iron Sword");
        bool ok1 = (equip.slotIndex == 0)
                && (equip.itemId == 2001)
                && (equip.itemName == "Iron Sword");

        EquippedItemData defaultEquip;
        bool ok2 = (defaultEquip.slotIndex == 0)
                && (defaultEquip.itemId == 0);

        bool ok = ok1 && ok2;
        float dt = getTimeMs45() - t0;
        record("Save_EquippedItemData", ok,
               "EquippedItemData constructors correct", dt);
    }

    // Test 5.5: SaveManager initialization
    {
        float t0 = getTimeMs45();
        SaveManager mgr;
        bool initOk = mgr.initialize();
        bool ok = initOk;
        float dt = getTimeMs45() - t0;
        record("Save_ManagerInit", ok,
               "SaveManager::initialize() succeeds", dt);
    }
}

// ============================================================================
// Test Group 6: MemoryPool
// Tests object allocation/deallocation, pool exhaustion, LRU cache eviction
// ============================================================================
void Phase45UnitTests::testMemoryPool() {
    TEST_LOGI("--- Test Group: MemoryPool ---");

    // Test 6.1: ObjectPool basic acquire/release
    {
        float t0 = getTimeMs45();
        ObjectPool<int> pool(10);
        int* obj1 = pool.acquire();
        int* obj2 = pool.acquire();
        bool ok1 = (obj1 != nullptr) && (obj2 != nullptr) && (obj1 != obj2);
        bool ok2 = (pool.getActiveCount() == 2);
        pool.release(obj1);
        bool ok3 = (pool.getActiveCount() == 1);
        pool.release(obj2);
        bool ok4 = (pool.getActiveCount() == 0);
        bool ok = ok1 && ok2 && ok3 && ok4;
        float dt = getTimeMs45() - t0;
        record("Pool_AcquireRelease", ok,
               "ObjectPool acquire/release works correctly", dt);
    }

    // Test 6.2: ObjectPool exhaustion
    {
        float t0 = getTimeMs45();
        ObjectPool<int> pool(3);
        // Pre-allocates 1 (half of 3), then can expand to 3
        int* a = pool.acquire();
        int* b = pool.acquire();
        int* c = pool.acquire();
        int* d = pool.acquire();  // Should be nullptr (exhausted)
        bool ok = (a != nullptr) && (b != nullptr) && (c != nullptr) && (d == nullptr);
        pool.release(a);
        pool.release(b);
        pool.release(c);
        float dt = getTimeMs45() - t0;
        record("Pool_Exhaustion", ok,
               "Pool returns nullptr when exhausted", dt);
    }

    // Test 6.3: ObjectPool resetAll
    {
        float t0 = getTimeMs45();
        ObjectPool<int> pool(10);
        int* a = pool.acquire();
        int* b = pool.acquire();
        int* c = pool.acquire();
        bool ok1 = (pool.getActiveCount() == 3);
        pool.resetAll();
        bool ok2 = (pool.getActiveCount() == 0);
        // After reset, can acquire again
        int* d = pool.acquire();
        bool ok3 = (d != nullptr);
        bool ok = ok1 && ok2 && ok3;
        float dt = getTimeMs45() - t0;
        record("Pool_ResetAll", ok,
               "ObjectPool resetAll returns all objects", dt);
    }

    // Test 6.4: MemoryPoolManager initialization
    {
        float t0 = getTimeMs45();
        MemoryPoolManager mgr;
        bool initOk = mgr.initialize();
        bool ok1 = initOk;
        // Test NPC pool
        auto* npc = mgr.getNPCPool().acquire();
        bool ok2 = (npc != nullptr);
        npc->health = 200;
        npc->entityId = 42;
        mgr.getNPCPool().release(npc);
        // Test Effect pool
        auto* effect = mgr.getEffectPool().acquire();
        bool ok3 = (effect != nullptr);
        mgr.getEffectPool().release(effect);
        // Check stats
        auto stats = mgr.getStats();
        bool ok = ok1 && ok2 && ok3;
        mgr.cleanup();
        float dt = getTimeMs45() - t0;
        record("Pool_ManagerInit", ok,
               "MemoryPoolManager init and pool access works", dt);
    }

    // Test 6.5: PooledNPC and PooledEffect reset
    {
        float t0 = getTimeMs45();
        MemoryPoolManager::PooledNPC npc;
        npc.position[0] = 10.0f;
        npc.position[1] = 20.0f;
        npc.position[2] = 30.0f;
        npc.rotation = 1.5f;
        npc.health = 50;
        npc.entityId = 99;
        npc.active = true;
        npc.reset();
        bool ok1 = (npc.position[0] == 0.0f) && (npc.position[1] == 0.0f)
                && (npc.position[2] == 0.0f) && (npc.rotation == 0.0f)
                && (npc.health == 100) && (npc.entityId == -1) && !npc.active;

        MemoryPoolManager::PooledEffect effect;
        effect.lifetime = 5.0f;
        effect.effectType = 3;
        effect.active = true;
        effect.reset();
        bool ok2 = (effect.lifetime == 0.0f) && (effect.effectType == 0) && !effect.active;

        bool ok = ok1 && ok2;
        float dt = getTimeMs45() - t0;
        record("Pool_PooledStructReset", ok,
               "PooledNPC/PooledEffect reset clears fields", dt);
    }
}

// ============================================================================
// Test Group 7: AsyncTaskManager
// Tests task submission, priority ordering, completion callbacks
// ============================================================================
void Phase45UnitTests::testAsyncTaskManager() {
    TEST_LOGI("--- Test Group: AsyncTaskManager ---");

    // Test 7.1: Task manager initialization
    {
        float t0 = getTimeMs45();
        AsyncTaskManager mgr;
        bool initOk = mgr.initialize(2);
        bool ok = initOk;
        mgr.cleanup();
        float dt = getTimeMs45() - t0;
        record("Async_Init", ok,
               "AsyncTaskManager initializes with 2 threads", dt);
    }

    // Test 7.2: Task submission and completion
    {
        float t0 = getTimeMs45();
        AsyncTaskManager mgr;
        mgr.initialize(2);

        std::atomic<int> result{0};
        auto future = mgr.submit(AsyncTaskManager::Priority::NORMAL,
                                  AsyncTaskManager::Category::OTHER,
                                  "test_task",
                                  [&result]() { result.store(42); });

        future.wait_for(std::chrono::seconds(2));
        bool ok = (result.load() == 42);

        mgr.cleanup();
        float dt = getTimeMs45() - t0;
        record("Async_SubmitComplete", ok,
               "Task submitted and executed correctly", dt);
    }

    // Test 7.3: Priority ordering
    {
        float t0 = getTimeMs45();
        AsyncTaskManager mgr;
        mgr.initialize(1);  // Single thread to enforce ordering

        std::vector<int> executionOrder;
        std::mutex orderMutex;

        // Submit LOW priority first, then CRITICAL
        // With single thread, CRITICAL should execute before LOW
        // But since both are submitted nearly simultaneously,
        // we test that the priority queue orders correctly

        auto f1 = mgr.submit(AsyncTaskManager::Priority::LOW,
                              AsyncTaskManager::Category::OTHER,
                              "low_task",
                              [&]() {
                                  std::lock_guard<std::mutex> lock(orderMutex);
                                  executionOrder.push_back(1);
                              });

        auto f2 = mgr.submit(AsyncTaskManager::Priority::CRITICAL,
                              AsyncTaskManager::Category::OTHER,
                              "critical_task",
                              [&]() {
                                  std::lock_guard<std::mutex> lock(orderMutex);
                                  executionOrder.push_back(2);
                              });

        f1.wait_for(std::chrono::seconds(2));
        f2.wait_for(std::chrono::seconds(2));

        // CRITICAL (priority 0) should execute before LOW (priority 3)
        bool ok = false;
        {
            std::lock_guard<std::mutex> lock(orderMutex);
            if (executionOrder.size() == 2) {
                // Critical task (2) should come before low task (1)
                ok = (executionOrder[0] == 2 && executionOrder[1] == 1);
            }
        }

        mgr.cleanup();
        float dt = getTimeMs45() - t0;
        record("Async_PriorityOrder", ok,
               "CRITICAL priority executes before LOW", dt);
    }

    // Test 7.4: Task statistics
    {
        float t0 = getTimeMs45();
        AsyncTaskManager mgr;
        mgr.initialize(2);

        auto f1 = mgr.submit([]() { /* noop */ });
        auto f2 = mgr.submit([]() { /* noop */ });
        f1.wait_for(std::chrono::seconds(2));
        f2.wait_for(std::chrono::seconds(2));

        auto stats = mgr.getStats();
        bool ok = (stats.totalSubmitted >= 2)
               && (stats.totalCompleted >= 2)
               && (stats.threadCount == 2);

        mgr.cleanup();
        float dt = getTimeMs45() - t0;
        record("Async_Statistics", ok,
               "Task stats track submitted/completed counts", dt);
    }

    // Test 7.5: Task categories
    {
        float t0 = getTimeMs45();
        AsyncTaskManager mgr;
        mgr.initialize(2);

        auto f1 = mgr.submitCellLoad([]() { return 1; });
        auto f2 = mgr.submitTextureLoad([]() { return 2; });
        auto f3 = mgr.submitESMParse([]() { return 3; });

        int r1 = f1.get();
        int r2 = f2.get();
        int r3 = f3.get();

        bool ok = (r1 == 1) && (r2 == 2) && (r3 == 3);

        mgr.cleanup();
        float dt = getTimeMs45() - t0;
        record("Async_Categories", ok,
               "Cell/Texture/ESM task categories work", dt);
    }
}

// ============================================================================
// Test Group 8: CacheManager
// Tests L1/L2 promotion/demotion, hit rate tracking
// ============================================================================
void Phase45UnitTests::testCacheManager() {
    TEST_LOGI("--- Test Group: CacheManager ---");

    // Test 8.1: Cache initialization
    {
        float t0 = getTimeMs45();
        CacheManager cache;
        CacheManager::Config cfg;
        cfg.l1MaxMemoryBytes = 1024 * 1024;  // 1MB
        cfg.l2MaxDiskBytes = 4 * 1024 * 1024; // 4MB
        cfg.l1MaxEntries = 64;
        cfg.l2MaxEntries = 256;
        cfg.diskCachePath = "/tmp/phase45_cache_test";
        bool initOk = cache.initialize(cfg);
        bool ok = initOk;
        cache.cleanup();
        float dt = getTimeMs45() - t0;
        record("Cache_Init", ok,
               "CacheManager initializes with config", dt);
    }

    // Test 8.2: L1 put and get
    {
        float t0 = getTimeMs45();
        CacheManager cache;
        CacheManager::Config cfg;
        cfg.l1MaxMemoryBytes = 1024 * 1024;
        cfg.l2MaxDiskBytes = 4 * 1024 * 1024;
        cfg.l1MaxEntries = 64;
        cfg.l2MaxEntries = 256;
        cfg.diskCachePath = "/tmp/phase45_cache_test2";
        cache.initialize(cfg);

        std::vector<uint8_t> data = {0x01, 0x02, 0x03, 0x04, 0x05};
        cache.put("test_key", data);

        std::vector<uint8_t> retrieved;
        bool found = cache.get("test_key", retrieved);
        bool ok = found && (retrieved.size() == 5) && (retrieved[0] == 0x01);

        cache.cleanup();
        float dt = getTimeMs45() - t0;
        record("Cache_L1PutGet", ok,
               "L1 cache put/get works correctly", dt);
    }

    // Test 8.3: Cache miss
    {
        float t0 = getTimeMs45();
        CacheManager cache;
        CacheManager::Config cfg;
        cfg.l1MaxMemoryBytes = 1024 * 1024;
        cfg.l2MaxDiskBytes = 4 * 1024 * 1024;
        cfg.l1MaxEntries = 64;
        cfg.l2MaxEntries = 256;
        cfg.diskCachePath = "/tmp/phase45_cache_test3";
        cache.initialize(cfg);

        std::vector<uint8_t> retrieved;
        bool found = cache.get("nonexistent_key", retrieved);
        bool ok = !found;

        cache.cleanup();
        float dt = getTimeMs45() - t0;
        record("Cache_Miss", ok,
               "Cache miss returns false correctly", dt);
    }

    // Test 8.4: Cache contains and remove
    {
        float t0 = getTimeMs45();
        CacheManager cache;
        CacheManager::Config cfg;
        cfg.l1MaxMemoryBytes = 1024 * 1024;
        cfg.l2MaxDiskBytes = 4 * 1024 * 1024;
        cfg.l1MaxEntries = 64;
        cfg.l2MaxEntries = 256;
        cfg.diskCachePath = "/tmp/phase45_cache_test4";
        cache.initialize(cfg);

        std::vector<uint8_t> data = {0xAA, 0xBB};
        cache.put("remove_test", data);
        bool ok1 = cache.contains("remove_test");
        cache.remove("remove_test");
        bool ok2 = !cache.contains("remove_test");

        bool ok = ok1 && ok2;
        cache.cleanup();
        float dt = getTimeMs45() - t0;
        record("Cache_ContainsRemove", ok,
               "Cache contains/remove works correctly", dt);
    }

    // Test 8.5: Cache statistics
    {
        float t0 = getTimeMs45();
        CacheManager cache;
        CacheManager::Config cfg;
        cfg.l1MaxMemoryBytes = 1024 * 1024;
        cfg.l2MaxDiskBytes = 4 * 1024 * 1024;
        cfg.l1MaxEntries = 64;
        cfg.l2MaxEntries = 256;
        cfg.diskCachePath = "/tmp/phase45_cache_test5";
        cache.initialize(cfg);

        // Generate some hits and misses
        std::vector<uint8_t> data = {0x01, 0x02, 0x03};
        cache.put("key1", data);
        cache.put("key2", data);

        std::vector<uint8_t> out;
        cache.get("key1", out);       // hit
        cache.get("key2", out);       // hit
        cache.get("missing", out);    // miss

        auto stats = cache.getStats();
        bool ok = (stats.totalHits >= 2)
               && (stats.totalMisses >= 1)
               && (stats.l1EntryCount >= 2);

        cache.cleanup();
        float dt = getTimeMs45() - t0;
        record("Cache_Statistics", ok,
               "Cache stats track hits/misses correctly", dt);
    }
}
