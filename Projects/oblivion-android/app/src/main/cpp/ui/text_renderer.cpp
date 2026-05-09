#include "text_renderer.h"
#include <GLES3/gl3.h>
#include <android/log.h>
#include <android/asset_manager.h>
#include <vector>
#include <string>
#include <cstring>

#define LOG_TAG_TEXT "TextRenderer"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_TEXT, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_TEXT, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_TEXT, __VA_ARGS__)

// stb_truetype implementation
#define STB_TRUETYPE_IMPLEMENTATION
#include "../third_party/stb_truetype.h"

static const char* textVertexShader =
    "#version 300 es\n"
    "layout(location = 0) in vec2 aPos;\n"
    "layout(location = 1) in vec2 aTexCoord;\n"
    "out vec2 vTexCoord;\n"
    "uniform vec2 uScreenSize;\n"
    "void main() {\n"
    "    vec2 clip = (aPos / uScreenSize) * 2.0 - 1.0;\n"
    "    clip.y = -clip.y;\n"
    "    gl_Position = vec4(clip, 0.0, 1.0);\n"
    "    vTexCoord = aTexCoord;\n"
    "}\n";

static const char* textFragmentShader =
    "#version 300 es\n"
    "precision mediump float;\n"
    "in vec2 vTexCoord;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D uTexture;\n"
    "uniform vec4 uColor;\n"
    "void main() {\n"
    "    float alpha = texture(uTexture, vTexCoord).r;\n"
    "    FragColor = vec4(uColor.rgb, uColor.a * alpha);\n"
    "}\n";

static GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        LOGE("Shader compile error: %s", log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

struct TextRenderer::FontData {
    stbtt_fontinfo info;
    std::vector<unsigned char> ttfBuffer;
    GLuint atlasTexture = 0;
    int atlasWidth = 512;
    int atlasHeight = 512;
    float scale = 1.0f;
    int ascent = 0;
    int descent = 0;
    int lineGap = 0;
    stbtt_packedchar chars[96]; // ASCII 32..127
};

TextRenderer::TextRenderer()
    : screenWidth(0), screenHeight(0), initialized(false),
      fontData(nullptr), shaderProgram(0), vao(0), vbo(0) {
}

TextRenderer::~TextRenderer() {
    cleanup();
}

bool TextRenderer::initialize() {
    LOGI("TextRenderer initializing with system font...");

    // Try common Android system font paths
    const char* fontPaths[] = {
        "/system/fonts/Roboto-Regular.ttf",
        "/system/fonts/Roboto.ttf",
        "/system/fonts/DroidSans.ttf",
        "/system/fonts/NotoSansCJK-Regular.ttc",
        nullptr
    };

    FILE* fontFile = nullptr;
    const char* usedPath = nullptr;
    for (int i = 0; fontPaths[i]; ++i) {
        fontFile = fopen(fontPaths[i], "rb");
        if (fontFile) {
            usedPath = fontPaths[i];
            break;
        }
    }

    if (!fontFile) {
        LOGE("Failed to open any system font file");
        return false;
    }

    LOGI("Opened system font: %s", usedPath);

    fseek(fontFile, 0, SEEK_END);
    long length = ftell(fontFile);
    fseek(fontFile, 0, SEEK_SET);

    std::vector<unsigned char> buffer(length);
    fread(buffer.data(), 1, length, fontFile);
    fclose(fontFile);

    LOGI("Font loaded: %ld bytes", length);

    fontData = new FontData();
    fontData->ttfBuffer = std::move(buffer);

    if (!stbtt_InitFont(&fontData->info, fontData->ttfBuffer.data(), 0)) {
        LOGE("Failed to init font");
        delete fontData;
        fontData = nullptr;
        return false;
    }

    // Create atlas
    const int fontSizePixels = 48;
    fontData->scale = stbtt_ScaleForPixelHeight(&fontData->info, fontSizePixels);

    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&fontData->info, &ascent, &descent, &lineGap);
    fontData->ascent = ascent;
    fontData->descent = descent;
    fontData->lineGap = lineGap;

    std::vector<unsigned char> atlasBitmap(fontData->atlasWidth * fontData->atlasHeight);
    stbtt_pack_context pc;
    if (!stbtt_PackBegin(&pc, atlasBitmap.data(), fontData->atlasWidth, fontData->atlasHeight, 0, 1, nullptr)) {
        LOGE("Failed to begin font pack");
        delete fontData;
        fontData = nullptr;
        return false;
    }

    stbtt_PackSetOversampling(&pc, 2, 2);
    stbtt_PackFontRange(&pc, fontData->ttfBuffer.data(), 0, fontSizePixels, 32, 96, fontData->chars);
    stbtt_PackEnd(&pc);

    // Upload atlas to GPU
    glGenTextures(1, &fontData->atlasTexture);
    glBindTexture(GL_TEXTURE_2D, fontData->atlasTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, fontData->atlasWidth, fontData->atlasHeight, 0, GL_RED, GL_UNSIGNED_BYTE, atlasBitmap.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Compile shader
    GLuint vs = compileShader(GL_VERTEX_SHADER, textVertexShader);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, textFragmentShader);
    if (!vs || !fs) {
        cleanup();
        return false;
    }

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);
    glLinkProgram(shaderProgram);
    GLint linkSuccess;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &linkSuccess);
    if (!linkSuccess) {
        char log[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, log);
        LOGE("Shader link error: %s", log);
        glDeleteShader(vs);
        glDeleteShader(fs);
        cleanup();
        return false;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);

    // Setup VAO/VBO
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);

    initialized = true;
    LOGI("TextRenderer initialized successfully with system font");
    return true;
}

bool TextRenderer::initialize(AAssetManager* assetManager) {
    if (initialized) {
        cleanup();
    }

    LOGI("TextRenderer initializing with AssetManager...");

    // Load font from assets
    AAsset* asset = AAssetManager_open(assetManager, "fonts/roboto.ttf", AASSET_MODE_STREAMING);
    if (!asset) {
        LOGE("Failed to open font file: fonts/roboto.ttf");
        return false;
    }

    off_t length = AAsset_getLength(asset);
    std::vector<unsigned char> buffer(length);
    AAsset_read(asset, buffer.data(), length);
    AAsset_close(asset);

    LOGI("Font loaded: %ld bytes", length);

    fontData = new FontData();
    fontData->ttfBuffer = std::move(buffer);

    if (!stbtt_InitFont(&fontData->info, fontData->ttfBuffer.data(), 0)) {
        LOGE("Failed to init font");
        delete fontData;
        fontData = nullptr;
        return false;
    }

    // Create atlas
    const int fontSizePixels = 48;
    fontData->scale = stbtt_ScaleForPixelHeight(&fontData->info, fontSizePixels);

    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&fontData->info, &ascent, &descent, &lineGap);
    fontData->ascent = ascent;
    fontData->descent = descent;
    fontData->lineGap = lineGap;

    std::vector<unsigned char> atlasBitmap(fontData->atlasWidth * fontData->atlasHeight);
    stbtt_pack_context pc;
    if (!stbtt_PackBegin(&pc, atlasBitmap.data(), fontData->atlasWidth, fontData->atlasHeight, 0, 1, nullptr)) {
        LOGE("Failed to begin font pack");
        delete fontData;
        fontData = nullptr;
        return false;
    }

    stbtt_PackSetOversampling(&pc, 2, 2);
    stbtt_PackFontRange(&pc, fontData->ttfBuffer.data(), 0, fontSizePixels, 32, 96, fontData->chars);
    stbtt_PackEnd(&pc);

    // Upload atlas to GPU
    glGenTextures(1, &fontData->atlasTexture);
    glBindTexture(GL_TEXTURE_2D, fontData->atlasTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, fontData->atlasWidth, fontData->atlasHeight, 0, GL_RED, GL_UNSIGNED_BYTE, atlasBitmap.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Compile shader
    GLuint vs = compileShader(GL_VERTEX_SHADER, textVertexShader);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, textFragmentShader);
    if (!vs || !fs) {
        cleanup();
        return false;
    }

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);
    glLinkProgram(shaderProgram);
    GLint linkSuccess;
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &linkSuccess);
    if (!linkSuccess) {
        char log[512];
        glGetProgramInfoLog(shaderProgram, 512, nullptr, log);
        LOGE("Shader link error: %s", log);
        glDeleteShader(vs);
        glDeleteShader(fs);
        cleanup();
        return false;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);

    // Setup VAO/VBO
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);

    initialized = true;
    LOGI("TextRenderer initialized successfully with font atlas %dx%d",
         fontData->atlasWidth, fontData->atlasHeight);
    return true;
}

void TextRenderer::cleanup() {
    if (fontData) {
        if (fontData->atlasTexture) {
            glDeleteTextures(1, &fontData->atlasTexture);
        }
        delete fontData;
        fontData = nullptr;
    }
    if (shaderProgram) {
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
    }
    if (vao) {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
    }
    if (vbo) {
        glDeleteBuffers(1, &vbo);
        vbo = 0;
    }
    initialized = false;
    LOGD("TextRenderer cleaned up");
}

void TextRenderer::setScreenSize(unsigned int width, unsigned int height) {
    screenWidth = width;
    screenHeight = height;
    LOGD("TextRenderer screen size set: %ux%u", screenWidth, screenHeight);
}

void TextRenderer::renderText(const std::string& text, float x, float y,
                                float scale, const float color[4]) {
    if (!initialized || !fontData || !shaderProgram) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    glUseProgram(shaderProgram);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, fontData->atlasTexture);
    glUniform1i(glGetUniformLocation(shaderProgram, "uTexture"), 0);
    glUniform4fv(glGetUniformLocation(shaderProgram, "uColor"), 1, color);
    glUniform2f(glGetUniformLocation(shaderProgram, "uScreenSize"), (float)screenWidth, (float)screenHeight);
    glBindVertexArray(vao);

    float cursorX = x;
    float cursorY = y;

    for (size_t i = 0; i < text.size(); ++i) {
        char c = text[i];
        if (c == '\n') {
            cursorX = x;
            cursorY += (fontData->ascent - fontData->descent) * fontData->scale * scale * 1.2f;
            continue;
        }
        if (c < 32 || c >= 127) continue; // Unsupported char

        stbtt_packedchar* pc = &fontData->chars[c - 32];

        // Calculate quad position in screen space
        float x0 = cursorX + pc->xoff * scale;
        float y0 = cursorY + pc->yoff * scale;
        float x1 = x0 + (pc->x1 - pc->x0) * scale;
        float y1 = y0 + (pc->y1 - pc->y0) * scale;

        // Advance cursor
        cursorX += pc->xadvance * scale;

        // UV coordinates from packed char
        float s0 = pc->x0 / (float)fontData->atlasWidth;
        float t0 = pc->y0 / (float)fontData->atlasHeight;
        float s1 = pc->x1 / (float)fontData->atlasWidth;
        float t1 = pc->y1 / (float)fontData->atlasHeight;

        float vertices[6][4] = {
            { x0, y0, s0, t0 },
            { x0, y1, s0, t1 },
            { x1, y1, s1, t1 },
            { x0, y0, s0, t0 },
            { x1, y1, s1, t1 },
            { x1, y0, s1, t0 }
        };

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void TextRenderer::renderText(const std::string& text, float x, float y,
                                float scale, float r, float g, float b, float a) {
    float color[4] = {r, g, b, a};
    renderText(text, x, y, scale, color);
}

float TextRenderer::getTextWidth(const std::string& text, float scale) const {
    if (!fontData) return text.size() * scale * 0.5f;
    float width = 0.0f;
    for (size_t i = 0; i < text.size(); ++i) {
        char c = text[i];
        if (c < 32 || c >= 127) continue;
        stbtt_packedchar* pc = &fontData->chars[c - 32];
        width += pc->xadvance * scale;
    }
    return width;
}

float TextRenderer::getTextHeight(float scale) const {
    if (!fontData) return scale * 1.0f;
    return fontData->scale * (fontData->ascent - fontData->descent) * scale;
}
