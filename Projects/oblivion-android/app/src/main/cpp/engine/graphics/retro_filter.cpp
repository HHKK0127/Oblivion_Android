#include "retro_filter.h"
#include <android/log.h>
#include <cmath>

#define LOG_TAG "RetroFilter"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Shared vertex shader for all fullscreen passes
const std::string RetroFilter::vertexShaderSource = R"(#version 300 es
precision highp float;
in vec2 aPos;
out vec2 vTexCoord;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    vTexCoord = aPos * 0.5 + 0.5;
})";

// Simple texture copy
const std::string RetroFilter::copyFragmentSource = R"(#version 300 es
precision highp float;
in vec2 vTexCoord;
out vec4 FragColor;
uniform sampler2D uTexture;
void main() {
    FragColor = texture(uTexture, vTexCoord);
})";

// Scanlines
const std::string RetroFilter::scanlineFragmentSource = R"(#version 300 es
precision highp float;
in vec2 vTexCoord;
out vec4 FragColor;
uniform sampler2D uTexture;
uniform float uIntensity;
uniform vec2 uResolution;
void main() {
    vec4 color = texture(uTexture, vTexCoord);
    float scanline = sin(vTexCoord.y * uResolution.y * 3.14159) * 0.5 + 0.5;
    scanline = mix(1.0, scanline, uIntensity);
    FragColor = vec4(color.rgb * scanline, color.a);
})";

// Pixelation
const std::string RetroFilter::pixelateFragmentSource = R"(#version 300 es
precision highp float;
in vec2 vTexCoord;
out vec4 FragColor;
uniform sampler2D uTexture;
uniform float uScale;
uniform vec2 uResolution;
void main() {
    vec2 pixelSize = vec2(uScale) / uResolution;
    vec2 pixelCoord = floor(vTexCoord / pixelSize) * pixelSize + pixelSize * 0.5;
    FragColor = texture(uTexture, pixelCoord);
})";

// Color reduction
const std::string RetroFilter::colorReduceFragmentSource = R"(#version 300 es
precision highp float;
in vec2 vTexCoord;
out vec4 FragColor;
uniform sampler2D uTexture;
uniform float uBits;
void main() {
    vec4 color = texture(uTexture, vTexCoord);
    float levels = pow(2.0, uBits);
    color.rgb = floor(color.rgb * levels) / levels;
    FragColor = color;
})";

// CRT barrel distortion
const std::string RetroFilter::crtFragmentSource = R"(#version 300 es
precision highp float;
in vec2 vTexCoord;
out vec4 FragColor;
uniform sampler2D uTexture;
uniform float uStrength;
void main() {
    vec2 center = vec2(0.5);
    vec2 delta = vTexCoord - center;
    float r2 = dot(delta, delta);
    vec2 distorted = center + delta * (1.0 + uStrength * r2);
    if (distorted.x < 0.0 || distorted.x > 1.0 ||
        distorted.y < 0.0 || distorted.y > 1.0) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    } else {
        FragColor = texture(uTexture, distorted);
    }
})";

// Film grain
const std::string RetroFilter::grainFragmentSource = R"(#version 300 es
precision highp float;
in vec2 vTexCoord;
out vec4 FragColor;
uniform sampler2D uTexture;
uniform float uStrength;
uniform float uTime;
float random(vec2 co) {
    return fract(sin(dot(co, vec2(12.9898, 78.233))) * 43758.5453);
}
void main() {
    vec4 color = texture(uTexture, vTexCoord);
    float noise = random(vTexCoord + uTime) - 0.5;
    color.rgb += noise * uStrength;
    FragColor = clamp(color, 0.0, 1.0);
})";

RetroFilter::RetroFilter() {}

RetroFilter::~RetroFilter() {
    cleanup();
}

bool RetroFilter::initialize(uint32_t width, uint32_t height) {
    screenWidth = width;
    screenHeight = height;

    // Create FBOs
    if (!createFBO(sceneFBO, sceneTex, width, height)) {
        LOGE("Failed to create scene FBO");
        return false;
    }
    if (!createFBO(workFBO[0], workTex[0], width, height)) {
        LOGE("Failed to create work FBO 0");
        return false;
    }
    if (!createFBO(workFBO[1], workTex[1], width, height)) {
        LOGE("Failed to create work FBO 1");
        return false;
    }

    // Create quad
    createQuad();

    // Compile shaders
    progCopy = compileShader(vertexShaderSource, copyFragmentSource);
    progScanline = compileShader(vertexShaderSource, scanlineFragmentSource);
    progPixelate = compileShader(vertexShaderSource, pixelateFragmentSource);
    progColorReduce = compileShader(vertexShaderSource, colorReduceFragmentSource);
    progCRT = compileShader(vertexShaderSource, crtFragmentSource);
    progGrain = compileShader(vertexShaderSource, grainFragmentSource);

    if (!progCopy || !progScanline || !progPixelate || !progColorReduce || !progCRT || !progGrain) {
        LOGE("Failed to compile one or more shaders");
        cleanup();
        return false;
    }

    LOGI("RetroFilter initialized: %dx%d", width, height);
    return true;
}

void RetroFilter::cleanup() {
    deleteProgram(progCopy);
    deleteProgram(progScanline);
    deleteProgram(progPixelate);
    deleteProgram(progColorReduce);
    deleteProgram(progCRT);
    deleteProgram(progGrain);

    deleteQuad();

    deleteFBO(sceneFBO, sceneTex);
    deleteFBO(workFBO[0], workTex[0]);
    deleteFBO(workFBO[1], workTex[1]);

    screenWidth = 0;
    screenHeight = 0;
}

bool RetroFilter::createFBO(GLuint& fbo, GLuint& tex, uint32_t w, uint32_t h) {
    glGenFramebuffers(1, &fbo);
    glGenTextures(1, &tex);

    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        LOGE("Framebuffer incomplete");
        return false;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

void RetroFilter::deleteFBO(GLuint& fbo, GLuint& tex) {
    if (fbo) {
        glDeleteFramebuffers(1, &fbo);
        fbo = 0;
    }
    if (tex) {
        glDeleteTextures(1, &tex);
        tex = 0;
    }
}

void RetroFilter::createQuad() {
    float vertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f,
         1.0f,  1.0f,
    };

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);

    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glBindVertexArray(0);
}

void RetroFilter::deleteQuad() {
    if (quadVBO) {
        glDeleteBuffers(1, &quadVBO);
        quadVBO = 0;
    }
    if (quadVAO) {
        glDeleteVertexArrays(1, &quadVAO);
        quadVAO = 0;
    }
}

GLuint RetroFilter::compileShader(const std::string& vertSrc, const std::string& fragSrc) {
    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    const char* vertCStr = vertSrc.c_str();
    glShaderSource(vert, 1, &vertCStr, nullptr);
    glCompileShader(vert);

    GLint success;
    glGetShaderiv(vert, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(vert, 512, nullptr, log);
        LOGE("Vertex shader compilation failed: %s", log);
        glDeleteShader(vert);
        return 0;
    }

    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fragCStr = fragSrc.c_str();
    glShaderSource(frag, 1, &fragCStr, nullptr);
    glCompileShader(frag);

    glGetShaderiv(frag, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(frag, 512, nullptr, log);
        LOGE("Fragment shader compilation failed: %s", log);
        glDeleteShader(vert);
        glDeleteShader(frag);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vert);
    glAttachShader(program, frag);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(program, 512, nullptr, log);
        LOGE("Shader program linking failed: %s", log);
        glDeleteProgram(program);
        glDeleteShader(vert);
        glDeleteShader(frag);
        return 0;
    }

    glDeleteShader(vert);
    glDeleteShader(frag);
    return program;
}

void RetroFilter::deleteProgram(GLuint& prog) {
    if (prog) {
        glDeleteProgram(prog);
        prog = 0;
    }
}

void RetroFilter::bindSceneFramebuffer() {
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
    glViewport(0, 0, screenWidth, screenHeight);
}

void RetroFilter::drawFullscreenQuad(GLuint inputTex) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTex);
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void RetroFilter::apply(const Settings& settings, float time) {
    if (!settings.enabled) {
        return;
    }

    GLuint currentTex = sceneTex;
    int pingPong = 0;

    auto applyPass = [&](GLuint program, auto&& setUniforms) {
        glBindFramebuffer(GL_FRAMEBUFFER, workFBO[pingPong]);
        glUseProgram(program);
        glUniform1i(glGetUniformLocation(program, "uTexture"), 0);
        setUniforms(program);
        drawFullscreenQuad(currentTex);
        currentTex = workTex[pingPong];
        pingPong = 1 - pingPong;
    };

    // Pixelation
    if (settings.pixelation_enabled) {
        applyPass(progPixelate, [&](GLuint prog) {
            glUniform1f(glGetUniformLocation(prog, "uScale"), settings.pixelation_scale);
            glUniform2f(glGetUniformLocation(prog, "uResolution"), static_cast<float>(screenWidth), static_cast<float>(screenHeight));
        });
    }

    // Color reduction
    if (settings.color_reduction_enabled) {
        applyPass(progColorReduce, [&](GLuint prog) {
            glUniform1f(glGetUniformLocation(prog, "uBits"), static_cast<float>(settings.color_bits));
        });
    }

    // CRT distortion
    if (settings.crt_distortion_enabled) {
        applyPass(progCRT, [&](GLuint prog) {
            glUniform1f(glGetUniformLocation(prog, "uStrength"), settings.distortion_strength);
        });
    }

    // Scanlines
    if (settings.scanlines_enabled) {
        applyPass(progScanline, [&](GLuint prog) {
            glUniform1f(glGetUniformLocation(prog, "uIntensity"), settings.scanlines_intensity);
            glUniform2f(glGetUniformLocation(prog, "uResolution"), static_cast<float>(screenWidth), static_cast<float>(screenHeight));
        });
    }

    // Grain
    if (settings.grain_enabled) {
        applyPass(progGrain, [&](GLuint prog) {
            glUniform1f(glGetUniformLocation(prog, "uStrength"), settings.grain_strength);
            glUniform1f(glGetUniformLocation(prog, "uTime"), time);
        });
    }

    // Store final result in workTex[pingPong ^ 1] (last written)
    // currentTex now holds the final result
    sceneTex = currentTex;
}

void RetroFilter::renderToScreen() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenWidth, screenHeight);
    glUseProgram(progCopy);
    glUniform1i(glGetUniformLocation(progCopy, "uTexture"), 0);
    drawFullscreenQuad(sceneTex);
}

void RetroFilter::resize(uint32_t width, uint32_t height) {
    cleanup();
    initialize(width, height);
}
