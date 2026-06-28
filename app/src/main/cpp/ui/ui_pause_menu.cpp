#include "ui_pause_menu.h"
#include "placeholder_assets.h"
#include "ui_draw_helper.h"
#include <GLES3/gl3.h>
#include <cstdio>
#include <algorithm>

UIPauseMenu::UIPauseMenu(const std::string& title)
    : UIPanel(title.empty() ? "Pause Menu" : title) {
    // Dark medieval style menu
    setBackgroundColor(glm::vec4(
        PlaceholderAssets::Colors::PARCHMENT_DARK.x * 0.9f,
        PlaceholderAssets::Colors::PARCHMENT_DARK.y * 0.85f,
        PlaceholderAssets::Colors::PARCHMENT_DARK.z * 0.8f, 0.95f));
    setBorderColor(glm::vec4(
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT.x,
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT.y,
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT.z, 1.0f));
    setBorderWidth(4.0f);
    setTitleBarColor(glm::vec4(0.15f, 0.1f, 0.05f, 0.9f));
    setCloseButtonVisible(true);
    setDraggable(false);
}

bool UIPauseMenu::initialize(TextRenderer* tr) {
    if (!tr) return false;
    textRenderer = tr;
    return UIPanel::initialize();
}

void UIPauseMenu::open() {
    selectedItemIndex = RESUME;  // Default to Resume
    scrollOffset = 0;
    setVisible(true);
}

void UIPauseMenu::close() {
    setVisible(false);
}

void UIPauseMenu::update(float deltaTime) {
    UIPanel::update(deltaTime);
}

bool UIPauseMenu::onTouchDown(float x, float y, int pointerId) {
    if (!isVisible() || !isEnabled()) return false;

    // Close button
    if (isInsideCloseButton(x, y)) {
        close();
        return true;
    }

    // Menu item selection
    int itemIdx = hitTestMenuItem(x, y);
    if (itemIdx >= 0) {
        selectedItemIndex = itemIdx;
        if (onMenuSelected) {
            onMenuSelected(static_cast<MenuItem>(itemIdx));
        }
        return true;
    }

    return true;  // Consume all touches while open
}

void UIPauseMenu::render() {
    if (!isVisible()) return;

    GLboolean depthTestEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    renderBackgroundOverlay();

    UIPanel::render();

    renderMenuItems();
    renderSelectedDescription();

    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
}

void UIPauseMenu::renderBackgroundOverlay() {
    // Semi-transparent black overlay for the game world behind the menu
    UIDrawHelper::drawColoredQuad(0.0f, 0.0f, screenWidth, screenHeight,
        glm::vec4(0.0f, 0.0f, 0.0f, 0.5f),
        screenWidth, screenHeight);
}

void UIPauseMenu::renderMenuItems() {
    if (!textRenderer) return;
    glm::vec2 cp = getContentPosition();
    float cy = getMenuStartY();

    // Menu header
    PlaceholderAssets::drawPanel(cp.x + 10.0f, cy - 30.0f, getContentSize().x - 20.0f, 28.0f,
        PlaceholderAssets::Colors::BROWN_ACCENT,
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT);
    textRenderer->renderText("Game Menu",
        cp.x + 20.0f, cy - 24.0f,
        glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT), 0.75f);
    cy += 8.0f;

    // Render visible menu items
    int visEnd = std::min(scrollOffset + MAX_VISIBLE_ITEMS, static_cast<int>(MENU_COUNT));

    for (int i = scrollOffset; i < visEnd; ++i) {
        bool selected = (i == selectedItemIndex);

        // Item background
        glm::vec3 bgColor = selected
                            ? glm::vec3(PlaceholderAssets::Colors::GOLD_HIGHLIGHT)
                            : glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT) * 0.85f;
        glm::vec3 borderColor = selected
                                ? glm::vec3(PlaceholderAssets::Colors::GOLD_HIGHLIGHT)
                                : glm::vec3(PlaceholderAssets::Colors::BROWN_ACCENT);

        float itemY = cy + (i - scrollOffset) * (MENU_ITEM_H + MENU_ITEM_GAP);
        PlaceholderAssets::drawPanel(cp.x + 10.0f, itemY, getContentSize().x - 20.0f, MENU_ITEM_H,
            bgColor, borderColor);

        // Item text
        glm::vec3 textColor = selected
                             ? glm::vec3(0.0f, 0.0f, 0.0f)
                             : glm::vec3(0.1f, 0.08f, 0.06f);
        textRenderer->renderText(MENU_LABELS[i],
            cp.x + 25.0f, itemY + 14.0f,
            textColor, 0.7f);
    }
}

void UIPauseMenu::renderSelectedDescription() {
    if (!textRenderer || selectedItemIndex < 0 || selectedItemIndex >= MENU_COUNT) return;

    glm::vec2 cp = getContentPosition();
    float descY = getDescriptionY();

    // Description panel
    PlaceholderAssets::drawPanel(cp.x + 10.0f, descY, getContentSize().x - 20.0f, 60.0f,
        glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT) * 0.8f,
        glm::vec3(PlaceholderAssets::Colors::BROWN_ACCENT));

    // Description text
    textRenderer->renderText(MENU_DESCRIPTIONS[selectedItemIndex],
        cp.x + 20.0f, descY + 8.0f,
        glm::vec3(0.15f, 0.1f, 0.08f), 0.6f);
}

// ─── hit testing ──────────────────────────────────────────────────────────────

int UIPauseMenu::hitTestMenuItem(float x, float y) const {
    glm::vec2 cp = getContentPosition();
    float cy = getMenuStartY() + 8.0f;

    int visEnd = std::min(scrollOffset + MAX_VISIBLE_ITEMS, static_cast<int>(MENU_COUNT));

    for (int i = scrollOffset; i < visEnd; ++i) {
        float itemY = cy + (i - scrollOffset) * (MENU_ITEM_H + MENU_ITEM_GAP);
        if (x >= cp.x + 10.0f && x <= cp.x + getContentSize().x - 10.0f &&
            y >= itemY && y <= itemY + MENU_ITEM_H) {
            return i;
        }
    }
    return -1;
}

// ─── layout helpers ───────────────────────────────────────────────────────────

float UIPauseMenu::getMenuStartY() const {
    return getContentPosition().y + 20.0f;
}

float UIPauseMenu::getDescriptionY() const {
    float menuEnd = getMenuStartY() + 8.0f + MAX_VISIBLE_ITEMS * (MENU_ITEM_H + MENU_ITEM_GAP);
    return getContentPosition().y + getContentSize().y - 75.0f;
}
