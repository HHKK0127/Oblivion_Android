#pragma once

#include "text_renderer.h"
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>

/**
 * @brief HUDコンパス方向表示
 *
 * Phase 16: ゲーム内HUD - 画面上部のコンパス
 * プレイヤーの向き、周囲のランドマークや目標方向を表示
 */
class UIHudCompass {
public:
    enum CompassMarker {
        MARKER_NONE = 0,
        MARKER_QUEST_TARGET = 1,
        MARKER_NPC = 2,
        MARKER_POI = 3,  // Point of Interest
        MARKER_LOCATION = 4
    };

    struct CompassPoint {
        float angle;      // 0-360 degrees, 0 = North
        std::string label;
        CompassMarker type;
        glm::vec3 color;

        CompassPoint() : angle(0.0f), label(""), type(MARKER_NONE), color(1.0f) {}
    };

    UIHudCompass();
    ~UIHudCompass() = default;

    bool initialize(TextRenderer* textRenderer, int screenW, int screenH);

    void setPlayerRotation(float yawDegrees) { playerYaw_ = yawDegrees; }
    float getPlayerRotation() const { return playerYaw_; }

    // Add/remove compass markers
    void addMarker(int markerId, float angle, const std::string& label,
                   CompassMarker type, glm::vec3 color);
    void removeMarker(int markerId);
    void clearMarkers();

    void update(float deltaTime);
    void render();

    void setScreenSize(int w, int h) {
        screenWidth = w;
        screenHeight = h;
    }

private:
    TextRenderer* textRenderer = nullptr;

    int screenWidth = 1080;
    int screenHeight = 1920;

    float playerYaw_ = 0.0f;  // Player rotation in degrees
    std::unordered_map<int, CompassPoint> markers_;

    static constexpr float COMPASS_WIDTH = 300.0f;
    static constexpr float COMPASS_HEIGHT = 40.0f;
    static constexpr float COMPASS_X_CENTER = 540.0f;  // Half of 1080
    static constexpr float COMPASS_Y = 10.0f;

    // Cardinal direction angles
    static constexpr float ANGLE_NORTH = 0.0f;
    static constexpr float ANGLE_EAST = 90.0f;
    static constexpr float ANGLE_SOUTH = 180.0f;
    static constexpr float ANGLE_WEST = 270.0f;

    void renderCompassBar();
    void renderCardinalDirections();
    void renderMarkers();
    void renderPlayerIndicator();

    float normalizeAngle(float angle) const;
    float getMarkerScreenPosition(float markerAngle) const;
    glm::vec3 getMarkerColor(CompassMarker type) const;

    bool isMarkerInView(float markerAngle) const;
};
