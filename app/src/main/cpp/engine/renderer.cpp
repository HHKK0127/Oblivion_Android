#include "renderer.h"
// #include "../jni_audio_bridge.h"  // Deferred - requires Java MainActivity
#include <thread>

Renderer::Renderer()
    : showTitleScreen(false), screenWidth(1080), screenHeight(1920),
      targetFPS(60), frameTimeThreshold(1000.0f / 60.0f) {
    LOGD("Renderer created with target FPS: %d", targetFPS);
    lastFrameTime = std::chrono::high_resolution_clock::now();
}

Renderer::~Renderer() {
    cleanup();
    LOGD("Renderer destroyed");
}

bool Renderer::init(unsigned int width, unsigned int height) {
    LOGI("===== Renderer::init() START with %ux%u =====", width, height);
    initialized = false;  // Reset initialization flag

    screenWidth = width;
    screenHeight = height;

    LOGI("Renderer initializing: %ux%u", screenWidth, screenHeight);

    try {
        // Initialize localization
        LOGI("Step 1: Calling initLocalization()");
        initLocalization();
        LOGI("Step 1: initLocalization() completed");

        // Initialize game systems
        LOGI("Step 2: Calling initGameSystems()");
        initGameSystems();
        LOGI("Step 2: initGameSystems() completed");

        // Initialize retro filter (post-processing)
        LOGI("Step 3: Initializing RetroFilter");
        retroFilter = std::make_unique<RetroFilter>();
        if (!retroFilter->initialize(screenWidth, screenHeight)) {
            LOGE("Failed to initialize RetroFilter");
            return false;
        }
        LOGI("Step 3: RetroFilter initialized");

        // Create test scenario (combat, quests, etc.)
        LOGI("Step 4: Calling createTestScenario()");
        createTestScenario();
        LOGI("Step 4: createTestScenario() completed");

        initialized = true;  // Mark as successfully initialized
        LOGI("===== Renderer initialized successfully =====");
        return true;
    } catch (const std::exception& e) {
        LOGE("CRITICAL: Exception during Renderer::init(): %s", e.what());
        initialized = false;
        return false;
    } catch (...) {
        LOGE("CRITICAL: Unknown exception during Renderer::init()");
        initialized = false;
        return false;
    }
}

void Renderer::resize(unsigned int width, unsigned int height) {
    LOGI("===== Renderer::resize() called with %ux%u =====", width, height);
    screenWidth = width;
    screenHeight = height;
    LOGI("Renderer resized to: %ux%u", screenWidth, screenHeight);

    // Update TextRenderer with new dimensions - CRITICAL for correct projection
    if (textRenderer) {
        LOGI("TextRenderer exists, calling setScreenSize(%u, %u)", screenWidth, screenHeight);
        textRenderer->setScreenSize(screenWidth, screenHeight);
        LOGI("TextRenderer screen size updated to: %ux%u", screenWidth, screenHeight);
    } else {
        LOGW("WARNING: TextRenderer is NULL in resize()! Dimensions not updated!");
    }

    // Update RetroFilter resolution
    if (retroFilter) {
        retroFilter->setNativeResolution(screenWidth, screenHeight);
        LOGI("RetroFilter resolution updated to: %ux%u", screenWidth, screenHeight);
    }
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
    LOGI("=== initGameSystems() called ===");

    // CRITICAL: Get actual viewport dimensions from OpenGL (not JNI parameters)
    // This handles timing issues where render thread starts before onSurfaceChanged
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    unsigned int actualWidth = viewport[2];
    unsigned int actualHeight = viewport[3];
    LOGI("Actual OpenGL viewport: %ux%u", actualWidth, actualHeight);

    if (actualWidth > 0 && actualHeight > 0) {
        screenWidth = actualWidth;
        screenHeight = actualHeight;
        LOGI("Using actual viewport dimensions: %ux%u (instead of init %ux%u)",
             screenWidth, screenHeight,
             ((int)1920), ((int)1080));  // These are hardcoded init values for comparison
    }

    // Initialize Settings Manager
    LOGI("Creating SettingsManager...");
    settingsManager = std::make_unique<SettingsManager>();
    if (!settingsManager->initialize()) {
        LOGE("Failed to initialize SettingsManager");
        return;
    }
    LOGI("SettingsManager initialized successfully");

    // Initialize Text Renderer (for debug HUD and settings UI)
    LOGI("Creating TextRenderer...");
    textRenderer = std::make_unique<TextRenderer>();
    if (!textRenderer->initialize()) {
        LOGE("Failed to initialize TextRenderer");
        return;
    }
    textRenderer->setScreenSize(screenWidth, screenHeight);
    LOGI("TextRenderer initialized successfully with size %ux%u", screenWidth, screenHeight);

    // Initialize Debug HUD
    LOGI("Creating DebugHUD...");
    debugHUD = std::make_unique<DebugHUD>();
    if (!debugHUD->initialize(textRenderer.get())) {
        LOGE("Failed to initialize DebugHUD");
        return;
    }
    LOGI("DebugHUD initialized successfully");

    // Initialize Settings UI
    LOGI("Creating SettingsUI...");
    settingsUI = std::make_unique<SettingsUI>();
    if (!settingsUI->initialize(textRenderer.get(), settingsManager.get())) {
        LOGE("Failed to initialize SettingsUI");
        return;
    }
    LOGI("SettingsUI initialized successfully");

    // Initialize Asset Manager (before WorldManager)
    LOGI("Creating AssetManager...");
    assetManager = std::make_unique<AssetManager>();
    if (!assetManager->initialize()) {
        LOGE("Failed to initialize AssetManager");
        return;
    }
    LOGI("AssetManager initialized successfully");

    // Initialize NPC Manager (before WorldManager)
    LOGI("Creating NpcManager...");
    npcManager = std::make_unique<NpcManager>();
    if (!npcManager->initialize()) {
        LOGE("Failed to initialize NpcManager");
        return;
    }
    LOGI("NpcManager initialized successfully");

    // Initialize World Manager
    LOGI("Creating WorldManager...");
    worldManager = std::make_unique<WorldManager>();
    LOGI("Calling WorldManager::initialize() with managers...");
    if (!worldManager->initialize(npcManager.get(), assetManager.get())) {
        LOGE("Failed to initialize WorldManager");
        return;
    }
    LOGI("WorldManager initialized successfully");

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

    // Initialize PlayerController (Phase 3+)
    playerController = std::make_unique<PlayerController>();
    if (!playerController->initialize(worldManager.get())) {
        LOGE("Failed to initialize PlayerController");
        return;
    }
    LOGI("PlayerController initialized successfully");

    // Initialize InventoryManager (Phase 3+)
    inventoryManager = std::make_unique<InventoryManager>();
    if (!inventoryManager->initialize()) {
        LOGE("Failed to initialize InventoryManager");
        return;
    }
    LOGI("InventoryManager initialized successfully");

    // Initialize InventoryUI (Phase 3+)
    inventoryUI = std::make_unique<InventoryUI>();
    if (!inventoryUI->initialize(inventoryManager->getPlayerInventory(), textRenderer.get())) {
        LOGE("Failed to initialize InventoryUI");
        return;
    }
    LOGI("InventoryUI initialized successfully");

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

    // Initialize Audio via JNI Bridge (Java MediaPlayer)
    // Using pragmatic approach: Android's native MediaPlayer instead of OpenAL
    // NOTE: Audio system deferred - JNI bridge requires Java MainActivity class
    // which is not available in pure NativeActivity context
    // LOGI("Initializing audio system via JNI bridge...");
    // LOGI("Playing Oblivion test BGM: explore.mp3");
    // jni_audio_play_bgm("explore.mp3");
    // LOGI("Audio playback initiated via JNI bridge");

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
    LOGI("=== createTestScenario() START ===");

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
        // These are relative paths that will be resolved by AssetManager
        izar->meshAssetPath = "meshes/creatures/imp.nif";  // Monster model
        hellas->meshAssetPath = "meshes/characters/imperial_male.nif";  // NPC model
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
            // 破壊の魔法：ファイアボール
            fireball = spellManager->createSpell(
                "Fireball", "ファイアボール",
                MagicSchool::DESTRUCTION, 50.0f, 30.0f);
            if (fireball != 0) {
                spellManager->addEffectToSpell(fireball,
                    SpellEffect(SpellEffectType::DAMAGE, 30.0f, 0.0f));
                spellManager->teachSpellToNpc(izar->npcId, fireball);
                spellManager->equipSpellToNpc(izar->npcId, fireball);
            }

            // 回復の魔法：ヒール
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

            // 神秘の魔法：マナ回復
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

        // Test spell casting - FIX: Use correct spell IDs instead of hardcoded values
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

        // Check if Settings was requested
        if (titleScreen->isSettingsRequested()) {
            titleScreen->resetSettingsRequest();
            if (settingsUI) {
                settingsUI->toggle();  // Open settings UI
                LOGI("Settings UI opened from title screen menu");
            }
        }

        if (titleScreen->isGameStarted()) {
            showTitleScreen = false;
            LOGI("Title screen closed, starting main game");
        }
        // Skip frame rate control for quick return
        if (performanceMonitor) {
            performanceMonitor->endFrame();
        }
        return;  // Skip game rendering while title screen is active
    }

    // Update game systems
    if (worldManager) {
        worldManager->update(deltaTime);
    }

    // Update player controller
    if (playerController) {
        playerController->update(deltaTime);
    }

    // Update inventory manager
    if (inventoryManager) {
        inventoryManager->update(deltaTime);
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

    // Update Audio Manager (listener position tracking)
#ifdef AUDIO_SYSTEM_ENABLED
    if (audioManager && worldManager) {
        // Get camera position from world manager and set as audio listener position
        const glm::vec3& cameraPos = worldManager->getCameraPosition();
        const glm::vec3& cameraForward = worldManager->getCameraForward();
        const glm::vec3& cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);  // Standard up vector

        audioManager->setListenerPosition(cameraPos);
        audioManager->setListenerOrientation(cameraForward, cameraUp);
        audioManager->update(deltaTime);
    }
#endif

    // ===== RETRO FILTER: Bind scene framebuffer for rendering =====
    if (retroFilter) {
        retroFilter->bindSceneFramebuffer();
    }

    // Render World (main game scene) - Clear with game background color
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);  // Dark gray for game screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Enable depth testing for proper face rendering
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // Load and bind NPC meshes via AssetManager
    if (assetManager && worldManager) {
        NpcManager* npcMgr = worldManager->getNpcManager();
        if (npcMgr) {
            auto allNpcs = npcMgr->getAllNPCs();
            for (const auto& npc : allNpcs) {
                if (npc) {
                    // Load mesh if not already loaded and asset path is set
                    if (!npc->mesh && !npc->meshAssetPath.empty()) {
                        npc->mesh = assetManager->loadNifMesh(npc->meshAssetPath);
                        if (npc->mesh) {
                            LOGD("Loaded mesh for NPC %u: %s", npc->npcId, npc->meshAssetPath.c_str());
                        } else {
                            LOGW("Failed to load mesh for NPC %u: %s", npc->npcId, npc->meshAssetPath.c_str());
                        }
                    }

                    // Update model matrix every frame
                    npc->updateModelMatrix();
                }
            }
        }
    }

    // Safety check: if initialization failed, don't try to render
    if (!initialized) {
        static int nullRenderCount = 0;
        if (nullRenderCount % 60 == 0) {  // Log every 60 frames (~1 second at 60 FPS)
            LOGE("CRITICAL: render() called but Renderer is not initialized! worldManager=%p debugHUD=%p",
                 worldManager.get(), debugHUD.get());
        }
        nullRenderCount++;
        // Just clear the screen and return to prevent crash
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        return;
    }

    // Render world objects
    if (worldManager) {
        LOGD("Calling worldManager->render()");
        worldManager->render();
        LOGD("worldManager->render() completed");
    } else {
        LOGW("worldManager is null!");
    }

    // Render UI
    if (questUI) {
        questUI->render();
    }

    // Render Inventory UI
    if (inventoryUI && inventoryUI->isVisible()) {
        inventoryUI->render();
    }

    // Update and render Debug HUD (always enabled for text rendering testing)
    if (debugHUD) {
        LOGD("About to call debugHUD->update() and render()");
        // DeltaTime is in milliseconds from the JNI layer
        debugHUD->update(deltaTime);
        debugHUD->render();
        LOGD("debugHUD->render() completed");
    } else {
        LOGD("debugHUD is null!");
    }

    // Render Settings UI if visible
    if (settingsUI && settingsUI->isVisible()) {
        settingsUI->render();
    }

    // ===== RETRO FILTER: Apply post-processing effects and render to screen =====
    if (retroFilter) {
        retroFilter->apply(retroSettings);
        retroFilter->renderToScreen();
    }

    // Frame rate control - enforce target FPS
    auto currentFrameTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float, std::milli> frameElapsed = currentFrameTime - lastFrameTime;
    float elapsedMs = frameElapsed.count();

    if (elapsedMs < frameTimeThreshold) {
        // Sleep to maintain target FPS (with microsecond precision)
        float sleepTimeMs = frameTimeThreshold - elapsedMs;
        auto sleepDuration = std::chrono::microseconds(static_cast<long long>(sleepTimeMs * 1000.0f));
        std::this_thread::sleep_for(sleepDuration);
        lastFrameTime = std::chrono::high_resolution_clock::now();
    } else {
        lastFrameTime = currentFrameTime;
    }

    // End performance monitoring (after frame limiting)
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

    LOGD("Frame rendered: deltaTime=%.3f, FPS=%.1f, Target FPS=%d",
         deltaTime, performanceMonitor ? performanceMonitor->getFPS() : 0.0f, targetFPS);
}

void Renderer::onTouchEvent(float x, float y) {
    LOGI("=== タッチイベント検出 === 座標: (%.1f, %.1f)", x, y);

    // Settings UI has highest priority
    if (settingsUI && settingsUI->isVisible()) {
        settingsUI->onTouchEvent(x, y);
        LOGI("タッチイベント → SettingsUI");

        // Check if should return to menu
        if (settingsUI->shouldReturnToMenu()) {
            settingsUI->resetReturnFlag();
            settingsUI->toggle();  // Close settings
        }
        return;
    }

    // Inventory UI has next priority (Phase 3+)
    if (inventoryUI && inventoryUI->isVisible()) {
        inventoryUI->onTouchEvent(x, y);
        LOGI("タッチイベント → InventoryUI");
        return;
    }

    // Pass touch event to title screen
    if (showTitleScreen && titleScreen) {
        titleScreen->onTouchEvent(x, y);
        LOGI("タッチイベント → TitleScreen");
        return;
    }

    // In-game touch handling - detect NPC/object interactions or camera control
    if (!showTitleScreen && worldManager) {
        LOGI("ゲーム内タッチ検出 - NPCインタラクション確認中");

        // Log world state
        if (worldManager) {
            LOGI("ワールド座標系でのタッチ位置確認: スクリーン(%.1f, %.1f)", x, y);
        }

        // Pass to quest UI
        if (questUI) {
            questUI->onTouchEvent(x, y);
            LOGI("タッチイベント → QuestUI");
        }

        // Pass to player controller for camera rotation (Phase 3+)
        if (playerController) {
            playerController->onTouchInput(x, y);
            LOGI("タッチイベント → PlayerController (カメラ回転)");
        }

        // Log combat/NPC state
        if (combatManager) {
            LOGI("戦闘マネージャー: アクティブ戦闘確認中");
        }

        // Log interaction attempt
        LOGI("NPC相互作用試行: 画面座標 (%.1f, %.1f)", x, y);
    } else {
        LOGW("ゲーム状態不明 - タイトル画面状態: %d, WorldManager: %p",
             showTitleScreen, worldManager.get());
    }
}

void Renderer::cleanup() {
    LOGI("Renderer cleaning up");

    if (retroFilter) {
        retroFilter->cleanup();
        retroFilter = nullptr;
    }

    if (performanceMonitor) {
        performanceMonitor->logDetailedMetrics();
    }

    if (debugHUD) {
        debugHUD->cleanup();
    }

    if (settingsUI) {
        settingsUI->cleanup();
    }

    if (inventoryUI) {
        inventoryUI = nullptr;
    }

    if (textRenderer) {
        textRenderer->cleanup();
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

    if (playerController) {
        playerController->cleanup();
        playerController = nullptr;
    }

    if (inventoryManager) {
        inventoryManager->cleanup();
        inventoryManager = nullptr;
    }

    if (assetManager) {
        assetManager->cleanup();
    }

    if (worldManager) {
        worldManager->cleanup();
    }

    if (localizationManager) {
        localizationManager->cleanup();
    }

#ifdef AUDIO_SYSTEM_ENABLED
    if (audioManager) {
        audioManager->cleanup();
    }
#endif

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
