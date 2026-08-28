#include "touch_gesture_handler.h"
#include <cmath>
#include <algorithm>

TouchGestureHandler::TouchGestureHandler()
    : tapTimeout(0.3f)
    , doubleTapTimeout(0.35f)
    , longPressTimeout(0.6f)
    , swipeThreshold(50.0f)
    , pinchThreshold(0.15f)
    , enabled(true)
    , currentTime(0.0f)
    , activePointerCount(0)
    , lastTapTime(-1.0f)
    , lastTapPos(0.0f, 0.0f)
    , initialPinchDistance(0.0f)
    , currentPinchDistance(0.0f) {
}

void TouchGestureHandler::registerCallback(GestureType type, GestureCallback callback) {
    callbacks.emplace_back(type, std::move(callback));
}

void TouchGestureHandler::registerAllCallback(GestureCallback callback) {
    allCallback = std::move(callback);
}

void TouchGestureHandler::onTouchDown(float x, float y, int pointerId) {
    if (!enabled || pointerId < 0 || pointerId >= MAX_POINTERS) return;

    auto& pt = pointers[pointerId];
    pt.startPos = glm::vec2(x, y);
    pt.currentPos = pt.startPos;
    pt.startTime = currentTime;
    pt.active = true;

    activePointerCount = findActivePointerCount();

    // 2本指の初期距離を記録
    if (activePointerCount == 2) {
        for (int i = 0; i < MAX_POINTERS; ++i) {
            for (int j = i + 1; j < MAX_POINTERS; ++j) {
                if (pointers[i].active && pointers[j].active) {
                    initialPinchDistance = getDistance(pointers[i].currentPos, pointers[j].currentPos);
                    currentPinchDistance = initialPinchDistance;
                    break;
                }
            }
        }
    }

    GESTURE_LOGD("onTouchDown: pointer=%d pos=(%.1f,%.1f) active=%d",
                 pointerId, x, y, activePointerCount);
}

void TouchGestureHandler::onTouchMove(float x, float y, int pointerId) {
    if (!enabled || pointerId < 0 || pointerId >= MAX_POINTERS) return;

    auto& pt = pointers[pointerId];
    if (!pt.active) return;

    pt.currentPos = glm::vec2(x, y);

    // 2本指操作の更新
    if (activePointerCount >= 2) {
        for (int i = 0; i < MAX_POINTERS; ++i) {
            for (int j = i + 1; j < MAX_POINTERS; ++j) {
                if (pointers[i].active && pointers[j].active) {
                    currentPinchDistance = getDistance(pointers[i].currentPos, pointers[j].currentPos);
                    break;
                }
            }
        }
    }
}

void TouchGestureHandler::onTouchUp(float x, float y, int pointerId) {
    if (!enabled || pointerId < 0 || pointerId >= MAX_POINTERS) return;

    auto& pt = pointers[pointerId];
    if (!pt.active) return;

    pt.currentPos = glm::vec2(x, y);

    // シングルタッチの判定
    if (activePointerCount == 1) {
        float dist = getDistance(pt.startPos, pt.currentPos);
        float duration = currentTime - pt.startTime;

        if (dist < swipeThreshold) {
            // スワイプでない → タップ系
            if (duration < tapTimeout) {
                // ダブルタップ判定
                if (lastTapTime >= 0.0f &&
                    (currentTime - lastTapTime) < doubleTapTimeout &&
                    getDistance(pt.currentPos, lastTapPos) < swipeThreshold) {
                    // ダブルタップ
                    GestureEvent event;
                    event.type = GestureType::DOUBLE_TAP;
                    event.position = pt.currentPos;
                    event.duration = currentTime - lastTapTime;
                    event.pointerCount = 1;
                    fireGesture(event);
                    lastTapTime = -1.0f;
                } else {
                    // シングルタップ（次フレームでダブルタップの可能性あり）
                    lastTapTime = currentTime;
                    lastTapPos = pt.currentPos;

                    GestureEvent event;
                    event.type = GestureType::SINGLE_TAP;
                    event.position = pt.currentPos;
                    event.duration = duration;
                    event.pointerCount = 1;
                    fireGesture(event);
                }
            }
        } else {
            // スワイプ判定
            detectSwipe(pointerId);
        }
    }

    pt.active = false;
    activePointerCount = findActivePointerCount();
}

void TouchGestureHandler::update(float deltaTime) {
    if (!enabled) return;

    currentTime += deltaTime;

    // ロングプレス判定
    for (int i = 0; i < MAX_POINTERS; ++i) {
        if (pointers[i].active) {
            detectLongPress(i);
        }
    }

    // ピンチ判定（2本指操作中）
    if (activePointerCount >= 2) {
        detectPinch();
        detectTwoFingerSwipe();
    }
}

void TouchGestureHandler::reset() {
    for (auto& pt : pointers) {
        pt.active = false;
    }
    activePointerCount = 0;
    lastTapTime = -1.0f;
    initialPinchDistance = 0.0f;
    currentPinchDistance = 0.0f;
}

// === 内部ジェスチャー検出 ===

void TouchGestureHandler::detectTap(int pointerId) {
    // onTouchUp で処理済み
}

void TouchGestureHandler::detectLongPress(int pointerId) {
    auto& pt = pointers[pointerId];
    if (!pt.active) return;

    float duration = currentTime - pt.startTime;
    float dist = getDistance(pt.startPos, pt.currentPos);

    if (duration >= longPressTimeout && dist < swipeThreshold) {
        GestureEvent event;
        event.type = GestureType::LONG_PRESS;
        event.position = pt.startPos;
        event.duration = duration;
        event.pointerCount = 1;
        fireGesture(event);

        // 一度発火したら無効化（連続発火防止）
        pt.startTime = currentTime + 999.0f;
    }
}

void TouchGestureHandler::detectSwipe(int pointerId) {
    auto& pt = pointers[pointerId];
    glm::vec2 delta = pt.currentPos - pt.startPos;
    float dist = getDistance(pt.startPos, pt.currentPos);

    if (dist < swipeThreshold) return;

    GestureEvent event;
    event.position = pt.startPos;
    event.delta = delta;
    event.magnitude = dist;
    event.pointerCount = 1;
    event.duration = currentTime - pt.startTime;

    // 方向判定（最も大きな軸成分で判定）
    if (std::abs(delta.x) > std::abs(delta.y)) {
        event.type = (delta.x > 0) ? GestureType::SWIPE_RIGHT : GestureType::SWIPE_LEFT;
    } else {
        event.type = (delta.y > 0) ? GestureType::SWIPE_DOWN : GestureType::SWIPE_UP;
    }

    fireGesture(event);
}

void TouchGestureHandler::detectPinch() {
    if (initialPinchDistance <= 0.0f) return;

    float ratio = currentPinchDistance / initialPinchDistance;
    float change = std::abs(ratio - 1.0f);

    if (change < pinchThreshold) return;

    GestureEvent event;
    event.type = (ratio < 1.0f) ? GestureType::PINCH_IN : GestureType::PINCH_OUT;
    event.magnitude = ratio;
    event.pointerCount = 2;

    // 中心位置を計算
    for (int i = 0; i < MAX_POINTERS; ++i) {
        for (int j = i + 1; j < MAX_POINTERS; ++j) {
            if (pointers[i].active && pointers[j].active) {
                event.position = (pointers[i].currentPos + pointers[j].currentPos) * 0.5f;
                break;
            }
        }
    }

    fireGesture(event);

    // 発火後に基準をリセット（連続検出用）
    initialPinchDistance = currentPinchDistance;
}

void TouchGestureHandler::detectTwoFingerSwipe() {
    // 2本指の平均移動量を計算
    glm::vec2 avgDelta(0.0f, 0.0f);
    int count = 0;

    for (int i = 0; i < MAX_POINTERS; ++i) {
        if (pointers[i].active) {
            glm::vec2 delta = pointers[i].currentPos - pointers[i].startPos;
            if (getDistance(glm::vec2(0.0f, 0.0f), delta) >= swipeThreshold) {
                avgDelta += delta;
                ++count;
            }
        }
    }

    if (count < 2) return;

    avgDelta /= static_cast<float>(count);
    float dist = getDistance(glm::vec2(0.0f), avgDelta);

    if (dist < swipeThreshold) return;

    GestureEvent event;
    event.type = GestureType::TWO_FINGER_SWIPE;
    event.delta = avgDelta;
    event.magnitude = dist;
    event.pointerCount = 2;

    // 中心位置
    glm::vec2 center(0.0f);
    int centerCount = 0;
    for (int i = 0; i < MAX_POINTERS; ++i) {
        if (pointers[i].active) {
            center += pointers[i].currentPos;
            ++centerCount;
        }
    }
    if (centerCount > 0) {
        event.position = center / static_cast<float>(centerCount);
    }

    fireGesture(event);

    // 連続発火防止: 開始位置をリセット
    for (int i = 0; i < MAX_POINTERS; ++i) {
        if (pointers[i].active) {
            pointers[i].startPos = pointers[i].currentPos;
        }
    }
}

// === ユーティリティ ===

float TouchGestureHandler::getDistance(const glm::vec2& a, const glm::vec2& b) const {
    glm::vec2 diff = a - b;
    return std::sqrt(diff.x * diff.x + diff.y * diff.y);
}

void TouchGestureHandler::fireGesture(const GestureEvent& event) {
    GESTURE_LOGD("Gesture: type=%d pos=(%.1f,%.1f) delta=(%.1f,%.1f) mag=%.2f",
                 static_cast<int>(event.type), event.position.x, event.position.y,
                 event.delta.x, event.delta.y, event.magnitude);

    // 個別コールバック
    for (const auto& cb : callbacks) {
        if (cb.first == event.type) {
            cb.second(event);
        }
    }

    // 共通コールバック
    if (allCallback) {
        allCallback(event);
    }
}

int TouchGestureHandler::findActivePointerCount() const {
    int count = 0;
    for (int i = 0; i < MAX_POINTERS; ++i) {
        if (pointers[i].active) ++count;
    }
    return count;
}
