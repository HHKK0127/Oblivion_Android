#pragma once

#include <cstdint>
#include <GLES3/gl3.h>
#include <string>

class RetroFilter {
public:
    struct Settings {
        bool enabled = false;

        bool scanlines_enabled = true;
        float scanlines_intensity = 0.5f;

        bool pixelation_enabled = false;
        float pixelation_scale = 8.0f;

        bool color_reduction_enabled = false;
        int color_bits = 6;

        bool crt_distortion_enabled = false;
        float distortion_strength = 0.1f;

        bool grain_enabled = false;
        float grain_strength = 0.3f;
    };

    RetroFilter();
    ~RetroFilter();

    bool initialize(uint32_t width, uint32_t height);
    void cleanup();

    void bindSceneFramebuffer();
    void apply(const Settings& settings, float time);
    void renderToScreen();

    void resize(uint32_t width, uint32_t height);

private:
    uint32_t screenWidth = 0;
    uint32_t screenHeight = 0;

    GLuint sceneFBO = 0;
    GLuint sceneTex = 0;
    GLuint workFBO[2] = {0, 0};
    GLuint workTex[2] = {0, 0};

    GLuint quadVAO = 0;
    GLuint quadVBO = 0;

    // Shader programs
    GLuint progCopy = 0;
    GLuint progScanline = 0;
    GLuint progPixelate = 0;
    GLuint progColorReduce = 0;
    GLuint progCRT = 0;
    GLuint progGrain = 0;

    bool createFBO(GLuint& fbo, GLuint& tex, uint32_t w, uint32_t h);
    void deleteFBO(GLuint& fbo, GLuint& tex);
    void createQuad();
    void deleteQuad();

    GLuint compileShader(const std::string& vertSrc, const std::string& fragSrc);
    void deleteProgram(GLuint& prog);

    void drawFullscreenQuad(GLuint inputTex);

    static const std::string vertexShaderSource;
    static const std::string copyFragmentSource;
    static const std::string scanlineFragmentSource;
    static const std::string pixelateFragmentSource;
    static const std::string colorReduceFragmentSource;
    static const std::string crtFragmentSource;
    static const std::string grainFragmentSource;
};
