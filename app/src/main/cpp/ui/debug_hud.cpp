#include "debug_hud.h"
#include "../audio/audio_manager.h"
#include "../engine/renderer.h"
#include <android/log.h>
#include <sstream>
#include <iomanip>
#include <sys/sysinfo.h>

#undef LOG_TAG
#define LOG_TAG "DebugHUD"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

DebugHUD::DebugHUD()
    : textRenderer(nullptr), audioManager(nullptr), renderer(nullptr), visible(true),
      fps(0.0f), frameTimeMs(0.0f), avgFrameTimeMs(0.0f),
      minFrameTimeMs(100.0f), maxFrameTimeMs(0.0f),
      frameCount(0), timeSinceLastUpdate(0.0f) {
    LOGD("DebugHUD created");
}

DebugHUD::~DebugHUD() {
    cleanup();
}

bool DebugHUD::initialize(TextRenderer* textRend, AudioManager* audioMgr, Renderer* rend) {
    if (!textRend) {
        LOGD("Error: TextRenderer is null");
        return false;
    }

    textRenderer = textRend;
    audioManager = audioMgr;
    renderer = rend;
    LOGD("DebugHUD initialized");
    return true;
}

void DebugHUD::update(float deltaTime) {
    // deltaTime は秒単位で渡される
    frameTimeMs = deltaTime * 1000.0f;  // 秒 → ミリ秒に変換

    // フレームタイムの統計を更新
    avgFrameTimeMs = (avgFrameTimeMs * frameCount + frameTimeMs) / (frameCount + 1);
    minFrameTimeMs = (frameTimeMs < minFrameTimeMs) ? frameTimeMs : minFrameTimeMs;
    maxFrameTimeMs = (frameTimeMs > maxFrameTimeMs) ? frameTimeMs : maxFrameTimeMs;

    frameCount++;
    timeSinceLastUpdate += deltaTime;

    // 0.5秒ごとにFPSを更新
    if (timeSinceLastUpdate >= UPDATE_INTERVAL) {
        if (frameTimeMs > 0.0f) {
            fps = 1000.0f / frameTimeMs;
        }
        timeSinceLastUpdate = 0.0f;
        LOGD("FPS: %.1f, Frame Time: %.2f ms", fps, frameTimeMs);
    }
}

void DebugHUD::render() {
    if (!visible || !textRenderer) {
        __android_log_print(ANDROID_LOG_WARN, "DebugHUD",
            "render() skipped: visible=%d, textRenderer=%p", visible, textRenderer);
        return;
    }
    __android_log_print(ANDROID_LOG_INFO, "DebugHUD", "render() proceeding...");

    // 白いテキストで情報を表示
    glm::vec3 textColor(1.0f, 1.0f, 1.0f);
    float xPos = 10.0f;
    float yPos = 10.0f;
    float lineHeight = 20.0f;

    // FPS表示
    {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(1) << "FPS: " << fps;
        __android_log_print(ANDROID_LOG_INFO, "DebugHUD", "Calling renderText with: %s", ss.str().c_str());
        textRenderer->renderText(ss.str(), xPos, yPos, textColor, 1.0f);
        yPos += lineHeight;
    }

    // フレームタイム表示
    {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << "Frame: " << frameTimeMs << " ms";
        textRenderer->renderText(ss.str(), xPos, yPos, textColor, 1.0f);
        yPos += lineHeight;
    }

    // 平均フレームタイム表示
    {
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << "Avg: " << avgFrameTimeMs << " ms";
        textRenderer->renderText(ss.str(), xPos, yPos, textColor, 1.0f);
        yPos += lineHeight;
    }

    // メモリ情報表示
    {
        MemoryInfo memInfo = getMemoryInfo();
        std::stringstream ss;
        ss << "Mem: " << formatMemorySize(memInfo.usedMemory);
        textRenderer->renderText(ss.str(), xPos, yPos, textColor, 1.0f);
        yPos += lineHeight;
    }

    // キューブ数表示（固定値）
    {
        textRenderer->renderText("Cubes: 5", xPos, yPos, textColor, 1.0f);
        yPos += lineHeight;
    }

    // デバッグモード表示
    {
        textRenderer->renderText("DEBUG: ON", xPos, yPos,
                                glm::vec3(1.0f, 1.0f, 0.0f), 1.0f);  // 黄色
        yPos += lineHeight;
    }

    // オーディオシステム表示
    {
        std::string audioStatus = getAudioStatus();
        glm::vec3 audioColor(0.0f, 1.0f, 1.0f);  // シアン
        textRenderer->renderText(audioStatus, xPos, yPos, audioColor, 1.0f);
        yPos += lineHeight;
    }

    // RetroFilterステータス表示
    {
        std::string filterStatus = getRetroFilterStatus();
        if (!filterStatus.empty()) {
            glm::vec3 filterColor(1.0f, 0.5f, 0.0f);  // オレンジ
            textRenderer->renderText(filterStatus, xPos, yPos, filterColor, 1.0f);
            yPos += lineHeight;
        }
    }
}

void DebugHUD::toggle() {
    visible = !visible;
    LOGD("Debug HUD toggled: %s", visible ? "ON" : "OFF");
}

void DebugHUD::cleanup() {
    textRenderer = nullptr;
    LOGD("DebugHUD cleaned up");
}

DebugHUD::MemoryInfo DebugHUD::getMemoryInfo() const {
    MemoryInfo info = {0, 0, 0};

    // /proc/meminfo から メモリ情報を取得
    FILE* memFile = fopen("/proc/meminfo", "r");
    if (memFile) {
        char line[256];
        while (fgets(line, sizeof(line), memFile)) {
            if (sscanf(line, "MemTotal: %ld kB", &info.totalMemory) == 1) {
                info.totalMemory *= 1024;  // KB から Bytes に変換
            } else if (sscanf(line, "MemAvailable: %ld kB", &info.freeMemory) == 1) {
                info.freeMemory *= 1024;
            }
        }
        fclose(memFile);
        info.usedMemory = info.totalMemory - info.freeMemory;
    }

    return info;
}

std::string DebugHUD::formatMemorySize(long bytes) const {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(0);

    if (bytes < 1024) {
        ss << bytes << " B";
    } else if (bytes < 1024 * 1024) {
        ss << bytes / 1024.0f << " KB";
    } else if (bytes < 1024 * 1024 * 1024) {
        ss << bytes / (1024.0f * 1024.0f) << " MB";
    } else {
        ss << bytes / (1024.0f * 1024.0f * 1024.0f) << " GB";
    }

    return ss.str();
}

std::string DebugHUD::getAudioStatus() const {
    std::stringstream ss;

    if (!audioManager) {
        ss << "Audio: [disabled]";
        return ss.str();
    }

    // Audio system disabled - JNI bridge not fully implemented
    // size_t clipCount = audioManager->getLoadedClipsCount();
    // size_t sourceCount = audioManager->getActiveSourcesesCount();
    // bool bgmPlaying = audioManager->isBGMPlaying();
    // ss << "Audio: clips=" << clipCount << " sources=" << sourceCount;
    // if (bgmPlaying) {
    //     ss << " [BGM playing]";
    // }

    ss << "Audio: [not available]";
    return ss.str();
}

std::string DebugHUD::getRetroFilterStatus() const {
    std::stringstream ss;

    if (!renderer) {
        return "";
    }

    auto settings = renderer->getRetroSettings();
    if (!settings) {
        return "";
    }

    // 有効なエフェクトを列挙
    std::string activeEffects;
    if (settings->scanlines_enabled) activeEffects += "S";
    if (settings->pixelation_enabled) activeEffects += "P";
    if (settings->color_reduction_enabled) activeEffects += "C";
    if (settings->crt_distortion_enabled) activeEffects += "D";
    if (settings->grain_enabled) activeEffects += "G";

    if (activeEffects.empty()) {
        ss << "Filters: [none active]";
    } else {
        ss << "Filters: " << activeEffects;
        ss << " (S=scanlines P=pixelation C=color D=distortion G=grain)";
    }

    return ss.str();
}
