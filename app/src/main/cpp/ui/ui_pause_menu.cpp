#include "ui_pause_menu.h"
#include "ui_draw_helper.h"
#include "../engine/texture_loader.h"
#include <algorithm>
#include <android/log.h>

#undef LOG_TAG
#undef LOGD
#undef LOGI
#undef LOGE
#define LOG_TAG "UIPauseMenu"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {
// Values from options\pause_menu.xml (PC variant).
constexpr float BODY_WIDTH = 240.0f;          // pause_background user0
constexpr float BODY_HEIGHT = 400.0f;         // pause_background user1
constexpr float BUTTON_HEIGHT = 64.0f;        // button_floating_2 height
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
constexpr int ITEM_COUNT = 5;

// XML <font> values mapped to FontType: 1 = Kingthings_Regular, 3 = Tahoma_Bold_Small.
constexpr FontType BUTTON_FONT = FontType::KingthingsRegular;
constexpr FontType PAUSE_FONT = FontType::TahomaBoldSmall;

const glm::vec4 kFocusedText(117.0f / 255.0f, 59.0f / 255.0f, 33.0f / 255.0f, 1.0f);
const glm::vec4 kIdleText(128.0f / 255.0f, 128.0f / 255.0f, 128.0f / 255.0f, 196.0f / 255.0f);
const glm::vec4 kPauseText(51.0f / 255.0f, 79.0f / 255.0f, 142.0f / 255.0f, 1.0f);

// strings()._return / _save / _load / _options / _quit, in XML listindex order.
const char* const kItemLabels[ITEM_COUNT] = {"Return", "Save", "Load", "Options", "Quit"};

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
}  // namespace

UIPauseMenu::UIPauseMenu()
    : textRenderer(nullptr), screenWidth(1920), screenHeight(1080),
      visible(false), focusIndex(0), hasSelection(false),
      pendingSelection(Item::RETURN),
      bgCenterTexture(0), bgEdgeTopTexture(0), bgEdgeBottomTexture(0),
      bgEdgeLeftTexture(0), bgEdgeRightTexture(0),
      bgCornerTLTexture(0), bgCornerTRTexture(0),
      bgCornerBLTexture(0), bgCornerBRTexture(0),
      selectionFullTexture(0), selectionCutTexture(0),
      decorLeftTexture(0), decorRightTexture(0),
      cachedPauseTextHeight(0.0f) {
    std::fill(std::begin(cachedButtonWidths), std::end(cachedButtonWidths), SELECTION_MIN_WIDTH);
}

UIPauseMenu::~UIPauseMenu() {
    cleanup();
}

bool UIPauseMenu::initialize(TextRenderer* renderer) {
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
        decorLeftTexture = TextureLoader::loadTextureFromAsset(dir + "shared__shared_decor_blue_left.png");
        decorRightTexture = TextureLoader::loadTextureFromAsset(dir + "shared__shared_decor_blue_right.png");
    }

    LOGI("UIPauseMenu initialized");
    return true;
}

void UIPauseMenu::cleanup() {
    for (GLuint* tex : {&bgCenterTexture, &bgEdgeTopTexture, &bgEdgeBottomTexture,
                        &bgEdgeLeftTexture, &bgEdgeRightTexture,
                        &bgCornerTLTexture, &bgCornerTRTexture,
                        &bgCornerBLTexture, &bgCornerBRTexture,
                        &selectionFullTexture, &selectionCutTexture,
                        &decorLeftTexture, &decorRightTexture}) {
        if (*tex != 0) {
            TextureLoader::deleteTexture(*tex);
            *tex = 0;
        }
    }
    textRenderer = nullptr;
    visible = false;
}

void UIPauseMenu::setScreenSize(int width, int height) {
    screenWidth = width;
    screenHeight = height;
}

void UIPauseMenu::open() {
    visible = true;
    hasSelection = false;
    focusIndex = 0;  // pause_return_button carries xdefault 3 (focused by default)
}

void UIPauseMenu::close() {
    visible = false;
    hasSelection = false;
}

UIPauseMenu::Rect UIPauseMenu::bodyRect() const {
    return {(screenWidth - BODY_WIDTH) * 0.5f, (screenHeight - BODY_HEIGHT) * 0.5f,
            BODY_WIDTH, BODY_HEIGHT};
}

// XML: _top = (parentH - height * 5 + _y_buffer * 4) / 2 + pause_indication.y
//              + pause_indication.height / 2, relative to pause_background.
float UIPauseMenu::bodyTop() const {
    return (BODY_HEIGHT - BUTTON_HEIGHT * static_cast<float>(ITEM_COUNT)) * 0.5f +
           PAUSE_TEXT_Y + cachedPauseTextHeight * 0.5f;
}

UIPauseMenu::Rect UIPauseMenu::buttonRect(int index) const {
    const Rect body = bodyRect();
    const float w = cachedButtonWidths[index];
    const float centerX = body.x + BODY_WIDTH * 0.5f;
    return {centerX - w * 0.5f, body.y + bodyTop() + BUTTON_HEIGHT * static_cast<float>(index),
            w, BUTTON_HEIGHT};
}

int UIPauseMenu::hitTestButton(float x, float y) const {
    for (int i = 0; i < ITEM_COUNT; ++i) {
        const Rect r = buttonRect(i);
        if (x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h) {
            return i;
        }
    }
    return -1;
}

void UIPauseMenu::activate(int index) {
    if (index < 0 || index >= ITEM_COUNT) {
        return;
    }
    focusIndex = index;
    pendingSelection = static_cast<Item>(index);
    hasSelection = true;
}

UIPauseMenu::Item UIPauseMenu::consumeSelection() {
    hasSelection = false;
    return pendingSelection;
}

void UIPauseMenu::onTouchEvent(float x, float y) {
    if (!visible) {
        return;
    }
    const int index = hitTestButton(x, y);
    if (index >= 0) {
        activate(index);
    }
}

bool UIPauseMenu::handleBack() {
    if (!visible) {
        return false;
    }
    activate(0);  // pause_return_button, also the root's xbuttonb target
    return true;
}

bool UIPauseMenu::onKeyPress(int keyCode) {
    if (!visible) {
        return false;
    }
    switch (keyCode) {
        case 19:  // DPAD_UP
            focusIndex = (focusIndex + ITEM_COUNT - 1) % ITEM_COUNT;
            return true;
        case 20:  // DPAD_DOWN
            focusIndex = (focusIndex + 1) % ITEM_COUNT;
            return true;
        case 23:  // DPAD_CENTER
        case 66:  // ENTER
            activate(focusIndex);
            return true;
        case 4:  // BACK
            return handleBack();
        default:
            return false;
    }
}

void UIPauseMenu::renderBorder(const Rect& b) {
    const GLuint slices[9] = {bgCenterTexture,   bgEdgeTopTexture,    bgEdgeBottomTexture,
                              bgEdgeLeftTexture, bgEdgeRightTexture,  bgCornerTLTexture,
                              bgCornerTRTexture, bgCornerBLTexture,   bgCornerBRTexture};
    UIDrawHelper::drawGenericBackground(slices, b.x, b.y, b.w, b.h, screenWidth,
                                        screenHeight);
}

void UIPauseMenu::renderButton(int index) {
    const Rect r = buttonRect(index);
    const bool focused = (index == focusIndex);

    if (focused) {
        // button_floating spans the full rect width at y + 3; button_end caps the
        // right edge with an 8px overlap.
        drawIfLoaded(selectionFullTexture, r.x, r.y + SELECTION_OFFSET_Y, r.w, BUTTON_HEIGHT,
                     screenWidth, screenHeight);
        drawIfLoaded(selectionCutTexture,
                     r.x + r.w + SELECTION_CAP_GAP - SELECTION_MIN_WIDTH,
                     r.y + SELECTION_OFFSET_Y, SELECTION_MIN_WIDTH, BUTTON_HEIGHT,
                     screenWidth, screenHeight);
    }

    if (textRenderer) {
        const std::string label = kItemLabels[index];
        const float tw = textRenderer->getTextWidth(label, LABEL_SCALE);
        const float th = textRenderer->getTextHeight(LABEL_SCALE);
        textRenderer->renderText(label, r.x + (r.w - tw) * 0.5f, r.y + (BUTTON_HEIGHT - th) * 0.5f,
                                 focused ? kFocusedText : kIdleText, LABEL_SCALE);
    }
}

void UIPauseMenu::render() {
    if (!visible || !textRenderer) {
        return;
    }

    // 2D overlay: ignore depth left over from the 3D passes
    glDisable(GL_DEPTH_TEST);

    // The XML root carries alpha 0 and an explorefade of 0.25: the world behind
    // is faded out while the pause menu owns the screen.
    UIDrawHelper::drawColoredQuad(0.0f, 0.0f, static_cast<float>(screenWidth),
                                  static_cast<float>(screenHeight),
                                  glm::vec4(0.0f, 0.0f, 0.0f, DIM_ALPHA),
                                  screenWidth, screenHeight);

    const Rect body = bodyRect();
    renderBorder(body);

    {
        FontScope buttonFont(textRenderer, BUTTON_FONT);
        for (int i = 0; i < ITEM_COUNT; ++i) {
            cachedButtonWidths[i] = std::max(
                textRenderer->getTextWidth(kItemLabels[i], LABEL_SCALE) + BUTTON_LABEL_PAD,
                SELECTION_MIN_WIDTH);
        }
    }

    {
        FontScope pauseFont(textRenderer, PAUSE_FONT);
        cachedPauseTextHeight = textRenderer->getTextHeight(LABEL_SCALE);
        const std::string paused = "Paused";
        const float tw = textRenderer->getTextWidth(paused, LABEL_SCALE);

        drawIfLoaded(decorLeftTexture, body.x + PAUSE_DECOR_INSET, body.y + PAUSE_DECOR_Y,
                     PAUSE_DECOR_WIDTH, PAUSE_DECOR_HEIGHT, screenWidth, screenHeight);
        drawIfLoaded(decorRightTexture,
                     body.x + BODY_WIDTH - PAUSE_DECOR_INSET - PAUSE_DECOR_WIDTH,
                     body.y + PAUSE_DECOR_Y, PAUSE_DECOR_WIDTH, PAUSE_DECOR_HEIGHT,
                     screenWidth, screenHeight);
        textRenderer->renderText(paused, body.x + (BODY_WIDTH - tw) * 0.5f, body.y + PAUSE_TEXT_Y,
                                 kPauseText, LABEL_SCALE);
    }

    {
        FontScope buttonFont(textRenderer, BUTTON_FONT);
        for (int i = 0; i < ITEM_COUNT; ++i) {
            renderButton(i);
        }
    }
}

