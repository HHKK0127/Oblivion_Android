#include "player_controller.h"
#include <glm/glm.hpp>
#include <cmath>

constexpr float WALK_SPEED = 5.0f;
constexpr float SPRINT_SPEED = 8.0f;
constexpr float ROTATION_SENSITIVITY = 0.01f;
constexpr float MOUSE_DRAG_SCALE = 0.5f;

PlayerController::PlayerController()
    : player(nullptr), worldManager(nullptr) {
    LOGD("PlayerController created");
}

PlayerController::~PlayerController() {
    cleanup();
}

bool PlayerController::initialize(WorldManager* worldMgr) {
    if (!worldMgr) {
        LOGE("Cannot initialize PlayerController with null WorldManager");
        return false;
    }

    worldManager = worldMgr;

    // Create player
    player = std::make_shared<Player>();
    player->initialize(glm::vec3(0.0f, 0.0f, 0.0f));

    LOGI("PlayerController initialized");
    return true;
}

void PlayerController::update(float deltaTime) {
    if (!player || !worldManager) return;

    // 1. Calculate movement from input
    glm::vec3 moveVec = calculateMovementVector();

    // 2. Apply sprint modifier
    float currentSpeed = isSprinting ? SPRINT_SPEED : WALK_SPEED;
    moveVec = moveVec * currentSpeed;

    // 3. Apply movement input to player
    player->applyMovementInput(moveVec, isSprinting);

    // 4. Apply gravity
    applyGravity(deltaTime);

    // 5. Update position with velocity
    updatePlayerPosition(deltaTime);

    // 6. Check ground collision
    checkGroundCollision();

    // 7. Check cell transitions
    checkCellTransition();

    // 8. Update player internal state
    player->update(deltaTime);
}

void PlayerController::cleanup() {
    if (player) {
        player = nullptr;
    }
    worldManager = nullptr;
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
}

void PlayerController::setSprinting(bool sprint) {
    isSprinting = sprint;
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

    // Normalize if moving
    float len = std::sqrt(movement.x * movement.x + movement.z * movement.z);
    if (len > 0.0f) {
        movement.x /= len;
        movement.z /= len;
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

void PlayerController::updatePlayerRotation(float deltaTime) {
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
