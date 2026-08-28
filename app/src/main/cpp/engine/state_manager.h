#pragma once

#include <cstdint>
#include <string>
#include <functional>
#include <unordered_map>
#include <vector>
#include <android/log.h>

#define LOG_TAG "StateManager"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
#define LOGD(...) do {} while(0)
#endif
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ============================================================================
// StateManager - Game state finite state machine
// Phase 42: Full game loop integration
// ============================================================================

enum class GamePlayState : uint8_t {
    TITLE_SCREEN,       // Title screen
    MAIN_MENU,          // Main menu
    CHARACTER_CREATION, // Character creation
    GAMEPLAY,           // Active gameplay
    PAUSED,             // Pause menu
    INVENTORY,          // Inventory screen
    DIALOGUE,           // NPC dialogue
    LOADING,            // Loading screen
    SAVE_MENU           // Save/load menu
};

// Convert GamePlayState to string for logging
const char* gamePlayStateToString(GamePlayState state);

// State transition event
struct StateTransition {
    GamePlayState from;
    GamePlayState to;
    float timestamp;
};

class StateManager {
public:
    using TransitionCallback = std::function<void(GamePlayState from, GamePlayState to)>;

    StateManager();
    ~StateManager();

    // Initialize with default state
    bool initialize();

    // State queries
    GamePlayState getCurrentState() const { return currentState_; }
    GamePlayState getPreviousState() const { return previousState_; }
    const char* getCurrentStateName() const { return gamePlayStateToString(currentState_); }

    // State transitions
    bool transitionTo(GamePlayState newState);
    bool canTransitionTo(GamePlayState targetState) const;

    // Transition callbacks
    void onEnterState(GamePlayState state, TransitionCallback callback);
    void onExitState(GamePlayState state, TransitionCallback callback);
    void onAnyTransition(TransitionCallback callback);

    // State duration
    float getStateDuration() const { return stateDuration_; }

    // Update (for state-specific timers)
    void update(float deltaTime);

    // History
    const std::vector<StateTransition>& getHistory() const { return history_; }
    void clearHistory();

    // Convenience state checks
    bool isGameplayActive() const { return currentState_ == GamePlayState::GAMEPLAY; }
    bool isPaused() const { return currentState_ == GamePlayState::PAUSED; }
    bool isUIActive() const;
    bool acceptsGameInput() const;

private:
    GamePlayState currentState_ = GamePlayState::TITLE_SCREEN;
    GamePlayState previousState_ = GamePlayState::TITLE_SCREEN;
    float stateDuration_ = 0.0f;

    // Callbacks
    std::unordered_map<uint8_t, std::vector<TransitionCallback>> enterCallbacks_;
    std::unordered_map<uint8_t, std::vector<TransitionCallback>> exitCallbacks_;
    std::vector<TransitionCallback> anyTransitionCallbacks_;

    // Transition history
    std::vector<StateTransition> history_;
    static constexpr size_t MAX_HISTORY = 64;

    // Valid transition table
    bool isValidTransition(GamePlayState from, GamePlayState to) const;

    void notifyEnter(GamePlayState state);
    void notifyExit(GamePlayState state);
    void notifyTransition(GamePlayState from, GamePlayState to);
};
