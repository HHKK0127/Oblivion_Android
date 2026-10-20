#include "fast_travel_controller.h"
#include <android/log.h>

#define LOG_TAG "FastTravel"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

FastTravelController::FastTravelController() = default;
FastTravelController::~FastTravelController() = default;

void FastTravelController::attach(UIMessageBox* msgBox, UISleepWaitMenu* sleepWait) {
    msgBox_ = msgBox;
    sleepWait_ = sleepWait;
}

void FastTravelController::beginTravel(const std::string& location, int cellX, int cellY,
                                       TravelCallback onArrive) {
    if (!msgBox_) {
        LOGE("FastTravelController: MessageBox not attached");
        return;
    }
    pendingLocation_ = location;
    pendingCellX_ = cellX;
    pendingCellY_ = cellY;
    onArrive_ = std::move(onArrive);
    travelInProgress_ = true;

    // Stage 1: confirmation dialog
    const std::string body = "Fast travel to " + location + "?";
    msgBox_->open(body, {"Yes", "No"},
                  [this](int value) { onConfirmDialog(value); });
    LOGI("FastTravel: confirmation dialog for '%s' (%d,%d)", location.c_str(), cellX, cellY);
}

bool FastTravelController::isTravelInProgress() const {
    return travelInProgress_;
}

void FastTravelController::update(float /*deltaTime*/) {
    // If the confirm dialog is visible, wait for the callback.
    // If fast-forward is in progress, the UISleepWaitMenu::update drives it.
    // The sleepWait callback fires when fast-forward completes.
}

void FastTravelController::onConfirmDialog(int value) {
    if (value != 0) {
        // "No" or cancelled
        LOGI("FastTravel: cancelled by user");
        travelInProgress_ = false;
        pendingLocation_.clear();
        return;
    }
    // "Yes" — execute travel immediately (no wait loop needed for FT)
    LOGI("FastTravel: confirmed, travelling to '%s' (%d,%d)",
         pendingLocation_.c_str(), pendingCellX_, pendingCellY_);
    executeTravel();
}

void FastTravelController::executeTravel() {
    if (onArrive_) {
        onArrive_(pendingCellX_, pendingCellY_, pendingLocation_);
    }
    LOGI("FastTravel: arrived at '%s'", pendingLocation_.c_str());
    travelInProgress_ = false;
    pendingLocation_.clear();
}