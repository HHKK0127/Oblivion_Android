#include "ui_floating_text.h"
#include "placeholder_assets.h"
#include <GLES3/gl3.h>
#include <cmath>
#include <algorithm>

UIFloatingText::UIFloatingText() = default;

bool UIFloatingText::initialize(TextRenderer* textRenderer, int screenW, int screenH) {
    if (!textRenderer) return false;
    textRenderer_ = textRenderer;
    screenWidth = screenW;
    screenHeight = screenH;
    return true;
}

void UIFloatingText::addText(const std::string& text, float screenX, float screenY,
                              TextType type, float duration) {
    if (instances_.size() >= MAX_INSTANCES) {
        removeExpiredInstances();
    }

    if (instances_.size() >= MAX_INSTANCES) {
        return;  // Still at max capacity, drop the new text
    }

    auto instance = std::make_shared<FloatingTextInstance>(
        text, glm::vec3(screenX, screenY, 0.0f), type, duration);
    instances_.push_back(instance);
}

void UIFloatingText::addWorldText(const std::string& text, glm::vec3 worldPos,
                                   TextType type, float duration) {
    // For now, treat world position as screen position
    // In a full implementation, this would use camera projection
    addText(text, worldPos.x, worldPos.y, type, duration);
}

void UIFloatingText::update(float deltaTime) {
    for (auto& instance : instances_) {
        instance->elapsedTime += deltaTime;

        // Move text upward
        instance->position.y += instance->velocity_y * deltaTime;
    }

    removeExpiredInstances();
}

void UIFloatingText::render() {
    if (!textRenderer_) return;

    GLboolean depthTestEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    for (const auto& instance : instances_) {
        if (instance->elapsedTime >= instance->duration) continue;

        glm::vec3 color = getTextColor(instance->type);
        float scale = getTextScale(instance->type);
        float alpha = getCurrentAlpha(*instance);

        // Adjust color with alpha
        glm::vec3 fadeColor = color * alpha;

        textRenderer_->renderText(instance->text,
            instance->position.x, instance->position.y,
            fadeColor, scale);
    }

    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
}

glm::vec3 UIFloatingText::getTextColor(TextType type) const {
    switch (type) {
    case DAMAGE:
        return glm::vec3(1.0f, 0.2f, 0.2f);  // Red
    case HEAL:
        return glm::vec3(0.2f, 0.9f, 0.2f);  // Green
    case CRITICAL:
        return glm::vec3(1.0f, 0.8f, 0.0f);  // Gold
    case MISS:
        return glm::vec3(0.7f, 0.7f, 0.7f);  // Gray
    case BLOCK:
        return glm::vec3(0.3f, 0.6f, 1.0f);  // Blue
    case BUFF:
        return glm::vec3(0.5f, 1.0f, 0.5f);  // Light green
    case DEBUFF:
        return glm::vec3(1.0f, 0.5f, 0.5f);  // Light red
    case LEVEL_UP:
        return PlaceholderAssets::Colors::GOLD_HIGHLIGHT;
    default:
        return glm::vec3(1.0f, 1.0f, 1.0f);  // White
    }
}

float UIFloatingText::getTextScale(TextType type) const {
    switch (type) {
    case CRITICAL:
    case LEVEL_UP:
        return 1.0f;  // Larger
    case DAMAGE:
    case HEAL:
        return 0.8f;  // Medium
    default:
        return 0.6f;  // Small
    }
}

float UIFloatingText::getCurrentAlpha(const FloatingTextInstance& instance) const {
    float progress = instance.elapsedTime / instance.duration;

    // Fade out in the last 0.3 seconds
    float fadeStartTime = instance.duration * 0.7f;
    if (instance.elapsedTime >= fadeStartTime) {
        float fadeProgress = (instance.elapsedTime - fadeStartTime) / (instance.duration - fadeStartTime);
        return instance.startAlpha * (1.0f - fadeProgress);
    }

    return instance.startAlpha;
}

void UIFloatingText::removeExpiredInstances() {
    instances_.erase(
        std::remove_if(instances_.begin(), instances_.end(),
            [](const std::shared_ptr<FloatingTextInstance>& inst) {
                return inst->elapsedTime >= inst->duration;
            }),
        instances_.end()
    );
}
