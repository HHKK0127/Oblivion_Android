#include "player_controller.h"
#include <glm/glm.hpp>
#include <cmath>
#include "../physics/physics_manager.h"

constexpr float WALK_SPEED = 5.0f;
constexpr float SPRINT_SPEED = 8.0f;
constexpr float ROTATION_SENSITIVITY = 0.01f;

PlayerController::PlayerController()
    : player(nullptr), worldManager(nullptr), inventoryManager(nullptr) {
    LOGD("PlayerController created");
}

PlayerController::~PlayerController() {
    cleanup();
}

bool PlayerController::initialize(WorldManager* worldMgr, InventoryManager* invMgr) {
    if (!worldMgr) {
        LOGE("Cannot initialize PlayerController with null WorldManager");
        return false;
    }

    worldManager = worldMgr;
    inventoryManager = invMgr;

    // Create player
    player = std::make_shared<Player>();
    player->initialize(glm::vec3(0.0f, 0.0f, 0.0f));

    LOGI("PlayerController initialized (InventoryManager: %s)",
         inventoryManager ? "available" : "not available");
    return true;
}

// ============================================================================
// Phase 31: Fixed/Variable timestep separation
// ============================================================================

void PlayerController::fixedUpdate(float fixedDt) {
    if (!player) return;

    // Physics & movement at fixed rate (deterministic)
    glm::vec3 moveVec = calculateMovementVector();
    bool canActuallySprint = isSprinting && player->canSprint &&
                             (moveVec.x != 0.0f || moveVec.z != 0.0f);
    float speed = canActuallySprint ? SPRINT_SPEED : WALK_SPEED;
    moveVec = moveVec * speed;

    player->applyMovementInput(moveVec, canActuallySprint);
    player->updateStamina(fixedDt, canActuallySprint);
    applyGravity(fixedDt);
    updatePlayerPosition(fixedDt);
    checkGroundCollision();
}

void PlayerController::update(float deltaTime) {
    if (!player || !worldManager) return;

    // Phase 36: Use Jolt Physics if available
    if (physicsCharacter) {
        updatePhysics(deltaTime);
    } else {
        // Phase 31: Fixed timestep accumulator for physics
        fixedAccumulator += deltaTime;
        while (fixedAccumulator >= FIXED_DT) {
            fixedUpdate(FIXED_DT);
            fixedAccumulator -= FIXED_DT;
        }
    }

    // Variable-rate updates (animation, rendering)
    updateAnimState(deltaTime);
    applyAnimToSkeleton(deltaTime);

    // Cell transition check
    checkCellTransition();

    // Update player internal state
    player->update(deltaTime);
}

void PlayerController::cleanup() {
    if (player) {
        player = nullptr;
    }
    worldManager = nullptr;
    inventoryManager = nullptr;
    skeleton = nullptr;
    animator = nullptr;
    charController = nullptr;
    LOGD("PlayerController cleaned up");
}

void PlayerController::onTouchInput(float deltaX, float deltaY) {
    if (!player) return;

    // Convert touch drag to rotation
    // deltaX → yaw (rotation around Y axis)
    // deltaY → pitch (rotation around X axis)
    player->rotation.x += deltaY * ROTATION_SENSITIVITY;  // Pitch
    player->rotation.y += deltaX * ROTATION_SENSITIVITY;  // Yaw

    // Clamp pitch to prevent over-rotation
    if (player->rotation.x > 89.0f) player->rotation.x = 89.0f;
    if (player->rotation.x < -89.0f) player->rotation.x = -89.0f;
}

void PlayerController::onKeyboardInput(int key, bool isPressed) {
    if (key < 0 || key >= 256) return;

    keysPressed[key] = isPressed;

    if (key == 32) {  // Space = Jump
        if (isPressed && player->isOnGround) {
            player->applyJump();
            LOGD("Jump triggered");
        }
    }
    else if (key == 'E' || key == 'e') {  // E = Pickup Item
        if (isPressed && worldManager && inventoryManager) {
            // Find nearest pickupable item
            auto nearbyItem = worldManager->getNearbyPickupItem(player->position, 3.0f);
            if (nearbyItem) {
                // Add to inventory
                // TODO: Create InventoryItem from WorldItem data
                // For now, just mark as picked up
                worldManager->pickupWorldItem(nearbyItem->worldItemId);
                LOGI("Picked up item: %s", nearbyItem->itemName.c_str());
            }
        }
    }
}

void PlayerController::setSprinting(bool sprint) {
    // Only allow sprinting if player has stamina
    if (sprint && player && player->canSprint) {
        isSprinting = true;
        LOGD("Sprint activated (stamina: %.1f)", player->stamina);
    } else if (sprint && player && !player->canSprint) {
        isSprinting = false;
        LOGD("Cannot sprint - stamina depleted");
    } else {
        isSprinting = false;
    }
}

void PlayerController::setJoystickInput(float x, float y) {
    joystickInput.x = x;
    joystickInput.y = y;
}

glm::vec3 PlayerController::calculateMovementVector() {
    glm::vec3 movement(0.0f, 0.0f, 0.0f);

    // W/Up
    if (keysPressed['W'] || keysPressed['w']) movement.z -= 1.0f;
    // S/Down
    if (keysPressed['S'] || keysPressed['s']) movement.z += 1.0f;
    // A/Left
    if (keysPressed['A'] || keysPressed['a']) movement.x -= 1.0f;
    // D/Right
    if (keysPressed['D'] || keysPressed['d']) movement.x += 1.0f;

    // Add joystick input (x is right, y is down/up)
    // Assuming joystick positive Y is down (backward) and positive X is right
    movement.x += joystickInput.x;
    movement.z += joystickInput.y;

    // Normalize if moving
    float len = std::sqrt(movement.x * movement.x + movement.z * movement.z);
    if (len > 0.0f) {
        // Clamp magnitude to 1.0 (so joystick halfway doesn't get normalized to 1.0 if not necessary, 
        // but if > 1.0 from keyboard + joystick, we normalize)
        if (len > 1.0f) {
            movement.x /= len;
            movement.z /= len;
        }
    }

    return movement;
}

void PlayerController::updatePlayerPosition(float deltaTime) {
    if (!player) return;

    // Update position with velocity
    player->position.x += player->velocity.x * deltaTime;
    player->position.y += player->velocity.y * deltaTime;
    player->position.z += player->velocity.z * deltaTime;
}

void PlayerController::updatePlayerRotation(float /* deltaTime */) {
    // Rotation is handled directly via onTouchInput
}

void PlayerController::applyGravity(float deltaTime) {
    if (!player) return;

    if (!player->isOnGround) {
        player->velocity.y += player->gravityAccel * deltaTime;
    } else {
        // Keep on ground, zero vertical velocity
        player->velocity.y = 0.0f;
    }
}

void PlayerController::checkGroundCollision() {
    if (!player || !worldManager) return;

    // Get terrain height at player position
    uint32_t cellX = static_cast<uint32_t>(player->position.x / 128.0f);
    uint32_t cellY = static_cast<uint32_t>(player->position.z / 128.0f);

    auto cell = worldManager->getCell(cellX, cellY);
    if (!cell) {
        player->isOnGround = false;
        return;
    }

    float terrainHeight = cell->getTerrainHeightAt(
        player->position.x - cellX * 128.0f,
        player->position.z - cellY * 128.0f
    );

    // Check if player is at or below terrain
    if (player->position.y <= terrainHeight) {
        player->position.y = terrainHeight;
        player->isOnGround = true;
    } else {
        player->isOnGround = false;
    }
}

void PlayerController::checkCellTransition() {
    if (!player || !worldManager) return;

    int32_t newCellX = static_cast<int32_t>(player->position.x / 128.0f);
    int32_t newCellY = static_cast<int32_t>(player->position.z / 128.0f);

    if (newCellX != player->currentCellX || newCellY != player->currentCellY) {
        LOGD("Player transitioning cell: (%d,%d) -> (%d,%d)",
             player->currentCellX, player->currentCellY, newCellX, newCellY);

        player->currentCellX = newCellX;
        player->currentCellY = newCellY;

        // WorldManager will handle cell loading
        worldManager->setPlayerPosition(player->position);
    }
}

// ============================================================================
// Phase 31: Animation State Machine with Hysteresis
// ============================================================================
//
// Hysteresis prevents state chattering when velocity oscillates around thresholds.
// Example: without hysteresis, a player at 0.12 m/s could trigger
// IDLE → WALK → IDLE rapidly. With WALK_ENTER=0.15 and WALK_EXIT=0.05,
// the state only enters WALK when speed > 0.15 and only returns to IDLE
// when speed drops below 0.05.

void PlayerController::updateAnimState(float deltaTime) {
    // Calculate current speed from velocity
    glm::vec3 v = player->velocity;
    currentSpeed = std::sqrt(v.x * v.x + v.z * v.z);  // ignore Y (vertical)

    bool isGrounded = charController ? charController->isGrounded() : true;

    // Decrement attack timer
    if (attackTimer > 0.0f) {
        attackTimer -= deltaTime;
        if (attackTimer <= 0.0f) {
            attackTimer = 0.0f;
        }
    }

    // State transitions with hysteresis
    PlayerAnimState newState = animState;

    // Attack has highest priority (returns to IDLE when timer expires)
    if (animState == PlayerAnimState::ATTACK && attackTimer <= 0.0f) {
        newState = PlayerAnimState::IDLE;
    } else if (animState == PlayerAnimState::ATTACK) {
        // Stay in ATTACK state until timer expires
        return;
    }

    // Jump state
    if (!isGrounded && animState != PlayerAnimState::JUMP) {
        newState = PlayerAnimState::JUMP;
    } else if (isGrounded && animState == PlayerAnimState::JUMP) {
        // Just landed → go to IDLE
        newState = PlayerAnimState::IDLE;
    } else if (isGrounded) {
        // Grounded movement states (IDLE / WALK / RUN with hysteresis)
        switch (animState) {
            case PlayerAnimState::IDLE:
                if (currentSpeed > WALK_ENTER_THRESHOLD) {
                    newState = PlayerAnimState::WALK;
                }
                break;

            case PlayerAnimState::WALK:
                if (currentSpeed < WALK_EXIT_THRESHOLD) {
                    newState = PlayerAnimState::IDLE;
                } else if (currentSpeed > RUN_ENTER_THRESHOLD) {
                    newState = PlayerAnimState::RUN;
                }
                break;

            case PlayerAnimState::RUN:
                if (currentSpeed < RUN_EXIT_THRESHOLD) {
                    newState = PlayerAnimState::WALK;
                }
                break;

            default:
                break;
        }
    }

    // Apply state transition to animation player
    if (newState != animState && animator) {
        float crossfade = CROSSFADE_IDLE_WALK;
        uint32_t targetSeq = 0;

        switch (newState) {
            case PlayerAnimState::IDLE:
                targetSeq = ANIM_IDLE;
                crossfade = CROSSFADE_IDLE_WALK;
                break;
            case PlayerAnimState::WALK:
                targetSeq = ANIM_WALK;
                crossfade = CROSSFADE_IDLE_WALK;
                break;
            case PlayerAnimState::RUN:
                targetSeq = ANIM_RUN;
                crossfade = CROSSFADE_WALK_RUN;
                break;
            case PlayerAnimState::JUMP:
                targetSeq = ANIM_JUMP;
                crossfade = CROSSFADE_TO_JUMP;
                break;
            case PlayerAnimState::ATTACK:
                targetSeq = ANIM_ATTACK;
                crossfade = CROSSFADE_TO_ATTACK;
                break;
        }

        // Stop current and play new sequence
        uint32_t prevSeq = static_cast<uint32_t>(animState);
        animator->stop(prevSeq);
        animator->play(targetSeq, newState != PlayerAnimState::ATTACK &&
                                   newState != PlayerAnimState::JUMP, 1.0f);
        LOGD("Anim state: %s -> %s (speed=%.2f)",
             getAnimStateName(), getAnimStateName(), currentSpeed);
    }

    animState = newState;
}

const char* PlayerController::getAnimStateName() const {
    switch (animState) {
        case PlayerAnimState::IDLE: return "IDLE";
        case PlayerAnimState::WALK: return "WALK";
        case PlayerAnimState::RUN: return "RUN";
        case PlayerAnimState::JUMP: return "JUMP";
        case PlayerAnimState::ATTACK: return "ATTACK";
    }
    return "UNKNOWN";
}

void PlayerController::toggleCombatStance() {
    combatStance = !combatStance;
    LOGD("Combat stance: %s", combatStance ? "ON" : "OFF");
}

void PlayerController::attack() {
    if (animState == PlayerAnimState::ATTACK) return;  // Already attacking
    animState = PlayerAnimState::ATTACK;
    attackTimer = ATTACK_DURATION;
    if (animator) {
        animator->stop(static_cast<uint32_t>(PlayerAnimState::IDLE));
        animator->stop(static_cast<uint32_t>(PlayerAnimState::WALK));
        animator->stop(static_cast<uint32_t>(PlayerAnimState::RUN));
        animator->play(ANIM_ATTACK, false, 1.0f);  // Non-looping
    }
    LOGD("Attack triggered");
}

// ============================================================================
// Imperial Weave EventBus integration
// ============================================================================
void PlayerController::subscribeToCombatEvents() {
    if (!eventBus) return;

    // Subscribe to combat events that affect player animation
    eventBus->subscribe("COMBAT_ATTACK_HIT", [this](const weave::Event& e) {
        // Player hit an enemy - could trigger hit reaction animation
        LOGD("Player hit enemy: %s", e.payload.c_str());
    });

    eventBus->subscribe("COMBAT_CRITICAL_HIT", [this](const weave::Event& e) {
        // Player landed a critical hit
        LOGD("Player critical hit: %s", e.payload.c_str());
    });

    eventBus->subscribe("COMBAT_BLOCK", [this](const weave::Event& e) {
        // Player blocked an attack
        LOGD("Player blocked: %s", e.payload.c_str());
        // TODO: Trigger block animation
    });

    eventBus->subscribe("COMBAT_PARRY", [this](const weave::Event& e) {
        // Player parried an attack
        LOGD("Player parried: %s", e.payload.c_str());
        // TODO: Trigger parry animation
    });

    eventBus->subscribe("COMBAT_DODGE", [this](const weave::Event& e) {
        // Player dodged an attack
        LOGD("Player dodged: %s", e.payload.c_str());
        // TODO: Trigger dodge animation
    });

    eventBus->subscribe("COMBAT_DEATH", [this](const weave::Event& e) {
        // Player died
        LOGD("Player died: %s", e.payload.c_str());
        // TODO: Trigger death animation
    });

    LOGI("PlayerController subscribed to combat events");
}

// ============================================================================
// Phase 31: Apply animation to skeleton
// ============================================================================

void PlayerController::applyAnimToSkeleton(float deltaTime) {
    if (!animator || !skeleton) return;

    // Update animation playback
    animator->update(deltaTime);

    // Apply animation transforms to skeleton bones
    // The AnimationPlayer samples tracks and sets bone local transforms
    // We then update the skeleton to compute world transforms and skinning matrices
    skeleton->update();
}

const std::vector<glm::mat4>& PlayerController::getSkinningMatrices() const {
    if (skeleton) {
        return skeleton->getSkinningMatrices();
    }
    static const std::vector<glm::mat4> empty;
    return empty;
}

// ============================================================================
// Phase 36: Jolt Physics Integration
// ============================================================================

void PlayerController::initPhysics(JPH::CharacterVirtual* character) {
    physicsCharacter = character;
    if (physicsCharacter) {
        auto& physics = oblivion::PhysicsManager::getInstance();
        player->position = physics.getCharacterPosition(physicsCharacter);
        LOGI("Physics character initialized at (%.2f, %.2f, %.2f)",
             player->position.x, player->position.y, player->position.z);
    }
}

void PlayerController::updatePhysics(float deltaTime) {
    if (!physicsCharacter || !player) return;

    auto& physics = oblivion::PhysicsManager::getInstance();

    // 入力を移動ベクトルに変換
    glm::vec3 input(0.0f, 0.0f, 0.0f);
    glm::vec3 moveVec = calculateMovementVector();
    if (moveVec.x != 0.0f || moveVec.z != 0.0f) {
        input.x = moveVec.x;
        input.z = moveVec.z;
    }

    // ジャンプ
    if (jumpRequested && physics.isCharacterGrounded(physicsCharacter)) {
        JPH::Vec3 vel = physicsCharacter->GetLinearVelocity();
        vel.SetY(jumpForce);
        physicsCharacter->SetLinearVelocity(vel);
        jumpRequested = false;
        LOGD("Jump executed");
    }

    // CharacterVirtual 更新
    physics.updateCharacter(physicsCharacter, deltaTime, input);

    // 位置同期
    player->position = physics.getCharacterPosition(physicsCharacter);
    player->isOnGround = physics.isCharacterGrounded(physicsCharacter);
}

void PlayerController::setPosition(const glm::vec3& pos) {
    if (player) {
        player->position = pos;
    }
}
