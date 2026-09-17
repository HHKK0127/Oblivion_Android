#pragma once

#include <vector>
#include <string>
#include <functional>
#include <glm/glm.hpp>

class TextRenderer;
class GameConsole;
class TextureViewer;
class ModelViewer;
class WorldViewer;
class Viewer3D;

/**
 * @brief Debug Menu - Touch-based GUI for game system testing
 *
 * Provides categorized buttons for all game operations:
 * - Player: stats, skills, attributes, level
 * - Combat: attack, block, dodge, damage
 * - Inventory: add/remove/equip items
 * - Magic: spells, mana
 * - Quest: accept/complete/fail
 * - NPC: spawn, AI, aggression
 * - Dialogue: start, select topics
 * - World: weather, time, cells
 * - Save/Load: save, load, quicksave
 *
 * Enhancements:
 * - Slider UI for continuous value parameters
 * - Keyboard shortcuts (F1, arrow keys, Tab, Enter)
 * - State dump to file for debugging
 * - Minimum 48dp touch target size
 */
class DebugMenu {
public:
    DebugMenu();
    ~DebugMenu();

    bool initialize(TextRenderer* textRenderer, GameConsole* console);
    void cleanup();

    // Viewer access
    TextureViewer* getTextureViewer() { return textureViewer; }
    ModelViewer* getModelViewer() { return modelViewer; }
    WorldViewer* getWorldViewer() { return worldViewer; }
    Viewer3D* getViewer3D() { return viewer3D; }

    void toggle();
    bool isVisible() const { return visible; }
    void setVisible(bool v) { visible = v; }
    void selectTab(int tabIndex);

    // Touch event handling (separate DOWN/MOVE/UP for proper tap vs scroll detection)
    void onTouchDown(float x, float y);
    void onTouchMove(float x, float y);
    void onTouchUp(float x, float y);
    void onTouchCancel();

    // Keyboard event handling
    void onKeyDown(int32_t keyCode);
    void onKeyUp(int32_t keyCode);

    // Slider support
    struct Slider {
        float x, y, w, h;
        float value;
        float min, max;
        float step;
        bool dragging;
        std::string label;
        std::function<void(float)> onChange;
        std::string format;  // printf format for display (e.g., "%.1f")
        Slider() : x(0), y(0), w(0), h(0), value(0), min(0), max(1), step(0.1f),
                   dragging(false), format("%.1f") {}
    };

    void addSlider(const std::string& label, float min, float max, float value,
                   float step, std::function<void(float)> onChange,
                   const std::string& format = "%.1f");
    void updateSliderValue(const std::string& label, float value);

    void update(float deltaTime);
    void render();

    void setScreenSize(int w, int h);
    void setOnStartGame(std::function<void()> callback) { onStartGame = std::move(callback); }
    void setDensity(float density) { screenDensity = density; }

    /** Connect a WorldManager to the WorldViewer (deferred injection) */
    void setWorldManager(class WorldManager* worldManager);

    // State dump
    void dumpState();

private:
    TextRenderer* textRenderer;
    GameConsole* console;
    std::function<void()> onStartGame;
    bool visible;
    bool initialized;

    int screenWidth;
    int screenHeight;
    float screenDensity = 2.0f;  // Default xxhdpi

    // Safe area insets
    float safeLeft, safeTop, safeRight, safeBottom;

    // Tab system
    enum class Tab {
        PLAYER,
        COMBAT,
        INVENTORY,
        MAGIC,
        QUEST,
        NPC,
        DIALOGUE,
        WORLD,
        SAVE,
        SYSTEM,
        SOUND,
        ASSETS,
        LOGS,
        // Phase 64: Debug Tool tabs
        DBG_PICKER,
        DBG_AI,
        DBG_ANIM,
        DBG_SCRIPT,
        DBG_QUEST,
        DBG_ITEM,
        DBG_HOTRELOAD,
        DBG_LIGHT,
        DBG_MEM,
        DBG_DIALOG,
        DBG_INPUT,
        DBG_AUDIO,
        DBG_PHYS,
        DBG_SLOT,
        DBG_NAV,
        DBG_TIME,
        DBG_AGGRO,
        DBG_STATS,
        COUNT
    };

    Tab currentTab;

    // Button definition
    struct Button {
        float x, y, w, h;
        std::string label;
        std::string command;  // Console command to execute
        glm::vec3 baseColor;
        bool isPressed;
        float pressTimer;
        Button() : x(0), y(0), w(0), h(0), isPressed(false), pressTimer(0.0f) {}
    };

    // Tab buttons
    std::vector<Button> tabButtons;

    // Content buttons per tab
    struct TabContent {
        std::vector<Button> buttons;
        std::vector<Slider> sliders;
        float scrollOffset = 0.0f;
    };
    std::vector<TabContent> tabContents;

    // Touch state for proper tap vs scroll detection
    struct TouchState {
        bool isActive = false;
        float startX = 0.0f, startY = 0.0f;
        float lastX = 0.0f, lastY = 0.0f;
        bool isScrolling = false;
        static constexpr float TAP_THRESHOLD = 15.0f;
        static constexpr float SCROLL_THRESHOLD = 10.0f;
        Button* pressedButton = nullptr;
        Slider* pressedSlider = nullptr;
    } touchState;

    // Keyboard selection state
    struct KeyState {
        int selectedTabIndex = 0;
        int selectedItemIndex = -1;  // -1 = tab bar, 0+ = content items
        bool onSlider = false;
    } keyState;

    // Command feedback
    std::string feedbackText;
    float feedbackTimer;
    glm::vec3 feedbackColor;

    // Viewers
    TextureViewer* textureViewer;
    ModelViewer* modelViewer;
    WorldViewer* worldViewer;
    Viewer3D* viewer3D;

    // UI constants
    static constexpr float TAB_HEIGHT = 56.0f;
    static constexpr float BUTTON_HEIGHT = 52.0f;
    static constexpr float SLIDER_HEIGHT = 48.0f;
    static constexpr float BUTTON_MARGIN = 8.0f;
    static constexpr float MIN_TOUCH_DP = 48.0f;

    // Helper methods
    void createTabButtons();
    void createAllTabContents();

    void calculateButtonPositions();
    Button* hitTestTab(float x, float y);
    Button* hitTestContent(float x, float y);
    Slider* hitTestSlider(float x, float y);
    void executeButtonCommand(Button& btn);
    int findTabIndex(const Button* btn) const;

    void renderBackground();
    void renderTabBar();
    void renderContent();
    void renderButton(Button& btn, float scale);
    void renderSlider(Slider& slider, float scale);

    // Keyboard helpers
    void moveSelection(int delta);
    void activateSelected();
    void nudgeSlider(int direction);
    void switchTab(int direction);
    bool isOnSlider() const;

    // Touch size enforcement
    void ensureMinTouchSize(Button& btn);
    void ensureMinTouchSize(Slider& slider);
    float getMinTouchPx() const;

    float getScale() const;
    std::string getTabName(Tab tab) const;
    void clampScrollOffsets();
};
