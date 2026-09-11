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
     * @brief Draw textured quad with 3D model-view transform (rotation + zoom).
     * Vertices must already be uploaded. Used by 3D viewer.
     *
     * @param textureId   Texture to bind
     * @param model       Model matrix (object/world transform)
     * @param view        View matrix (camera transform)
     * @param projection  Perspective projection matrix
     * @param tintColor   Multiplicative tint
     * @param vertexCount Number of vertices to draw
     * @param positions   Flat array of XYZ positions (size = vertexCount * 3)
     * @param normals     Flat array of XYZ normals (size = vertexCount * 3)
     * @param texCoords   Flat array of UV coordinates (size = vertexCount * 2)
     * @param indices     Flat array of indices (size = indexCount). If null,
     *                    the mesh is drawn as a non-indexed triangle list.
     * @param indexCount  Number of indices. If 0, drawArrays is used.
     */
    static void drawTexturedQuad3D(GLuint textureId,
                                    const glm::mat4& model,
                                    const glm::mat4& view,
                                    const glm::mat4& projection,
                                    const glm::vec4& tintColor,
                                    int vertexCount,
                                    const float* positions,
                                    const float* normals,
                                    const float* texCoords,
                                    const unsigned short* indices = nullptr,
                                    int indexCount = 0,
                                    const glm::vec3& cameraPos = glm::vec3(0.0f, 0.0f, 0.0f));

    /**
     * @brief Set up 3D viewport with depth testing.
     * Call before drawTexturedQuad3D, then restoreViewport after.
     */
    static void push3DViewport(int x, int y, int w, int h);

    /**
     * @brief Restore 2D viewport (no depth test).
     */
    static void popViewport();

    /**
     * @brief Is initialized
     */
    static bool isInitialized();

private:
    static GLuint s_colorProgram;
    static GLuint s_textureProgram;
    static GLuint s_3dTextureProgram;
    static GLuint s_vao;
    static GLuint s_vbo;
    static GLuint s_3dVbo;
    static GLuint s_ebo;
    static GLint  s_savedViewport[4];
    static bool   s_viewportSaved;
    static bool s_initialized;

    static GLuint compileShader(GLenum type, const char* source);
    static void ensureInit();
};
