#pragma once

#include <string>
#include <cstdint>

class TextRenderer {
public:
    TextRenderer();
    ~TextRenderer();

    bool initialize();
    void cleanup();
    void setScreenSize(unsigned int width, unsigned int height);

    void renderText(const std::string& text, float x, float y,
                    float scale, const float color[4]);
    void renderText(const std::string& text, float x, float y,
                    float scale, float r, float g, float b, float a);

    float getTextWidth(const std::string& text, float scale) const;
    float getTextHeight(float scale) const;

    unsigned int getScreenWidth() const { return screenWidth; }
    unsigned int getScreenHeight() const { return screenHeight; }

private:
    unsigned int screenWidth;
    unsigned int screenHeight;
    bool initialized;
};