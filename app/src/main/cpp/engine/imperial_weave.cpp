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
    ::DistantLodManager* distantLod
) {
    renderer_ = renderer;
    worldManager_ = world;
    npcManager_ = npc;
    combatManager_ = combat;
    questManager_ = quest;
    physics_ = physics;
    animPlayer_ = anim;
    playerController_ = player;
    inventoryManager_ = inventory;
    spellManager_ = spell;
    audioManager_ = audio;
    joltPhysics_ = joltPhysics;
    scriptManager_ = script;
    distantLodManager_ = distantLod;

    // Register services for cross-module access
    if (renderer) locator_.registerService(renderer);
    if (world)    locator_.registerService(world);
    if (npc)      locator_.registerService(npc);
    if (combat)   locator_.registerService(combat);
    if (quest)    locator_.registerService(quest);
    if (physics)  locator_.registerService(physics);
    if (anim)     locator_.registerService(anim);
    if (player)   locator_.registerService(player);
    if (inventory) locator_.registerService(inventory);
    if (spell)    locator_.registerService(spell);
    if (audio)    locator_.registerService(audio);
    if (joltPhysics) locator_.registerService(joltPhysics);
    if (script)   locator_.registerService(script);
    if (distantLod) locator_.registerService(distantLod);

    initialized_ = true;
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
    joltPhysics_ = nullptr;
    scriptManager_ = nullptr;
    distantLodManager_ = nullptr;
    initialized_ = false;
}

void ImperialWeave::update(float deltaTime) {
    if (!initialized_) return;

#ifdef WEAVE_DEBUG
    PhaseProfiler::resetFrame();
#endif

    // v3: 例外安全 - 各フェーズを個別に保護
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
        WEAVE_PROFILE_PHASE(AudioUpdate,     phaseAudioUpdate(deltaTime));
        WEAVE_PROFILE_PHASE(RenderSubmit,    phaseRenderSubmit(deltaTime));
    } catch (const std::exception& e) {
        // ログ出力（デバッグビルド時は再スロー）
#ifdef WEAVE_DEBUG
        // LOGD("ImperialWeave::update exception: %s", e.what());
        throw;
#endif
        (void)e;
    }
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

void ImperialWeave::phaseRenderSubmit(float dt) {
    (void)dt;
    // Phase 50: Distant LOD rendering
    if (distantLodManager_ && renderer_) {
        // Build view-projection matrix from renderer state
        // LOD manager handles its own frustum culling and rendering
        glm::mat4 viewProj;  // Placeholder - actual VP from renderer
        distantLodManager_->update(dt);
        distantLodManager_->render(renderer_, viewProj);
    }
}

} // namespace weave
