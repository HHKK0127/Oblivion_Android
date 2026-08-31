#include "debug_system.h"
#include "debug_hud.h"
#include "debug_menu.h"
#include "game_console.h"
#include <cmath>
#include <android/log.h>

#define LOG_TAG "DebugSystem"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

DebugSystem& DebugSystem::getInstance() {
    static DebugSystem instance;
    return instance;
}

DebugSystem::DebugSystem()
    : debugHUD(nullptr), debugMenu(nullptr), gameConsole(nullptr),
      threeFingerDown(false), threeFingerDownTime(0),
      threeFingerStartX(0), threeFingerStartY(0),
      tapCount(0), lastTapTime(0), lastTapX(0), lastTapY(0),
      twoFingerDown(false) {
}

DebugSystem::~DebugSystem() {
    cleanup();
}

bool DebugSystem::initialize(DebugHUD* hud, DebugMenu* menu, GameConsole* console) {
    debugHUD = hud;
    debugMenu = menu;
    gameConsole = console;
    LOGI("DebugSystem initialized (HUD=%p, Menu=%p, Console=%p)",
         (void*)hud, (void*)menu, (void*)console);
    return true;
}

void DebugSystem::cleanup() {
    debugHUD = nullptr;
    debugMenu = nullptr;
    gameConsole = nullptr;
    LOGI("DebugSystem cleaned up");
}

void DebugSystem::onTouch(int action, int pointerCount, float x, float y, int64_t eventTime) {
    // ACTION_DOWN=0, ACTION_UP=1, ACTION_MOVE=2,
    // ACTION_POINTER_DOWN=5, ACTION_POINTER_UP=6

    // --- 3-finger tap detection ---
    if (pointerCount >= MIN_POINTER_COUNT_FOR_3FINGER) {
        if (action == 5 || action == 0) {
            // 3rd finger placed down
            threeFingerDown = true;
            threeFingerDownTime = eventTime;
            threeFingerStartX = x;
            threeFingerStartY = y;
            LOGD("3-finger down detected at (%.0f, %.0f)", x, y);
        }
    }

    if (threeFingerDown && (action == 1 || action == 6)) {
        // Finger lifted - check if it was a valid tap
        int64_t duration = eventTime - threeFingerDownTime;
        float dx = x - threeFingerStartX;
        float dy = y - threeFingerStartY;
        float dist = std::sqrt(dx * dx + dy * dy);

        if (duration <= TAP_MAX_DURATION_MS && dist <= TAP_MAX_DISTANCE_PX) {
            // 3-finger tap confirmed -> toggle DebugMenu
            if (debugMenu) {
                debugMenu->toggle();
                LOGI("3-finger tap: DebugMenu %s",
                     debugMenu->isVisible() ? "opened" : "closed");
            }
        }
        threeFingerDown = false;
    }

    // --- 2-finger double-tap detection ---
    if (pointerCount >= MIN_POINTER_COUNT_FOR_2FINGER &&
        pointerCount < MIN_POINTER_COUNT_FOR_3FINGER) {
        if (action == 5 || action == 0) {
            twoFingerDown = true;
        }
    }

    if (twoFingerDown && (action == 1 || action == 6)) {
        twoFingerDown = false;

        // Check if this is a continuation of a double-tap
        if (eventTime - lastTapTime <= DOUBLE_TAP_MAX_INTERVAL_MS) {
            float dx = x - lastTapX;
            float dy = y - lastTapY;
            float dist = std::sqrt(dx * dx + dy * dy);

            if (dist <= TAP_MAX_DISTANCE_PX * 2.0f) {
                // 2-finger double-tap confirmed -> toggle DebugHUD
                if (debugHUD) {
                    bool wasVisible = debugHUD->isVisible();
                    debugHUD->setVisible(!wasVisible);
                    LOGI("2-finger double-tap: DebugHUD %s",
                         debugHUD->isVisible() ? "shown" : "hidden");
                }
                tapCount = 0;
                lastTapTime = 0;
                return;
            }
        }

        // First tap of potential double-tap
        tapCount = 1;
        lastTapTime = eventTime;
        lastTapX = x;
        lastTapY = y;
    }

    // Reset tap count if too much time passed
    if (tapCount > 0 && (eventTime - lastTapTime) > DOUBLE_TAP_MAX_INTERVAL_MS) {
        tapCount = 0;
    }
}
