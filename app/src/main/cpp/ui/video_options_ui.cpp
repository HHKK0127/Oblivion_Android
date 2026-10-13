#include "video_options_ui.h"

#include "ui_draw_helper.h"
#include "../engine/texture_loader.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include <android/log.h>

#undef LOG_TAG
#undef LOGD
#undef LOGI
#undef LOGE
#define LOG_TAG "VideoOptionsUI"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {

// VideoMenu values from the original PC layout (menus/options/video_menu.xml).
// All coordinates are relative to the menu body (900x600).
constexpr float BODY_WIDTH = 900.0f;
constexpr float BODY_HEIGHT = 600.0f;
constexpr float BORDER = 44.0f;

constexpr float MAIN_X = 37.0f;      // parent.width - 2*_x_buffer
constexpr float MAIN_Y = 50.0f;
constexpr float MAIN_W = 826.0f;     // parent.width - 2*_x_buffer
constexpr float MAIN_H = 472.0f;     // parent.height - 2*_y_buffer

constexpr int VISIBLE_ROWS = 7;      // VideoMenu.user1
constexpr float ROW_PITCH = MAIN_H / static_cast<float>(VISIBLE_ROWS);  // VideoMenu.user2

constexpr float CENTER_X = MAIN_W * 0.65f;  // VideoMenu.user0

// Selection marker group (slider focus highlight).
constexpr float MARKER_H = 64.0f;
constexpr float MARKER_W = 520.0f;
constexpr float MARKER_DY = -15.0f;
constexpr float CUT_X_IN_MARKER = 420.0f;   // parent.width - 100
constexpr float CUT_W = 104.0f;
constexpr float CUT_H = 64.0f;

// Horizontal scroll line inside the marker group.
constexpr float LINE_X_IN_MARKER = 60.0f;
constexpr float LINE_Y_IN_MARKER = 22.0f;
constexpr float LINE_W = 400.0f;
constexpr float LINE_H = 16.0f;
constexpr float HANDLE_W = 39.0f;
constexpr float HANDLE_H = 34.0f;
constexpr float HANDLE_DY = -10.0f;
constexpr float LEFT_ARROW_DX = -49.0f;
constexpr float LEFT_ARROW_W = 50.0f;
constexpr float LEFT_ARROW_H = 37.0f;
constexpr float RIGHT_ARROW_DX = 399.0f;
constexpr float RIGHT_ARROW_W = 51.0f;
constexpr float RIGHT_ARROW_H = 42.0f;
constexpr float ARROW_DY = -11.0f;

// Button rows (button_floating_2.xml).
constexpr float BUTTON_H = 64.0f;
constexpr float BUTTON_PAD = 40.0f;
constexpr float BUTTON_MIN_W = 104.0f;
constexpr float BUTTON_DY = -12.0f;

// Arrow rows (resolution / antialiasing).
constexpr float ARROW_LEFT_X = 288.0f;
constexpr float ARROW_IMG_W = 121.0f;
constexpr float ARROW_IMG_H = 36.0f;
constexpr float ARROW_RIGHT_X = 664.8f;
constexpr float ARROW_FLOAT_W = 512.0f;
constexpr float ARROW_FLOAT_H = 64.0f;
constexpr float ARROW_FLOAT_DX = -10.0f;

// Vertical scroll bar (vert_floating_scroll.xml).
constexpr float SCROLL_X = MAIN_W;          // line sits on the main rect right edge
constexpr float SCROLL_TOP_H = 50.0f;
constexpr float SCROLL_BOTTOM_H = 51.0f;
constexpr float SCROLL_ARROW_DX = -11.0f;
constexpr float SCROLL_MARKER_DX = -10.0f;
constexpr float SCROLL_MARKER_W = 34.0f;
constexpr float SCROLL_MARKER_H = 39.0f;

// Footer buttons (button_long.xml, 190x50).
constexpr float FOOT_W = 190.0f;
constexpr float FOOT_H = 50.0f;
constexpr float FOOT_MARGIN_X = 30.0f;
constexpr float FOOT_MARGIN_Y = 20.0f;
constexpr float FOOT_GAP = 10.0f;

constexpr float STEP = 0.02f;
constexpr float JUMP = 0.10f;
constexpr float DIM_ALPHA = 0.6f;
constexpr float LABEL_SCALE = 1.0f;

constexpr FontType LABEL_FONT = FontType::KingthingsRegular;
const glm::vec4 kTextColor(117.0f / 255.0f, 59.0f / 255.0f, 33.0f / 255.0f, 1.0f);
const glm::vec4 kIdleText(128.0f / 255.0f, 128.0f / 255.0f, 128.0f / 255.0f, 196.0f / 255.0f);

constexpr int KEY_BACK = 4;
constexpr int KEY_DPAD_UP = 19;
constexpr int KEY_DPAD_DOWN = 20;
constexpr int KEY_DPAD_LEFT = 21;
constexpr int KEY_DPAD_RIGHT = 22;
constexpr int KEY_DPAD_CENTER = 23;
constexpr int KEY_BUTTON_Y = 100;
constexpr int KEY_BUTTON_L1 = 102;
constexpr int KEY_BUTTON_R1 = 103;
constexpr int KEY_ENTER = 66;

// Option label sets for the discrete (button / arrow) rows.
const char* const kOffOn[] = {"Off", "On"};
const char* const kTextureSize[] = {"Small", "Medium", "Large"};
const char* const kWaterDetail[] = {"Low", "Medium", "High"};
const char* const kResolution[] = {"800 x 600", "1024 x 768", "1280 x 720",
                                   "1280 x 960", "1600 x 900", "1920 x 1080"};
constexpr int kResolutionCount = 6;

const char* optionLabel(const char* const* set, int count, int index) {
    if (index < 0) index = 0;
    if (index >= count) index = count - 1;
    return set[index];
}

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

VideoOptionsUI::VideoOptionsUI() = default;

VideoOptionsUI::~VideoOptionsUI() {
    cleanup();
}

bool VideoOptionsUI::initialize(TextRenderer* renderer) {
    if (!renderer) {
        LOGE("Error: TextRenderer is null");
        return false;
    }
    text_renderer_ = renderer;

    if (tex_generic_bg_ == 0) {
        const std::string dir = "textures/ui/menus/";
        tex_generic_bg_ = TextureLoader::loadTextureFromAsset(dir + "genericbackground__center_background.png");
        tex_selection_full_ = TextureLoader::loadTextureFromAsset(dir + "dialog__dialog_selection_full.png");
        tex_selection_cut_ = TextureLoader::loadTextureFromAsset(dir + "dialog__dialog_selection_cut.png");
        tex_button_long_off_ = TextureLoader::loadTextureFromAsset(dir + "shared__shared_button_long_off.png");
        tex_button_long_on_ = TextureLoader::loadTextureFromAsset(dir + "shared__shared_button_long_on.png");
        tex_scrollbar_line_ = TextureLoader::loadTextureFromAsset(dir + "dialog__dialog_scrollbar_line.png");
        tex_scrollbar_up_ = TextureLoader::loadTextureFromAsset(dir + "dialog__dialog_scrollbar_up.png");
        tex_scrollbar_down_ = TextureLoader::loadTextureFromAsset(dir + "dialog__dialog_scrollbar_down.png");
        tex_scrollbar_marker_ = TextureLoader::loadTextureFromAsset(dir + "dialog__dialog_scrollbar_marker.png");
        tex_scrollbar_parts_ = TextureLoader::loadTextureFromAsset(dir + "shared__horizontal_scrollbar_parts.png");
        tex_arrow_left_ = TextureLoader::loadTextureFromAsset(dir + "shared__shared_arrow_left.png");
        tex_arrow_right_ = TextureLoader::loadTextureFromAsset(dir + "shared__shared_arrow_right.png");
    }

    buildRows();
    measureLabels();
    LOGI("VideoOptionsUI initialized");
    return true;
}

void VideoOptionsUI::cleanup() {
    for (GLuint* tex : {&tex_generic_bg_, &tex_selection_full_, &tex_selection_cut_,
                        &tex_button_long_off_, &tex_button_long_on_,
                        &tex_scrollbar_line_, &tex_scrollbar_up_, &tex_scrollbar_down_,
                        &tex_scrollbar_marker_, &tex_scrollbar_parts_,
                        &tex_arrow_left_, &tex_arrow_right_}) {
        if (*tex != 0) {
            TextureLoader::deleteTexture(*tex);
            *tex = 0;
        }
    }
    text_renderer_ = nullptr;
    visible_ = false;
}

void VideoOptionsUI::setScreenSize(int width, int height) {
    screen_width_ = width;
    screen_height_ = height;
}

void VideoOptionsUI::buildRows() {
    if (!rows_.empty()) {
        return;
    }
    rows_ = {
        {"Resolution:",        Row::Kind::Arrows, nullptr, &settings_.resolution_index, kResolutionCount},
        {"Gamma:",             Row::Kind::Slider, &settings_.gamma, nullptr, 0},
        {"Texture Size:",      Row::Kind::Button, nullptr, &settings_.texture_size, 3},
        {"Tree LOD:",          Row::Kind::Slider, &settings_.tree_lod, nullptr, 0},
        {"Actor LOD:",         Row::Kind::Slider, &settings_.actor_lod, nullptr, 0},
        {"Item LOD:",          Row::Kind::Slider, &settings_.item_lod, nullptr, 0},
        {"Object LOD:",        Row::Kind::Slider, &settings_.object_lod, nullptr, 0},
        {"Grass:",             Row::Kind::Slider, &settings_.grass, nullptr, 0},
        {"View Distance:",     Row::Kind::Slider, &settings_.view_distance, nullptr, 0},
        {"Distant LOD:",       Row::Kind::Button, nullptr, &settings_.distant_lod, 2},
        {"Distant Building:",  Row::Kind::Button, nullptr, &settings_.dist_build, 2},
        {"Distant Trees:",     Row::Kind::Button, nullptr, &settings_.dist_trees, 2},
        {"Interior Shadows:",  Row::Kind::Slider, &settings_.interior_shadows, nullptr, 0},
        {"Exterior Shadows:",  Row::Kind::Slider, &settings_.exterior_shadows, nullptr, 0},
        {"Self Shadows:",      Row::Kind::Button, nullptr, &settings_.self_shadows, 2},
        {"Grass Shadows:",     Row::Kind::Button, nullptr, &settings_.grass_shadows, 2},
        {"Canopy Shadows:",    Row::Kind::Button, nullptr, &settings_.canopy_shadows, 2},
        {"Shadow Filtering:",  Row::Kind::Button, nullptr, &settings_.shadow_filter, 2},
        {"Specular Light:",    Row::Kind::Slider, &settings_.specular_light, nullptr, 0},
        {"HDR:",               Row::Kind::Button, nullptr, &settings_.hdr, 2},
        {"Bloom:",             Row::Kind::Button, nullptr, &settings_.bloom, 2},
        {"Water Detail:",      Row::Kind::Button, nullptr, &settings_.water_detail, 3},
        {"Water Reflections:", Row::Kind::Button, nullptr, &settings_.water_reflections, 2},
        {"Water Displacement:",Row::Kind::Button, nullptr, &settings_.water_displacement, 2},
        {"Window Reflections:",Row::Kind::Button, nullptr, &settings_.window_reflections, 2},
        {"Blood:",             Row::Kind::Button, nullptr, &settings_.blood, 2},
        {"Antialiasing:",      Row::Kind::Arrows, nullptr, &settings_.antialiasing, 2},
    };
}

void VideoOptionsUI::measureLabels() {
    if (!text_renderer_) {
        return;
    }
    FontScope font(text_renderer_, LABEL_FONT);
    label_height_ = text_renderer_->getTextHeight(LABEL_SCALE);
    label_widths_.assign(rows_.size(), 0.0f);
    for (size_t i = 0; i < rows_.size(); ++i) {
        label_widths_[i] = text_renderer_->getTextWidth(rows_[i].label, LABEL_SCALE);
    }
}

void VideoOptionsUI::open() {
    visible_ = true;
    return_to_options_ = false;
    changed_ = false;
    display_request_ = false;
    scroll_row_ = 0;
    focus_row_ = 0;
    drag_slider_ = -1;
    buildRows();
    measureLabels();
}

void VideoOptionsUI::close() {
    visible_ = false;
    drag_slider_ = -1;
}

bool VideoOptionsUI::consumeChanged() {
    const bool result = changed_;
    changed_ = false;
    return result;
}

bool VideoOptionsUI::consumeDisplayRequest(int& selected_index) {
    if (!display_request_) {
        return false;
    }
    display_request_ = false;
    selected_index = settings_.resolution_index;
    return true;
}

void VideoOptionsUI::update(float /*dt*/) {
}

VideoOptionsUI::Rect VideoOptionsUI::bodyRect() const {
    return {(screen_width_ - BODY_WIDTH) * 0.5f, (screen_height_ - BODY_HEIGHT) * 0.5f,
            BODY_WIDTH, BODY_HEIGHT};
}

float VideoOptionsUI::rowLocalY(int index) const {
    return static_cast<float>(index - scroll_row_) * ROW_PITCH;
}

VideoOptionsUI::Rect VideoOptionsUI::markerRect(int index) const {
    const Rect body = bodyRect();
    const float x = body.x + MAIN_X + CENTER_X - MARKER_W * 0.5f;
    const float y = body.y + MAIN_Y + rowLocalY(index) + MARKER_DY;
    return {x, y, MARKER_W, MARKER_H};
}

VideoOptionsUI::Rect VideoOptionsUI::sliderLineRect(int index) const {
    const Rect marker = markerRect(index);
    return {marker.x + LINE_X_IN_MARKER, marker.y + LINE_Y_IN_MARKER, LINE_W, LINE_H};
}

VideoOptionsUI::Rect VideoOptionsUI::sliderHandleRect(int index) const {
    const Rect line = sliderLineRect(index);
    const float value = rows_[index].value ? clamp01(*rows_[index].value) : 0.0f;
    const float travel = LINE_W - HANDLE_W;
    return {line.x + travel * value, line.y + HANDLE_DY, HANDLE_W, HANDLE_H};
}

VideoOptionsUI::Rect VideoOptionsUI::rowButtonRect(int index) const {
    const Rect body = bodyRect();
    const float text_w = label_widths_.size() > static_cast<size_t>(index)
                             ? label_widths_[index] : 0.0f;
    const float w = std::max(text_w + BUTTON_PAD, BUTTON_MIN_W);
    const float x = body.x + MAIN_X + CENTER_X - w * 0.5f;
    const float y = body.y + MAIN_Y + rowLocalY(index) + BUTTON_DY;
    return {x, y, w, BUTTON_H};
}

VideoOptionsUI::Rect VideoOptionsUI::arrowsLeftRect(int index) const {
    const Rect body = bodyRect();
    const float y = body.y + MAIN_Y + rowLocalY(index) + (label_height_ - ARROW_IMG_H) * 0.5f;
    return {body.x + MAIN_X + ARROW_LEFT_X, y, ARROW_IMG_W, ARROW_IMG_H};
}

VideoOptionsUI::Rect VideoOptionsUI::arrowsRightRect(int index) const {
    const Rect body = bodyRect();
    const float y = body.y + MAIN_Y + rowLocalY(index) + (label_height_ - ARROW_IMG_H) * 0.5f;
    return {body.x + MAIN_X + ARROW_RIGHT_X, y, ARROW_IMG_W, ARROW_IMG_H};
}

VideoOptionsUI::Rect VideoOptionsUI::arrowsValueRect(int index) const {
    const Rect body = bodyRect();
    const float w = ARROW_RIGHT_X - ARROW_LEFT_X - ARROW_IMG_W;
    const float x = body.x + MAIN_X + ARROW_LEFT_X + ARROW_IMG_W;
    const float y = body.y + MAIN_Y + rowLocalY(index);
    return {x, y, w, label_height_};
}

VideoOptionsUI::Rect VideoOptionsUI::scrollLineRect() const {
    const Rect body = bodyRect();
    const float h = MAIN_H - SCROLL_TOP_H - SCROLL_BOTTOM_H;
    return {body.x + MAIN_X + SCROLL_X, body.y + MAIN_Y, 16.0f, h};
}

VideoOptionsUI::Rect VideoOptionsUI::scrollTopRect() const {
    const Rect body = bodyRect();
    return {body.x + MAIN_X + SCROLL_X + SCROLL_ARROW_DX, body.y + MAIN_Y - SCROLL_TOP_H,
            37.0f, SCROLL_TOP_H};
}

VideoOptionsUI::Rect VideoOptionsUI::scrollBottomRect() const {
    const Rect body = bodyRect();
    const float line_h = MAIN_H - SCROLL_TOP_H - SCROLL_BOTTOM_H;
    return {body.x + MAIN_X + SCROLL_X + SCROLL_ARROW_DX, body.y + MAIN_Y + line_h,
            42.0f, SCROLL_BOTTOM_H};
}

VideoOptionsUI::Rect VideoOptionsUI::scrollMarkerRect() const {
    const Rect line = scrollLineRect();
    const int max_scroll = std::max(1, static_cast<int>(rows_.size()) - VISIBLE_ROWS);
    const float t = static_cast<float>(scroll_row_) / static_cast<float>(max_scroll);
    const float travel = line.h - SCROLL_MARKER_H;
    return {line.x + SCROLL_MARKER_DX, line.y + travel * t, SCROLL_MARKER_W, SCROLL_MARKER_H};
}

VideoOptionsUI::Rect VideoOptionsUI::footerButtonRect(int which) const {
    const Rect body = bodyRect();
    const float return_x = body.w - FOOT_W - FOOT_MARGIN_X;
    const float y = body.h - FOOT_H - FOOT_MARGIN_Y;
    if (which == 0) {
        return {body.x + return_x, body.y + y, FOOT_W, FOOT_H};
    }
    return {body.x + return_x - FOOT_W - FOOT_GAP, body.y + y, FOOT_W, FOOT_H};
}

bool VideoOptionsUI::pointInRect(float px, float py, const Rect& rect) const {
    return contains(px, py, rect.x, rect.y, rect.w, rect.h);
}

void VideoOptionsUI::focusRow(int index) {
    focus_row_ = index;
    ensureVisible(index);
}

void VideoOptionsUI::ensureVisible(int index) {
    if (index < scroll_row_) {
        scroll_row_ = index;
    } else if (index >= scroll_row_ + VISIBLE_ROWS) {
        scroll_row_ = index - VISIBLE_ROWS + 1;
    }
    scroll_row_ = std::max(0, std::min(scroll_row_,
                                       static_cast<int>(rows_.size()) - VISIBLE_ROWS));
}

void VideoOptionsUI::adjustSlider(int index, float delta) {
    if (!rows_[index].value) {
        return;
    }
    const float next = clamp01(*rows_[index].value + delta);
    if (std::fabs(next - *rows_[index].value) > 1e-6f) {
        *rows_[index].value = next;
        changed_ = true;
    }
}

void VideoOptionsUI::setSliderFromX(int index, float screen_x) {
    if (!rows_[index].value) {
        return;
    }
    const Rect line = sliderLineRect(index);
    const float travel = LINE_W - HANDLE_W;
    const float next = clamp01((screen_x - line.x - HANDLE_W * 0.5f) / travel);
    if (std::fabs(next - *rows_[index].value) > 1e-6f) {
        *rows_[index].value = next;
        changed_ = true;
    }
}

void VideoOptionsUI::adjustIndex(int index, int delta) {
    if (!rows_[index].index) {
        return;
    }
    const int count = std::max(1, rows_[index].option_count);
    int next = (*rows_[index].index + delta) % count;
    if (next < 0) {
        next += count;
    }
    if (next != *rows_[index].index) {
        *rows_[index].index = next;
        changed_ = true;
        // Changing the resolution shows the original VideoDisplayMenu panel,
        // which lists the fixed modes and the current selection.
        if (rows_[index].index == &settings_.resolution_index) {
            display_request_ = true;
        }
    }
}

int VideoOptionsUI::rowAtPoint(float px, float py) const {
    for (size_t i = 0; i < rows_.size(); ++i) {
        const int index = static_cast<int>(i);
        const float ly = rowLocalY(index);
        if (ly < 0.0f || ly + 20.0f >= MAIN_H) {
            continue;  // culled rows do not receive input
        }
        const Rect body = bodyRect();
        const Rect row{body.x + MAIN_X, body.y + MAIN_Y + ly, MAIN_W, ROW_PITCH};
        if (pointInRect(px, py, row)) {
            return index;
        }
    }
    return -1;
}

bool VideoOptionsUI::onTouchDown(float x, float y) {
    if (!visible_) {
        return false;
    }

    if (pointInRect(x, y, scrollTopRect())) {
        focusRow(std::max(0, scroll_row_ - 1));
        return true;
    }
    if (pointInRect(x, y, scrollBottomRect())) {
        focusRow(std::min(static_cast<int>(rows_.size()) - 1, scroll_row_ + 1));
        return true;
    }
    if (pointInRect(x, y, footerButtonRect(1))) {
        applyDefaults();
        return true;
    }
    if (pointInRect(x, y, footerButtonRect(0))) {
        return handleBack();
    }

    const int index = rowAtPoint(x, y);
    if (index < 0) {
        return false;
    }
    focus_row_ = index;
    const Row& row = rows_[index];

    if (row.kind == Row::Kind::Slider) {
        const Rect line = sliderLineRect(index);
        const Rect left{line.x + LEFT_ARROW_DX, line.y + ARROW_DY, LEFT_ARROW_W, LEFT_ARROW_H};
        const Rect right{line.x + RIGHT_ARROW_DX, line.y + ARROW_DY, RIGHT_ARROW_W, RIGHT_ARROW_H};
        const Rect track{line.x, line.y - 11.0f, LINE_W, 37.0f};
        const Rect handle = sliderHandleRect(index);
        if (pointInRect(x, y, left)) {
            adjustSlider(index, -STEP);
        } else if (pointInRect(x, y, right)) {
            adjustSlider(index, STEP);
        } else if (pointInRect(x, y, handle)) {
            drag_slider_ = index;
        } else if (pointInRect(x, y, track)) {
            adjustSlider(index, x < handle.x ? -JUMP : JUMP);
        }
        return true;
    }

    if (row.kind == Row::Kind::Arrows) {
        const Rect left = arrowsLeftRect(index);
        const Rect right = arrowsRightRect(index);
        if (pointInRect(x, y, left)) {
            adjustIndex(index, -1);
        } else if (pointInRect(x, y, right)) {
            adjustIndex(index, 1);
        }
        return true;
    }

    // Button row: cycle forward on tap.
    adjustIndex(index, 1);
    return true;
}

bool VideoOptionsUI::onTouchMove(float x) {
    if (visible_ && drag_slider_ >= 0) {
        setSliderFromX(drag_slider_, x);
    }
    return visible_ && drag_slider_ >= 0;
}

bool VideoOptionsUI::onTouchUp() {
    drag_slider_ = -1;
    return false;
}

bool VideoOptionsUI::onKeyPress(int keyCode) {
    if (!visible_) {
        return false;
    }
    const Row& row = rows_[focus_row_];
    switch (keyCode) {
        case KEY_DPAD_UP:
            focusRow((focus_row_ + static_cast<int>(rows_.size()) - 1) % static_cast<int>(rows_.size()));
            return true;
        case KEY_DPAD_DOWN:
            focusRow((focus_row_ + 1) % static_cast<int>(rows_.size()));
            return true;
        case KEY_DPAD_LEFT:
        case KEY_DPAD_RIGHT: {
            const int delta = (keyCode == KEY_DPAD_LEFT) ? -1 : 1;
            if (row.kind == Row::Kind::Slider) {
                adjustSlider(focus_row_, delta * STEP);
            } else {
                adjustIndex(focus_row_, delta);
            }
            return true;
        }
        case KEY_BUTTON_L1:
            if (row.kind == Row::Kind::Slider) {
                adjustSlider(focus_row_, -JUMP);
            } else {
                adjustIndex(focus_row_, -1);
            }
            return true;
        case KEY_BUTTON_R1:
            if (row.kind == Row::Kind::Slider) {
                adjustSlider(focus_row_, JUMP);
            } else {
                adjustIndex(focus_row_, 1);
            }
            return true;
        case KEY_DPAD_CENTER:
        case KEY_ENTER:
            if (row.kind == Row::Kind::Button) {
                adjustIndex(focus_row_, 1);
            }
            return true;
        case KEY_BUTTON_Y:
            applyDefaults();
            return true;
        case KEY_BACK:
            return handleBack();
        default:
            return false;
    }
}

bool VideoOptionsUI::handleBack() {
    if (!visible_) {
        return false;
    }
    return_to_options_ = true;
    close();
    return true;
}

void VideoOptionsUI::applyDefaults() {
    settings_ = Settings{};
    changed_ = true;
    LOGI("VideoOptionsUI: defaults restored");
}

void VideoOptionsUI::renderBorder() {
    const Rect b = bodyRect();
    const int sw = screen_width_;
    const int sh = screen_height_;
    drawIfLoaded(tex_generic_bg_, b.x, b.y, b.w, b.h, sw, sh);
}

void VideoOptionsUI::renderSliderRow(const Row& row, int index, float row_y) {
    const int sw = screen_width_;
    const int sh = screen_height_;
    const Rect line = sliderLineRect(index);
    const bool focused = (focus_row_ == index);

    if (focused) {
        const Rect marker = markerRect(index);
        drawIfLoaded(tex_selection_full_, marker.x, marker.y, marker.w, marker.h, sw, sh);
        drawIfLoaded(tex_selection_cut_, marker.x + CUT_X_IN_MARKER, marker.y, CUT_W, CUT_H, sw, sh);
    }

    drawIfLoaded(tex_scrollbar_line_, line.x, line.y, line.w, line.h, sw, sh);

    constexpr float PARTS_W = 256.0f;
    constexpr float PARTS_H = 64.0f;
    drawCropped(tex_scrollbar_parts_, line.x + LEFT_ARROW_DX, line.y + ARROW_DY,
                LEFT_ARROW_W, LEFT_ARROW_H, 0.0f, 0.0f, 50.0f, 37.0f, PARTS_W, PARTS_H, sw, sh);
    drawCropped(tex_scrollbar_parts_, line.x + RIGHT_ARROW_DX, line.y + ARROW_DY,
                RIGHT_ARROW_W, RIGHT_ARROW_H, 128.0f, 0.0f, 51.0f, 42.0f, PARTS_W, PARTS_H, sw, sh);

    const Rect handle = sliderHandleRect(index);
    drawCropped(tex_scrollbar_parts_, handle.x, handle.y, handle.w, handle.h,
                64.0f, 0.0f, 39.0f, 34.0f, PARTS_W, PARTS_H, sw, sh);

    if (row.value && text_renderer_) {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%d", static_cast<int>(std::lround(clamp01(*row.value) * 100.0f)));
        FontScope font(text_renderer_, LABEL_FONT);
        const float tw = text_renderer_->getTextWidth(buf, LABEL_SCALE);
        const float th = text_renderer_->getTextHeight(LABEL_SCALE);
        text_renderer_->renderText(buf, handle.x + (handle.w - tw) * 0.5f,
                                   handle.y + (handle.h - th) * 0.5f, kTextColor, LABEL_SCALE);
    }
    (void)row_y;
}

void VideoOptionsUI::renderButtonRow(const Row& row, int index, float row_y) {
    const int sw = screen_width_;
    const int sh = screen_height_;
    const Rect r = rowButtonRect(index);
    const bool focused = (focus_row_ == index);

    if (focused) {
        const float float_w = std::max(r.w, BUTTON_MIN_W);
        drawIfLoaded(tex_selection_full_, r.x, r.y + 3.0f, float_w, BUTTON_H, sw, sh);
        drawIfLoaded(tex_selection_cut_, r.x + r.w + 8.0f - CUT_W, r.y, CUT_W, CUT_H, sw, sh);
    }

    const char* value = optionLabel(kOffOn, 2, row.index ? *row.index : 0);
    if (row.option_count == 3) {
        const bool is_water = (row.label == "Water Detail:");
        value = optionLabel(is_water ? kWaterDetail : kTextureSize, 3, row.index ? *row.index : 0);
    }

    if (text_renderer_) {
        FontScope font(text_renderer_, LABEL_FONT);
        const float tw = text_renderer_->getTextWidth(value, LABEL_SCALE);
        const float th = text_renderer_->getTextHeight(LABEL_SCALE);
        text_renderer_->renderText(value, r.x + (r.w - tw) * 0.5f,
                                   r.y + (r.h - th) * 0.5f,
                                   focused ? kTextColor : kIdleText, LABEL_SCALE);
    }
    (void)row_y;
}

void VideoOptionsUI::renderArrowsRow(const Row& row, int index, float row_y) {
    const int sw = screen_width_;
    const int sh = screen_height_;
    const Rect left = arrowsLeftRect(index);
    const Rect right = arrowsRightRect(index);
    const bool focused = (focus_row_ == index);

    if (focused) {
        const Rect value = arrowsValueRect(index);
        const float fx = left.x + ARROW_FLOAT_DX;
        const float fy = left.y + (ARROW_IMG_H - ARROW_FLOAT_H) * 0.5f;
        drawIfLoaded(tex_selection_full_, fx, fy, ARROW_FLOAT_W, ARROW_FLOAT_H, sw, sh);
        (void)value;
    }

    drawIfLoaded(tex_arrow_left_, left.x, left.y, left.w, left.h, sw, sh);
    drawIfLoaded(tex_arrow_right_, right.x, right.y, right.w, right.h, sw, sh);

    if (row.index && text_renderer_) {
        const int count = row.option_count;
        const char* value = (count == kResolutionCount)
                                ? optionLabel(kResolution, kResolutionCount, *row.index)
                                : optionLabel(kOffOn, 2, *row.index);
        FontScope font(text_renderer_, LABEL_FONT);
        const float tw = text_renderer_->getTextWidth(value, LABEL_SCALE);
        const float th = text_renderer_->getTextHeight(LABEL_SCALE);
        const Rect vr = arrowsValueRect(index);
        text_renderer_->renderText(value, vr.x + (vr.w - tw) * 0.5f,
                                   vr.y + (vr.h - th) * 0.5f,
                                   focused ? kTextColor : kIdleText, LABEL_SCALE);
    }
    (void)row_y;
}

void VideoOptionsUI::renderRow(const Row& row, int index, float row_y) {
    switch (row.kind) {
        case Row::Kind::Slider: renderSliderRow(row, index, row_y); break;
        case Row::Kind::Button: renderButtonRow(row, index, row_y); break;
        case Row::Kind::Arrows: renderArrowsRow(row, index, row_y); break;
    }
}

void VideoOptionsUI::renderScrollBar() {
    const int sw = screen_width_;
    const int sh = screen_height_;
    const Rect line = scrollLineRect();
    drawIfLoaded(tex_scrollbar_line_, line.x, line.y, line.w, line.h, sw, sh);

    const Rect top = scrollTopRect();
    const Rect bottom = scrollBottomRect();
    drawIfLoaded(tex_scrollbar_up_, top.x, top.y, top.w, top.h, sw, sh);
    drawIfLoaded(tex_scrollbar_down_, bottom.x, bottom.y, bottom.w, bottom.h, sw, sh);

    const Rect marker = scrollMarkerRect();
    drawIfLoaded(tex_scrollbar_marker_, marker.x, marker.y, marker.w, marker.h, sw, sh);
}

void VideoOptionsUI::renderFooter() {
    const int sw = screen_width_;
    const int sh = screen_height_;
    const Rect ret = footerButtonRect(0);
    const Rect def = footerButtonRect(1);

    drawIfLoaded(tex_button_long_off_, ret.x, ret.y, ret.w, ret.h, sw, sh);
    drawIfLoaded(tex_button_long_off_, def.x, def.y, def.w, def.h, sw, sh);

    if (text_renderer_) {
        FontScope font(text_renderer_, LABEL_FONT);
        const float th = text_renderer_->getTextHeight(LABEL_SCALE);
        const char* labels[2] = {"Return", "Defaults"};
        const Rect rects[2] = {ret, def};
        for (int i = 0; i < 2; ++i) {
            const float tw = text_renderer_->getTextWidth(labels[i], LABEL_SCALE);
            text_renderer_->renderText(labels[i], rects[i].x + (rects[i].w - tw) * 0.5f,
                                       rects[i].y + (rects[i].h - th) * 0.5f,
                                       kTextColor, LABEL_SCALE);
        }
    }
}

void VideoOptionsUI::render() {
    if (!visible_ || !text_renderer_) {
        return;
    }

    glDisable(GL_DEPTH_TEST);

    UIDrawHelper::drawColoredQuad(0.0f, 0.0f, static_cast<float>(screen_width_),
                                  static_cast<float>(screen_height_),
                                  glm::vec4(0.0f, 0.0f, 0.0f, DIM_ALPHA),
                                  screen_width_, screen_height_);

    renderBorder();

    const Rect body = bodyRect();

    {
        FontScope font(text_renderer_, LABEL_FONT);
        for (size_t i = 0; i < rows_.size(); ++i) {
            const int index = static_cast<int>(i);
            const float ly = rowLocalY(index);
            if (ly < 0.0f || ly + 20.0f >= MAIN_H) {
                continue;  // row culling: <visible> = y>=0 AND y+20 < parent.height
            }
            text_renderer_->renderText(rows_[i].label, body.x + MAIN_X,
                                       body.y + MAIN_Y + ly, kTextColor, LABEL_SCALE);
        }
    }

    for (size_t i = 0; i < rows_.size(); ++i) {
        const int index = static_cast<int>(i);
        const float ly = rowLocalY(index);
        if (ly < 0.0f || ly + 20.0f >= MAIN_H) {
            continue;
        }
        renderRow(rows_[i], index, ly);
    }

    renderScrollBar();
    renderFooter();
}
