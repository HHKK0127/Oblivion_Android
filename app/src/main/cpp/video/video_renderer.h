#pragma once

// ============================================================================
// VideoRenderer - renders decoded video frames to screen via OpenGL ES
// Phase 53: Bink Video replacement
// ============================================================================

#include <cstdint>
#include <mutex>

// Forward declarations for OpenGL ES
typedef unsigned int GLuint;
typedef int GLint;
typedef int GLsizei;

namespace oblivion {
namespace video {

// ============================================================================
// VideoRenderer - fullscreen video frame rendering
// ============================================================================

class VideoRenderer {
public:
    VideoRenderer();
    ~VideoRenderer();

    // Initialize OpenGL resources (shader, VBO, texture)
    bool initialize();

    // Release OpenGL resources
    void shutdown();

    // Set the texture ID from SurfaceTexture (updated by MediaCodec)
    void setExternalTextureId(GLuint textureId);

    // Update the texture transform matrix from SurfaceTexture
    void setTransformMatrix(const float matrix[16]);

    // Render the current video frame fullscreen
    void renderFullscreen();

    // Render the video frame within a specific viewport
    void renderInViewport(int x, int y, int width, int height);

    // Set video dimensions for aspect ratio calculation
    void setVideoDimensions(int width, int height);

    // Set letterbox/pillarbox color (RGBA)
    void setLetterboxColor(float r, float g, float b, float a);

    // Check if renderer is initialized
    bool isInitialized() const { return initialized_; }

    // Get current texture ID
    GLuint getTextureId() const { return textureId_; }

private:
    // Create and compile the video rendering shader
    bool createShader();

    // Create fullscreen quad geometry
    bool createGeometry();

    // Apply letterbox scaling
    void calculateLetterboxMatrix(float screenAspect, float videoAspect,
                                  float* outMatrix) const;

    bool initialized_ = false;

    // OpenGL resources
    GLuint shaderProgram_ = 0;
    GLuint vbo_ = 0;
    GLuint textureId_ = 0;

    // Shader uniform locations
    GLint uniformTexture_ = -1;
    GLint uniformTransform_ = -1;
    GLint uniformMVP_ = -1;

    // Shader attribute locations
    GLint attribPosition_ = -1;
    GLint attribTexCoord_ = -1;

    // Video dimensions
    int videoWidth_ = 0;
    int videoHeight_ = 0;

    // Letterbox color
    float letterboxColor_[4] = {0.0f, 0.0f, 0.0f, 1.0f};

    // SurfaceTexture transform matrix
    float transformMatrix_[16];

    mutable std::mutex mutex_;
};

} // namespace video
} // namespace oblivion
