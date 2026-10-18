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

    void open();
    void close();
    bool isVisible() const { return visible; }

    // RE 48: fade alpha applied to the whole menu (0 hidden .. 1 opaque). The
    // renderer drives this from MenuFadeSystem while the open/close fade runs.
    void setFadeAlpha(float alpha);

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
    float fadeAlpha;
    int focusIndex;

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
