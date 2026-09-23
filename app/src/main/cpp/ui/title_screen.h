#pragma once

#include <vector>
#include <string>
#include <memory>
#include <array>
#include <android/log.h>
#include "../localization/localization_manager.h"
#include "../video/bink_video_player.h"
#include "settings_ui.h"
#include "ui_panel.h"
#include "ui_button.h"

class AudioManager;

#define LOG_TAG "TitleScreen"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

class TextRenderer;

enum class TitleScreenState {
    INTRO_MOVIE,    // Oblivion logo fade-in
    LOGO_DISPLAY,   // "Press any key to continue" wait
    MENU,           // Main menu
    OPTIONS,        // Options sub-menu (reserved)
    CREDITS,        // Credits display
    TRANSITIONING   // Game start fade-out
};

struct TitleParticle {
    float x, y;
    float size;
    float alpha;
    float driftX;
    float driftY;
    float phase;
};

struct TouchRipple {
    float x, y;
    float radius;
    float maxRadius;
    float alpha;
    float speed;
    bool active;
};

class TitleScreen {
private:
    TitleScreenState state;
    float displayTimer;
    float bgAnimTime = 0.0f;
    std::vector<std::string> menuItems;
    int selectedIndex;
    bool gameStarted;
    bool settingsRequested;
    bool loadGameRequested;
    bool creditsRequested;
    bool quitRequested = false;
    LocalizationManager* localizationManager;
    std::unique_ptr<SettingsUI> settingsUI;
    TextRenderer* textRenderer = nullptr;

    std::shared_ptr<UIPanel> menuPanel;
    std::vector<std::shared_ptr<UIButton>> menuButtons;
    int screenWidth = 1920;
    int screenHeight = 1080;

    GLuint bgTexture = 0;
    GLuint logoTexture = 0;
    GLuint vignetteTexture = 0;
    GLuint selectBarTexture = 0;
    GLuint selectCutTexture = 0;
    bool texturesLoaded = false;

    std::vector<GLuint> movieFrames;
    int currentMovieFrame = 0;
    float movieFrameTime = 0.0f;
    static constexpr float MOVIE_FPS = 30.0f;

    static constexpr int MAX_PARTICLES = 48;
    std::array<TitleParticle, MAX_PARTICLES> particles{};
    bool particlesInitialized = false;

    float glowPhase = 0.0f;
    float logoFadeAlpha = 0.0f;
    float introLogoAlpha = 0.0f;
    float lastTouchX = 0.0f;
    float lastTouchY = 0.0f;

    // Menu animation
    float menuFadeAlpha = 0.0f;
    float menuSlideOffset = 0.0f;
    static constexpr float MENU_FADE_DURATION = 0.8f;
    static constexpr float MENU_SLIDE_DURATION = 0.6f;
    float menuAnimTimer = 0.0f;

    // Per-button staggered animation
    static constexpr int MAX_MENU_BUTTONS = 6;
    float buttonAnimTimers[MAX_MENU_BUTTONS] = {0.0f};
    float buttonAlphas[MAX_MENU_BUTTONS] = {0.0f};
    float buttonSlideOffsets[MAX_MENU_BUTTONS] = {0.0f};
    static constexpr float BUTTON_STAGGER_DELAY = 0.12f;
    static constexpr float BUTTON_ANIM_DURATION = 0.5f;

    // Selection indicator (the current item carries the original's highlight bar texture,
    // loaded as selectBarTexture / selectCutTexture)

    // Logo glow effect
    float logoGlowIntensity = 0.0f;
    static constexpr float LOGO_GLOW_SPEED = 1.5f;

    // Transition fade
    float transitionAlpha = 0.0f;
    static constexpr float TRANSITION_FADE_DURATION = 1.5f;

    // Touch ripple effect
    static constexpr int MAX_RIPPLES = 4;
    std::array<TouchRipple, MAX_RIPPLES> ripples{};
    int nextRippleIndex = 0;

    // Credits
    float creditsScrollY = 0.0f;
    float creditsAlpha = 0.0f;
    static constexpr float CREDITS_FADE_DURATION = 0.5f;
    float creditsFadeTimer = 0.0f;
    bool creditsActive = false;

    // Sound
    AudioManager* audioManager = nullptr;

    // Bink video integration
    oblivion::video::BinkVideoPlayer* videoPlayer = nullptr;
    bool videoPlaybackActive = false;
    bool videoCompleted = false;
    bool videoInitAttempted = false;
    oblivion::video::VideoCallbacks videoCallbacks;

    // Title screen video background (OES external texture from MediaPlayer)
    GLuint videoBackgroundTexture = 0;
    bool videoBackgroundActive = false;
    bool videoReadyOverride = false;

    static constexpr float INTRO_DURATION = 4.0f;
    static constexpr float LOGO_FADE_DURATION = 2.0f;

    static constexpr int MENU_CONTINUE = 0;
    static constexpr int MENU_NEW      = 1;
    static constexpr int MENU_LOAD     = 2;
    static constexpr int MENU_OPTIONS  = 3;
    static constexpr int MENU_CREDITS  = 4;
    static constexpr int MENU_EXIT     = 5;

    // Menu row geometry. The original main menu is a single horizontal row of items
    // sitting near the bottom of the screen, aligned on a shared baseline.
    struct MenuRowLayout {
        float startX = 0.0f;
        float baselineY = 0.0f;
        float scale = 1.0f;
        float gap = 0.0f;
        float capHeight = 0.0f;
        float width[MAX_MENU_BUTTONS] = {0.0f};
    };
    MenuRowLayout menuLayout;

    // The title screen draws dark text over bright parchment, so it runs the text renderer
    // in a style of its own: a gentler coverage window, a plain size multiplier, and an
    // outline stamped in the ink colour instead of the renderer's black. Those settings are
    // global, so they are applied once and handed back when the screen is dismissed instead
    // of being flipped every frame.
    bool fontStyleApplied = false;
    bool fontStyleReleased = false;
    bool savedFontAlphaCurve = false;
    float savedFontSizeMultiplier = 1.0f;
    float savedFontAlphaCurveLo = 0.0f;
    float savedFontAlphaCurveHi = 1.0f;
    bool savedFontOutline = false;
    float savedFontOutlineWidth = 1.0f;
    glm::vec4 savedFontOutlineColor = glm::vec4(0.0f, 0.0f, 0.0f, 0.9f);
    // Debug aid: when set, the plain style is not forced, so the renderer's global
    // settings (outline / contrast curve / size) apply to the title screen verbatim.
    bool plainStyleDisabled = false;

    // Reference geometry, as fractions of the screen, measured from the original
    // 16:9 main menu art (640x360, measured pixel by pixel): one item row centred on
    // 78.9% of the frame height and spanning 54.5% of the frame width, capitals 9 px
    // tall (2.50% of the frame) and warm brown ink RGB(117,59,33), the colour the
    // original menu declares in menus/prefabs/button_floating.xml. The art carries no
    // black outline: not one near-black pixel exists anywhere in its item row.
    //
    // The art's letter forms are spread about 1.14x wider per unit of capital height
    // than this face's own metrics ("Continue" is 52 px at 9 px capitals in the art,
    // 6.6 px of width per px of capitals against the art's 5.8), so one layout serves
    // both axes: MENU_ROW_SPAN pins the row and the capitals land near the art's size
    // at the same time. Measured on device, MENU_ROW_SPAN alone yields 24.0 px capitals
    // = 2.36% of the frame, which is what the art shows (2.2-2.5%).
    //
    // Both axes cannot be exact at once: this frame is 1.888:1 against the art's 1.778:1,
    // and the letters of this face run about 10% wider per unit of capital height than the
    // art's, so the capitals asked for here decide what is left for the gaps.
    //
    // MENU_CAP_HEIGHT_RATIO is the size the row is drawn at, and the *gap* is what pays for
    // it: the layout squeezes the item gap from the art's 4.22% down just far enough to keep
    // the row on MENU_ROW_SPAN, so the row keeps the art's width and centre at any size.
    // 0.025f is the art's own 2.50% capitals, which lands the row on the art's width, centre
    // and size at once (device measured: 25.4 px capitals, 73.7 px gaps against the art's
    // 81 px) and leaves the art's gap unused. Asking for more grows the capitals and pays for
    // them with the gaps: 0.030f gave 30.5 px capitals (3.00% of the frame) with 46.6 px gaps,
    // and 0.037 put the row at 73.4% of the frame. Readability at the art's own size is bought
    // with stroke weight (MENU_OUTLINE_*), not with size.
    static constexpr float MENU_ROW_SPAN      = 0.545f;   // reference width of the item row
    static constexpr float MENU_ROW_GAP       = 0.0422f;  // art's gap between items
    static constexpr float MENU_ROW_CENTER_Y  = 0.786f;   // text centre
    static constexpr float MENU_CAP_HEIGHT_RATIO = 0.025f;   // art's own capitals (2.50% of the frame)
    static constexpr float MENU_ROW_MAX_SPAN     = 0.88f;    // widest the grown row may get
    static constexpr float MENU_GAP_MIN_RATIO    = 0.010f;   // tightest gap between items

    // Legibility aids for the item row. The frame under it is bright and moving, and this
    // face's strokes are thin, so every label gets a soft warm-black drop shadow stamped
    // behind it, offset by a fraction of the capitals so it scales with the row.
    static constexpr float MENU_SHADOW_OFFSET_RATIO = 0.075f;   // of the capital height
    static constexpr float MENU_SHADOW_ALPHA         = 0.40f;   // of the label's own alpha

    // Coverage-curve window used for the title screen; the renderer's UI default is the
    // harder 0.30/0.72. The window below is tighter than the 0.22/0.78 the row was first
    // tuned with: it drops the faintest anti-aliased fringe instead of keeping it as a pale
    // halo and reaches full opacity sooner. It cannot move the row: the curve only remaps
    // coverage, never glyph geometry. Replaying the curve over the atlas puts the change from
    // 0.22/0.78 at about +1% of ink mass and +2% of fully covered core pixels, so this window
    // is a finish, not the cure for the row's weight; MENU_OUTLINE_* is what carries that.
    static constexpr float MENU_INK_CURVE_LO = 0.26f;
    static constexpr float MENU_INK_CURVE_HI = 0.70f;

    // Stroke weight for the item row. Replaying the renderer's own nine-stamp outline over
    // the atlas shows why the row reads thin: at the art's capitals only about 34% of the
    // glyph cores reach full coverage against the parchment, and an 0.8 px ring lifts that to
    // about 65% while widening every stroke by twice the ring. The ring is stamped in the ink
    // colour rather than the renderer's black default, so the extra weight cannot leave the
    // near-black rim the art does not have. The ring is a fraction of the face's own scale, so
    // this is a fixed proportion of the capitals at any size.
    //
    // The width is a step down from the 0.6 the row was first tuned with, at the user's request
    // for thinner strokes. The renderer's old 0.6 px floor is what made anything below that a
    // no-op, so the floor itself was lowered to 0.25 px (text_renderer.cpp) and the ring now
    // measures 0.92 * 0.45 = 0.41 px instead of 0.60 px: every stroke is widened by 0.83 px
    // rather than 1.20 px. The alpha is left alone so the row keeps the contrast it gained.
    static constexpr float MENU_OUTLINE_WIDTH = 0.45f;   // atlas px; ring = width * fontScale
    static constexpr float MENU_OUTLINE_ALPHA = 0.85f;   // of the label's own alpha

    // Selection marker, taken from the original's own menu prefab. The art does not change
    // the label's colour; menus/prefabs/button_floating.xml stretches dialog_selection_full
    // behind the hovered button (x=-10, y=3, w=buttonW-10, h=64) and overlays
    // dialog_selection_cut at its right end (w=104, h=64), where buttonW = textWidth+40.
    // The XML figures are expressed in the units of the capital height, so the bar keeps the
    // art's proportions at any resolution.
    static constexpr float MENU_SELECT_UNIT        = 28.5f;   // capitals the XML figures assume
    static constexpr float MENU_SELECT_BAR_HEIGHT  = 64.0f;   // XML bar height
    static constexpr float MENU_SELECT_LEFT_PAD    = 10.0f;   // XML bar x = -10
    static constexpr float MENU_SELECT_RIGHT_PAD   = 30.0f;   // XML bar right edge = textW+30
    static constexpr float MENU_SELECT_CAP_WIDTH   = 104.0f;  // XML cut width
    static constexpr float MENU_SELECT_CAP_RIGHT   = 48.0f;   // XML cut right edge = textW+48
    // The XML hangs the 64-unit bar at y=3 of the button, i.e. its centre sits 3 units below
    // the centre of the label it marks, so the bar is not centred on the ink but dropped.
    static constexpr float MENU_SELECT_BAR_DROP    = 3.0f;    // XML bar y = 3
    // dialog_selection_full.png is 2048 px wide but only its left 1053 px (51.42%) carry the
    // bar; the transparent tail is the part the cut piece covers. The quad is therefore
    // stretched by the inverse so the visible bar lands at the width the XML asks for.
    static constexpr float MENU_SELECT_BAR_OPAQUE  = 0.5142f;

    static constexpr float LOGO_WIDTH_RATIO   = 0.431f;
    static constexpr float LOGO_HEIGHT_RATIO  = 0.161f;
    static constexpr float LOGO_TOP_RATIO     = 0.275f;

    // The lockup's "Special Edition" line was removed at the user's request, so the logo box
    // (which ends at LOGO_TOP_RATIO + LOGO_HEIGHT_RATIO = 0.436 of the frame) runs straight into
    // the item row. The prompt hangs from this ratio instead of being measured down from the
    // removed line's baseline; 0.513 is the measured height it had while that line was drawn
    // (521.7 px on the 1017 px content frame), so the prompt does not move either.
    static constexpr float PRESS_KEY_TOP_RATIO = 0.513f;

    const glm::vec3 COLOR_PARCHMENT = glm::vec3(0.72f, 0.64f, 0.49f);
    const glm::vec3 COLOR_GOLD      = glm::vec3(0.85f, 0.72f, 0.35f);
    const glm::vec3 COLOR_WHITE     = glm::vec3(1.0f, 1.0f, 1.0f);
    // Original Oblivion main menu uses warm brown text on the parchment background and
    // declares it as RGB(117,59,33) in menus/prefabs/button_floating.xml. The reference
    // art agrees: the darkest ink in its item row is RGB(84,46,13) and the glyph cores
    // average RGB(100,60,30), never the near-black a black outline would leave behind
    // (the art's row holds zero near-black pixels). The previous (0.07,0.04,0.015) =
    // RGB(18,10,4) rendered roughly four times darker than the art, which is why the row
    // read as black.
    //
    // Measured on device (1920x1080): the current item used to be drawn *brighter* than
    // the parchment beneath it - RGB(187,127,68) over a background of L 184-211 is about
    // 1.5:1, so the label the player is looking at was the faintest thing on the row,
    // while the other items sat at 2.8:1 for the glyph cores (4.5:1 averaged, because the
    // soft atlas spreads coverage over the stroke edges). Selection is now marked the way
    // the original marks it, with the highlight bar texture behind the label (see
    // MENU_SELECT_* below); against that bar's cream RGB(246,237,220) the declared
    // RGB(117,59,33) ink measures 7.62:1, so no item needs to leave the declared colour.
    // COLOR_MENU_TEXT_SELECTED is the fallback for the case where the bar's texture is
    // missing: then selection falls back to a heavier ink plus the underline.
    const glm::vec3 COLOR_MENU_TEXT = glm::vec3(0.459f, 0.231f, 0.129f);  // RGB(117,59,33)
    // Deep ember: clearly the heaviest ink on the row, still in the menu's warm brown family.
    const glm::vec3 COLOR_MENU_TEXT_SELECTED = glm::vec3(0.278f, 0.121f, 0.047f);  // RGB(71,31,12)
    // Shadow under the row: a warm near-black rather than pure black, so it separates the
    // ink from the parchment without the hard ring the old outline left behind.
    const glm::vec3 COLOR_MENU_SHADOW = glm::vec3(0.075f, 0.042f, 0.016f);  // RGB(19,11,4)

public:
    TitleScreen();
    ~TitleScreen();

    void initialize(LocalizationManager* lm, TextRenderer* tr);
    void update(float deltaTime);
    void render();
    void onTouchEvent(float x, float y, int action);
    void debugStartNewGame();
    void onKeyPress(int key);
    void setAudioManager(AudioManager* am) { audioManager = am; }

    // Video background from MediaPlayer (OES texture)
    void setVideoBackgroundTexture(GLuint tex) {
        videoBackgroundTexture = tex;
        videoBackgroundActive = (tex != 0);
        LOGI("Video background texture set: %u, active: %d", tex, videoBackgroundActive);
    }
    void updateVideoBackground() { /* Frame update handled by SurfaceTexture */ }

    bool isGameStarted() const { return gameStarted; }
    bool isSettingsRequested() const { return settingsRequested; }
    void resetSettingsRequest() { settingsRequested = false; }
    bool isLoadGameRequested() const { return loadGameRequested; }
    void resetLoadGameRequest() { loadGameRequested = false; }
    bool isCreditsRequested() const { return creditsRequested; }
    void resetCreditsRequest() { creditsRequested = false; }
    bool isQuitRequested() const { return quitRequested; }
    void resetQuitRequest() { quitRequested = false; }
    bool isVideoReady() const { return videoBackgroundActive || videoReadyOverride; }
    void setVideoReadyOverride() { videoReadyOverride = true; }
    TitleScreenState getState() const { return state; }

    // Debug aid for font tuning: when disabled, the title screen stops forcing its
    // plain font style and the renderer's global settings apply instead.
    void setPlainStyleDisabled(bool disabled) { plainStyleDisabled = disabled; }
    bool isPlainStyleDisabled() const { return plainStyleDisabled; }

    void setScreenSize(int w, int h);

private:
    void transitionToLogo();
    void transitionToMenu();
    void updateMenu(float deltaTime);
    void handleMenuSelection();
    void buildGraphicalMenu();
    void rebuildMenuLayout();
    void computeMenuLayout();
    void applyMenuLayoutToButtons();
    void applyPlainFontStyle();
    void restoreSavedFontStyle();
    void releasePlainFontStyle();
    void initParticles();
    void setupIntroVideo();

    void renderIntroMovie();
    void renderLogoDisplay();
    void renderMenu();
    void renderCredits();
    void renderFadeOut();
    void renderBackground(float alpha, bool menuMode);
    void renderVideoBackground(float alpha);
    void renderSepiaOverlay();
    void renderVignette();
    void renderOblivionLogo(float alpha, bool large);
    void renderPressAnyKey(float alpha);
    void renderVersionText();
    void renderParticles();
    void renderRipples();
    void spawnRipple(float x, float y);
    void playUINavigateSound();
    void playUISelectSound();

    static float easeInQuad(float t) { return t * t; }
    static float easeOutQuad(float t) { return 1.0f - (1.0f - t) * (1.0f - t); }
};
