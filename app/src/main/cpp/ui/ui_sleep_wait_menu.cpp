#include "ui_sleep_wait_menu.h"
#include "text_renderer.h"
#include "ui_draw_helper.h"
#include "../engine/texture_loader.h"
#include "../engine/sky_weather_system.h"

#include <algorithm>
#include <android/log.h>
#include <cstdio>

#undef LOG_TAG
#undef LOGD
#undef LOGI
#undef LOGE
#define LOG_TAG "UISleepWaitMenu"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {

constexpr float kDimAlpha = 0.6f;
constexpr float kPanelW = 400.0f;
constexpr float kPanelH = 360.0f;
constexpr float kSliderW = 280.0f;
constexpr float kSliderH = 24.0f;
constexpr float kSliderThumbW = 16.0f;
constexpr float kSliderThumbH = 32.0f;
constexpr float kModeBtnW = 140.0f;
constexpr float kModeBtnH = 36.0f;
constexpr float kModeBtnY = 80.0f;
constexpr float kModeBtn1X = 220.0f;
constexpr float kModeBtn2X = 380.0f;
constexpr float kSliderY = 150.0f;
constexpr float kDateTimeY = 200.0f;
constexpr float kHoursTextY = 120.0f;
constexpr float kActionBtnY = 280.0f;
constexpr float kAcceptBtnX = 250.0f;
constexpr float kCancelBtnX = 410.0f;
constexpr float kActionBtnW = 140.0f;
constexpr float kActionBtnH = 40.0f;
constexpr float kPanelCenterX = 640.0f;
constexpr float kPanelCenterY = 480.0f;
constexpr float kPanelX = kPanelCenterX - kPanelW * 0.5f;
constexpr float kPanelY = kPanelCenterY - kPanelH * 0.5f;

constexpr FontType kFont = FontType::KingthingsRegular;
constexpr FontType kSmallFont = FontType::TahomaBoldSmall;

const glm::vec4 kFocusedText(117.0f / 255.0f, 59.0f / 255.0f, 33.0f / 255.0f, 1.0f);
const glm::vec4 kIdleText(128.0f / 255.0f, 128.0f / 255.0f, 128.0f / 255.0f, 196.0f / 255.0f);
const glm::vec4 kHeaderText(51.0f / 255.0f, 79.0f / 255.0f, 142.0f / 255.0f, 1.0f);
const glm::vec4 kButtonBg(0.2f, 0.2f, 0.35f, 0.5f);
const glm::vec4 kButtonActiveBg(0.4f, 0.5f, 0.8f, 0.5f);
const glm::vec4 kSliderTrack(0.15f, 0.15f, 0.25f, 0.7f);
const glm::vec4 kSliderFill(0.3f, 0.5f, 0.9f, 0.8f);

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

float centeredTextY(float rowTop, float rowHeight, TextRenderer* renderer, float scale) {
    const float th = renderer ? renderer->getTextCapHeight(scale) : 0.0f;
    return rowTop + (rowHeight - th) * 0.5f;
}

} // namespace

UISleepWaitMenu::UISleepWaitMenu()
    : textRenderer_(nullptr), screenWidth_(1280), screenHeight_(960),
      visible_(false), fastForwarding_(false), fadeAlpha_(1.0f),
      sleepMode_(true), selectedHours_(1), pendingHours_(0),
      ffAccumulator_(0.0f), ffRemainingHours_(0),
      sliderValue_(0.0f),
      bgCenterTexture_(0), buttonTexture_(0), sliderTrackTexture_(0), sliderThumbTexture_(0) {
}

UISleepWaitMenu::~UISleepWaitMenu() {
    cleanup();
}

bool UISleepWaitMenu::initialize(TextRenderer* renderer) {
    if (!renderer) {
        LOGE("TextRenderer is null");
        return false;
    }
    textRenderer_ = renderer;

    if (bgCenterTexture_ == 0) {
        const std::string dir = "textures/ui/menus/";
        bgCenterTexture_ = TextureLoader::loadTextureFromAsset(dir + "genericbackground__center_background.png");
    }
    if (buttonTexture_ == 0) {
        buttonTexture_ = TextureLoader::loadTextureFromAsset("textures/ui/menus/dialog__dialog_selection_full.png");
    }
    if (sliderTrackTexture_ == 0) {
        sliderTrackTexture_ = TextureLoader::loadTextureFromAsset("textures/ui/menus/shared__slider_track.png");
    }
    if (sliderThumbTexture_ == 0) {
        sliderThumbTexture_ = TextureLoader::loadTextureFromAsset("textures/ui/menus/shared__slider_thumb.png");
    }

    LOGI("UISleepWaitMenu initialized");
    return true;
}

void UISleepWaitMenu::cleanup() {
    for (GLuint* tex : {&bgCenterTexture_, &buttonTexture_,
                        &sliderTrackTexture_, &sliderThumbTexture_}) {
        if (*tex != 0) {
            TextureLoader::deleteTexture(*tex);
            *tex = 0;
        }
    }
    textRenderer_ = nullptr;
    visible_ = false;
    fastForwarding_ = false;
}

void UISleepWaitMenu::setScreenSize(int width, int height) {
    if (width <= 0 || height <= 0) return;
    screenWidth_ = width;
    screenHeight_ = height;
}

void UISleepWaitMenu::setFadeAlpha(float alpha) {
    fadeAlpha_ = std::clamp(alpha, 0.0f, 1.0f);
}

void UISleepWaitMenu::open(bool isSleep) {
    sleepMode_ = isSleep;
    selectedHours_ = kSliderMin;
    sliderValue_ = 0.0f;
    pendingHours_ = 0;
    ffAccumulator_ = 0.0f;
    ffRemainingHours_ = 0;
    fastForwarding_ = false;
    visible_ = true;
    LOGI("SleepWaitMenu opened (mode=%s)", isSleep ? "sleep" : "wait");
}

void UISleepWaitMenu::close() {
    if (fastForwarding_) {
        fastForwarding_ = false;
        ffRemainingHours_ = 0;
    }
    visible_ = false;
    if (onCancel_) onCancel_();
    LOGI("SleepWaitMenu closed");
}

int UISleepWaitMenu::consumeHours() {
    int h = pendingHours_;
    pendingHours_ = 0;
    return h;
}

void UISleepWaitMenu::update(float deltaTime) {
    if (!visible_ || !fastForwarding_) return;

    ffAccumulator_ += deltaTime;
    while (ffAccumulator_ >= 1.0f && ffRemainingHours_ > 0) {
        ffAccumulator_ -= 1.0f;
        advanceOneHour();
    }
}

void UISleepWaitMenu::advanceOneHour() {
    ffRemainingHours_--;

    float currentHours = engine::SkyWeatherSystem::instance().getGameTime();
    currentHours = std::fmod(currentHours + 1.0f, 24.0f);
    if (currentHours < 0.0f) currentHours += 24.0f;
    engine::SkyWeatherSystem::instance().setGameTime(currentHours);

    if (ffRemainingHours_ <= 0) {
        endFastForward();
    }
}

void UISleepWaitMenu::beginFastForward() {
    fastForwarding_ = true;
    ffAccumulator_ = 0.0f;
    ffRemainingHours_ = selectedHours_;
    pendingHours_ = selectedHours_;
    LOGI("Fast-forward started: %d hours", selectedHours_);
}

void UISleepWaitMenu::endFastForward() {
    fastForwarding_ = false;
    ffRemainingHours_ = 0;
    if (onConfirm_) onConfirm_();
    visible_ = false;
    LOGI("Fast-forward finished");
}

void UISleepWaitMenu::onTouchEvent(float x, float y) {
    if (!visible_ || fastForwarding_) return;

    const float sx = scaleX();
    const float sy = scaleY();
    const float ox = originX();
    const float oy = originY();
    const float uiX = (x - ox) / sx;
    const float uiY = (y - oy) / sy;

    const int modeHit = hitTestModeButton(uiX, uiY);
    if (modeHit >= 0) {
        sleepMode_ = (modeHit == 1);
        LOGI("Mode toggled: %s", sleepMode_ ? "sleep" : "wait");
        return;
    }

    const int sliderHit = hitTestSlider(uiX, uiY);
    if (sliderHit >= 0) {
        sliderValue_ = std::clamp(static_cast<float>(sliderHit) / (kSliderMax - kSliderMin), 0.0f, 1.0f);
        selectedHours_ = std::clamp(sliderHit, kSliderMin, kSliderMax);
        LOGI("Slider set to %d hours", selectedHours_);
        return;
    }

    const int actionHit = hitTestActionButton(uiX, uiY);
    if (actionHit == 0) {
        beginFastForward();
    } else if (actionHit == 1) {
        close();
    }
}

bool UISleepWaitMenu::onKeyPress(int keyCode) {
    if (!visible_) return false;

    if (fastForwarding_) {
        if (keyCode == 4 || keyCode == 10) {
            endFastForward();
            return true;
        }
        return false;
    }

    switch (keyCode) {
        case 19:
        case 20: {
            sleepMode_ = (keyCode == 20);
            return true;
        }
        case 21: {
            selectedHours_ = std::max(kSliderMin, selectedHours_ - 1);
            sliderValue_ = static_cast<float>(selectedHours_ - kSliderMin) /
                           static_cast<float>(kSliderMax - kSliderMin);
            return true;
        }
        case 22: {
            selectedHours_ = std::min(kSliderMax, selectedHours_ + 1);
            sliderValue_ = static_cast<float>(selectedHours_ - kSliderMin) /
                           static_cast<float>(kSliderMax - kSliderMin);
            return true;
        }
        case 23:
        case 66:
            beginFastForward();
            return true;
        case 4:
            return handleBack();
        default:
            return false;
    }
}

bool UISleepWaitMenu::handleBack() {
    if (!visible_) return false;
    if (fastForwarding_) {
        endFastForward();
        return true;
    }
    close();
    return true;
}

float UISleepWaitMenu::scaleX() const {
    return static_cast<float>(screenWidth_) / static_cast<float>(kCanvasWidth);
}

float UISleepWaitMenu::scaleY() const {
    return static_cast<float>(screenHeight_) / static_cast<float>(kCanvasHeight);
}

float UISleepWaitMenu::originX() const {
    return (static_cast<float>(screenWidth_) - static_cast<float>(kCanvasWidth) * scaleX()) * 0.5f;
}

float UISleepWaitMenu::originY() const {
    return (static_cast<float>(screenHeight_) - static_cast<float>(kCanvasHeight) * scaleY()) * 0.5f;
}

int UISleepWaitMenu::hitTestModeButton(float uiX, float uiY) const {
    const float px = kPanelX;
    const float py = kPanelY;

    const Rect r1 = {px + kModeBtn1X, py + kModeBtnY, kModeBtnW, kModeBtnH};
    if (uiX >= r1.x && uiX <= r1.x + r1.w && uiY >= r1.y && uiY <= r1.y + r1.h) {
        return 1;
    }
    const Rect r2 = {px + kModeBtn2X, py + kModeBtnY, kModeBtnW, kModeBtnH};
    if (uiX >= r2.x && uiX <= r2.x + r2.w && uiY >= r2.y && uiY <= r2.y + r2.h) {
        return 2;
    }
    return -1;
}

int UISleepWaitMenu::hitTestSlider(float uiX, float uiY) const {
    const float px = kPanelX;
    const float py = kPanelY;
    const float sliderX = px + (kPanelW - kSliderW) * 0.5f;

    if (uiY < py + kSliderY || uiY > py + kSliderY + kSliderH) return -1;
    if (uiX < sliderX || uiX > sliderX + kSliderW) return -1;

    const float t = (uiX - sliderX) / kSliderW;
    const int hours = kSliderMin + static_cast<int>(t * static_cast<float>(kSliderMax - kSliderMin) + 0.5f);
    return std::clamp(hours, kSliderMin, kSliderMax);
}

int UISleepWaitMenu::hitTestActionButton(float uiX, float uiY) const {
    const float px = kPanelX;
    const float py = kPanelY;

    const Rect rAccept = {px + kAcceptBtnX, py + kActionBtnY, kActionBtnW, kActionBtnH};
    if (uiX >= rAccept.x && uiX <= rAccept.x + rAccept.w &&
        uiY >= rAccept.y && uiY <= rAccept.y + rAccept.h) {
        return 0;
    }
    const Rect rCancel = {px + kCancelBtnX, py + kActionBtnY, kActionBtnW, kActionBtnH};
    if (uiX >= rCancel.x && uiX <= rCancel.x + rCancel.w &&
        uiY >= rCancel.y && uiY <= rCancel.y + rCancel.h) {
        return 1;
    }
    return -1;
}

void UISleepWaitMenu::render() {
    if (!visible_ || !textRenderer_) return;

    GLboolean depthWasEnabled = GL_TRUE;
    glGetBooleanv(GL_DEPTH_TEST, &depthWasEnabled);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    drawBackground();
    drawModeButtons();
    drawSlider();
    drawDateTime();
    drawActionButtons();

    if (fastForwarding_) {
        drawFastForwardOverlay();
    }

    if (depthWasEnabled) glEnable(GL_DEPTH_TEST);
    else glDisable(GL_DEPTH_TEST);
}

void UISleepWaitMenu::drawBackground() {
    const float alpha = fadeAlpha_;
    UIDrawHelper::drawColoredQuad(0.0f, 0.0f,
                                  static_cast<float>(screenWidth_),
                                  static_cast<float>(screenHeight_),
                                  glm::vec4(0.0f, 0.0f, 0.0f, kDimAlpha * alpha),
                                  screenWidth_, screenHeight_);

    const float sx = scaleX();
    const float sy = scaleY();
    const float ox = originX();
    const float oy = originY();

    const float absX = ox + kPanelX * sx;
    const float absY = oy + kPanelY * sy;
    const float absW = kPanelW * sx;
    const float absH = kPanelH * sy;

    const GLuint slices[9] = {bgCenterTexture_, 0, 0, 0, 0, 0, 0, 0, 0};
    UIDrawHelper::drawGenericBackground(slices, absX, absY, absW, absH,
                                         screenWidth_, screenHeight_, alpha);

    UIDrawHelper::drawBorder(absX, absY, absW, absH, 2.0f,
                              glm::vec4(0.3f, 0.4f, 0.7f, 0.6f * alpha),
                              screenWidth_, screenHeight_);

    {
        FontScope font(textRenderer_, kFont);
        const char* title = sleepMode_ ? "Rest" : "Wait";
        const float tw = textRenderer_->getTextWidth(title, sy);
        glm::vec4 color = kHeaderText;
        color.w *= alpha;
        const float titleY = oy + (kPanelY + 30.0f) * sy;
        textRenderer_->renderText(title, ox + (kPanelX + (kPanelW - tw * 0.5f / sy) * 0.5f) * sx,
                                  titleY, color, sy);
    }
}

void UISleepWaitMenu::drawModeButtons() {
    const float sx = scaleX();
    const float sy = scaleY();
    const float ox = originX();
    const float oy = originY();
    const float alpha = fadeAlpha_;

    const float absY = oy + (kPanelY + kModeBtnY) * sy;
    const float absH = kModeBtnH * sy;

    {
        const float absX1 = ox + (kPanelX + kModeBtn1X) * sx;
        const float absW1 = kModeBtnW * sx;
        const bool sleepActive = sleepMode_;
        glm::vec4 bg = sleepActive ? kButtonActiveBg : kButtonBg;
        bg.w *= alpha;
        UIDrawHelper::drawColoredQuad(absX1, absY, absW1, absH, bg, screenWidth_, screenHeight_);

        FontScope font(textRenderer_, kSmallFont);
        glm::vec4 color = sleepActive ? kFocusedText : kIdleText;
        color.w *= alpha;
        const float textY = centeredTextY(absY, absH, textRenderer_, sy);
        float tw = textRenderer_->getTextWidth("Sleep", sy);
        textRenderer_->renderText("Sleep", absX1 + (absW1 - tw) * 0.5f, textY, color, sy);
    }
    {
        const float absX2 = ox + (kPanelX + kModeBtn2X) * sx;
        const float absW2 = kModeBtnW * sx;
        const bool waitActive = !sleepMode_;
        glm::vec4 bg = waitActive ? kButtonActiveBg : kButtonBg;
        bg.w *= alpha;
        UIDrawHelper::drawColoredQuad(absX2, absY, absW2, absH, bg, screenWidth_, screenHeight_);

        FontScope font(textRenderer_, kSmallFont);
        glm::vec4 color = waitActive ? kFocusedText : kIdleText;
        color.w *= alpha;
        const float textY = centeredTextY(absY, absH, textRenderer_, sy);
        float tw = textRenderer_->getTextWidth("Wait", sy);
        textRenderer_->renderText("Wait", absX2 + (absW2 - tw) * 0.5f, textY, color, sy);
    }
}

void UISleepWaitMenu::drawSlider() {
    const float sx = scaleX();
    const float sy = scaleY();
    const float ox = originX();
    const float oy = originY();
    const float alpha = fadeAlpha_;

    const float px = kPanelX;
    const float py = kPanelY;
    const float sliderX = px + (kPanelW - kSliderW) * 0.5f;

    const float absX = ox + sliderX * sx;
    const float absY = oy + (py + kSliderY) * sy;
    const float absW = kSliderW * sx;
    const float absH = kSliderH * sy;

    UIDrawHelper::drawColoredQuad(absX, absY, absW, absH,
                                   glm::vec4(kSliderTrack.x, kSliderTrack.y, kSliderTrack.z, kSliderTrack.w * alpha),
                                   screenWidth_, screenHeight_);

    const float fillFrac = sliderValue_;
    float fillW = absW * fillFrac;
    if (fillW > 0.0f) {
        UIDrawHelper::drawColoredQuad(absX, absY, fillW, absH,
                                       glm::vec4(kSliderFill.x, kSliderFill.y, kSliderFill.z, kSliderFill.w * alpha),
                                       screenWidth_, screenHeight_);
    }

    const float thumbX = absX + (absW - kSliderThumbW * sx) * fillFrac;
    const float thumbY = absY - (kSliderThumbH * sy - absH) * 0.5f;
    if (sliderThumbTexture_ != 0) {
        UIDrawHelper::drawTexturedQuad(thumbX, thumbY, kSliderThumbW * sx, kSliderThumbH * sy,
                                        sliderThumbTexture_,
                                        glm::vec4(1.0f, 1.0f, 1.0f, alpha),
                                        screenWidth_, screenHeight_);
    } else {
        UIDrawHelper::drawColoredQuad(thumbX, thumbY, kSliderThumbW * sx, kSliderThumbH * sy,
                                       glm::vec4(0.8f, 0.8f, 1.0f, alpha),
                                       screenWidth_, screenHeight_);
    }

    {
        FontScope font(textRenderer_, kSmallFont);
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%d hours", selectedHours_);
        const float textY = oy + (py + kHoursTextY) * sy;
        const float tw = textRenderer_->getTextWidth(buf, sy);
        glm::vec4 color = kHeaderText;
        color.w *= alpha;
        textRenderer_->renderText(buf, ox + (px + (kPanelW - tw * 0.5f / sy) * 0.5f) * sx,
                                  textY, color, sy);
    }
}

void UISleepWaitMenu::drawDateTime() {
    const float sx = scaleX();
    const float sy = scaleY();
    const float ox = originX();
    const float oy = originY();
    const float alpha = fadeAlpha_;

    const float textY = oy + (kPanelY + kDateTimeY) * sy;

    float currentHour = engine::SkyWeatherSystem::instance().getGameTime();
    int curH = static_cast<int>(currentHour);
    int curM = static_cast<int>((currentHour - curH) * 60.0f);
    curH %= 24;

    float afterHour = std::fmod(currentHour + selectedHours_, 24.0f);
    int aftH = static_cast<int>(afterHour);
    int aftM = static_cast<int>((afterHour - aftH) * 60.0f);
    aftH %= 24;

    const char* curAmPm = (curH < 12) ? "AM" : "PM";
    const char* aftAmPm = (aftH < 12) ? "AM" : "PM";
    int cur12 = curH % 12; if (cur12 == 0) cur12 = 12;
    int aft12 = aftH % 12; if (aft12 == 0) aft12 = 12;

    FontScope font(textRenderer_, kSmallFont);
    glm::vec4 color = kHeaderText;
    color.w *= alpha;

    char buf[64];
    std::snprintf(buf, sizeof(buf), "Now: %d:%02d %s", cur12, curM, curAmPm);
    const float tw1 = textRenderer_->getTextWidth(buf, sy);
    textRenderer_->renderText(buf, ox + (kPanelX + (kPanelW - tw1 * 0.5f / sy) * 0.5f) * sx,
                              textY, color, sy);

    std::snprintf(buf, sizeof(buf), "After: %d:%02d %s", aft12, aftM, aftAmPm);
    const float tw2 = textRenderer_->getTextWidth(buf, sy);
    textRenderer_->renderText(buf, ox + (kPanelX + (kPanelW - tw2 * 0.5f / sy) * 0.5f) * sx,
                              textY + 22.0f * sy, color, sy);
}

void UISleepWaitMenu::drawActionButtons() {
    const float sx = scaleX();
    const float sy = scaleY();
    const float ox = originX();
    const float oy = originY();
    const float alpha = fadeAlpha_;

    const float absY = oy + (kPanelY + kActionBtnY) * sy;
    const float absH = kActionBtnH * sy;

    {
        const float absX = ox + (kPanelX + kAcceptBtnX) * sx;
        const float absW = kActionBtnW * sx;
        UIDrawHelper::drawColoredQuad(absX, absY, absW, absH,
                                       glm::vec4(kButtonBg.x, kButtonBg.y, kButtonBg.z, kButtonBg.w * alpha),
                                       screenWidth_, screenHeight_);
        UIDrawHelper::drawBorder(absX, absY, absW, absH, 1.0f,
                                  glm::vec4(0.3f, 0.5f, 0.8f, 0.6f * alpha),
                                  screenWidth_, screenHeight_);
        FontScope font(textRenderer_, kSmallFont);
        glm::vec4 color = kFocusedText;
        color.w *= alpha;
        const char* label = sleepMode_ ? "Sleep" : "Wait";
        const float tw = textRenderer_->getTextWidth(label, sy);
        const float textY = centeredTextY(absY, absH, textRenderer_, sy);
        textRenderer_->renderText(label, absX + (absW - tw) * 0.5f, textY, color, sy);
    }
    {
        const float absX = ox + (kPanelX + kCancelBtnX) * sx;
        const float absW = kActionBtnW * sx;
        UIDrawHelper::drawColoredQuad(absX, absY, absW, absH,
                                       glm::vec4(kButtonBg.x, kButtonBg.y, kButtonBg.z, kButtonBg.w * alpha),
                                       screenWidth_, screenHeight_);
        UIDrawHelper::drawBorder(absX, absY, absW, absH, 1.0f,
                                  glm::vec4(0.3f, 0.5f, 0.8f, 0.6f * alpha),
                                  screenWidth_, screenHeight_);
        FontScope font(textRenderer_, kSmallFont);
        glm::vec4 color = kIdleText;
        color.w *= alpha;
        const float tw = textRenderer_->getTextWidth("Cancel", sy);
        const float textY = centeredTextY(absY, absH, textRenderer_, sy);
        textRenderer_->renderText("Cancel", absX + (absW - tw) * 0.5f, textY, color, sy);
    }
}

void UISleepWaitMenu::drawFastForwardOverlay() {
    const float sx = scaleX();
    const float sy = scaleY();
    const float ox = originX();
    const float oy = originY();
    const float alpha = 0.9f;

    UIDrawHelper::drawColoredQuad(0.0f, 0.0f,
                                  static_cast<float>(screenWidth_),
                                  static_cast<float>(screenHeight_),
                                  glm::vec4(0.0f, 0.0f, 0.0f, 0.5f * alpha),
                                  screenWidth_, screenHeight_);

    FontScope font(textRenderer_, kFont);
    glm::vec4 color = kHeaderText;
    color.w *= alpha;

    const char* action = sleepMode_ ? "Sleeping" : "Waiting";
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%s... %d hours remaining", action, ffRemainingHours_);
    const float tw = textRenderer_->getTextWidth(buf, sy);
    const float cx = static_cast<float>(screenWidth_) * 0.5f;
    const float cy = static_cast<float>(screenHeight_) * 0.5f;
    textRenderer_->renderText(buf, cx - tw * 0.5f, cy, color, sy);
}