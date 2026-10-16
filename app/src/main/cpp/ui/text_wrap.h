#pragma once

#include <cstddef>
#include <string>
#include <vector>

class TextRenderer;

/**
 * @brief Shared UTF-8 aware word wrap for the menu text panels.
 *
 * TextRenderer has no wrap helper of its own, so every panel that honours a menu
 * <wrapwidth> (message_menu, hud_subtitle_menu, ...) goes through here instead of
 * carrying its own copy.
 */
namespace text_wrap {

// Start offset of every UTF-8 codepoint, so wrapping never splits a character.
std::vector<size_t> utf8Offsets(const std::string& text);

// Leading/trailing space, tab and CR removal for one wrapped line.
std::string trimSpaces(const std::string& text);

// Width based word wrap. Long words (and CJK runs, which carry no spaces) are broken
// between codepoints. An empty paragraph still emits one empty line so callers can
// keep their line accounting simple.
void appendWrappedParagraph(TextRenderer* renderer, const std::string& paragraph,
                            float maxWidth, float scale, std::vector<std::string>& out);

// Splits on '\n' and wraps every paragraph. Always returns at least one line.
std::vector<std::string> wrapText(TextRenderer* renderer, const std::string& text,
                                  float maxWidth, float scale);

}  // namespace text_wrap
