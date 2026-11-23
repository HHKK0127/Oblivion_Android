#include "race_sex_panel.h"

#include "menu_text_component.h"
#include "ui_button.h"
#include "ui_slider.h"
#include "ui_toggle.h"

#include <memory>

// Device-only: turns the built descriptors into live widgets. Kept in its own
// translation unit so the host suite (which links race_sex_panel.cpp but not
// this file) stays free of the GL / TextRenderer / UISystem stack. This mirrors
// the menu_ui_builder (descriptor, host-testable) vs menu_ui_instantiator
// (device-only) split.
//
// Element identity is by id (spec §0); the widget is presentation only.
void RaceSexPanel::instantiate(TextRenderer* text_renderer) {
    for (Element& e : elements_) {
        std::shared_ptr<UIComponent> widget;
        switch (e.type) {
            case ElemType::Button: {
                auto button = std::make_shared<UIButton>(e.label);
                button->setLabel(e.label);
                button->setTextRenderer(text_renderer);
                widget = button;
                break;
            }
            case ElemType::Text: {
                auto text =
                    std::make_shared<oblivion::ui::MenuTextComponent>(e.label, text_renderer);
                text->setText(e.label);
                widget = text;
                break;
            }
            case ElemType::Toggle: {
                auto toggle = std::make_shared<UIToggle>(e.label);
                toggle->setLabel(e.label);
                toggle->setChecked(e.checked);
                widget = toggle;
                break;
            }
            case ElemType::Slider: {
                auto slider = std::make_shared<UISlider>(e.label);
                slider->setLabel(e.label);
                slider->setRange(e.min, e.max, e.step, e.jump);
                slider->setValue(e.value);
                widget = slider;
                break;
            }
        }
        e.widget = widget;
        if (widget) {
            addChild(widget);
        }
    }
}
