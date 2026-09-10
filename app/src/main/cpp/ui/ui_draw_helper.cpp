#include "ui_draw_helper.h"
#include <android/log.h>

GLuint UIDrawHelper::s_colorProgram = 0;
GLuint UIDrawHelper::s_textureProgram = 0;
GLuint UIDrawHelper::s_gradientProgram = 0;
GLuint UIDrawHelper::s_radialProgram = 0;
GLuint UIDrawHelper::s_vao = 0;
GLuint UIDrawHelper::s_vbo = 0;
GLuint UIDrawHelper::s_ebo = 0;
bool UIDrawHelper::s_initialized = false;

static const char* s_uiVertexShader = R"(#version 300 es
precision mediump float;
layout(location = 0) in vec2 aPos;
uniform mat4 uProjection;
void main() {
    gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
}
)";

static const char* s_uiFragmentShader = R"(#version 300 es
precision mediump float;
uniform vec4 uColor;
out vec4 fragColor;
void main() {
    fragColor = uColor;
}
)";

static const char* s_uiTextureVertexShader = R"(#version 300 es
precision mediump float;
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;
uniform mat4 uProjection;
out vec2 vTexCoord;
void main() {
    gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
    vTexCoord = aTexCoord;
}
)";

static const char* s_uiTextureFragmentShader = R"(#version 300 es
precision mediump float;
uniform vec4 uColor;
uniform sampler2D uTexture;
in vec2 vTexCoord;
out vec4 fragColor;
void main() {
    fragColor = texture(uTexture, vTexCoord) * uColor;
}
)";

// Gradient vertex shader: passes normalized y-coordinate [0,1] (vertical) or x (horizontal)
static const char* s_gradientVertexShader = R"(#version 300 es
precision mediump float;
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
uniform mat4 uProjection;
out vec2 vUV;
void main() {
    gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
    vUV = aUV;
}
)";

// Gradient fragment shader: 4 colors at 4 corners for full gradient interpolation
static const char* s_gradientFragmentShader = R"(#version 300 es
precision mediump float;
uniform vec4 uColorTL;
uniform vec4 uColorTR;
uniform vec4 uColorBL;
uniform vec4 uColorBR;
in vec2 vUV;
out vec4 fragColor;
void main() {
    vec4 top = mix(uColorTL, uColorTR, vUV.x);
    vec4 bottom = mix(uColorBL, uColorBR, vUV.x);
    fragColor = mix(bottom, top, vUV.y);
}
)";

// Radial glow vertex shader
static const char* s_radialVertexShader = R"(#version 300 es
precision mediump float;
layout(location = 0) in vec2 aPos;
uniform mat4 uProjection;
void main() {
    gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
}
)";

// Radial glow fragment shader: distance from center -> smooth falloff
static const char* s_radialFragmentShader = R"(#version 300 es
precision mediump float;
uniform vec4 uInnerColor;
uniform vec4 uOuterColor;
uniform vec2 uCenter;
uniform vec2 uRadius;
uniform vec2 uScreenSize;
out vec4 fragColor;
void main() {
    vec2 px = gl_FragCoord.xy;
    vec2 d = (px - uCenter) / uRadius;
    float r = length(d);
    float t = clamp(r, 0.0, 1.0);
    // Smooth falloff with quadratic curve
    t = t * t * (3.0 - 2.0 * t);
    fragColor = mix(uInnerColor, uOuterColor, t);
}
)";

GLuint UIDrawHelper::compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint compiled;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        char log[512];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        __android_log_print(ANDROID_LOG_ERROR, "UIDrawHelper",
                            "Shader compile error: %s", log);
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

void UIDrawHelper::initialize() {
    if (s_initialized) return;

    // Color shader program
    GLuint vs = compileShader(GL_VERTEX_SHADER, s_uiVertexShader);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, s_uiFragmentShader);
    if (vs && fs) {
        s_colorProgram = glCreateProgram();
        glAttachShader(s_colorProgram, vs);
        glAttachShader(s_colorProgram, fs);
        glLinkProgram(s_colorProgram);
        glDeleteShader(vs);
        glDeleteShader(fs);
    }

    // Texture shader program
    GLuint tvs = compileShader(GL_VERTEX_SHADER, s_uiTextureVertexShader);
    GLuint tfs = compileShader(GL_FRAGMENT_SHADER, s_uiTextureFragmentShader);
    if (tvs && tfs) {
        s_textureProgram = glCreateProgram();
        glAttachShader(s_textureProgram, tvs);
        glAttachShader(s_textureProgram, tfs);
        glLinkProgram(s_textureProgram);
        glDeleteShader(tvs);
        glDeleteShader(tfs);
    }

    // Gradient shader program
    GLuint gvs = compileShader(GL_VERTEX_SHADER, s_gradientVertexShader);
    GLuint gfs = compileShader(GL_FRAGMENT_SHADER, s_gradientFragmentShader);
    if (gvs && gfs) {
        s_gradientProgram = glCreateProgram();
        glAttachShader(s_gradientProgram, gvs);
        glAttachShader(s_gradientProgram, gfs);
        glLinkProgram(s_gradientProgram);
        glDeleteShader(gvs);
        glDeleteShader(gfs);
    }

    // Radial glow shader program
    GLuint rvs = compileShader(GL_VERTEX_SHADER, s_radialVertexShader);
    GLuint rfs = compileShader(GL_FRAGMENT_SHADER, s_radialFragmentShader);
    if (rvs && rfs) {
        s_radialProgram = glCreateProgram();
        glAttachShader(s_radialProgram, rvs);
        glAttachShader(s_radialProgram, rfs);
        glLinkProgram(s_radialProgram);
        glDeleteShader(rvs);
        glDeleteShader(rfs);
    }

    // VAO/VBO
    glGenVertexArrays(1, &s_vao);
    glGenBuffers(1, &s_vbo);
    glGenBuffers(1, &s_ebo);

    s_initialized = true;
}

void UIDrawHelper::cleanup() {
    if (!s_initialized) return;
    glDeleteProgram(s_colorProgram);
    glDeleteProgram(s_textureProgram);
    glDeleteProgram(s_gradientProgram);
    glDeleteProgram(s_radialProgram);
    glDeleteBuffers(1, &s_vbo);
    glDeleteBuffers(1, &s_ebo);
    glDeleteVertexArrays(1, &s_vao);
    s_colorProgram = 0;
    s_textureProgram = 0;
    s_gradientProgram = 0;
    s_radialProgram = 0;
    s_vao = 0;
    s_vbo = 0;
    s_ebo = 0;
    s_initialized = false;
}

void UIDrawHelper::ensureInit() {
    if (!s_initialized) initialize();
}

void UIDrawHelper::drawColoredQuad(float x, float y, float w, float h,
                                   const glm::vec4& color,
                                   int screenW, int screenH) {
    ensureInit();

    glUseProgram(s_colorProgram);

    // Orthographic projection (0,0 at top-left)
    float left = 0.0f;
    float right = static_cast<float>(screenW);
    float top = 0.0f;
    float bottom = static_cast<float>(screenH);

    float projection[16] = {
        2.0f / (right - left), 0.0f, 0.0f, 0.0f,
        0.0f, -2.0f / (bottom - top), 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        -(right + left) / (right - left), (bottom + top) / (bottom - top), 0.0f, 1.0f
    };

    GLint projLoc = glGetUniformLocation(s_colorProgram, "uProjection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, projection);

    GLint colorLoc = glGetUniformLocation(s_colorProgram, "uColor");
    glUniform4f(colorLoc, color.x, color.y, color.z, color.w);

    float vertices[8] = {
        x,     y,
        x + w, y,
        x,     y + h,
        x + w, y + h
    };

    glBindVertexArray(s_vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void UIDrawHelper::drawTexturedQuad(float x, float y, float w, float h,
                                    GLuint textureId,
                                    const glm::vec4& color,
                                    int screenW, int screenH) {
    drawTexturedQuad(x, y, w, h, textureId, color, screenW, screenH, 0.0f, 0.0f, 1.0f, 1.0f);
}

void UIDrawHelper::drawTexturedQuad(float x, float y, float w, float h,
                                    GLuint textureId,
                                    const glm::vec4& color,
                                    int screenW, int screenH,
                                    float uMin, float vMin, float uMax, float vMax) {
    ensureInit();

    glUseProgram(s_textureProgram);

    float left = 0.0f;
    float right = static_cast<float>(screenW);
    float top = 0.0f;
    float bottom = static_cast<float>(screenH);

    float projection[16] = {
        2.0f / (right - left), 0.0f, 0.0f, 0.0f,
        0.0f, -2.0f / (bottom - top), 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        -(right + left) / (right - left), (bottom + top) / (bottom - top), 0.0f, 1.0f
    };

    GLint projLoc = glGetUniformLocation(s_textureProgram, "uProjection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, projection);

    GLint colorLoc = glGetUniformLocation(s_textureProgram, "uColor");
    glUniform4f(colorLoc, color.x, color.y, color.z, color.w);

    GLint texLoc = glGetUniformLocation(s_textureProgram, "uTexture");
    glUniform1i(texLoc, 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);

    float vertices[16] = {
        x,     y,     uMin, vMin,
        x + w, y,     uMax, vMin,
        x,     y + h, uMin, vMax,
        x + w, y + h, uMax, vMax
    };

    glBindVertexArray(s_vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void UIDrawHelper::drawBorder(float x, float y, float w, float h,
                              float borderWidth, const glm::vec4& color,
                              int screenW, int screenH) {
    // Top
    drawColoredQuad(x, y, w, borderWidth, color, screenW, screenH);
    // Bottom
    drawColoredQuad(x, y + h - borderWidth, w, borderWidth, color, screenW, screenH);
    // Left
    drawColoredQuad(x, y, borderWidth, h, color, screenW, screenH);
    // Right
    drawColoredQuad(x + w - borderWidth, y, borderWidth, h, color, screenW, screenH);
}

bool UIDrawHelper::isInitialized() {
    return s_initialized;
}

// ============================================================================
// High-quality effects
// ============================================================================

void UIDrawHelper::drawVerticalGradient(float x, float y, float w, float h,
                                        const glm::vec4& topColor,
                                        const glm::vec4& bottomColor,
                                        int screenW, int screenH) {
    drawHorizontalGradient(x, y, w, h, topColor, bottomColor, screenW, screenH);
}

void UIDrawHelper::drawHorizontalGradient(float x, float y, float w, float h,
                                          const glm::vec4& leftColor,
                                          const glm::vec4& rightColor,
                                          int screenW, int screenH) {
    ensureInit();
    glUseProgram(s_gradientProgram);

    float left = 0.0f;
    float right = static_cast<float>(screenW);
    float top = 0.0f;
    float bottom = static_cast<float>(screenH);

    float projection[16] = {
        2.0f / (right - left), 0.0f, 0.0f, 0.0f,
        0.0f, -2.0f / (bottom - top), 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        -(right + left) / (right - left), (bottom + top) / (bottom - top), 0.0f, 1.0f
    };

    GLint projLoc = glGetUniformLocation(s_gradientProgram, "uProjection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, projection);

    // For vertical gradient, top color = TL/TR, bottom color = BL/BR
    glUniform4f(glGetUniformLocation(s_gradientProgram, "uColorTL"), leftColor.x, leftColor.y, leftColor.z, leftColor.w);
    glUniform4f(glGetUniformLocation(s_gradientProgram, "uColorTR"), leftColor.x, leftColor.y, leftColor.z, leftColor.w);
    glUniform4f(glGetUniformLocation(s_gradientProgram, "uColorBL"), rightColor.x, rightColor.y, rightColor.z, rightColor.w);
    glUniform4f(glGetUniformLocation(s_gradientProgram, "uColorBR"), rightColor.x, rightColor.y, rightColor.z, rightColor.w);

    // 4 vertices with UV (0,0)-(1,1)
    float vertices[16] = {
        x,     y,     0.0f, 1.0f,  // TL
        x + w, y,     1.0f, 1.0f,  // TR
        x,     y + h, 0.0f, 0.0f,  // BL
        x + w, y + h, 1.0f, 0.0f   // BR
    };

    glBindVertexArray(s_vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void UIDrawHelper::drawRadialGlow(float cx, float cy, float radius,
                                  const glm::vec4& innerColor,
                                  const glm::vec4& outerColor,
                                  int screenW, int screenH) {
    ensureInit();
    glUseProgram(s_radialProgram);

    float left = 0.0f;
    float right = static_cast<float>(screenW);
    float top = 0.0f;
    float bottom = static_cast<float>(screenH);

    float projection[16] = {
        2.0f / (right - left), 0.0f, 0.0f, 0.0f,
        0.0f, -2.0f / (bottom - top), 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        -(right + left) / (right - left), (bottom + top) / (bottom - top), 0.0f, 1.0f
    };

    GLint projLoc = glGetUniformLocation(s_radialProgram, "uProjection");
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, projection);

    // Note: gl_FragCoord origin is bottom-left in OpenGL ES.
    // The shader expects screen coords with origin at bottom-left, so pass as-is.
    // Our quad y is in top-left space; convert to bottom-left for shader.
    float cxShader = cx;
    float cyShader = static_cast<float>(screenH) - cy;
    glUniform2f(glGetUniformLocation(s_radialProgram, "uCenter"), cxShader, cyShader);
    glUniform2f(glGetUniformLocation(s_radialProgram, "uRadius"), radius, radius);
    glUniform2f(glGetUniformLocation(s_radialProgram, "uScreenSize"),
                static_cast<float>(screenW), static_cast<float>(screenH));
    glUniform4f(glGetUniformLocation(s_radialProgram, "uInnerColor"),
                innerColor.x, innerColor.y, innerColor.z, innerColor.w);
    glUniform4f(glGetUniformLocation(s_radialProgram, "uOuterColor"),
                outerColor.x, outerColor.y, outerColor.z, outerColor.w);

    // Use a square that covers the glow area (2x radius)
    float r = radius * 2.0f;
    float vertices[8] = {
        cx - r, cy - r,
        cx + r, cy - r,
        cx - r, cy + r,
        cx + r, cy + r
    };

    glBindVertexArray(s_vao);
    glBindBuffer(GL_ARRAY_BUFFER, s_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glDisableVertexAttribArray(1);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void UIDrawHelper::drawGlowRect(float x, float y, float w, float h,
                                float glowSize,
                                const glm::vec4& innerColor,
                                const glm::vec4& outerColor,
                                int screenW, int screenH) {
    // Outer glow halo
    drawColoredQuad(x - glowSize, y - glowSize,
                    w + 2 * glowSize, glowSize,
                    glm::vec4(outerColor.x, outerColor.y, outerColor.z, outerColor.w * 0.6f),
                    screenW, screenH);
    drawColoredQuad(x - glowSize, y + h,
                    w + 2 * glowSize, glowSize,
                    glm::vec4(outerColor.x, outerColor.y, outerColor.z, outerColor.w * 0.6f),
                    screenW, screenH);
    drawColoredQuad(x - glowSize, y,
                    glowSize, h,
                    glm::vec4(outerColor.x, outerColor.y, outerColor.z, outerColor.w * 0.6f),
                    screenW, screenH);
    drawColoredQuad(x + w, y,
                    glowSize, h,
                    glm::vec4(outerColor.x, outerColor.y, outerColor.z, outerColor.w * 0.6f),
                    screenW, screenH);

    // Inner solid
    drawColoredQuad(x, y, w, h, innerColor, screenW, screenH);
}

void UIDrawHelper::drawSoftCircle(float cx, float cy, float radius,
                                  const glm::vec4& color,
                                  int screenW, int screenH) {
    drawRadialGlow(cx, cy, radius, color, glm::vec4(color.x, color.y, color.z, 0.0f),
                   screenW, screenH);
}

void UIDrawHelper::drawOrnateFrame(float x, float y, float w, float h,
                                   float borderWidth,
                                   const glm::vec4& outerColor,
                                   const glm::vec4& innerShadowColor,
                                   int screenW, int screenH) {
    // Inner shadow strip just inside the border
    UIDrawHelper::drawBorder(x + borderWidth, y + borderWidth,
                              w - 2 * borderWidth, h - 2 * borderWidth,
                              1.5f, innerShadowColor, screenW, screenH);
    // Outer gold border
    UIDrawHelper::drawBorder(x, y, w, h, borderWidth, outerColor, screenW, screenH);
    // Highlight (thinner, brighter inner line)
    glm::vec4 highlight(outerColor.x * 1.2f, outerColor.y * 1.2f, outerColor.z * 1.2f,
                        outerColor.w * 0.7f);
    highlight.x = std::min(highlight.x, 1.0f);
    highlight.y = std::min(highlight.y, 1.0f);
    highlight.z = std::min(highlight.z, 1.0f);
    UIDrawHelper::drawBorder(x + borderWidth * 0.5f, y + borderWidth * 0.5f,
                              w - borderWidth, h - borderWidth,
                              1.0f, highlight, screenW, screenH);
}
