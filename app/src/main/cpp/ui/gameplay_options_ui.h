#ifndef OBLIVION_GAMEPLAY_OPTIONS_UI_H
#define OBLIVION_GAMEPLAY_OPTIONS_UI_H

#include "text_renderer.h"
#include <GLES3/gl3.h>
#include <glm/glm.hpp>

// Gameplay submenu of the Options family (GameplayMenu in the original XML, 790x528 PC layout).
// One difficulty slider (0..100), six On/Off toggles, plus Defaults and Return buttons.
class GameplayOptionsUI {
public:
    struct Settings {
        int difficulty = 50;            // Oblivion_default.ini iDifficultyLevel
        bool generalSubtitles = false;  // bGeneralSubtitles
        bool dialogueSubtitles = true;  // bDialogueSubtitles
        bool crosshair = true;          // bCrossHair
        bool saveOnRest = true;         // bSaveOnRest
        bool saveOnWait = true;         // bSaveOnWait
        bool saveOnTravel = true;       // bSaveOnTravel
    };

    GameplayOptionsUI();
    ~GameplayOptionsUI();

    bool initialize(TextRenderer* renderer);
    void cleanup();
    void setScreenSize(int width, int height);

    void open(const Settings& initial);
    void close();
    bool isVisible() const { return visible; }

    // Set when Return was chosen; the owner reopens the Options screen.
    bool shouldReturnToOptions() const { return returnToOptions; }
    void resetReturnFlag() { returnToOptions = false; }

    // Set when a value changed; the owner reads settings() to apply them.
    bool consumeChanged();
    Settings settings() const { return current; }

    // Seed the panel from persisted options without opening it.
    void setSettings(const Settings& value) { current = value; }

    void onTouchDown(float x, float y);
    void onTouchMove(float x);
    void onTouchUp();
    bool onKeyPress(int keyCode);
    bool handleBack();

    void render();

private:
    enum Row : int {
        ROW_DIFFICULTY = 0,
        ROW_GENERAL_SUBTITLES,
        ROW_DIALOGUE_SUBTITLES,
        ROW_CROSSHAIR,
        ROW_SAVE_REST,
        ROW_SAVE_WAIT,
        ROW_SAVE_TRAVEL,
        ROW_DEFAULTS,
        ROW_RETURN,
        ROW_COUNT
    };

    struct Rect {
        float x, y, w, h;
    };

    Rect bodyRect() const;
    float labelLocalY(int row) const;
    Rect sliderLineRect() const;     // absolute screen coordinates
    Rect sliderHandleRect() const;   // absolute screen coordinates
    Rect toggleRect(int row) const;  // absolute screen coordinates
    Rect buttonRect(int row) const;  // absolute screen coordinates

    void measureLabels();
    bool isToggleRow(int row) const;
    bool getToggle(int row) const;
    void setToggle(int row, bool on);
    void toggleRow(int row);
    void setDifficulty(int value);
    void setDifficultyFromX(float screenX);
    void resetToDefaults();
    void activateRow(int row);
    void focusRow(int row);

    void renderBorder(const Rect& b);
    void renderSlider();
    void renderToggle(int row);
    void renderButton(int row, const char* label);

    TextRenderer* textRenderer;
    int screenWidth;
    int screenHeight;
    bool visible;
    bool returnToOptions;
    bool changed;
    bool dragging;
    int focusIndex;
    float longLabelWidth;
    float labelHeight;

    Settings current;

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

#endif // OBLIVION_GAMEPLAY_OPTIONS_UI_H
