#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>

class TextRenderer;

/**
 * @brief On-screen subtitles for spoken dialogue lines (the HUDSubtitleMenu).
 *
 * Layout, face and colour mirror the shipped Data\Menus\Main\hud_subtitle_menu.xml
 * so the port renders the retail subtitle instead of an invented one:
 *   text "hudsubtitle_text" - font 2 (KingthingsRegular, the port's active face),
 *   rgb(254,244,183) / alpha 255, justify &center;, wrapwidth 1000,
 *   x = screen width / 2, y = screen height - 115 - text height.
 * The menu carries <menufade> 0.15, so a line fades in and back out over
 * kFadeSeconds rather than popping.
 *
 * The menu is does_not_stack and holds a single text element, so a new line
 * replaces whatever is on screen - the original never shows two at once.
 */
class SubtitleSystem {
public:
    struct Line {
        std::string text;
        float duration = 0.0f;  // seconds held at full alpha, excluding the fades
        float elapsed = 0.0f;
    };

    // Values from hud_subtitle_menu.xml.
    static constexpr float kFadeSeconds = 0.15f;    // <menufade>
    static constexpr float kWrapWidth = 1000.0f;    // <wrapwidth>
    static constexpr float kBottomMargin = 115.0f;  // screen().height - ... - 115
    static constexpr float kTextScale = 1.0f;       // <font> 2 at the port UI scale

    // How long a line stays up. The engine times a subtitle off the voice clip it
    // accompanies; the port has no clip length query yet, so it is derived from the
    // text and clamped to a readable window. Kept as named constants so a later
    // voice-length source only has to replace durationForText().
    static constexpr float kSecondsPerCharacter = 0.075f;
    static constexpr float kMinDuration = 1.5f;
    static constexpr float kMaxDuration = 10.0f;

    // hudsubtitle_text <red>/<green>/<blue>/<alpha> = 254/244/183/255.
    static glm::vec4 textColor();

    static float durationForText(const std::string& text);

    // Replaces the line on screen. A duration of 0 asks for durationForText().
    void show(const std::string& text, float duration = 0.0f);
    void clear();

    void update(float deltaTime);
    void render(TextRenderer* textRenderer, int screenWidth, int screenHeight);

    bool hasActive() const { return active_; }
    const Line& activeLine() const { return line_; }

    // Fades in over kFadeSeconds, holds, then fades back out over kFadeSeconds.
    static float alphaFor(const Line& line);

    // Word wrap at kWrapWidth. Also the layout the renderer draws, so it is exposed
    // for tests that cannot reach GL.
    std::vector<std::string> wrap(TextRenderer* textRenderer, const std::string& text) const;

private:
    Line line_;
    bool active_ = false;
};
