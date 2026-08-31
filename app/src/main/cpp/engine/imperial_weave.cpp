#include "imperial_weave.h"
#include <algorithm>
#include <stdexcept>
#include <typeindex>

// Existing system headers
#include "renderer.h"
#include "../world/world_manager.h"
#include "../game/npc_manager.h"
#include "../game/combat_manager.h"
#include "../game/quest_manager.h"
#include "../collision/collision_world.h"
#include "../animation/animation_player.h"
#include "../physics/physics_manager.h"
#include "../script/script_manager.h"
#include "../world/distant_lod/distant_lod_manager.h"
#include "../vegetation/speed_tree_manager.h"
#include "../character/face_gen_morpher.h"
#include "../video/bink_video_player.h"

namespace weave {

// ============================================================================
// PhaseProfiler implementation (debug builds only)
// ============================================================================

#ifdef WEAVE_DEBUG
std::vector<PhaseProfiler::Record> PhaseProfiler::currentRecords_;
std::vector<PhaseProfiler::Record> PhaseProfiler::lastFrameRecords_;
uint64_t PhaseProfiler::frameNumber_ = 0;
std::mutex PhaseProfiler::mutex_;

void PhaseProfiler::record(const std::string& phase, double ms) {
    std::lock_guard<std::mutex> lock(mutex_);
    currentRecords_.push_back({phase, ms, frameNumber_});
}

std::vector<PhaseProfiler::Record> PhaseProfiler::getLastFrameRecords() {
    std::lock_guard<std::mutex> lock(mutex_);
    return lastFrameRecords_;
}

std::string PhaseProfiler::getSummaryString() {
    std::lock_guard<std::mutex> lock(mutex_);
    std::ostringstream oss;
    oss << "=== Imperial Weave Frame " << frameNumber_ << " ===\n";
    double total = 0.0;
    for (const auto& r : lastFrameRecords_) {
        oss << std::setw(20) << std::left << r.phase << ": "
            << std::fixed << std::setprecision(3) << std::setw(8) << r.durationMs << " ms\n";
        total += r.durationMs;
    }
    oss << std::setw(20) << "TOTAL" << ": " << std::fixed << std::setprecision(3) << total << " ms\n";
    return oss.str();
}

void PhaseProfiler::resetFrame() {
    std::lock_guard<std::mutex> lock(mutex_);
    lastFrameRecords_.swap(currentRecords_);
    currentRecords_.clear();
    ++frameNumber_;
}

uint64_t PhaseProfiler::getFrameNumber() {
    std::lock_guard<std::mutex> lock(mutex_);
    return frameNumber_;
}
#endif

// ============================================================================
// EventBus implementation (thread-safe)
// ============================================================================

void EventBus::subscribe(const std::string& type, Handler handler) {
    std::lock_guard<std::mutex> lock(handlersMutex_);
    auto it = handlers_.find(type);
    if (it == handlers_.end()) {
        it = handlers_.emplace(type, std::make_shared<std::vector<Handler>>()).first;
    }
    it->second->push_back(std::move(handler));
}

void EventBus::unsubscribe(const std::string& type) {
    std::lock_guard<std::mutex> lock(handlersMutex_);
    handlers_.erase(type);
}

void EventBus::emitImmediate(const Event& event) {
    // v3: shared_ptr のコピーのみ（vector コピーを回避）
    std::shared_ptr<std::vector<Handler>> localHandlers;
    {
        std::lock_guard<std::mutex> lock(handlersMutex_);
        auto it = handlers_.find(event.type);
        if (it == handlers_.end()) return;
        localHandlers = it->second;
    }
    for (const auto& handler : *localHandlers) {
        if (handler) handler(event);
    }
}

void EventBus::emit(const Event& event) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    queue_.push_back(event);
}

void EventBus::processQueue() {
    std::vector<Event> localQueue;
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        if (queue_.empty()) return;
        localQueue.swap(queue_);
    }

    for (const auto& event : localQueue) {
        // v3: shared_ptr のコピーのみ
        std::shared_ptr<std::vector<Handler>> localHandlers;
        {
            std::lock_guard<std::mutex> lock(handlersMutex_);
            auto it = handlers_.find(event.type);
            if (it == handlers_.end()) continue;
            localHandlers = it->second;
        }
        for (const auto& handler : *localHandlers) {
            if (handler) handler(event);
        }
    }
}

void EventBus::clearQueue() {
    std::lock_guard<std::mutex> lock(queueMutex_);
    queue_.clear();
}

size_t EventBus::getQueueSize() const {
    std::lock_guard<std::mutex> lock(queueMutex_);
    return queue_.size();
}

// ============================================================================
// ServiceLocator implementation
// ============================================================================

void ServiceLocator::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    services_.clear();
}

bool ServiceLocator::has(const std::string& typeName) const {
    // v3: type_index ベースでは文字列検索不可。デバッグ用に残すが常に false
    (void)typeName;
    return false;
}

// ============================================================================
// ImperialWeave implementation
// ============================================================================

ImperialWeave& ImperialWeave::instance() {
    static ImperialWeave inst;
    return inst;
}

// v4: Config-based init (preferred)
void ImperialWeave::init(const ImperialWeaveConfig& config) {
    renderer_ = config.renderer;
    worldManager_ = config.world;
    npcManager_ = config.npc;
    combatManager_ = config.combat;
    questManager_ = config.quest;
    physics_ = config.physics;
    animPlayer_ = config.anim;
    playerController_ = config.player;
    inventoryManager_ = config.inventory;
    spellManager_ = config.spell;
    audioManager_ = config.audio;
    joltPhysics_ = config.joltPhysics;
    scriptManager_ = config.script;
    distantLodManager_ = config.distantLod;
    speedTreeManager_ = config.speedTree;
    faceGenMorpher_ = config.faceGen;
    binkVideoPlayer_ = config.binkVideo;
    frameBudgetMs_ = config.frameBudgetMs;

    // Register services for cross-module access
    if (renderer_) locator_.registerService(renderer_);
    if (worldManager_) locator_.registerService(worldManager_);
    if (npcManager_) locator_.registerService(npcManager_);
    if (combatManager_) locator_.registerService(combatManager_);
    if (questManager_) locator_.registerService(questManager_);
    if (physics_) locator_.registerService(physics_);
    if (animPlayer_) locator_.registerService(animPlayer_);
    if (playerController_) locator_.registerService(playerController_);
    if (inventoryManager_) locator_.registerService(inventoryManager_);
    if (spellManager_) locator_.registerService(spellManager_);
    if (audioManager_) locator_.registerService(audioManager_);
    if (joltPhysics_) locator_.registerService(joltPhysics_);
    if (scriptManager_) locator_.registerService(scriptManager_);
    if (distantLodManager_) locator_.registerService(distantLodManager_);
    if (speedTreeManager_) locator_.registerService(speedTreeManager_);
    if (faceGenMorpher_) locator_.registerService(faceGenMorpher_);
    if (binkVideoPlayer_) locator_.registerService(binkVideoPlayer_);

    // v4: Subscribe to new engine events
    eventBus_.subscribe("TREE_WIND_CHANGE", [this](const Event& e) {
        // Forward wind changes to SpeedTree for dynamic wind response
        if (speedTreeManager_) {
            // SpeedTree reads wind params internally during update
        }
    });
    eventBus_.subscribe("FACE_MORPH_UPDATE", [this](const Event& e) {
        // FaceGen morph target updates are handled in phaseFaceGenUpdate
    });
    eventBus_.subscribe("VIDEO_PLAYBACK_EVENT", [this](const Event& e) {
        // BinkVideo playback state changes
        if (binkVideoPlayer_) {
            // Video state is queried via isPlaying() in phaseVideoUpdate
        }
    });
    eventBus_.subscribe("LOD_DISTANCE_CHANGE", [this](const Event& e) {
        // Distant LOD distance threshold changes
        if (distantLodManager_) {
            // LOD distances are updated in phaseRenderSubmit
        }
    });

    initialized_ = true;
}

// v3 backward compatibility: positional init
void ImperialWeave::init(
    ::Renderer* renderer,
    ::WorldManager* world,
    ::NpcManager* npc,
    ::CombatManager* combat,
    ::QuestManager* quest,
    ::CollisionWorld* physics,
    ::animation::AnimationPlayer* anim,
    ::PlayerController* player,
    ::InventoryManager* inventory,
    ::SpellManager* spell,
    ::AudioManager* audio,
    ::oblivion::PhysicsManager* joltPhysics,
    ::oblivion::script::ScriptManager* script,
    ::DistantLodManager* distantLod,
    ::vegetation::SpeedTreeManager* speedTree,
    ::facegen::FaceGenMorpher* faceGen,
    ::oblivion::video::BinkVideoPlayer* binkVideo
) {
    ImperialWeaveConfig config;
    config.renderer = renderer;
    config.world = world;
    config.npc = npc;
    config.combat = combat;
    config.quest = quest;
    config.physics = physics;
    config.anim = anim;
    config.player = player;
    config.inventory = inventory;
    config.spell = spell;
    config.audio = audio;
    config.joltPhysics = joltPhysics;
    config.script = script;
    config.distantLod = distantLod;
    config.speedTree = speedTree;
    config.faceGen = faceGen;
    config.binkVideo = binkVideo;
    init(config);
}

void ImperialWeave::shutdown() {
    eventBus_.clearQueue();
    locator_.clear();
    renderer_ = nullptr;
    worldManager_ = nullptr;
    npcManager_ = nullptr;
    combatManager_ = nullptr;
    questManager_ = nullptr;
    physics_ = nullptr;
    animPlayer_ = nullptr;
    playerController_ = nullptr;
    inventoryManager_ = nullptr;
    spellManager_ = nullptr;
    audioManager_ = nullptr;
    joltPhysics_ = nullptr;
    scriptManager_ = nullptr;
    distantLodManager_ = nullptr;
    speedTreeManager_ = nullptr;
    faceGenMorpher_ = nullptr;
    binkVideoPlayer_ = nullptr;
    lastFrameTimeMs_ = 0.0f;
    frameBudgetExceeded_ = false;
    initialized_ = false;
}

// v4: Frame budget check helper
bool ImperialWeave::withinBudget(std::chrono::high_resolution_clock::time_point frameStart) const {
    auto now = std::chrono::high_resolution_clock::now();
    float elapsed = std::chrono::duration<float, std::milli>(now - frameStart).count();
    return elapsed < frameBudgetMs_;
}

void ImperialWeave::update(float deltaTime) {
    if (!initialized_) return;

    auto frameStart = std::chrono::high_resolution_clock::now();

#ifdef WEAVE_DEBUG
    PhaseProfiler::resetFrame();
#endif

    // v4: 15-phase pipeline with frame budget
    try {
        WEAVE_PROFILE_PHASE(PreUpdate,       phasePreUpdate(deltaTime));
        WEAVE_PROFILE_PHASE(EventProcess,    phaseEventProcess());
        WEAVE_PROFILE_PHASE(WorldUpdate,     phaseWorldUpdate(deltaTime));
        WEAVE_PROFILE_PHASE(AiUpdate,        phaseAiUpdate(deltaTime));
        WEAVE_PROFILE_PHASE(PlayerUpdate,    phasePlayerUpdate(deltaTime));
        WEAVE_PROFILE_PHASE(InventoryUpdate, phaseInventoryUpdate(deltaTime));
        WEAVE_PROFILE_PHASE(SpellUpdate,     phaseSpellUpdate(deltaTime));
        WEAVE_PROFILE_PHASE(AnimationUpdate, phaseAnimationUpdate(deltaTime));
        WEAVE_PROFILE_PHASE(JoltPhysics,     phaseJoltPhysicsUpdate(deltaTime));
        WEAVE_PROFILE_PHASE(PhysicsSync,     phasePhysicsSync(deltaTime));
        WEAVE_PROFILE_PHASE(CombatUpdate,    phaseCombatUpdate(deltaTime));
        WEAVE_PROFILE_PHASE(QuestUpdate,     phaseQuestUpdate(deltaTime));
        WEAVE_PROFILE_PHASE(ScriptUpdate,    phaseScriptUpdate(deltaTime));

        // v4: New engine phases (skip if over budget)
        if (withinBudget(frameStart)) {
            WEAVE_PROFILE_PHASE(VegetationUpdate, phaseVegetationUpdate(deltaTime));
        }
        if (withinBudget(frameStart)) {
            WEAVE_PROFILE_PHASE(FaceGenUpdate, phaseFaceGenUpdate(deltaTime));
        }
        if (withinBudget(frameStart)) {
            WEAVE_PROFILE_PHASE(VideoUpdate, phaseVideoUpdate(deltaTime));
        }

        WEAVE_PROFILE_PHASE(AudioUpdate,     phaseAudioUpdate(deltaTime));
        WEAVE_PROFILE_PHASE(RenderSubmit,    phaseRenderSubmit(deltaTime));
    } catch (const std::exception& e) {
#ifdef WEAVE_DEBUG
        throw;
#endif
        (void)e;
    }

    // v4: Track frame time
    auto frameEnd = std::chrono::high_resolution_clock::now();
    lastFrameTimeMs_ = std::chrono::duration<float, std::milli>(frameEnd - frameStart).count();
    frameBudgetExceeded_ = (lastFrameTimeMs_ > frameBudgetMs_);
}

// --- Phase implementations ---

void ImperialWeave::phasePreUpdate(float dt) {
    (void)dt;
    // Reserved: frame start bookkeeping, delta time clamping, etc.
}

void ImperialWeave::phaseEventProcess() {
    eventBus_.processQueue();
}

void ImperialWeave::phaseWorldUpdate(float dt) {
    if (worldManager_) worldManager_->update(dt);
}

void ImperialWeave::phaseAiUpdate(float dt) {
    if (npcManager_) npcManager_->update(dt);
}

void ImperialWeave::phaseAnimationUpdate(float dt) {
    if (animPlayer_) animPlayer_->update(dt);
}

void ImperialWeave::phasePhysicsSync(float dt) {
    if (physics_) physics_->step(dt);
}

void ImperialWeave::phaseJoltPhysicsUpdate(float dt) {
    if (joltPhysics_) joltPhysics_->update(dt);
}

void ImperialWeave::phaseCombatUpdate(float dt) {
    if (combatManager_) combatManager_->update(dt);
}

void ImperialWeave::phaseQuestUpdate(float dt) {
    if (questManager_) questManager_->update(dt);
}

void ImperialWeave::phaseScriptUpdate(float dt) {
    if (scriptManager_) scriptManager_->update(dt);
}

void ImperialWeave::phasePlayerUpdate(float dt) {
    if (playerController_) playerController_->update(dt);
}

void ImperialWeave::phaseInventoryUpdate(float dt) {
    if (inventoryManager_) inventoryManager_->update(dt);
}

void ImperialWeave::phaseSpellUpdate(float dt) {
    if (spellManager_) spellManager_->update(dt);
}

void ImperialWeave::phaseAudioUpdate(float dt) {
    if (audioManager_) audioManager_->update(dt);
}

// v4: SpeedTree vegetation update
void ImperialWeave::phaseVegetationUpdate(float dt) {
    if (speedTreeManager_) {
        // Update wind field simulation
        // Update LOD transitions based on camera distance
        // Update billboard generation for distant trees
        glm::vec3 windDir(1.0f, 0.0f, 0.5f); // Default wind direction
        speedTreeManager_->update(dt, windDir);
    }
}

// v4: FaceGen morph update
void ImperialWeave::phaseFaceGenUpdate(float dt) {
    (void)dt;
    if (faceGenMorpher_) {
        // FaceGen morphing is request-driven, no per-frame update needed
        // Morph targets are applied when face generation is requested
    }
}

// v4: BinkVideo frame update
void ImperialWeave::phaseVideoUpdate(float dt) {
    if (binkVideoPlayer_) {
        // Decode next video frame if playing
        // Upload decoded frame to GPU texture
        // Handle loop/end-of-video transitions
        binkVideoPlayer_->update(dt);
    }
}

void ImperialWeave::phaseRenderSubmit(float dt) {
    (void)dt;

    // Phase 50: Distant LOD rendering
    if (distantLodManager_ && renderer_) {
        glm::mat4 viewProj;
        glm::vec3 cameraPos;
        if (worldManager_) {
            cameraPos = worldManager_->getCameraPosition();
            // TODO: Get actual view-projection matrix from renderer when available
        }
        distantLodManager_->update(dt);
        distantLodManager_->render(renderer_, viewProj);
    }

    // v4: Phase 51: SpeedTree vegetation rendering
    if (speedTreeManager_ && renderer_) {
        // SpeedTree handles its own instanced rendering
        // Billboard trees rendered for distant LOD
        glm::mat4 viewProj;
        glm::vec3 cameraPos;
        if (worldManager_) {
            cameraPos = worldManager_->getCameraPosition();
            // TODO: Get actual view-projection matrix from renderer when available
        }
        speedTreeManager_->render(renderer_, viewProj, cameraPos);
    }

    // v4: Phase 52: FaceGen face rendering
    if (faceGenMorpher_ && renderer_) {
        // FaceGen morphed meshes are rendered as part of NPC rendering
        // Texture compositing happens during face generation, not per-frame
    }

    // v4: Phase 53: BinkVideo texture overlay
    if (binkVideoPlayer_ && renderer_) {
        // Video frames are rendered as fullscreen overlay when playing
        if (binkVideoPlayer_->isPlaying()) {
            binkVideoPlayer_->update(dt);
        }
    }
}

} // namespace weave
