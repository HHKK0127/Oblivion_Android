#pragma once

#include <vector>
#include <string>
#include <functional>
#include <glm/glm.hpp>

class TextRenderer;
class GameConsole;

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
 */
class DebugMenu {
public:
    DebugMenu();
    ~DebugMenu();

    bool initialize(TextRenderer* textRenderer, GameConsole* console);
    void cleanup();

    void toggle();
    bool isVisible() const { return visible; }
    void setVisible(bool v) { visible = v; }

    void onTouchEvent(float x, float y, int action);
    void update(float deltaTime);
    void render();

    void setScreenSize(int w, int h) { screenWidth = w; screenHeight = h; }

private:
    TextRenderer* textRenderer;
    GameConsole* console;
    bool visible;
    bool initialized;

    int screenWidth;
    int screenHeight;

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
        COUNT
    };

    Tab currentTab;
    float tabScrollOffset;

    // Button definition
    struct Button {
        float x, y, w, h;
        std::string label;
        std::string command;  // Console command to execute
        glm::vec3 color;
        bool pressed;
    };

    // Tab buttons
    std::vector<Button> tabButtons;

    // Content buttons per tab
    struct TabContent {
        std::vector<Button> buttons;
        float scrollOffset;
    };
    std::vector<TabContent> tabContents;

    // UI constants
    static constexpr float TAB_HEIGHT = 50.0f;
    static constexpr float BUTTON_HEIGHT = 45.0f;
    static constexpr float BUTTON_MARGIN = 5.0f;
    static constexpr float CONTENT_PADDING = 10.0f;

    // Helper methods
    void createTabButtons();
    void createPlayerButtons();
    void createCombatButtons();
    void createInventoryButtons();
    void createMagicButtons();
    void createQuestButtons();
    void createNpcButtons();
    void createDialogueButtons();
    void createWorldButtons();
    void createSaveButtons();

    void renderTabBar();
    void renderContent();
    void renderButton(const Button& btn, float y);

    bool hitTest(const Button& btn, float x, float y) const;
    void handleTabTap(float x, float y);
    void handleContentTap(float x, float y);

    float getScale() const;
    std::string getTabName(Tab tab) const;
};
