#include "crosshair.h"
#include "ui_draw_helper.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

Crosshair::Crosshair() : UIComponent("Crosshair") {
    setVisible(true);
}

bool Crosshair::initialize() {
    if (!UIComponent::initialize()) {
        return false;
    }

    // Set initial position to center
    centerX = static_cast<float>(screenWidth) / 2.0f;
    centerY = static_cast<float>(screenHeight) / 2.0f;

    return true;
}

void Crosshair::onScreenResize(int width, int height) {
    UIComponent::onScreenResize(width, height);
    centerX = static_cast<float>(width) / 2.0f;
    centerY = static_cast<float>(height) / 2.0f;
}

void Crosshair::render() {
    if (!isVisible()) return;

    switch (style) {
        case Style::DOT:
            renderDot();
            break;
        case Style::CROSS:
            renderCross();
            break;
        case Style::CIRCLE:
            renderCircle();
            break;
        case Style::CROSS_DOT:
            renderCross();
            renderDot();
            break;
    }
}

void Crosshair::renderDot() {
    float dotSize = crosshairThickness * 3.0f;
    float halfSize = dotSize / 2.0f;
    UIDrawHelper::drawColoredQuad(
        centerX - halfSize, centerY - halfSize,
        dotSize, dotSize,
        crosshairColor,
        screenWidth, screenHeight
    );
}

void Crosshair::renderCross() {
    float halfSize = crosshairSize / 2.0f;
    float gap = crosshairGap;
    float thickness = crosshairThickness;

    // Vertical line (top)
    UIDrawHelper::drawColoredQuad(
        centerX - thickness / 2.0f, centerY - halfSize,
        thickness, halfSize - gap,
        crosshairColor,
        screenWidth, screenHeight
    );
    // Vertical line (bottom)
    UIDrawHelper::drawColoredQuad(
        centerX - thickness / 2.0f, centerY + gap,
        thickness, halfSize - gap,
        crosshairColor,
        screenWidth, screenHeight
    );
    // Horizontal line (left)
    UIDrawHelper::drawColoredQuad(
        centerX - halfSize, centerY - thickness / 2.0f,
        halfSize - gap, thickness,
        crosshairColor,
        screenWidth, screenHeight
    );
    // Horizontal line (right)
    UIDrawHelper::drawColoredQuad(
        centerX + gap, centerY - thickness / 2.0f,
        halfSize - gap, thickness,
        crosshairColor,
        screenWidth, screenHeight
    );
}

void Crosshair::renderCircle() {
    // Approximate circle with a small quad for now
    float radius = crosshairSize / 2.0f;
    float thickness = crosshairThickness;
    UIDrawHelper::drawColoredQuad(
        centerX - radius, centerY - radius,
        radius * 2.0f, radius * 2.0f,
        glm::vec4(crosshairColor.x, crosshairColor.y, crosshairColor.z, crosshairColor.w * 0.3f),
        screenWidth, screenHeight
    );
    // Draw a smaller quad inside to simulate a circle outline
    float innerRadius = radius - thickness;
    if (innerRadius > 0) {
        UIDrawHelper::drawColoredQuad(
            centerX - innerRadius, centerY - innerRadius,
            innerRadius * 2.0f, innerRadius * 2.0f,
            glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
            screenWidth, screenHeight
        );
    }
}
