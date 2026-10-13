#include "controls_options_ui.h"
#include "ui_draw_helper.h"
#include "../engine/texture_loader.h"
#include <algorithm>
#include <cmath>
#include <android/log.h>

#undef LOG_TAG
#undef LOGD
#undef LOGI
#undef LOGE
#define LOG_TAG "ControlsOptionsUI"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {
// ControlsMenu values from menus/options/controls_menu.xml (PC variant).
// All coordinates are relative to the 666x580 body unless noted.
constexpr float BODY_WIDTH = 666.0f;
constexpr float BODY_HEIGHT = 580.0f;
constexpr float BORDER = 44.0f;
constexpr float BUTTON_BUFFER = 50.0f;        // _button_buffer

// Page 1: horizontal sensitivity slider (horiz_floating_scroll.xml).
constexpr float SENS_LABEL_Y = 96.0f;
constexpr float LINE_WIDTH = 400.0f;
constexpr float LINE_HEIGHT = 16.0f;
constexpr float LEFT_CAP_WIDTH = 50.0f;
constexpr float RIGHT_CAP_WIDTH = 51.0f;
constexpr float LEFT_CAP_DX = -49.0f;
constexpr float RIGHT_CAP_DX = 399.0f;
constexpr float CAP_DY = -11.0f;
constexpr float LEFT_CAP_HEIGHT = 37.0f;
constexpr float RIGHT_CAP_HEIGHT = 42.0f;
constexpr float TRACK_DY = -11.0f;
constexpr float TRACK_HEIGHT = 37.0f;
constexpr float HANDLE_WIDTH = 39.0f;
constexpr float HANDLE_HEIGHT = 34.0f;
constexpr float HANDLE_DY = -10.0f;
constexpr float MARKER_WIDTH_EXTRA = 120.0f;
constexpr float MARKER_HEIGHT = 64.0f;
constexpr float MARKER_CUT_DX = -60.0f;
constexpr float MARKER_CUT_OFFSET = 420.0f;   // parent.width - 100
constexpr float CUT_WIDTH = 104.0f;
constexpr float CUT_HEIGHT = 64.0f;
constexpr float LABEL_TO_SLIDER_DY = 12.0f;
constexpr float LABEL_TO_BLOCK_DY = 96.0f;    // invert-Y / left-stick blocks
constexpr int SENSITIVITY_MIN = 0;
constexpr int SENSITIVITY_MAX = 100;
constexpr int SENSITIVITY_STEP = 1;           // XML user3 = 1 on PC
constexpr int SENSITIVITY_JUMP = 10;          // XML user4 = 10

// Page 1: floating toggles (button_floating_2.xml).
constexpr float FLOAT_TOGGLE_HEIGHT = 64.0f;
constexpr float FLOAT_TOGGLE_MIN_WIDTH = 104.0f;
constexpr float FLOAT_TOGGLE_PAD = 40.0f;
constexpr float FLOAT_TOGGLE_TEXT_DY = -12.0f; // _top = label.y - 12
constexpr float INVERT_BUTTON_RIGHT_PAD = 32.0f; // _center_x = label.x + label.w - 32

// Page 1: long buttons (button_long.xml).
constexpr float BUTTON_WIDTH = 177.0f;
constexpr float BUTTON_HEIGHT = 44.0f;
constexpr float BUTTON_MARGIN_Y = 25.0f;

// Page 2: list frame.
constexpr float MAIN_WIDTH = 538.0f;
constexpr float MAIN_HEIGHT = 400.0f;
constexpr float MAIN_Y = 75.0f;
constexpr float BORDER_TOP_HEIGHT = 6.0f;
constexpr float BORDER_SIDE_WIDTH = 4.0f;
constexpr float BORDER_RIGHT_X = MAIN_WIDTH - 32.0f;  // 506
constexpr float BORDER_TOP_DX = -24.0f;
constexpr float BORDER_LEFT_DX = -4.0f;
constexpr float BORDER_MIDDLE_X = 300.0f;
constexpr float ROW_LABEL_DY = 12.0f;
constexpr float ROW_BUTTON_FROM_RIGHT = 128.0f;  // user2 = parent.width - 128
constexpr float ROW_LEFT_BUFFER = 10.0f;         // _left_edge_buffer
constexpr float ROW_HIGHLIGHT_INSET = 32.0f;     // width = parent.width - 32
constexpr int VISIBLE_ROWS = 6;                  // user8 = 6

// Page 2: vertical scroll bar (vert_floating_scroll.xml).
constexpr float SCROLL_TOP_DX = -11.0f;
constexpr float SCROLL_TOP_DY = -50.0f;
constexpr float SCROLL_TOP_WIDTH = 37.0f;
constexpr float SCROLL_TOP_HEIGHT = 50.0f;
constexpr float SCROLL_BOTTOM_WIDTH = 42.0f;
constexpr float SCROLL_BOTTOM_HEIGHT = 51.0f;
constexpr float SCROLL_MARKER_DX = -10.0f;
constexpr float SCROLL_MARKER_WIDTH = 34.0f;
constexpr float SCROLL_MARKER_HEIGHT = 39.0f;
constexpr float SCROLL_LINE_WIDTH = 16.0f;
constexpr int SCROLL_STEP = 1;                   // user3
constexpr int SCROLL_JUMP = 5;                   // user4

// Page 2: device selector.
constexpr float DEVICE_ARROW_HEIGHT = 36.0f;
constexpr float DEVICE_ARROW_LEFT_WIDTH = 124.0f;
constexpr float DEVICE_ARROW_RIGHT_WIDTH = 121.0f;
constexpr float DEVICE_BUFFER = 10.0f;           // _buffer
constexpr float DEVICE_TEXT_Y = 20.0f;

constexpr float DIM_ALPHA = 0.6f;
constexpr float LABEL_SCALE = 1.0f;
constexpr FontType LABEL_FONT = FontType::KingthingsRegular;  // XML font 1

const glm::vec4 kTextColor(117.0f / 255.0f, 59.0f / 255.0f, 33.0f / 255.0f, 1.0f);
const glm::vec4 kIdleText(128.0f / 255.0f, 128.0f / 255.0f, 128.0f / 255.0f, 196.0f / 255.0f);

// English strings taken verbatim from Menus\strings.xml of the retail game.
constexpr const char* kStrMouseSensitivity = "Mouse Sensitivity:";
constexpr const char* kStrInvertY = "Invert Y:";
constexpr const char* kStrSwapSticks = "Swap Stick Controls";
constexpr const char* kStrLeftStick = "Left Stick:";
constexpr const char* kStrRightStick = "Right Stick:";
constexpr const char* kStrMove = "Move";
constexpr const char* kStrLook = "Look";
constexpr const char* kStrDefaults = "Defaults";
constexpr const char* kStrControls = "Controls";
constexpr const char* kStrReturn = "Return";

// Device names are code-set in the original (ControlsMenu.user5).
const char* const kDeviceNames[] = {"Touch Screen", "Virtual Joystick", "Gamepad"};
constexpr int DEVICE_COUNT = 3;

// Row contents are generated in code by the original (controls_list_pane).
const char* const kRowLabels[] = {
    "Move Forward:", "Move Backward:", "Move Left:", "Move Right:",
    "Activate:", "Jump:", "Sneak:", "Run:",
    "Ready/Sheathe Weapon:", "Cast Spell:", "Block:", "Attack:",
    "Toggle POV:", "Wait:", "Journal:", "Quick Menu:",
    "Take All:", "Toggle Always Run:",
};
const char* const kRowBindings[] = {
    "Up", "Down", "Left", "Right",
    "Center", "A", "L1", "L3",
    "R1", "R2", "L2", "Y",
    "R3", "Back", "X", "B",
    "Select", "Start",
};
constexpr int ROW_COUNT = static_cast<int>(sizeof(kRowLabels) / sizeof(kRowLabels[0]));

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

// Restricts drawing to a rectangle so the list rows are clipped by the frame.
class ScissorScope {
public:
    ScissorScope(float x, float y, float w, float h, int screen_h) {
        glGetIntegerv(GL_SCISSOR_BOX, previous_);
        was_enabled_ = glIsEnabled(GL_SCISSOR_TEST) == GL_TRUE;
        glEnable(GL_SCISSOR_TEST);
        glScissor(static_cast<GLint>(x), static_cast<GLint>(screen_h - (y + h)),
                  static_cast<GLsizei>(std::max(0.0f, w)), static_cast<GLsizei>(std::max(0.0f, h)));
    }
    ~ScissorScope() {
        glScissor(previous_[0], previous_[1], previous_[2], previous_[3]);
        if (!was_enabled_) {
            glDisable(GL_SCISSOR_TEST);
        }
    }

private:
    GLint previous_[4] = {0, 0, 0, 0};
    bool was_enabled_ = false;
};
}  // namespace

ControlsOptionsUI::ControlsOptionsUI() = default;

ControlsOptionsUI::~ControlsOptionsUI() {
    cleanup();
}

bool ControlsOptionsUI::initialize(TextRenderer* text_renderer) {
    if (!text_renderer) {
        LOGE("Error: TextRenderer is null");
        return false;
    }
    text_renderer_ = text_renderer;

    if (tex_generic_bg_ == 0) {
        const std::string dir = "textures/ui/menus/";
        tex_generic_bg_ = TextureLoader::loadTextureFromAsset(dir + "genericbackground__center_background.png");
        tex_bg_edge_top_ = TextureLoader::loadTextureFromAsset(dir + "genericbackground__edge_top.png");
        tex_bg_edge_bottom_ = TextureLoader::loadTextureFromAsset(dir + "genericbackground__edge_bottom.png");
        tex_bg_edge_left_ = TextureLoader::loadTextureFromAsset(dir + "genericbackground__edge_left.png");
        tex_bg_edge_right_ = TextureLoader::loadTextureFromAsset(dir + "genericbackground__edge_right.png");
        tex_bg_corner_tl_ = TextureLoader::loadTextureFromAsset(dir + "genericbackground__top_left_corner.png");
        tex_bg_corner_tr_ = TextureLoader::loadTextureFromAsset(dir + "genericbackground__top_right_corner.png");
        tex_bg_corner_bl_ = TextureLoader::loadTextureFromAsset(dir + "genericbackground__bottom_left_corner.png");
        tex_bg_corner_br_ = TextureLoader::loadTextureFromAsset(dir + "genericbackground__bottom_right_corner.png");
        tex_selection_full_ = TextureLoader::loadTextureFromAsset(dir + "dialog__dialog_selection_full.png");
        tex_selection_cut_ = TextureLoader::loadTextureFromAsset(dir + "dialog__dialog_selection_cut.png");
        tex_scroll_line_ = TextureLoader::loadTextureFromAsset(dir + "shared__horizontal_scrollbar_line.png");
        tex_scroll_parts_ = TextureLoader::loadTextureFromAsset(dir + "shared__horizontal_scrollbar_parts.png");
        tex_scrollbar_up_ = TextureLoader::loadTextureFromAsset(dir + "dialog__dialog_scrollbar_up.png");
        tex_scrollbar_down_ = TextureLoader::loadTextureFromAsset(dir + "dialog__dialog_scrollbar_down.png");
        tex_scrollbar_line_ = TextureLoader::loadTextureFromAsset(dir + "dialog__dialog_scrollbar_line.png");
        tex_scrollbar_marker_ = TextureLoader::loadTextureFromAsset(dir + "dialog__dialog_scrollbar_marker.png");
        tex_button_off_ = TextureLoader::loadTextureFromAsset(dir + "shared__shared_button_long_off.png");
        tex_button_on_ = TextureLoader::loadTextureFromAsset(dir + "shared__shared_button_long_on.png");
        tex_arrow_left_ = TextureLoader::loadTextureFromAsset(dir + "shared__shared_arrow_left.png");
        tex_arrow_right_ = TextureLoader::loadTextureFromAsset(dir + "shared__shared_arrow_right.png");
        tex_border_h_ = TextureLoader::loadTextureFromAsset(dir + "shared__shared_border_horizontal_1.png");
        tex_border_v_ = TextureLoader::loadTextureFromAsset(dir + "shared__shared_border_vertical_2.png");
    }

    buildRows();
    measureLabels();
    LOGI("ControlsOptionsUI initialized");
    return true;
}

void ControlsOptionsUI::cleanup() {
    for (GLuint* tex : {&tex_generic_bg_, &tex_bg_edge_top_, &tex_bg_edge_bottom_,
                        &tex_bg_edge_left_, &tex_bg_edge_right_,
                        &tex_bg_corner_tl_, &tex_bg_corner_tr_,
                        &tex_bg_corner_bl_, &tex_bg_corner_br_,
                        &tex_selection_full_, &tex_selection_cut_,
                        &tex_scroll_line_, &tex_scroll_parts_,
                        &tex_scrollbar_up_, &tex_scrollbar_down_,
                        &tex_scrollbar_line_, &tex_scrollbar_marker_,
                        &tex_button_off_, &tex_button_on_,
                        &tex_arrow_left_, &tex_arrow_right_,
                        &tex_border_h_, &tex_border_v_}) {
        if (*tex != 0) {
            TextureLoader::deleteTexture(*tex);
            *tex = 0;
        }
    }
    text_renderer_ = nullptr;
    visible_ = false;
}

void ControlsOptionsUI::setScreenSize(int width, int height) {
    screen_width_ = width;
    screen_height_ = height;
}

void ControlsOptionsUI::open(const Settings& initial) {
    settings_ = initial;
    settings_.sensitivity = std::min(1.0f, std::max(0.0f, settings_.sensitivity));
    settings_.device_index = std::min(DEVICE_COUNT - 1, std::max(0, settings_.device_index));
    visible_ = true;
    return_to_options_ = false;
    changed_ = false;
    dragging_ = false;
    page_ = 1;
    focus_ = P1_SENSITIVITY;
    page2_focus_ = 0;
    scroll_row_ = 0;
    measureLabels();
}

void ControlsOptionsUI::close() {
    visible_ = false;
    dragging_ = false;
}

void ControlsOptionsUI::setPage(int page) {
    page_ = (page == 2) ? 2 : 1;
    focus_ = P1_SENSITIVITY;
    page2_focus_ = 0;
    ensureRowVisible(0);
}

void ControlsOptionsUI::buildRows() {
    rows_.clear();
    rows_.reserve(ROW_COUNT);
    for (int i = 0; i < ROW_COUNT; ++i) {
        rows_.push_back(Row{kRowLabels[i], kRowBindings[i]});
    }
}

void ControlsOptionsUI::applyDefaults() {
    const Settings defaults;
    settings_.sensitivity = defaults.sensitivity;
    settings_.invert_y = defaults.invert_y;
    settings_.swap_axes = defaults.swap_axes;
    changed_ = true;
    LOGI("ControlsOptionsUI: defaults restored");
}

void ControlsOptionsUI::measureLabels() {
    if (!text_renderer_) {
        return;
    }
    FontScope font(text_renderer_, LABEL_FONT);
    label_height_ = text_renderer_->getTextHeight(LABEL_SCALE);
    sensitivity_label_width_ = text_renderer_->getTextWidth(kStrMouseSensitivity, LABEL_SCALE);
    action_label_width_ = text_renderer_->getTextWidth(kStrInvertY, LABEL_SCALE);
}

bool ControlsOptionsUI::consumeChanged() {
    const bool was = changed_;
    changed_ = false;
    return was;
}

// ---------------------------------------------------------------- geometry

ControlsOptionsUI::Rect ControlsOptionsUI::bodyRect() const {
    return Rect{(screen_width_ - BODY_WIDTH) * 0.5f,
                (screen_height_ - BODY_HEIGHT) * 0.5f,
                BODY_WIDTH, BODY_HEIGHT};
}

ControlsOptionsUI::Rect ControlsOptionsUI::sensitivityLabelRect() const {
    const Rect body = bodyRect();
    const Rect slider = sliderLineRect();
    return Rect{slider.x + (slider.w - sensitivity_label_width_) * 0.5f,
                body.y + SENS_LABEL_Y, sensitivity_label_width_, label_height_};
}

ControlsOptionsUI::Rect ControlsOptionsUI::sliderLineRect() const {
    const Rect body = bodyRect();
    // x = left.width + (parent.width - me.width - left.width - right.width) / 2
    const float x = body.x + LEFT_CAP_WIDTH +
                    (BODY_WIDTH - LINE_WIDTH - LEFT_CAP_WIDTH - RIGHT_CAP_WIDTH) * 0.5f;
    const float y = body.y + SENS_LABEL_Y + label_height_ + LABEL_TO_SLIDER_DY;
    return Rect{x, y, LINE_WIDTH, LINE_HEIGHT};
}

ControlsOptionsUI::Rect ControlsOptionsUI::sliderHandleRect() const {
    const Rect line = sliderLineRect();
    const float travel = LINE_WIDTH - HANDLE_WIDTH;
    return Rect{line.x + travel * settings_.sensitivity, line.y + HANDLE_DY,
                HANDLE_WIDTH, HANDLE_HEIGHT};
}

ControlsOptionsUI::Rect ControlsOptionsUI::invertYLabelRect() const {
    const Rect label = sensitivityLabelRect();
    return Rect{label.x, label.y + label_height_ + LABEL_TO_BLOCK_DY,
                action_label_width_, label_height_};
}

ControlsOptionsUI::Rect ControlsOptionsUI::stickLabelRect(int which) const {
    const Rect invert = invertYLabelRect();
    const float y = (which == 0) ? invert.y + label_height_ + LABEL_TO_BLOCK_DY
                                 : stickLabelRect(0).y + label_height_;
    const float w = text_renderer_
        ? text_renderer_->getTextWidth(which == 0 ? kStrLeftStick : kStrRightStick, LABEL_SCALE)
        : 0.0f;
    return Rect{invert.x, y, w, label_height_};
}

ControlsOptionsUI::Rect ControlsOptionsUI::invertYButtonRect() const {
    const Rect label = invertYLabelRect();
    const float center_x = label.x + action_label_width_ - INVERT_BUTTON_RIGHT_PAD;
    const float text_w = text_renderer_
        ? std::max(text_renderer_->getTextWidth("On", LABEL_SCALE),
                   text_renderer_->getTextWidth("Off", LABEL_SCALE))
        : 0.0f;
    const float w = std::max(text_w + FLOAT_TOGGLE_PAD, FLOAT_TOGGLE_MIN_WIDTH);
    return Rect{center_x - w * 0.5f, label.y + FLOAT_TOGGLE_TEXT_DY, w, FLOAT_TOGGLE_HEIGHT};
}

ControlsOptionsUI::Rect ControlsOptionsUI::swapButtonRect() const {
    const Rect label = stickLabelRect(0);
    const float text_w = text_renderer_
        ? text_renderer_->getTextWidth(kStrSwapSticks, LABEL_SCALE) : 0.0f;
    const float w = std::max(text_w + FLOAT_TOGGLE_PAD, FLOAT_TOGGLE_MIN_WIDTH);
    const Rect body = bodyRect();
    return Rect{body.x + BODY_WIDTH * 0.5f - w * 0.5f,
                label.y - FLOAT_TOGGLE_HEIGHT, w, FLOAT_TOGGLE_HEIGHT};
}

ControlsOptionsUI::Rect ControlsOptionsUI::stickSettingRect(int which) const {
    const Rect invert_button = invertYButtonRect();
    const Rect label = stickLabelRect(which);
    const float center_x = invert_button.x + invert_button.w * 0.5f;
    const float text_w = text_renderer_
        ? text_renderer_->getTextWidth(which == 0 ? kStrMove : kStrLook, LABEL_SCALE) : 0.0f;
    return Rect{center_x - text_w * 0.5f, label.y, text_w, label_height_};
}

ControlsOptionsUI::Rect ControlsOptionsUI::page1ReturnRect() const {
    const Rect body = bodyRect();
    return Rect{body.x + BODY_WIDTH - BUTTON_WIDTH - BUTTON_BUFFER,
                body.y + BODY_HEIGHT - BUTTON_HEIGHT - BUTTON_MARGIN_Y,
                BUTTON_WIDTH, BUTTON_HEIGHT};
}

ControlsOptionsUI::Rect ControlsOptionsUI::longButtonRect(int which) const {
    const Rect ret = page1ReturnRect();
    if (which == 0) {   // defaults / more - both sit at x = _button_buffer
        return Rect{bodyRect().x + BUTTON_BUFFER, ret.y, BUTTON_WIDTH, BUTTON_HEIGHT};
    }
    return ret;
}

ControlsOptionsUI::Rect ControlsOptionsUI::mainRect() const {
    const Rect body = bodyRect();
    return Rect{body.x + (BODY_WIDTH - MAIN_WIDTH) * 0.5f, body.y + MAIN_Y,
                MAIN_WIDTH, MAIN_HEIGHT};
}

ControlsOptionsUI::Rect ControlsOptionsUI::scrollLineRect() const {
    const Rect main = mainRect();
    // controls_vertical_scroll: x = parent.width, y = top.h, h = parent.h - top.h - bottom.h
    return Rect{main.x + MAIN_WIDTH, main.y + BORDER_TOP_HEIGHT, SCROLL_LINE_WIDTH,
                MAIN_HEIGHT - BORDER_TOP_HEIGHT * 2.0f};
}

ControlsOptionsUI::Rect ControlsOptionsUI::scrollTopRect() const {
    const Rect line = scrollLineRect();
    return Rect{line.x + SCROLL_TOP_DX, line.y + SCROLL_TOP_DY,
                SCROLL_TOP_WIDTH, SCROLL_TOP_HEIGHT};
}

ControlsOptionsUI::Rect ControlsOptionsUI::scrollBottomRect() const {
    const Rect line = scrollLineRect();
    return Rect{line.x + SCROLL_TOP_DX, line.y + line.h,
                SCROLL_BOTTOM_WIDTH, SCROLL_BOTTOM_HEIGHT};
}

ControlsOptionsUI::Rect ControlsOptionsUI::scrollMarkerRect() const {
    const Rect line = scrollLineRect();
    const int max_row = std::max(0, static_cast<int>(rows_.size()) - VISIBLE_ROWS);
    const float ratio = (max_row > 0)
        ? static_cast<float>(scroll_row_) / static_cast<float>(max_row) : 0.0f;
    return Rect{line.x + SCROLL_MARKER_DX,
                line.y + (line.h - SCROLL_MARKER_HEIGHT) * ratio,
                SCROLL_MARKER_WIDTH, SCROLL_MARKER_HEIGHT};
}

float ControlsOptionsUI::listRowLocalY(int row) const {
    const float row_h = MAIN_HEIGHT / static_cast<float>(VISIBLE_ROWS);
    return -scroll_row_ * row_h + row * row_h;
}

ControlsOptionsUI::Rect ControlsOptionsUI::listRowRect(int row) const {
    const Rect main = mainRect();
    const float row_h = MAIN_HEIGHT / static_cast<float>(VISIBLE_ROWS);
    return Rect{main.x, main.y + listRowLocalY(row), MAIN_WIDTH, row_h};
}

ControlsOptionsUI::Rect ControlsOptionsUI::deviceToggleLeftRect() const {
    const Rect main = mainRect();
    const float x = main.x + BORDER_LEFT_DX + DEVICE_BUFFER;
    const Rect text = deviceTextRect();
    return Rect{x, text.y + (text.h - DEVICE_ARROW_HEIGHT) * 0.5f,
                DEVICE_ARROW_LEFT_WIDTH, DEVICE_ARROW_HEIGHT};
}

ControlsOptionsUI::Rect ControlsOptionsUI::deviceTextRect() const {
    const Rect body = bodyRect();
    const float h = text_renderer_
        ? text_renderer_->getTextHeight(LABEL_SCALE) : label_height_;
    return Rect{body.x, body.y + DEVICE_TEXT_Y, 0.0f, h};
}

ControlsOptionsUI::Rect ControlsOptionsUI::deviceToggleRightRect() const {
    const Rect main = mainRect();
    const Rect left = deviceToggleLeftRect();
    return Rect{main.x + BORDER_RIGHT_X - DEVICE_ARROW_RIGHT_WIDTH - DEVICE_BUFFER,
                left.y, DEVICE_ARROW_RIGHT_WIDTH, DEVICE_ARROW_HEIGHT};
}

bool ControlsOptionsUI::pointInRect(float px, float py, const Rect& rect) const {
    return contains(px, py, rect.x, rect.y, rect.w, rect.h);
}

int ControlsOptionsUI::listRowAtPoint(float px, float py) const {
    const Rect main = mainRect();
    if (!contains(px, py, main.x, main.y, main.w, main.h)) {
        return -1;
    }
    const float row_h = MAIN_HEIGHT / static_cast<float>(VISIBLE_ROWS);
    const int index = static_cast<int>(std::floor((py - main.y - listRowLocalY(0)) / row_h));
    if (index < 0 || index >= static_cast<int>(rows_.size())) {
        return -1;
    }
    return index;
}

// ---------------------------------------------------------------- actions

void ControlsOptionsUI::setSliderFromX(float screen_x) {
    const Rect line = sliderLineRect();
    const float travel = LINE_WIDTH - HANDLE_WIDTH;
    const float ratio = (screen_x - line.x - HANDLE_WIDTH * 0.5f) / travel;
    const float value = std::min(1.0f, std::max(0.0f, ratio));
    if (std::fabs(value - settings_.sensitivity) > 0.0005f) {
        settings_.sensitivity = value;
        changed_ = true;
    }
}

void ControlsOptionsUI::adjustSlider(float delta) {
    const float value = std::min(1.0f, std::max(0.0f, settings_.sensitivity + delta));
    if (value != settings_.sensitivity) {
        settings_.sensitivity = value;
        changed_ = true;
    }
}

void ControlsOptionsUI::adjustDevice(int delta) {
    const int next = (settings_.device_index + delta + DEVICE_COUNT) % DEVICE_COUNT;
    if (next != settings_.device_index) {
        settings_.device_index = next;
        changed_ = true;
    }
}

void ControlsOptionsUI::ensureRowVisible(int row) {
    const int max_row = std::max(0, static_cast<int>(rows_.size()) - VISIBLE_ROWS);
    scroll_row_ = std::min(max_row, std::max(0, row - VISIBLE_ROWS + 1));
    if (row < scroll_row_) {
        scroll_row_ = row;
    }
    scroll_row_ = std::min(max_row, std::max(0, scroll_row_));
}

void ControlsOptionsUI::moveFocus(int delta) {
    if (page_ == 1) {
        focus_ = (focus_ + delta + P1_COUNT) % P1_COUNT;
        return;
    }
    const int count = static_cast<int>(rows_.size()) + 3;  // device + rows + defaults + return
    page2_focus_ = (page2_focus_ + delta + count) % count;
    if (page2_focus_ >= 1 && page2_focus_ <= static_cast<int>(rows_.size())) {
        ensureRowVisible(page2_focus_ - 1);
    }
}

void ControlsOptionsUI::activateFocus() {
    if (page_ == 1) {
        switch (focus_) {
            case P1_INVERT_Y:
                settings_.invert_y = !settings_.invert_y;
                changed_ = true;
                break;
            case P1_SWAP:
                settings_.swap_axes = !settings_.swap_axes;
                changed_ = true;
                break;
            case P1_MORE:
                setPage(2);
                break;
            case P1_RETURN:
                return_to_options_ = true;
                close();
                break;
            default:
                break;
        }
        return;
    }

    const int row_count = static_cast<int>(rows_.size());
    if (page2_focus_ == 0) {
        adjustDevice(1);
    } else if (page2_focus_ <= row_count) {
        // Binding rows open the assignment flow in the original; the Android
        // port keeps the default binding, so only the selection moves.
    } else if (page2_focus_ == row_count + 1) {
        applyDefaults();
    } else {
        return_to_options_ = true;
        close();
    }
}

// ---------------------------------------------------------------- input

bool ControlsOptionsUI::onTouchDown(float x, float y) {
    if (!visible_) {
        return false;
    }

    if (page_ == 1) {
        const Rect line = sliderLineRect();
        const Rect left_cap{line.x + LEFT_CAP_DX, line.y + CAP_DY, LEFT_CAP_WIDTH, LEFT_CAP_HEIGHT};
        const Rect right_cap{line.x + RIGHT_CAP_DX, line.y + CAP_DY, RIGHT_CAP_WIDTH, RIGHT_CAP_HEIGHT};
        const Rect track{line.x, line.y + TRACK_DY, LINE_WIDTH, TRACK_HEIGHT};
        const Rect handle = sliderHandleRect();

        if (pointInRect(x, y, left_cap)) {
            focus_ = P1_SENSITIVITY;
            adjustSlider(-static_cast<float>(SENSITIVITY_STEP) / SENSITIVITY_MAX);
            return true;
        }
        if (pointInRect(x, y, right_cap)) {
            focus_ = P1_SENSITIVITY;
            adjustSlider(static_cast<float>(SENSITIVITY_STEP) / SENSITIVITY_MAX);
            return true;
        }
        if (pointInRect(x, y, handle)) {
            focus_ = P1_SENSITIVITY;
            dragging_ = true;
            return true;
        }
        if (pointInRect(x, y, track)) {
            focus_ = P1_SENSITIVITY;
            adjustSlider((x < handle.x ? -static_cast<float>(SENSITIVITY_JUMP)
                                       : static_cast<float>(SENSITIVITY_JUMP)) / SENSITIVITY_MAX);
            return true;
        }
        if (pointInRect(x, y, invertYButtonRect())) {
            focus_ = P1_INVERT_Y;
            settings_.invert_y = !settings_.invert_y;
            changed_ = true;
            return true;
        }
        if (pointInRect(x, y, swapButtonRect())) {
            focus_ = P1_SWAP;
            settings_.swap_axes = !settings_.swap_axes;
            changed_ = true;
            return true;
        }
        if (pointInRect(x, y, longButtonRect(0))) {
            focus_ = P1_MORE;
            setPage(2);
            return true;
        }
        if (pointInRect(x, y, page1ReturnRect())) {
            focus_ = P1_RETURN;
            return_to_options_ = true;
            close();
            return true;
        }
        return false;
    }

    // Page 2
    const Rect device_left = deviceToggleLeftRect();
    const Rect device_right = deviceToggleRightRect();
    const Rect device_highlight{mainRect().x, device_left.y - DEVICE_BUFFER,
                                MAIN_WIDTH, DEVICE_ARROW_HEIGHT + DEVICE_BUFFER * 2.0f};
    if (pointInRect(x, y, device_left)) {
        page2_focus_ = 0;
        adjustDevice(-1);
        return true;
    }
    if (pointInRect(x, y, device_right)) {
        page2_focus_ = 0;
        adjustDevice(1);
        return true;
    }

    const Rect scroll_top = scrollTopRect();
    const Rect scroll_bottom = scrollBottomRect();
    const Rect scroll_line = scrollLineRect();
    if (pointInRect(x, y, scroll_top)) {
        scroll_row_ = std::max(0, scroll_row_ - SCROLL_STEP);
        return true;
    }
    if (pointInRect(x, y, scroll_bottom)) {
        const int max_row = std::max(0, static_cast<int>(rows_.size()) - VISIBLE_ROWS);
        scroll_row_ = std::min(max_row, scroll_row_ + SCROLL_STEP);
        return true;
    }
    if (pointInRect(x, y, scroll_line)) {
        const int max_row = std::max(0, static_cast<int>(rows_.size()) - VISIBLE_ROWS);
        scroll_row_ = std::min(max_row, scroll_row_ + SCROLL_JUMP);
        return true;
    }

    const int row = listRowAtPoint(x, y);
    if (row >= 0) {
        page2_focus_ = row + 1;
        ensureRowVisible(row);
        return true;
    }

    if (pointInRect(x, y, device_highlight)) {
        page2_focus_ = 0;
        return true;
    }
    if (pointInRect(x, y, longButtonRect(0))) {
        page2_focus_ = static_cast<int>(rows_.size()) + 1;
        applyDefaults();
        return true;
    }
    if (pointInRect(x, y, longButtonRect(1))) {
        page2_focus_ = static_cast<int>(rows_.size()) + 2;
        return_to_options_ = true;
        close();
        return true;
    }
    return false;
}

bool ControlsOptionsUI::onTouchMove(float x) {
    if (!visible_ || !dragging_) {
        return false;
    }
    setSliderFromX(x);
    return true;
}

bool ControlsOptionsUI::onTouchUp() {
    dragging_ = false;
    return true;
}

bool ControlsOptionsUI::onKeyPress(int key_code) {
    if (!visible_) {
        return false;
    }

    switch (key_code) {
        case KEY_DPAD_UP:
            moveFocus(-1);
            return true;
        case KEY_DPAD_DOWN:
            moveFocus(1);
            return true;
        case KEY_DPAD_LEFT:
            if (page_ == 1) {
                focus_ = P1_SENSITIVITY;
                adjustSlider(-static_cast<float>(SENSITIVITY_JUMP) / SENSITIVITY_MAX);
            } else if (page2_focus_ == 0) {
                adjustDevice(-1);
            }
            return true;
        case KEY_DPAD_RIGHT:
            if (page_ == 1) {
                focus_ = P1_SENSITIVITY;
                adjustSlider(static_cast<float>(SENSITIVITY_JUMP) / SENSITIVITY_MAX);
            } else if (page2_focus_ == 0) {
                adjustDevice(1);
            }
            return true;
        case KEY_DPAD_CENTER:
        case KEY_ENTER:
        case KEY_BUTTON_B:
        case KEY_BUTTON_Y:
        case KEY_BUTTON_L1:
        case KEY_BUTTON_R1:
            activateFocus();
            return true;
        case KEY_BACK:
            return handleBack();
        default:
            return false;
    }
}

bool ControlsOptionsUI::handleBack() {
    if (!visible_) {
        return false;
    }
    if (page_ == 2) {
        setPage(1);
        return true;
    }
    return_to_options_ = true;
    close();
    return true;
}

// ---------------------------------------------------------------- render

void ControlsOptionsUI::renderBackground() {
    const Rect body = bodyRect();
    const int sw = screen_width_;
    const int sh = screen_height_;

    drawIfLoaded(tex_generic_bg_, body.x, body.y, body.w, body.h, sw, sh);
    drawIfLoaded(tex_bg_edge_top_, body.x, body.y - BORDER, body.w, BORDER, sw, sh);
    drawIfLoaded(tex_bg_edge_bottom_, body.x, body.y + body.h, body.w, BORDER, sw, sh);
    drawIfLoaded(tex_bg_edge_left_, body.x - BORDER, body.y, BORDER, body.h, sw, sh);
    drawIfLoaded(tex_bg_edge_right_, body.x + body.w, body.y, BORDER, body.h, sw, sh);
    drawIfLoaded(tex_bg_corner_tl_, body.x - BORDER, body.y - BORDER, BORDER, BORDER, sw, sh);
    drawIfLoaded(tex_bg_corner_tr_, body.x + body.w, body.y - BORDER, BORDER, BORDER, sw, sh);
    drawIfLoaded(tex_bg_corner_bl_, body.x - BORDER, body.y + body.h, BORDER, BORDER, sw, sh);
    drawIfLoaded(tex_bg_corner_br_, body.x + body.w, body.y + body.h, BORDER, BORDER, sw, sh);
}

void ControlsOptionsUI::renderPage1() {
    const int sw = screen_width_;
    const int sh = screen_height_;
    const Rect line = sliderLineRect();

    FontScope font(text_renderer_, LABEL_FONT);

    const Rect sens_label = sensitivityLabelRect();
    text_renderer_->renderText(kStrMouseSensitivity, sens_label.x, sens_label.y,
                               kTextColor, LABEL_SCALE);

    // Focus frame behind the slider (controls_sensitivity_slider_marker).
    if (focus_ == P1_SENSITIVITY) {
        const float marker_x = line.x + MARKER_CUT_DX;
        const float marker_y = line.y - 18.0f;
        drawIfLoaded(tex_selection_full_, marker_x, marker_y,
                     LINE_WIDTH + MARKER_WIDTH_EXTRA, MARKER_HEIGHT, sw, sh);
        drawIfLoaded(tex_selection_cut_, marker_x + MARKER_CUT_OFFSET, marker_y,
                     CUT_WIDTH, CUT_HEIGHT, sw, sh);
    }

    // Slider line + caps + handle (user8 = false hides the numeric readout).
    drawIfLoaded(tex_scroll_line_, line.x, line.y, line.w, line.h, sw, sh);
    constexpr float PARTS_W = 256.0f;
    constexpr float PARTS_H = 64.0f;
    drawCropped(tex_scroll_parts_, line.x + LEFT_CAP_DX, line.y + CAP_DY,
                LEFT_CAP_WIDTH, LEFT_CAP_HEIGHT, 0.0f, 0.0f, 50.0f, 37.0f, PARTS_W, PARTS_H, sw, sh);
    drawCropped(tex_scroll_parts_, line.x + RIGHT_CAP_DX, line.y + CAP_DY,
                RIGHT_CAP_WIDTH, RIGHT_CAP_HEIGHT, 128.0f, 0.0f, 51.0f, 42.0f, PARTS_W, PARTS_H, sw, sh);
    const Rect handle = sliderHandleRect();
    drawCropped(tex_scroll_parts_, handle.x, handle.y, handle.w, handle.h,
                64.0f, 0.0f, 39.0f, 34.0f, PARTS_W, PARTS_H, sw, sh);

    const Rect invert_label = invertYLabelRect();
    text_renderer_->renderText(kStrInvertY, invert_label.x, invert_label.y,
                               kTextColor, LABEL_SCALE);

    // Invert Y toggle (button_floating_2).
    {
        const Rect r = invertYButtonRect();
        const bool focused = (focus_ == P1_INVERT_Y);
        const char* value = settings_.invert_y ? "On" : "Off";
        const float tw = text_renderer_->getTextWidth(value, LABEL_SCALE);
        const float th = text_renderer_->getTextHeight(LABEL_SCALE);
        if (focused) {
            drawIfLoaded(tex_selection_full_, r.x, r.y, r.w, FLOAT_TOGGLE_HEIGHT, sw, sh);
            drawIfLoaded(tex_selection_cut_, r.x + r.w + 8.0f - CUT_WIDTH, r.y,
                         CUT_WIDTH, CUT_HEIGHT, sw, sh);
        }
        text_renderer_->renderText(value, r.x + (r.w - tw) * 0.5f,
                                   r.y + (FLOAT_TOGGLE_HEIGHT - th) * 0.5f,
                                   focused ? kTextColor : kIdleText, LABEL_SCALE);
    }

    const Rect left_label = stickLabelRect(0);
    const Rect right_label = stickLabelRect(1);
    text_renderer_->renderText(kStrLeftStick, left_label.x, left_label.y,
                               kTextColor, LABEL_SCALE);
    text_renderer_->renderText(kStrRightStick, right_label.x, right_label.y,
                               kTextColor, LABEL_SCALE);

    // Swap Stick Controls toggle (centred, above the stick block).
    {
        const Rect r = swapButtonRect();
        const bool focused = (focus_ == P1_SWAP);
        const float tw = text_renderer_->getTextWidth(kStrSwapSticks, LABEL_SCALE);
        const float th = text_renderer_->getTextHeight(LABEL_SCALE);
        if (focused) {
            drawIfLoaded(tex_selection_full_, r.x, r.y, r.w, FLOAT_TOGGLE_HEIGHT, sw, sh);
            drawIfLoaded(tex_selection_cut_, r.x + r.w + 8.0f - CUT_WIDTH, r.y,
                         CUT_WIDTH, CUT_HEIGHT, sw, sh);
        }
        text_renderer_->renderText(kStrSwapSticks, r.x + (r.w - tw) * 0.5f,
                                   r.y + (FLOAT_TOGGLE_HEIGHT - th) * 0.5f,
                                   focused ? kTextColor : kIdleText, LABEL_SCALE);
    }

    const Rect left_setting = stickSettingRect(0);
    const Rect right_setting = stickSettingRect(1);
    text_renderer_->renderText(settings_.swap_axes ? kStrLook : kStrMove,
                               left_setting.x, left_setting.y, kTextColor, LABEL_SCALE);
    text_renderer_->renderText(settings_.swap_axes ? kStrMove : kStrLook,
                               right_setting.x, right_setting.y, kTextColor, LABEL_SCALE);

    // Bottom row: "Controls" (opens page 2) and "Return".
    const Rect more = longButtonRect(0);
    const bool more_focused = (focus_ == P1_MORE);
    drawIfLoaded(more_focused ? tex_button_on_ : tex_button_off_, more.x, more.y, more.w, more.h, sw, sh);
    {
        const float tw = text_renderer_->getTextWidth(kStrControls, LABEL_SCALE);
        const float th = text_renderer_->getTextHeight(LABEL_SCALE);
        text_renderer_->renderText(kStrControls, more.x + (more.w - tw) * 0.5f,
                                   more.y + (more.h - th) * 0.5f,
                                   more_focused ? kTextColor : kIdleText, LABEL_SCALE);
    }

    const Rect ret = page1ReturnRect();
    const bool ret_focused = (focus_ == P1_RETURN);
    drawIfLoaded(ret_focused ? tex_button_on_ : tex_button_off_, ret.x, ret.y, ret.w, ret.h, sw, sh);
    {
        const float tw = text_renderer_->getTextWidth(kStrReturn, LABEL_SCALE);
        const float th = text_renderer_->getTextHeight(LABEL_SCALE);
        text_renderer_->renderText(kStrReturn, ret.x + (ret.w - tw) * 0.5f,
                                   ret.y + (ret.h - th) * 0.5f,
                                   ret_focused ? kTextColor : kIdleText, LABEL_SCALE);
    }
}

void ControlsOptionsUI::renderPage2() {
    const int sw = screen_width_;
    const int sh = screen_height_;
    const Rect main = mainRect();

    // Device selector -------------------------------------------------------
    {
        FontScope font(text_renderer_, LABEL_FONT);
        const Rect left = deviceToggleLeftRect();
        const Rect right = deviceToggleRightRect();
        const float center_x = left.x + (right.x + right.w - left.x) * 0.5f;

        drawIfLoaded(tex_arrow_left_, left.x, left.y, left.w, left.h, sw, sh);
        drawIfLoaded(tex_arrow_right_, right.x, right.y, right.w, right.h, sw, sh);

        const char* name = kDeviceNames[std::min(DEVICE_COUNT - 1,
                                                 std::max(0, settings_.device_index))];
        const float tw = text_renderer_->getTextWidth(name, LABEL_SCALE);
        const float th = text_renderer_->getTextHeight(LABEL_SCALE);
        text_renderer_->renderText(name, center_x - tw * 0.5f, left.y + (left.h - th) * 0.5f,
                                   kTextColor, LABEL_SCALE);
    }

    // List frame borders ----------------------------------------------------
    drawIfLoaded(tex_border_h_, main.x + BORDER_TOP_DX, main.y, main.w - BORDER_TOP_DX - 32.0f,
                 BORDER_TOP_HEIGHT, sw, sh);
    drawIfLoaded(tex_border_h_, main.x + BORDER_TOP_DX, main.y + main.h - BORDER_TOP_HEIGHT,
                 main.w - BORDER_TOP_DX - 32.0f, BORDER_TOP_HEIGHT, sw, sh);
    drawIfLoaded(tex_border_v_, main.x + BORDER_LEFT_DX, main.y, BORDER_SIDE_WIDTH, main.h, sw, sh);
    drawIfLoaded(tex_border_v_, main.x + BORDER_MIDDLE_X, main.y, BORDER_SIDE_WIDTH, main.h, sw, sh);
    drawIfLoaded(tex_border_v_, main.x + BORDER_RIGHT_X, main.y, BORDER_SIDE_WIDTH, main.h, sw, sh);

    // Rows (clipped to the frame) -------------------------------------------
    {
        ScissorScope clip(main.x, main.y, main.w, main.h, sh);
        FontScope font(text_renderer_, LABEL_FONT);
        for (int i = 0; i < static_cast<int>(rows_.size()); ++i) {
            const Rect row = listRowRect(i);
            if (row.y + row.h <= main.y || row.y >= main.y + main.h) {
                continue;
            }
            const bool focused = (page2_focus_ == i + 1);
            if (focused) {
                drawIfLoaded(tex_selection_full_, row.x + ROW_HIGHLIGHT_INSET * 0.5f, row.y,
                             row.w - ROW_HIGHLIGHT_INSET, FLOAT_TOGGLE_HEIGHT, sw, sh);
                drawIfLoaded(tex_selection_cut_,
                             row.x + ROW_HIGHLIGHT_INSET * 0.5f + (row.w - ROW_HIGHLIGHT_INSET) + 8.0f - CUT_WIDTH,
                             row.y, CUT_WIDTH, CUT_HEIGHT, sw, sh);
            }
            text_renderer_->renderText(rows_[i].label, main.x + ROW_LEFT_BUFFER,
                                       row.y + ROW_LABEL_DY, kTextColor, LABEL_SCALE);
            const float bw = text_renderer_->getTextWidth(rows_[i].binding, LABEL_SCALE);
            text_renderer_->renderText(rows_[i].binding,
                                       main.x + MAIN_WIDTH - ROW_BUTTON_FROM_RIGHT - bw * 0.5f,
                                       row.y + ROW_LABEL_DY, kTextColor, LABEL_SCALE);
        }
    }

    // Vertical scroll bar ---------------------------------------------------
    {
        const Rect line = scrollLineRect();
        const Rect top = scrollTopRect();
        const Rect bottom = scrollBottomRect();
        drawIfLoaded(tex_scrollbar_line_, line.x, line.y, line.w, line.h, sw, sh);
        drawIfLoaded(tex_scrollbar_up_, top.x, top.y, top.w, top.h, sw, sh);
        drawIfLoaded(tex_scrollbar_down_, bottom.x, bottom.y, bottom.w, bottom.h, sw, sh);
        const Rect marker = scrollMarkerRect();
        drawIfLoaded(tex_scrollbar_marker_, marker.x, marker.y, marker.w, marker.h, sw, sh);
    }

    // Bottom row: "Defaults" and "Return" -----------------------------------
    {
        FontScope font(text_renderer_, LABEL_FONT);
        const Rect defaults = longButtonRect(0);
        const int defaults_index = static_cast<int>(rows_.size()) + 1;
        const bool defaults_focused = (page2_focus_ == defaults_index);
        drawIfLoaded(defaults_focused ? tex_button_on_ : tex_button_off_,
                     defaults.x, defaults.y, defaults.w, defaults.h, sw, sh);
        const float dtw = text_renderer_->getTextWidth(kStrDefaults, LABEL_SCALE);
        const float dth = text_renderer_->getTextHeight(LABEL_SCALE);
        text_renderer_->renderText(kStrDefaults, defaults.x + (defaults.w - dtw) * 0.5f,
                                   defaults.y + (defaults.h - dth) * 0.5f,
                                   defaults_focused ? kTextColor : kIdleText, LABEL_SCALE);

        const Rect ret = longButtonRect(1);
        const bool ret_focused = (page2_focus_ == static_cast<int>(rows_.size()) + 2);
        drawIfLoaded(ret_focused ? tex_button_on_ : tex_button_off_,
                     ret.x, ret.y, ret.w, ret.h, sw, sh);
        const float rtw = text_renderer_->getTextWidth(kStrReturn, LABEL_SCALE);
        const float rth = text_renderer_->getTextHeight(LABEL_SCALE);
        text_renderer_->renderText(kStrReturn, ret.x + (ret.w - rtw) * 0.5f,
                                   ret.y + (ret.h - rth) * 0.5f,
                                   ret_focused ? kTextColor : kIdleText, LABEL_SCALE);
    }
}

void ControlsOptionsUI::render() {
    if (!visible_ || !text_renderer_) {
        return;
    }

    glDisable(GL_DEPTH_TEST);

    UIDrawHelper::drawColoredQuad(0.0f, 0.0f, static_cast<float>(screen_width_),
                                  static_cast<float>(screen_height_),
                                  glm::vec4(0.0f, 0.0f, 0.0f, DIM_ALPHA),
                                  screen_width_, screen_height_);

    renderBackground();

    if (page_ == 1) {
        renderPage1();
    } else {
        renderPage2();
    }
}
