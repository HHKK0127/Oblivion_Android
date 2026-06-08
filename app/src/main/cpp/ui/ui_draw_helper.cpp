#include "ui_draw_helper.h"
#include <android/log.h>

GLuint UIDrawHelper::s_colorProgram = 0;
GLuint UIDrawHelper::s_textureProgram = 0;
GLuint UIDrawHelper::s_vao = 0;
GLuint UIDrawHelper::s_vbo = 0;
bool UIDrawHelper::s_initialized = false;

static const char* s_uiVertexShader = R"(
    #version 300 es
    precision mediump float;
    layout(location = 0) in vec2 aPos;
    uniform mat4 uProjection;
    void main() {
        gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
    }
)";

static const char* s_uiFragmentShader = R"(
    #version 300 es
    precision mediump float;
    uniform vec4 uColor;
    out vec4 fragColor;
    void main() {
        fragColor = uColor;
    }
)";

static const char* s_uiTextureVertexShader = R"(
    #version 300 es
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

static const char* s_uiTextureFragmentShader = R"(
    #version 300 es
    precision mediump float;
    uniform vec4 uColor;
    uniform sampler2D uTexture;
    in vec2 vTexCoord;
    out vec4 fragColor;
    void main() {
        fragColor = texture(uTexture, vTexCoord) * uColor;
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

    // VAO/VBO
    glGenVertexArrays(1, &s_vao);
    glGenBuffers(1, &s_vbo);

    s_initialized = true;
}

void UIDrawHelper::cleanup() {
    if (!s_initialized) return;
    glDeleteProgram(s_colorProgram);
    glDeleteProgram(s_textureProgram);
    glDeleteBuffers(1, &s_vbo);
    glDeleteVertexArrays(1, &s_vao);
    s_colorProgram = 0;
    s_textureProgram = 0;
    s_vao = 0;
    s_vbo = 0;
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
