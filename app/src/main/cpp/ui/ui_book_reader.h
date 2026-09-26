#pragma once

#include "ui_panel.h"
#include "text_renderer.h"
#include <string>
#include <vector>
#include <functional>

/**
 * @brief Book reading UI
 *
 * Displays a book's title and body text on a parchment panel, with page
 * navigation for books longer than one screen. Text is supplied by the
 * caller (BookReader::getBookDescription()), so the panel stays free of
 * ESM and localization dependencies.
 */
class UIBookReader : public UIPanel {
public:
    using CloseCallback = std::function<void()>;

    explicit UIBookReader(const std::string& title = "");
    ~UIBookReader() override = default;

    bool initialize(TextRenderer* textRenderer);

    /**
     * @brief Open a book
     * @param title Book title (already localized)
     * @param body Book body text (already localized)
     */
    void openBook(const std::string& title, const std::string& body);
    void closeBook();

    void update(float deltaTime) override;
    bool onTouchDown(float x, float y, int pointerId) override;
    void render() override;

    void setScreenSize(int w, int h) { screenWidth = w; screenHeight = h; }

    void setOnClose(CloseCallback cb) { onClose = std::move(cb); }

    const std::string& getBookTitle() const { return bookTitle; }
    int getPageCount() const { return static_cast<int>(pages.size()); }
    int getCurrentPage() const { return currentPage; }

private:
    TextRenderer* textRenderer = nullptr;
    CloseCallback onClose;

    std::string bookTitle;
    std::vector<std::string> pages;
    int currentPage = 0;

    int screenWidth  = 1080;
    int screenHeight = 1920;

    static constexpr float LINE_H = 26.0f;

    void paginate(const std::string& body);
    void renderTitle();
    void renderBody();
    void renderPageFooter();

    float getBodyTop() const;
    float getBodyWidth() const;
    int   getLinesPerPage() const;

    bool isInsidePrevButton(float x, float y) const;
    bool isInsideNextButton(float x, float y) const;
};
