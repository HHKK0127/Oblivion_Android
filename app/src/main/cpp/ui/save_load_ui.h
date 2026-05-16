#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "text_renderer.h"
#include "../save_system/save_manager.h"

class Renderer;

#define LOG_TAG "SaveLoadUI"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#include <android/log.h>

class SaveLoadUI {
public:
    enum class Mode {
        SAVE,
        LOAD
    };

    enum class DialogState {
        NONE,
        CONFIRM_OVERWRITE,
        CONFIRM_DELETE,
        ERROR_SAVE_FAILED,
        ERROR_LOAD_FAILED,
        ERROR_DELETE_FAILED
    };

    SaveLoadUI();
    ~SaveLoadUI();

    bool initialize(TextRenderer* textRenderer, SaveManager* saveManager, Renderer* renderer = nullptr);
    void cleanup();

    void render();
    void update(float deltaTime);

    void toggle();
    void open(Mode mode);
    void close();

    bool isVisible() const { return visible; }
    Mode getCurrentMode() const { return currentMode; }

    void onTouchEvent(float x, float y);

    bool shouldReturnToMenu() const { return returnToMenu; }
    void resetReturnFlag() { returnToMenu = false; }

    std::string getSelectedSlot() const;

private:
    TextRenderer* textRenderer = nullptr;
    SaveManager* saveManager = nullptr;
    Renderer* renderer = nullptr;

    bool visible = false;
    bool returnToMenu = false;

    Mode currentMode = Mode::LOAD;
    DialogState dialogState = DialogState::NONE;

    int selectedSlotIndex = 0;
    int hoveredSlotIndex = -1;

    std::vector<std::string> availableSlots;
    std::string pendingSlotName;
    std::string errorMessage;

    static constexpr float SLOT_HEIGHT = 60.0f;
    static constexpr float SLOT_MARGIN = 10.0f;
    static constexpr float BUTTON_HEIGHT = 50.0f;
    static constexpr float PADDING = 20.0f;

    struct SlotEntry {
        std::string name;
        glm::vec2 position;
        glm::vec2 size;
        bool hovered = false;
        bool selected = false;
    };

    std::vector<SlotEntry> slotEntries;

    void refreshSaveSlots();
    void renderSlotList();
    void renderConfirmDialog();
    void renderErrorDialog();
    void renderButtons();

    void handleSlotSelection(int index);
    void handleExecuteAction();
    void handleConfirmOverwrite();
    void handleConfirmDelete();
    void handleCancel();

    void updateLayout();
};
