#pragma once

#include "ui_component.h"

#include <string>

/**
 * @brief Toggle widget (checkbox with a checked flag).
 *
 * Like UISlider, the codebase has no generic toggle widget (toggles elsewhere
 * are panel-local immediate-mode rows). This follows UIButton's base and naming
 * and is renderer-free so it can be constructed without a TextRenderer.
 */
class UIToggle : public UIComponent {
public:
    UIToggle(const std::string& name = "UIToggle");
    ~UIToggle() override = default;

    void render() override;

    void setChecked(bool checked) { checked_ = checked; }
    bool isChecked() const { return checked_; }
    void toggleChecked() { checked_ = !checked_; }

    void setLabel(const std::string& label) { label_ = label; }
    const std::string& getLabel() const { return label_; }

private:
    bool checked_ = false;
    std::string label_;
};
