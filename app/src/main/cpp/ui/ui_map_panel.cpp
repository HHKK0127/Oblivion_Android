#include "ui_map_panel.h"
#include "ui_draw_helper.h"
#include <GLES3/gl3.h>
#include <algorithm>
#include <cmath>

namespace ui {

MapUI::MapUI(const std::string& title) : UIPanel(title) {
    // Map panel defaults
    setBackgroundColor(glm::vec4(0.05f, 0.05f, 0.07f, 0.95f));
    setBorderColor(glm::vec4(0.4f, 0.4f, 0.45f, 1.0f));
    setBorderWidth(2.0f);
    setTitleBarColor(glm::vec4(0.2f, 0.2f, 0.25f, 0.95f));
    setTitle(title);
    setCloseButtonVisible(true);
    setDraggable(true);
}

void MapUI::setMapSystem(map::MapSystem* system) {
    mapSystem = system;
}

void MapUI::setMiniMapMode(bool mini) {
    miniMapMode = mini;
    if (miniMapMode) {
        setTitleBarHeight(0.0f);
        setCloseButtonVisible(false);
        setDraggable(false);
        setBackgroundColor(glm::vec4(0.0f, 0.0f, 0.0f, 0.6f));
        setBorderWidth(1.0f);
        if (mapSystem) {
            cameraOffset = mapSystem->getPlayerPosition();
        }
        zoom = 0.3f;
    } else {
        setTitleBarHeight(40.0f);
        setCloseButtonVisible(true);
        setDraggable(true);
        setBackgroundColor(glm::vec4(0.05f, 0.05f, 0.07f, 0.95f));
        setBorderWidth(2.0f);
        zoom = 1.0f;
    }
}

void MapUI::update(float deltaTime) {
    UIPanel::update(deltaTime);

    if (miniMapMode && mapSystem) {
        cameraOffset = mapSystem->getPlayerPosition();
    }
}

bool MapUI::onTouchDown(float x, float y, int pointerId) {
    if (!isVisible() || !isEnabled()) return false;

    // Close button
    if (isInsideCloseButton(x, y)) {
        return UIPanel::onTouchDown(x, y, pointerId);
    }

    // Title bar drag
    if (isInsideTitleBar(x, y)) {
        return UIPanel::onTouchDown(x, y, pointerId);
    }

    // Map panning (client area only)
    glm::vec2 cp = getContentPosition();
    glm::vec2 cs = getContentSize();
    if (x >= cp.x && x <= cp.x + cs.x && y >= cp.y && y <= cp.y + cs.y) {
        mapDragging = true;
        lastDragPos = glm::vec2(x, y);
        return true;
    }

    return false;
}

bool MapUI::onTouchMove(float x, float y, float dx, float dy, int pointerId) {
    if (!isVisible() || !isEnabled()) return false;

    // Let panel handle title bar drag
    if (UIPanel::isDragging) {
        return UIPanel::onTouchMove(x, y, dx, dy, pointerId);
    }

    // Map panning
    if (mapDragging) {
        cameraOffset.x -= dx / zoom;
        cameraOffset.y -= dy / zoom;
        lastDragPos = glm::vec2(x, y);
        return true;
    }

    return false;
}

bool MapUI::onTouchUp(float x, float y, int pointerId) {
    // Always clear panel drag state
    bool panelResult = UIPanel::onTouchUp(x, y, pointerId);
    mapDragging = false;
    return panelResult;
}

void MapUI::zoomIn(float factor) {
    zoom = std::min(zoom * factor, MAX_ZOOM);
}

void MapUI::zoomOut(float factor) {
    zoom = std::max(zoom / factor, MIN_ZOOM);
}

void MapUI::resetView() {
    zoom = 1.0f;
    if (mapSystem) {
        cameraOffset = mapSystem->getPlayerPosition();
    } else {
        cameraOffset = glm::vec2(0.0f, 0.0f);
    }
}

void MapUI::render() {
    if (!isVisible()) return;

    if (!miniMapMode) {
        UIPanel::render();
    } else {
        // Mini-map: just background + content, no title bar
        GLboolean depthTestEnabled;
        glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glm::vec2 absPos = getAbsolutePosition();
        UIDrawHelper::drawColoredQuad(absPos.x, absPos.y, size.x, size.y,
                                      backgroundColor, screenWidth, screenHeight);
        if (borderWidth > 0.0f) {
            UIDrawHelper::drawBorder(absPos.x, absPos.y, size.x, size.y,
                                     borderWidth, borderColor, screenWidth, screenHeight);
        }

        renderMapContent();

        if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
    }

    if (!miniMapMode) {
        renderMapContent();
    }
}

void MapUI::renderMapContent() {
    if (!mapSystem) return;

    glm::vec2 cp = getContentPosition();
    glm::vec2 cs = getContentSize();
    if (cs.x <= 0.0f || cs.y <= 0.0f) return;

    float cellSize = mapSystem->getCellSize();
    glm::vec4 bounds = mapSystem->getWorldBounds();

    // Calculate visible world range
    float halfW = (cs.x * 0.5f) / zoom;
    float halfH = (cs.y * 0.5f) / zoom;
    float minWX = cameraOffset.x - halfW;
    float maxWX = cameraOffset.x + halfW;
    float minWY = cameraOffset.y - halfH;
    float maxWY = cameraOffset.y + halfH;

    int minCX = static_cast<int>(std::floor(minWX / cellSize));
    int maxCX = static_cast<int>(std::floor(maxWX / cellSize));
    int minCY = static_cast<int>(std::floor(minWY / cellSize));
    int maxCY = static_cast<int>(std::floor(maxWY / cellSize));

    // Clamp to world bounds if set
    if (bounds.y > bounds.x) {
        minCX = std::max(minCX, static_cast<int>(std::floor(bounds.x / cellSize)));
        maxCX = std::min(maxCX, static_cast<int>(std::floor(bounds.y / cellSize)));
    }
    if (bounds.w > bounds.z) {
        minCY = std::max(minCY, static_cast<int>(std::floor(bounds.z / cellSize)));
        maxCY = std::min(maxCY, static_cast<int>(std::floor(bounds.w / cellSize)));
    }

    // Render discovered cells
    for (int cy = minCY; cy <= maxCY; ++cy) {
        for (int cx = minCX; cx <= maxCX; ++cx) {
            bool discovered = mapSystem->isCellDiscovered(cx, cy);
            if (!discovered) continue;

            map::CellInfo* info = mapSystem->getCellInfo(cx, cy);
            uint32_t color = info ? info->terrainColor : 0xFF888888;

            float wx = static_cast<float>(cx) * cellSize;
            float wy = static_cast<float>(cy) * cellSize;
            glm::vec2 tl = worldToScreen(glm::vec2(wx, wy));
            glm::vec2 br = worldToScreen(glm::vec2(wx + cellSize, wy + cellSize));

            // Convert ABGR to glm::vec4 RGBA
            glm::vec4 col(
                static_cast<float>((color >> 16) & 0xFF) / 255.0f,
                static_cast<float>((color >> 8) & 0xFF) / 255.0f,
                static_cast<float>(color & 0xFF) / 255.0f,
                static_cast<float>((color >> 24) & 0xFF) / 255.0f
            );

            float rx = tl.x;
            float ry = tl.y;
            float rw = br.x - tl.x;
            float rh = br.y - tl.y;
            if (rw > 0.0f && rh > 0.0f) {
                UIDrawHelper::drawColoredQuad(rx, ry, rw, rh, col, screenWidth, screenHeight);
            }
        }
    }

    // Render grid lines
    if (showGrid) {
        glm::vec4 gridColor(0.3f, 0.3f, 0.35f, 0.5f);
        for (int cx = minCX; cx <= maxCX + 1; ++cx) {
            float wx = static_cast<float>(cx) * cellSize;
            glm::vec2 top = worldToScreen(glm::vec2(wx, minWY));
            glm::vec2 bot = worldToScreen(glm::vec2(wx, maxWY));
            if (top.x >= cp.x && top.x <= cp.x + cs.x) {
                UIDrawHelper::drawColoredQuad(top.x, cp.y, 1.0f, cs.y, gridColor, screenWidth, screenHeight);
            }
        }
        for (int cy = minCY; cy <= maxCY + 1; ++cy) {
            float wy = static_cast<float>(cy) * cellSize;
            glm::vec2 left = worldToScreen(glm::vec2(minWX, wy));
            glm::vec2 right = worldToScreen(glm::vec2(maxWX, wy));
            if (left.y >= cp.y && left.y <= cp.y + cs.y) {
                UIDrawHelper::drawColoredQuad(cp.x, left.y, cs.x, 1.0f, gridColor, screenWidth, screenHeight);
            }
        }
    }

    // Render markers
    if (showMarkers) {
        for (const auto& m : mapSystem->getMarkers()) {
            if (!m.visible) continue;
            glm::vec2 sp = worldToScreen(m.worldPos);
            if (sp.x < cp.x || sp.x > cp.x + cs.x || sp.y < cp.y || sp.y > cp.y + cs.y) continue;

            float ms = MARKER_SIZE;
            glm::vec4 mcol(
                static_cast<float>((m.color >> 16) & 0xFF) / 255.0f,
                static_cast<float>((m.color >> 8) & 0xFF) / 255.0f,
                static_cast<float>(m.color & 0xFF) / 255.0f,
                static_cast<float>((m.color >> 24) & 0xFF) / 255.0f
            );
            UIDrawHelper::drawColoredQuad(sp.x - ms * 0.5f, sp.y - ms * 0.5f, ms, ms, mcol, screenWidth, screenHeight);
        }
    }

    // Render player marker
    if (showPlayer) {
        glm::vec2 pp = mapSystem->getPlayerPosition();
        glm::vec2 sp = worldToScreen(pp);
        if (sp.x >= cp.x && sp.x <= cp.x + cs.x && sp.y >= cp.y && sp.y <= cp.y + cs.y) {
            float ps = PLAYER_MARKER_SIZE;
            glm::vec4 pcol(0.2f, 1.0f, 0.2f, 1.0f); // bright green
            UIDrawHelper::drawColoredQuad(sp.x - ps * 0.5f, sp.y - ps * 0.5f, ps, ps, pcol, screenWidth, screenHeight);
        }
    }
}

glm::vec2 MapUI::worldToScreen(const glm::vec2& worldPos) const {
    glm::vec2 cp = getContentPosition();
    glm::vec2 cs = getContentSize();
    float sx = cp.x + (worldPos.x - cameraOffset.x) * zoom + cs.x * 0.5f;
    float sy = cp.y + (worldPos.y - cameraOffset.y) * zoom + cs.y * 0.5f;
    return glm::vec2(sx, sy);
}

glm::vec2 MapUI::screenToWorld(const glm::vec2& screenPos) const {
    glm::vec2 cp = getContentPosition();
    glm::vec2 cs = getContentSize();
    float wx = (screenPos.x - cp.x - cs.x * 0.5f) / zoom + cameraOffset.x;
    float wy = (screenPos.y - cp.y - cs.y * 0.5f) / zoom + cameraOffset.y;
    return glm::vec2(wx, wy);
}

} // namespace ui
