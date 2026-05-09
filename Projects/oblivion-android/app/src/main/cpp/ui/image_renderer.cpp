#include "image_renderer.h"
#include <android/log.h>
#include <cstdio>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "../third_party/stb_image.h"

#define LOG_TAG_IMG "ImageRenderer"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_IMG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_IMG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_IMG, __VA_ARGS__)

static const char* vertexShaderSource =
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

static const char* fragmentShaderSource =
    "#version 300 es\n"
    "precision mediump float;\n"
    "in vec2 vTexCoord;\n"
    "out vec4 FragColor;\n"
    "uniform sampler2D uTexture;\n"
    "void main() {\n"
    "    FragColor = texture(uTexture, vTexCoord);\n"
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

ImageRenderer::ImageRenderer()
    : textureId(0), shaderProgram(0), vao(0), vbo(0),
      imgWidth(0), imgHeight(0), initialized(false),
      screenWidth(1920), screenHeight(1080) {
}

ImageRenderer::~ImageRenderer() {
    cleanup();
}

bool ImageRenderer::compileShaders() {
    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
    if (!vs || !fs) return false;

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
        return false;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);

    // Setup VAO/VBO for a quad
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    // 6 vertices * 4 floats (x, y, u, v)
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);

    return true;
}

void ImageRenderer::uploadTexture(const unsigned char* data, int width, int height) {
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, 0);
    imgWidth = width;
    imgHeight = height;
}

bool ImageRenderer::loadFromFile(const std::string& path) {
    cleanup();

    int width, height, channels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 4);
    if (!data) {
        LOGE("Failed to load image: %s", path.c_str());
        return false;
    }

    if (!compileShaders()) {
        stbi_image_free(data);
        return false;
    }

    uploadTexture(data, width, height);
    stbi_image_free(data);

    initialized = true;
    LOGI("Loaded image: %s (%dx%d)", path.c_str(), width, height);
    return true;
}

bool ImageRenderer::loadFromMemory(const unsigned char* data, int len) {
    cleanup();

    int width, height, channels;
    unsigned char* pixels = stbi_load_from_memory(data, len, &width, &height, &channels, 4);
    if (!pixels) {
        LOGE("Failed to load image from memory");
        return false;
    }

    if (!compileShaders()) {
        stbi_image_free(pixels);
        return false;
    }

    uploadTexture(pixels, width, height);
    stbi_image_free(pixels);

    initialized = true;
    LOGI("Loaded image from memory: %dx%d", width, height);
    return true;
}

bool ImageRenderer::loadFromAssetManager(AAssetManager* mgr, const std::string& assetPath) {
    cleanup();

    AAsset* asset = AAssetManager_open(mgr, assetPath.c_str(), AASSET_MODE_STREAMING);
    if (!asset) {
        LOGE("Failed to open asset: %s", assetPath.c_str());
        return false;
    }

    off_t length = AAsset_getLength(asset);
    std::vector<unsigned char> buffer(length);
    AAsset_read(asset, buffer.data(), length);
    AAsset_close(asset);

    int width, height, channels;
    unsigned char* pixels = stbi_load_from_memory(buffer.data(), length, &width, &height, &channels, 4);
    if (!pixels) {
        LOGE("Failed to decode image from asset: %s", assetPath.c_str());
        return false;
    }

    if (!compileShaders()) {
        stbi_image_free(pixels);
        return false;
    }

    uploadTexture(pixels, width, height);
    stbi_image_free(pixels);

    initialized = true;
    LOGI("Loaded image from asset: %s (%dx%d)", assetPath.c_str(), width, height);
    return true;
}

void ImageRenderer::draw(float x, float y, float width, float height) {
    if (!initialized || !textureId) return;

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float x0 = x;
    float y0 = y;
    float x1 = x + width;
    float y1 = y + height;

    float vertices[6][4] = {
        { x0, y0, 0.0f, 0.0f },
        { x0, y1, 0.0f, 1.0f },
        { x1, y1, 1.0f, 1.0f },
        { x0, y0, 0.0f, 0.0f },
        { x1, y1, 1.0f, 1.0f },
        { x1, y0, 1.0f, 0.0f }
    };

    glUseProgram(shaderProgram);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glUniform1i(glGetUniformLocation(shaderProgram, "uTexture"), 0);
    glUniform2f(glGetUniformLocation(shaderProgram, "uScreenSize"), (float)screenWidth, (float)screenHeight);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void ImageRenderer::drawFullscreen() {
    draw(0, 0, (float)screenWidth, (float)screenHeight);
}

void ImageRenderer::cleanup() {
    if (textureId) {
        glDeleteTextures(1, &textureId);
        textureId = 0;
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
    imgWidth = 0;
    imgHeight = 0;
    initialized = false;
}
