#pragma once

#include "ui_panel.h"
#include "../inventory/inventory_grid.h"
#include "../inventory/equipment_manager.h"
#include <memory>

// Forward declaration
class TextRenderer;

namespace ui {

class UIInventoryPanel : public UIPanel {
public:
    explicit UIInventoryPanel(const std::string& title = "Inventory");
    ~UIInventoryPanel() override = default;

    void setInventory(inventory::InventoryGrid* inv) { inventory = inv; }
    void setEquipment(inventory::EquipmentManager* eq) { equipment = eq; }
    void setTextRenderer(TextRenderer* tr) { textRenderer = tr; }

    void update(float deltaTime) override;
    bool onTouchDown(float x, float y, int pointerId) override;
    bool onTouchUp(float x, float y, int pointerId) override;
    bool onTouchMove(float x, float y, float dx, float dy, int pointerId) override;
    void render() override;

    // Convenience
    void show() { setVisible(true); }
    void hide() { setVisible(false); }

    // Screen size for coordinate calculations
    void setScreenSize(int w, int h) { screenWidth = w; screenHeight = h; }

protected:
    void renderInventoryGrid();
    void renderEquipmentSlots();
    void renderItemDetail();
    void renderDraggedItem();
    void renderSortButtons();

    // Hit testing
    int hitTestGrid(float x, float y) const; // returns slot index or -1
    inventory::EquipSlot hitTestEquipSlot(float x, float y) const;
    bool hitTestSortButton(float x, float y, inventory::InventoryGrid::SortMode& outMode) const;

    void refreshLayout();

private:
    inventory::InventoryGrid* inventory = nullptr;
    inventory::EquipmentManager* equipment = nullptr;
    TextRenderer* textRenderer = nullptr;

    // Layout metrics (calculated in refreshLayout)
    float cellSize = 48.0f;
    float gridStartX = 0.0f;
    float gridStartY = 0.0f;
    float equipStartX = 0.0f;
    float equipStartY = 0.0f;

    // Selection / drag state
    int selectedSlot = -1;
    int dragSourceSlot = -1;
    glm::vec2 dragPos{0.0f, 0.0f};
    bool isDraggingItem = false;

    static constexpr uint32_t GRID_COLS = inventory::InventoryGrid::COLUMNS;
    static constexpr uint32_t GRID_ROWS = inventory::InventoryGrid::ROWS;
    static constexpr float CELL_MARGIN = 4.0f;
    static constexpr float EQUIP_SLOT_SIZE = 48.0f;

    int screenWidth = 1080;
    int screenHeight = 1920;
};

} // namespace ui
