#include "debug_menu.h"
#include "game_console.h"
#include "text_renderer.h"
#include "ui_draw_helper.h"
#include "texture_viewer.h"
#include "model_viewer.h"
#include "world_viewer.h"
#include "viewer_3d.h"
#include <algorithm>
#include <cmath>
#include <ctime>
#include <android/log.h>

#define LOG_TAG_DEBUG "DebugMenu"
#define LOGI_DEBUG(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_DEBUG, __VA_ARGS__)

DebugMenu::DebugMenu()
    : textRenderer(nullptr), console(nullptr),
      visible(false), initialized(false),
      screenWidth(1080), screenHeight(1920),
      safeLeft(0), safeTop(0), safeRight(160), safeBottom(220),
      currentTab(Tab::PLAYER),
      feedbackTimer(0.0f),
      feedbackColor(0.4f, 0.9f, 0.4f),
      textureViewer(nullptr), modelViewer(nullptr), worldViewer(nullptr), viewer3D(nullptr) {}

DebugMenu::~DebugMenu() { cleanup(); }

bool DebugMenu::initialize(TextRenderer* tr, GameConsole* c) {
    if (initialized) return true;
    if (!tr || !c) return false;
    textRenderer = tr;
    console = c;
    createTabButtons();
    createAllTabContents();

    // Initialize viewers
    textureViewer = new TextureViewer();
    textureViewer->initialize(textRenderer, nullptr);
    textureViewer->setScreenSize(screenWidth, screenHeight);

    modelViewer = new ModelViewer();
    modelViewer->initialize(textRenderer, nullptr);
    modelViewer->setScreenSize(screenWidth, screenHeight);

    worldViewer = new WorldViewer();
    worldViewer->initialize(textRenderer, nullptr);
    worldViewer->setScreenSize(screenWidth, screenHeight);

    viewer3D = new Viewer3D();
    viewer3D->initialize(textRenderer);
    viewer3D->setScreenSize(screenWidth, screenHeight);

    initialized = true;
    LOGI_DEBUG("DebugMenu initialized");
    return true;
}

void DebugMenu::cleanup() {
    tabButtons.clear();
    tabContents.clear();

    // Cleanup viewers
    if (textureViewer) {
        textureViewer->cleanup();
        delete textureViewer;
        textureViewer = nullptr;
    }
    if (modelViewer) {
        modelViewer->cleanup();
        delete modelViewer;
        modelViewer = nullptr;
    }
    if (worldViewer) {
        worldViewer->cleanup();
        delete worldViewer;
        worldViewer = nullptr;
    }
    if (viewer3D) {
        viewer3D->cleanup();
        delete viewer3D;
        viewer3D = nullptr;
    }

    initialized = false;
}

void DebugMenu::toggle() {
    visible = !visible;
    touchState = {};
    LOGI_DEBUG("DebugMenu %s", visible ? "opened" : "closed");
}

void DebugMenu::selectTab(int tabIndex) {
    if (tabIndex >= 0 && tabIndex < static_cast<int>(Tab::COUNT)) {
        currentTab = static_cast<Tab>(tabIndex);
        touchState = {};
        LOGI_DEBUG("DebugMenu::selectTab(%d) -> %s", tabIndex, getTabName(currentTab).c_str());
    } else {
        LOGI_DEBUG("DebugMenu::selectTab(%d) - invalid index", tabIndex);
    }
}

void DebugMenu::setScreenSize(int w, int h) {
    screenWidth = std::max(1, w);
    screenHeight = std::max(1, h);

    // Keep the debug surface inside the display safe area. The old fixed
    // right/bottom insets left too little room on narrow devices and caused
    // tabs and labels to render outside their panel.
    const float minDim = static_cast<float>(std::min(screenWidth, screenHeight));
    const float s = std::clamp(minDim / 1080.0f, 0.5f, 2.0f);
    safeLeft = std::max(140.0f * s, screenWidth * 0.025f);
    safeTop = std::max(8.0f * s, screenHeight * 0.012f);
    safeRight = std::max(24.0f * s, screenWidth * 0.025f);
    safeBottom = std::max(64.0f * s, screenHeight * 0.08f);

    // Propagate to viewers
    if (textureViewer) textureViewer->setScreenSize(w, h);
    if (modelViewer) modelViewer->setScreenSize(w, h);
    if (worldViewer) worldViewer->setScreenSize(w, h);
    if (viewer3D) viewer3D->setScreenSize(w, h);
}

void DebugMenu::setWorldManager(class WorldManager* worldManager) {
    if (worldViewer) {
        worldViewer->setWorldManager(worldManager);
        LOGI_DEBUG("WorldManager connected to WorldViewer");
    }
}

// ==================== Touch Event Handling ====================

void DebugMenu::onTouchDown(float x, float y) {
    if (!visible) return;

    // Route to Viewer3D first when it is visible (modal overlay)
    if (viewer3D && viewer3D->isVisible()) {
        viewer3D->onTouchDown(x, y);
        return;
    }

    touchState.isActive = true;
    touchState.startX = touchState.lastX = x;
    touchState.startY = touchState.lastY = y;
    touchState.pressedButton = nullptr;
    touchState.pressedSlider = nullptr;
    touchState.isScrolling = false;

    LOGI_DEBUG("onTouchDown: touch=(%.1f, %.1f) screen=%dx%d scale=%.2f",
               x, y, screenWidth, screenHeight, getScale());

    // Check tab buttons first
    Button* tabBtn = hitTestTab(x, y);
    if (tabBtn) {
        touchState.pressedButton = tabBtn;
        tabBtn->isPressed = true;
        tabBtn->pressTimer = 0.15f;
        LOGI_DEBUG("Hit tab button: %s at (%.0f, %.0f) size (%.0f x %.0f)",
                   tabBtn->label.c_str(), tabBtn->x, tabBtn->y, tabBtn->w, tabBtn->h);
        return;
    }

    // Check sliders (before content buttons, as sliders are more precise)
    Slider* slider = hitTestSlider(x, y);
    if (slider) {
        touchState.pressedSlider = slider;
        slider->dragging = true;
        // Update value immediately based on touch position
        float t = (x - slider->x) / slider->w;
        t = std::clamp(t, 0.0f, 1.0f);
        float raw = slider->min + (slider->max - slider->min) * t;
        float snapped = std::round(raw / slider->step) * slider->step;
        snapped = std::clamp(snapped, slider->min, slider->max);
        if (snapped != slider->value) {
            slider->value = snapped;
            if (slider->onChange) slider->onChange(snapped);
        }
        LOGI_DEBUG("Hit slider: '%s' value=%.2f", slider->label.c_str(), slider->value);
        return;
    }

    // Check content buttons
    Button* contentBtn = hitTestContent(x, y);
    if (contentBtn) {
        touchState.pressedButton = contentBtn;
        contentBtn->isPressed = true;
        contentBtn->pressTimer = 0.15f;
        LOGI_DEBUG("Hit content button: '%s' at (%.0f, %.0f) size (%.0f x %.0f)",
                   contentBtn->label.c_str(), contentBtn->x, contentBtn->y,
                   contentBtn->w, contentBtn->h);
        return;
    }

    // No button hit - start scrolling
    LOGI_DEBUG("No button hit at (%.1f, %.1f) - starting scroll", x, y);
    touchState.isScrolling = true;
}

void DebugMenu::onTouchMove(float x, float y) {
    // Route to Viewer3D first when it is visible
    if (viewer3D && viewer3D->isVisible()) {
        viewer3D->onTouchMove(x, y);
        return;
    }

    if (!touchState.isActive) return;

    // Handle slider dragging
    if (touchState.pressedSlider) {
        Slider* slider = touchState.pressedSlider;
        float t = (x - slider->x) / slider->w;
        t = std::clamp(t, 0.0f, 1.0f);
        float raw = slider->min + (slider->max - slider->min) * t;
        float snapped = std::round(raw / slider->step) * slider->step;
        snapped = std::clamp(snapped, slider->min, slider->max);
        if (snapped != slider->value) {
            slider->value = snapped;
            if (slider->onChange) slider->onChange(snapped);
        }
        touchState.lastX = x;
        touchState.lastY = y;
        return;
    }

    float dx = x - touchState.startX;
    float dy = y - touchState.startY;
    float dist = std::hypot(dx, dy);

    // If moved beyond threshold, switch to scrolling mode
    if (!touchState.isScrolling && dist > TouchState::SCROLL_THRESHOLD) {
        touchState.isScrolling = true;
        if (touchState.pressedButton) {
            touchState.pressedButton->isPressed = false;
            touchState.pressedButton = nullptr;
        }
    }

    // Handle scrolling
    if (touchState.isScrolling) {
        float moveY = y - touchState.lastY;
        size_t idx = static_cast<size_t>(currentTab);
        if (idx < tabContents.size()) {
            tabContents[idx].scrollOffset -= moveY;
            clampScrollOffsets();
        }
    }

    touchState.lastX = x;
    touchState.lastY = y;
}

void DebugMenu::onTouchUp(float x, float y) {
    // Route to Viewer3D first when it is visible
    if (viewer3D && viewer3D->isVisible()) {
        viewer3D->onTouchUp();
        return;
    }

    LOGI_DEBUG("onTouchUp: touch=(%.1f, %.1f) isActive=%d pressedBtn=%p pressedSlider=%p isScrolling=%d",
               x, y, touchState.isActive ? 1 : 0, (void*)touchState.pressedButton,
               (void*)touchState.pressedSlider, touchState.isScrolling ? 1 : 0);
    if (!touchState.isActive) return;

    // Release slider
    if (touchState.pressedSlider) {
        touchState.pressedSlider->dragging = false;
        touchState.pressedSlider = nullptr;
        touchState = {};
        return;
    }

    float dx = x - touchState.startX;
    float dy = y - touchState.startY;
    float dist = std::hypot(dx, dy);

    LOGI_DEBUG("onTouchUp: dist=%.2f threshold=%.2f", dist, TouchState::TAP_THRESHOLD);

    // Only execute command if it was a tap (not a scroll)
    if (!touchState.isScrolling && dist < TouchState::TAP_THRESHOLD) {
        if (touchState.pressedButton) {
            // Check if this is a tab button
            int tabIdx = findTabIndex(touchState.pressedButton);
            if (tabIdx >= 0) {
                // Switch to the selected tab
                currentTab = static_cast<Tab>(tabIdx);
                LOGI_DEBUG("Switched to tab: %s", getTabName(currentTab).c_str());
            } else {
                // Execute content button command
                executeButtonCommand(*touchState.pressedButton);
            }
        }
    }

    // Reset pressed state
    if (touchState.pressedButton) {
        touchState.pressedButton->isPressed = false;
    }
    touchState = {};
}

void DebugMenu::onTouchCancel() {
    if (touchState.pressedButton) {
        touchState.pressedButton->isPressed = false;
    }
    if (touchState.pressedSlider) {
        touchState.pressedSlider->dragging = false;
    }
    touchState = {};
}

// ==================== Command Execution ====================

void DebugMenu::executeButtonCommand(Button& btn) {
    if (btn.command.empty()) {
        LOGI_DEBUG("Button '%s' has no command", btn.label.c_str());
        return;
    }

    // Viewer toggle commands
    if (btn.command == "textureviewer") {
        if (textureViewer) {
            textureViewer->toggle();
            feedbackText = "Texture Viewer: " + std::string(textureViewer->isVisible() ? "ON" : "OFF");
            feedbackTimer = 2.0f;
            feedbackColor = glm::vec3(0.3f, 0.7f, 0.9f);
            LOGI_DEBUG("TextureViewer toggled");
        }
        return;
    }
    if (btn.command == "modelviewer") {
        if (modelViewer) {
            modelViewer->toggle();
            feedbackText = "Model Viewer: " + std::string(modelViewer->isVisible() ? "ON" : "OFF");
            feedbackTimer = 2.0f;
            feedbackColor = glm::vec3(0.3f, 0.7f, 0.9f);
            LOGI_DEBUG("ModelViewer toggled");
        }
        return;
    }
    if (btn.command == "worldviewer") {
        if (worldViewer) {
            worldViewer->toggle();
            feedbackText = "World Viewer: " + std::string(worldViewer->isVisible() ? "ON" : "OFF");
            feedbackTimer = 2.0f;
            feedbackColor = glm::vec3(0.3f, 0.7f, 0.9f);
            LOGI_DEBUG("WorldViewer toggled");
        }
        return;
    }
    if (btn.command == "viewer3d") {
        if (viewer3D) {
            viewer3D->toggle();
            feedbackText = "3D Viewer: " + std::string(viewer3D->isVisible() ? "ON" : "OFF");
            feedbackTimer = 2.0f;
            feedbackColor = glm::vec3(0.9f, 0.5f, 0.3f);
            LOGI_DEBUG("Viewer3D toggled");
        }
        return;
    }
    if (btn.command == "dumpstate") {
        dumpState();
        return;
    }
    if (btn.command == "startgame") {
        LOGI_DEBUG("Start Game command - closing debug menu and entering game world");
        feedbackText = "Starting Game...";
        feedbackTimer = 1.0f;
        feedbackColor = glm::vec3(0.2f, 0.9f, 0.4f);
        visible = false;  // Close debug menu
        if (onStartGame) {
            onStartGame();
        }
        return;
    }

    if (!console) {
        LOGI_DEBUG("Console not available");
        return;
    }
    console->executeCommand(btn.command);
    btn.pressTimer = 0.2f; // Visual feedback

    // Show feedback in DebugMenu
    feedbackText = "Executed: " + btn.command;
    feedbackTimer = 3.0f;  // Show for 3 seconds
    feedbackColor = glm::vec3(0.4f, 0.9f, 0.4f);  // Green

    LOGI_DEBUG("Executed: %s", btn.command.c_str());
}

int DebugMenu::findTabIndex(const Button* btn) const {
    for (size_t i = 0; i < tabButtons.size(); ++i) {
        if (&tabButtons[i] == btn) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

// ==================== Hit Testing ====================

DebugMenu::Button* DebugMenu::hitTestTab(float x, float y) {
    float s = getScale();
    float tabBarY = safeTop + 8.0f * s;
    float tabH = TAB_HEIGHT * s;

    if (y < tabBarY || y > tabBarY + tabH) return nullptr;

    for (auto& btn : tabButtons) {
        if (x >= btn.x && x <= btn.x + btn.w) {
            return &btn;
        }
    }
    return nullptr;
}

DebugMenu::Button* DebugMenu::hitTestContent(float x, float y) {
    float s = getScale();
    float tabBarY = safeTop + 8.0f * s;
    float tabH = TAB_HEIGHT * s;
    float contentY = tabBarY + tabH + 20.0f * s;
    float contentH = screenHeight - safeBottom - contentY;
    if (y < contentY || y > contentY + contentH) {
        LOGI_DEBUG("hitTestContent: y=%.1f outside content range [%.1f, %.1f] (screen=%d, safeBottom=%.0f)",
                   y, contentY, contentY + contentH, screenHeight, safeBottom);
        return nullptr;
    }

    size_t idx = static_cast<size_t>(currentTab);
    if (idx >= tabContents.size()) return nullptr;

    int btnCount = 0;
    for (auto& btn : tabContents[idx].buttons) {
        if (btn.y + btn.h < contentY || btn.y > contentY + contentH) continue;
        if (x >= btn.x && x <= btn.x + btn.w && y >= btn.y && y <= btn.y + btn.h) {
            LOGI_DEBUG("hitTestContent: HIT button '%s' at (%.0f, %.0f)-(%.0f, %.0f) touch=(%.1f, %.1f)",
                       btn.label.c_str(), btn.x, btn.y, btn.x + btn.w, btn.y + btn.h, x, y);
            return &btn;
        }
        btnCount++;
    }
    LOGI_DEBUG("hitTestContent: no hit for touch=(%.1f, %.1f) checked %d buttons, first btn at (%.0f, %.0f)",
               x, y, btnCount,
               tabContents[idx].buttons.empty() ? 0.0f : tabContents[idx].buttons[0].x,
               tabContents[idx].buttons.empty() ? 0.0f : tabContents[idx].buttons[0].y);
    return nullptr;
}

// ==================== Update and Render ====================

void DebugMenu::update(float deltaTime) {
    if (!visible) return;

    // Update press timers for visual feedback
    for (auto& btn : tabButtons) {
        if (btn.pressTimer > 0) {
            btn.pressTimer -= deltaTime;
            if (btn.pressTimer <= 0) btn.isPressed = false;
        }
    }
    for (auto& content : tabContents) {
        for (auto& btn : content.buttons) {
            if (btn.pressTimer > 0) {
                btn.pressTimer -= deltaTime;
                if (btn.pressTimer <= 0) btn.isPressed = false;
            }
        }
    }

    // Update feedback timer
    if (feedbackTimer > 0.0f) {
        feedbackTimer -= deltaTime;
    }

    // Update viewers
    if (textureViewer) textureViewer->update(deltaTime);
    if (modelViewer) modelViewer->update(deltaTime);
    if (worldViewer) worldViewer->update(deltaTime);
    if (viewer3D) viewer3D->update(deltaTime);

    calculateButtonPositions();
}

void DebugMenu::calculateButtonPositions() {
    float s = getScale();
    float margin = BUTTON_MARGIN * s;
    float x = safeLeft + margin;

    // Tab buttons - use smaller width to fit 13 tabs
    float tabY = safeTop + 8.0f * s;
    float tabH = TAB_HEIGHT * s;
    const float availableWidth = std::max(1.0f, static_cast<float>(screenWidth) - safeLeft - safeRight - margin * (tabButtons.size() + 1));
    const float naturalTabW = availableWidth / std::max<size_t>(1, tabButtons.size());
    const float tabW = naturalTabW >= 48.0f * s
        ? std::min(96.0f * s, naturalTabW)
        : std::max(1.0f, naturalTabW);
    for (auto& btn : tabButtons) {
        btn.x = x;
        btn.y = tabY;
        btn.w = tabW;
        btn.h = tabH;
        ensureMinTouchSize(btn);
        x += btn.w + margin;
    }

    // Content buttons
    size_t idx = static_cast<size_t>(currentTab);
    if (idx >= tabContents.size()) return;

    auto& content = tabContents[idx];
    float contentY = tabY + tabH + 20.0f * s;
    float btnH = BUTTON_HEIGHT * s;
    float btnW = (screenWidth - safeLeft - safeRight - margin * 3.0f) * 0.5f;

    for (size_t i = 0; i < content.buttons.size(); ++i) {
        int col = i % 2;
        int row = i / 2;
        content.buttons[i].x = safeLeft + margin + (btnW + margin) * col;
        content.buttons[i].y = contentY + (btnH + margin) * row - content.scrollOffset;
        content.buttons[i].w = btnW;
        content.buttons[i].h = btnH;
        ensureMinTouchSize(content.buttons[i]);
    }

    // Position sliders after buttons (full width)
    int buttonRows = (static_cast<int>(content.buttons.size()) + 1) / 2;
    float sliderY = contentY + (btnH + margin) * buttonRows;
    float sliderW = screenWidth - safeLeft - safeRight - margin * 2.0f;
    float sliderH = SLIDER_HEIGHT * s;

    for (size_t i = 0; i < content.sliders.size(); ++i) {
        content.sliders[i].x = safeLeft + margin;
        content.sliders[i].y = sliderY + (sliderH + margin) * i - content.scrollOffset;
        content.sliders[i].w = sliderW;
        content.sliders[i].h = sliderH;
        ensureMinTouchSize(content.sliders[i]);
    }
}

void DebugMenu::render() {
    if (!visible || !textRenderer) return;
    renderBackground();
    renderTabBar();
    renderContent();

    // Render viewers (overlay on top)
    if (textureViewer && textureViewer->isVisible()) {
        textureViewer->render();
    }
    if (modelViewer && modelViewer->isVisible()) {
        modelViewer->render();
    }
    if (worldViewer && worldViewer->isVisible()) {
        worldViewer->render();
    }
    if (viewer3D && viewer3D->isVisible()) {
        viewer3D->render();
    }
}

void DebugMenu::renderBackground() {
    // Semi-transparent dark overlay (full screen)
    UIDrawHelper::drawColoredQuad(0, 0, screenWidth, screenHeight,
                                   glm::vec4(0.0f, 0.0f, 0.0f, 0.75f),
                                   screenWidth, screenHeight);
}

void DebugMenu::renderTabBar() {
    float s = getScale();

    for (size_t i = 0; i < tabButtons.size(); ++i) {
        auto& btn = tabButtons[i];
        bool isActive = (i == static_cast<size_t>(currentTab));

        // Tab background color
        glm::vec4 bgColor = isActive ?
            glm::vec4(0.4f, 0.7f, 0.4f, 0.9f) :
            glm::vec4(0.2f, 0.2f, 0.3f, 0.8f);

        if (btn.isPressed) {
            bgColor = glm::vec4(0.5f, 0.8f, 0.5f, 1.0f);
        }

        UIDrawHelper::drawColoredQuad(btn.x, btn.y, btn.w, btn.h, bgColor,
                                       screenWidth, screenHeight);

        // Keep labels inside the tab even when the device is narrow.
                const float labelScale = std::max(0.60f, 0.82f * s);
        const float maxTextWidth = std::max(1.0f, btn.w - 8.0f * s);
        std::string label = btn.label;
        while (label.size() > 1 && textRenderer->getTextWidth(label, labelScale) > maxTextWidth) {
            label.pop_back();
        }
        if (label != btn.label && label.size() > 1) {
            label.back() = '.';
        }
        glm::vec3 textColor = isActive ? glm::vec3(1.0f, 1.0f, 1.0f) : glm::vec3(0.82f, 0.82f, 0.86f);
        const float textWidth = textRenderer->getTextWidth(label, labelScale);
                const float textHeight = textRenderer->getTextHeight(labelScale);
                textRenderer->renderText(label.c_str(),
                                          btn.x + std::max(4.0f * s, (btn.w - textWidth) * 0.5f),
                                          btn.y + (btn.h - textHeight) * 0.5f,
                                          textColor, labelScale);
    }
}

void DebugMenu::renderContent() {
    float s = getScale();
    float tabY = safeTop + 8.0f * s;
    float tabH = TAB_HEIGHT * s;
    float contentY = tabY + tabH + 20.0f * s;
    float contentH = screenHeight - safeBottom - contentY;

    size_t tabIdx = static_cast<size_t>(currentTab);
    if (tabIdx >= tabContents.size()) return;
    TabContent& content = tabContents[tabIdx];

    // Clip content area (draw background)
    UIDrawHelper::drawColoredQuad(safeLeft, contentY,
                                   screenWidth - safeLeft - safeRight, contentH,
                                   glm::vec4(0.1f, 0.1f, 0.15f, 0.6f),
                                   screenWidth, screenHeight);

    for (auto& btn : content.buttons) {
        // Skip if outside visible area
        if (btn.y + btn.h < contentY || btn.y > contentY + contentH) continue;

        renderButton(btn, s);
    }

    // Render sliders
    for (auto& slider : content.sliders) {
        if (slider.y + slider.h < contentY || slider.y > contentY + contentH) continue;
        renderSlider(slider, s);
    }

    // Render command feedback at bottom of content area
    if (feedbackTimer > 0.0f && textRenderer) {
        float feedbackY = contentY + contentH - 30.0f * s;
            float feedbackFontSize = 0.60f * s;
        float textW = textRenderer->getTextWidth(feedbackText.c_str(), feedbackFontSize);
        float feedbackX = (screenWidth - textW) * 0.5f;

        // Background for feedback
        UIDrawHelper::drawColoredQuad(feedbackX - 10.0f * s, feedbackY - 5.0f * s,
                                       textW + 20.0f * s, 25.0f * s,
                                       glm::vec4(0.0f, 0.0f, 0.0f, 0.8f),
                                       screenWidth, screenHeight);

        // Feedback text
        float alpha = std::min(1.0f, feedbackTimer);
            float textH = textRenderer->getTextHeight(feedbackFontSize);
            textRenderer->renderText(feedbackText.c_str(), feedbackX, feedbackY + (25.0f * s - textH) * 0.5f,
                                      glm::vec4(feedbackColor.x, feedbackColor.y, feedbackColor.z, alpha), feedbackFontSize);
    }
}

void DebugMenu::renderButton(Button& btn, float s) {
    glm::vec4 bgColor = btn.isPressed ?
        glm::vec4(0.5f, 0.7f, 1.0f, 0.9f) :
        glm::vec4(0.3f, 0.3f, 0.35f, 0.8f);

    UIDrawHelper::drawColoredQuad(btn.x, btn.y, btn.w, btn.h, bgColor,
                                   screenWidth, screenHeight);

        const float labelScale = std::max(0.60f, 0.78f * s);
    const float maxTextWidth = std::max(1.0f, btn.w - 20.0f * s);
    std::string label = btn.label;
    while (label.size() > 1 && textRenderer->getTextWidth(label, labelScale) > maxTextWidth) {
        label.pop_back();
    }
    if (label != btn.label && label.size() > 1) {
        label.back() = '.';
    }
        const float textW = textRenderer->getTextWidth(label, labelScale);
        const float textH = textRenderer->getTextHeight(labelScale);
        textRenderer->renderText(label.c_str(),
                                  btn.x + 10.0f * s,
                                  btn.y + (btn.h - textH) * 0.5f,
                                  glm::vec3(1.0f, 1.0f, 1.0f), labelScale);
    }

void DebugMenu::renderSlider(Slider& slider, float s) {
    // Slider track background
    float trackY = slider.y + slider.h * 0.4f;
    float trackH = slider.h * 0.2f;
    UIDrawHelper::drawColoredQuad(slider.x, trackY, slider.w, trackH,
                                   glm::vec4(0.2f, 0.2f, 0.25f, 0.8f),
                                   screenWidth, screenHeight);

    // Slider fill (from min to current value)
    float t = (slider.value - slider.min) / (slider.max - slider.min);
    t = std::clamp(t, 0.0f, 1.0f);
    float fillW = slider.w * t;
    UIDrawHelper::drawColoredQuad(slider.x, trackY, fillW, trackH,
                                   glm::vec4(0.3f, 0.6f, 1.0f, 0.9f),
                                   screenWidth, screenHeight);

    // Slider handle
    float handleW = slider.h * 0.6f;
    float handleH = slider.h * 0.8f;
    float handleX = slider.x + fillW - handleW * 0.5f;
    float handleY = slider.y + (slider.h - handleH) * 0.5f;
    glm::vec4 handleColor = slider.dragging ?
        glm::vec4(0.6f, 0.8f, 1.0f, 1.0f) :
        glm::vec4(0.8f, 0.8f, 0.85f, 1.0f);
    UIDrawHelper::drawColoredQuad(handleX, handleY, handleW, handleH,
                                   handleColor, screenWidth, screenHeight);

    // Label and value text
    if (textRenderer) {
        const float labelScale = std::max(0.50f, 0.65f * s);
        char valueStr[64];
        snprintf(valueStr, sizeof(valueStr), slider.format.c_str(), slider.value);
        std::string displayText = slider.label + ": " + valueStr;

        const float maxTextWidth = slider.w - 10.0f * s;
        std::string label = displayText;
        while (label.size() > 1 && textRenderer->getTextWidth(label, labelScale) > maxTextWidth) {
            label.pop_back();
        }
        if (label != displayText && label.size() > 1) {
            label.back() = '.';
        }

        float textH = textRenderer->getTextHeight(labelScale);
        textRenderer->renderText(label.c_str(),
                                  slider.x + 5.0f * s,
                                  slider.y + (slider.h - textH) * 0.5f,
                                  glm::vec3(1.0f, 1.0f, 1.0f), labelScale);
    }
}

DebugMenu::Slider* DebugMenu::hitTestSlider(float x, float y) {
    size_t tabIdx = static_cast<size_t>(currentTab);
    if (tabIdx >= tabContents.size()) return nullptr;
    auto& content = tabContents[tabIdx];

    for (auto& slider : content.sliders) {
        if (x >= slider.x && x <= slider.x + slider.w &&
            y >= slider.y && y <= slider.y + slider.h) {
            return &slider;
        }
    }
    return nullptr;
}

// ==================== Slider Management ====================

void DebugMenu::addSlider(const std::string& label, float min, float max, float value,
                           float step, std::function<void(float)> onChange,
                           const std::string& format) {
    size_t tabIdx = static_cast<size_t>(currentTab);
    if (tabIdx >= tabContents.size()) return;

    Slider slider;
    slider.label = label;
    slider.min = min;
    slider.max = max;
    slider.value = value;
    slider.step = step;
    slider.onChange = std::move(onChange);
    slider.format = format;
    ensureMinTouchSize(slider);
    tabContents[tabIdx].sliders.push_back(slider);
}

void DebugMenu::updateSliderValue(const std::string& label, float value) {
    for (auto& tab : tabContents) {
        for (auto& slider : tab.sliders) {
            if (slider.label == label) {
                slider.value = value;
                return;
            }
        }
    }
}

// ==================== Keyboard Handling ====================

void DebugMenu::onKeyDown(int32_t keyCode) {
    if (!visible) return;

    // AKEYCODE_F1 = 131, AKEYCODE_ESCAPE = 111
    switch (keyCode) {
        case 131:  // F1
            toggle();
            return;
        case 111:  // Escape
            visible = false;
            return;
    }

    if (!visible) return;

    // Tab key: switch tab
    if (keyCode == 61) {  // AKEYCODE_TAB
        switchTab(1);
        return;
    }

    // Arrow keys
    switch (keyCode) {
        case 19:  // AKEYCODE_DPAD_UP
            moveSelection(-1);
            break;
        case 20:  // AKEYCODE_DPAD_DOWN
            moveSelection(1);
            break;
        case 21:  // AKEYCODE_DPAD_LEFT
            if (isOnSlider()) {
                nudgeSlider(-1);
            } else {
                switchTab(-1);
            }
            break;
        case 22:  // AKEYCODE_DPAD_RIGHT
            if (isOnSlider()) {
                nudgeSlider(1);
            } else {
                switchTab(1);
            }
            break;
        case 66:  // AKEYCODE_ENTER
            activateSelected();
            break;
        case 112:  // AKEYCODE_FORWARD_DEL (Delete)
            // Reset selected slider to midpoint
            {
                size_t tabIdx = static_cast<size_t>(currentTab);
                if (tabIdx < tabContents.size() && keyState.onSlider &&
                    keyState.selectedItemIndex >= 0 &&
                    keyState.selectedItemIndex < static_cast<int>(tabContents[tabIdx].sliders.size())) {
                    auto& slider = tabContents[tabIdx].sliders[keyState.selectedItemIndex];
                    slider.value = (slider.min + slider.max) * 0.5f;
                    if (slider.onChange) slider.onChange(slider.value);
                }
            }
            break;
    }
}

void DebugMenu::onKeyUp(int32_t keyCode) {
    // Not used currently
}

void DebugMenu::moveSelection(int delta) {
    size_t tabIdx = static_cast<size_t>(currentTab);
    if (tabIdx >= tabContents.size()) return;
    auto& content = tabContents[tabIdx];

    int totalItems = static_cast<int>(content.buttons.size()) +
                     static_cast<int>(content.sliders.size());

    if (totalItems == 0) return;

    if (keyState.selectedItemIndex < 0) {
        // Was on tab bar, move to content
        keyState.selectedItemIndex = (delta > 0) ? 0 : totalItems - 1;
    } else {
        keyState.selectedItemIndex += delta;
        if (keyState.selectedItemIndex < 0) {
            keyState.selectedItemIndex = -1;  // Back to tab bar
        } else if (keyState.selectedItemIndex >= totalItems) {
            keyState.selectedItemIndex = totalItems - 1;
        }
    }

    // Determine if we're on a slider
    keyState.onSlider = keyState.selectedItemIndex >= static_cast<int>(content.buttons.size());
}

void DebugMenu::activateSelected() {
    size_t tabIdx = static_cast<size_t>(currentTab);
    if (tabIdx >= tabContents.size()) return;
    auto& content = tabContents[tabIdx];

    if (keyState.selectedItemIndex < 0) {
        // On tab bar, do nothing (tabs are switched with left/right)
        return;
    }

    int btnCount = static_cast<int>(content.buttons.size());
    if (keyState.selectedItemIndex < btnCount) {
        executeButtonCommand(content.buttons[keyState.selectedItemIndex]);
    }
    // Sliders are adjusted with left/right, not enter
}

void DebugMenu::nudgeSlider(int direction) {
    size_t tabIdx = static_cast<size_t>(currentTab);
    if (tabIdx >= tabContents.size()) return;
    auto& content = tabContents[tabIdx];

    int btnCount = static_cast<int>(content.buttons.size());
    int sliderIdx = keyState.selectedItemIndex - btnCount;

    if (sliderIdx >= 0 && sliderIdx < static_cast<int>(content.sliders.size())) {
        auto& slider = content.sliders[sliderIdx];
        slider.value += slider.step * direction;
        slider.value = std::clamp(slider.value, slider.min, slider.max);
        if (slider.onChange) slider.onChange(slider.value);
    }
}

void DebugMenu::switchTab(int direction) {
    int tabCount = static_cast<int>(Tab::COUNT);
    keyState.selectedTabIndex += direction;
    if (keyState.selectedTabIndex < 0) keyState.selectedTabIndex = tabCount - 1;
    if (keyState.selectedTabIndex >= tabCount) keyState.selectedTabIndex = 0;
    currentTab = static_cast<Tab>(keyState.selectedTabIndex);
    keyState.selectedItemIndex = -1;
    keyState.onSlider = false;
}

bool DebugMenu::isOnSlider() const {
    return keyState.onSlider;
}

// ==================== Touch Size Enforcement ====================

float DebugMenu::getMinTouchPx() const {
    return MIN_TOUCH_DP * screenDensity;
}

void DebugMenu::ensureMinTouchSize(Button& btn) {
    float minPx = getMinTouchPx();
    if (btn.w < minPx) btn.w = minPx;
    if (btn.h < minPx) btn.h = minPx;
}

void DebugMenu::ensureMinTouchSize(Slider& slider) {
    float minPx = getMinTouchPx();
    if (slider.w < minPx) slider.w = minPx;
    if (slider.h < minPx) slider.h = minPx;
}

// ==================== State Dump ====================

void DebugMenu::dumpState() {
    // Use app-specific files directory
    char path[256];
    snprintf(path, sizeof(path), "/data/data/com.hhkk.oblivion/files/dump_%lld.log",
             static_cast<long long>(std::time(nullptr)));

    FILE* f = fopen(path, "w");
    if (!f) {
        LOGI_DEBUG("Failed to create dump file: %s", path);
        return;
    }

    fprintf(f, "# Oblivion Android Debug Dump\n");
    fprintf(f, "# timestamp: %lld\n", static_cast<long long>(std::time(nullptr)));
    fprintf(f, "# screen: %dx%d density=%.2f\n", screenWidth, screenHeight, screenDensity);
    fprintf(f, "\n");

    // Dump all tab contents
    for (int t = 0; t < static_cast<int>(Tab::COUNT); ++t) {
        fprintf(f, "## Tab: %s\n", getTabName(static_cast<Tab>(t)).c_str());
        if (t < static_cast<int>(tabContents.size())) {
            const auto& content = tabContents[t];
            for (const auto& btn : content.buttons) {
                fprintf(f, "  Button: %s -> %s\n", btn.label.c_str(), btn.command.c_str());
            }
            for (const auto& slider : content.sliders) {
                fprintf(f, "  Slider: %s = ", slider.label.c_str());
                char valStr[32];
                snprintf(valStr, sizeof(valStr), slider.format.c_str(), slider.value);
                fprintf(f, "%s [%.2f..%.2f step=%.2f]\n",
                        valStr, slider.min, slider.max, slider.step);
            }
        }
        fprintf(f, "\n");
    }

    fclose(f);
    LOGI_DEBUG("State dump written to: %s", path);

    // Show feedback
    feedbackText = "Dump saved: ";
    feedbackText += path;
    feedbackTimer = 3.0f;
    feedbackColor = glm::vec3(0.4f, 0.9f, 0.4f);
}

// ==================== Utility ====================

float DebugMenu::getScale() const {
    float minDim = static_cast<float>(std::min(screenWidth, screenHeight));
    float scale = minDim / 1080.0f;
    return std::clamp(scale, 0.5f, 2.0f);
}

std::string DebugMenu::getTabName(Tab tab) const {
    switch (tab) {
        case Tab::PLAYER: return "Player";
        case Tab::COMBAT: return "Combat";
        case Tab::INVENTORY: return "Items";
        case Tab::MAGIC: return "Magic";
        case Tab::QUEST: return "Quest";
        case Tab::NPC: return "NPC";
        case Tab::DIALOGUE: return "Talk";
        case Tab::WORLD: return "World";
        case Tab::SAVE: return "Save";
        case Tab::SYSTEM: return "System";
        case Tab::SOUND: return "Sound";
        case Tab::ASSETS: return "Assets";
        case Tab::LOGS: return "Logs";
        case Tab::DBG_PICKER: return "Picker";
        case Tab::DBG_AI: return "AI";
        case Tab::DBG_ANIM: return "Anim";
        case Tab::DBG_SCRIPT: return "Script";
        case Tab::DBG_QUEST: return "Qst";
        case Tab::DBG_ITEM: return "Item";
        case Tab::DBG_HOTRELOAD: return "Reload";
        case Tab::DBG_LIGHT: return "Light";
        case Tab::DBG_MEM: return "Mem";
        case Tab::DBG_DIALOG: return "Dlg";
        case Tab::DBG_INPUT: return "Input";
        case Tab::DBG_AUDIO: return "Audio";
        case Tab::DBG_PHYS: return "Phys";
        case Tab::DBG_SLOT: return "Slot";
        case Tab::DBG_NAV: return "Nav";
        case Tab::DBG_TIME: return "Time";
        case Tab::DBG_AGGRO: return "Aggro";
        case Tab::DBG_STATS: return "Stats";
        default: return "?";
    }
}

void DebugMenu::clampScrollOffsets() {
    size_t idx = static_cast<size_t>(currentTab);
    if (idx >= tabContents.size()) return;

    auto& content = tabContents[idx];
    float s = getScale();
    float tabY = safeTop + 8.0f * s;
    float tabH = TAB_HEIGHT * s;
    float contentY = tabY + tabH + 20.0f * s;
    float contentH = screenHeight - safeBottom - contentY;
    float btnH = BUTTON_HEIGHT * s;
    float sliderH = SLIDER_HEIGHT * s;
    float margin = BUTTON_MARGIN * s;

    // 2-column layout: calculate row count for buttons
    int buttonRows = (static_cast<int>(content.buttons.size()) + 1) / 2;
    float buttonHeight = buttonRows * (btnH + margin);

    // Add slider height
    float sliderHeight = static_cast<float>(content.sliders.size()) * (sliderH + margin);
    float totalHeight = buttonHeight + sliderHeight;

    float maxScroll = std::max(0.0f, totalHeight - contentH);
    content.scrollOffset = std::clamp(content.scrollOffset, 0.0f, maxScroll);
}

// ==================== Tab Content Creation ====================

void DebugMenu::createTabButtons() {
    for (int i = 0; i < static_cast<int>(Tab::COUNT); ++i) {
        Button btn;
        btn.label = getTabName(static_cast<Tab>(i));
        btn.baseColor = glm::vec3(0.4f, 0.5f, 0.6f);
        tabButtons.push_back(btn);
    }
}

void DebugMenu::createAllTabContents() {
    // Player tab
    {
        TabContent content;
        std::vector<std::pair<std::string, std::string>> items = {
            {"Start Game", "startgame"},
            {"Heal", "heal"},
            {"God Mode", "god"},
            {"Set HP 100", "sethealth 100"},
            {"Set MP 100", "setmana 100"},
            {"Set Stamina 100", "setstamina 100"},
            {"Set Level 50", "setlevel 50"},
            {"Add XP 1000", "addxp 1000"},
            {"Max Skills", "maxskills"},
            {"Reset Stats", "resetstats"},
            {"Set Blade 100", "setskill Blade 100"},
            {"Set Dest 100", "setskill Destruction 100"},
            {"Set Speed 100", "setattr Speed 100"},
            {"Show Stats", "stats"},
        };
        for (const auto& item : items) {
            Button btn;
            btn.label = item.first;
            btn.command = item.second;
            btn.baseColor = glm::vec3(0.3f, 0.5f, 0.7f);
            content.buttons.push_back(btn);
        }
        tabContents.push_back(content);
    }

    // Combat tab
    {
        TabContent content;
        std::vector<std::pair<std::string, std::string>> items = {
            {"Attack", "attack"},
            {"Block", "block"},
            {"Dodge", "dodge"},
            {"Kill Nearest", "kill"},
            {"Kill All", "killall"},
            {"Resurrect", "resurrect"},
            {"Damage 10", "damage 0 10"},
            {"Damage 100", "damage 0 100"},
            {"Combat Debug", "combatdebug"},
            {"Combat Stats", "combatstats"},
            {"Active Combats", "activecombats"},
            {"Attack Nearest", "attacknearest"},
            {"Set Damage x2", "setdamagemultiplier 2.0"},
            {"Set Damage x5", "setdamagemultiplier 5.0"},
            {"Invincible On", "invincible on"},
            {"Invincible Off", "invincible off"},
        };
        for (const auto& item : items) {
            Button btn;
            btn.label = item.first;
            btn.command = item.second;
            btn.baseColor = glm::vec3(0.7f, 0.3f, 0.3f);
            content.buttons.push_back(btn);
        }
        tabContents.push_back(content);
    }

    // Inventory tab
    {
        TabContent content;
        std::vector<std::pair<std::string, std::string>> items = {
            {"Add Gold x100", "additem 0 100"},
            {"Add Apple x5", "additem 1 5"},
            {"Add Health Pot", "additem 10 5"},
            {"Add Magicka Pot", "additem 11 5"},
            {"Remove Apple", "removeitem 1 1"},
            {"Clear Inv", "clearinv"},
            {"List Items", "listitems"},
            {"Max Weight", "setweight 9999"},
            {"Inventory Info", "inventoryinfo"},
            {"Item Info", "iteminfo 0"},
            {"Add Sword", "additem 100 1"},
            {"Add Shield", "additem 200 1"},
            {"Add Armor", "additem 300 1"},
            {"Carry Weight", "carryweight"},
        };
        for (const auto& item : items) {
            Button btn;
            btn.label = item.first;
            btn.command = item.second;
            btn.baseColor = glm::vec3(0.6f, 0.5f, 0.2f);
            content.buttons.push_back(btn);
        }
        tabContents.push_back(content);
    }

    // Magic tab
    {
        TabContent content;
        std::vector<std::pair<std::string, std::string>> items = {
            {"Learn Fire", "learnspell 1"},
            {"Learn Heal", "learnspell 2"},
            {"Learn Light", "learnspell 3"},
            {"Equip Fire", "equipspell 1"},
            {"Cast Fire", "castspell 1 0"},
            {"Cast Heal", "castspell 2 0"},
            {"List Spells", "listspells"},
            {"Set MP 100", "setmana 100"},
            {"Spell Info", "spellinfo 1"},
            {"Player Spells", "playerspells"},
            {"Cast at Enemy", "castspellatenemy 1"},
            {"Infinite Mana On", "infinitmana on"},
            {"Infinite Mana Off", "infinitmana off"},
            {"Spell Damage x2", "setspelldamage 2.0"},
        };
        for (const auto& item : items) {
            Button btn;
            btn.label = item.first;
            btn.command = item.second;
            btn.baseColor = glm::vec3(0.5f, 0.2f, 0.7f);
            content.buttons.push_back(btn);
        }
        tabContents.push_back(content);
    }

    // Quest tab
    {
        TabContent content;
        std::vector<std::pair<std::string, std::string>> items = {
            {"List Quests", "listquests"},
            {"Active Quests", "activequests"},
            {"Quest Details", "questdetails 1"},
            {"Accept Main", "acceptquest 1"},
            {"Accept Side", "acceptquest 2"},
            {"Complete Q1", "completequest 1"},
            {"Fail Q1", "failquest 1"},
            {"Update Obj", "updateobj 1 1 5"},
            {"Reset Quest", "resetquest 1"},
            {"Quest Reward", "questreward 1"},
            {"Complete Obj", "completeobjectives 1"},
        };
        for (const auto& item : items) {
            Button btn;
            btn.label = item.first;
            btn.command = item.second;
            btn.baseColor = glm::vec3(0.7f, 0.5f, 0.2f);
            content.buttons.push_back(btn);
        }
        tabContents.push_back(content);
    }

    // NPC tab
    {
        TabContent content;
        std::vector<std::pair<std::string, std::string>> items = {
            {"List NPCs", "listnpcs"},
            {"NPC Count", "npccount"},
            {"NPC Info", "npcinfo 0"},
            {"Nearby", "nearby"},
            {"Spawn Guard", "spawnat Guard 0 0 0"},
            {"Spawn Mage", "spawnat Mage 5 0 5"},
            {"Spawn Bandit", "spawnat Bandit -5 0 5"},
            {"Spawn at Player", "spawnplayer Guard"},
            {"Kill All NPCs", "killallnpcs"},
            {"Aggro NPC", "aggro 0"},
            {"Calm NPC", "calm 0"},
            {"Set AI Combat", "setai 0 combat"},
            {"Set AI Idle", "setai 0 idle"},
            {"Resurrect", "resurrectnpc 0"},
            {"Set Speed", "setnpcspeed 100"},
        };
        for (const auto& item : items) {
            Button btn;
            btn.label = item.first;
            btn.command = item.second;
            btn.baseColor = glm::vec3(0.4f, 0.5f, 0.3f);
            content.buttons.push_back(btn);
        }
        tabContents.push_back(content);
    }

    // Dialogue tab
    {
        TabContent content;
        std::vector<std::pair<std::string, std::string>> items = {
            {"Talk NPC", "talk 0"},
            {"Dialogue State", "dialoguestate"},
            {"Topics", "dialoguetopics"},
            {"Choices", "dialoguechoices"},
            {"History", "dialoguehistory"},
            {"Topic 0", "selecttopic 0"},
            {"Topic 1", "selecttopic 1"},
            {"Topic 2", "selecttopic 2"},
            {"Choice 0", "selectchoice 0"},
            {"Choice 1", "selectchoice 1"},
            {"End Talk", "endtalk"},
            {"Reset Dialogue", "resetdialogue"},
        };
        for (const auto& item : items) {
            Button btn;
            btn.label = item.first;
            btn.command = item.second;
            btn.baseColor = glm::vec3(0.6f, 0.4f, 0.6f);
            content.buttons.push_back(btn);
        }
        tabContents.push_back(content);
    }

    // World tab
    {
        TabContent content;
        std::vector<std::pair<std::string, std::string>> items = {
            {"Weather Clear", "setweather clear"},
            {"Weather Rain", "setweather rain"},
            {"Weather Snow", "setweather snow"},
            {"Weather Fog", "setweather fog"},
            {"Weather Storm", "setweather storm"},
            {"Time Dawn (6)", "settime 6"},
            {"Time Noon (12)", "settime 12"},
            {"Time Dusk (18)", "settime 18"},
            {"Time Midnight", "settime 0"},
            {"Time x30", "settimescale 30"},
            {"Time x1", "settimescale 1"},
            {"Time x0 (Pause)", "settimescale 0"},
            {"Load Cell 0,0", "loadcell 0 0"},
            {"World Info", "worldinfo"},
            {"World Detail", "worldinfodetail"},
            // Phase 66: Map debug
            {"Player Position", "playerpos"},
            {"Nearby Cells", "nearbycells"},
            {"Active Cells", "activecells"},
            {"Cell Details", "celldetails 0 0"},
            {"World Items", "worlditems"},
            {"Door Info", "doorinfo"},
            {"Teleport 0,0", "teleportcell 0 0"},
            {"Teleport 1,0", "teleportcell 1 0"},
            {"Teleport 0,1", "teleportcell 0 1"},
            {"Teleport 1,1", "teleportcell 1 1"},
            {"Move North +512", "moverel 0 -512"},
            {"Move South +512", "moverel 0 512"},
            {"Move East +512", "moverel 512 0"},
            {"Move West +512", "moverel -512 0"},
            // Viewer toggle
            {"World Viewer", "worldviewer"},
        };
        for (const auto& item : items) {
            Button btn;
            btn.label = item.first;
            btn.command = item.second;
            btn.baseColor = glm::vec3(0.3f, 0.6f, 0.6f);
            content.buttons.push_back(btn);
        }
        tabContents.push_back(content);
    }

    // Save tab
    {
        TabContent content;
        std::vector<std::pair<std::string, std::string>> items = {
            {"Quick Save", "quicksave"},
            {"Quick Load", "quickload"},
            {"Save Slot 0", "save 0"},
            {"Save Slot 1", "save 1"},
            {"Save Slot 2", "save 2"},
            {"Load Slot 0", "load 0"},
            {"Load Slot 1", "load 1"},
            {"Load Slot 2", "load 2"},
            {"List Saves", "listsaves"},
        };
        for (const auto& item : items) {
            Button btn;
            btn.label = item.first;
            btn.command = item.second;
            btn.baseColor = glm::vec3(0.5f, 0.5f, 0.3f);
            content.buttons.push_back(btn);
        }
        tabContents.push_back(content);
    }

    // System tab
    {
        TabContent content;
        std::vector<std::pair<std::string, std::string>> items = {
            {"Toggle Wireframe", "wireframe"},
            {"Toggle AABB", "aabb"},
            {"NPC Overlay", "npcoverlay"},
            {"Touch Trail", "touchtrail"},
            {"Debug HUD+", "debughudnext"},
            {"Debug HUD-", "debughudprev"},
            {"Debug Log", "debuglog"},
            {"FPS Stats", "fpsstats"},
            {"Memory Stats", "memorystats"},
            {"Performance", "performance"},
            {"Reset Stats", "resetstats"},
            {"Dump State", "dumpstate"},
            {"Font: Roboto", "font_roboto"},
            {"Font: Daedric", "font_daedric"},
            {"Font: Kingthings", "font_kingthings"},
            {"Font: Handwritten", "font_handwritten"},
            {"Font: Tahoma", "font_tahoma"},
        };
        for (const auto& item : items) {
            Button btn;
            btn.label = item.first;
            btn.command = item.second;
            btn.baseColor = glm::vec3(0.4f, 0.4f, 0.5f);
            content.buttons.push_back(btn);
        }

        // FOV slider
        Slider fovSlider;
        fovSlider.label = "FOV";
        fovSlider.min = 45.0f;
        fovSlider.max = 120.0f;
        fovSlider.value = 75.0f;
        fovSlider.step = 1.0f;
        fovSlider.format = "%.0f";
        fovSlider.onChange = [this](float v) {
            if (console) {
                char cmd[64];
                snprintf(cmd, sizeof(cmd), "setfov %.0f", v);
                console->executeCommand(cmd);
            }
        };
        content.sliders.push_back(fovSlider);

        // FPS cap slider
        Slider fpsSlider;
        fpsSlider.label = "FPS Cap";
        fpsSlider.min = 15.0f;
        fpsSlider.max = 120.0f;
        fpsSlider.value = 60.0f;
        fpsSlider.step = 5.0f;
        fpsSlider.format = "%.0f";
        fpsSlider.onChange = [this](float v) {
            if (console) {
                char cmd[64];
                snprintf(cmd, sizeof(cmd), "setfpscap %.0f", v);
                console->executeCommand(cmd);
            }
        };
        content.sliders.push_back(fpsSlider);

        // Render distance slider
        Slider renderDistSlider;
        renderDistSlider.label = "Render Dist";
        renderDistSlider.min = 100.0f;
        renderDistSlider.max = 5000.0f;
        renderDistSlider.value = 1000.0f;
        renderDistSlider.step = 100.0f;
        renderDistSlider.format = "%.0f";
        renderDistSlider.onChange = [this](float v) {
            if (console) {
                char cmd[64];
                snprintf(cmd, sizeof(cmd), "setrenderdist %.0f", v);
                console->executeCommand(cmd);
            }
        };
        content.sliders.push_back(renderDistSlider);

        tabContents.push_back(content);
    }

    // Sound tab
    {
        TabContent content;
        std::vector<std::pair<std::string, std::string>> items = {
            {"Play BGM", "playbgm"},
            {"Stop BGM", "stopbgm"},
            {"Play SE", "playse"},
            {"Stop All SE", "stopallse"},
            {"Mute All", "mute"},
            {"Unmute All", "unmute"},
            {"List Audio", "listaudio"},
            {"Audio Stats", "audiostats"},
            // Phase 66: BGM browsing
            {"List BGM Tracks", "listbgm"},
            {"BGM Info", "bgminfo"},
        };
        for (const auto& item : items) {
            Button btn;
            btn.label = item.first;
            btn.command = item.second;
            btn.baseColor = glm::vec3(0.6f, 0.3f, 0.6f);
            content.buttons.push_back(btn);
        }

        // Master volume slider
        Slider masterVol;
        masterVol.label = "Master Vol";
        masterVol.min = 0.0f;
        masterVol.max = 1.0f;
        masterVol.value = 0.8f;
        masterVol.step = 0.05f;
        masterVol.format = "%.0f%%";
        masterVol.onChange = [this](float v) {
            if (console) {
                char cmd[64];
                snprintf(cmd, sizeof(cmd), "setvolume %.2f", v);
                console->executeCommand(cmd);
            }
        };
        content.sliders.push_back(masterVol);

        // BGM volume slider
        Slider bgmVol;
        bgmVol.label = "BGM Vol";
        bgmVol.min = 0.0f;
        bgmVol.max = 1.0f;
        bgmVol.value = 0.7f;
        bgmVol.step = 0.05f;
        bgmVol.format = "%.0f%%";
        bgmVol.onChange = [this](float v) {
            if (console) {
                char cmd[64];
                snprintf(cmd, sizeof(cmd), "bgmvolume %.2f", v);
                console->executeCommand(cmd);
            }
        };
        content.sliders.push_back(bgmVol);

        // SFX volume slider
        Slider sfxVol;
        sfxVol.label = "SFX Vol";
        sfxVol.min = 0.0f;
        sfxVol.max = 1.0f;
        sfxVol.value = 1.0f;
        sfxVol.step = 0.05f;
        sfxVol.format = "%.0f%%";
        sfxVol.onChange = [this](float v) {
            if (console) {
                char cmd[64];
                snprintf(cmd, sizeof(cmd), "sfxvolume %.2f", v);
                console->executeCommand(cmd);
            }
        };
        content.sliders.push_back(sfxVol);

        tabContents.push_back(content);
    }

    // Assets tab
    {
        TabContent content;
        std::vector<std::pair<std::string, std::string>> items = {
            {"List Textures", "listtextures"},
            {"List Models", "listmodels"},
            {"List Audio", "listaudio"},
            {"Texture Info", "textureinfo"},
            {"Model Info", "modelinfo"},
            {"Cache Stats", "cachestats"},
            {"Clear Cache", "clearcache"},
            {"Reload Assets", "reloadassets"},
            {"Memory Usage", "memoryusage"},
            {"Asset Stats", "assetstats"},
            // Phase 66: Texture browsing
            {"Textures Detail", "texturesdetail"},
            // Viewer toggles
            {"Texture Viewer", "textureviewer"},
            {"Model Viewer", "modelviewer"},
            {"3D Viewer", "viewer3d"},
        };
        for (const auto& item : items) {
            Button btn;
            btn.label = item.first;
            btn.command = item.second;
            btn.baseColor = glm::vec3(0.3f, 0.6f, 0.5f);
            content.buttons.push_back(btn);
        }
        tabContents.push_back(content);
    }

    // Logs tab
    {
        TabContent content;
        std::vector<std::pair<std::string, std::string>> items = {
            {"Show All Logs", "loglevel all"},
            {"Show Debug", "loglevel debug"},
            {"Show Info", "loglevel info"},
            {"Show Warning", "loglevel warn"},
            {"Show Error", "loglevel error"},
            {"Clear Logs", "clearlogs"},
            {"Export Logs", "exportlogs"},
            {"Log Stats", "logstats"},
            {"Search Logs", "searchlog"},
            {"Toggle Auto-scroll", "logautoscroll"},
        };
        for (const auto& item : items) {
            Button btn;
            btn.label = item.first;
            btn.command = item.second;
            btn.baseColor = glm::vec3(0.5f, 0.4f, 0.3f);
            content.buttons.push_back(btn);
        }
        tabContents.push_back(content);
    }

    // ==================== Phase 64: Debug Tool Tabs ====================

    // DBG_PICKER: Picker + Teleport
    {
        TabContent content;
        std::vector<std::pair<std::string, std::string>> items = {
            {"Pick Mode", "pick"},
            {"Teleport", "teleport"},
            {"Follow NPC", "pickfollow"},
            {"Info", "pickinfo"},
        };
        for (const auto& item : items) {
            Button btn;
            btn.label = item.first;
            btn.command = item.second;
            btn.baseColor = glm::vec3(0.2f, 0.8f, 0.9f);
            content.buttons.push_back(btn);
        }
        tabContents.push_back(content);
    }

    // DBG_AI: AI State Viewer
    {
        TabContent content;
        std::vector<std::pair<std::string, std::string>> items = {
            {"Show AI", "aishow"},
            {"Step NPC", "aistep"},
            {"List States", "ailist"},
            {"Reset AI", "aireset"},
        };
        for (const auto& item : items) {
            Button btn;
            btn.label = item.first;
            btn.command = item.second;
            btn.baseColor = glm::vec3(0.9f, 0.6f, 0.2f);
            content.buttons.push_back(btn);
        }
        tabContents.push_back(content);
    }

    // DBG_ANIM: Animation Scrubber
    {
        TabContent content;
        std::vector<std::pair<std::string, std::string>> items = {
            {"Show Anim", "animshow"},
            {"Play/Pause", "animplay"},
            {"Step +1", "animstep 1"},
            {"Step -1", "animstep -1"},
            {"Reset", "animreset"},
        };
        for (const auto& item : items) {
            Button btn;
            btn.label = item.first;
            btn.command = item.second;
            btn.baseColor = glm::vec3(0.9f, 0.3f, 0.5f);
            content.buttons.push_back(btn);
        }
        tabContents.push_back(content);
    }

    // DBG_SCRIPT: Script Debugger
    {
        TabContent content;
        std::vector<std::pair<std::string, std::string>> items = {
            {"Show Script", "scriptshow"},
            {"Step VM", "scriptstep"},
            {"Break", "scriptbreak"},
            {"Resume", "scriptresume"},
            {"Run File", "scriptrun"},
        };
        for (const auto& item : items) {
            Button btn;
            btn.label = item.first;
            btn.command = item.second;
            btn.baseColor = glm::vec3(0.6f, 0.4f, 0.9f);
            content.buttons.push_back(btn);
        }
        tabContents.push_back(content);
    }

    // DBG_QUEST: Quest Debugger
    {
        TabContent content;
        std::vector<std::pair<std::string, std::string>> items = {
            {"Show Quests", "questshow"},
            {"Complete", "questcomplete"},
            {"Fail", "questfail"},
            {"Set Stage", "queststage"},
            {"Flags", "questflags"},
        };
        for (const auto& item : items) {
            Button btn;
            btn.label = item.first;
            btn.command = item.second;
            btn.baseColor = glm::vec3(0.9f, 0.8f, 0.2f);
            content.buttons.push_back(btn);
        }
        tabContents.push_back(content);
    }

    // DBG_ITEM: Item Editor
    {
        TabContent content;
        std::vector<std::pair<std::string, std::string>> items = {
            {"Show Items", "itemshow"},
            {"Edit Item", "itemedit"},
            {"Add Item", "itemadd"},
            {"Remove", "itemremove"},
        };
        for (const auto& item : items) {
            Button btn;
            btn.label = item.first;
            btn.command = item.second;
            btn.baseColor = glm::vec3(0.8f, 0.5f, 0.2f);
            content.buttons.push_back(btn);
        }
        tabContents.push_back(content);
    }

    // DBG_HOTRELOAD: Hot Reload
    {
        TabContent content;
        std::vector<std::pair<std::string, std::string>> items = {
            {"Reload Assets", "hotreload"},
            {"Watch On", "hotwatchon"},
            {"Watch Off", "hotwatchoff"},
            {"Reload Log", "hotreloadlog"},
        };
        for (const auto& item : items) {
            Button btn;
            btn.label = item.first;
            btn.command = item.second;
            btn.baseColor = glm::vec3(0.3f, 0.9f, 0.3f);
            content.buttons.push_back(btn);
        }
        tabContents.push_back(content);
    }

    // DBG_LIGHT: Lighting Control
    {
        TabContent content;
        std::vector<std::pair<std::string, std::string>> items = {
            {"Show Light", "lightshow"},
            {"Reset", "lightreset"},
            {"Ambient", "lightambient"},
            {"Shadow", "lightshadow"},
        };
        for (const auto& item : items) {
            Button btn;
            btn.label = item.first;
            btn.command = item.second;
            btn.baseColor = glm::vec3(1.f, 0.9f, 0.4f);
            content.buttons.push_back(btn);
        }
        tabContents.push_back(content);
    }

    // DBG_MEM: Memory Trace
    {
        TabContent content;
        std::vector<std::pair<std::string, std::string>> items = {
            {"Show Mem", "memshow"},
            {"Dump", "memdump"},
            {"Diff", "memdiff"},
            {"Reset", "memreset"},
        };
        for (const auto& item : items) {
            Button btn;
            btn.label = item.first;
            btn.command = item.second;
            btn.baseColor = glm::vec3(0.5f, 0.2f, 0.5f);
            content.buttons.push_back(btn);
        }
        tabContents.push_back(content);
    }

    // DBG_DIALOG: Dialogue Debugger
    {
        TabContent content;
        std::vector<std::pair<std::string, std::string>> items = {
            {"Show Dialog", "dialogshow"},
            {"Topics", "dialogtopics"},
            {"Choices", "dialogchoices"},
            {"History", "dialoghistory"},
            {"Reset", "dialogreset"},
        };
        for (const auto& item : items) {
            Button btn;
            btn.label = item.first;
            btn.command = item.second;
            btn.baseColor = glm::vec3(0.7f, 0.3f, 0.7f);
            content.buttons.push_back(btn);
        }
        tabContents.push_back(content);
    }

    // DBG_INPUT: Input Visualizer
    {
        TabContent content;
        std::vector<std::pair<std::string, std::string>> items = {
            {"Show Input", "inputshow"},
            {"Touch Trail", "inputtrail"},
            {"Pad Map", "inputpad"},
            {"Clear", "inputclear"},
        };
        for (const auto& item : items) {
            Button btn;
            btn.label = item.first;
            btn.command = item.second;
            btn.baseColor = glm::vec3(0.3f, 0.6f, 0.9f);
            content.buttons.push_back(btn);
        }
        tabContents.push_back(content);
    }

    // DBG_AUDIO: Audio Browser
    {
        TabContent content;
        std::vector<std::pair<std::string, std::string>> items = {
            {"Browse", "audiobrowse"},
            {"Play", "audioplay"},
            {"Stop", "audiostop"},
            {"Waveform", "audiowave"},
        };
        for (const auto& item : items) {
            Button btn;
            btn.label = item.first;
            btn.command = item.second;
            btn.baseColor = glm::vec3(0.6f, 0.3f, 0.8f);
            content.buttons.push_back(btn);
        }
        tabContents.push_back(content);
    }

    // DBG_PHYS: Physics Tool
    {
        TabContent content;
        std::vector<std::pair<std::string, std::string>> items = {
            {"Show Phys", "physshow"},
            {"Apply Force", "physforce"},
            {"Impulse", "physimpulse"},
            {"Reset", "physreset"},
        };
        for (const auto& item : items) {
            Button btn;
            btn.label = item.first;
            btn.command = item.second;
            btn.baseColor = glm::vec3(0.2f, 0.7f, 0.5f);
            content.buttons.push_back(btn);
        }
        tabContents.push_back(content);
    }

    // DBG_SLOT: Save Slot Viewer
    {
        TabContent content;
        std::vector<std::pair<std::string, std::string>> items = {
            {"Show Slots", "slotshow"},
            {"Compare", "slotcompare"},
            {"Delete Slot", "slotdelete"},
        };
        for (const auto& item : items) {
            Button btn;
            btn.label = item.first;
            btn.command = item.second;
            btn.baseColor = glm::vec3(0.5f, 0.5f, 0.4f);
            content.buttons.push_back(btn);
        }
        tabContents.push_back(content);
    }

    // DBG_NAV: Navmesh Viewer
    {
        TabContent content;
        std::vector<std::pair<std::string, std::string>> items = {
            {"Show Nav", "navshow"},
            {"Path", "navpath"},
            {"Zones", "navzones"},
            {"Portals", "navportals"},
        };
        for (const auto& item : items) {
            Button btn;
            btn.label = item.first;
            btn.command = item.second;
            btn.baseColor = glm::vec3(0.3f, 0.8f, 0.4f);
            content.buttons.push_back(btn);
        }
        tabContents.push_back(content);
    }

    // DBG_TIME: Time & Weather Control
    {
        TabContent content;
        std::vector<std::pair<std::string, std::string>> items = {
            {"Show Time", "timeshow"},
            {"Dawn", "timeset 6"},
            {"Noon", "timeset 12"},
            {"Dusk", "timeset 18"},
            {"Midnight", "timeset 0"},
        };
        for (const auto& item : items) {
            Button btn;
            btn.label = item.first;
            btn.command = item.second;
            btn.baseColor = glm::vec3(0.9f, 0.7f, 0.2f);
            content.buttons.push_back(btn);
        }
        tabContents.push_back(content);
    }

    // DBG_AGGRO: Aggro HUD
    {
        TabContent content;
        std::vector<std::pair<std::string, std::string>> items = {
            {"Show Aggro", "aggroshow"},
            {"Set Target", "aggrotarget"},
            {"Clear", "aggroclear"},
            {"Enemy List", "aggroenemies"},
        };
        for (const auto& item : items) {
            Button btn;
            btn.label = item.first;
            btn.command = item.second;
            btn.baseColor = glm::vec3(0.9f, 0.2f, 0.2f);
            content.buttons.push_back(btn);
        }
        tabContents.push_back(content);
    }

    // DBG_STATS: Player Stats Panel
    {
        TabContent content;
        std::vector<std::pair<std::string, std::string>> items = {
            {"Show Stats", "statsshow"},
            {"Attributes", "statsattr"},
            {"Skills", "statsskills"},
            {"Modifiers", "statsmods"},
        };
        for (const auto& item : items) {
            Button btn;
            btn.label = item.first;
            btn.command = item.second;
            btn.baseColor = glm::vec3(0.4f, 0.6f, 0.8f);
            content.buttons.push_back(btn);
        }
        tabContents.push_back(content);
    }
}