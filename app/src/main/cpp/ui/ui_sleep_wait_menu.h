#pragma once

#include <functional>
#include <GLES3/gl3.h>
#include <glm/glm.hpp>

class TextRenderer;

class UISleepWaitMenu {
public:
    UISleepWaitMenu();
    ~UISleepWaitMenu();

    bool initialize(TextRenderer* renderer);
    void cleanup();

    void open(bool isSleep);
    void close();
    bool isVisible() const { return visible_; }
    bool isFastForwarding() const { return fastForwarding_; }

    void setScreenSize(int width, int height);
    void setFadeAlpha(float alpha);

    int consumeHours();
    bool isSleepMode() const { return sleepMode_; }

    void update(float deltaTime);
    void render();

    void onTouchEvent(float x, float y);
    bool onKeyPress(int keyCode);
    bool handleBack();

    using Callback = std::function<void()>;
    void setOnConfirmCallback(Callback cb) { onConfirm_ = std::move(cb); }
    void setOnCancelCallback(Callback cb) { onCancel_ = std::move(cb); }

private:
    static constexpr int kCanvasWidth = 1280;
    static constexpr int kCanvasHeight = 960;
    static constexpr int kSliderMin = 1;
    static constexpr int kSliderMax = 24;
    static constexpr int kSliderJump = 6;

    struct Rect { float x, y, w, h; };

    void drawBackground();
    void drawModeButtons();
    void drawSlider();
    void drawDateTime();
    void drawActionButtons();
    void drawFastForwardOverlay();

    int hitTestModeButton(float touchX, float touchY) const;
    int hitTestSlider(float touchX, float touchY) const;
    int hitTestActionButton(float touchX, float touchY) const;

    void beginFastForward();
    void advanceOneHour();
    void endFastForward();

    float scaleX() const;
    float scaleY() const;
    float originX() const;
    float originY() const;

    TextRenderer* textRenderer_;
    int screenWidth_;
    int screenHeight_;
    bool visible_;
    bool fastForwarding_;
    float fadeAlpha_;
    bool sleepMode_;
    int selectedHours_;
    int pendingHours_;

    float ffAccumulator_;
    int ffRemainingHours_;

    float sliderValue_;

    Callback onConfirm_;
    Callback onCancel_;

    GLuint bgCenterTexture_;
    GLuint buttonTexture_;
    GLuint sliderTrackTexture_;
    GLuint sliderThumbTexture_;
};