#include "save_load_ui.h"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <ctime>

SaveLoadUI::SaveLoadUI()
    : currentPanel(SaveLoadPanel::MAIN_MENU),
      previousPanel(SaveLoadPanel::MAIN_MENU),
      isVisible_(false),
      selectedSlotId(-1),
      highlightedSlotId(-1),
      gameManager(nullptr),
      localizationManager(nullptr),
      pendingDeletion(false),
      pendingSave(false),
      pendingLoad(false),
      animationTimer(0.0f) {
    LOGI("SaveLoadUI created");
}

SaveLoadUI::~SaveLoadUI() {
    cleanup();
    LOGI("SaveLoadUI destroyed");
}

// ============================================================================
// Initialization
// ============================================================================

void SaveLoadUI::initialize(GameManager* gm, LocalizationManager* lm) {
    gameManager = gm;
    localizationManager = lm;

    if (!gameManager) {
        LOGE("GameManager is null");
        return;
    }
    if (!localizationManager) {
        LOGE("LocalizationManager is null");
        return;
    }

    updateSlotButtons();
    LOGI("SaveLoadUI initialized");
}

void SaveLoadUI::cleanup() {
    slotButtons.clear();
    LOGI("SaveLoadUI cleanup complete");
}

// ============================================================================
// Panel Transitions
// ============================================================================

void SaveLoadUI::showSaveMenu() {
    LOGI("Showing save menu");
    previousPanel = currentPanel;
    currentPanel = SaveLoadPanel::SAVE_MENU;
    selectedSlotId = -1;
    updateSlotButtons();
}

void SaveLoadUI::showLoadMenu() {
    LOGI("Showing load menu");
    previousPanel = currentPanel;
    currentPanel = SaveLoadPanel::LOAD_MENU;
    selectedSlotId = -1;
    updateSlotButtons();
}

void SaveLoadUI::showAutoSave() {
    LOGI("Checking auto-save");
    if (gameManager->hasAutoSave()) {
        previousPanel = currentPanel;
        currentPanel = SaveLoadPanel::SAVE_MENU;
        selectedSlotId = -1;
    } else {
        LOGW("No auto-save available");
    }
}

void SaveLoadUI::back() {
    LOGI("Going back from save/load menu");
    currentPanel = previousPanel;
    selectedSlotId = -1;
    pendingDeletion = false;
    pendingSave = false;
    pendingLoad = false;
    clearTextInput();
}

// ============================================================================
// Slot Interaction
// ============================================================================

void SaveLoadUI::selectSlot(int slotId) {
    LOGI("Slot %d selected", slotId);
    selectedSlotId = slotId;

    if (currentPanel == SaveLoadPanel::SAVE_MENU) {
        currentPanel = SaveLoadPanel::SAVE_NAME_INPUT;
        clearTextInput();
    } else if (currentPanel == SaveLoadPanel::LOAD_MENU) {
        currentPanel = SaveLoadPanel::LOAD_CONFIRM;
    }
}

void SaveLoadUI::deleteSlot(int slotId) {
    LOGI("Delete requested for slot %d", slotId);
    selectedSlotId = slotId;
    currentPanel = SaveLoadPanel::DELETE_CONFIRM;
    pendingDeletion = true;
}

void SaveLoadUI::confirmSave() {
    LOGI("Confirming save");
    validateAndSaveGame();
}

void SaveLoadUI::confirmLoad() {
    LOGI("Confirming load for slot %d", selectedSlotId);
    if (selectedSlotId >= 0 && gameManager) {
        if (gameManager->loadGame(selectedSlotId)) {
            LOGI("Game loaded successfully");
            toggle();  // Close UI after successful load
        } else {
            LOGE("Failed to load game from slot %d", selectedSlotId);
        }
    }
}

void SaveLoadUI::confirmDelete() {
    LOGI("Confirming delete for slot %d", selectedSlotId);
    if (selectedSlotId >= 0 && gameManager) {
        if (gameManager->deleteSaveSlot(selectedSlotId)) {
            LOGI("Slot %d deleted successfully", selectedSlotId);
            updateSlotButtons();
            back();
        } else {
            LOGE("Failed to delete slot %d", selectedSlotId);
        }
    }
}

// ============================================================================
// Text Input (for save names)
// ============================================================================

void SaveLoadUI::appendTextInput(char c) {
    if (textInput.length() < 20) {  // Max 20 characters
        textInput += c;
        LOGD("Text input: %s", textInput.c_str());
    }
}

void SaveLoadUI::removeTextInput() {
    if (!textInput.empty()) {
        textInput.pop_back();
        LOGD("Text input: %s", textInput.c_str());
    }
}

void SaveLoadUI::clearTextInput() {
    textInput.clear();
}

// ============================================================================
// User Input Handling
// ============================================================================

void SaveLoadUI::onTouchEvent(float x, float y) {
    if (!isVisible_) return;

    LOGD("Touch at (%.0f, %.0f)", x, y);

    switch (currentPanel) {
        case SaveLoadPanel::SAVE_MENU:
        case SaveLoadPanel::LOAD_MENU:
            handleSlotButtonClick(x, y);
            break;

        case SaveLoadPanel::SAVE_CONFIRM:
        case SaveLoadPanel::LOAD_CONFIRM:
        case SaveLoadPanel::DELETE_CONFIRM:
            // Handle dialog buttons (Yes/No)
            // Approximate button positions
            if (isPointInRect(x, y, 100, 350, 150, 50)) {
                // Yes button
                if (currentPanel == SaveLoadPanel::SAVE_CONFIRM) {
                    confirmSave();
                } else if (currentPanel == SaveLoadPanel::LOAD_CONFIRM) {
                    confirmLoad();
                } else if (currentPanel == SaveLoadPanel::DELETE_CONFIRM) {
                    confirmDelete();
                }
            } else if (isPointInRect(x, y, 280, 350, 150, 50)) {
                // No button
                back();
            }
            break;

        case SaveLoadPanel::SAVE_NAME_INPUT:
            // Handle text input completion button
            if (isPointInRect(x, y, 200, 300, 150, 50)) {
                confirmSave();
            }
            break;

        default:
            break;
    }
}

void SaveLoadUI::onKeyPress(int key) {
    if (!isVisible_) return;

    // Handle backspace for text input
    if (key == 8) {  // Backspace
        removeTextInput();
    }
    // Handle Enter for confirmation
    else if (key == 13) {  // Enter
        if (currentPanel == SaveLoadPanel::SAVE_NAME_INPUT) {
            confirmSave();
        }
    }
    // Handle Escape to go back
    else if (key == 27) {  // Escape
        back();
    }
    // Handle character input for save name
    else if (currentPanel == SaveLoadPanel::SAVE_NAME_INPUT && key >= 32 && key <= 126) {
        appendTextInput(static_cast<char>(key));
    }
}

// ============================================================================
// Update and Render
// ============================================================================

void SaveLoadUI::update(float deltaTime) {
    if (!isVisible_) return;

    animationTimer += deltaTime;
    if (animationTimer > ANIMATION_SPEED) {
        animationTimer = 0.0f;
    }
}

void SaveLoadUI::render() {
    if (!isVisible_) return;

    // Render background (semi-transparent overlay)
    // In real implementation, use OpenGL to draw rectangle
    // glColor4f(0.0f, 0.0f, 0.0f, 0.7f);

    switch (currentPanel) {
        case SaveLoadPanel::SAVE_MENU:
            renderSaveMenu();
            break;
        case SaveLoadPanel::LOAD_MENU:
            renderLoadMenu();
            break;
        case SaveLoadPanel::SAVE_NAME_INPUT:
            renderSaveNameInput();
            break;
        case SaveLoadPanel::SAVE_CONFIRM:
            renderSaveConfirmDialog();
            break;
        case SaveLoadPanel::LOAD_CONFIRM:
            renderLoadConfirmDialog();
            break;
        case SaveLoadPanel::DELETE_CONFIRM:
            renderDeleteConfirmDialog();
            break;
        default:
            break;
    }
}

void SaveLoadUI::toggle() {
    isVisible_ = !isVisible_;
    LOGI("SaveLoadUI toggled: %s", isVisible_ ? "visible" : "hidden");
    if (isVisible_) {
        showLoadMenu();
    } else {
        back();
    }
}

// ============================================================================
// Rendering: Panel Methods
// ============================================================================

void SaveLoadUI::renderSaveMenu() {
    // Title
    std::string title = localizationManager ?
        localizationManager->getString("save_menu_title") : "セーブ";
    renderText(title, 640, 50, 1.5f);

    // Render slot buttons
    for (const auto& button : slotButtons) {
        renderSlotButton(button);
    }

    // Back button
    renderButton(localizationManager ?
        localizationManager->getString("button_back") : "戻る",
        START_X, START_Y + (MAX_VISIBLE_SLOTS * (BUTTON_HEIGHT + BUTTON_SPACING)) + 20,
        BUTTON_WIDTH, BUTTON_HEIGHT);
}

void SaveLoadUI::renderLoadMenu() {
    // Title
    std::string title = localizationManager ?
        localizationManager->getString("load_menu_title") : "ロード";
    renderText(title, 640, 50, 1.5f);

    // Render slot buttons (only non-empty slots)
    int yPos = START_Y;
    for (const auto& button : slotButtons) {
        if (!button.isEmpty) {
            SaveSlotButton displayButton = button;
            displayButton.posY = yPos;
            renderSlotButton(displayButton);
            yPos += BUTTON_HEIGHT + BUTTON_SPACING;
        }
    }

    // Auto-save display (if available)
    if (gameManager && gameManager->hasAutoSave()) {
        renderButton("[オートセーブ]", START_X, yPos, BUTTON_WIDTH, BUTTON_HEIGHT);
        yPos += BUTTON_HEIGHT + BUTTON_SPACING;
    }

    // Back button
    renderButton(localizationManager ?
        localizationManager->getString("button_back") : "戻る",
        START_X, yPos + 20, BUTTON_WIDTH, BUTTON_HEIGHT);
}

void SaveLoadUI::renderAutoSavePanel() {
    std::string title = localizationManager ?
        localizationManager->getString("autosave_title") : "オートセーブ";
    renderText(title, 640, 50, 1.5f);

    std::string message = localizationManager ?
        localizationManager->getString("autosave_loading") : "オートセーブを読み込み中...";
    renderText(message, 640, 200, 1.0f);
}

void SaveLoadUI::renderSaveNameInput() {
    // Title
    std::string title = localizationManager ?
        localizationManager->getString("save_name_input_title") : "セーブ名入力";
    renderText(title, 640, 50, 1.5f);

    // Instructions
    std::string instruction = localizationManager ?
        localizationManager->getString("save_name_instruction") : "セーブ名を入力してください";
    renderText(instruction, 640, 150, 1.0f);

    // Current slot info
    if (selectedSlotId >= 0 && selectedSlotId < slotButtons.size()) {
        std::string slotInfo = "スロット " + std::to_string(selectedSlotId);
        renderText(slotInfo, 640, 200, 0.9f);
    }

    // Text input box
    renderButton("_" + textInput, 300, 250, 400, 50);

    // Buttons
    renderButton(localizationManager ?
        localizationManager->getString("button_ok") : "決定",
        200, 350, 150, 50);
    renderButton(localizationManager ?
        localizationManager->getString("button_cancel") : "キャンセル",
        450, 350, 150, 50);
}

void SaveLoadUI::renderSaveConfirmDialog() {
    std::string title = localizationManager ?
        localizationManager->getString("save_confirm_title") : "セーブ確認";
    renderText(title, 640, 100, 1.5f);

    std::string message = "セーブ名: " + textInput;
    renderText(message, 640, 200, 1.0f);

    std::string confirm = localizationManager ?
        localizationManager->getString("save_confirm_message") : "このスロットに上書きしますか？";
    renderText(confirm, 640, 250, 1.0f);

    // Yes/No buttons
    renderButton(localizationManager ?
        localizationManager->getString("button_yes") : "はい",
        100, 350, 150, 50, highlightedSlotId == 0);
    renderButton(localizationManager ?
        localizationManager->getString("button_no") : "いいえ",
        280, 350, 150, 50, highlightedSlotId == 1);
}

void SaveLoadUI::renderLoadConfirmDialog() {
    std::string title = localizationManager ?
        localizationManager->getString("load_confirm_title") : "ロード確認";
    renderText(title, 640, 100, 1.5f);

    if (selectedSlotId >= 0 && selectedSlotId < slotButtons.size()) {
        renderSlotMetadata(slotButtons[selectedSlotId].slotInfo, 640, 200);
    }

    std::string confirm = localizationManager ?
        localizationManager->getString("load_confirm_message") : "このセーブをロードしますか？";
    renderText(confirm, 640, 300, 1.0f);

    // Yes/No buttons
    renderButton(localizationManager ?
        localizationManager->getString("button_yes") : "はい",
        100, 350, 150, 50, highlightedSlotId == 0);
    renderButton(localizationManager ?
        localizationManager->getString("button_no") : "いいえ",
        280, 350, 150, 50, highlightedSlotId == 1);
}

void SaveLoadUI::renderDeleteConfirmDialog() {
    std::string title = localizationManager ?
        localizationManager->getString("delete_confirm_title") : "削除確認";
    renderText(title, 640, 100, 1.5f);

    if (selectedSlotId >= 0 && selectedSlotId < slotButtons.size()) {
        renderSlotMetadata(slotButtons[selectedSlotId].slotInfo, 640, 200);
    }

    std::string confirm = localizationManager ?
        localizationManager->getString("delete_confirm_message") : "このセーブを削除してもよろしいですか？";
    renderText(confirm, 640, 300, 1.0f);

    // Yes/No buttons
    renderButton(localizationManager ?
        localizationManager->getString("button_yes") : "はい",
        100, 350, 150, 50, highlightedSlotId == 0);
    renderButton(localizationManager ?
        localizationManager->getString("button_no") : "いいえ",
        280, 350, 150, 50, highlightedSlotId == 1);
}

// ============================================================================
// Rendering: UI Elements
// ============================================================================

void SaveLoadUI::renderSlotButton(const SaveSlotButton& button) {
    // Determine color based on hover state
    bool highlighted = (button.slotId == highlightedSlotId) || button.isHovered;

    // Draw button background
    std::string label = "スロット " + std::to_string(button.slotId);
    if (button.isEmpty) {
        label += " - [空]";
    }

    renderButton(label, button.posX, button.posY, button.width, button.height, highlighted);

    // Draw metadata if slot is not empty
    if (!button.isEmpty) {
        renderSlotMetadata(button.slotInfo,
            button.posX + button.width + 20,
            button.posY);
    }
}

void SaveLoadUI::renderSlotMetadata(const SaveSlot& slot, float x, float y) {
    // Character name
    std::string nameStr = "キャラ: " + slot.characterName;
    renderText(nameStr, x, y, 0.8f);

    // Level
    std::string levelStr = "Lv." + std::to_string(slot.playerLevel);
    renderText(levelStr, x, y + 25, 0.8f);

    // Location
    std::string locStr = "場所: " + slot.lastLocation;
    renderText(locStr, x, y + 50, 0.8f);

    // Play time
    std::string timeStr = "プレイ時間: " + formatPlaytime(slot.playTime);
    renderText(timeStr, x, y + 75, 0.8f);

    // Save time
    std::string saveStr = "セーブ: " + slot.saveTime;
    renderText(saveStr, x, y + 100, 0.7f);
}

void SaveLoadUI::renderButton(const std::string& label, float x, float y, float w, float h, bool highlighted) {
    // In real implementation, would use OpenGL to render button background
    // For now, just log that button would be rendered
    LOGD("Button: %s at (%.0f, %.0f) %s",
        label.c_str(), x, y, highlighted ? "[HIGHLIGHTED]" : "");
}

void SaveLoadUI::renderText(const std::string& text, float x, float y, float scale) {
    // In real implementation, would use text renderer to display text
    // For now, just log that text would be rendered
    LOGD("Text: %s at (%.0f, %.0f) scale=%.2f", text.c_str(), x, y, scale);
}

// ============================================================================
// Helper Methods
// ============================================================================

void SaveLoadUI::updateSlotButtons() {
    slotButtons.clear();

    if (!gameManager) return;

    auto slots = gameManager->getAllSaveSlots();
    float yPos = START_Y;

    for (const auto& slot : slots) {
        SaveSlotButton button;
        button.slotId = slot.slotId;
        button.posX = START_X;
        button.posY = yPos;
        button.width = BUTTON_WIDTH;
        button.height = BUTTON_HEIGHT;
        button.slotInfo = slot;
        button.isHovered = false;
        button.isEmpty = slot.isEmpty;

        slotButtons.push_back(button);
        yPos += BUTTON_HEIGHT + BUTTON_SPACING;
    }

    LOGI("Updated slot buttons: %zu slots", slotButtons.size());
}

void SaveLoadUI::handleSlotButtonClick(float x, float y) {
    for (auto& button : slotButtons) {
        if (isPointInRect(x, y, button.posX, button.posY, button.width, button.height)) {
            LOGI("Clicked slot %d", button.slotId);
            selectSlot(button.slotId);
            return;
        }
    }
}

bool SaveLoadUI::isPointInRect(float px, float py, float x, float y, float w, float h) const {
    return (px >= x && px <= x + w && py >= y && py <= y + h);
}

std::string SaveLoadUI::formatPlaytime(float hours) const {
    int h = static_cast<int>(hours);
    int m = static_cast<int>((hours - h) * 60);

    std::ostringstream oss;
    oss << h << "時間" << m << "分";
    return oss.str();
}

std::string SaveLoadUI::truncateString(const std::string& str, size_t maxLength) const {
    if (str.length() <= maxLength) {
        return str;
    }
    return str.substr(0, maxLength) + "...";
}

bool SaveLoadUI::isValidSaveName(const std::string& name) const {
    if (name.empty() || name.length() > 20) {
        return false;
    }
    // Check for invalid characters
    for (char c : name) {
        if (c < 32 || c > 126) {
            return false;
        }
    }
    return true;
}

void SaveLoadUI::validateAndSaveGame() {
    LOGI("Validating save");

    if (selectedSlotId < 0 || selectedSlotId >= 5) {
        LOGE("Invalid slot ID: %d", selectedSlotId);
        return;
    }

    // Use default save name if empty
    std::string saveName = textInput.empty() ?
        ("セーブ" + std::to_string(selectedSlotId)) : textInput;

    if (!isValidSaveName(saveName)) {
        LOGE("Invalid save name: %s", saveName.c_str());
        return;
    }

    if (gameManager && gameManager->saveGame(selectedSlotId, saveName)) {
        LOGI("Game saved successfully to slot %d", selectedSlotId);
        clearTextInput();
        toggle();  // Close UI after successful save
    } else {
        LOGE("Failed to save game to slot %d", selectedSlotId);
    }
}

