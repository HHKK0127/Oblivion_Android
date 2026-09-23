#pragma once

#include <string>
#include <map>
#include <vector>
#include <glm/glm.hpp>
#include <GLES3/gl3.h>
#include <android/asset_manager.h>

/**
 * @brief Font type selector for Oblivion original fonts
 */
enum class FontType : uint8_t {
    Roboto = 0,
    Daedric = 1,
    KingthingsRegular = 2,
    KingthingsShadowed = 3,
    Handwritten = 4,
    TahomaBoldSmall = 5,
    COUNT = 6
};

/**
 * @brief Text rendering system supporting both TTF (stb_truetype) and Oblivion .fnt fonts
 */
class TextRenderer {
public:
    TextRenderer();
    ~TextRenderer();

    bool initialize(AAssetManager* assetMgr);

    void renderText(const std::string& text, float x, float y,
                    const glm::vec3& color = glm::vec3(1.0f, 1.0f, 1.0f),
                    float scale = 1.0f);

    void renderText(const std::string& text, float x, float y,
                    const glm::vec4& color, float scale = 1.0f);

    float getTextWidth(const std::string& text, float scale = 1.0f);
    float getTextHeight(float scale = 1.0f) const;
    // Screen-space cap height (baseline to cap top) of the active Oblivion face at the
    // given scale. Lets callers place text by its baseline instead of its box top.
    float getTextCapHeight(float scale = 1.0f) const;
    float getGlyphBearingX(char ch) const;
    float getGlyphHeight(char ch, float scale = 1.0f) const;
    float getGlyphOffsetY(char ch, float scale = 1.0f) const;

    void setScreenSize(int width, int height);
    void cleanup();
    void renderDebugQuad();

    // Font switching
    bool loadOblivionFont(FontType type);
    void setActiveFont(FontType type);
    // Safe to call from any thread. The atlas is uploaded to GL later, on the render
    // thread, so nothing here may touch the GL context.
    void requestFont(FontType type);
    // Uploads whatever requestFont deferred into the texture. Render thread only.
    void processPendingFontRequest();
    FontType getActiveFont() const { return activeFont; }
    const char* getFontTypeName(FontType type) const;

    // Readability tuning for Oblivion .fnt fonts.
    // The original atlases carry soft, low-contrast coverage values; the contrast
    // curve steepens them so thin strokes stay opaque over bright terrain.
    // Applies to the Oblivion glyph coverage only (never to Roboto).
    void setFontAlphaCurve(float lo, float hi);
    void setFontAlphaCurveEnabled(bool enabled);
    bool isFontAlphaCurveEnabled() const { return fontAlphaCurveEnabled_; }
    float getFontAlphaCurveLo() const { return fontAlphaLo_; }
    float getFontAlphaCurveHi() const { return fontAlphaHi_; }

    // Uniform size multiplier for Oblivion fonts (1.0 = atlas design size).
    void setFontSizeMultiplier(float mult);
    float getFontSizeMultiplier() const { return fontSizeMultiplier_; }

    // Dark outline drawn behind Oblivion glyphs. The decorative original faces are
    // thin, soft and light, so over bright terrain they wash out; a dark ring keeps
    // the stroke separated from whatever is behind it. Oblivion glyphs only.
    void setFontOutlineEnabled(bool enabled);
    bool isFontOutlineEnabled() const { return fontOutlineEnabled_; }
    // Radius in atlas pixels (1.0 follows the glyph scale, ~1.5 screen px at UI sizes).
    void setFontOutlineWidth(float atlasPixels);
    float getFontOutlineWidth() const { return fontOutlineWidth_; }
    void setFontOutlineColor(const glm::vec4& color);
    glm::vec4 getFontOutlineColor() const { return fontOutlineColor_; }

    // Oblivion font rendering
    void renderTextOblivion(const std::string& text, float x, float y,
                            const glm::vec4& color, float scale = 1.0f);

private:
    struct Glyph {
        float x0, y0, x1, y1;  // Texture coordinates
        float advanceX;
        float bearingX, bearingY;
    };

    // Oblivion .fnt glyph record (parsed from binary at offset 344, stride 56)
    struct OblivionGlyph {
        float bearing_x;           // Left side bearing; cells are tight, so this is 0
        float advance;             // Horizontal advance = cell width (f[1] is NOT an advance)
        float u0, v0;              // UV top-left (f[3], f[4])
        float u1, v1;              // UV bottom-right (f[5], f[8])
        float width, height;       // Pixel dimensions (f[11], f[12])
        // Cell rows from the cell top down to this glyph's ink bottom, clamped to the
        // baseline. Every cell is a tight box around its own ink, so x-height letters
        // sit lower inside their cell than capitals; aligning cell tops would push
        // "o"/"n"/"s" about 8px above the capitals at UI sizes.
        float baseline_rel;
    };

    // Oblivion font atlas (loaded from .fnt + .png)
    struct OblivionFontAtlas {
        GLuint textureId;
        int texWidth, texHeight;
        float fontSize;
        std::map<unsigned int, OblivionGlyph> glyphs;
        // Cell rows from the top of a capital cell down to the baseline, measured from
        // the atlas (the .fnt header carries no vertical metrics at all).
        float capBaseline = 0.0f;
        // kingthings_shadowed stores the crisp letter in RGB and a blurred
        // shadow/glow silhouette in alpha. Sampling alpha alone renders the
        // glow only, so such atlases must read RGB instead.
        bool useRgbChannel;
    };

    // For font rendering
    struct FontData {
        unsigned char* fontData;
        float fontScale;
        int fontHeight;
        std::map<unsigned int, Glyph> glyphCache;
    };

    GLuint vao;
    GLuint vbo;
    GLuint shaderProgram;
    GLint projectionLoc;
    GLint colorLoc;
    GLint alphaChannelLoc;
    GLint alphaRangeLoc;
    GLint alphaCurveLoc;
    GLuint fontTexture;  // Font texture
    GLint fontTextureLoc;  // Cached "fontTexture" sampler uniform location

    // Scratch buffer reused across text draws so a whole string is uploaded and
    // drawn in a single GL call instead of one call per glyph
    std::vector<float> batchScratch_;

    int screenWidth;
    int screenHeight;

    // Font data
    FontData* fontData;

    // Android AssetManager (for font loading)
    AAssetManager* assetManager;

    // Texture atlas size
    static constexpr int ATLAS_WIDTH = 512;
    static constexpr int ATLAS_HEIGHT = 512;
    static constexpr int FONT_SIZE = 32;

    // Oblivion font system
    FontType activeFont;
    OblivionFontAtlas oblivionFonts[static_cast<int>(FontType::COUNT)];

    // Font switches are requested from the input thread, which owns no GL context, so
    // the upload is deferred to the render thread (see requestFont).
    bool hasPendingFontRequest_ = false;
    FontType pendingFontRequest_ = FontType::Roboto;
    // A missing atlas is reported once per font instead of on every drawn frame.
    bool fontMissingLogged_[static_cast<int>(FontType::COUNT)] = {};

    // Oblivion readability tuning (see setFontAlphaCurve / setFontSizeMultiplier).
    // The original atlases are authored at their .fnt design size (Kingthings: 28 px em) and
    // get magnified ~1.6x on screen, so strokes render thin and soft. The contrast curve
    // re-hardens that soft coverage ramp, and the size multiplier gives the decorative
    // original face enough weight to read comfortably on a phone panel.
    float fontAlphaLo_ = 0.30f;
    float fontAlphaHi_ = 0.72f;
    bool fontAlphaCurveEnabled_ = true;
    float fontSizeMultiplier_ = 1.15f;

    // Oblivion outline tuning (see setFontOutlineEnabled).
    bool fontOutlineEnabled_ = true;
    float fontOutlineWidth_ = 1.0f;
    glm::vec4 fontOutlineColor_ = glm::vec4(0.0f, 0.0f, 0.0f, 0.9f);

    void compileShaders();
    void drawOblivionText(const std::string& text, float x, float y,
                          const glm::vec4& color, float scale, int atlasIndex);
    void appendOblivionRun(const std::string& text, float x, float y, float fontScale,
                           const OblivionFontAtlas& atlas);
    void uploadAndDrawBatch();
    Glyph getGlyph(unsigned int codepoint);
    bool loadFontFromAssets(const std::string& filename);
    bool createFontTextureAtlas();
    bool loadOblivionFnt(const char* fntPath, const char* pngPath, FontType type);
};
