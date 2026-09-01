#pragma once

#include "ui_panel.h"
#include "text_renderer.h"
#include "../game/npc.h"
#include "../assets/esm_reader.h"
#include <memory>
#include <array>
#include <functional>
#include <vector>

/**
 * @brief Character creation UI
 *
 * Phase 12: Character creation screen at game start
 * Displays player name, attribute values, and skill selection on parchment panel
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

    // Set ESM manager for race/class data
    void setESMManager(const oblivion::ESMManager* esm) { esmManager = esm; }

private:
    TextRenderer* textRenderer = nullptr;
    ConfirmCallback onConfirm;
    const oblivion::ESMManager* esmManager = nullptr;

    enum Tab {
        NAME = 0,
        RACE = 1,
        ATTRIBUTES = 2,
        SKILLS = 3,
        APPEARANCE = 4
    };

    Tab currentTab = NAME;
    CharacterStatus createdCharacter_;

    // For NAME tab
    std::string playerName_;
    bool nameInputActive_ = false;
    int nameCaretPos_ = 0;

    // For RACE tab
    int selectedRaceIndex_ = 0;
    std::vector<oblivion::RaceData> availableRaces_;
    void loadRacesFromESM();

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
    void renderRaceTab();
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
    bool hitTestRaceRow(float x, float y, int& outIndex) const;

    // Layout helpers
    float getTabY() const;
    float getContentStartY() const;
    float getConfirmButtonY() const;

    // Helper functions
    void initializeDefaultCharacter();
    void addSkillPoints(int skillId, int points);
    void ensureValidCharacter();
    void applyRaceBonuses();
};
