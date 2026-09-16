#pragma once

#include "ui_component.h"
#include <glm/glm.hpp>
#include <array>
#include <bitset>
#include <functional>
#include <chrono>

// Forward declarations
class TextRenderer;

/**
 * @brief Virtual controller button identifiers
 */
enum class VirtualControllerButton {
    A = 0,
    B = 1,
    X = 2,
    Y = 3,
    L = 4,
    R = 5,
    START = 6,
    SELECT = 7,
    DPAD_UP = 8,
    DPAD_DOWN = 9,
    DPAD_LEFT = 10,
    DPAD_RIGHT = 11,
    COUNT = 12
};

/**
 * @brief Virtual controller input state
 *
 * Queryable snapshot of all controller inputs.
 * Use getState() to retrieve the current frame's input.
 */
struct VirtualControllerState {
    glm::vec2 left_stick;    // Movement input (-1.0 to 1.0)
    glm::vec2 right_stick;   // Camera input (-1.0 to 1.0)
    std::bitset<12> buttons; // Button states (indexed by VirtualControllerButton)
    glm::vec2 dpad;          // D-pad direction (-1.0/0.0/1.0 per axis)

    VirtualControllerState()
        : left_stick(0.0f), right_stick(0.0f), dpad(0.0f) {}

    bool is_button_pressed(VirtualControllerButton btn) const {
        return buttons[static_cast<int>(btn)];
    }
};

/**
 * @brief Configuration for virtual controller layout and behavior
 */
struct VirtualControllerConfig {
    // Joystick settings
    float joystick_radius;         // Base radius in pixels
    float joystick_knob_radius;    // Knob radius in pixels
    float joystick_dead_zone;      // Dead zone (0.0 - 0.3)
    float joystick_opacity;        // Base opacity (0.0 - 1.0)

    // Button settings
    float button_radius;           // Action button radius in pixels
    float button_opacity;          // Normal button opacity
    float button_pressed_opacity;  // Pressed button opacity
    float button_pressed_scale;    // Scale factor when pressed

    // D-pad settings
    float dpad_size;               // D-pad arm length in pixels
    float dpad_opacity;            // D-pad opacity

    // Trigger settings
    float trigger_width;           // Trigger width in pixels
    float trigger_height;          // Trigger height in pixels
    float trigger_opacity;         // Trigger opacity
    float trigger_pressed_opacity; // Pressed trigger opacity

    // Start/Select settings
    float start_select_width;      // Button width in pixels
    float start_select_height;     // Button height in pixels
    float start_select_opacity;    // Opacity
    float start_select_pressed_opacity; // Pressed opacity

    // Visual feedback
    float press_anim_duration;     // Press animation duration in seconds
    float release_anim_duration;   // Release animation duration in seconds

    // Colors (RGBA)
    glm::vec4 joystick_base_color;
    glm::vec4 joystick_knob_color;
    glm::vec4 joystick_active_color;
    glm::vec4 button_a_color;
    glm::vec4 button_b_color;
    glm::vec4 button_x_color;
    glm::vec4 button_y_color;
    glm::vec4 button_pressed_color;
    glm::vec4 dpad_color;
    glm::vec4 dpad_pressed_color;
    glm::vec4 trigger_color;
    glm::vec4 trigger_pressed_color;
    glm::vec4 start_select_color;
    glm::vec4 start_select_pressed_color;
    glm::vec4 label_color;

    VirtualControllerConfig();
};

/**
 * @brief Virtual game controller overlay
 *
 * Semi-transparent touch controller overlay for Android gameplay.
 * Provides dual joysticks, action buttons, D-pad, triggers, and
 * Start/Select buttons. All input is multi-touch aware.
 *
 * Usage:
 *   auto controller = std::make_shared<VirtualController>();
 *   controller->setScreenSize(screenW, screenH);
 *   controller->initialize();
 *   // In touch handler:
 *   controller->onTouchDown(x, y, pointerId);
 *   // In game loop:
 *   VirtualControllerState state = controller->getState();
 *   if (state.is_button_pressed(VirtualControllerButton::A)) { ... }
 */
class VirtualController : public UIComponent {
public:
    VirtualController();
    ~VirtualController() override;

    // === Lifecycle ===
    bool initialize() override;
    void update(float deltaTime) override;
    void render() override;
    void cleanup() override;

    // === Touch Events ===
    bool onTouchDown(float x, float y, int pointerId) override;
    bool onTouchUp(float x, float y, int pointerId) override;
    bool onTouchMove(float x, float y, float dx, float dy, int pointerId) override;
    bool onEvent(const UIEvent& event) override;

    // === State Query ===
    VirtualControllerState getState() const;

    // === Configuration ===
    void set_config(const VirtualControllerConfig& config);
    const VirtualControllerConfig& get_config() const { return config_; }

    // === Convenience ===
    void set_text_renderer(TextRenderer* renderer) { text_renderer_ = renderer; }

private:
    // === Sub-element hit testing ===
    bool hit_test_left_joystick(float x, float y) const;
    bool hit_test_right_joystick(float x, float y) const;
    bool hit_test_button(int index, float x, float y) const;
    bool hit_test_dpad(float x, float y) const;
    int  hit_test_dpad_direction(float x, float y) const;
    bool hit_test_trigger(int index, float x, float y) const;
    bool hit_test_start_select(int index, float x, float y) const;

    // === Rendering helpers ===
    void render_left_joystick() const;
    void render_right_joystick() const;
    void render_action_buttons() const;
    void render_dpad() const;
    void render_triggers() const;
    void render_start_select() const;
    void render_button_label(float cx, float cy, const char* label, const glm::vec4& color) const;

    // === Joystick processing ===
    void update_joystick(glm::vec2& stick_value, glm::vec2& knob_pos,
                         float center_x, float center_y, float touch_x, float touch_y) const;
    void reset_joystick(glm::vec2& stick_value, glm::vec2& knob_pos,
                        float center_x, float center_y);

    // === Layout calculation ===
    void calculate_layout();
    float get_left_joystick_center_x() const;
    float get_left_joystick_center_y() const;
    float get_right_joystick_center_x() const;
    float get_right_joystick_center_y() const;
    float get_dpad_center_x() const;
    float get_dpad_center_y() const;
    float get_button_center_x(int index) const;
    float get_button_center_y(int index) const;
    float get_trigger_center_x(int index) const;
    float get_trigger_center_y() const;
    float get_start_select_center_x(int index) const;
    float get_start_select_center_y() const;

    // === Configuration ===
    VirtualControllerConfig config_;
    TextRenderer* text_renderer_;

    // === Joystick state ===
    glm::vec2 left_stick_value_;
    glm::vec2 left_knob_pos_;
    int left_stick_pointer_id_;

    glm::vec2 right_stick_value_;
    glm::vec2 right_knob_pos_;
    int right_stick_pointer_id_;

    // === Button state ===
    std::array<bool, 12> button_pressed_;
    std::array<int, 12> button_pointer_ids_;
    std::array<float, 12> button_press_timers_;

    // === D-pad state ===
    glm::vec2 dpad_value_;
    int dpad_pointer_id_;
    int dpad_active_direction_;

    // === Trigger state ===
    std::array<bool, 2> trigger_pressed_;
    std::array<int, 2> trigger_pointer_ids_;
    std::array<float, 2> trigger_press_timers_;

    // === Start/Select state ===
    std::array<bool, 2> start_select_pressed_;
    std::array<int, 2> start_select_pointer_ids_;
    std::array<float, 2> start_select_press_timers_;

    // === Layout cache ===
    float screen_w_;
    float screen_h_;
    bool layout_dirty_;
};
