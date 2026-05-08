#include "retro_filter.h"
#include <glm/gtc/type_ptr.hpp>

/**
 * RetroFilter Implementation
 * Manages post-processing pipeline for retro CRT monitor effects
 */

// ========== Constructor/Destructor ==========

RetroFilter::RetroFilter() {
    LOGI("RetroFilter constructor called");
    startTime = std::chrono::steady_clock::now();
}

RetroFilter::~RetroFilter() {
    LOGI("RetroFilter destructor called");
    cleanup();
}

// ========== Initialization ==========

bool RetroFilter::initialize(unsigned int width, unsigned int height) {
    LOGI("===== RetroFilter::initialize(%u, %u) =====", width, height);

    screenWidth = width;
    screenHeight = height;

    // Start at full resolution
    computeSceneResolution(FULL_RES);
    lastScale = FULL_RES;

    LOGI("Screen resolution: %ux%u", screenWidth, screenHeight);
    LOGI("Scene resolution: %ux%u (initial: FULL_RES)", sceneWidth, sceneHeight);

    try {
        // Create framebuffer objects
        LOGI("Creating framebuffers...");
        createFramebuffers();

        // Create shader programs
        LOGI("Creating shaders...");
        createShaders();

        // Create fullscreen quad mesh
        LOGI("Creating quad mesh...");
        createQuadMesh();

        LOGI("===== RetroFilter initialized successfully =====");
        return true;
    }
    catch (const std::exception& e) {
        LOGE("CRITICAL: Exception during RetroFilter::initialize(): %s", e.what());
        return false;
    }
    catch (...) {
        LOGE("CRITICAL: Unknown exception during RetroFilter::initialize()");
        return false;
    }
}

// ========== Framebuffer Management ==========

void RetroFilter::createFramebuffers() {
    LOGI("Creating framebuffers (scene: %ux%u)", sceneWidth, sceneHeight);

    // ===== Scene Framebuffer =====
    glGenFramebuffers(1, &sceneFBO);
    glGenTextures(1, &sceneTex);

    // Configure scene texture
    // OPTIMIZATION: Use RGB565 instead of RGBA8 to save 50% VRAM (10.8 MB -> 5.4 MB)
    glBindTexture(GL_TEXTURE_2D, sceneTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB565, sceneWidth, sceneHeight, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Attach texture to scene FBO
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, sceneTex, 0);

    // Check FBO completeness
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        LOGE("Scene FBO incomplete! Status: 0x%x", status);
        throw std::runtime_error("Scene framebuffer is not complete");
    }
    LOGI("Scene FBO created successfully (ID: %u)", sceneFBO);

    // ===== Work Framebuffer 1 =====
    glGenFramebuffers(1, &workFBO1);
    glGenTextures(1, &workTex1);

    glBindTexture(GL_TEXTURE_2D, workTex1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB565, sceneWidth, sceneHeight, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, workFBO1);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, workTex1, 0);

    status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        LOGE("Work FBO1 incomplete! Status: 0x%x", status);
        throw std::runtime_error("Work framebuffer 1 is not complete");
    }
    LOGI("Work FBO1 created successfully (ID: %u)", workFBO1);

    // ===== Work Framebuffer 2 =====
    glGenFramebuffers(1, &workFBO2);
    glGenTextures(1, &workTex2);

    glBindTexture(GL_TEXTURE_2D, workTex2);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB565, sceneWidth, sceneHeight, 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, workFBO2);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, workTex2, 0);

    status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        LOGE("Work FBO2 incomplete! Status: 0x%x", status);
        throw std::runtime_error("Work framebuffer 2 is not complete");
    }
    LOGI("Work FBO2 created successfully (ID: %u)", workFBO2);

    // Reset to screen framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    checkGLError("createFramebuffers");
}

// ========== Mesh Creation ==========

void RetroFilter::createQuadMesh() {
    LOGI("Creating fullscreen quad mesh");

    // Fullscreen quad: 2 triangles covering -1 to 1 in NDC space
    // Vertices: position (x, y), texCoord (u, v)
    float quadVertices[] = {
        // Position     TexCoord
        -1.0f,  1.0f,   0.0f, 1.0f,  // Top-left
        -1.0f, -1.0f,   0.0f, 0.0f,  // Bottom-left
         1.0f,  1.0f,   1.0f, 1.0f,  // Top-right
         1.0f, -1.0f,   1.0f, 0.0f   // Bottom-right
    };

    // Create VAO and VBO
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);

    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    // Position attribute (location 0)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // TexCoord attribute (location 1)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    LOGI("Quad mesh created (VAO: %u, VBO: %u)", quadVAO, quadVBO);
    checkGLError("createQuadMesh");
}

// ========== Shader Creation ==========

void RetroFilter::createShaders() {
    LOGI("Creating retro filter shaders");

    // Shared vertex shader source for all effects
    const char* vertexShaderSource = R"(
        #version 300 es
        precision highp float;

        in vec2 position;
        in vec2 texCoord;

        out vec2 vTexCoord;

        void main() {
            gl_Position = vec4(position, 0.0, 1.0);
            vTexCoord = texCoord;
        }
    )";

    // ===== Scanline Shader =====
    const char* scanlineFragSource = R"(
        #version 300 es
        precision highp float;

        in vec2 vTexCoord;
        out vec4 FragColor;

        uniform sampler2D inputTexture;
        uniform float intensity;
        uniform vec2 sceneResolution;

        void main() {
            vec4 color = texture(inputTexture, vTexCoord);

            // Create horizontal scanlines
            float scanlineFreq = sceneResolution.y * 2.0;
            float scanline = sin(vTexCoord.y * scanlineFreq * 3.14159) * 0.5 + 0.5;
            scanline = mix(1.0, scanline, intensity);

            FragColor = color * scanline;
        }
    )";

    scanlineShader = new ShaderProgram();
    if (!scanlineShader->compile(vertexShaderSource, scanlineFragSource)) {
        LOGE("Failed to compile scanline shader");
        throw std::runtime_error("Scanline shader compilation failed");
    }
    LOGI("Scanline shader created");

    // ===== Pixelation Shader =====
    const char* pixelationFragSource = R"(
        #version 300 es
        precision highp float;

        in vec2 vTexCoord;
        out vec4 FragColor;

        uniform sampler2D inputTexture;
        uniform float pixelScale;
        uniform vec2 sceneResolution;

        void main() {
            // Block-based downsampling
            vec2 pixelSize = vec2(pixelScale) / sceneResolution;
            vec2 pixelCoord = floor(vTexCoord / pixelSize) * pixelSize;
            pixelCoord = clamp(pixelCoord, 0.0, 1.0);

            FragColor = texture(inputTexture, pixelCoord);
        }
    )";

    pixelationShader = new ShaderProgram();
    if (!pixelationShader->compile(vertexShaderSource, pixelationFragSource)) {
        LOGE("Failed to compile pixelation shader");
        throw std::runtime_error("Pixelation shader compilation failed");
    }
    LOGI("Pixelation shader created");

    // ===== Color Reduction Shader =====
    const char* colorReductionFragSource = R"(
        #version 300 es
        precision highp float;

        in vec2 vTexCoord;
        out vec4 FragColor;

        uniform sampler2D inputTexture;
        uniform float colorLevels;  // OPTIMIZATION: Pre-calculated pow(2.0, colorBits)

        void main() {
            vec4 color = texture(inputTexture, vTexCoord);

            // Quantize RGB to specified bit depth
            // colorLevels is pre-calculated on CPU to avoid 921,600 pow() calls per frame
            color.rgb = floor(color.rgb * colorLevels) / colorLevels;

            FragColor = color;
        }
    )";

    colorReductionShader = new ShaderProgram();
    if (!colorReductionShader->compile(vertexShaderSource, colorReductionFragSource)) {
        LOGE("Failed to compile color reduction shader");
        throw std::runtime_error("Color reduction shader compilation failed");
    }
    LOGI("Color reduction shader created");

    // ===== CRT Distortion Shader =====
    const char* distortionFragSource = R"(
        #version 300 es
        precision highp float;

        in vec2 vTexCoord;
        out vec4 FragColor;

        uniform sampler2D inputTexture;
        uniform float strength;
        uniform vec2 sceneResolution;

        void main() {
            // Barrel distortion
            vec2 center = vec2(0.5);
            vec2 delta = vTexCoord - center;
            float r2 = dot(delta, delta);

            vec2 distorted = center + delta * (1.0 + strength * r2);

            if (distorted.x < 0.0 || distorted.x > 1.0 ||
                distorted.y < 0.0 || distorted.y > 1.0) {
                FragColor = vec4(0.0);  // Black border
            } else {
                FragColor = texture(inputTexture, distorted);
            }
        }
    )";

    distortionShader = new ShaderProgram();
    if (!distortionShader->compile(vertexShaderSource, distortionFragSource)) {
        LOGE("Failed to compile distortion shader");
        throw std::runtime_error("Distortion shader compilation failed");
    }
    LOGI("Distortion shader created");

    // ===== Film Grain Shader =====
    const char* grainFragSource = R"(
        #version 300 es
        precision mediump float;  // OPTIMIZATION: Reduced from highp (20-30% faster on Mali/PowerVR)

        in vec2 vTexCoord;
        out vec4 FragColor;

        uniform sampler2D inputTexture;
        uniform float strength;
        uniform float time;

        float random(vec2 co, float t) {
            return fract(sin(dot(co + t, vec2(12.9898, 78.233))) * 43758.5453);
        }

        void main() {
            vec4 color = texture(inputTexture, vTexCoord);

            // Time-varying noise
            float noise = random(vTexCoord, time) - 0.5;
            color.rgb += noise * strength;

            FragColor = clamp(color, 0.0, 1.0);
        }
    )";

    grainShader = new ShaderProgram();
    if (!grainShader->compile(vertexShaderSource, grainFragSource)) {
        LOGE("Failed to compile grain shader");
        throw std::runtime_error("Grain shader compilation failed");
    }
    LOGI("Grain shader created");

    // ===== Composite Shader (for upscaling) =====
    const char* compositeFragSource = R"(
        #version 300 es
        precision highp float;

        in vec2 vTexCoord;
        out vec4 FragColor;

        uniform sampler2D inputTexture;
        uniform vec2 sceneResolution;
        uniform vec2 screenResolution;

        void main() {
            // Nearest-neighbor upscaling for pixel-perfect look
            vec2 scaledCoord = vTexCoord * (sceneResolution / screenResolution);
            scaledCoord = clamp(scaledCoord, 0.0, 1.0);

            FragColor = texture(inputTexture, scaledCoord);
        }
    )";

    compositeShader = new ShaderProgram();
    if (!compositeShader->compile(vertexShaderSource, compositeFragSource)) {
        LOGE("Failed to compile composite shader");
        throw std::runtime_error("Composite shader compilation failed");
    }
    LOGI("Composite shader created");

    LOGI("All shaders created successfully");
    checkGLError("createShaders");
}

// ========== Framebuffer Binding ==========

void RetroFilter::bindSceneFramebuffer() {
    glBindFramebuffer(GL_FRAMEBUFFER, sceneFBO);
}

// ========== Effect Pipeline ==========

void RetroFilter::apply(const Settings& settings) {
    // OPTIMIZATION: Removed per-frame LOGD call
    // LOGD("Applying retro filter effects");

    // Update resolution if changed
    if (settings.resolution_scale != lastScale) {
        LOGI("Resolution scale changed: %d → %d", (int)lastScale, (int)settings.resolution_scale);
        computeSceneResolution(settings.resolution_scale);
        lastScale = settings.resolution_scale;
        sceneResolutionChanged = true;  // Mark that resolution changed
    }

    // OPTIMIZATION: Pre-compute sceneResolution once (instead of per-effect)
    if (sceneResolutionChanged) {
        cachedSceneResolution = glm::vec2(sceneWidth, sceneHeight);
        sceneResolutionChanged = false;
    }

    // OPTIMIZATION: Set viewport once for entire pipeline (instead of per-effect)
    glViewport(0, 0, sceneWidth, sceneHeight);

    // Start with scene texture
    GLuint currentTex = sceneTex;
    GLuint currentFBO = 0;
    int fboToggle = 0;  // 0 = FBO1, 1 = FBO2

    // Helper to switch between work FBOs
    auto switchFBO = [&]() {
        fboToggle = 1 - fboToggle;
        currentFBO = (fboToggle == 0) ? workFBO1 : workFBO2;
        currentTex = (fboToggle == 0) ? workTex1 : workTex2;
    };

    // ===== Effect Chain (order matters!) =====

    // 1. Pixelation (first - affects downstream effects)
    if (settings.pixelation_enabled) {
        // OPTIMIZATION: Removed per-effect LOGD call
        // OPTIMIZATION: Removed redundant glViewport (set once at pipeline start)
        switchFBO();
        glBindFramebuffer(GL_FRAMEBUFFER, currentFBO);

        pixelationShader->use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, currentTex == workTex1 ? sceneTex :
                                     (currentTex == workTex2 ? workTex1 : workTex2));
        pixelationShader->setUniform("inputTexture", 0);
        pixelationShader->setUniform("pixelScale", settings.pixelation_scale);
        pixelationShader->setUniform("sceneResolution", cachedSceneResolution);

        renderScreenQuad(currentTex);
    }

    // 2. Color Reduction
    if (settings.color_reduction_enabled) {
        // OPTIMIZATION: Removed per-effect LOGD call
        // OPTIMIZATION: Pre-calculate pow(2.0, colorBits) on CPU instead of in shader
        if (settings.color_bits != lastColorBits) {
            cachedColorLevels = std::pow(2.0f, (float)settings.color_bits);
            lastColorBits = settings.color_bits;
        }

        // OPTIMIZATION: Removed redundant glViewport (set once at pipeline start)
        switchFBO();
        glBindFramebuffer(GL_FRAMEBUFFER, currentFBO);

        colorReductionShader->use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, currentTex == workTex1 ? workTex2 :
                                     (currentTex == workTex2 ? workTex1 : sceneTex));
        colorReductionShader->setUniform("inputTexture", 0);
        colorReductionShader->setUniform("colorLevels", cachedColorLevels);

        renderScreenQuad(currentTex);
    }

    // 3. CRT Distortion
    if (settings.crt_distortion_enabled) {
        // OPTIMIZATION: Removed per-effect LOGD call
        // OPTIMIZATION: Removed redundant glViewport (set once at pipeline start)
        switchFBO();
        glBindFramebuffer(GL_FRAMEBUFFER, currentFBO);

        distortionShader->use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, currentTex == workTex1 ? workTex2 :
                                     (currentTex == workTex2 ? workTex1 : sceneTex));
        distortionShader->setUniform("inputTexture", 0);
        distortionShader->setUniform("strength", settings.distortion_strength);
        distortionShader->setUniform("sceneResolution", cachedSceneResolution);

        renderScreenQuad(currentTex);
    }

    // 4. Scanlines
    if (settings.scanlines_enabled) {
        // OPTIMIZATION: Removed per-effect LOGD call
        // OPTIMIZATION: Removed redundant glViewport (set once at pipeline start)
        switchFBO();
        glBindFramebuffer(GL_FRAMEBUFFER, currentFBO);

        scanlineShader->use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, currentTex == workTex1 ? workTex2 :
                                     (currentTex == workTex2 ? workTex1 : sceneTex));
        scanlineShader->setUniform("inputTexture", 0);
        scanlineShader->setUniform("intensity", settings.scanlines_intensity);
        scanlineShader->setUniform("sceneResolution", cachedSceneResolution);

        renderScreenQuad(currentTex);
    }

    // 5. Film Grain (always last)
    if (settings.grain_enabled) {
        // OPTIMIZATION: Removed per-effect LOGD call
        // OPTIMIZATION: Implement grain time caching - update only every N frames
        framesSinceLastUpdate++;
        if (framesSinceLastUpdate >= GRAIN_TIME_UPDATE_INTERVAL) {
            cachedGrainTime = std::chrono::duration<float>(
                std::chrono::steady_clock::now() - startTime
            ).count();
            framesSinceLastUpdate = 0;
        }

        // OPTIMIZATION: Removed redundant glViewport (set once at pipeline start)
        switchFBO();
        glBindFramebuffer(GL_FRAMEBUFFER, currentFBO);

        grainShader->use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, currentTex == workTex1 ? workTex2 :
                                     (currentTex == workTex2 ? workTex1 : sceneTex));
        grainShader->setUniform("inputTexture", 0);
        grainShader->setUniform("strength", settings.grain_strength);
        grainShader->setUniform("time", cachedGrainTime);

        renderScreenQuad(currentTex);
    }

    // Store final result texture
    finalResultTex = currentTex;

    checkGLError("apply");
}

// ========== Screen Rendering ==========

void RetroFilter::renderToScreen() {
    // Bind default framebuffer (screen)
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, screenWidth, screenHeight);

    // If scene was at lower resolution, use composite shader for upscaling
    if (sceneWidth != screenWidth || sceneHeight != screenHeight) {
        LOGD("Upscaling from %ux%u to %ux%u", sceneWidth, sceneHeight, screenWidth, screenHeight);
        compositeShader->use();
        compositeShader->setUniform("sceneResolution", glm::vec2(sceneWidth, sceneHeight));
        compositeShader->setUniform("screenResolution", glm::vec2(screenWidth, screenHeight));
    } else {
        // Direct rendering to screen
        if (scanlineShader) scanlineShader->use();  // Use any shader
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, finalResultTex);

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    checkGLError("renderToScreen");
}

// ========== Rendering Utilities ==========

void RetroFilter::renderScreenQuad(GLuint inputTexture) {
    // Use currently bound shader program
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTexture);

    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

// ========== Utilities ==========

void RetroFilter::computeSceneResolution(ResolutionScale scale) {
    switch (scale) {
        case FULL_RES:
            sceneWidth = screenWidth;
            sceneHeight = screenHeight;
            LOGI("Resolution: FULL (%ux%u)", sceneWidth, sceneHeight);
            break;
        case HALF_RES:
            sceneWidth = screenWidth / 2;
            sceneHeight = screenHeight / 2;
            LOGI("Resolution: HALF (%ux%u)", sceneWidth, sceneHeight);
            break;
        case QUARTER_RES:
            sceneWidth = screenWidth / 4;
            sceneHeight = screenHeight / 4;
            LOGI("Resolution: QUARTER (%ux%u)", sceneWidth, sceneHeight);
            break;
    }
}

void RetroFilter::setNativeResolution(unsigned int width, unsigned int height) {
    if (screenWidth != width || screenHeight != height) {
        LOGI("setNativeResolution: %ux%u → %ux%u", screenWidth, screenHeight, width, height);
        screenWidth = width;
        screenHeight = height;
        // Note: Framebuffers would need to be recreated here in full implementation
    }
}

void RetroFilter::checkGLError(const char* operation) {
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        const char* errStr = "UNKNOWN";
        switch (err) {
            case GL_INVALID_ENUM: errStr = "INVALID_ENUM"; break;
            case GL_INVALID_VALUE: errStr = "INVALID_VALUE"; break;
            case GL_INVALID_OPERATION: errStr = "INVALID_OPERATION"; break;
            case GL_OUT_OF_MEMORY: errStr = "OUT_OF_MEMORY"; break;
        }
        LOGE("GL Error in %s: %s (0x%x)", operation, errStr, err);
    }
}

// ========== Cleanup ==========

void RetroFilter::cleanup() {
    LOGI("RetroFilter::cleanup() - cleaning up GPU resources");

    // Delete framebuffers
    if (sceneFBO) glDeleteFramebuffers(1, &sceneFBO);
    if (workFBO1) glDeleteFramebuffers(1, &workFBO1);
    if (workFBO2) glDeleteFramebuffers(1, &workFBO2);

    // Delete textures
    if (sceneTex) glDeleteTextures(1, &sceneTex);
    if (workTex1) glDeleteTextures(1, &workTex1);
    if (workTex2) glDeleteTextures(1, &workTex2);

    // Delete quad mesh
    if (quadVAO) glDeleteVertexArrays(1, &quadVAO);
    if (quadVBO) glDeleteBuffers(1, &quadVBO);

    // Delete shaders
    if (scanlineShader) delete scanlineShader;
    if (pixelationShader) delete pixelationShader;
    if (colorReductionShader) delete colorReductionShader;
    if (distortionShader) delete distortionShader;
    if (grainShader) delete grainShader;
    if (compositeShader) delete compositeShader;

    LOGI("RetroFilter cleanup complete");
}
