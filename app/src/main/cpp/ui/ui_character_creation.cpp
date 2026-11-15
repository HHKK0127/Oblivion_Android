#include "ui_character_creation.h"
#include "placeholder_assets.h"
#include "ui_draw_helper.h"
#include <GLES3/gl3.h>
#include <cstdio>
#include <algorithm>

// The confirm/cancel buttons are drawn by renderConfirmCancelButtons() and hit
// tested by hitTestConfirmButton()/hitTestCancelButton(). Both sides read these
// constants and the two helpers below, so the touch box and the drawn box come
// from one source and cannot drift apart: the hit tests used to hardcode 105/32
// and a 226 px offset that only matched because 105 + 16 + 105 happened to
// equal it.
namespace {
constexpr float kConfirmCancelWidth = 105.0f;
constexpr float kConfirmCancelHeight = 32.0f;
constexpr float kConfirmCancelSpacing = 16.0f;

float cancel_button_x(float contentX, float contentWidth) {
    return contentX + contentWidth - kConfirmCancelWidth;
}

float confirm_button_x(float contentX, float contentWidth) {
    return cancel_button_x(contentX, contentWidth)
        - kConfirmCancelSpacing - kConfirmCancelWidth;
}
}  // namespace

UICharacterCreation::UICharacterCreation(const std::string& title)
    : UIPanel(title.empty() ? "Character Creation" : title) {
    setBackgroundColor(glm::vec4(
        PlaceholderAssets::Colors::PARCHMENT_DARK.x * 0.85f,
        PlaceholderAssets::Colors::PARCHMENT_DARK.y * 0.8f,
        PlaceholderAssets::Colors::PARCHMENT_DARK.z * 0.75f, 0.97f));
    setBorderColor(glm::vec4(
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT.x,
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT.y,
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT.z, 1.0f));
    setBorderWidth(3.0f);
    setTitleBarColor(glm::vec4(0.2f, 0.15f, 0.1f, 0.85f));
    setCloseButtonVisible(false);
    setDraggable(false);
}

bool UICharacterCreation::initialize(TextRenderer* tr) {
    if (!tr) return false;
    textRenderer = tr;
    initializeDefaultCharacter();
    if (!UIPanel::initialize()) return false;
    // UIComponent defaults to visible; the panel is only shown by open(). Left
    // visible it would be an invisible modal that swallows every touch event
    // in Renderer::onTouchEvent and keep anyMenuOpen() permanently true.
    setVisible(false);
    return true;
}

void UICharacterCreation::initializeDefaultCharacter() {
    createdCharacter_.name = "Adventurer";
    // createdCharacter_.level = 1;
    createdCharacter_.currentHealth = 100;
    createdCharacter_.maxHealth = 100;
    createdCharacter_.currentMana = 50;
    createdCharacter_.maxMana = 50;
    createdCharacter_.stamina = 100;
    createdCharacter_.maxStamina = 100;

    // Set default attributes (50 each)
    for (int i = 0; i < 8; ++i) {
        createdCharacter_.attributes[ATTRIBUTE_NAMES[i]] = 50.0f;
    }

    // Set default skills
    for (int i = 0; i < static_cast<int>(SKILL_NAMES.size()); ++i) {
        createdCharacter_.skills[SKILL_NAMES[i]] = 15.0f;
    }

    playerName_ = "Adventurer";
    nameCaretPos_ = playerName_.length();
}

void UICharacterCreation::open() {
    setVisible(true);
    currentTab = NAME;
    nameInputActive_ = true;
    loadRacesFromESM();
}

void UICharacterCreation::close() {
    setVisible(false);
    nameInputActive_ = false;
}

void UICharacterCreation::update(float deltaTime) {
    UIPanel::update(deltaTime);
}

bool UICharacterCreation::onTouchDown(float x, float y, int pointerId) {
    if (!isVisible() || !isEnabled()) return false;

    // Tab switching
    int tabIdx = hitTestTabButton(x, y);
    if (tabIdx >= 0) {
        currentTab = static_cast<Tab>(tabIdx);
        nameInputActive_ = (currentTab == NAME);
        selectedAttributeIndex_ = 0;
        selectedSkillIndex_ = 0;
        return true;
    }

    // Confirm/Cancel buttons
    if (hitTestConfirmButton(x, y)) {
        createdCharacter_.name = playerName_;
        if (onConfirm) {
            onConfirm(createdCharacter_);
        }
        return true;
    }

    if (hitTestCancelButton(x, y)) {
        close();
        return true;
    }

    // Tab-specific input
    switch (currentTab) {
    case NAME: {
        // Text input for character name handled via virtual keyboard
        LOGI("Name tab: text input via virtual keyboard");
        break;
    }
    case RACE: {
        int raceIdx;
        if (hitTestRaceRow(x, y, raceIdx)) {
            selectedRaceIndex_ = raceIdx;
            applyRaceBonuses();
            return true;
        }
        break;
    }
    case ATTRIBUTES: {
        if (hitTestIncreaseAttribute(x, y)) {
            if (selectedAttributeIndex_ < 8) {
                createdCharacter_.attributes[ATTRIBUTE_NAMES[selectedAttributeIndex_]]++;
            }
            return true;
        }
        if (hitTestDecreaseAttribute(x, y)) {
            if (selectedAttributeIndex_ < 8 && createdCharacter_.attributes[ATTRIBUTE_NAMES[selectedAttributeIndex_]] > 1) {
                createdCharacter_.attributes[ATTRIBUTE_NAMES[selectedAttributeIndex_]]--;
            }
            return true;
        }
        int attrIdx;
        if (hitTestAttributeRow(x, y, attrIdx)) {
            selectedAttributeIndex_ = attrIdx;
            return true;
        }
        break;
    }
    case SKILLS: {
        int skillIdx;
        if (hitTestSkillRow(x, y, skillIdx)) {
            selectedSkillIndex_ = skillIdx;
            return true;
        }
        if (hitTestToggleSkill(x, y)) {
            if (selectedSkillIndex_ < static_cast<int>(SKILL_NAMES.size())) {
                std::string skillName = SKILL_NAMES[selectedSkillIndex_];
                createdCharacter_.skills[skillName] =
                    (createdCharacter_.skills[skillName] > 15) ? 15 : 50;
            }
            return true;
        }
        break;
    }
    case APPEARANCE: {
        // Appearance customization (hair, face, etc.) - preview rendered in 3D
        LOGI("Appearance tab: customization options");
        break;
    }
    }

    return true;  // Consume all touches while open
}

void UICharacterCreation::render() {
    if (!isVisible()) return;

    GLboolean depthTestEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    UIPanel::render();

    renderTabButtons();

    switch (currentTab) {
    case NAME:
        renderNameTab();
        break;
    case RACE:
        renderRaceTab();
        break;
    case ATTRIBUTES:
        renderAttributesTab();
        break;
    case SKILLS:
        renderSkillsTab();
        break;
    case APPEARANCE:
        renderAppearanceTab();
        break;
    }

    renderConfirmCancelButtons();

    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
}

void UICharacterCreation::renderTabButtons() {
    if (!textRenderer) return;
    glm::vec2 cp = getContentPosition();
    float tabW = getContentSize().x / 5.0f;
    float tabH = 32.0f;
    float ty = getTabY();

    const char* tabs[] = { "Name", "Race", "Attributes", "Skills", "Appearance" };
    for (int i = 0; i < 5; ++i) {
        glm::vec3 bgColor = (currentTab == i)
                             ? glm::vec3(PlaceholderAssets::Colors::GOLD_HIGHLIGHT)
                             : glm::vec3(PlaceholderAssets::Colors::BROWN_ACCENT);
        PlaceholderAssets::drawPanel(cp.x + i * tabW, ty, tabW - 1.0f, tabH, bgColor,
            glm::vec3(PlaceholderAssets::Colors::GOLD_HIGHLIGHT));
        textRenderer->renderText(tabs[i],
            cp.x + i * tabW + 4.0f, ty + 6.0f,
            glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT), 0.6f);
    }
}

void UICharacterCreation::renderNameTab() {
    if (!textRenderer) return;
    glm::vec2 cp = getContentPosition();
    float cy = getContentStartY();

    // Name input label
    PlaceholderAssets::drawPanel(cp.x + 20.0f, cy, getContentSize().x - 40.0f, 40.0f,
        glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT) * 0.9f,
        glm::vec3(PlaceholderAssets::Colors::BROWN_ACCENT));
    textRenderer->renderText("Character Name:",
        cp.x + 30.0f, cy + 4.0f,
        glm::vec3(0.1f, 0.08f, 0.06f), 0.65f);

    // Name input box
    PlaceholderAssets::drawPanel(cp.x + 20.0f, cy + 45.0f, getContentSize().x - 40.0f, 48.0f,
        glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT) * 0.75f,
        glm::vec3(PlaceholderAssets::Colors::GOLD_HIGHLIGHT));
    textRenderer->renderText(playerName_,
        cp.x + 30.0f, cy + 57.0f,
        glm::vec3(0.1f, 0.08f, 0.06f), 0.75f);

    // Info text
    textRenderer->renderText("Enter your character's name to begin your journey.",
        cp.x + 30.0f, cy + 110.0f,
        glm::vec3(0.3f, 0.2f, 0.1f), 0.6f);
}

void UICharacterCreation::renderAttributesTab() {
    if (!textRenderer) return;
    glm::vec2 cp = getContentPosition();
    float cy = getContentStartY();
    float rowH = 36.0f;
    float rowGap = 2.0f;

    PlaceholderAssets::drawPanel(cp.x + 20.0f, cy, getContentSize().x - 40.0f, 20.0f,
        PlaceholderAssets::Colors::BROWN_ACCENT,
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT);
    textRenderer->renderText("Attributes (Click to select, +/- to adjust)",
        cp.x + 30.0f, cy + 2.0f,
        glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT), 0.6f);
    cy += 25.0f;

    for (int i = 0; i < 8; ++i) {
        bool sel = (i == selectedAttributeIndex_);
        glm::vec3 bg = sel
                       ? glm::vec3(PlaceholderAssets::Colors::PARCHMENT_DARK)
                       : glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT) * 0.85f;
        PlaceholderAssets::drawPanel(cp.x + 20.0f, cy, getContentSize().x - 40.0f, rowH, bg,
            glm::vec3(sel ? PlaceholderAssets::Colors::GOLD_HIGHLIGHT
                         : PlaceholderAssets::Colors::BROWN_ACCENT));

        char attrStr[64];
        snprintf(attrStr, sizeof(attrStr), "%s: %.0f",
                 ATTRIBUTE_NAMES[i], createdCharacter_.attributes[ATTRIBUTE_NAMES[i]]);
        textRenderer->renderText(attrStr,
            cp.x + 30.0f, cy + 10.0f,
            sel ? glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT)
                : glm::vec3(0.1f, 0.08f, 0.06f),
            0.65f);

        cy += rowH + rowGap;
    }
}

void UICharacterCreation::renderSkillsTab() {
    if (!textRenderer) return;
    glm::vec2 cp = getContentPosition();
    float cy = getContentStartY();
    float rowH = 32.0f;
    float rowGap = 1.0f;

    PlaceholderAssets::drawPanel(cp.x + 20.0f, cy, getContentSize().x - 40.0f, 20.0f,
        PlaceholderAssets::Colors::BROWN_ACCENT,
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT);
    textRenderer->renderText("Skills (Specialized skills start at 50)",
        cp.x + 30.0f, cy + 2.0f,
        glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT), 0.55f);
    cy += 25.0f;

    for (int i = 0; i < static_cast<int>(SKILL_NAMES.size()); ++i) {
        bool sel = (i == selectedSkillIndex_);
        glm::vec3 bg = sel
                       ? glm::vec3(PlaceholderAssets::Colors::PARCHMENT_DARK)
                       : glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT) * 0.85f;
        PlaceholderAssets::drawPanel(cp.x + 20.0f, cy, getContentSize().x - 40.0f, rowH, bg,
            glm::vec3(sel ? PlaceholderAssets::Colors::GOLD_HIGHLIGHT
                         : PlaceholderAssets::Colors::BROWN_ACCENT));

        char skillStr[64];
        snprintf(skillStr, sizeof(skillStr), "%s: %.0f",
                 SKILL_NAMES[i], createdCharacter_.skills[SKILL_NAMES[i]]);
        textRenderer->renderText(skillStr,
            cp.x + 30.0f, cy + 6.0f,
            sel ? glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT)
                : glm::vec3(0.1f, 0.08f, 0.06f),
            0.6f);

        cy += rowH + rowGap;
    }
}

void UICharacterCreation::renderAppearanceTab() {
    if (!textRenderer) return;
    glm::vec2 cp = getContentPosition();
    float cy = getContentStartY();

    textRenderer->renderText("Appearance customization coming soon...",
        cp.x + 30.0f, cy + 20.0f,
        glm::vec3(0.3f, 0.2f, 0.1f), 0.65f);
}

void UICharacterCreation::renderRaceTab() {
    if (!textRenderer) return;
    glm::vec2 cp = getContentPosition();
    float cy = getContentStartY();
    float rowH = 40.0f;
    float rowGap = 2.0f;

    PlaceholderAssets::drawPanel(cp.x + 20.0f, cy, getContentSize().x - 40.0f, 20.0f,
        PlaceholderAssets::Colors::BROWN_ACCENT,
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT);
    textRenderer->renderText("Select Race (Bonuses applied automatically)",
        cp.x + 30.0f, cy + 2.0f,
        glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT), 0.6f);
    cy += 25.0f;

    if (availableRaces_.empty()) {
        textRenderer->renderText("No races available (ESM data not loaded)",
            cp.x + 30.0f, cy + 10.0f,
            glm::vec3(0.5f, 0.3f, 0.2f), 0.6f);
        return;
    }

    for (int i = 0; i < static_cast<int>(availableRaces_.size()); ++i) {
        bool sel = (i == selectedRaceIndex_);
        glm::vec3 bg = sel
                       ? glm::vec3(PlaceholderAssets::Colors::PARCHMENT_DARK)
                       : glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT) * 0.85f;
        PlaceholderAssets::drawPanel(cp.x + 20.0f, cy, getContentSize().x - 40.0f, rowH, bg,
            glm::vec3(sel ? PlaceholderAssets::Colors::GOLD_HIGHLIGHT
                         : PlaceholderAssets::Colors::BROWN_ACCENT));

        // Race name
        textRenderer->renderText(availableRaces_[i].fullName,
            cp.x + 30.0f, cy + 4.0f,
            sel ? glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT)
                : glm::vec3(0.1f, 0.08f, 0.06f),
            0.7f);

        // Race bonuses summary
        char bonusStr[128] = "";
        if (availableRaces_[i].startingHealth > 0) {
            snprintf(bonusStr, sizeof(bonusStr), "HP+%u", availableRaces_[i].startingHealth);
        }
        textRenderer->renderText(bonusStr,
            cp.x + 30.0f, cy + 22.0f,
            glm::vec3(0.4f, 0.3f, 0.2f), 0.5f);

        cy += rowH + rowGap;
    }
}

void UICharacterCreation::renderConfirmCancelButtons() {
    if (!textRenderer) return;
    glm::vec2 cp = getContentPosition();
    float by = getConfirmButtonY();
    float buttonW = kConfirmCancelWidth;
    float buttonH = kConfirmCancelHeight;
    float cancelX = cancel_button_x(cp.x, getContentSize().x);
    float confirmX = confirm_button_x(cp.x, getContentSize().x);
    constexpr float kLabelScale = 0.65f;

    // Keep each label inside its panel. The label width is measured with the
    // same scale and advance drawOblivionText uses, so what is measured is what
    // is drawn -- but TextRenderer::fontSizeMultiplier_ defaults to 1.15
    // (text_renderer.h:246) and TitleScreen restores that saved value before the
    // creation panel opens (title_screen.cpp:515 gate, :567 restore), so the
    // runtime widths are 108.46 px for "Create" and 106.55 px for "Cancel" on a
    // 105 px panel. The old fixed insets made that 21.46 px and 18.55 px of
    // overshoot; the multiplier is a user setting, so a fixed panel or a fixed
    // scale cannot be made to fit for every value. Step the label scale down
    // instead, which keeps the whole word (truncating, as debug_menu.cpp:642
    // does, would turn "Create" into "Creat") and keeps the panel and the touch
    // box at their existing size.
    auto drawLabel = [&](const char* label, float panelX) {
        constexpr float kInset = 4.0f;
        // fontSizeMultiplier_ comes from the `fontsize` console command
        // (game_console.cpp:863), which the setter clamps to [0.5, 2.0]
        // (text_renderer.cpp:1162), so every value reachable today already
        // fits. The loop is bounded by step count rather than by a scale floor
        // so that widening that clamp later cannot bring the overflow back:
        // 0.95^48 is ~0.085, far past any setting that leaves the rest of the
        // UI readable, and the label always reaches maxWidth before that.
        constexpr int kMaxShrinkSteps = 48;
        const float maxWidth = buttonW - 2.0f * kInset;
        float labelScale = kLabelScale;
        for (int i = 0; i < kMaxShrinkSteps &&
             textRenderer->getTextWidth(label, labelScale) > maxWidth; ++i) {
            labelScale *= 0.95f;
        }
        float textW = textRenderer->getTextWidth(label, labelScale);
        float inset = (buttonW - textW) * 0.5f;
        if (inset < 0.0f) inset = 0.0f;   // never start left of the panel edge
        textRenderer->renderText(label, panelX + inset, by + 6.0f,
            glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT), labelScale);
    };

    // Confirm button
    PlaceholderAssets::drawPanel(confirmX, by, buttonW, buttonH,
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT,
        glm::vec3(PlaceholderAssets::Colors::GOLD_HIGHLIGHT));
    drawLabel("Create", confirmX);

    // Cancel button
    PlaceholderAssets::drawPanel(cancelX, by, buttonW, buttonH,
        PlaceholderAssets::Colors::BROWN_ACCENT,
        glm::vec3(PlaceholderAssets::Colors::GOLD_HIGHLIGHT));
    drawLabel("Cancel", cancelX);
}

// ─── hit testing ──────────────────────────────────────────────────────────────

int UICharacterCreation::hitTestTabButton(float x, float y) const {
    glm::vec2 cp = getContentPosition();
    float tabW = getContentSize().x / 5.0f;
    float tabH = 32.0f;
    float ty = getTabY();

    for (int i = 0; i < 5; ++i) {
        if (x >= cp.x + i * tabW && x <= cp.x + (i + 1) * tabW &&
            y >= ty && y <= ty + tabH) {
            return i;
        }
    }
    return -1;
}

bool UICharacterCreation::hitTestConfirmButton(float x, float y) const {
    glm::vec2 cp = getContentPosition();
    float by = getConfirmButtonY();
    float buttonX = confirm_button_x(cp.x, getContentSize().x);

    return x >= buttonX && x <= buttonX + kConfirmCancelWidth &&
           y >= by && y <= by + kConfirmCancelHeight;
}

bool UICharacterCreation::hitTestCancelButton(float x, float y) const {
    glm::vec2 cp = getContentPosition();
    float by = getConfirmButtonY();
    float buttonX = cancel_button_x(cp.x, getContentSize().x);

    return x >= buttonX && x <= buttonX + kConfirmCancelWidth &&
           y >= by && y <= by + kConfirmCancelHeight;
}

bool UICharacterCreation::hitTestAttributeRow(float x, float y, int& outIndex) const {
    glm::vec2 cp = getContentPosition();
    float cy = getContentStartY() + 25.0f;
    float rowH = 36.0f;
    float rowGap = 2.0f;

    for (int i = 0; i < 8; ++i) {
        float ry = cy + i * (rowH + rowGap);
        if (x >= cp.x + 20.0f && x <= cp.x + getContentSize().x - 20.0f &&
            y >= ry && y <= ry + rowH) {
            outIndex = i;
            return true;
        }
    }
    return false;
}

bool UICharacterCreation::hitTestSkillRow(float x, float y, int& outIndex) const {
    glm::vec2 cp = getContentPosition();
    float cy = getContentStartY() + 25.0f;
    float rowH = 32.0f;
    float rowGap = 1.0f;

    for (int i = 0; i < static_cast<int>(SKILL_NAMES.size()); ++i) {
        float ry = cy + i * (rowH + rowGap);
        if (x >= cp.x + 20.0f && x <= cp.x + getContentSize().x - 20.0f &&
            y >= ry && y <= ry + rowH) {
            outIndex = i;
            return true;
        }
    }
    return false;
}

bool UICharacterCreation::hitTestIncreaseAttribute(float x, float y) const {
    // + button for attribute increase (right side of attribute row)
    float buttonX = 0.65f;
    float buttonW = 0.04f;
    float buttonH = 0.03f;
    for (int i = 0; i < 8; i++) {
        float buttonY = 0.25f + i * 0.06f;
        if (x >= buttonX && x <= buttonX + buttonW &&
            y >= buttonY && y <= buttonY + buttonH) {
            return true;
        }
    }
    return false;
}

bool UICharacterCreation::hitTestDecreaseAttribute(float x, float y) const {
    // - button for attribute decrease (left side of attribute row)
    float buttonX = 0.55f;
    float buttonW = 0.04f;
    float buttonH = 0.03f;
    for (int i = 0; i < 8; i++) {
        float buttonY = 0.25f + i * 0.06f;
        if (x >= buttonX && x <= buttonX + buttonW &&
            y >= buttonY && y <= buttonY + buttonH) {
            return true;
        }
    }
    return false;
}

bool UICharacterCreation::hitTestToggleSkill(float x, float y) const {
    // Skill toggle buttons (left column of skills list)
    float skillX = 0.1f;
    float skillW = 0.35f;
    float skillH = 0.04f;
    for (int i = 0; i < 10; i++) {
        float skillY = 0.25f + i * 0.06f;
        if (x >= skillX && x <= skillX + skillW &&
            y >= skillY && y <= skillY + skillH) {
            return true;
        }
    }
    return false;
}

bool UICharacterCreation::hitTestRaceRow(float x, float y, int& outIndex) const {
    glm::vec2 cp = getContentPosition();
    float cy = getContentStartY() + 25.0f;
    float rowH = 40.0f;
    float rowGap = 2.0f;

    for (int i = 0; i < static_cast<int>(availableRaces_.size()); ++i) {
        float ry = cy + i * (rowH + rowGap);
        if (x >= cp.x + 20.0f && x <= cp.x + getContentSize().x - 20.0f &&
            y >= ry && y <= ry + rowH) {
            outIndex = i;
            return true;
        }
    }
    return false;
}

// ─── layout helpers ───────────────────────────────────────────────────────────

float UICharacterCreation::getTabY() const {
    return getContentPosition().y + 4.0f;
}

float UICharacterCreation::getContentStartY() const {
    return getContentPosition().y + 40.0f;
}

float UICharacterCreation::getConfirmButtonY() const {
    return getContentPosition().y + getContentSize().y - 44.0f;
}

void UICharacterCreation::addSkillPoints(int skillId, int points) {
    if (skillId >= 0 && skillId < static_cast<int>(SKILL_NAMES.size())) {
        std::string skillName = SKILL_NAMES[skillId];
        createdCharacter_.skills[skillName] = std::min(100.0f, createdCharacter_.skills[skillName] + points);
    }
}

void UICharacterCreation::ensureValidCharacter() {
    // Ensure attributes are within valid range
    for (int i = 0; i < 8; ++i) {
        std::string attrName = ATTRIBUTE_NAMES[i];
        createdCharacter_.attributes[attrName] = std::clamp(createdCharacter_.attributes[attrName], 1.0f, 100.0f);
    }

    // Ensure skills are within valid range
    for (int i = 0; i < static_cast<int>(SKILL_NAMES.size()); ++i) {
        std::string skillName = SKILL_NAMES[i];
        createdCharacter_.skills[skillName] = std::clamp(createdCharacter_.skills[skillName], 5.0f, 100.0f);
    }

    if (playerName_.empty()) {
        playerName_ = "Adventurer";
    }
    createdCharacter_.name = playerName_;
}

void UICharacterCreation::loadRacesFromESM() {
    availableRaces_.clear();
    if (!esmManager) return;

    // Get all races from ESM
    const auto& races = esmManager->getAllRaces();
    for (const auto& race : races) {
        if (race.fullName.empty() && race.editorID.empty()) {
            continue;
        }

        oblivion::RaceData displayRace = race;
        if (displayRace.fullName.empty()) {
            displayRace.fullName = displayRace.editorID;
        }
        availableRaces_.push_back(std::move(displayRace));
    }

    if (!availableRaces_.empty()) {
        selectedRaceIndex_ = 0;
        applyRaceBonuses();
    }
}

void UICharacterCreation::applyRaceBonuses() {
    if (availableRaces_.empty() || selectedRaceIndex_ < 0 ||
        selectedRaceIndex_ >= static_cast<int>(availableRaces_.size())) {
        return;
    }

    const auto& race = availableRaces_[selectedRaceIndex_];

    // Reset attributes to base values
    for (int i = 0; i < 8; ++i) {
        createdCharacter_.attributes[ATTRIBUTE_NAMES[i]] = 50.0f;
    }

    // Apply race attribute bonuses
    createdCharacter_.attributes["Strength"] += static_cast<float>(race.attrStrength);
    createdCharacter_.attributes["Intelligence"] += static_cast<float>(race.attrIntelligence);
    createdCharacter_.attributes["Willpower"] += static_cast<float>(race.attrWillpower);
    createdCharacter_.attributes["Agility"] += static_cast<float>(race.attrAgility);
    createdCharacter_.attributes["Speed"] += static_cast<float>(race.attrSpeed);
    createdCharacter_.attributes["Endurance"] += static_cast<float>(race.attrEndurance);
    createdCharacter_.attributes["Personality"] += static_cast<float>(race.attrPersonality);

    // Apply race skill bonuses
    for (const auto& bonus : race.skillBonuses) {
        // Skill bonuses use formID, need to look up skill name
        // For now, skip skill bonuses that use formID
        (void)bonus;
    }

    // Apply race HP bonus
    createdCharacter_.maxHealth += static_cast<float>(race.startingHealth);
    createdCharacter_.currentHealth = createdCharacter_.maxHealth;

    LOGD("Applied race bonuses for: %s", race.fullName.c_str());
}
