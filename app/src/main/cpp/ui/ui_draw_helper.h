#pragma once

#include <GLES3/gl3.h>
#include <glm/glm.hpp>

/**
 * @brief UI描画用ヘルパー関数群
 *
 * Phase 9: UIコンポーネント間で共有するOpenGL ES 3.0描画ヘルパー。
 * シェーダープログラム、VAO/VBO、単色/テクスチャ付きquad描画を提供。
 */
class UIDrawHelper {
public:
    /**
     * @brief シェーダーとVAO/VBOを初期化（初回呼び出し時のみ実行）
     */
    static void initialize();

    /**
     * @brief リソース解放
     */
    static void cleanup();

    /**
     * @brief 単色quadを描画
     * @param x 左上X座標
     * @param y 左上Y座標
     * @param w 幅
     * @param h 高さ
     * @param color RGBA色
     * @param screenW スクリーン幅
     * @param screenH スクリーン高さ
     */
    static void drawColoredQuad(float x, float y, float w, float h,
                                const glm::vec4& color,
                                int screenW, int screenH);

    /**
     * @brief テクスチャ付きquadを描画
     * @param textureId テクスチャID
     * @param color 乗算色（通常は白）
     */
    static void drawTexturedQuad(float x, float y, float w, float h,
                                 GLuint textureId,
                                 const glm::vec4& color,
                                 int screenW, int screenH);

    /**
     * @brief テクスチャ付きquadを描画（カスタムUV指定）
     * @param uMin,vMin,uMax,vMax UV座標範囲
     */
    static void drawTexturedQuad(float x, float y, float w, float h,
                                 GLuint textureId,
                                 const glm::vec4& color,
                                 int screenW, int screenH,
                                 float uMin, float vMin, float uMax, float vMax);

    /**
     * @brief 枠線を描画（4辺のquad）
     */
    static void drawBorder(float x, float y, float w, float h,
                           float borderWidth, const glm::vec4& color,
                           int screenW, int screenH);

    /**
     * @brief 初期化済みか
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
