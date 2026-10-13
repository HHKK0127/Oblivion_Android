#pragma once

#include <string>
#include <vector>

#include "text_renderer.h"
#include <GLES3/gl3.h>
#include <glm/glm.hpp>

// Video options menu (VideoMenu) rebuilt from the original PC layout
// (menus/options/video_menu.xml). 900x600 canvas, a scrollable list of 27
// rows (7 visible) with sliders, button rows and two arrow rows.
class VideoOptionsUI {
public:
    struct Settings {
        int resolution_index = 2;   // 1280x720
        float gamma = 0.5f;
        int texture_size = 2;       // Small / Medium / Large
        float tree_lod = 0.5f;
        float actor_lod = 0.5f;
        float item_lod = 0.5f;
        float object_lod = 0.5f;
        float grass = 0.5f;
        float view_distance = 0.5f;
        int distant_lod = 1;        // Off / On
        int dist_build = 1;
        int dist_trees = 1;
        float interior_shadows = 0.5f;
        float exterior_shadows = 0.5f;
        int self_shadows = 0;
        int grass_shadows = 0;
        int canopy_shadows = 0;
        int shadow_filter = 0;
        float specular_light = 0.5f;
        int hdr = 0;
        int bloom = 0;
        int water_detail = 1;       // Low / Medium / High
        int water_reflections = 1;
        int water_displacement = 0;
        int window_reflections = 1;
        int blood = 1;
        int antialiasing = 0;
    };

    VideoOptionsUI();
    ~VideoOptionsUI();

    bool initialize(TextRenderer* text_renderer);
    void cleanup();

    void setScreenSize(int width, int height);

    void open();
    void close();
    bool isVisible() const { return visible_; }

    bool shouldReturnToOptions() const { return return_to_options_; }
    void resetReturnFlag() { return_to_options_ = false; }

    // True when any setting changed since the last consume.
    bool consumeChanged();

    // True once when the resolution row changed; fills the new index and asks
    // the caller to show the VideoDisplayMenu resolution panel.
    bool consumeDisplayRequest(int& selected_index);

    void update(float dt);
    void render();

    bool onTouchDown(float x, float y);
    bool onTouchMove(float x);
    bool onTouchUp();
    bool onKeyPress(int key_code);
    bool handleBack();

    const Settings& settings() const { return settings_; }
    void setSettings(const Settings& settings) { settings_ = settings; }

private:
    struct Rect {
        float x = 0.0f;
        float y = 0.0f;
        float w = 0.0f;
        float h = 0.0f;
    };

    struct Row {
        std::string label;
        enum class Kind { Slider, Button, Arrows } kind = Kind::Slider;
        float* value = nullptr;   // slider rows
        int* index = nullptr;     // button / arrows rows
        int option_count = 2;     // button / arrows rows
    };

    void buildRows();
    void applyDefaults();
    void measureLabels();

    void renderBorder();
    void renderRow(const Row& row, int index, float row_y);
    void renderSliderRow(const Row& row, int index, float row_y);
    void renderButtonRow(const Row& row, int index, float row_y);
    void renderArrowsRow(const Row& row, int index, float row_y);
    void renderScrollBar();
    void renderFooter();

    Rect bodyRect() const;
    float rowLocalY(int index) const;
    Rect markerRect(int index) const;
    Rect sliderLineRect(int index) const;
    Rect sliderHandleRect(int index) const;
    Rect rowButtonRect(int index) const;
    Rect arrowsLeftRect(int index) const;
    Rect arrowsRightRect(int index) const;
    Rect arrowsValueRect(int index) const;
    Rect scrollLineRect() const;
    Rect scrollTopRect() const;
    Rect scrollBottomRect() const;
    Rect scrollMarkerRect() const;
    Rect footerButtonRect(int which) const;

    bool pointInRect(float px, float py, const Rect& rect) const;
    int rowAtPoint(float px, float py) const;
    void setSliderFromX(int index, float screen_x);
    void adjustSlider(int index, float delta);
    void adjustIndex(int index, int delta);
    void focusRow(int index);
    void ensureVisible(int index);

    TextRenderer* text_renderer_ = nullptr;

    int screen_width_ = 1280;
    int screen_height_ = 720;

    bool visible_ = false;
    bool return_to_options_ = false;
    bool changed_ = false;
    bool display_request_ = false;

    Settings settings_;
    std::vector<Row> rows_;

    int scroll_row_ = 0;
    int focus_row_ = 0;
    int drag_slider_ = -1;

    float label_height_ = 28.0f;
    std::vector<float> label_widths_;

    // Loaded textures.
    unsigned int tex_generic_bg_ = 0;
    unsigned int tex_selection_full_ = 0;
    unsigned int tex_selection_cut_ = 0;
    unsigned int tex_button_long_off_ = 0;
    unsigned int tex_button_long_on_ = 0;
    unsigned int tex_scrollbar_line_ = 0;
    unsigned int tex_scrollbar_up_ = 0;
    unsigned int tex_scrollbar_down_ = 0;
    unsigned int tex_scrollbar_marker_ = 0;
    unsigned int tex_scrollbar_parts_ = 0;
    unsigned int tex_arrow_left_ = 0;
    unsigned int tex_arrow_right_ = 0;
};
