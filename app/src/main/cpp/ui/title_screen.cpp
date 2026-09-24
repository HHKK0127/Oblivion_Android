#include "title_screen.h"
#include "text_renderer.h"
#include "../engine/texture_loader.h"
#include "../audio/audio_manager.h"
#include "ui_draw_helper.h"
#include <GLES3/gl3.h>
#include <GLES2/gl2ext.h>
#include <algorithm>
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
    TextureLoader::deleteTexture(selectBarTexture);
    TextureLoader::deleteTexture(selectCutTexture);
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
    menuItems.push_back("menu_continue");
    menuItems.push_back("menu_new");
    menuItems.push_back("menu_load");
    menuItems.push_back("menu_options");
    menuItems.push_back("menu_credits");
    menuItems.push_back("menu_exit");

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
        selectBarTexture = TextureLoader::loadTextureFromAsset("textures/ui/dialog_selection_full.png");
        selectCutTexture = TextureLoader::loadTextureFromAsset("textures/ui/dialog_selection_cut.png");

        texturesLoaded = true;
        LOGI("TitleScreen textures: bg=%u logo=%u vignette=%u selectBar=%u selectCut=%u",
             bgTexture, logoTexture, vignetteTexture, selectBarTexture, selectCutTexture);
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
            buttonSlideOffsets[i] = 60.0f;
        }
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
        {MENU_CONTINUE, "menu_continue"},
        {MENU_NEW,      "menu_new"},
        {MENU_LOAD,     "menu_load"},
        {MENU_OPTIONS,  "menu_options"},
        {MENU_CREDITS,  "menu_credits"},
        {MENU_EXIT,     "menu_exit"}
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
    // The panel is only a hit-test host: the row is drawn directly in renderMenu, so
    // the panel sits at the origin and the buttons carry absolute screen coordinates.
    menuPanel->setPosition(0.0f, 0.0f);
    menuPanel->setSize(static_cast<float>(screenWidth), static_cast<float>(screenHeight));

    computeMenuLayout();

    LOGD("Menu row: startX=%.1f baselineY=%.1f scale=%.4f gap=%.1f cap=%.1f",
         menuLayout.startX, menuLayout.baselineY, menuLayout.scale, menuLayout.gap,
         menuLayout.capHeight);
}

// Single horizontal row of items near the bottom of the screen, aligned on a shared
// baseline. Proportions come from the original main menu art (16:9 reference).
void TitleScreen::computeMenuLayout() {
    const int n = static_cast<int>(menuButtons.size());
    menuLayout = MenuRowLayout{};
    if (n <= 0 || n > MAX_MENU_BUTTONS || !textRenderer) return;

    // Measure every label at scale 1. renderText applies the same size multiplier that
    // getTextWidth folds in, so width[i] * scale is the exact drawn width.
    float total = 0.0f;
    for (int i = 0; i < n; ++i) {
        menuLayout.width[i] = textRenderer->getTextWidth(menuButtons[i]->getLabel(), 1.0f);
        total += menuLayout.width[i];
    }
    if (total <= 0.0f) return;

    // Reference fit: one row spanning MENU_ROW_SPAN of the frame, with reference sized
    // gaps between the items. The original art scales the whole row with the frame, so
    // this single rule is what reproduces it.
    const float referenceGap = static_cast<float>(screenWidth) * MENU_ROW_GAP;
    const float span = static_cast<float>(screenWidth) * MENU_ROW_SPAN;
    const float referenceBudget = span - referenceGap * static_cast<float>(n - 1);
    const float referenceScale = (referenceBudget > 0.0f) ? referenceBudget / total : 1.0f;

    // Asked-for size. The row is laid out at this size first; what gives way is the gap
    // between the items, not the width or the centre of the row.
    const float capAtScaleOne = textRenderer->getTextCapHeight(1.0f);
    const float askedCap = static_cast<float>(screenHeight) * MENU_CAP_HEIGHT_RATIO;
    const float askedScale = (capAtScaleOne > 0.0f) ? askedCap / capAtScaleOne
                                                    : referenceScale;
    float scale = std::max(referenceScale, askedScale);

    const float maxRowWidth = static_cast<float>(screenWidth) * MENU_ROW_MAX_SPAN;
    const float minGap = static_cast<float>(screenWidth) * MENU_GAP_MIN_RATIO;
    float gap = referenceGap;
    if (n > 1) {
        // The gap that lands the row exactly on the reference span: equal to the art's gap
        // when the row is drawn at the art's size, and smaller than it whenever the row is
        // grown for legibility. The art's gap is never exceeded.
        const float spanGap = (span - total * scale) / static_cast<float>(n - 1);
        gap = std::min(referenceGap, std::max(spanGap, minGap));

        // On a narrow frame even the tightest gap can leave the row too wide.
        // Trade capitals down rather than let the row overhang the frame edges.
        const float maxTotal = maxRowWidth - gap * static_cast<float>(n - 1);
        if (maxTotal > 0.0f && total * scale > maxTotal) {
            scale = maxTotal / total;
        }
    }
    float rowWidth = total * scale + gap * static_cast<float>(n - 1);

    menuLayout.scale = scale;
    menuLayout.gap = gap;
    menuLayout.capHeight = capAtScaleOne * scale;
    menuLayout.startX = (static_cast<float>(screenWidth) - rowWidth) * 0.5f;

    // The reference row is baseline aligned; centre the capitals on the reference line.
    menuLayout.baselineY = static_cast<float>(screenHeight) * MENU_ROW_CENTER_Y +
                           menuLayout.capHeight * 0.5f;
    // This function draws the row, but the touches go through the menu panel, so the
    // button rectangles have to follow every recomputed layout.
    applyMenuLayoutToButtons();
}

void TitleScreen::applyMenuLayoutToButtons() {
    if (menuButtons.empty()) return;

    float hitH = std::max(menuLayout.capHeight, 24.0f) * 1.8f;
    float hitY = menuLayout.baselineY - hitH * 0.78f;
    float x = menuLayout.startX;
    for (size_t i = 0; i < menuButtons.size(); ++i) {
        float w = menuLayout.width[i] * menuLayout.scale;
        menuButtons[i]->setPosition(x, hitY);
        menuButtons[i]->setSize(w, hitH);
        menuButtons[i]->setScreenSize(screenWidth, screenHeight);
        x += w + menuLayout.gap;
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
    // The debug menu can hand styling back to the renderer while this screen is up, so
    // drop the plain style as soon as that happens.
    if (plainStyleDisabled && fontStyleApplied) {
        restoreSavedFontStyle();
    }
    if (!fontStyleReleased) {
        applyPlainFontStyle();
    }

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

    // The menu has committed to leaving the title screen, so give the renderer its
    // styling back. The remaining fade-out frames keep the plain style.
    if (!fontStyleReleased && (gameStarted || quitRequested)) {
        releasePlainFontStyle();
    }
}

// The title screen draws warm brown text over bright parchment, so the console's blunt
// alpha handling is skipped here. The renderer's coverage curve used to be forced off as
// well, but this face has an unusually soft coverage ramp: at the row's small capitals
// roughly half of every stroke is edge, and with the curve off the row read as a pale
// halo around a thin core (measured: glyph cores at RGB(117,59,33), a 4.5:1 average
// against the parchment, but only 2.8:1 for the cores themselves). The curve is left on
// with a gentler window than the UI default (0.30/0.72) so it clips the halo and fills
// the cores without the extra stroke width the harder window adds at this size; replayed
// over the atlas it is worth about +1% of ink mass on its own, so it is a finish rather
// than the cure.
//
// Weight comes from the renderer's outline instead, stamped in the ink colour rather than
// the renderer's black default. The row is drawn at the art's own capitals, where only
// about a third of the glyph cores reach full coverage; a sub-pixel ring in the same warm
// brown lifts that to about two thirds, and because the ring is the ink's own colour it
// cannot leave the near-black rim a black ring leaves (that ring put 28% of the row's ink
// at near-black, against none in the art, and fattened every glyph by several pixels a
// side at the sizes the row used to be drawn at). renderMenu keeps its own smaller dark
// shadow under the row. `fontcontrast` / `fontoutline on|off|width` still tune the
// renderer globally for every other screen.
void TitleScreen::applyPlainFontStyle() {
    if (!textRenderer || fontStyleApplied || plainStyleDisabled) return;
    savedFontAlphaCurve = textRenderer->isFontAlphaCurveEnabled();
    savedFontSizeMultiplier = textRenderer->getFontSizeMultiplier();
    savedFontAlphaCurveLo = textRenderer->getFontAlphaCurveLo();
    savedFontAlphaCurveHi = textRenderer->getFontAlphaCurveHi();
    savedFontOutline = textRenderer->isFontOutlineEnabled();
    savedFontOutlineWidth = textRenderer->getFontOutlineWidth();
    savedFontOutlineColor = textRenderer->getFontOutlineColor();
    textRenderer->setFontSizeMultiplier(1.0f);
    textRenderer->setFontAlphaCurve(MENU_INK_CURVE_LO, MENU_INK_CURVE_HI);
    textRenderer->setFontAlphaCurveEnabled(true);
    textRenderer->setFontOutlineColor(glm::vec4(COLOR_MENU_TEXT.x, COLOR_MENU_TEXT.y,
                                                COLOR_MENU_TEXT.z, MENU_OUTLINE_ALPHA));
    textRenderer->setFontOutlineWidth(MENU_OUTLINE_WIDTH);
    textRenderer->setFontOutlineEnabled(true);
    fontStyleApplied = true;
    LOGI("TitleScreen legibility pass: ink curve %.2f/%.2f on, ink-coloured outline %.2f px "
         "at alpha %.2f, shadow %.3f of cap",
         MENU_INK_CURVE_LO, MENU_INK_CURVE_HI, MENU_OUTLINE_WIDTH, MENU_OUTLINE_ALPHA,
         MENU_SHADOW_OFFSET_RATIO);
}

void TitleScreen::restoreSavedFontStyle() {
    if (!textRenderer) return;
    textRenderer->setFontAlphaCurveEnabled(savedFontAlphaCurve);
    textRenderer->setFontAlphaCurve(savedFontAlphaCurveLo, savedFontAlphaCurveHi);
    textRenderer->setFontSizeMultiplier(savedFontSizeMultiplier);
    textRenderer->setFontOutlineEnabled(savedFontOutline);
    textRenderer->setFontOutlineWidth(savedFontOutlineWidth);
    textRenderer->setFontOutlineColor(savedFontOutlineColor);
    fontStyleApplied = false;
}

void TitleScreen::releasePlainFontStyle() {
    if (!textRenderer || fontStyleReleased) return;
    if (fontStyleApplied) restoreSavedFontStyle();
    fontStyleReleased = true;
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

    computeMenuLayout();
    const int n = static_cast<int>(menuButtons.size());
    const float capHeight = menuLayout.capHeight;
    float textTop = menuLayout.baselineY - capHeight;

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // The parchment behind the row is bright and the background movie keeps moving under
    // it, while this face's strokes are thin; each label is therefore stamped twice, a
    // soft warm-black shadow offset down-right by a fraction of the capitals, then the ink
    // on top. The shadow is what holds the row legible when the frame behind it darkens.
    const float shadowOffset = std::max(1.5f, capHeight * MENU_SHADOW_OFFSET_RATIO);

    // The original marks the hovered item with a parchment highlight texture behind the
    // label instead of changing the ink (menus/prefabs/button_floating.xml), and that bar is
    // also what carries the row's contrast: its cream RGB(246,237,220) holds the declared
    // brown ink at 7.62:1 whatever the movie frame behind it happens to be doing. Drawn
    // first, so the ink and its shadow land on top of it.
    const bool useSelectBar = (selectBarTexture != 0);
    if (useSelectBar && selectedIndex >= 0 && selectedIndex < n) {
        const float unit = capHeight / MENU_SELECT_UNIT;
        float barItemX = menuLayout.startX;
        for (int i = 0; i < selectedIndex; ++i) {
            barItemX += menuLayout.width[i] * menuLayout.scale + menuLayout.gap;
        }
        const float barTextW = menuLayout.width[selectedIndex] * menuLayout.scale;
        const float barAlpha = std::clamp(buttonAlphas[selectedIndex] * menuFadeAlpha, 0.0f, 1.0f);
        if (barAlpha > 0.0f) {
            const float barH = MENU_SELECT_BAR_HEIGHT * unit;
            const float barY = textTop + menuSlideOffset + buttonSlideOffsets[selectedIndex]
                             + (capHeight - barH) * 0.5f + MENU_SELECT_BAR_DROP * unit;
            const float barVisible =
                barTextW + (MENU_SELECT_LEFT_PAD + MENU_SELECT_RIGHT_PAD) * unit;
            const float barX = barItemX - MENU_SELECT_LEFT_PAD * unit;
            const glm::vec4 barTint(1.0f, 1.0f, 1.0f, barAlpha);
            UIDrawHelper::drawTexturedQuad(barX, barY,
                barVisible / MENU_SELECT_BAR_OPAQUE, barH,
                selectBarTexture, barTint, screenWidth, screenHeight);
            if (selectCutTexture != 0) {
                const float capW = MENU_SELECT_CAP_WIDTH * unit;
                UIDrawHelper::drawTexturedQuad(
                    barItemX + barTextW + MENU_SELECT_CAP_RIGHT * unit - capW, barY, capW, barH,
                    selectCutTexture, barTint, screenWidth, screenHeight);
            }
        }
    }

    float selX = 0.0f, selY = 0.0f, selW = 0.0f, selAlpha = 0.0f;
    float x = menuLayout.startX;
    for (int i = 0; i < n; ++i) {
        const std::string& label = menuButtons[i]->getLabel();
        float w = menuLayout.width[i] * menuLayout.scale;
        float alpha = std::clamp(buttonAlphas[i] * menuFadeAlpha, 0.0f, 1.0f);
        float y = textTop + menuSlideOffset + buttonSlideOffsets[i];

        // All six labels keep the ink the menu declares, exactly as the art does - the bar
        // above is what marks the current item. Only when the bar's texture is missing does
        // the current item fall back to the heavier ink, with the underline below.
        glm::vec3 color = COLOR_MENU_TEXT;
        if (i == selectedIndex) {
            if (!useSelectBar) {
                color = COLOR_MENU_TEXT_SELECTED;
            }
            selX = x;
            selY = y;
            selW = w;
            selAlpha = alpha;
        }

        textRenderer->renderText(label, x + shadowOffset, y + shadowOffset,
            glm::vec4(COLOR_MENU_SHADOW.x, COLOR_MENU_SHADOW.y, COLOR_MENU_SHADOW.z,
                      alpha * MENU_SHADOW_ALPHA), menuLayout.scale);
        textRenderer->renderText(label, x, y,
            glm::vec4(color.x, color.y, color.z, alpha), menuLayout.scale);
        x += w + menuLayout.gap;
    }

    // Fallback selection marker: only used when the highlight bar's texture could not be
    // loaded, so that the current item is never indistinguishable from the rest.
    if (!useSelectBar && selW > 0.0f && selAlpha > 0.0f) {
        float pulse = 0.5f + 0.5f * sin(glowPhase);
        float h = std::max(2.0f, capHeight * 0.10f);
        glm::vec4 c(COLOR_MENU_TEXT_SELECTED.x, COLOR_MENU_TEXT_SELECTED.y,
                    COLOR_MENU_TEXT_SELECTED.z, (0.55f + 0.25f * pulse) * selAlpha);
        UIDrawHelper::drawColoredQuad(selX, selY + capHeight * 1.34f, selW, h,
                                      c, screenWidth, screenHeight);
    }
}

void TitleScreen::renderBackground(float alpha, bool menuMode) {
    // If video background is active, render it instead of static background
    if (videoBackgroundActive && videoBackgroundTexture != 0) {
        // The map video already carries the parchment tone, the dark top/bottom borders and
        // its own edge falloff, so it is drawn as-is. Measured against the reference art the
        // raw frame matches within 3%, while a dark overlay pushed the whole screen to ~0.65x.
        renderVideoBackground(alpha);
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
    // The map video is already sepia parchment, so tinting it again only darkens the frame.
    if (videoBackgroundActive && videoBackgroundTexture != 0) return;

    glm::vec4 sepia(0.44f, 0.26f, 0.08f, 0.15f);
    UIDrawHelper::drawColoredQuad(
        0.0f, 0.0f,
        static_cast<float>(screenWidth), static_cast<float>(screenHeight),
        sepia, screenWidth, screenHeight);
}

void TitleScreen::renderVignette() {
    // load_in_game_default.png is an opaque loading-screen artwork, not a vignette mask:
    // stretching it over the screen washed the video out and darkened the middle of it.
    if (videoBackgroundActive && videoBackgroundTexture != 0) return;

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
        // Reference lockup box: 0.431W x 0.161H with its top edge at 0.275H.
        float logoW = static_cast<float>(screenWidth) * LOGO_WIDTH_RATIO;
        float logoH = static_cast<float>(screenHeight) * LOGO_HEIGHT_RATIO;
        float logoY = static_cast<float>(screenHeight) * LOGO_TOP_RATIO;
        if (!large) {
            // Intro overlay: same lockup, a little smaller and higher up.
            logoW *= 0.8f;
            logoH *= 0.8f;
            logoY *= 0.75f;
        }
        float logoX = cx - logoW * 0.5f;
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

    // Hang the prompt below the logo box. It used to be measured down from the lockup's own
    // "Special Edition" line; that line is gone, so the height it left for the prompt is kept
    // as a fixed ratio and the lockup itself does not move.
    float textY = static_cast<float>(screenHeight) * PRESS_KEY_TOP_RATIO;

    glm::vec3 hintColor(0.165f, 0.118f, 0.086f);
    textRenderer->renderText(hint, textX, textY,
                             glm::vec4(hintColor.x, hintColor.y, hintColor.z, alpha), fontScale);
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
        buttonSlideOffsets[i] = 60.0f;
    }

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
        // The row rises into place from below, one item after another.
        buttonSlideOffsets[i] = 60.0f * (1.0f - easeOutQuad(t));
    }

    // Logo glow animation
    logoGlowIntensity = 0.5f + 0.5f * sin(glowPhase * LOGO_GLOW_SPEED);
}

void TitleScreen::handleMenuSelection() {
    if (selectedIndex >= static_cast<int>(menuItems.size())) return;

    const std::string& selected = menuItems[selectedIndex];
    playUISelectSound();

    if (selected == "menu_continue") {
        // Continue resumes the most recent save, which the load screen opens by default.
        loadGameRequested = true;
        LOGI("Menu selection: Continue");
    } else if (selected == "menu_new") {
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
    } else if (selected == "menu_exit") {
        // The sixth item's label key is menu_exit; the renderer turns this flag into
        // "return to launcher", which re-initializes the launcher intro animation.
        quitRequested = true;
        LOGI("Menu selection: Exit");
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
