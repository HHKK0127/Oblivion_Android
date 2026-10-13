#include "video_display_ui.h"
#include "ui_draw_helper.h"
#include "../engine/texture_loader.h"
#include <algorithm>
#include <android/log.h>

#undef LOG_TAG
#undef LOGD
#undef LOGI
#undef LOGE
#define LOG_TAG "VideoDisplayUI"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {
// VideoDisplayMenu values from the original XML (PC variant). Coordinates are
// relative to the 520x680 page image, which is centred on the screen.
constexpr float PANEL_WIDTH = 520.0f;
constexpr float PANEL_HEIGHT = 680.0f;

constexpr float TEXT_X = 40.0f;
constexpr float TITLE_Y = 60.0f;
constexpr float TITLE_TO_FIRST_DY = 60.0f;   // video_first_text.y = title.y + 60
constexpr float LINE_STRIDE = 40.0f;         // each following line is +40

constexpr float BUTTON_WIDTH = 190.0f;       // cont_button_long_off.dds
constexpr float BUTTON_HEIGHT = 50.0f;
constexpr float BUTTON_ON_WIDTH = 176.0f;    // cont_button_long_on.dds
constexpr float BUTTON_ON_HEIGHT = 51.0f;
// x = (parent.width - me.width) / 2 + 150, y = video_fourth_text.y + 280
constexpr float BUTTON_X_OFFSET = 150.0f;
constexpr float BUTTON_Y_FROM_FOURTH = 280.0f;

constexpr float LABEL_SCALE = 1.0f;
constexpr FontType LABEL_FONT = FontType::KingthingsRegular;

const glm::vec4 kTextColor(117.0f / 255.0f, 59.0f / 255.0f, 33.0f / 255.0f, 200.0f / 255.0f);
const glm::vec4 kDimTextColor(117.0f / 255.0f, 59.0f / 255.0f, 33.0f / 255.0f, 110.0f / 255.0f);
const glm::vec4 kButtonTextColor(117.0f / 255.0f, 59.0f / 255.0f, 33.0f / 255.0f, 1.0f);

const char* const kTitle = "Resolution List";
const char* const kReturnLabel = "Return";
// Fixed list baked into video_display_menu.xml; the game holds the actual
// selection in code (video_first_text..fifth_text).
const char* const kLines[] = {"- 800 x 600", "- 1080 x 607", "- 1024 x 768",
                              "- 1280 x 720", "- 1280 x 960"};
constexpr int kLineCount = 5;

constexpr int KEY_BACK = 4;
constexpr int KEY_DPAD_CENTER = 23;
constexpr int KEY_BUTTON_B = 97;
constexpr int KEY_ENTER = 66;

void drawIfLoaded(GLuint tex, float x, float y, float w, float h, int sw, int sh) {
    if (tex != 0) {
        UIDrawHelper::drawTexturedQuad(x, y, w, h, tex, glm::vec4(1.0f), sw, sh);
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
}  // namespace

VideoDisplayUI::VideoDisplayUI() = default;
VideoDisplayUI::~VideoDisplayUI() = default;

bool VideoDisplayUI::initialize(TextRenderer* renderer) {
    text_renderer_ = renderer;
    if (text_renderer_ == nullptr) {
        LOGE("initialize: null TextRenderer");
        return false;
    }

    const std::string dir = "textures/ui/menus/";
    tex_panel_ = TextureLoader::loadTextureFromAsset(dir + "shared__main_page_5.png");
    tex_button_off_ = TextureLoader::loadTextureFromAsset(dir + "container__cont_button_long_off.png");
    tex_button_on_ = TextureLoader::loadTextureFromAsset(dir + "container__cont_button_long_on.png");

    if (tex_panel_ == 0) {
        LOGE("initialize: failed to load shared__main_page_5.png");
        return false;
    }
    if (tex_button_off_ == 0) {
        LOGE("initialize: failed to load container__cont_button_long_off.png");
    }
    LOGI("initialize: ok");
    return true;
}

void VideoDisplayUI::cleanup() {
    if (tex_panel_ != 0) {
        glDeleteTextures(1, &tex_panel_);
        tex_panel_ = 0;
    }
    if (tex_button_off_ != 0) {
        glDeleteTextures(1, &tex_button_off_);
        tex_button_off_ = 0;
    }
    if (tex_button_on_ != 0) {
        glDeleteTextures(1, &tex_button_on_);
        tex_button_on_ = 0;
    }
}

void VideoDisplayUI::setScreenSize(int width, int height) {
    screen_width_ = width;
    screen_height_ = height;
}

void VideoDisplayUI::open(int selected_index) {
    visible_ = true;
    return_requested_ = false;
    selected_index_ = selected_index;
}

void VideoDisplayUI::close() {
    visible_ = false;
}

VideoDisplayUI::Rect VideoDisplayUI::panelRect() const {
    return Rect{(static_cast<float>(screen_width_) - PANEL_WIDTH) * 0.5f,
                (static_cast<float>(screen_height_) - PANEL_HEIGHT) * 0.5f,
                PANEL_WIDTH, PANEL_HEIGHT};
}

float VideoDisplayUI::lineLocalY(int index) const {
    return TITLE_Y + TITLE_TO_FIRST_DY + LINE_STRIDE * static_cast<float>(index);
}

VideoDisplayUI::Rect VideoDisplayUI::returnButtonRect() const {
    const Rect panel = panelRect();
    const float local_x = (PANEL_WIDTH - BUTTON_WIDTH) * 0.5f + BUTTON_X_OFFSET;
    const float local_y = lineLocalY(3) + BUTTON_Y_FROM_FOURTH;
    return Rect{panel.x + local_x, panel.y + local_y, BUTTON_WIDTH, BUTTON_HEIGHT};
}

bool VideoDisplayUI::pointInRect(float px, float py, const Rect& rect) const {
    return contains(px, py, rect.x, rect.y, rect.w, rect.h);
}

void VideoDisplayUI::render() {
    if (!visible_) {
        return;
    }
    renderPanel();
    renderTexts();
    renderReturnButton();
}

void VideoDisplayUI::renderPanel() {
    const Rect panel = panelRect();
    drawIfLoaded(tex_panel_, panel.x, panel.y, panel.w, panel.h, screen_width_, screen_height_);
}

void VideoDisplayUI::renderTexts() {
    if (text_renderer_ == nullptr) {
        return;
    }
    FontScope font(text_renderer_, LABEL_FONT);
    const Rect panel = panelRect();

    text_renderer_->renderText(kTitle, panel.x + TEXT_X, panel.y + TITLE_Y, kTextColor, LABEL_SCALE);

    for (int i = 0; i < kLineCount; ++i) {
        // The original keeps the actual selection in code; dim the other lines
        // so the caller's current mode stays readable.
        const bool selected = (selected_index_ < 0) || (selected_index_ == i);
        text_renderer_->renderText(kLines[i], panel.x + TEXT_X, panel.y + lineLocalY(i),
                                   selected ? kTextColor : kDimTextColor, LABEL_SCALE);
    }
}

void VideoDisplayUI::renderReturnButton() {
    const Rect button = returnButtonRect();
    // cont_button_long_off.dds is the resting face; the "on" image (176x51 at
    // the button origin) only shows while the parent alpha drops below 1, which
    // never happens here, so the resting face is drawn alone.
    drawIfLoaded(tex_button_off_, button.x, button.y, button.w, button.h,
                 screen_width_, screen_height_);

    if (text_renderer_ != nullptr) {
        FontScope font(text_renderer_, LABEL_FONT);
        const float text_w = text_renderer_->getTextWidth(kReturnLabel, LABEL_SCALE);
        // x = (parent.width - me.width) / 2 - 2, y = 3 (both relative to the button)
        const float tx = button.x + (button.w - text_w) * 0.5f - 2.0f;
        const float ty = button.y + 3.0f;
        text_renderer_->renderText(kReturnLabel, tx, ty, kButtonTextColor, LABEL_SCALE);
    }
}

bool VideoDisplayUI::onTouchDown(float x, float y) {
    if (!visible_) {
        return false;
    }
    if (pointInRect(x, y, returnButtonRect())) {
        return_requested_ = true;
        return true;
    }
    // Swallow taps on the panel so they do not fall through to the world.
    return pointInRect(x, y, panelRect());
}

bool VideoDisplayUI::onKeyPress(int keyCode) {
    if (!visible_) {
        return false;
    }
    switch (keyCode) {
        case KEY_BACK:
        case KEY_BUTTON_B:
        case KEY_DPAD_CENTER:
        case KEY_ENTER:
            return_requested_ = true;
            return true;
        default:
            return false;
    }
}

bool VideoDisplayUI::handleBack() {
    if (!visible_) {
        return false;
    }
    return_requested_ = true;
    return true;
}
