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
     * @param x Top-left X coordinate
     * @param y Top-left Y coordinate
     * @param w Width
     * @param h Height
     * @param color RGBA color
     * @param screenW Screen width
     * @param screenH Screen height
     */
    static void drawColoredQuad(float x, float y, float w, float h,
                                const glm::vec4& color,
                                int screenW, int screenH);

    /**
     * @brief Draw textured quad
     * @param textureId Texture ID
     * @param color Multiply color (usually white)
     */
    static void drawTexturedQuad(float x, float y, float w, float h,
                                 GLuint textureId,
                                 const glm::vec4& color,
                                 int screenW, int screenH);

    /**
     * @brief Draw textured quad (custom UV)
     * @param uMin,vMin,uMax,vMax UV coordinate range
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
     * @brief Is initialized
     */
    static bool isInitialized();

private:
    static GLuint s_colorProgram;
    static GLuint s_textureProgram;
    static GLuint s_vao;
    static GLuint s_vbo;
    static bool s_initialized;

    static GLuint compileShader(GLenum type, const char* source);
    static void ensureInit();
};
