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

    // =========================================================================
    // Books - Titles and Content
    // =========================================================================

    // Book 1: The Elder Scrolls (3 pages)
    translations["book_elder_scrolls_title"] = {"The Elder Scrolls", "エルダースクロール"};
    translations["book_elder_scrolls_page1"] = {
        "The Elder Scrolls speak of a time when dragons ruled the skies and mortals trembled beneath their shadow. "
        "Alduin, the World-Eater, whose name alone strikes fear into the hearts of men and mer alike, "
        "shall return when the wheel turns upon the Last Dragonborn.",
        "エルダースクロールは、竜が空を支配し、人々がその影の下で震えていた時代を語っている。 "
        "世界を食らう者アルドゥイン、その名だけで人間もエルフも恐れる彼が、 "
        "最後の竜裔が現れるとき、帰ってくると書かれている。"
    };
    translations["book_elder_scrolls_page2"] = {
        "But there is a force older than the scrolls themselves, a power that sleeps beneath the mountains "
        "and dreams in the tongues of forgotten gods. When the scrolls are unrolled, the world shall tremble "
        "and the stars shall weep fire upon the earth.",
        "しかし、スクロール自体よりも古い力が存在する。山々の下で眠り、 "
        "忘れられた神々の言葉で夢を見る力。スクロールが開かれるとき、世界は震え、 "
        "星々は地上に火の涙を流すだろう。"
    };
    translations["book_elder_scrolls_page3"] = {
        "So it is written, and so it shall be. The Dragonborn comes, and with them, the end of all things "
        "or the beginning of a new age. Only time, that relentless river, shall tell.",
        "アルドゥインの予言にそう書かれている。竜裔が来る。彼らと共に、すべての終わりか、 "
        "新しい時代の始まりか。語るべきは、容赦なく流れる時の川のみ。"
    };

    // Book 2: Nerevar Moon-and-Star (4 pages)
    translations["book_nerevar_title"] = {"Nerevar Moon-and-Star", "ネレヴァー月と星"};
    translations["book_nerevar_page1"] = {
        "In the First Era, when the Chimer walked the ashlands of Resdayn, there arose a leader "
        "whose wisdom was matched only by his courage. Indoril Nerevar, called Moon-and-Star, "
        "united the fractious clans beneath a banner of hope.",
        "第一紀、キメールがレスデインの灰の大地を歩いていた頃、知恵と勇気を兼ね備えた指導者が現れた。 "
        "インダリル・ネレヴァー、月と星と呼ばれる彼は、希望の旗の下で分断された氏族を団結させた。"
    };
    translations["book_nerevar_page2"] = {
        "The Dwemer, deep-elves of unnatural science, had delved too deep beneath Red Mountain. "
        "Their brass machines and steam-forges shook the earth, and Nerevar knew that peace could not last.",
        "不自然な科学を持つ深きエルフ、ドウェマーは、赤き山の下に深く掘りすぎていた。 "
        "彼らの真鍮の機械と蒸気の鍛冶場が大地を揺るがし、ネレヴァーは平和が長くは続かないことを知っていた。"
    };
    translations["book_nerevar_page3"] = {
        "At the Battle of Red Mountain, Nerevar faced the Dwarf King Dumac in single combat. "
        "Their clash shook the heavens, and when the dust settled, both lay fallen.",
        "赤き山の戦いで、ネレヴァーはドワーフ王デュマックと一騎打ちを行った。 "
        "彼らの衝突は天を揺るがし、砂埃が収まると、二人とも倒れていた。"
    };
    translations["book_nerevar_page4"] = {
        "Thus ended the life of the greatest Hortator, but his legacy endures. The Nerevarine shall return, "
        "prophesied by the ashlanders, to set right what was made wrong by gods and mortals alike.",
        "最も偉大なホルテーターの生涯はこうして終わったが、彼の遺産は残った。 "
        "アッシュランダーの予言によれば、ネレヴァリンは帰ってくる。神々と人間の両方によって間違えられたことを正すために。"
    };

    // Book 3: Song of the Tenpenny Towers (2 pages)
    translations["book_tenpenny_title"] = {"Song of the Tenpenny Towers", "テンペニータワーの歌"};
    translations["book_tenpenny_page1"] = {
        "Oh, Tenpenny Towers tall and grand, where lords and ladies dance hand in hand! "
        "The wine flows sweet, the music plays, and golden light adorns the days.",
        "ああ、高く壮大なテンペニータワーよ、そこでは貴族たちが手を取り合って踊る！ "
        "ワインは甘く流れ、音楽は奏でられ、金色の光が日々を飾る。"
    };
    translations["book_tenpenny_page2"] = {
        "But woe to those who climb too high, for pride precedes the fatal nigh. "
        "The towers fell, as towers do, when those within forget what's true.",
        "しかし、高く登りすぎた者には災いあれ。高ぶりは破滅の夜の前兆。 "
        "塔は崩れる、塔がそうするように、中の者が真実を忘れたとき。"
    };

    // Book 4: A Dance in Fire (3 pages)
    translations["book_dance_fire_title"] = {"A Dance in Fire", "炎の中の舞踏"};
    translations["book_dance_fire_page1"] = {
        "Decumus Scotti had never meant to find himself in Valenwood. The clerk from the Imperial City "
        "had taken a wrong turn somewhere between Bravil and the border, and now found himself surrounded "
        "by walking trees and talking apes.",
        "デクムス・スコッティは、故意にヴァレンウッドに来たわけではなかった。帝国都市の書記官は、 "
        "ブラヴィルと国境の間のどこかで道を間違え、今や歩く木々と話す猿に囲まれている。"
    };
    translations["book_dance_fire_page2"] = {
        "The Bosmer were hospitable enough, once they stopped laughing at his clothes. "
        "They fed him roasted boar and some kind of fermented milk that made his head spin "
        "and his tongue loose.",
        "ボスマーは、彼の服装の笑いが止まれば、十分に親切だった。 "
        "彼らは焼いた猪と、頭をくらくらさせ舌を緩ませる何かの発酵乳を彼に与えた。"
    };
    translations["book_dance_fire_page3"] = {
        "When the fire came, dancing across the forest canopy, Scotti understood why the Wood Elves "
        "smiled even in the face of destruction. For them, fire was not an end, but a beginning. "
        "A dance that cleared the old to make way for the new.",
        "炎が来て、森林の天蓋を踊り横切るとき、スコッティはなぜ木のエルフたちが破壊の前でも微笑むのかを理解した。 "
        "彼らにとって、火は終わりではなく始まりだった。古いものを掃除し、新しいもののために道を開ける舞踏。"
    };

    // Book 5: Beggar (2 pages)
    translations["book_beggar_title"] = {"Beggar", "乞食"};
    translations["book_beggar_page1"] = {
        "To be poor is to be invisible. The wealthy walk past you as if you were a shadow, "
        "a trick of the light, something not quite real enough to warrant attention.",
        "貧しいということは、見えない存在になるということだ。金持ちはあなたのそばを通り過ぎる。 "
        "あなたが影であり、光の戯れであり、注意に値するほど十分に実在しない何かであるかのように。"
    };
    translations["book_beggar_page2"] = {
        "But I have seen things the nobles never will. I have seen the city wake at dawn, "
        "watched the sun paint gold upon the cobblestones, heard secrets whispered in alleyways "
        "where the rich dare not tread.",
        "しかし、私は貴族たちが決して見ることのないものを見てきた。夜明けに目覚める街を見た。 "
        "太陽が石畳に金色を塗るのを見た。金持ちが踏み入ることを恐れる路地で囁かれる秘密を聞いた。"
    };

    // Book 6: The Refugees (2 pages)
    translations["book_refugees_title"] = {"The Refugees", "難民たち"};
    translations["book_refugees_page1"] = {
        "They came across the sea in ships of sorrow, leaving behind lands swallowed by war. "
        "Men, women, and children huddled together, speaking in tongues that sounded like wind through broken glass.",
        "彼らは悲しみの船で海を渡って来た。戦争に飲まれた土地を後にして。 "
        "男、女、子供が身を寄せ合い、割れたガラスを通る風のような言葉で話した。"
    };
    translations["book_refugees_page2"] = {
        "The High Rock lords were not kind to strangers. 'No room,' they said, though their halls stood empty. "
        "'No food,' they claimed, while their granaries overflowed. But still the refugees came, for hope "
        "is the last thing to die in the human heart.",
        "ハイロックの領主たちは、見知らぬ人には親切ではなかった。『場所がない』と言った。 "
        "彼らの館が空っぽであるのに。『食料がない』と主張した。貯蔵庫があふれているのに。 "
        "しかし、難民たちはまだ来た。人間の心で最後に死ぬものは希望だから。"
    };

    // Book 7: Mysticism Spellcraft (3 pages)
    translations["book_mysticism_title"] = {"Mysticism Spellcraft", "神秘術の呪文作成"};
    translations["book_mysticism_page1"] = {
        "Mysticism is the school of magic least understood by the common practitioner. It deals with the manipulation "
        "of magical energy itself, the raw force that flows through all living things and binds the world together.",
        "神秘術は、一般の実践者にとって最も理解されていない魔法の学派である。 "
        "それは魔法エネルギーそのもの、すべての生き物を流れ世界を結びつける原初の力を扱う。"
    };
    translations["book_mysticism_page2"] = {
        "The spell 'Soul Trap' captures the animus of a dying creature, binding it to a gemstone for later use. "
        "This requires precise timing and a steady hand, for the soul must be caught at the exact moment of death.",
        "『魂の罠』の呪文は、死にゆく生き物のアニムスを捉え、後で使うために宝石に縛りつける。 "
        "これには正確なタイミングと落ち着いた手が必要だ。魂は死の正確な瞬間に捕らえられなければならない。"
    };
    translations["book_mysticism_page3"] = {
        "Telekinesis, another mystic art, allows the caster to move objects at a distance. With practice, "
        "one can lift weapons from enemy hands or retrieve keys from across locked doors.",
        "テレキネシス、もう一つの神秘術は、術者が遠距離で物体を動かすことを可能にする。 "
        "修行すれば、敵の手から武器を奪ったり、鍵をかけられたドアの向こうから鍵を取り出したりできる。"
    };

    // Book 8: Arboretum (2 pages)
    translations["book_arboretum_title"] = {"Arboretum", "樹木園"};
    translations["book_arboretum_page1"] = {
        "Welcome to the Imperial Arboretum, where flora from across Tamriel has been collected and cultivated "
        "for the education and enjoyment of all citizens. Please do not pick the flowers, no matter how tempting.",
        "帝国樹木園へようこそ。ここではタムリエル各地から集められた植物が栽培され、 "
        "すべての市民の教育と楽しみのために公開されている。どんなに誘惑されても、花を摘まないでください。"
    };
    translations["book_arboretum_page2"] = {
        "The Nirnroot, that glowing plant found only near water, has remarkable alchemical properties. "
        "Its chiming sound can be heard from quite a distance, making it easier to locate in the dark.",
        "ニルンルート、水辺にしか生えない光る植物は、顕著な錬金術的性質を持っている。 "
        "そのチャイムのような音はかなりの距離から聞こえ、暗闇での発見を容易にする。"
    };

    // Book 9: Legend of Pelinal (3 pages)
    translations["book_pelinal_title"] = {"Legend of Pelinal", "ペリナールの伝説"};
    translations["book_pelinal_page1"] = {
        "Pelinal Whitestrake, the Divine Crusader, came to Cyrodiil in the time of the Ayleid tyranny. "
        "He was not a man, not truly, but a weapon in the shape of one. Sent by the gods to free the slaves.",
        "ペリナール・ホワイトストレイク、神聖な十字軍戦士は、アイレイドの暴政の時代にシロディールに来た。 "
        "彼は人間ではなかった。本当に。人間の形をした武器だった。神々によって奴隷を解放するために送られた。"
    };
    translations["book_pelinal_page2"] = {
        "His armor was white as bone, and his hand glowed with righteous fire. The Ayleid sorcerers "
        "threw their darkest magic at him, but Pelinal walked through it as if it were morning mist.",
        "彼の鎧は骨のように白く、彼の手は正義の炎で輝いていた。アイレイドの魔術師たちは最も暗い魔法を彼に向けたが、 "
        "ペリナールはそれを朝霧であるかのように通り抜けた。"
    };
    translations["book_pelinal_page3"] = {
        "In the end, even Pelinal could not stand against the might of Umaril the Unfeathered. "
        "But his sacrifice was not in vain, for he paved the way for Alessia's rebellion and the freedom of all men.",
        "最後には、ペリナールも羽なきウマリルの力に立ち向かうことはできなかった。 "
        "しかし、彼の犠牲は無駄ではなかった。彼はアレッシアの反乱とすべての人間の自由への道を切り開いたのだから。"
    };

    // Book 10: Immortal Blood (2 pages)
    translations["book_immortal_blood_title"] = {"Immortal Blood", "不死の血"};
    translations["book_immortal_blood_page1"] = {
        "They say the vampires of Skyrim are different from those of High Rock or Valenwood. "
        "Colder, more ancient, closer to the original curse that started it all in the time before time.",
        "人々は言う。スカイリムの吸血鬼は、ハイロックやヴァレンウッドのものとは違うと。 "
        "より冷たく、より古く、時のない時代に始まった最初の呪いにより近い存在だと。"
    };
    translations["book_immortal_blood_page2"] = {
        "To become a vampire is to trade your soul for power. You gain the night, but lose the sun. "
        "You gain strength beyond mortal measure, but hunger for that which you once loved most.",
        "吸血鬼になるということは、魂と力を交換することだ。夜を得るが、太陽を失う。 "
        "人間を超える力を得るが、かつて最も愛していたものへの渇望を抱く。"
    };

    LOGI("Loaded %zu translations into database", translations.size());
}

LocalizationManager* getLocalizationManager() {
    return g_localizationManager;
}

void setLocalizationManager(LocalizationManager* manager) {
    g_localizationManager = manager;
}
