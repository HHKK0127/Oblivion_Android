#pragma once

#include <string>
#include <vector>

#include "text_renderer.h"
#include <GLES3/gl3.h>
#include <glm/glm.hpp>

// Resolution list panel (VideoDisplayMenu) rebuilt from the original PC layout
// (menus/options/video_display_menu.xml). Unlike the other option screens this
// menu does not use generic_background: a single 520x680 page image is centred
// on screen (x = screen.width / 2 - width / 2, likewise for y) and holds a
// localised title, the resolution lines and a Return button.
//
// The lines come from system/display_modes.h, which holds the modes the platform
// reports (RE section 36: the original enumerates them at runtime instead of
// reading them from XML).
class VideoDisplayUI {
public:
    VideoDisplayUI();
    ~VideoDisplayUI();

    bool initialize(TextRenderer* text_renderer);
    void cleanup();

    void setScreenSize(int width, int height);

    // selected_index highlights the line the caller currently uses (-1 = none).
    void open(int selected_index);
    void close();
    bool isVisible() const { return visible_; }

    bool shouldReturn() const { return return_requested_; }
    void resetReturnFlag() { return_requested_ = false; }

    void render();

    bool onTouchDown(float x, float y);
    bool onKeyPress(int key_code);
    bool handleBack();

private:
    struct Rect {
        float x = 0.0f;
        float y = 0.0f;
        float w = 0.0f;
        float h = 0.0f;
    };

    Rect panelRect() const;
    Rect returnButtonRect() const;
    float lineLocalY(int index) const;

    bool pointInRect(float px, float py, const Rect& rect) const;

    void renderPanel();
    void renderTexts();
    void renderReturnButton();

    TextRenderer* text_renderer_ = nullptr;

    int screen_width_ = 1280;
    int screen_height_ = 720;

    bool visible_ = false;
    bool return_requested_ = false;
    int selected_index_ = -1;

    unsigned int tex_panel_ = 0;
    unsigned int tex_button_off_ = 0;
    unsigned int tex_button_on_ = 0;
};
