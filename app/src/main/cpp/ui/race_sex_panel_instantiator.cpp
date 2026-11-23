#include "race_sex_panel.h"

#include "menu_text_component.h"
#include "ui_button.h"
#include "ui_component.h"
#include "ui_slider.h"
#include "ui_toggle.h"

#include <glm/glm.hpp>
#include <memory>

// Device-only: turns the built descriptors into live widgets and lays them out.
// Kept in its own translation unit so the host suite (which links
// race_sex_panel.cpp but not this file) stays free of the GL / TextRenderer /
// UISystem stack. This mirrors the menu_ui_builder (descriptor, host-testable)
// vs menu_ui_instantiator (device-only) split.
//
// Element identity is by id (spec §0); the widget is presentation only.

namespace {

// Set the text scale on whichever label-bearing widget this is. Both setters are
// inline on their classes, so the dynamic_cast only needs the headers (no link
// dependency on ui_button.cpp / menu_text_component.cpp).
void setWidgetScale(UIComponent* widget, float scale) {
    if (auto* button = dynamic_cast<UIButton*>(widget)) {
        button->setLabelScale(scale);
    } else if (auto* text = dynamic_cast<oblivion::ui::MenuTextComponent*>(widget)) {
        text->setTextScale(scale);
    }
}

}  // namespace

void RaceSexPanel::instantiate(TextRenderer* text_renderer) {
    // Idempotent: drop any widgets from a previous call before rebuilding, so a
    // re-open (which may call this again) never doubles the children.
    for (Element& e : elements_) {
        if (e.widget) {
            removeChild(e.widget);
            e.widget.reset();
        }
    }
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

    // The pane-external controls (spec §9.3) have no descriptor; give them plain
    // placeholder widgets so they pass through the same layout transform. Their
    // final art (button_long.xml / vertical_scroll.xml) is a later step.
    if (!done_widget_) {
        auto done = std::make_shared<UIButton>("RaceDone");
        done->setLabel("Done");
        done->setTextRenderer(text_renderer);
        done_widget_ = done;
        addChild(done_widget_);
    }
    if (!scroll_bar_widget_) {
        auto bar = std::make_shared<UIComponent>("RaceScrollBar");
        bar->setBackgroundColor(glm::vec4(0.55f, 0.55f, 0.60f, 0.6f));
        scroll_bar_widget_ = bar;
        addChild(scroll_bar_widget_);
    }
    // §10.1 race_name (id 10): the name label. Text is filled from the player
    // name in applyLayout().
    if (!name_widget_) {
        auto label =
            std::make_shared<oblivion::ui::MenuTextComponent>("RaceName", text_renderer);
        label->setJustify("center");
        name_widget_ = label;
        addChild(name_widget_);
    }

    // The chargen menu is a game screen, not a desktop window: no title bar and
    // no close button (spec §9.5).
    setTitleBarHeight(0.0f);
    setCloseButtonVisible(false);

    // Page switches and scroll changes go through setScrollRows/handleClick, which
    // call this hook to re-place and re-cull the widgets (spec §9.7).
    setOnLayoutChanged([this]() { applyLayout(); });

    applyLayout();
}

void RaceSexPanel::applyLayout() {
    // The panel itself is fixed at the device origin and spans the screen; its
    // children then carry absolute (device) coordinates, so the parent offset
    // getAbsolutePosition() adds is (0,0) and does not double-count.
    setPosition(0.0f, 0.0f);
    setSize(static_cast<float>(screenWidth), static_cast<float>(screenHeight));

    const float s = layoutScale();
    for (Element& e : elements_) {
        if (!e.widget) continue;
        const int row = rowOfElement(e.id);
        if (row < 0) continue;
        const RectF r = itemRectDevice(e.page_slot, row);
        e.widget->setPosition(r.x, r.y);
        e.widget->setSize(r.w, r.h);
        e.widget->setVisible(isElementVisible(e.page_slot, row));
        setWidgetScale(e.widget.get(), s);
    }

    if (done_widget_) {
        const RectF r = doneRectDevice();
        done_widget_->setPosition(r.x, r.y);
        done_widget_->setSize(r.w, r.h);
        done_widget_->setVisible(true);
    }
    if (scroll_bar_widget_) {
        const RectF r = scrollBarRectDevice();
        scroll_bar_widget_->setPosition(r.x, r.y);
        scroll_bar_widget_->setSize(r.w, r.h);
        // §9.2: the scroll bar only exists when the page overflows the window.
        scroll_bar_widget_->setVisible(itemCount(current_page_) > SCROLL_VISIBLE);
    }
    if (name_widget_) {
        // §10.1: race_name shows the player's current name (never blank).
        const RectF r = nameRectDevice();
        name_widget_->setPosition(r.x, r.y);
        name_widget_->setSize(r.w, r.h);
        name_widget_->setVisible(true);
        setWidgetScale(name_widget_.get(), s);
        if (auto* text = dynamic_cast<oblivion::ui::MenuTextComponent*>(name_widget_.get())) {
            text->setText(player_name_);
        }
    }
}
