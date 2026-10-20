#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "../map/map_system.h"
#include "ui_message_box.h"
#include "ui_sleep_wait_menu.h"

// FastTravelConfirm orchestrates the fast-travel confirmation flow:
//   1. MessageBox: "Fast travel to [location]?"  Yes / No
//   2. If Yes -> time fast-forward via SleepWaitMenu-style loop
//   3. On completion -> callback with destination cell coordinates
//
// Each step chains into the next via callback so no step blocks.
//
// Integration into the game loop requires wiring from renderer.cpp
// (the map->confirm flow) which is deferred to when that file is
// available for editing.
class FastTravelController {
public:
    using TravelCallback = std::function<void(int cellX, int cellY, const std::string& name)>;

    FastTravelController();
    ~FastTravelController();

    // Attach shared UI dependencies (called once from the menu init path).
    void attach(UIMessageBox* msgBox, UISleepWaitMenu* sleepWait);

    // Start the flow for travelling to a specific fast-travel point.
    //   location : display name (e.g. "Bruma")
    //   cellX, cellY : destination exterior cell
    //   onArrive  : called after the travel completes
    void beginTravel(const std::string& location, int cellX, int cellY,
                     TravelCallback onArrive);

    // Whether a travel flow is in progress (MessageBox visible OR fast-forwarding).
    bool isTravelInProgress() const;

    // Frame update — drives the fast-forward progress.
    void update(float deltaTime);

private:
    // Stage 1 callback: after MessageBox Yes/No.
    void onConfirmDialog(int value);

    // Stage 2: perform the actual travel (teleport + callback).
    void executeTravel();

    UIMessageBox* msgBox_ = nullptr;
    UISleepWaitMenu* sleepWait_ = nullptr;

    // Pending travel request (filled by beginTravel, consumed by executeTravel).
    std::string pendingLocation_;
    int pendingCellX_ = 0;
    int pendingCellY_ = 0;
    TravelCallback onArrive_;

    // State.
    bool travelInProgress_ = false;
};