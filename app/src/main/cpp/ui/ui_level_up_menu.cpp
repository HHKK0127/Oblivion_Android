#include "ui_level_up_menu.h"
#include "text_renderer.h"
#include "ui_draw_helper.h"
#include "../engine/texture_loader.h"

#include <algorithm>
#include <android/log.h>
#include <cstdio>

#undef LOG_TAG
#undef LOGD
#undef LOGI
#undef LOGE
#define LOG_TAG "UILevelUpMenu"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {

constexpr float kDimAlpha = 0.6f;
constexpr float kBorderSize = 44.0f;
constexpr float kRowAlphaGap = 128.0f;
constexpr float kRowMultiplier = 127.0f;
constexpr float kLabelScale = 0.7f;
constexpr float kTitleLabelScale = 1.0f;
constexpr float kDescLabelScale = 0.6f;

const glm::vec4 kTitleColor(51.0f / 255.0f, 79.0f / 255.0f, 142.0f / 255.0f, 1.0f);
const glm::vec4 kRowTextColor(117.0f / 255.0f, 59.0f / 255.0f, 33.0f / 255.0f, 1.0f);
const glm::vec4 kValueColor(33.0f / 255.0f, 59.0f / 255.0f, 117.0f / 255.0f, 1.0f);
const glm::vec4 kMultiplierColor(33.0f / 255.0f, 59.0f / 255.0f, 117.0f / 255.0f, 1.0f);
const glm::vec4 kDescColor(117.0f / 255.0f, 59.0f / 255.0f, 33.0f / 255.0f, 1.0f);
const glm::vec4 kFocusedRow(0.3f, 0.4f, 0.7f, 0.25f);

constexpr FontType kRowFont = FontType::KingthingsRegular;
constexpr FontType kTitleFont = FontType::TahomaBoldSmall;
constexpr FontType kDescFont = FontType::TahomaBoldSmall;

class FontScope {
public:
    FontScope(TextRenderer* renderer, FontType font)
        : renderer_(renderer),
          previous_(renderer ? renderer->getActiveFont() : FontType::KingthingsRegular) {
        if (renderer_ && previous_ != font) {
            renderer_->setActiveFont(font);
        }
    }
    ~FontScope() {
        if (renderer_) renderer_->setActiveFont(previous_);
    }
private:
    TextRenderer* renderer_;
    FontType previous_;
};

void drawIfLoaded(GLuint tex, float x, float y, float w, float h, int sw, int sh,
                  float alpha) {
    if (tex != 0 && alpha > 0.0f) {
        UIDrawHelper::drawTexturedQuad(x, y, w, h, tex, glm::vec4(1.0f, 1.0f, 1.0f, alpha),
                                       sw, sh);
    }
}

} // namespace

const char* UILevelUpMenu::kAttributeNames[8] = {
    "Strength",
    "Intelligence",
    "Willpower",
    "Agility",
    "Speed",
    "Endurance",
    "Personality",
    "Luck"
};

const char* UILevelUpMenu::kAttributeDescriptions[8] = {
    "Strength governs melee damage and carry weight.",
    "Intelligence governs magicka and spell effectiveness.",
    "Willpower governs magicka regeneration and fatigue.",
    "Agility governs sneaking, lockpicking, and accuracy.",
    "Speed governs movement speed and physical reflexes.",
    "Endurance governs health and physical resilience.",
    "Personality governs disposition and speechcraft.",
    "Luck governs all skills indirectly."
};

UILevelUpMenu::UILevelUpMenu()
    : scaleX_(1.0f), scaleY_(1.0f), originX_(0.0f), originY_(0.0f),
      screenWidth_(1920), screenHeight_(1080), visible_(false), fadeAlpha_(1.0f),
      focusIndex_(0), selectedAttribute_(-1), hasSelection_(false), pendingAttribute_(-1),
      playerLevel_(1), textRenderer_(nullptr),
      bgCenterTexture_(0), bgEdgeTopTexture_(0), bgEdgeBottomTexture_(0),
      bgEdgeLeftTexture_(0), bgEdgeRightTexture_(0),
      bgCornerTLTexture_(0), bgCornerTRTexture_(0),
      bgCornerBLTexture_(0), bgCornerBRTexture_(0),
      selectionFullTexture_(0) {
}

UILevelUpMenu::~UILevelUpMenu() {
    cleanup();
}

bool UILevelUpMenu::initialize(TextRenderer* renderer) {
    if (!renderer) {
        LOGE("TextRenderer is null");
        return false;
    }
    textRenderer_ = renderer;

    if (bgCenterTexture_ == 0) {
        const std::string dir = "textures/ui/menus/";
        bgCenterTexture_ = TextureLoader::loadTextureFromAsset(dir + "genericbackground__center_background.png");
        bgEdgeTopTexture_ = TextureLoader::loadTextureFromAsset(dir + "genericbackground__edge_top.png");
        bgEdgeBottomTexture_ = TextureLoader::loadTextureFromAsset(dir + "genericbackground__edge_bottom.png");
        bgEdgeLeftTexture_ = TextureLoader::loadTextureFromAsset(dir + "genericbackground__edge_left.png");
        bgEdgeRightTexture_ = TextureLoader::loadTextureFromAsset(dir + "genericbackground__edge_right.png");
        bgCornerTLTexture_ = TextureLoader::loadTextureFromAsset(dir + "genericbackground__top_left_corner.png");
        bgCornerTRTexture_ = TextureLoader::loadTextureFromAsset(dir + "genericbackground__top_right_corner.png");
        bgCornerBLTexture_ = TextureLoader::loadTextureFromAsset(dir + "genericbackground__bottom_left_corner.png");
        bgCornerBRTexture_ = TextureLoader::loadTextureFromAsset(dir + "genericbackground__bottom_right_corner.png");
        selectionFullTexture_ = TextureLoader::loadTextureFromAsset(dir + "dialog__dialog_selection_full.png");
    }

    LOGI("UILevelUpMenu initialized");
    return true;
}

void UILevelUpMenu::cleanup() {
    for (GLuint* tex : {&bgCenterTexture_, &bgEdgeTopTexture_, &bgEdgeBottomTexture_,
                        &bgEdgeLeftTexture_, &bgEdgeRightTexture_,
                        &bgCornerTLTexture_, &bgCornerTRTexture_,
                        &bgCornerBLTexture_, &bgCornerBRTexture_,
                        &selectionFullTexture_}) {
        if (*tex != 0) {
            TextureLoader::deleteTexture(*tex);
            *tex = 0;
        }
    }
    textRenderer_ = nullptr;
    visible_ = false;
    rows_.clear();
}

void UILevelUpMenu::setScreenSize(int width, int height) {
    if (width <= 0 || height <= 0) return;
    screenWidth_ = width;
    screenHeight_ = height;
    scaleX_ = static_cast<float>(width) / static_cast<float>(screenWidth_);
    scaleY_ = static_cast<float>(height) / static_cast<float>(screenHeight_);
}

void UILevelUpMenu::setFadeAlpha(float alpha) {
    fadeAlpha_ = std::clamp(alpha, 0.0f, 1.0f);
}

void UILevelUpMenu::open(const std::vector<int>& attributeChoices,
                         const std::vector<int>& skillBonusCounts,
                         int playerLevel) {
    rows_.clear();
    rebuildRows(attributeChoices, skillBonusCounts);
    playerLevel_ = playerLevel;
    focusIndex_ = 0;
    selectedAttribute_ = -1;
    hasSelection_ = false;
    pendingAttribute_ = -1;
    visible_ = true;
    LOGI("LevelUpMenu opened (level=%d, choices=%zu)", playerLevel, rows_.size());
}

void UILevelUpMenu::close() {
    visible_ = false;
    hasSelection_ = false;
    selectedAttribute_ = -1;
    pendingAttribute_ = -1;
    if (onCancel_) onCancel_();
    LOGI("LevelUpMenu closed");
}

void UILevelUpMenu::update(float deltaTime) {
    (void)deltaTime;
}

int UILevelUpMenu::consumeSelectedAttribute() {
    int attr = pendingAttribute_;
    pendingAttribute_ = -1;
    hasSelection_ = false;
    return attr;
}

bool UILevelUpMenu::hasSelection() const {
    return hasSelection_;
}

int UILevelUpMenu::calculateBonus(int skillIncreaseCount) {
    if (skillIncreaseCount <= 0) return 1;
    if (skillIncreaseCount <= 4) return 2;
    if (skillIncreaseCount <= 7) return 3;
    if (skillIncreaseCount <= 9) return 4;
    return 5;
}

UILevelUpMenu::Rect UILevelUpMenu::contentRect() const {
    const float cx = static_cast<float>(screenWidth_) * 0.5f;
    const float cy = static_cast<float>(screenHeight_) * 0.5f;
    return {cx - kContentW * 0.5f, cy - kContentH * 0.5f, kContentW, kContentH};
}

void UILevelUpMenu::rebuildRows(const std::vector<int>& choices,
                                const std::vector<int>& skillCounts) {
    rows_.clear();
    const size_t count = std::min<size_t>(choices.size(), kAttributeCount);
    for (size_t i = 0; i < count; ++i) {
        int attrId = choices[i];
        if (attrId < 0 || attrId > 7) attrId = 0;
        AttributeRow row;
        row.attributeId = attrId;
        row.name = kAttributeNames[attrId];
        row.description = kAttributeDescriptions[attrId];
        row.bonus = (static_cast<size_t>(i) < skillCounts.size())
                    ? calculateBonus(skillCounts[i]) : 1;
        rows_.push_back(row);
    }
}

int UILevelUpMenu::hitTestRow(float touchX, float touchY) const {
    if (!visible_) return -1;
    const Rect cr = contentRect();
    const float rowsY = cr.y + 60.0f; // below title area

    for (int i = 0; i < static_cast<int>(rows_.size()); ++i) {
        const float ry = rowsY + static_cast<float>(i) * kRowHeight;
        if (touchX >= cr.x + 20.0f && touchX <= cr.x + kContentW - 20.0f &&
            touchY >= ry && touchY < ry + kRowHeight) {
            return i;
        }
    }
    return -1;
}

int UILevelUpMenu::hitTestExitButton(float touchX, float touchY) const {
    const Rect cr = contentRect();
    const float bx = cr.x + kExitBtnX;
    const float by = cr.y + kExitBtnY;
    if (touchX >= bx && touchX <= bx + kExitBtnW &&
        touchY >= by && touchY <= by + kExitBtnH) {
        return 0;
    }
    return -1;
}

void UILevelUpMenu::onTouchEvent(float x, float y) {
    if (!visible_) return;

    if (hitTestExitButton(x, y) >= 0) {
        handleBack();
        return;
    }

    int row = hitTestRow(x, y);
    if (row >= 0 && row < static_cast<int>(rows_.size())) {
        if (row == selectedAttribute_) {
            // Tap same row again to confirm
            pendingAttribute_ = rows_[row].attributeId;
            hasSelection_ = true;
            if (onLevelUp_) onLevelUp_();
        } else {
            selectedAttribute_ = row;
            focusIndex_ = row;
            hasSelection_ = false;
        }
    }
}

bool UILevelUpMenu::onKeyPress(int keyCode) {
    if (!visible_) return false;

    switch (keyCode) {
        case 19: // DPAD_UP
            if (focusIndex_ > 0) {
                focusIndex_--;
                selectedAttribute_ = focusIndex_;
                hasSelection_ = false;
            }
            return true;
        case 20: // DPAD_DOWN
            if (focusIndex_ < static_cast<int>(rows_.size()) - 1) {
                focusIndex_++;
                selectedAttribute_ = focusIndex_;
                hasSelection_ = false;
            }
            return true;
        case 23: // DPAD_CENTER
        case 66: // ENTER
            if (selectedAttribute_ >= 0 && selectedAttribute_ < static_cast<int>(rows_.size())) {
                pendingAttribute_ = rows_[selectedAttribute_].attributeId;
                hasSelection_ = true;
                if (onLevelUp_) onLevelUp_();
            }
            return true;
        case 4: // BACK
            return handleBack();
        default:
            return false;
    }
}

bool UILevelUpMenu::handleBack() {
    if (!visible_) return false;
    close();
    return true;
}

void UILevelUpMenu::render() {
    if (!visible_ || !textRenderer_) return;

    GLboolean depthWasEnabled = GL_TRUE;
    glGetBooleanv(GL_DEPTH_TEST, &depthWasEnabled);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    drawBackground();
    drawTitle(playerLevel_);
    drawRows();
    drawPicture();
    drawAttributeDescription();
    drawExitButton();

    if (depthWasEnabled) glEnable(GL_DEPTH_TEST);
    else glDisable(GL_DEPTH_TEST);
}

void UILevelUpMenu::drawBackground() {
    const float alpha = fadeAlpha_;

    UIDrawHelper::drawColoredQuad(0.0f, 0.0f,
                                  static_cast<float>(screenWidth_),
                                  static_cast<float>(screenHeight_),
                                  glm::vec4(0.0f, 0.0f, 0.0f, kDimAlpha * alpha),
                                  screenWidth_, screenHeight_);

    const Rect cr = contentRect();
    const GLuint slices[9] = {bgCenterTexture_, bgEdgeTopTexture_, bgEdgeBottomTexture_,
                              bgEdgeLeftTexture_, bgEdgeRightTexture_,
                              bgCornerTLTexture_, bgCornerTRTexture_,
                              bgCornerBLTexture_, bgCornerBRTexture_};
    UIDrawHelper::drawGenericBackground(slices, cr.x - kBorderSize, cr.y - kBorderSize,
                                        kContentW + kBorderSize * 2.0f,
                                        kContentH + kBorderSize * 2.0f,
                                        screenWidth_, screenHeight_, alpha);
}

void UILevelUpMenu::drawTitle(int playerLevel) {
    const Rect cr = contentRect();
    const float alpha = fadeAlpha_;

    FontScope font(textRenderer_, kTitleFont);
    glm::vec4 color = kTitleColor;
    color.w *= alpha;

    char buf[64];
    std::snprintf(buf, sizeof(buf), "You have ascended to level %d", playerLevel);
    const float tw = textRenderer_->getTextWidth(buf, kTitleLabelScale);
    const float tx = cr.x + (kContentW - tw) * 0.5f;
    textRenderer_->renderText(buf, tx, cr.y + kTitleY, color, kTitleLabelScale);
}

void UILevelUpMenu::drawPicture() {
    const Rect cr = contentRect();
    const float alpha = fadeAlpha_;
    const float px = cr.x + kPictureX;

    // Placeholder: draw a colored square where the attribute icon would go
    UIDrawHelper::drawColoredQuad(px, cr.y + 60.0f,
                                  kPictureSize, kPictureSize,
                                  glm::vec4(0.1f, 0.1f, 0.2f, 0.4f * alpha),
                                  screenWidth_, screenHeight_);
    UIDrawHelper::drawBorder(px, cr.y + 60.0f,
                             kPictureSize, kPictureSize, 2.0f,
                             glm::vec4(0.3f, 0.4f, 0.7f, 0.5f * alpha),
                             screenWidth_, screenHeight_);

    // Show attribute name in the picture area for the selected row
    if (selectedAttribute_ >= 0 && selectedAttribute_ < static_cast<int>(rows_.size())) {
        const AttributeRow& row = rows_[selectedAttribute_];
        FontScope font(textRenderer_, kRowFont);
        glm::vec4 textColor = kRowTextColor;
        textColor.w *= alpha;
        const float tw = textRenderer_->getTextWidth(row.name, kLabelScale);
        textRenderer_->renderText(row.name,
                                  px + (kPictureSize - tw) * 0.5f,
                                  cr.y + 60.0f + kPictureSize * 0.5f - textRenderer_->getTextHeight(kLabelScale) * 0.5f,
                                  textColor, kLabelScale);
    }
}

void UILevelUpMenu::drawAttributeDescription() {
    if (selectedAttribute_ < 0 || selectedAttribute_ >= static_cast<int>(rows_.size())) return;

    const Rect cr = contentRect();
    const float alpha = fadeAlpha_;
    const AttributeRow& row = rows_[selectedAttribute_];

    FontScope font(textRenderer_, kDescFont);
    glm::vec4 color = kDescColor;
    color.w *= alpha;

    const float dx = cr.x + kPictureX;
    const float dy = cr.y + 60.0f + kPictureSize + kDescY;
    textRenderer_->renderText(row.description, dx, dy, color, kDescLabelScale);
}

void UILevelUpMenu::drawRows() {
    const Rect cr = contentRect();
    const float alpha = fadeAlpha_;

    const float rowsTop = cr.y + 60.0f;

    for (int i = 0; i < static_cast<int>(rows_.size()); ++i) {
        const float ry = rowsTop + static_cast<float>(i) * kRowHeight;
        const float rowX = cr.x + 20.0f;
        const float rowW = kContentW - 40.0f;

        const bool focused = (i == selectedAttribute_);

        // Row background
        if (focused) {
            glm::vec4 bgColor = kFocusedRow;
            bgColor.w *= alpha;
            UIDrawHelper::drawColoredQuad(rowX, ry, rowW, kRowHeight, bgColor,
                                          screenWidth_, screenHeight_);
            drawSelectionFrame(rowX, ry, rowW, kRowHeight);
        }

        // Row alpha: _enabled_alpha = (user6 OR user0-1)*127+128
        // For non-selected: user0/user1/user6 = 0 → alpha = 128
        // For selected: alpha = 1.0 * fadeAlpha
        float rowAlpha = focused ? alpha : (kRowAlphaGap / 255.0f);

        // Coin icon placeholder
        UIDrawHelper::drawColoredQuad(cr.x + kCoinIconX, ry + 2.0f,
                                      kCoinIconSize, kCoinIconSize,
                                      glm::vec4(0.6f, 0.4f, 0.1f, 0.5f * rowAlpha),
                                      screenWidth_, screenHeight_);

        const AttributeRow& row = rows_[i];

        // Attribute name (wrap width 300)
        {
            FontScope font(textRenderer_, kRowFont);
            glm::vec4 textColor = kRowTextColor;
            textColor.w *= rowAlpha;
            textRenderer_->renderText(row.name, cr.x + kTextX, ry + 8.0f,
                                      textColor, kLabelScale);
        }

        // Bonus value
        {
            FontScope font(textRenderer_, kRowFont);
            char buf[16];
            std::snprintf(buf, sizeof(buf), "+%d", row.bonus);
            glm::vec4 valColor = kValueColor;
            valColor.w *= rowAlpha;
            textRenderer_->renderText(buf, cr.x + kValueX, ry + 8.0f,
                                      valColor, kLabelScale);
        }

        // Multiplier label "x5"
        {
            FontScope font(textRenderer_, kRowFont);
            char buf[16];
            std::snprintf(buf, sizeof(buf), "x%d", row.bonus);
            glm::vec4 multColor = kMultiplierColor;
            multColor.w *= rowAlpha;
            textRenderer_->renderText(buf, cr.x + kMultiplierX, ry + 8.0f,
                                      multColor, kLabelScale);
        }
    }
}

void UILevelUpMenu::drawExitButton() {
    const Rect cr = contentRect();
    const float alpha = fadeAlpha_;

    const float bx = cr.x + kExitBtnX;
    const float by = cr.y + kExitBtnY;

    UIDrawHelper::drawColoredQuad(bx, by, kExitBtnW, kExitBtnH,
                                  glm::vec4(0.2f, 0.05f, 0.05f, 0.6f * alpha),
                                  screenWidth_, screenHeight_);

    FontScope font(textRenderer_, kRowFont);
    glm::vec4 color(1.0f, 1.0f, 1.0f, alpha);
    textRenderer_->renderText("X", bx + 8.0f, by + 4.0f, color, kLabelScale);
}

void UILevelUpMenu::drawSelectionFrame(float x, float y, float w, float h) {
    drawIfLoaded(selectionFullTexture_, x, y, w, h,
                 screenWidth_, screenHeight_, fadeAlpha_);
}