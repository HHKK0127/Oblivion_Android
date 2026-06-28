#include "ui_character_creation.h"
#include "placeholder_assets.h"
#include "ui_draw_helper.h"
#include <GLES3/gl3.h>
#include <cstdio>
#include <algorithm>

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
    return UIPanel::initialize();
}

void UICharacterCreation::initializeDefaultCharacter() {
    // createdCharacter_.name = "Adventurer";
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
    for (int i = 0; i < 20; ++i) {
        createdCharacter_.skills[SKILL_NAMES[i]] = 15.0f;
    }

    playerName_ = "Adventurer";
    nameCaretPos_ = playerName_.length();
}

void UICharacterCreation::open() {
    setVisible(true);
    currentTab = NAME;
    nameInputActive_ = true;
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

    // Tab切り替え
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
        // createdCharacter_.name = playerName_;
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
        // TODO: Implement text input for name
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
            if (selectedSkillIndex_ < 20) {
                std::string skillName = SKILL_NAMES[selectedSkillIndex_];
                createdCharacter_.skills[skillName] =
                    (createdCharacter_.skills[skillName] > 15) ? 15 : 50;
            }
            return true;
        }
        break;
    }
    case APPEARANCE: {
        // TODO: Implement appearance customization
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
    float tabW = getContentSize().x / 4.0f;
    float tabH = 32.0f;
    float ty = getTabY();

    const char* tabs[] = { "Name", "Attributes", "Skills", "Appearance" };
    for (int i = 0; i < 4; ++i) {
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

    for (int i = 0; i < 20; ++i) {
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

void UICharacterCreation::renderConfirmCancelButtons() {
    if (!textRenderer) return;
    glm::vec2 cp = getContentPosition();
    float by = getConfirmButtonY();
    float buttonW = 80.0f;
    float buttonH = 32.0f;
    float spacing = 20.0f;

    // Confirm button
    PlaceholderAssets::drawPanel(cp.x + getContentSize().x - 180.0f, by, buttonW, buttonH,
        PlaceholderAssets::Colors::GOLD_HIGHLIGHT,
        glm::vec3(PlaceholderAssets::Colors::GOLD_HIGHLIGHT));
    textRenderer->renderText("Create",
        cp.x + getContentSize().x - 170.0f, by + 6.0f,
        glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT), 0.65f);

    // Cancel button
    PlaceholderAssets::drawPanel(cp.x + getContentSize().x - 85.0f, by, buttonW, buttonH,
        PlaceholderAssets::Colors::BROWN_ACCENT,
        glm::vec3(PlaceholderAssets::Colors::GOLD_HIGHLIGHT));
    textRenderer->renderText("Cancel",
        cp.x + getContentSize().x - 75.0f, by + 6.0f,
        glm::vec3(PlaceholderAssets::Colors::PARCHMENT_LIGHT), 0.65f);
}

// ─── hit testing ──────────────────────────────────────────────────────────────

int UICharacterCreation::hitTestTabButton(float x, float y) const {
    glm::vec2 cp = getContentPosition();
    float tabW = getContentSize().x / 4.0f;
    float tabH = 32.0f;
    float ty = getTabY();

    for (int i = 0; i < 4; ++i) {
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
    float buttonX = cp.x + getContentSize().x - 180.0f;
    float buttonY = by;

    return x >= buttonX && x <= buttonX + 80.0f &&
           y >= buttonY && y <= buttonY + 32.0f;
}

bool UICharacterCreation::hitTestCancelButton(float x, float y) const {
    glm::vec2 cp = getContentPosition();
    float by = getConfirmButtonY();
    float buttonX = cp.x + getContentSize().x - 85.0f;
    float buttonY = by;

    return x >= buttonX && x <= buttonX + 80.0f &&
           y >= buttonY && y <= buttonY + 32.0f;
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

    for (int i = 0; i < 20; ++i) {
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
    // TODO: Implement + button hit test
    return false;
}

bool UICharacterCreation::hitTestDecreaseAttribute(float x, float y) const {
    // TODO: Implement - button hit test
    return false;
}

bool UICharacterCreation::hitTestToggleSkill(float x, float y) const {
    // TODO: Implement toggle skill hit test
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
    if (skillId >= 0 && skillId < 20) {
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
    for (int i = 0; i < 20; ++i) {
        std::string skillName = SKILL_NAMES[i];
        createdCharacter_.skills[skillName] = std::clamp(createdCharacter_.skills[skillName], 5.0f, 100.0f);
    }

    if (playerName_.empty()) {
        playerName_ = "Adventurer";
    }
    // createdCharacter_.name = playerName_;
}
