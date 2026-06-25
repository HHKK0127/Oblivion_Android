#pragma once

#include <memory>
#include "ui_panel.h"
#include "ui_character_sheet.h"
#include "ui_spellbook.h"
#include "ui_quest_log.h"
#include "ui_dialogue.h"
#include "text_renderer.h"

class QuestManager;
struct CharacterStatus;

/**
 * @brief UIシステム統合マネージャー
 *
 * Phase 10 Integration: すべてのUIパネルのライフサイクル管理、
 * イベントルーティング、更新・描画の中央制御。
 */
class UIManager {
public:
    UIManager();
    ~UIManager();

    // 初期化・クリーンアップ
    bool initialize(TextRenderer* textRenderer,
                    QuestManager* questManager,
                    CharacterStatus* playerStatus = nullptr);
    void cleanup();

    // Player status setter (for when character is loaded)
    void setPlayerStatus(CharacterStatus* playerStatus);

    // ライフサイクル
    void update(float deltaTime);
    void render();
    void setScreenSize(int width, int height);

    // UI表示制御
    void showCharacterSheet() { characterSheet_->setVisible(true); }
    void hideCharacterSheet() { characterSheet_->setVisible(false); }
    void toggleCharacterSheet() { characterSheet_->setVisible(!characterSheet_->isVisible()); }

    void showSpellbook() { spellbook_->setVisible(true); }
    void hideSpellbook() { spellbook_->setVisible(false); }
    void toggleSpellbook() { spellbook_->setVisible(!spellbook_->isVisible()); }

    void showQuestLog() { questLog_->setVisible(true); }
    void hideQuestLog() { questLog_->setVisible(false); }
    void toggleQuestLog() { questLog_->setVisible(!questLog_->isVisible()); }

    void openDialogue(std::shared_ptr<Dialogue> dialogue) {
        dialogue_->openDialogue(dialogue);
    }
    void closeDialogue() { dialogue_->closeDialogue(); }
    bool isDialogueOpen() const { return dialogue_->isVisible(); }

    // イベント入力
    bool onTouchDown(float x, float y, int pointerId);
    bool onTouchUp(float x, float y, int pointerId);

    // Getter
    UICharacterSheet* getCharacterSheet() { return characterSheet_.get(); }
    UISpellbook* getSpellbook() { return spellbook_.get(); }
    UIQuestLog* getQuestLog() { return questLog_.get(); }
    UIDialogue* getDialogue() { return dialogue_.get(); }

private:
    std::unique_ptr<UICharacterSheet> characterSheet_;
    std::unique_ptr<UISpellbook> spellbook_;
    std::unique_ptr<UIQuestLog> questLog_;
    std::unique_ptr<UIDialogue> dialogue_;

    TextRenderer* textRenderer_ = nullptr;
    QuestManager* questManager_ = nullptr;

    int screenWidth_ = 1080;
    int screenHeight_ = 1920;
};
