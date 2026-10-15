#include "options_ui.h"
#include "ui_draw_helper.h"
#include "../engine/texture_loader.h"
#include <algorithm>
#include <android/log.h>

#undef LOG_TAG
#undef LOGD
#undef LOGI
#undef LOGE
#define LOG_TAG "OptionsUI"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {
// Values from options_menu (OptionsMenu) in the original XML, PC variant.
constexpr float BODY_WIDTH = 240.0f;
constexpr float BODY_HEIGHT = 360.0f;
constexpr float PAUSE_EXTRA_HEIGHT = 40.0f;   // body becomes 400 when opened from pause
constexpr float BUTTON_HEIGHT = 64.0f;
constexpr float BUTTON_LABEL_PAD = 40.0f;     // rect width = label width + 40
constexpr float SELECTION_MIN_WIDTH = 104.0f; // dialog_selection_cut width
constexpr float SELECTION_CAP_GAP = 8.0f;
constexpr float SELECTION_OFFSET_Y = 3.0f;
constexpr float PAUSE_TEXT_Y = 20.0f;
constexpr float PAUSE_DECOR_Y = 26.0f;
constexpr float PAUSE_DECOR_INSET = 30.0f;
constexpr float PAUSE_DECOR_WIDTH = 35.0f;
constexpr float PAUSE_DECOR_HEIGHT = 17.0f;
constexpr float LABEL_SCALE = 1.0f;
constexpr float DIM_ALPHA = 0.6f;

// Font indices come from the XML <font> value (1-based) mapped to FontType.
// XML font 1 = Kingthings_Regular, font 3 = Tahoma_Bold_Small (Oblivion_default.ini [Fonts]).
constexpr FontType BUTTON_FONT = FontType::KingthingsRegular;
constexpr FontType PAUSE_FONT = FontType::TahomaBoldSmall;

const glm::vec4 kFocusedText(117.0f / 255.0f, 59.0f / 255.0f, 33.0f / 255.0f, 1.0f);
const glm::vec4 kIdleText(128.0f / 255.0f, 128.0f / 255.0f, 128.0f / 255.0f, 196.0f / 255.0f);
const glm::vec4 kPauseText(51.0f / 255.0f, 79.0f / 255.0f, 142.0f / 255.0f, 1.0f);

// Button order follows the XML listindex. Downloads is Xbox-only and omitted on PC.
const char* const kButtonLabels[] = {"Return", "Gameplay", "Video", "Audio", "Controls"};
constexpr int kButtonCount = 5;

void drawIfLoaded(GLuint tex, float x, float y, float w, float h, int sw, int sh) {
    if (tex != 0) {
        UIDrawHelper::drawTexturedQuad(x, y, w, h, tex, glm::vec4(1.0f), sw, sh);
    }
}

// Switches the text face for one scope, then restores the previous face.
class FontScope {
public:
    FontScope(TextRenderer* renderer, FontType type)
        : renderer_(renderer), previous_(renderer->getActiveFont()) {
        renderer_->setActiveFont(type);
    }
    ~FontScope() {
        renderer_->setActiveFont(previous_);
    }

private:
    TextRenderer* renderer_;
    FontType previous_;
};
}

OptionsUI::OptionsUI()
    : textRenderer(nullptr), screenWidth(1920), screenHeight(1080),
      visible(false), fromPause(false), returnToMenu(false),
      focusIndex(0), pendingCategory(Category::NONE),
      bgCenterTexture(0), bgEdgeTopTexture(0), bgEdgeBottomTexture(0),
      bgEdgeLeftTexture(0), bgEdgeRightTexture(0),
      bgCornerTLTexture(0), bgCornerTRTexture(0),
      bgCornerBLTexture(0), bgCornerBRTexture(0),
      selectionFullTexture(0), selectionCutTexture(0),
      pauseDecorLeftTexture(0), pauseDecorRightTexture(0),
      cachedPauseTextHeight(0.0f) {
    std::fill(std::begin(cachedButtonWidths), std::end(cachedButtonWidths), SELECTION_MIN_WIDTH);
}

OptionsUI::~OptionsUI() {
    cleanup();
}

bool OptionsUI::initialize(TextRenderer* renderer) {
    if (!renderer) {
        LOGE("Error: TextRenderer is null");
        return false;
    }
    textRenderer = renderer;

    if (bgCenterTexture == 0) {
        const std::string dir = "textures/ui/menus/";
        bgCenterTexture = TextureLoader::loadTextureFromAsset(dir + "genericbackground__center_background.png");
        bgEdgeTopTexture = TextureLoader::loadTextureFromAsset(dir + "genericbackground__edge_top.png");
        bgEdgeBottomTexture = TextureLoader::loadTextureFromAsset(dir + "genericbackground__edge_bottom.png");
        bgEdgeLeftTexture = TextureLoader::loadTextureFromAsset(dir + "genericbackground__edge_left.png");
        bgEdgeRightTexture = TextureLoader::loadTextureFromAsset(dir + "genericbackground__edge_right.png");
        bgCornerTLTexture = TextureLoader::loadTextureFromAsset(dir + "genericbackground__top_left_corner.png");
        bgCornerTRTexture = TextureLoader::loadTextureFromAsset(dir + "genericbackground__top_right_corner.png");
        bgCornerBLTexture = TextureLoader::loadTextureFromAsset(dir + "genericbackground__bottom_left_corner.png");
        bgCornerBRTexture = TextureLoader::loadTextureFromAsset(dir + "genericbackground__bottom_right_corner.png");
        selectionFullTexture = TextureLoader::loadTextureFromAsset(dir + "dialog__dialog_selection_full.png");
        selectionCutTexture = TextureLoader::loadTextureFromAsset(dir + "dialog__dialog_selection_cut.png");
        pauseDecorLeftTexture = TextureLoader::loadTextureFromAsset(dir + "shared__shared_decor_blue_left.png");
        pauseDecorRightTexture = TextureLoader::loadTextureFromAsset(dir + "shared__shared_decor_blue_right.png");
    }

    LOGI("OptionsUI initialized");
    return true;
}

void OptionsUI::cleanup() {
    for (GLuint* tex : {&bgCenterTexture, &bgEdgeTopTexture, &bgEdgeBottomTexture,
                        &bgEdgeLeftTexture, &bgEdgeRightTexture,
                        &bgCornerTLTexture, &bgCornerTRTexture,
                        &bgCornerBLTexture, &bgCornerBRTexture,
                        &selectionFullTexture, &selectionCutTexture,
                        &pauseDecorLeftTexture, &pauseDecorRightTexture}) {
        if (*tex != 0) {
            TextureLoader::deleteTexture(*tex);
            *tex = 0;
        }
    }
    textRenderer = nullptr;
    visible = false;
}

void OptionsUI::setScreenSize(int width, int height) {
    screenWidth = width;
    screenHeight = height;
}

void OptionsUI::open(bool pauseMode) {
    fromPause = pauseMode;
    visible = true;
    returnToMenu = false;
    focusIndex = 0;
    pendingCategory = Category::NONE;
}

void OptionsUI::close() {
    visible = false;
    pendingCategory = Category::NONE;
}

int OptionsUI::buttonCount() const {
    return kButtonCount;
}

OptionsUI::Rect OptionsUI::bodyRect() const {
    const float h = BODY_HEIGHT + (fromPause ? PAUSE_EXTRA_HEIGHT : 0.0f);
    return {(screenWidth - BODY_WIDTH) * 0.5f, (screenHeight - h) * 0.5f, BODY_WIDTH, h};
}

// XML: top = (bodyH - 64 * count + (pause ? 20 + textHeight : 0)) / 2, relative to the body.
float OptionsUI::bodyTop() const {
    const Rect body = bodyRect();
    const float listHeight = BUTTON_HEIGHT * static_cast<float>(buttonCount());
    const float pauseHeight = fromPause ? PAUSE_TEXT_Y + cachedPauseTextHeight : 0.0f;
    return (body.h - listHeight + pauseHeight) * 0.5f;
}

float OptionsUI::buttonWidth(int index) const {
    return cachedButtonWidths[index];
}

OptionsUI::Rect OptionsUI::buttonRect(int index) const {
    const Rect body = bodyRect();
    const float w = buttonWidth(index);
    return {body.x + (body.w - w) * 0.5f,
            body.y + bodyTop() + BUTTON_HEIGHT * static_cast<float>(index),
            w, BUTTON_HEIGHT};
}

int OptionsUI::hitTestButton(float x, float y) const {
    for (int i = 0; i < buttonCount(); ++i) {
        const Rect r = buttonRect(i);
        if (x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h) {
            return i;
        }
    }
    return -1;
}

void OptionsUI::activateButton(int index) {
    focusIndex = index;
    switch (index) {
        case 0:  // Return
            close();
            returnToMenu = true;
            LOGI("Options: Return");
            break;
        case 1:
            pendingCategory = Category::GAMEPLAY;
            break;
        case 2:
            pendingCategory = Category::VIDEO;
            break;
        case 3:
            pendingCategory = Category::AUDIO;
            break;
        case 4:
            pendingCategory = Category::CONTROLS;
            break;
        default:
            break;
    }
}

void OptionsUI::onTouchEvent(float x, float y) {
    if (!visible) {
        return;
    }
    const int index = hitTestButton(x, y);
    if (index >= 0) {
        activateButton(index);
    }
}

bool OptionsUI::handleBack() {
    if (!visible) {
        return false;
    }
    activateButton(0);
    return true;
}

// Key codes match the title screen: 19 UP, 20 DOWN, 23/66 Enter, 4 Back.
bool OptionsUI::onKeyPress(int keyCode) {
    if (!visible) {
        return false;
    }
    const int count = buttonCount();
    switch (keyCode) {
        case 19:  // DPAD_UP
            focusIndex = (focusIndex + count - 1) % count;
            return true;
        case 20:  // DPAD_DOWN
            focusIndex = (focusIndex + 1) % count;
            return true;
        case 23:  // DPAD_CENTER
        case 66:  // ENTER
            activateButton(focusIndex);
            return true;
        case 4:   // BACK
            return handleBack();
        default:
            return false;
    }
}

OptionsUI::Category OptionsUI::consumeCategoryRequest() {
    const Category c = pendingCategory;
    pendingCategory = Category::NONE;
    return c;
}

void OptionsUI::renderBorder(const Rect& b) {
    const GLuint slices[9] = {
        bgCenterTexture, bgEdgeTopTexture, bgEdgeBottomTexture, bgEdgeLeftTexture,
        bgEdgeRightTexture, bgCornerTLTexture, bgCornerTRTexture, bgCornerBLTexture,
        bgCornerBRTexture};
    UIDrawHelper::drawGenericBackground(slices, b.x, b.y, b.w, b.h, screenWidth,
                                        screenHeight);
}

void OptionsUI::renderButton(int index) {
    const Rect r = buttonRect(index);
    const bool focused = (index == focusIndex);

    if (focused) {
        // Highlight bar is the full rect width (at least 104) with a cap on the right.
        drawIfLoaded(selectionFullTexture, r.x, r.y + SELECTION_OFFSET_Y, r.w, BUTTON_HEIGHT, screenWidth, screenHeight);
        drawIfLoaded(selectionCutTexture, r.x + r.w + SELECTION_CAP_GAP - SELECTION_MIN_WIDTH, r.y,
                     SELECTION_MIN_WIDTH, BUTTON_HEIGHT, screenWidth, screenHeight);
    }

    if (textRenderer) {
        const std::string label = kButtonLabels[index];
        const float tw = textRenderer->getTextWidth(label, LABEL_SCALE);
        const float th = textRenderer->getTextHeight(LABEL_SCALE);
        textRenderer->renderText(label, r.x + (r.w - tw) * 0.5f, r.y + (BUTTON_HEIGHT - th) * 0.5f,
                                 focused ? kFocusedText : kIdleText, LABEL_SCALE);
    }
}

void OptionsUI::render() {
    if (!visible || !textRenderer) {
        return;
    }

    // 2D overlay: ignore depth left over from 3D passes
    glDisable(GL_DEPTH_TEST);

    UIDrawHelper::drawColoredQuad(0.0f, 0.0f, static_cast<float>(screenWidth),
                                  static_cast<float>(screenHeight),
                                  glm::vec4(0.0f, 0.0f, 0.0f, DIM_ALPHA),
                                  screenWidth, screenHeight);

    const Rect body = bodyRect();
    renderBorder(body);

    {
        FontScope buttonFont(textRenderer, BUTTON_FONT);
        for (int i = 0; i < buttonCount(); ++i) {
            cachedButtonWidths[i] = std::max(
                textRenderer->getTextWidth(kButtonLabels[i], LABEL_SCALE) + BUTTON_LABEL_PAD,
                SELECTION_MIN_WIDTH);
        }
    }

    if (fromPause) {
        FontScope pauseFont(textRenderer, PAUSE_FONT);
        const std::string paused = "Paused";
        cachedPauseTextHeight = textRenderer->getTextHeight(LABEL_SCALE);
        const float tw = textRenderer->getTextWidth(paused, LABEL_SCALE);

        drawIfLoaded(pauseDecorLeftTexture, body.x + PAUSE_DECOR_INSET, body.y + PAUSE_DECOR_Y,
                     PAUSE_DECOR_WIDTH, PAUSE_DECOR_HEIGHT, screenWidth, screenHeight);
        drawIfLoaded(pauseDecorRightTexture,
                     body.x + body.w - PAUSE_DECOR_INSET - PAUSE_DECOR_WIDTH, body.y + PAUSE_DECOR_Y,
                     PAUSE_DECOR_WIDTH, PAUSE_DECOR_HEIGHT, screenWidth, screenHeight);
        textRenderer->renderText(paused, body.x + (body.w - tw) * 0.5f, body.y + PAUSE_TEXT_Y,
                                 kPauseText, LABEL_SCALE);
    }

    {
        FontScope buttonFont(textRenderer, BUTTON_FONT);
        for (int i = 0; i < buttonCount(); ++i) {
            renderButton(i);
        }
    }
}
