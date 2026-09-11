#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "../geometry/material.h"

namespace UI {

struct MaterialPreset {
    std::string name;
    std::string description;
    Material material;
    std::string category;
};

class MaterialManager {
public:
    static MaterialManager& instance();

    void initialize();
    void shutdown();

    const std::vector<MaterialPreset>& getPresets() const { return m_presets; }
    const MaterialPreset* getPreset(const std::string& name) const;
    const std::vector<std::string>& getCategories() const { return m_categories; }
    std::vector<const MaterialPreset*> getPresetsByCategory(const std::string& category) const;

    void addCustomPreset(const MaterialPreset& preset);
    bool removeCustomPreset(const std::string& name);

private:
    MaterialManager() = default;
    void loadDefaultPresets();

    std::vector<MaterialPreset> m_presets;
    std::vector<std::string> m_categories;
    std::unordered_map<std::string, size_t> m_presetIndex;
};

} // namespace UI
