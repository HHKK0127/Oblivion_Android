#pragma once

#include "player.h"
#include "../world/world_manager.h"
#include "inventory_manager.h"
#include "../collision/character_controller.h"
#include "../animation/skeleton.h"
#include "../animation/animation_player.h"
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

// Phase 31: Animation state with hysteresis to prevent chattering
enum class PlayerAnimState : uint8_t {
    IDLE,
    WALK,
    RUN,
    JUMP,
    ATTACK
};

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

    // Phase 31: Skeleton & Animation integration
    void setSkeleton(Skeleton* skel) { skeleton = skel; }
    void setAnimator(animation::AnimationPlayer* anim) { animator = anim; }
    void setCharacterController(CharacterController* ctrl) { charController = ctrl; }

    // Phase 31: Get skinning matrices for rendering
    const std::vector<glm::mat4>& getSkinningMatrices() const;

    // Phase 31: Animation state query
    PlayerAnimState getAnimState() const { return animState; }
    const char* getAnimStateName() const;

    // Phase 31: Combat stance
    void toggleCombatStance();
    bool isInCombatStance() const { return combatStance; }
    void attack();

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

    // Phase 31: Integrated components (non-owning pointers)
    Skeleton* skeleton = nullptr;
    animation::AnimationPlayer* animator = nullptr;
    CharacterController* charController = nullptr;

    // Phase 31: Animation state machine with hysteresis
    PlayerAnimState animState = PlayerAnimState::IDLE;
    float currentSpeed = 0.0f;

    // Hysteresis thresholds (feedback: prevent chattering)
    static constexpr float WALK_ENTER_THRESHOLD = 0.15f;   // IDLE → WALK
    static constexpr float WALK_EXIT_THRESHOLD = 0.05f;    // WALK → IDLE
    static constexpr float RUN_ENTER_THRESHOLD = 5.0f;     // WALK → RUN
    static constexpr float RUN_EXIT_THRESHOLD = 4.0f;      // RUN → WALK (20% hysteresis)

    // Animation sequence indices (mapped from NIF sequences)
    static constexpr uint32_t ANIM_IDLE = 0;
    static constexpr uint32_t ANIM_WALK = 1;
    static constexpr uint32_t ANIM_RUN = 2;
    static constexpr uint32_t ANIM_JUMP = 3;
    static constexpr uint32_t ANIM_ATTACK = 4;

    // Attack state
    bool combatStance = false;
    float attackTimer = 0.0f;
    static constexpr float ATTACK_DURATION = 0.8f;

    // Crossfade durations
    static constexpr float CROSSFADE_WALK_RUN = 0.2f;
    static constexpr float CROSSFADE_IDLE_WALK = 0.3f;
    static constexpr float CROSSFADE_TO_JUMP = 0.1f;
    static constexpr float CROSSFADE_TO_ATTACK = 0.15f;

    // Phase 31: Update animation state with hysteresis
    void updateAnimState(float deltaTime);

    // Phase 31: Apply animation transforms to skeleton
    void applyAnimToSkeleton(float deltaTime);

    // Phase 31: Fixed timestep accumulator
    float fixedAccumulator = 0.0f;
    static constexpr float FIXED_DT = 1.0f / 60.0f;  // 60Hz physics
    void fixedUpdate(float fixedDt);
};
