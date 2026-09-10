#pragma once

#include <GLES3/gl3.h>
#include <glm/glm.hpp>

/**
 * @brief UI drawing helper functions
 *
 * Phase 9: OpenGL ES 3.0 drawing helpers shared among UI components.
 * Provides shader programs, VAO/VBO, and solid/textured quad drawing.
 */
class UIDrawHelper {
public:
    /**
     * @brief Initialize shader and VAO/VBO (only on first call)
     */
    static void initialize();

    /**
     * @brief Release resources
     */
    static void cleanup();

    /**
     * @brief Draw solid color quad
     */
    static void drawColoredQuad(float x, float y, float w, float h,
                                const glm::vec4& color,
                                int screenW, int screenH);

    /**
     * @brief Draw textured quad
     */
    static void drawTexturedQuad(float x, float y, float w, float h,
                                 GLuint textureId,
                                 const glm::vec4& color,
                                 int screenW, int screenH);

    /**
     * @brief Draw textured quad (custom UV)
     */
    static void drawTexturedQuad(float x, float y, float w, float h,
                                 GLuint textureId,
                                 const glm::vec4& color,
                                 int screenW, int screenH,
                                 float uMin, float vMin, float uMax, float vMax);

    /**
     * @brief Draw border (4-edge quad)
     */
    static void drawBorder(float x, float y, float w, float h,
                           float borderWidth, const glm::vec4& color,
                           int screenW, int screenH);

    /**
     * @brief Draw vertical gradient quad (top color -> bottom color)
     */
    static void drawVerticalGradient(float x, float y, float w, float h,
                                     const glm::vec4& topColor,
                                     const glm::vec4& bottomColor,
                                     int screenW, int screenH);

    /**
     * @brief Draw horizontal gradient quad (left color -> right color)
     */
    static void drawHorizontalGradient(float x, float y, float w, float h,
                                       const glm::vec4& leftColor,
                                       const glm::vec4& rightColor,
                                       int screenW, int screenH);

    /**
     * @brief Draw radial glow (centered circle with smooth falloff)
     */
    static void drawRadialGlow(float cx, float cy, float radius,
                               const glm::vec4& innerColor,
                               const glm::vec4& outerColor,
                               int screenW, int screenH);

    /**
     * @brief Draw outer-glow rect (rectangle with soft glow on edges)
     */
    static void drawGlowRect(float x, float y, float w, float h,
                             float glowSize,
                             const glm::vec4& innerColor,
                             const glm::vec4& outerColor,
                             int screenW, int screenH);

    /**
     * @brief Draw soft-edged circle (center+radius with falloff)
     */
    static void drawSoftCircle(float cx, float cy, float radius,
                               const glm::vec4& color,
                               int screenW, int screenH);

    /**
     * @brief Draw ornate frame: outer gold + inner inset shadow.
     * Provides an Oblivion-style ornamental border around an area.
     */
    static void drawOrnateFrame(float x, float y, float w, float h,
                                float borderWidth,
                                const glm::vec4& outerColor,
                                const glm::vec4& innerShadowColor,
                                int screenW, int screenH);

    /**
     * @brief Is initialized
     */
    static bool isInitialized();

private:
    static GLuint s_colorProgram;
    static GLuint s_textureProgram;
    static GLuint s_gradientProgram;
    static GLuint s_radialProgram;
    static GLuint s_vao;
    static GLuint s_vbo;
    static GLuint s_ebo;
    static bool s_initialized;

    static GLuint compileShader(GLenum type, const char* source);
    static void ensureInit();
};

