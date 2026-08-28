#pragma once

#include "state_manager.h"
#include <glm/glm.hpp>
#include <cstdint>
#include <vector>
#include <functional>
#include <android/log.h>

#define LOG_TAG "InputRouter"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
#define LOGD(...) do {} while(0)
#endif
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ============================================================================
// InputRouter - Routes touch input to appropriate systems
// Phase 42: Full game loop integration
// ============================================================================

// Input action types
enum class InputAction : uint8_t {
    DOWN,       // Touch down
    MOVE,       // Touch move
    UP,         // Touch up
    CANCEL      // Touch cancelled
};

// Input zone classification
enum class InputZone : uint8_t {
    JOYSTICK,       // Left side virtual joystick
    ACTION_BUTTON,  // Right side action buttons
    UI_ELEMENT,     // UI buttons, menus
    CAMERA,         // Camera drag area
    DIALOGUE,       // Dialogue choice area
    INVENTORY,      // Inventory grid area
    UNKNOWN
};

// Classified input event
struct ClassifiedInput {
    int pointerId;
    float x;
    float y;
    InputAction action;
    InputZone zone;
    float timestamp;
};

// Input handler callback
using InputHandler = std::function<bool(const ClassifiedInput&)>;

class InputRouter {
public:
    InputRouter();
    ~InputRouter();

    bool initialize(StateManager* stateManager);

    // Route a raw touch event
    void routeInput(int pointerId, float x, float y, int action);

    // Register input handlers for specific zones
    void registerHandler(InputZone zone, InputHandler handler);

    // Screen dimensions (for zone classification)
    void setScreenSize(float width, float height);

    // Joystick zone configuration
    void setJoystickZone(float centerX, float centerY, float radius);

    // Get joystick input (normalized -1..1)
    glm::vec2 getJoystickInput() const { return joystickInput_; }

    // Update (processes queued inputs)
    void update(float deltaTime);

    // State
    void setStateManager(StateManager* sm) { stateManager_ = sm; }

private:
    StateManager* stateManager_ = nullptr;

    // Screen dimensions
    float screenWidth_ = 1080.0f;
    float screenHeight_ = 1920.0f;

    // Joystick zone
    float joystickCenterX_ = 200.0f;
    float joystickCenterY_ = 1400.0f;
    float joystickRadius_ = 150.0f;

    // Current joystick state
    glm::vec2 joystickInput_ = glm::vec2(0.0f, 0.0f);

    // Input handlers per zone
    std::vector<InputHandler> handlers_[7]; // Indexed by InputZone

    // Pending input queue
    std::vector<ClassifiedInput> pendingInputs_;
    std::vector<ClassifiedInput> activePointers_;

    // Zone classification
    InputZone classifyZone(float x, float y, GamePlayState state) const;

    // Process a single classified input
    void processInput(const ClassifiedInput& input);

    // Update joystick from input
    void updateJoystick(const ClassifiedInput& input);

    // Find active pointer by ID
    int findActivePointer(int pointerId) const;
};
