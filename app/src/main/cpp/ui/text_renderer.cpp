#include "text_renderer.h"
#include <android/log.h>
#include <android/asset_manager.h>
#include <algorithm>
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
// alphaRange/alphaCurveOn: contrast curve applied to Oblivion glyph coverage so the
// soft original atlases keep thin strokes readable over bright, busy backgrounds.
const char* textFragmentShader = R"(#version 300 es
precision mediump float;

in vec2 fragTexCoord;

uniform vec4 textColor;
uniform sampler2D fontTexture;
uniform int useAlphaChannel;
uniform vec2 alphaRange;
uniform int alphaCurveOn;

out vec4 FragColor;

void main() {
    vec4 texColor = texture(fontTexture, fragTexCoord);
    float a;
    if (useAlphaChannel == 0) {
        a = texColor.r;
    } else {
        a = texColor.a;
        if (alphaCurveOn == 1) {
            float span = max(1.0e-5, alphaRange.y - alphaRange.x);
            float t = clamp((a - alphaRange.x) / span, 0.0, 1.0);
            a = t * t * (3.0 - 2.0 * t);
        }
    }
    FragColor = vec4(textColor.rgb, textColor.a * a);
}
)";

TextRenderer::TextRenderer()
    : vao(0), vbo(0), shaderProgram(0), projectionLoc(-1), colorLoc(-1),
      alphaChannelLoc(-1), alphaRangeLoc(-1), alphaCurveLoc(-1),
      fontTexture(0), fontTextureLoc(-1),
      screenWidth(1080), screenHeight(1920), fontData(nullptr),
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
    alphaRangeLoc = glGetUniformLocation(shaderProgram, "alphaRange");
    alphaCurveLoc = glGetUniformLocation(shaderProgram, "alphaCurveOn");
    fontTextureLoc = glGetUniformLocation(shaderProgram, "fontTexture");

    LOGI("Shader program=%u, projectionLoc=%d, colorLoc=%d, alphaChannelLoc=%d, alphaRangeLoc=%d, alphaCurveLoc=%d",
         shaderProgram, projectionLoc, colorLoc, alphaChannelLoc, alphaRangeLoc, alphaCurveLoc);
    LOGI("Fragment shader source:\n%s", textFragmentShader);

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
        drawOblivionText(text, x, y, glm::vec4(color.x, color.y, color.z, 1.0f), scale, static_cast<int>(activeFont));
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

    batchScratch_.clear();
    batchScratch_.reserve(text.size() * 24);

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
        const float vertices[24] = {
            // Position coords        Texture coords
            posX,             posY,              glyph.x0, glyph.y0,  // Top-left
            posX + charWidth, posY,              glyph.x1, glyph.y0,  // Top-right
            posX,             posY + charHeight, glyph.x0, glyph.y1,  // Bottom-left

            posX + charWidth, posY,              glyph.x1, glyph.y0,  // Top-right
            posX + charWidth, posY + charHeight, glyph.x1, glyph.y1,  // Bottom-right
            posX,             posY + charHeight, glyph.x0, glyph.y1,  // Bottom-left
        };
        batchScratch_.insert(batchScratch_.end(), vertices, vertices + 24);

        currentX += glyph.advanceX * scale;
    }

    if (!batchScratch_.empty()) {
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(batchScratch_.size() * sizeof(float)),
                     batchScratch_.data(), GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(batchScratch_.size() / 4));
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
        drawOblivionText(text, x, y, color, scale, static_cast<int>(activeFont));
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

    batchScratch_.clear();
    batchScratch_.reserve(text.size() * 24);

    for (char ch : text) {
        unsigned int codepoint = (unsigned char)ch;
        Glyph glyph = getGlyph(codepoint);

        float charWidth = (glyph.x1 - glyph.x0) * ATLAS_WIDTH * scale;
        float charHeight = (glyph.y1 - glyph.y0) * ATLAS_HEIGHT * scale;

        float posX = currentX + glyph.bearingX * scale;
        float posY = currentY + (FONT_SIZE + glyph.bearingY) * scale;

        const float vertices[24] = {
            posX,             posY,              glyph.x0, glyph.y0,
            posX + charWidth, posY,              glyph.x1, glyph.y0,
            posX,             posY + charHeight, glyph.x0, glyph.y1,

            posX + charWidth, posY,              glyph.x1, glyph.y0,
            posX + charWidth, posY + charHeight, glyph.x1, glyph.y1,
            posX,             posY + charHeight, glyph.x0, glyph.y1,
        };
        batchScratch_.insert(batchScratch_.end(), vertices, vertices + 24);

        currentX += glyph.advanceX * scale;
    }

    if (!batchScratch_.empty()) {
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(batchScratch_.size() * sizeof(float)),
                     batchScratch_.data(), GL_DYNAMIC_DRAW);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(batchScratch_.size() / 4));
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}

void TextRenderer::appendOblivionRun(const std::string& text, float x, float y, float fontScale,
                                     const OblivionFontAtlas& atlas) {
    float currentX = x;
    float currentY = y;

    for (char ch : text) {
        unsigned int codepoint = (unsigned char)ch;
        auto it = atlas.glyphs.find(codepoint);
        if (it == atlas.glyphs.end()) {
            auto spaceIt = atlas.glyphs.find(32);
            float spaceAdvance = (spaceIt != atlas.glyphs.end()) ? spaceIt->second.advance : atlas.fontSize * 0.5f;
            currentX += spaceAdvance * fontScale;
            continue;
        }
        const OblivionGlyph& g = it->second;

        float gw = g.width * fontScale;
        float gh = g.height * fontScale;
        float posX = currentX + g.bearing_x * fontScale;
        // y is the top of a capital cell. Cells are tight boxes around their own ink,
        // so shift each glyph down until its ink bottom reaches the shared baseline;
        // otherwise x-height letters float ~8px above the capitals at UI sizes.
        float posY = currentY + (atlas.capBaseline - g.baseline_rel) * fontScale;

        const float vertices[24] = {
            posX,        posY,        g.u0, g.v0,
            posX + gw,   posY,        g.u1, g.v0,
            posX,        posY + gh,   g.u0, g.v1,
            posX + gw,   posY,        g.u1, g.v0,
            posX + gw,   posY + gh,   g.u1, g.v1,
            posX,        posY + gh,   g.u0, g.v1,
        };
        batchScratch_.insert(batchScratch_.end(), vertices, vertices + 24);

        currentX += g.advance * fontScale;
    }
}

void TextRenderer::uploadAndDrawBatch() {
    if (batchScratch_.empty()) {
        return;
    }
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(batchScratch_.size() * sizeof(float)),
                 batchScratch_.data(), GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(batchScratch_.size() / 4));
}

void TextRenderer::drawOblivionText(const std::string& text, float x, float y,
                                    const glm::vec4& color, float scale, int atlasIndex) {
    if (atlasIndex < 0 || atlasIndex >= static_cast<int>(FontType::COUNT)) {
        return;
    }
    const OblivionFontAtlas& atlas = oblivionFonts[atlasIndex];
    if (atlas.textureId == 0) {
        if (!fontMissingLogged_[atlasIndex]) {
            fontMissingLogged_[atlasIndex] = true;
            LOGW("Oblivion font %d not loaded", atlasIndex);
        }
        return;
    }
    if (text.empty() || shaderProgram == 0) {
        return;
    }

    glUseProgram(shaderProgram);

    glm::mat4 projection = glm::ortho(0.0f, (float)screenWidth, (float)screenHeight, 0.0f, -1.0f, 1.0f);
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, &projection[0][0]);
    glUniform1i(alphaChannelLoc, atlas.useRgbChannel ? 0 : 1);  // shadowed atlases keep the letter in RGB
    glUniform1i(fontTextureLoc, 0);
    glUniform2f(alphaRangeLoc, fontAlphaLo_, fontAlphaHi_);
    glUniform1i(alphaCurveLoc, fontAlphaCurveEnabled_ ? 1 : 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, atlas.textureId);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    float fontScale = scale * (atlas.fontSize / 22.0f) * fontSizeMultiplier_;

    // Outline pass: stamp the same glyph batch several times, offset by a small ring of
    // screen pixels so the decorative faces stay separated from the background behind them.
    // The floor only binds for the decorative faces that ask for a sub-pixel ring (the title
    // row does); it is low enough that the UI default of 1.0 px is never touched by it, so
    // lowering it does not thin any other screen's text.
    if (fontOutlineEnabled_ && fontOutlineColor_.w > 0.0f) {
        const float ring = std::clamp(fontScale * fontOutlineWidth_, 0.25f, 3.0f);
        const float offsets[9][2] = {
            {-ring, 0.0f}, {ring, 0.0f}, {0.0f, -ring}, {0.0f, ring},
            {-ring, -ring}, {ring, -ring}, {-ring, ring}, {ring, ring},
            {0.0f, 0.0f},   // centre pass: covers counters and the glyph risers
        };

        glUniform4f(colorLoc, fontOutlineColor_.x, fontOutlineColor_.y,
                    fontOutlineColor_.z, fontOutlineColor_.w * color.w);

        batchScratch_.clear();
        batchScratch_.reserve(text.size() * 24 * 9);
        for (int i = 0; i < 9; ++i) {
            appendOblivionRun(text, x + offsets[i][0], y + offsets[i][1], fontScale, atlas);
        }
        uploadAndDrawBatch();
    }

    glUniform4f(colorLoc, color.x, color.y, color.z, color.w);

    batchScratch_.clear();
    batchScratch_.reserve(text.size() * 24);
    appendOblivionRun(text, x, y, fontScale, atlas);
    uploadAndDrawBatch();

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glUseProgram(0);
}

float TextRenderer::getTextWidth(const std::string& text, float scale) {
    if (text.empty()) return 0.0f;

    // Use Oblivion font metrics when active font is not Roboto
    if (activeFont != FontType::Roboto) {
        int idx = static_cast<int>(activeFont);
        const OblivionFontAtlas& atlas = oblivionFonts[idx];
        if (atlas.textureId == 0 || atlas.glyphs.empty()) {
            return 0.0f;
        }
        float fontScale = scale * (atlas.fontSize / 22.0f) * fontSizeMultiplier_;
        float width = 0.0f;
        for (char ch : text) {
            unsigned int codepoint = (unsigned char)ch;
            auto it = atlas.glyphs.find(codepoint);
            if (it != atlas.glyphs.end()) {
                width += it->second.advance * fontScale;
            } else {
                auto spaceIt = atlas.glyphs.find(32);
                float spaceAdvance = (spaceIt != atlas.glyphs.end()) ? spaceIt->second.advance : atlas.fontSize * 0.5f;
                width += spaceAdvance * fontScale;
            }
        }
        return width;
    }

    if (fontData == nullptr) {
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

    float TextRenderer::getTextCapHeight(float scale) const {
        if (activeFont == FontType::Roboto) {
            return getTextHeight(scale);
        }
        const OblivionFontAtlas& atlas = oblivionFonts[static_cast<int>(activeFont)];
        if (atlas.textureId == 0) return 0.0f;
        return atlas.capBaseline * scale * (atlas.fontSize / 22.0f) * fontSizeMultiplier_;
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

    // Parse glyph records (56 bytes each, 255 records starting at offset 344)
    // Record layout (14 floats):
    //   [0]  bearing_x (float)
    //   [1]  advance   (float)
    //   [2]  padding   (float, always 0)
    //   [3]  u0        (float, texture U left)
    //   [4]  v0        (float, texture V top)
    //   [5]  u1        (float, texture U right, same row as v0)
    //   [6]  v0_dup    (float, duplicate of v0)
    //   [7]  u0_dup    (float, duplicate of u0)
    //   [8]  v1        (float, texture V bottom)
    //   [9]  u1_dup    (float, duplicate of u1)
    //   [10] v1_dup    (float, duplicate of v1)
    //   [11] width     (float, pixel width)
    //   [12] height    (float, pixel height)
    //   [13] padding   (float, always 0)
    // Record index = ASCII code (32..254)
    static const int GLYPH_START_OFFSET = 344;
    static const int GLYPH_STRIDE = 56;
    static const int GLYPH_COUNT = 255;

    oblivionFonts[idx].glyphs.clear();
    int parsedCount = 0;
    for (int i = 0; i < GLYPH_COUNT; i++) {
        int off = GLYPH_START_OFFSET + i * GLYPH_STRIDE;
        if (off + GLYPH_STRIDE > (int)fntSize) break;

        const float* f = (const float*)(fntBuf + off);

        // Skip empty records (bearing_x == 2.0 and advance == 0 and no UV)
        if (f[3] == 0.0f && f[4] == 0.0f && f[5] == 0.0f && f[11] == 0.0f)
            continue;

        OblivionGlyph g;
        // Every cell is a tight box around its own ink, so consecutive cells must be
        // butted together: the advance is the cell width and the bearing is zero.
        // f[1] is not an advance (look-alike glyphs carry wildly different values,
        // e.g. '/'=3 vs '\'=22) and f[0] is -1 for every glyph except '!'.
        g.bearing_x = 0.0f;
        g.advance   = f[11];
        g.u0        = f[3];   // texture U left
        g.v0        = f[4];   // texture V top
        g.u1        = f[5];   // texture U right
        g.v1        = f[8];   // texture V bottom (f[8] is the second row's v)
        g.width     = f[11];  // pixel width
        g.height    = f[12];  // pixel height
        g.baseline_rel = 0.0f;  // measured from the atlas below
        // Record index maps to codepoint as (record + 1): record 0 -> ASCII 1,
        // record 32 -> ASCII 33 ('!'), record 65 -> ASCII 66 ('B'), etc.
        // Verified by template-matching each glyph against reference renderings.
        oblivionFonts[idx].glyphs[i + 1] = g;
        parsedCount++;
    }

    // Space (ASCII 32) has no glyph record; synthesize one so word spacing works.
    if (oblivionFonts[idx].glyphs.find(32) == oblivionFonts[idx].glyphs.end()) {
        OblivionGlyph space;
        space.bearing_x = 0.0f;
        space.advance   = fontSize * 0.35f;
        space.u0 = space.v0 = space.u1 = space.v1 = 0.0f;
        space.width = space.height = 0.0f;
        space.baseline_rel = 0.0f;
        oblivionFonts[idx].glyphs[32] = space;
    }
    LOGI("  Parsed %d glyph records (offset %d, stride %d)",
         parsedCount, GLYPH_START_OFFSET, GLYPH_STRIDE);
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

    // Some original atlases (kingthings_shadowed) keep the crisp letter in RGB and a
    // blurred shadow/glow silhouette in alpha. Detect that split and read RGB instead
    // of rendering the glow alone.
    long alphaInk = 0, rgbInk = 0;
    for (int i = 0; i < w * h; i++) {
        if (pixels[i * 4 + 3] > 8) alphaInk++;
        if (pixels[i * 4 + 0] > 8) rgbInk++;
    }
    oblivionFonts[idx].useRgbChannel = alphaInk > (long)(rgbInk * 3 / 2);
    LOGI("  Ink: rgb=%ld alpha=%ld -> sample %s",
         rgbInk, alphaInk, oblivionFonts[idx].useRgbChannel ? "RGB" : "alpha");

    // The .fnt carries no vertical metrics (header offsets 76..344 are all zero), so
    // recover the baseline from the atlas itself: cells are tight boxes around their
    // own ink and every capital's ink stops on the baseline. Glyphs that stop above
    // that row (x-height letters, punctuation) get pushed down onto it, while
    // descenders keep their natural depth below it.
    {
        const bool useRgb = oblivionFonts[idx].useRgbChannel;
        const int stride = w * 4;
        std::map<unsigned int, int> inkBottom;
        int capHistogram[512] = {0};
        for (const auto& entry : oblivionFonts[idx].glyphs) {
            const OblivionGlyph& g = entry.second;
            int cw = (int)(g.width + 0.5f);
            int chh = (int)(g.height + 0.5f);
            if (cw <= 0 || chh <= 0) continue;
            int x0 = (int)(g.u0 * w + 0.5f);
            int y0 = (int)(g.v0 * h + 0.5f);
            if (x0 < 0 || y0 < 0 || x0 + cw > w || y0 + chh > h) continue;
            int last = -1;
            for (int y = 0; y < chh; y++) {
                const uint8_t* row = pixels + (size_t)(y0 + y) * stride + x0 * 4;
                bool ink = false;
                for (int x = 0; x < cw; x++) {
                    if (row[x * 4 + (useRgb ? 0 : 3)] > 60) { ink = true; break; }
                }
                if (ink) last = y;
            }
            if (last < 0) continue;
            inkBottom[entry.first] = last;
            if (entry.first >= 'A' && entry.first <= 'Z') capHistogram[last]++;
        }
        int capBaseline = -1, best = 0;
        for (int i = 0; i < 512; i++) {
            if (capHistogram[i] > best) { best = capHistogram[i]; capBaseline = i; }
        }
        if (capBaseline < 0) {
            // No capitals in this atlas: fall back to the deepest ink measured.
            for (const auto& e : inkBottom) capBaseline = std::max(capBaseline, e.second);
        }
        if (capBaseline < 0) capBaseline = 0;
        oblivionFonts[idx].capBaseline = (float)capBaseline;
        for (auto& entry : oblivionFonts[idx].glyphs) {
            auto it = inkBottom.find(entry.first);
            entry.second.baseline_rel = (it == inkBottom.end())
                ? (float)capBaseline
                : (float)std::min(it->second, capBaseline);
        }
        LOGI("  Baseline: cap rows=%d (%zu glyphs measured)", capBaseline, inkBottom.size());
    }

    // Upload as RGBA8 texture. A face can be requested again (font switch, or a retry
    // after a failed load) and this is the only place that allocates the atlas texture,
    // so release the previous name first or every reload leaks one (see shutdown()).
    if (oblivionFonts[idx].textureId != 0) {
        glDeleteTextures(1, &oblivionFonts[idx].textureId);
        oblivionFonts[idx].textureId = 0;
    }
    glGenTextures(1, &oblivionFonts[idx].textureId);
    glBindTexture(GL_TEXTURE_2D, oblivionFonts[idx].textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    // Mipmaps keep the smaller UI scales (0.5-0.7) from aliasing the thin strokes.
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    stbi_image_free(pixels);
    LOGI("Oblivion font loaded: type=%d texId=%u glyphs=%zu", idx, oblivionFonts[idx].textureId, oblivionFonts[idx].glyphs.size());
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

void TextRenderer::requestFont(FontType type) {
    // The DebugMenu and the console run on the input thread, which owns no GL context:
    // generating the atlas texture there silently yields textureId 0 and the face never
    // draws. Record the request and let the render thread do the upload.
    pendingFontRequest_ = type;
    hasPendingFontRequest_ = true;
    LOGI("Font switch requested: %d (%s)", (int)type, getFontTypeName(type));
}

void TextRenderer::processPendingFontRequest() {
    if (!hasPendingFontRequest_) {
        return;
    }
    hasPendingFontRequest_ = false;

    const FontType type = pendingFontRequest_;
    if (type != FontType::Roboto) {
        // Roboto has no Oblivion atlas; it is always available.
        if (!loadOblivionFont(type)) {
            LOGE("Font switch failed, keeping %s: %d (%s)",
                 getFontTypeName(activeFont), (int)type, getFontTypeName(type));
            return;
        }
        fontMissingLogged_[static_cast<int>(type)] = false;
    }
    setActiveFont(type);
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

void TextRenderer::setFontAlphaCurve(float lo, float hi) {
    // Clamp so the curve can never invert or collapse to a hard cutoff.
    fontAlphaLo_ = std::clamp(lo, 0.0f, 0.95f);
    fontAlphaHi_ = std::clamp(hi, fontAlphaLo_ + 0.05f, 1.0f);
    LOGI("Font alpha curve: lo=%.3f hi=%.3f enabled=%d",
         fontAlphaLo_, fontAlphaHi_, fontAlphaCurveEnabled_ ? 1 : 0);
}

void TextRenderer::setFontAlphaCurveEnabled(bool enabled) {
    fontAlphaCurveEnabled_ = enabled;
    LOGI("Font alpha curve %s", enabled ? "enabled" : "disabled");
}

void TextRenderer::setFontSizeMultiplier(float mult) {
    fontSizeMultiplier_ = std::clamp(mult, 0.5f, 2.0f);
    LOGI("Font size multiplier: %.3f", fontSizeMultiplier_);
}

void TextRenderer::setFontOutlineEnabled(bool enabled) {
    fontOutlineEnabled_ = enabled;
    LOGI("Font outline %s", enabled ? "enabled" : "disabled");
}

void TextRenderer::setFontOutlineWidth(float atlasPixels) {
    fontOutlineWidth_ = std::clamp(atlasPixels, 0.0f, 4.0f);
    LOGI("Font outline width: %.2f", fontOutlineWidth_);
}

void TextRenderer::setFontOutlineColor(const glm::vec4& color) {
    fontOutlineColor_ = glm::vec4(std::clamp(color.x, 0.0f, 1.0f), std::clamp(color.y, 0.0f, 1.0f),
                                  std::clamp(color.z, 0.0f, 1.0f), std::clamp(color.w, 0.0f, 1.0f));
    LOGI("Font outline color: %.2f %.2f %.2f %.2f", fontOutlineColor_.x, fontOutlineColor_.y,
         fontOutlineColor_.z, fontOutlineColor_.w);
}

void TextRenderer::renderTextOblivion(const std::string& text, float x, float y,
                                       const glm::vec4& color, float scale) {
    // Superseded by the batched Oblivion path inside renderText(). Forwarding
    // here keeps the two entry points from drifting apart.
    renderText(text, x, y, color, scale);
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
