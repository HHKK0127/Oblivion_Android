// ============================================================================
// VideoRenderer - renders decoded video frames to screen via OpenGL ES
// Phase 53: Bink Video replacement
// ============================================================================

#include "video_renderer.h"
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <GLES2/gl2ext.h>
#include <android/log.h>
#include <cstring>
#include <cmath>

#define LOG_TAG "VideoRenderer"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace oblivion {
namespace video {

// ============================================================================
// Shader source code
// ============================================================================

static const char* kVertexShaderSource = R"(#version 300 es
precision mediump float;

layout(location = 0) in vec4 aPosition;
layout(location = 1) in vec2 aTexCoord;

uniform mat4 uMVP;
uniform mat4 uTexTransform;

out vec2 vTexCoord;

void main() {
    gl_Position = uMVP * aPosition;
    vTexCoord = (uTexTransform * vec4(aTexCoord, 0.0, 1.0)).xy;
}
)";

static const char* kFragmentShaderSource = R"(#version 300 es
#extension GL_OES_EGL_image_external_essl3 : require
precision mediump float;

uniform samplerExternalOES uTexture;

in vec2 vTexCoord;
out vec4 fragColor;

void main() {
    fragColor = texture(uTexture, vTexCoord);
}
)";

// ============================================================================
// Fullscreen quad vertex data (position + texcoord)
// ============================================================================

static const float kQuadVertices[] = {
    // Position (x, y)   TexCoord (u, v)
    -1.0f, -1.0f,        0.0f, 1.0f,
     1.0f, -1.0f,        1.0f, 1.0f,
    -1.0f,  1.0f,        0.0f, 0.0f,
     1.0f,  1.0f,        1.0f, 0.0f,
};

// ============================================================================
// Constructor / Destructor
// ============================================================================

VideoRenderer::VideoRenderer() {
    std::memset(transformMatrix_, 0, sizeof(transformMatrix_));
    // Identity matrix
    transformMatrix_[0] = 1.0f;
    transformMatrix_[5] = 1.0f;
    transformMatrix_[10] = 1.0f;
    transformMatrix_[15] = 1.0f;
}

VideoRenderer::~VideoRenderer() {
    if (initialized_) {
        shutdown();
    }
}

// ============================================================================
// Initialization
// ============================================================================

bool VideoRenderer::initialize() {
    if (initialized_) {
        LOGW("VideoRenderer already initialized");
        return true;
    }

    if (!createShader()) {
        LOGE("Failed to create video shader");
        return false;
    }

    if (!createGeometry()) {
        LOGE("Failed to create video geometry");
        return false;
    }

    initialized_ = true;
    LOGI("VideoRenderer initialized successfully");
    return true;
}

void VideoRenderer::shutdown() {
    if (shaderProgram_ != 0) {
        glDeleteProgram(shaderProgram_);
        shaderProgram_ = 0;
    }

    if (vbo_ != 0) {
        glDeleteBuffers(1, &vbo_);
        vbo_ = 0;
    }

    // Note: textureId_ is managed by SurfaceTexture, not deleted here
    textureId_ = 0;
    initialized_ = false;

    LOGI("VideoRenderer shutdown");
}

// ============================================================================
// Texture management
// ============================================================================

void VideoRenderer::setExternalTextureId(GLuint textureId) {
    std::lock_guard<std::mutex> lock(mutex_);
    textureId_ = textureId;
}

void VideoRenderer::setTransformMatrix(const float matrix[16]) {
    std::lock_guard<std::mutex> lock(mutex_);
    std::memcpy(transformMatrix_, matrix, sizeof(float) * 16);
}

// ============================================================================
// Rendering
// ============================================================================

void VideoRenderer::renderFullscreen() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_ || textureId_ == 0) {
        return;
    }

    // Save current GL state
    GLint prevProgram = 0;
    glGetIntegerv(GL_CURRENT_PROGRAM, &prevProgram);

    GLint prevBlend = 0;
    glGetIntegerv(GL_BLEND, &prevBlend);

    // Disable blending for video (opaque)
    glDisable(GL_BLEND);

    // Use video shader
    glUseProgram(shaderProgram_);

    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, textureId_);
    glUniform1i(uniformTexture_, 0);

    // Set transform matrix (from SurfaceTexture)
    glUniformMatrix4fv(uniformTransform_, 1, GL_FALSE, transformMatrix_);

    // Identity MVP for fullscreen
    float identity[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    glUniformMatrix4fv(uniformMVP_, 1, GL_FALSE, identity);

    // Bind VBO and set attributes
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);

    glEnableVertexAttribArray(attribPosition_);
    glVertexAttribPointer(attribPosition_, 2, GL_FLOAT, GL_FALSE,
                          4 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(attribTexCoord_);
    glVertexAttribPointer(attribTexCoord_, 2, GL_FLOAT, GL_FALSE,
                          4 * sizeof(float),
                          (void*)(2 * sizeof(float)));

    // Draw fullscreen quad
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    // Cleanup
    glDisableVertexAttribArray(attribPosition_);
    glDisableVertexAttribArray(attribTexCoord_);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_EXTERNAL_OES, 0);

    // Restore GL state
    glUseProgram(prevProgram);
    if (prevBlend) {
        glEnable(GL_BLEND);
    }
}

void VideoRenderer::renderInViewport(int x, int y, int width, int height) {
    if (!initialized_) {
        return;
    }

    // Save current viewport
    GLint prevViewport[4];
    glGetIntegerv(GL_VIEWPORT, prevViewport);

    // Set new viewport
    glViewport(x, y, width, height);

    // Render
    renderFullscreen();

    // Restore viewport
    glViewport(prevViewport[0], prevViewport[1],
               prevViewport[2], prevViewport[3]);
}

// ============================================================================
// Video dimensions and letterbox
// ============================================================================

void VideoRenderer::setVideoDimensions(int width, int height) {
    std::lock_guard<std::mutex> lock(mutex_);
    videoWidth_ = width;
    videoHeight_ = height;
    LOGD("Video dimensions set: %dx%d", width, height);
}

void VideoRenderer::setLetterboxColor(float r, float g, float b, float a) {
    std::lock_guard<std::mutex> lock(mutex_);
    letterboxColor_[0] = r;
    letterboxColor_[1] = g;
    letterboxColor_[2] = b;
    letterboxColor_[3] = a;
}

void VideoRenderer::calculateLetterboxMatrix(
    float screenAspect, float videoAspect, float* outMatrix) const {

    // Identity matrix
    std::memset(outMatrix, 0, sizeof(float) * 16);
    outMatrix[0] = 1.0f;
    outMatrix[5] = 1.0f;
    outMatrix[10] = 1.0f;
    outMatrix[15] = 1.0f;

    if (videoAspect <= 0.0f || screenAspect <= 0.0f) {
        return;
    }

    float scaleX = 1.0f;
    float scaleY = 1.0f;

    if (videoAspect > screenAspect) {
        // Video is wider than screen -> letterbox (black bars top/bottom)
        scaleY = screenAspect / videoAspect;
    } else if (videoAspect < screenAspect) {
        // Video is taller than screen -> pillarbox (black bars left/right)
        scaleX = videoAspect / screenAspect;
    }

    outMatrix[0] = scaleX;
    outMatrix[5] = scaleY;
}

// ============================================================================
// Shader creation
// ============================================================================

bool VideoRenderer::createShader() {
    // Compile vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &kVertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    GLint compileStatus = 0;
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &compileStatus);
    if (!compileStatus) {
        char logBuffer[512];
        glGetShaderInfoLog(vertexShader, sizeof(logBuffer), nullptr, logBuffer);
        LOGE("Vertex shader compile error: %s", logBuffer);
        glDeleteShader(vertexShader);
        return false;
    }

    // Compile fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &kFragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &compileStatus);
    if (!compileStatus) {
        char logBuffer[512];
        glGetShaderInfoLog(fragmentShader, sizeof(logBuffer), nullptr, logBuffer);
        LOGE("Fragment shader compile error: %s", logBuffer);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }

    // Link program
    shaderProgram_ = glCreateProgram();
    glAttachShader(shaderProgram_, vertexShader);
    glAttachShader(shaderProgram_, fragmentShader);
    glLinkProgram(shaderProgram_);

    GLint linkStatus = 0;
    glGetProgramiv(shaderProgram_, GL_LINK_STATUS, &linkStatus);
    if (!linkStatus) {
        char logBuffer[512];
        glGetProgramInfoLog(shaderProgram_, sizeof(logBuffer), nullptr, logBuffer);
        LOGE("Shader program link error: %s", logBuffer);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(shaderProgram_);
        shaderProgram_ = 0;
        return false;
    }

    // Cleanup shaders (linked into program)
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Get uniform locations
    uniformTexture_ = glGetUniformLocation(shaderProgram_, "uTexture");
    uniformTransform_ = glGetUniformLocation(shaderProgram_, "uTexTransform");
    uniformMVP_ = glGetUniformLocation(shaderProgram_, "uMVP");

    // Get attribute locations
    attribPosition_ = glGetAttribLocation(shaderProgram_, "aPosition");
    attribTexCoord_ = glGetAttribLocation(shaderProgram_, "aTexCoord");

    LOGI("Video shader compiled and linked (program=%u)", shaderProgram_);
    return true;
}

bool VideoRenderer::createGeometry() {
    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kQuadVertices),
                 kQuadVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    LOGI("Video geometry created (VBO=%u)", vbo_);
    return true;
}

} // namespace video
} // namespace oblivion
