#include "renderer.h"
#include "skinning_shader.h"
#include "texture_loader.h"
#include "imperial_weave.h"
#include "../ui/ui_draw_helper.h"
#include "../assets/bsa_reader.h"
#include "../inventory/item_factory.h"
// #include "../jni_audio_bridge.h"  // Deferred - requires Java MainActivity
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
    LOGI("===== Renderer::init() START with %ux%u =====", width, height);
    __android_log_print(ANDROID_LOG_ERROR, "Renderer", "SYNC_CHECKPOINT_1: init() called");
    initialized = false;  // Reset initialization flag

    screenWidth = width;
    screenHeight = height;

    LOGI("Renderer initializing: %ux%u", screenWidth, screenHeight);

    try {
        // Initialize localization
        LOGI("Step 1: Calling initLocalization()");
        initLocalization();
        LOGI("Step 1: initLocalization() completed");
        __android_log_print(ANDROID_LOG_ERROR, "Renderer", "SYNC_CHECKPOINT_2: localization done");

        // Initialize game systems
        LOGI("Step 2: Calling initGameSystems()");
        initGameSystems();
        LOGI("Step 2: initGameSystems() completed");
        __android_log_print(ANDROID_LOG_ERROR, "Renderer", "SYNC_CHECKPOINT_3: game systems done");

        // Initialize retro filter (post-processing)
        LOGI("Step 3: Initializing RetroFilter");
        retroFilter = std::make_unique<RetroFilter>();
        if (!retroFilter->initialize(screenWidth, screenHeight)) {
            LOGE("Failed to initialize RetroFilter");
            __android_log_print(ANDROID_LOG_ERROR, "Renderer", "ERROR_RETROFILTER: initialization failed");
            return false;
        }
        LOGI("Step 3: RetroFilter initialized");
        __android_log_print(ANDROID_LOG_ERROR, "Renderer", "SYNC_CHECKPOINT_4: retrofilter done");

        // Create test scenario (combat, quests, etc.)
        LOGI("Step 4: Calling createTestScenario()");
        createTestScenario();
        LOGI("Step 4: createTestScenario() completed");
        __android_log_print(ANDROID_LOG_ERROR, "Renderer", "SYNC_CHECKPOINT_5: test scenario done");

        initialized = true;  // Mark as successfully initialized
        LOGI("===== Renderer initialized successfully =====");
        __android_log_print(ANDROID_LOG_ERROR, "Renderer", "SYNC_CHECKPOINT_FINAL: SUCCESS");
        return true;
    } catch (const std::exception& e) {
        LOGE("CRITICAL: Exception during Renderer::init(): %s", e.what());
        __android_log_print(ANDROID_LOG_ERROR, "Renderer", "EXCEPTION_CAUGHT: %s", e.what());
        initialized = false;
        return false;
    } catch (...) {
        LOGE("CRITICAL: Unknown exception during Renderer::init()");
        __android_log_print(ANDROID_LOG_ERROR, "Renderer", "UNKNOWN_EXCEPTION");
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

    // Update Phase 9 UI Framework screen size
    if (uiSystem) {
        uiSystem->setScreenSize(static_cast<int>(screenWidth), static_cast<int>(screenHeight));
        LOGI("UISystem screen size updated to: %ux%u", screenWidth, screenHeight);
        
        // Setup joystick position based on new screen size
        if (!joystick) {
            joystick = std::make_shared<UIJoystick>(250.0f, screenHeight - 250.0f, 150.0f);
            uiSystem->registerComponent(joystick, 100); // Draw above other components if overlapping
        } else {
            // Reposition existing joystick
            // Note: Since we don't have a direct setter yet, we can recreate it or add a setPosition method
            uiSystem->unregisterComponent(joystick);
            joystick = std::make_shared<UIJoystick>(250.0f, screenHeight - 250.0f, 150.0f);
            uiSystem->registerComponent(joystick, 100);
        }

        // Setup combat buttons on right side of screen
        float btnSize = 120.0f;
        float btnMargin = 20.0f;
        float btnX = screenWidth - btnSize - btnMargin;

        // Attack button (bottom-right)
        if (!attackButton) {
            attackButton = std::make_shared<UIButton>("AttackButton");
            attackButton->setPosition(btnX, screenHeight - btnSize - btnMargin);
            attackButton->setSize(btnSize, btnSize);
            attackButton->setLabel("ATK");
            attackButton->setLabelScale(0.8f);
            attackButton->setNormalColor(glm::vec4(0.8f, 0.2f, 0.2f, 0.7f));
            attackButton->setPressedColor(glm::vec4(1.0f, 0.1f, 0.1f, 0.9f));
            attackButton->setTextRenderer(textRenderer.get());
            attackButton->setOnClick([this]() {
                if (playerController) {
                    playerController->attack();
                }
                // Find nearest enemy and attack
                if (combatManager && worldManager) {
                    glm::vec3 playerPos = worldManager->getPlayerPosition();
                    auto nearestEnemy = combatManager->findNearestEnemyToPlayer(playerPos, 30.0f);
                    if (nearestEnemy) {
                        // Attack the nearest enemy (player ID = 1, weapon ID = 0 for unarmed)
                        combatManager->playerAttack(1, nearestEnemy->npcId, 0);
                        LOGD("Attacking nearest enemy: %s", nearestEnemy->name.c_str());
                    } else {
                        LOGD("No enemy in range");
                    }
                }
            });
            attackButton->setVisible(false); // Hidden until game starts
            uiSystem->registerComponent(attackButton, 100);
        }

        // Block button (above attack)
        if (!blockButton) {
            blockButton = std::make_shared<UIButton>("BlockButton");
            blockButton->setPosition(btnX, screenHeight - btnSize * 2 - btnMargin * 2);
            blockButton->setSize(btnSize, btnSize);
            blockButton->setLabel("BLK");
            blockButton->setLabelScale(0.8f);
            blockButton->setNormalColor(glm::vec4(0.2f, 0.4f, 0.8f, 0.7f));
            blockButton->setPressedColor(glm::vec4(0.3f, 0.5f, 1.0f, 0.9f));
            blockButton->setTextRenderer(textRenderer.get());
            blockButton->setOnClick([this]() {
                if (playerController && combatManager) {
                    // Toggle combat stance for blocking
                    playerController->toggleCombatStance();
                }
            });
            blockButton->setVisible(false); // Hidden until game starts
            uiSystem->registerComponent(blockButton, 100);
        }

        // Cast spell button (above block)
        if (!castSpellButton) {
            castSpellButton = std::make_shared<UIButton>("CastSpellButton");
            castSpellButton->setPosition(btnX, screenHeight - btnSize * 3 - btnMargin * 3);
            castSpellButton->setSize(btnSize, btnSize);
            castSpellButton->setLabel("MAG");
            castSpellButton->setLabelScale(0.8f);
            castSpellButton->setNormalColor(glm::vec4(0.6f, 0.2f, 0.8f, 0.7f));
            castSpellButton->setPressedColor(glm::vec4(0.8f, 0.3f, 1.0f, 0.9f));
            castSpellButton->setTextRenderer(textRenderer.get());
            castSpellButton->setOnClick([this]() {
                // Cast spell using SpellManager
                if (spellManager && combatManager) {
                    LOGD("Cast spell button pressed");

                    // If no spell selected, show spell selection panel
                    if (!selectedSpell) {
                        // Get player's known spells (player ID = 1)
                        auto playerSpells = spellManager->getNpcEquippedSpells(1);
                        if (playerSpells.empty()) {
                            playerSpells = spellManager->getNpcSpells(1);
                        }

                        if (playerSpells.empty()) {
                            LOGD("No spells available for player");
                            return;
                        }

                        if (spellSelectionPanel) {
                            spellSelectionPanel->setSpells(playerSpells);
                            spellSelectionPanel->setVisible(true);
                            LOGD("Opened spell selection panel with %zu spells", playerSpells.size());
                        }
                        return;
                    }

                    // Find nearest enemy target
                    auto enemy = combatManager->findNearestEnemyToPlayer(worldManager->getPlayerPosition());
                    uint32_t targetId = enemy ? enemy->npcId : 0;

                    // Cast selected spell (player ID = 1)
                    bool success = spellManager->castSpell(1, selectedSpell->spellId, targetId);
                    LOGD("Cast spell %s: %s", selectedSpell->name.c_str(), success ? "success" : "failed");

                    // Play magic sound effect
                    if (audioManager && success) {
                        audioManager->playSound("magic/spell_equip", worldManager->getPlayerPosition());
                    }
                } else {
                    LOGD("Cast spell button pressed - SpellManager not available");
                }
            });
            castSpellButton->setVisible(false); // Hidden until game starts
            uiSystem->registerComponent(castSpellButton, 100);
        }

        // Quick-slot buttons (bottom-left, above joystick)
        // 4 slots side by side: [F1][F2][F3][F4]
        float slotSize = 80.0f;
        float slotMargin = 8.0f;
        float slotStartX = 30.0f;
        float slotY = screenHeight - slotSize - 20.0f;

        for (int i = 0; i < QUICK_SLOT_COUNT; i++) {
            if (!quickSlotButtons[i]) {
                float slotX = slotStartX + i * (slotSize + slotMargin);
                std::string slotName = "QuickSlot" + std::to_string(i + 1);
                auto btn = std::make_shared<UIButton>(slotName);
                btn->setPosition(slotX, slotY);
                btn->setSize(slotSize, slotSize);
                btn->setLabel("F" + std::to_string(i + 1));
                btn->setLabelScale(0.6f);
                btn->setNormalColor(glm::vec4(0.2f, 0.2f, 0.3f, 0.6f));
                btn->setPressedColor(glm::vec4(0.4f, 0.4f, 0.6f, 0.9f));
                btn->setTextRenderer(textRenderer.get());

                // Capture slot index
                btn->setOnClick([this, i]() {
                    if (!playerController || !playerController->getPlayer()) return;
                    auto& player = *playerController->getPlayer();
                    auto spell = player.quickSlotSpells[i];
                    if (!spell) {
                        // Open spell panel to assign to this slot
                        pendingAssignSlot = i;
                        if (spellSelectionPanel && spellManager) {
                            auto spells = spellManager->getNpcEquippedSpells(1);
                            if (spells.empty()) spells = spellManager->getNpcSpells(1);
                            spellSelectionPanel->setSpells(spells);
                            spellSelectionPanel->setVisible(true);
                        }
                    } else {
                        // Cast spell in this slot
                        selectedSpell = spell;
                        auto enemies = npcManager ? npcManager->getNPCsInArea(
                            playerController->getPlayer()->position, 20.0f) : std::vector<std::shared_ptr<NPC>>{};
                        uint32_t targetId = 0;
                        for (auto& e : enemies) {
                            if (e && e->npcId != 1) { targetId = e->npcId; break; }
                        }
                        if (spellManager && targetId != 0) {
                            spellManager->castSpell(1, targetId, spell->spellId);
                        }
                        if (audioManager) {
                            audioManager->playSound("magic/spell_equip");
                        }
                    }
                });

                btn->setVisible(false);
                uiSystem->registerComponent(btn, 90);
                quickSlotButtons[i] = btn;
            }
        }
    }

    // Update TitleScreen layout
    if (titleScreen) {
        titleScreen->setScreenSize(static_cast<int>(screenWidth), static_cast<int>(screenHeight));
        LOGI("TitleScreen screen size updated to: %ux%u", screenWidth, screenHeight);
    }

    // Update SettingsUI layout
    if (settingsUI) {
        settingsUI->setScreenSize(static_cast<int>(screenWidth), static_cast<int>(screenHeight));
        LOGI("SettingsUI screen size updated to: %ux%u", screenWidth, screenHeight);
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
    if (!g_assetManager) {
        LOGE("g_assetManager is null, cannot initialize TextRenderer");
        return;
    }
    if (!textRenderer->initialize(g_assetManager)) {
        LOGE("Failed to initialize TextRenderer");
        return;
    }
    textRenderer->setScreenSize(screenWidth, screenHeight);
    LOGI("TextRenderer initialized successfully with size %ux%u", screenWidth, screenHeight);

    // Initialize Phase 9 UI Framework System
    LOGI("Creating UISystem...");
    uiSystem = std::make_unique<UISystem>();
    if (!uiSystem->initialize(textRenderer.get())) {
        LOGE("Failed to initialize UISystem");
    } else {
        uiSystem->setScreenSize(static_cast<int>(screenWidth), static_cast<int>(screenHeight));
        LOGI("UISystem initialized successfully (Phase 9 UI Framework ready)");
    }

    // Initialize Floating Combat Text
    LOGI("Creating UIFloatingText...");
    floatingText = std::make_unique<UIFloatingText>();
    if (!floatingText->initialize(textRenderer.get(), static_cast<int>(screenWidth), static_cast<int>(screenHeight))) {
        LOGE("Failed to initialize UIFloatingText");
    } else {
        LOGI("UIFloatingText initialized successfully");
    }

    // Initialize Animation Subscriber
    LOGI("Creating AnimationSubscriber...");
    animSubscriber = std::make_unique<animation::AnimationSubscriber>();
    LOGI("AnimationSubscriber created successfully");

    // Initialize Audio Subscriber
    LOGI("Creating AudioSubscriber...");
    audioSubscriber = std::make_unique<audio::AudioSubscriber>();
    LOGI("AudioSubscriber created successfully");

    // Initialize Spell Selection Panel
    LOGI("Creating SpellSelectionPanel...");
    spellSelectionPanel = std::make_shared<SpellSelectionPanel>();
    if (!spellSelectionPanel->initialize()) {
        LOGE("Failed to initialize SpellSelectionPanel");
    } else {
        spellSelectionPanel->setPosition(screenWidth * 0.2f, screenHeight * 0.2f);
        spellSelectionPanel->setSize(screenWidth * 0.6f, screenHeight * 0.6f);
        spellSelectionPanel->setTextRenderer(textRenderer.get());
        spellSelectionPanel->setVisible(false);
        spellSelectionPanel->setOnSpellSelected([this](std::shared_ptr<Spell> spell) {
            if (pendingAssignSlot >= 0 && pendingAssignSlot < QUICK_SLOT_COUNT) {
                // Assign to quick slot
                if (playerController && playerController->getPlayer()) {
                    playerController->getPlayer()->quickSlotSpells[pendingAssignSlot] = spell;
                }
                // Update button label to spell name
                if (quickSlotButtons[pendingAssignSlot]) {
                    std::string shortName = spell->nameJa.empty() ? spell->name : spell->nameJa;
                    if (shortName.size() > 4) shortName = shortName.substr(0, 4);
                    std::string btnLabel = "F" + std::to_string(pendingAssignSlot + 1) + "\n" + shortName;
                    quickSlotButtons[pendingAssignSlot]->setLabel(btnLabel);
                    // Color by school
                    glm::vec4 col = glm::vec4(0.4f, 0.2f, 0.6f, 0.8f); // default
                    switch (spell->school) {
                        case MagicSchool::DESTRUCTION:  col = glm::vec4(0.7f, 0.15f, 0.1f, 0.8f); break;
                        case MagicSchool::RESTORATION:  col = glm::vec4(0.1f, 0.6f, 0.25f, 0.8f); break;
                        case MagicSchool::CONJURATION:  col = glm::vec4(0.4f, 0.15f, 0.6f, 0.8f); break;
                        case MagicSchool::ALTERATION:   col = glm::vec4(0.1f, 0.5f, 0.7f, 0.8f); break;
                        case MagicSchool::ILLUSION:     col = glm::vec4(0.1f, 0.6f, 0.45f, 0.8f); break;
                        case MagicSchool::MYSTICISM:    col = glm::vec4(0.6f, 0.5f, 0.1f, 0.8f); break;
                        default: break;
                    }
                    quickSlotButtons[pendingAssignSlot]->setNormalColor(col);
                }
                LOGI("Assigned spell '%s' to quick slot %d", spell->name.c_str(), pendingAssignSlot + 1);
                pendingAssignSlot = -1;
            } else {
                selectedSpell = spell;
                LOGI("Spell selected: %s", spell->name.c_str());
            }
        });
        uiSystem->registerComponent(spellSelectionPanel, 200);
        LOGI("SpellSelectionPanel initialized successfully");
    }

    // Initialize Debug HUD
    LOGI("Creating DebugHUD...");
    debugHUD = std::make_unique<DebugHUD>();
    if (!debugHUD->initialize(textRenderer.get(), nullptr, this)) {
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

    // Initialize SaveLoadUI (Phase 5+)
    LOGI("Creating SaveLoadUI...");
    saveLoadUI = std::make_unique<SaveLoadUI>();
    // Initialize SaveLoadUI after SaveManager is created
    // (will be fully initialized after SaveManager setup)
    LOGI("SaveLoadUI created (full initialization deferred)");

    // Initialize Asset Manager (before WorldManager)
    LOGI("Creating AssetManager...");
    assetManager = std::make_unique<AssetManager>();
    if (!assetManager->initialize()) {
        LOGE("Failed to initialize AssetManager");
        return;
    }
    LOGI("AssetManager initialized successfully");

    // Load BSA archives (must succeed or game has no data)
    {
        LOGI("Loading BSA archives...");

        // Default BSA archive list for Oblivion
        const char* bsaArchives[] = {
            "Oblivion - Meshes.bsa",
            "Oblivion - Textures - Compressed.bsa",
            "Oblivion - Textures.bsa",
            "Oblivion - Sounds.bsa",
            "Oblivion - Voices.bsa",
            "Oblivion - Misc.bsa",
            "DLCShiveringIsles - Meshes.bsa",
            "DLCShiveringIsles - Textures - Compressed.bsa",
            "DLCShiveringIsles - Sounds.bsa",
            "DLCShiveringIsles - Voices.bsa",
            "DLCShiveringIsles - Misc.bsa"
        };

        int loadedCount = 0;
        for (const auto& bsa : bsaArchives) {
            if (assetManager->loadArchive(bsa)) {
                loadedCount++;
                LOGI("  [OK] Loaded BSA: %s", bsa);
            } else {
                LOGW("  [--] BSA not found (optional): %s", bsa);
            }
        }

        LOGI("Loaded %d / %zu BSA archives", loadedCount,
             sizeof(bsaArchives) / sizeof(bsaArchives[0]));
    }

    // Load ESM/ESP game data from BSA archives
    {
        LOGI("Loading ESM game data...");
        // Oblivion.esm is inside Oblivion - Misc.bsa
        if (assetManager->loadEsmFromArchive("Oblivion.esm")) {
            LOGI("  [OK] Loaded Oblivion.esm");
            LOGI("Record count: %zu", assetManager->getEsmManager().getRecordCount());
            LOGI("Plugin count: %zu", assetManager->getEsmManager().getPluginCount());

            // Log a sample of loaded records
            LOGI("CELL records: %zu", assetManager->getEsmManager().findRecordsByType("CELL"));
            LOGI("NPC_ records: %zu", assetManager->getEsmManager().findRecordsByType("NPC_"));
            LOGI("WEAP records: %zu", assetManager->getEsmManager().findRecordsByType("WEAP"));
        } else {
            LOGW("  [--] Oblivion.esm not found (will test without ESM data)");
        }
    }

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

    // Initialize NavMesh Manager
    navMeshManager = std::make_unique<oblivion::NavMeshManager>();

    // Initialize AI Scheduler (Phase 35: Radiant AI)
    aiScheduler = std::make_unique<ai::AIScheduler>();

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

    // Phase 31: Initialize WorldLoader
    LOGI("Creating WorldLoader...");
    worldLoader = std::make_unique<WorldLoader>();
    worldLoader->init(assetManager.get(), nullptr);  // CollisionWorld will be set later if needed
    LOGI("WorldLoader initialized successfully");

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

    // Complete SaveLoadUI initialization (now that SaveManager is ready)
    if (saveLoadUI && saveManager) {
        if (!saveLoadUI->initialize(textRenderer.get(), saveManager.get(), this)) {
            LOGE("Failed to initialize SaveLoadUI");
        } else {
            LOGI("SaveLoadUI initialized successfully");
        }
    }

    // Initialize Audio Manager (Phase 8+)
#ifdef AUDIO_SYSTEM_ENABLED
    LOGI("Initializing AudioManager...");
    audioManager = std::make_unique<AudioManager>();
    if (!audioManager->initialize()) {
        LOGE("Failed to initialize AudioManager");
    } else {
        LOGI("AudioManager initialized successfully");
        audioManager->setListenerPosition(glm::vec3(0.0f, 1.7f, 0.0f));
        LOGD("Audio listener positioned at world center");
        
        // Load sound definitions from JSON
        LOGI("Loading sound definitions...");
        if (audioManager->loadSoundDefinitions("audio/sound_definitions.json")) {
            LOGI("Sound definitions loaded successfully");
        } else {
            LOGW("Failed to load sound definitions - audio will use manual clip loading");
        }
        
        if (debugHUD) {
            debugHUD->setAudioManager(audioManager.get());
        }
    }
#endif

    // Initialize Phase 9.1 Map System
    LOGI("Creating MapSystem...");
    mapSystem = std::make_unique<map::MapSystem>();
    mapSystem->setWorldBounds(-81920.0f, 81920.0f, -81920.0f, 81920.0f);
    LOGI("MapSystem initialized");

    // Create Map UI (fullscreen map)
    if (uiSystem) {
        auto fullMap = std::make_shared<ui::MapUI>("World Map");
        fullMap->setMapSystem(mapSystem.get());
        fullMap->setSize(static_cast<float>(screenWidth) * 0.8f, static_cast<float>(screenHeight) * 0.8f);
        fullMap->setPosition(static_cast<float>(screenWidth) * 0.1f, static_cast<float>(screenHeight) * 0.1f);
        fullMap->setVisible(false);
        fullMap->setDraggable(false); // Full-screen map doesn't need dragging

        // Apply background texture if available
        GLuint mapBgTex = TextureLoader::loadTextureFromAsset("textures/ui/main_background.png");
        if (mapBgTex != 0) {
            fullMap->setTexture(mapBgTex);
            fullMap->setTextureScaleMode(TextureScaleMode::STRETCH);
        }

        uiSystem->registerComponent(fullMap, 10);
        mapUI = fullMap.get();
        LOGI("MapUI created and registered in UISystem");

        // Create Mini-Map UI
        auto miniMap = std::make_shared<ui::MapUI>("MiniMap");
        miniMap->setMapSystem(mapSystem.get());
        miniMap->setMiniMapMode(true);
        miniMap->setSize(200.0f, 200.0f);
        miniMap->setPosition(static_cast<float>(screenWidth) - 220.0f, 20.0f);
        miniMap->setVisible(true);
        miniMap->onMiniMapTapped = [this]() {
            LOGI("Mini-map tapped - opening world map");
            toggleMap();
        };
        uiSystem->registerComponent(miniMap, 5);
        LOGI("MiniMap created and registered in UISystem");
    }

    // Initialize Phase 9.2 Inventory System
    LOGI("Creating InventoryGrid...");
    inventoryGrid = std::make_unique<inventory::InventoryGrid>(150.0f);
    LOGI("InventoryGrid initialized (max weight 150.0f)");

    equipmentManager = std::make_unique<inventory::EquipmentManager>();
    LOGI("EquipmentManager initialized");

    // Create test items
    {
        inventory::Item sword;
        sword.id = 1;
        sword.name = "Iron Sword";
        sword.description = "A basic iron sword.";
        sword.category = inventory::ItemCategory::Weapon;
        sword.rarity = inventory::ItemRarity::Common;
        sword.equipSlot = inventory::EquipSlot::Weapon;
        sword.weight = 3.5f;
        sword.value = 50;
        sword.stats.damage = 10;
        inventoryGrid->addItem(sword, 1);

        inventory::Item helm;
        helm.id = 2;
        helm.name = "Leather Helm";
        helm.category = inventory::ItemCategory::Armor;
        helm.equipSlot = inventory::EquipSlot::Head;
        helm.weight = 1.2f;
        helm.value = 30;
        helm.stats.defense = 5;
        inventoryGrid->addItem(helm, 1);

        inventory::Item potion;
        potion.id = 3;
        potion.name = "Health Potion";
        potion.category = inventory::ItemCategory::Consumable;
        potion.weight = 0.3f;
        potion.value = 15;
        potion.maxStack = 20;
        potion.healAmount = 25;
        inventoryGrid->addItem(potion, 5);

        inventory::Item questItem;
        questItem.id = 4;
        questItem.name = "Ancient Key";
        questItem.category = inventory::ItemCategory::Quest;
        questItem.weight = 0.1f;
        questItem.value = 0;
        inventoryGrid->addItem(questItem, 1);

        LOGI("Test items added to inventory");
    }

    // Create Inventory UI
    if (uiSystem) {
        auto invPanel = std::make_shared<ui::UIInventoryPanel>("Inventory");
        invPanel->setInventory(inventoryGrid.get());
        invPanel->setEquipment(equipmentManager.get());
        invPanel->setSize(static_cast<float>(screenWidth) * 0.85f, static_cast<float>(screenHeight) * 0.75f);
        invPanel->setPosition(static_cast<float>(screenWidth) * 0.075f, static_cast<float>(screenHeight) * 0.125f);
        invPanel->setVisible(false);
        uiSystem->registerComponent(invPanel, 15);
        uiInventoryPanel = invPanel.get();
        LOGI("UIInventoryPanel created and registered in UISystem");
    }

    // Initialize Title Screen
    titleScreen = std::make_unique<TitleScreen>();
    titleScreen->initialize(localizationManager.get(), textRenderer.get());

    // Initialize Quest UI
    questUI = std::make_unique<QuestUI>();
    questUI->initialize(questManager.get(), worldManager->getNpcManager(),
                        localizationManager.get());

    LOGI("All game systems initialized");

    // Imperial Weave: initialize thin integration layer
        LOGI("Initializing Imperial Weave...");
        weave::ImperialWeave::instance().init(
            this,
            worldManager.get(),
            npcManager.get(),
            combatManager.get(),
            questManager.get(),
            nullptr,  // CollisionWorld - will be set when physics is integrated
            nullptr,  // AnimationPlayer - will be set when animation is integrated
            playerController.get(),
            inventoryManager.get(),
            spellManager.get(),
            audioManager.get()
        );
        imperialWeaveInitialized = true;

        // Connect CombatManager to Imperial Weave EventBus
        if (combatManager) {
            combatManager->setEventBus(&weave::ImperialWeave::instance().getEventBus());
            LOGI("CombatManager connected to Imperial Weave EventBus");
        }

        // Connect PlayerController to Imperial Weave EventBus for animation events
        if (playerController) {
            playerController->setEventBus(&weave::ImperialWeave::instance().getEventBus());
            playerController->subscribeToCombatEvents();
            LOGI("PlayerController connected to Imperial Weave EventBus");
        }

        // Connect WorldLoader to Imperial Weave EventBus for animation events
        if (worldLoader) {
            worldLoader->setEventBus(&weave::ImperialWeave::instance().getEventBus());
            LOGI("WorldLoader connected to Imperial Weave EventBus");
        }

        // Connect UIFloatingText to Imperial Weave EventBus for combat feedback
        if (floatingText) {
            auto& bus = weave::ImperialWeave::instance().getEventBus();
            bus.subscribe("COMBAT_ATTACK_HIT", [this](const weave::Event& e) {
                floatingText->addText("Hit!", screenWidth * 0.5f, screenHeight * 0.4f,
                                     UIFloatingText::DAMAGE, 1.5f);
            });
            bus.subscribe("COMBAT_CRITICAL_HIT", [this](const weave::Event& e) {
                floatingText->addText("CRITICAL!", screenWidth * 0.5f, screenHeight * 0.35f,
                                     UIFloatingText::CRITICAL, 2.0f);
            });
            bus.subscribe("COMBAT_BLOCK", [this](const weave::Event& e) {
                floatingText->addText("Blocked!", screenWidth * 0.5f, screenHeight * 0.4f,
                                     UIFloatingText::BLOCK, 1.5f);
            });
            bus.subscribe("COMBAT_PARRY", [this](const weave::Event& e) {
                floatingText->addText("Parry!", screenWidth * 0.5f, screenHeight * 0.4f,
                                     UIFloatingText::BUFF, 1.5f);
            });
            bus.subscribe("COMBAT_DODGE", [this](const weave::Event& e) {
                floatingText->addText("Dodge!", screenWidth * 0.5f, screenHeight * 0.4f,
                                     UIFloatingText::MISS, 1.5f);
            });
            LOGI("UIFloatingText connected to Imperial Weave EventBus");
        }

        // Connect AnimationSubscriber to Imperial Weave EventBus
        if (animSubscriber && worldLoader) {
            animSubscriber->init(&weave::ImperialWeave::instance().getEventBus(), worldLoader.get());
            animSubscriber->subscribeToEvents();
            LOGI("AnimationSubscriber connected to Imperial Weave EventBus");
        }

        // Connect AudioSubscriber to Imperial Weave EventBus
        if (audioSubscriber && audioManager) {
            audioSubscriber->init(&weave::ImperialWeave::instance().getEventBus(), audioManager.get());
            audioSubscriber->subscribeToEvents();

            // Connect NPC position callback for 3D spatial audio
            if (npcManager) {
                audioSubscriber->setNpcPositionCallback([this](uint32_t npcId) -> glm::vec3 {
                    auto npc = npcManager->getNPC(npcId);
                    if (npc) return npc->position;
                    return glm::vec3(0.0f, 0.0f, 0.0f);
                });
            }
            LOGI("AudioSubscriber connected to Imperial Weave EventBus");
        }

        LOGI("Imperial Weave initialized successfully");
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

    NpcManager* npcMgr = worldManager->getNpcManager();
    if (!npcMgr) {
        LOGE("ERROR: getNpcManager() returned null");
        return;
    }

    // Check if we have real ESM data loaded
    const auto& esmMgr = assetManager->getEsmManager();
    bool hasEsmData = (esmMgr.getPluginCount() > 0);

    if (hasEsmData) {
        LOGI("=== Building world from ESM data ===");

        // 1. Load CELL records into WorldManager
        const auto& esmCells = esmMgr.getAllCells();
        LOGI("Loading %zu cells from ESM data", esmCells.size());
        for (const auto& cell : esmCells) {
            LOGD("  Cell: 0x%08X '%s' (%s) grid=[%d,%d]",
                 cell.formID, cell.editorID.c_str(),
                 cell.fullName.c_str(), cell.gridX, cell.gridY);
            worldManager->addCellFromESM(
                cell.gridX, cell.gridY,
                cell.editorID, cell.fullName,
                cell.formID);
        }

        // 2. Build lookup: baseFormID → NPCData for reference resolution
        std::unordered_map<uint32_t, const oblivion::NPCData*> npcLookup;
        const auto& esmNpcs = esmMgr.getAllNPCs();
        for (const auto& npc : esmNpcs) {
            npcLookup[npc.formID] = &npc;
        }

        // 3. Process REFR references to place NPCs at correct positions
        const auto& refs = esmMgr.getAllReferences();
        LOGI("Processing %zu references from ESM data", refs.size());
        for (const auto& ref : refs) {
            auto it = npcLookup.find(ref.baseFormID);
            if (it != npcLookup.end()) {
                const oblivion::NPCData* npcData = it->second;
                auto npcPtr = npcMgr->createNPC(
                    npcData->fullName.empty() ? npcData->editorID : npcData->fullName,
                    ref.position);
                if (npcPtr) {
                    npcPtr->status.initialize(
                        static_cast<float>(npcData->health),
                        static_cast<float>(npcData->magicka),
                        npcData->level);
                    npcPtr->rotation = ref.rotation;
                    npcPtr->meshAssetPath = "meshes/characters/imperial_male.nif";
                    npcPtr->updateModelMatrix();

                    // Register ESM NPC with AI Scheduler (Phase 35: Radiant AI)
                    if (aiScheduler) {
                        aiScheduler->registerNPC(npcPtr->npcId);
                    }

                    LOGD("  Placed NPC: 0x%08X '%s' at (%.1f, %.1f, %.1f)",
                         ref.formID, npcData->fullName.c_str(),
                         ref.position.x, ref.position.y, ref.position.z);
                }
            }
        }

        // 4. Load LAND terrain data and assign to cells
        const auto& terrains = esmMgr.getAllTerrains();
        LOGI("Loading %zu terrain records from ESM data", terrains.size());
        for (const auto& terrain : terrains) {
            auto cell = worldManager->getCellByFormID(terrain.formID);
            if (cell && terrain.hasHeights()) {
                cell->heightData = terrain.heights;
                cell->isDirty = true;
                LOGD("  Assigned terrain to cell 0x%08X (%zu heights)",
                     terrain.formID, terrain.heights.size());
            }
        }

        // 5. Load WEAP records for reference
        const auto& weapons = esmMgr.getAllWeapons();
        LOGI("Loaded %zu weapons from ESM data", weapons.size());
        for (const auto& weapon : weapons) {
            LOGD("  Weapon: 0x%08X '%s' dmg=%u value=%u weight=%u",
                 weapon.formID, weapon.fullName.c_str(),
                 weapon.damage, weapon.value, weapon.weight);
        }

        // 6. Log worldspace definitions with bounds
        const auto& worlds = esmMgr.getAllWorlds();
        LOGI("Found %zu worldspaces", worlds.size());
        for (const auto& w : worlds) {
            LOGI("  WRLD: 0x%08X '%s' '%s' bounds=[%d,%d] to [%d,%d]",
                 w.formID, w.editorID.c_str(), w.fullName.c_str(),
                 w.minX, w.minY, w.maxX, w.maxY);
        }

                // 7. Import armor records from ESM into ItemFactory
                inventory::ItemFactory::getInstance().loadArmorsFromESM(esmMgr);

                // 8. Import spell records from ESM into SpellManager
                if (spellManager) {
                    spellManager->loadSpellsFromESM(esmMgr);
                }

                // 9. Process leveled lists (LVLI/LVLC/LVSP)
                const auto& leveledLists = esmMgr.getAllLeveledLists();
                LOGI("Found %zu leveled lists", leveledLists.size());
                size_t lvliCount = 0, lvlcCount = 0, lvspCount = 0;
                for (const auto& ll : leveledLists) {
                    // Count by type based on editorID prefix or entries
                    if (ll.editorID.find("LL") != std::string::npos) lvliCount++;
                    else if (ll.editorID.find("LC") != std::string::npos) lvlcCount++;
                    else lvspCount++;
                    LOGD("  LeveledList: 0x%08X '%s' chanceNone=%u flags=0x%02X entries=%zu",
                         ll.formID, ll.editorID.c_str(), ll.chanceNone, ll.flags, ll.entries.size());
                }
                LOGI("  LVLI=%zu LVLC=%zu LVSP=%zu", lvliCount, lvlcCount, lvspCount);

                // 10. Resolve LVLC lists to spawn creatures at player level 5 (test)
                uint32_t testPlayerLevel = 5;
                size_t spawnedFromLists = 0;
                for (const auto& ll : leveledLists) {
                    // Only process creature lists (those with NPC_ references)
                    if (ll.entries.empty()) continue;
                    auto resolved = esmMgr.resolveLeveledList(ll.formID, testPlayerLevel);
                    for (const auto& [refFormID, count] : resolved) {
                        const auto* npcData = esmMgr.findNPC(refFormID);
                        if (npcData) {
                            // Spawn creature from leveled list at origin
                            glm::vec3 spawnPos(spawnedFromLists * 3.0f, 0.0f, -10.0f);
                            auto npc = npcMgr->createNPC(
                                npcData->fullName.empty() ? npcData->editorID : npcData->fullName,
                                spawnPos);
                            if (npc) {
                                npc->status.initialize(
                                    static_cast<float>(npcData->health),
                                    static_cast<float>(npcData->magicka),
                                    npcData->level);
                                npc->meshAssetPath = "meshes/characters/imperial_male.nif";
                                npc->updateModelMatrix();

                                // Register spawned creature with AI Scheduler
                                if (aiScheduler) {
                                    aiScheduler->registerNPC(npc->npcId);
                                }

                                spawnedFromLists++;
                                LOGD("  Spawned from LVLC '%s': NPC '%s' (0x%08X) level=%u",
                                     ll.editorID.c_str(), npcData->fullName.c_str(),
                                     refFormID, npcData->level);
                            }
                        }
                    }
                }
                LOGI("Spawned %zu NPCs from leveled creature lists", spawnedFromLists);

                // 11. Load NavMesh data for AI pathfinding
                const auto& navMeshes = esmMgr.getAllNavMeshes();
                LOGI("Found %zu NavMesh records", navMeshes.size());
                size_t totalVertices = 0, totalTriangles = 0;
                for (const auto& nm : navMeshes) {
                    totalVertices += nm.vertices.size();
                    totalTriangles += nm.triangles.size();
                    LOGD("  NavMesh: 0x%08X '%s' verts=%zu tris=%zu",
                         nm.formID, nm.editorID.c_str(), nm.vertices.size(), nm.triangles.size());
                }
                LOGI("  Total: %zu vertices, %zu triangles", totalVertices, totalTriangles);

                // Load NavMesh data into NavMeshManager
                if (navMeshManager) {
                    navMeshManager->loadFromESM(esmMgr);
                }

                // Initialize AI Scheduler with game systems
                if (aiScheduler && npcManager && worldManager) {
                    aiScheduler->init(npcManager.get(), worldManager.get(), navMeshManager.get());
                    LOGI("AI Scheduler initialized with %zu registered NPCs", aiScheduler->getRegisteredNPCCount());
                }

                // 12. Log race and class data for character creation
                const auto& races = esmMgr.getAllRaces();
                LOGI("Found %zu races", races.size());
                for (const auto& race : races) {
                    LOGI("  RACE: 0x%08X '%s' '%s' HP=%u spells=%zu",
                         race.formID, race.editorID.c_str(), race.fullName.c_str(),
                         race.startingHealth, race.spellFormIDs.size());
                    LOGD("    STR=%u INT=%u WIL=%u AGI=%u SPD=%u END=%u PER=%u",
                         race.attrStrength, race.attrIntelligence, race.attrWillpower,
                         race.attrAgility, race.attrSpeed, race.attrEndurance, race.attrPersonality);
                }

                const auto& classes = esmMgr.getAllClasses();
                LOGI("Found %zu classes", classes.size());
                for (const auto& cls : classes) {
                    LOGI("  CLAS: 0x%08X '%s' '%s' spec=%u",
                         cls.formID, cls.editorID.c_str(), cls.fullName.c_str(), cls.specialization);
                }

                // 13. Log book data (skill books)
                const auto& books = esmMgr.getAllBooks();
                LOGI("Found %zu books", books.size());
                size_t skillBookCount = 0;
                for (const auto& book : books) {
                    if (book.teachesSkillID != 0) {
                        skillBookCount++;
                        LOGD("  SkillBook: 0x%08X '%s' skill=0x%08X level=%u value=%u",
                             book.formID, book.fullName.c_str(),
                             book.teachesSkillID, book.teachesSkillLevel, book.value);
                    }
                }
                LOGI("  Skill books: %zu", skillBookCount);

                // 14. Log remaining item types
                const auto& clothing = esmMgr.getAllClothing();
                LOGI("Found %zu clothing items", clothing.size());
                for (const auto& cl : clothing) {
                    LOGD("  CLOT: 0x%08X '%s' value=%u weight=%.1f",
                         cl.formID, cl.fullName.c_str(), cl.value, cl.weight);
                }

                const auto& ingredients = esmMgr.getAllIngredients();
                LOGI("Found %zu ingredients", ingredients.size());
                for (const auto& ing : ingredients) {
                    LOGD("  INGR: 0x%08X '%s' value=%u weight=%.1f",
                         ing.formID, ing.fullName.c_str(), ing.value, ing.weight);
                }

                const auto& alchemy = esmMgr.getAllAlchemy();
                LOGI("Found %zu alchemy items", alchemy.size());
                for (const auto& alc : alchemy) {
                    LOGD("  ALCH: 0x%08X '%s' value=%u weight=%.1f",
                         alc.formID, alc.fullName.c_str(), alc.value, alc.weight);
                }

                const auto& miscItems = esmMgr.getAllMiscItems();
                LOGI("Found %zu misc items", miscItems.size());
                for (const auto& misc : miscItems) {
                    LOGD("  MISC: 0x%08X '%s' value=%u weight=%.1f",
                         misc.formID, misc.fullName.c_str(), misc.value, misc.weight);
                }

                LOGI("ESM-based world generation complete");

                // Phase 31: Load NIF files as WorldEntities
                if (worldLoader) {
                    LOGI("=== Phase 31: Loading WorldEntities from NIF files ===");

                    // Test NIF paths (common Oblivion meshes)
                    const char* testNifs[] = {
                        "meshes/characters/imperial_male.nif",
                        "meshes/creatures/imp.nif",
                        "meshes/architecture/buildings/imperial_house_01.nif",
                        "meshes/furniture/chair_01.nif",
                        "meshes/clutter/barrel_01.nif"
                    };

                    for (size_t i = 0; i < sizeof(testNifs)/sizeof(testNifs[0]); ++i) {
                        glm::vec3 pos(static_cast<float>(i) * 5.0f, 0.0f, -15.0f);
                        WorldEntity entity;

                        // Try loading as actor first (has skeleton/animation), then static
                        entity = worldLoader->loadActor(testNifs[i], pos);
                        if (entity.mesh || entity.skinnedMesh) {
                            entity.entityId = worldLoader->getNextEntityId();
                            worldEntities.push_back(std::move(entity));
                            LOGI("  [OK] Loaded actor: %s (id=%u)", testNifs[i], worldEntities.back().entityId);
                        } else {
                            entity = worldLoader->loadStatic(testNifs[i], pos);
                            if (entity.mesh || entity.skinnedMesh) {
                                entity.entityId = worldLoader->getNextEntityId();
                                worldEntities.push_back(std::move(entity));
                                LOGI("  [OK] Loaded static: %s (id=%u)", testNifs[i], worldEntities.back().entityId);
                            } else {
                                LOGW("  [--] Failed to load: %s", testNifs[i]);
                            }
                        }
                    }

                    // Wire player skeleton/animation if we loaded an actor entity
                    for (auto& ent : worldEntities) {
                        if (ent.type == WorldEntityType::ACTOR && ent.skeleton && ent.animator) {
                            playerController->setSkeleton(ent.skeleton.get());
                            playerController->setAnimator(ent.animator.get());
                            LOGI("  PlayerController wired to entity %u skeleton/animation", ent.entityId);
                            break;  // Use first actor for player
                        }
                    }

                    LOGI("Phase 31: Loaded %zu WorldEntities (cache size=%zu)",
                         worldEntities.size(), worldLoader->getCacheSize());
                }
    } else {
        LOGI("=== No ESM data available, using hardcoded test scenario ===");
        // Fall back to hardcoded test (existing code below)
    }

    // Declare spell variables at function scope so they're available for spell casting
    uint32_t fireball = 0;
    uint32_t heal = 0;
    uint32_t restoreMana = 0;

    // Create test NPCs (always create at least basic test NPCs)
    NpcManager* npcMgr2 = npcMgr;  // reuse pointer
    
    // Create NPCs using available data
    auto izar = npcMgr2->createNPC("Izar", glm::vec3(0.0f, 0.0f, 0.0f));
    auto hellas = npcMgr2->createNPC("Hellas", glm::vec3(5.0f, 0.0f, 0.0f));

    if (!izar) { LOGE("ERROR: Failed to create NPC 'Izar'"); return; }
    if (!hellas) { LOGE("ERROR: Failed to create NPC 'Hellas'"); return; }

    if (izar && hellas) {
        izar->status.initialize(150.0f, 100.0f, 5);
        hellas->status.initialize(120.0f, 80.0f, 4);

        // Register NPCs with AI Scheduler (Phase 35: Radiant AI)
        if (aiScheduler) {
            aiScheduler->registerNPC(izar->npcId);
            aiScheduler->registerNPC(hellas->npcId);
            LOGI("Registered test NPCs with AI Scheduler: Izar=%u, Hellas=%u",
                 izar->npcId, hellas->npcId);
        }

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
            if (hasEsmData) {
                // ESM mode: pick the first Destruction and Restoration spells
                const auto& spells = esmMgr.getAllSpells();
                for (const auto& s : spells) {
                    uint32_t spellId = s.formID;
                    if (spellId == 0) continue;

                    // Assign first Destruction spell to Izar (monster)
                    if (s.effectType == 2 && fireball == 0) {
                        fireball = spellId;
                        spellManager->teachSpellToNpc(izar->npcId, spellId);
                        spellManager->equipSpellToNpc(izar->npcId, spellId);
                        LOGI("  ESM Destruction spell for Izar: 0x%08X '%s'", spellId, s.fullName.c_str());
                    }

                    // Assign first Restoration spell to Hellas (healer)
                    if (s.effectType == 5 && heal == 0) {
                        heal = spellId;
                        spellManager->teachSpellToNpc(hellas->npcId, spellId);
                        spellManager->equipSpellToNpc(hellas->npcId, spellId);
                        LOGI("  ESM Restoration spell for Hellas: 0x%08X '%s'", spellId, s.fullName.c_str());
                    }

                    // Assign first Mysticism spell to both
                    if (s.effectType == 4 && restoreMana == 0) {
                        restoreMana = spellId;
                        spellManager->teachSpellToNpc(izar->npcId, spellId);
                        spellManager->equipSpellToNpc(izar->npcId, spellId);
                        LOGI("  ESM Mysticism spell for both: 0x%08X '%s'", spellId, s.fullName.c_str());
                    }

                    // Also teach the Restoration spell to Izar so he can heal too
                    if (heal != 0 && spellId == heal && izar) {
                        spellManager->teachSpellToNpc(izar->npcId, heal);
                        spellManager->equipSpellToNpc(izar->npcId, heal);
                    }
                }
                LOGI("ESM spells assigned: fireball=0x%08X, heal=0x%08X, restoreMana=0x%08X",
                     fireball, heal, restoreMana);
            } else {
                // No ESM: use hardcoded spells
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
    // Set joystick input before Imperial Weave update
    if (playerController && joystick) {
        glm::vec2 input = joystick->getInputValue();
        playerController->setJoystickInput(input.x, input.y);
    }

    // Update audio listener position before Imperial Weave update
#ifdef AUDIO_SYSTEM_ENABLED
    if (audioManager && worldManager) {
        const glm::vec3& cameraPos = worldManager->getCameraPosition();
        const glm::vec3& cameraForward = worldManager->getCameraForward();
        const glm::vec3& cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
        audioManager->setListenerPosition(cameraPos);
        audioManager->setListenerOrientation(cameraForward, cameraUp);

        // Update AudioSubscriber player position for 3D combat sounds
        if (audioSubscriber) {
            audioSubscriber->setPlayerPosition(cameraPos);
        }
    }
#endif

    // Imperial Weave: process game logic updates (events, world, AI, animation, physics)
    if (imperialWeaveInitialized) {
        weave::ImperialWeave::instance().update(deltaTime);
    }

    // Phase 35: Radiant AI — update AI scheduler after ImperialWeave
    if (aiScheduler) {
        aiScheduler->update(deltaTime);
    }

    // Begin performance monitoring
    if (performanceMonitor) {
        performanceMonitor->beginFrame();
    }

    // Update Title Screen
    if (showTitleScreen) {
        titleScreen->update(deltaTime);
        titleScreen->render();

        // Check if Load Game was requested (Phase 5+)
        if (titleScreen->isLoadGameRequested()) {
            titleScreen->resetLoadGameRequest();
            if (saveLoadUI) {
                saveLoadUI->open(SaveLoadUI::Mode::LOAD);  // Open load UI
                LOGI("SaveLoadUI opened in LOAD mode from title screen menu");
            }
        }

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
            // Show combat buttons when game starts
            if (attackButton) attackButton->setVisible(true);
            if (blockButton) blockButton->setVisible(true);
            if (castSpellButton) castSpellButton->setVisible(true);
            for (auto& btn : quickSlotButtons) {
                if (btn) btn->setVisible(true);
            }
            LOGI("Title screen closed, starting main game");
        }
        // Skip frame rate control for quick return
        if (performanceMonitor) {
            performanceMonitor->endFrame();
        }
        return;  // Skip game rendering while title screen is active
    }

    // Update Phase 9 UI Framework
    if (uiSystem) {
        uiSystem->update(deltaTime);
    }

    // Update Floating Combat Text
    if (floatingText) {
        floatingText->update(deltaTime);
    }

    // Update Animation Subscriber
    if (animSubscriber) {
        animSubscriber->updateNpcAnimations(deltaTime);
    }

    // Update map system with player position and cell discovery
    if (mapSystem && worldManager) {
        glm::vec3 playerPos3D = worldManager->getPlayerPosition();
        glm::vec2 playerPos2D(playerPos3D.x, playerPos3D.z);
        mapSystem->setPlayerPosition(playerPos2D);

        // Auto-discover cells around player
        float cellSize = mapSystem->getCellSize();
        glm::vec2 cellCoord = map::MapSystem::worldToCell(playerPos2D, cellSize);
        int pcx = static_cast<int>(cellCoord.x);
        int pcy = static_cast<int>(cellCoord.y);

        for (int dy = -2; dy <= 2; ++dy) {
            for (int dx = -2; dx <= 2; ++dx) {
                int cx = pcx + dx;
                int cy = pcy + dy;
                if (!mapSystem->isCellDiscovered(cx, cy)) {
                    mapSystem->discoverCell(cx, cy);

                    // Set procedural cell info with terrain color
                    map::CellInfo info;
                    info.x = cx;
                    info.y = cy;
                    info.discovered = true;

                    // Procedural terrain color based on coordinates
                    uint32_t hash = static_cast<uint32_t>(cx * 374761393u + cy * 668265263u);
                    int terrainType = hash % 10;
                    if (terrainType < 5) {
                        info.terrainColor = 0xFF4A8C4A; // Grass green
                        info.name = "Wilderness";
                    } else if (terrainType < 7) {
                        info.terrainColor = 0xFF8C8C4A; // Hills brown-yellow
                        info.name = "Hills";
                    } else if (terrainType < 8) {
                        info.terrainColor = 0xFF4A6A8C; // Water blue
                        info.name = "Lake";
                    } else {
                        info.terrainColor = 0xFF7A7A7A; // Mountains gray
                        info.name = "Mountains";
                    }
                    mapSystem->setCellInfo(cx, cy, info);
                }
            }
        }

        // Update quest markers from active quests
        if (questManager && npcManager) {
            auto activeQuests = questManager->getActiveQuests();
            // Clear old quest markers
            mapSystem->clearMarkersByType(map::MarkerType::QuestMain);
            mapSystem->clearMarkersByType(map::MarkerType::QuestSide);
            // Add markers for active quests at giver NPC positions
            for (const auto& quest : activeQuests) {
                if (!quest) continue;
                auto npc = npcManager->getNPC(quest->giverNpcId);
                if (!npc) continue;
                map::MapMarker marker;
                marker.type = map::MarkerType::QuestSide;
                marker.worldPos = glm::vec2(npc->position.x, npc->position.z);
                marker.label = quest->title;
                marker.questId = quest->questId;
                marker.color = 0xFFFFD700; // Gold color ABGR
                mapSystem->addMarker(marker);
            }
        }
    }

    // Update game systems
    // NOTE: worldManager->update() is now handled by ImperialWeave (phaseWorldUpdate)
    // if (worldManager) {
    //     worldManager->update(deltaTime);
    // }

    // NOTE: playerController->update() is now handled by ImperialWeave (phasePlayerUpdate)
    // Joystick input is set before update - this needs to be moved to playerController's update method
    // if (playerController) {
    //     if (joystick) {
    //         glm::vec2 input = joystick->getInputValue();
    //         playerController->setJoystickInput(input.x, input.y);
    //     }
    //     playerController->update(deltaTime);
    // }

    // NOTE: inventoryManager->update() is now handled by ImperialWeave (phaseInventoryUpdate)
    // if (inventoryManager) {
    //     inventoryManager->update(deltaTime);
    // }

    // NOTE: questManager->update() is now handled by ImperialWeave (phaseQuestUpdate)
        // if (questManager) {
        //     questManager->update(deltaTime);
        // }

    // NOTE: combatManager->update() is now handled by ImperialWeave (phaseCombatUpdate)
        // if (combatManager) {
        //     combatManager->update(deltaTime);
        // }

    // NOTE: spellManager->update() is now handled by ImperialWeave (phaseSpellUpdate)
    // if (spellManager) {
    //     spellManager->update(deltaTime);
    // }

    // NOTE: audioManager->update() is now handled by ImperialWeave (phaseAudioUpdate)
    // #ifdef AUDIO_SYSTEM_ENABLED
    // if (audioManager && worldManager) {
    //     // Get camera position from world manager and set as audio listener position
    //     const glm::vec3& cameraPos = worldManager->getCameraPosition();
    //     const glm::vec3& cameraForward = worldManager->getCameraForward();
    //     const glm::vec3& cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);  // Standard up vector
    //
    //     audioManager->setListenerPosition(cameraPos);
    //     audioManager->setListenerOrientation(cameraForward, cameraUp);
    //     audioManager->update(deltaTime);
    // }
    // #endif

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

    // Phase 31: Render WorldEntities (skinned meshes with bone matrices)
    if (!worldEntities.empty()) {
        // Create skinning shader if not exists (static for reuse)
        static ShaderProgram skinningShader;
        static bool shaderInitialized = false;
        if (!shaderInitialized) {
            skinningShader.compile(SkinningShader::vertexSource, SkinningShader::fragmentSource);
            shaderInitialized = true;
        }

        for (const auto& entity : worldEntities) {
            if (!entity.isActive || !entity.isVisible) continue;

            // Update skeleton animation if entity has one
            if (entity.skeleton && entity.animator) {
                // Animation is already updated by PlayerController for player entity
                // For other entities, update here
                if (entity.type != WorldEntityType::ACTOR || !playerController) {
                    entity.animator->update(deltaTime);
                    entity.skeleton->update();
                }
            }

            // Compute model matrix
            glm::mat4 modelMatrix = entity.getModelMatrix();

            // Render skinned mesh
            if (entity.skinnedMesh && entity.skeleton) {
                const auto& boneMatrices = entity.skeleton->getSkinningMatrices();
                if (!boneMatrices.empty()) {
                    entity.skinnedMesh->updateBoneMatrices(boneMatrices);
                    entity.skinnedMesh->render(skinningShader, modelMatrix);
                }
            }
            // Render static mesh
            else if (entity.mesh) {
                entity.mesh->render(skinningShader, modelMatrix);
            }
        }
    }

    // ===== RETRO FILTER: Apply post-processing effects and render to screen =====
    if (retroFilter) {
        LOGI("RetroFilter exists, calling apply()...");
        retroFilter->apply(retroSettings);
        LOGI("RetroFilter apply() completed, calling renderToScreen()...");
        retroFilter->renderToScreen();
        LOGI("RetroFilter renderToScreen() completed");
    } else {
        LOGE("CRITICAL: retroFilter is NULL!");
    }

    // ===== NATIVE UI & HUD: Render directly on top of the screen at crisp, 100% full native resolution =====
    // 2D UIやHUDをレトロフィルター適用「後」に描画することで、文字が潰れたりレイアウトが歪むのを完全に防ぎます。

    // Update Debug HUD (DeltaTime is in milliseconds from the JNI layer)
    if (debugHUD) {
        debugHUD->update(deltaTime);
    }

    // Render UI
    if (questUI) {
        questUI->render();
    }

    // Render Inventory UI
    if (inventoryUI && inventoryUI->isVisible()) {
        inventoryUI->render();
    }

    // Render Debug HUD
    if (debugHUD) {
        debugHUD->render();
    }

    // Render SaveLoadUI if visible (higher priority than SettingsUI)
    if (saveLoadUI && saveLoadUI->isVisible()) {
        saveLoadUI->render();
    }

    // Render Settings UI if visible
    if (settingsUI && settingsUI->isVisible()) {
        settingsUI->render();
    }

    // Render Phase 9 UI Framework components (overlays on top of existing UI)
    if (uiSystem) {
        uiSystem->render();
    }

    // Render Floating Combat Text
    if (floatingText) {
        floatingText->render();
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

void Renderer::onTouchEvent(int pointerId, float x, float y, int action) {
    LOGD("=== タッチイベント検出 === ID: %d, Action: %d, 座標: (%.1f, %.1f)", pointerId, action, x, y);

    float dx = 0.0f;
    float dy = 0.0f;

    if (action == 0 || action == 5) { // DOWN
        touchStates[pointerId] = {x, y, true};
    } else if (action == 2) { // MOVE
        if (touchStates.find(pointerId) != touchStates.end() && touchStates[pointerId].active) {
            dx = x - touchStates[pointerId].lastX;
            dy = y - touchStates[pointerId].lastY;
        }
        touchStates[pointerId].lastX = x;
        touchStates[pointerId].lastY = y;
    } else if (action == 1 || action == 6 || action == 3) { // UP or CANCEL
        if (touchStates.find(pointerId) != touchStates.end()) {
            touchStates[pointerId].active = false;
        }
    }

    // Phase 9: UISystem handles all actions
    if (uiSystem) {
        bool uiHandled = false;
        if (action == 0 || action == 5) {
            uiHandled = uiSystem->onTouchDown(x, y, pointerId);
        } else if (action == 1 || action == 6) {
            uiHandled = uiSystem->onTouchUp(x, y, pointerId);
        } else if (action == 2) {
            uiHandled = uiSystem->onTouchMove(x, y, dx, dy, pointerId);
        }
        
        if (uiHandled) {
            return;
        }
    }

    // Only process legacy UI elements on ACTION_DOWN (0 or 5)
    if (action == 0 || action == 5) {
        if (saveLoadUI && saveLoadUI->isVisible()) {
            saveLoadUI->onTouchEvent(x, y);
            if (saveLoadUI->shouldReturnToMenu()) {
                saveLoadUI->resetReturnFlag();
                saveLoadUI->close();
            }
            return;
        }

        if (settingsUI && settingsUI->isVisible()) {
            settingsUI->onTouchEvent(x, y);
            if (settingsUI->shouldReturnToMenu()) {
                settingsUI->resetReturnFlag();
                settingsUI->toggle();
            }
            return;
        }

        if (inventoryUI && inventoryUI->isVisible()) {
            inventoryUI->onTouchEvent(x, y);
            return;
        }

        if (showTitleScreen && titleScreen) {
            titleScreen->onTouchEvent(x, y);
            return;
        }

        if (!showTitleScreen && worldManager && questUI) {
            questUI->onTouchEvent(x, y);
        }
    }

    // In-game camera control - only on MOVE (action 2) and if not clicking a UI
    if (!showTitleScreen && worldManager) {
        if (action == 2 && playerController) {
            // Check if touch is on right side of screen (for camera rotation)
            // Hardcoding a check assuming half screen width is around 1000px, 
            // but normally we should check real screen size. Let's just pass dx/dy for now
            // since Joystick will consume touches on the left side via UISystem.
            playerController->onTouchInput(dx, dy);
        }
    }
}
void Renderer::cleanup() {
    LOGI("Renderer cleaning up");

    // Imperial Weave: shutdown integration layer
    if (imperialWeaveInitialized) {
        weave::ImperialWeave::instance().shutdown();
        imperialWeaveInitialized = false;
        LOGI("Imperial Weave shut down");
    }

    // Clean up static UI drawing programs/buffers to prevent stale GL context handles across EGL context recreations
    UIDrawHelper::cleanup();

    if (mapSystem) {
        mapSystem.reset();
    }

    if (equipmentManager) {
        equipmentManager.reset();
    }

    if (inventoryGrid) {
        inventoryGrid.reset();
    }

    if (uiSystem) {
        uiSystem->cleanup();
        uiSystem = nullptr;
    }

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

    if (saveLoadUI) {
        saveLoadUI->cleanup();
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

void Renderer::toggleMap() {
    if (!mapUI) return;
    bool visible = !mapUI->isVisible();
    mapUI->setVisible(visible);
    if (visible) {
        mapUI->resetView();
        LOGI("World Map opened");
    } else {
        LOGI("World Map closed");
    }
}

void Renderer::toggleInventory() {
    if (!uiInventoryPanel) return;
    bool visible = !uiInventoryPanel->isVisible();
    uiInventoryPanel->setVisible(visible);
    if (visible) {
        LOGI("Inventory UI opened");
    } else {
        LOGI("Inventory UI closed");
    }
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

