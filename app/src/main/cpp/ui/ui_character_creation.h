#pragma once

#include "ui_panel.h"
#include "text_renderer.h"
#include "../game/npc.h"
#include <memory>
#include <array>
#include <functional>

/**
 * @brief キャラクター作成UI
 *
 * Phase 12: ゲーム開始時のキャラクターメイク画面
 * プレイヤー名、属性値、スキル選択を羊皮紙パネルで表示
 */
class UICharacterCreation : public UIPanel {
public:
    using ConfirmCallback = std::function<void(const CharacterStatus& character)>;

    explicit UICharacterCreation(const std::string& title = "Create Character");
    ~UICharacterCreation() override = default;

    bool initialize(TextRenderer* textRenderer);

    void open();
    void close();

    void update(float deltaTime) override;
    bool onTouchDown(float x, float y, int pointerId) override;
    void render() override;

    void setScreenSize(int w, int h) { screenWidth = w; screenHeight = h; }

    // Callback when character creation is confirmed
    void setOnConfirm(ConfirmCallback cb) { onConfirm = cb; }

    // Get created character
    const CharacterStatus& getCreatedCharacter() const { return createdCharacter_; }

private:
    TextRenderer* textRenderer = nullptr;
    ConfirmCallback onConfirm;

    enum Tab {
        NAME = 0,
        ATTRIBUTES = 1,
        SKILLS = 2,
        APPEARANCE = 3
    };

    Tab currentTab = NAME;
    CharacterStatus createdCharacter_;

    // For NAME tab
    std::string playerName_;
    bool nameInputActive_ = false;
    int nameCaretPos_ = 0;

    // For ATTRIBUTES tab
    int selectedAttributeIndex_ = 0;
    static constexpr std::array<const char*, 8> ATTRIBUTE_NAMES = {{
        "Strength", "Intelligence", "Willpower", "Agility",
        "Speed", "Endurance", "Personality", "Luck"
    }};

    // For SKILLS tab
    int selectedSkillIndex_ = 0;
    static constexpr std::array<const char*, 20> SKILL_NAMES = {{
        "Blade", "Blunt", "Marksman", "Hand-to-Hand",
        "Destruction", "Alteration", "Illusion", "Conjuration",
        "Mysticism", "Restoration", "Alchemy", "Unarmored",
        "Armor", "Block", "Heavy Armor", "Light Armor",
        "Marksman", "Sneak", "Acrobatics", "Athletics"
    }};

    int screenWidth  = 1080;
    int screenHeight = 1920;

    void renderNameTab();
    void renderAttributesTab();
    void renderSkillsTab();
    void renderAppearanceTab();
    void renderTabButtons();
    void renderConfirmCancelButtons();

    int  hitTestTabButton(float x, float y) const;
    bool hitTestConfirmButton(float x, float y) const;
    bool hitTestCancelButton(float x, float y) const;
    bool hitTestAttributeRow(float x, float y, int& outIndex) const;
    bool hitTestSkillRow(float x, float y, int& outIndex) const;
    bool hitTestIncreaseAttribute(float x, float y) const;
    bool hitTestDecreaseAttribute(float x, float y) const;
    bool hitTestToggleSkill(float x, float y) const;

    // Layout helpers
    float getTabY() const;
    float getContentStartY() const;
    float getConfirmButtonY() const;

    // Helper functions
    void initializeDefaultCharacter();
    void addSkillPoints(int skillId, int points);
    void ensureValidCharacter();
};
