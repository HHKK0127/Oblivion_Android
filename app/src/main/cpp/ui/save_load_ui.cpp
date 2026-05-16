#include "save_load_ui.h"
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

    // 背景を暗くする
    glClearColor(0.0f, 0.0f, 0.0f, 0.5f);
    glClear(GL_COLOR_BUFFER_BIT);

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
        // エラーダイアログの場合は OK ボタンのみ
        if (dialogState == DialogState::ERROR_SAVE_FAILED ||
            dialogState == DialogState::ERROR_LOAD_FAILED ||
            dialogState == DialogState::ERROR_DELETE_FAILED) {
            if (y > 380.0f && y < 460.0f) {
                // OK ボタン
                dialogState = DialogState::NONE;
            }
            return;
        }

        // 確認ダイアログのボタン処理（簡略版：タップ位置で判定）
        if (y > 300.0f && y < 380.0f) {
            // YES ボタン
            DialogState previousState = dialogState;
            if (dialogState == DialogState::CONFIRM_OVERWRITE) {
                handleConfirmOverwrite();
            } else if (dialogState == DialogState::CONFIRM_DELETE) {
                handleConfirmDelete();
            }
            // ハンドラーがエラー状態を設定していない場合のみ NONE に設定
            if (dialogState == previousState) {
                dialogState = DialogState::NONE;
            }
        } else if (y > 420.0f && y < 500.0f) {
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

    // ボタン処理（y > 500 の領域）
    const float BUTTON_Y = 520.0f;
    const float BUTTON_WIDTH = 150.0f;
    const float BUTTON_SPACING = 20.0f;

    // Execute ボタン（x: 150-300）
    if (x >= 150.0f && x < 150.0f + BUTTON_WIDTH && y >= BUTTON_Y && y < BUTTON_Y + BUTTON_HEIGHT) {
        handleExecuteAction();
    }
    // Cancel ボタン（x: 470-620）
    else if (x >= 470.0f && x < 470.0f + BUTTON_WIDTH && y >= BUTTON_Y && y < BUTTON_Y + BUTTON_HEIGHT) {
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
    glm::vec3 normalColor(1.0f, 1.0f, 1.0f);
    glm::vec3 selectedColor(1.0f, 0.0f, 0.0f);
    glm::vec3 hoverColor(0.7f, 0.7f, 1.0f);

    float yPos = 150.0f;

    for (size_t i = 0; i < availableSlots.size(); i++) {
        const std::string& slotName = availableSlots[i];

        glm::vec3 color = normalColor;
        if (static_cast<int>(i) == selectedSlotIndex) {
            color = selectedColor;
        } else if (static_cast<int>(i) == hoveredSlotIndex) {
            color = hoverColor;
        }

        std::string displayText = slotName;
        if (i == selectedSlotIndex) {
            displayText = "> " + displayText;
        }

        textRenderer->renderText(displayText, 200.0f, yPos, color, 1.0f);
        yPos += SLOT_HEIGHT + SLOT_MARGIN;
    }

    // スロットが空の場合
    if (availableSlots.empty()) {
        glm::vec3 emptyColor(0.7f, 0.7f, 0.7f);
        std::string emptyText = (currentMode == Mode::SAVE) ? "New Save" : "No saves available";
        textRenderer->renderText(emptyText, 200.0f, 250.0f, emptyColor, 1.0f);
    }
}

void SaveLoadUI::renderConfirmDialog() {
    // 半透明背景
    glClearColor(0.0f, 0.0f, 0.0f, 0.7f);
    glClear(GL_COLOR_BUFFER_BIT);

    glm::vec3 dialogColor(1.0f, 1.0f, 1.0f);
    glm::vec3 titleColor(1.0f, 0.5f, 0.0f);

    std::string message;
    if (dialogState == DialogState::CONFIRM_OVERWRITE) {
        message = "Overwrite '" + pendingSlotName + "'?";
    } else {
        message = "Delete '" + pendingSlotName + "'?";
    }

    textRenderer->renderText(message, 250.0f, 200.0f, titleColor, 1.2f);

    // ボタン
    glm::vec3 buttonColor(0.2f, 0.8f, 0.2f);
    textRenderer->renderText("YES", 350.0f, 320.0f, buttonColor, 1.0f);

    glm::vec3 cancelColor(0.8f, 0.2f, 0.2f);
    textRenderer->renderText("NO", 450.0f, 320.0f, cancelColor, 1.0f);
}

void SaveLoadUI::renderErrorDialog() {
    // 半透明背景
    glClearColor(0.0f, 0.0f, 0.0f, 0.7f);
    glClear(GL_COLOR_BUFFER_BIT);

    // エラータイトル
    glm::vec3 errorColor(1.0f, 0.0f, 0.0f);
    textRenderer->renderText("ERROR", 350.0f, 120.0f, errorColor, 1.5f);

    // エラーメッセージ（複数行対応）
    glm::vec3 messageColor(1.0f, 1.0f, 1.0f);
    float yPos = 200.0f;
    size_t lineStart = 0;
    while (lineStart < errorMessage.size()) {
        size_t lineEnd = errorMessage.find('\n', lineStart);
        if (lineEnd == std::string::npos) {
            lineEnd = errorMessage.size();
        }
        std::string line = errorMessage.substr(lineStart, lineEnd - lineStart);
        textRenderer->renderText(line, 200.0f, yPos, messageColor, 1.0f);
        yPos += 40.0f;
        lineStart = lineEnd + 1;
    }

    // OK ボタン
    glm::vec3 buttonColor(0.2f, 0.8f, 0.2f);
    textRenderer->renderText("OK", 400.0f, 420.0f, buttonColor, 1.2f);
}

void SaveLoadUI::renderButtons() {
    glm::vec3 buttonColor(0.0f, 1.0f, 0.0f);
    glm::vec3 cancelColor(1.0f, 0.0f, 0.0f);

    // Execute ボタン
    std::string executeLabel = (currentMode == Mode::SAVE) ? "SAVE" : "LOAD";
    textRenderer->renderText(executeLabel, 160.0f, 530.0f, buttonColor, 1.0f);

    // Cancel ボタン
    textRenderer->renderText("CANCEL", 480.0f, 530.0f, cancelColor, 1.0f);

    // ヘルプテキスト
    glm::vec3 helpColor(0.7f, 0.7f, 0.7f);
    textRenderer->renderText("Tap slot to select", 250.0f, 620.0f, helpColor, 0.8f);
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
