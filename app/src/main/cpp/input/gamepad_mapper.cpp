#include "gamepad_mapper.h"
#include <algorithm>
#include <cmath>

GamepadMapper::GamepadMapper() {
}

void GamepadMapper::initialize() {
    GPAD_LOGI("GamepadMapper initialized (max %d gamepads)", MAX_GAMEPADS);
}

GamepadButton GamepadMapper::mapKeyCode(int keyCode) const {
    // Direct mapping for standard Android gamepad keycodes
    switch (keyCode) {
        case 96:  return GamepadButton::BUTTON_A;
        case 97:  return GamepadButton::BUTTON_B;
        case 99:  return GamepadButton::BUTTON_X;
        case 100: return GamepadButton::BUTTON_Y;
        case 102: return GamepadButton::BUMPER_LEFT;
        case 103: return GamepadButton::BUMPER_RIGHT;
        case 104: return GamepadButton::TRIGGER_LEFT;
        case 105: return GamepadButton::TRIGGER_RIGHT;
        case 106: return GamepadButton::THUMB_LEFT;
        case 107: return GamepadButton::THUMB_RIGHT;
        case 19:  return GamepadButton::DPAD_UP;
        case 20:  return GamepadButton::DPAD_DOWN;
        case 21:  return GamepadButton::DPAD_LEFT;
        case 22:  return GamepadButton::DPAD_RIGHT;
        case 108: return GamepadButton::START;
        case 109: return GamepadButton::SELECT;
        case 110: return GamepadButton::HOME;
        default:  return GamepadButton::UNKNOWN;
    }
}

GamepadAxis GamepadMapper::mapAxisId(int axisId) const {
    switch (axisId) {
        case 0:  return GamepadAxis::LEFT_X;
        case 1:  return GamepadAxis::LEFT_Y;
        case 11: return GamepadAxis::RIGHT_X;
        case 14: return GamepadAxis::RIGHT_Y;
        case 17: return GamepadAxis::TRIGGER_LEFT;
        case 18: return GamepadAxis::TRIGGER_RIGHT;
        case 15: return GamepadAxis::HAT_X;
        case 16: return GamepadAxis::HAT_Y;
        default: return GamepadAxis::UNKNOWN;
    }
}

GamepadDevice* GamepadMapper::getOrCreateDevice(int deviceId) {
    auto it = devices.find(deviceId);
    if (it != devices.end()) {
        return &it->second;
    }
    return nullptr;
}

bool GamepadMapper::onKeyEvent(int deviceId, int keyCode, bool pressed) {
    GamepadButton button = mapKeyCode(keyCode);
    if (button == GamepadButton::UNKNOWN) {
        return false;
    }

    GamepadDevice* device = getOrCreateDevice(deviceId);
    if (!device || device->state == GamepadState::DISCONNECTED) {
        return false;
    }

    // Update button state
    device->buttonStates[keyCode] = pressed;
    device->state = GamepadState::ACTIVE;

    GPAD_LOGD("Gamepad %d: button %d %s", deviceId, keyCode, pressed ? "pressed" : "released");

    // Fire callback
    if (buttonCallback) {
        buttonCallback(deviceId, button, pressed);
    }

    // Check custom bindings
    auto bindIt = deviceBindings.find(deviceId);
    if (bindIt != deviceBindings.end()) {
        for (const auto& binding : bindIt->second) {
            if (!binding.isAxis && binding.button == button) {
                // Button binding matched - the callback above already fired
            }
        }
    }

    return true;
}

bool GamepadMapper::onMotionEvent(int deviceId, int axisId, float value) {
    GamepadAxis axis = mapAxisId(axisId);
    if (axis == GamepadAxis::UNKNOWN) {
        return false;
    }

    GamepadDevice* device = getOrCreateDevice(deviceId);
    if (!device || device->state == GamepadState::DISCONNECTED) {
        return false;
    }

    // Get axis config
    AxisConfig config;
    auto cfgIt = device->axisConfigs.find(axis);
    if (cfgIt != device->axisConfigs.end()) {
        config = cfgIt->second;
    }

    // Process value through dead zone and curve
    float processed = processAxisValue(value, config);

    // Store in device state
    switch (axis) {
        case GamepadAxis::LEFT_X:
            device->leftStickX = processed;
            break;
        case GamepadAxis::LEFT_Y:
            device->leftStickY = processed;
            break;
        case GamepadAxis::RIGHT_X:
            device->rightStickX = processed;
            break;
        case GamepadAxis::RIGHT_Y:
            device->rightStickY = processed;
            break;
        case GamepadAxis::TRIGGER_LEFT:
            device->leftTrigger = processed;
            break;
        case GamepadAxis::TRIGGER_RIGHT:
            device->rightTrigger = processed;
            break;
        case GamepadAxis::HAT_X:
            device->hatX = processed;
            break;
        case GamepadAxis::HAT_Y:
            device->hatY = processed;
            break;
        default:
            break;
    }

    // Fire callback
    if (axisCallback) {
        axisCallback(deviceId, axis, processed);
    }

    // Check axis-to-button bindings
    checkAxisBindings(deviceId, axis, processed);

    return true;
}

float GamepadMapper::processAxisValue(float rawValue, const AxisConfig& config) const {
    float value = rawValue;

    // Invert if configured
    if (config.invert) {
        value = -value;
    }

    // Apply dead zone
    float absVal = std::abs(value);
    if (absVal < config.deadZone) {
        return 0.0f;
    }

    // Rescale from [deadZone, maxValue] to [0, 1]
    float rescaled = (absVal - config.deadZone) / (config.maxValue - config.deadZone);
    rescaled = std::min(rescaled, 1.0f);

    // Apply sensitivity
    rescaled *= config.sensitivity;
    rescaled = std::min(rescaled, 1.0f);

    return (value >= 0.0f) ? rescaled : -rescaled;
}

void GamepadMapper::checkAxisBindings(int deviceId, GamepadAxis axis, float value) {
    auto bindIt = deviceBindings.find(deviceId);
    if (bindIt == deviceBindings.end()) return;

    for (const auto& binding : bindIt->second) {
        if (binding.isAxis && binding.axis == axis) {
            bool pressed = std::abs(value) >= binding.axisThreshold;
            if (buttonCallback) {
                buttonCallback(deviceId, binding.button, pressed);
            }
        }
    }
}

void GamepadMapper::update(float /*deltaTime*/) {
    // Periodic maintenance - could add connection polling here
}

void GamepadMapper::onDeviceConnected(int deviceId, const std::string& deviceName) {
    GamepadDevice device;
    device.deviceId = deviceId;
    device.deviceName = deviceName;
    device.state = GamepadState::CONNECTED;
    device.playerIndex = static_cast<int>(devices.size());

    // Set default axis configs
    device.axisConfigs[GamepadAxis::LEFT_X] = AxisConfig();
    device.axisConfigs[GamepadAxis::LEFT_Y] = AxisConfig();
    device.axisConfigs[GamepadAxis::RIGHT_X] = AxisConfig();
    device.axisConfigs[GamepadAxis::RIGHT_Y] = AxisConfig();
    device.axisConfigs[GamepadAxis::TRIGGER_LEFT] = AxisConfig();
    device.axisConfigs[GamepadAxis::TRIGGER_RIGHT] = AxisConfig();

    devices[deviceId] = device;

    GPAD_LOGI("Gamepad connected: id=%d name=%s player=%d",
              deviceId, deviceName.c_str(), device.playerIndex);

    if (connectionCallback) {
        connectionCallback(deviceId, true);
    }
}

void GamepadMapper::onDeviceDisconnected(int deviceId) {
    auto it = devices.find(deviceId);
    if (it != devices.end()) {
        it->second.state = GamepadState::DISCONNECTED;
        GPAD_LOGI("Gamepad disconnected: id=%d", deviceId);

        if (connectionCallback) {
            connectionCallback(deviceId, false);
        }
    }
}

int GamepadMapper::getConnectedDeviceCount() const {
    int count = 0;
    for (const auto& pair : devices) {
        if (pair.second.state != GamepadState::DISCONNECTED) {
            count++;
        }
    }
    return count;
}

const GamepadDevice* GamepadMapper::getDevice(int deviceId) const {
    auto it = devices.find(deviceId);
    if (it != devices.end() && it->second.state != GamepadState::DISCONNECTED) {
        return &it->second;
    }
    return nullptr;
}

std::vector<int> GamepadMapper::getConnectedDeviceIds() const {
    std::vector<int> ids;
    for (const auto& pair : devices) {
        if (pair.second.state != GamepadState::DISCONNECTED) {
            ids.push_back(pair.first);
        }
    }
    return ids;
}

void GamepadMapper::addBinding(int deviceId, const GamepadBinding& binding) {
    deviceBindings[deviceId].push_back(binding);
    GPAD_LOGI("Binding added: device=%d action=%s", deviceId, binding.actionName.c_str());
}

void GamepadMapper::removeBinding(int deviceId, const std::string& actionName) {
    auto it = deviceBindings.find(deviceId);
    if (it != deviceBindings.end()) {
        auto& bindings = it->second;
        bindings.erase(
            std::remove_if(bindings.begin(), bindings.end(),
                [&actionName](const GamepadBinding& b) { return b.actionName == actionName; }),
            bindings.end());
    }
}

bool GamepadMapper::isActionPressed(int deviceId, const std::string& actionName) const {
    auto bindIt = deviceBindings.find(deviceId);
    if (bindIt == deviceBindings.end()) return false;

    auto devIt = devices.find(deviceId);
    if (devIt == devices.end()) return false;

    for (const auto& binding : bindIt->second) {
        if (binding.actionName == actionName) {
            if (binding.isAxis) {
                // Check axis value against threshold
                float axisValue = 0.0f;
                switch (binding.axis) {
                    case GamepadAxis::LEFT_X: axisValue = devIt->second.leftStickX; break;
                    case GamepadAxis::LEFT_Y: axisValue = devIt->second.leftStickY; break;
                    case GamepadAxis::RIGHT_X: axisValue = devIt->second.rightStickX; break;
                    case GamepadAxis::RIGHT_Y: axisValue = devIt->second.rightStickY; break;
                    case GamepadAxis::TRIGGER_LEFT: axisValue = devIt->second.leftTrigger; break;
                    case GamepadAxis::TRIGGER_RIGHT: axisValue = devIt->second.rightTrigger; break;
                    default: break;
                }
                return std::abs(axisValue) >= binding.axisThreshold;
            } else {
                // Check button state
                auto btnIt = devIt->second.buttonStates.find(static_cast<int32_t>(binding.button));
                if (btnIt != devIt->second.buttonStates.end()) {
                    return btnIt->second;
                }
            }
        }
    }
    return false;
}

void GamepadMapper::setAxisConfig(int deviceId, GamepadAxis axis, const AxisConfig& config) {
    GamepadDevice* device = getOrCreateDevice(deviceId);
    if (device) {
        device->axisConfigs[axis] = config;
    }
}

AxisConfig GamepadMapper::getAxisConfig(int deviceId, GamepadAxis axis) const {
    auto it = devices.find(deviceId);
    if (it != devices.end()) {
        auto cfgIt = it->second.axisConfigs.find(axis);
        if (cfgIt != it->second.axisConfigs.end()) {
            return cfgIt->second;
        }
    }
    return AxisConfig();
}

float GamepadMapper::getLeftStickX(int deviceId) const {
    auto it = devices.find(deviceId);
    return (it != devices.end()) ? it->second.leftStickX : 0.0f;
}

float GamepadMapper::getLeftStickY(int deviceId) const {
    auto it = devices.find(deviceId);
    return (it != devices.end()) ? it->second.leftStickY : 0.0f;
}

float GamepadMapper::getRightStickX(int deviceId) const {
    auto it = devices.find(deviceId);
    return (it != devices.end()) ? it->second.rightStickX : 0.0f;
}

float GamepadMapper::getRightStickY(int deviceId) const {
    auto it = devices.find(deviceId);
    return (it != devices.end()) ? it->second.rightStickY : 0.0f;
}

float GamepadMapper::getLeftTrigger(int deviceId) const {
    auto it = devices.find(deviceId);
    return (it != devices.end()) ? it->second.leftTrigger : 0.0f;
}

float GamepadMapper::getRightTrigger(int deviceId) const {
    auto it = devices.find(deviceId);
    return (it != devices.end()) ? it->second.rightTrigger : 0.0f;
}
