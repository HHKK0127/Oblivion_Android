#include "localization_manager.h"
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cctype>
#include <android/log.h>
#include <android/asset_manager.h>

#define LOG_TAG "LocalizationManager"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

extern "C" AAssetManager* jni_audio_get_asset_manager();

static LocalizationManager* g_localizationManager = nullptr;

LocalizationManager::LocalizationManager()
    : currentLanguage(Language::ENGLISH) {
    LOGD("LocalizationManager created");
}

LocalizationManager::~LocalizationManager() {
    cleanup();
    LOGD("LocalizationManager destroyed");
}

bool LocalizationManager::initialize() {
    LOGI("LocalizationManager initializing");

    // Initialize translation database
    initializeTranslationDatabase();

    // Load the JPWiki game setting data from the APK assets
    loadJpwikiData();

    // Load language preference from persistent storage
    loadLanguagePreference();

    LOGI("LocalizationManager initialized with %zu translations, %zu game settings",
         translations.size(), gameSettings.size());
    return true;
}

void LocalizationManager::cleanup() {
    // Save current language preference
    saveLanguagePreference();

    translations.clear();
    gameSettings.clear();
    bookTexts.clear();
    infoTexts.clear();
    dialogTexts.clear();
    questTexts.clear();
    fullTexts.clear();
    LOGD("LocalizationManager cleaned up");
}

void LocalizationManager::setLanguage(Language lang) {
    currentLanguage = lang;
    saveLanguagePreference();

    const char* langName = (lang == Language::JAPANESE) ? "Japanese" : "English";
    LOGI("Language switched to: %s", langName);
}

std::string LocalizationManager::getString(const std::string& key) {
    auto it = translations.find(key);
    if (it == translations.end()) {
        LOGW("Translation key not found: %s", key.c_str());
        return key;  // Return key as fallback
    }

    if (currentLanguage == Language::JAPANESE) {
        return it->second.second;
    } else {
        return it->second.first;
    }
}

std::string LocalizationManager::getGameSetting(const std::string& editorID,
                                                const std::string& fallback) const {
    if (currentLanguage != Language::JAPANESE) {
        return fallback;
    }
    auto it = gameSettings.find(editorID);
    if (it == gameSettings.end()) {
        return fallback;
    }
    return it->second;
}

namespace {

// FormID keys are stored as lowercase 8-digit hex so lookups are stable
// regardless of how the caller obtained the ID.
std::string formKey(uint32_t formID) {
    char buf[9];
    snprintf(buf, sizeof(buf), "%08x", formID);
    return std::string(buf);
}

std::string lookup(const std::unordered_map<std::string, std::string>& table,
                   const std::string& key, const std::string& fallback) {
    auto it = table.find(key);
    return (it == table.end()) ? fallback : it->second;
}

}  // namespace

std::string LocalizationManager::getBookText(uint32_t formID,
                                             const std::string& fallback) const {
    if (currentLanguage != Language::JAPANESE) return fallback;
    return lookup(bookTexts, formKey(formID), fallback);
}

std::string LocalizationManager::getInfoText(uint32_t formID,
                                             const std::string& fallback) const {
    if (currentLanguage != Language::JAPANESE) return fallback;
    return lookup(infoTexts, formKey(formID), fallback);
}

std::string LocalizationManager::getDialogText(uint32_t formID,
                                               const std::string& fallback) const {
    if (currentLanguage != Language::JAPANESE) return fallback;
    return lookup(dialogTexts, formKey(formID), fallback);
}

std::string LocalizationManager::getQuestText(uint32_t formID,
                                              const std::string& fallback) const {
    if (currentLanguage != Language::JAPANESE) return fallback;
    return lookup(questTexts, formKey(formID), fallback);
}

std::string LocalizationManager::getFullText(const std::string& recordType, uint32_t formID,
                                             const std::string& fallback) const {
    if (currentLanguage != Language::JAPANESE) return fallback;
    return lookup(fullTexts, recordType + ":" + formKey(formID), fallback);
}

void LocalizationManager::loadLanguagePreference() {
    // The language preference is owned by SettingsManager, which persists it to
    // the app settings file. LocalizationManager mirrors that value so the two
    // stay in sync; see Renderer::initLocalization().
    LOGD("Language preference loaded: %s",
         (currentLanguage == Language::JAPANESE) ? "Japanese" : "English");
}

void LocalizationManager::saveLanguagePreference() {
    // Persistence is handled by SettingsManager::saveSettings().
    LOGD("Language preference saved: %s",
         (currentLanguage == Language::JAPANESE) ? "Japanese" : "English");
}

std::string LocalizationManager::getLanguageName() const {
    return (currentLanguage == Language::JAPANESE) ? "日本語" : "English";
}

void LocalizationManager::logTranslationStats() const {
    LOGD("========== Localization Manager Status ==========");
    LOGD("Current Language: %s", getLanguageName().c_str());
    LOGD("Total Translations: %zu", translations.size());
    LOGD("Total Game Settings: %zu", gameSettings.size());
    LOGD("Total Record Texts: book=%zu info=%zu dial=%zu qst=%zu full=%zu",
         bookTexts.size(), infoTexts.size(), dialogTexts.size(),
         questTexts.size(), fullTexts.size());
    LOGD("================================================");
}

void LocalizationManager::loadJpwikiData() {
    AAssetManager* mgr = jni_audio_get_asset_manager();
    if (!mgr) {
        LOGW("AAssetManager unavailable, skipping JPWiki data");
        return;
    }

    AAsset* asset = AAssetManager_open(mgr, JPWIKI_DATA_ASSET, AASSET_MODE_BUFFER);
    if (!asset) {
        LOGW("JPWiki data asset not found: %s", JPWIKI_DATA_ASSET);
        return;
    }

    off_t len = AAsset_getLength(asset);
    std::string content;
    content.resize(static_cast<size_t>(len));
    int read = AAsset_read(asset, &content[0], static_cast<size_t>(len));
    AAsset_close(asset);

    if (read <= 0) {
        LOGE("Failed to read JPWiki data asset");
        return;
    }
    content.resize(static_cast<size_t>(read));

    std::istringstream stream(content);
    std::string line;
    size_t loaded = 0;
    while (std::getline(stream, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }
        // kind \t key \t english \t japanese
        size_t t1 = line.find('\t');
        if (t1 == std::string::npos) {
            continue;
        }
        size_t t2 = line.find('\t', t1 + 1);
        if (t2 == std::string::npos) {
            continue;
        }
        size_t t3 = line.find('\t', t2 + 1);
        if (t3 == std::string::npos) {
            continue;
        }
        const std::string kind = line.substr(0, t1);
        std::string key = line.substr(t1 + 1, t2 - t1 - 1);
        const std::string japanese = line.substr(t3 + 1);
        if (key.empty() || japanese.empty()) {
            continue;
        }
        // FormID keys are normalized to lowercase hex so they match formKey().
        if (kind != "gmst") {
            for (char& c : key) {
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            }
        }
        if (kind == "gmst") {
            gameSettings[key] = japanese;
        } else if (kind == "book") {
            bookTexts[key] = japanese;
        } else if (kind == "info") {
            infoTexts[key] = japanese;
        } else if (kind == "dial") {
            dialogTexts[key] = japanese;
        } else if (kind == "qst") {
            questTexts[key] = japanese;
        } else if (kind == "full") {
            fullTexts[key] = japanese;
        } else {
            continue;
        }
        ++loaded;
    }

    LOGI("Loaded %zu JPWiki entries from %s (gmst=%zu book=%zu info=%zu dial=%zu qst=%zu full=%zu)",
         loaded, JPWIKI_DATA_ASSET, gameSettings.size(), bookTexts.size(),
         infoTexts.size(), dialogTexts.size(), questTexts.size(), fullTexts.size());
}

void LocalizationManager::initializeTranslationDatabase() {
    // UI - Menu Items (Original Oblivion title screen style)
    translations["menu_continue"] = {"Continue", "続ける"};
    translations["menu_new"] = {"New", "新規"};
    translations["menu_load"] = {"Load", "ロード"};
    translations["menu_options"] = {"Options", "オプション"};
    translations["menu_credits"] = {"Credits", "クレジット"};
    translations["menu_exit"] = {"Exit", "終了"};
    translations["menu_quit"] = {"Quit", "終了"};
    translations["menu_debug"] = {"Debug", "デバッグ"};
    translations["menu_start"] = {"Start", "ゲーム開始"};
    translations["menu_settings"] = {"Settings", "設定"};
    translations["menu_resume"] = {"Resume", "再開"};
    translations["menu_save"] = {"Save Game", "ゲームを保存"};

    // Launcher menu items (Oblivion PC launcher style)
    translations["launcher_play"] = {"Play", "プレイ"};
    translations["launcher_options"] = {"Options", "オプション"};
    translations["launcher_data_files"] = {"Data Files", "データファイル"};
    translations["launcher_website"] = {"Elderscrolls.com", "Elderscrolls.com"};
    translations["launcher_technical_support"] = {"Technical Support", "テクニカルサポート"};
    translations["launcher_support"] = {"Support", "サポート"};
    translations["launcher_uninstall"] = {"Uninstall", "アンインストール"};
    translations["launcher_exit"] = {"Exit", "終了"};

    // UI - Language Menu
    translations["menu_language"] = {"Language", "言語"};
    translations["language_english"] = {"English", "英語"};
    translations["language_japanese"] = {"Japanese", "日本語"};

    // UI - Quest System
    translations["quest_log"] = {"Quest Log", "クエストログ"};
    translations["quest_detail"] = {"Quest Details", "クエスト詳細"};
    translations["quest_status_active"] = {"Active", "進行中"};
    translations["quest_status_completed"] = {"Completed", "完了"};
    translations["quest_status_failed"] = {"Failed", "失敗"};
    translations["quest_objectives"] = {"Objectives", "目標"};
    translations["quest_reward"] = {"Reward", "報酬"};

    // UI - NPC Interaction
    translations["npc_greet"] = {"Greetings", "こんにちは"};
    translations["npc_talk"] = {"Talk", "話す"};
    translations["npc_available_quests"] = {"Available Quests", "利用可能なクエスト"};
    translations["npc_accept"] = {"Accept", "受け入れる"};
    translations["npc_decline"] = {"Decline", "断る"};
    translations["npc_goodbye"] = {"Goodbye", "さようなら"};

    // Combat System
    translations["combat_engage"] = {"Combat Engaged", "戦闘開始"};
    translations["combat_victory"] = {"Victory", "勝利"};
    translations["combat_defeat"] = {"Defeat", "敗北"};
    translations["damage_dealt"] = {"Damage Dealt", "ダメージを与えた"};
    translations["health_points"] = {"Health", "Health"};
    translations["mana_points"] = {"Magicka", "Magicka"};
    translations["stamina_points"] = {"Fatigue", "Fatigue"};

    // Magic System (for M5-3)
    translations["magic_spell"] = {"Spell", "呪文"};
    translations["magic_cast"] = {"Cast", "発動"};
    translations["magic_school_alteration"] = {"Alteration", "変化の魔法"};
    translations["magic_school_conjuration"] = {"Conjuration", "召喚の魔法"};
    translations["magic_school_destruction"] = {"Destruction", "破壊の魔法"};
    translations["magic_school_illusion"] = {"Illusion", "幻覚の魔法"};
    translations["magic_school_mysticism"] = {"Mysticism", "神秘の魔法"};
    translations["magic_school_restoration"] = {"Restoration", "回復の魔法"};

    // Spells (M5-3)
    translations["spell_fireball"] = {"Fireball", "ファイアボール"};
    translations["spell_heal"] = {"Heal", "ヒール"};
    translations["spell_restore_mana"] = {"Restore Magicka", "Magicka回復"};
    translations["spell_restore_stamina"] = {"Restore Fatigue", "Fatigue回復"};
    translations["spell_paralyze"] = {"Paralyze", "麻痺"};
    translations["spell_invisibility"] = {"Invisibility", "姿を隠す"};
    translations["spell_summon"] = {"Summon", "召喚"};
    translations["spell_fortify"] = {"Fortify Attribute", "属性強化"};

    // Magic Effects
    translations["effect_damage"] = {"Damage", "ダメージ"};
    translations["effect_heal"] = {"Healing", "回復"};
    translations["effect_mana"] = {"Magicka Effect", "Magicka効果"};
    translations["effect_buff"] = {"Fortification", "強化"};
    translations["effect_debuff"] = {"Weakness", "弱体化"};

    // Combat AI Messages
    translations["ai_casting_spell"] = {"Casting spell", "呪文を発動"};
    translations["ai_low_health"] = {"Low health", "Healthが低い"};
    translations["ai_low_mana"] = {"Low magicka", "Magickaが不足"};
    translations["ai_selecting_heal"] = {"Selecting healing spell", "回復呪文を選択"};
    translations["ai_selecting_damage"] = {"Selecting damage spell", "攻撃呪文を選択"};
    translations["ai_selecting_restore"] = {"Selecting restore spell", "回復系呪文を選択"};

    // Test Quests
    translations["quest_kill_monster"] = {"Kill the Monster", "怪物を倒す"};
    translations["quest_collect_items"] = {"Collect Items", "アイテムを集める"};
    translations["quest_deliver_message"] = {"Deliver Message", "メッセージを配達する"};

    // Test NPC Names
    translations["npc_izar"] = {"Izar", "イザール"};
    translations["npc_hellas"] = {"Hellas", "ヘラス"};
    translations["npc_merchant"] = {"Merchant", "商人"};
    translations["npc_guard"] = {"Guard", "衛兵"};
    translations["npc_mage"] = {"Mage", "魔術師"};

    // Attributes (Character Stats)
    translations["attr_strength"] = {"Strength", "力"};
    translations["attr_intelligence"] = {"Intelligence", "知性"};
    translations["attr_willpower"] = {"Willpower", "意志力"};
    translations["attr_agility"] = {"Agility", "敏捷性"};
    translations["attr_speed"] = {"Speed", "速度"};
    translations["attr_endurance"] = {"Endurance", "耐久力"};
    translations["attr_personality"] = {"Personality", "魅力"};
    translations["attr_luck"] = {"Luck", "運"};

    // Items/Equipment
    translations["item_iron_sword"] = {"Iron Sword", "鉄の剣"};
    translations["item_steel_armor"] = {"Steel Armor", "鋼の鎧"};
    translations["item_health_potion"] = {"Health Potion", "Health回復薬"};
    translations["item_mana_potion"] = {"Magicka Potion", "Magicka回復薬"};
    translations["item_gold"] = {"Gold", "ゴールド"};

    // General Messages
    translations["message_welcome"] = {"Welcome to Oblivion", "オブリビオンへようこそ"};
    translations["message_loading"] = {"Loading...", "ロード中..."};
    translations["message_error"] = {"Error", "エラー"};
    translations["message_success"] = {"Success", "成功"};
    translations["message_back"] = {"Back", "戻る"};
    translations["message_confirm"] = {"Confirm", "確認"};
    translations["message_cancel"] = {"Cancel", "取り消し"};

    LOGI("Loaded %zu translations into database", translations.size());
}

LocalizationManager* getLocalizationManager() {
    return g_localizationManager;
}

void setLocalizationManager(LocalizationManager* manager) {
    g_localizationManager = manager;
}
