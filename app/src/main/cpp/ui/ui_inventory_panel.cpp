#include "ui_inventory_panel.h"
#include "ui_draw_helper.h"
#include <GLES3/gl3.h>
#include <algorithm>

namespace ui {

UIInventoryPanel::UIInventoryPanel(const std::string& title)
    : UIPanel(title),
      selectedSlot(-1),
      dragSourceSlot(-1),
      isDraggingItem(false) {
    setBackgroundColor(glm::vec4(0.08f, 0.08f, 0.1f, 0.95f));
    setBorderColor(glm::vec4(0.4f, 0.4f, 0.5f, 1.0f));
    setBorderWidth(2.0f);
    setTitleBarColor(glm::vec4(0.2f, 0.18f, 0.25f, 0.95f));
    setTitle(title);
    setCloseButtonVisible(true);
    setDraggable(true);
}

void UIInventoryPanel::update(float deltaTime) {
    UIPanel::update(deltaTime);
    if (isVisible()) {
        refreshLayout();
    }
}

bool UIInventoryPanel::onTouchDown(float x, float y, int pointerId) {
    if (!isVisible() || !isEnabled()) return false;

    // Close button / title bar drag handled by UIPanel
    if (isInsideCloseButton(x, y) || isInsideTitleBar(x, y)) {
        return UIPanel::onTouchDown(x, y, pointerId);
    }

    // Sort buttons (top of content area)
    inventory::InventoryGrid::SortMode sortMode;
    if (hitTestSortButton(x, y, sortMode)) {
        if (inventory) inventory->sort(sortMode);
        return true;
    }

    // Inventory grid click
    int gridIdx = hitTestGrid(x, y);
    if (gridIdx >= 0) {
        selectedSlot = gridIdx;
        dragSourceSlot = -1;
        isDraggingItem = false;
        // Drag-and-drop requires TOUCH_MOVE/TOUCH_UP events from JNI layer
        // For now, just select the slot to show item details
        return true;
    }

    // Equipment slot click
    inventory::EquipSlot eqSlot = hitTestEquipSlot(x, y);
    if (eqSlot != inventory::EquipSlot::None) {
        // Unequip if occupied
        if (equipment && equipment->isSlotOccupied(eqSlot)) {
            const inventory::Item* item = equipment->getEquipped(eqSlot);
            if (item && inventory) {
                if (inventory->addItem(*item, 1)) {
                    equipment->unequip(eqSlot);
                }
            }
        }
        return true;
    }

    return false;
}

bool UIInventoryPanel::onTouchUp(float x, float y, int pointerId) {
    if (!isVisible()) return false;

    // Panel base handling (title bar drag end)
    bool panelResult = UIPanel::onTouchUp(x, y, pointerId);

    if (isDraggingItem && dragSourceSlot >= 0 && inventory) {
        int targetSlot = hitTestGrid(x, y);
        if (targetSlot >= 0 && targetSlot != dragSourceSlot) {
            inventory->moveSlot(static_cast<uint32_t>(dragSourceSlot), static_cast<uint32_t>(targetSlot));
        } else {
            // Check equipment drop
            inventory::EquipSlot eqSlot = hitTestEquipSlot(x, y);
            if (eqSlot != inventory::EquipSlot::None && equipment) {
                const auto& slot = inventory->getSlot(static_cast<uint32_t>(dragSourceSlot));
                if (!slot.isEmpty() && slot.item.equipSlot == eqSlot) {
                    if (equipment->equip(slot.item)) {
                        inventory->removeSlot(static_cast<uint32_t>(dragSourceSlot), 1);
                    }
                }
            }
        }
    }

    isDraggingItem = false;
    dragSourceSlot = -1;
    return panelResult;
}

bool UIInventoryPanel::onTouchMove(float x, float y, float dx, float dy, int pointerId) {
    if (isDraggingItem) {
        dragPos = glm::vec2(x, y);
        return true;
    }
    return UIPanel::onTouchMove(x, y, dx, dy, pointerId);
}

void UIInventoryPanel::render() {
    if (!isVisible()) return;

    UIPanel::render();

    GLboolean depthTestEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    renderInventoryGrid();
    renderEquipmentSlots();
    renderSortButtons();
    if (selectedSlot >= 0) {
        renderItemDetail();
    }
    if (isDraggingItem) {
        renderDraggedItem();
    }

    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
}

void UIInventoryPanel::refreshLayout() {
    glm::vec2 cs = getContentSize();
    glm::vec2 cp = getContentPosition();

    float availableWidth = cs.x;
    float equipPanelWidth = EQUIP_SLOT_SIZE * 1.5f + 20.0f;
    float gridAreaWidth = availableWidth - equipPanelWidth - 20.0f;

    cellSize = std::min(56.0f, (gridAreaWidth - (GRID_COLS - 1) * CELL_MARGIN) / GRID_COLS);
    float gridTotalWidth = cellSize * GRID_COLS + CELL_MARGIN * (GRID_COLS - 1);
    float gridTotalHeight = cellSize * GRID_ROWS + CELL_MARGIN * (GRID_ROWS - 1);

    gridStartX = cp.x;
    gridStartY = cp.y + 30.0f; // leave room for sort buttons

    equipStartX = cp.x + gridTotalWidth + 20.0f;
    equipStartY = gridStartY;
}

void UIInventoryPanel::renderInventoryGrid() {
    if (!inventory) return;

    for (uint32_t row = 0; row < GRID_ROWS; ++row) {
        for (uint32_t col = 0; col < GRID_COLS; ++col) {
            uint32_t idx = row * GRID_COLS + col;
            float x = gridStartX + col * (cellSize + CELL_MARGIN);
            float y = gridStartY + row * (cellSize + CELL_MARGIN);

            // Cell background
            glm::vec4 bgColor(0.15f, 0.15f, 0.18f, 1.0f);
            if (static_cast<int>(idx) == selectedSlot) {
                bgColor = glm::vec4(0.3f, 0.3f, 0.5f, 1.0f);
            }
            if (isDraggingItem && static_cast<int>(idx) == dragSourceSlot) {
                bgColor = glm::vec4(0.5f, 0.5f, 0.3f, 1.0f);
            }
            UIDrawHelper::drawColoredQuad(x, y, cellSize, cellSize, bgColor, screenWidth, screenHeight);

            // Item icon placeholder (colored square by category)
            const auto& slot = inventory->getSlot(idx);
            if (!slot.isEmpty()) {
                glm::vec4 itemColor = glm::vec4(0.6f, 0.6f, 0.6f, 1.0f);
                switch (slot.item.category) {
                    case inventory::ItemCategory::Weapon:     itemColor = glm::vec4(0.9f, 0.3f, 0.3f, 1.0f); break;
                    case inventory::ItemCategory::Armor:      itemColor = glm::vec4(0.3f, 0.5f, 0.9f, 1.0f); break;
                    case inventory::ItemCategory::Consumable: itemColor = glm::vec4(0.3f, 0.9f, 0.4f, 1.0f); break;
                    case inventory::ItemCategory::Material:   itemColor = glm::vec4(0.7f, 0.6f, 0.3f, 1.0f); break;
                    case inventory::ItemCategory::Quest:      itemColor = glm::vec4(0.9f, 0.8f, 0.2f, 1.0f); break;
                    default: break;
                }
                float pad = cellSize * 0.15f;
                UIDrawHelper::drawColoredQuad(x + pad, y + pad, cellSize - pad * 2.0f, cellSize - pad * 2.0f,
                                              itemColor, screenWidth, screenHeight);

                // Stack quantity indicator
                if (slot.quantity > 1) {
                    glm::vec4 qtyBg(0.0f, 0.0f, 0.0f, 0.7f);
                    UIDrawHelper::drawColoredQuad(x + cellSize - 18.0f, y + cellSize - 14.0f, 18.0f, 14.0f,
                                                  qtyBg, screenWidth, screenHeight);
                    // Text rendering omitted for now (use color/size later)
                }
            }
        }
    }
}

void UIInventoryPanel::renderEquipmentSlots() {
    if (!equipment) return;

    const char* labels[] = { "None", "Head", "Body", "Hands", "Feet", "Weapon", "Offhand", "Accessory" };
    for (uint32_t i = 1; i < static_cast<uint32_t>(inventory::EquipSlot::Accessory) + 1; ++i) {
        float x = equipStartX;
        float y = equipStartY + (i - 1) * (EQUIP_SLOT_SIZE + 8.0f);

        glm::vec4 bg(0.2f, 0.2f, 0.25f, 1.0f);
        auto slot = static_cast<inventory::EquipSlot>(i);
        if (equipment->isSlotOccupied(slot)) {
            bg = glm::vec4(0.4f, 0.4f, 0.6f, 1.0f);
        }
        UIDrawHelper::drawColoredQuad(x, y, EQUIP_SLOT_SIZE, EQUIP_SLOT_SIZE, bg, screenWidth, screenHeight);
    }
}

void UIInventoryPanel::renderItemDetail() {
    if (!inventory || selectedSlot < 0) return;
    const auto& slot = inventory->getSlot(static_cast<uint32_t>(selectedSlot));
    if (slot.isEmpty()) return;

    // Simple detail box at bottom of content area
    glm::vec2 cp = getContentPosition();
    glm::vec2 cs = getContentSize();
    float boxH = 80.0f;
    float boxY = cp.y + cs.y - boxH;
    UIDrawHelper::drawColoredQuad(cp.x, boxY, cs.x, boxH,
                                  glm::vec4(0.1f, 0.1f, 0.15f, 0.95f), screenWidth, screenHeight);

    // Highlight border
    UIDrawHelper::drawBorder(cp.x, boxY, cs.x, boxH, 2.0f,
                             glm::vec4(0.6f, 0.6f, 0.8f, 1.0f), screenWidth, screenHeight);
}

void UIInventoryPanel::renderDraggedItem() {
    if (!inventory || dragSourceSlot < 0) return;
    const auto& slot = inventory->getSlot(static_cast<uint32_t>(dragSourceSlot));
    if (slot.isEmpty()) return;

    float sz = cellSize * 0.9f;
    glm::vec4 col(0.8f, 0.8f, 0.8f, 0.8f);
    UIDrawHelper::drawColoredQuad(dragPos.x - sz * 0.5f, dragPos.y - sz * 0.5f, sz, sz,
                                  col, screenWidth, screenHeight);
}

void UIInventoryPanel::renderSortButtons() {
    // Small color bars at top as sort buttons
    glm::vec2 cp = getContentPosition();
    float btnW = 50.0f;
    float btnH = 22.0f;
    float gap = 6.0f;
    float startX = cp.x;
    float y = cp.y;

    glm::vec4 colors[] = {
        glm::vec4(0.5f, 0.5f, 0.5f, 1.0f),
        glm::vec4(0.5f, 0.3f, 0.3f, 1.0f),
        glm::vec4(0.3f, 0.3f, 0.5f, 1.0f),
        glm::vec4(0.3f, 0.5f, 0.3f, 1.0f),
        glm::vec4(0.5f, 0.5f, 0.3f, 1.0f)
    };
    for (int i = 0; i < 5; ++i) {
        UIDrawHelper::drawColoredQuad(startX + i * (btnW + gap), y, btnW, btnH,
                                      colors[i], screenWidth, screenHeight);
    }
}

int UIInventoryPanel::hitTestGrid(float x, float y) const {
    for (uint32_t row = 0; row < GRID_ROWS; ++row) {
        for (uint32_t col = 0; col < GRID_COLS; ++col) {
            float cx = gridStartX + col * (cellSize + CELL_MARGIN);
            float cy = gridStartY + row * (cellSize + CELL_MARGIN);
            if (x >= cx && x <= cx + cellSize && y >= cy && y <= cy + cellSize) {
                return static_cast<int>(row * GRID_COLS + col);
            }
        }
    }
    return -1;
}

inventory::EquipSlot UIInventoryPanel::hitTestEquipSlot(float x, float y) const {
    for (uint32_t i = 1; i <= static_cast<uint32_t>(inventory::EquipSlot::Accessory); ++i) {
        float ex = equipStartX;
        float ey = equipStartY + (i - 1) * (EQUIP_SLOT_SIZE + 8.0f);
        if (x >= ex && x <= ex + EQUIP_SLOT_SIZE && y >= ey && y <= ey + EQUIP_SLOT_SIZE) {
            return static_cast<inventory::EquipSlot>(i);
        }
    }
    return inventory::EquipSlot::None;
}

bool UIInventoryPanel::hitTestSortButton(float x, float y, inventory::InventoryGrid::SortMode& outMode) const {
    glm::vec2 cp = getContentPosition();
    float btnW = 50.0f;
    float btnH = 22.0f;
    float gap = 6.0f;
    float startX = cp.x;
    float by = cp.y;
    for (int i = 0; i < 5; ++i) {
        float bx = startX + i * (btnW + gap);
        if (x >= bx && x <= bx + btnW && y >= by && y <= by + btnH) {
            outMode = static_cast<inventory::InventoryGrid::SortMode>(i);
            return true;
        }
    }
    return false;
}

} // namespace ui
