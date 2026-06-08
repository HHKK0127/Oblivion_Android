#pragma once

#include <GLES3/gl3.h>
#include <string>
#include <unordered_map>
#include <utility>

/**
 * @brief PNGテクスチャローダー
 * stb_image.h を使用してPNGファイルをOpenGLテクスチャに読み込む
 */
class TextureLoader {
public:
    /**
     * @brief PNGファイルを読み込んでOpenGLテクスチャを生成
     * @param filename assets内のファイルパス（例: "textures/ui/main_background.png"）
     * @return テクスチャID（0 = 失敗）
     */
    static GLuint loadTextureFromAsset(const std::string& filename);

    /**
     * @brief テクスチャを削除
     * @param textureId 削除するテクスチャID
     */
    static void deleteTexture(GLuint textureId);

    /**
     * @brief テクスチャサイズを取得
     * @param textureId テクスチャID
     * @param width 出力幅
     * @param height 出力高さ
     * @return 成功時true
     */
    static bool getTextureSize(GLuint textureId, int& width, int& height);

private:
    static std::unordered_map<GLuint, std::pair<int, int>> s_textureSizes;
};
