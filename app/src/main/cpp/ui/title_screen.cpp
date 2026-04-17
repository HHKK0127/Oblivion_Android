#include "title_screen.h"
#include <GLES3/gl3.h>

TitleScreen::TitleScreen()
    : state(TitleScreenState::LOGO_DISPLAY), displayTimer(0.0f),
      selectedIndex(0), gameStarted(false), localizationManager(nullptr) {
    LOGD("TitleScreen created");
}

TitleScreen::~TitleScreen() {
    LOGD("TitleScreen destroyed");
}

void TitleScreen::initialize(LocalizationManager* lm) {
    localizationManager = lm;

    // Initialize menu items
    menuItems.clear();
    menuItems.push_back("menu_start");
    menuItems.push_back("menu_settings");
    menuItems.push_back("menu_quit");

    displayTimer = 0.0f;
    state = TitleScreenState::LOGO_DISPLAY;
    selectedIndex = 0;
    gameStarted = false;

    LOGI("TitleScreen initialized");
}

void TitleScreen::update(float deltaTime) {
    switch (state) {
        case TitleScreenState::LOGO_DISPLAY: {
            displayTimer += deltaTime;
            if (displayTimer >= LOGO_DISPLAY_DURATION) {
                // Auto-transition to game (skip menu for now)
                state = TitleScreenState::TRANSITIONING;
                startGame();
                LOGI("Logo display complete - starting game automatically");
            }
            break;
        }

        case TitleScreenState::MENU: {
            updateMenu(deltaTime);
            break;
        }

        case TitleScreenState::LANGUAGE: {
            // Language menu update
            break;
        }

        case TitleScreenState::TRANSITIONING: {
            // Game started - nothing to do
            break;
        }

        default:
            break;
    }
}

void TitleScreen::render() {
    // Oblivion風 タイトル画面レンダリング
    // OpenGL ES 3.0でOblivionのカラースキーム（赤/黒）を使用

    switch (state) {
        case TitleScreenState::LOGO_DISPLAY: {
            renderLogoDisplay();
            break;
        }

        case TitleScreenState::MENU: {
            renderMenu();
            break;
        }

        case TitleScreenState::LANGUAGE: {
            renderLanguageSelection();
            break;
        }

        case TitleScreenState::TRANSITIONING: {
            renderFadeOut();
            break;
        }

        default:
            break;
    }
}

void TitleScreen::renderLogoDisplay() {
    // Oblivionロゴ表示フェーズ（3秒間）
    // 背景：暗い赤/黒（Oblivion風）
    glClearColor(0.1f, 0.02f, 0.02f, 1.0f);  // Oblivion赤黒
    glClear(GL_COLOR_BUFFER_BIT);

    // アルファフェードイン（0〜1）
    float fadeAlpha = displayTimer / LOGO_DISPLAY_DURATION;
    fadeAlpha = fadeAlpha > 1.0f ? 1.0f : fadeAlpha;

    // Oblivionロゴテキスト（画面中央）
    renderOblivionLogo(fadeAlpha);

    // "Press to Start" メッセージ（下部）
    renderPressToStartMessage(fadeAlpha);

    LOGD("Logo Display: %.1f/%.1f (alpha: %.2f)", displayTimer, LOGO_DISPLAY_DURATION, fadeAlpha);
}

void TitleScreen::renderMenu() {
    // Oblivionメニュー画面
    glClearColor(0.12f, 0.05f, 0.05f, 1.0f);  // わずかに赤い黒
    glClear(GL_COLOR_BUFFER_BIT);

    // メニューのタイトル
    renderMenuTitle();

    // メニュー項目
    renderMenuItems();

    LOGD("Menu rendered: selected=%d", selectedIndex);
}

void TitleScreen::renderLanguageSelection() {
    // 言語選択画面
    glClearColor(0.1f, 0.02f, 0.02f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // 言語選択メニュー
    // （未実装 - テキストレンダリングが必要）

    LOGD("Language Selection rendered");
}

void TitleScreen::renderFadeOut() {
    // フェードアウトエフェクト
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    LOGD("Fade-out effect");
}

void TitleScreen::renderOblivionLogo(float alpha) {
    // Oblivionロゴを描画
    // 注：完全な実装にはテクスチャやフォントレンダリングが必要
    // 暫定版：画面中央に赤い四角形でロゴのプレースホルダーを表示

    // 画面中央（1920x1080の場合、960x540）
    float centerX = 0.5f;
    float centerY = 0.5f;

    // Oblivion風の赤いロゴプレースホルダー（会炎）
    // シンプル版：赤い矩形（後で本物テクスチャに置き換え）
    glDisable(GL_DEPTH_TEST);

    // 赤い背景（炎のテーマ）
    // NOTE: フルOpenGL実装にはVAO/VBOとシェーダーが必要
    LOGD("  Rendering Oblivion logo (alpha: %.2f)", alpha);
}

void TitleScreen::renderPressToStartMessage(float alpha) {
    // 画面下部に "Press to Start" メッセージを表示
    // NOTE: テキストレンダリングが必要（FreeType等の実装）
    LOGD("  Rendering 'Press to Start' message");
}

void TitleScreen::renderMenuTitle() {
    // メニュータイトル："Oblivion" または ロゴ
    LOGD("  Rendering menu title");
}

void TitleScreen::renderMenuItems() {
    // メニュー項目を描画
    // - Start Game
    // - Settings
    // - Quit

    for (size_t i = 0; i < menuItems.size(); ++i) {
        bool isSelected = (i == selectedIndex);
        float itemX = 0.2f;  // 画面左20%
        float itemY = 0.3f + (i * 0.15f);  // 30%から各項目15%間隔

        std::string displayText = localizationManager ?
            localizationManager->getString(menuItems[i]) :
            menuItems[i];

        // 選択中の項目は明るい赤、非選択は暗い赤
        float colorIntensity = isSelected ? 1.0f : 0.5f;

        LOGD("  Menu item[%zu]: %s (selected: %s, color: %.2f)",
             i, displayText.c_str(), isSelected ? "yes" : "no", colorIntensity);
    }
}

void TitleScreen::onTouchEvent(float x, float y) {
    if (state == TitleScreenState::MENU) {
        // Simple menu item detection based on Y coordinate
        float menuItemHeight = 100.0f;
        float menuStartY = 100.0f;

        int selectedItem = -1;
        for (size_t i = 0; i < menuItems.size(); ++i) {
            float itemY = menuStartY + (i * menuItemHeight);
            if (y >= itemY && y < itemY + menuItemHeight) {
                selectedItem = i;
                break;
            }
        }

        if (selectedItem >= 0) {
            selectedIndex = selectedItem;
            handleMenuSelection();
        }
    } else if (state == TitleScreenState::LOGO_DISPLAY) {
        // Skip logo by tapping
        displayTimer = LOGO_DISPLAY_DURATION;
    }
}

void TitleScreen::onKeyPress(int key) {
    if (state == TitleScreenState::MENU) {
        // Arrow key navigation (simplified)
        if (key == 19) {  // Up arrow
            selectedIndex = (selectedIndex - 1 + menuItems.size()) % menuItems.size();
        } else if (key == 20) {  // Down arrow
            selectedIndex = (selectedIndex + 1) % menuItems.size();
        } else if (key == 23 || key == 66) {  // Select (ENTER or TAP)
            handleMenuSelection();
        }
    }
}

void TitleScreen::transitionToMenu() {
    state = TitleScreenState::MENU;
    displayTimer = 0.0f;
    selectedIndex = 0;  // Highlight "Start" by default
    LOGD("Transitioned to menu state");
}

void TitleScreen::transitionToLanguageMenu() {
    state = TitleScreenState::LANGUAGE;
    selectedIndex = 0;
    LOGD("Transitioned to language selection state");
}

void TitleScreen::updateMenu(float deltaTime) {
    // Menu update logic (animations, etc.)
}

void TitleScreen::handleMenuSelection() {
    if (selectedIndex >= menuItems.size()) {
        return;
    }

    const std::string& selected = menuItems[selectedIndex];

    if (selected == "menu_start") {
        state = TitleScreenState::TRANSITIONING;
        gameStarted = true;
        LOGI("Menu selection: Start Game");
    } else if (selected == "menu_settings") {
        transitionToLanguageMenu();
        LOGD("Menu selection: Settings (Language)");
    } else if (selected == "menu_quit") {
        LOGD("Menu selection: Quit");
        // Would call Android activity finish() in real implementation
    }
}

void TitleScreen::startGame() {
    gameStarted = true;
    LOGI("Game started - transitioning to main game");
}
