#pragma once

#include <string>
#include <map>
#include <vector>
#include <glm/glm.hpp>
#include <GLES3/gl3.h>
#include <android/asset_manager.h>
// stb_truetype is used for bundled TTF faces (Kingthings Petrock Light + the
// SAIMOJI/TKaisho CJK faces). The implementation is compiled once in
// text_renderer.cpp via STB_TRUETYPE_IMPLEMENTATION; including the header here
// only exposes the fontinfo struct needed by the CJK atlas.
#include "../include/stb_truetype.h"

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

    // Selects the CJK face used for Japanese text. Off: SAIMOJI (default, all UI).
    // On: TKaisho kaisho, used while a book panel is open; codepoints TKaisho
    // lacks fall back to SAIMOJI. Safe to call from the render thread.
    void setBookStyle(bool enabled) { bookStyle_ = enabled; }
    bool isBookStyle() const { return bookStyle_; }

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

        // CJK glyph cached in a dynamic page atlas (SAIMOJI by default, TKaisho in book
        // style). The glyph is baked as an alpha coverage bitmap at FONT_SIZE; UVs
        // point into the page that holds it.
        struct CjkGlyph {
            float u0, v0, u1, v1;    // UV rectangle in the page atlas
            float advanceX;           // Horizontal advance in atlas pixels
            float bearingX, bearingY; // Bitmap offset from the pen origin (baselineY is
                                      // negative, pointing above the baseline)
            float width, height;      // Bitmap size in atlas pixels
            size_t pageIndex;         // Index into cjkPages_ that owns this glyph's texel
        };

        // One 1024x1024 GL_R8 atlas page. Glyphs are packed left-to-right in rows; a new
        // row starts below the tallest glyph in the current one.
        struct CjkAtlasPage {
            GLuint textureId = 0;
            int cursorX = 0;   // Left edge of the next free cell (current row)
            int cursorY = 0;   // Top of the current row
            int rowHeight = 0; // Height of the tallest cell in the current row
        };

        // Vertex run collected for one page texture, so a mixed Latin/JP string draws in
        // a single call per page instead of one call per glyph.
        struct CjkBatch {
            GLuint pageTextureId;
            std::vector<float> vertices;
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

        // CJK fonts (bundled SAIMOJI + TKaisho) and their dynamic page atlases.
        // Oblivion ships no CJK face at all, so any Japanese text falls back to these
        // bundled TTFs. Glyphs are rasterized lazily on first use and cached; pages
        // are created on demand. Both faces share cjkPages_ (the atlas is generic
        // coverage) but keep separate glyph caches so the two never mix UVs.
        unsigned char* cjkFontData_ = nullptr;
        stbtt_fontinfo cjkFontInfo_;
        bool cjkFontReady_ = false;
        float cjkFontScale_ = 0.0f;
        std::map<unsigned int, CjkGlyph> cjkGlyphs_;
        // TKaisho kaisho face, active only while bookStyle_ is set (book panels).
        unsigned char* cjkBookFontData_ = nullptr;
        stbtt_fontinfo cjkBookFontInfo_;
        bool cjkBookFontReady_ = false;
        float cjkBookFontScale_ = 0.0f;
        std::map<unsigned int, CjkGlyph> cjkBookGlyphs_;
        bool bookStyle_ = false;
        std::vector<CjkAtlasPage> cjkPages_;

    // Android AssetManager (for font loading)
    AAssetManager* assetManager;

    // Texture atlas size
    static constexpr int ATLAS_WIDTH = 512;
    static constexpr int ATLAS_HEIGHT = 512;
    static constexpr int FONT_SIZE = 32;
        // CJK page atlas size. Japanese strings can reach thousands of unique glyphs, so
        // pages are 1024x1024 and new ones are allocated on demand.
        static constexpr int CJK_ATLAS_SIZE = 1024;

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
        // Lays out one codepoint run (Latin + CJK) into batchScratch_ (Latin, current
        // atlas texture) and cjkBatches (JP glyphs, one per page texture). The batched
        // vertices are uploaded and drawn by uploadAndDrawScratch once per texture.
        void appendMixedRun(const std::vector<unsigned int>& codepoints, float x, float y,
                            float fontScale, const OblivionFontAtlas& atlas,
                            std::vector<CjkBatch>& cjkBatches);
        void uploadAndDrawScratch(const std::vector<float>& vertices, GLuint textureId,
                                  int alphaChannel);
        Glyph getGlyph(unsigned int codepoint);
        bool loadFontFromAssets(const std::string& filename);
        bool createFontTextureAtlas();
        bool loadOblivionFnt(const char* fntPath, const char* pngPath, FontType type);

        // Shared Roboto path used by both renderText overloads (UTF-8 → codepoint loop).
                void drawRobotoText(const std::string& text, float x, float y,
                                    const glm::vec4& color, float scale);

                // CJK (Japanese) support. Loads one bundled face into the given
                // info/scale pair; resolves a TTC collection to its first face.
                bool loadCjkFontFromAssets(const std::string& filename,
                                           stbtt_fontinfo* outInfo,
                                           unsigned char** outData, float* outScale);
        // Rasterizes the codepoint into a page atlas on first use and caches it.
        const CjkGlyph* getCjkGlyph(unsigned int codepoint);
        // Shared rasterize+pack path for one face; cache is passed in so the primary
        // and book faces keep independent glyph entries.
        const CjkGlyph* getCjkGlyphForFace(unsigned int codepoint,
                                           const stbtt_fontinfo& info, float scale,
                                           std::map<unsigned int, CjkGlyph>& cache);
        // Advance width without rasterizing; safe on the const getTextWidth path.
        float getCjkAdvance(unsigned int codepoint, float scale) const;
        GLuint createCjkAtlasTexture();
    };
