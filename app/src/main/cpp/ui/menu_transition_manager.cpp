#include "menu_transition_manager.h"
#include "text_renderer.h"
#include "ui_draw_helper.h"
#include <cmath>
#include <algorithm>

MenuTransitionManager::MenuTransitionManager()
    : textRenderer(nullptr)
    , screenWidth(1080)
    , screenHeight(1920)
    , state(TransitionState::IDLE)
    , currentType(TransitionType::NONE)
    , transitionDuration(0.3f)
    , elapsedTime(0.0f)
    , currentAlpha(0.0f)
    , defaultDuration(0.3f)
    , overlayColor(0.0f, 0.0f, 0.0f, 1.0f)
    , slideOffset(0.0f)
    , outgoingCallbackFired(false) {
}

void MenuTransitionManager::initialize(TextRenderer* renderer, int w, int h) {
    textRenderer = renderer;
    screenWidth = w;
    screenHeight = h;
    MTRANS_LOGI("MenuTransitionManager initialized: %dx%d", w, h);
}

void MenuTransitionManager::update(float deltaTime) {
    if (state == TransitionState::IDLE) return;

    elapsedTime += deltaTime;
    float progress = std::min(elapsedTime / transitionDuration, 1.0f);

    switch (currentType) {
        case TransitionType::FADE_IN:
        case TransitionType::FADE_OUT:
            updateFade(progress);
            break;
        case TransitionType::SLIDE_LEFT:
        case TransitionType::SLIDE_RIGHT:
        case TransitionType::SLIDE_UP:
        case TransitionType::SLIDE_DOWN:
            updateSlide(progress);
            break;
        default:
            break;
    }

    // 50% 遷移完了で outgoing コールバック発火
    if (!outgoingCallbackFired && progress >= 0.5f && onOutgoingComplete) {
        onOutgoingComplete();
        outgoingCallbackFired = true;
        state = TransitionState::INCOMING;
        MTRANS_LOGD("Transition: outgoing complete, switching to incoming");
    }

    // 100% 完了
    if (progress >= 1.0f) {
        if (onIncomingComplete) {
            onIncomingComplete();
        }
        state = TransitionState::IDLE;
        currentType = TransitionType::NONE;
        currentAlpha = 0.0f;
        slideOffset = glm::vec2(0.0f, 0.0f);
        MTRANS_LOGD("Transition complete");
    }
}

void MenuTransitionManager::render() {
    if (state == TransitionState::IDLE) return;

    glm::vec4 color = overlayColor;
    color.w = currentAlpha;

    if (currentAlpha > 0.01f) {
        UIDrawHelper::drawColoredQuad(
            slideOffset.x, slideOffset.y,
            static_cast<float>(screenWidth), static_cast<float>(screenHeight),
            color, screenWidth, screenHeight
        );
    }
}

void MenuTransitionManager::startTransition(TransitionType type, float duration,
                                             std::function<void()> outComplete,
                                             std::function<void()> inComplete) {
    if (state != TransitionState::IDLE) {
        MTRANS_LOGW("Transition already in progress, forcing complete");
        if (onIncomingComplete) onIncomingComplete();
    }

    currentType = type;
    transitionDuration = (duration > 0.0f) ? duration : defaultDuration;
    elapsedTime = 0.0f;
    currentAlpha = 0.0f;
    slideOffset = glm::vec2(0.0f, 0.0f);
    state = TransitionState::OUTGOING;
    outgoingCallbackFired = false;

    onOutgoingComplete = std::move(outComplete);
    onIncomingComplete = std::move(inComplete);

    MTRANS_LOGI("Transition started: type=%d duration=%.2f",
                static_cast<int>(type), transitionDuration);
}

void MenuTransitionManager::setScreenSize(int w, int h) {
    screenWidth = w;
    screenHeight = h;
}

// === イージング関数 ===

float MenuTransitionManager::easeInOut(float t) const {
    return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;
}

float MenuTransitionManager::easeOutCubic(float t) const {
    return 1.0f - std::pow(1.0f - t, 3.0f);
}

// === 遷移タイプ別更新 ===

void MenuTransitionManager::updateFade(float progress) {
    float eased = easeInOut(progress);

    if (currentType == TransitionType::FADE_IN) {
        // フェードイン: 透明 → 不透明 → 透明
        if (progress < 0.5f) {
            currentAlpha = eased * 2.0f;
        } else {
            currentAlpha = 2.0f - eased * 2.0f;
        }
    } else {
        // フェードアウト: 不透明 → 透明
        currentAlpha = 1.0f - eased;
    }
}

void MenuTransitionManager::updateSlide(float progress) {
    float eased = easeOutCubic(progress);
    float sw = static_cast<float>(screenWidth);
    float sh = static_cast<float>(screenHeight);

    // スライドのオフセット計算
    float offset;
    if (progress < 0.5f) {
        offset = (1.0f - eased * 2.0f);
    } else {
        offset = -(1.0f - (eased - 0.5f) * 2.0f);
    }

    switch (currentType) {
        case TransitionType::SLIDE_LEFT:
            slideOffset = glm::vec2(-sw * offset, 0.0f);
            break;
        case TransitionType::SLIDE_RIGHT:
            slideOffset = glm::vec2(sw * offset, 0.0f);
            break;
        case TransitionType::SLIDE_UP:
            slideOffset = glm::vec2(0.0f, -sh * offset);
            break;
        case TransitionType::SLIDE_DOWN:
            slideOffset = glm::vec2(0.0f, sh * offset);
            break;
        default:
            break;
    }

    // スライド中もフェードを適用
    currentAlpha = std::abs(offset) * 0.5f;
}
