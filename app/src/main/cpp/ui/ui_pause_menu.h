#pragma once

#include "ui_panel.h"
#include "text_renderer.h"
#include <memory>
#include <functional>

/**
 * @brief ゲーム一時停止メニューUI
 *
 * Phase 13: ゲーム中に一時停止して、各機能にアクセスするメニュー
 * キャラクター表示、スペルブック、クエスト、
 * セーブ/ロード、設定、ゲーム再開などのオプション
 */
class UIPauseMenu : public UIPanel {
public:
    enum MenuItem {
        CHARACTER_SHEET = 0,
        SPELLBOOK = 1,
        QUEST_LOG = 2,
        INVENTORY = 3,
        SETTINGS = 4,
        SAVE_GAME = 5,
        LOAD_GAME = 6,
        RESUME = 7,
        QUIT_TO_TITLE = 8,
        MENU_COUNT = 9
    };

    using MenuCallback = std::function<void(MenuItem)>;

    explicit UIPauseMenu(const std::string& title = "Pause");
    ~UIPauseMenu() override = default;

    bool initialize(TextRenderer* textRenderer);

    void open();
    void close();

    void update(float deltaTime) override;
    bool onTouchDown(float x, float y, int pointerId) override;
    void render() override;

    void setScreenSize(int w, int h) { screenWidth = w; screenHeight = h; }

    // Callback when menu item is selected
    void setOnMenuSelected(MenuCallback cb) { onMenuSelected = cb; }

private:
    TextRenderer* textRenderer = nullptr;
    MenuCallback onMenuSelected;

    int selectedItemIndex = 0;
    int scrollOffset = 0;

    int screenWidth  = 1080;
    int screenHeight = 1920;

    static constexpr float MENU_ITEM_H = 48.0f;
    static constexpr float MENU_ITEM_GAP = 3.0f;
    static constexpr int MAX_VISIBLE_ITEMS = 8;

    static constexpr std::array<const char*, MENU_COUNT> MENU_LABELS = {{
        "Character Sheet",
        "Spellbook",
        "Quest Log",
        "Inventory",
        "Settings",
        "Save Game",
        "Load Game",
        "Resume Game",
        "Quit to Title"
    }};

    static constexpr std::array<const char*, MENU_COUNT> MENU_DESCRIPTIONS = {{
        "View and manage your character",
        "View and manage your spells",
        "View your active and completed quests",
        "Access your inventory and equipment",
        "Adjust game settings",
        "Save your current progress",
        "Load a previously saved game",
        "Resume playing",
        "Return to the title screen"
    }};

    void renderMenuItems();
    void renderSelectedDescription();
    void renderBackgroundOverlay();

    int hitTestMenuItem(float x, float y) const;

    // Layout helpers
    float getMenuStartY() const;
    float getDescriptionY() const;
};
