#pragma once

#include <vector>
#include <string>
#include <memory>
#include <android/log.h>
#include "../game/game_manager.h"
#include "../save_system/save_manager.h"
#include "../localization/localization_manager.h"

using SaveSystem::SaveSlot;

#define LOG_TAG "SaveLoadUI"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ============================================================================
// UI Panel Types
// ============================================================================

enum class SaveLoadPanel {
    SAVE_MENU,           // セーブスロット選択
    LOAD_MENU,           // ロードスロット選択
    SAVE_CONFIRM,        // セーブ確認
    SAVE_NAME_INPUT,     // セーブ名入力
    LOAD_CONFIRM,        // ロード確認
    DELETE_CONFIRM,      // 削除確認
    MAIN_MENU            // メニュー戻る
};

// ============================================================================
// Button and UI Element Definitions
// ============================================================================

struct SaveSlotButton {
    int slotId;
    int posX, posY;
    int width, height;
    SaveSlot slotInfo;
    bool isHovered;
    bool isEmpty;
};

// ============================================================================
// SaveLoadUI Class
// ============================================================================

class SaveLoadUI {
public:
    SaveLoadUI();
    ~SaveLoadUI();

    // Initialization
    void initialize(GameManager* gm, LocalizationManager* lm);
    void cleanup();

    // Panel transitions
    void showSaveMenu();
    void showLoadMenu();
    void showAutoSave();
    void back();

    // Slot interaction
    void selectSlot(int slotId);
    void deleteSlot(int slotId);
    void confirmSave();
    void confirmLoad();
    void confirmDelete();

    // User input
    void onTouchEvent(float x, float y);
    void onKeyPress(int key);

    // Text input for save names
    void appendTextInput(char c);
    void removeTextInput();
    void clearTextInput();

    // Rendering
    void update(float deltaTime);
    void render();
    void toggle();

    // State queries
    bool isVisible() const { return isVisible_; }
    SaveLoadPanel getCurrentPanel() const { return currentPanel; }
    int getSelectedSlot() const { return selectedSlotId; }

private:
    // ========== UI State ==========
    SaveLoadPanel currentPanel;
    SaveLoadPanel previousPanel;
    bool isVisible_;
    int selectedSlotId;
    int highlightedSlotId;
    std::string textInput;  // For save name input
    float animationTimer;

    // ========== System References ==========
    GameManager* gameManager;
    LocalizationManager* localizationManager;

    // ========== UI Elements ==========
    std::vector<SaveSlotButton> slotButtons;
    bool pendingDeletion;
    bool pendingSave;
    bool pendingLoad;

    // ========== UI Dimensions ==========
    static constexpr float BUTTON_WIDTH = 400.0f;
    static constexpr float BUTTON_HEIGHT = 60.0f;
    static constexpr float BUTTON_SPACING = 20.0f;
    static constexpr float START_Y = 100.0f;
    static constexpr float START_X = 50.0f;
    static constexpr int MAX_VISIBLE_SLOTS = 5;
    static constexpr float ANIMATION_SPEED = 0.3f;

    // ========== Private Rendering Methods ==========
    void renderSaveMenu();
    void renderLoadMenu();
    void renderAutoSavePanel();
    void renderSaveConfirmDialog();
    void renderLoadConfirmDialog();
    void renderDeleteConfirmDialog();
    void renderSaveNameInput();
    void renderSlotButton(const SaveSlotButton& button);
    void renderSlotMetadata(const SaveSlot& slot, float x, float y);
    void renderButton(const std::string& label, float x, float y, float w, float h, bool highlighted = false);
    void renderText(const std::string& text, float x, float y, float scale = 1.0f);

    // ========== Private Helper Methods ==========
    void updateSlotButtons();
    void handleSlotButtonClick(float x, float y);
    void handleMenuButtonClick(const std::string& buttonLabel);
    bool isPointInRect(float px, float py, float x, float y, float w, float h) const;
    std::string formatPlaytime(float hours) const;
    std::string truncateString(const std::string& str, size_t maxLength) const;

    // ========== Input Validation ==========
    bool isValidSaveName(const std::string& name) const;
    void validateAndSaveGame();
};

