#pragma once

#include "ui_panel.h"
#include "../map/map_system.h"
#include <memory>
#include <vector>

namespace ui {

class MapUI : public UIPanel {
public:
    explicit MapUI(const std::string& title = "Map");
    ~MapUI() override = default;

    void setMapSystem(map::MapSystem* system);
    void setMiniMapMode(bool mini);
    bool isMiniMapMode() const { return miniMapMode; }

    void update(float deltaTime) override;
    bool onTouchDown(float x, float y, int pointerId) override;
    bool onTouchUp(float x, float y, int pointerId) override;
    bool onTouchMove(float x, float y, float dx, float dy, int pointerId) override;
    void render() override;

    // Zoom controls
    void zoomIn(float factor = 1.2f);
    void zoomOut(float factor = 1.2f);
    void resetView();

    void setShowGrid(bool show) { showGrid = show; }
    void setShowMarkers(bool show) { showMarkers = show; }
    void setShowPlayer(bool show) { showPlayer = show; }

protected:
    void renderMapContent();
    void renderMapBackground();
    void renderGrid();
    void renderCells();
    void renderMarkers();
    void renderPlayerMarker();

    // Convert world position to screen position within panel
    glm::vec2 worldToScreen(const glm::vec2& worldPos) const;
    glm::vec2 screenToWorld(const glm::vec2& screenPos) const;

    bool isPointInsideMap(float screenX, float screenY) const;

private:
    map::MapSystem* mapSystem = nullptr;
    bool miniMapMode = false;

    float zoom = 1.0f;
    glm::vec2 cameraOffset{0.0f, 0.0f};
    bool mapDragging = false;
    glm::vec2 lastDragPos{0.0f, 0.0f};

    bool showGrid = true;
    bool showMarkers = true;
    bool showPlayer = true;

    static constexpr float MIN_ZOOM = 0.25f;
    static constexpr float MAX_ZOOM = 5.0f;
    static constexpr float PLAYER_MARKER_SIZE = 8.0f;
    static constexpr float MARKER_SIZE = 6.0f;
};

} // namespace ui
