#include "gameplay_options_ui.h"
#include "ui_draw_helper.h"
#include "../engine/texture_loader.h"
#include <algorithm>
#include <cmath>
#include <android/log.h>

#undef LOG_TAG
#undef LOGD
#undef LOGI
#undef LOGE
#define LOG_TAG "GameplayOptionsUI"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {
// GameplayMenu values from the original XML (PC variant), coordinates relative to the body.
constexpr float BODY_WIDTH = 790.0f;
constexpr float BODY_HEIGHT = 528.0f;
constexpr float BORDER = 44.0f;               // generic border sits outside the body
constexpr float LABEL_FIRST_Y = 32.0f;
constexpr float LABEL_STRIDE = 52.0f;         // label height 28 + _y_buffer 24
constexpr float LABEL_TO_SCROLL_DY = 12.0f;   // scroll line sits 12px below its label
constexpr float LABEL_TO_MARKER_DY = -10.0f;
constexpr float LABEL_TO_SCROLLBAR_BUFFER = 64.0f;
constexpr float TOGGLE_TOP_DY = -12.0f;       // _top = label.y - 12
constexpr float TOGGLE_HEIGHT = 64.0f;
constexpr float TOGGLE_MIN_WIDTH = 104.0f;
constexpr float TOGGLE_TEXT_PAD = 40.0f;
constexpr float LINE_WIDTH = 400.0f;
constexpr float LINE_HEIGHT = 16.0f;
constexpr float ARROW_LEFT_WIDTH = 50.0f;     // scroll bar left cap width
constexpr float ARROW_RIGHT_WIDTH = 51.0f;    // scroll bar right cap width
constexpr float FULL_WIDTH = LINE_WIDTH + ARROW_LEFT_WIDTH + ARROW_RIGHT_WIDTH;
constexpr float MARKER_WIDTH_EXTRA = 120.0f;
constexpr float MARKER_HEIGHT = 64.0f;
constexpr float MARKER_CUT_X = 420.0f;        // parent.width - 100
constexpr float CUT_WIDTH = 104.0f;
constexpr float CUT_HEIGHT = 64.0f;
constexpr float HANDLE_WIDTH = 39.0f;
constexpr float HANDLE_HEIGHT = 34.0f;
constexpr float HANDLE_DY = -10.0f;
constexpr float TRACK_DY = -11.0f;
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
constexpr int DIFFICULTY_MIN = 0;
constexpr int DIFFICULTY_MAX = 100;
constexpr int DIFFICULTY_STEP = 1;            // XML user3 = 1 on PC
constexpr int DIFFICULTY_JUMP = 10;           // XML user4 = 10
constexpr float DIM_ALPHA = 0.6f;
constexpr float LABEL_SCALE = 1.0f;

constexpr FontType LABEL_FONT = FontType::KingthingsRegular; // XML font 1
const glm::vec4 kTextColor(117.0f / 255.0f, 59.0f / 255.0f, 33.0f / 255.0f, 1.0f);
const glm::vec4 kIdleText(128.0f / 255.0f, 128.0f / 255.0f, 128.0f / 255.0f, 196.0f / 255.0f);

const char* const kLabels[] = {"Difficulty:", "General Subtitles:", "Dialogue Subtitles:",
                               "Crosshair:", "Save on Rest:", "Save on Wait:", "Save on Travel:"};

// Android keycodes
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

GameplayOptionsUI::GameplayOptionsUI()
    : textRenderer(nullptr), screenWidth(1920), screenHeight(1080),
      visible(false), returnToOptions(false), changed(false), dragging(false),
      focusIndex(ROW_DIFFICULTY), longLabelWidth(0.0f), labelHeight(28.0f),
      bgCenterTexture(0), bgEdgeTopTexture(0), bgEdgeBottomTexture(0),
      bgEdgeLeftTexture(0), bgEdgeRightTexture(0),
      bgCornerTLTexture(0), bgCornerTRTexture(0),
      bgCornerBLTexture(0), bgCornerBRTexture(0),
      selectionFullTexture(0), selectionCutTexture(0),
      scrollLineTexture(0), scrollPartsTexture(0),
      buttonOffTexture(0), buttonOnTexture(0) {
}

GameplayOptionsUI::~GameplayOptionsUI() {
    cleanup();
}

bool GameplayOptionsUI::initialize(TextRenderer* renderer) {
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
    LOGI("GameplayOptionsUI initialized");
    return true;
}

void GameplayOptionsUI::cleanup() {
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

void GameplayOptionsUI::setScreenSize(int width, int height) {
    screenWidth = width;
    screenHeight = height;
}

void GameplayOptionsUI::open(const Settings& initial) {
    current = initial;
    current.difficulty = std::min(DIFFICULTY_MAX, std::max(DIFFICULTY_MIN, current.difficulty));
    visible = true;
    returnToOptions = false;
    changed = false;
    dragging = false;
    focusIndex = ROW_DIFFICULTY;
    measureLabels();
}

void GameplayOptionsUI::close() {
    visible = false;
    dragging = false;
}

bool GameplayOptionsUI::consumeChanged() {
    const bool result = changed;
    changed = false;
    return result;
}

void GameplayOptionsUI::measureLabels() {
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

GameplayOptionsUI::Rect GameplayOptionsUI::bodyRect() const {
    return {(screenWidth - BODY_WIDTH) * 0.5f, (screenHeight - BODY_HEIGHT) * 0.5f,
            BODY_WIDTH, BODY_HEIGHT};
}

float GameplayOptionsUI::labelLocalY(int row) const {
    return LABEL_FIRST_Y + static_cast<float>(row) * LABEL_STRIDE;
}

// The label block and scroll bar are centred as one unit; every value column shares the same x.
GameplayOptionsUI::Rect GameplayOptionsUI::sliderLineRect() const {
    const Rect body = bodyRect();
    const float blockWidth = longLabelWidth + LABEL_TO_SCROLLBAR_BUFFER + FULL_WIDTH;
    const float blockX = (BODY_WIDTH - blockWidth) * 0.5f;
    const float lineX = blockX + longLabelWidth + LABEL_TO_SCROLLBAR_BUFFER + ARROW_LEFT_WIDTH;
    return {body.x + lineX, body.y + labelLocalY(ROW_DIFFICULTY) + LABEL_TO_SCROLL_DY,
            LINE_WIDTH, LINE_HEIGHT};
}

GameplayOptionsUI::Rect GameplayOptionsUI::sliderHandleRect() const {
    const Rect line = sliderLineRect();
    const float travel = LINE_WIDTH - HANDLE_WIDTH;
    const float ratio = static_cast<float>(current.difficulty - DIFFICULTY_MIN) /
                        static_cast<float>(DIFFICULTY_MAX - DIFFICULTY_MIN);
    return {line.x + travel * ratio, line.y + HANDLE_DY, HANDLE_WIDTH, HANDLE_HEIGHT};
}

GameplayOptionsUI::Rect GameplayOptionsUI::toggleRect(int row) const {
    const Rect line = sliderLineRect();
    const float centerX = line.x + LINE_WIDTH * 0.5f;
    const Rect body = bodyRect();
    const float top = body.y + labelLocalY(row) + TOGGLE_TOP_DY;
    return {centerX - TOGGLE_MIN_WIDTH * 0.5f, top, TOGGLE_MIN_WIDTH, TOGGLE_HEIGHT};
}

GameplayOptionsUI::Rect GameplayOptionsUI::buttonRect(int row) const {
    const Rect body = bodyRect();
    const float returnX = body.w - BUTTON_WIDTH - BUTTON_MARGIN_X;
    const float y = body.h - BUTTON_HEIGHT - BUTTON_MARGIN_Y;
    if (row == ROW_RETURN) {
        return {body.x + returnX, body.y + y, BUTTON_WIDTH, BUTTON_HEIGHT};
    }
    return {body.x + returnX - BUTTON_WIDTH - BUTTON_GAP, body.y + y, BUTTON_WIDTH, BUTTON_HEIGHT};
}

bool GameplayOptionsUI::isToggleRow(int row) const {
    return row >= ROW_GENERAL_SUBTITLES && row <= ROW_SAVE_TRAVEL;
}

bool GameplayOptionsUI::getToggle(int row) const {
    switch (row) {
        case ROW_GENERAL_SUBTITLES: return current.generalSubtitles;
        case ROW_DIALOGUE_SUBTITLES: return current.dialogueSubtitles;
        case ROW_CROSSHAIR: return current.crosshair;
        case ROW_SAVE_REST: return current.saveOnRest;
        case ROW_SAVE_WAIT: return current.saveOnWait;
        case ROW_SAVE_TRAVEL: return current.saveOnTravel;
        default: return false;
    }
}

void GameplayOptionsUI::setToggle(int row, bool on) {
    if (getToggle(row) == on) {
        return;
    }
    switch (row) {
        case ROW_GENERAL_SUBTITLES: current.generalSubtitles = on; break;
        case ROW_DIALOGUE_SUBTITLES: current.dialogueSubtitles = on; break;
        case ROW_CROSSHAIR: current.crosshair = on; break;
        case ROW_SAVE_REST: current.saveOnRest = on; break;
        case ROW_SAVE_WAIT: current.saveOnWait = on; break;
        case ROW_SAVE_TRAVEL: current.saveOnTravel = on; break;
        default: return;
    }
    changed = true;
}

void GameplayOptionsUI::toggleRow(int row) {
    setToggle(row, !getToggle(row));
}

void GameplayOptionsUI::setDifficulty(int value) {
    const int clamped = std::min(DIFFICULTY_MAX, std::max(DIFFICULTY_MIN, value));
    if (clamped != current.difficulty) {
        current.difficulty = clamped;
        changed = true;
    }
}

void GameplayOptionsUI::setDifficultyFromX(float screenX) {
    const Rect line = sliderLineRect();
    const float travel = LINE_WIDTH - HANDLE_WIDTH;
    const float ratio = (screenX - line.x - HANDLE_WIDTH * 0.5f) / travel;
    setDifficulty(DIFFICULTY_MIN + static_cast<int>(std::lround(
        std::min(1.0f, std::max(0.0f, ratio)) * (DIFFICULTY_MAX - DIFFICULTY_MIN))));
}

void GameplayOptionsUI::resetToDefaults() {
    const Settings defaults;
    setDifficulty(defaults.difficulty);
    setToggle(ROW_GENERAL_SUBTITLES, defaults.generalSubtitles);
    setToggle(ROW_DIALOGUE_SUBTITLES, defaults.dialogueSubtitles);
    setToggle(ROW_CROSSHAIR, defaults.crosshair);
    setToggle(ROW_SAVE_REST, defaults.saveOnRest);
    setToggle(ROW_SAVE_WAIT, defaults.saveOnWait);
    setToggle(ROW_SAVE_TRAVEL, defaults.saveOnTravel);
    LOGI("GameplayOptionsUI: defaults restored");
}

void GameplayOptionsUI::focusRow(int row) {
    focusIndex = row;
}

void GameplayOptionsUI::activateRow(int row) {
    focusRow(row);
    if (row == ROW_DEFAULTS) {
        resetToDefaults();
    } else if (row == ROW_RETURN) {
        handleBack();
    } else if (isToggleRow(row)) {
        toggleRow(row);
    }
}

void GameplayOptionsUI::onTouchDown(float x, float y) {
    if (!visible) {
        return;
    }

    const Rect line = sliderLineRect();
    const Rect leftArrow{line.x + LEFT_ARROW_DX, line.y + ARROW_DY, LEFT_ARROW_WIDTH, LEFT_ARROW_HEIGHT};
    const Rect rightArrow{line.x + RIGHT_ARROW_DX, line.y + ARROW_DY, RIGHT_ARROW_WIDTH, RIGHT_ARROW_HEIGHT};
    const Rect track{line.x, line.y + TRACK_DY, LINE_WIDTH, TRACK_HEIGHT};
    const Rect handle = sliderHandleRect();

    if (contains(x, y, leftArrow.x, leftArrow.y, leftArrow.w, leftArrow.h)) {
        focusRow(ROW_DIFFICULTY);
        setDifficulty(current.difficulty - DIFFICULTY_STEP);
        return;
    }
    if (contains(x, y, rightArrow.x, rightArrow.y, rightArrow.w, rightArrow.h)) {
        focusRow(ROW_DIFFICULTY);
        setDifficulty(current.difficulty + DIFFICULTY_STEP);
        return;
    }
    if (contains(x, y, handle.x, handle.y, handle.w, handle.h)) {
        focusRow(ROW_DIFFICULTY);
        dragging = true;
        return;
    }
    if (contains(x, y, track.x, track.y, track.w, track.h)) {
        focusRow(ROW_DIFFICULTY);
        setDifficulty(current.difficulty + (x < handle.x ? -DIFFICULTY_JUMP : DIFFICULTY_JUMP));
        return;
    }

    for (int row = ROW_GENERAL_SUBTITLES; row <= ROW_SAVE_TRAVEL; ++row) {
        const Rect r = toggleRect(row);
        if (contains(x, y, r.x, r.y, r.w, r.h)) {
            activateRow(row);
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

void GameplayOptionsUI::onTouchMove(float x) {
    if (visible && dragging) {
        setDifficultyFromX(x);
    }
}

void GameplayOptionsUI::onTouchUp() {
    dragging = false;
}

bool GameplayOptionsUI::onKeyPress(int keyCode) {
    if (!visible) {
        return false;
    }

    const bool onSlider = focusIndex == ROW_DIFFICULTY;
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
                setDifficulty(current.difficulty + (keyCode == KEY_DPAD_LEFT ? -DIFFICULTY_STEP : DIFFICULTY_STEP));
            } else if (focusIndex == ROW_DEFAULTS || focusIndex == ROW_RETURN) {
                // Defaults and Return are neighbours on the same row.
                focusRow(focusIndex == ROW_DEFAULTS ? ROW_RETURN : ROW_DEFAULTS);
            }
            return true;
        case KEY_BUTTON_L1:
            if (onSlider) {
                setDifficulty(current.difficulty - DIFFICULTY_JUMP);
            }
            return true;
        case KEY_BUTTON_R1:
            if (onSlider) {
                setDifficulty(current.difficulty + DIFFICULTY_JUMP);
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

bool GameplayOptionsUI::handleBack() {
    if (!visible) {
        return false;
    }
    returnToOptions = true;
    close();
    return true;
}

void GameplayOptionsUI::renderBorder(const Rect& b) {
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

void GameplayOptionsUI::renderSlider() {
    const int sw = screenWidth;
    const int sh = screenHeight;
    const Rect body = bodyRect();
    const Rect line = sliderLineRect();

    if (focusIndex == ROW_DIFFICULTY) {
        const float markerX = line.x - 60.0f;
        const float markerY = body.y + labelLocalY(ROW_DIFFICULTY) + LABEL_TO_MARKER_DY;
        drawIfLoaded(selectionFullTexture, markerX, markerY, LINE_WIDTH + MARKER_WIDTH_EXTRA, MARKER_HEIGHT, sw, sh);
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

    const Rect handle = sliderHandleRect();
    drawCropped(scrollPartsTexture, handle.x, handle.y, handle.w, handle.h,
                64.0f, 0.0f, 39.0f, 34.0f, PARTS_W, PARTS_H, sw, sh);
}

void GameplayOptionsUI::renderToggle(int row) {
    if (!textRenderer) {
        return;
    }
    const bool focused = (focusIndex == row);
    const char* value = getToggle(row) ? "On" : "Off";

    FontScope font(textRenderer, LABEL_FONT);
    const float textW = textRenderer->getTextWidth(value, LABEL_SCALE);
    const float textH = textRenderer->getTextHeight(LABEL_SCALE);
    const float barW = std::max(textW + TOGGLE_TEXT_PAD, TOGGLE_MIN_WIDTH);

    const Rect r = toggleRect(row);
    const float centerX = r.x + r.w * 0.5f;
    if (focused) {
        // Selection bar follows the value text width; the cap overlaps its right end.
        drawIfLoaded(selectionFullTexture, centerX - barW * 0.5f, r.y, barW, TOGGLE_HEIGHT, screenWidth, screenHeight);
        drawIfLoaded(selectionCutTexture, centerX + barW * 0.5f + 8.0f - CUT_WIDTH, r.y, CUT_WIDTH, CUT_HEIGHT,
                     screenWidth, screenHeight);
    }
    textRenderer->renderText(value, centerX - textW * 0.5f, r.y + (TOGGLE_HEIGHT - textH) * 0.5f,
                             focused ? kTextColor : kIdleText, LABEL_SCALE);
}

void GameplayOptionsUI::renderButton(int row, const char* label) {
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

void GameplayOptionsUI::render() {
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
        const Rect line = sliderLineRect();
        const float blockX = line.x - LABEL_TO_SCROLLBAR_BUFFER - ARROW_LEFT_WIDTH - longLabelWidth;
        for (int i = 0; i < static_cast<int>(sizeof(kLabels) / sizeof(kLabels[0])); ++i) {
            textRenderer->renderText(kLabels[i], blockX, body.y + labelLocalY(i), kTextColor, LABEL_SCALE);
        }
    }

    renderSlider();
    for (int row = ROW_GENERAL_SUBTITLES; row <= ROW_SAVE_TRAVEL; ++row) {
        renderToggle(row);
    }

    renderButton(ROW_DEFAULTS, "Defaults");
    renderButton(ROW_RETURN, "Return");
}
