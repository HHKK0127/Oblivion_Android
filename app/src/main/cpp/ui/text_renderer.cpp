#include "text_renderer.h"
#include <android/log.h>
#include <android/asset_manager.h>
#include <cstring>
#include <glm/gtc/matrix_transform.hpp>

#define STB_TRUETYPE_IMPLEMENTATION
#include "../include/stb_truetype.h"

#undef LOG_TAG
#define LOG_TAG "TextRenderer"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

// Vertex shader: for 2D text
const char* textVertexShader = R"(#version 300 es
precision highp float;

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texCoord;

uniform mat4 projection;

out vec2 fragTexCoord;

void main() {
    gl_Position = projection * vec4(position, 0.0, 1.0);
    fragTexCoord = texCoord;
}
)";

// Fragment shader: texture-based text display
// useAlphaChannel: 0 = sample .r (GL_R8/Roboto), 1 = sample .a (RGBA8/Oblivion fonts)
const char* textFragmentShader = R"(#version 300 es
precision mediump float;

in vec2 fragTexCoord;

uniform vec4 textColor;
uniform sampler2D fontTexture;
uniform int useAlphaChannel;

out vec4 FragColor;

void main() {
    vec4 texColor = texture(fontTexture, fragTexCoord);
    float alpha = useAlphaChannel == 1 ? texColor.a : texColor.r;
    FragColor = vec4(textColor.rgb, textColor.a * alpha);
}
)";

TextRenderer::TextRenderer()
    : vao(0), vbo(0), shaderProgram(0), projectionLoc(-1), colorLoc(-1),
      fontTexture(0), screenWidth(1080), screenHeight(1920), fontData(nullptr),
      assetManager(nullptr), activeFont(FontType::Roboto) {
    memset(oblivionFonts, 0, sizeof(oblivionFonts));
    LOGD("TextRenderer created");
}

TextRenderer::~TextRenderer() {
    cleanup();
}

bool TextRenderer::initialize(AAssetManager* assetMgr) {
    LOGI("===== TextRenderer::initialize() START =====");

    if (!assetMgr) {
        LOGE("AssetManager is null");
        return false;
    }
    assetManager = assetMgr;
    LOGI("AssetManager set: %p", assetManager);

    // Shader compilation
    compileShaders();

    if (shaderProgram == 0) {
        LOGE("Failed to compile text shaders");
        return false;
    }

    // Get uniform locations
    projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    colorLoc = glGetUniformLocation(shaderProgram, "textColor");
    alphaChannelLoc = glGetUniformLocation(shaderProgram, "useAlphaChannel");

    // Generate VAO/VBO
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // Vertex attributes: position (2D) and texture coordinates
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                         (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Initialize font data
    fontData = new FontData();
    fontData->fontData = nullptr;
    fontData->fontScale = 0.0f;
    fontData->fontHeight = FONT_SIZE;

    // Load font and create texture atlas
    // On failure, create texture atlas to avoid crash
    // Keep the font in app assets so text is available on clean installs.
    if (!loadFontFromAssets("Roboto-Regular.ttf")) {
        LOGW("Font loading failed, using fallback");
    }

    if (!createFontTextureAtlas()) {
        LOGE("Failed to create font texture atlas");
        return false;
    }

    LOGI("TextRenderer initialized successfully");
    return true;
}

bool TextRenderer::loadFontFromAssets(const std::string& filename) {
    LOGI("Loading font: %s via AAssetManager", filename.c_str());

    if (!assetManager) {
        LOGE("AssetManager is null, cannot load font");
        return false;
    }

    // Open font file from assets using AAssetManager
    AAsset* asset = AAssetManager_open(assetManager, filename.c_str(), AASSET_MODE_STREAMING);
    if (!asset) {
        LOGE("Could not open font asset: %s", filename.c_str());
        return false;
    }

    LOGI("Font asset opened successfully");

    // Get file size
    off_t fontDataSize = AAsset_getLength(asset);
    LOGI("Font data size: %ld bytes", fontDataSize);

    if (fontDataSize <= 0) {
        LOGE("Invalid font file size: %ld", fontDataSize);
        AAsset_close(asset);
        return false;
    }

    // Load font data into memory
    fontData->fontData = new unsigned char[fontDataSize];
    int bytesRead = AAsset_read(asset, fontData->fontData, fontDataSize);

    if (bytesRead != fontDataSize) {
        LOGE("Failed to read font file: read %d bytes, expected %ld", bytesRead, fontDataSize);
        AAsset_close(asset);
        delete[] fontData->fontData;
        fontData->fontData = nullptr;
        return false;
    }

    AAsset_close(asset);

    LOGI("Font loaded successfully: %ld bytes", fontDataSize);
    return true;
}

bool TextRenderer::createFontTextureAtlas() {
    LOGI("Creating font texture atlas (%d x %d)", ATLAS_WIDTH, ATLAS_HEIGHT);

    // Create texture atlas bitmap
    unsigned char* atlasBuffer = new unsigned char[ATLAS_WIDTH * ATLAS_HEIGHT];
    memset(atlasBuffer, 0, ATLAS_WIDTH * ATLAS_HEIGHT);

    // Initialize stb_truetype font
    stbtt_fontinfo fontInfo;
    if (!fontData->fontData) {
        LOGW("Font data not loaded, using placeholder texture");
        // Create a simple 1x1 white texture as placeholder
        memset(atlasBuffer, 255, ATLAS_WIDTH * ATLAS_HEIGHT);
        glGenTextures(1, &fontTexture);
        glBindTexture(GL_TEXTURE_2D, fontTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, ATLAS_WIDTH, ATLAS_HEIGHT, 0, GL_RED, GL_UNSIGNED_BYTE, atlasBuffer);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        delete[] atlasBuffer;
        LOGI("Placeholder font texture created");
        return true;
    }

    if (!stbtt_InitFont(&fontInfo, fontData->fontData, 0)) {
        LOGE("Failed to initialize stb_truetype font");
        delete[] atlasBuffer;
        return false;
    }

    LOGI("stb_truetype font initialized successfully");

    // Calculate font scale
    float scale = stbtt_ScaleForPixelHeight(&fontInfo, FONT_SIZE);
    fontData->fontScale = scale;

    // Place glyphs
    int currentAtlasX = 0;
    int currentAtlasY = 0;
    int rowHeight = 0;

    // Render ASCII characters (32-126)
    for (int codepoint = 32; codepoint < 127; codepoint++) {
        int glyph_index = stbtt_FindGlyphIndex(&fontInfo, codepoint);

        // Get glyph bitmap
        int width, height, xoff, yoff;
        unsigned char* bitmap = stbtt_GetGlyphBitmap(&fontInfo, scale, scale,
                                                      glyph_index, &width, &height,
                                                      &xoff, &yoff);

        if (!bitmap) {
            // Use space if glyph not found
            if (codepoint == 32) {
                width = 8;
                height = FONT_SIZE;
            } else {
                continue;
            }
        }

        // Check if it fits in atlas
        if (currentAtlasX + width > ATLAS_WIDTH) {
            currentAtlasX = 0;
            currentAtlasY += rowHeight + 2;  // 2 pixel padding
            rowHeight = 0;
        }

        if (currentAtlasY + height > ATLAS_HEIGHT) {
            LOGW("Font atlas is full, cannot fit all glyphs");
            if (bitmap) {
                free(bitmap);
            }
            break;
        }

        // Copy bitmap to atlas
        if (bitmap) {
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    int atlasIdx = (currentAtlasY + y) * ATLAS_WIDTH + (currentAtlasX + x);
                    int bitmapIdx = y * width + x;
                    atlasBuffer[atlasIdx] = bitmap[bitmapIdx];
                }
            }
            free(bitmap);
        }

        // Register glyph info to cache
        Glyph g;
        g.x0 = (float)currentAtlasX / ATLAS_WIDTH;
        g.y0 = (float)currentAtlasY / ATLAS_HEIGHT;
        g.x1 = (float)(currentAtlasX + width) / ATLAS_WIDTH;
        g.y1 = (float)(currentAtlasY + height) / ATLAS_HEIGHT;

        // Get advance width
        int advance_width;
        stbtt_GetCodepointHMetrics(&fontInfo, codepoint, &advance_width, nullptr);
        g.advanceX = (float)advance_width * scale;

        g.bearingX = (float)xoff;
        g.bearingY = (float)yoff;

        fontData->glyphCache[codepoint] = g;

        LOGD("Glyph %c (%d): atlas pos=(%d,%d) size=(%d,%d) advance=%f",
             (char)codepoint, codepoint, currentAtlasX, currentAtlasY, width, height, g.advanceX);

        currentAtlasX += width + 1;  // 1 pixel padding
        rowHeight = (height > rowHeight) ? height : rowHeight;
    }

    // Create as OpenGL texture
    glGenTextures(1, &fontTexture);
    glBindTexture(GL_TEXTURE_2D, fontTexture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, ATLAS_WIDTH, ATLAS_HEIGHT, 0,
                 GL_RED, GL_UNSIGNED_BYTE, atlasBuffer);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);

    delete[] atlasBuffer;

    LOGI("Font texture atlas created successfully with %zu glyphs cached",
         fontData->glyphCache.size());
    return true;
}

void TextRenderer::renderText(const std::string& text, float x, float y,
                              const glm::vec3& color, float scale) {
    // Route to Oblivion font renderer if active font is not Roboto
    if (activeFont != FontType::Roboto) {
        renderTextOblivion(text, x, y, glm::vec4(color.x, color.y, color.z, 1.0f), scale);
        return;
    }

    if (text.empty() || shaderProgram == 0 || fontTexture == 0) {
        return;
    }

    glUseProgram(shaderProgram);

    // Set projection matrix
    glm::mat4 projection = glm::ortho(0.0f, (float)screenWidth, (float)screenHeight, 0.0f, -1.0f, 1.0f);
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, &projection[0][0]);

    // Set text color (vec4: alpha=1.0 for opaque)
    glUniform4f(colorLoc, color.x, color.y, color.z, 1.0f);
    glUniform1i(alphaChannelLoc, 0);  // Use red channel for Roboto (GL_R8)

    // Bind font texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fontTexture);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    float currentX = x;
    float currentY = y;

    for (char ch : text) {
        unsigned int codepoint = (unsigned char)ch;

        // Get glyph info
        Glyph glyph = getGlyph(codepoint);

        // Actual character width and height (restored from texture coordinates in atlas)
        float charWidth = (glyph.x1 - glyph.x0) * ATLAS_WIDTH * scale;
        float charHeight = (glyph.y1 - glyph.y0) * ATLAS_HEIGHT * scale;
        
        // Apply bearing (position adjustment offset)
        float posX = currentX + glyph.bearingX * scale;
        // stb_truetype bearingY is offset from baseline (usually negative)
        // Offset by FONT_SIZE here to align top-based drawing with baseline
        float posY = currentY + (FONT_SIZE + glyph.bearingY) * scale;

        // Generate vertex data (quad: 2 triangles)
        float vertices[] = {
            // Position coords        Texture coords
            posX,             posY,              glyph.x0, glyph.y0,  // Top-left
            posX + charWidth, posY,              glyph.x1, glyph.y0,  // Top-right
            posX,             posY + charHeight, glyph.x0, glyph.y1,  // Bottom-left

            posX + charWidth, posY,              glyph.x1, glyph.y0,  // Top-right
            posX + charWidth, posY + charHeight, glyph.x1, glyph.y1,  // Bottom-right
            posX,             posY + charHeight, glyph.x0, glyph.y1,  // Bottom-left
        };

        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        currentX += glyph.advanceX * scale;
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}

void TextRenderer::renderText(const std::string& text, float x, float y,
                              const glm::vec4& color, float scale) {
    // Route to Oblivion font renderer if active font is not Roboto
    if (activeFont != FontType::Roboto) {
        renderTextOblivion(text, x, y, color, scale);
        return;
    }

    if (text.empty() || shaderProgram == 0 || fontTexture == 0) {
        return;
    }

    glUseProgram(shaderProgram);

    glm::mat4 projection = glm::ortho(0.0f, (float)screenWidth, (float)screenHeight, 0.0f, -1.0f, 1.0f);
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, &projection[0][0]);

    // Set text color (vec4: with alpha channel)
    glUniform4f(colorLoc, color.x, color.y, color.z, color.w);
    glUniform1i(alphaChannelLoc, 0);  // Use red channel for Roboto (GL_R8)

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fontTexture);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    float currentX = x;
    float currentY = y;

    for (char ch : text) {
        unsigned int codepoint = (unsigned char)ch;
        Glyph glyph = getGlyph(codepoint);

        float charWidth = (glyph.x1 - glyph.x0) * ATLAS_WIDTH * scale;
        float charHeight = (glyph.y1 - glyph.y0) * ATLAS_HEIGHT * scale;

        float posX = currentX + glyph.bearingX * scale;
        float posY = currentY + (FONT_SIZE + glyph.bearingY) * scale;

        float vertices[] = {
            posX,             posY,              glyph.x0, glyph.y0,
            posX + charWidth, posY,              glyph.x1, glyph.y0,
            posX,             posY + charHeight, glyph.x0, glyph.y1,

            posX + charWidth, posY,              glyph.x1, glyph.y0,
            posX + charWidth, posY + charHeight, glyph.x1, glyph.y1,
            posX,             posY + charHeight, glyph.x0, glyph.y1,
        };

        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        currentX += glyph.advanceX * scale;
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}

float TextRenderer::getTextWidth(const std::string& text, float scale) {
    if (text.empty() || fontData == nullptr) {
        return 0.0f;
    }
    float width = 0.0f;
    for (char ch : text) {
        unsigned int codepoint = (unsigned char)ch;
        Glyph glyph = getGlyph(codepoint);
        width += glyph.advanceX * scale;
    }
    return width;
}

    float TextRenderer::getTextHeight(float scale) const {
        if (!fontData) return 0.0f;
        return FONT_SIZE * scale;
    }

    TextRenderer::Glyph TextRenderer::getGlyph(unsigned int codepoint) {
    // Search from cache
    auto it = fontData->glyphCache.find(codepoint);
    if (it != fontData->glyphCache.end()) {
        return it->second;
    }

    // Return space if not found
    return fontData->glyphCache[32];  // ASCII 32 = space
}

float TextRenderer::getGlyphBearingX(char ch) const {
    if (!fontData) return 0.0f;
    unsigned int codepoint = (unsigned char)ch;
    auto it = fontData->glyphCache.find(codepoint);
    if (it != fontData->glyphCache.end()) {
        return it->second.bearingX;
    }
    return 0.0f;
}

float TextRenderer::getGlyphHeight(char ch, float scale) const {
    if (!fontData) return 0.0f;
    unsigned int codepoint = (unsigned char)ch;
    auto it = fontData->glyphCache.find(codepoint);
    if (it == fontData->glyphCache.end()) return 0.0f;
    const Glyph& g = it->second;
    return (g.y1 - g.y0) * ATLAS_HEIGHT * scale;
}

float TextRenderer::getGlyphOffsetY(char ch, float scale) const {
    // The y passed to renderText is interpreted as the baseline of the text area.
    // Returns the y offset (downward) where the glyph bitmap is placed relative to
    // that baseline. For most glyphs, this is (FONT_SIZE + bearingY) * scale.
    if (!fontData) return 0.0f;
    unsigned int codepoint = (unsigned char)ch;
    auto it = fontData->glyphCache.find(codepoint);
    if (it == fontData->glyphCache.end()) return FONT_SIZE * scale;
    const Glyph& g = it->second;
    return (FONT_SIZE + g.bearingY) * scale;
}

void TextRenderer::setScreenSize(int width, int height) {
    screenWidth = width;
    screenHeight = height;
    LOGD("TextRenderer screen size set to %d x %d", screenWidth, screenHeight);
}

void TextRenderer::compileShaders() {
    LOGD("Compiling text shaders");

    // Compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &textVertexShader, nullptr);
    glCompileShader(vertexShader);

    // Check vertex shader compilation
    GLint success;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vertexShader, sizeof(infoLog), nullptr, infoLog);
        LOGE("Vertex shader compilation failed: %s", infoLog);
        glDeleteShader(vertexShader);
        return;
    }

    // Compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &textFragmentShader, nullptr);
    glCompileShader(fragmentShader);

    // Check fragment shader compilation
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fragmentShader, sizeof(infoLog), nullptr, infoLog);
        LOGE("Fragment shader compilation failed: %s", infoLog);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return;
    }

    // Link program
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Check link status
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram, sizeof(infoLog), nullptr, infoLog);
        LOGE("Shader program linking failed: %s", infoLog);
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    LOGD("Shaders compiled and linked (program=%u)", shaderProgram);
}

// --- Oblivion .fnt font loading ---

// STB Image implementation is in texture_loader.cpp - only include declarations here
#include "../include/stb_image.h"

bool TextRenderer::loadOblivionFnt(const char* fntPath, const char* pngPath, FontType type) {
    int idx = static_cast<int>(type);
    LOGI("Loading Oblivion font: fnt=%s png=%s type=%d", fntPath, pngPath, idx);

    // 1. Load .fnt binary
    AAsset* fntAsset = AAssetManager_open(assetManager, fntPath, AASSET_MODE_BUFFER);
    if (!fntAsset) {
        LOGE("Cannot open fnt: %s", fntPath);
        return false;
    }
    off_t fntSize = AAsset_getLength(fntAsset);
    const uint8_t* fntBuf = (const uint8_t*)AAsset_getBuffer(fntAsset);
    if (!fntBuf || fntSize < 76) {
        LOGE("Invalid fnt file: %s (%ld bytes)", fntPath, (long)fntSize);
        AAsset_close(fntAsset);
        return false;
    }

    // Parse header (76 bytes)
    float fontSize;
    memcpy(&fontSize, fntBuf + 0, 4);
    char texName[65] = {0};
    memcpy(texName, fntBuf + 12, 64);

    oblivionFonts[idx].fontSize = fontSize;
    LOGI("  fontSize=%.1f texName='%s'", fontSize, texName);

    // Parse glyph records (56 bytes each, starting at offset 76)
    const uint8_t* glyphData = fntBuf + 76;
    int glyphDataSize = (int)fntSize - 76;
    int glyphCount = glyphDataSize / 56;
    LOGI("  glyphDataSize=%d glyphCount=%d", glyphDataSize, glyphCount);

    oblivionFonts[idx].glyphs.clear();
    for (int i = 0; i < glyphCount && i < 256; i++) {
        const float* f = (const float*)(glyphData + i * 56);
        OblivionGlyph g;
        g.u0 = f[0]; g.v0 = f[1]; g.u1 = f[2]; g.v1 = f[3];
        g.u0b = f[4]; g.v0b = f[5]; g.u1b = f[6]; g.v1b = f[7];
        g.width = f[8]; g.height = f[9];
        g.bearing_x = f[10]; g.bearing_y = f[11];
        g.advance = f[12];
        oblivionFonts[idx].glyphs[i] = g;
    }
    AAsset_close(fntAsset);
    LOGI("  Loaded %zu glyphs", oblivionFonts[idx].glyphs.size());

    // 2. Load .png atlas
    AAsset* pngAsset = AAssetManager_open(assetManager, pngPath, AASSET_MODE_STREAMING);
    if (!pngAsset) {
        LOGE("Cannot open png: %s", pngPath);
        return false;
    }
    off_t pngSize = AAsset_getLength(pngAsset);
    uint8_t* pngBuf = new uint8_t[pngSize];
    AAsset_read(pngAsset, pngBuf, pngSize);
    AAsset_close(pngAsset);

    int w, h, ch;
    uint8_t* pixels = stbi_load_from_memory(pngBuf, (int)pngSize, &w, &h, &ch, 4);
    delete[] pngBuf;
    if (!pixels) {
        LOGE("stbi_load failed for %s: %s", pngPath, stbi_failure_reason());
        return false;
    }

    oblivionFonts[idx].texWidth = w;
    oblivionFonts[idx].texHeight = h;
    LOGI("  Atlas: %dx%d (requested RGBA8)", w, h);

    // Upload as RGBA8 texture
    glGenTextures(1, &oblivionFonts[idx].textureId);
    glBindTexture(GL_TEXTURE_2D, oblivionFonts[idx].textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(pixels);
    LOGI("Oblivion font loaded: type=%d texId=%u", idx, oblivionFonts[idx].textureId);
    return true;
}

bool TextRenderer::loadOblivionFont(FontType type) {
    // Map FontType to file paths
    struct { const char* fnt; const char* png; } fontFiles[] = {
        { "fonts/daedric_font.fnt",     "fonts/daedric_font_0_lod_a.png" },      // Roboto placeholder
        { "fonts/daedric_font.fnt",     "fonts/daedric_font_0_lod_a.png" },      // Daedric
        { "fonts/kingthings_regular.fnt", "fonts/kingthings_regular_0_lod_a.png" }, // KingthingsRegular
        { "fonts/kingthings_shadowed.fnt","fonts/kingthings_shadowed_0_lod_a.png" },// KingthingsShadowed
        { "fonts/handwritten.fnt",      "fonts/handwritten_0_lod_a.png" },       // Handwritten
        { "fonts/tahoma_bold_small.fnt","fonts/tahoma_bold_small_0_lod_a.png" }, // TahomaBoldSmall
    };
    int idx = static_cast<int>(type);
    if (idx < 1 || idx >= static_cast<int>(FontType::COUNT)) {
        LOGE("Invalid FontType %d for Oblivion font", idx);
        return false;
    }
    return loadOblivionFnt(fontFiles[idx].fnt, fontFiles[idx].png, type);
}

void TextRenderer::setActiveFont(FontType type) {
    activeFont = type;
    LOGI("Active font set to %d (%s)", (int)type, getFontTypeName(type));
}

const char* TextRenderer::getFontTypeName(FontType type) const {
    switch (type) {
        case FontType::Roboto:            return "Roboto";
        case FontType::Daedric:           return "Daedric";
        case FontType::KingthingsRegular: return "Kingthings Regular";
        case FontType::KingthingsShadowed:return "Kingthings Shadowed";
        case FontType::Handwritten:       return "Handwritten";
        case FontType::TahomaBoldSmall:   return "Tahoma Bold Small";
        default:                          return "Unknown";
    }
}

void TextRenderer::renderTextOblivion(const std::string& text, float x, float y,
                                       const glm::vec4& color, float scale) {
    int idx = static_cast<int>(activeFont);
    const OblivionFontAtlas& atlas = oblivionFonts[idx];
    if (atlas.textureId == 0) {
        LOGW("Oblivion font %d not loaded", idx);
        return;
    }

    glUseProgram(shaderProgram);

    glm::mat4 projection = glm::ortho(0.0f, (float)screenWidth, (float)screenHeight, 0.0f, -1.0f, 1.0f);
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, &projection[0][0]);
    glUniform4f(colorLoc, color.x, color.y, color.z, color.w);
    glUniform1i(alphaChannelLoc, 1);  // Use alpha channel for Oblivion fonts

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, atlas.textureId);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    float currentX = x;
    float currentY = y;
    float texW = (float)atlas.texWidth;
    float texH = (float)atlas.texHeight;
    float fontScale = scale * (atlas.fontSize / 22.0f);  // Normalize to ~22pt base

    for (char ch : text) {
        unsigned int codepoint = (unsigned char)ch;
        auto it = atlas.glyphs.find(codepoint);
        if (it == atlas.glyphs.end()) {
            // Skip missing glyphs
            currentX += atlas.fontSize * fontScale * 0.5f;
            continue;
        }
        const OblivionGlyph& g = it->second;

        // Convert UV to pixel coords
        float gw = g.width * fontScale;
        float gh = g.height * fontScale;
        float posX = currentX + g.bearing_x * fontScale;
        float posY = currentY + g.bearing_y * fontScale;

        float vertices[] = {
            posX,        posY,        g.u0, g.v0,
            posX + gw,   posY,        g.u1, g.v0,
            posX,        posY + gh,   g.u0, g.v1,
            posX + gw,   posY,        g.u1, g.v0,
            posX + gw,   posY + gh,   g.u1, g.v1,
            posX,        posY + gh,   g.u0, g.v1,
        };

        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        currentX += g.advance * fontScale;
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}

void TextRenderer::cleanup() {
    if (shaderProgram != 0) {
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
    }

    if (fontTexture != 0) {
        glDeleteTextures(1, &fontTexture);
        fontTexture = 0;
    }

    // Cleanup Oblivion font textures
    for (int i = 0; i < static_cast<int>(FontType::COUNT); i++) {
        if (oblivionFonts[i].textureId != 0) {
            glDeleteTextures(1, &oblivionFonts[i].textureId);
            oblivionFonts[i].textureId = 0;
        }
        oblivionFonts[i].glyphs.clear();
    }

    if (vao != 0) {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
    }

    if (vbo != 0) {
        glDeleteBuffers(1, &vbo);
        vbo = 0;
    }

    if (fontData != nullptr) {
        if (fontData->fontData != nullptr) {
            delete[] fontData->fontData;
        }
        delete fontData;
        fontData = nullptr;
    }

    LOGD("TextRenderer cleaned up");
}

void TextRenderer::renderDebugQuad() {
    if (shaderProgram == 0 || vao == 0) {
        return;
    }

    glUseProgram(shaderProgram);

    glm::mat4 projection = glm::ortho(0.0f, (float)screenWidth, (float)screenHeight, 0.0f, -1.0f, 1.0f);
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, &projection[0][0]);
    glUniform4f(colorLoc, 1.0f, 1.0f, 1.0f, 1.0f);

    float vertices[] = {
        50.0f, 350.0f,   0.0f, 0.0f,
        250.0f, 350.0f,  1.0f, 0.0f,
        50.0f, 450.0f,   0.0f, 1.0f,
        250.0f, 450.0f,  1.0f, 1.0f,
    };

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glUseProgram(0);
}
