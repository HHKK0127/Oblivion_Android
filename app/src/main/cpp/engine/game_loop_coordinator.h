#pragma once

#include "state_manager.h"
#include "input_router.h"
#include "imperial_weave.h"
#include <cstdint>
#include <chrono>
#include <android/log.h>

#define LOG_TAG "GameLoopCoordinator"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
#define LOGD(...) do {} while(0)
#endif
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Forward declarations (global namespace)
class Renderer;
class WorldManager;
class NpcManager;
class CombatManager;
class QuestManager;
class AudioManager;
class SaveManager;
class PlayerController;
class InventoryManager;
class SpellManager;

namespace oblivion {
class PhysicsManager;
namespace script {
class ScriptManager;
}
namespace dialogue {
class DialogueRunner;
}
}

// ============================================================================
// GameLoopCoordinator - Orchestrates all system updates
// Phase 42: Full game loop integration
// ============================================================================

// Phase timing data for profiling
struct PhaseTiming {
    const char* name;
    float durationMs;
};

class GameLoopCoordinator {
public:
    GameLoopCoordinator();
    ~GameLoopCoordinator();

    // Lifecycle
    bool initialize();
    void shutdown();

    // System registration
    void setRenderer(Renderer* renderer);
    void setWorldManager(WorldManager* world);
    void setNpcManager(NpcManager* npc);
    void setCombatManager(CombatManager* combat);
    void setQuestManager(QuestManager* quest);
    void setAudioManager(AudioManager* audio);
    void setSaveManager(SaveManager* save);
    void setPlayerController(PlayerController* player);
    void setInventoryManager(InventoryManager* inventory);
    void setSpellManager(SpellManager* spell);
    void setPhysicsManager(oblivion::PhysicsManager* physics);
    void setScriptManager(oblivion::script::ScriptManager* script);
    void setDialogueRunner(oblivion::dialogue::DialogueRunner* dialogue);
    void setStateManager(StateManager* state);
    void setInputRouter(InputRouter* input);

    // Main update - runs all 12 phases
    void update(float deltaTime);

    // Frame counter
    uint64_t getFrameCount() const { return frameCount_; }

    // Phase timing (last frame)
    const PhaseTiming* getPhaseTimings() const { return phaseTimings_; }
    int getPhaseTimingCount() const { return PHASE_COUNT; }

    // Auto-save interval (seconds)
    void setAutoSaveInterval(float seconds) { autoSaveInterval_ = seconds; }

private:
    static constexpr int PHASE_COUNT = 12;

    // System pointers (non-owning)
    Renderer* renderer_ = nullptr;
    WorldManager* worldManager_ = nullptr;
    NpcManager* npcManager_ = nullptr;
    CombatManager* combatManager_ = nullptr;
    QuestManager* questManager_ = nullptr;
    AudioManager* audioManager_ = nullptr;
    SaveManager* saveManager_ = nullptr;
    PlayerController* playerController_ = nullptr;
    InventoryManager* inventoryManager_ = nullptr;
    SpellManager* spellManager_ = nullptr;
    oblivion::PhysicsManager* physicsManager_ = nullptr;
    oblivion::script::ScriptManager* scriptManager_ = nullptr;
    oblivion::dialogue::DialogueRunner* dialogueRunner_ = nullptr;
    StateManager* stateManager_ = nullptr;
    InputRouter* inputRouter_ = nullptr;

    // ImperialWeave reference
    weave::ImperialWeave* imperialWeave_ = nullptr;

    // Frame state
    uint64_t frameCount_ = 0;
    float autoSaveTimer_ = 0.0f;
    float autoSaveInterval_ = 300.0f; // 5 minutes default

    // Phase timing
    PhaseTiming phaseTimings_[PHASE_COUNT];

    // 12-phase update pipeline
    void phaseInput(float dt);
    void phaseScriptVM(float dt);
    void phaseAI(float dt);
    void phasePhysics(float dt);
    void phaseNPC(float dt);
    void phaseCombat(float dt);
    void phaseQuest(float dt);
    void phaseDialogue(float dt);
    void phaseWorld(float dt);
    void phaseAudio(float dt);
    void phaseAutoSave(float dt);
    void phaseRender(float dt);
};
