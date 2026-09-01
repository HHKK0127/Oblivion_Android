#pragma once

#include <string>
#include <android/log.h>

#undef LOG_TAG
#undef LOGD
#undef LOGW
#undef LOGE
#undef LOGI

/**
 * @brief Game settings management system
 * Manages various game settings (language, debug mode, etc.)
 */
class SettingsManager {
public:
    SettingsManager();
    ~SettingsManager();

    /**
     * @brief Initialize settings
     */
    bool initialize();

    /**
     * @brief Enable/disable debug mode
     */
    void setDebugMode(bool enabled);

    /**
     * @brief Check if debug mode is enabled
     */
    bool isDebugModeEnabled() const { return debugModeEnabled; }

    /**
     * @brief Set language ("ja" = Japanese, "en" = English)
     */
    void setLanguage(const std::string& lang);

    /**
     * @brief Get current language
     */
    std::string getLanguage() const { return currentLanguage; }

    /**
     * @brief Save settings
     */
    void saveSettings();

    /**
     * @brief Load settings
     */
    void loadSettings();

    /**
     * @brief Reset settings
     */
    void resetToDefaults();

    /**
     * @brief クリーンアップ
     */
    void cleanup();

private:
    bool debugModeEnabled;
    std::string currentLanguage;

    // ファイルパス
    std::string getSettingsFilePath() const;

    static constexpr const char* LOG_TAG = "SettingsManager";
    #define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
    #define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
};
