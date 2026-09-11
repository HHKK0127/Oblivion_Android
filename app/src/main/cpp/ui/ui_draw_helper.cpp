#include "ui_draw_helper.h"
#include <android/log.h>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

GLuint UIDrawHelper::s_colorProgram = 0;
GLuint UIDrawHelper::s_textureProgram = 0;
GLuint UIDrawHelper::s_3dTextureProgram = 0;
GLuint UIDrawHelper::s_vao = 0;
GLuint UIDrawHelper::s_vbo = 0;
GLuint UIDrawHelper::s_3dVbo = 0;
GLuint UIDrawHelper::s_ebo = 0;
GLint  UIDrawHelper::s_savedViewport[4] = {0, 0, 0, 0};
bool   UIDrawHelper::s_viewportSaved = false;
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

// 3D textured quad shader: applies model-view-projection and lighting
static const char* s_3dTextureVertexShader = R"(#version 300 es
precision mediump float;
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;
out vec3 vNormal;
out vec3 vWorldPos;
out vec2 vTexCoord;
void main() {
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vWorldPos = worldPos.xyz;
    vNormal = normalize(uNormalMatrix * aNormal);
    vTexCoord = aTexCoord;
    gl_Position = uProjection * uView * worldPos;
}
)";

static const char* s_3dTextureFragmentShader = R"(#version 300 es
precision mediump float;
in vec3 vNormal;
in vec3 vWorldPos;
in vec2 vTexCoord;
uniform vec4 uTintColor;
uniform vec3 uLightDir;
uniform vec3 uAmbientColor;
uniform vec3 uLightColor;
uniform vec3 uCameraPos;
uniform vec3 uSpecularColor;
uniform float uSpecularPower;
uniform float uRimStrength;
uniform vec3 uFogColor;
uniform float uFogNear;
uniform float uFogFar;
uniform sampler2D uTexture;
out vec4 fragColor;
void main() {
    vec3 normal = normalize(vNormal);
    vec3 lightDir = normalize(uLightDir);
    vec3 viewDir = normalize(uCameraPos - vWorldPos);
    vec3 halfDir = normalize(lightDir + viewDir);

    // Diffuse (Lambert)
    float diffuse = max(dot(normal, lightDir), 0.0);

    // Specular (Blinn-Phong)
    float specular = pow(max(dot(normal, halfDir), 0.0), uSpecularPower);

    // Rim light (Fresnel-like edge highlight)
    float rim = pow(1.0 - max(dot(normal, viewDir), 0.0), 2.0);
    vec3 rimLight = uSpecularColor * rim * uRimStrength;

    vec3 lighting = uAmbientColor + uLightColor * diffuse;
    vec3 texColor = texture(uTexture, vTexCoord).rgb;
    if (texture(uTexture, vTexCoord).a < 0.05) discard;

    vec3 result = texColor * lighting
                + uSpecularColor * specular * 0.6
                + rimLight;
    result *= uTintColor.rgb;

    // Fog (linear)
    float dist = length(uCameraPos - vWorldPos);
    float fogFactor = clamp((dist - uFogNear) / (uFogFar - uFogNear), 0.0, 1.0);
    result = mix(result, uFogColor, fogFactor);

    fragColor = vec4(result, uTintColor.a);
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

    // 3D textured shader program (lit)
    GLuint tvs3 = compileShader(GL_VERTEX_SHADER, s_3dTextureVertexShader);
    GLuint tfs3 = compileShader(GL_FRAGMENT_SHADER, s_3dTextureFragmentShader);
    if (tvs3 && tfs3) {
        s_3dTextureProgram = glCreateProgram();
        glAttachShader(s_3dTextureProgram, tvs3);
        glAttachShader(s_3dTextureProgram, tfs3);
        glLinkProgram(s_3dTextureProgram);
        glDeleteShader(tvs3);
        glDeleteShader(tfs3);
    }

    // VAO/VBO
    glGenVertexArrays(1, &s_vao);
    glGenBuffers(1, &s_vbo);
    glGenBuffers(1, &s_3dVbo);
    glGenBuffers(1, &s_ebo);

    s_initialized = true;
}

void UIDrawHelper::cleanup() {
    if (!s_initialized) return;
    glDeleteProgram(s_colorProgram);
    glDeleteProgram(s_textureProgram);
    glDeleteProgram(s_3dTextureProgram);
    glDeleteBuffers(1, &s_vbo);
    glDeleteBuffers(1, &s_3dVbo);
    glDeleteBuffers(1, &s_ebo);
    glDeleteVertexArrays(1, &s_vao);
    s_colorProgram = 0;
    s_textureProgram = 0;
    s_3dTextureProgram = 0;
    s_vao = 0;
    s_vbo = 0;
    s_3dVbo = 0;
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

// ==================== 3D Drawing ====================

void UIDrawHelper::push3DViewport(int x, int y, int w, int h) {
    ensureInit();
    // Save current state
    glGetIntegerv(GL_VIEWPORT, s_savedViewport);
    s_viewportSaved = true;

    glViewport(x, y, w, h);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glClear(GL_DEPTH_BUFFER_BIT);
}

void UIDrawHelper::popViewport() {
    if (s_viewportSaved) {
        glViewport(s_savedViewport[0], s_savedViewport[1],
                   s_savedViewport[2], s_savedViewport[3]);
        s_viewportSaved = false;
    }
    glDisable(GL_DEPTH_TEST);
}

void UIDrawHelper::drawTexturedQuad3D(GLuint textureId,
                                       const glm::mat4& model,
                                       const glm::mat4& view,
                                       const glm::mat4& projection,
                                       const glm::vec4& tintColor,
                                       int vertexCount,
                                       const float* positions,
                                       const float* normals,
                                       const float* texCoords,
                                       const unsigned short* indices,
                                       int indexCount,
                                       const glm::vec3& cameraPos) {
    ensureInit();
    if (!s_3dTextureProgram || vertexCount <= 0 || !positions || !normals || !texCoords) return;

    glUseProgram(s_3dTextureProgram);

    GLint modelLoc = glGetUniformLocation(s_3dTextureProgram, "uModel");
    GLint viewLoc = glGetUniformLocation(s_3dTextureProgram, "uView");
    GLint projLoc = glGetUniformLocation(s_3dTextureProgram, "uProjection");
    GLint lightDirLoc = glGetUniformLocation(s_3dTextureProgram, "uLightDir");
    GLint ambientLoc = glGetUniformLocation(s_3dTextureProgram, "uAmbientColor");
    GLint lightColorLoc = glGetUniformLocation(s_3dTextureProgram, "uLightColor");
    GLint tintLoc = glGetUniformLocation(s_3dTextureProgram, "uTintColor");
    GLint normalMatLoc = glGetUniformLocation(s_3dTextureProgram, "uNormalMatrix");
    GLint texLoc = glGetUniformLocation(s_3dTextureProgram, "uTexture");
    GLint cameraLoc = glGetUniformLocation(s_3dTextureProgram, "uCameraPos");
    GLint specularColorLoc = glGetUniformLocation(s_3dTextureProgram, "uSpecularColor");
    GLint specularPowerLoc = glGetUniformLocation(s_3dTextureProgram, "uSpecularPower");
    GLint rimStrengthLoc = glGetUniformLocation(s_3dTextureProgram, "uRimStrength");
    GLint fogColorLoc = glGetUniformLocation(s_3dTextureProgram, "uFogColor");
    GLint fogNearLoc = glGetUniformLocation(s_3dTextureProgram, "uFogNear");
    GLint fogFarLoc = glGetUniformLocation(s_3dTextureProgram, "uFogFar");

    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);

    // Normal matrix: transpose of the upper-left 3x3 of model. The custom
    // glm in this project does not expose glm::mat3, so we pass the 9
    // floats directly. For pure rotations the transpose is the inverse.
    float normalMat[9] = {
        model[0][0], model[0][1], model[0][2],
        model[1][0], model[1][1], model[1][2],
        model[2][0], model[2][1], model[2][2]
    };
    // Transpose in place so lighting uses the correct normal basis.
    float t;
    t = normalMat[1]; normalMat[1] = normalMat[3]; normalMat[3] = t;
    t = normalMat[2]; normalMat[2] = normalMat[6]; normalMat[6] = t;
    t = normalMat[5]; normalMat[5] = normalMat[7]; normalMat[7] = t;
    glUniformMatrix3fv(normalMatLoc, 1, GL_FALSE, normalMat);

    // Lighting + fog defaults tuned for the DebugMenu viewer
    glUniform3f(lightDirLoc, 0.4f, 0.7f, 0.5f);
    glUniform3f(ambientLoc, 0.30f, 0.30f, 0.36f);
    glUniform3f(lightColorLoc, 0.95f, 0.92f, 0.85f);
    glUniform4f(tintLoc, tintColor.x, tintColor.y, tintColor.z, tintColor.w);
    glUniform3f(cameraLoc, cameraPos.x, cameraPos.y, cameraPos.z);
    glUniform3f(specularColorLoc, 1.0f, 0.95f, 0.85f);
    glUniform1f(specularPowerLoc, 32.0f);
    glUniform1f(rimStrengthLoc, 0.5f);
    glUniform3f(fogColorLoc, 0.04f, 0.04f, 0.08f);
    glUniform1f(fogNearLoc, 4.0f);
    glUniform1f(fogFarLoc, 12.0f);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glUniform1i(texLoc, 0);

    std::vector<float> interleaved(vertexCount * 8);
    for (int i = 0; i < vertexCount; ++i) {
        interleaved[i * 8 + 0] = positions[i * 3 + 0];
        interleaved[i * 8 + 1] = positions[i * 3 + 1];
        interleaved[i * 8 + 2] = positions[i * 3 + 2];
        interleaved[i * 8 + 3] = normals[i * 3 + 0];
        interleaved[i * 8 + 4] = normals[i * 3 + 1];
        interleaved[i * 8 + 5] = normals[i * 3 + 2];
        interleaved[i * 8 + 6] = texCoords[i * 2 + 0];
        interleaved[i * 8 + 7] = texCoords[i * 2 + 1];
    }

    glBindBuffer(GL_ARRAY_BUFFER, s_3dVbo);
    glBufferData(GL_ARRAY_BUFFER, interleaved.size() * sizeof(float),
                 interleaved.data(), GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

    if (indices && indexCount > 0) {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, s_ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(unsigned short),
                     indices, GL_DYNAMIC_DRAW);
        glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_SHORT, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    } else {
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    }

    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisableVertexAttribArray(2);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}
