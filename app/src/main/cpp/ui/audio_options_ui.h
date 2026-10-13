#ifndef OBLIVION_AUDIO_OPTIONS_UI_H
#define OBLIVION_AUDIO_OPTIONS_UI_H

#include "text_renderer.h"
#include <GLES3/gl3.h>
#include <glm/glm.hpp>
#include <array>

// Audio submenu of the Options family (AudioMenu in the original XML, 900x500 PC layout).
// Five volume sliders plus Defaults and Return buttons. Values are normalized to 0..1.
class AudioOptionsUI {
public:
    struct Volumes {
        float master = 1.0f;
        float voice = 0.8f;
        float effect = 0.8f;
        float foot = 0.8f;
        float music = 0.3f;
    };

    AudioOptionsUI();
    ~AudioOptionsUI();

    bool initialize(TextRenderer* renderer);
    void cleanup();
    void setScreenSize(int width, int height);

    void open(const Volumes& initial);
    void close();
    bool isVisible() const { return visible; }

    // Set when Return was chosen; the owner reopens the Options screen.
    bool shouldReturnToOptions() const { return returnToOptions; }
    void resetReturnFlag() { returnToOptions = false; }

    // Set when a value changed; the owner pushes Volumes() to the audio system.
    bool consumeChanged();
    Volumes volumes() const;

    void onTouchDown(float x, float y);
    void onTouchMove(float x);
    void onTouchUp();
    bool onKeyPress(int keyCode);
    bool handleBack();

    void render();

private:
    enum Row : int {
        ROW_MASTER = 0,
        ROW_VOICE,
        ROW_EFFECT,
        ROW_FOOT,
        ROW_MUSIC,
        ROW_DEFAULTS,
        ROW_RETURN,
        ROW_COUNT
    };
    static constexpr int SLIDER_COUNT = 5;

    struct Rect {
        float x, y, w, h;
    };

    Rect bodyRect() const;
    Rect sliderLineRect(int slider) const;   // absolute screen coordinates
    Rect sliderHandleRect(int slider) const; // absolute screen coordinates
    Rect buttonRect(int row) const;          // absolute screen coordinates
    float labelLocalY(int slider) const;

    void measureLabels();
    void adjustSlider(int slider, float delta);
    void setSliderFromX(int slider, float screenX);
    void setValue(int slider, float value);
    void resetToDefaults();
    void activateRow(int row);
    void focusRow(int row);

    void renderBorder(const Rect& b);
    void renderSlider(int slider);
    void renderButton(int row, const char* label);

    TextRenderer* textRenderer;
    int screenWidth;
    int screenHeight;
    bool visible;
    bool returnToOptions;
    bool changed;
    int focusIndex;
    int dragSlider;
    float longLabelWidth;
    float labelHeight;

    std::array<float, SLIDER_COUNT> values;

    GLuint bgCenterTexture;
    GLuint bgEdgeTopTexture;
    GLuint bgEdgeBottomTexture;
    GLuint bgEdgeLeftTexture;
    GLuint bgEdgeRightTexture;
    GLuint bgCornerTLTexture;
    GLuint bgCornerTRTexture;
    GLuint bgCornerBLTexture;
    GLuint bgCornerBRTexture;
    GLuint selectionFullTexture;
    GLuint selectionCutTexture;
    GLuint scrollLineTexture;
    GLuint scrollPartsTexture;
    GLuint buttonOffTexture;
    GLuint buttonOnTexture;
};

#endif // OBLIVION_AUDIO_OPTIONS_UI_H
