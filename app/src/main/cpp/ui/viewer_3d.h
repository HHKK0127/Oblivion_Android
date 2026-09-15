#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <string>
#include <vector>
#include <GLES3/gl3.h>

class TextRenderer;
class TextureViewer;

/**
 * @brief 3D Viewer - Interactive 3D preview of textures and meshes.
 *
 * Features:
 * - Three preview modes: Plane, Cube, Sphere
 * - Rotate with finger drag (yaw/pitch)
 * - Pinch zoom (handled by host via setDistance)
 * - Auto-rotation when not interacting
 * - Wireframe overlay toggle
 */
class Viewer3D {
public:
    enum class DisplayMode {
        PLANE = 0,
        CUBE  = 1,
        SPHERE = 2
    };

    Viewer3D();
    ~Viewer3D();

    bool initialize(TextRenderer* textRenderer);
    void cleanup();

    void toggle();
    bool isVisible() const { return visible; }
    void setVisible(bool v) { visible = v; }

    void update(float deltaTime);
    void render();

    void setScreenSize(int w, int h);

    // Texture to preview
    void setTexture(GLuint textureId, int width, int height);
    GLuint getTexture() const { return currentTexture; }

    // Display mode
    void setDisplayMode(DisplayMode mode) { displayMode = mode; }
    DisplayMode getDisplayMode() const { return displayMode; }
    void cycleDisplayMode();

    // Camera control
    void setRotation(float yaw, float pitch);
    void addRotation(float deltaYaw, float deltaPitch);
    void setDistance(float d) { distance = d; }
    void addDistance(float delta);

    // Wireframe
    void setWireframe(bool w) { showWireframe = w; }
    bool isWireframe() const { return showWireframe; }
    void toggleWireframe() { showWireframe = !showWireframe; }

    // Auto-rotate
    void setAutoRotate(bool r) { autoRotate = r; }
    bool isAutoRotate() const { return autoRotate; }
    void toggleAutoRotate() { autoRotate = !autoRotate; }

    // Drag state for touch input
    void onTouchDown(float x, float y);
    void onTouchMove(float x, float y);
    void onTouchUp();

    static const char* displayModeName(DisplayMode mode);

    enum class ToolbarAction {
        NONE = 0,
        MODE_PLANE,
        MODE_CUBE,
        MODE_SPHERE,
        TOGGLE_AUTO,
        TOGGLE_WIRE,
        CLOSE,
        ZOOM_IN,
        ZOOM_OUT
    };

    ToolbarAction hitTestToolbar(float x, float y) const;
    void triggerAction(ToolbarAction action);

private:
    TextRenderer* textRenderer;
    bool visible;
    bool initialized;

    int screenWidth;
    int screenHeight;

    GLuint currentTexture;
    int textureWidth;
    int textureHeight;

    DisplayMode displayMode;
    float yaw;          // Rotation around Y axis
    float pitch;        // Rotation around X axis (clamped)
    float distance;     // Camera distance from object
    float minDistance;
    float maxDistance;

    bool autoRotate;
    bool showWireframe;

    // Touch tracking
    struct TouchState {
        bool isActive = false;
        float lastX = 0.0f;
        float lastY = 0.0f;
    } touch;

    // Pre-baked mesh data
    std::vector<float> planeVertices;
    std::vector<unsigned short> planeIndices;
    std::vector<float> cubeVertices;
    std::vector<unsigned short> cubeIndices;
    std::vector<float> sphereVertices;
    std::vector<unsigned short> sphereIndices;

    // EBO
    GLuint ebo;

    bool meshesGenerated;

    void generateMeshes();
    void renderToolbar();
    void renderViewport();
    void drawCurrentMesh();
    void drawCurrentMeshWireframe(const glm::vec4& color);
    void renderInfo();

    glm::mat4 buildProjection(float aspect);
    glm::mat4 buildView();
    glm::mat4 buildModel();
};
