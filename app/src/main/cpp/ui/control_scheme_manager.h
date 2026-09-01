#pragma once

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <glm/glm.hpp>
#include <android/log.h>

#define CTRL_LOG_TAG "ControlSchemeManager"
#define CTRL_LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, CTRL_LOG_TAG, __VA_ARGS__)
#define CTRL_LOGI(...) __android_log_print(ANDROID_LOG_INFO, CTRL_LOG_TAG, __VA_ARGS__)
#define CTRL_LOGW(...) __android_log_print(ANDROID_LOG_WARN, CTRL_LOG_TAG, __VA_ARGS__)

/**
 * @brief Control scheme type
 */
enum class ControlSchemeType {
    TOUCH,           // Touch input (default)
    VIRTUAL_JOYSTICK, // Virtual joystick
    GAMEPAD          // Gamepad support
};

/**
 * @brief Input action
 */
enum class InputAction {
    MOVE_FORWARD,
    MOVE_BACKWARD,
    MOVE_LEFT,
    MOVE_RIGHT,
    ATTACK,
    BLOCK,
    JUMP,
    INTERACT,
    CAST_SPELL,
    OPEN_INVENTORY,
    OPEN_MAP,
    OPEN_QUEST_LOG,
    OPEN_PAUSE_MENU,
    QUICK_SLOT_1,
    QUICK_SLOT_2,
    QUICK_SLOT_3,
    QUICK_SLOT_4,
    QUICK_SLOT_5,
    CAMERA_LOOK,
    SPRINT,
    COUNT
};

/**
 * @brief Button layout information
 */
struct ButtonBinding {
    InputAction action;
    glm::vec2 position;   // Pixel coordinates
    glm::vec2 size;       // Pixel size
    std::string label;
    bool visible;
    float opacity;

    ButtonBinding()
        : action(InputAction::MOVE_FORWARD), position(0.0f), size(80.0f),
          visible(true), opacity(0.7f) {}
};

/**
 * @brief Control scheme switch callbacks
 */
using SchemeChangeCallback = std::function<void(ControlSchemeType)>;

/**
 * @brief Control scheme management
 *
 * Phase 43: Manages multiple control schemes and provides scheme switching and
 * customizable button layouts.
 */
class ControlSchemeManager {
public:
    ControlSchemeManager();
    ~ControlSchemeManager() = default;

    /**
     * @brief Initialize
     * @param screenWidth Screen width
     * @param screenHeight Screen height
     */
    void initialize(int screenWidth, int screenHeight);

    /**
     * @brief Switch control scheme
     */
    void setScheme(ControlSchemeType type);

    /**
     * @brief Get current scheme
     */
    ControlSchemeType getCurrentScheme() const { return currentScheme; }

    /**
     * @brief Get scheme name
     */
    std::string getSchemeName(ControlSchemeType type) const;

    /**
     * @brief Register scheme change callback
     */
    void registerSchemeChangeCallback(SchemeChangeCallback callback);

    // === Button Layout ===

    /**
     * @brief Get current scheme button layout
     */
    const std::vector<ButtonBinding>& getButtonBindings() const;

    /**
     * @brief Customize button layout
     */
    void setButtonPosition(InputAction action, const glm::vec2& position);

    /**
     * @brief Change button size
     */
    void setButtonSize(InputAction action, const glm::vec2& size);

    /**
     * @brief Set button visibility
     */
    void setButtonVisible(InputAction action, bool visible);

    /**
     * @brief Set button transparency
     */
    void setButtonOpacity(InputAction action, float opacity);

    /**
     * @brief Reset button layout to default
     */
    void resetBindings();

    // === Input Processing ===

    /**
     * @brief Detect action from touch position
     * @return Matching action (COUNT if none)
     */
    InputAction hitTest(float x, float y) const;

    /**
     * @brief Screen size change
     */
    void setScreenSize(int width, int height);

    /**
     * @brief Process gamepad input
     * @param buttonId Button ID
     * @param pressed Pressed state
     */
    void onGamepadButton(int buttonId, bool pressed);

    /**
     * @brief Process gamepad axis input
     * @param axisId Axis ID
     * @param value Axis value (-1.0 to 1.0)
     */
    void onGamepadAxis(int axisId, float value);

private:
    ControlSchemeType currentScheme;
    int screenWidth;
    int screenHeight;
    float uiScale;

    // Button layout per scheme
    std::map<ControlSchemeType, std::vector<ButtonBinding>> schemeBindings;

    // Callbacks
    std::vector<SchemeChangeCallback> changeCallbacks;

    // Generate default layout
    void setupTouchScheme();
    void setupVirtualJoystickScheme();
    void setupGamepadScheme();

    // Button layout helper
    ButtonBinding createBinding(InputAction action, const glm::vec2& pos,
                                const glm::vec2& size, const std::string& label);
};
