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

    void setScreenSize(int w, int h) {
        screenWidth = w;
        screenHeight = h;

        const float panelWidth = static_cast<float>(w) * 0.72f;
        const float panelHeight = static_cast<float>(h) * 0.82f;
        setSize(panelWidth, panelHeight);
        setPosition((static_cast<float>(w) - panelWidth) * 0.5f,
                    (static_cast<float>(h) - panelHeight) * 0.5f);
    }

    // Callback when character creation is confirmed
    void setOnConfirm(ConfirmCallback cb) { onConfirm = cb; }

    // Fired when Create is pressed with a name the original's Done band
    // rejects (RE v370): the owner shows the sNoNameCharacter notification
    // and the panel stays open instead of transitioning.
    using RejectCallback = std::function<void()>;
    void setOnNameRejected(RejectCallback cb) { onNameRejected = std::move(cb); }

    // Get created character
    const CharacterStatus& getCreatedCharacter() const { return createdCharacter_; }

    // Set ESM manager for race/class data
    void setESMManager(const oblivion::ESMManager* esm) { esmManager = esm; }

    // Soft-keyboard name entry. The NAME tab owns this buffer; the Android side
    // pushes every edit here so the field updates as it is typed, which is how
    // the original writes the name (RE task #123: built-in editor, realtime
    // write to the player). MainActivity polls isNameInputActive() to decide
    // whether the IME belongs to this panel or to the debug console.
    void setPlayerName(const std::string& name);
    const std::string& getPlayerName() const { return playerName_; }

    // True while the NAME tab owns the keyboard. open() starts here and tab
    // switching follows the selected tab (see onTouchDown).
    bool isNameInputActive() const { return nameInputActive_; }

private:
    TextRenderer* textRenderer = nullptr;
    ConfirmCallback onConfirm;
    RejectCallback onNameRejected;
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
    // Frames since the last attempt to refill availableRaces_ from the ESM. The
    // list is read from ESMManager, which the parse worker publishes into without
    // a lock, so it is polled at most about once a second instead of per frame.
    int raceRetryTicks_ = 0;
    void loadRacesFromESM();

    // For ATTRIBUTES tab
    int selectedAttributeIndex_ = 0;
    static constexpr std::array<const char*, 8> ATTRIBUTE_NAMES = {{
        "Strength", "Intelligence", "Willpower", "Agility",
        "Speed", "Endurance", "Personality", "Luck"
    }};

    // For SKILLS tab
    int selectedSkillIndex_ = 0;
    // Canonical 21-skill set in AV index order 0x0c..0x20 (RE_EXE_REPORT §46.1,
    // name table at 0x00B0A1A8). This matches the engine's 21-iteration chargen
    // skill loop documented in s58_chargen.md §1.7. Display spellings keep the
    // previous UI strings ("Hand-to-Hand", spaced armor skills).
    static constexpr std::array<const char*, 21> SKILL_NAMES = {{
        "Armorer", "Athletics", "Blade", "Block", "Blunt",
        "Hand-to-Hand", "Heavy Armor", "Alchemy", "Alteration", "Conjuration",
        "Destruction", "Illusion", "Mysticism", "Restoration",
        "Acrobatics", "Light Armor", "Marksman", "Mercantile",
        "Security", "Sneak", "Speechcraft"
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
    bool hasConfirmableName() const;
    void applyRaceBonuses();
};
