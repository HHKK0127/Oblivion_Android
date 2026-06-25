#pragma once

#include "text_renderer.h"
#include <glm/glm.hpp>
#include <string>
#include <memory>
#include <functional>

/**
 * @brief クイックスロットバー
 *
 * Phase 15: ゲーム内HUD - 画面下部のクイックアクセスバー
 * よく使うスペルやアイテムを割り当て、素早くアクセス可能
 */
class UIQuickSlotBar {
public:
    enum SlotType {
        EMPTY = 0,
        SPELL = 1,
        ITEM = 2,
        ABILITY = 3
    };

    struct SlotData {
        SlotType type;
        std::string itemId;
        std::string itemName;
        int quantity;  // For consumables
        int manaCost;  // For spells

        SlotData() : type(EMPTY), itemId(""), itemName(""), quantity(0), manaCost(0) {}
    };

    using SlotCallback = std::function<void(int slotIndex)>;

    UIQuickSlotBar();
    ~UIQuickSlotBar() = default;

    bool initialize(TextRenderer* textRenderer, int screenW, int screenH);

    // Slot management
    void setSlot(int slotIndex, const SlotData& data);
    void clearSlot(int slotIndex);
    const SlotData& getSlot(int slotIndex) const;

    // Selection
    void selectSlot(int slotIndex);
    int getSelectedSlot() const { return selectedSlotIndex_; }

    void update(float deltaTime);
    bool onTouchDown(float x, float y, int pointerId);
    void render();

    void setScreenSize(int w, int h) {
        screenWidth = w;
        screenHeight = h;
    }

    // Callback when slot is activated/selected
    void setOnSlotSelected(SlotCallback cb) { onSlotSelected = cb; }

private:
    TextRenderer* textRenderer = nullptr;
    SlotCallback onSlotSelected;

    static constexpr int SLOT_COUNT = 8;
    SlotData slots_[SLOT_COUNT];
    int selectedSlotIndex_ = 0;

    int screenWidth = 1080;
    int screenHeight = 1920;

    static constexpr float SLOT_SIZE = 56.0f;
    static constexpr float SLOT_GAP = 4.0f;
    static constexpr float BAR_PADDING = 10.0f;

    void renderSlot(int slotIndex, float x, float y);
    void renderSlotIcon(const SlotData& slot, float x, float y);
    void renderSlotQuantity(const SlotData& slot, float x, float y);

    int hitTestSlot(float x, float y) const;

    // Layout helpers
    float getBarStartX() const;
    float getBarY() const;
};
