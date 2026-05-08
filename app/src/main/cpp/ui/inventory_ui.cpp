#include "inventory_ui.h"
#include <cmath>

InventoryUI::InventoryUI() {
    LOGD("InventoryUI created");
}

InventoryUI::~InventoryUI() {
}

bool InventoryUI::initialize(std::shared_ptr<Inventory> inv, TextRenderer* tr) {
    if (!inv || !tr) {
        LOGE("Cannot initialize InventoryUI with null pointers");
        return false;
    }

    inventory = inv;
    textRenderer = tr;
    LOGI("InventoryUI initialized");
    return true;
}

void InventoryUI::update(float deltaTime) {
    // Can be used for animations, etc.
}

void InventoryUI::render() {
    if (!visible || !inventory || !textRenderer) return;

    // Draw background panel
    // Note: Simple text rendering, visual polish in later phases
    textRenderer->renderText("=== INVENTORY ===", 50.0f, 20.0f);

    renderGridHeader();
    renderGridSlots();
    renderWeightInfo();
    renderItemInfo();
}

void InventoryUI::show() {
    visible = true;
    LOGD("Inventory UI shown");
}

void InventoryUI::hide() {
    visible = false;
    selectedSlot = 0;
    LOGD("Inventory UI hidden");
}

void InventoryUI::onTouchEvent(float x, float y) {
    if (!visible || !inventory) return;

    int slot = getSlotAtPosition(x, y);
    if (slot >= 0 && slot < 60) {
        selectedSlot = slot;
        LOGD("Selected slot: %d", slot);
    }
}

void InventoryUI::onKeyPress(int key) {
    if (!visible) return;

    if (key == 27) {  // ESC key
        hide();
    } else if (key == 'D') {
        // Drop item
        const auto& slot = inventory->getSlot(selectedSlot);
        if (!slot.isEmpty()) {
            LOGI("Drop item: %s (slot %d)", slot.item.name.c_str(), selectedSlot);
        }
    }
}

void InventoryUI::renderGridHeader() {
    float startY = GRID_START_Y - 20.0f;
    std::string header = "Inventory Grid (10x6)";
    textRenderer->renderText(header, GRID_START_X, startY);
}

void InventoryUI::renderGridSlots() {
    for (int row = 0; row < 6; ++row) {
        for (int col = 0; col < 10; ++col) {
            int slotIdx = row * 10 + col;
            float x = GRID_START_X + col * (SLOT_WIDTH + 5.0f);
            float y = GRID_START_Y + row * (SLOT_HEIGHT + 5.0f);

            const auto& slot = inventory->getSlot(slotIdx);

            // Draw slot box (simple text representation)
            if (!slot.isEmpty()) {
                // Show item abbreviation
                std::string slotText = slot.item.name.substr(0, 2);
                if (slot.quantity > 1) {
                    slotText += "x" + std::to_string(slot.quantity);
                }
                textRenderer->renderText(slotText, x, y);
            } else {
                textRenderer->renderText("[_]", x, y);
            }

            // Highlight selected slot
            if (slotIdx == selectedSlot) {
                textRenderer->renderText(">>>", x - 15.0f, y);
            }
        }
    }
}

void InventoryUI::renderItemInfo() {
    const auto& slot = inventory->getSlot(selectedSlot);
    float y = GRID_START_Y + 280.0f;

    if (slot.isEmpty()) {
        textRenderer->renderText("Empty Slot", GRID_START_X, y);
    } else {
        textRenderer->renderText("Item: " + slot.item.name, GRID_START_X, y);
        textRenderer->renderText("Weight: " + std::to_string(slot.item.weight) + " kg",
                                GRID_START_X, y + 20.0f);
        textRenderer->renderText("Value: " + std::to_string(slot.item.value) + " gold",
                                GRID_START_X, y + 40.0f);
        textRenderer->renderText("Qty: " + std::to_string(slot.quantity),
                                GRID_START_X, y + 60.0f);
    }
}

void InventoryUI::renderWeightInfo() {
    float totalWeight = inventory->getTotalWeight();
    float maxWeight = 100.0f;
    float y = GRID_START_Y + 320.0f;

    std::string weightStr = "Weight: " + std::to_string((int)totalWeight) +
                           " / " + std::to_string((int)maxWeight) + " kg";
    textRenderer->renderText(weightStr, GRID_START_X, y);
}

int InventoryUI::getSlotAtPosition(float x, float y) const {
    // Calculate which slot was touched
    float relX = x - GRID_START_X;
    float relY = y - GRID_START_Y;

    if (relX < 0 || relY < 0) return -1;

    int col = (int)((relX) / (SLOT_WIDTH + 5.0f));
    int row = (int)((relY) / (SLOT_HEIGHT + 5.0f));

    if (col < 0 || col >= 10 || row < 0 || row >= 6) return -1;

    return row * 10 + col;
}

void InventoryUI::getSlotPosition(int slot, float& outX, float& outY) const {
    int row = slot / 10;
    int col = slot % 10;

    outX = GRID_START_X + col * (SLOT_WIDTH + 5.0f);
    outY = GRID_START_Y + row * (SLOT_HEIGHT + 5.0f);
}
