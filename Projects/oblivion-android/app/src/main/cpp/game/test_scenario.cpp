#include "test_scenario.h"
#include "../engine/renderer.h"
#include "npc_manager.h"
#include "quest_manager.h"
#include "combat_manager.h"
#include "spell_manager.h"
#include "../world/world_manager.h"
#include <android/log.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "TestScenario", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "TestScenario", __VA_ARGS__)

void createTestScenario(Renderer* renderer) {
    LOGI("=== createTestScenario() START ===");

    auto worldManager = renderer->getWorldManager();
    auto questManager = renderer->getQuestManager();
    auto combatManager = renderer->getCombatManager();
    auto spellManager = renderer->getSpellManager();

    // Safety checks
    if (!worldManager) {
        LOGE("ERROR: worldManager is null in createTestScenario()");
        return;
    }

    if (!questManager) {
        LOGE("ERROR: questManager is null in createTestScenario()");
        return;
    }

    if (!combatManager) {
        LOGE("ERROR: combatManager is null in createTestScenario()");
        return;
    }

    if (!spellManager) {
        LOGE("ERROR: spellManager is null in createTestScenario()");
        return;
    }

    // Declare spell variables at function scope so they're available for spell casting
    uint32_t fireball = 0;
    uint32_t heal = 0;
    uint32_t restoreMana = 0;

    // Create test NPCs
    NpcManager* npcMgr = worldManager->getNpcManager();
    if (!npcMgr) {
        LOGE("ERROR: getNpcManager() returned null");
        return;
    }

    auto izar = npcMgr->createNPC("Izar", glm::vec3(0.0f, 0.0f, 0.0f));
    auto hellas = npcMgr->createNPC("Hellas", glm::vec3(5.0f, 0.0f, 0.0f));

    if (!izar) {
        LOGE("ERROR: Failed to create NPC 'Izar'");
        return;
    }
    if (!hellas) {
        LOGE("ERROR: Failed to create NPC 'Hellas'");
        return;
    }

    if (izar && hellas) {
        izar->status.initialize(150.0f, 100.0f, 5);
        hellas->status.initialize(120.0f, 80.0f, 4);

        // Set mesh asset paths (from Oblivion ISO extracted meshes)
        izar->meshAssetPath = "meshes/creatures/imp.nif";
        hellas->meshAssetPath = "meshes/characters/imperial_male.nif";
        LOGI("NPC mesh paths set: Izar=%s, Hellas=%s",
             izar->meshAssetPath.c_str(), hellas->meshAssetPath.c_str());

        // Create test quests
        uint32_t quest1 = questManager->createQuest(izar->npcId, "Kill the Monster",
                                                    "Slay the beast terrorizing the area");
        uint32_t quest2 = questManager->createQuest(hellas->npcId, "Collect Items",
                                                    "Gather 5 crystals for the mage");

        if (quest1 != 0) {
            questManager->addObjective(quest1, "Defeat the monster", 1);
            QuestReward reward1;
            reward1.goldAmount = 100;
            reward1.experiencePoints = 150.0f;
            questManager->setQuestReward(quest1, reward1);
            izar->addQuestToOffer(quest1);
        }

        if (quest2 != 0) {
            questManager->addObjective(quest2, "Find crystals", 5);
            QuestReward reward2;
            reward2.goldAmount = 75;
            reward2.experiencePoints = 100.0f;
            questManager->setQuestReward(quest2, reward2);
            hellas->addQuestToOffer(quest2);
        }

        LOGI("Test quests created: Quest1=%u from Izar, Quest2=%u from Hellas",
             quest1, quest2);

        // Create test spells
        if (spellManager) {
            fireball = spellManager->createSpell(
                "Fireball", "ファイアボール",
                MagicSchool::DESTRUCTION, 50.0f, 30.0f);
            if (fireball != 0) {
                spellManager->addEffectToSpell(fireball,
                    SpellEffect(SpellEffectType::DAMAGE, 30.0f, 0.0f));
                spellManager->teachSpellToNpc(izar->npcId, fireball);
                spellManager->equipSpellToNpc(izar->npcId, fireball);
            }

            heal = spellManager->createSpell(
                "Heal", "ヒール",
                MagicSchool::RESTORATION, 40.0f, 0.0f);
            if (heal != 0) {
                spellManager->addEffectToSpell(heal,
                    SpellEffect(SpellEffectType::HEAL, 50.0f, 0.0f));
                spellManager->teachSpellToNpc(hellas->npcId, heal);
                spellManager->teachSpellToNpc(izar->npcId, heal);
                spellManager->equipSpellToNpc(hellas->npcId, heal);
                spellManager->equipSpellToNpc(izar->npcId, heal);
            }

            restoreMana = spellManager->createSpell(
                "Restore Mana", "マナ回復",
                MagicSchool::MYSTICISM, 30.0f, 0.0f);
            if (restoreMana != 0) {
                spellManager->addEffectToSpell(restoreMana,
                    SpellEffect(SpellEffectType::RESTORE_MANA, 40.0f, 0.0f));
                spellManager->teachSpellToNpc(izar->npcId, restoreMana);
                spellManager->equipSpellToNpc(izar->npcId, restoreMana);
            }

            LOGI("Test spells created: Fireball=%u, Heal=%u, RestoreMana=%u",
                 fireball, heal, restoreMana);
        }

        // Initiate test combat
        if (combatManager) {
            combatManager->initiateCombat(izar, hellas);
            LOGI("Combat initiated: Izar vs Hellas");
        }

        // Test spell casting
        LOGI("Testing spell casting...");
        if (fireball != 0) {
            LOGI("Casting Fireball (ID=%u) from Izar to Hellas", fireball);
            spellManager->castSpell(izar->npcId, fireball, hellas->npcId);
        }
        if (heal != 0) {
            LOGI("Casting Heal (ID=%u) from Hellas to self", heal);
            spellManager->castSpell(hellas->npcId, heal, hellas->npcId);
        }
        if (restoreMana != 0) {
            LOGI("Casting Restore Mana (ID=%u) from Izar to self", restoreMana);
            spellManager->castSpell(izar->npcId, restoreMana, izar->npcId);
        }
    }

    npcMgr->logNpcStatus();
    questManager->logQuestStatus();
    combatManager->logCombatStatus();
    if (spellManager) {
        spellManager->logSpellStatus();
    }

    LOGI("=== createTestScenario() END ===");
}