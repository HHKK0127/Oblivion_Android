#include "ui_book_reader.h"
#include "placeholder_assets.h"
#include "ui_draw_helper.h"
#include <GLES3/gl3.h>
#include <algorithm>
#include <cctype>
#include <sstream>

namespace {

// Oblivion book bodies carry HTML-like markup (<font>, <DIV>, <br>, <IMG>).
// The panel renders plain text, so tags are stripped and <br>/<p> become
// line breaks.
std::string stripMarkup(const std::string& in) {
    std::string out;
    out.reserve(in.size());

    for (size_t i = 0; i < in.size(); ) {
        if (in[i] != '<') {
            out.push_back(in[i++]);
            continue;
        }

        const size_t close = in.find('>', i);
        if (close == std::string::npos) {
            // Unterminated tag: keep the remainder verbatim.
            out.append(in, i, std::string::npos);
            break;
        }

        std::string tag = in.substr(i + 1, close - i - 1);
        std::transform(tag.begin(), tag.end(), tag.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

        if (tag.rfind("br", 0) == 0 || tag.rfind("p", 0) == 0 ||
            tag.rfind("/p", 0) == 0 || tag.rfind("div", 0) == 0) {
            out.push_back('\n');
        }

        i = close + 1;
    }

    return out;
}

// Greedy word wrap. CJK text has no spaces, so a run without a break
// opportunity is split at the character budget instead of overflowing.
std::vector<std::string> wrapText(const std::string& text, size_t maxChars) {
    std::vector<std::string> lines;
    if (maxChars == 0) return lines;

    std::istringstream stream(text);
    std::string paragraph;

    while (std::getline(stream, paragraph)) {
        if (!paragraph.empty() && paragraph.back() == '\r') {
            paragraph.pop_back();
        }

        if (paragraph.empty()) {
            lines.emplace_back();
            continue;
        }

        std::string line;
        size_t lastSpace = std::string::npos;

        for (char ch : paragraph) {
            line.push_back(ch);
            if (ch == ' ') {
                lastSpace = line.size() - 1;
            }

            if (line.size() >= maxChars) {
                if (lastSpace != std::string::npos && lastSpace > 0) {
                    lines.push_back(line.substr(0, lastSpace));
                    line = line.substr(lastSpace + 1);
                } else {
                    lines.push_back(line);
                    line.clear();
                }
                lastSpace = std::string::npos;
            }
        }

        if (!line.empty()) {
            lines.push_back(line);
        }
    }

    return lines;
}

} // namespace

UIBookReader::UIBookReader(const std::string& title)
    : UIPanel(title.empty() ? "BookReader" : title) {
    setBackgroundColor(glm::vec4(
        PlaceholderAssets::Colors::PARCHMENT_LIGHT.x,
        PlaceholderAssets::Colors::PARCHMENT_LIGHT.y,
        PlaceholderAssets::Colors::PARCHMENT_LIGHT.z, 0.98f));
    setBorderColor(glm::vec4(
        PlaceholderAssets::Colors::BROWN_ACCENT.x,
        PlaceholderAssets::Colors::BROWN_ACCENT.y,
        PlaceholderAssets::Colors::BROWN_ACCENT.z, 1.0f));
    setBorderWidth(3.0f);
    setTitleBarColor(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f));  // title drawn in the body
    setCloseButtonVisible(true);
    setDraggable(false);
}

bool UIBookReader::initialize(TextRenderer* tr) {
    if (!tr) return false;
    textRenderer = tr;
    return UIPanel::initialize();
}

void UIBookReader::openBook(const std::string& title, const std::string& body) {
    bookTitle = title;
    currentPage = 0;
    paginate(stripMarkup(body));
    setVisible(true);
}

void UIBookReader::closeBook() {
    setVisible(false);
    pages.clear();
    bookTitle.clear();
    currentPage = 0;
    if (onClose) onClose();
}

void UIBookReader::paginate(const std::string& body) {
    pages.clear();

    const int linesPerPage = getLinesPerPage();
    if (linesPerPage <= 0) {
        pages.push_back(body);
        return;
    }

    // Approximate the character budget from the panel width. The Oblivion
    // face is proportional, so this is a conservative estimate that keeps
    // lines inside the frame.
    const float bodyWidth = getBodyWidth();
    const size_t maxChars = static_cast<size_t>(std::max(16.0f, bodyWidth / 11.0f));

    const std::vector<std::string> lines = wrapText(body, maxChars);
    if (lines.empty()) {
        pages.emplace_back();
        return;
    }

    for (size_t i = 0; i < lines.size(); i += static_cast<size_t>(linesPerPage)) {
        const size_t end = std::min(lines.size(), i + static_cast<size_t>(linesPerPage));
        std::string page;
        for (size_t j = i; j < end; ++j) {
            if (!page.empty()) page.push_back('\n');
            page += lines[j];
        }
        pages.push_back(std::move(page));
    }
}

void UIBookReader::update(float deltaTime) {
    UIPanel::update(deltaTime);
}

bool UIBookReader::onTouchDown(float x, float y, int pointerId) {
    (void)pointerId;
    if (!isVisible() || !isEnabled()) return false;

    if (isInsideCloseButton(x, y)) {
        closeBook();
        return true;
    }

    if (isInsidePrevButton(x, y)) {
        if (currentPage > 0) currentPage--;
        return true;
    }

    if (isInsideNextButton(x, y)) {
        if (currentPage + 1 < static_cast<int>(pages.size())) currentPage++;
        return true;
    }

    return true;  // consume all touches while open
}

void UIBookReader::render() {
    if (!isVisible()) return;

    GLboolean depthTestEnabled;
    glGetBooleanv(GL_DEPTH_TEST, &depthTestEnabled);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    UIPanel::render();

    renderTitle();
    renderBody();
    renderPageFooter();

    if (depthTestEnabled) glEnable(GL_DEPTH_TEST);
}

void UIBookReader::renderTitle() {
    if (!textRenderer) return;

    const glm::vec2 cp = getContentPosition();
    const float bodyWidth = getBodyWidth();

    textRenderer->renderText(bookTitle, cp.x, cp.y,
        glm::vec3(PlaceholderAssets::Colors::BROWN_ACCENT), 1.15f);

    // Rule under the title
    UIDrawHelper::drawColoredQuad(
        cp.x, cp.y + 30.0f, bodyWidth, 1.5f,
        glm::vec4(PlaceholderAssets::Colors::BROWN_ACCENT.x,
                  PlaceholderAssets::Colors::BROWN_ACCENT.y,
                  PlaceholderAssets::Colors::BROWN_ACCENT.z, 0.6f),
        screenWidth, screenHeight);}

void UIBookReader::renderBody() {
    if (!textRenderer || pages.empty()) return;

    const glm::vec2 cp = getContentPosition();
    const float top = getBodyTop();
    const int page = std::clamp(currentPage, 0, static_cast<int>(pages.size()) - 1);

    float y = top;
    std::istringstream stream(pages[page]);
    std::string line;
    while (std::getline(stream, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        textRenderer->renderText(line, cp.x, y,
            glm::vec3(0.12f, 0.09f, 0.06f), 0.85f);
        y += LINE_H;
    }
}

void UIBookReader::renderPageFooter() {
    if (!textRenderer) return;

    const glm::vec2 cp = getContentPosition();
    const glm::vec2 cs = getContentSize();
    const float footerY = cp.y + cs.y - 34.0f;

    const std::string label = std::to_string(currentPage + 1) + " / " +
                              std::to_string(std::max<size_t>(1, pages.size()));
    textRenderer->renderText(label, cp.x + cs.x * 0.5f - 24.0f, footerY,
        glm::vec3(PlaceholderAssets::Colors::BROWN_ACCENT), 0.8f);

    const bool hasPrev = currentPage > 0;
    const bool hasNext = currentPage + 1 < static_cast<int>(pages.size());

    UIDrawHelper::drawColoredQuad(
        cp.x, footerY - 6.0f, 90.0f, 30.0f,
        glm::vec4(PlaceholderAssets::Colors::BROWN_ACCENT.x,
                  PlaceholderAssets::Colors::BROWN_ACCENT.y,
                  PlaceholderAssets::Colors::BROWN_ACCENT.z,
                  hasPrev ? 0.85f : 0.25f),
        screenWidth, screenHeight);
    textRenderer->renderText("< Prev", cp.x + 12.0f, footerY,
        glm::vec3(1.0f, 0.96f, 0.88f), 0.8f);

    UIDrawHelper::drawColoredQuad(
        cp.x + cs.x - 90.0f, footerY - 6.0f, 90.0f, 30.0f,
        glm::vec4(PlaceholderAssets::Colors::BROWN_ACCENT.x,
                  PlaceholderAssets::Colors::BROWN_ACCENT.y,
                  PlaceholderAssets::Colors::BROWN_ACCENT.z,
                  hasNext ? 0.85f : 0.25f),
        screenWidth, screenHeight);
    textRenderer->renderText("Next >", cp.x + cs.x - 78.0f, footerY,
        glm::vec3(1.0f, 0.96f, 0.88f), 0.8f);
}

float UIBookReader::getBodyTop() const {
    return getContentPosition().y + 44.0f;
}

float UIBookReader::getBodyWidth() const {
    return getContentSize().x;
}

int UIBookReader::getLinesPerPage() const {
    const float available = getContentSize().y - 44.0f - 48.0f;
    return static_cast<int>(available / LINE_H);
}

bool UIBookReader::isInsidePrevButton(float x, float y) const {
    const glm::vec2 cp = getContentPosition();
    const glm::vec2 cs = getContentSize();
    const float footerY = cp.y + cs.y - 40.0f;
    return x >= cp.x && x <= cp.x + 90.0f &&
           y >= footerY && y <= footerY + 30.0f;
}

bool UIBookReader::isInsideNextButton(float x, float y) const {
    const glm::vec2 cp = getContentPosition();
    const glm::vec2 cs = getContentSize();
    const float footerY = cp.y + cs.y - 40.0f;
    return x >= cp.x + cs.x - 90.0f && x <= cp.x + cs.x &&
           y >= footerY && y <= footerY + 30.0f;
}
