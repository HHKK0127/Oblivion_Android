#include "ui_quick_slot_bar.h"
#include "placeholder_assets.h"
#include "ui_draw_helper.h"
#include <GLES3/gl3.h>
#include <cmath>
#include <algorithm>

UIQuickSlotBar::UIQuickSlotBar() = default;

bool UIQuickSlotBar::initialize(TextRenderer* textRenderer, int screenW, int screenH) {
    if (!textRenderer) return false;
    textRenderer = textRenderer;
    screenWidth = screenW;
    screenHeight = screenH;
    return true;
}

void UIQuickSlotBar::setSlot(int slotIndex, const SlotData& data) {
    if (slotIndex >= 0 && slotIndex < SLOT_COUNT) {
        slots_[slotIndex] = data;
    }
}

void UIQuickSlotBar::clearSlot(int slotIndex) {
    if (slotIndex >= 0 && slotIndex < SLOT_COUNT) {
        slots_[slotIndex] = SlotData();
    }
}

const UIQuickSlotBar::SlotData& UIQuickSlotBar::getSlot(int slotIndex) const {
    static SlotData emptySlot;
    if (slotIndex >= 0 && slotIndex < SLOT_COUNT) {
        return slots_[slotIndex];
    }
    return emptySlot;
}

void UIQuickSlotBar::selectSlot(int slotIndex) {
    if (slotIndex >= 0 && slotIndex < SLOT_COUNT) {
        selectedSlotIndex_ = slotIndex;
        if (onSlotSelected) {
            onSlotSelected(slotIndex);
        }
    }
}

void UIQuickSlotBar::update(float deltaTime) {
    // Animation updates can be added here if needed
}

bool UIQuickSlotBar::onTouchDown(float x, float y, int pointerId) {
    int slotIndex = hitTestSlot(x, y);
    if (slotIndex >= 0) {
        selectSlot(slotIndex);
        return true;
    }
    return false;
}

void UIQuickSlotBar::render() {
    if (!textRenderer) return;

    GLboolean depthTestEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float barStartX = getBarStartX();
    float barY = getBarY();

    for (int i = 0; i < SLOT_COUNT; i++) {
        float slotX = barStartX + i * (SLOT_SIZE + SLOT_GAP);
        renderSlot(i, slotX, barY);
    }

    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
}

void UIQuickSlotBar::renderSlot(int slotIndex, float x, float y) {
    const SlotData& slot = slots_[slotIndex];

    // Slot background
    glm::vec4 bgColor(0.2f, 0.15f, 0.08f, 0.9f);
    if (slotIndex == selectedSlotIndex_) {
        bgColor = glm::vec4(0.7f, 0.6f, 0.2f, 0.9f);  // Highlighted
    }
    UIDrawHelper::drawColoredQuad(x, y, SLOT_SIZE, SLOT_SIZE, bgColor,
        screenWidth, screenHeight);

    // Slot border
    glm::vec4 borderColor(0.5f, 0.4f, 0.2f, 1.0f);
    if (slotIndex == selectedSlotIndex_) {
        borderColor = glm::vec4(1.0f, 0.9f, 0.4f, 1.0f);
    }
    UIDrawHelper::drawBorder(x, y, SLOT_SIZE, SLOT_SIZE, 2.0f, borderColor,
        screenWidth, screenHeight);

    if (slot.type != EMPTY) {
        renderSlotIcon(slot, x, y);
        if (slot.quantity > 0) {
            renderSlotQuantity(slot, x, y);
        }
    }
}

void UIQuickSlotBar::renderSlotIcon(const SlotData& slot, float x, float y) {
    if (!textRenderer) return;

    glm::vec3 iconColor = glm::vec3(1.0f, 1.0f, 1.0f);
    switch (slot.type) {
    case SPELL:
        iconColor = glm::vec3(0.4f, 0.6f, 1.0f);  // Blue
        break;
    case ITEM:
        iconColor = glm::vec3(0.9f, 0.8f, 0.3f);  // Gold
        break;
    case ABILITY:
        iconColor = glm::vec3(0.8f, 0.4f, 0.8f);  // Purple
        break;
    default:
        break;
    }

    // Display first character of item ID as placeholder
    std::string iconText = slot.itemId.empty() ? "?" : slot.itemId.substr(0, 1);
    textRenderer->renderText(iconText,
        x + SLOT_SIZE / 2.0f - 4.0f, y + SLOT_SIZE / 2.0f - 6.0f,
        iconColor, 0.8f);
}

void UIQuickSlotBar::renderSlotQuantity(const SlotData& slot, float x, float y) {
    if (!textRenderer || slot.quantity <= 0) return;

    std::string qtyText = std::to_string(slot.quantity);
    glm::vec3 qtyColor(0.9f, 0.9f, 0.2f);  // Yellow
    textRenderer->renderText(qtyText,
        x + SLOT_SIZE - 12.0f, y + SLOT_SIZE - 10.0f,
        qtyColor, 0.6f);
}

int UIQuickSlotBar::hitTestSlot(float x, float y) const {
    float barStartX = getBarStartX();
    float barY = getBarY();

    for (int i = 0; i < SLOT_COUNT; i++) {
        float slotX = barStartX + i * (SLOT_SIZE + SLOT_GAP);
        if (x >= slotX && x < slotX + SLOT_SIZE &&
            y >= barY && y < barY + SLOT_SIZE) {
            return i;
        }
    }
    return -1;
}

float UIQuickSlotBar::getBarStartX() const {
    float barWidth = SLOT_COUNT * SLOT_SIZE + (SLOT_COUNT - 1) * SLOT_GAP;
    return (screenWidth - barWidth) / 2.0f;
}

float UIQuickSlotBar::getBarY() const {
    return screenHeight - SLOT_SIZE - BAR_PADDING;
}
