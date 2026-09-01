#pragma once

#include <glm/glm.hpp>
#include <GLES3/gl3.h>
#include <memory>
#include <string>

class TextRenderer;
class UISystem;

/**
 * @brief In-game HUD renderer
 *
 * Phase 9: Renders player status, minimap, quick slots, etc. on the game screen
 * Overlay HUD system. Uses PlaceholderAssets for placeholder display.
 */
class HUDRenderer {
public:
    HUDRenderer();
    ~HUDRenderer();

    /**
     * @brief Initialize HUD renderer
     * @param textRenderer Pointer to text renderer
     * @param uiSystem Pointer to UI system (optional)
     * @param screenWidth Screen width
     * @param screenHeight Screen height
     */
    bool initialize(TextRenderer* textRenderer, UISystem* uiSystem = nullptr,
                   int screenWidth = 1080, int screenHeight = 1920);

    /**
     * @brief Update HUD
     * @param deltaTime Frame elapsed time (seconds)
     */
    void update(float deltaTime);

    /**
     * @brief Render HUD
     */
    void render();

    /**
     * @brief Cleanup
     */
    void cleanup();

    /**
     * @brief Call when screen resolution changes
     */
    void onScreenResize(int width, int height);

    // === Show/Hide ===

    void setVisible(bool visible) { this->visible = visible; }
    bool isVisible() const { return visible; }

    // === Player Status Settings ===

    /**
     * @brief Set player HP
     * @param current Current HP
     * @param max Max HP
     */
    void setPlayerHealth(float current, float max);

    /**
     * @brief Set player MP
     * @param current Current MP
     * @param max Max MP
     */
    void setPlayerMana(float current, float max);

    /**
     * @brief Set player stamina
     * @param current Current stamina
     * @param max Max stamina
     */
    void setPlayerStamina(float current, float max);

    /**
     * @brief Set player level
     */
    void setPlayerLevel(int level) { playerLevel = level; }

    /**
     * @brief Set player position (for minimap)
     */
    void setPlayerPosition(const glm::vec3& pos) { playerPosition = pos; }

    // === Quick Slot Settings ===

    /**
     * @brief Set quick slot item
     * @param slotIndex Slot index (0-9)
     * @param itemName Item name
     */
    void setQuickSlotItem(int slotIndex, const std::string& itemName);

    /**
     * @brief Clear quick slots
     */
    void clearQuickSlots();

    // === Layout Settings ===

    /**
     * @brief Set status bar position (default: bottom-left)
     */
    void setStatusBarPosition(float x, float y);

    /**
     * @brief Set minimap position (default: top-right)
     */
    void setMinimapPosition(float x, float y);

    /**
     * @brief Set quick slot position (default: bottom-center)
     */
    void setQuickSlotPosition(float x, float y);

private:
    // Initialization state
    bool initialized = false;
    bool visible = true;

    // Text renderer
    TextRenderer* textRenderer = nullptr;
    UISystem* uiSystem = nullptr;

    // Screen size
    int screenWidth = 1080;
    int screenHeight = 1920;

    // === Player Status ===

    float healthCurrent = 100.0f;
    float healthMax = 100.0f;
    float manaCurrent = 50.0f;
    float manaMax = 50.0f;
    float staminaCurrent = 100.0f;
    float staminaMax = 100.0f;
    int playerLevel = 1;
    glm::vec3 playerPosition = glm::vec3(0.0f, 0.0f, 0.0f);

    // === Quick Slots ===

    static constexpr int NUM_QUICK_SLOTS = 10;
    std::string quickSlotItems[NUM_QUICK_SLOTS];

    // === Layout ===

    float statusBarX = 20.0f;
    float statusBarY = 0.0f;  // Updated during calculation
    float minimapX = 0.0f;    // Updated during calculation
    float minimapY = 20.0f;
    float quickSlotX = 0.0f;  // Updated during calculation
    float quickSlotY = 0.0f;  // Updated during calculation

    // === Helper Drawing Functions ===

    void renderStatusBars();
    void renderMinimap();
    void renderQuickSlots();
    void renderPlayerLevel();
};
