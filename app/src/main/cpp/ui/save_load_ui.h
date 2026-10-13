#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "text_renderer.h"
#include "../save_system/save_slot_manager.h"

#undef LOG_TAG
#undef LOGD
#undef LOGW
#undef LOGE
#undef LOGI

#include <android/log.h>
#include "../save_system/save_manager.h"

class Renderer;

#define LOG_TAG "SaveLoadUI"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

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

    void setScreenSize(int w, int h) { screenWidth = w; screenHeight = h; updateLayout(); }

private:
    TextRenderer* textRenderer = nullptr;
    SaveManager* saveManager = nullptr;
    Renderer* renderer = nullptr;

    // Original Oblivion textures (extracted from the menus tier of the BSA)
    GLuint bgTexture = 0;               // Menus/Level_up/level_up_background.dds
    GLuint linesTexture = 0;            // Menus/Loading/loading_save_lines.dds
    GLuint foldTexture = 0;             // Menus/Loading/loading_save_center_fold.dds
    GLuint previewFrameTexture = 0;     // Menus/Loading/loading_save_wide_frame.dds
    GLuint emptyPreviewTexture = 0;     // Menus/Shared/shared_empty_large.dds

    // Screen size (Renderer::setScreenSize must be called; landscape default)
    int screenWidth = 1920;
    int screenHeight = 1080;

    bool visible = false;
    bool returnToMenu = false;

    Mode currentMode = Mode::LOAD;
    DialogState dialogState = DialogState::NONE;

    int selectedSlotIndex = 0;
    int hoveredSlotIndex = -1;

    std::vector<SaveSlotInfo> availableSlots;
    std::string pendingSlotName;
    std::string errorMessage;

    // Geometry taken from LoadMenu in the original master_menu_file.xml (pixels)
    static constexpr float BG_WIDTH = 1020.0f;
    static constexpr float BG_HEIGHT = 614.0f;
    static constexpr float LINES_WIDTH = 420.0f;
    static constexpr float LINES_HEIGHT = 600.0f;
    static constexpr float LINES_Y_OFFSET = 18.0f;
    static constexpr float LIST_X_OFFSET = 20.0f;
    static constexpr float LIST_Y_OFFSET = 58.0f;
    static constexpr float ITEM_HEIGHT = 82.0f;
    static constexpr int VISIBLE_ROWS = 6;
    static constexpr float FOLD_WIDTH = 70.0f;
    static constexpr float PREVIEW_WIDTH = 458.0f;
    static constexpr float PREVIEW_HEIGHT = 262.0f;
    static constexpr float PREVIEW_X_OFFSET = 30.0f;
    static constexpr float PREVIEW_Y_OFFSET = 64.0f;
    static constexpr float BUTTON_HEIGHT = 50.0f;

    struct Layout {
        float bgX, bgY;
        float linesX, linesY;
        float rowX, rowY;
        float foldX;
        float previewX, previewY;
    };
    Layout computeLayout() const;

    // LOAD/SAVE (execute) and CANCEL button rectangles, placed under the preview column
    struct ButtonRects {
        float execX, cancelX, y, w, h;
    };
    ButtonRects computeButtonRects(const Layout& l) const;

    struct SlotEntry {
        std::string name;
        uint32_t slotIndex = 0;
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

#undef LOG_TAG
#undef LOGD
#undef LOGW
#undef LOGE
#undef LOGI
