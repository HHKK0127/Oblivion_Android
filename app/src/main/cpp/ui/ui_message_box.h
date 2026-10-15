#pragma once

#include <GLES3/gl3.h>

#include <functional>
#include <string>
#include <vector>

#include <glm/glm.hpp>

#include "text_renderer.h"

/**
 * @brief MessageMenu (menu id 0x3e9) rebuilt from Data\Menus\message_menu.xml.
 *
 * Layout follows the original data:
 *   - the body rect is 700 px wide and the engine centres it on the screen
 *     (message_menu.xml defines neither x nor y),
 *   - the generic_background prefab paints the 44 px border outside that rect,
 *   - message_text wraps at background.user0 - 24 and sits 15 px below the top,
 *   - buttons stack from "text bottom + 30" with a 10 px gap, each row centred,
 *   - the panel height is the last button's bottom + 15,
 *   - an empty body falls back to sDefaultMessage and an empty option list to sOk.
 *
 * The original draws invisible 256x256 placeholders for the buttons, so only the
 * labels are visible; the focused row uses the shared menu text colours.
 */
class UIMessageBox {
public:
    using Callback = std::function<void(int value)>;

    UIMessageBox();
    ~UIMessageBox();

    bool initialize(TextRenderer* renderer);
    void cleanup();
    void setScreenSize(int width, int height);

    // Option values handed to the callback are the labels' indices (0-based), which
    // matches the original data where option 0 is sYes and option 1 is sNo.
    void open(const std::string& body, const std::vector<std::string>& optionLabels = {},
              Callback callback = nullptr);
    void close();
    bool isVisible() const { return visible; }

    void render();
    void onTouchEvent(float x, float y);
    bool onKeyPress(int keyCode);
    bool handleBack();

    bool hasPendingSelection() const;
    // Invokes the stored callback for the option activated since the last call and
    // returns its index, or -1 when nothing was selected.
    int dispatchSelection();

private:
    struct Rect {
        float x = 0.0f;
        float y = 0.0f;
        float w = 0.0f;
        float h = 0.0f;
    };

    Rect bodyRect() const;
    void layout();
    void renderBackground(const Rect& body);
    int hitTestButton(float x, float y) const;
    void activate(int index);

    TextRenderer* textRenderer = nullptr;
    int screenWidth = 1920;
    int screenHeight = 1080;
    bool visible = false;

    std::string text;
    std::vector<std::string> optionLabels;
    Callback callback;

    std::vector<std::string> bodyLines;
    std::vector<Rect> buttonRects;
    float lineHeight = 0.0f;
    float textBlockHeight = 0.0f;
    float panelHeight = 0.0f;

    int focusIndex = 0;
    bool hasSelection = false;
    int pendingSelection = 0;

    GLuint bgCenterTexture = 0;
    GLuint bgEdgeTopTexture = 0;
    GLuint bgEdgeBottomTexture = 0;
    GLuint bgEdgeLeftTexture = 0;
    GLuint bgEdgeRightTexture = 0;
    GLuint bgCornerTLTexture = 0;
    GLuint bgCornerTRTexture = 0;
    GLuint bgCornerBLTexture = 0;
    GLuint bgCornerBRTexture = 0;
};
