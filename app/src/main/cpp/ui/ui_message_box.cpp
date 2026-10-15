#include "ui_message_box.h"

#include <algorithm>
#include <cstddef>

#include <android/log.h>

#include "../engine/texture_loader.h"
#include "ui_draw_helper.h"

#undef LOG_TAG
#undef LOGD
#undef LOGI
#undef LOGE
#define LOG_TAG "UIMessageBox"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {

// Values from Data\Menus\message_menu.xml and the generic_background prefab:
// background width 700, _border_thickness 44, text wrapwidth = 700 - 24,
// text y = 15, first button y = text height + 30, button gap = 10, bottom pad = 15.
constexpr float BODY_WIDTH = 700.0f;
constexpr float TEXT_TOP = 15.0f;
constexpr float TEXT_WRAP_INSET = 24.0f;
constexpr float TEXT_TO_BUTTONS = 30.0f;
constexpr float BUTTON_GAP = 10.0f;
constexpr float BUTTON_BOTTOM_PAD = 15.0f;
constexpr float BUTTON_LABEL_Y = 4.0f;
constexpr float LABEL_SCALE = 1.0f;
constexpr float DIM_ALPHA = 0.6f;
// Touch-only affordance: the gap between rows is 10 px, so the active area of a row
// is grown by half of it plus a little horizontal slack.
constexpr float BUTTON_TOUCH_PAD_Y = BUTTON_GAP * 0.5f;
constexpr float BUTTON_TOUCH_PAD_X = 12.0f;

// XML <font> on message_text is absent, so the menu default face applies.
constexpr FontType MESSAGE_FONT = FontType::KingthingsRegular;

// Engine default menu text colour (RE report: rgb(117,59,33)); idle matches the
// colour used by the other menu panels for unselected rows.
const glm::vec4 kFocusedText(117.0f / 255.0f, 59.0f / 255.0f, 33.0f / 255.0f, 1.0f);
const glm::vec4 kIdleText(128.0f / 255.0f, 128.0f / 255.0f, 128.0f / 255.0f, 196.0f / 255.0f);

// MessageMenu defaults, from the string table keys sDefaultMessage and sOk.
const char* const DEFAULT_MESSAGE = "DEFAULT MESSAGE TEXT";
const char* const DEFAULT_OPTION = "Ok";

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

std::string trimSpaces(const std::string& s) {
    size_t begin = 0;
    size_t end = s.size();
    while (begin < end && (s[begin] == ' ' || s[begin] == '\t' || s[begin] == '\r')) {
        ++begin;
    }
    while (end > begin &&
           (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r')) {
        --end;
    }
    return s.substr(begin, end - begin);
}

// Start offset of every UTF-8 codepoint, so wrapping never splits a character.
std::vector<size_t> utf8Offsets(const std::string& s) {
    std::vector<size_t> offsets;
    for (size_t i = 0; i < s.size();) {
        offsets.push_back(i);
        const unsigned char c = static_cast<unsigned char>(s[i]);
        size_t len = 1;
        if ((c & 0xE0) == 0xC0) {
            len = 2;
        } else if ((c & 0xF0) == 0xE0) {
            len = 3;
        } else if ((c & 0xF8) == 0xF0) {
            len = 4;
        }
        i += std::min(len, s.size() - i);
    }
    return offsets;
}

// Width based word wrap; long words (and CJK runs, which carry no spaces) are broken
// between codepoints. text_renderer has no wrap helper of its own.
void appendWrappedParagraph(TextRenderer* renderer, const std::string& paragraph,
                            float maxWidth, float scale, std::vector<std::string>& out) {
    const std::vector<size_t> offsets = utf8Offsets(paragraph);
    const size_t count = offsets.size();
    if (count == 0) {
        out.push_back(std::string());
        return;
    }

    size_t start = 0;
    while (start < count) {
        size_t i = start;
        size_t lastSpace = std::string::npos;
        while (i < count) {
            const size_t end = (i + 1 < count) ? offsets[i + 1] : paragraph.size();
            const float width =
                renderer->getTextWidth(paragraph.substr(offsets[start], end - offsets[start]), scale);
            if (width > maxWidth && i > start) {
                break;
            }
            if (paragraph[offsets[i]] == ' ') {
                lastSpace = i;
            }
            ++i;
        }

        if (i >= count) {
            out.push_back(trimSpaces(paragraph.substr(offsets[start])));
            break;
        }

        const bool breakAtSpace = (lastSpace != std::string::npos && lastSpace > start);
        size_t cut = breakAtSpace ? lastSpace : i;
        if (cut <= start) {
            cut = start + 1;  // single codepoint wider than the wrap width
        }
        out.push_back(trimSpaces(paragraph.substr(offsets[start], offsets[cut] - offsets[start])));

        if (breakAtSpace) {
            ++cut;  // consume the space we broke on
        }
        start = cut;
        while (start < count && paragraph[offsets[start]] == ' ') {
            ++start;
        }
    }
}

}  // namespace

UIMessageBox::UIMessageBox() = default;

UIMessageBox::~UIMessageBox() {
    cleanup();
}

bool UIMessageBox::initialize(TextRenderer* renderer) {
    if (!renderer) {
        LOGE("Error: TextRenderer is null");
        return false;
    }
    textRenderer = renderer;

    if (bgCenterTexture == 0) {
        const std::string dir = "textures/ui/menus/";
        bgCenterTexture =
            TextureLoader::loadTextureFromAsset(dir + "genericbackground__center_background.png");
        bgEdgeTopTexture = TextureLoader::loadTextureFromAsset(dir + "genericbackground__edge_top.png");
        bgEdgeBottomTexture =
            TextureLoader::loadTextureFromAsset(dir + "genericbackground__edge_bottom.png");
        bgEdgeLeftTexture = TextureLoader::loadTextureFromAsset(dir + "genericbackground__edge_left.png");
        bgEdgeRightTexture =
            TextureLoader::loadTextureFromAsset(dir + "genericbackground__edge_right.png");
        bgCornerTLTexture =
            TextureLoader::loadTextureFromAsset(dir + "genericbackground__top_left_corner.png");
        bgCornerTRTexture =
            TextureLoader::loadTextureFromAsset(dir + "genericbackground__top_right_corner.png");
        bgCornerBLTexture =
            TextureLoader::loadTextureFromAsset(dir + "genericbackground__bottom_left_corner.png");
        bgCornerBRTexture =
            TextureLoader::loadTextureFromAsset(dir + "genericbackground__bottom_right_corner.png");
    }

    LOGI("UIMessageBox initialized");
    return true;
}

void UIMessageBox::cleanup() {
    for (GLuint* tex : {&bgCenterTexture, &bgEdgeTopTexture, &bgEdgeBottomTexture,
                        &bgEdgeLeftTexture, &bgEdgeRightTexture, &bgCornerTLTexture,
                        &bgCornerTRTexture, &bgCornerBLTexture, &bgCornerBRTexture}) {
        if (*tex != 0) {
            TextureLoader::deleteTexture(*tex);
            *tex = 0;
        }
    }
    textRenderer = nullptr;
    visible = false;
}

void UIMessageBox::setScreenSize(int width, int height) {
    screenWidth = width;
    screenHeight = height;
}

void UIMessageBox::open(const std::string& body, const std::vector<std::string>& optionLabels,
                        Callback callback) {
    text = body.empty() ? std::string(DEFAULT_MESSAGE) : body;
    this->optionLabels = optionLabels;
    if (this->optionLabels.empty()) {
        this->optionLabels.push_back(DEFAULT_OPTION);
    }
    this->callback = std::move(callback);

    bodyLines.clear();
    buttonRects.clear();
    lineHeight = 0.0f;
    textBlockHeight = 0.0f;
    panelHeight = 0.0f;
    focusIndex = 0;
    hasSelection = false;
    pendingSelection = 0;
    visible = true;
}

void UIMessageBox::close() {
    // hasSelection is deliberately kept: pumpMessageBox() closes the menu before it
    // dispatches, so the callback still sees the option the player activated.
    visible = false;
}

UIMessageBox::Rect UIMessageBox::bodyRect() const {
    const float height = (panelHeight > 0.0f) ? panelHeight : 100.0f;
    return {(static_cast<float>(screenWidth) - BODY_WIDTH) * 0.5f,
            (static_cast<float>(screenHeight) - height) * 0.5f, BODY_WIDTH, height};
}

void UIMessageBox::layout() {
    if (!textRenderer) {
        return;
    }

    lineHeight = textRenderer->getTextHeight(LABEL_SCALE);
    if (lineHeight <= 0.0f) {
        lineHeight = 1.0f;
    }

    bodyLines.clear();
    const float wrapWidth = BODY_WIDTH - TEXT_WRAP_INSET;
    std::string paragraph;
    for (size_t i = 0; i <= text.size(); ++i) {
        if (i == text.size() || text[i] == '\n') {
            appendWrappedParagraph(textRenderer, paragraph, wrapWidth, LABEL_SCALE, bodyLines);
            paragraph.clear();
        } else {
            paragraph.push_back(text[i]);
        }
    }
    if (bodyLines.empty()) {
        bodyLines.push_back(std::string());
    }

    textBlockHeight = lineHeight * static_cast<float>(bodyLines.size());

    // message_menu.xml sets the background height (user1) to "last button bottom + 15",
    // and the first button to "text height + 30". Resolve it before the body rect so the
    // background, the text and the buttons all use the same origin in one frame.
    const float buttonsHeight =
        optionLabels.empty()
            ? 0.0f
            : static_cast<float>(optionLabels.size()) * (lineHeight + BUTTON_GAP) - BUTTON_GAP;
    panelHeight = TEXT_TOP + textBlockHeight + BUTTON_BOTTOM_PAD;
    if (!optionLabels.empty()) {
        panelHeight += TEXT_TO_BUTTONS + buttonsHeight;
    }

    const Rect body = bodyRect();
    buttonRects.clear();
    float y = body.y + TEXT_TOP + textBlockHeight + TEXT_TO_BUTTONS;
    for (size_t i = 0; i < optionLabels.size(); ++i) {
        const float width = textRenderer->getTextWidth(optionLabels[i], LABEL_SCALE);
        Rect r;
        r.w = width;
        r.h = lineHeight;
        r.x = body.x + (BODY_WIDTH - width) * 0.5f;
        r.y = y;
        buttonRects.push_back(r);
        y += lineHeight + BUTTON_GAP;
    }
}

void UIMessageBox::renderBackground(const Rect& b) {
    const GLuint slices[9] = {
        bgCenterTexture, bgEdgeTopTexture, bgEdgeBottomTexture, bgEdgeLeftTexture,
        bgEdgeRightTexture, bgCornerTLTexture, bgCornerTRTexture, bgCornerBLTexture,
        bgCornerBRTexture};
    UIDrawHelper::drawGenericBackground(slices, b.x, b.y, b.w, b.h, screenWidth,
                                        screenHeight);
}

void UIMessageBox::render() {
    if (!visible || !textRenderer) {
        return;
    }

    // 2D overlay: ignore depth left over from the 3D passes
    glDisable(GL_DEPTH_TEST);

    FontScope font(textRenderer, MESSAGE_FONT);
    layout();

    // The root carries alpha 0 and an explorefade of 0.25, so the world behind the
    // panel is faded out while the message owns the screen.
    UIDrawHelper::drawColoredQuad(0.0f, 0.0f, static_cast<float>(screenWidth),
                                  static_cast<float>(screenHeight),
                                  glm::vec4(0.0f, 0.0f, 0.0f, DIM_ALPHA), screenWidth,
                                  screenHeight);

    const Rect body = bodyRect();
    renderBackground(body);

    // message_text is a left justified block centred on the panel.
    float blockWidth = 0.0f;
    for (const std::string& line : bodyLines) {
        blockWidth = std::max(blockWidth, textRenderer->getTextWidth(line, LABEL_SCALE));
    }
    const float textX = body.x + (BODY_WIDTH - blockWidth) * 0.5f;
    for (size_t i = 0; i < bodyLines.size(); ++i) {
        textRenderer->renderText(bodyLines[i], textX,
                                 body.y + TEXT_TOP + lineHeight * static_cast<float>(i),
                                 kFocusedText, LABEL_SCALE);
    }

    for (size_t i = 0; i < buttonRects.size(); ++i) {
        const Rect& r = buttonRects[i];
        const bool focused = (static_cast<int>(i) == focusIndex);
        textRenderer->renderText(optionLabels[i], r.x, r.y + BUTTON_LABEL_Y,
                                 focused ? kFocusedText : kIdleText, LABEL_SCALE);
    }
}

int UIMessageBox::hitTestButton(float x, float y) const {
    for (size_t i = 0; i < buttonRects.size(); ++i) {
        const Rect& r = buttonRects[i];
        if (x >= r.x - BUTTON_TOUCH_PAD_X && x <= r.x + r.w + BUTTON_TOUCH_PAD_X &&
            y >= r.y - BUTTON_TOUCH_PAD_Y && y <= r.y + r.h + BUTTON_TOUCH_PAD_Y) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void UIMessageBox::activate(int index) {
    if (index < 0 || index >= static_cast<int>(optionLabels.size())) {
        return;
    }
    focusIndex = index;
    pendingSelection = index;
    hasSelection = true;
}

bool UIMessageBox::hasPendingSelection() const {
    return hasSelection;
}

int UIMessageBox::dispatchSelection() {
    if (!hasSelection) {
        return -1;
    }
    hasSelection = false;
    if (callback) {
        callback(pendingSelection);
    }
    return pendingSelection;
}

void UIMessageBox::onTouchEvent(float x, float y) {
    if (!visible) {
        return;
    }
    const int index = hitTestButton(x, y);
    if (index >= 0) {
        activate(index);
    }
}

bool UIMessageBox::handleBack() {
    if (!visible) {
        return false;
    }
    // Back maps to the dismissing choice: the last option carries sNo / sCancel.
    activate(static_cast<int>(optionLabels.size()) - 1);
    return true;
}

bool UIMessageBox::onKeyPress(int keyCode) {
    if (!visible) {
        return false;
    }
    const int count = static_cast<int>(optionLabels.size());
    switch (keyCode) {
        case 19:  // DPAD_UP
            focusIndex = (focusIndex + count - 1) % count;
            return true;
        case 20:  // DPAD_DOWN
            focusIndex = (focusIndex + 1) % count;
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
