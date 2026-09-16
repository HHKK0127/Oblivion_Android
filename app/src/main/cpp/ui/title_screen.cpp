#include "title_screen.h"
#include "text_renderer.h"
#include "../engine/texture_loader.h"
#include "../audio/audio_manager.h"
#include "ui_draw_helper.h"
#include <GLES3/gl3.h>
#include <cmath>

TitleScreen::TitleScreen()
    : state(TitleScreenState::INTRO_MOVIE), displayTimer(0.0f),
      selectedIndex(0), gameStarted(false), settingsRequested(false),
      loadGameRequested(false), creditsRequested(false),
      localizationManager(nullptr), textRenderer(nullptr) {
    LOGD("TitleScreen created (Oblivion Authentic Edition)");
}

TitleScreen::~TitleScreen() {
    TextureLoader::deleteTexture(bgTexture);
    TextureLoader::deleteTexture(logoTexture);
    TextureLoader::deleteTexture(vignetteTexture);
    for (auto& tex : movieFrames) {
        TextureLoader::deleteTexture(tex);
    }
    LOGD("TitleScreen destroyed");
}

void TitleScreen::initialize(LocalizationManager* lm, TextRenderer* tr) {
    localizationManager = lm;
    textRenderer = tr;

    menuItems.clear();
    menuItems.push_back("menu_new");
    menuItems.push_back("menu_load");
    menuItems.push_back("menu_options");
    menuItems.push_back("menu_credits");
    menuItems.push_back("menu_quit");

    displayTimer = 0.0f;
    bgAnimTime = 0.0f;
    logoFadeAlpha = 0.0f;
    introLogoAlpha = 0.0f;
    glowPhase = 0.0f;
    state = TitleScreenState::INTRO_MOVIE;
    selectedIndex = 0;
    gameStarted = false;

    buildGraphicalMenu();
    initParticles();

    if (!texturesLoaded) {
        bgTexture = TextureLoader::loadTextureFromAsset("textures/ui/loading_background.png");
        logoTexture = TextureLoader::loadTextureFromAsset("textures/ui/tes_oblivion_logo_final.png");
        vignetteTexture = TextureLoader::loadTextureFromAsset("textures/ui/load_in_game_default.png");

        texturesLoaded = true;
        LOGI("TitleScreen textures: bg=%u logo=%u vignette=%u",
             bgTexture, logoTexture, vignetteTexture);
    }

    LOGI("TitleScreen initialized (Oblivion Authentic)");
}

void TitleScreen::setScreenSize(int w, int h) {
    screenWidth = w;
    screenHeight = h;
    rebuildMenuLayout();
}

void TitleScreen::initParticles() {
    if (particlesInitialized) return;
    srand(42);
    for (int i = 0; i < MAX_PARTICLES; ++i) {
        particles[i].x = static_cast<float>(rand()) / RAND_MAX;
        particles[i].y = static_cast<float>(rand()) / RAND_MAX;
        particles[i].size = 0.8f + (static_cast<float>(rand()) / RAND_MAX) * 2.0f;
        particles[i].alpha = 0.08f + (static_cast<float>(rand()) / RAND_MAX) * 0.25f;
        particles[i].driftX = (static_cast<float>(rand()) / RAND_MAX - 0.5f) * 0.0003f;
        particles[i].driftY = -0.0002f - (static_cast<float>(rand()) / RAND_MAX) * 0.0003f;
        particles[i].phase = static_cast<float>(rand()) / RAND_MAX * 6.283f;
    }
    particlesInitialized = true;
}

void TitleScreen::buildGraphicalMenu() {
    menuPanel = std::make_shared<UIPanel>("TitleMenuPanel");
    menuPanel->initialize();
    menuPanel->setTitle("");
    menuPanel->setTitleBarHeight(0.0f);
    menuPanel->setCloseButtonVisible(false);
    menuPanel->setDraggable(false);
    menuPanel->setBackgroundColor(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
    menuPanel->setBorderColor(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));
    menuPanel->setBorderWidth(0.0f);

    struct BtnInfo { int index; std::string labelKey; };
    BtnInfo infos[] = {
        {MENU_NEW,     "menu_new"},
        {MENU_LOAD,    "menu_load"},
        {MENU_OPTIONS, "menu_options"},
        {MENU_CREDITS, "menu_credits"},
        {MENU_QUIT,    "menu_quit"}
    };

    for (const auto& info : infos) {
        auto btn = std::make_shared<UIButton>("MenuBtn" + std::to_string(info.index));
        btn->initialize();
        std::string label = localizationManager ? localizationManager->getString(info.labelKey) : info.labelKey;
        btn->setLabel(label);
        btn->setTextRenderer(textRenderer);
        btn->setSize(340.0f, 42.0f);
        btn->setLabelScale(1.1f);
        btn->setLabelColor(COLOR_PARCHMENT);
        btn->setNormalColor(glm::vec4(0.08f, 0.05f, 0.02f, 0.70f));
        btn->setHoverColor(glm::vec4(0.18f, 0.12f, 0.06f, 0.80f));
        btn->setPressedColor(glm::vec4(0.28f, 0.20f, 0.10f, 0.90f));
        btn->setBorderColor(glm::vec4(0.45f, 0.32f, 0.16f, 0.60f));
        btn->setBorderWidth(1.5f);

        int idx = info.index;
        btn->setOnClick([this, idx]() {
            selectedIndex = idx;
            handleMenuSelection();
        });

        menuButtons.push_back(btn);
        menuPanel->addChild(btn);
    }

    rebuildMenuLayout();
}

void TitleScreen::rebuildMenuLayout() {
    if (!menuPanel) return;
    menuPanel->setScreenSize(screenWidth, screenHeight);

    float panelW = 420.0f;
    float panelH = 400.0f;
    float px = screenWidth * 0.04f;
    float py = screenHeight * 0.55f;
    menuPanel->setPosition(px, py);
    menuPanel->setSize(panelW, panelH);

    float btnW = 400.0f;
    float btnH = 42.0f;
    float startY = 30.0f;
    float gap = 10.0f;
    for (size_t i = 0; i < menuButtons.size(); ++i) {
        float bx = 15.0f;
        float by = startY + static_cast<float>(i) * (btnH + gap);
        menuButtons[i]->setPosition(bx, by);
        menuButtons[i]->setSize(btnW, btnH);
        menuButtons[i]->setScreenSize(screenWidth, screenHeight);
    }
}

void TitleScreen::update(float deltaTime) {
    bgAnimTime += deltaTime;
    glowPhase += deltaTime * 2.0f;
    movieFrameTime += deltaTime;

    if (!movieFrames.empty() && movieFrameTime > 1.0f / MOVIE_FPS) {
        currentMovieFrame = (currentMovieFrame + 1) % movieFrames.size();
        movieFrameTime = 0.0f;
    }

    // Update ripples
    for (int i = 0; i < MAX_RIPPLES; ++i) {
        auto& r = ripples[i];
        if (!r.active) continue;
        r.radius += r.speed * deltaTime;
        r.alpha -= deltaTime * 1.5f;
        if (r.alpha <= 0.0f || r.radius >= r.maxRadius) {
            r.active = false;
        }
    }

    switch (state) {
        case TitleScreenState::INTRO_MOVIE: {
            displayTimer += deltaTime;
            float t = displayTimer / INTRO_DURATION;
            if (t > 1.0f) t = 1.0f;
            introLogoAlpha = easeOutQuad(t);
            if (displayTimer >= INTRO_DURATION) {
                transitionToLogo();
            }
            break;
        }
        case TitleScreenState::LOGO_DISPLAY: {
            displayTimer += deltaTime;
            float t = displayTimer / LOGO_FADE_DURATION;
            if (t > 1.0f) t = 1.0f;
            logoFadeAlpha = easeInQuad(t);
            break;
        }
        case TitleScreenState::MENU: {
            updateMenu(deltaTime);
            break;
        }
        default:
            break;
    }
}

void TitleScreen::render() {
    switch (state) {
        case TitleScreenState::INTRO_MOVIE:
            renderIntroMovie();
            break;
        case TitleScreenState::LOGO_DISPLAY:
            renderLogoDisplay();
            break;
        case TitleScreenState::MENU:
            renderMenu();
            break;
        case TitleScreenState::TRANSITIONING:
            renderFadeOut();
            break;
        default:
            break;
    }

    // Render ripples on top of everything
    renderRipples();
}

void TitleScreen::renderIntroMovie() {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    if (logoTexture != 0) {
        float scale = (screenWidth > screenHeight) ? 0.55f : 0.9f;
        float logoW = screenWidth * scale;
        float logoH = logoW * 0.22f;
        float logoX = (screenWidth - logoW) * 0.5f;
        float logoY = (screenHeight - logoH) * 0.45f;

        UIDrawHelper::drawTexturedQuad(
            logoX, logoY, logoW, logoH,
            logoTexture, glm::vec4(1.0f, 1.0f, 1.0f, introLogoAlpha),
            screenWidth, screenHeight);
    }
}

void TitleScreen::renderLogoDisplay() {
    renderBackground(logoFadeAlpha, false);
    renderSepiaOverlay();
    renderVignette();
    renderParticles();
    renderOblivionLogo(logoFadeAlpha, true);

    if (displayTimer > LOGO_FADE_DURATION + 0.3f) {
        float msgT = displayTimer - (LOGO_FADE_DURATION + 0.3f);
        float msgAlpha = std::min(msgT * 2.0f, 1.0f);
        msgAlpha *= 0.5f + 0.5f * sin(displayTimer * 2.5f);
        renderPressAnyKey(msgAlpha);
    }
}

void TitleScreen::renderMenu() {
    renderBackground(1.0f, true);
    renderSepiaOverlay();
    renderVignette();
    renderParticles();
    renderOblivionLogo(1.0f, false);

    // Dark overlay behind the menu area for button readability (with animation).
    float overlayX = 0.0f;
    float overlayY = static_cast<float>(screenHeight) * 0.25f;
    float overlayW = static_cast<float>(screenWidth) * 0.35f;
    float overlayH = static_cast<float>(screenHeight) * 0.65f;
    UIDrawHelper::drawColoredQuad(
        overlayX, overlayY, overlayW, overlayH,
        glm::vec4(0.0f, 0.0f, 0.0f, 0.35f * menuFadeAlpha),
        screenWidth, screenHeight);

    // Apply slide offset to menu panel position
    if (menuPanel) {
        float baseX = screenWidth * 0.04f;
        menuPanel->setPosition(baseX - menuSlideOffset, screenHeight * 0.55f);
    }

    // Draw selection indicator bar
    if (menuPanel) {
        float panelX = screenWidth * 0.04f - menuSlideOffset;
        float panelY = screenHeight * 0.55f;
        float barX = panelX + 5.0f;
        float barW = 6.0f;
        float barH = 36.0f;
        float barYPos = panelY + 30.0f + selectionBarY;

        // Golden selection bar with glow
        glm::vec4 barColor(COLOR_GOLD.x, COLOR_GOLD.y, COLOR_GOLD.z, selectionBarAlpha * menuFadeAlpha);
        UIDrawHelper::drawColoredQuad(
            barX, barYPos, barW, barH,
            barColor, screenWidth, screenHeight);

        // Glow effect behind bar
        glm::vec4 glowColor(COLOR_GOLD.x, COLOR_GOLD.y, COLOR_GOLD.z, 0.2f * menuFadeAlpha);
        UIDrawHelper::drawColoredQuad(
            barX - 4.0f, barYPos - 2.0f, barW + 8.0f, barH + 4.0f,
            glowColor, screenWidth, screenHeight);
    }

    for (size_t i = 0; i < menuButtons.size(); ++i) {
        bool isSelected = (static_cast<int>(i) == selectedIndex);
        int idx = static_cast<int>(i);

        // Apply per-button animation
        float buttonAlpha = buttonAlphas[idx];
        float buttonSlide = buttonSlideOffsets[idx];

        if (isSelected) {
            float glow = 0.6f + 0.4f * sin(glowPhase);
            glm::vec3 c(COLOR_GOLD.x + (COLOR_WHITE.x - COLOR_GOLD.x) * glow * 0.6f,
                        COLOR_GOLD.y + (COLOR_WHITE.y - COLOR_GOLD.y) * glow * 0.6f,
                        COLOR_GOLD.z + (COLOR_WHITE.z - COLOR_GOLD.z) * glow * 0.6f);
            menuButtons[i]->setLabelColor(c);
        } else {
            glm::vec3 dimmed(COLOR_PARCHMENT.x * 0.85f, COLOR_PARCHMENT.y * 0.85f, COLOR_PARCHMENT.z * 0.85f);
            menuButtons[i]->setLabelColor(dimmed);
        }
    }

    if (menuPanel) {
        menuPanel->render();
    }
    renderVersionText();
}

void TitleScreen::renderBackground(float alpha, bool menuMode) {
    // Original Oblivion PC: warm parchment background.
    // We use the extracted loading_background.png when available;
    // otherwise fall back to a procedural sepia color block.
    float w = static_cast<float>(screenWidth);
    float h = static_cast<float>(screenHeight);

    if (bgTexture != 0) {
        UIDrawHelper::drawTexturedQuad(
            0.0f, 0.0f, w, h,
            bgTexture, glm::vec4(1.0f, 1.0f, 1.0f, alpha),
            screenWidth, screenHeight);
    } else {
        // Procedural sepia parchment base.
        glm::vec4 baseCol(0.32f, 0.20f, 0.09f, alpha);
        UIDrawHelper::drawColoredQuad(
            0.0f, 0.0f, w, h,
            baseCol, screenWidth, screenHeight);

        // Subtle inner radial highlight.
        float t = bgAnimTime * 0.6f;
        float hlW = w * 0.7f;
        float hlH = h * 0.55f;
        float hlX = (w - hlW) * 0.5f + std::sin(t * 0.07f) * w * 0.01f;
        float hlY = (h - hlH) * 0.5f + std::cos(t * 0.05f) * h * 0.01f;
        glm::vec4 hlCol(0.55f, 0.36f, 0.18f, alpha * 0.35f * (menuMode ? 1.0f : 0.85f));
        UIDrawHelper::drawColoredQuad(
            hlX, hlY, hlW, hlH,
            hlCol, screenWidth, screenHeight);
    }

    // Vignette darkening at edges.
    float vignetteStrength = alpha * (menuMode ? 0.55f : 0.70f);
    if (vignetteTexture != 0) {
        UIDrawHelper::drawTexturedQuad(
            0.0f, 0.0f, w, h,
            vignetteTexture, glm::vec4(1.0f, 1.0f, 1.0f, vignetteStrength * 0.5f),
            screenWidth, screenHeight);
    } else {
        float edge = std::min(w, h) * 0.18f;
        glm::vec4 dark(0.05f, 0.03f, 0.01f, vignetteStrength);
        UIDrawHelper::drawColoredQuad(0.0f, 0.0f, edge, h, dark, screenWidth, screenHeight);
        UIDrawHelper::drawColoredQuad(w - edge, 0.0f, edge, h, dark, screenWidth, screenHeight);
        UIDrawHelper::drawColoredQuad(0.0f, 0.0f, w, edge, dark, screenWidth, screenHeight);
        UIDrawHelper::drawColoredQuad(0.0f, h - edge, w, edge, dark, screenWidth, screenHeight);
    }
}

void TitleScreen::renderSepiaOverlay() {
    glm::vec4 sepia(0.44f, 0.26f, 0.08f, 0.15f);
    UIDrawHelper::drawColoredQuad(
        0.0f, 0.0f,
        static_cast<float>(screenWidth), static_cast<float>(screenHeight),
        sepia, screenWidth, screenHeight);
}

void TitleScreen::renderVignette() {
    if (vignetteTexture != 0) {
        UIDrawHelper::drawTexturedQuad(
            0.0f, 0.0f,
            static_cast<float>(screenWidth), static_cast<float>(screenHeight),
            vignetteTexture, glm::vec4(1.0f, 1.0f, 1.0f, 0.6f),
            screenWidth, screenHeight);
    }
}

void TitleScreen::renderOblivionLogo(float alpha, bool large) {
    // Prefer the extracted original Oblivion logo texture (tes_oblivion_logo_final.png);
    // fall back to text rendering if the texture is unavailable.
    float minDim = static_cast<float>(std::min(screenWidth, screenHeight));
    float scale = minDim / 1080.0f;
    if (scale < 0.5f) scale = 0.5f;
    if (scale > 2.0f) scale = 2.0f;

    float cx = static_cast<float>(screenWidth) * 0.5f;

    if (logoTexture != 0) {
        // Logo aspect is 1024:256 = 4:1
        float logoW = static_cast<float>(screenWidth) * (large ? 0.55f : 0.42f);
        float logoH = logoW * 0.25f;
        float logoX = cx - logoW * 0.5f;
        float logoY = static_cast<float>(screenHeight) * (large ? 0.30f : 0.18f);
        UIDrawHelper::drawTexturedQuad(
            logoX, logoY, logoW, logoH,
            logoTexture, glm::vec4(1.0f, 1.0f, 1.0f, alpha),
            screenWidth, screenHeight);
    } else if (textRenderer) {
        // Text fallback for "The Elder Scrolls IV: OBLIVION"
        float titleScale = (large ? 1.15f : 0.85f) * scale;
        float mainScale = (large ? 1.7f : 1.25f) * scale;
        const char* line1 = "THE ELDER SCROLLS IV";
        const char* line2 = "OBLIVION";
        float y1 = static_cast<float>(screenHeight) * (large ? 0.20f : 0.10f);
        float y2 = y1 + titleScale * 36.0f;
        glm::vec3 gold(COLOR_GOLD.x, COLOR_GOLD.y, COLOR_GOLD.z);
        glm::vec3 titleColor = gold * (0.9f + 0.1f * std::sin(glowPhase));
        glm::vec3 mainColor = gold;
        float w1 = textRenderer->getTextWidth(line1, titleScale);
        textRenderer->renderText(line1, cx - w1 * 0.5f, y1, titleColor, titleScale);
        float w2 = textRenderer->getTextWidth(line2, mainScale);
        textRenderer->renderText(line2, cx - w2 * 0.5f, y2, mainColor, mainScale);
    }
}

void TitleScreen::renderPressAnyKey(float alpha) {
    if (!textRenderer) return;

    float minDim = static_cast<float>(std::min(screenWidth, screenHeight));
    float scale = minDim / 1080.0f;
    if (scale < 0.5f) scale = 0.5f;
    if (scale > 2.0f) scale = 2.0f;

    const char* hint = "Press any key to continue";
    float fontScale = 0.7f * scale;
    float textWidth = textRenderer->getTextWidth(hint, fontScale);
    float textX = (static_cast<float>(screenWidth) - textWidth) * 0.5f;
    float textY = static_cast<float>(screenHeight) * 0.32f;

    glm::vec3 hintColor(COLOR_PARCHMENT.x, COLOR_PARCHMENT.y, COLOR_PARCHMENT.z);
    textRenderer->renderText(hint, textX, textY, hintColor, fontScale);
}

void TitleScreen::renderVersionText() {
    if (!textRenderer) return;

    float minDim = static_cast<float>(std::min(screenWidth, screenHeight));
    float scale = minDim / 1080.0f;
    if (scale < 0.5f) scale = 0.5f;
    if (scale > 2.0f) scale = 2.0f;

    const char* version = "v1.2.0416";
    float fontScale = 0.5f * scale;
    float textWidth = textRenderer->getTextWidth(version, fontScale);
    float textX = static_cast<float>(screenWidth) - textWidth - 16.0f;
    float textY = static_cast<float>(screenHeight) - 16.0f;

    glm::vec3 verColor(COLOR_PARCHMENT.x * 0.5f, COLOR_PARCHMENT.y * 0.5f, COLOR_PARCHMENT.z * 0.5f);
    textRenderer->renderText(version, textX, textY, verColor, fontScale);
}

void TitleScreen::renderParticles() {
    for (int i = 0; i < MAX_PARTICLES; ++i) {
        auto& p = particles[i];
        p.x += p.driftX;
        p.y += p.driftY;
        if (p.x < 0) p.x += 1.0f;
        if (p.x > 1) p.x -= 1.0f;
        if (p.y < 0) p.y += 1.0f;
        if (p.y > 1) p.y -= 1.0f;

        float flicker = 0.4f + 0.6f * sin(bgAnimTime * 0.5f + p.phase);
        float a = p.alpha * flicker * 0.25f;
        float px = p.x * screenWidth;
        float py = p.y * screenHeight;

        // Render as small colored quads since drawPoint is not available
        float sz = p.size;
        UIDrawHelper::drawColoredQuad(
            px - sz * 0.5f, py - sz * 0.5f, sz, sz,
            glm::vec4(0.9f, 0.8f, 0.6f, a),
            screenWidth, screenHeight);
    }
}

void TitleScreen::renderFadeOut() {
    // Smooth fade to black transition
    transitionAlpha += 0.016f / TRANSITION_FADE_DURATION; // Assuming ~60fps
    if (transitionAlpha > 1.0f) transitionAlpha = 1.0f;

    // Render the menu underneath fading out
    renderBackground(1.0f - transitionAlpha, true);
    renderSepiaOverlay();
    renderVignette();
    renderParticles();
    renderOblivionLogo(1.0f - transitionAlpha, false);

    // Black overlay with increasing opacity
    UIDrawHelper::drawColoredQuad(
        0.0f, 0.0f,
        static_cast<float>(screenWidth), static_cast<float>(screenHeight),
        glm::vec4(0.0f, 0.0f, 0.0f, transitionAlpha),
        screenWidth, screenHeight);
}

void TitleScreen::onTouchEvent(float x, float y, int action) {
    if (state == TitleScreenState::INTRO_MOVIE) {
        if (action == 0) {
            spawnRipple(x, y);
            transitionToLogo();
        }
    } else if (state == TitleScreenState::LOGO_DISPLAY) {
        if (action == 0) {
            spawnRipple(x, y);
            transitionToMenu();
        }
    } else if (state == TitleScreenState::MENU) {
        if (menuPanel) {
            if (action == 0) {
                lastTouchX = x;
                lastTouchY = y;
                spawnRipple(x, y);
                menuPanel->onTouchDown(x, y, 0);
            } else if (action == 1) {
                menuPanel->onTouchUp(x, y, 0);
            } else if (action == 2) {
                float dx = x - lastTouchX;
                float dy = y - lastTouchY;
                lastTouchX = x;
                lastTouchY = y;
                menuPanel->onTouchMove(x, y, dx, dy, 0);
            }
        }
    }
}

// Debug helper: directly start a new game bypassing the menu UI hit-test.
void TitleScreen::debugStartNewGame() {
    if (state == TitleScreenState::MENU) {
        selectedIndex = MENU_NEW;
        handleMenuSelection();
    }
}

void TitleScreen::onKeyPress(int key) {
    if (state == TitleScreenState::INTRO_MOVIE) {
        transitionToLogo();
    } else if (state == TitleScreenState::LOGO_DISPLAY) {
        transitionToMenu();
    } else if (state == TitleScreenState::MENU) {
        if (key == 19) {
            selectedIndex = (selectedIndex - 1 + static_cast<int>(menuButtons.size())) % static_cast<int>(menuButtons.size());
            playUINavigateSound();
        } else if (key == 20) {
            selectedIndex = (selectedIndex + 1) % static_cast<int>(menuButtons.size());
            playUINavigateSound();
        } else if (key == 23 || key == 66) {
            handleMenuSelection();
        }
    }
}

void TitleScreen::transitionToLogo() {
    state = TitleScreenState::LOGO_DISPLAY;
    displayTimer = 0.0f;
    logoFadeAlpha = 0.0f;
    LOGI("Transitioned to logo display");
}

void TitleScreen::transitionToMenu() {
    state = TitleScreenState::MENU;
    displayTimer = 0.0f;
    selectedIndex = 0;
    menuAnimTimer = 0.0f;
    menuFadeAlpha = 0.0f;
    menuSlideOffset = 50.0f;

    // Initialize per-button staggered animation
    for (int i = 0; i < MAX_MENU_BUTTONS; ++i) {
        buttonAnimTimers[i] = -BUTTON_STAGGER_DELAY * i;
        buttonAlphas[i] = 0.0f;
        buttonSlideOffsets[i] = 80.0f;
    }

    // Initialize selection bar
    selectionBarAlpha = 0.0f;
    selectionBarY = 0.0f;
    selectionBarTargetY = 0.0f;

    // Initialize logo glow
    logoGlowIntensity = 0.5f;

    LOGI("Transitioned to menu");
}

void TitleScreen::updateMenu(float deltaTime) {
    // Menu fade-in and slide animation
    if (menuAnimTimer < MENU_FADE_DURATION) {
        menuAnimTimer += deltaTime;
        float t = menuAnimTimer / MENU_FADE_DURATION;
        if (t > 1.0f) t = 1.0f;
        menuFadeAlpha = easeOutQuad(t);
        menuSlideOffset = 50.0f * (1.0f - easeOutQuad(t));
    }

    // Per-button staggered animation
    for (int i = 0; i < MAX_MENU_BUTTONS; ++i) {
        if (buttonAnimTimers[i] < BUTTON_ANIM_DURATION) {
            buttonAnimTimers[i] += deltaTime;
            float t = buttonAnimTimers[i] / BUTTON_ANIM_DURATION;
            if (t < 0.0f) t = 0.0f;
        }
        float t = buttonAnimTimers[i] / BUTTON_ANIM_DURATION;
        if (t > 1.0f) t = 1.0f;
        buttonAlphas[i] = easeOutQuad(t);
        buttonSlideOffsets[i] = 80.0f * (1.0f - easeOutQuad(t));
    }

    // Selection bar smooth movement
    float barTargetY = static_cast<float>(selectedIndex) * 52.0f;
    selectionBarY += (barTargetY - selectionBarY) * SELECTION_BAR_SPEED * deltaTime;
    selectionBarAlpha = 0.8f + 0.2f * sin(glowPhase * 2.0f);

    // Logo glow animation
    logoGlowIntensity = 0.5f + 0.5f * sin(glowPhase * LOGO_GLOW_SPEED);
}

void TitleScreen::handleMenuSelection() {
    if (selectedIndex >= static_cast<int>(menuItems.size())) return;

    const std::string& selected = menuItems[selectedIndex];
    playUISelectSound();

    if (selected == "menu_new") {
        state = TitleScreenState::TRANSITIONING;
        transitionAlpha = 0.0f;
        gameStarted = true;
        LOGI("Menu selection: New Game");
    } else if (selected == "menu_load") {
        loadGameRequested = true;
        LOGI("Menu selection: Load Game");
    } else if (selected == "menu_options") {
        settingsRequested = true;
        LOGI("Menu selection: Options");
    } else if (selected == "menu_credits") {
        creditsRequested = true;
        LOGI("Menu selection: Credits");
    } else if (selected == "menu_quit") {
        quitRequested = true;
        LOGI("Menu selection: Quit");
    }
}

void TitleScreen::spawnRipple(float x, float y) {
    auto& r = ripples[nextRippleIndex];
    r.x = x;
    r.y = y;
    r.radius = 0.0f;
    r.maxRadius = 80.0f;
    r.alpha = 0.6f;
    r.speed = 300.0f;
    r.active = true;
    nextRippleIndex = (nextRippleIndex + 1) % MAX_RIPPLES;
}

void TitleScreen::renderRipples() {
    for (int i = 0; i < MAX_RIPPLES; ++i) {
        auto& r = ripples[i];
        if (!r.active) continue;

        float thickness = 3.0f;
        float innerR = r.radius - thickness;
        if (innerR < 0.0f) innerR = 0.0f;

        // Outer circle approximation using colored quad ring
        glm::vec4 rippleColor(COLOR_GOLD.x, COLOR_GOLD.y, COLOR_GOLD.z, r.alpha * 0.5f);
        UIDrawHelper::drawColoredQuad(
            r.x - r.radius, r.y - r.radius,
            r.radius * 2.0f, r.radius * 2.0f,
            rippleColor, screenWidth, screenHeight);

        // Inner clear circle
        glm::vec4 clearColor(0.0f, 0.0f, 0.0f, 0.0f);
        UIDrawHelper::drawColoredQuad(
            r.x - innerR, r.y - innerR,
            innerR * 2.0f, innerR * 2.0f,
            clearColor, screenWidth, screenHeight);
    }
}

void TitleScreen::playUINavigateSound() {
    if (audioManager && audioManager->hasSoundDefinitions()) {
        audioManager->playSound("ui/navigate");
    }
}

void TitleScreen::playUISelectSound() {
    if (audioManager && audioManager->hasSoundDefinitions()) {
        audioManager->playSound("ui/select");
    }
}
