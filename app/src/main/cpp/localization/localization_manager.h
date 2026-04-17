#pragma once

#include <unordered_map>
#include <string>
#include <memory>
#include <android/log.h>

#define LOG_TAG "LocalizationManager"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

/**
 * LocalizationManager
 * Manages Japanese/English language switching for game UI and content
 * Follows the Manager pattern: initialize() -> update(deltaTime) -> cleanup()
 */

enum class Language {
    ENGLISH = 0,
    JAPANESE = 1
};

class LocalizationManager {
private:
    // Language strings database
    // Key -> {English, Japanese}
    std::unordered_map<std::string, std::pair<std::string, std::string>> translations;

    // Current language setting
    Language currentLanguage;

    // Preference key for persistent storage
    static constexpr const char* PREF_LANGUAGE_KEY = "language_preference";
    static constexpr int DEFAULT_LANGUAGE = static_cast<int>(Language::ENGLISH);

public:
    LocalizationManager();
    ~LocalizationManager();

    // Lifecycle management
    bool initialize();
    void cleanup();

    // Language selection
    void setLanguage(Language lang);
    Language getLanguage() const { return currentLanguage; }

    // String retrieval
    std::string getString(const std::string& key);

    // Batch translation loading
    void loadTranslations();

    // Helper method to load preference from file/SharedPreferences
    void loadLanguagePreference();
    void saveLanguagePreference();

    // Get language name
    std::string getLanguageName() const;

    // Debug/logging
    void logTranslationStats() const;

private:
    // Initialize translation database with hardcoded strings
    void initializeTranslationDatabase();
};

// Singleton accessor (optional, can be replaced with dependency injection)
LocalizationManager* getLocalizationManager();
void setLocalizationManager(LocalizationManager* manager);
