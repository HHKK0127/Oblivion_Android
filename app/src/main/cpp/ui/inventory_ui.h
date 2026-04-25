#pragma once

#include "../game/inventory.h"
#include "../ui/text_renderer.h"
#include <memory>
#include <android/log.h>

#define LOG_TAG "InventoryUI"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

class InventoryUI {
public:
    InventoryUI();
    ~InventoryUI();

    // Lifecycle
    bool initialize(std::shared_ptr<Inventory> inv, TextRenderer* textRenderer);
    void update(float deltaTime);
    void render();

    // UI Control
    void show();
    void hide();
    bool isVisible() const { return visible; }
    void toggle() { visible ? hide() : show(); }

    // Input
    void onTouchEvent(float x, float y);
    void onKeyPress(int key);

private:
    std::shared_ptr<Inventory> inventory;
    TextRenderer* textRenderer;
    bool visible = false;

    // Grid layout
    static constexpr float SLOT_WIDTH = 40.0f;
    static constexpr float SLOT_HEIGHT = 40.0f;
    static constexpr float GRID_PADDING = 10.0f;
    static constexpr float GRID_START_X = 10.0f;
    static constexpr float GRID_START_Y = 50.0f;

    int selectedSlot = 0;

    // Rendering helpers
    void renderGridHeader();
    void renderGridSlots();
    void renderItemInfo();
    void renderWeightInfo();

    int getSlotAtPosition(float x, float y) const;
    void getSlotPosition(int slot, float& outX, float& outY) const;
};
