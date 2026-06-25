#pragma once

#include "ui_panel.h"
#include "../game/npc.h"
#include "text_renderer.h"
#include <memory>
#include <array>

/**
 * @brief キャラクターシートUI
 *
 * Phase 10: プレイヤー属性・スキル・能力値の表示と管理UI
 * Oblivion風デザインで、属性値、スキル、一時的な効果を表示
 */
class UICharacterSheet : public UIPanel {
public:
    enum class CharacterTab {
        ATTRIBUTES = 0,  // 属性値（力、知力など）
        SKILLS = 1,      // スキル（刀剣、魔法など）
        EFFECTS = 2      // 一時的な効果とボーナス
    };

    explicit UICharacterSheet(const std::string& title = "Character");
    ~UICharacterSheet() override = default;

    // Initialize with character status and text renderer
    bool initialize(CharacterStatus* playerStatus, TextRenderer* textRenderer);

    // Set character to display (for viewing other NPCs)
    void setCharacter(CharacterStatus* charStatus);

    // Lifecycle
    void update(float deltaTime) override;
    bool onTouchDown(float x, float y, int pointerId) override;
    void render() override;

    // Tab control
    void setActiveTab(CharacterTab tab);
    CharacterTab getActiveTab() const { return activeTab; }

    // Screen size for coordinate calculations
    void setScreenSize(int w, int h) { screenWidth = w; screenHeight = h; }

private:
    CharacterStatus* characterStatus = nullptr;
    TextRenderer* textRenderer = nullptr;
    CharacterTab activeTab = CharacterTab::ATTRIBUTES;

    int screenWidth = 1080;
    int screenHeight = 1920;

    // Tab rendering functions
    void renderAttributesTab();
    void renderSkillsTab();
    void renderEffectsTab();
    void renderTabButtons();

    // Helper functions
    void renderAttributeRow(float x, float y, const std::string& attrName, float value);
    void renderSkillRow(float x, float y, const std::string& skillName, float value, float maxValue);
    void renderStatDisplay(float x, float y, const std::string& label, float current, float max);

    // Hit testing for tab buttons
    int hitTestTabButton(float x, float y) const;
};
