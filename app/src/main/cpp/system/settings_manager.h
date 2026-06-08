#pragma once

#include <string>
#include <android/log.h>

#undef LOG_TAG
#undef LOGD
#undef LOGW
#undef LOGE
#undef LOGI

/**
 * @brief ゲーム設定管理システム
 * ゲームの各種設定（言語、デバッグモードなど）を管理します
 */
class SettingsManager {
public:
    SettingsManager();
    ~SettingsManager();

    /**
     * @brief 設定を初期化
     */
    bool initialize();

    /**
     * @brief デバッグモードを有効/無効にする
     */
    void setDebugMode(bool enabled);

    /**
     * @brief デバッグモードが有効かどうかを取得
     */
    bool isDebugModeEnabled() const { return debugModeEnabled; }

    /**
     * @brief 言語を設定（"ja" = 日本語, "en" = 英語）
     */
    void setLanguage(const std::string& lang);

    /**
     * @brief 現在の言語を取得
     */
    std::string getLanguage() const { return currentLanguage; }

    /**
     * @brief 設定を保存
     */
    void saveSettings();

    /**
     * @brief 設定を読み込み
     */
    void loadSettings();

    /**
     * @brief 設定をリセット
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
