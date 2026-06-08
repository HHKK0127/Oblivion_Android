#pragma once

#include <string>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include "text_renderer.h"
#include "../system/settings_manager.h"
#include "ui_panel.h"
#include "ui_button.h"

/**
 * @brief 設定メニューUI
 */
class SettingsUI {
public:
    enum class SettingItem {
        DEBUG_MODE,
        LANGUAGE,
        PIXELATION,
        SCANLINES,
        COLOR_REDUCTION,
        CRT_DISTORTION,
        FILM_GRAIN,
        BACK
    };

    SettingsUI();
    ~SettingsUI();

    /**
     * @brief 設定UIを初期化
     */
    bool initialize(TextRenderer* textRenderer, SettingsManager* settingsManager, class Renderer* renderer = nullptr);

    /**
     * @brief 表示/非表示を切り替え
     */
    void toggle();

    /**
     * @brief 表示中かどうか
     */
    bool isVisible() const { return visible; }

    void setScreenSize(int w, int h);

    /**
     * @brief 設定UIを描画
     */
    void render();

    /**
     * @brief タッチイベントを処理
     */
    void onTouchEvent(float x, float y);

    /**
     * @brief 前のメニューに戻るかどうか
     */
    bool shouldReturnToMenu() const { return returnToMenu; }

    /**
     * @brief 戻るフラグをリセット
     */
    void resetReturnFlag() { returnToMenu = false; }

    /**
     * @brief クリーンアップ
     */
    void cleanup();

private:
    TextRenderer* textRenderer;
    SettingsManager* settingsManager;
    class Renderer* renderer;

    bool visible;
    bool returnToMenu;

    int selectedIndex;
    std::vector<SettingItem> menuItems;

    struct MenuItem {
        std::string label;
        float x, y;
        float width, height;
    };

    std::vector<MenuItem> renderItems;

    void updateMenuItems();
    std::string getSettingLabel(SettingItem item) const;
    void selectItem(SettingItem item);

    static constexpr const char* LOG_TAG = "SettingsUI";
    #define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

    // Phase 9: Graphical UI components
    std::shared_ptr<UIPanel> settingsPanel;
    std::vector<std::shared_ptr<UIButton>> settingButtons;
    int screenWidth = 1080;
    int screenHeight = 1920;

    // Panel background texture
    GLuint panelTexture = 0;

    void buildGraphicalSettings();
    void refreshLayout();
    void updateButtonLabels();
};
