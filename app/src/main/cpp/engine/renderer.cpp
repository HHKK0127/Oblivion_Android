#include "renderer.h"
#include <thread>

Renderer::Renderer()
    : showTitleScreen(true), screenWidth(1080), screenHeight(1920),
      targetFPS(60), frameTimeThreshold(1000.0f / 60.0f) {
    LOGD("Renderer created with target FPS: %d", targetFPS);
    lastFrameTime = std::chrono::high_resolution_clock::now();
}

Renderer::~Renderer() {
    cleanup();
    LOGD("Renderer destroyed");
}

bool Renderer::init(unsigned int width, unsigned int height) {
    screenWidth = width;
    screenHeight = height;

    LOGI("Renderer initializing: %ux%u", screenWidth, screenHeight);

    // Initialize localization
    initLocalization();

    // Initialize game systems
    initGameSystems();

    // Create test scenario (combat, quests, etc.)
    createTestScenario();

    LOGI("Renderer initialized successfully");
    return true;
}

void Renderer::resize(unsigned int width, unsigned int height) {
    screenWidth = width;
    screenHeight = height;
    LOGD("Renderer resized to: %ux%u", screenWidth, screenHeight);
}

void Renderer::setTargetFPS(int fps) {
    if (fps <= 0) {
        LOGW("Invalid FPS value: %d, using 60 fps", fps);
        fps = 60;
    }

    targetFPS = fps;
    frameTimeThreshold = 1000.0f / fps;

    LOGI("Target FPS changed to: %d (%.2f ms per frame)", targetFPS, frameTimeThreshold);
}

void Renderer::initLocalization() {
    localizationManager = std::make_unique<LocalizationManager>();
    if (!localizationManager->initialize()) {
        LOGE("Failed to initialize LocalizationManager");
        return;
    }

    LOGI("LocalizationManager initialized");
    localizationManager->logTranslationStats();
}

void Renderer::initGameSystems() {
    // Initialize World Manager
    worldManager = std::make_unique<WorldManager>();
    if (!worldManager->initialize()) {
        LOGE("Failed to initialize WorldManager");
        return;
    }

    // Initialize Quest Manager
    questManager = std::make_unique<QuestManager>();
    if (!questManager->initialize(worldManager->getNpcManager())) {
        LOGE("Failed to initialize QuestManager");
        return;
    }

    // Initialize Spell Manager (before CombatManager)
    spellManager = std::make_unique<SpellManager>();
    if (!spellManager->initialize(worldManager->getNpcManager())) {
        LOGE("Failed to initialize SpellManager");
        return;
    }

    // Initialize Combat Manager (with SpellManager)
    combatManager = std::make_unique<CombatManager>();
    if (!combatManager->initialize(worldManager.get(), worldManager->getNpcManager(),
                                   spellManager.get())) {
        LOGE("Failed to initialize CombatManager");
        return;
    }

    // Initialize Performance Monitor
    performanceMonitor = std::make_unique<PerformanceMonitor>();
    performanceMonitor->initialize();
    LOGI("PerformanceMonitor initialized");

    // Initialize Save Manager
    saveManager = std::make_unique<SaveManager>();
    if (!saveManager->initialize()) {
        LOGE("Failed to initialize SaveManager");
    } else {
        LOGI("SaveManager initialized");
        // List available saves
        auto saves = saveManager->getSaveSlots();
        LOGI("Found %zu save slots", saves.size());
    }

    // Initialize Title Screen
    titleScreen = std::make_unique<TitleScreen>();
    titleScreen->initialize(localizationManager.get());

    // Initialize Quest UI
    questUI = std::make_unique<QuestUI>();
    questUI->initialize(questManager.get(), worldManager->getNpcManager(),
                        localizationManager.get());

    LOGI("All game systems initialized");
}

void Renderer::createTestScenario() {
    // Create test NPCs
    NpcManager* npcMgr = worldManager->getNpcManager();

    auto izar = npcMgr->createNPC("Izar", glm::vec3(0.0f, 0.0f, 0.0f));
    auto hellas = npcMgr->createNPC("Hellas", glm::vec3(5.0f, 0.0f, 0.0f));

    if (izar && hellas) {
        izar->status.initialize(150.0f, 100.0f, 5);
        hellas->status.initialize(120.0f, 80.0f, 4);

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
            // 破壊の魔法：ファイアボール
            uint32_t fireball = spellManager->createSpell(
                "Fireball", "ファイアボール",
                MagicSchool::DESTRUCTION, 50.0f, 30.0f);
            if (fireball != 0) {
                spellManager->addEffectToSpell(fireball,
                    SpellEffect(SpellEffectType::DAMAGE, 30.0f, 0.0f));
                spellManager->teachSpellToNpc(izar->npcId, fireball);
                spellManager->equipSpellToNpc(izar->npcId, fireball);
            }

            // 回復の魔法：ヒール
            uint32_t heal = spellManager->createSpell(
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

            // 神秘の魔法：マナ回復
            uint32_t restoreMana = spellManager->createSpell(
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
        if (spellManager) {
            LOGI("Testing spell casting...");
            // ファイアボールをキャスト
            spellManager->castSpell(izar->npcId, 2000, hellas->npcId);
            // ヒールをキャスト
            spellManager->castSpell(hellas->npcId, 2001, hellas->npcId);
        }
    }

    npcMgr->logNpcStatus();
    questManager->logQuestStatus();
    combatManager->logCombatStatus();
    if (spellManager) {
        spellManager->logSpellStatus();
    }
}

void Renderer::render(float deltaTime) {
    // Begin performance monitoring
    if (performanceMonitor) {
        performanceMonitor->beginFrame();
    }

    // Update Title Screen
    if (showTitleScreen) {
        titleScreen->update(deltaTime);
        titleScreen->render();

        if (titleScreen->isGameStarted()) {
            showTitleScreen = false;
            LOGI("Title screen closed, starting main game");
        }
        if (performanceMonitor) {
            performanceMonitor->endFrame();
        }
        return;  // Skip game rendering while title screen is active
    }

    // Update game systems
    if (worldManager) {
        worldManager->update(deltaTime);
    }

    if (questManager) {
        questManager->update(deltaTime);
    }

    if (combatManager) {
        combatManager->update(deltaTime);
    }

    if (spellManager) {
        spellManager->update(deltaTime);
    }

    // Render UI
    if (questUI) {
        questUI->render();
    }

    // End performance monitoring
    if (performanceMonitor) {
        performanceMonitor->endFrame();

        // Log performance metrics every 300 frames (5 seconds at 60fps)
        static int frameCounter = 0;
        frameCounter++;
        if (frameCounter >= 300) {
            performanceMonitor->logPerformanceReport();
            frameCounter = 0;
        }
    }

    // Frame rate control - enforce target FPS
    auto currentFrameTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float, std::milli> frameElapsed = currentFrameTime - lastFrameTime;
    float elapsedMs = frameElapsed.count();

    if (elapsedMs < frameTimeThreshold) {
        // Sleep to maintain target FPS
        float sleepTimeMs = frameTimeThreshold - elapsedMs;
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<long>(sleepTimeMs)));
    }

    lastFrameTime = std::chrono::high_resolution_clock::now();

    LOGD("Frame rendered: deltaTime=%.3f, FPS=%.1f, Target FPS=%d",
         deltaTime, performanceMonitor ? performanceMonitor->getFPS() : 0.0f, targetFPS);
}

void Renderer::cleanup() {
    LOGI("Renderer cleaning up");

    if (performanceMonitor) {
        performanceMonitor->logDetailedMetrics();
    }

    if (questUI) {
        questUI->cleanup();
    }

    if (questManager) {
        questManager->cleanup();
    }

    if (combatManager) {
        combatManager->cleanup();
    }

    if (spellManager) {
        spellManager->cleanup();
    }

    if (worldManager) {
        worldManager->cleanup();
    }

    if (localizationManager) {
        localizationManager->cleanup();
    }

    LOGD("Renderer cleaned up");
}

bool Renderer::saveGameState(const std::string& slotName) {
    if (!saveManager) {
        LOGE("SaveManager not initialized");
        return false;
    }

    // Create current game state
    GameState state;
    state.saveName = slotName;
    state.saveTimestamp = std::time(nullptr);

    // Capture world state
    if (worldManager) {
        state.playerPosition = glm::vec3(0.0f, 10.0f, 0.0f);  // Default player position

        // Capture NPC states
        NpcManager* npcMgr = worldManager->getNpcManager();
        if (npcMgr) {
            auto allNpcs = npcMgr->getAllNPCs();
            for (const auto& npc : allNpcs) {
                if (npc) {
                    state.npcStates[npc->npcId] =
                        std::make_pair(npc->position, npc->status);
                }
            }
        }
    }

    // Capture quest states
    if (questManager) {
        auto activeQuests = questManager->getActiveQuests();
        for (const auto& quest : activeQuests) {
            if (quest) {
                state.questStates[quest->questId] = static_cast<int>(quest->state);
            }
        }
    }

    // Save to file
    bool success = saveManager->saveGame(slotName, state);
    if (success) {
        LOGI("Game saved to slot: %s", slotName.c_str());
    } else {
        LOGE("Failed to save game to slot: %s", slotName.c_str());
    }
    return success;
}

bool Renderer::loadGameState(const std::string& slotName) {
    if (!saveManager) {
        LOGE("SaveManager not initialized");
        return false;
    }

    GameState state;
    bool success = saveManager->loadGame(slotName, state);

    if (!success) {
        LOGE("Failed to load game from slot: %s", slotName.c_str());
        return false;
    }

    // Restore NPC states
    if (worldManager && !state.npcStates.empty()) {
        NpcManager* npcMgr = worldManager->getNpcManager();
        if (npcMgr) {
            for (const auto& [npcId, positionStatus] : state.npcStates) {
                auto npc = npcMgr->getNPC(npcId);
                if (npc) {
                    npc->position = positionStatus.first;
                    npc->status = positionStatus.second;
                }
            }
        }
    }

    // Restore quest states
    if (questManager && !state.questStates.empty()) {
        for (const auto& [questId, questState] : state.questStates) {
            auto quest = questManager->getQuest(questId);
            if (quest) {
                quest->state = static_cast<QuestState>(questState);
            }
        }
    }

    LOGI("Game loaded from slot: %s", slotName.c_str());
    return true;
}
