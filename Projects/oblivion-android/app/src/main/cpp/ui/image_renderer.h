#pragma once

#include <string>
#include <GLES3/gl3.h>
#include <android/asset_manager.h>

class ImageRenderer {
public:
    ImageRenderer();
    ~ImageRenderer();

    // Load image from file path (using stb_image)
    bool loadFromFile(const std::string& path);
    // Load image from memory buffer
    bool loadFromMemory(const unsigned char* data, int len);
    // Load image from Android assets
    bool loadFromAssetManager(AAssetManager* mgr, const std::string& assetPath);

    // Draw image at specified screen position and size
    void draw(float x, float y, float width, float height);
    // Draw fullscreen
    void drawFullscreen();

    void cleanup();

    bool isLoaded() const { return textureId != 0; }
    int getWidth() const { return imgWidth; }
    int getHeight() const { return imgHeight; }

private:
    GLuint textureId;
    GLuint shaderProgram;
    GLuint vao;
    GLuint vbo;
    int imgWidth;
    int imgHeight;
    bool initialized;
    unsigned int screenWidth;
    unsigned int screenHeight;

    bool compileShaders();
    void uploadTexture(const unsigned char* data, int width, int height);
};
