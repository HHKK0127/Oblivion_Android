#include "hud_layout.h"
#include <algorithm>
#include <cmath>

HUDLayout::HUDLayout()
    : screenWidth(1080)
    , screenHeight(1920)
    , uiScale(1.0f)
    , edgeMargin(20.0f)
    , barHeight(24.0f)
    , barWidth(250.0f)
    , quickSlotSize(64.0f)
    , minimapSize(180.0f)
    , compassHeight(32.0f) {

    // 全要素を初期化
    elements.resize(static_cast<int>(HUDElementType::ACTION_PROMPT) + 1);
    for (int i = 0; i <= static_cast<int>(HUDElementType::ACTION_PROMPT); ++i) {
        elements[i].type = static_cast<HUDElementType>(i);
        elements[i].visible = true;
        elements[i].scale = 1.0f;
    }
}

void HUDLayout::initialize(int w, int h) {
    screenWidth = w;
    screenHeight = h;

    // UIスケール計算（基準: 1080x1920）
    float baseWidth = 1080.0f;
    uiScale = static_cast<float>(screenWidth) / baseWidth;
    uiScale = std::max(0.5f, std::min(uiScale, 2.0f));

    // スケール適用
    edgeMargin = 20.0f * uiScale;
    barHeight = 24.0f * uiScale;
    barWidth = 250.0f * uiScale;
    quickSlotSize = 64.0f * uiScale;
    minimapSize = 180.0f * uiScale;
    compassHeight = 32.0f * uiScale;

    calculateDefaultLayout();

    HLAYOUT_LOGI("HUDLayout initialized: %dx%d scale=%.2f", w, h, uiScale);
}

void HUDLayout::recalculate(int w, int h) {
    initialize(w, h);
}

const HUDElementLayout& HUDLayout::getElementLayout(HUDElementType type) const {
    const auto* elem = findElement(type);
    if (elem) return *elem;

    // フォールバック: 空のレイアウトを返す
    static HUDElementLayout fallback;
    return fallback;
}

void HUDLayout::setElementPosition(HUDElementType type, const glm::vec2& position) {
    auto* elem = findElement(type);
    if (elem) elem->position = position;
}

void HUDLayout::setElementSize(HUDElementType type, const glm::vec2& size) {
    auto* elem = findElement(type);
    if (elem) elem->size = size;
}

void HUDLayout::setElementVisible(HUDElementType type, bool visible) {
    auto* elem = findElement(type);
    if (elem) elem->visible = visible;
}

void HUDLayout::setElementScale(HUDElementType type, float scale) {
    auto* elem = findElement(type);
    if (elem) {
        elem->scale = scale;
        // スケール変更時にデフォルトレイアウトを再計算
        calculateDefaultLayout();
    }
}

void HUDLayout::debugLogLayout() const {
    for (const auto& elem : elements) {
        HLAYOUT_LOGD("HUD[%d]: pos=(%.1f,%.1f) size=(%.1f,%.1f) vis=%d scale=%.2f",
                     static_cast<int>(elem.type),
                     elem.position.x, elem.position.y,
                     elem.size.x, elem.size.y,
                     elem.visible ? 1 : 0, elem.scale);
    }
}

// === 内部ヘルパー ===

HUDElementLayout* HUDLayout::findElement(HUDElementType type) {
    int idx = static_cast<int>(type);
    if (idx >= 0 && idx < static_cast<int>(elements.size())) {
        return &elements[idx];
    }
    return nullptr;
}

const HUDElementLayout* HUDLayout::findElement(HUDElementType type) const {
    int idx = static_cast<int>(type);
    if (idx >= 0 && idx < static_cast<int>(elements.size())) {
        return &elements[idx];
    }
    return nullptr;
}

void HUDLayout::calculateDefaultLayout() {
    float sw = static_cast<float>(screenWidth);
    float sh = static_cast<float>(screenHeight);
    float m = edgeMargin;

    // ヘルスバー（左上）
    auto* health = findElement(HUDElementType::HEALTH_BAR);
    if (health) {
        health->position = glm::vec2(m, m);
        health->size = glm::vec2(barWidth * health->scale, barHeight * health->scale);
    }

    // マナバー（右上）
    auto* mana = findElement(HUDElementType::MANA_BAR);
    if (mana) {
        mana->position = glm::vec2(sw - barWidth * mana->scale - m, m);
        mana->size = glm::vec2(barWidth * mana->scale, barHeight * mana->scale);
    }

    // スタミナバー（中央下）
    auto* stamina = findElement(HUDElementType::STAMINA_BAR);
    if (stamina) {
        float stW = barWidth * stamina->scale;
        stamina->position = glm::vec2((sw - stW) * 0.5f, sh - barHeight * stamina->scale - m - quickSlotSize - m);
        stamina->size = glm::vec2(stW, barHeight * stamina->scale);
    }

    // クイックスロット（右下）
    auto* quickSlots = findElement(HUDElementType::QUICK_SLOTS);
    if (quickSlots) {
        float qsSize = quickSlotSize * quickSlots->scale;
        float qsTotalW = qsSize * 5.0f; // 5スロット分
        quickSlots->position = glm::vec2(sw - qsTotalW - m, sh - qsSize - m);
        quickSlots->size = glm::vec2(qsTotalW, qsSize);
    }

    // コンパス（上部中央）
    auto* compass = findElement(HUDElementType::COMPASS);
    if (compass) {
        float cW = 300.0f * uiScale * compass->scale;
        float cH = compassHeight * compass->scale;
        compass->position = glm::vec2((sw - cW) * 0.5f, m);
        compass->size = glm::vec2(cW, cH);
    }

    // ミニマップ（左上、ヘルスバーの下）
    auto* minimap = findElement(HUDElementType::MINIMAP);
    if (minimap) {
        float mmSize = minimapSize * minimap->scale;
        minimap->position = glm::vec2(m, m + barHeight * uiScale + m);
        minimap->size = glm::vec2(mmSize, mmSize);
    }

    // ターゲット情報（中央上部、コンパスの下）
    auto* targetInfo = findElement(HUDElementType::TARGET_INFO);
    if (targetInfo) {
        float tiW = 300.0f * uiScale * targetInfo->scale;
        float tiH = 80.0f * uiScale * targetInfo->scale;
        targetInfo->position = glm::vec2((sw - tiW) * 0.5f, m + compassHeight * uiScale + m);
        targetInfo->size = glm::vec2(tiW, tiH);
    }

    // プレイヤーレベル（左上、ミニマップの下）
    auto* playerLevel = findElement(HUDElementType::PLAYER_LEVEL);
    if (playerLevel) {
        float mmSize = minimapSize * uiScale;
        playerLevel->position = glm::vec2(m, m + barHeight * uiScale + m + mmSize + m);
        playerLevel->size = glm::vec2(120.0f * uiScale * playerLevel->scale, 30.0f * uiScale * playerLevel->scale);
    }

    // アクティブエフェクト（右上、マナバーの下）
    auto* activeEffects = findElement(HUDElementType::ACTIVE_EFFECTS);
    if (activeEffects) {
        activeEffects->position = glm::vec2(sw - barWidth * uiScale - m, m + barHeight * uiScale + m);
        activeEffects->size = glm::vec2(barWidth * uiScale * activeEffects->scale, 100.0f * uiScale * activeEffects->scale);
    }

    // アクションプロンプト（中央下部、スタミナバーの上）
    auto* actionPrompt = findElement(HUDElementType::ACTION_PROMPT);
    if (actionPrompt) {
        float apW = 200.0f * uiScale * actionPrompt->scale;
        float apH = 40.0f * uiScale * actionPrompt->scale;
        float stH = barHeight * uiScale;
        actionPrompt->position = glm::vec2((sw - apW) * 0.5f,
                                            sh - stH - m - quickSlotSize - m - stH - m - apH);
        actionPrompt->size = glm::vec2(apW, apH);
    }
}
