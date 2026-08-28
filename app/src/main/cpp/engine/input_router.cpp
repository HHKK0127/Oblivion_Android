#include "input_router.h"
#include <cmath>

// ============================================================================
// InputRouter implementation
// ============================================================================

InputRouter::InputRouter() = default;

InputRouter::~InputRouter() = default;

bool InputRouter::initialize(StateManager* stateManager) {
    stateManager_ = stateManager;
    pendingInputs_.clear();
    activePointers_.clear();
    joystickInput_ = glm::vec2(0.0f, 0.0f);
    LOGI("InputRouter initialized");
    return true;
}

void InputRouter::routeInput(int pointerId, float x, float y, int action) {
    // Convert raw action to InputAction
    InputAction inputAction;
    switch (action) {
        case 0: inputAction = InputAction::DOWN; break;    // ACTION_DOWN
        case 1: inputAction = InputAction::UP; break;      // ACTION_UP
        case 2: inputAction = InputAction::MOVE; break;    // ACTION_MOVE
        case 3: inputAction = InputAction::CANCEL; break;  // ACTION_CANCEL
        default:
            LOGD("Unknown touch action: %d", action);
            return;
    }

    // Get current game state for zone classification
    GamePlayState gameState = stateManager_ ? stateManager_->getCurrentState()
                                        : GamePlayState::GAMEPLAY;

    // Classify the input zone
    InputZone zone = classifyZone(x, y, gameState);

    // Create classified input
    ClassifiedInput classified;
    classified.pointerId = pointerId;
    classified.x = x;
    classified.y = y;
    classified.action = inputAction;
    classified.zone = zone;
    classified.timestamp = 0.0f;

    // Queue for processing
    pendingInputs_.push_back(classified);
}

void InputRouter::registerHandler(InputZone zone, InputHandler handler) {
    int idx = static_cast<int>(zone);
    if (idx >= 0 && idx < 7) {
        handlers_[idx].push_back(std::move(handler));
        LOGD("Handler registered for zone %d", idx);
    }
}

void InputRouter::setScreenSize(float width, float height) {
    screenWidth_ = width;
    screenHeight_ = height;
    LOGI("InputRouter screen size: %.0f x %.0f", width, height);
}

void InputRouter::setJoystickZone(float centerX, float centerY, float radius) {
    joystickCenterX_ = centerX;
    joystickCenterY_ = centerY;
    joystickRadius_ = radius;
    LOGI("Joystick zone: center=(%.0f, %.0f) radius=%.0f", centerX, centerY, radius);
}

void InputRouter::update(float deltaTime) {
    (void)deltaTime;

    // Process all pending inputs
    std::vector<ClassifiedInput> inputs;
    inputs.swap(pendingInputs_);

    for (const auto& input : inputs) {
        processInput(input);
    }
}

InputZone InputRouter::classifyZone(float x, float y, GamePlayState state) const {
    // In dialogue state, all input goes to dialogue
    if (state == GamePlayState::DIALOGUE) {
        return InputZone::DIALOGUE;
    }

    // In inventory state, all input goes to inventory
    if (state == GamePlayState::INVENTORY) {
        return InputZone::INVENTORY;
    }

    // In paused/menu states, everything is UI
    if (state == GamePlayState::PAUSED ||
        state == GamePlayState::MAIN_MENU ||
        state == GamePlayState::TITLE_SCREEN ||
        state == GamePlayState::SAVE_MENU ||
        state == GamePlayState::CHARACTER_CREATION) {
        return InputZone::UI_ELEMENT;
    }

    // Gameplay state: classify by screen region
    // Left 30% of screen = joystick zone
    float joystickThreshold = screenWidth_ * 0.3f;
    if (x < joystickThreshold) {
        // Check if within joystick circle
        float dx = x - joystickCenterX_;
        float dy = y - joystickCenterY_;
        float dist = std::sqrt(dx * dx + dy * dy);
        if (dist <= joystickRadius_ * 1.5f) {
            return InputZone::JOYSTICK;
        }
    }

    // Right 25% of screen, bottom 40% = action buttons
    float actionThresholdX = screenWidth_ * 0.75f;
    float actionThresholdY = screenHeight_ * 0.6f;
    if (x > actionThresholdX && y > actionThresholdY) {
        return InputZone::ACTION_BUTTON;
    }

    // Right side, upper area = camera drag
    if (x > screenWidth_ * 0.3f && y < screenHeight_ * 0.5f) {
        return InputZone::CAMERA;
    }

    // Default to UI element
    return InputZone::UI_ELEMENT;
}

void InputRouter::processInput(const ClassifiedInput& input) {
    // Update joystick if in joystick zone
    if (input.zone == InputZone::JOYSTICK) {
        updateJoystick(input);
    }

    // Dispatch to registered handlers for this zone
    int zoneIdx = static_cast<int>(input.zone);
    if (zoneIdx >= 0 && zoneIdx < 7) {
        for (const auto& handler : handlers_[zoneIdx]) {
            if (handler) {
                if (handler(input)) {
                    // Handler consumed the input
                    return;
                }
            }
        }
    }

    LOGD("Unhandled input: zone=%d action=%d pos=(%.0f, %.0f)",
         zoneIdx, static_cast<int>(input.action), input.x, input.y);
}

void InputRouter::updateJoystick(const ClassifiedInput& input) {
    switch (input.action) {
        case InputAction::DOWN:
        case InputAction::MOVE: {
            float dx = input.x - joystickCenterX_;
            float dy = input.y - joystickCenterY_;
            float dist = std::sqrt(dx * dx + dy * dy);

            if (dist > 0.001f) {
                // Normalize to -1..1 range, clamped to radius
                float normalizedDist = std::min(dist / joystickRadius_, 1.0f);
                joystickInput_ = glm::vec2(
                    (dx / dist) * normalizedDist,
                    (dy / dist) * normalizedDist
                );
            }
            break;
        }
        case InputAction::UP:
        case InputAction::CANCEL: {
            // Only reset if this was the joystick pointer
            joystickInput_ = glm::vec2(0.0f, 0.0f);
            break;
        }
    }
}

int InputRouter::findActivePointer(int pointerId) const {
    for (size_t i = 0; i < activePointers_.size(); ++i) {
        if (activePointers_[i].pointerId == pointerId) {
            return static_cast<int>(i);
        }
    }
    return -1;
}
