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

    // Save/Load UI (Phase 4D-UI)
    translations["save_menu_title"] = {"Save Game", "ゲームを保存"};
    translations["load_menu_title"] = {"Load Game", "ゲームをロード"};
    translations["autosave_title"] = {"Auto-Save", "オートセーブ"};
    translations["autosave_loading"] = {"Loading Auto-Save...", "オートセーブを読み込み中..."};
    translations["save_name_input_title"] = {"Enter Save Name", "セーブ名を入力"};
    translations["save_name_instruction"] = {"Enter a name for this save (Max 20 characters)", "セーブ名を入力してください（最大20文字）"};
    translations["save_confirm_title"] = {"Confirm Save", "セーブ確認"};
    translations["save_confirm_message"] = {"Overwrite this save slot?", "このスロットに上書きしますか？"};
    translations["load_confirm_title"] = {"Confirm Load", "ロード確認"};
    translations["load_confirm_message"] = {"Load this save?", "このセーブをロードしますか？"};
    translations["delete_confirm_title"] = {"Confirm Delete", "削除確認"};
    translations["delete_confirm_message"] = {"Delete this save permanently?", "このセーブを削除してもよろしいですか？"};

    // Buttons
    translations["button_yes"] = {"Yes", "はい"};
    translations["button_no"] = {"No", "いいえ"};
    translations["button_ok"] = {"OK", "決定"};
    translations["button_cancel"] = {"Cancel", "キャンセル"};
    translations["button_back"] = {"Back", "戻る"};
    translations["button_delete"] = {"Delete", "削除"};
    translations["button_load"] = {"Load", "ロード"};
    translations["button_save"] = {"Save", "保存"};

    // Save Slot Status
    translations["slot_empty"] = {"[Empty]", "[空]"};
    translations["slot_character"] = {"Character: ", "キャラ: "};
    translations["slot_level"] = {"Lv.", "Lv."};
    translations["slot_location"] = {"Location: ", "場所: "};
    translations["slot_playtime"] = {"Playtime: ", "プレイ時間: "};
    translations["slot_save_time"] = {"Saved: ", "セーブ: "};

    // MOD Support Translations (Phase 4E)
    translations["mod_active_mods"] = {"Active Mods", "有効なMOD"};
    translations["mod_no_mods"] = {"No mods active", "有効なMODなし"};

    // MOD Names
    translations["mod_cm_partners"] = {"CM Partners", "CM パートナー"};
    translations["mod_house_cats"] = {"Maigrets House Cats", "メグレの家の猫"};
    translations["mod_female_servants"] = {"More Female Servants", "女性召使い追加"};
    translations["mod_mad_spells"] = {"MadCompanionship Spells", "マッドコンパニオン呪文"};
    translations["mod_custom_save"] = {"Custom Save Rooms", "カスタムセーブルーム"};
    translations["mod_carry_capacity"] = {"Carry Capacity Mod", "運搬容量MOD"};

    // MOD Related Status
    translations["mod_companion_active"] = {"Companion Active", "コンパニオン有効"};
    translations["mod_pet_active"] = {"Pet Active", "ペット有効"};
    translations["mod_spells_learned"] = {"Spells Learned", "習得呪文"};
    translations["mod_balance_applied"] = {"Balance Applied", "バランス適用中"};

    // MOD Information Display
    translations["mod_companion_name"] = {"Companion", "コンパニオン"};
    translations["mod_companion_relationship"] = {"Relationship", "関係度"};
    translations["mod_pet_name"] = {"Pet", "ペット"};
    translations["mod_pet_type"] = {"Pet Type", "ペット種別"};
    translations["mod_carry_multiplier"] = {"Carry Capacity x", "運搬容量 x"};
    translations["mod_damage_multiplier"] = {"Damage Multiplier x", "ダメージ倍率 x"};
    translations["mod_health_regen"] = {"Health Regen Rate x", "体力回復速度 x"};

    // ============================================================
    // Phase 4: Dialogue System (会話システム)
    // ============================================================

    // Dialogue Options (Common)
    translations["dialogue_option_quest"] = {"Tell me about quests", "クエストについて話してください"};
    translations["dialogue_option_followme"] = {"Follow me", "私について来てください"};
    translations["dialogue_option_trade"] = {"Let's trade", "取引しましょう"};
    translations["dialogue_option_news"] = {"Any news?", "何か情報がありますか？"};
    translations["dialogue_option_help"] = {"Can you help?", "手伝ってもらえますか？"};
    translations["dialogue_option_spells"] = {"Tell me about spells", "呪文について教えてください"};
    translations["dialogue_option_magic"] = {"Teach me magic", "魔法を教えてください"};
    translations["dialogue_option_accept"] = {"I accept", "受け入れます"};
    translations["dialogue_option_decline"] = {"I decline", "遠慮します"};
    translations["dialogue_option_goodbye"] = {"Goodbye", "さようなら"};
    translations["dialogue_option_bye"] = {"I must go", "行かなければなりません"};

    // Companion Dialogue
    translations["dialogue_companion_greeting"] = {
        "Hello, adventurer! It's good to see you again.",
        "こんにちは、冒険者様！お久しぶりですね。"
    };

    translations["dialogue_companion_quest"] = {
        "I need your help with something important. A mysterious artifact was stolen from the village. "
        "Would you help me recover it?",
        "重要なことであなたの助けが必要です。村から神秘的な遺物が盗まれました。"
        "それを取り戻すのを手伝ってくれませんか？"
    };

    translations["dialogue_companion_accepted"] = {
        "Thank you! You're a true hero. The artifact was last seen near the ancient ruins to the north. "
        "Be careful - dangerous creatures roam there.",
        "ありがとうございます！あなたは本当のヒーローです。遺物は北の古い遺跡の近くで最後に目撃されました。"
        "気をつけてください。危険な生き物が出没しています。"
    };

    translations["dialogue_companion_declined"] = {
        "I understand. It's a dangerous task. If you change your mind, I'll be here.",
        "わかります。危険な仕事ですから。気が変わったら、ここにいます。"
    };

    translations["dialogue_companion_follow"] = {
        "I'll follow you wherever you go, my friend. Let's make the world a better place together!",
        "どこへ行こうとも、あなたについていきます。一緒にこの世界をより良い場所にしましょう！"
    };

    translations["dialogue_companion_goodbye"] = {
        "Safe travels, friend. We'll meet again.",
        "安全な旅をお祈りします、友よ。また会いましょう。"
    };

    // Merchant Dialogue
    translations["dialogue_merchant_greeting"] = {
        "Welcome to my shop! I have the finest wares in all the land.",
        "私の店へようこそ！この地で最高の商品を揃えています。"
    };

    translations["dialogue_merchant_trade"] = {
        "I have powerful weapons, protective armor, and rare potions. What catches your eye?",
        "強力な武器、防具、そして希少なポーションを揃えています。何かお目に留まりましたか？"
    };

    translations["dialogue_merchant_news"] = {
        "Business is good! I hear rumors of treasure hidden in the mountains. "
        "Some say a dragon guards it. Interested?",
        "商売は好調です！山に隠された宝物の噂を聞きました。"
        "ドラゴンがそれを守っているという者もいます。興味ありますか？"
    };

    translations["dialogue_merchant_goodbye"] = {
        "Come again soon! Your gold is always welcome here.",
        "また来てください！あなたの金は常に歓迎されます。"
    };

    // Guard Dialogue
    translations["dialogue_guard_greeting"] = {
        "Halt! State your business. Are you here to cause trouble?",
        "止まりなさい！用件を言いなさい。ここで問題を起こすつもりですか？"
    };

    translations["dialogue_guard_help"] = {
        "Help? Well, we could use assistance dealing with the bandits to the east. "
        "They've been causing problems for travelers.",
        "手助け？東の盗賊団を相手にするのに助力が必要です。"
        "彼らは旅人に問題を起こしています。"
    };

    translations["dialogue_guard_news"] = {
        "Things have been quiet lately, thank the gods. Though we've had reports of strange "
        "creatures in the nearby forest.",
        "最近は静かなものです。ありがたいことに。ただし、近くの森で奇妙な生き物が目撃されているという報告があります。"
    };

    translations["dialogue_guard_goodbye"] = {
        "Move along then. Keep your eyes open out there.",
        "さあ、どうぞ。外では気をつけてください。"
    };

    // Mage Dialogue
    translations["dialogue_mage_greeting"] = {
        "Greetings, seeker of knowledge. Welcome to my tower of arcane wisdom.",
        "知識を求める者よ、ようこそ。私の秘術の知識の塔へようこそ。"
    };

    translations["dialogue_mage_spells"] = {
        "I possess spells of great power - Fireball to incinerate your foes, "
        "Heal to mend your wounds, and many others. Would you like to learn?",
        "私は強大な呪文を持っています。敵を焼き払うファイアボール、"
        "傷を癒すヒール、そして他の多くの呪文があります。学びたいですか？"
    };

    translations["dialogue_mage_magic"] = {
        "Magic is the key to unlocking the world's mysteries. With proper study and discipline, "
        "you too can harness its power.",
        "魔法は世界の謎を解き明かすカギです。適切な研究と修行があれば、"
        "あなたもその力を駆使することができます。"
    };

    translations["dialogue_mage_goodbye"] = {
        "May your journey be illuminated by arcane light. Return anytime.",
        "あなたの旅が秘術の光で照らされるように。いつでも戻ってください。"
    };

    LOGI("Loaded %zu translations into database", translations.size());
}

LocalizationManager* getLocalizationManager() {
    return g_localizationManager;
}

void setLocalizationManager(LocalizationManager* manager) {
    g_localizationManager = manager;
}
