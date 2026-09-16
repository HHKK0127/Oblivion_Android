#include "virtual_controller.h"
#include "ui_draw_helper.h"
#include "text_renderer.h"
#include <cmath>
#include <algorithm>
#include <cstring>

// Logging
#define VC_LOG_TAG "VirtualController"
#define VC_LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, VC_LOG_TAG, __VA_ARGS__)
#define VC_LOGI(...) __android_log_print(ANDROID_LOG_INFO, VC_LOG_TAG, __VA_ARGS__)
#define VC_LOGW(...) __android_log_print(ANDROID_LOG_WARN, VC_LOG_TAG, __VA_ARGS__)

// GLM does not always expose glm::mix for vec4; provide a local lerp helper.
static inline glm::vec4 vc_lerp(const glm::vec4& a, const glm::vec4& b, float t) {
    return glm::vec4(
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t,
        a.w + (b.w - a.w) * t
    );
}

// =============================================================================
// Constants
// =============================================================================

static constexpr float LAYOUT_MARGIN_RATIO = 0.05f;       // 5% margin from screen edge
static constexpr float JOYSTICK_Y_RATIO = 0.70f;          // 70% down from top
static constexpr float BUTTON_Y_RATIO = 0.65f;            // 65% down from top
static constexpr float DPAD_Y_RATIO = 0.65f;              // 65% down from top
static constexpr float TRIGGER_Y_RATIO = 0.08f;           // 8% down from top
static constexpr float START_SELECT_Y_RATIO = 0.08f;      // 8% down from top
static constexpr float BUTTON_SPACING_RATIO = 0.08f;      // 8% of screen width between buttons
static constexpr float DPAD_ARM_RATIO = 0.06f;            // 6% of screen width for D-pad arm
static constexpr float KNOB_RADIUS_RATIO = 0.4f;          // Knob is 40% of base radius
static constexpr float HIT_AREA_EXPANSION = 1.3f;         // 30% larger hit area for usability

// =============================================================================
// VirtualControllerConfig defaults
// =============================================================================

VirtualControllerConfig::VirtualControllerConfig()
    : joystick_radius(80.0f)
    , joystick_knob_radius(32.0f)
    , joystick_dead_zone(0.15f)
    , joystick_opacity(0.4f)
    , button_radius(40.0f)
    , button_opacity(0.4f)
    , button_pressed_opacity(0.8f)
    , button_pressed_scale(0.9f)
    , dpad_size(60.0f)
    , dpad_opacity(0.4f)
    , trigger_width(100.0f)
    , trigger_height(40.0f)
    , trigger_opacity(0.4f)
    , trigger_pressed_opacity(0.8f)
    , start_select_width(60.0f)
    , start_select_height(25.0f)
    , start_select_opacity(0.35f)
    , start_select_pressed_opacity(0.7f)
    , press_anim_duration(0.08f)
    , release_anim_duration(0.12f)
    , joystick_base_color(0.3f, 0.3f, 0.3f, 0.4f)
    , joystick_knob_color(0.7f, 0.7f, 0.7f, 0.6f)
    , joystick_active_color(0.9f, 0.9f, 0.9f, 0.8f)
    , button_a_color(0.2f, 0.8f, 0.2f, 0.6f)       // Green
    , button_b_color(0.8f, 0.2f, 0.2f, 0.6f)       // Red
    , button_x_color(0.2f, 0.4f, 0.9f, 0.6f)       // Blue
    , button_y_color(0.9f, 0.8f, 0.1f, 0.6f)       // Yellow
    , button_pressed_color(1.0f, 1.0f, 1.0f, 0.9f)  // White flash
    , dpad_color(0.3f, 0.3f, 0.3f, 0.4f)
    , dpad_pressed_color(0.7f, 0.7f, 0.7f, 0.7f)
    , trigger_color(0.35f, 0.35f, 0.35f, 0.4f)
    , trigger_pressed_color(0.7f, 0.7f, 0.7f, 0.8f)
    , start_select_color(0.3f, 0.3f, 0.3f, 0.35f)
    , start_select_pressed_color(0.6f, 0.6f, 0.6f, 0.7f)
    , label_color(0.9f, 0.9f, 0.9f, 0.8f) {
}

// =============================================================================
// Constructor / Destructor
// =============================================================================

VirtualController::VirtualController()
    : UIComponent("VirtualController")
    , text_renderer_(nullptr)
    , left_stick_value_(0.0f)
    , left_knob_pos_(0.0f)
    , left_stick_pointer_id_(-1)
    , right_stick_value_(0.0f)
    , right_knob_pos_(0.0f)
    , right_stick_pointer_id_(-1)
    , dpad_value_(0.0f)
    , dpad_pointer_id_(-1)
    , dpad_active_direction_(-1)
    , screen_w_(0.0f)
    , screen_h_(0.0f)
    , layout_dirty_(true) {
    button_pressed_.fill(false);
    button_pointer_ids_.fill(-1);
    button_press_timers_.fill(0.0f);
    trigger_pressed_.fill(false);
    trigger_pointer_ids_.fill(-1);
    trigger_press_timers_.fill(0.0f);
    start_select_pressed_.fill(false);
    start_select_pointer_ids_.fill(-1);
    start_select_press_timers_.fill(0.0f);
}

VirtualController::~VirtualController() {
    cleanup();
}

// =============================================================================
// Lifecycle
// =============================================================================

bool VirtualController::initialize() {
    VC_LOGI("Initializing virtual controller");
    calculate_layout();
    VC_LOGI("Virtual controller initialized successfully");
    return true;
}

void VirtualController::update(float deltaTime) {
    if (!isVisible()) return;

    // Update press animation timers
    for (int i = 0; i < 12; ++i) {
        if (button_pressed_[i]) {
            button_press_timers_[i] = std::min(button_press_timers_[i] + deltaTime,
                                               config_.press_anim_duration);
        } else {
            button_press_timers_[i] = std::max(button_press_timers_[i] - deltaTime,
                                               0.0f);
        }
    }

    for (int i = 0; i < 2; ++i) {
        if (trigger_pressed_[i]) {
            trigger_press_timers_[i] = std::min(trigger_press_timers_[i] + deltaTime,
                                                config_.press_anim_duration);
        } else {
            trigger_press_timers_[i] = std::max(trigger_press_timers_[i] - deltaTime,
                                                0.0f);
        }
        if (start_select_pressed_[i]) {
            start_select_press_timers_[i] = std::min(start_select_press_timers_[i] + deltaTime,
                                                     config_.press_anim_duration);
        } else {
            start_select_press_timers_[i] = std::max(start_select_press_timers_[i] - deltaTime,
                                                     0.0f);
        }
    }

    if (layout_dirty_) {
        calculate_layout();
        layout_dirty_ = false;
    }
}

void VirtualController::render() {
    if (!isVisible()) return;

    render_left_joystick();
    render_right_joystick();
    render_action_buttons();
    render_dpad();
    render_triggers();
    render_start_select();
}

void VirtualController::cleanup() {
    VC_LOGI("VirtualController cleanup");
    left_stick_pointer_id_ = -1;
    right_stick_pointer_id_ = -1;
    dpad_pointer_id_ = -1;
    button_pointer_ids_.fill(-1);
    trigger_pointer_ids_.fill(-1);
    start_select_pointer_ids_.fill(-1);
}

// =============================================================================
// Touch Events
// =============================================================================

bool VirtualController::onEvent(const UIEvent& event) {
    if (!isVisible() || !isEnabled()) return false;

    // For MOVE and UP, process if we are tracking this pointer
    if ((event.type == UIEventType::TOUCH_MOVE || event.type == UIEventType::TOUCH_UP)) {
        if (event.type == UIEventType::TOUCH_MOVE) {
            return onTouchMove(event.x, event.y, event.dx, event.dy, event.pointerId);
        } else {
            return onTouchUp(event.x, event.y, event.pointerId);
        }
    }

    if (event.type == UIEventType::TOUCH_DOWN) {
        return onTouchDown(event.x, event.y, event.pointerId);
    }

    return false;
}

bool VirtualController::onTouchDown(float x, float y, int pointerId) {
    if (!isVisible() || !isEnabled()) return false;

    // Check left joystick
    if (hit_test_left_joystick(x, y) && left_stick_pointer_id_ == -1) {
        left_stick_pointer_id_ = pointerId;
        update_joystick(left_stick_value_, left_knob_pos_,
                        get_left_joystick_center_x(), get_left_joystick_center_y(), x, y);
        VC_LOGD("Left joystick DOWN: pointer=%d", pointerId);
        return true;
    }

    // Check right joystick
    if (hit_test_right_joystick(x, y) && right_stick_pointer_id_ == -1) {
        right_stick_pointer_id_ = pointerId;
        update_joystick(right_stick_value_, right_knob_pos_,
                        get_right_joystick_center_x(), get_right_joystick_center_y(), x, y);
        VC_LOGD("Right joystick DOWN: pointer=%d", pointerId);
        return true;
    }

    // Check action buttons (A=0, B=1, X=2, Y=3)
    for (int i = 0; i < 4; ++i) {
        if (hit_test_button(i, x, y) && button_pointer_ids_[i] == -1) {
            button_pressed_[i] = true;
            button_pointer_ids_[i] = pointerId;
            button_press_timers_[i] = 0.0f;
            VC_LOGD("Button %d DOWN: pointer=%d", i, pointerId);
            return true;
        }
    }

    // Check D-pad
    if (hit_test_dpad(x, y) && dpad_pointer_id_ == -1) {
        dpad_pointer_id_ = pointerId;
        dpad_active_direction_ = hit_test_dpad_direction(x, y);
        // Set dpad value based on direction
        dpad_value_ = glm::vec2(0.0f);
        if (dpad_active_direction_ == 0) dpad_value_.y = -1.0f;  // Up
        else if (dpad_active_direction_ == 1) dpad_value_.y = 1.0f;   // Down
        else if (dpad_active_direction_ == 2) dpad_value_.x = -1.0f;  // Left
        else if (dpad_active_direction_ == 3) dpad_value_.x = 1.0f;   // Right
        VC_LOGD("D-pad DOWN: direction=%d pointer=%d", dpad_active_direction_, pointerId);
        return true;
    }

    // Check triggers (L=4, R=5)
    for (int i = 0; i < 2; ++i) {
        if (hit_test_trigger(i, x, y) && trigger_pointer_ids_[i] == -1) {
            trigger_pressed_[i] = true;
            trigger_pointer_ids_[i] = pointerId;
            trigger_press_timers_[i] = 0.0f;
            VC_LOGD("Trigger %d DOWN: pointer=%d", i, pointerId);
            return true;
        }
    }

    // Check Start/Select (Start=6, Select=7)
    for (int i = 0; i < 2; ++i) {
        if (hit_test_start_select(i, x, y) && start_select_pointer_ids_[i] == -1) {
            start_select_pressed_[i] = true;
            start_select_pointer_ids_[i] = pointerId;
            start_select_press_timers_[i] = 0.0f;
            VC_LOGD("Start/Select %d DOWN: pointer=%d", i, pointerId);
            return true;
        }
    }

    return false;
}

bool VirtualController::onTouchUp(float x, float y, int pointerId) {
    if (!isVisible()) return false;
    bool consumed = false;

    // Release left joystick
    if (left_stick_pointer_id_ == pointerId) {
        reset_joystick(left_stick_value_, left_knob_pos_,
                       get_left_joystick_center_x(), get_left_joystick_center_y());
        left_stick_pointer_id_ = -1;
        VC_LOGD("Left joystick UP: pointer=%d", pointerId);
        consumed = true;
    }

    // Release right joystick
    if (right_stick_pointer_id_ == pointerId) {
        reset_joystick(right_stick_value_, right_knob_pos_,
                       get_right_joystick_center_x(), get_right_joystick_center_y());
        right_stick_pointer_id_ = -1;
        VC_LOGD("Right joystick UP: pointer=%d", pointerId);
        consumed = true;
    }

    // Release action buttons
    for (int i = 0; i < 4; ++i) {
        if (button_pointer_ids_[i] == pointerId) {
            button_pressed_[i] = false;
            button_pointer_ids_[i] = -1;
            VC_LOGD("Button %d UP: pointer=%d", i, pointerId);
            consumed = true;
        }
    }

    // Release D-pad
    if (dpad_pointer_id_ == pointerId) {
        dpad_value_ = glm::vec2(0.0f);
        dpad_pointer_id_ = -1;
        dpad_active_direction_ = -1;
        VC_LOGD("D-pad UP: pointer=%d", pointerId);
        consumed = true;
    }

    // Release triggers
    for (int i = 0; i < 2; ++i) {
        if (trigger_pointer_ids_[i] == pointerId) {
            trigger_pressed_[i] = false;
            trigger_pointer_ids_[i] = -1;
            VC_LOGD("Trigger %d UP: pointer=%d", i, pointerId);
            consumed = true;
        }
    }

    // Release Start/Select
    for (int i = 0; i < 2; ++i) {
        if (start_select_pointer_ids_[i] == pointerId) {
            start_select_pressed_[i] = false;
            start_select_pointer_ids_[i] = -1;
            VC_LOGD("Start/Select %d UP: pointer=%d", i, pointerId);
            consumed = true;
        }
    }

    return consumed;
}

bool VirtualController::onTouchMove(float x, float y, float dx, float dy, int pointerId) {
    if (!isVisible()) return false;
    bool consumed = false;

    // Update left joystick
    if (left_stick_pointer_id_ == pointerId) {
        update_joystick(left_stick_value_, left_knob_pos_,
                        get_left_joystick_center_x(), get_left_joystick_center_y(), x, y);
        consumed = true;
    }

    // Update right joystick
    if (right_stick_pointer_id_ == pointerId) {
        update_joystick(right_stick_value_, right_knob_pos_,
                        get_right_joystick_center_x(), get_right_joystick_center_y(), x, y);
        consumed = true;
    }

    // Update D-pad (slide to change direction)
    if (dpad_pointer_id_ == pointerId) {
        int new_dir = hit_test_dpad_direction(x, y);
        if (new_dir != dpad_active_direction_) {
            dpad_active_direction_ = new_dir;
            dpad_value_ = glm::vec2(0.0f);
            if (new_dir == 0) dpad_value_.y = -1.0f;
            else if (new_dir == 1) dpad_value_.y = 1.0f;
            else if (new_dir == 2) dpad_value_.x = -1.0f;
            else if (new_dir == 3) dpad_value_.x = 1.0f;
        }
        consumed = true;
    }

    return consumed;
}

// =============================================================================
// State Query
// =============================================================================

VirtualControllerState VirtualController::getState() const {
    VirtualControllerState state;

    // Apply dead zone to joysticks
    float lx = left_stick_value_.x;
    float ly = left_stick_value_.y;
    float lmag = std::sqrt(lx * lx + ly * ly);
    if (lmag < config_.joystick_dead_zone) {
        state.left_stick = glm::vec2(0.0f);
    } else {
        float scale = (lmag - config_.joystick_dead_zone) / (1.0f - config_.joystick_dead_zone);
        scale = std::min(scale, 1.0f);
        state.left_stick = glm::vec2(lx / lmag * scale, ly / lmag * scale);
    }

    float rx = right_stick_value_.x;
    float ry = right_stick_value_.y;
    float rmag = std::sqrt(rx * rx + ry * ry);
    if (rmag < config_.joystick_dead_zone) {
        state.right_stick = glm::vec2(0.0f);
    } else {
        float scale = (rmag - config_.joystick_dead_zone) / (1.0f - config_.joystick_dead_zone);
        scale = std::min(scale, 1.0f);
        state.right_stick = glm::vec2(rx / rmag * scale, ry / rmag * scale);
    }

    // Buttons: A, B, X, Y, L, R, Start, Select
    state.buttons[static_cast<int>(VirtualControllerButton::A)] = button_pressed_[0];
    state.buttons[static_cast<int>(VirtualControllerButton::B)] = button_pressed_[1];
    state.buttons[static_cast<int>(VirtualControllerButton::X)] = button_pressed_[2];
    state.buttons[static_cast<int>(VirtualControllerButton::Y)] = button_pressed_[3];
    state.buttons[static_cast<int>(VirtualControllerButton::L)] = trigger_pressed_[0];
    state.buttons[static_cast<int>(VirtualControllerButton::R)] = trigger_pressed_[1];
    state.buttons[static_cast<int>(VirtualControllerButton::START)] = start_select_pressed_[0];
    state.buttons[static_cast<int>(VirtualControllerButton::SELECT)] = start_select_pressed_[1];

    // D-pad
    state.buttons[static_cast<int>(VirtualControllerButton::DPAD_UP)] = (dpad_active_direction_ == 0);
    state.buttons[static_cast<int>(VirtualControllerButton::DPAD_DOWN)] = (dpad_active_direction_ == 1);
    state.buttons[static_cast<int>(VirtualControllerButton::DPAD_LEFT)] = (dpad_active_direction_ == 2);
    state.buttons[static_cast<int>(VirtualControllerButton::DPAD_RIGHT)] = (dpad_active_direction_ == 3);
    state.dpad = dpad_value_;

    return state;
}

// =============================================================================
// Configuration
// =============================================================================

void VirtualController::set_config(const VirtualControllerConfig& config) {
    config_ = config;
    layout_dirty_ = true;
    VC_LOGI("Virtual controller config updated");
}

// =============================================================================
// Hit Testing
// =============================================================================

bool VirtualController::hit_test_left_joystick(float x, float y) const {
    float cx = get_left_joystick_center_x();
    float cy = get_left_joystick_center_y();
    float dx = x - cx;
    float dy = y - cy;
    float hit_radius = config_.joystick_radius * HIT_AREA_EXPANSION;
    return (dx * dx + dy * dy) <= (hit_radius * hit_radius);
}

bool VirtualController::hit_test_right_joystick(float x, float y) const {
    float cx = get_right_joystick_center_x();
    float cy = get_right_joystick_center_y();
    float dx = x - cx;
    float dy = y - cy;
    float hit_radius = config_.joystick_radius * HIT_AREA_EXPANSION;
    return (dx * dx + dy * dy) <= (hit_radius * hit_radius);
}

bool VirtualController::hit_test_button(int index, float x, float y) const {
    float cx = get_button_center_x(index);
    float cy = get_button_center_y(index);
    float dx = x - cx;
    float dy = y - cy;
    float hit_radius = config_.button_radius * HIT_AREA_EXPANSION;
    return (dx * dx + dy * dy) <= (hit_radius * hit_radius);
}

bool VirtualController::hit_test_dpad(float x, float y) const {
    float cx = get_dpad_center_x();
    float cy = get_dpad_center_y();
    float dx = x - cx;
    float dy = y - cy;
    float hit_radius = config_.dpad_size * 1.8f;
    return (dx * dx + dy * dy) <= (hit_radius * hit_radius);
}

int VirtualController::hit_test_dpad_direction(float x, float y) const {
    float cx = get_dpad_center_x();
    float cy = get_dpad_center_y();
    float dx = x - cx;
    float dy = y - cy;

    // Determine dominant axis
    if (std::abs(dx) > std::abs(dy)) {
        return (dx < 0) ? 2 : 3;  // Left or Right
    } else {
        return (dy < 0) ? 0 : 1;  // Up or Down
    }
}

bool VirtualController::hit_test_trigger(int index, float x, float y) const {
    float cx = get_trigger_center_x(index);
    float cy = get_trigger_center_y();
    float half_w = config_.trigger_width * 0.5f * HIT_AREA_EXPANSION;
    float half_h = config_.trigger_height * 0.5f * HIT_AREA_EXPANSION;
    return (x >= cx - half_w && x <= cx + half_w && y >= cy - half_h && y <= cy + half_h);
}

bool VirtualController::hit_test_start_select(int index, float x, float y) const {
    float cx = get_start_select_center_x(index);
    float cy = get_start_select_center_y();
    float half_w = config_.start_select_width * 0.5f * HIT_AREA_EXPANSION;
    float half_h = config_.start_select_height * 0.5f * HIT_AREA_EXPANSION;
    return (x >= cx - half_w && x <= cx + half_w && y >= cy - half_h && y <= cy + half_h);
}

// =============================================================================
// Layout Calculation
// =============================================================================

void VirtualController::calculate_layout() {
    screen_w_ = static_cast<float>(screenWidth);
    screen_h_ = static_cast<float>(screenHeight);

    if (screen_w_ <= 0.0f || screen_h_ <= 0.0f) return;

    // Scale config values based on screen size
    float scale = std::min(screen_w_, screen_h_) / 1080.0f;  // Reference: 1080p
    config_.joystick_radius = 80.0f * scale;
    config_.joystick_knob_radius = config_.joystick_radius * KNOB_RADIUS_RATIO;
    config_.button_radius = 40.0f * scale;
    config_.dpad_size = 60.0f * scale;
    config_.trigger_width = 100.0f * scale;
    config_.trigger_height = 40.0f * scale;
    config_.start_select_width = 60.0f * scale;
    config_.start_select_height = 25.0f * scale;

    VC_LOGD("Layout calculated: screen=%.0fx%.0f scale=%.2f", screen_w_, screen_h_, scale);
}

float VirtualController::get_left_joystick_center_x() const {
    return screen_w_ * LAYOUT_MARGIN_RATIO + config_.joystick_radius;
}

float VirtualController::get_left_joystick_center_y() const {
    return screen_h_ * JOYSTICK_Y_RATIO;
}

float VirtualController::get_right_joystick_center_x() const {
    return screen_w_ * (1.0f - LAYOUT_MARGIN_RATIO) - config_.joystick_radius;
}

float VirtualController::get_right_joystick_center_y() const {
    return screen_h_ * JOYSTICK_Y_RATIO;
}

float VirtualController::get_dpad_center_x() const {
    return screen_w_ * LAYOUT_MARGIN_RATIO + config_.joystick_radius;
}

float VirtualController::get_dpad_center_y() const {
    return screen_h_ * DPAD_Y_RATIO;
}

float VirtualController::get_button_center_x(int index) const {
    // Buttons laid out in a diamond: Y(top), X(left), B(right), A(bottom)
    float base_x = screen_w_ * (1.0f - LAYOUT_MARGIN_RATIO) - config_.joystick_radius;
    float spacing = config_.button_radius * 1.5f;

    switch (index) {
        case 0: return base_x;                    // A (bottom)
        case 1: return base_x + spacing;          // B (right)
        case 2: return base_x - spacing;          // X (left)
        case 3: return base_x;                    // Y (top)
        default: return base_x;
    }
}

float VirtualController::get_button_center_y(int index) const {
    float base_y = screen_h_ * BUTTON_Y_RATIO;
    float spacing = config_.button_radius * 1.5f;

    switch (index) {
        case 0: return base_y + spacing;          // A (bottom)
        case 1: return base_y;                    // B (right)
        case 2: return base_y;                    // X (left)
        case 3: return base_y - spacing;          // Y (top)
        default: return base_y;
    }
}

float VirtualController::get_trigger_center_x(int index) const {
    if (index == 0) {
        return screen_w_ * 0.25f;  // L trigger
    }
    return screen_w_ * 0.75f;      // R trigger
}

float VirtualController::get_trigger_center_y() const {
    return screen_h_ * TRIGGER_Y_RATIO;
}

float VirtualController::get_start_select_center_x(int index) const {
    if (index == 0) {
        return screen_w_ * 0.42f;  // Start
    }
    return screen_w_ * 0.58f;      // Select
}

float VirtualController::get_start_select_center_y() const {
    return screen_h_ * START_SELECT_Y_RATIO;
}

// =============================================================================
// Joystick Processing
// =============================================================================

void VirtualController::update_joystick(glm::vec2& stick_value, glm::vec2& knob_pos,
                                         float center_x, float center_y,
                                         float touch_x, float touch_y) const {
    float dx = touch_x - center_x;
    float dy = touch_y - center_y;
    float dist = std::sqrt(dx * dx + dy * dy);
    float radius = config_.joystick_radius;

    if (dist > radius) {
        // Clamp to edge
        float ratio = radius / dist;
        knob_pos.x = center_x + dx * ratio;
        knob_pos.y = center_y + dy * ratio;
        stick_value.x = dx / dist;
        stick_value.y = dy / dist;
    } else {
        knob_pos.x = touch_x;
        knob_pos.y = touch_y;
        stick_value.x = (radius > 0.0f) ? dx / radius : 0.0f;
        stick_value.y = (radius > 0.0f) ? dy / radius : 0.0f;
    }
}

void VirtualController::reset_joystick(glm::vec2& stick_value, glm::vec2& knob_pos,
                                        float center_x, float center_y) {
    stick_value = glm::vec2(0.0f);
    knob_pos = glm::vec2(center_x, center_y);
}

// =============================================================================
// Rendering
// =============================================================================

void VirtualController::render_left_joystick() const {
    float cx = get_left_joystick_center_x();
    float cy = get_left_joystick_center_y();
    float r = config_.joystick_radius;
    float kr = config_.joystick_knob_radius;
    int sw = screenWidth;
    int sh = screenHeight;

    // Base circle (outer ring)
    glm::vec4 base_color = config_.joystick_base_color;
    base_color.w *= config_.joystick_opacity;
    UIDrawHelper::drawSoftCircle(cx, cy, r, base_color, sw, sh);

    // Inner ring (subtle border effect)
    glm::vec4 ring_color = glm::vec4(0.5f, 0.5f, 0.5f, 0.2f);
    UIDrawHelper::drawSoftCircle(cx, cy, r * 0.85f, ring_color, sw, sh);

    // Knob
    bool active = (left_stick_pointer_id_ != -1);
    glm::vec4 knob_color = active ? config_.joystick_active_color : config_.joystick_knob_color;
    knob_color.w *= config_.joystick_opacity;

    // Subtle glow when active
    if (active) {
        UIDrawHelper::drawRadialGlow(left_knob_pos_.x, left_knob_pos_.y, kr * 1.5f,
                                      glm::vec4(0.5f, 0.7f, 1.0f, 0.15f),
                                      glm::vec4(0.5f, 0.7f, 1.0f, 0.0f), sw, sh);
    }

    UIDrawHelper::drawSoftCircle(left_knob_pos_.x, left_knob_pos_.y, kr, knob_color, sw, sh);

    // Dead zone indicator (small inner circle)
    float dz_radius = r * config_.joystick_dead_zone;
    glm::vec4 dz_color = glm::vec4(0.6f, 0.6f, 0.6f, 0.1f);
    UIDrawHelper::drawSoftCircle(cx, cy, dz_radius, dz_color, sw, sh);
}

void VirtualController::render_right_joystick() const {
    float cx = get_right_joystick_center_x();
    float cy = get_right_joystick_center_y();
    float r = config_.joystick_radius;
    float kr = config_.joystick_knob_radius;
    int sw = screenWidth;
    int sh = screenHeight;

    // Base circle
    glm::vec4 base_color = config_.joystick_base_color;
    base_color.w *= config_.joystick_opacity;
    UIDrawHelper::drawSoftCircle(cx, cy, r, base_color, sw, sh);

    // Inner ring
    glm::vec4 ring_color = glm::vec4(0.5f, 0.5f, 0.5f, 0.2f);
    UIDrawHelper::drawSoftCircle(cx, cy, r * 0.85f, ring_color, sw, sh);

    // Knob
    bool active = (right_stick_pointer_id_ != -1);
    glm::vec4 knob_color = active ? config_.joystick_active_color : config_.joystick_knob_color;
    knob_color.w *= config_.joystick_opacity;

    if (active) {
        UIDrawHelper::drawRadialGlow(right_knob_pos_.x, right_knob_pos_.y, kr * 1.5f,
                                      glm::vec4(0.7f, 0.5f, 1.0f, 0.15f),
                                      glm::vec4(0.7f, 0.5f, 1.0f, 0.0f), sw, sh);
    }

    UIDrawHelper::drawSoftCircle(right_knob_pos_.x, right_knob_pos_.y, kr, knob_color, sw, sh);

    // Dead zone indicator
    float dz_radius = r * config_.joystick_dead_zone;
    glm::vec4 dz_color = glm::vec4(0.6f, 0.6f, 0.6f, 0.1f);
    UIDrawHelper::drawSoftCircle(cx, cy, dz_radius, dz_color, sw, sh);
}

void VirtualController::render_action_buttons() const {
    int sw = screenWidth;
    int sh = screenHeight;

    // Button colors (A=green, B=red, X=blue, Y=yellow)
    const glm::vec4 colors[4] = {
        config_.button_a_color,
        config_.button_b_color,
        config_.button_x_color,
        config_.button_y_color
    };
    const char* labels[4] = { "A", "B", "X", "Y" };

    for (int i = 0; i < 4; ++i) {
        float cx = get_button_center_x(i);
        float cy = get_button_center_y(i);
        float r = config_.button_radius;

        // Apply press scale
        float timer = button_press_timers_[i];
        float press_t = (config_.press_anim_duration > 0.0f)
                        ? timer / config_.press_anim_duration : 0.0f;
        press_t = std::min(press_t, 1.0f);

        float scale = button_pressed_[i]
            ? (1.0f - press_t * (1.0f - config_.button_pressed_scale))
            : (1.0f - (1.0f - press_t) * (1.0f - config_.button_pressed_scale));

        float display_r = r * scale;

        // Button color with press feedback
        glm::vec4 color = colors[i];
        if (button_pressed_[i]) {
            color = vc_lerp(color, config_.button_pressed_color, press_t * 0.5f);
            color.w = config_.button_pressed_opacity;
        } else {
            color.w = config_.button_opacity;
        }

        // Glow when pressed
        if (button_pressed_[i]) {
            UIDrawHelper::drawRadialGlow(cx, cy, display_r * 1.4f,
                                          glm::vec4(color.x, color.y, color.z, 0.2f),
                                          glm::vec4(color.x, color.y, color.z, 0.0f), sw, sh);
        }

        // Button circle
        UIDrawHelper::drawSoftCircle(cx, cy, display_r, color, sw, sh);

        // Border ring
        glm::vec4 border_color = glm::vec4(1.0f, 1.0f, 1.0f, 0.3f);
        UIDrawHelper::drawSoftCircle(cx, cy, display_r * 1.05f, border_color, sw, sh);
        UIDrawHelper::drawSoftCircle(cx, cy, display_r * 0.9f, color, sw, sh);

        // Label
        glm::vec4 label_col = config_.label_color;
        if (button_pressed_[i]) {
            label_col.w = 1.0f;
        }
        render_button_label(cx, cy, labels[i], label_col);
    }
}

void VirtualController::render_dpad() const {
    int sw = screenWidth;
    int sh = screenHeight;
    float cx = get_dpad_center_x();
    float cy = get_dpad_center_y();
    float arm = config_.dpad_size;
    float arm_w = arm * 0.4f;  // Width of each D-pad arm

    // Draw D-pad as a cross shape using quads
    // Vertical arm
    glm::vec4 v_color = config_.dpad_color;
    v_color.w *= config_.dpad_opacity;
    if (dpad_active_direction_ == 0 || dpad_active_direction_ == 1) {
        v_color = vc_lerp(v_color, config_.dpad_pressed_color, 0.6f);
    }
    UIDrawHelper::drawColoredQuad(cx - arm_w * 0.5f, cy - arm,
                                   arm_w, arm * 2.0f, v_color, sw, sh);

    // Horizontal arm
    glm::vec4 h_color = config_.dpad_color;
    h_color.w *= config_.dpad_opacity;
    if (dpad_active_direction_ == 2 || dpad_active_direction_ == 3) {
        h_color = vc_lerp(h_color, config_.dpad_pressed_color, 0.6f);
    }
    UIDrawHelper::drawColoredQuad(cx - arm, cy - arm_w * 0.5f,
                                   arm * 2.0f, arm_w, h_color, sw, sh);

    // Center circle
    glm::vec4 center_color = config_.dpad_color;
    center_color.w *= config_.dpad_opacity * 0.8f;
    UIDrawHelper::drawSoftCircle(cx, cy, arm_w * 0.6f, center_color, sw, sh);

    // Direction indicators (small triangles/arrows using colored quads)
    float indicator_size = arm * 0.15f;
    glm::vec4 indicator_color = glm::vec4(0.8f, 0.8f, 0.8f, 0.3f);

    // Up indicator
    indicator_color.w = (dpad_active_direction_ == 0) ? 0.7f : 0.3f;
    UIDrawHelper::drawColoredQuad(cx - indicator_size, cy - arm + indicator_size,
                                   indicator_size * 2.0f, indicator_size, indicator_color, sw, sh);

    // Down indicator
    indicator_color.w = (dpad_active_direction_ == 1) ? 0.7f : 0.3f;
    UIDrawHelper::drawColoredQuad(cx - indicator_size, cy + arm - indicator_size * 2.0f,
                                   indicator_size * 2.0f, indicator_size, indicator_color, sw, sh);

    // Left indicator
    indicator_color.w = (dpad_active_direction_ == 2) ? 0.7f : 0.3f;
    UIDrawHelper::drawColoredQuad(cx - arm + indicator_size, cy - indicator_size,
                                   indicator_size, indicator_size * 2.0f, indicator_color, sw, sh);

    // Right indicator
    indicator_color.w = (dpad_active_direction_ == 3) ? 0.7f : 0.3f;
    UIDrawHelper::drawColoredQuad(cx + arm - indicator_size * 2.0f, cy - indicator_size,
                                   indicator_size, indicator_size * 2.0f, indicator_color, sw, sh);
}

void VirtualController::render_triggers() const {
    int sw = screenWidth;
    int sh = screenHeight;
    float tw = config_.trigger_width;
    float th = config_.trigger_height;

    for (int i = 0; i < 2; ++i) {
        float cx = get_trigger_center_x(i);
        float cy = get_trigger_center_y();

        // Press animation
        float timer = trigger_press_timers_[i];
        float press_t = (config_.press_anim_duration > 0.0f)
                        ? timer / config_.press_anim_duration : 0.0f;
        press_t = std::min(press_t, 1.0f);

        glm::vec4 color = config_.trigger_color;
        if (trigger_pressed_[i]) {
            color = vc_lerp(color, config_.trigger_pressed_color, press_t);
            color.w = config_.trigger_pressed_opacity;
        } else {
            color.w = config_.trigger_opacity;
        }

        // Trigger background
        UIDrawHelper::drawGlowRect(cx - tw * 0.5f, cy - th * 0.5f, tw, th,
                                    3.0f, color, glm::vec4(color.x, color.y, color.z, 0.0f),
                                    sw, sh);

        // Border
        glm::vec4 border_col = glm::vec4(0.6f, 0.6f, 0.6f, 0.3f);
        UIDrawHelper::drawBorder(cx - tw * 0.5f, cy - th * 0.5f, tw, th,
                                  1.0f, border_col, sw, sh);

        // Label
        const char* label = (i == 0) ? "L" : "R";
        glm::vec4 label_col = config_.label_color;
        if (trigger_pressed_[i]) label_col.w = 1.0f;
        render_button_label(cx, cy, label, label_col);
    }
}

void VirtualController::render_start_select() const {
    int sw = screenWidth;
    int sh = screenHeight;
    float bw = config_.start_select_width;
    float bh = config_.start_select_height;

    for (int i = 0; i < 2; ++i) {
        float cx = get_start_select_center_x(i);
        float cy = get_start_select_center_y();

        // Press animation
        float timer = start_select_press_timers_[i];
        float press_t = (config_.press_anim_duration > 0.0f)
                        ? timer / config_.press_anim_duration : 0.0f;
        press_t = std::min(press_t, 1.0f);

        glm::vec4 color = config_.start_select_color;
        if (start_select_pressed_[i]) {
            color = vc_lerp(color, config_.start_select_pressed_color, press_t);
            color.w = config_.start_select_pressed_opacity;
        } else {
            color.w = config_.start_select_opacity;
        }

        // Ornate frame background
        glm::vec4 shadow_color = glm::vec4(0.1f, 0.1f, 0.1f, 0.3f);
        UIDrawHelper::drawOrnateFrame(cx - bw * 0.5f, cy - bh * 0.5f, bw, bh,
                                       2.0f, color, shadow_color, sw, sh);

        // Label
        const char* label = (i == 0) ? "ST" : "SE";
        glm::vec4 label_col = config_.label_color;
        label_col.w *= 0.8f;
        if (start_select_pressed_[i]) label_col.w = 1.0f;
        render_button_label(cx, cy, label, label_col);
    }
}

void VirtualController::render_button_label(float cx, float cy, const char* label,
                                             const glm::vec4& color) const {
    if (!text_renderer_) return;

    // Simple text rendering - center the label on the button
    // TextRenderer::renderText expects screen coordinates
    float scale = 0.8f;
    text_renderer_->renderText(label, cx - 4.0f, cy + 4.0f, color, scale);
}
