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
    float getGlyphBearingX(char ch) const;
    float getGlyphHeight(char ch, float scale = 1.0f) const;
    float getGlyphOffsetY(char ch, float scale = 1.0f) const;

    void setScreenSize(int width, int height);
    void cleanup();
    void renderDebugQuad();

    // Font switching
    bool loadOblivionFont(FontType type);
    void setActiveFont(FontType type);
    FontType getActiveFont() const { return activeFont; }
    const char* getFontTypeName(FontType type) const;

private:
    struct Glyph {
        float x0, y0, x1, y1;  // Texture coordinates
        float advanceX;
        float bearingX, bearingY;
    };

    // Oblivion .fnt glyph record (parsed from binary)
    struct OblivionGlyph {
        float u0, v0, u1, v1;       // Primary UV rect
        float u0b, v0b, u1b, v1b;   // Shadow UV rect
        float width, height;         // Pixel dimensions
        float bearing_x, bearing_y;  // Bearing offsets
        float advance;               // Horizontal advance
    };

    // Oblivion font atlas (loaded from .fnt + .png)
    struct OblivionFontAtlas {
        GLuint textureId;
        int texWidth, texHeight;
        float fontSize;
        std::map<unsigned int, OblivionGlyph> glyphs;
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
    GLuint fontTexture;  // Font texture

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

    void compileShaders();
    Glyph getGlyph(unsigned int codepoint);
    bool loadFontFromAssets(const std::string& filename);
    bool createFontTextureAtlas();
    bool loadOblivionFnt(const char* fntPath, const char* pngPath, FontType type);
    void renderTextOblivion(const std::string& text, float x, float y,
                            const glm::vec4& color, float scale);
};
