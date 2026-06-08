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

// 頂点シェーダー: 2D テキスト用
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

// フラグメントシェーダー: テクスチャベースのテキスト表示
const char* textFragmentShader = R"(#version 300 es
precision mediump float;

in vec2 fragTexCoord;

uniform vec3 textColor;
uniform sampler2D fontTexture;

out vec4 FragColor;

void main() {
    float alpha = texture(fontTexture, fragTexCoord).r;
    FragColor = vec4(textColor, alpha);
}
)";

TextRenderer::TextRenderer()
    : vao(0), vbo(0), shaderProgram(0), projectionLoc(-1), colorLoc(-1),
      fontTexture(0), screenWidth(1080), screenHeight(1920), fontData(nullptr),
      assetManager(nullptr) {
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

    // シェーダーコンパイル
    compileShaders();

    if (shaderProgram == 0) {
        LOGE("Failed to compile text shaders");
        return false;
    }

    // ユニフォーム位置を取得
    projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    colorLoc = glGetUniformLocation(shaderProgram, "textColor");

    // VAO/VBO を生成
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    // 頂点属性: 位置（2D）と テクスチャ座標
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float),
                         (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // フォントデータを初期化
    fontData = new FontData();
    fontData->fontData = nullptr;
    fontData->fontScale = 0.0f;
    fontData->fontHeight = FONT_SIZE;

    // フォントを読み込んでテクスチャアトラスを作成
    // 失敗時はクラッシュを避けてテクスチャアトラスを作成
    if (!loadFontFromAssets("arial.ttf")) {
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

    // AAssetManager を使用して assets から font ファイルを開く
    AAsset* asset = AAssetManager_open(assetManager, filename.c_str(), AASSET_MODE_STREAMING);
    if (!asset) {
        LOGE("Could not open font asset: %s", filename.c_str());
        return false;
    }

    LOGI("Font asset opened successfully");

    // ファイルサイズを取得
    off_t fontDataSize = AAsset_getLength(asset);
    LOGI("Font data size: %ld bytes", fontDataSize);

    if (fontDataSize <= 0) {
        LOGE("Invalid font file size: %ld", fontDataSize);
        AAsset_close(asset);
        return false;
    }

    // フォントデータをメモリに読み込む
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

    // テクスチャアトラスのビットマップを作成
    unsigned char* atlasBuffer = new unsigned char[ATLAS_WIDTH * ATLAS_HEIGHT];
    memset(atlasBuffer, 0, ATLAS_WIDTH * ATLAS_HEIGHT);

    // stb_truetype フォント初期化
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

    // フォントスケール計算
    float scale = stbtt_ScaleForPixelHeight(&fontInfo, FONT_SIZE);
    fontData->fontScale = scale;

    // グリフを配置
    int currentAtlasX = 0;
    int currentAtlasY = 0;
    int rowHeight = 0;

    // ASCII 文字 (32-126) をレンダリング
    for (int codepoint = 32; codepoint < 127; codepoint++) {
        int glyph_index = stbtt_FindGlyphIndex(&fontInfo, codepoint);

        // グリフビットマップを取得
        int width, height, xoff, yoff;
        unsigned char* bitmap = stbtt_GetGlyphBitmap(&fontInfo, scale, scale,
                                                      glyph_index, &width, &height,
                                                      &xoff, &yoff);

        if (!bitmap) {
            // グリフがない場合、スペースを使用
            if (codepoint == 32) {
                width = 8;
                height = FONT_SIZE;
            } else {
                continue;
            }
        }

        // アトラスに収まるか確認
        if (currentAtlasX + width > ATLAS_WIDTH) {
            currentAtlasX = 0;
            currentAtlasY += rowHeight + 2;  // 2ピクセルの余白
            rowHeight = 0;
        }

        if (currentAtlasY + height > ATLAS_HEIGHT) {
            LOGW("Font atlas is full, cannot fit all glyphs");
            if (bitmap) {
                free(bitmap);
            }
            break;
        }

        // ビットマップをアトラスにコピー
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

        // グリフ情報をキャッシュに登録
        Glyph g;
        g.x0 = (float)currentAtlasX / ATLAS_WIDTH;
        g.y0 = (float)currentAtlasY / ATLAS_HEIGHT;
        g.x1 = (float)(currentAtlasX + width) / ATLAS_WIDTH;
        g.y1 = (float)(currentAtlasY + height) / ATLAS_HEIGHT;

        // アドバンス幅を取得
        int advance_width;
        stbtt_GetCodepointHMetrics(&fontInfo, codepoint, &advance_width, nullptr);
        g.advanceX = (float)advance_width * scale;

        g.bearingX = (float)xoff;
        g.bearingY = (float)yoff;

        fontData->glyphCache[codepoint] = g;

        LOGD("Glyph %c (%d): atlas pos=(%d,%d) size=(%d,%d) advance=%f",
             (char)codepoint, codepoint, currentAtlasX, currentAtlasY, width, height, g.advanceX);

        currentAtlasX += width + 1;  // 1ピクセルの余白
        rowHeight = (height > rowHeight) ? height : rowHeight;
    }

    // OpenGL テクスチャとして作成
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
    if (text.empty() || shaderProgram == 0 || fontTexture == 0) {
        return;
    }

    glUseProgram(shaderProgram);

    // 投影行列を設定
    glm::mat4 projection = glm::ortho(0.0f, (float)screenWidth, (float)screenHeight, 0.0f, -1.0f, 1.0f);
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, &projection[0][0]);

    // テキスト色を設定
    glUniform3f(colorLoc, color.x, color.y, color.z);

    // フォントテクスチャをバインド
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fontTexture);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    float currentX = x;
    float currentY = y;

    for (char ch : text) {
        unsigned int codepoint = (unsigned char)ch;

        // グリフ情報を取得
        Glyph glyph = getGlyph(codepoint);

        // 文字の幅と高さ
        float charWidth = 16.0f * scale;
        float charHeight = 16.0f * scale;

        // 頂点データを生成（四角形: 2三角形）
        float vertices[] = {
            // 位置座標              テクスチャ座標
            currentX,               currentY,               glyph.x0, glyph.y0,  // 左上
            currentX + charWidth,   currentY,               glyph.x1, glyph.y0,  // 右上
            currentX,               currentY + charHeight,  glyph.x0, glyph.y1,  // 左下

            currentX + charWidth,   currentY,               glyph.x1, glyph.y0,  // 右上
            currentX + charWidth,   currentY + charHeight,  glyph.x1, glyph.y1,  // 右下
            currentX,               currentY + charHeight,  glyph.x0, glyph.y1,  // 左下
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

TextRenderer::Glyph TextRenderer::getGlyph(unsigned int codepoint) {
    // キャッシュから検索
    auto it = fontData->glyphCache.find(codepoint);
    if (it != fontData->glyphCache.end()) {
        return it->second;
    }

    // 見つからない場合はスペースを返す
    return fontData->glyphCache[32];  // ASCII 32 = space
}

void TextRenderer::setScreenSize(int width, int height) {
    screenWidth = width;
    screenHeight = height;
    LOGD("TextRenderer screen size set to %d x %d", screenWidth, screenHeight);
}

void TextRenderer::compileShaders() {
    LOGD("Compiling text shaders");

    // 頂点シェーダーをコンパイル
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &textVertexShader, nullptr);
    glCompileShader(vertexShader);

    // フラグメントシェーダーをコンパイル
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &textFragmentShader, nullptr);
    glCompileShader(fragmentShader);

    // プログラムをリンク
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    LOGD("Shaders compiled and linked");
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
    glUniform3f(colorLoc, 1.0f, 1.0f, 1.0f);

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
