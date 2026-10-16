#include "text_wrap.h"

#include <algorithm>

#include "text_renderer.h"

namespace text_wrap {

std::string trimSpaces(const std::string& s) {
    size_t begin = 0;
    size_t end = s.size();
    while (begin < end && (s[begin] == ' ' || s[begin] == '\t' || s[begin] == '\r')) {
        ++begin;
    }
    while (end > begin &&
           (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r')) {
        --end;
    }
    return s.substr(begin, end - begin);
}

std::vector<size_t> utf8Offsets(const std::string& s) {
    std::vector<size_t> offsets;
    for (size_t i = 0; i < s.size();) {
        offsets.push_back(i);
        const unsigned char c = static_cast<unsigned char>(s[i]);
        size_t len = 1;
        if ((c & 0xE0) == 0xC0) {
            len = 2;
        } else if ((c & 0xF0) == 0xE0) {
            len = 3;
        } else if ((c & 0xF8) == 0xF0) {
            len = 4;
        }
        i += std::min(len, s.size() - i);
    }
    return offsets;
}

void appendWrappedParagraph(TextRenderer* renderer, const std::string& paragraph,
                            float maxWidth, float scale, std::vector<std::string>& out) {
    const std::vector<size_t> offsets = utf8Offsets(paragraph);
    const size_t count = offsets.size();
    if (count == 0) {
        out.push_back(std::string());
        return;
    }

    size_t start = 0;
    while (start < count) {
        size_t i = start;
        size_t lastSpace = std::string::npos;
        while (i < count) {
            const size_t end = (i + 1 < count) ? offsets[i + 1] : paragraph.size();
            const float width =
                renderer->getTextWidth(paragraph.substr(offsets[start], end - offsets[start]), scale);
            if (width > maxWidth && i > start) {
                break;
            }
            if (paragraph[offsets[i]] == ' ') {
                lastSpace = i;
            }
            ++i;
        }

        if (i >= count) {
            out.push_back(trimSpaces(paragraph.substr(offsets[start])));
            break;
        }

        const bool breakAtSpace = (lastSpace != std::string::npos && lastSpace > start);
        size_t cut = breakAtSpace ? lastSpace : i;
        if (cut <= start) {
            cut = start + 1;  // single codepoint wider than the wrap width
        }
        out.push_back(trimSpaces(paragraph.substr(offsets[start], offsets[cut] - offsets[start])));

        if (breakAtSpace) {
            ++cut;  // consume the space we broke on
        }
        start = cut;
        while (start < count && paragraph[offsets[start]] == ' ') {
            ++start;
        }
    }
}

std::vector<std::string> wrapText(TextRenderer* renderer, const std::string& text,
                                  float maxWidth, float scale) {
    std::vector<std::string> lines;
    if (renderer == nullptr) {
        lines.push_back(trimSpaces(text));
        return lines;
    }

    std::string paragraph;
    for (size_t i = 0; i <= text.size(); ++i) {
        if (i == text.size() || text[i] == '\n') {
            appendWrappedParagraph(renderer, paragraph, maxWidth, scale, lines);
            paragraph.clear();
        } else {
            paragraph.push_back(text[i]);
        }
    }
    if (lines.empty()) {
        lines.push_back(std::string());
    }
    return lines;
}

}  // namespace text_wrap
