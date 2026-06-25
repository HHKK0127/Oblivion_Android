#include "ui_manager.h"
#include "../game/quest_manager.h"
#include "../game/spell_manager.h"
#include "../game/dialogue.h"

UIManager::UIManager() = default;

UIManager::~UIManager() {
    cleanup();
}

bool UIManager::initialize(TextRenderer* textRenderer,
                           QuestManager* questManager,
                           CharacterStatus* playerStatus) {
    if (!textRenderer) return false;

    textRenderer_ = textRenderer;
    questManager_ = questManager;

    // UICharacterSheet の初期化
    characterSheet_ = std::make_unique<UICharacterSheet>();
    if (!characterSheet_->initialize(playerStatus, textRenderer)) {
        return false;
    }
    characterSheet_->setScreenSize(screenWidth_, screenHeight_);
    characterSheet_->setVisible(false);

    // UISpellbook の初期化
    spellbook_ = std::make_unique<UISpellbook>();
    if (!spellbook_->initialize(playerStatus, textRenderer)) {
        return false;
    }
    spellbook_->setScreenSize(screenWidth_, screenHeight_);
    spellbook_->setVisible(false);

    // UIQuestLog の初期化
    questLog_ = std::make_unique<UIQuestLog>();
    if (!questLog_->initialize(questManager, textRenderer)) {
        return false;
    }
    questLog_->setScreenSize(screenWidth_, screenHeight_);
    questLog_->setVisible(false);

    // UIDialogue の初期化
    dialogue_ = std::make_unique<UIDialogue>();
    if (!dialogue_->initialize(textRenderer)) {
        return false;
    }
    dialogue_->setScreenSize(screenWidth_, screenHeight_);
    dialogue_->setVisible(false);

    // UIShop の初期化
    shop_ = std::make_unique<UIShop>();
    if (!shop_->initialize(textRenderer)) {
        return false;
    }
    shop_->setScreenSize(screenWidth_, screenHeight_);
    shop_->setVisible(false);

    // UICharacterCreation の初期化
    characterCreation_ = std::make_unique<UICharacterCreation>();
    if (!characterCreation_->initialize(textRenderer)) {
        return false;
    }
    characterCreation_->setScreenSize(screenWidth_, screenHeight_);
    characterCreation_->setVisible(false);

    // UIPauseMenu の初期化
    pauseMenu_ = std::make_unique<UIPauseMenu>();
    if (!pauseMenu_->initialize(textRenderer)) {
        return false;
    }
    pauseMenu_->setScreenSize(screenWidth_, screenHeight_);
    pauseMenu_->setVisible(false);

    // UIMessageBox の初期化
    messageBox_ = std::make_unique<UIMessageBox>();
    if (!messageBox_->initialize(textRenderer)) {
        return false;
    }
    messageBox_->setScreenSize(screenWidth_, screenHeight_);
    messageBox_->setVisible(false);

    // UIToast の初期化
    toast_ = std::make_unique<UIToast>();
    if (!toast_->initialize(textRenderer, screenWidth_, screenHeight_)) {
        return false;
    }

    // UIQuickSlotBar の初期化
    quickSlotBar_ = std::make_unique<UIQuickSlotBar>();
    if (!quickSlotBar_->initialize(textRenderer, screenWidth_, screenHeight_)) {
        return false;
    }

    // UIHudStatusDisplay の初期化
    hudStatusDisplay_ = std::make_unique<UIHudStatusDisplay>();
    if (!hudStatusDisplay_->initialize(textRenderer, screenWidth_, screenHeight_)) {
        return false;
    }
    if (playerStatus) {
        hudStatusDisplay_->setCharacter(playerStatus);
    }

    // UIHudCompass の初期化
    hudCompass_ = std::make_unique<UIHudCompass>();
    if (!hudCompass_->initialize(textRenderer, screenWidth_, screenHeight_)) {
        return false;
    }

    // UIFloatingText の初期化
    floatingText_ = std::make_unique<UIFloatingText>();
    if (!floatingText_->initialize(textRenderer, screenWidth_, screenHeight_)) {
        return false;
    }

    return true;
}

void UIManager::cleanup() {
    characterSheet_.reset();
    spellbook_.reset();
    questLog_.reset();
    dialogue_.reset();
    shop_.reset();
    characterCreation_.reset();
    pauseMenu_.reset();
    messageBox_.reset();
    toast_.reset();
    quickSlotBar_.reset();
    hudStatusDisplay_.reset();
    hudCompass_.reset();
    floatingText_.reset();
}

void UIManager::setPlayerStatus(CharacterStatus* playerStatus) {
    if (characterSheet_) {
        characterSheet_->setCharacter(playerStatus);
    }
    if (hudStatusDisplay_) {
        hudStatusDisplay_->setCharacter(playerStatus);
    }
}

void UIManager::update(float deltaTime) {
    if (characterSheet_) characterSheet_->update(deltaTime);
    if (spellbook_) spellbook_->update(deltaTime);
    if (questLog_) questLog_->update(deltaTime);
    if (dialogue_) dialogue_->update(deltaTime);
    if (shop_) shop_->update(deltaTime);
    if (characterCreation_) characterCreation_->update(deltaTime);
    if (pauseMenu_) pauseMenu_->update(deltaTime);
    if (messageBox_) messageBox_->update(deltaTime);
    if (toast_) toast_->update(deltaTime);
    if (quickSlotBar_) quickSlotBar_->update(deltaTime);
    if (hudStatusDisplay_) hudStatusDisplay_->update(deltaTime);
    if (hudCompass_) hudCompass_->update(deltaTime);
    if (floatingText_) floatingText_->update(deltaTime);
}

void UIManager::render() {
    // Render HUD elements first (always visible)
    if (hudCompass_) {
        hudCompass_->render();
    }
    if (hudStatusDisplay_) {
        hudStatusDisplay_->render();
    }
    if (quickSlotBar_) {
        quickSlotBar_->render();
    }

    // Render UIs in order (notifications are always on top)
    if (characterSheet_ && characterSheet_->isVisible()) {
        characterSheet_->render();
    }
    if (spellbook_ && spellbook_->isVisible()) {
        spellbook_->render();
    }
    if (questLog_ && questLog_->isVisible()) {
        questLog_->render();
    }
    if (shop_ && shop_->isVisible()) {
        shop_->render();
    }
    if (dialogue_ && dialogue_->isVisible()) {
        dialogue_->render();
    }
    if (pauseMenu_ && pauseMenu_->isVisible()) {
        pauseMenu_->render();
    }
    if (characterCreation_ && characterCreation_->isVisible()) {
        characterCreation_->render();
    }
    if (messageBox_ && messageBox_->isVisible()) {
        messageBox_->render();
    }
    if (toast_) {
        toast_->render();
    }

    // Floating text rendered last (on top of everything)
    if (floatingText_) {
        floatingText_->render();
    }
}

void UIManager::setScreenSize(int width, int height) {
    screenWidth_ = width;
    screenHeight_ = height;

    if (characterSheet_) characterSheet_->setScreenSize(width, height);
    if (spellbook_) spellbook_->setScreenSize(width, height);
    if (questLog_) questLog_->setScreenSize(width, height);
    if (dialogue_) dialogue_->setScreenSize(width, height);
    if (shop_) shop_->setScreenSize(width, height);
    if (characterCreation_) characterCreation_->setScreenSize(width, height);
    if (pauseMenu_) pauseMenu_->setScreenSize(width, height);
    if (messageBox_) messageBox_->setScreenSize(width, height);
    if (toast_) toast_->setScreenSize(width, height);
    if (quickSlotBar_) quickSlotBar_->setScreenSize(width, height);
    if (hudStatusDisplay_) hudStatusDisplay_->setScreenSize(width, height);
    if (hudCompass_) hudCompass_->setScreenSize(width, height);
    if (floatingText_) floatingText_->setScreenSize(width, height);
}

bool UIManager::onTouchDown(float x, float y, int pointerId) {
    // Route touch to the topmost visible UI (message box > character creation > pause menu > others)
    if (messageBox_ && messageBox_->isVisible() && messageBox_->isEnabled()) {
        if (messageBox_->onTouchDown(x, y, pointerId)) {
            return true;
        }
    }

    if (characterCreation_ && characterCreation_->isVisible() && characterCreation_->isEnabled()) {
        if (characterCreation_->onTouchDown(x, y, pointerId)) {
            return true;
        }
    }

    if (pauseMenu_ && pauseMenu_->isVisible() && pauseMenu_->isEnabled()) {
        if (pauseMenu_->onTouchDown(x, y, pointerId)) {
            return true;
        }
    }

    if (dialogue_ && dialogue_->isVisible() && dialogue_->isEnabled()) {
        if (dialogue_->onTouchDown(x, y, pointerId)) {
            return true;
        }
    }

    if (shop_ && shop_->isVisible() && shop_->isEnabled()) {
        if (shop_->onTouchDown(x, y, pointerId)) {
            return true;
        }
    }

    if (characterSheet_ && characterSheet_->isVisible() && characterSheet_->isEnabled()) {
        if (characterSheet_->onTouchDown(x, y, pointerId)) {
            return true;
        }
    }

    if (spellbook_ && spellbook_->isVisible() && spellbook_->isEnabled()) {
        if (spellbook_->onTouchDown(x, y, pointerId)) {
            return true;
        }
    }

    if (questLog_ && questLog_->isVisible() && questLog_->isEnabled()) {
        if (questLog_->onTouchDown(x, y, pointerId)) {
            return true;
        }
    }

    // Quick slot bar is always available for interaction
    if (quickSlotBar_) {
        if (quickSlotBar_->onTouchDown(x, y, pointerId)) {
            return true;
        }
    }

    return false;
}

bool UIManager::onTouchUp(float x, float y, int pointerId) {
    // Route touch to the topmost visible UI
    if (messageBox_ && messageBox_->isVisible() && messageBox_->isEnabled()) {
        if (messageBox_->onTouchUp(x, y, pointerId)) {
            return true;
        }
    }

    if (characterCreation_ && characterCreation_->isVisible() && characterCreation_->isEnabled()) {
        if (characterCreation_->onTouchUp(x, y, pointerId)) {
            return true;
        }
    }

    if (pauseMenu_ && pauseMenu_->isVisible() && pauseMenu_->isEnabled()) {
        if (pauseMenu_->onTouchUp(x, y, pointerId)) {
            return true;
        }
    }

    if (dialogue_ && dialogue_->isVisible() && dialogue_->isEnabled()) {
        if (dialogue_->onTouchUp(x, y, pointerId)) {
            return true;
        }
    }

    if (shop_ && shop_->isVisible() && shop_->isEnabled()) {
        if (shop_->onTouchUp(x, y, pointerId)) {
            return true;
        }
    }

    if (characterSheet_ && characterSheet_->isVisible() && characterSheet_->isEnabled()) {
        if (characterSheet_->onTouchUp(x, y, pointerId)) {
            return true;
        }
    }

    if (spellbook_ && spellbook_->isVisible() && spellbook_->isEnabled()) {
        if (spellbook_->onTouchUp(x, y, pointerId)) {
            return true;
        }
    }

    if (questLog_ && questLog_->isVisible() && questLog_->isEnabled()) {
        if (questLog_->onTouchUp(x, y, pointerId)) {
            return true;
        }
    }

    return false;
}
