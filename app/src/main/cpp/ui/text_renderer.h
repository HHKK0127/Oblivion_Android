#pragma once

#include <string>
#include <map>
#include <glm/glm.hpp>
#include <GLES3/gl3.h>
#include <android/asset_manager.h>

/**
 * @brief stb_truetype を使用したテキストレンダリングシステム
 * TTFフォントをビットマップに変換してテクスチャアトラスを作成
 */
class TextRenderer {
public:
    TextRenderer();
    ~TextRenderer();

    /**
     * @brief テキストレンダリングシステムを初期化
     * @param assetMgr Android AssetManager（フォント読み込み用）
     * @return 初期化成功時true
     */
    bool initialize(AAssetManager* assetMgr);

    /**
     * @brief テキストを画面に描画
     * @param text 描画するテキスト
     * @param x 画面左上を原点とした X座標（ピクセル）
     * @param y 画面左上を原点とした Y座標（ピクセル）
     * @param color テキストの色 (r, g, b)
     * @param scale スケール（デフォルト 1.0f）
     */
    void renderText(const std::string& text, float x, float y,
                    const glm::vec3& color = glm::vec3(1.0f, 1.0f, 1.0f),
                    float scale = 1.0f);

    /**
     * @brief 画面サイズを設定（投影行列計算用）
     */
    void setScreenSize(int width, int height);

    /**
     * @brief クリーンアップ
     */
    void cleanup();

    /**
     * @brief デバッグ用: 単純な四角形を描画（テスト用）
     */
    void renderDebugQuad();

private:
    struct Glyph {
        float x0, y0, x1, y1;  // テクスチャ座標
        float advanceX;
        float bearingX, bearingY;
    };

    // フォントレンダリング用
    struct FontData {
        unsigned char* fontData;
        float fontScale;
        int fontHeight;
        std::map<unsigned int, Glyph> glyphCache;
    };

    GLuint vao;
    GLuint vbo;
    GLuint shaderProgram;
    GLint projectionLoc;
    GLint colorLoc;
    GLuint fontTexture;  // フォントテクスチャ

    int screenWidth;
    int screenHeight;

    // フォントデータ
    FontData* fontData;

    // Android AssetManager（フォント読み込み用）
    AAssetManager* assetManager;

    // テクスチャアトラスのサイズ
    static constexpr int ATLAS_WIDTH = 512;
    static constexpr int ATLAS_HEIGHT = 512;
    static constexpr int FONT_SIZE = 32;

    void compileShaders();
    Glyph getGlyph(unsigned int codepoint);
    bool loadFontFromAssets(const std::string& filename);
    bool createFontTextureAtlas();
};
