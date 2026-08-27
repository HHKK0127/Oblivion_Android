#pragma once
#include <vector>
#include <functional>
#include <string>
#include <unordered_map>
#include <queue>
#include <cstdint>
#include <memory>
#include <mutex>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <typeindex>
#include <typeinfo>

// ============================================================================
// Imperial Weave v3.0
// 統合レイヤー: EventBus + ServiceLocator + Coordinator
// v3: type_index化 / shared_ptr EventHandlers / 例外安全
// ============================================================================

// Forward declarations (existing systems - global namespace)
class Renderer;
class WorldManager;
class NpcManager;
class CombatManager;
class QuestManager;
class CollisionWorld;
class PlayerController;
class InventoryManager;
class SpellManager;
class AudioManager;

namespace oblivion {
class PhysicsManager;
}

namespace animation {
class AnimationPlayer;
}

namespace weave {

// ============================================================================
// Debug profiling macro
// ============================================================================

#ifdef WEAVE_DEBUG
    #define WEAVE_PROFILE_PHASE(name, code) \
        auto __t1_##name = std::chrono::high_resolution_clock::now(); \
        code; \
        auto __t2_##name = std::chrono::high_resolution_clock::now(); \
        double __dt_##name = std::chrono::duration<double, std::milli>(__t2_##name - __t1_##name).count(); \
        PhaseProfiler::record(#name, __dt_##name);
#else
    #define WEAVE_PROFILE_PHASE(name, code) code
#endif

// ============================================================================
// PhaseProfiler - frame timing data collection (debug only)
// ============================================================================

#ifdef WEAVE_DEBUG
class PhaseProfiler {
public:
    struct Record {
        std::string phase;
        double durationMs;
        uint64_t frameNumber;
    };

    static void record(const std::string& phase, double ms);
    static std::vector<Record> getLastFrameRecords();
    static std::string getSummaryString();
    static void resetFrame();
    static uint64_t getFrameNumber();

private:
    static std::vector<Record> currentRecords_;
    static std::vector<Record> lastFrameRecords_;
    static uint64_t frameNumber_;
    static std::mutex mutex_;
};
#endif

// ============================================================================
// EventBus - thread-safe event system
// ============================================================================

struct Event {
    std::string type;
    uint32_t sender = 0;
    uint32_t targetId = 0;  // Target entity/NPC ID (for combat events etc.)
    float time = 0.0f;
    std::string payload;

    // Factory methods for common event types
    static Event animEvent(uint32_t senderId, const std::string& key, float t) {
        Event e;
        e.type = "ANIM_EVENT";
        e.sender = senderId;
        e.time = t;
        e.payload = key;
        return e;
    }
    static Event collision(uint32_t senderId, const std::string& info) {
        Event e;
        e.type = "COLLISION";
        e.sender = senderId;
        e.payload = info;
        return e;
    }
    static Event death(uint32_t senderId, const std::string& entityType) {
        Event e;
        e.type = "ENTITY_DEATH";
        e.sender = senderId;
        e.payload = entityType;
        return e;
    }
    static Event questTrigger(uint32_t senderId, const std::string& questId) {
        Event e;
        e.type = "QUEST_TRIGGER";
        e.sender = senderId;
        e.payload = questId;
        return e;
    }
};

class EventBus {
public:
    using Handler = std::function<void(const Event&)>;

    void subscribe(const std::string& type, Handler handler);
    void unsubscribe(const std::string& type);

    // Immediate dispatch (runs on calling thread)
    void emitImmediate(const Event& event);

    // Queue for deferred processing (thread-safe)
    void emit(const Event& event);

    // Process queued events (call from GL thread)
    void processQueue();
    void clearQueue();

    size_t getQueueSize() const;

private:
    mutable std::mutex handlersMutex_;
    mutable std::mutex queueMutex_;
    // v3: shared_ptr でコピー回避（emitImmediate での vector コピーを防止）
    std::unordered_map<std::string, std::shared_ptr<std::vector<Handler>>> handlers_;
    std::vector<Event> queue_;
};

// ============================================================================
// ServiceLocator - thread-safe service registry
// ============================================================================

class ServiceLocator {
public:
    template<typename T>
    void registerService(T* service) {
        std::lock_guard<std::mutex> lock(mutex_);
        services_[std::type_index(typeid(T))] = static_cast<void*>(service);
    }

    template<typename T>
    T* get() const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = services_.find(std::type_index(typeid(T)));
        if (it != services_.end()) {
            return static_cast<T*>(it->second);
        }
#ifdef WEAVE_DEBUG
        // v3: デバッグ時にサービス未登録を警告
        // LOGD("ServiceLocator: service not found for type");
#endif
        return nullptr;
    }

    void clear();
    bool has(const std::string& typeName) const;

private:
    mutable std::mutex mutex_;
    // v3: typeid(T).name() はコンパイラ依存 → type_index で統一
    std::unordered_map<std::type_index, void*> services_;
};

// ============================================================================
// ImperialWeave - integration coordinator
// ============================================================================

class ImperialWeave {
public:
    static ImperialWeave& instance();

    void init(
        ::Renderer* renderer,
        ::WorldManager* world,
        ::NpcManager* npc,
        ::CombatManager* combat,
        ::QuestManager* quest,
        ::CollisionWorld* physics,
        ::animation::AnimationPlayer* anim,
        ::PlayerController* player = nullptr,
        ::InventoryManager* inventory = nullptr,
        ::SpellManager* spell = nullptr,
        ::AudioManager* audio = nullptr,
        ::oblivion::PhysicsManager* joltPhysics = nullptr
    );

    void shutdown();
    void update(float deltaTime);

    EventBus& getEventBus() { return eventBus_; }
    ServiceLocator& getLocator() { return locator_; }

    ::CollisionWorld* physics() const { return physics_; }
    ::animation::AnimationPlayer* anim() const { return animPlayer_; }
        ::NpcManager* npc() const { return npcManager_; }

    bool isInitialized() const { return initialized_; }

private:
    ImperialWeave() = default;
    ~ImperialWeave() = default;
    ImperialWeave(const ImperialWeave&) = delete;
    ImperialWeave& operator=(const ImperialWeave&) = delete;

    EventBus eventBus_;
    ServiceLocator locator_;

    ::Renderer* renderer_ = nullptr;
    ::WorldManager* worldManager_ = nullptr;
    ::NpcManager* npcManager_ = nullptr;
    ::CombatManager* combatManager_ = nullptr;
    ::QuestManager* questManager_ = nullptr;
    ::CollisionWorld* physics_ = nullptr;
    ::animation::AnimationPlayer* animPlayer_ = nullptr;
    ::PlayerController* playerController_ = nullptr;
    ::InventoryManager* inventoryManager_ = nullptr;
    ::SpellManager* spellManager_ = nullptr;
    ::AudioManager* audioManager_ = nullptr;
    ::oblivion::PhysicsManager* joltPhysics_ = nullptr;

    bool initialized_ = false;

    // 12-phase update pipeline
    void phasePreUpdate(float dt);
    void phaseEventProcess();
    void phaseWorldUpdate(float dt);
    void phaseAiUpdate(float dt);
    void phasePlayerUpdate(float dt);
    void phaseInventoryUpdate(float dt);
    void phaseSpellUpdate(float dt);
    void phaseAnimationUpdate(float dt);
    void phasePhysicsSync(float dt);
    void phaseJoltPhysicsUpdate(float dt);
    void phaseCombatUpdate(float dt);
    void phaseQuestUpdate(float dt);
    void phaseAudioUpdate(float dt);
    void phaseRenderSubmit(float dt);
};

} // namespace weave
