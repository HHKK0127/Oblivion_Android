#pragma once

#include <functional>
#include <string>
#include <vector>
#include <GLES3/gl3.h>
#include <glm/glm.hpp>

class TextRenderer;

class UILevelUpMenu {
public:
    UILevelUpMenu();
    ~UILevelUpMenu();

    bool initialize(TextRenderer* renderer);
    void cleanup();

    void open(const std::vector<int>& attributeChoices,
              const std::vector<int>& skillBonusCounts,
              int playerLevel);
    void close();
    bool isVisible() const { return visible_; }

    void setScreenSize(int width, int height);
    void setFadeAlpha(float alpha);

    void update(float deltaTime);
    void render();

    void onTouchEvent(float x, float y);
    bool onKeyPress(int keyCode);
    bool handleBack();

    int consumeSelectedAttribute();
    bool hasSelection() const;

    using Callback = std::function<void()>;
    void setOnLevelUpCallback(Callback cb) { onLevelUp_ = std::move(cb); }
    void setOnCancelCallback(Callback cb) { onCancel_ = std::move(cb); }

    static int calculateBonus(int skillIncreaseCount);

private:
    struct AttributeRow {
        int attributeId;
        std::string name;
        std::string description;
        int bonus;
    };

    static constexpr float kContentW = 540.0f;
    static constexpr float kContentH = 536.0f;
    static constexpr float kRowHeight = 67.0f;
    static constexpr float kTitleY = 20.0f;
    static constexpr float kTitleWrapW = 900.0f;
    static constexpr float kPictureX = 620.0f;
    static constexpr float kPictureSize = 256.0f;
    static constexpr float kCoinIconX = 40.0f;
    static constexpr float kCoinIconSize = 64.0f;
    static constexpr float kTextX = 130.0f;
    static constexpr float kTextWrapW = 300.0f;
    static constexpr float kValueX = 380.0f;
    static constexpr float kMultiplierX = 480.0f;
    static constexpr float kDescY = 60.0f;
    static constexpr float kExitBtnW = 40.0f;
    static constexpr float kExitBtnH = 40.0f;
    static constexpr float kExitBtnX = 500.0f;
    static constexpr float kExitBtnY = 500.0f;
    static constexpr int kAttributeCount = 3;

    struct Rect { float x, y, w, h; };

    void rebuildRows(const std::vector<int>& choices,
                     const std::vector<int>& skillCounts);
    void drawBackground();
    void drawTitle(int playerLevel);
    void drawPicture();
    void drawAttributeDescription();
    void drawRows();
    void drawExitButton();
    void drawSelectionFrame(float x, float y, float w, float h);

    int hitTestRow(float touchX, float touchY) const;
    int hitTestExitButton(float touchX, float touchY) const;

    Rect contentRect() const;

    float scaleX_;
    float scaleY_;
    float originX_;
    float originY_;
    int screenWidth_;
    int screenHeight_;
    bool visible_;
    float fadeAlpha_;
    int focusIndex_;
    int selectedAttribute_;
    bool hasSelection_;
    int pendingAttribute_;
    int playerLevel_;
    TextRenderer* textRenderer_;

    std::vector<AttributeRow> rows_;

    GLuint bgCenterTexture_;
    GLuint bgEdgeTopTexture_;
    GLuint bgEdgeBottomTexture_;
    GLuint bgEdgeLeftTexture_;
    GLuint bgEdgeRightTexture_;
    GLuint bgCornerTLTexture_;
    GLuint bgCornerTRTexture_;
    GLuint bgCornerBLTexture_;
    GLuint bgCornerBRTexture_;
    GLuint selectionFullTexture_;

    Callback onLevelUp_;
    Callback onCancel_;

    static const char* kAttributeNames[8];
    static const char* kAttributeDescriptions[8];
};