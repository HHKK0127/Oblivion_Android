#include "title_screen.h"
#include "text_renderer.h"
#include "../engine/texture_loader.h"
#include "../audio/audio_manager.h"
#include "ui_draw_helper.h"
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#include <cmath>

// Re-define LOG_TAG after includes to override audio LOG_TAG
#undef LOG_TAG
#undef LOGD
#undef LOGI
#undef LOGW
#define LOG_TAG "TitleScreen"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

// OES texture shader for video background (MediaPlayer SurfaceTexture)
static const char* s_oesVertexShader = R"(#version 300 es
layout(location = 0) in vec2 aPosition;
layout(location = 1) in vec2 aTexCoord;
uniform mat4 uProjection;
out vec2 vTexCoord;
void main() {
    gl_Position = uProjection * vec4(aPosition, 0.0, 1.0);
    vTexCoord = aTexCoord;
}
)";

static const char* s_oesFragmentShader = R"(#version 300 es
#extension GL_OES_EGL_image_external_essl3 : require
precision mediump float;
in vec2 vTexCoord;
out vec4 fragColor;
uniform samplerExternalOES uTexture;
uniform vec4 uColor;
void main() {
    fragColor = texture(uTexture, vTexCoord) * uColor;
}
)";

static GLuint s_oesProgram = 0;
static GLuint s_oesVAO = 0;
static GLuint s_oesVBO = 0;

static void ensureOESShader() {
    if (s_oesProgram != 0) return;

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &s_oesVertexShader, nullptr);
    glCompileShader(vs);

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &s_oesFragmentShader, nullptr);
    glCompileShader(fs);

    s_oesProgram = glCreateProgram();
    glAttachShader(s_oesProgram, vs);
    glAttachShader(s_oesProgram, fs);
    glLinkProgram(s_oesProgram);

    glDeleteShader(vs);
    glDeleteShader(fs);

    glGenVertexArrays(1, &s_oesVAO);
    glGenBuffers(1, &s_oesVBO);
    LOGI("OES texture shader compiled and linked: program=%u", s_oesProgram);
}

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
    // Clean up OES shader resources
    if (s_oesProgram != 0) {
        glDeleteProgram(s_oesProgram);
        s_oesProgram = 0;
    }
    if (s_oesVAO != 0) {
        glDeleteVertexArrays(1, &s_oesVAO);
        s_oesVAO = 0;
    }
    if (s_oesVBO != 0) {
        glDeleteBuffers(1, &s_oesVBO);
        s_oesVBO = 0;
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

    // Register intro video clip if BinkVideoPlayer is available
    setupIntroVideo();

    // Skip intro movie if no video player available
    if (!videoPlaybackActive && !videoPlayer) {
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
        selectionBarAlpha = 0.0f;
        selectionBarY = 0.0f;
        selectionBarTargetY = 0.0f;
        logoGlowIntensity = 0.5f;
        LOGI("TitleScreen initialized (Oblivion Authentic) - skipping to MENU");
    } else {
        LOGI("TitleScreen initialized (Oblivion Authentic)");
    }
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
    menuButtons.clear();
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
        btn->setLabelScale(1.3f);
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
    LOGD("=== rebuildMenuLayout called, buttons=%zu, screen=%dx%d ===",
          menuButtons.size(), screenWidth, screenHeight);
    if (!menuPanel) return;
    menuPanel->setScreenSize(screenWidth, screenHeight);

    // Original Oblivion: buttons centered horizontally, lower-center area
    // Original 1280x1024: buttons at Y~620, each ~150px wide, 8px gap
    float btnW = 150.0f;
    float btnH = 38.0f;
    float gap = 8.0f;
    float totalW = static_cast<float>(menuButtons.size()) * btnW
                 + static_cast<float>(menuButtons.size() - 1) * gap;
    float panelW = totalW + 30.0f;
    float panelH = btnH + 20.0f;

    // Position: center-lower (original Oblivion ~60% from top)
    float px = (screenWidth - panelW) / 2.0f;
    float py = screenHeight * 0.60f;
    menuPanel->setPosition(px, py);
    menuPanel->setSize(panelW, panelH);
    LOGD("Panel: pos=(%.1f, %.1f), size=(%.1f, %.1f)", px, py, panelW, panelH);

    // Horizontal layout: buttons positioned relative to panel
    float startX = 15.0f;
    float startY = 10.0f;
    for (size_t i = 0; i < menuButtons.size(); ++i) {
        float bx = startX + static_cast<float>(i) * (btnW + gap);
        menuButtons[i]->setPosition(bx, startY);
        menuButtons[i]->setSize(btnW, btnH);
        menuButtons[i]->setScreenSize(screenWidth, screenHeight);
        LOGD("Button %zu: pos=(%.1f, %.1f), size=(%.1f, %.1f)", i, bx, startY, btnW, btnH);
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

            // Update BinkVideoPlayer if video is playing
            if (videoPlaybackActive && videoPlayer) {
                videoPlayer->update(deltaTime);

                // Monitor video player state
                if (!videoPlayer->isPlaying() && videoPlayer->getState() != oblivion::video::VideoState::FINISHED) {
                    // Video stopped unexpectedly
                    LOGW("Video player stopped unexpectedly, treating as completed");
                    videoPlaybackActive = false;
                    videoCompleted = true;
                }

                if (videoCompleted) {
                    videoPlaybackActive = false;
                    transitionToLogo();
                    break;
                }
            }

            // Reset videoInitAttempted after timeout to allow retry
            if (!videoPlaybackActive && videoInitAttempted) {
                static float retryTimer = 0.0f;
                retryTimer += deltaTime;
                if (retryTimer > 5.0f) {
                    LOGI("Resetting videoInitAttempted for retry");
                    videoInitAttempted = false;
                    retryTimer = 0.0f;
                }
            }

            // Fallback: procedural logo fade if no video
            if (!videoPlaybackActive) {
                float t = displayTimer / INTRO_DURATION;
                if (t > 1.0f) t = 1.0f;
                introLogoAlpha = easeOutQuad(t);
                if (displayTimer >= INTRO_DURATION) {
                    LOGI("INTRO_MOVIE timer reached %.1f, transitioning to logo", displayTimer);
                    transitionToLogo();
                }
            }
            break;
        }
        case TitleScreenState::LOGO_DISPLAY: {
            displayTimer += deltaTime;
            float t = displayTimer / LOGO_FADE_DURATION;
            if (t > 1.0f) t = 1.0f;
            logoFadeAlpha = easeInQuad(t);
            // Stay in LOGO_DISPLAY until user taps (handled in onTouchEvent)
            break;
        }
        case TitleScreenState::MENU: {
            updateMenu(deltaTime);
            break;
        }
        case TitleScreenState::CREDITS: {
            // Fade in credits
            if (creditsFadeTimer < CREDITS_FADE_DURATION) {
                creditsFadeTimer += deltaTime;
                float t = creditsFadeTimer / CREDITS_FADE_DURATION;
                if (t > 1.0f) t = 1.0f;
                creditsAlpha = easeOutQuad(t);
            }
            // Auto-scroll credits
            creditsScrollY += deltaTime * 60.0f;
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
        case TitleScreenState::CREDITS:
            renderCredits();
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

    // If video is playing, the VideoRenderer handles frame display
    // (frames are rendered to the SurfaceTexture which is composited by the system).
    // We only draw a fallback logo when video is NOT active.
    if (videoPlaybackActive) {
        // Video frames are rendered via VideoRenderer to the native window.
        // Draw a subtle logo overlay with fade-in after a short delay.
        if (displayTimer > 1.0f && logoTexture != 0) {
            float overlayAlpha = std::min((displayTimer - 1.0f) * 0.5f, 0.15f);
            float scale = (screenWidth > screenHeight) ? 0.35f : 0.6f;
            float logoW = screenWidth * scale;
            float logoH = logoW * 0.22f;
            float logoX = (screenWidth - logoW) * 0.5f;
            float logoY = screenHeight * 0.80f;

            UIDrawHelper::drawTexturedQuad(
                logoX, logoY, logoW, logoH,
                logoTexture, glm::vec4(1.0f, 1.0f, 1.0f, overlayAlpha),
                screenWidth, screenHeight);
        }
        return;
    }

    // Fallback: static logo fade-in (original behavior)
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

    // Show "PRESS ANY KEY" once logo fade is complete
    if (logoFadeAlpha >= 0.9f) {
        float msgT = displayTimer - LOGO_FADE_DURATION;
        if (msgT < 0.0f) msgT = 0.0f;
        float msgAlpha = std::min(msgT * 3.0f, 1.0f);
        msgAlpha *= 0.5f + 0.5f * sin(displayTimer * 2.5f);
        renderPressAnyKey(msgAlpha);
    }
}

void TitleScreen::renderMenu() {
    renderBackground(1.0f, true);
    renderSepiaOverlay();
    renderVignette();
    renderParticles();
    renderOblivionLogo(1.0f, true);  // Keep logo at same position as LOGO_DISPLAY

    // Apply slide offset to menu panel position
    if (menuPanel) {
        float btnW = 150.0f;
        float gap = 8.0f;
        float totalW = static_cast<float>(menuButtons.size()) * btnW
                     + static_cast<float>(menuButtons.size() - 1) * gap;
        float panelW = totalW + 30.0f;
        float baseX = (screenWidth - panelW) / 2.0f;
        menuPanel->setPosition(baseX - menuSlideOffset, screenHeight * 0.60f);
    }

    // Draw selection indicator bar
    if (menuPanel) {
        float btnW = 150.0f;
        float gap = 8.0f;
        float totalW = static_cast<float>(menuButtons.size()) * btnW
                     + static_cast<float>(menuButtons.size() - 1) * gap;
        float panelW = totalW + 30.0f;
        float panelX = (screenWidth - panelW) / 2.0f - menuSlideOffset;
        float panelY = screenHeight * 0.60f;
        float btnStartX = panelX + 15.0f;
        float barX = btnStartX + selectionBarY + 5.0f; // selectionBarY holds animated X offset
        float barW = 6.0f;
        float barH = 36.0f;
        float barYPos = panelY + 10.0f + 3.0f;

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
            glm::vec3 c(COLOR_MENU_TEXT_SELECTED.x + (COLOR_GOLD.x - COLOR_MENU_TEXT_SELECTED.x) * glow * 0.5f,
                        COLOR_MENU_TEXT_SELECTED.y + (COLOR_GOLD.y - COLOR_MENU_TEXT_SELECTED.y) * glow * 0.5f,
                        COLOR_MENU_TEXT_SELECTED.z + (COLOR_GOLD.z - COLOR_MENU_TEXT_SELECTED.z) * glow * 0.5f);
            menuButtons[i]->setLabelColor(c);
        } else {
            menuButtons[i]->setLabelColor(COLOR_MENU_TEXT);
        }
    }

    // Render buttons directly at absolute positions (no panel dependency)
    // Temporarily set panel to origin so child absolute positions match screen coords
    if (menuPanel) {
        menuPanel->setPosition(0.0f, 0.0f);
        menuPanel->setSize(static_cast<float>(screenWidth), static_cast<float>(screenHeight));
    }
    {
        float btnW = 150.0f;
        float btnH = 38.0f;
        float gap = 8.0f;
        float totalW = static_cast<float>(menuButtons.size()) * btnW
                     + static_cast<float>(menuButtons.size() - 1) * gap;
        float startX = (screenWidth - totalW) / 2.0f - menuSlideOffset;
        float startY = screenHeight * 0.60f + 10.0f;

        // Draw selection bar before buttons
        if (selectionBarAlpha > 0.0f) {
            float barX = startX + selectionBarY + 5.0f;
            float barYPos = startY + 1.0f;
            glm::vec4 barColor(COLOR_GOLD.x, COLOR_GOLD.y, COLOR_GOLD.z, selectionBarAlpha * menuFadeAlpha);
            UIDrawHelper::drawColoredQuad(barX, barYPos, 6.0f, btnH - 2.0f,
                                          barColor, screenWidth, screenHeight);
            // Glow
            glm::vec4 glowColor(COLOR_GOLD.x, COLOR_GOLD.y, COLOR_GOLD.z, 0.2f * menuFadeAlpha);
            UIDrawHelper::drawColoredQuad(barX - 4.0f, barYPos - 2.0f, 14.0f, btnH + 2.0f,
                                          glowColor, screenWidth, screenHeight);
        }

        for (size_t i = 0; i < menuButtons.size(); ++i) {
            float bx = startX + static_cast<float>(i) * (btnW + gap);
            menuButtons[i]->setPosition(bx, startY);
            menuButtons[i]->setSize(btnW, btnH);
            // Use Oblivion font for menu labels
            bool isSelected = (static_cast<int>(i) == selectedIndex);
            int idx = static_cast<int>(i);
            float buttonAlpha = buttonAlphas[idx];
            glm::vec3 labelColor = menuButtons[i]->getLabelColor();
            float labelScale = 1.3f;
            float textW = textRenderer->getTextWidth(menuButtons[i]->getLabel(), labelScale);
            float textX = bx + (btnW - textW) * 0.5f;
            float textY = startY + btnH * 0.65f;
            float textAlpha = buttonAlpha * menuFadeAlpha;
            if (textAlpha < 0.0f) textAlpha = 0.0f;
            if (textAlpha > 1.0f) textAlpha = 1.0f;

            // Reset GL state before text rendering
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            textRenderer->renderText(menuButtons[i]->getLabel(), textX, textY,
                glm::vec4(labelColor.x, labelColor.y, labelColor.z, textAlpha), labelScale);
        }
    }
    renderVersionText();
}

void TitleScreen::renderBackground(float alpha, bool menuMode) {
    // If video background is active, render it instead of static background
    if (videoBackgroundActive && videoBackgroundTexture != 0) {
        renderVideoBackground(alpha);

        // Add a dark overlay for readability when in menu mode
        if (menuMode) {
            float w = static_cast<float>(screenWidth);
            float h = static_cast<float>(screenHeight);
            UIDrawHelper::drawColoredQuad(
                0.0f, 0.0f, w, h,
                glm::vec4(0.0f, 0.0f, 0.0f, 0.35f * alpha),
                screenWidth, screenHeight);
        }
        return;
    }

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

void TitleScreen::renderVideoBackground(float alpha) {
    if (!videoBackgroundActive || videoBackgroundTexture == 0) return;

    ensureOESShader();
    if (s_oesProgram == 0) return;

    float w = static_cast<float>(screenWidth);
    float h = static_cast<float>(screenHeight);

    // Compute aspect-correct UV for 16:9 video on screen
    // Map loop.mp4 is 1280x720 (16:9), screen may be different aspect
    float screenAspect = w / h;
    float videoAspect = 16.0f / 9.0f;
    float uMin = 0.0f, vMin = 0.0f, uMax = 1.0f, vMax = 1.0f;

    if (screenAspect > videoAspect) {
        // Screen is wider than video - crop top/bottom
        float scale = videoAspect / screenAspect;
        vMin = (1.0f - scale) * 0.5f;
        vMax = 1.0f - vMin;
    } else if (screenAspect < videoAspect) {
        // Screen is taller than video - crop left/right
        float scale = screenAspect / videoAspect;
        uMin = (1.0f - scale) * 0.5f;
        uMax = 1.0f - uMin;
    }

    glUseProgram(s_oesProgram);

    float left = 0.0f, right = w, top = 0.0f, bottom = h;
    float projection[16] = {
        2.0f / (right - left), 0.0f, 0.0f, 0.0f,
        0.0f, -2.0f / (bottom - top), 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        -(right + left) / (right - left), (bottom + top) / (bottom - top), 0.0f, 1.0f
    };

    GLint projLoc = glGetUniformLocation(s_oesProgram, "uProjection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, projection);

    GLint colorLoc = glGetUniformLocation(s_oesProgram, "uColor");
    glUniform4f(colorLoc, 1.0f, 1.0f, 1.0f, alpha);

    GLint texLoc = glGetUniformLocation(s_oesProgram, "uTexture");
    glUniform1i(texLoc, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, videoBackgroundTexture);

    float vertices[16] = {
        0.0f, 0.0f, uMin, vMin,
        w,    0.0f, uMax, vMin,
        0.0f, h,    uMin, vMax,
        w,    h,    uMax, vMax
    };

    glBindVertexArray(s_oesVAO);
    glBindBuffer(GL_ARRAY_BUFFER, s_oesVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);
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
    // Position below the logo (logo is at screenHeight * 0.30, height ~ screenWidth * 0.55 * 0.25)
    float logoBottom = screenHeight * 0.30f + screenWidth * 0.55f * 0.25f + 20.0f;
    float textY = logoBottom;

    glm::vec3 hintColor(COLOR_PARCHMENT.x, COLOR_PARCHMENT.y, COLOR_PARCHMENT.z);
    textRenderer->renderText(hint, textX, textY, glm::vec3(hintColor.x, hintColor.y, hintColor.z), fontScale);
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
            // Skip video if playing, otherwise skip fallback
            if (videoPlaybackActive && videoPlayer) {
                videoPlayer->stop();
                videoPlaybackActive = false;
            }
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
                LOGI("Menu touch DOWN at (%.1f, %.1f), panel pos=(%.1f, %.1f), panel size=(%.1f, %.1f)",
                     x, y, menuPanel->getPosition().x, menuPanel->getPosition().y,
                     menuPanel->getSize().x, menuPanel->getSize().y);
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
    } else if (state == TitleScreenState::CREDITS) {
        if (action == 0) {
            // Tap to return to menu
            state = TitleScreenState::MENU;
            creditsActive = false;
            LOGI("Credits dismissed");
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
    } else if (state == TitleScreenState::CREDITS) {
        if (key == 4 || key == 23 || key == 66) {
            // Back or Enter to return to menu
            state = TitleScreenState::MENU;
            creditsActive = false;
            LOGI("Credits dismissed");
        }
    }
}

void TitleScreen::setupIntroVideo() {
    if (videoInitAttempted) return;
    videoInitAttempted = true;

    auto& player = oblivion::video::BinkVideoPlayer::instance();
    if (!player.isInitialized()) {
        LOGI("BinkVideoPlayer not initialized, skipping intro video");
        return;
    }

    videoPlayer = &player;

    // Register the intro video clip
    oblivion::video::VideoClip introClip;
    introClip.clipId = "oblivion_intro";
    introClip.filePath = "videos/oblivion_intro.mp4";
    introClip.width = 1280;
    introClip.height = 720;
    introClip.frameRate = 30.0f;
    introClip.durationSeconds = 115.5f;  // 1:55.48 actual duration
    introClip.hasAudio = true;

    if (!player.loadClip(introClip.clipId, introClip)) {
        LOGW("Failed to load intro video clip");
        videoPlayer = nullptr;
        return;
    }

    // Set up completion callback
    videoCallbacks.onComplete = [this](const std::string& clipId) {
        LOGI("Intro video completed: %s", clipId.c_str());
        videoCompleted = true;
    };
    videoCallbacks.onError = [this](const std::string& clipId, int errCode, const std::string& errMsg) {
        LOGW("Intro video error [%d]: %s - %s", errCode, clipId.c_str(), errMsg.c_str());
        videoCompleted = true;  // Treat error as completion to proceed
    };
    player.setCallbacks(videoCallbacks);

    // Try to start intro video playback
    if (player.playIntroVideo("oblivion_intro")) {
        videoPlaybackActive = true;
        videoCompleted = false;
        LOGI("Intro video playback started");
    } else {
        LOGI("Intro video not available, using fallback logo");
        videoPlayer = nullptr;
    }
}

void TitleScreen::transitionToLogo() {
    // Stop video if still playing
    if (videoPlaybackActive && videoPlayer) {
        videoPlayer->stop();
        videoPlaybackActive = false;
    }

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

    // Selection bar smooth movement (horizontal)
    float barTargetX = static_cast<float>(selectedIndex) * 158.0f; // btnW(150) + gap(8)
    selectionBarY += (barTargetX - selectionBarY) * SELECTION_BAR_SPEED * deltaTime;
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
        state = TitleScreenState::CREDITS;
        creditsActive = true;
        creditsFadeTimer = 0.0f;
        creditsAlpha = 0.0f;
        creditsScrollY = 0.0f;
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

void TitleScreen::renderCredits() {
    renderBackground(1.0f, true);
    renderSepiaOverlay();
    renderVignette();

    if (!textRenderer) return;

    float minDim = static_cast<float>(std::min(screenWidth, screenHeight));
    float scale = minDim / 1080.0f;
    if (scale < 0.5f) scale = 0.5f;
    if (scale > 2.0f) scale = 2.0f;

    // Dark overlay
    UIDrawHelper::drawColoredQuad(
        0.0f, 0.0f,
        static_cast<float>(screenWidth), static_cast<float>(screenHeight),
        glm::vec4(0.0f, 0.0f, 0.0f, 0.85f * creditsAlpha),
        screenWidth, screenHeight);

    // Title
    float titleScale = 1.2f * scale;
    const char* title = "CREDITS";
    float titleW = textRenderer->getTextWidth(title, titleScale);
    float titleX = (static_cast<float>(screenWidth) - titleW) * 0.5f;
    float titleY = 80.0f * scale;
    glm::vec3 gold(COLOR_GOLD.x, COLOR_GOLD.y, COLOR_GOLD.z);
    textRenderer->renderText(title, titleX, titleY, gold * creditsAlpha, titleScale);

    // Credits content
    struct CreditLine {
        const char* text;
        float scaleMul;
        bool isHeader;
    };

    CreditLine credits[] = {
        {"OBLIVION ANDROID", 1.0f, true},
        {"", 0.5f, false},
        {"The Elder Scrolls IV: Oblivion", 0.7f, false},
        {"Originally developed by Bethesda Game Studios", 0.55f, false},
        {"", 0.5f, false},
        {"ANDROID PORT", 0.9f, true},
        {"", 0.5f, false},
        {"Engine Architecture", 0.65f, false},
        {"C++17 / OpenGL ES 3.0 / Android NDK", 0.55f, false},
        {"", 0.4f, false},
        {"Audio System", 0.65f, false},
        {"OpenAL-Soft / JNI Audio Bridge", 0.55f, false},
        {"", 0.4f, false},
        {"Physics", 0.65f, false},
        {"Jolt Physics Engine", 0.55f, false},
        {"", 0.4f, false},
        {"Asset Pipeline", 0.65f, false},
        {"NIF / DDS / BSA Loader", 0.55f, false},
        {"", 0.6f, false},
        {"SPECIAL THANKS", 0.9f, true},
        {"", 0.5f, false},
        {"Bethesda Game Studios", 0.65f, false},
        {"OpenMW Project", 0.65f, false},
        {"Android NDK Community", 0.65f, false},
        {"", 0.6f, false},
        {"", 0.6f, false},
        {"Tap anywhere to return", 0.6f, false},
    };

    int numLines = sizeof(credits) / sizeof(credits[0]);
    float baseY = screenHeight * 0.25f - creditsScrollY;
    float lineSpacing = 32.0f * scale;

    for (int i = 0; i < numLines; ++i) {
        float y = baseY + static_cast<float>(i) * lineSpacing;
        if (y < -50.0f || y > screenHeight + 50.0f) continue;

        float lineScale = credits[i].scaleMul * scale;
        float textW = textRenderer->getTextWidth(credits[i].text, lineScale);
        float textX = (static_cast<float>(screenWidth) - textW) * 0.5f;

        glm::vec3 color;
        if (credits[i].isHeader) {
            color = gold;
        } else {
            color = glm::vec3(COLOR_PARCHMENT.x, COLOR_PARCHMENT.y, COLOR_PARCHMENT.z);
        }

        float lineAlpha = creditsAlpha;
        // Fade out at edges
        if (y < screenHeight * 0.15f) {
            lineAlpha *= (y - screenHeight * 0.05f) / (screenHeight * 0.10f);
        } else if (y > screenHeight * 0.85f) {
            lineAlpha *= (screenHeight * 0.95f - y) / (screenHeight * 0.10f);
        }
        if (lineAlpha < 0.0f) lineAlpha = 0.0f;
        if (lineAlpha > 1.0f) lineAlpha = 1.0f;

        textRenderer->renderText(credits[i].text, textX, y, color * lineAlpha, lineScale);
    }
}
