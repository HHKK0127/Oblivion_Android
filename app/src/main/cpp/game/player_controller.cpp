#include "player_controller.h"
#include <glm/glm.hpp>
#include <cmath>

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

void PlayerController::update(float deltaTime) {
    if (!player || !worldManager) return;

    // 1. Calculate movement from input
    glm::vec3 moveVec = calculateMovementVector();

    // 2. Check if sprinting is allowed (must have stamina and be moving)
    bool canActuallySprint = isSprinting && player->canSprint && (moveVec.x != 0.0f || moveVec.z != 0.0f);

    // 3. Apply sprint modifier
    float currentSpeed = canActuallySprint ? SPRINT_SPEED : WALK_SPEED;
    moveVec = moveVec * currentSpeed;

    // 4. Apply movement input to player
    player->applyMovementInput(moveVec, canActuallySprint);

    // 5. Update stamina
    player->updateStamina(deltaTime, canActuallySprint);

    // 6. Apply gravity
    applyGravity(deltaTime);

    // 7. Update position with velocity
    updatePlayerPosition(deltaTime);

    // 8. Check ground collision
    checkGroundCollision();

    // 9. Check cell transitions
    checkCellTransition();

    // 10. Update player internal state
    player->update(deltaTime);
}

void PlayerController::cleanup() {
    if (player) {
        player = nullptr;
    }
    worldManager = nullptr;
    inventoryManager = nullptr;
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
