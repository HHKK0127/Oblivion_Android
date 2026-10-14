#include "audio_options_ui.h"
#include "ui_draw_helper.h"
#include "../engine/texture_loader.h"
#include <algorithm>
#include <cmath>
#include <android/log.h>

#undef LOG_TAG
#undef LOGD
#undef LOGI
#undef LOGE
#define LOG_TAG "AudioOptionsUI"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {
// AudioMenu values from the original XML (PC variant), coordinates relative to the body.
constexpr float BODY_WIDTH = 900.0f;
constexpr float BODY_HEIGHT = 500.0f;
constexpr float BORDER = 44.0f;             // generic border sits outside the body
constexpr float LABEL_X = 30.0f;
constexpr float LABEL_FIRST_Y = 50.0f;
constexpr float LABEL_GAP = 40.0f;          // y(N) = y(N-1) + labelHeight + 40
constexpr float LABEL_TO_MARKER_DY = -10.0f;
constexpr float MARKER_HEIGHT = 64.0f;
constexpr float MARKER_WIDTH_EXTRA = 120.0f;
constexpr float MARKER_CUT_X = 420.0f;      // parent.width - 100 (relative to the marker)
constexpr float CUT_WIDTH = 104.0f;
constexpr float CUT_HEIGHT = 64.0f;
constexpr float LINE_X_IN_MARKER = 60.0f;
constexpr float LINE_Y_IN_MARKER = 22.0f;
constexpr float LINE_WIDTH = 400.0f;
constexpr float LINE_HEIGHT = 16.0f;
constexpr float HANDLE_WIDTH = 39.0f;
constexpr float HANDLE_HEIGHT = 34.0f;
constexpr float HANDLE_DY = -10.0f;         // relative to the scroll line
constexpr float TRACK_DY = -11.0f;          // hit area top relative to the scroll line
constexpr float TRACK_HEIGHT = 37.0f;
constexpr float LEFT_ARROW_DX = -49.0f;
constexpr float LEFT_ARROW_WIDTH = 50.0f;
constexpr float LEFT_ARROW_HEIGHT = 37.0f;
constexpr float RIGHT_ARROW_DX = 399.0f;
constexpr float RIGHT_ARROW_WIDTH = 51.0f;
constexpr float RIGHT_ARROW_HEIGHT = 42.0f;
constexpr float ARROW_DY = -11.0f;
constexpr float BUTTON_WIDTH = 177.0f;
constexpr float BUTTON_HEIGHT = 44.0f;
constexpr float BUTTON_MARGIN_X = 30.0f;
constexpr float BUTTON_MARGIN_Y = 20.0f;
constexpr float BUTTON_GAP = 10.0f;
constexpr float STEP = 0.01f;               // XML user3 = 1 on PC (0..100 range)
constexpr float JUMP = 0.10f;               // XML user4 = 10
constexpr float DIM_ALPHA = 0.6f;
constexpr float LABEL_SCALE = 1.0f;

constexpr FontType LABEL_FONT = FontType::KingthingsRegular; // XML font 1
const glm::vec4 kTextColor(117.0f / 255.0f, 59.0f / 255.0f, 33.0f / 255.0f, 1.0f);
const glm::vec4 kIdleText(128.0f / 255.0f, 128.0f / 255.0f, 128.0f / 255.0f, 196.0f / 255.0f);

const char* const kLabels[] = {"Master Volume:", "Voice Volume:", "Effect Volume:",
                               "Footstep Volume:", "Music Volume:"};

// Initial values from Oblivion_default.ini [Audio] fDefault*Volume:
// master 0.8 / voice 0.0 / effect 0.8 / foot 0.3 / music 0.8.
constexpr float kDefaults[] = {0.8f, 0.0f, 0.8f, 0.3f, 0.8f};

// Android keycodes
constexpr int KEY_BACK = 4;
constexpr int KEY_DPAD_UP = 19;
constexpr int KEY_DPAD_DOWN = 20;
constexpr int KEY_DPAD_LEFT = 21;
constexpr int KEY_DPAD_RIGHT = 22;
constexpr int KEY_DPAD_CENTER = 23;
constexpr int KEY_BUTTON_B = 97;
constexpr int KEY_BUTTON_Y = 100;
constexpr int KEY_BUTTON_L1 = 102;
constexpr int KEY_BUTTON_R1 = 103;
constexpr int KEY_ENTER = 66;

void drawIfLoaded(GLuint tex, float x, float y, float w, float h, int sw, int sh) {
    if (tex != 0) {
        UIDrawHelper::drawTexturedQuad(x, y, w, h, tex, glm::vec4(1.0f), sw, sh);
    }
}

void drawCropped(GLuint tex, float x, float y, float w, float h,
                 float cropX, float cropY, float cropW, float cropH,
                 float texW, float texH, int sw, int sh) {
    if (tex != 0) {
        UIDrawHelper::drawTexturedQuad(x, y, w, h, tex, glm::vec4(1.0f), sw, sh,
                                       cropX / texW, cropY / texH,
                                       (cropX + cropW) / texW, (cropY + cropH) / texH);
    }
}

bool contains(float px, float py, float x, float y, float w, float h) {
    return px >= x && px <= x + w && py >= y && py <= y + h;
}

float clamp01(float v) {
    return std::min(1.0f, std::max(0.0f, v));
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

AudioOptionsUI::AudioOptionsUI()
    : textRenderer(nullptr), screenWidth(1920), screenHeight(1080),
      visible(false), returnToOptions(false), changed(false),
      focusIndex(ROW_MASTER), dragSlider(-1),
      longLabelWidth(0.0f), labelHeight(28.0f),
      bgCenterTexture(0), bgEdgeTopTexture(0), bgEdgeBottomTexture(0),
      bgEdgeLeftTexture(0), bgEdgeRightTexture(0),
      bgCornerTLTexture(0), bgCornerTRTexture(0),
      bgCornerBLTexture(0), bgCornerBRTexture(0),
      selectionFullTexture(0), selectionCutTexture(0),
      scrollLineTexture(0), scrollPartsTexture(0),
      buttonOffTexture(0), buttonOnTexture(0) {
    for (int i = 0; i < SLIDER_COUNT; ++i) {
        values[i] = kDefaults[i];
    }
}

AudioOptionsUI::~AudioOptionsUI() {
    cleanup();
}

bool AudioOptionsUI::initialize(TextRenderer* renderer) {
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
        scrollLineTexture = TextureLoader::loadTextureFromAsset(dir + "shared__horizontal_scrollbar_line.png");
        scrollPartsTexture = TextureLoader::loadTextureFromAsset(dir + "shared__horizontal_scrollbar_parts.png");
        buttonOffTexture = TextureLoader::loadTextureFromAsset(dir + "shared__shared_button_long_off.png");
        buttonOnTexture = TextureLoader::loadTextureFromAsset(dir + "shared__shared_button_long_on.png");
    }

    measureLabels();
    LOGI("AudioOptionsUI initialized");
    return true;
}

void AudioOptionsUI::cleanup() {
    for (GLuint* tex : {&bgCenterTexture, &bgEdgeTopTexture, &bgEdgeBottomTexture,
                        &bgEdgeLeftTexture, &bgEdgeRightTexture,
                        &bgCornerTLTexture, &bgCornerTRTexture,
                        &bgCornerBLTexture, &bgCornerBRTexture,
                        &selectionFullTexture, &selectionCutTexture,
                        &scrollLineTexture, &scrollPartsTexture,
                        &buttonOffTexture, &buttonOnTexture}) {
        if (*tex != 0) {
            TextureLoader::deleteTexture(*tex);
            *tex = 0;
        }
    }
    textRenderer = nullptr;
    visible = false;
}

void AudioOptionsUI::setScreenSize(int width, int height) {
    screenWidth = width;
    screenHeight = height;
}

void AudioOptionsUI::open(const Volumes& initial) {
    values[0] = clamp01(initial.master);
    values[1] = clamp01(initial.voice);
    values[2] = clamp01(initial.effect);
    values[3] = clamp01(initial.foot);
    values[4] = clamp01(initial.music);
    visible = true;
    returnToOptions = false;
    changed = false;
    focusIndex = ROW_MASTER;
    dragSlider = -1;
    measureLabels();
}

void AudioOptionsUI::close() {
    visible = false;
    dragSlider = -1;
}

bool AudioOptionsUI::consumeChanged() {
    const bool result = changed;
    changed = false;
    return result;
}

AudioOptionsUI::Volumes AudioOptionsUI::volumes() const {
    Volumes v;
    v.master = values[0];
    v.voice = values[1];
    v.effect = values[2];
    v.foot = values[3];
    v.music = values[4];
    return v;
}

void AudioOptionsUI::measureLabels() {
    if (!textRenderer) {
        return;
    }
    FontScope font(textRenderer, LABEL_FONT);
    float maxWidth = 0.0f;
    for (const char* label : kLabels) {
        maxWidth = std::max(maxWidth, textRenderer->getTextWidth(label, LABEL_SCALE));
    }
    longLabelWidth = maxWidth;
    labelHeight = textRenderer->getTextHeight(LABEL_SCALE);
}

AudioOptionsUI::Rect AudioOptionsUI::bodyRect() const {
    return {(screenWidth - BODY_WIDTH) * 0.5f, (screenHeight - BODY_HEIGHT) * 0.5f,
            BODY_WIDTH, BODY_HEIGHT};
}

float AudioOptionsUI::labelLocalY(int slider) const {
    return LABEL_FIRST_Y + static_cast<float>(slider) * (labelHeight + LABEL_GAP);
}

// Scroll line position: marker.x = longLabelWidth + 60, line.x = marker.x + 60.
AudioOptionsUI::Rect AudioOptionsUI::sliderLineRect(int slider) const {
    const Rect body = bodyRect();
    const float markerX = longLabelWidth + 60.0f;
    const float markerY = labelLocalY(slider) + LABEL_TO_MARKER_DY;
    return {body.x + markerX + LINE_X_IN_MARKER,
            body.y + markerY + LINE_Y_IN_MARKER,
            LINE_WIDTH, LINE_HEIGHT};
}

AudioOptionsUI::Rect AudioOptionsUI::sliderHandleRect(int slider) const {
    const Rect line = sliderLineRect(slider);
    const float travel = LINE_WIDTH - HANDLE_WIDTH;
    return {line.x + travel * values[slider], line.y + HANDLE_DY, HANDLE_WIDTH, HANDLE_HEIGHT};
}

AudioOptionsUI::Rect AudioOptionsUI::buttonRect(int row) const {
    const Rect body = bodyRect();
    const float returnX = body.w - BUTTON_WIDTH - BUTTON_MARGIN_X;
    const float y = body.h - BUTTON_HEIGHT - BUTTON_MARGIN_Y;
    if (row == ROW_RETURN) {
        return {body.x + returnX, body.y + y, BUTTON_WIDTH, BUTTON_HEIGHT};
    }
    return {body.x + returnX - BUTTON_WIDTH - BUTTON_GAP, body.y + y, BUTTON_WIDTH, BUTTON_HEIGHT};
}

void AudioOptionsUI::setValue(int slider, float value) {
    const float clamped = clamp01(value);
    if (std::fabs(clamped - values[slider]) > 1e-6f) {
        values[slider] = clamped;
        changed = true;
    }
}

void AudioOptionsUI::adjustSlider(int slider, float delta) {
    setValue(slider, values[slider] + delta);
}

void AudioOptionsUI::setSliderFromX(int slider, float screenX) {
    const Rect line = sliderLineRect(slider);
    const float travel = LINE_WIDTH - HANDLE_WIDTH;
    setValue(slider, (screenX - line.x - HANDLE_WIDTH * 0.5f) / travel);
}

void AudioOptionsUI::resetToDefaults() {
    for (int i = 0; i < SLIDER_COUNT; ++i) {
        setValue(i, kDefaults[i]);
    }
    LOGI("AudioOptionsUI: defaults restored");
}

void AudioOptionsUI::focusRow(int row) {
    focusIndex = row;
}

void AudioOptionsUI::activateRow(int row) {
    focusRow(row);
    if (row == ROW_DEFAULTS) {
        resetToDefaults();
    } else if (row == ROW_RETURN) {
        handleBack();
    }
}

void AudioOptionsUI::onTouchDown(float x, float y) {
    if (!visible) {
        return;
    }

    for (int i = 0; i < SLIDER_COUNT; ++i) {
        const Rect line = sliderLineRect(i);
        const Rect leftArrow{line.x + LEFT_ARROW_DX, line.y + ARROW_DY, LEFT_ARROW_WIDTH, LEFT_ARROW_HEIGHT};
        const Rect rightArrow{line.x + RIGHT_ARROW_DX, line.y + ARROW_DY, RIGHT_ARROW_WIDTH, RIGHT_ARROW_HEIGHT};
        const Rect track{line.x, line.y + TRACK_DY, LINE_WIDTH, TRACK_HEIGHT};
        const Rect handle = sliderHandleRect(i);

        if (contains(x, y, leftArrow.x, leftArrow.y, leftArrow.w, leftArrow.h)) {
            focusRow(i);
            adjustSlider(i, -STEP);
            return;
        }
        if (contains(x, y, rightArrow.x, rightArrow.y, rightArrow.w, rightArrow.h)) {
            focusRow(i);
            adjustSlider(i, STEP);
            return;
        }
        if (contains(x, y, handle.x, handle.y, handle.w, handle.h)) {
            focusRow(i);
            dragSlider = i;
            return;
        }
        if (contains(x, y, track.x, track.y, track.w, track.h)) {
            focusRow(i);
            adjustSlider(i, x < handle.x ? -JUMP : JUMP);
            return;
        }
    }

    for (int row : {ROW_DEFAULTS, ROW_RETURN}) {
        const Rect r = buttonRect(row);
        if (contains(x, y, r.x, r.y, r.w, r.h)) {
            activateRow(row);
            return;
        }
    }
}

void AudioOptionsUI::onTouchMove(float x) {
    if (visible && dragSlider >= 0) {
        setSliderFromX(dragSlider, x);
    }
}

void AudioOptionsUI::onTouchUp() {
    dragSlider = -1;
}

bool AudioOptionsUI::onKeyPress(int keyCode) {
    if (!visible) {
        return false;
    }

    const bool onSlider = focusIndex < SLIDER_COUNT;
    switch (keyCode) {
        case KEY_DPAD_UP:
            focusRow((focusIndex + ROW_COUNT - 1) % ROW_COUNT);
            return true;
        case KEY_DPAD_DOWN:
            focusRow((focusIndex + 1) % ROW_COUNT);
            return true;
        case KEY_DPAD_LEFT:
        case KEY_DPAD_RIGHT:
            if (onSlider) {
                adjustSlider(focusIndex, keyCode == KEY_DPAD_LEFT ? -STEP : STEP);
            } else {
                // Defaults and Return are neighbours on the same row.
                focusRow(focusIndex == ROW_DEFAULTS ? ROW_RETURN : ROW_DEFAULTS);
            }
            return true;
        case KEY_BUTTON_L1:
            if (onSlider) {
                adjustSlider(focusIndex, -JUMP);
            }
            return true;
        case KEY_BUTTON_R1:
            if (onSlider) {
                adjustSlider(focusIndex, JUMP);
            }
            return true;
        case KEY_DPAD_CENTER:
        case KEY_ENTER:
            if (!onSlider) {
                activateRow(focusIndex);
            }
            return true;
        case KEY_BUTTON_Y:
            resetToDefaults();
            return true;
        case KEY_BUTTON_B:
            return handleBack();
        default:
            return false;
    }
}

bool AudioOptionsUI::handleBack() {
    if (!visible) {
        return false;
    }
    returnToOptions = true;
    close();
    return true;
}

void AudioOptionsUI::renderBorder(const Rect& b) {
    const int sw = screenWidth;
    const int sh = screenHeight;

    drawIfLoaded(bgCenterTexture, b.x, b.y, b.w, b.h, sw, sh);
    drawIfLoaded(bgEdgeTopTexture, b.x, b.y - BORDER, b.w, BORDER, sw, sh);
    drawIfLoaded(bgEdgeBottomTexture, b.x, b.y + b.h, b.w, BORDER, sw, sh);
    drawIfLoaded(bgEdgeLeftTexture, b.x - BORDER, b.y, BORDER, b.h, sw, sh);
    drawIfLoaded(bgEdgeRightTexture, b.x + b.w, b.y, BORDER, b.h, sw, sh);
    drawIfLoaded(bgCornerTLTexture, b.x - BORDER, b.y - BORDER, BORDER, BORDER, sw, sh);
    drawIfLoaded(bgCornerTRTexture, b.x + b.w, b.y - BORDER, BORDER, BORDER, sw, sh);
    drawIfLoaded(bgCornerBLTexture, b.x - BORDER, b.y + b.h, BORDER, BORDER, sw, sh);
    drawIfLoaded(bgCornerBRTexture, b.x + b.w, b.y + b.h, BORDER, BORDER, sw, sh);
}

void AudioOptionsUI::renderSlider(int slider) {
    const int sw = screenWidth;
    const int sh = screenHeight;
    const Rect body = bodyRect();
    const Rect line = sliderLineRect(slider);
    const bool focused = (focusIndex == slider);

    // Focus marker (marker group is visible only while its row is focused)
    if (focused) {
        const float markerX = body.x + longLabelWidth + 60.0f;
        const float markerY = body.y + labelLocalY(slider) + LABEL_TO_MARKER_DY;
        const float markerW = LINE_WIDTH + MARKER_WIDTH_EXTRA;
        drawIfLoaded(selectionFullTexture, markerX, markerY, markerW, MARKER_HEIGHT, sw, sh);
        drawIfLoaded(selectionCutTexture, markerX + MARKER_CUT_X, markerY, CUT_WIDTH, CUT_HEIGHT, sw, sh);
    }

    drawIfLoaded(scrollLineTexture, line.x, line.y, line.w, line.h, sw, sh);

    // Arrows and handle are cropped from the 256x64 parts sheet.
    constexpr float PARTS_W = 256.0f;
    constexpr float PARTS_H = 64.0f;
    drawCropped(scrollPartsTexture, line.x + LEFT_ARROW_DX, line.y + ARROW_DY,
                LEFT_ARROW_WIDTH, LEFT_ARROW_HEIGHT, 0.0f, 0.0f, 50.0f, 37.0f, PARTS_W, PARTS_H, sw, sh);
    drawCropped(scrollPartsTexture, line.x + RIGHT_ARROW_DX, line.y + ARROW_DY,
                RIGHT_ARROW_WIDTH, RIGHT_ARROW_HEIGHT, 128.0f, 0.0f, 51.0f, 42.0f, PARTS_W, PARTS_H, sw, sh);

    const Rect handle = sliderHandleRect(slider);
    drawCropped(scrollPartsTexture, handle.x, handle.y, handle.w, handle.h,
                64.0f, 0.0f, 39.0f, 34.0f, PARTS_W, PARTS_H, sw, sh);
}

void AudioOptionsUI::renderButton(int row, const char* label) {
    const Rect r = buttonRect(row);
    const bool focused = (focusIndex == row);
    drawIfLoaded(focused ? buttonOnTexture : buttonOffTexture, r.x, r.y, r.w, r.h, screenWidth, screenHeight);

    if (textRenderer) {
        FontScope font(textRenderer, LABEL_FONT);
        const float tw = textRenderer->getTextWidth(label, LABEL_SCALE);
        const float th = textRenderer->getTextHeight(LABEL_SCALE);
        textRenderer->renderText(label, r.x + (r.w - tw) * 0.5f, r.y + (r.h - th) * 0.5f,
                                 focused ? kTextColor : kIdleText, LABEL_SCALE);
    }
}

void AudioOptionsUI::render() {
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
        FontScope font(textRenderer, LABEL_FONT);
        for (int i = 0; i < SLIDER_COUNT; ++i) {
            textRenderer->renderText(kLabels[i], body.x + LABEL_X, body.y + labelLocalY(i),
                                     kTextColor, LABEL_SCALE);
        }
    }

    for (int i = 0; i < SLIDER_COUNT; ++i) {
        renderSlider(i);
    }

    renderButton(ROW_DEFAULTS, "Defaults");
    renderButton(ROW_RETURN, "Return");
}
