#include "ui_inventory_panel.h"
#include "ui_draw_helper.h"
#include "placeholder_assets.h"
#include "text_renderer.h"
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

            // Cell background panel with border
            glm::vec3 bgColor = glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT);
            glm::vec3 borderColor = glm::vec3(PlaceholderAssets::Colors::BROWN_ACCENT);

            const auto& slot = inventory->getSlot(idx);
            if (static_cast<int>(idx) == selectedSlot) {
                // Selected: gold highlight
                borderColor = glm::vec3(PlaceholderAssets::Colors::GOLD_HIGHLIGHT);
                bgColor = glm::vec3(PlaceholderAssets::Colors::PARCHMENT_DARK);
            } else if (!slot.isEmpty()) {
                // Has item: slightly highlighted
                bgColor = glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT);
            } else {
                // Empty: normal
                bgColor = glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT) * 0.8f;
            }

            if (isDraggingItem && static_cast<int>(idx) == dragSourceSlot) {
                borderColor = glm::vec3(PlaceholderAssets::Colors::GOLD_HIGHLIGHT) * 1.2f;
            }

            PlaceholderAssets::drawPanel(x, y, cellSize, cellSize, bgColor, borderColor);

            // Item icon placeholder (category-based color)
            if (!slot.isEmpty()) {
                glm::vec3 itemColor(0.6f, 0.6f, 0.6f);
                switch (slot.item.category) {
                    case inventory::ItemCategory::Weapon:
                        itemColor = glm::vec3(0.9f, 0.3f, 0.3f);
                        break;
                    case inventory::ItemCategory::Armor:
                        itemColor = glm::vec3(0.3f, 0.5f, 0.9f);
                        break;
                    case inventory::ItemCategory::Consumable:
                        itemColor = glm::vec3(0.3f, 0.9f, 0.4f);
                        break;
                    case inventory::ItemCategory::Material:
                        itemColor = glm::vec3(0.7f, 0.6f, 0.3f);
                        break;
                    case inventory::ItemCategory::Quest:
                        itemColor = glm::vec3(0.9f, 0.8f, 0.2f);
                        break;
                    default:
                        break;
                }
                float pad = cellSize * 0.15f;
                UIDrawHelper::drawColoredQuad(x + pad, y + pad, cellSize - pad * 2.0f, cellSize - pad * 2.0f,
                                              glm::vec4(itemColor.x, itemColor.y, itemColor.z, 1.0f), screenWidth, screenHeight);

                // Stack quantity indicator
                if (slot.quantity > 1) {
                    PlaceholderAssets::drawPanel(x + cellSize - 18.0f, y + cellSize - 14.0f, 18.0f, 14.0f,
                                                 PlaceholderAssets::Colors::DARK_GRAY,
                                                 PlaceholderAssets::Colors::GOLD_HIGHLIGHT);
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

        auto slot = static_cast<inventory::EquipSlot>(i);
        glm::vec3 bgColor = glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT);
        glm::vec3 borderColor = glm::vec3(PlaceholderAssets::Colors::BROWN_ACCENT);

        if (equipment->isSlotOccupied(slot)) {
            // Equipped: highlight with gold
            bgColor = glm::vec3(PlaceholderAssets::Colors::PARCHMENT_DARK);
            borderColor = glm::vec3(PlaceholderAssets::Colors::GOLD_HIGHLIGHT);
        }

        PlaceholderAssets::drawPanel(x, y, EQUIP_SLOT_SIZE, EQUIP_SLOT_SIZE, bgColor, borderColor);

        // Draw label if text renderer available
        if (textRenderer && i > 0 && i < 8) {
            textRenderer->renderText(labels[i], x + 4.0f, y + 4.0f, glm::vec3(0.2f, 0.2f, 0.2f), 0.6f);
        }
    }
}

void UIInventoryPanel::renderItemDetail() {
    if (!inventory || selectedSlot < 0) return;
    const auto& slot = inventory->getSlot(static_cast<uint32_t>(selectedSlot));
    if (slot.isEmpty()) return;

    glm::vec2 cp = getContentPosition();
    glm::vec2 cs = getContentSize();

    // Detail popup panel positioned at bottom
    float detailW = 280.0f;
    float detailH = 200.0f;
    float detailX = cp.x + cs.x - detailW - 10.0f;
    float detailY = cp.y + cs.y - detailH - 10.0f;

    // Draw detail panel with Oblivion styling
    PlaceholderAssets::drawPanel(detailX, detailY, detailW, detailH,
                                 PlaceholderAssets::Colors::PARCHMENT_LIGHT,
                                 PlaceholderAssets::Colors::GOLD_HIGHLIGHT);

    // Render item information
    if (textRenderer) {
        float textX = detailX + 8.0f;
        float textY = detailY + 8.0f;
        const auto& item = slot.item;

        // Item name
        textRenderer->renderText(item.name, textX, textY, glm::vec3(0.1f, 0.1f, 0.1f), 1.0f);
        textY += 18.0f;

        // Category label
        const char* catLabel = "Item";
        switch (item.category) {
            case inventory::ItemCategory::Weapon:     catLabel = "Weapon"; break;
            case inventory::ItemCategory::Armor:      catLabel = "Armor"; break;
            case inventory::ItemCategory::Consumable: catLabel = "Consumable"; break;
            case inventory::ItemCategory::Material:   catLabel = "Material"; break;
            case inventory::ItemCategory::Quest:      catLabel = "Quest Item"; break;
            default: break;
        }
        textRenderer->renderText(catLabel, textX, textY, glm::vec3(0.4f, 0.25f, 0.2f), 0.8f);
        textY += 14.0f;

        // Weight and value
        char infoStr[128];
        snprintf(infoStr, sizeof(infoStr), "Weight: %.1f kg", item.weight);
        textRenderer->renderText(infoStr, textX, textY, glm::vec3(0.2f, 0.2f, 0.2f), 0.7f);
        textY += 12.0f;

        snprintf(infoStr, sizeof(infoStr), "Value: %u gp", item.value);
        textRenderer->renderText(infoStr, textX, textY, glm::vec3(0.2f, 0.2f, 0.2f), 0.7f);
        textY += 14.0f;

        // Stats display (if any)
        if (item.stats.damage > 0 || item.stats.defense > 0) {
            if (item.stats.damage > 0) {
                snprintf(infoStr, sizeof(infoStr), "Damage: +%d", item.stats.damage);
                textRenderer->renderText(infoStr, textX, textY, glm::vec3(0.8f, 0.2f, 0.2f), 0.7f);
                textY += 12.0f;
            }
            if (item.stats.defense > 0) {
                snprintf(infoStr, sizeof(infoStr), "Defense: +%d", item.stats.defense);
                textRenderer->renderText(infoStr, textX, textY, glm::vec3(0.3f, 0.4f, 0.8f), 0.7f);
                textY += 12.0f;
            }
        }

        // Consumable effects
        if (item.isConsumable()) {
            if (item.healAmount > 0) {
                snprintf(infoStr, sizeof(infoStr), "Restores %d HP", item.healAmount);
                textRenderer->renderText(infoStr, textX, textY, glm::vec3(0.8f, 0.1f, 0.1f), 0.7f);
                textY += 12.0f;
            }
            if (item.manaAmount > 0) {
                snprintf(infoStr, sizeof(infoStr), "Restores %d MP", item.manaAmount);
                textRenderer->renderText(infoStr, textX, textY, glm::vec3(0.2f, 0.3f, 0.8f), 0.7f);
            }
        }
    }
}

void UIInventoryPanel::renderDraggedItem() {
    if (!inventory || dragSourceSlot < 0) return;
    const auto& slot = inventory->getSlot(static_cast<uint32_t>(dragSourceSlot));
    if (slot.isEmpty()) return;

    float sz = cellSize * 0.9f;
    glm::vec3 itemColor(0.6f, 0.6f, 0.6f);
    switch (slot.item.category) {
        case inventory::ItemCategory::Weapon:
            itemColor = glm::vec3(0.9f, 0.3f, 0.3f);
            break;
        case inventory::ItemCategory::Armor:
            itemColor = glm::vec3(0.3f, 0.5f, 0.9f);
            break;
        case inventory::ItemCategory::Consumable:
            itemColor = glm::vec3(0.3f, 0.9f, 0.4f);
            break;
        case inventory::ItemCategory::Material:
            itemColor = glm::vec3(0.7f, 0.6f, 0.3f);
            break;
        case inventory::ItemCategory::Quest:
            itemColor = glm::vec3(0.9f, 0.8f, 0.2f);
            break;
        default:
            break;
    }

    // Draw dragged item as semi-transparent icon
    PlaceholderAssets::drawPanel(dragPos.x - sz * 0.5f, dragPos.y - sz * 0.5f, sz, sz,
                                 itemColor * 0.8f, PlaceholderAssets::Colors::GOLD_HIGHLIGHT);
}

void UIInventoryPanel::renderSortButtons() {
    // Sort buttons with Oblivion styling
    glm::vec2 cp = getContentPosition();
    float btnW = 50.0f;
    float btnH = 22.0f;
    float gap = 6.0f;
    float startX = cp.x;
    float y = cp.y;

    const char* labels[] = { "Name", "Type", "Rare", "Weight", "Value" };
    const glm::vec3 bgColors[] = {
        PlaceholderAssets::Colors::PARCHMENT_DARK,
        PlaceholderAssets::Colors::PARCHMENT_DARK,
        PlaceholderAssets::Colors::PARCHMENT_DARK,
        PlaceholderAssets::Colors::PARCHMENT_DARK,
        PlaceholderAssets::Colors::PARCHMENT_DARK
    };

    for (int i = 0; i < 5; ++i) {
        float btnX = startX + i * (btnW + gap);
        PlaceholderAssets::drawPanel(btnX, y, btnW, btnH,
                                     bgColors[i], PlaceholderAssets::Colors::BROWN_ACCENT);
        if (textRenderer) {
            textRenderer->renderText(labels[i], btnX + 4.0f, y + 2.0f, glm::vec3(0.1f, 0.1f, 0.1f), 0.6f);
        }
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
