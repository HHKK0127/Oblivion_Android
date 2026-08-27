#pragma once

// Animation Subscriber
// Listens for NPC animation events via Imperial Weave EventBus
// and plays appropriate animations on WorldEntities

#include "../engine/imperial_weave.h"
#include "../world/world_entity.h"
#include "../game/npc.h"
#include <memory>
#include <unordered_map>
#include <android/log.h>

#define LOG_TAG "AnimationSubscriber"
#define LOGD_SUB(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI_SUB(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

namespace animation {

class AnimationSubscriber {
public:
    AnimationSubscriber() = default;
    ~AnimationSubscriber() = default;

    // Initialize with EventBus and WorldLoader
    void init(weave::EventBus* bus, WorldLoader* loader);

    // Subscribe to NPC animation events
    void subscribeToEvents();

    // Update NPC animations based on state
    void updateNpcAnimations(float deltaTime);

    // Set NPC animation state (called by CombatManager or NPC update)
    void setNpcAnimState(uint32_t npcId, NPC::AnimState state);

private:
    weave::EventBus* eventBus = nullptr;
    WorldLoader* worldLoader = nullptr;

    // NPC animation state tracking
    struct NpcAnimInfo {
        NPC::AnimState currentState = NPC::AnimState::IDLE;
        NPC::AnimState previousState = NPC::AnimState::IDLE;
        float stateTimer = 0.0f;
        bool stateChanged = false;
    };

    std::unordered_map<uint32_t, NpcAnimInfo> npcAnimStates;

    // Play animation on WorldEntity
    void playAnimation(WorldEntity* entity, const std::string& animName, bool loop = false);

    // Get animation names for state (multiple fallbacks)
    std::vector<std::string> getAnimNamesForState(NPC::AnimState state) const;

    // Try to play first available animation from list
    void playFirstAvailableAnimation(WorldEntity* entity, const std::vector<std::string>& animNames, bool loop);
};

} // namespace animation