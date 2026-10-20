#pragma once

#include <string>
#include <vector>

#include "text_renderer.h"
#include <GLES3/gl3.h>
#include <glm/glm.hpp>

// Controls menu (ControlsMenu) rebuilt from the original PC layout
// (menus/options/controls_menu.xml). 666x580 canvas with two pages:
//   page 1 - mouse sensitivity slider, invert Y toggle, swap axes toggle and
//            the two stick labels.
//   page 2 - device selector plus a scrollable list of input bindings whose
//            rows are generated in code from the template.
class ControlsOptionsUI {
public:
    struct Settings {
        float sensitivity = 0.5f;   // 0.0 .. 1.0, shown as 0 .. 100
        bool invert_y = false;
        bool swap_axes = false;
        int device_index = 0;       // 0 = touch, 1 = virtual joystick, 2 = gamepad
    };

    ControlsOptionsUI();
    ~ControlsOptionsUI();

    bool initialize(TextRenderer* text_renderer);
    void cleanup();

    void setScreenSize(int width, int height);

    void open(const Settings& initial);
    void close();
    bool isVisible() const { return visible_; }

    bool shouldReturnToOptions() const { return return_to_options_; }
    void resetReturnFlag() { return_to_options_ = false; }

    // True when any setting changed since the last consume.
    bool consumeChanged();

    void render();

    bool onTouchDown(float x, float y);
    bool onTouchMove(float x);
    bool onTouchUp();
    bool onKeyPress(int key_code);
    bool handleBack();

    const Settings& settings() const { return settings_; }
    int page() const { return page_; }
    void setPage(int page);

private:
    struct Rect {
        float x = 0.0f;
        float y = 0.0f;
        float w = 0.0f;
        float h = 0.0f;
    };

    struct Row {
        std::string label;
        std::string binding;
    };

    // Page 1 focus slots.
    enum Page1Focus {
        P1_SENSITIVITY = 0,
        P1_INVERT_Y,
        P1_SWAP,
        P1_MORE,
        P1_RETURN,
        P1_COUNT
    };

    void buildRows();
    void applyDefaults();
    void measureLabels();

    void renderBackground();
    void renderPage1();
    void renderPage2();

    Rect bodyRect() const;
    Rect sliderLineRect() const;
    Rect sliderHandleRect() const;
    Rect sensitivityLabelRect() const;
    Rect invertYLabelRect() const;
    Rect invertYButtonRect() const;
    Rect swapButtonRect() const;
    Rect stickLabelRect(int which) const;
    Rect stickSettingRect(int which) const;
    Rect page1ReturnRect() const;
    Rect mainRect() const;
    Rect deviceToggleLeftRect() const;
    Rect deviceToggleRightRect() const;
    Rect deviceTextRect() const;
    Rect scrollLineRect() const;
    Rect scrollTopRect() const;
    Rect scrollBottomRect() const;
    Rect scrollMarkerRect() const;
    Rect listRowRect(int row) const;
    Rect longButtonRect(int which) const;

    bool pointInRect(float px, float py, const Rect& rect) const;
    float listRowLocalY(int row) const;
    int listRowAtPoint(float px, float py) const;
    void setSliderFromX(float screen_x);
    void adjustSlider(float delta);
    void adjustDevice(int delta);
    void ensureRowVisible(int row);
    void moveFocus(int delta);
    void activateFocus();

    TextRenderer* text_renderer_ = nullptr;

    int screen_width_ = 1280;
    int screen_height_ = 720;

    bool visible_ = false;
    bool return_to_options_ = false;
    bool changed_ = false;
    bool dragging_ = false;

    Settings settings_;
    std::vector<Row> rows_;

    int page_ = 1;
    int focus_ = P1_SENSITIVITY;   // page 1 slot
    int page2_focus_ = 0;          // 0 = device, 1..N = rows, N+1 = defaults, N+2 = return
    int scroll_row_ = 0;
    bool remapping_ = false;       // waiting for a key press to remap
    int remapping_row_ = -1;       // row index being remapped (-1 = none)

    float label_height_ = 28.0f;
    float sensitivity_label_width_ = 0.0f;
    float action_label_width_ = 0.0f;

    unsigned int tex_generic_bg_ = 0;
    unsigned int tex_bg_edge_top_ = 0;
    unsigned int tex_bg_edge_bottom_ = 0;
    unsigned int tex_bg_edge_left_ = 0;
    unsigned int tex_bg_edge_right_ = 0;
    unsigned int tex_bg_corner_tl_ = 0;
    unsigned int tex_bg_corner_tr_ = 0;
    unsigned int tex_bg_corner_bl_ = 0;
    unsigned int tex_bg_corner_br_ = 0;
    unsigned int tex_selection_full_ = 0;
    unsigned int tex_selection_cut_ = 0;
    unsigned int tex_scroll_line_ = 0;
    unsigned int tex_scroll_parts_ = 0;
    unsigned int tex_scrollbar_up_ = 0;
    unsigned int tex_scrollbar_down_ = 0;
    unsigned int tex_scrollbar_line_ = 0;
    unsigned int tex_scrollbar_marker_ = 0;
    unsigned int tex_button_off_ = 0;
    unsigned int tex_button_on_ = 0;
    unsigned int tex_arrow_left_ = 0;
    unsigned int tex_arrow_right_ = 0;
    unsigned int tex_border_h_ = 0;
    unsigned int tex_border_v_ = 0;
};
