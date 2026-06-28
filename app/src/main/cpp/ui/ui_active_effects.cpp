#include "ui_active_effects.h"
#include "placeholder_assets.h"
#include "ui_draw_helper.h"
#include <GLES3/gl3.h>
#include <cmath>
#include <algorithm>
#include <iomanip>
#include <sstream>

UIActiveEffects::UIActiveEffects() = default;

bool UIActiveEffects::initialize(TextRenderer* textRenderer, int screenW, int screenH) {
    if (!textRenderer) return false;
    textRenderer = textRenderer;
    screenWidth = screenW;
    screenHeight = screenH;
    return true;
}

void UIActiveEffects::addEffect(int effectId, const std::string& name, EffectType type,
                                 float duration, glm::vec3 color, const std::string& iconChar) {
    // Check if effect already exists
    if (findEffect(effectId) != nullptr) {
        return;  // Effect already active
    }

    if (effects_.size() >= MAX_VISIBLE_EFFECTS) {
        return;  // Max effects reached
    }

    auto effect = std::make_shared<ActiveEffect>();
    effect->effectId = effectId;
    effect->name = name;
    effect->type = type;
    effect->duration = duration;
    effect->maxDuration = duration;
    effect->color = color;
    effect->iconChar = iconChar;

    effects_.push_back(effect);
}

void UIActiveEffects::removeEffect(int effectId) {
    effects_.erase(
        std::remove_if(effects_.begin(), effects_.end(),
            [effectId](const std::shared_ptr<ActiveEffect>& effect) {
                return effect->effectId == effectId;
            }),
        effects_.end()
    );
}

void UIActiveEffects::updateEffectDuration(int effectId, float newDuration) {
    auto effect = const_cast<ActiveEffect*>(findEffect(effectId));
    if (effect) {
        effect->duration = newDuration;
    }
}

void UIActiveEffects::clearAllEffects() {
    effects_.clear();
}

const UIActiveEffects::ActiveEffect* UIActiveEffects::getEffect(int index) const {
    if (index >= 0 && index < static_cast<int>(effects_.size())) {
        return effects_[index].get();
    }
    return nullptr;
}

const UIActiveEffects::ActiveEffect* UIActiveEffects::findEffect(int effectId) const {
    for (const auto& effect : effects_) {
        if (effect->effectId == effectId) {
            return effect.get();
        }
    }
    return nullptr;
}

void UIActiveEffects::update(float deltaTime) {
    for (auto& effect : effects_) {
        effect->duration -= deltaTime;
    }
    removeExpiredEffects();
}

void UIActiveEffects::render() {
    if (effects_.empty() || !textRenderer) return;

    GLboolean depthTestEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    renderEffectIcons();
    renderDurationBars();
    renderEffectTooltip();

    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
}

void UIActiveEffects::renderEffectIcons() {
    float x = START_X;
    float y = START_Y;

    for (size_t i = 0; i < effects_.size(); i++) {
        const auto& effect = effects_[i];

        // Calculate icon position (grid layout)
        float iconX = x + (i % 4) * (ICON_SIZE + ICON_GAP);
        float iconY = y + (i / 4) * (ICON_SIZE + ICON_GAP);

        // Icon background
        glm::vec4 bgColor(effect->color.x * 0.4f, effect->color.y * 0.4f, effect->color.z * 0.4f, 0.8f);
        UIDrawHelper::drawColoredQuad(iconX, iconY, ICON_SIZE, ICON_SIZE,
            bgColor, screenWidth, screenHeight);

        // Icon border
        glm::vec4 borderColor(effect->color.x, effect->color.y, effect->color.z, 1.0f);
        UIDrawHelper::drawBorder(iconX, iconY, ICON_SIZE, ICON_SIZE, 2.0f,
            borderColor, screenWidth, screenHeight);

        // Effect icon character
        textRenderer->renderText(effect->iconChar,
            iconX + ICON_SIZE / 2.0f - 4.0f, iconY + 10.0f,
            effect->color, 0.9f);

        // Duration indicator (if applicable)
        if (effect->maxDuration > 0.0f) {
            float durationPercent = effect->duration / effect->maxDuration;
            durationPercent = std::max(0.0f, std::min(1.0f, durationPercent));

            // Duration bar at bottom of icon
            float barHeight = 3.0f;
            float barWidth = ICON_SIZE - 4.0f;
            float barX = iconX + 2.0f;
            float barY = iconY + ICON_SIZE - 5.0f;

            // Background bar
            glm::vec4 barBgColor(0.2f, 0.2f, 0.2f, 0.7f);
            UIDrawHelper::drawColoredQuad(barX, barY, barWidth, barHeight,
                barBgColor, screenWidth, screenHeight);

            // Duration bar
            float fillWidth = barWidth * durationPercent;
            glm::vec4 barFillColor(effect->color.x, effect->color.y, effect->color.z, 0.8f);
            UIDrawHelper::drawColoredQuad(barX, barY, fillWidth, barHeight,
                barFillColor, screenWidth, screenHeight);
        }
    }
}

void UIActiveEffects::renderDurationBars() {
    if (effects_.size() <= 1) return;

    float panelX = START_X;
    float panelY = START_Y + (((effects_.size() - 1) / 4 + 1) * (ICON_SIZE + ICON_GAP)) + 10.0f;

    for (size_t i = 0; i < std::min(effects_.size(), static_cast<size_t>(3)); i++) {
        const auto& effect = effects_[i];
        float barY = panelY + i * 15.0f;

        if (effect->maxDuration <= 0.0f) continue;

        // Effect name
        textRenderer->renderText(effect->name,
            panelX, barY,
            effect->color, 0.6f);

        // Duration time
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << effect->duration << "s";
        std::string durationText = ss.str();

        textRenderer->renderText(durationText,
            panelX + 120.0f, barY,
            PlaceholderAssets::Colors::PARCHMENT_LIGHT, 0.5f);
    }
}

void UIActiveEffects::renderEffectTooltip() {
    // Tooltip for first (primary) effect if available
    if (effects_.empty() || !textRenderer) return;

    const auto& primaryEffect = effects_[0];
    float tooltipX = START_X + ICON_SIZE + 20.0f;
    float tooltipY = START_Y;

    // Effect type label
    std::string typeLabel = getEffectTypeLabel(primaryEffect->type);
    textRenderer->renderText(typeLabel,
        tooltipX, tooltipY,
        primaryEffect->color, 0.7f);

    // Effect name
    textRenderer->renderText(primaryEffect->name,
        tooltipX, tooltipY + 15.0f,
        PlaceholderAssets::Colors::PARCHMENT_LIGHT, 0.6f);

    // Remaining duration
    if (primaryEffect->maxDuration > 0.0f) {
        std::stringstream ss;
        ss << "Duration: " << std::fixed << std::setprecision(1)
           << primaryEffect->duration << "s";
        std::string durationStr = ss.str();

        textRenderer->renderText(durationStr,
            tooltipX, tooltipY + 28.0f,
            glm::vec3(0.8f, 0.8f, 0.8f), 0.5f);
    }
}

glm::vec3 UIActiveEffects::getEffectColor(EffectType type) const {
    switch (type) {
    case EFFECT_BUFF:
    case EFFECT_ENHANCE:
        return glm::vec3(0.2f, 0.8f, 0.2f);  // Green
    case EFFECT_DEBUFF:
    case EFFECT_DISEASE:
        return glm::vec3(1.0f, 0.4f, 0.2f);  // Orange
    case EFFECT_POISON:
        return glm::vec3(0.4f, 0.8f, 0.2f);  // Yellow-green
    case EFFECT_CURSE:
        return glm::vec3(0.6f, 0.2f, 0.6f);  // Purple
    case EFFECT_SPELL:
        return glm::vec3(0.3f, 0.6f, 1.0f);  // Blue
    default:
        return glm::vec3(0.8f, 0.8f, 0.8f);  // Gray
    }
}

std::string UIActiveEffects::getEffectTypeLabel(EffectType type) const {
    switch (type) {
    case EFFECT_BUFF:
        return "BUFF";
    case EFFECT_DEBUFF:
        return "DEBUFF";
    case EFFECT_DISEASE:
        return "DISEASE";
    case EFFECT_POISON:
        return "POISON";
    case EFFECT_CURSE:
        return "CURSE";
    case EFFECT_ENHANCE:
        return "ENHANCE";
    case EFFECT_SPELL:
        return "SPELL";
    default:
        return "EFFECT";
    }
}

void UIActiveEffects::removeExpiredEffects() {
    effects_.erase(
        std::remove_if(effects_.begin(), effects_.end(),
            [](const std::shared_ptr<ActiveEffect>& effect) {
                return effect->duration <= 0.0f;
            }),
        effects_.end()
    );
}
