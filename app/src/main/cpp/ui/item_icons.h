#pragma once

#include <GLES3/gl3.h>
#include <string>
#include <unordered_map>

/**
 * @brief Map item names → BSA-original icon textures.
 *
 * Phase 9 / P68: loads icons from textures/ui/icons/menus/ following the
 * BSA-extraction naming convention:
 *   icons_{category}__{normalized_name}.png
 *
 * Falls back through several strategies before returning the default
 * category icon.
 */
class ItemIconResolver {
public:
    static ItemIconResolver& instance();

    /**
     * @brief Resolve icon texture for item.
     * @param itemId   ESM formID
     * @param itemName Display name
     * @param category Numeric category index (0..9)
     * @return OpenGL texture ID (0 on failure)
     */
    GLuint getIcon(uint32_t itemId, const std::string& itemName, int category);

    /**
     * @brief Resolve icon texture for item (convenience, by name + category only).
     */
    GLuint getIconByName(const std::string& itemName, int category);

    /** @brief Pre-cache a category fallback texture. */
    GLuint getCategoryIcon(int category);

    /** @brief Clear all cached textures. */
    void clearCache();

private:
    ItemIconResolver() = default;
    ~ItemIconResolver();

    // ── Resolution strategies ──────────────────────────────────────────

    /** Try exact filename in textures/ui/icons/menus/. */
    GLuint tryLoad(const std::string& filename);

    /** Build candidate filenames from an item name + category. */
    void buildCandidates(const std::string& name, int category,
                         std::vector<std::string>& out);

    /** Normalize item name for icon path. */
    std::string normalizeName(const std::string& raw) const;

    /** Category prefix (e.g. "weapons", "clutter", "armor"). */
    std::string categoryPrefix(int category) const;

    /** Subcategory prefix for special groups (potions, gems, keys, soulgems). */
    std::string subcategorySuffix(const std::string& name) const;

    static constexpr const char* kBasePath = "textures/ui/icons/menus/";

    // Cache: itemId → textureId   (0 = not found)
    std::unordered_map<uint32_t, GLuint> iconCache_;
    // Category fallback textures
    GLuint categoryFallbacks_[10] = {0};
    bool initialized_ = false;

    // Prevent copy
    ItemIconResolver(const ItemIconResolver&) = delete;
    ItemIconResolver& operator=(const ItemIconResolver&) = delete;
};