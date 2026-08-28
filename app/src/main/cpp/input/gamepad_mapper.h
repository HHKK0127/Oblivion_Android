#pragma once

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <cstdint>
#include <android/log.h>

#define GAMEPAD_LOG_TAG "GamepadMapper"
#define GPAD_LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, GAMEPAD_LOG_TAG, __VA_ARGS__)
#define GPAD_LOGI(...) __android_log_print(ANDROID_LOG_INFO, GAMEPAD_LOG_TAG, __VA_ARGS__)
#define GPAD_LOGW(...) __android_log_print(ANDROID_LOG_WARN, GAMEPAD_LOG_TAG, __VA_ARGS__)

// Android gamepad button constants (matching AKEYCODE values)
enum class GamepadButton : int32_t {
    BUTTON_A        = 96,   // AKEYCODE_BUTTON_A
    BUTTON_B        = 97,   // AKEYCODE_BUTTON_B
    BUTTON_X        = 99,   // AKEYCODE_BUTTON_X
    BUTTON_Y        = 100,  // AKEYCODE_BUTTON_Y
    BUMPER_LEFT     = 102,  // AKEYCODE_BUTTON_L1
    BUMPER_RIGHT    = 103,  // AKEYCODE_BUTTON_R1
    TRIGGER_LEFT    = 104,  // AKEYCODE_BUTTON_L2
    TRIGGER_RIGHT   = 105,  // AKEYCODE_BUTTON_R2
    THUMB_LEFT      = 106,  // AKEYCODE_BUTTON_THUMBL
    THUMB_RIGHT     = 107,  // AKEYCODE_BUTTON_THUMBR
    DPAD_UP         = 19,   // AKEYCODE_DPAD_UP
    DPAD_DOWN       = 20,   // AKEYCODE_DPAD_DOWN
    DPAD_LEFT       = 21,   // AKEYCODE_DPAD_LEFT
    DPAD_RIGHT      = 22,   // AKEYCODE_DPAD_RIGHT
    START           = 108,  // AKEYCODE_BUTTON_START
    SELECT          = 109,  // AKEYCODE_BUTTON_SELECT
    HOME            = 110,  // AKEYCODE_BUTTON_MODE
    UNKNOWN         = -1
};

// Android gamepad axis constants
enum class GamepadAxis : int32_t {
    LEFT_X          = 0,    // AMOTION_EVENT_AXIS_X
    LEFT_Y          = 1,    // AMOTION_EVENT_AXIS_Y
    RIGHT_X         = 11,   // AMOTION_EVENT_AXIS_Z
    RIGHT_Y         = 14,   // AMOTION_EVENT_AXIS_RZ
    TRIGGER_LEFT    = 17,   // AMOTION_EVENT_AXIS_LTRIGGER
    TRIGGER_RIGHT   = 18,   // AMOTION_EVENT_AXIS_RTRIGGER
    HAT_X           = 15,   // AMOTION_EVENT_AXIS_HAT_X
    HAT_Y           = 16,   // AMOTION_EVENT_AXIS_HAT_Y
    UNKNOWN         = -1
};

// Gamepad connection state
enum class GamepadState {
    DISCONNECTED,
    CONNECTED,
    ACTIVE
};

// Axis configuration for dead zone and curve
struct AxisConfig {
    float deadZone;       // 0.0 - 0.3
    float maxValue;       // Expected maximum axis value (usually 1.0)
    float sensitivity;    // Multiplier (0.5 - 2.0)
    bool invert;          // Invert axis direction

    AxisConfig()
        : deadZone(0.12f), maxValue(1.0f), sensitivity(1.0f), invert(false) {}
};

// Custom action binding (maps a game action to a gamepad button)
struct GamepadBinding {
    std::string actionName;
    GamepadButton button;
    bool isAxis;          // true if this binding uses an axis instead of a button
    GamepadAxis axis;
    float axisThreshold;  // Threshold for axis-to-button conversion

    GamepadBinding()
        : actionName(""), button(GamepadButton::UNKNOWN),
          isAxis(false), axis(GamepadAxis::UNKNOWN), axisThreshold(0.5f) {}
};

// Per-gamepad device info
struct GamepadDevice {
    int deviceId;
    std::string deviceName;
    GamepadState state;
    int playerIndex;      // 0-3 for local multiplayer

    // Button states
    std::map<int32_t, bool> buttonStates;

    // Axis configurations
    std::map<GamepadAxis, AxisConfig> axisConfigs;

    // Current axis values (after processing)
    float leftStickX, leftStickY;
    float rightStickX, rightStickY;
    float leftTrigger, rightTrigger;
    float hatX, hatY;

    GamepadDevice()
        : deviceId(-1), deviceName("Unknown"), state(GamepadState::DISCONNECTED),
          playerIndex(-1),
          leftStickX(0.0f), leftStickY(0.0f),
          rightStickX(0.0f), rightStickY(0.0f),
          leftTrigger(0.0f), rightTrigger(0.0f),
          hatX(0.0f), hatY(0.0f) {}
};

// Callback types
using GamepadButtonCallback = std::function<void(int deviceId, GamepadButton button, bool pressed)>;
using GamepadAxisCallback = std::function<void(int deviceId, GamepadAxis axis, float value)>;
using GamepadConnectionCallback = std::function<void(int deviceId, bool connected)>;

// Gamepad mapper system
// Handles Android gamepad API integration, button mapping, axis configuration,
// multiple gamepad support, custom bindings, and hot-plug detection
class GamepadMapper {
public:
    GamepadMapper();
    ~GamepadMapper() = default;

    // Initialize the mapper
    void initialize();

    // Process Android KeyEvent for gamepad buttons
    // Returns true if the event was handled
    bool onKeyEvent(int deviceId, int keyCode, bool pressed);

    // Process Android MotionEvent for gamepad axes
    // Returns true if the event was handled
    bool onMotionEvent(int deviceId, int axisId, float value);

    // Update all connected gamepads (call each frame)
    void update(float deltaTime);

    // Device management
    void onDeviceConnected(int deviceId, const std::string& deviceName);
    void onDeviceDisconnected(int deviceId);
    int getConnectedDeviceCount() const;
    const GamepadDevice* getDevice(int deviceId) const;
    std::vector<int> getConnectedDeviceIds() const;

    // Custom binding management
    void addBinding(int deviceId, const GamepadBinding& binding);
    void removeBinding(int deviceId, const std::string& actionName);
    bool isActionPressed(int deviceId, const std::string& actionName) const;

    // Axis configuration
    void setAxisConfig(int deviceId, GamepadAxis axis, const AxisConfig& config);
    AxisConfig getAxisConfig(int deviceId, GamepadAxis axis) const;

    // Callbacks
    void setButtonCallback(GamepadButtonCallback callback) { buttonCallback = callback; }
    void setAxisCallback(GamepadAxisCallback callback) { axisCallback = callback; }
    void setConnectionCallback(GamepadConnectionCallback callback) { connectionCallback = callback; }

    // Query stick/trigger state
    float getLeftStickX(int deviceId) const;
    float getLeftStickY(int deviceId) const;
    float getRightStickX(int deviceId) const;
    float getRightStickY(int deviceId) const;
    float getLeftTrigger(int deviceId) const;
    float getRightTrigger(int deviceId) const;

private:
    static constexpr int MAX_GAMEPADS = 4;
    std::map<int, GamepadDevice> devices;
    std::map<int, std::vector<GamepadBinding>> deviceBindings;

    GamepadButtonCallback buttonCallback;
    GamepadAxisCallback axisCallback;
    GamepadConnectionCallback connectionCallback;

    // Apply dead zone and curve to axis value
    float processAxisValue(float rawValue, const AxisConfig& config) const;

    // Find or create device entry
    GamepadDevice* getOrCreateDevice(int deviceId);

    // Map Android keyCode to GamepadButton
    GamepadButton mapKeyCode(int keyCode) const;

    // Map Android axis ID to GamepadAxis
    GamepadAxis mapAxisId(int axisId) const;

    // Check custom bindings for axis-to-button conversion
    void checkAxisBindings(int deviceId, GamepadAxis axis, float value);
};
