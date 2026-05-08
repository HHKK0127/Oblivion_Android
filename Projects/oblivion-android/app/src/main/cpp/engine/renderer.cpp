#include "renderer.h"
// #include "../jni_audio_bridge.h"  // Deferred - requires Java MainActivity
#include <thread>
#include <ctime>
#include <nlohmann/json.hpp>
#include "../save_system/save_system.h"
#include "../game/test_scenario.h"

using json = nlohmann::json;

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
    screenWidth = width;
    screenHeight = height;

    LOGI("Renderer initializing: %ux%u", screenWidth, screenHeight);

    // Initialize localization
    initLocalization();

    // Initialize game systems
    initGameSystems();

    // Initialize retro filter
    retroFilter = std::make_unique<RetroFilter>();
    if (!retroFilter->initialize(screenWidth, screenHeight)) {
        LOGW("Failed to initialize retro filter");
        retroFilter.reset();
    }
    retroTime = 0.0f;

    // Create test scenario (combat, quests, etc.)
    createTestScenario(this);

    LOGI("Renderer initialized successfully");
    return true;
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
    if (!settingsUI->initialize(textRenderer.get(), settingsManager.get(), this)) {
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

    // Initialize Performance Monitor
    performanceMonitor = std::make_unique<PerformanceMonitor>();
    performanceMonitor->initialize();
    LOGI("PerformanceMonitor initialized");

    // Initialize Game Manager
    gameManager = std::make_unique<GameManager>();
    gameManager->initialize();
    LOGI("GameManager initialized");

    // Set up auto-save on cell change
    if (worldManager) {
        worldManager->onCellChangedCallback = [this]() {
            if (gameManager) {
                LOGI("Cell changed - triggering auto-save");
                gameManager->createAutoSave();
            }
        };
    }

    // Initialize Save Manager (legacy - now managed by GameManager)
    saveManager = std::make_unique<SaveSystem::SaveManager>();
    if (!saveManager->initialize()) {
        LOGE("Failed to initialize SaveManager");
    } else {
        LOGI("SaveManager initialized");
        auto saves = saveManager->getAllSaveSlots();
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

    // Bind retro filter FBO if enabled
    if (retroFilter && retroSettings.enabled) {
        retroFilter->bindSceneFramebuffer();
        retroTime += deltaTime * 0.001f;
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

    // Apply retro filter and render to screen if enabled
    if (retroFilter && retroSettings.enabled) {
        retroFilter->apply(retroSettings, retroTime);
        retroFilter->renderToScreen();
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

    // Pass touch event to title screen
    if (showTitleScreen && titleScreen) {
        titleScreen->onTouchEvent(x, y);
        LOGI("タッチイベント → TitleScreen");
        return;
    }

    // In-game touch handling - detect NPC/object interactions
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

    if (performanceMonitor) {
        performanceMonitor->logDetailedMetrics();
    }

    if (debugHUD) {
        debugHUD->cleanup();
    }

    if (settingsUI) {
        settingsUI->cleanup();
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

    if (assetManager) {
        assetManager->cleanup();
    }

    if (worldManager) {
        worldManager->cleanup();
    }

    if (gameManager) {
        gameManager->cleanup();
    }

    if (retroFilter) {
        retroFilter->cleanup();
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

bool Renderer::saveGameState(int slotId) {
    if (!gameManager) {
        LOGE("GameManager not initialized");
        return false;
    }
    bool success = gameManager->saveGame(slotId, "");
    if (success) {
        LOGI("Game saved to slot: %d", slotId);
    } else {
        LOGE("Failed to save game to slot: %d", slotId);
    }
    return success;
}

bool Renderer::loadGameState(int slotId) {
    if (!gameManager) {
        LOGE("GameManager not initialized");
        return false;
    }
    bool success = gameManager->loadGame(slotId);
    if (success) {
        LOGI("Game loaded from slot: %d", slotId);
    } else {
        LOGE("Failed to load game from slot: %d", slotId);
    }
    return success;
}
