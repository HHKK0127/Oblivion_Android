#pragma once

#include <string>
#include <functional>
#include <vector>
#include <glm/glm.hpp>
#include <android/log.h>

#define MENU_TRANS_LOG_TAG "MenuTransitionManager"
#define MTRANS_LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, MENU_TRANS_LOG_TAG, __VA_ARGS__)
#define MTRANS_LOGI(...) __android_log_print(ANDROID_LOG_INFO, MENU_TRANS_LOG_TAG, __VA_ARGS__)
#define MTRANS_LOGW(...) __android_log_print(ANDROID_LOG_WARN, MENU_TRANS_LOG_TAG, __VA_ARGS__)

class TextRenderer;

/**
 * @brief Menu transition type
 */
enum class TransitionType {
    FADE_IN,
    FADE_OUT,
    SLIDE_LEFT,
    SLIDE_RIGHT,
    SLIDE_UP,
    SLIDE_DOWN,
    NONE
};

/**
 * @brief Transition state
 */
enum class TransitionState {
    IDLE,
    OUTGOING,   // Outgoing transition
    INCOMING    // Incoming transition
};

/**
 * @brief Menu transition animation management
 *
 * Phase 43: Provides smooth transition animations between menus.
 * - Fade in/out
 * - Slide transition (left/right/up/down)
 * - Input blocking during transition
 * - Easing function support
 */
class MenuTransitionManager {
public:
    MenuTransitionManager();
    ~MenuTransitionManager() = default;

    /**
     * @brief Initialize
     * @param textRenderer Text renderer
     * @param screenWidth Screen width
     * @param screenHeight Screen height
     */
    void initialize(TextRenderer* textRenderer, int screenWidth, int screenHeight);

    /**
     * @brief Update every frame
     * @param deltaTime Elapsed time from previous frame (seconds)
     */
    void update(float deltaTime);

    /**
     * @brief Render transition overlay
     */
    void render();

    /**
     * @brief Start transition
     * @param type Transition type
     * @param duration Transition duration (seconds)
     * @param onOutgoingComplete Source exit complete callback
     * @param onIncomingComplete Destination enter complete callback
     */
    void startTransition(TransitionType type, float duration,
                         std::function<void()> onOutgoingComplete = nullptr,
                         std::function<void()> onIncomingComplete = nullptr);

    /**
     * @brief Is transitioning
     */
    bool isTransitioning() const { return state != TransitionState::IDLE; }

    /**
     * @brief Is input blocked
     */
    bool isInputBlocked() const { return state != TransitionState::IDLE; }

    /**
     * @brief Get current transition alpha (0.0 to 1.0)
     */
    float getCurrentAlpha() const { return currentAlpha; }

    /**
     * @brief Screen size change
     */
    void setScreenSize(int width, int height);

    /**
     * @brief Set transition speed (default: 0.3 seconds)
     */
    void setDefaultDuration(float seconds) { defaultDuration = seconds; }

    /**
     * @brief Set overlay color
     */
    void setOverlayColor(const glm::vec4& color) { overlayColor = color; }

private:
    TextRenderer* textRenderer;
    int screenWidth;
    int screenHeight;

    // Transition state
    TransitionState state;
    TransitionType currentType;
    float transitionDuration;
    float elapsedTime;
    float currentAlpha;

    // Default settings
    float defaultDuration;
    glm::vec4 overlayColor;

    // Slide offset
    glm::vec2 slideOffset;

    // Callbacks
    std::function<void()> onOutgoingComplete;
    std::function<void()> onIncomingComplete;
    bool outgoingCallbackFired;

    // Easing function
    float easeInOut(float t) const;
    float easeOutCubic(float t) const;

    // Update by transition type
    void updateFade(float progress);
    void updateSlide(float progress);
};
