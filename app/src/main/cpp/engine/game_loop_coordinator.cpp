#include "game_loop_coordinator.h"
#include "renderer.h"
#include "../world/world_manager.h"
#include "../game/npc_manager.h"
#include "../game/combat_manager.h"
#include "../game/quest_manager.h"
#include "../audio/audio_manager.h"
#include "../save_system/save_manager.h"
#include "../game/player_controller.h"
#include "../game/inventory_manager.h"
#include "../game/spell_manager.h"
#include "../physics/physics_manager.h"
#include "../script/script_manager.h"
#include "../dialogue/dialogue_runner.h"

// ============================================================================
// GameLoopCoordinator implementation
// ============================================================================

GameLoopCoordinator::GameLoopCoordinator() {
    // Initialize phase timing names
    phaseTimings_[0]  = {"Input",      0.0f};
    phaseTimings_[1]  = {"ScriptVM",   0.0f};
    phaseTimings_[2]  = {"AI",         0.0f};
    phaseTimings_[3]  = {"Physics",    0.0f};
    phaseTimings_[4]  = {"NPC",        0.0f};
    phaseTimings_[5]  = {"Combat",     0.0f};
    phaseTimings_[6]  = {"Quest",      0.0f};
    phaseTimings_[7]  = {"Dialogue",   0.0f};
    phaseTimings_[8]  = {"World",      0.0f};
    phaseTimings_[9]  = {"Audio",      0.0f};
    phaseTimings_[10] = {"AutoSave",   0.0f};
    phaseTimings_[11] = {"Render",     0.0f};
}

GameLoopCoordinator::~GameLoopCoordinator() {
    shutdown();
}

bool GameLoopCoordinator::initialize() {
    imperialWeave_ = &weave::ImperialWeave::instance();
    frameCount_ = 0;
    autoSaveTimer_ = 0.0f;
    LOGI("GameLoopCoordinator initialized");
    return true;
}

void GameLoopCoordinator::shutdown() {
    renderer_ = nullptr;
    worldManager_ = nullptr;
    npcManager_ = nullptr;
    combatManager_ = nullptr;
    questManager_ = nullptr;
    audioManager_ = nullptr;
    saveManager_ = nullptr;
    playerController_ = nullptr;
    inventoryManager_ = nullptr;
    spellManager_ = nullptr;
    physicsManager_ = nullptr;
    scriptManager_ = nullptr;
    dialogueRunner_ = nullptr;
    stateManager_ = nullptr;
    inputRouter_ = nullptr;
    imperialWeave_ = nullptr;
    LOGI("GameLoopCoordinator shutdown");
}

// System registration
void GameLoopCoordinator::setRenderer(Renderer* r) { renderer_ = r; }
void GameLoopCoordinator::setWorldManager(WorldManager* w) { worldManager_ = w; }
void GameLoopCoordinator::setNpcManager(NpcManager* n) { npcManager_ = n; }
void GameLoopCoordinator::setCombatManager(CombatManager* c) { combatManager_ = c; }
void GameLoopCoordinator::setQuestManager(QuestManager* q) { questManager_ = q; }
void GameLoopCoordinator::setAudioManager(AudioManager* a) { audioManager_ = a; }
void GameLoopCoordinator::setSaveManager(SaveManager* s) { saveManager_ = s; }
void GameLoopCoordinator::setPlayerController(PlayerController* p) { playerController_ = p; }
void GameLoopCoordinator::setInventoryManager(InventoryManager* i) { inventoryManager_ = i; }
void GameLoopCoordinator::setSpellManager(SpellManager* s) { spellManager_ = s; }
void GameLoopCoordinator::setPhysicsManager(oblivion::PhysicsManager* p) { physicsManager_ = p; }
void GameLoopCoordinator::setScriptManager(oblivion::script::ScriptManager* s) { scriptManager_ = s; }
void GameLoopCoordinator::setDialogueRunner(oblivion::dialogue::DialogueRunner* d) { dialogueRunner_ = d; }
void GameLoopCoordinator::setStateManager(StateManager* s) { stateManager_ = s; }
void GameLoopCoordinator::setInputRouter(InputRouter* i) { inputRouter_ = i; }

// ============================================================================
// Main update - 12-phase pipeline
// ============================================================================

void GameLoopCoordinator::update(float deltaTime) {
    auto frameStart = std::chrono::high_resolution_clock::now();

    // Guard against NaN/Inf delta time
    if (std::isnan(deltaTime) || std::isinf(deltaTime)) {
        LOGW("Invalid deltaTime detected (%f), using default 1/60s", deltaTime);
        deltaTime = 1.0f / 60.0f;
    }

    // Clamp delta time to prevent spiral of death
    if (deltaTime > 0.1f) deltaTime = 0.1f;
    if (deltaTime <= 0.0f) deltaTime = 1.0f / 60.0f;

    // Phase 1: Input
    auto t1 = std::chrono::high_resolution_clock::now();
    phaseInput(deltaTime);
    auto t2 = std::chrono::high_resolution_clock::now();
    phaseTimings_[0].durationMs = std::chrono::duration<float, std::milli>(t2 - t1).count();

    // Phase 2: Script VM
    t1 = std::chrono::high_resolution_clock::now();
    phaseScriptVM(deltaTime);
    t2 = std::chrono::high_resolution_clock::now();
    phaseTimings_[1].durationMs = std::chrono::duration<float, std::milli>(t2 - t1).count();

    // Phase 3: AI
    t1 = std::chrono::high_resolution_clock::now();
    phaseAI(deltaTime);
    t2 = std::chrono::high_resolution_clock::now();
    phaseTimings_[2].durationMs = std::chrono::duration<float, std::milli>(t2 - t1).count();

    // Phase 4: Physics
    t1 = std::chrono::high_resolution_clock::now();
    phasePhysics(deltaTime);
    t2 = std::chrono::high_resolution_clock::now();
    phaseTimings_[3].durationMs = std::chrono::duration<float, std::milli>(t2 - t1).count();

    // Phase 5: NPC
    t1 = std::chrono::high_resolution_clock::now();
    phaseNPC(deltaTime);
    t2 = std::chrono::high_resolution_clock::now();
    phaseTimings_[4].durationMs = std::chrono::duration<float, std::milli>(t2 - t1).count();

    // Phase 6: Combat
    t1 = std::chrono::high_resolution_clock::now();
    phaseCombat(deltaTime);
    t2 = std::chrono::high_resolution_clock::now();
    phaseTimings_[5].durationMs = std::chrono::duration<float, std::milli>(t2 - t1).count();

    // Phase 7: Quest
    t1 = std::chrono::high_resolution_clock::now();
    phaseQuest(deltaTime);
    t2 = std::chrono::high_resolution_clock::now();
    phaseTimings_[6].durationMs = std::chrono::duration<float, std::milli>(t2 - t1).count();

    // Phase 8: Dialogue
    t1 = std::chrono::high_resolution_clock::now();
    phaseDialogue(deltaTime);
    t2 = std::chrono::high_resolution_clock::now();
    phaseTimings_[7].durationMs = std::chrono::duration<float, std::milli>(t2 - t1).count();

    // Phase 9: World
    t1 = std::chrono::high_resolution_clock::now();
    phaseWorld(deltaTime);
    t2 = std::chrono::high_resolution_clock::now();
    phaseTimings_[8].durationMs = std::chrono::duration<float, std::milli>(t2 - t1).count();

    // Phase 10: Audio
    t1 = std::chrono::high_resolution_clock::now();
    phaseAudio(deltaTime);
    t2 = std::chrono::high_resolution_clock::now();
    phaseTimings_[9].durationMs = std::chrono::duration<float, std::milli>(t2 - t1).count();

    // Phase 11: AutoSave
    t1 = std::chrono::high_resolution_clock::now();
    phaseAutoSave(deltaTime);
    t2 = std::chrono::high_resolution_clock::now();
    phaseTimings_[10].durationMs = std::chrono::duration<float, std::milli>(t2 - t1).count();

    // Phase 12: Render
    t1 = std::chrono::high_resolution_clock::now();
    phaseRender(deltaTime);
    t2 = std::chrono::high_resolution_clock::now();
    phaseTimings_[11].durationMs = std::chrono::duration<float, std::milli>(t2 - t1).count();

    // Update state manager
    if (stateManager_) {
        stateManager_->update(deltaTime);
    }

    // Process ImperialWeave event queue
    if (imperialWeave_ && imperialWeave_->isInitialized()) {
        imperialWeave_->getEventBus().processQueue();
    }

    frameCount_++;

    // Log frame timing every 300 frames (~5 seconds at 60fps)
    if (frameCount_ % 300 == 0) {
        float totalMs = 0.0f;
        for (int i = 0; i < PHASE_COUNT; ++i) {
            totalMs += phaseTimings_[i].durationMs;
        }
        LOGI("Frame %lu: total=%.2fms (In=%.2f Scr=%.2f AI=%.2f Phy=%.2f NPC=%.2f Cmb=%.2f Qst=%.2f Dlg=%.2f Wld=%.2f Aud=%.2f Sav=%.2f Ren=%.2f)",
             (unsigned long)frameCount_, totalMs,
             phaseTimings_[0].durationMs, phaseTimings_[1].durationMs,
             phaseTimings_[2].durationMs, phaseTimings_[3].durationMs,
             phaseTimings_[4].durationMs, phaseTimings_[5].durationMs,
             phaseTimings_[6].durationMs, phaseTimings_[7].durationMs,
             phaseTimings_[8].durationMs, phaseTimings_[9].durationMs,
             phaseTimings_[10].durationMs, phaseTimings_[11].durationMs);
    }
}

// ============================================================================
// Phase implementations
// ============================================================================

void GameLoopCoordinator::phaseInput(float dt) {
    (void)dt;
    if (inputRouter_) {
        inputRouter_->update(dt);
    }
}

void GameLoopCoordinator::phaseScriptVM(float dt) {
    if (scriptManager_) {
        scriptManager_->update(dt);
    }
}

void GameLoopCoordinator::phaseAI(float dt) {
    // AI is handled by NpcManager::update which includes AI scheduling
    // This phase is for any additional AI processing
    // The actual NPC AI runs in phaseNPC
}

void GameLoopCoordinator::phasePhysics(float dt) {
    if (physicsManager_) {
        physicsManager_->update(dt);
    }
}

void GameLoopCoordinator::phaseNPC(float dt) {
    if (npcManager_) {
        npcManager_->update(dt);
    }
}

void GameLoopCoordinator::phaseCombat(float dt) {
    if (combatManager_) {
        combatManager_->update(dt);
    }
}

void GameLoopCoordinator::phaseQuest(float dt) {
    if (questManager_) {
        questManager_->update(dt);
    }
}

void GameLoopCoordinator::phaseDialogue(float dt) {
    if (dialogueRunner_) {
        dialogueRunner_->update(dt);
    }
}

void GameLoopCoordinator::phaseWorld(float dt) {
    if (worldManager_) {
        worldManager_->update(dt);
    }
}

void GameLoopCoordinator::phaseAudio(float dt) {
    // Audio updates are typically event-driven
    // This phase handles ambient audio, music transitions, etc.
    (void)dt;
}

void GameLoopCoordinator::phaseAutoSave(float dt) {
    if (!saveManager_) return;

    autoSaveTimer_ += dt;
    if (autoSaveTimer_ >= autoSaveInterval_) {
        autoSaveTimer_ = 0.0f;
        LOGI("Auto-save triggered at frame %lu", (unsigned long)frameCount_);
        saveManager_->getAutoSave().triggerAutoSave();
    }

    // Let SaveManager handle its own auto-save timing too
    saveManager_->update(dt);
}

void GameLoopCoordinator::phaseRender(float dt) {
    if (renderer_) {
        renderer_->render(dt);
    }
}
