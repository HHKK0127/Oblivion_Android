#pragma once

#include <functional>
#include <GLES3/gl3.h>
#include <glm/glm.hpp>
#include <string>

class TextRenderer;

class UIQuickKeysMenu {
public:
    static constexpr int kSlotCount = 8;

    // Shared slot data type matching the QuickSlotBar's SlotData concept
    struct SlotEntry {
        std::string name;
        uint32_t itemId = 0;
        std::string texturePath;
        bool occupied = false;
    };

    using SlotCallback = std::function<void(int slotIndex)>;

    UIQuickKeysMenu();
    ~UIQuickKeysMenu();

    bool initialize(TextRenderer* renderer);
    void cleanup();

    void open();
    void close();
    bool isVisible() const { return visible_; }

    void setScreenSize(int width, int height);
    void setFadeAlpha(float alpha);

    void update(float deltaTime);
    void render();

    void onTouchEvent(float x, float y);
    bool onKeyPress(int keyCode);
    bool handleBack();

    // Populate slots
    void setSlot(int index, const SlotEntry& entry);
    const SlotEntry& getSlot(int index) const;

    // Selection result
    int consumeSelectedSlot();
    bool hasSelection() const;

    void setOnSlotSelected(SlotCallback cb) { onSlotSelected_ = std::move(cb); }
    void setOnCancelCallback(SlotCallback cb) { onCancel_ = std::move(cb); }

private:
    // RE spec: radial slot positions (center-relative)
    struct Position { float x, y; };
    static constexpr Position kSlotPositions[8] = {
        {202, 102}, {274, 132}, {302, 205}, {272, 275},
        {200, 303}, {130, 273}, {102, 202}, {132, 130},
    };
    static constexpr float kPaperSize = 512.0f;
    static constexpr float kSlotIconSize = 64.0f;
    static constexpr float kCanvasWidth = 640.0f;
    static constexpr float kCanvasHeight = 640.0f;

    void drawBackground();
    void drawPaper();
    void drawSlots();
    int slotFromTouch(float x, float y) const;
    int slotFromKey(int keyCode) const;

    TextRenderer* textRenderer_ = nullptr;
    int screenWidth_ = 1920;
    int screenHeight_ = 1080;
    bool visible_ = false;
    float fadeAlpha_ = 1.0f;

    SlotEntry slots_[kSlotCount];
    int selectedSlot_ = -1;
    bool hasSelection_ = false;
    int pendingSlot_ = -1;

    SlotCallback onSlotSelected_;
    SlotCallback onCancel_;

    GLuint paperTexture_ = 0;
    GLuint overlayTexture_ = 0;
};