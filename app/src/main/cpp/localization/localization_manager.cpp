#include "localization_manager.h"
#include <fstream>
#include <sstream>

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

    // Load language preference from persistent storage
    loadLanguagePreference();

    LOGI("LocalizationManager initialized with %zu translations", translations.size());
    return true;
}

void LocalizationManager::cleanup() {
    // Save current language preference
    saveLanguagePreference();

    translations.clear();
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

void LocalizationManager::loadLanguagePreference() {
    // TODO: Implement persistent storage via Android SharedPreferences
    // For now, default to English
    currentLanguage = Language::ENGLISH;
    LOGD("Language preference loaded: %s",
         (currentLanguage == Language::JAPANESE) ? "Japanese" : "English");
}

void LocalizationManager::saveLanguagePreference() {
    // TODO: Implement persistent storage via Android SharedPreferences
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
    LOGD("================================================");
}

void LocalizationManager::initializeTranslationDatabase() {
    // UI - Menu Items
    translations["menu_start"] = {"Start", "ゲーム開始"};
    translations["menu_settings"] = {"Settings", "設定"};
    translations["menu_quit"] = {"Quit", "終了"};
    translations["menu_resume"] = {"Resume", "再開"};
    translations["menu_load"] = {"Load Game", "ゲームをロード"};
    translations["menu_save"] = {"Save Game", "ゲームを保存"};

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
    translations["health_points"] = {"Health", "体力"};
    translations["mana_points"] = {"Mana", "マナ"};
    translations["stamina_points"] = {"Stamina", "スタミナ"};

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
    translations["spell_restore_mana"] = {"Restore Mana", "マナ回復"};
    translations["spell_restore_stamina"] = {"Restore Stamina", "スタミナ回復"};
    translations["spell_paralyze"] = {"Paralyze", "麻痺"};
    translations["spell_invisibility"] = {"Invisibility", "姿を隠す"};
    translations["spell_summon"] = {"Summon", "召喚"};
    translations["spell_fortify"] = {"Fortify Attribute", "属性強化"};

    // Magic Effects
    translations["effect_damage"] = {"Damage", "ダメージ"};
    translations["effect_heal"] = {"Healing", "回復"};
    translations["effect_mana"] = {"Mana Effect", "マナ効果"};
    translations["effect_buff"] = {"Fortification", "強化"};
    translations["effect_debuff"] = {"Weakness", "弱体化"};

    // Combat AI Messages
    translations["ai_casting_spell"] = {"Casting spell", "呪文を発動"};
    translations["ai_low_health"] = {"Low health", "体力が低い"};
    translations["ai_low_mana"] = {"Low mana", "マナが不足"};
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
    translations["item_health_potion"] = {"Health Potion", "体力回復薬"};
    translations["item_mana_potion"] = {"Mana Potion", "マナ回復薬"};
    translations["item_gold"] = {"Gold", "ゴールド"};

    // General Messages
    translations["message_welcome"] = {"Welcome to Oblivion", "オブリビオンへようこそ"};
    translations["message_loading"] = {"Loading...", "ロード中..."};
    translations["message_error"] = {"Error", "エラー"};
    translations["message_success"] = {"Success", "成功"};
    translations["message_back"] = {"Back", "戻る"};
    translations["message_confirm"] = {"Confirm", "確認"};
    translations["message_cancel"] = {"Cancel", "キャンセル"};

    LOGI("Loaded %zu translations into database", translations.size());
}

LocalizationManager* getLocalizationManager() {
    return g_localizationManager;
}

void setLocalizationManager(LocalizationManager* manager) {
    g_localizationManager = manager;
}
