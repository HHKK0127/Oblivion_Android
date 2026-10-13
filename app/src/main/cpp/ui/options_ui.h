#pragma once

#include <glm/glm.hpp>
#include <GLES3/gl3.h>
#include "text_renderer.h"

// Top-level Options menu laid out from the original OptionsMenu XML (PC values).
// Body is 240x360 (400 when opened from pause); the 44px generic border sits outside it.
// Buttons are 64px tall, stacked from the top of the body in XML order.
// Sub-category buttons only report a request; their contents are shown by the caller.
class OptionsUI {
public:
    enum class Category {
        NONE,
        GAMEPLAY,
        VIDEO,
        AUDIO,
        CONTROLS
    };

    OptionsUI();
    ~OptionsUI();

    bool initialize(TextRenderer* textRenderer);
    void cleanup();

    void setScreenSize(int width, int height);
    void open(bool fromPause);
    void close();
    // Hide while a child screen is shown; focus is preserved for resume().
    void suspend() { visible = false; pendingCategory = Category::NONE; }
    void resume() { visible = true; pendingCategory = Category::NONE; }
    bool isVisible() const { return visible; }

    void render();
    void onTouchEvent(float x, float y);

    // Back key / Return button. Returns true when the event was consumed.
    bool handleBack();

    // Keyboard / D-pad navigation. Returns true when the key was consumed.
    bool onKeyPress(int keyCode);

    bool shouldReturnToMenu() const { return returnToMenu; }
    void resetReturnFlag() { returnToMenu = false; }

    // Returns the sub-category requested since the last call and clears it.
    Category consumeCategoryRequest();

private:
    struct Rect {
        float x, y, w, h;
    };

    Rect bodyRect() const;
    float bodyTop() const;
    float buttonWidth(int index) const;
    Rect buttonRect(int index) const;
    int buttonCount() const;
    int hitTestButton(float x, float y) const;
    void activateButton(int index);

    void renderBorder(const Rect& body);
    void renderButton(int index);

    TextRenderer* textRenderer;
    int screenWidth;
    int screenHeight;
    bool visible;
    bool fromPause;
    bool returnToMenu;
    int focusIndex;
    Category pendingCategory;

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
    GLuint pauseDecorLeftTexture;
    GLuint pauseDecorRightTexture;

    // Measured during render() so touch handling never touches font state.
    float cachedButtonWidths[5];
    float cachedPauseTextHeight;
};
