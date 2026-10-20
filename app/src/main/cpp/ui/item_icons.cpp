#include "item_icons.h"
#include "../engine/texture_loader.h"

#include <algorithm>
#include <cctype>
#include <regex>
#include <android/log.h>

#define LOG_TAG "ItemIcons"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

// ── Singleton ──────────────────────────────────────────────────────────

ItemIconResolver& ItemIconResolver::instance() {
    static ItemIconResolver inst;
    return inst;
}

ItemIconResolver::~ItemIconResolver() {
    clearCache();
}

void ItemIconResolver::clearCache() {
    for (const auto& kv : iconCache_) {
        if (kv.second != 0) {
            TextureLoader::deleteTexture(kv.second);
        }
    }
    iconCache_.clear();
    for (auto& fb : categoryFallbacks_) {
        if (fb != 0) {
            TextureLoader::deleteTexture(fb);
            fb = 0;
        }
    }
}

// ── Normalization ──────────────────────────────────────────────────────

std::string ItemIconResolver::normalizeName(const std::string& raw) const {
    std::string s = raw;
    // Lowercase
    for (auto& ch : s) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));

    // Remove apostrophes
    s.erase(std::remove(s.begin(), s.end(), '\''), s.end());

    // Replace hyphens with nothing (e.g. "foo-bar" -> "foobar")
    s.erase(std::remove(s.begin(), s.end(), '-'), s.end());

    // Replace slashes with nothing
    s.erase(std::remove(s.begin(), s.end(), '/'), s.end());

    // Remove all punctuation except spaces
    s.erase(
        std::remove_if(s.begin(), s.end(), [](char c) {
            return std::ispunct(static_cast<unsigned char>(c)) && c != '_';
        }),
        s.end()
    );

    // Collapse consecutive spaces → single space
    std::string result;
    bool lastWasSpace = false;
    for (char c : s) {
        if (c == ' ') {
            if (!lastWasSpace) {
                result += c;
                lastWasSpace = true;
            }
        } else {
            result += c;
            lastWasSpace = false;
        }
    }

    // Remove spaces → flat name
    result.erase(std::remove(result.begin(), result.end(), ' '), result.end());
    return result;
}

// ── Category helpers ───────────────────────────────────────────────────

std::string ItemIconResolver::categoryPrefix(int category) const {
    switch (category) {
        case 1: return "weapons";
        case 2: return "armor";
        case 3: case 4: return "clutter";
        case 5: return "clutter";
        case 6: return "clutter";
        case 7: return "clutter";
        case 8: return "clutter";
        case 9: return "clutter";
        default: return "clutter";
    }
}

std::string ItemIconResolver::subcategorySuffix(const std::string& name) const {
    // Heuristic: magic items, potions, gems, keys, soulgems
    const std::string lower = normalizeName(name);
    if (lower.find("potion") != std::string::npos ||
        lower.find("poison") != std::string::npos)
        return "_potions";
    if (lower.find("gem") != std::string::npos ||
        lower.find("diamond") != std::string::npos ||
        lower.find("emerald") != std::string::npos ||
        lower.find("sapphire") != std::string::npos ||
        lower.find("ruby") != std::string::npos ||
        lower.find("topaz") != std::string::npos ||
        lower.find("pearl") != std::string::npos ||
        lower.find("nugget") != std::string::npos)
        return "_gems";
    if (lower.find("soul") != std::string::npos &&
        (lower.find("gem") != std::string::npos ||
         lower.find("star") != std::string::npos))
        return "_soulgems";
    if (lower.find("key") != std::string::npos &&
        lower.find("lockpick") == std::string::npos)
        return "_key";
    if (lower.find("upper") != std::string::npos)
        return "_upper";
    if (lower.find("middle") != std::string::npos)
        return "_middleclass";
    return "";
}

// ── Candidate generation ──────────────────────────────────────────────

void ItemIconResolver::buildCandidates(const std::string& name, int category,
                                        std::vector<std::string>& out) {
    const std::string norm = normalizeName(name);
    const std::string prefix = categoryPrefix(category);
    const std::string subcat = subcategorySuffix(name);

    // --- Weapons (1): icons_weapons__{name}.png ---
    if (category == 1) {
        // Try direct: normal
        out.push_back("icons_weapons__" + norm + ".png");

        // Try with "icon" prefix (some weapons have it like icon_inv_weapon_1)
        out.push_back("icons_weapons__icon_" + norm + ".png");
        out.push_back("icons_weapons__icon" + norm + ".png");
    }

    // --- Armor (2): icons_armor__icon_inv_armor_{N}.png ---
    if (category == 2) {
        out.push_back("icons_armor__icon_inv_armor_" + norm + ".png");
        out.push_back("icons_armor__" + norm + ".png");
    }

    // --- Clutter (3-9): icons_clutter{_subcat}__icon{name}.png ---
    if (category >= 3) {
        // With subcategory
        out.push_back("icons_clutter" + subcat + "__icon" + norm + ".png");
        // Without subcategory
        out.push_back("icons_clutter__icon" + norm + ".png");
        // Direct
        out.push_back("icons_clutter__" + norm + ".png");
        if (!subcat.empty()) {
            out.push_back("icons_clutter" + subcat + "__" + norm + ".png");
        }
    }

    // --- Fallback: category default ---
    out.push_back(""); // sentinel: use category fallback
}

// ── Texture load wrapper ───────────────────────────────────────────────

GLuint ItemIconResolver::tryLoad(const std::string& filename) {
    const std::string fullPath = std::string(kBasePath) + filename;
    GLuint tex = TextureLoader::loadTextureFromAsset(fullPath);
    if (tex == 0) {
        LOGD("Icon not found: %s", fullPath.c_str());
    }
    return tex;
}

// ── Category fallback ──────────────────────────────────────────────────

GLuint ItemIconResolver::getCategoryIcon(int category) {
    if (category < 0 || category > 9) category = 0;
    if (categoryFallbacks_[category] != 0)
        return categoryFallbacks_[category];

    // Build fallback path: try various generic icons
    GLuint tex = 0;

    const char* const fallbacks[10] = {
        "icons__icon_image_set_3.png",          // all
        "icons_weapons__icon_inv_weapon_1.png",  // weapons
        "icons_armor__icon_inv_armor_1.png",     // apparel
        "icons_clutter__iconalembic.png",        // apparatus
        "icons_clutter_potions__iconpotion01.png", // potions
        "icons_clutter__iconapple.png",          // ingredients
        "icons_clutter__iconbook1.png",         // books
        "icons_clutter_key__key.png",           // keys
        "icons_clutter_gems__icondiamond.png",  // stones
        "icons_clutter__iconlockpick.png",      // misc
    };

    tex = tryLoad(fallbacks[category]);
    if (tex == 0 && category != 0) {
        tex = tryLoad(fallbacks[0]); // fallback to "all"
    }

    categoryFallbacks_[category] = tex;
    return tex;
}

// ── Main resolution ────────────────────────────────────────────────────

GLuint ItemIconResolver::getIcon(uint32_t itemId, const std::string& itemName, int category) {
    if (itemId == 0) return getIconByName(itemName, category);

    auto it = iconCache_.find(itemId);
    if (it != iconCache_.end()) return it->second;

    GLuint tex = getIconByName(itemName, category);
    iconCache_[itemId] = tex;
    return tex;
}

GLuint ItemIconResolver::getIconByName(const std::string& itemName, int category) {
    if (itemName.empty()) return getCategoryIcon(category);

    std::vector<std::string> candidates;
    buildCandidates(itemName, category, candidates);

    for (const auto& cand : candidates) {
        if (cand.empty()) break; // sentinel
        GLuint tex = tryLoad(cand);
        if (tex != 0) return tex;
    }

    // Fallback to category default
    GLuint fallback = getCategoryIcon(category);
    LOGD("No exact icon for '%s' (cat %d), using category fallback",
         itemName.c_str(), category);
    return fallback;
}