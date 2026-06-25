#include "ui_minimap.h"
#include "placeholder_assets.h"
#include "ui_draw_helper.h"
#include <GLES3/gl3.h>
#include <cmath>
#include <algorithm>

UIMinimap::UIMinimap() = default;

bool UIMinimap::initialize(TextRenderer* textRenderer, int screenW, int screenH) {
    if (!textRenderer) return false;
    textRenderer_ = textRenderer;
    screenWidth = screenW;
    screenHeight = screenH;
    computeMapPosition();
    return true;
}

void UIMinimap::setMapSize(int width, int height) {
    mapWidth_ = width;
    mapHeight_ = height;
    mapCells_.assign(width * height, MapCell());
}

void UIMinimap::setCell(int x, int z, bool explored, bool walkable, glm::vec3 color) {
    if (x < 0 || x >= mapWidth_ || z < 0 || z >= mapHeight_) return;
    MapCell& cell = mapCells_[z * mapWidth_ + x];
    cell.explored = explored;
    cell.walkable = walkable;
    cell.color = color;
}

void UIMinimap::revealCircle(float worldX, float worldZ, float radius) {
    if (mapWidth_ <= 0 || mapHeight_ <= 0) return;

    int cellRadius = static_cast<int>(radius) + 1;
    int centerCX = static_cast<int>(worldX);
    int centerCZ = static_cast<int>(worldZ);

    for (int dz = -cellRadius; dz <= cellRadius; dz++) {
        for (int dx = -cellRadius; dx <= cellRadius; dx++) {
            if (dx * dx + dz * dz <= cellRadius * cellRadius) {
                int cx = centerCX + dx;
                int cz = centerCZ + dz;
                if (cx >= 0 && cx < mapWidth_ && cz >= 0 && cz < mapHeight_) {
                    mapCells_[cz * mapWidth_ + cx].explored = true;
                }
            }
        }
    }
}

void UIMinimap::addMarker(int id, float worldX, float worldZ,
                           MarkerType type, const std::string& label) {
    MapMarker marker;
    marker.id = id;
    marker.worldX = worldX;
    marker.worldZ = worldZ;
    marker.type = type;
    marker.label = label;
    marker.color = getMarkerColor(type);
    marker.visible = true;
    markers_[id] = marker;
}

void UIMinimap::updateMarker(int id, float worldX, float worldZ) {
    auto it = markers_.find(id);
    if (it != markers_.end()) {
        it->second.worldX = worldX;
        it->second.worldZ = worldZ;
    }
}

void UIMinimap::removeMarker(int id) {
    markers_.erase(id);
}

void UIMinimap::clearMarkers() {
    markers_.clear();
}

void UIMinimap::setScreenSize(int w, int h) {
    screenWidth = w;
    screenHeight = h;
    computeMapPosition();
}

void UIMinimap::update(float /* deltaTime */) {
    // Dynamic updates (e.g. fog of war) can go here
}

void UIMinimap::render() {
    if (!visible_ || !textRenderer_) return;

    GLboolean depthTestEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    renderBackground();
    renderExploredCells();
    renderMarkers();
    renderPlayerMarker();
    renderBorderAndCompass();

    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
}

void UIMinimap::computeMapPosition() {
    // Top-right corner
    mapX_ = screenWidth - MAP_SIZE - MAP_MARGIN;
    mapY_ = MAP_MARGIN;
}

void UIMinimap::renderBackground() {
    // Dark background
    glm::vec4 bgColor(0.08f, 0.06f, 0.04f, 0.85f);
    UIDrawHelper::drawColoredQuad(mapX_, mapY_, MAP_SIZE, MAP_SIZE,
        bgColor, screenWidth, screenHeight);
}

void UIMinimap::renderExploredCells() {
    if (mapWidth_ <= 0 || mapHeight_ <= 0) {
        // No map data: draw a simple placeholder grid
        float cellSize = 4.0f;
        int gridCount = static_cast<int>(MAP_SIZE / cellSize);
        for (int z = 0; z < gridCount; z++) {
            for (int x = 0; x < gridCount; x++) {
                float cx = mapX_ + x * cellSize;
                float cy = mapY_ + z * cellSize;
                glm::vec4 cellColor(0.15f, 0.12f, 0.08f, 0.6f);
                UIDrawHelper::drawColoredQuad(cx + 0.5f, cy + 0.5f,
                    cellSize - 1.0f, cellSize - 1.0f,
                    cellColor, screenWidth, screenHeight);
            }
        }
        return;
    }

    // Render explored cells as pixels in minimap
    for (int z = 0; z < mapHeight_; z++) {
        for (int x = 0; x < mapWidth_; x++) {
            const MapCell& cell = mapCells_[z * mapWidth_ + x];
            if (!cell.explored) continue;

            glm::vec2 screenPos = worldToMinimap(static_cast<float>(x),
                                                  static_cast<float>(z));
            if (!isInsideMap(screenPos)) continue;

            glm::vec3 c = cell.walkable ? cell.color : glm::vec3(0.4f, 0.35f, 0.25f);
            glm::vec4 cellColor(c, 0.9f);
            UIDrawHelper::drawColoredQuad(screenPos.x, screenPos.y,
                2.0f, 2.0f, cellColor, screenWidth, screenHeight);
        }
    }
}

void UIMinimap::renderMarkers() {
    for (const auto& pair : markers_) {
        const MapMarker& marker = pair.second;
        if (!marker.visible) continue;
        if (marker.type == MARKER_PLAYER) continue;  // Drawn separately

        glm::vec2 screenPos = worldToMinimap(marker.worldX, marker.worldZ);
        if (!isInsideMap(screenPos)) continue;

        glm::vec4 markerColor(marker.color, 0.95f);
        float half = MARKER_SIZE / 2.0f;
        UIDrawHelper::drawColoredQuad(screenPos.x - half, screenPos.y - half,
            MARKER_SIZE, MARKER_SIZE, markerColor, screenWidth, screenHeight);
    }
}

void UIMinimap::renderPlayerMarker() {
    glm::vec2 center(mapX_ + MAP_SIZE / 2.0f, mapY_ + MAP_SIZE / 2.0f);

    // Player dot
    float half = PLAYER_MARKER_SIZE / 2.0f;
    glm::vec4 playerColor(0.2f, 0.8f, 1.0f, 1.0f);
    UIDrawHelper::drawColoredQuad(center.x - half, center.y - half,
        PLAYER_MARKER_SIZE, PLAYER_MARKER_SIZE,
        playerColor, screenWidth, screenHeight);

    // Direction indicator: small line in front
    float rad = playerYaw_ * (3.14159265f / 180.0f);
    float dx = std::sin(rad) * 8.0f;
    float dz = -std::cos(rad) * 8.0f;

    glm::vec4 dirColor(1.0f, 1.0f, 0.2f, 1.0f);
    UIDrawHelper::drawColoredQuad(center.x + dx - 1.5f, center.y + dz - 1.5f,
        3.0f, 3.0f, dirColor, screenWidth, screenHeight);
}

void UIMinimap::renderBorderAndCompass() {
    // Outer border
    glm::vec4 borderColor(0.55f, 0.45f, 0.25f, 1.0f);
    UIDrawHelper::drawBorder(mapX_, mapY_, MAP_SIZE, MAP_SIZE, 2.0f,
        borderColor, screenWidth, screenHeight);

    // Cardinal labels
    glm::vec3 labelColor = PlaceholderAssets::Colors::PARCHMENT_LIGHT;
    float center = mapX_ + MAP_SIZE / 2.0f;

    textRenderer_->renderText("N", center - 3.0f, mapY_ - 14.0f, labelColor, 0.6f);
    textRenderer_->renderText("S", center - 3.0f, mapY_ + MAP_SIZE + 4.0f, labelColor, 0.6f);
    textRenderer_->renderText("W", mapX_ - 12.0f, mapY_ + MAP_SIZE / 2.0f - 5.0f, labelColor, 0.6f);
    textRenderer_->renderText("E", mapX_ + MAP_SIZE + 4.0f, mapY_ + MAP_SIZE / 2.0f - 5.0f, labelColor, 0.6f);
}

glm::vec2 UIMinimap::worldToMinimap(float worldX, float worldZ) const {
    float centerX = mapX_ + MAP_SIZE / 2.0f;
    float centerZ = mapY_ + MAP_SIZE / 2.0f;

    float relX = worldX - playerX_;
    float relZ = worldZ - playerZ_;

    float scale = (MAP_SIZE / 2.0f) / VIEW_RADIUS;

    return glm::vec2(centerX + relX * scale, centerZ + relZ * scale);
}

bool UIMinimap::isInsideMap(glm::vec2 mapPos) const {
    return mapPos.x >= mapX_ && mapPos.x <= mapX_ + MAP_SIZE &&
           mapPos.y >= mapY_ && mapPos.y <= mapY_ + MAP_SIZE;
}

glm::vec3 UIMinimap::getMarkerColor(MarkerType type) const {
    switch (type) {
    case MARKER_PLAYER:  return glm::vec3(0.2f, 0.8f, 1.0f);  // Cyan
    case MARKER_ENEMY:   return glm::vec3(1.0f, 0.2f, 0.2f);  // Red
    case MARKER_NPC:     return glm::vec3(0.3f, 0.9f, 0.3f);  // Green
    case MARKER_ITEM:    return glm::vec3(0.9f, 0.8f, 0.2f);  // Gold
    case MARKER_QUEST:   return glm::vec3(1.0f, 0.5f, 0.0f);  // Orange
    case MARKER_DOOR:    return glm::vec3(0.6f, 0.4f, 0.2f);  // Brown
    case MARKER_POI:     return glm::vec3(0.7f, 0.5f, 1.0f);  // Purple
    default:             return glm::vec3(0.8f, 0.8f, 0.8f);  // Gray
    }
}
