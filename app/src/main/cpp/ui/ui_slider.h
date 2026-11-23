#pragma once

#include "ui_component.h"

#include <string>

/**
 * @brief Slider widget (value + track + handle).
 *
 * The codebase has no generic slider widget: sliders elsewhere are panel-local
 * immediate-mode code (a nested Rect plus render()/hitTest()). This class follows
 * UIButton's base and naming so the chargen panel can carry real widgets, and
 * keeps the spec §5 template defaults (range 0..1, step 0.04, jump 0.1).
 *
 * Renderer-free: the numeric label is drawn by the owner (or omitted), so the
 * class can be constructed without a TextRenderer.
 */
class UISlider : public UIComponent {
public:
    UISlider(const std::string& name = "UISlider");
    ~UISlider() override = default;

    void render() override;

    void setRange(float min, float max, float step, float jump) {
        min_ = min;
        max_ = max;
        step_ = step;
        jump_ = jump;
    }
    void setValue(float v);          // clamped to [min, max]
    float getValue() const { return value_; }

    // Move by one step in `direction` (-1 / +1), clamped to [min, max]. This is
    // the per-tap adjustment the original applies with step 0.04 (spec §5).
    void stepValue(int direction);

    void setLabel(const std::string& label) { label_ = label; }
    const std::string& getLabel() const { return label_; }

    // Handle position in [0, 1] (0 when the range is degenerate).
    float normalized() const;

private:
    float min_ = 0.0f;
    float max_ = 1.0f;
    float step_ = 0.04f;
    float jump_ = 0.1f;
    float value_ = 0.0f;
    std::string label_;
};
