#pragma once

#include "text_renderer.h"
#include <glm/glm.hpp>
#include <string>

/**
 * @brief プレイヤー統計情報パネル
 *
 * Phase 24: ゲーム内HUD - 統計情報表示
 * 所持金、インベントリ重量、装備情報などを表示
 */
class UIPlayerStats {
public:
    struct EquipmentSlot {
        std::string itemName;
        int durability;
        int maxDurability;
        bool equipped;

        EquipmentSlot() : itemName("Empty"), durability(100),
                          maxDurability(100), equipped(false) {}
    };

    struct PlayerStats {
        int gold;
        float inventoryWeight;
        float maxCarryWeight;
        int activeSpells;
        int maxActiveSpells;
        EquipmentSlot armor[4];      // Head, Chest, Hands, Feet
        EquipmentSlot weapon;         // Main weapon
        std::string carryingStatus;   // "Normal", "Heavy", "Overencumbered"

        PlayerStats() : gold(0), inventoryWeight(0.0f), maxCarryWeight(100.0f),
                       activeSpells(0), maxActiveSpells(8) {}
    };

    UIPlayerStats();
    ~UIPlayerStats() = default;

    bool initialize(TextRenderer* textRenderer, int screenW, int screenH);

    void setPlayerStats(const PlayerStats& stats) { stats_ = stats; }
    void updateGold(int amount) { stats_.gold = amount; }
    void updateInventoryWeight(float weight) { stats_.inventoryWeight = weight; }
    void setEquipment(int slot, const std::string& itemName, int durability, int maxDurability);
    void setMainWeapon(const std::string& weaponName, int durability, int maxDurability);

    void update(float deltaTime);
    void render();

    void setScreenSize(int w, int h) {
        screenWidth = w;
        screenHeight = h;
    }

    bool isExpanded() const { return expanded_; }
    void toggleExpanded() { expanded_ = !expanded_; }

private:
    TextRenderer* textRenderer = nullptr;

    int screenWidth = 1080;
    int screenHeight = 1920;

    PlayerStats stats_;
    bool expanded_ = false;
    float expandAnimTime_ = 0.0f;

    static constexpr float PANEL_WIDTH = 200.0f;
    static constexpr float PANEL_HEIGHT = 100.0f;
    static constexpr float PANEL_EXPANDED_HEIGHT = 180.0f;
    static constexpr float EXPAND_DURATION = 0.3f;
    static constexpr float START_X = 30.0f;
    static constexpr float START_Y = 600.0f;  // Below other HUD elements

    static constexpr float WEIGHT_BAR_WIDTH = 160.0f;
    static constexpr float WEIGHT_BAR_HEIGHT = 10.0f;

    void renderCompactPanel();
    void renderExpandedPanel();
    void renderGoldInfo();
    void renderWeightBar();
    void renderEquipmentInfo();
    void renderCarryingStatus();

    glm::vec3 getWeightColor() const;
    std::string getCarryingStatusText() const;
    float getExpandedPanelHeight() const;
};
