#pragma once

#include "text_renderer.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <unordered_map>

/**
 * @brief ミニマップ表示
 *
 * Phase 23: ゲーム内HUD - ミニマップ
 * プレイヤー周辺のマップを右上に表示、マーカーと方角を含む
 */
class UIMinimap {
public:
    enum MarkerType {
        MARKER_PLAYER = 0,
        MARKER_ENEMY = 1,
        MARKER_NPC = 2,
        MARKER_ITEM = 3,
        MARKER_QUEST = 4,
        MARKER_DOOR = 5,
        MARKER_POI = 6
    };

    struct MapMarker {
        int id;
        float worldX;
        float worldZ;
        MarkerType type;
        std::string label;
        glm::vec3 color;
        bool visible;
    };

    struct MapCell {
        bool explored;
        bool walkable;
        glm::vec3 color;

        MapCell() : explored(false), walkable(true), color(0.3f, 0.3f, 0.3f) {}
    };

    UIMinimap();
    ~UIMinimap() = default;

    bool initialize(TextRenderer* textRenderer, int screenW, int screenH);

    // Player position / rotation
    void setPlayerPosition(float worldX, float worldZ) {
        playerX_ = worldX;
        playerZ_ = worldZ;
    }
    void setPlayerRotation(float yawDegrees) { playerYaw_ = yawDegrees; }

    // Map data
    void setMapSize(int width, int height);
    void setCell(int x, int z, bool explored, bool walkable, glm::vec3 color);
    void revealCircle(float worldX, float worldZ, float radius);

    // Markers
    void addMarker(int id, float worldX, float worldZ, MarkerType type,
                   const std::string& label = "");
    void updateMarker(int id, float worldX, float worldZ);
    void removeMarker(int id);
    void clearMarkers();

    // Visibility
    void setVisible(bool visible) { visible_ = visible; }
    bool isVisible() const { return visible_; }
    void toggleVisible() { visible_ = !visible_; }

    void update(float deltaTime);
    void render();

    void setScreenSize(int w, int h);

private:
    TextRenderer* textRenderer = nullptr;

    int screenWidth = 1080;
    int screenHeight = 1920;

    float playerX_ = 0.0f;
    float playerZ_ = 0.0f;
    float playerYaw_ = 0.0f;
    bool visible_ = true;

    int mapWidth_ = 0;
    int mapHeight_ = 0;
    std::vector<MapCell> mapCells_;

    std::unordered_map<int, MapMarker> markers_;

    // Display settings
    static constexpr float MAP_SIZE = 160.0f;   // Pixel size of minimap
    static constexpr float MAP_MARGIN = 12.0f;  // Margin from screen edge
    static constexpr float VIEW_RADIUS = 30.0f; // World units visible radius
    static constexpr float MARKER_SIZE = 5.0f;
    static constexpr float PLAYER_MARKER_SIZE = 7.0f;

    // Computed at render time
    float mapX_ = 0.0f;  // Top-left X of minimap on screen
    float mapY_ = 0.0f;  // Top-left Y of minimap on screen

    void computeMapPosition();
    void renderBackground();
    void renderExploredCells();
    void renderMarkers();
    void renderPlayerMarker();
    void renderBorderAndCompass();

    // Coordinate conversion
    glm::vec2 worldToMinimap(float worldX, float worldZ) const;
    bool isInsideMap(glm::vec2 mapPos) const;

    glm::vec3 getMarkerColor(MarkerType type) const;
};
