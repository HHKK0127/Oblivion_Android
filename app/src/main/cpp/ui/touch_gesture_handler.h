#pragma once

#include <functional>
#include <vector>
#include <array>
#include <glm/glm.hpp>
#include <android/log.h>

#define GESTURE_LOG_TAG "TouchGestureHandler"
#define GESTURE_LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, GESTURE_LOG_TAG, __VA_ARGS__)
#define GESTURE_LOGI(...) __android_log_print(ANDROID_LOG_INFO, GESTURE_LOG_TAG, __VA_ARGS__)
#define GESTURE_LOGW(...) __android_log_print(ANDROID_LOG_WARN, GESTURE_LOG_TAG, __VA_ARGS__)

/**
 * @brief Gesture type
 */
enum class GestureType {
    NONE,
    SINGLE_TAP,
    DOUBLE_TAP,
    LONG_PRESS,
    SWIPE_LEFT,
    SWIPE_RIGHT,
    SWIPE_UP,
    SWIPE_DOWN,
    PINCH_IN,
    PINCH_OUT,
    TWO_FINGER_SWIPE
};

/**
 * @brief Gesture event structure
 */
struct GestureEvent {
    GestureType type;
    glm::vec2 position;       // Start position
    glm::vec2 delta;          // Movement amount (swipe direction)
    float magnitude;          // Magnitude (pinch ratio, swipe distance)
    int pointerCount;         // Touch count
    float duration;           // Duration (seconds)

    GestureEvent()
        : type(GestureType::NONE), position(0.0f, 0.0f), delta(0.0f, 0.0f),
          magnitude(0.0f), pointerCount(0), duration(0.0f) {}
};

/**
 * @brief Gesture callback type
 */
using GestureCallback = std::function<void(const GestureEvent&)>;

/**
 * @brief Touch gesture recognizer
 *
 * Phase 43: Recognizes multi-touch gestures and notifies via callbacks.
 * - Single tap: select/interact
 * - Double tap: run/dash
 * - Long press: context menu
 * - Swipe: camera rotation
 * - Pinch: zoom (map screen)
 * - Two-finger swipe: camera movement
 */
class TouchGestureHandler {
public:
    TouchGestureHandler();
    ~TouchGestureHandler() = default;

    /**
     * @brief Set gesture recognition parameters
     */
    void setTapTimeout(float seconds) { tapTimeout = seconds; }
    void setDoubleTapTimeout(float seconds) { doubleTapTimeout = seconds; }
    void setLongPressTimeout(float seconds) { longPressTimeout = seconds; }
    void setSwipeThreshold(float pixels) { swipeThreshold = pixels; }
    void setPinchThreshold(float ratio) { pinchThreshold = ratio; }

    /**
     * @brief Register gesture callback
     * @param type Gesture type to register
     * @param callback Callback function
     */
    void registerCallback(GestureType type, GestureCallback callback);

    /**
     * @brief Register common callback for all gestures
     */
    void registerAllCallback(GestureCallback callback);

    // === Touch Input (called from UISystem) ===

    void onTouchDown(float x, float y, int pointerId);
    void onTouchMove(float x, float y, int pointerId);
    void onTouchUp(float x, float y, int pointerId);

    /**
     * @brief Update every frame (gesture timing management)
     * @param deltaTime Elapsed time from previous frame (seconds)
     */
    void update(float deltaTime);

    /**
     * @brief Reset state
     */
    void reset();

    /**
     * @brief Enable/disable toggle
     */
    void setEnabled(bool enabled) { this->enabled = enabled; }
    bool isEnabled() const { return enabled; }

private:
    /**
     * @brief Touch point information
     */
    struct TouchPoint {
        glm::vec2 startPos;
        glm::vec2 currentPos;
        float startTime;
        bool active;

        TouchPoint() : startPos(0.0f, 0.0f), currentPos(0.0f, 0.0f), startTime(0.0f), active(false) {}
    };

    // Configuration parameters
    float tapTimeout;         // Single tap detection time (seconds)
    float doubleTapTimeout;   // Double tap interval (seconds)
    float longPressTimeout;   // Long press detection time (seconds)
    float swipeThreshold;     // Minimum swipe distance (pixels)
    float pinchThreshold;     // Pinch detection ratio change

    // State
    bool enabled;
    float currentTime;

    // Multi-touch management (max 10 fingers)
    static constexpr int MAX_POINTERS = 10;
    std::array<TouchPoint, MAX_POINTERS> pointers;
    int activePointerCount;

    // For double tap detection
    float lastTapTime;
    glm::vec2 lastTapPos;

    // For pinch detection
    float initialPinchDistance;
    float currentPinchDistance;

    // Callbacks
    std::vector<std::pair<GestureType, GestureCallback>> callbacks;
    GestureCallback allCallback;

    // Internal helper
    void detectTap(int pointerId);
    void detectLongPress(int pointerId);
    void detectSwipe(int pointerId);
    void detectPinch();
    void detectTwoFingerSwipe();

    float getDistance(const glm::vec2& a, const glm::vec2& b) const;
    void fireGesture(const GestureEvent& event);
    int findActivePointerCount() const;
};
