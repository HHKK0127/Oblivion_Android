#pragma once

#include "player.h"
#include "../world/world_manager.h"
#include "inventory_manager.h"
#include <memory>
#include <android/log.h>

#define LOG_TAG "PlayerController"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
#define LOGD(...) do {} while(0)
#endif
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

class PlayerController {
public:
    PlayerController();
    ~PlayerController();

    // Lifecycle
    bool initialize(WorldManager* worldMgr, InventoryManager* invMgr = nullptr);
    void update(float deltaTime);
    void cleanup();

    // Input Handling
    void onTouchInput(float deltaX, float deltaY);  // Drag movement
    void setJoystickInput(float x, float y);        // Joystick movement (x, y normalized)
    void onKeyboardInput(int key, bool isPressed);  // WASD + Space
    void setSprinting(bool sprint);

    // Player Access
    std::shared_ptr<Player> getPlayer() { return player; }
    const glm::vec3& getPlayerPosition() const { return player->position; }
    const glm::vec3& getPlayerRotation() const { return player->rotation; }

    // Cell Transition Checks
    void checkCellTransition();

private:
    std::shared_ptr<Player> player;
    WorldManager* worldManager;
    InventoryManager* inventoryManager;

    // Input State
    bool keysPressed[256] = {};  // WASD, Space tracking
    glm::vec2 joystickInput = glm::vec2(0.0f, 0.0f); // Virtual joystick input
    bool isSprinting = false;

    // Movement Calculation
    glm::vec3 calculateMovementVector();
    void updatePlayerPosition(float deltaTime);
    void updatePlayerRotation(float deltaTime);

    // Physics & Collision
    void applyGravity(float deltaTime);
    void checkGroundCollision();
    void handleCellBoundaryCollision();
};
