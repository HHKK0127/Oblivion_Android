#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <GLES3/gl3.h>

/**
 * @brief 3D Debug Drawing System (inspired by GTA V grcDebugDraw)
 *
 * Provides immediate-mode 3D debug primitives for visualizing:
 * - NPC paths and patrol routes
 * - AI perception ranges (sight, hearing)
 * - Collision volumes and bounding boxes
 * - Navigation mesh edges
 * - Combat ranges and spell radii
 *
 * All primitives support a framesToLive parameter: negative values persist
 * until cleared, 0 draws for one frame only, positive values count down.
 *
 * GLES 3.0 compatible — no geometry shaders or compute shaders.
 */
class DebugDraw {
public:
    DebugDraw();
    ~DebugDraw();

    bool initialize();
    void shutdown();

    // --- 3D Primitives (world space) ---

    /** Draw a line between two points */
    void line(const glm::vec3& from, const glm::vec3& to,
              const glm::vec4& color, int framesToLive = 1);

    /** Draw a line with gradient color */
    void line(const glm::vec3& from, const glm::vec3& to,
              const glm::vec4& colorFrom, const glm::vec4& colorTo,
              int framesToLive = 1);

    /** Draw a crosshair marker at a position */
    void cross(const glm::vec3& pos, float size,
               const glm::vec4& color, int framesToLive = 1);

    /** Draw an axis-aligned bounding box */
    void boxAABB(const glm::vec3& min, const glm::vec3& max,
                 const glm::vec4& color, int framesToLive = 1);

    /** Draw an oriented bounding box */
    void boxOBB(const glm::vec3& min, const glm::vec3& max,
                const glm::mat4& transform, const glm::vec4& color,
                int framesToLive = 1);

    /** Draw a sphere (wireframe) */
    void sphere(const glm::vec3& center, float radius,
                const glm::vec4& color, int framesToLive = 1,
                int segments = 12);

    /** Draw a circle in 3D space (wireframe) */
    void circle(const glm::vec3& center, float radius,
                const glm::vec3& normal,
                const glm::vec4& color, int framesToLive = 1,
                int segments = 24);

    /** Draw a cylinder between two points */
    void cylinder(const glm::vec3& start, const glm::vec3& end, float radius,
                  const glm::vec4& color, int framesToLive = 1,
                  int segments = 12);

    /** Draw a cone (partial sphere) for vision/attack range visualization */
    void cone(const glm::vec3& origin, const glm::vec3& direction,
              float angleDeg, float length,
              const glm::vec4& color, int framesToLive = 1,
              int segments = 16);

    /** Draw a grid on the XZ plane */
    void grid(const glm::vec3& center, float size, int divisions,
              const glm::vec4& color, int framesToLive = 1);

    /** Draw a 3D arrow from a point in a direction */
    void arrow(const glm::vec3& from, const glm::vec3& to,
               const glm::vec4& color, int framesToLive = 1);

    // --- 2D Primitives (screen space, 0..1 normalized) ---

    /** Draw a 2D line in normalized screen coordinates */
    void line2D(const glm::vec2& from, const glm::vec2& to,
                const glm::vec4& color, int framesToLive = 1);

    /** Draw a 2D rectangle */
    void rect2D(const glm::vec2& min, const glm::vec2& max,
                const glm::vec4& color, bool filled = false,
                int framesToLive = 1);

    // --- Lifecycle ---

    /**
     * Call once per frame, after all primitives have been submitted.
     * Draws the active primitives and then ages them, so a primitive is always
     * visible in the frame it was submitted in.
     */
    void render(const glm::mat4& viewMatrix, const glm::mat4& projMatrix,
                int screenWidth, int screenHeight);

    /** Clear all primitives immediately */
    void clear();

    /** Enable/disable the entire debug draw system */
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }

private:
    struct DebugLine {
        glm::vec3 from;
        glm::vec3 to;
        glm::vec4 colorFrom;
        glm::vec4 colorTo;
        int framesToLive;
    };

    struct DebugLine2D {
        glm::vec2 from;
        glm::vec2 to;
        glm::vec4 color;
        int framesToLive;
    };

    struct DebugQuad2D {
        glm::vec2 min;
        glm::vec2 max;
        glm::vec4 color;
        bool filled;
        int framesToLive;
    };

    bool enabled_;
    bool initialized_;

    // Primitive storage
    std::vector<DebugLine> lines3D_;
    std::vector<DebugLine2D> lines2D_;
    std::vector<DebugQuad2D> quads2D_;

    // GPU resources
    GLuint shaderProgram_;
    GLuint vao_;
    GLuint vbo_;
    GLuint shader2D_;
    GLuint vao2D_;
    GLuint vbo2D_;

    // Shader uniform locations (3D)
    GLint locMVP_;
    GLint locColorFrom_;
    GLint locColorTo_;

    // Shader uniform locations (2D)
    GLint loc2DProj_;

    // Reusable scratch buffer for building vertex data
    std::vector<float> vertexBuffer_;

    bool compileShaders();
    void rebuild3DBuffer();
    void rebuild2DBuffer();
    void agePrimitives();

    // Helper: build sphere wireframe lines
    void buildSphereLines(const glm::vec3& center, float radius,
                          const glm::vec4& color, int framesToLive, int segments);
    // Helper: build circle lines
    void buildCircleLines(const glm::vec3& center, float radius,
                          const glm::vec3& normal,
                          const glm::vec4& color, int framesToLive, int segments);
    // Helper: build cylinder lines
    void buildCylinderLines(const glm::vec3& start, const glm::vec3& end,
                            float radius, const glm::vec4& color,
                            int framesToLive, int segments);
    // Helper: build cone lines
    void buildConeLines(const glm::vec3& origin, const glm::vec3& direction,
                        float angleDeg, float length,
                        const glm::vec4& color, int framesToLive, int segments);
    // Helper: build grid lines
    void buildGridLines(const glm::vec3& center, float size, int divisions,
                        const glm::vec4& color, int framesToLive);
    // Helper: build arrow lines
    void buildArrowLines(const glm::vec3& from, const glm::vec3& to,
                         const glm::vec4& color, int framesToLive);
};