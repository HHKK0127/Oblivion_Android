#pragma once

#include "ui_panel.h"
#include "../inventory/inventory_grid.h"
#include "../inventory/equipment_manager.h"

#include <GLES3/gl3.h>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

class TextRenderer;

namespace ui {

// InventoryMenu (menu id 0x3ea), laid out from Data\Menus\Main\inventory_menu.xml.
//
// The original is a sectioned vertical list inside a clipped window: every row is
// 81px tall, each category contributes one header row followed by its item rows,
// and inv_scroll_bar scrolls inv_pane by whole rows (y = -user7 * 81 - 8).
//
// Oblivion UI units sample textures 1:1 - the 596x160 border widget reads
// [0,596]x[0,160] of a 1024x256 DDS - so the whole menu is drawn through a single
// scale factor derived from the panel size, and every atlas widget crops with
// (cropx, cropy, width, height) directly.
class UIInventoryPanel : public UIPanel {
public:
    explicit UIInventoryPanel(const std::string& title = "Inventory");
    // Releases the cached UI/icon textures; the GL context is still current when the
    // renderer tears the UI down, exactly like UIPauseMenu::cleanup().
    ~UIInventoryPanel() override;

    void setInventory(inventory::InventoryGrid* inv);
    void setEquipment(inventory::EquipmentManager* eq) { equipment = eq; }
    void setTextRenderer(TextRenderer* tr) { textRenderer = tr; }

    // InventoryMenu.user9. The original always draws the gold row; the game has no
    // gold system yet, so it stays at 0 until one lands.
    void setGold(uint32_t value);

    void update(float deltaTime) override;
    void onScreenResize(int width, int height) override;
    // The menu swallows every event while it is up: the original gates the world
    // update on 0x00b3a6d4 (menu open) and the camera never sees the drag.
    bool onEvent(const UIEvent& event) override;
    bool onTouchDown(float x, float y, int pointerId) override;
    bool onTouchUp(float x, float y, int pointerId) override;
    bool onTouchMove(float x, float y, float dx, float dy, int pointerId) override;
    void render() override;

    void show() { setVisible(true); }
    void hide() { setVisible(false); }

    // InventoryMenu.user0: 1 = All, 2 = Weapons, 3 = Apparel, 4 = Alchemy, 5 = Misc.
    int page() const { return currentPage; }
    void setPage(int value);

private:
    // One entry of the scrollable list: either a category divider or an item row.
    struct ListRow {
        bool isHeader = false;
        int headerIndex = 0;   // 1..9, selects the header label and its column set
        int columnCount = 2;   // value columns shown for this section (2, 3 or 4)
        int slotIndex = -1;    // InventoryGrid slot for item rows
    };

    void refreshLayout();
    void rebuildRows();
    void clearTextureCache();

    float sx() const { return layoutScale; }
    float originX() const;
    float originY() const;

    void drawWidget(const char* assetName, float x, float y, float w, float h,
                    float cropx, float cropy, float cropW, float cropH,
                    float alpha = 1.0f);
    // cropW/cropH == 0 means "the whole texture".
    void drawAtlasIcon(float cropx, float x, float y, float size);
    void drawBackground();
    void drawRows();
    void drawItemRow(int rowIndex, float top);
    void drawHeaderRow(int rowIndex, float top);
    void drawTopHeader();
    void drawScrollBar();
    void drawTabs();
    void drawExtraData();
    void drawFocusBox(float x, float y, float w, float h);

    GLuint texture(const std::string& name);
    std::string iconPathFor(uint32_t iconId) const;
    void itemColumns(const inventory::Item& item, const ListRow& row, std::string out[4]) const;
    bool isEquipped(const inventory::Item& item) const;
    float extraDataX() const;

    int maxScrollOffset() const;
    int hitTestRow(float x, float y) const;
    int hitTestTab(float x, float y) const;
    int hitTestSortHeader(float x, float y) const;
    bool hitTestScrollBar(float x, float y) const;
    void toggleSort(int column);
    void scrollBy(int rows);
    void setScrollFromMarkerY(float screenY);

    // --- data sources ---
    inventory::InventoryGrid* inventory = nullptr;
    inventory::EquipmentManager* equipment = nullptr;
    TextRenderer* textRenderer = nullptr;

    // --- view state ---
    float layoutScale = 1.0f;
    int currentPage = 1;
    int scrollOffset = 0;
    int selectedRow = -1;
    uint32_t gold = 0;

    bool draggingMarker = false;
    float markerGrabOffset = 0.0f;

    // inv_window is clipwindow="true": rows and the scroll bar are cut off at the
    // window edges instead of spilling over the background art.
    bool clipActive = false;
    float clipTop = 0.0f;
    float clipBottom = 0.0f;

    // Sort header state. sortColumn 0 = insertion order, 1..4 = value/weight/third/
    // fourth column. headernum mirrors the XML's _headernum, which only selects
    // which of the img3/img4 icon variants the current page shows.
    int sortColumn = 0;
    bool sortAscending = true;
    int headernum = 1;

    // --- derived list ---
    std::vector<ListRow> rows;
    bool rowsDirty = true;
    int headerOfTopSection = 1;
    int totalWeight = 0;
    int armorRating = 0;

    std::unordered_map<uint32_t, GLuint> iconTextures;
    std::unordered_map<std::string, GLuint> uiTextures;

    GLuint getIconTexture(uint32_t iconId);
};

}  // namespace ui