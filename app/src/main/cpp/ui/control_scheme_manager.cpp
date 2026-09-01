#include "control_scheme_manager.h"
#include <cmath>

ControlSchemeManager::ControlSchemeManager()
    : currentScheme(ControlSchemeType::TOUCH)
    , screenWidth(1080)
    , screenHeight(1920)
    , uiScale(1.0f) {
}

void ControlSchemeManager::initialize(int w, int h) {
    screenWidth = w;
    screenHeight = h;
    uiScale = static_cast<float>(screenWidth) / 1080.0f;
    uiScale = std::max(0.5f, std::min(uiScale, 2.0f));

    setupTouchScheme();
    setupVirtualJoystickScheme();
    setupGamepadScheme();

    CTRL_LOGI("ControlSchemeManager initialized: %dx%d scale=%.2f", w, h, uiScale);
}

void ControlSchemeManager::setScheme(ControlSchemeType type) {
    if (currentScheme == type) return;

    currentScheme = type;
    CTRL_LOGI("Scheme changed to: %s", getSchemeName(type).c_str());

    for (const auto& cb : changeCallbacks) {
        cb(type);
    }
}

std::string ControlSchemeManager::getSchemeName(ControlSchemeType type) const {
    switch (type) {
        case ControlSchemeType::TOUCH: return "Touch";
        case ControlSchemeType::VIRTUAL_JOYSTICK: return "Virtual Joystick";
        case ControlSchemeType::GAMEPAD: return "Gamepad";
        default: return "Unknown";
    }
}

void ControlSchemeManager::registerSchemeChangeCallback(SchemeChangeCallback callback) {
    changeCallbacks.push_back(std::move(callback));
}

const std::vector<ButtonBinding>& ControlSchemeManager::getButtonBindings() const {
    auto it = schemeBindings.find(currentScheme);
    if (it != schemeBindings.end()) {
        return it->second;
    }
    // Fallback
    static std::vector<ButtonBinding> empty;
    return empty;
}

void ControlSchemeManager::setButtonPosition(InputAction action, const glm::vec2& position) {
    auto it = schemeBindings.find(currentScheme);
    if (it == schemeBindings.end()) return;

    for (auto& binding : it->second) {
        if (binding.action == action) {
            binding.position = position;
            break;
        }
    }
}

void ControlSchemeManager::setButtonSize(InputAction action, const glm::vec2& size) {
    auto it = schemeBindings.find(currentScheme);
    if (it == schemeBindings.end()) return;

    for (auto& binding : it->second) {
        if (binding.action == action) {
            binding.size = size;
            break;
        }
    }
}

void ControlSchemeManager::setButtonVisible(InputAction action, bool visible) {
    auto it = schemeBindings.find(currentScheme);
    if (it == schemeBindings.end()) return;

    for (auto& binding : it->second) {
        if (binding.action == action) {
            binding.visible = visible;
            break;
        }
    }
}

void ControlSchemeManager::setButtonOpacity(InputAction action, float opacity) {
    auto it = schemeBindings.find(currentScheme);
    if (it == schemeBindings.end()) return;

    for (auto& binding : it->second) {
        if (binding.action == action) {
            binding.opacity = std::max(0.0f, std::min(opacity, 1.0f));
            break;
        }
    }
}

void ControlSchemeManager::resetBindings() {
    schemeBindings.clear();
    setupTouchScheme();
    setupVirtualJoystickScheme();
    setupGamepadScheme();
    CTRL_LOGI("Bindings reset to defaults");
}

InputAction ControlSchemeManager::hitTest(float x, float y) const {
    const auto& bindings = getButtonBindings();

    // Search from back (prioritize front buttons)
    for (int i = static_cast<int>(bindings.size()) - 1; i >= 0; --i) {
        const auto& b = bindings[i];
        if (!b.visible) continue;

        float bx = b.position.x;
        float by = b.position.y;
        float bw = b.size.x;
        float bh = b.size.y;

        if (x >= bx && x <= bx + bw && y >= by && y <= by + bh) {
            return b.action;
        }
    }

    return InputAction::COUNT;
}

void ControlSchemeManager::setScreenSize(int w, int h) {
    screenWidth = w;
    screenHeight = h;
    uiScale = static_cast<float>(screenWidth) / 1080.0f;
    uiScale = std::max(0.5f, std::min(uiScale, 2.0f));

    // Recalculate layout for all schemes
    schemeBindings.clear();
    setupTouchScheme();
    setupVirtualJoystickScheme();
    setupGamepadScheme();
}

void ControlSchemeManager::onGamepadButton(int buttonId, bool pressed) {
    if (currentScheme != ControlSchemeType::GAMEPAD) return;

    // Standard gamepad button mapping
    InputAction action = InputAction::COUNT;
    switch (buttonId) {
        case 0: action = InputAction::ATTACK; break;       // A
        case 1: action = InputAction::BLOCK; break;        // B
        case 2: action = InputAction::CAST_SPELL; break;   // X
        case 3: action = InputAction::INTERACT; break;     // Y
        case 4: action = InputAction::OPEN_INVENTORY; break; // LB
        case 5: action = InputAction::OPEN_MAP; break;     // RB
        case 6: action = InputAction::OPEN_QUEST_LOG; break;
        case 7: action = InputAction::OPEN_PAUSE_MENU; break;
        default: break;
    }

    if (action != InputAction::COUNT && pressed) {
        CTRL_LOGD("Gamepad button %d -> action %d", buttonId, static_cast<int>(action));
    }
}

void ControlSchemeManager::onGamepadAxis(int axisId, float value) {
    if (currentScheme != ControlSchemeType::GAMEPAD) return;

    // axis 0,1: left stick (movement)
    // axis 2,3: right stick (camera)
    if (axisId >= 0 && axisId <= 3) {
        CTRL_LOGD("Gamepad axis %d: %.2f", axisId, value);
    }
}

// === Default Layout Per Scheme ===

ButtonBinding ControlSchemeManager::createBinding(InputAction action, const glm::vec2& pos,
                                                    const glm::vec2& size, const std::string& label) {
    ButtonBinding binding;
    binding.action = action;
    binding.position = pos;
    binding.size = size;
    binding.label = label;
    binding.visible = true;
    binding.opacity = 0.7f;
    return binding;
}

void ControlSchemeManager::setupTouchScheme() {
    std::vector<ButtonBinding> bindings;
    float sw = static_cast<float>(screenWidth);
    float sh = static_cast<float>(screenHeight);
    float m = 20.0f * uiScale;
    float btnSize = 80.0f * uiScale;

    // Attack button (bottom right)
    bindings.push_back(createBinding(InputAction::ATTACK,
        glm::vec2(sw - btnSize * 2 - m, sh - btnSize * 3 - m),
        glm::vec2(btnSize, btnSize), "ATK"));

    // Block button (left of attack button)
    bindings.push_back(createBinding(InputAction::BLOCK,
        glm::vec2(sw - btnSize * 3 - m, sh - btnSize * 2 - m),
        glm::vec2(btnSize, btnSize), "BLK"));

    // Interact button (upper right)
    bindings.push_back(createBinding(InputAction::INTERACT,
        glm::vec2(sw - btnSize - m, sh - btnSize * 4 - m),
        glm::vec2(btnSize, btnSize), "ACT"));

    // Jump button
    bindings.push_back(createBinding(InputAction::JUMP,
        glm::vec2(sw - btnSize - m, sh - btnSize * 2 - m),
        glm::vec2(btnSize, btnSize), "JMP"));

    // Sprint button
    bindings.push_back(createBinding(InputAction::SPRINT,
        glm::vec2(sw - btnSize * 2 - m, sh - btnSize * 4 - m),
        glm::vec2(btnSize, btnSize), "RUN"));

    // Quick slot (bottom center)
    float qsSize = 60.0f * uiScale;
    float qsStartX = (sw - qsSize * 5) * 0.5f;
    for (int i = 0; i < 5; ++i) {
        InputAction action = static_cast<InputAction>(
            static_cast<int>(InputAction::QUICK_SLOT_1) + i);
        bindings.push_back(createBinding(action,
            glm::vec2(qsStartX + qsSize * i, sh - qsSize - m),
            glm::vec2(qsSize, qsSize), "QS" + std::to_string(i + 1)));
    }

    schemeBindings[ControlSchemeType::TOUCH] = std::move(bindings);
}

void ControlSchemeManager::setupVirtualJoystickScheme() {
    std::vector<ButtonBinding> bindings;
    float sw = static_cast<float>(screenWidth);
    float sh = static_cast<float>(screenHeight);
    float m = 20.0f * uiScale;
    float btnSize = 80.0f * uiScale;

    // Joystick placed on left side (handled by UIJoystick component)
    // Action buttons on right side
    bindings.push_back(createBinding(InputAction::ATTACK,
        glm::vec2(sw - btnSize - m, sh - btnSize * 2 - m),
        glm::vec2(btnSize, btnSize), "ATK"));

    bindings.push_back(createBinding(InputAction::BLOCK,
        glm::vec2(sw - btnSize * 2 - m, sh - btnSize - m),
        glm::vec2(btnSize, btnSize), "BLK"));

    bindings.push_back(createBinding(InputAction::CAST_SPELL,
        glm::vec2(sw - btnSize - m, sh - btnSize * 3 - m),
        glm::vec2(btnSize, btnSize), "MAG"));

    bindings.push_back(createBinding(InputAction::JUMP,
        glm::vec2(sw - btnSize * 2 - m, sh - btnSize * 2 - m),
        glm::vec2(btnSize, btnSize), "JMP"));

    bindings.push_back(createBinding(InputAction::INTERACT,
        glm::vec2(sw - btnSize * 3 - m, sh - btnSize - m),
        glm::vec2(btnSize, btnSize), "ACT"));

    // Quick slot
    float qsSize = 60.0f * uiScale;
    float qsStartX = (sw - qsSize * 5) * 0.5f;
    for (int i = 0; i < 5; ++i) {
        InputAction action = static_cast<InputAction>(
            static_cast<int>(InputAction::QUICK_SLOT_1) + i);
        bindings.push_back(createBinding(action,
            glm::vec2(qsStartX + qsSize * i, sh - qsSize - m),
            glm::vec2(qsSize, qsSize), "QS" + std::to_string(i + 1)));
    }

    schemeBindings[ControlSchemeType::VIRTUAL_JOYSTICK] = std::move(bindings);
}

void ControlSchemeManager::setupGamepadScheme() {
    // Minimal on-screen buttons for gamepad scheme
    std::vector<ButtonBinding> bindings;

    // Show quick slots only
    float sw = static_cast<float>(screenWidth);
    float sh = static_cast<float>(screenHeight);
    float m = 20.0f * uiScale;
    float qsSize = 60.0f * uiScale;
    float qsStartX = (sw - qsSize * 5) * 0.5f;

    for (int i = 0; i < 5; ++i) {
        InputAction action = static_cast<InputAction>(
            static_cast<int>(InputAction::QUICK_SLOT_1) + i);
        bindings.push_back(createBinding(action,
            glm::vec2(qsStartX + qsSize * i, sh - qsSize - m),
            glm::vec2(qsSize, qsSize), "QS" + std::to_string(i + 1)));
    }

    schemeBindings[ControlSchemeType::GAMEPAD] = std::move(bindings);
}
