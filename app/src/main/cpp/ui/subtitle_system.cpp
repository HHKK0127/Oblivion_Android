#include "subtitle_system.h"

#include <algorithm>

#include "text_renderer.h"
#include "text_wrap.h"

namespace {

// hud_subtitle_text <red>/<green>/<blue>/<alpha>.
constexpr float kRed = 254.0f / 255.0f;
constexpr float kGreen = 244.0f / 255.0f;
constexpr float kBlue = 183.0f / 255.0f;

}  // namespace

glm::vec4 SubtitleSystem::textColor() {
    return glm::vec4(kRed, kGreen, kBlue, 1.0f);
}

float SubtitleSystem::durationForText(const std::string& text) {
    const float seconds = static_cast<float>(text.size()) * kSecondsPerCharacter;
    return std::min(kMaxDuration, std::max(kMinDuration, seconds));
}

void SubtitleSystem::show(const std::string& text, float duration) {
    if (text.empty()) {
        return;
    }
    line_.text = text;
    line_.duration = (duration > 0.0f) ? duration : durationForText(text);
    line_.elapsed = 0.0f;
    active_ = true;
}

void SubtitleSystem::clear() {
    active_ = false;
    line_ = Line();
}

void SubtitleSystem::update(float deltaTime) {
    if (!active_) {
        return;
    }
    line_.elapsed += deltaTime;
    if (line_.elapsed >= line_.duration + kFadeSeconds) {
        clear();
    }
}

float SubtitleSystem::alphaFor(const Line& line) {
    const float in = std::min(1.0f, line.elapsed / kFadeSeconds);
    const float remaining = line.duration + kFadeSeconds - line.elapsed;
    const float out = std::min(1.0f, std::max(0.0f, remaining) / kFadeSeconds);
    return std::min(in, out);
}

std::vector<std::string> SubtitleSystem::wrap(TextRenderer* textRenderer,
                                              const std::string& text) const {
    return text_wrap::wrapText(textRenderer, text, kWrapWidth, kTextScale);
}

void SubtitleSystem::render(TextRenderer* textRenderer, int screenWidth, int screenHeight) {
    if (!active_ || textRenderer == nullptr) {
        return;
    }

    const float alpha = alphaFor(line_);
    if (alpha <= 0.0f) {
        return;
    }

    const std::vector<std::string> lines = wrap(textRenderer, line_.text);
    const float lineHeight = textRenderer->getTextHeight(kTextScale);
    const float blockHeight = lineHeight * static_cast<float>(lines.size());

    // hudsubtitle_text y: screen height - 115 - the text's own height, so the block
    // grows upwards and the last line always keeps the 115 px gap from the bottom.
    const float top = static_cast<float>(screenHeight) - kBottomMargin - blockHeight;
    const float centerX = static_cast<float>(screenWidth) * 0.5f;

    glm::vec4 color = textColor();
    color.w = alpha;

    for (size_t i = 0; i < lines.size(); ++i) {
        const float width = textRenderer->getTextWidth(lines[i], kTextScale);
        textRenderer->renderText(lines[i], centerX - width * 0.5f,
                                 top + lineHeight * static_cast<float>(i), color, kTextScale);
    }
}
