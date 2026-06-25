#include "ui_hud_compass.h"
#include "placeholder_assets.h"
#include "ui_draw_helper.h"
#include <GLES3/gl3.h>
#include <cmath>
#include <algorithm>
#include <unordered_map>

UIHudCompass::UIHudCompass() = default;

bool UIHudCompass::initialize(TextRenderer* textRenderer, int screenW, int screenH) {
    if (!textRenderer) return false;
    textRenderer_ = textRenderer;
    screenWidth = screenW;
    screenHeight = screenH;
    return true;
}

void UIHudCompass::addMarker(int markerId, float angle, const std::string& label,
                               CompassMarker type, glm::vec3 color) {
    CompassPoint point;
    point.angle = normalizeAngle(angle);
    point.label = label;
    point.type = type;
    point.color = color;
    markers_[markerId] = point;
}

void UIHudCompass::removeMarker(int markerId) {
    markers_.erase(markerId);
}

void UIHudCompass::clearMarkers() {
    markers_.clear();
}

void UIHudCompass::update(float deltaTime) {
    // Compass updates happen in render
}

void UIHudCompass::render() {
    if (!textRenderer_) return;

    GLboolean depthTestEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    renderCompassBar();
    renderCardinalDirections();
    renderMarkers();
    renderPlayerIndicator();

    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
}

void UIHudCompass::renderCompassBar() {
    // Compass background bar
    float compassX = COMPASS_X_CENTER - COMPASS_WIDTH / 2.0f;
    float compassY = COMPASS_Y;

    glm::vec4 bgColor(0.15f, 0.12f, 0.08f, 0.8f);
    UIDrawHelper::drawColoredQuad(compassX, compassY, COMPASS_WIDTH, COMPASS_HEIGHT,
        bgColor, screenWidth, screenHeight);

    // Compass border
    glm::vec4 borderColor(0.5f, 0.4f, 0.2f, 0.9f);
    UIDrawHelper::drawBorder(compassX, compassY, COMPASS_WIDTH, COMPASS_HEIGHT, 1.0f,
        borderColor, screenWidth, screenHeight);
}

void UIHudCompass::renderCardinalDirections() {
    if (!textRenderer_) return;

    float compassX = COMPASS_X_CENTER - COMPASS_WIDTH / 2.0f;
    float compassY = COMPASS_Y;
    float centerX = COMPASS_X_CENTER;

    // Cardinal directions
    struct CardinalDir {
        const char* label;
        float angle;
    };

    CardinalDir directions[] = {
        {"N", ANGLE_NORTH},
        {"E", ANGLE_EAST},
        {"S", ANGLE_SOUTH},
        {"W", ANGLE_WEST}
    };

    for (const auto& dir : directions) {
        float screenPos = getMarkerScreenPosition(dir.angle);
        if (screenPos >= compassX && screenPos <= compassX + COMPASS_WIDTH) {
            glm::vec3 color = PlaceholderAssets::Colors::PARCHMENT_LIGHT;
            textRenderer_->renderText(dir.label,
                screenPos - 3.0f, compassY + 12.0f,
                color, 0.7f);
        }
    }
}

void UIHudCompass::renderMarkers() {
    if (!textRenderer_) return;

    float compassY = COMPASS_Y;

    for (const auto& pair : markers_) {
        const CompassPoint& marker = pair.second;

        if (!isMarkerInView(marker.angle)) continue;

        float screenPos = getMarkerScreenPosition(marker.angle);
        glm::vec3 markerColor = getMarkerColor(marker.type);

        // Render marker indicator
        float markerSize = 6.0f;
        glm::vec4 markerQuadColor(markerColor, 0.9f);
        UIDrawHelper::drawColoredQuad(screenPos - markerSize / 2.0f,
            compassY + COMPASS_HEIGHT / 2.0f - markerSize / 2.0f,
            markerSize, markerSize, markerQuadColor,
            screenWidth, screenHeight);

        // Render marker label if it fits
        if (!marker.label.empty()) {
            textRenderer_->renderText(marker.label,
                screenPos - 10.0f, compassY + COMPASS_HEIGHT - 12.0f,
                markerColor, 0.5f);
        }
    }
}

void UIHudCompass::renderPlayerIndicator() {
    if (!textRenderer_) return;

    float compassX = COMPASS_X_CENTER - COMPASS_WIDTH / 2.0f;
    float compassY = COMPASS_Y;

    // Draw player direction arrow at center
    float arrowSize = 8.0f;
    glm::vec4 arrowColor(0.9f, 0.8f, 0.2f, 1.0f);
    UIDrawHelper::drawColoredQuad(COMPASS_X_CENTER - arrowSize / 2.0f,
        compassY + 2.0f, arrowSize, arrowSize, arrowColor,
        screenWidth, screenHeight);

    // Draw rotation indicator text
    int yawDisplay = static_cast<int>(playerYaw_) % 360;
    if (yawDisplay < 0) yawDisplay += 360;

    std::string yawText = std::to_string(yawDisplay) + "°";
    textRenderer_->renderText(yawText,
        COMPASS_X_CENTER - 15.0f, compassY + COMPASS_HEIGHT - 10.0f,
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT, 0.6f);
}

float UIHudCompass::normalizeAngle(float angle) const {
    while (angle < 0.0f) angle += 360.0f;
    while (angle >= 360.0f) angle -= 360.0f;
    return angle;
}

float UIHudCompass::getMarkerScreenPosition(float markerAngle) const {
    float normalizedMarker = normalizeAngle(markerAngle);
    float normalizedPlayer = normalizeAngle(playerYaw_);

    // Calculate relative angle from player perspective
    float relativeAngle = normalizeAngle(normalizedMarker - normalizedPlayer);

    // Map angle to screen position on compass bar
    // -180 to 180 relative angles map to compass bar positions
    if (relativeAngle > 180.0f) {
        relativeAngle = relativeAngle - 360.0f;
    }

    float compassX = COMPASS_X_CENTER - COMPASS_WIDTH / 2.0f;
    float position = COMPASS_X_CENTER + (relativeAngle / 180.0f) * (COMPASS_WIDTH / 2.0f);

    return position;
}

glm::vec3 UIHudCompass::getMarkerColor(CompassMarker type) const {
    switch (type) {
    case MARKER_QUEST_TARGET:
        return glm::vec3(1.0f, 0.2f, 0.2f);  // Red
    case MARKER_NPC:
        return glm::vec3(0.3f, 0.8f, 0.3f);  // Green
    case MARKER_POI:
        return glm::vec3(0.9f, 0.8f, 0.2f);  // Gold
    case MARKER_LOCATION:
        return glm::vec3(0.4f, 0.6f, 1.0f);  // Blue
    default:
        return glm::vec3(0.8f, 0.8f, 0.8f);  // Gray
    }
}

bool UIHudCompass::isMarkerInView(float markerAngle) const {
    float normalizedMarker = normalizeAngle(markerAngle);
    float normalizedPlayer = normalizeAngle(playerYaw_);

    float relativeAngle = normalizeAngle(normalizedMarker - normalizedPlayer);
    if (relativeAngle > 180.0f) {
        relativeAngle = relativeAngle - 360.0f;
    }

    // Show markers within ~180 degree range in front of player
    return relativeAngle >= -180.0f && relativeAngle <= 180.0f;
}
