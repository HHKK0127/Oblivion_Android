#pragma once

#include "ui_component.h"
#include <GLES3/gl3.h>

/**
 * @brief Crosshair overlay for aiming
 *
 * Renders a crosshair at the center of the screen.
 * Supports different styles (dot, cross, circle).
 */
class Crosshair : public UIComponent {
public:
    enum class Style {
        DOT,        // Simple dot
        CROSS,      // Cross shape (+)
        CIRCLE,     // Circle outline
        CROSS_DOT   // Cross with center dot
    };

    Crosshair();
    ~Crosshair() override = default;

    bool initialize();
    void render() override;
    void onScreenResize(int width, int height) override;

    // Style
    void setStyle(Style style) { this->style = style; }
    Style getStyle() const { return style; }

    // Size
    void setSize(float size) { crosshairSize = size; }
    float getSize() const { return crosshairSize; }

    // Color
    void setColor(const glm::vec4& color) { crosshairColor = color; }
    const glm::vec4& getColor() const { return crosshairColor; }

    // Opacity
    void setOpacity(float opacity) { crosshairColor.w = opacity; }
    float getOpacity() const { return crosshairColor.w; }

    // Gap (space between cross lines and center)
    void setGap(float gap) { crosshairGap = gap; }
    float getGap() const { return crosshairGap; }

    // Thickness
    void setThickness(float thickness) { crosshairThickness = thickness; }
    float getThickness() const { return crosshairThickness; }

private:
    Style style = Style::CROSS_DOT;
    float crosshairSize = 20.0f;
    float crosshairGap = 4.0f;
    float crosshairThickness = 2.0f;
    glm::vec4 crosshairColor = glm::vec4(1.0f, 1.0f, 1.0f, 0.8f);

    // Center position (screen center)
    float centerX = 0.0f;
    float centerY = 0.0f;

    void renderDot();
    void renderCross();
    void renderCircle();

    // Simple OpenGL drawing helpers
    void drawLine(float x1, float y1, float x2, float y2, float thickness, const glm::vec4& color);
    void drawCircle(float cx, float cy, float radius, const glm::vec4& color, int segments = 16);
    void drawFilledCircle(float cx, float cy, float radius, const glm::vec4& color, int segments = 16);
};
