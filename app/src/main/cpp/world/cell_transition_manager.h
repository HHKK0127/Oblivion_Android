#pragma once

#include "world_data.h"
#include <glm/glm.hpp>
#include <memory>
#include <android/log.h>

#define LOG_TAG "CellTransitionManager"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

class CellTransitionManager {
public:
    CellTransitionManager();
    ~CellTransitionManager();

    // Initialize with WorldManager
    bool initialize(class WorldManager* worldMgr);
    void cleanup();

    // Update cell transitions based on player position
    void update(const glm::vec3& playerPos, float deltaTime);

    // Query current cells
    void getCurrentCell(const glm::vec3& pos, int32_t& outCellX, int32_t& outCellY) const;
    void getNearbyLoadedCells(const glm::vec3& pos,
                             std::vector<std::pair<int32_t, int32_t>>& outCells) const;

    // Load/Unload management
    bool shouldLoadCell(int32_t cellX, int32_t cellY) const;
    bool shouldUnloadCell(int32_t cellX, int32_t cellY) const;

    // State tracking
    struct CellLoadState {
        int32_t cellX;
        int32_t cellY;
        bool isLoaded;
        float timeSinceLoaded;
    };

    const std::vector<CellLoadState>& getLoadStates() const { return cellLoadStates; }

private:
    class WorldManager* worldManager;
    std::vector<CellLoadState> cellLoadStates;

    // Grid management
    static constexpr int32_t LOAD_DISTANCE = 2;      // 5x5 grid (±2 from player)
    static constexpr int32_t UNLOAD_DISTANCE = 4;    // Unload cells beyond ±4
    static constexpr float CELL_SIZE = 128.0f;

    void loadAdjacentCells(int32_t centerCellX, int32_t centerCellY);
    void unloadDistantCells(int32_t centerCellX, int32_t centerCellY);
    void transferNPCsBetweenCells(int32_t oldCellX, int32_t oldCellY,
                                   int32_t newCellX, int32_t newCellY);
};
