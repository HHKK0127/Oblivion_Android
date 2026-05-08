#pragma once

#include <string>
#include <cstdint>
#include <vector>

class TextRenderer;
class SettingsManager;
class Renderer;

class SettingsUI {
public:
    SettingsUI();
    ~SettingsUI();

    bool initialize(TextRenderer* textRenderer, SettingsManager* settingsManager, Renderer* renderer = nullptr);
    void cleanup();

    void update(float deltaTime);
    void render();

    void toggle();
    bool isVisible() const { return visible; }
    void show();
    void hide();

    void onTouchEvent(float x, float y);
    void onKeyEvent(int keyCode, int action);

    bool shouldReturnToMenu() const { return returnToMenu; }
    void resetReturnFlag();

private:
    TextRenderer* textRenderer;
    SettingsManager* settingsManager;
    Renderer* renderer;
    bool visible;
    bool returnToMenu;
    int selectedIndex;
    float scrollOffset;
};