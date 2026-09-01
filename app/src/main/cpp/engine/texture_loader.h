#pragma once

#include <GLES3/gl3.h>
#include <string>
#include <unordered_map>
#include <utility>
#include <mutex>

/**
 * @brief PNG texture loader
 * Load PNG files as OpenGL textures using stb_image.h
 */
class TextureLoader {
public:
    /**
     * @brief Load PNG file and generate OpenGL texture
     * @param filename File path within assets (e.g. "textures/ui/main_background.png")
     * @return Texture ID (0 = failure)
     */
    static GLuint loadTextureFromAsset(const std::string& filename);

    /**
     * @brief Delete texture
     * @param textureId Texture ID to delete
     */
    static void deleteTexture(GLuint textureId);

    /**
     * @brief Get texture size
     * @param textureId Texture ID
     * @param width Output width
     * @param height Output height
     * @return true on success
     */
    static bool getTextureSize(GLuint textureId, int& width, int& height);

private:
    static std::unordered_map<GLuint, std::pair<int, int>> s_textureSizes;
    static std::mutex s_textureMutex;
};
