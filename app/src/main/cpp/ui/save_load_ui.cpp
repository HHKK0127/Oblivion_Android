#include "save_load_ui.h"
#include "../engine/renderer.h"
#include "ui_draw_helper.h"
#include "placeholder_assets.h"
#include "../engine/texture_loader.h"
#include <sstream>
#include <iomanip>
#include <ctime>
#include <GLES3/gl3.h>

SaveLoadUI::SaveLoadUI()
    : textRenderer(nullptr), saveManager(nullptr),
      visible(false), returnToMenu(false),
      currentMode(Mode::LOAD), dialogState(DialogState::NONE),
      selectedSlotIndex(0), hoveredSlotIndex(-1) {
    LOGD("SaveLoadUI created");
}

SaveLoadUI::~SaveLoadUI() {
    if (bgTexture != 0) {
        TextureLoader::deleteTexture(bgTexture);
    }
    cleanup();
}

bool SaveLoadUI::initialize(TextRenderer* renderer, SaveManager* manager, Renderer* rend) {
    if (!renderer || !manager) {
        LOGE("Error: TextRenderer or SaveManager is null");
        return false;
    }

    textRenderer = renderer;
    saveManager = manager;
    this->renderer = rend;

    // Load background texture
    if (bgTexture == 0) {
        bgTexture = TextureLoader::loadTextureFromAsset("textures/ui/main_background.png");
        LOGI("SaveLoadUI: Background texture loaded: %u", bgTexture);
    }

    refreshSaveSlots();
    updateLayout();

    LOGI("SaveLoadUI initialized with %zu slots", availableSlots.size());
    return true;
}

void SaveLoadUI::cleanup() {
    textRenderer = nullptr;
    saveManager = nullptr;
    slotEntries.clear();
    availableSlots.clear();
    LOGD("SaveLoadUI cleaned up");
}

void SaveLoadUI::render() {
    if (!visible || !textRenderer) {
        return;
    }

    // Draw background texture if available
    if (bgTexture != 0 && renderer) {
        int sw = static_cast<int>(renderer->getScreenWidth());
        int sh = static_cast<int>(renderer->getScreenHeight());
        UIDrawHelper::drawTexturedQuad(
            0.0f, 0.0f,
            static_cast<float>(sw),
            static_cast<float>(sh),
            bgTexture,
            glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
            sw,
            sh
        );
    } else {
        // Fallback: dark background
        glClearColor(0.0f, 0.0f, 0.0f, 0.5f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    // タイトル
    std::string title = (currentMode == Mode::SAVE) ? "SAVE GAME" : "LOAD GAME";
    glm::vec3 titleColor(1.0f, 1.0f, 0.0f);
    textRenderer->renderText(title, 350.0f, 50.0f, titleColor, 1.5f);

    // ダイアログ状態によって表示を切り替え
    if (dialogState == DialogState::CONFIRM_OVERWRITE) {
        renderConfirmDialog();
    } else if (dialogState == DialogState::CONFIRM_DELETE) {
        renderConfirmDialog();
    } else if (dialogState == DialogState::ERROR_SAVE_FAILED ||
               dialogState == DialogState::ERROR_LOAD_FAILED ||
               dialogState == DialogState::ERROR_DELETE_FAILED) {
        renderErrorDialog();
    } else {
        renderSlotList();
        renderButtons();
    }
}

void SaveLoadUI::update(float deltaTime) {
    if (!visible) {
        return;
    }

    // ホバー状態を更新
    updateLayout();
}

void SaveLoadUI::toggle() {
    if (visible) {
        close();
    } else {
        open(currentMode);
    }
}

void SaveLoadUI::open(Mode mode) {
    currentMode = mode;
    visible = true;
    dialogState = DialogState::NONE;
    selectedSlotIndex = 0;
    hoveredSlotIndex = -1;
    returnToMenu = false;

    refreshSaveSlots();
    updateLayout();

    LOGI("SaveLoadUI opened in %s mode with %zu slots",
         (mode == Mode::SAVE) ? "SAVE" : "LOAD", availableSlots.size());
}

void SaveLoadUI::close() {
    visible = false;
    dialogState = DialogState::NONE;
    returnToMenu = false;
    LOGD("SaveLoadUI closed");
}

void SaveLoadUI::onTouchEvent(float x, float y) {
    if (!visible) {
        return;
    }

    // ダイアログ表示中は特別な処理
    if (dialogState != DialogState::NONE) {
        const float dlgW = 500.0f;
        const float dlgX = (screenWidth - dlgW) * 0.5f;

        // エラーダイアログの場合は OK ボタンのみ
        if (dialogState == DialogState::ERROR_SAVE_FAILED ||
            dialogState == DialogState::ERROR_LOAD_FAILED ||
            dialogState == DialogState::ERROR_DELETE_FAILED) {
            const float dlgH = 280.0f;
            const float dlgY = (screenHeight - dlgH) * 0.4f;
            const float btnW = 120.0f;
            const float btnH = 46.0f;
            float okX = dlgX + (dlgW - btnW) * 0.5f;
            float okY = dlgY + dlgH - btnH - 20.0f;
            if (x >= okX && x < okX + btnW && y >= okY && y < okY + btnH) {
                dialogState = DialogState::NONE;
            }
            return;
        }

        // 確認ダイアログのボタン処理
        const float dlgH = 220.0f;
        const float dlgY = (screenHeight - dlgH) * 0.4f;
        const float btnW = 140.0f;
        const float btnH = 50.0f;
        const float btnY = dlgY + dlgH - btnH - 20.0f;
        float yesX = dlgX + dlgW * 0.15f;
        float noX  = dlgX + dlgW * 0.6f;

        if (x >= yesX && x < yesX + btnW && y >= btnY && y < btnY + btnH) {
            // YES ボタン
            DialogState previousState = dialogState;
            if (dialogState == DialogState::CONFIRM_OVERWRITE) {
                handleConfirmOverwrite();
            } else if (dialogState == DialogState::CONFIRM_DELETE) {
                handleConfirmDelete();
            }
            if (dialogState == previousState) {
                dialogState = DialogState::NONE;
            }
        } else if (x >= noX && x < noX + btnW && y >= btnY && y < btnY + btnH) {
            // NO ボタン
            dialogState = DialogState::NONE;
        }
        return;
    }

    // スロット選択
    for (size_t i = 0; i < slotEntries.size(); i++) {
        const auto& slot = slotEntries[i];
        if (x >= slot.position.x && x < slot.position.x + slot.size.x &&
            y >= slot.position.y && y < slot.position.y + slot.size.y) {
            handleSlotSelection(static_cast<int>(i));
            return;
        }
    }

    // ボタン処理（画面下部）
    const float btnW = 200.0f;
    const float btnH = BUTTON_HEIGHT;
    const float centerX = screenWidth * 0.5f;
    const float btnY = screenHeight - 200.0f;
    const float gap = 30.0f;

    float execX = centerX - btnW - gap * 0.5f;
    float cancelX = centerX + gap * 0.5f;

    if (x >= execX && x < execX + btnW && y >= btnY && y < btnY + btnH) {
        handleExecuteAction();
    } else if (x >= cancelX && x < cancelX + btnW && y >= btnY && y < btnY + btnH) {
        handleCancel();
    }
}

void SaveLoadUI::refreshSaveSlots() {
    availableSlots.clear();
    slotEntries.clear();

    if (!saveManager) {
        LOGE("SaveManager is null");
        return;
    }

    availableSlots = saveManager->getSaveSlots();
    selectedSlotIndex = 0;

    LOGI("Refreshed %zu save slots", availableSlots.size());
}

void SaveLoadUI::renderSlotList() {
    const float panelX = (screenWidth - 640.0f) * 0.5f;
    const float panelY = 110.0f;
    const float panelW = 640.0f;
    const float slotW = panelW - PADDING * 2.0f;
    const float maxVisibleSlots = 6;

    // スロットパネル外枠
    PlaceholderAssets::drawPanel(panelX, panelY,
                                 panelW, maxVisibleSlots * (SLOT_HEIGHT + SLOT_MARGIN) + PADDING,
                                 PlaceholderAssets::Colors::PARCHMENT_DARK,
                                 PlaceholderAssets::Colors::BROWN_ACCENT);

    // スロットが空の場合
    if (availableSlots.empty()) {
        // 空スロットプレースホルダー
        for (int i = 0; i < 3; i++) {
            float sx = panelX + PADDING;
            float sy = panelY + PADDING + i * (SLOT_HEIGHT + SLOT_MARGIN);
            PlaceholderAssets::drawPanel(sx, sy, slotW, SLOT_HEIGHT,
                                        PlaceholderAssets::Colors::PARCHMENT_LIGHT,
                                        PlaceholderAssets::Colors::PARCHMENT_DARK);
            if (textRenderer) {
                textRenderer->renderText("EMPTY", sx + 20.0f, sy + SLOT_HEIGHT * 0.3f,
                                        PlaceholderAssets::Colors::BROWN_ACCENT, 0.8f);
            }
        }
        if (textRenderer && currentMode == Mode::LOAD) {
            textRenderer->renderText("No saves available",
                                    panelX + panelW * 0.2f, panelY + maxVisibleSlots * (SLOT_HEIGHT + SLOT_MARGIN) * 0.4f,
                                    PlaceholderAssets::Colors::PARCHMENT_DARK, 0.9f);
        }
        return;
    }

    // スロット一覧表示
    for (size_t i = 0; i < availableSlots.size() && i < static_cast<size_t>(maxVisibleSlots); i++) {
        float sx = panelX + PADDING;
        float sy = panelY + PADDING + static_cast<float>(i) * (SLOT_HEIGHT + SLOT_MARGIN);
        bool isSelected = (static_cast<int>(i) == selectedSlotIndex);
        bool isHovered = (static_cast<int>(i) == hoveredSlotIndex);

        // スロット背景：選択中は金色枠、ホバーは明るい羊皮紙
        if (isSelected) {
            PlaceholderAssets::drawPanel(sx, sy, slotW, SLOT_HEIGHT,
                                        PlaceholderAssets::Colors::PARCHMENT_LIGHT,
                                        PlaceholderAssets::Colors::GOLD_HIGHLIGHT);
        } else if (isHovered) {
            PlaceholderAssets::drawPanel(sx, sy, slotW, SLOT_HEIGHT,
                                        PlaceholderAssets::Colors::PARCHMENT_LIGHT,
                                        PlaceholderAssets::Colors::PARCHMENT_DARK);
        } else {
            PlaceholderAssets::drawPanel(sx, sy, slotW, SLOT_HEIGHT,
                                        PlaceholderAssets::Colors::PARCHMENT_DARK,
                                        PlaceholderAssets::Colors::BROWN_ACCENT);
        }

        // スロット名テキスト
        if (textRenderer) {
            glm::vec3 textColor = isSelected
                ? PlaceholderAssets::Colors::GOLD_HIGHLIGHT
                : PlaceholderAssets::Colors::PARCHMENT_LIGHT;
            textRenderer->renderText(availableSlots[i],
                                    sx + 20.0f, sy + SLOT_HEIGHT * 0.28f,
                                    textColor, 1.0f);

            // 選択中インジケーター
            if (isSelected) {
                textRenderer->renderText(">", sx + 5.0f, sy + SLOT_HEIGHT * 0.28f,
                                        PlaceholderAssets::Colors::GOLD_HIGHLIGHT, 1.0f);
            }
        }
    }

    // 追加の空スロット枠
    for (size_t i = availableSlots.size(); i < static_cast<size_t>(maxVisibleSlots); i++) {
        float sx = panelX + PADDING;
        float sy = panelY + PADDING + static_cast<float>(i) * (SLOT_HEIGHT + SLOT_MARGIN);
        PlaceholderAssets::drawPanel(sx, sy, slotW, SLOT_HEIGHT,
                                    PlaceholderAssets::Colors::PARCHMENT_LIGHT,
                                    PlaceholderAssets::Colors::PARCHMENT_DARK);
        if (textRenderer && currentMode == Mode::SAVE) {
            textRenderer->renderText("EMPTY - New Save",
                                    sx + 20.0f, sy + SLOT_HEIGHT * 0.28f,
                                    PlaceholderAssets::Colors::BROWN_ACCENT, 0.8f);
        }
    }
}

void SaveLoadUI::renderConfirmDialog() {
    const float dlgW = 500.0f;
    const float dlgH = 220.0f;
    const float dlgX = (screenWidth - dlgW) * 0.5f;
    const float dlgY = (screenHeight - dlgH) * 0.4f;

    // ダイアログパネル
    PlaceholderAssets::drawPanel(dlgX, dlgY, dlgW, dlgH,
                                 PlaceholderAssets::Colors::PARCHMENT_LIGHT,
                                 PlaceholderAssets::Colors::GOLD_HIGHLIGHT);

    // メッセージ
    if (textRenderer) {
        std::string message;
        if (dialogState == DialogState::CONFIRM_OVERWRITE) {
            message = "Overwrite '" + pendingSlotName + "'?";
        } else {
            message = "Delete '" + pendingSlotName + "'?";
        }
        textRenderer->renderText(message, dlgX + 30.0f, dlgY + 40.0f,
                                PlaceholderAssets::Colors::BROWN_ACCENT, 1.0f);
    }

    // YES ボタン
    const float btnW = 140.0f;
    const float btnH = 50.0f;
    const float btnY = dlgY + dlgH - btnH - 20.0f;
    float yesX = dlgX + dlgW * 0.15f;
    PlaceholderAssets::drawPanel(yesX, btnY, btnW, btnH,
                                 PlaceholderAssets::Colors::PARCHMENT_LIGHT,
                                 PlaceholderAssets::Colors::GOLD_HIGHLIGHT);
    if (textRenderer) {
        textRenderer->renderText("YES", yesX + 45.0f, btnY + 14.0f,
                                PlaceholderAssets::Colors::BROWN_ACCENT, 1.1f);
    }

    // NO ボタン
    float noX = dlgX + dlgW * 0.6f;
    PlaceholderAssets::drawPanel(noX, btnY, btnW, btnH,
                                 PlaceholderAssets::Colors::PARCHMENT_DARK,
                                 PlaceholderAssets::Colors::BROWN_ACCENT);
    if (textRenderer) {
        textRenderer->renderText("NO", noX + 50.0f, btnY + 14.0f,
                                PlaceholderAssets::Colors::PARCHMENT_LIGHT, 1.1f);
    }
}

void SaveLoadUI::renderErrorDialog() {
    const float dlgW = 500.0f;
    const float dlgH = 280.0f;
    const float dlgX = (screenWidth - dlgW) * 0.5f;
    const float dlgY = (screenHeight - dlgH) * 0.4f;

    // エラーダイアログパネル（茶色枠）
    PlaceholderAssets::drawPanel(dlgX, dlgY, dlgW, dlgH,
                                 PlaceholderAssets::Colors::PARCHMENT_LIGHT,
                                 PlaceholderAssets::Colors::BROWN_ACCENT);

    if (textRenderer) {
        // エラータイトル
        textRenderer->renderText("ERROR", dlgX + dlgW * 0.35f, dlgY + 25.0f,
                                glm::vec3(0.8f, 0.1f, 0.1f), 1.3f);

        // エラーメッセージ（複数行対応）
        float yPos = dlgY + 80.0f;
        size_t lineStart = 0;
        while (lineStart < errorMessage.size()) {
            size_t lineEnd = errorMessage.find('\n', lineStart);
            if (lineEnd == std::string::npos) lineEnd = errorMessage.size();
            std::string line = errorMessage.substr(lineStart, lineEnd - lineStart);
            textRenderer->renderText(line, dlgX + 25.0f, yPos,
                                    PlaceholderAssets::Colors::BROWN_ACCENT, 0.9f);
            yPos += 38.0f;
            lineStart = lineEnd + 1;
        }
    }

    // OK ボタン
    const float btnW = 120.0f;
    const float btnH = 46.0f;
    float okX = dlgX + (dlgW - btnW) * 0.5f;
    float okY = dlgY + dlgH - btnH - 20.0f;
    PlaceholderAssets::drawPanel(okX, okY, btnW, btnH,
                                 PlaceholderAssets::Colors::PARCHMENT_LIGHT,
                                 PlaceholderAssets::Colors::GOLD_HIGHLIGHT);
    if (textRenderer) {
        textRenderer->renderText("OK", okX + 42.0f, okY + 12.0f,
                                PlaceholderAssets::Colors::BROWN_ACCENT, 1.1f);
    }
}

void SaveLoadUI::renderButtons() {
    const float btnW = 200.0f;
    const float btnH = BUTTON_HEIGHT;
    const float centerX = screenWidth * 0.5f;
    const float btnY = screenHeight - 200.0f;
    const float gap = 30.0f;

    // Execute ボタン（左）
    float execX = centerX - btnW - gap * 0.5f;
    PlaceholderAssets::drawPanel(execX, btnY, btnW, btnH,
                                 PlaceholderAssets::Colors::PARCHMENT_LIGHT,
                                 PlaceholderAssets::Colors::GOLD_HIGHLIGHT);
    if (textRenderer) {
        std::string executeLabel = (currentMode == Mode::SAVE) ? "SAVE" : "LOAD";
        textRenderer->renderText(executeLabel,
                                execX + btnW * 0.3f, btnY + btnH * 0.25f,
                                PlaceholderAssets::Colors::BROWN_ACCENT, 1.1f);
    }

    // Cancel ボタン（右）
    float cancelX = centerX + gap * 0.5f;
    PlaceholderAssets::drawPanel(cancelX, btnY, btnW, btnH,
                                 PlaceholderAssets::Colors::PARCHMENT_DARK,
                                 PlaceholderAssets::Colors::BROWN_ACCENT);
    if (textRenderer) {
        textRenderer->renderText("CANCEL",
                                cancelX + btnW * 0.2f, btnY + btnH * 0.25f,
                                PlaceholderAssets::Colors::PARCHMENT_LIGHT, 1.1f);
    }

    // ヘルプテキスト
    if (textRenderer) {
        textRenderer->renderText("Tap a slot to select, then SAVE or LOAD",
                                screenWidth * 0.15f, btnY + btnH + 10.0f,
                                PlaceholderAssets::Colors::PARCHMENT_DARK, 0.75f);
    }
}

void SaveLoadUI::handleSlotSelection(int index) {
    if (index >= 0 && index < static_cast<int>(availableSlots.size())) {
        selectedSlotIndex = index;
        LOGD("Slot %d selected: %s", index, availableSlots[index].c_str());
    }
}

void SaveLoadUI::handleExecuteAction() {
    if (!saveManager) {
        LOGE("SaveManager is null");
        return;
    }

    if (currentMode == Mode::SAVE) {
        if (selectedSlotIndex < availableSlots.size() && saveManager->hasSave(availableSlots[selectedSlotIndex])) {
            dialogState = DialogState::CONFIRM_OVERWRITE;
            pendingSlotName = availableSlots[selectedSlotIndex];
        } else {
            handleConfirmOverwrite();
        }
    } else {
        if (selectedSlotIndex < availableSlots.size()) {
            pendingSlotName = availableSlots[selectedSlotIndex];
            handleConfirmOverwrite();
        } else {
            LOGD("No slot selected for load");
        }
    }
}

void SaveLoadUI::handleConfirmOverwrite() {
    if (!saveManager) {
        LOGE("SaveManager is null");
        return;
    }

    if (currentMode == Mode::SAVE) {
        std::string slotName;
        if (selectedSlotIndex < availableSlots.size()) {
            slotName = availableSlots[selectedSlotIndex];
        } else {
            // 新規スロット名を生成
            std::time_t now = std::time(nullptr);
            std::stringstream ss;
            ss << "Save_" << std::put_time(std::localtime(&now), "%Y%m%d_%H%M%S");
            slotName = ss.str();
        }

        // GameState を構築してセーブ
        GameState gameState;
        gameState.saveName = slotName;
        gameState.saveTimestamp = std::time(nullptr);

        // プレイヤーステータスを取得（Renderer 経由）
        if (renderer) {
            auto playerCtrl = renderer->getPlayerController();
            if (playerCtrl) {
                auto player = playerCtrl->getPlayer();
                if (player) {
                    // プレイヤー位置を保存
                    gameState.playerPosition = player->position;

                    // プレイヤーステータスを構築
                    gameState.playerStatus.currentHealth = player->health;
                    gameState.playerStatus.maxHealth = player->maxHealth;
                    gameState.playerStatus.currentMana = 100.0f;  // TODO: Player に mana フィールド追加
                    gameState.playerStatus.maxMana = 120.0f;

                    LOGD("Player stats saved: HP=%.0f/%.0f, Pos=(%.1f, %.1f, %.1f)",
                         player->health, player->maxHealth,
                         player->position.x, player->position.y, player->position.z);
                }
            }
        } else {
            LOGW("Renderer not available, using default player stats");
        }

        // セーブを実行
        if (saveManager->saveGame(slotName, gameState)) {
            LOGI("Game saved to slot: %s", slotName.c_str());
        } else {
            LOGE("Failed to save game to slot: %s", slotName.c_str());
            errorMessage = "Failed to save game.\nCheck storage space or permissions.";
            dialogState = DialogState::ERROR_SAVE_FAILED;
            return;
        }
    } else {
        // ロード
        if (selectedSlotIndex < availableSlots.size()) {
            const std::string& slotName = availableSlots[selectedSlotIndex];

            GameState gameState;
            if (saveManager->loadGame(slotName, gameState)) {
                LOGI("Game loaded from slot: %s", slotName.c_str());

                // ロード後の処理（Renderer 経由）
                if (renderer) {
                    auto playerCtrl = renderer->getPlayerController();
                    if (playerCtrl && playerCtrl->getPlayer()) {
                        auto player = playerCtrl->getPlayer();
                        player->position = gameState.playerPosition;
                        player->health = gameState.playerStatus.currentHealth;

                        LOGD("Player restored: HP=%.0f, Pos=(%.1f, %.1f, %.1f)",
                             player->health, player->position.x, player->position.y, player->position.z);
                    }
                }
            } else {
                LOGE("Failed to load game from slot: %s", slotName.c_str());
                errorMessage = "Failed to load game.\nThe save file may be corrupted.";
                dialogState = DialogState::ERROR_LOAD_FAILED;
                return;
            }
        }
    }

    close();
    returnToMenu = true;
}

void SaveLoadUI::handleConfirmDelete() {
    if (!saveManager) {
        LOGE("SaveManager is null");
        errorMessage = "SaveManager is not available.";
        dialogState = DialogState::ERROR_DELETE_FAILED;
        return;
    }

    if (selectedSlotIndex < availableSlots.size()) {
        if (saveManager->deleteSave(availableSlots[selectedSlotIndex])) {
            LOGI("Deleted slot: %s", availableSlots[selectedSlotIndex].c_str());
            refreshSaveSlots();
        } else {
            LOGE("Failed to delete slot: %s", availableSlots[selectedSlotIndex].c_str());
            errorMessage = "Failed to delete save.\nCheck permissions or storage.";
            dialogState = DialogState::ERROR_DELETE_FAILED;
        }
    }
}

void SaveLoadUI::handleCancel() {
    close();
    returnToMenu = true;
    LOGD("SaveLoadUI cancelled");
}

std::string SaveLoadUI::getSelectedSlot() const {
    if (selectedSlotIndex < availableSlots.size()) {
        return availableSlots[selectedSlotIndex];
    }
    return "";
}

void SaveLoadUI::updateLayout() {
    slotEntries.clear();

    float yPos = 150.0f;
    for (size_t i = 0; i < availableSlots.size(); i++) {
        SlotEntry entry;
        entry.name = availableSlots[i];
        entry.position = glm::vec2(150.0f, yPos);
        entry.size = glm::vec2(500.0f, SLOT_HEIGHT);
        entry.selected = (static_cast<int>(i) == selectedSlotIndex);
        entry.hovered = (static_cast<int>(i) == hoveredSlotIndex);

        slotEntries.push_back(entry);
        yPos += SLOT_HEIGHT + SLOT_MARGIN;
    }
}
