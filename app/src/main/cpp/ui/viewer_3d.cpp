#include "viewer_3d.h"
#include "mesh_3d.h"
#include "text_renderer.h"
#include "ui_draw_helper.h"
#include <android/log.h>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <vector>

#undef LOG_TAG
#define LOG_TAG "Viewer3D"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

Viewer3D::Viewer3D()
    : textRenderer(nullptr),
      visible(false), initialized(false),
      screenWidth(1080), screenHeight(1920),
      currentTexture(0), textureWidth(0), textureHeight(0),
      displayMode(DisplayMode::PLANE),
      yaw(0.6f), pitch(-0.3f),
      distance(3.0f),
      minDistance(1.2f), maxDistance(8.0f),
      autoRotate(true), showWireframe(false),
      ebo(0), meshesGenerated(false) {}

Viewer3D::~Viewer3D() { cleanup(); }

bool Viewer3D::initialize(TextRenderer* tr) {
    if (initialized) return true;
    if (!tr) return false;
    textRenderer = tr;
    initialized = true;
    LOGD("Viewer3D initialized");
    return true;
}

void Viewer3D::cleanup() {
    if (ebo) {
        glDeleteBuffers(1, &ebo);
        ebo = 0;
    }
    planeVertices.clear(); planeIndices.clear();
    cubeVertices.clear();   cubeIndices.clear();
    sphereVertices.clear(); sphereIndices.clear();
    meshesGenerated = false;
    initialized = false;
}

void Viewer3D::toggle() {
    visible = !visible;
    LOGD("Viewer3D %s", visible ? "opened" : "closed");
}

void Viewer3D::setScreenSize(int w, int h) {
    screenWidth = w;
    screenHeight = h;
}

void Viewer3D::setTexture(GLuint textureId, int width, int height) {
    currentTexture = textureId;
    textureWidth = width;
    textureHeight = height;
}

void Viewer3D::cycleDisplayMode() {
    int m = static_cast<int>(displayMode);
    m = (m + 1) % 3;
    displayMode = static_cast<DisplayMode>(m);
}

void Viewer3D::setRotation(float y, float p) {
    yaw = y;
    pitch = std::clamp(p, -1.4f, 1.4f);
}

void Viewer3D::addRotation(float dy, float dp) {
    yaw += dy;
    pitch = std::clamp(pitch + dp, -1.4f, 1.4f);
}

void Viewer3D::addDistance(float d) {
    distance = std::clamp(distance + d, minDistance, maxDistance);
}

void Viewer3D::onTouchMove(float x, float y) {
    if (!touch.isActive) return;
    float dx = x - touch.lastX;
    float dy = y - touch.lastY;
    touch.lastX = x;
    touch.lastY = y;
    // Sensitivity scaled by screen size
    float sens = 3.14159f / static_cast<float>(std::max(screenWidth, screenHeight));
    addRotation(dx * sens, dy * sens);
}

void Viewer3D::onTouchUp() {
    touch.isActive = false;
}

const char* Viewer3D::displayModeName(DisplayMode mode) {
    switch (mode) {
        case DisplayMode::PLANE:  return "PLANE";
        case DisplayMode::CUBE:   return "CUBE";
        case DisplayMode::SPHERE: return "SPHERE";
        default: return "UNKNOWN";
    }
}

void Viewer3D::generateMeshes() {
    if (meshesGenerated) return;
    Mesh3D::generatePlane(planeVertices, planeIndices, 1.6f);
    Mesh3D::generateCube(cubeVertices, cubeIndices, 1.4f);
    Mesh3D::generateSphere(sphereVertices, sphereIndices, 1.0f, 16, 24);
    glGenBuffers(1, &ebo);
    meshesGenerated = true;
    LOGD("3D meshes generated (plane=%zu, cube=%zu, sphere=%zu)",
         planeVertices.size(), cubeVertices.size(), sphereVertices.size());
}

void Viewer3D::update(float deltaTime) {
    if (!visible) return;
    if (autoRotate && !touch.isActive) {
        yaw += deltaTime * 0.6f;
    }
}

void Viewer3D::render() {
    if (!visible || !textRenderer) return;
    generateMeshes();

    float s = screenWidth / 1920.0f;
    s = std::clamp(s, 0.5f, 2.0f);

    // Background
    UIDrawHelper::drawColoredQuad(0, 0, screenWidth, screenHeight,
                                   glm::vec4(0.04f, 0.04f, 0.08f, 0.95f),
                                   screenWidth, screenHeight);

    renderToolbar();
    renderViewport();
    renderInfo();
}

void Viewer3D::renderToolbar() {
    float s = screenWidth / 1920.0f;
    s = std::clamp(s, 0.5f, 2.0f);

    float toolbarH = 90.0f * s;

    // Top toolbar background
    UIDrawHelper::drawColoredQuad(0, 0, screenWidth, toolbarH,
                                   glm::vec4(0.10f, 0.10f, 0.15f, 1.0f),
                                   screenWidth, screenHeight);

    // Title
    textRenderer->renderText("3D VIEWER", 24.0f * s, 32.0f * s,
                              glm::vec3(0.4f, 0.8f, 1.0f), 0.7f * s);

    // Mode display (right side)
    std::string modeText = "Mode: ";
    modeText += displayModeName(displayMode);
    float modeW = modeText.size() * 16.0f * s;
    textRenderer->renderText(modeText.c_str(), screenWidth - modeW - 24.0f * s,
                              38.0f * s, glm::vec3(0.95f, 0.95f, 0.95f), 0.45f * s);

    // Action buttons row
    float btnY = 14.0f * s;
    float btnH = 62.0f * s;
    float btnW = 110.0f * s;
    float btnGap = 8.0f * s;
    float rightX = screenWidth - 24.0f * s;
    auto drawToolbarButton = [&](const char* label, bool active,
                                  ToolbarAction action,
                                  float xRight) {
        float x = xRight - btnW;
        glm::vec4 fill = active
            ? glm::vec4(0.25f, 0.55f, 0.80f, 1.0f)
            : glm::vec4(0.18f, 0.18f, 0.24f, 1.0f);
        UIDrawHelper::drawColoredQuad(x, btnY, btnW, btnH, fill,
                                       screenWidth, screenHeight);
        UIDrawHelper::drawBorder(x, btnY, btnW, btnH, 2.0f,
                                  glm::vec4(0.35f, 0.35f, 0.42f, 0.9f),
                                  screenWidth, screenHeight);
        float textW = std::string(label).size() * 14.0f * s;
        textRenderer->renderText(label,
                                  x + (btnW - textW) * 0.5f,
                                  btnY + btnH * 0.32f,
                                  glm::vec3(0.95f, 0.95f, 0.95f), 0.42f * s);
        (void)action;
    };

    // Close (X)
    float closeX = rightX;
    drawToolbarButton("CLOSE", false, ToolbarAction::CLOSE, closeX);
    rightX -= (btnW + btnGap);

    // Wireframe
    drawToolbarButton("WIRE", showWireframe, ToolbarAction::TOGGLE_WIRE, rightX);
    rightX -= (btnW + btnGap);

    // Auto-rotate
    drawToolbarButton("AUTO", autoRotate, ToolbarAction::TOGGLE_AUTO, rightX);
    rightX -= (btnW + btnGap);

    // Zoom out
    drawToolbarButton("-", false, ToolbarAction::ZOOM_OUT, rightX);
    rightX -= (btnW + btnGap);

    // Zoom in
    drawToolbarButton("+", false, ToolbarAction::ZOOM_IN, rightX);
    rightX -= (btnW + btnGap);

    // Mode buttons
    drawToolbarButton("SPHERE", displayMode == DisplayMode::SPHERE,
                       ToolbarAction::MODE_SPHERE, rightX);
    rightX -= (btnW + btnGap);
    drawToolbarButton("CUBE", displayMode == DisplayMode::CUBE,
                       ToolbarAction::MODE_CUBE, rightX);
    rightX -= (btnW + btnGap);
    drawToolbarButton("PLANE", displayMode == DisplayMode::PLANE,
                       ToolbarAction::MODE_PLANE, rightX);
}

Viewer3D::ToolbarAction Viewer3D::hitTestToolbar(float x, float y) const {
    float s = screenWidth / 1920.0f;
    s = std::clamp(s, 0.5f, 2.0f);

    float btnY = 14.0f * s;
    float btnH = 62.0f * s;
    float btnW = 110.0f * s;
    float btnGap = 8.0f * s;
    float rightX = screenWidth - 24.0f * s;

    if (y < btnY || y > btnY + btnH) return ToolbarAction::NONE;

    auto inButton = [&](float xRight) {
        float bx = xRight - btnW;
        return x >= bx && x <= bx + btnW;
    };

    if (inButton(rightX))                                       return ToolbarAction::CLOSE;
    rightX -= (btnW + btnGap);
    if (inButton(rightX))                                       return ToolbarAction::TOGGLE_WIRE;
    rightX -= (btnW + btnGap);
    if (inButton(rightX))                                       return ToolbarAction::TOGGLE_AUTO;
    rightX -= (btnW + btnGap);
    if (inButton(rightX))                                       return ToolbarAction::ZOOM_OUT;
    rightX -= (btnW + btnGap);
    if (inButton(rightX))                                       return ToolbarAction::ZOOM_IN;
    rightX -= (btnW + btnGap);
    if (inButton(rightX))                                       return ToolbarAction::MODE_SPHERE;
    rightX -= (btnW + btnGap);
    if (inButton(rightX))                                       return ToolbarAction::MODE_CUBE;
    rightX -= (btnW + btnGap);
    if (inButton(rightX))                                       return ToolbarAction::MODE_PLANE;
    return ToolbarAction::NONE;
}

void Viewer3D::triggerAction(ToolbarAction action) {
    switch (action) {
        case ToolbarAction::MODE_PLANE:   setDisplayMode(DisplayMode::PLANE); break;
        case ToolbarAction::MODE_CUBE:    setDisplayMode(DisplayMode::CUBE); break;
        case ToolbarAction::MODE_SPHERE:  setDisplayMode(DisplayMode::SPHERE); break;
        case ToolbarAction::TOGGLE_AUTO:  toggleAutoRotate(); break;
        case ToolbarAction::TOGGLE_WIRE:  toggleWireframe(); break;
        case ToolbarAction::ZOOM_IN:      addDistance(-0.4f); break;
        case ToolbarAction::ZOOM_OUT:     addDistance(0.4f); break;
        case ToolbarAction::CLOSE:        toggle(); break;
        default: break;
    }
}

void Viewer3D::onTouchDown(float x, float y) {
    ToolbarAction action = hitTestToolbar(x, y);
    if (action != ToolbarAction::NONE) {
        triggerAction(action);
        touch.isActive = false; // don't start camera drag
        return;
    }
    touch.isActive = true;
    touch.lastX = x;
    touch.lastY = y;
}

void Viewer3D::renderViewport() {
    float s = screenWidth / 1920.0f;
    s = std::clamp(s, 0.5f, 2.0f);

    float toolbarH = 90.0f * s;
    float infoBarH = 130.0f * s;
    float vpY = toolbarH;
    float vpH = screenHeight - toolbarH - infoBarH; // Leave space for info bar
    float vpX = 0;
    float vpW = screenWidth;

    // Subtle frame around viewport
    UIDrawHelper::drawBorder(vpX, vpY, vpW, vpH, 2.0f * s,
                              glm::vec4(0.3f, 0.3f, 0.4f, 0.8f),
                              screenWidth, screenHeight);

    UIDrawHelper::push3DViewport(static_cast<int>(vpX), static_cast<int>(vpY),
                                 static_cast<int>(vpW), static_cast<int>(vpH));

    drawCurrentMesh();

    if (showWireframe) {
        // OpenGL ES 3.0 lacks glPolygonMode, so reuse drawCurrentMeshWireframe
        // which simply renders the same mesh through the same shader pipeline.
        // The slight extra cost is acceptable for a debug overlay.
        glm::vec4 wireColor(1.0f, 0.85f, 0.2f, 0.8f);
        drawCurrentMeshWireframe(wireColor);
    }

    UIDrawHelper::popViewport();
}

void Viewer3D::drawCurrentMesh() {
    if (currentTexture == 0) return;

    float s = screenWidth / 1920.0f;
    s = std::clamp(s, 0.5f, 2.0f);
    float toolbarH = 90.0f * s;
    float infoBarH = 130.0f * s;
    float vpH = screenHeight - toolbarH - infoBarH;
    float aspect = static_cast<float>(screenWidth) / std::max(1, static_cast<int>(vpH));
    glm::mat4 proj = buildProjection(aspect);
    glm::mat4 view = buildView();
    glm::mat4 model = buildModel();
    glm::vec3 cameraPos(0.0f, 0.0f, distance);

    const std::vector<float>* verts = nullptr;
    const std::vector<unsigned short>* idx = nullptr;
    switch (displayMode) {
        case DisplayMode::PLANE:  verts = &planeVertices;  idx = &planeIndices;  break;
        case DisplayMode::CUBE:   verts = &cubeVertices;   idx = &cubeIndices;   break;
        case DisplayMode::SPHERE: verts = &sphereVertices; idx = &sphereIndices; break;
    }
    if (!verts || verts->empty()) return;

    int indexCount = static_cast<int>(idx ? idx->size() : 0);
    if (indexCount == 0) return;

    int uniqueVerts = static_cast<int>(verts->size()) / 8;
    std::vector<float> positions(uniqueVerts * 3);
    std::vector<float> normals(uniqueVerts * 3);
    std::vector<float> texCoords(uniqueVerts * 2);
    for (int i = 0; i < uniqueVerts; ++i) {
        positions[i * 3 + 0] = (*verts)[i * 8 + 0];
        positions[i * 3 + 1] = (*verts)[i * 8 + 1];
        positions[i * 3 + 2] = (*verts)[i * 8 + 2];
        normals[i * 3 + 0]   = (*verts)[i * 8 + 3];
        normals[i * 3 + 1]   = (*verts)[i * 8 + 4];
        normals[i * 3 + 2]   = (*verts)[i * 8 + 5];
        texCoords[i * 2 + 0] = (*verts)[i * 8 + 6];
        texCoords[i * 2 + 1] = (*verts)[i * 8 + 7];
    }

    UIDrawHelper::drawTexturedQuad3D(currentTexture, model, view, proj,
                                      glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
                                      uniqueVerts,
                                      positions.data(),
                                      normals.data(),
                                      texCoords.data(),
                                      idx->data(),
                                      indexCount,
                                      cameraPos);
}

void Viewer3D::drawCurrentMeshWireframe(const glm::vec4& color) {
    if (currentTexture == 0) return;

    float s = screenWidth / 1920.0f;
    s = std::clamp(s, 0.5f, 2.0f);
    float toolbarH = 90.0f * s;
    float infoBarH = 130.0f * s;
    float vpH = screenHeight - toolbarH - infoBarH;
    float aspect = static_cast<float>(screenWidth) / std::max(1, static_cast<int>(vpH));
    glm::mat4 proj = buildProjection(aspect);
    glm::mat4 view = buildView();
    glm::mat4 model = buildModel();
    glm::vec3 cameraPos(0.0f, 0.0f, distance);

    const std::vector<float>* verts = nullptr;
    const std::vector<unsigned short>* idx = nullptr;
    switch (displayMode) {
        case DisplayMode::PLANE:  verts = &planeVertices;  idx = &planeIndices;  break;
        case DisplayMode::CUBE:   verts = &cubeVertices;   idx = &cubeIndices;   break;
        case DisplayMode::SPHERE: verts = &sphereVertices; idx = &sphereIndices; break;
    }
    if (!verts || verts->empty()) return;

    int indexCount = static_cast<int>(idx->size());
    int uniqueVerts = static_cast<int>(verts->size()) / 8;
    std::vector<float> positions(uniqueVerts * 3);
    std::vector<float> normals(uniqueVerts * 3);
    std::vector<float> texCoords(uniqueVerts * 2);
    for (int i = 0; i < uniqueVerts; ++i) {
        positions[i * 3 + 0] = (*verts)[i * 8 + 0];
        positions[i * 3 + 1] = (*verts)[i * 8 + 1];
        positions[i * 3 + 2] = (*verts)[i * 8 + 2];
        // Constant neutral normal so wireframe overlay reads consistently.
        normals[i * 3 + 0]   = 0.5f;
        normals[i * 3 + 1]   = 0.7f;
        normals[i * 3 + 2]   = 0.5f;
        texCoords[i * 2 + 0] = (*verts)[i * 8 + 6];
        texCoords[i * 2 + 1] = (*verts)[i * 8 + 7];
    }

    UIDrawHelper::drawTexturedQuad3D(currentTexture, model, view, proj, color,
                                      uniqueVerts,
                                      positions.data(),
                                      normals.data(),
                                      texCoords.data(),
                                      idx->data(),
                                      indexCount,
                                      cameraPos);
}

void Viewer3D::renderInfo() {
    float s = screenWidth / 1920.0f;
    s = std::clamp(s, 0.5f, 2.0f);

    float infoBarH = 130.0f * s;
    float infoY = screenHeight - infoBarH;
    UIDrawHelper::drawColoredQuad(0, infoY, screenWidth, infoBarH,
                                   glm::vec4(0.08f, 0.08f, 0.12f, 1.0f),
                                   screenWidth, screenHeight);

    // Top accent line
    UIDrawHelper::drawColoredQuad(0, infoY, screenWidth, 2.0f,
                                   glm::vec4(0.4f, 0.8f, 1.0f, 0.5f),
                                   screenWidth, screenHeight);

    // Hints row
    textRenderer->renderText("Drag: Rotate | Long-press mode buttons to change",
                              24.0f * s, infoY + 100.0f * s,
                              glm::vec3(0.7f, 0.7f, 0.8f), 0.34f * s);

    // Status row
    std::stringstream ss;
    ss << "Auto-rotate: " << (autoRotate ? "ON" : "OFF")
       << "  Wireframe: " << (showWireframe ? "ON" : "OFF")
       << "  Distance: " << std::fixed; ss.precision(1); ss << distance;
    textRenderer->renderText(ss.str().c_str(), 24.0f * s, infoY + 60.0f * s,
                              glm::vec3(0.95f, 0.95f, 0.95f), 0.40f * s);

    // Texture info
    std::string texInfo;
    if (currentTexture != 0) {
        std::stringstream ts;
        ts << "Texture: " << textureWidth << "x" << textureHeight
           << " (id=" << currentTexture << ")";
        texInfo = ts.str();
    } else {
        texInfo = "Texture: (none - open Texture Viewer to pick one)";
    }
    textRenderer->renderText(texInfo.c_str(), 24.0f * s, infoY + 20.0f * s,
                              glm::vec3(0.45f, 0.85f, 0.5f), 0.36f * s);
}

glm::mat4 Viewer3D::buildProjection(float aspect) {
    float fov = 50.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;
    return glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
}

glm::mat4 Viewer3D::buildView() {
    // Camera positioned at distance along -Z, looking at origin
    glm::vec3 cameraPos(0.0f, 0.0f, distance);
    glm::vec3 target(0.0f, 0.0f, 0.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    return glm::lookAt(cameraPos, target, up);
}

glm::mat4 Viewer3D::buildModel() {
    glm::mat4 model;  // Default constructor produces identity.
    model = glm::rotate(model, yaw, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, pitch, glm::vec3(1.0f, 0.0f, 0.0f));
    return model;
}
