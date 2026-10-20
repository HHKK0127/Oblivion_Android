#include "ui_quick_keys_menu.h"

#include "../engine/texture_loader.h"
#include "text_renderer.h"
#include "ui_draw_helper.h"

#include <android/log.h>
#include <cmath>

#define LOG_TAG "QuickKeysMenu"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

UIQuickKeysMenu::UIQuickKeysMenu() {
    for (auto& slot : slots_) {
        slot = SlotEntry{};
    }
}

UIQuickKeysMenu::~UIQuickKeysMenu() {
    cleanup();
}

bool UIQuickKeysMenu::initialize(TextRenderer* renderer) {
    textRenderer_ = renderer;
    paperTexture_ = TextureLoader::loadTextureFromAsset(
        "textures/ui/quickkeys__menu_paper.png");
    overlayTexture_ = TextureLoader::loadTextureFromAsset(
        "textures/ui/quickkeys__menu_overlay.png");
    if (paperTexture_ == 0) {
        LOGI("paper texture not found, continuing without it");
    }
    if (overlayTexture_ == 0) {
        LOGI("overlay texture not found, continuing without it");
    }
    return true;
}

void UIQuickKeysMenu::cleanup() {
    if (paperTexture_) {
        glDeleteTextures(1, &paperTexture_);
        paperTexture_ = 0;
    }
    if (overlayTexture_) {
        glDeleteTextures(1, &overlayTexture_);
        overlayTexture_ = 0;
    }
}

void UIQuickKeysMenu::open() {
    visible_ = true;
    selectedSlot_ = -1;
    hasSelection_ = false;
    pendingSlot_ = -1;
}

void UIQuickKeysMenu::close() {
    visible_ = false;
    selectedSlot_ = -1;
    hasSelection_ = false;
    pendingSlot_ = -1;
}

void UIQuickKeysMenu::setScreenSize(int width, int height) {
    screenWidth_ = width;
    screenHeight_ = height;
}

void UIQuickKeysMenu::setFadeAlpha(float alpha) {
    fadeAlpha_ = alpha;
}

void UIQuickKeysMenu::update(float deltaTime) {
    // no-op for now
}

void UIQuickKeysMenu::render() {
    if (!visible_) {
        return;
    }

    drawBackground();
    drawPaper();
    drawSlots();
}

void UIQuickKeysMenu::drawBackground() {
    glm::vec4 overlayColor(0.0f, 0.0f, 0.0f, 0.6f * fadeAlpha_);
    UIDrawHelper::drawColoredQuad(0.0f, 0.0f,
                        static_cast<float>(screenWidth_),
                        static_cast<float>(screenHeight_),
                        overlayColor,
                        screenWidth_, screenHeight_);
}

void UIQuickKeysMenu::drawPaper() {
    float paperX = (static_cast<float>(screenWidth_) - kPaperSize) * 0.5f;
    float paperY = (static_cast<float>(screenHeight_) - kPaperSize) * 0.5f;

    if (paperTexture_ != 0) {
        glm::vec4 tint(1.0f, 1.0f, 1.0f, fadeAlpha_);
        UIDrawHelper::drawTexturedQuad(paperX, paperY,
                                      kPaperSize, kPaperSize,
                                      paperTexture_, tint,
                                      screenWidth_, screenHeight_);
    }
}

void UIQuickKeysMenu::drawSlots() {
    float paperX = (static_cast<float>(screenWidth_) - kPaperSize) * 0.5f;
    float paperY = (static_cast<float>(screenHeight_) - kPaperSize) * 0.5f;
    float scale = kPaperSize / kCanvasWidth;

    for (int i = 0; i < kSlotCount; ++i) {
        if (!slots_[i].occupied) {
            continue;
        }

        float sx = paperX + kSlotPositions[i].x * scale - kSlotIconSize * scale * 0.5f;
        float sy = paperY + kSlotPositions[i].y * scale - kSlotIconSize * scale * 0.5f;
        float iconSize = kSlotIconSize * scale;

        glm::vec4 tint(1.0f, 1.0f, 1.0f, fadeAlpha_);

        if (i == selectedSlot_) {
            tint = glm::vec4(1.0f, 0.8f, 0.0f, 1.0f) * fadeAlpha_;
        }

        // Attempt to load and draw the slot's icon texture
        GLuint iconTex = 0;
        if (!slots_[i].texturePath.empty()) {
            iconTex = TextureLoader::loadTextureFromAsset("textures/ui/" + slots_[i].texturePath);
        }

        if (iconTex != 0) {
            UIDrawHelper::drawTexturedQuad(sx, sy, iconSize, iconSize,
                                          iconTex, tint,
                                          screenWidth_, screenHeight_);
        } else {
            // Fallback: colored quad with item id label
            glm::vec4 slotColor(0.3f, 0.3f, 0.5f, 0.8f * fadeAlpha_);
            UIDrawHelper::drawColoredQuad(sx, sy, iconSize, iconSize,
                                         slotColor,
                                         screenWidth_, screenHeight_);
        }

        if (iconTex != 0) {
            glDeleteTextures(1, &iconTex);
        }

        // Draw slot label below the icon
        if (textRenderer_ && !slots_[i].name.empty()) {
            float labelScale = scale * 0.5f;
            float textWidth = textRenderer_->getTextWidth(slots_[i].name, labelScale);
            float tx = sx + (iconSize - textWidth) * 0.5f;
            float ty = sy + iconSize + 4.0f * scale;
            glm::vec4 textColor(1.0f, 1.0f, 1.0f, 0.9f * fadeAlpha_);
            textRenderer_->renderText(slots_[i].name, tx, ty, textColor, labelScale);
        }
    }
}

int UIQuickKeysMenu::slotFromTouch(float x, float y) const {
    float centerX = static_cast<float>(screenWidth_) * 0.5f;
    float centerY = static_cast<float>(screenHeight_) * 0.5f;

    float dx = x - centerX;
    float dy = y - centerY;

    if (std::abs(dx) < 30.0f && std::abs(dy) < 30.0f) {
        return -1;
    }

    float angle = std::atan2(dy, dx);
    if (angle < 0.0f) {
        angle += 2.0f * static_cast<float>(M_PI);
    }

    // 8 sectors of 45 degrees each
    // Slot 0 = top (angle ~pi/2), going clockwise
    float sectorAngle = 2.0f * static_cast<float>(M_PI) / 8.0f;
    float offset = angle + sectorAngle * 0.5f;
    if (offset >= 2.0f * static_cast<float>(M_PI)) {
        offset -= 2.0f * static_cast<float>(M_PI);
    }
    int slot = static_cast<int>(offset / sectorAngle);
    return slot % 8;
}

int UIQuickKeysMenu::slotFromKey(int keyCode) const {
    // Map directional keys to radial slots
    switch (keyCode) {
        case 19: return 0;  // up    -> slot 1 (top)
        case 20: return 4;  // down  -> slot 5 (bottom)
        case 21: return 6;  // left  -> slot 7
        case 22: return 2;  // right -> slot 3
        default: return -1;
    }
}

void UIQuickKeysMenu::onTouchEvent(float x, float y) {
    if (!visible_) return;

    int slot = slotFromTouch(x, y);
    if (slot < 0 || slot >= kSlotCount) {
        return;
    }
    if (!slots_[slot].occupied) {
        return;
    }

    if (selectedSlot_ == slot) {
        // Tap again on the same slot confirms selection
        pendingSlot_ = slot;
        hasSelection_ = true;
        if (onSlotSelected_) {
            onSlotSelected_(slot);
        }
    } else {
        selectedSlot_ = slot;
    }
}

bool UIQuickKeysMenu::onKeyPress(int keyCode) {
    if (!visible_) return false;

    int slot = slotFromKey(keyCode);
    if (slot >= 0 && slot < kSlotCount) {
        if (slots_[slot].occupied) {
            selectedSlot_ = slot;
            return true;
        }
        return false;
    }

    // Enter / center key confirms current selection
    if (keyCode == 66 || keyCode == 23) {
        if (selectedSlot_ >= 0) {
            pendingSlot_ = selectedSlot_;
            hasSelection_ = true;
            if (onSlotSelected_) {
                onSlotSelected_(selectedSlot_);
            }
            return true;
        }
        return false;
    }

    return false;
}

bool UIQuickKeysMenu::handleBack() {
    if (!visible_) return false;

    close();
    if (onCancel_) {
        onCancel_(-1);
    }
    return true;
}

void UIQuickKeysMenu::setSlot(int index, const SlotEntry& entry) {
    if (index < 0 || index >= kSlotCount) return;
    slots_[index] = entry;
}

const UIQuickKeysMenu::SlotEntry& UIQuickKeysMenu::getSlot(int index) const {
    static const SlotEntry kEmptySlot;
    if (index < 0 || index >= kSlotCount) return kEmptySlot;
    return slots_[index];
}

int UIQuickKeysMenu::consumeSelectedSlot() {
    int slot = pendingSlot_;
    pendingSlot_ = -1;
    hasSelection_ = false;
    return slot;
}

bool UIQuickKeysMenu::hasSelection() const {
    return hasSelection_;
}