#pragma once

#include <unordered_map>
#include <string>
#include <memory>

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

    // Game Setting (GMST) strings keyed by editor ID (e.g. "sContinue").
    // These come from the JPWiki plugin data and cover the original game UI.
    std::unordered_map<std::string, std::string> gameSettings;

    // JPWiki text keyed by FormID, for records whose text lives in the plugin
    // rather than in a Game Setting. Keys are lowercase 8-digit hex FormIDs.
    std::unordered_map<std::string, std::string> bookTexts;    // BOOK DESC
    std::unordered_map<std::string, std::string> infoTexts;    // INFO NAM1
    std::unordered_map<std::string, std::string> dialogTexts;  // DIAL FULL
    std::unordered_map<std::string, std::string> questTexts;   // QUST FULL
    std::unordered_map<std::string, std::string> fullTexts;    // "<RECTYPE>:<FormID>" FULL

    // Current language setting
    Language currentLanguage;

    // Preference key for persistent storage
    static constexpr const char* PREF_LANGUAGE_KEY = "language_preference";
    static constexpr int DEFAULT_LANGUAGE = static_cast<int>(Language::ENGLISH);

    // Asset path of the JPWiki localization data (tab separated, UTF-8).
    static constexpr const char* JPWIKI_DATA_ASSET = "localization/jpwiki_localization.tsv";

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

    // Game Setting lookup. Returns the Japanese text when the current language
    // is Japanese and the key is present, otherwise the supplied fallback.
    std::string getGameSetting(const std::string& editorID,
                               const std::string& fallback = std::string()) const;

    // Record text lookups. Each returns the Japanese text when the current
    // language is Japanese and the FormID is present, otherwise the fallback.
    std::string getBookText(uint32_t formID, const std::string& fallback = std::string()) const;
    std::string getInfoText(uint32_t formID, const std::string& fallback = std::string()) const;
    std::string getDialogText(uint32_t formID, const std::string& fallback = std::string()) const;
    std::string getQuestText(uint32_t formID, const std::string& fallback = std::string()) const;
    std::string getFullText(const std::string& recordType, uint32_t formID,
                            const std::string& fallback = std::string()) const;

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

    // Load the JPWiki localization data from the APK assets.
    void loadJpwikiData();
};

// Singleton accessor (optional, can be replaced with dependency injection)
LocalizationManager* getLocalizationManager();
void setLocalizationManager(LocalizationManager* manager);
