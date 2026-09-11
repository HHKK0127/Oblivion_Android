#include "material_manager.h"
#include <algorithm>

namespace UI {

MaterialManager& MaterialManager::instance() {
    static MaterialManager s_instance;
    return s_instance;
}

void MaterialManager::initialize() {
    loadDefaultPresets();
}

void MaterialManager::shutdown() {
    m_presets.clear();
    m_categories.clear();
    m_presetIndex.clear();
}

const MaterialPreset* MaterialManager::getPreset(const std::string& name) const {
    auto it = m_presetIndex.find(name);
    if (it != m_presetIndex.end()) {
        return &m_presets[it->second];
    }
    return nullptr;
}

std::vector<const MaterialPreset*> MaterialManager::getPresetsByCategory(const std::string& category) const {
    std::vector<const MaterialPreset*> result;
    for (const auto& preset : m_presets) {
        if (preset.category == category) {
            result.push_back(&preset);
        }
    }
    return result;
}

void MaterialManager::addCustomPreset(const MaterialPreset& preset) {
    if (m_presetIndex.find(preset.name) != m_presetIndex.end()) {
        return;
    }
    m_presetIndex[preset.name] = m_presets.size();
    m_presets.push_back(preset);
}

bool MaterialManager::removeCustomPreset(const std::string& name) {
    auto it = m_presetIndex.find(name);
    if (it == m_presetIndex.end()) {
        return false;
    }
    size_t index = it->second;
    m_presets.erase(m_presets.begin() + index);
    m_presetIndex.clear();
    for (size_t i = 0; i < m_presets.size(); ++i) {
        m_presetIndex[m_presets[i].name] = i;
    }
    return true;
}

void MaterialManager::loadDefaultPresets() {
    m_categories = {"Metal", "Stone", "Wood", "Fabric", "Glass", "Magic"};

    // Metal presets
    {
        MaterialPreset preset;
        preset.name = "Iron";
        preset.category = "Metal";
        preset.description = "Standard iron material";
        preset.material.setAmbient({0.2f, 0.2f, 0.2f});
        preset.material.setDiffuse({0.5f, 0.5f, 0.5f});
        preset.material.setSpecular({0.8f, 0.8f, 0.8f});
        preset.material.setShininess(64.0f);
        m_presetIndex[preset.name] = m_presets.size();
        m_presets.push_back(preset);
    }
    {
        MaterialPreset preset;
        preset.name = "Gold";
        preset.category = "Metal";
        preset.description = "Shiny gold material";
        preset.material.setAmbient({0.2f, 0.15f, 0.05f});
        preset.material.setDiffuse({0.8f, 0.6f, 0.2f});
        preset.material.setSpecular({1.0f, 0.9f, 0.6f});
        preset.material.setShininess(128.0f);
        m_presetIndex[preset.name] = m_presets.size();
        m_presets.push_back(preset);
    }
    {
        MaterialPreset preset;
        preset.name = "Steel";
        preset.category = "Metal";
        preset.description = "Polished steel material";
        preset.material.setAmbient({0.15f, 0.15f, 0.18f});
        preset.material.setDiffuse({0.6f, 0.6f, 0.65f});
        preset.material.setSpecular({0.9f, 0.9f, 0.95f});
        preset.material.setShininess(96.0f);
        m_presetIndex[preset.name] = m_presets.size();
        m_presets.push_back(preset);
    }

    // Stone presets
    {
        MaterialPreset preset;
        preset.name = "Marble";
        preset.category = "Stone";
        preset.description = "Polished marble material";
        preset.material.setAmbient({0.2f, 0.2f, 0.2f});
        preset.material.setDiffuse({0.8f, 0.8f, 0.8f});
        preset.material.setSpecular({0.5f, 0.5f, 0.5f});
        preset.material.setShininess(32.0f);
        m_presetIndex[preset.name] = m_presets.size();
        m_presets.push_back(preset);
    }
    {
        MaterialPreset preset;
        preset.name = "Rough Stone";
        preset.category = "Stone";
        preset.description = "Rough stone material";
        preset.material.setAmbient({0.15f, 0.12f, 0.1f});
        preset.material.setDiffuse({0.4f, 0.35f, 0.3f});
        preset.material.setSpecular({0.1f, 0.1f, 0.1f});
        preset.material.setShininess(4.0f);
        m_presetIndex[preset.name] = m_presets.size();
        m_presets.push_back(preset);
    }

    // Wood presets
    {
        MaterialPreset preset;
        preset.name = "Oak";
        preset.category = "Wood";
        preset.description = "Oak wood material";
        preset.material.setAmbient({0.15f, 0.1f, 0.05f});
        preset.material.setDiffuse({0.5f, 0.35f, 0.2f});
        preset.material.setSpecular({0.15f, 0.1f, 0.05f});
        preset.material.setShininess(8.0f);
        m_presetIndex[preset.name] = m_presets.size();
        m_presets.push_back(preset);
    }

    // Fabric presets
    {
        MaterialPreset preset;
        preset.name = "Leather";
        preset.category = "Fabric";
        preset.description = "Leather material";
        preset.material.setAmbient({0.1f, 0.05f, 0.02f});
        preset.material.setDiffuse({0.35f, 0.2f, 0.1f});
        preset.material.setSpecular({0.1f, 0.08f, 0.05f});
        preset.material.setShininess(4.0f);
        m_presetIndex[preset.name] = m_presets.size();
        m_presets.push_back(preset);
    }
    {
        MaterialPreset preset;
        preset.name = "Cloth";
        preset.category = "Fabric";
        preset.description = "Cloth material";
        preset.material.setAmbient({0.1f, 0.1f, 0.1f});
        preset.material.setDiffuse({0.4f, 0.4f, 0.4f});
        preset.material.setSpecular({0.05f, 0.05f, 0.05f});
        preset.material.setShininess(2.0f);
        m_presetIndex[preset.name] = m_presets.size();
        m_presets.push_back(preset);
    }

    // Glass presets
    {
        MaterialPreset preset;
        preset.name = "Glass";
        preset.category = "Glass";
        preset.description = "Clear glass material";
        preset.material.setAmbient({0.05f, 0.05f, 0.05f});
        preset.material.setDiffuse({0.2f, 0.2f, 0.2f});
        preset.material.setSpecular({1.0f, 1.0f, 1.0f});
        preset.material.setShininess(256.0f);
        m_presetIndex[preset.name] = m_presets.size();
        m_presets.push_back(preset);
    }

    // Magic presets
    {
        MaterialPreset preset;
        preset.name = "Enchanted";
        preset.category = "Magic";
        preset.description = "Enchanted glowing material";
        preset.material.setAmbient({0.1f, 0.1f, 0.3f});
        preset.material.setDiffuse({0.3f, 0.3f, 0.8f});
        preset.material.setSpecular({0.8f, 0.8f, 1.0f});
        preset.material.setShininess(96.0f);
        m_presetIndex[preset.name] = m_presets.size();
        m_presets.push_back(preset);
    }
    {
        MaterialPreset preset;
        preset.name = "Daedric";
        preset.category = "Magic";
        preset.description = "Daedric artifact material";
        preset.material.setAmbient({0.15f, 0.02f, 0.02f});
        preset.material.setDiffuse({0.5f, 0.1f, 0.1f});
        preset.material.setSpecular({0.9f, 0.3f, 0.3f});
        preset.material.setShininess(128.0f);
        m_presetIndex[preset.name] = m_presets.size();
        m_presets.push_back(preset);
    }
}

} // namespace UI
