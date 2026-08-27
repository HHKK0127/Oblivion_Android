#include "animation_subscriber.h"

namespace animation {

void AnimationSubscriber::init(weave::EventBus* bus, WorldLoader* loader) {
    eventBus = bus;
    worldLoader = loader;
    LOGI_SUB("AnimationSubscriber initialized");
}

void AnimationSubscriber::subscribeToEvents() {
    if (!eventBus) return;

    // Subscribe to NPC animation state change events
    eventBus->subscribe("NPC_ANIM_STATE_CHANGE", [this](const weave::Event& e) {
        // Parse payload: "npcId:state"
        size_t colonPos = e.payload.find(':');
        if (colonPos != std::string::npos) {
            uint32_t npcId = std::stoul(e.payload.substr(0, colonPos));
            int stateInt = std::stoi(e.payload.substr(colonPos + 1));
            NPC::AnimState state = static_cast<NPC::AnimState>(stateInt);
            setNpcAnimState(npcId, state);
        }
    });

    // Subscribe to combat events that trigger NPC animations
    eventBus->subscribe("COMBAT_ATTACK_HIT", [this](const weave::Event& e) {
        // Target NPC gets hit reaction
        if (e.targetId > 0) {
            setNpcAnimState(e.targetId, NPC::AnimState::HIT_REACTION);
        }
    });

    eventBus->subscribe("COMBAT_CRITICAL_HIT", [this](const weave::Event& e) {
        // Target NPC gets hit reaction
        if (e.targetId > 0) {
            setNpcAnimState(e.targetId, NPC::AnimState::HIT_REACTION);
        }
    });

    eventBus->subscribe("COMBAT_DEATH", [this](const weave::Event& e) {
        // Target NPC dies
        if (e.targetId > 0) {
            setNpcAnimState(e.targetId, NPC::AnimState::DEATH);
        }
    });

    LOGI_SUB("AnimationSubscriber subscribed to events");
}

void AnimationSubscriber::updateNpcAnimations(float deltaTime) {
    if (!worldLoader) return;

    for (auto& [npcId, info] : npcAnimStates) {
        // Update state timer
        if (info.stateTimer > 0.0f) {
            info.stateTimer -= deltaTime;
            if (info.stateTimer <= 0.0f) {
                info.stateTimer = 0.0f;
                // Return to idle after animation completes
                if (info.currentState == NPC::AnimState::HIT_REACTION ||
                    info.currentState == NPC::AnimState::ATTACK) {
                    info.previousState = info.currentState;
                    info.currentState = NPC::AnimState::IDLE;
                    info.stateChanged = true;
                }
            }
        }

        // Play animation if state changed
        if (info.stateChanged) {
            WorldEntity* entity = worldLoader->getEntityByNpcId(npcId);
            if (entity && entity->animator) {
                auto animNames = getAnimNamesForState(info.currentState);
                bool loop = (info.currentState == NPC::AnimState::IDLE ||
                            info.currentState == NPC::AnimState::WALK ||
                            info.currentState == NPC::AnimState::RUN);
                playFirstAvailableAnimation(entity, animNames, loop);
            }
            info.stateChanged = false;
        }
    }
}

void AnimationSubscriber::setNpcAnimState(uint32_t npcId, NPC::AnimState state) {
    auto& info = npcAnimStates[npcId];

    // Don't interrupt death animation
    if (info.currentState == NPC::AnimState::DEATH && state != NPC::AnimState::DEATH) {
        return;
    }

    // Set new state
    info.previousState = info.currentState;
    info.currentState = state;
    info.stateChanged = true;

    // Set timer for timed animations
    switch (state) {
        case NPC::AnimState::HIT_REACTION:
            info.stateTimer = NPC::HIT_REACTION_DURATION;
            break;
        case NPC::AnimState::ATTACK:
            info.stateTimer = NPC::ATTACK_DURATION;
            break;
        default:
            info.stateTimer = 0.0f;
            break;
    }

    LOGD_SUB("NPC %u anim state set to %d", npcId, static_cast<int>(state));
}

void AnimationSubscriber::playAnimation(WorldEntity* entity, const std::string& animName, bool loop) {
    if (!entity || !entity->animator) return;

    // Find animation sequence by name
    int32_t animIndex = entity->animator->findSequenceByName(animName);
    if (animIndex >= 0) {
        // Stop all current animations
        entity->animator->stopAll();

        // Play new animation
        entity->animator->play(static_cast<uint32_t>(animIndex), loop, 1.0f);
        LOGD_SUB("Playing animation '%s' (index=%d) on entity #%u", animName.c_str(), animIndex, entity->entityId);
    } else {
        // Fallback: play first animation if name not found
        if (entity->animator->getSequenceCount() > 0) {
            entity->animator->stopAll();
            entity->animator->play(0, loop, 1.0f);
            LOGD_SUB("Animation '%s' not found, playing first animation on entity #%u", animName.c_str(), entity->entityId);
        } else {
            LOGD_SUB("No animations available for entity #%u", entity->entityId);
        }
    }
}

void AnimationSubscriber::playFirstAvailableAnimation(WorldEntity* entity, const std::vector<std::string>& animNames, bool loop) {
    if (!entity || !entity->animator) return;

    for (const auto& animName : animNames) {
        int32_t animIndex = entity->animator->findSequenceByName(animName);
        if (animIndex >= 0) {
            entity->animator->stopAll();
            entity->animator->play(static_cast<uint32_t>(animIndex), loop, 1.0f);
            LOGD_SUB("Playing animation '%s' (index=%d) on entity #%u", animName.c_str(), animIndex, entity->entityId);
            return;
        }
    }

    // Fallback: play first animation if none of the preferred names found
    if (entity->animator->getSequenceCount() > 0) {
        entity->animator->stopAll();
        entity->animator->play(0, loop, 1.0f);
        LOGD_SUB("None of preferred animations found, playing first animation on entity #%u", entity->entityId);
    } else {
        LOGD_SUB("No animations available for entity #%u", entity->entityId);
    }
}

std::vector<std::string> AnimationSubscriber::getAnimNamesForState(NPC::AnimState state) const {
    switch (state) {
        case NPC::AnimState::IDLE:
            return {"idle", "Idle", "idle1", "Idle1", "idle_loop"};
        case NPC::AnimState::WALK:
            return {"walk", "Walk", "forward", "Forward", "walkforward", "WalkForward"};
        case NPC::AnimState::RUN:
            return {"run", "Run", "runforward", "RunForward"};
        case NPC::AnimState::ATTACK:
            return {"attack", "Attack", "attackleft", "AttackLeft", "attackright", "AttackRight",
                    "attackpower", "AttackPower", "attack1", "Attack1"};
        case NPC::AnimState::HIT_REACTION:
            return {"recoil", "Recoil", "hit_reaction", "Hit_Reaction", "hit", "Hit",
                    "stagger", "Stagger", "flinch", "Flinch"};
        case NPC::AnimState::BLOCK:
            return {"block", "Block", "blockidle", "BlockIdle", "blockhit", "BlockHit"};
        case NPC::AnimState::DEATH:
            return {"death", "Death", "death1", "Death1", "die", "Die"};
        default:
            return {"idle", "Idle"};
    }
}

} // namespace animation