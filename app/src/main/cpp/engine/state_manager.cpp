#include "state_manager.h"

// ============================================================================
// GameState string conversion
// ============================================================================

const char* gamePlayStateToString(GamePlayState state) {
    switch (state) {
        case GamePlayState::TITLE_SCREEN:       return "TITLE_SCREEN";
        case GamePlayState::MAIN_MENU:          return "MAIN_MENU";
        case GamePlayState::CHARACTER_CREATION: return "CHARACTER_CREATION";
        case GamePlayState::GAMEPLAY:           return "GAMEPLAY";
        case GamePlayState::PAUSED:             return "PAUSED";
        case GamePlayState::INVENTORY:          return "INVENTORY";
        case GamePlayState::DIALOGUE:           return "DIALOGUE";
        case GamePlayState::LOADING:            return "LOADING";
        case GamePlayState::SAVE_MENU:          return "SAVE_MENU";
        default:                                return "UNKNOWN";
    }
}

// ============================================================================
// StateManager implementation
// ============================================================================

StateManager::StateManager() = default;

StateManager::~StateManager() = default;

bool StateManager::initialize() {
    currentState_ = GamePlayState::TITLE_SCREEN;
    previousState_ = GamePlayState::TITLE_SCREEN;
    stateDuration_ = 0.0f;
    history_.clear();
    LOGI("StateManager initialized in TITLE_SCREEN state");
    return true;
}

bool StateManager::transitionTo(GamePlayState newState) {
    if (newState == currentState_) {
        LOGD("Already in state %s, ignoring transition", gamePlayStateToString(newState));
        return true;
    }

    if (!canTransitionTo(newState)) {
        LOGW("Invalid transition: %s -> %s",
             gamePlayStateToString(currentState_), gamePlayStateToString(newState));
        return false;
    }

    // Notify exit callbacks
    notifyExit(currentState_);

    // Perform transition
    previousState_ = currentState_;
    currentState_ = newState;
    stateDuration_ = 0.0f;

    // Record history
    StateTransition transition;
    transition.from = previousState_;
    transition.to = newState;
    transition.timestamp = 0.0f; // Relative time not tracked globally here
    history_.push_back(transition);
    if (history_.size() > MAX_HISTORY) {
        history_.erase(history_.begin());
    }

    LOGI("State transition: %s -> %s",
         gamePlayStateToString(previousState_), gamePlayStateToString(currentState_));

    // Notify enter callbacks
    notifyEnter(currentState_);
    notifyTransition(previousState_, currentState_);

    return true;
}

bool StateManager::canTransitionTo(GamePlayState targetState) const {
    return isValidTransition(currentState_, targetState);
}

void StateManager::onEnterState(GamePlayState state, TransitionCallback callback) {
    enterCallbacks_[static_cast<uint8_t>(state)].push_back(std::move(callback));
}

void StateManager::onExitState(GamePlayState state, TransitionCallback callback) {
    exitCallbacks_[static_cast<uint8_t>(state)].push_back(std::move(callback));
}

void StateManager::onAnyTransition(TransitionCallback callback) {
    anyTransitionCallbacks_.push_back(std::move(callback));
}

void StateManager::update(float deltaTime) {
    stateDuration_ += deltaTime;
}

void StateManager::clearHistory() {
    history_.clear();
}

bool StateManager::isUIActive() const {
    switch (currentState_) {
        case GamePlayState::PAUSED:
        case GamePlayState::INVENTORY:
        case GamePlayState::SAVE_MENU:
        case GamePlayState::MAIN_MENU:
        case GamePlayState::CHARACTER_CREATION:
            return true;
        default:
            return false;
    }
}

bool StateManager::acceptsGameInput() const {
    switch (currentState_) {
        case GamePlayState::GAMEPLAY:
        case GamePlayState::DIALOGUE:
            return true;
        default:
            return false;
    }
}

bool StateManager::isValidTransition(GamePlayState from, GamePlayState to) const {
    // Define valid state transitions
    switch (from) {
        case GamePlayState::TITLE_SCREEN:
            return to == GamePlayState::MAIN_MENU || to == GamePlayState::LOADING;

        case GamePlayState::MAIN_MENU:
            return to == GamePlayState::CHARACTER_CREATION ||
                   to == GamePlayState::LOADING ||
                   to == GamePlayState::TITLE_SCREEN;

        case GamePlayState::CHARACTER_CREATION:
            return to == GamePlayState::LOADING || to == GamePlayState::MAIN_MENU;

        case GamePlayState::GAMEPLAY:
            return to == GamePlayState::PAUSED ||
                   to == GamePlayState::INVENTORY ||
                   to == GamePlayState::DIALOGUE ||
                   to == GamePlayState::SAVE_MENU ||
                   to == GamePlayState::LOADING;

        case GamePlayState::PAUSED:
            return to == GamePlayState::GAMEPLAY ||
                   to == GamePlayState::SAVE_MENU ||
                   to == GamePlayState::MAIN_MENU;

        case GamePlayState::INVENTORY:
            return to == GamePlayState::GAMEPLAY;

        case GamePlayState::DIALOGUE:
            return to == GamePlayState::GAMEPLAY;

        case GamePlayState::LOADING:
            return to == GamePlayState::GAMEPLAY ||
                   to == GamePlayState::MAIN_MENU ||
                   to == GamePlayState::CHARACTER_CREATION;

        case GamePlayState::SAVE_MENU:
            return to == GamePlayState::PAUSED ||
                   to == GamePlayState::GAMEPLAY;

        default:
            return false;
    }
}

void StateManager::notifyEnter(GamePlayState state) {
    auto it = enterCallbacks_.find(static_cast<uint8_t>(state));
    if (it != enterCallbacks_.end()) {
        for (const auto& cb : it->second) {
            if (cb) cb(previousState_, currentState_);
        }
    }
}

void StateManager::notifyExit(GamePlayState state) {
    auto it = exitCallbacks_.find(static_cast<uint8_t>(state));
    if (it != exitCallbacks_.end()) {
        for (const auto& cb : it->second) {
            if (cb) cb(previousState_, currentState_);
        }
    }
}

void StateManager::notifyTransition(GamePlayState from, GamePlayState to) {
    for (const auto& cb : anyTransitionCallbacks_) {
        if (cb) cb(from, to);
    }
}
