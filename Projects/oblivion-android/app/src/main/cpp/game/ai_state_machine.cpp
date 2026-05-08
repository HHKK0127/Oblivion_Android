#include "ai_state_machine.h"
#include <algorithm>

AIStateMachine::AIStateMachine() {
    LOGD("AIStateMachine created");
}

AIStateMachine::~AIStateMachine() {
    LOGD("AIStateMachine destroyed");
}

void AIStateMachine::addTransition(const AITransition& transition) {
    transitions.push_back(transition);
    LOGD("Transition added: %d -> %d (event: %d)",
         static_cast<int>(transition.fromState),
         static_cast<int>(transition.toState),
         static_cast<int>(transition.trigger));
}

void AIStateMachine::addTransition(AIState from, AIState to, AIEvent trigger,
                                    std::function<bool(std::shared_ptr<NPC>)> condition,
                                    float timeout) {
    AITransition trans(from, to, trigger, condition, timeout);
    addTransition(trans);
}

AIStateMachine::StateInfo* AIStateMachine::findOrCreateStateInfo(uint32_t npcId) {
    // Check if state info already exists
    auto it = npcStates.find(npcId);
    if (it != npcStates.end()) {
        return &it->second;
    }

    // Create new state info
    StateInfo newInfo;
    newInfo.npcId = npcId;
    newInfo.currentState = AIState::IDLE;
    newInfo.stateTime = 0.0f;

    npcStates[npcId] = newInfo;
    LOGD("State info created for NPC %u (initial state: %d)",
         npcId, static_cast<int>(newInfo.currentState));

    return &npcStates[npcId];
}

bool AIStateMachine::canTransition(std::shared_ptr<NPC> npc, const AITransition& trans) const {
    if (!npc) return false;

    // Check if condition is satisfied (if one exists)
    if (trans.condition) {
        return trans.condition(npc);
    }

    // No condition = always can transition
    return true;
}

void AIStateMachine::processEvent(std::shared_ptr<NPC> npc, AIEvent event) {
    if (!npc) {
        LOGW("processEvent called with null NPC");
        return;
    }

    auto stateInfo = findOrCreateStateInfo(npc->npcId);

    LOGD("NPC %u processing event %d (current state: %d)",
         npc->npcId, static_cast<int>(event), static_cast<int>(stateInfo->currentState));

    // Find applicable transitions for this event
    for (const auto& trans : transitions) {
        // Check if this transition applies
        if (trans.fromState != stateInfo->currentState || trans.trigger != event) {
            continue;
        }

        // Check transition condition
        if (!canTransition(npc, trans)) {
            LOGD("NPC %u transition blocked by condition (%d -> %d)",
                 npc->npcId, static_cast<int>(trans.fromState),
                 static_cast<int>(trans.toState));
            continue;
        }

        // Perform transition
        AIState oldState = stateInfo->currentState;
        stateInfo->currentState = trans.toState;
        stateInfo->stateTime = 0.0f;

        LOGI("NPC %u transitioned: %d -> %d (event: %d)",
             npc->npcId, static_cast<int>(oldState),
             static_cast<int>(trans.toState), static_cast<int>(event));

        // Update NPC's AI state
        npc->setAIState(trans.toState);

        return;  // Only process first matching transition
    }

    LOGD("NPC %u: No transition found for event %d from state %d",
         npc->npcId, static_cast<int>(event), static_cast<int>(stateInfo->currentState));
}

void AIStateMachine::update(std::shared_ptr<NPC> npc, float deltaTime) {
    if (!npc) return;

    auto stateInfo = findOrCreateStateInfo(npc->npcId);

    // Update state time
    stateInfo->stateTime += deltaTime;

    // Check for timeout-based transitions
    for (const auto& trans : transitions) {
        if (trans.fromState != stateInfo->currentState) continue;
        if (trans.timeout <= 0.0f) continue;  // Skip non-timeout transitions

        if (stateInfo->stateTime >= trans.timeout) {
            LOGD("NPC %u state timeout: %d (%.2f/%.2f seconds)",
                 npc->npcId, static_cast<int>(stateInfo->currentState),
                 stateInfo->stateTime, trans.timeout);

            processEvent(npc, AIEvent::TIMEOUT);
            return;
        }
    }

    // Health-based event triggers (automatic events)
    if (npc->status.isAlive()) {
        // Prevent division by zero
        float hpPercent = (npc->status.maxHealth > 0.0f)
            ? npc->status.currentHealth / npc->status.maxHealth
            : 1.0f;

        // Health low event (30%)
        if (hpPercent < 0.3f && hpPercent >= 0.2f) {
            processEvent(npc, AIEvent::HEALTH_LOW);
        }
        // Health critical event (20%)
        else if (hpPercent < 0.2f) {
            processEvent(npc, AIEvent::HEALTH_CRITICAL);
        }

        // Mana low event (20%)
        float manaPercent = (npc->status.maxMana > 0.0f)
            ? npc->status.currentMana / npc->status.maxMana
            : 1.0f;
        if (manaPercent < 0.2f) {
            processEvent(npc, AIEvent::MANA_LOW);
        }
    }
}

AIState AIStateMachine::getCurrentState(std::shared_ptr<NPC> npc) const {
    if (!npc) return AIState::IDLE;

    auto it = npcStates.find(npc->npcId);
    if (it != npcStates.end()) {
        return it->second.currentState;
    }

    return AIState::IDLE;  // Default state if not found
}

float AIStateMachine::getStateTime(std::shared_ptr<NPC> npc) const {
    if (!npc) return 0.0f;

    auto it = npcStates.find(npc->npcId);
    if (it != npcStates.end()) {
        return it->second.stateTime;
    }

    return 0.0f;
}

void AIStateMachine::initializeDefaultTransitions() {
    LOGI("Initializing default state transitions");

    // ============================================================
    // IDLE State Transitions
    // ============================================================

    // IDLE -> WANDER (player far away)
    addTransition(AIState::IDLE, AIState::WANDER, AIEvent::PLAYER_APPROACHED);

    // IDLE -> COMBAT (enemy detected)
    addTransition(AIState::IDLE, AIState::COMBAT, AIEvent::ENEMY_DETECTED);

    // IDLE -> CONVERSATION (conversation start)
    addTransition(AIState::IDLE, AIState::CONVERSATION, AIEvent::CONVERSATION_START);

    // ============================================================
    // WANDER State Transitions
    // ============================================================

    // WANDER -> COMBAT (enemy detected)
    addTransition(AIState::WANDER, AIState::COMBAT, AIEvent::ENEMY_DETECTED);

    // WANDER -> FOLLOW_PLAYER (player approached)
    addTransition(AIState::WANDER, AIState::FOLLOW_PLAYER, AIEvent::PLAYER_APPROACHED);

    // WANDER -> IDLE (timeout - return to idle after 30 seconds)
    addTransition(AIState::WANDER, AIState::IDLE, AIEvent::TIMEOUT, nullptr, 30.0f);

    // ============================================================
    // PATROL State Transitions
    // ============================================================

    // PATROL -> COMBAT (enemy detected)
    addTransition(AIState::PATROL, AIState::COMBAT, AIEvent::ENEMY_DETECTED);

    // PATROL -> IDLE (interrupted)
    addTransition(AIState::PATROL, AIState::IDLE, AIEvent::PLAYER_APPROACHED);

    // ============================================================
    // FOLLOW_PLAYER State Transitions
    // ============================================================

    // FOLLOW_PLAYER -> COMBAT (player in combat)
    addTransition(AIState::FOLLOW_PLAYER, AIState::COMBAT, AIEvent::ENEMY_DETECTED);

    // FOLLOW_PLAYER -> IDLE (lost player after timeout)
    addTransition(AIState::FOLLOW_PLAYER, AIState::IDLE, AIEvent::TIMEOUT, nullptr, 20.0f);

    // ============================================================
    // COMBAT State Transitions
    // ============================================================

    // COMBAT -> IDLE (target defeated)
    addTransition(AIState::COMBAT, AIState::IDLE, AIEvent::TARGET_DEFEATED);

    // COMBAT -> FLEEING (health critical)
    addTransition(
        AIState::COMBAT,
        AIState::FLEEING,
        AIEvent::HEALTH_CRITICAL,
        [](std::shared_ptr<NPC> npc) { return npc && npc->shouldFlee(); }
    );

    // ============================================================
    // FLEEING State Transitions
    // ============================================================

    // FLEEING -> IDLE (timeout or safely away)
    addTransition(AIState::FLEEING, AIState::IDLE, AIEvent::TIMEOUT, nullptr, 15.0f);

    // FLEEING -> COMBAT (health recovered and player engaged)
    addTransition(
        AIState::FLEEING,
        AIState::COMBAT,
        AIEvent::PLAYER_APPROACHED,
        [](std::shared_ptr<NPC> npc) {
            return npc && npc->status.currentHealth > npc->status.maxHealth * 0.5f;
        }
    );

    // ============================================================
    // CONVERSATION State Transitions
    // ============================================================

    // CONVERSATION -> IDLE (conversation end)
    addTransition(AIState::CONVERSATION, AIState::IDLE, AIEvent::CONVERSATION_END);

    // CONVERSATION -> COMBAT (interrupted by combat)
    addTransition(AIState::CONVERSATION, AIState::COMBAT, AIEvent::ENEMY_DETECTED);

    // CONVERSATION -> IDLE (timeout)
    addTransition(AIState::CONVERSATION, AIState::IDLE, AIEvent::TIMEOUT, nullptr, 60.0f);

    LOGI("Default state transitions initialized");
}
