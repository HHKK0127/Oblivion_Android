#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>

class TextRenderer;
class WorldManager;

/**
 * @brief World Cell Debug Info Display
 *
 * Shows debug information about the current world state:
 * - Current cell coordinates
 * - Loaded cells count
 * - Player position
 * - Camera direction
 * - Terrain info
 * - Active entities count
 */
class WorldDebugInfo {
public:
    WorldDebugInfo();
    ~WorldDebugInfo();

    bool initialize(TextRenderer* textRenderer, WorldManager* worldManager);
    void cleanup();

    void toggle();
    bool isVisible() const { return visible; }
    void setVisible(bool v) { visible = v; }

    void update(float deltaTime);
    void render();

private:
    TextRenderer* textRenderer;
    WorldManager* worldManager;
    bool visible;
    bool initialized;

    int screenWidth;
    int screenHeight;

    float updateInterval;
    float timeSinceLastUpdate;

    // Cached info
    struct CachedInfo {
        glm::vec3 playerPos;
        glm::vec3 cameraPos;
        glm::vec3 cameraForward;
        int32_t currentCellX;
        int32_t currentCellY;
        int loadedCells;
        float fps;
    };
    CachedInfo cachedInfo;

    void updateCachedInfo();
    std::string formatVector(const glm::vec3& v) const;
};
