#include "save_load_ui.h"
#include "../engine/renderer.h"
#include "ui_draw_helper.h"
#include "placeholder_assets.h"
#include "../engine/texture_loader.h"
#include <sstream>
#include <iomanip>
#include <cstdio>
#include <ctime>
#include <GLES3/gl3.h>

// Redefine LOG macros after renderer.h may have overridden them
#undef LOG_TAG
#undef LOGD
#undef LOGI
#undef LOGE
#define LOG_TAG "SaveLoadUI"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace {
void drawTextureIfLoaded(GLuint texture, float x, float y, float w, float h, int sw, int sh) {
    if (texture != 0) {
        UIDrawHelper::drawTexturedQuad(x, y, w, h, texture, glm::vec4(1.0f), sw, sh);
    }
}

// Original save/load rows show the play time as "%02i:%02i:%02i"
// (RE_EXE_REPORT §27). Hours keep the minimum width but may exceed two digits.
std::string formatPlayTime(uint32_t totalSeconds) {
    const uint32_t hours = totalSeconds / 3600u;
    const uint32_t minutes = (totalSeconds / 60u) % 60u;
    const uint32_t seconds = totalSeconds % 60u;

    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%02u:%02u:%02u", hours, minutes, seconds);
    return std::string(buffer);
}

// The original .dds art has transparent margins. The opaque area (measured from
// the PNG alpha bounds) is stretched to the target rect so the visible panel
// matches the XML geometry instead of the padded texture size.
struct OpaqueBounds {
    float x, y, w, h;   // opaque sub-rect in texture pixels
    float texW, texH;   // full texture size in pixels
};

void drawTextureFitOpaque(GLuint texture, const OpaqueBounds& b,
                          float tx, float ty, float tw, float th, int sw, int sh) {
    if (texture == 0) {
        return;
    }
    const float scaleX = tw / b.w;
    const float scaleY = th / b.h;
    const float quadW = b.texW * scaleX;
    const float quadH = b.texH * scaleY;
    const float originX = tx - b.x * scaleX;
    const float originY = ty - b.y * scaleY;
    UIDrawHelper::drawTexturedQuad(originX, originY, quadW, quadH, texture, glm::vec4(1.0f), sw, sh);
}

// Opaque bounds measured from the extracted PNGs (alpha bbox).
const OpaqueBounds kBgBounds    {0.0f, 0.0f, 820.0f, 648.0f, 1024.0f, 1024.0f};
const OpaqueBounds kLinesBounds {1.0f, 0.0f, 364.0f, 467.0f, 512.0f, 512.0f};
const OpaqueBounds kFoldBounds  {0.0f, 0.0f, 60.0f, 512.0f, 64.0f, 512.0f};
const OpaqueBounds kFrameBounds {0.0f, 0.0f, 396.0f, 225.0f, 512.0f, 256.0f};
}

SaveLoadUI::SaveLoadUI()
    : textRenderer(nullptr), saveManager(nullptr),
      visible(false), returnToMenu(false),
      currentMode(Mode::LOAD), dialogState(DialogState::NONE),
      selectedSlotIndex(0), hoveredSlotIndex(-1) {
    LOGD("SaveLoadUI created");
}

SaveLoadUI::~SaveLoadUI() {
    for (GLuint* tex : {&bgTexture, &linesTexture, &foldTexture,
                        &previewFrameTexture, &emptyPreviewTexture}) {
        if (*tex != 0) {
            TextureLoader::deleteTexture(*tex);
            *tex = 0;
        }
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

    // Load original menu textures (placed under assets/textures/ui/menus/)
    if (bgTexture == 0) {
        bgTexture = TextureLoader::loadTextureFromAsset("textures/ui/menus/level_up__level_up_background.png");
        linesTexture = TextureLoader::loadTextureFromAsset("textures/ui/menus/loading__loading_save_lines.png");
        foldTexture = TextureLoader::loadTextureFromAsset("textures/ui/menus/loading__loading_save_center_fold.png");
        previewFrameTexture = TextureLoader::loadTextureFromAsset("textures/ui/menus/loading__loading_save_wide_frame.png");
        emptyPreviewTexture = TextureLoader::loadTextureFromAsset("textures/ui/menus/shared__shared_empty_large.png");
        LOGI("SaveLoadUI: textures bg=%u lines=%u fold=%u frame=%u empty=%u",
             bgTexture, linesTexture, foldTexture, previewFrameTexture, emptyPreviewTexture);
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

    const Layout l = computeLayout();

    // 2D overlay: ignore depth left over from 3D passes so the dim covers everything
    glDisable(GL_DEPTH_TEST);

    // Translucent dim over the scene already drawn by the caller (title or world)
    UIDrawHelper::drawColoredQuad(0.0f, 0.0f, static_cast<float>(screenWidth),
                                  static_cast<float>(screenHeight),
                                  glm::vec4(0.0f, 0.0f, 0.0f, 0.72f),
                                  screenWidth, screenHeight);
    drawTextureFitOpaque(bgTexture, kBgBounds, l.bgX, l.bgY, BG_WIDTH, BG_HEIGHT,
                         screenWidth, screenHeight);

    // Title sits in the left column above the list so it never crosses the center fold
    std::string title = (currentMode == Mode::SAVE) ? "SAVE GAME" : "LOAD GAME";
    glm::vec3 titleColor(117.0f / 255.0f, 59.0f / 255.0f, 33.0f / 255.0f);
    const float titleScale = 1.2f;
    textRenderer->renderText(title, l.bgX + LIST_X_OFFSET, l.bgY + 14.0f, titleColor, titleScale);

    // Switch display based on dialog state
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
    (void)deltaTime;
    if (!visible) {
        return;
    }

    // Update hover state
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

    // Special handling while dialog is displayed
    if (dialogState != DialogState::NONE) {
        const float dlgW = 500.0f;
        const float dlgX = (screenWidth - dlgW) * 0.5f;

        // Only OK button for error dialog
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

        // Confirm dialog button handling
        const float dlgH = 220.0f;
        const float dlgY = (screenHeight - dlgH) * 0.4f;
        const float btnW = 140.0f;
        const float btnH = 50.0f;
        const float btnY = dlgY + dlgH - btnH - 20.0f;
        float yesX = dlgX + dlgW * 0.15f;
        float noX  = dlgX + dlgW * 0.6f;

        if (x >= yesX && x < yesX + btnW && y >= btnY && y < btnY + btnH) {
            // YES button
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
            // NO button
            dialogState = DialogState::NONE;
        }
        return;
    }

    // Slot selection
    for (size_t i = 0; i < slotEntries.size(); i++) {
        const auto& slot = slotEntries[i];
        if (x >= slot.position.x && x < slot.position.x + slot.size.x &&
            y >= slot.position.y && y < slot.position.y + slot.size.y) {
            handleSlotSelection(static_cast<int>(i));
            return;
        }
    }

    // Button handling (under the preview column)
    const ButtonRects b = computeButtonRects(computeLayout());

    if (x >= b.execX && x < b.execX + b.w && y >= b.y && y < b.y + b.h) {
        handleExecuteAction();
    } else if (x >= b.cancelX && x < b.cancelX + b.w && y >= b.y && y < b.y + b.h) {
        handleCancel();
    }
}

SaveLoadUI::ButtonRects SaveLoadUI::computeButtonRects(const Layout& l) const {
    const float gap = 30.0f;
    const float w = (PREVIEW_WIDTH - gap) * 0.5f;
    ButtonRects b;
    b.w = w;
    b.h = BUTTON_HEIGHT;
    b.y = l.previewY + PREVIEW_HEIGHT + 180.0f;
    b.execX = l.previewX;
    b.cancelX = l.previewX + w + gap;
    return b;
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
    const Layout l = computeLayout();
    const int sw = screenWidth;
    const int sh = screenHeight;
    const float rowW = LINES_WIDTH - LIST_X_OFFSET * 2.0f;
    const glm::vec3 textColor(117.0f / 255.0f, 59.0f / 255.0f, 33.0f / 255.0f);

    // Original list backdrop (loading_save_lines.dds)
    drawTextureFitOpaque(linesTexture, kLinesBounds, l.linesX, l.linesY,
                         LINES_WIDTH, LINES_HEIGHT, sw, sh);

    // Every visible row is drawn, including empty ones
    for (int i = 0; i < VISIBLE_ROWS; i++) {
        const float rx = l.rowX;
        const float ry = l.rowY + static_cast<float>(i) * ITEM_HEIGHT;
        const bool hasSlot = (i < static_cast<int>(availableSlots.size()));
        const bool isSelected = (i == selectedSlotIndex);
        const bool isHovered = (i == hoveredSlotIndex);

        if (hasSlot && isSelected) {
            UIDrawHelper::drawColoredQuad(rx, ry, rowW, ITEM_HEIGHT,
                                          glm::vec4(0.85f, 0.65f, 0.25f, 0.35f), sw, sh);
        } else if (hasSlot && isHovered) {
            UIDrawHelper::drawColoredQuad(rx, ry, rowW, ITEM_HEIGHT,
                                          glm::vec4(1.0f, 1.0f, 1.0f, 0.15f), sw, sh);
        }

        if (!textRenderer) {
            continue;
        }
        const float textY = ry + ITEM_HEIGHT * 0.3f;
        if (hasSlot) {
            const SaveSlotInfo& slot = availableSlots[i];
            const std::string rowName = slot.displayName.empty()
                ? "Save " + std::to_string(i + 1) : slot.displayName;
            textRenderer->renderText(rowName, rx + 16.0f, textY, textColor, 1.0f);
            if (!slot.isEmpty) {
                // Right side of the row: level then play time (RE_EXE_REPORT §27)
                textRenderer->renderText("Lv " + std::to_string(slot.playerLevel),
                                         rx + rowW - 200.0f, textY, textColor, 0.9f);
                textRenderer->renderText(formatPlayTime(slot.playTimeSeconds),
                                         rx + rowW - 110.0f, textY, textColor, 0.9f);
            }
        } else if (i == 0 && currentMode == Mode::LOAD) {
            textRenderer->renderText("No saves available", rx + 16.0f, textY, textColor, 0.9f);
        } else if (currentMode == Mode::SAVE) {
            textRenderer->renderText("New Save", rx + 16.0f, textY, textColor, 1.0f);
        }
    }

    // Center fold separates the list from the preview (loading_save_center_fold.dds)
    drawTextureFitOpaque(foldTexture, kFoldBounds, l.foldX, l.bgY, FOLD_WIDTH, BG_HEIGHT, sw, sh);

    // Preview frame (loading_save_wide_frame.dds) with placeholder art (shared_empty_large.dds)
    drawTextureFitOpaque(previewFrameTexture, kFrameBounds, l.previewX, l.previewY,
                         PREVIEW_WIDTH, PREVIEW_HEIGHT, sw, sh);
    const float artW = 256.0f;
    const float artH = 252.0f;
    drawTextureIfLoaded(emptyPreviewTexture,
                        l.previewX + (PREVIEW_WIDTH - artW) * 0.5f, l.previewY + 5.0f,
                        artW, artH, sw, sh);

    // Details of the selected slot below the preview
    if (textRenderer && selectedSlotIndex >= 0 &&
        selectedSlotIndex < static_cast<int>(availableSlots.size())) {
        const SaveSlotInfo& slot = availableSlots[selectedSlotIndex];
        const float tx = l.previewX + 10.0f;
        const float ty = l.previewY + PREVIEW_HEIGHT + 14.0f;
        textRenderer->renderText(slot.displayName, tx, ty, textColor, 1.0f);
        if (!slot.isEmpty) {
            textRenderer->renderText("Lv " + std::to_string(slot.playerLevel) +
                                     "   " + formatPlayTime(slot.playTimeSeconds),
                                     tx, ty + 34.0f, textColor, 0.9f);
        }
        if (!slot.locationName.empty()) {
            textRenderer->renderText(slot.locationName, tx, ty + 68.0f, textColor, 0.9f);
        }
    }
}

void SaveLoadUI::renderConfirmDialog() {
    const float dlgW = 500.0f;
    const float dlgH = 220.0f;
    const float dlgX = (screenWidth - dlgW) * 0.5f;
    const float dlgY = (screenHeight - dlgH) * 0.4f;

    // Dialog panel
    PlaceholderAssets::drawPanel(dlgX, dlgY, dlgW, dlgH,
                                 PlaceholderAssets::Colors::PARCHMENT_LIGHT,
                                 PlaceholderAssets::Colors::GOLD_HIGHLIGHT);

    // Message
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

    // YES button
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

    // NO button
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

    // Error dialog panel (brown frame)
    PlaceholderAssets::drawPanel(dlgX, dlgY, dlgW, dlgH,
                                 PlaceholderAssets::Colors::PARCHMENT_LIGHT,
                                 PlaceholderAssets::Colors::BROWN_ACCENT);

    if (textRenderer) {
        // Error title
        textRenderer->renderText("ERROR", dlgX + dlgW * 0.35f, dlgY + 25.0f,
                                glm::vec3(0.8f, 0.1f, 0.1f), 1.3f);

        // Error message (multi-line)
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

    // OK button
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
    const ButtonRects b = computeButtonRects(computeLayout());

    // Execute button (left)
    PlaceholderAssets::drawPanel(b.execX, b.y, b.w, b.h,
                                 PlaceholderAssets::Colors::PARCHMENT_LIGHT,
                                 PlaceholderAssets::Colors::GOLD_HIGHLIGHT);
    if (textRenderer) {
        std::string executeLabel = (currentMode == Mode::SAVE) ? "SAVE" : "LOAD";
        const float labelX = b.execX + (b.w - textRenderer->getTextWidth(executeLabel, 1.1f)) * 0.5f;
        textRenderer->renderText(executeLabel, labelX, b.y + b.h * 0.25f,
                                PlaceholderAssets::Colors::BROWN_ACCENT, 1.1f);
    }

    // Cancel button (right)
    PlaceholderAssets::drawPanel(b.cancelX, b.y, b.w, b.h,
                                 PlaceholderAssets::Colors::PARCHMENT_DARK,
                                 PlaceholderAssets::Colors::BROWN_ACCENT);
    if (textRenderer) {
        const float cancelLabelX = b.cancelX + (b.w - textRenderer->getTextWidth("CANCEL", 1.1f)) * 0.5f;
        textRenderer->renderText("CANCEL", cancelLabelX, b.y + b.h * 0.25f,
                                PlaceholderAssets::Colors::BROWN_ACCENT, 1.1f);
    }
}

void SaveLoadUI::handleSlotSelection(int index) {
    if (index >= 0 && index < static_cast<int>(availableSlots.size())) {
        selectedSlotIndex = index;
        LOGD("Slot %d selected: %s (index=%u)", index,
             availableSlots[index].displayName.c_str(), availableSlots[index].slotIndex);
    }
}

void SaveLoadUI::handleExecuteAction() {
    if (!saveManager) {
        LOGE("SaveManager is null");
        return;
    }

    if (currentMode == Mode::SAVE) {
        if (selectedSlotIndex < static_cast<int>(availableSlots.size()) && saveManager->hasSave(availableSlots[selectedSlotIndex].slotIndex)) {
            dialogState = DialogState::CONFIRM_OVERWRITE;
            pendingSlotName = availableSlots[selectedSlotIndex].displayName;
        } else {
            handleConfirmOverwrite();
        }
    } else {
        if (selectedSlotIndex < static_cast<int>(availableSlots.size())) {
            pendingSlotName = availableSlots[selectedSlotIndex].displayName;
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
        uint32_t slotIndex;
        std::string slotName;
        if (selectedSlotIndex < static_cast<int>(availableSlots.size())) {
            slotIndex = availableSlots[selectedSlotIndex].slotIndex;
            slotName = availableSlots[selectedSlotIndex].displayName;
        } else {
            // New slot: use first available slot
            slotIndex = static_cast<uint32_t>(availableSlots.size());
            std::time_t now = std::time(nullptr);
            std::stringstream ss;
            ss << "Save_" << std::put_time(std::localtime(&now), "%Y%m%d_%H%M%S");
            slotName = ss.str();
        }

        // Execute binary save (serialize all system state)
        if (saveManager->saveGame(slotIndex, slotName)) {
            LOGI("Game saved to slot %u: %s", slotIndex, slotName.c_str());
        } else {
            LOGE("Failed to save game to slot %u: %s", slotIndex, slotName.c_str());
            errorMessage = "Failed to save game.\nCheck storage space or permissions.";
            dialogState = DialogState::ERROR_SAVE_FAILED;
            return;
        }
    } else {
        // Load
        if (selectedSlotIndex < static_cast<int>(availableSlots.size())) {
            uint32_t slotIndex = availableSlots[selectedSlotIndex].slotIndex;

            // Execute binary load (deserialize all system state)
            if (saveManager->loadGame(slotIndex)) {
                LOGI("Game loaded from slot %u", slotIndex);
            } else {
                LOGE("Failed to load game from slot %u", slotIndex);
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

    if (selectedSlotIndex < static_cast<int>(availableSlots.size())) {
        uint32_t slotIndex = availableSlots[selectedSlotIndex].slotIndex;
        if (saveManager->deleteSave(slotIndex)) {
            LOGI("Deleted slot %u: %s", slotIndex, availableSlots[selectedSlotIndex].displayName.c_str());
            refreshSaveSlots();
        } else {
            LOGE("Failed to delete slot %u: %s", slotIndex, availableSlots[selectedSlotIndex].displayName.c_str());
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
    if (selectedSlotIndex < static_cast<int>(availableSlots.size())) {
        return availableSlots[selectedSlotIndex].displayName;
    }
    return "";
}

void SaveLoadUI::updateLayout() {
    slotEntries.clear();

    const Layout l = computeLayout();
    const float rowW = LINES_WIDTH - LIST_X_OFFSET * 2.0f;

    // Hit rectangles match the rows drawn by renderSlotList()
    for (int i = 0; i < VISIBLE_ROWS && i < static_cast<int>(availableSlots.size()); i++) {
        SlotEntry entry;
        entry.name = availableSlots[i].displayName;
        entry.slotIndex = availableSlots[i].slotIndex;
        entry.position = glm::vec2(l.rowX, l.rowY + static_cast<float>(i) * ITEM_HEIGHT);
        entry.size = glm::vec2(rowW, ITEM_HEIGHT);
        entry.selected = (i == selectedSlotIndex);
        entry.hovered = (i == hoveredSlotIndex);

        slotEntries.push_back(entry);
    }
}

SaveLoadUI::Layout SaveLoadUI::computeLayout() const {
    Layout l{};
    l.bgX = (static_cast<float>(screenWidth) - BG_WIDTH) * 0.5f;
    l.bgY = (static_cast<float>(screenHeight) - BG_HEIGHT) * 0.5f;
    l.linesX = l.bgX;
    l.linesY = l.bgY + LINES_Y_OFFSET;
    l.rowX = l.bgX + LIST_X_OFFSET;
    l.rowY = l.bgY + LIST_Y_OFFSET;
    l.foldX = l.bgX + (BG_WIDTH - FOLD_WIDTH) * 0.5f;
    l.previewX = l.bgX + BG_WIDTH * 0.5f + PREVIEW_X_OFFSET;
    l.previewY = l.bgY + PREVIEW_Y_OFFSET;
    return l;
}
