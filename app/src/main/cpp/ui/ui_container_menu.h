#pragma once

#include <memory>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <GLES3/gl3.h>
#include <unordered_map>

class TextRenderer;
class Container;

struct Player;

class UIContainerMenu {
public:
    UIContainerMenu();
    ~UIContainerMenu();

    bool initialize(TextRenderer* renderer);
    void cleanup();

    void open(std::shared_ptr<Container> container, Player* player);
    void close();
    bool isVisible() const { return visible_; }

    void setScreenSize(int width, int height);

    void update(float deltaTime);
    void render();

    void onTouchEvent(float x, float y);
    bool onKeyPress(int keyCode);
    bool handleBack();

    void takeItem(size_t containerIndex);
    void depositItem(size_t playerIndex);
    void takeAll();
    void setFadeAlpha(float alpha);

    static int itemTypeToCategory(int typeInt);
    static const char* categoryLabel(int category);
    static int categoryForTab(int tab);

    Player* getPlayer() const { return player_; }
    std::shared_ptr<Container> getContainer() const { return container_; }

private:
    enum class Pane {
        CONTAINER,
        PLAYER
    };

    static constexpr int kCanvasWidth = 1280;
    static constexpr int kCanvasHeight = 960;
    static constexpr int kCategoryCount = 10;

    static constexpr int kContainerPaneX = 10;
    static constexpr int kContainerPaneY = 150;
    static constexpr int kContainerPaneW = 620;
    static constexpr int kContainerPaneH = 450;

    static constexpr int kPlayerPaneX = 650;
    static constexpr int kPlayerPaneY = 150;
    static constexpr int kPlayerPaneW = 620;
    static constexpr int kPlayerPaneH = 450;

    static constexpr int kRowHeight = 81;
    static constexpr int kVisibleRows = 5;
    static constexpr int kItemIconX = 5;
    static constexpr int kItemIconY = 7;
    static constexpr int kItemIconSize = 64;
    static constexpr int kItemNameX = 80;
    static constexpr int kItemNameWrap = 280;
    static constexpr int kItemValueX = 418;
    static constexpr int kItemValueWrap = 90;
    static constexpr int kItemWeightX = 496;
    static constexpr int kItemWeightWrap = 64;
    static constexpr int kItemDamageX = 556;
    static constexpr int kItemDamageWrap = 70;
    static constexpr int kItemHealthX = 616;
    static constexpr int kItemHealthWrap = 50;

    static constexpr int kTabsY = 605;
    static constexpr int kTabHeight = 40;
    static constexpr float kTabWidths[5] = {120.0f, 120.0f, 120.0f, 120.0f, 120.0f};
    static constexpr float kTabOffsets[5] = {15.0f, 145.0f, 275.0f, 405.0f, 535.0f};

    static constexpr int kButtonsY = 660;
    static constexpr int kButtonHeight = 40;
    static constexpr int kButtonTakeAllX = 100;
    static constexpr int kButtonHaggleX = 400;
    static constexpr int kButtonExitX = 700;
    static constexpr int kButtonWidth = 200;

    static constexpr int kIconContainerX = 10;
    static constexpr int kIconContainerY = 630;
    static constexpr int kIconPlayerX = 1150;
    static constexpr int kIconPlayerY = 630;
    static constexpr int kIconSize = 48;

    static constexpr int kPaneHeaderHeight = 30;
    static constexpr int kPaneIconSize = 40;
    static constexpr int kPaneIconY = 105;

    static constexpr int kWeightDisplayY = 108;
    static constexpr int kWeightDisplayX = 700;
    static constexpr int kWeightDisplayW = 400;
    static constexpr int kWeightDisplayH = 24;

    static constexpr int kSelectionFrameW = 620;
    static constexpr int kSelectionFrameH = 86;

    struct ItemEntry {
        uint32_t itemId;
        std::string name;
        float weight;
        uint32_t value;
        uint32_t quantity;
        uint32_t iconId;
        int category;
        size_t sourceIndex;
    };

    struct Rect {
        float x, y, w, h;
    };

    void rebuildContainerList();
    void rebuildPlayerList();
    void rebuildFilteredLists();

    float scaleX() const { return scaleX_; }
    float scaleY() const { return scaleY_; }
    float originX() const;
    float originY() const;

    int hitTestRow(float touchX, float touchY, float paneX, float paneY, float paneW,
                   float paneH, int scrollOffset, const std::vector<size_t>& indices) const;
    int hitTestTab(float touchX, float touchY) const;
    int hitTestButton(float touchX, float touchY, float btnX, float btnY, float btnW, float btnH) const;

    void drawBackground();
    void drawPane(bool containerSide);
    void drawTabs();
    void drawButtons();
    void drawWeightDisplay();
    void drawSelectionFrame(float x, float y, float w, float h);

    GLuint loadTexture(const std::string& path);
    GLuint getIconTexture(const ItemEntry& entry);
    void clearTextureCache();

    // p68: preload UI textures
    void preloadUiTextures();
    GLuint uiTex(const std::string& key) const;

    TextRenderer* textRenderer_;
    int screenWidth_;
    int screenHeight_;
    bool visible_;
    float fadeAlpha_;

    std::shared_ptr<Container> container_;
    Player* player_;

    std::vector<ItemEntry> containerItems_;
    std::vector<ItemEntry> playerItems_;
    std::vector<size_t> filteredContainerIndices_[kCategoryCount];
    std::vector<size_t> filteredPlayerIndices_[kCategoryCount];

    int selectedContainerRow_;
    int selectedPlayerRow_;
    int activeTab_;
    Pane activePane_;
    int scrollOffsetContainer_;
    int scrollOffsetPlayer_;

    // p68: UI textures (by short key, e.g. "faders__black")
    std::unordered_map<std::string, GLuint> uiTextures_;
    // p68: 9-slice generic background keynames for loadUiTexture
    static const char* kGenericBgKeys[9];

    float scaleX_;
    float scaleY_;
};