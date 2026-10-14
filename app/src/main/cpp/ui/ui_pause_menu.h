#pragma once

#include <GLES3/gl3.h>
#include <glm/glm.hpp>
#include "text_renderer.h"

/**
 * @brief Pause menu, laid out from the original options\pause_menu.xml (PC values).
 *
 * The body is a 240x400 generic_background rect centred on the screen with the
 * 44px border drawn outside it. Five 64px buttons stack from the body top in XML
 * listindex order - Return / Save / Load / Options / Quit, which are the original
 * PauseMenu widget ids 3, 4, 5, 7 and 8. The blue decor pair and the "Paused"
 * caption reproduce the XML's pause_indication block.
 *
 * Open/close runs the original menufade transition (RE_EXE_REPORT §33.3): the
 * screen dim and every widget alpha ramp over 0.25s on open, and the same ramp
 * plays in reverse on close. The menu stops accepting input the moment it is
 * closed, so the fade-out is purely visual.
 */
class UIPauseMenu {
public:
    enum class Item {
        RETURN = 0,
        SAVE,
        LOAD,
        OPTIONS,
        QUIT
    };

    UIPauseMenu();
    ~UIPauseMenu();

    bool initialize(TextRenderer* textRenderer);
    void cleanup();

    void setScreenSize(int width, int height);

    // fadeIn=false shows the menu immediately without blending (sub-menu restore).
    void open(bool fadeIn = true);
    // fadeOut=false hides the menu instantly; used when a sub-menu opened from
    // the pause menu takes over the screen, so the restore does not re-blend.
    void close(bool fadeOut = true);
    bool isVisible() const { return visible; }
    // True while the open/close fade is still playing. The renderer must keep
    // drawing the menu during the close ramp even though isVisible() is false.
    bool isFading() const { return fadeProgress > 0.0f; }

    // Advances the open/close fade. dt is in seconds.
    void update(float dt);
    void render();
    void onTouchEvent(float x, float y);
    // Key codes match the other menus: 19 UP, 20 DOWN, 23/66 Enter, 4 Back.
    bool onKeyPress(int keyCode);
    bool handleBack();

    bool hasPendingSelection() const { return hasSelection; }
    // Returns the item activated since the last call, then clears it.
    Item consumeSelection();

private:
    struct Rect {
        float x, y, w, h;
    };

    Rect bodyRect() const;
    float bodyTop() const;
    Rect buttonRect(int index) const;
    int hitTestButton(float x, float y) const;
    void activate(int index);

    void renderBorder(const Rect& body);
    void renderButton(int index);

    TextRenderer* textRenderer;
    int screenWidth;
    int screenHeight;
    bool visible;
    int focusIndex;
    // 0.0 = fully faded out, 1.0 = fully shown. Scales the dim quad and every
    // widget alpha (RE_EXE_REPORT §33.3, menufade duration 0.25s).
    float fadeProgress;

    bool hasSelection;
    Item pendingSelection;

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
    GLuint decorLeftTexture;
    GLuint decorRightTexture;

    // Measured once per frame while rendering; the XML sizes the button rects
    // from the label width, so hit testing needs the same numbers.
    float cachedButtonWidths[5];
    float cachedPauseTextHeight;
};
