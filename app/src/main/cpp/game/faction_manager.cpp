#include "faction_manager.h"
#include <android/log.h>
#include <algorithm>

#define LOG_TAG "FactionManager"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
#define LOGD(...) do {} while(0)
#endif
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace oblivion {

FactionManager::FactionManager() {
}

FactionManager::~FactionManager() {
}

void FactionManager::initialize(const ESMManager* esmMgr) {
    esmManager = esmMgr;
    LOGI("FactionManager initialized");
}

const FactionData* FactionManager::getFaction(uint32_t formID) const {
    if (!esmManager) return nullptr;
    return esmManager->findFaction(formID);
}

const std::vector<FactionData>& FactionManager::getAllFactions() const {
    static std::vector<FactionData> empty;
    if (!esmManager) return empty;
    return esmManager->getAllFactions();
}

FactionRelationship FactionManager::getFactionRelationship(uint32_t faction1FormID, uint32_t faction2FormID) const {
    if (faction1FormID == faction2FormID) {
        return FactionRelationship::Allied;
    }

    int32_t modifier = getRelationModifier(faction1FormID, faction2FormID);

    if (modifier >= 100) return FactionRelationship::Allied;
    if (modifier >= 50) return FactionRelationship::Friendly;
    if (modifier <= -100) return FactionRelationship::Hostile;
    return FactionRelationship::Neutral;
}

void FactionManager::setNPCFaction(uint32_t npcFormID, const FactionMembership& membership) {
    npcFactions[npcFormID].push_back(membership);
    LOGD("Set faction 0x%08X for NPC 0x%08X", membership.factionFormID, npcFormID);
}

std::vector<FactionMembership> FactionManager::getNPCFactions(uint32_t npcFormID) const {
    auto it = npcFactions.find(npcFormID);
    if (it != npcFactions.end()) {
        return it->second;
    }
    return {};
}

uint32_t FactionManager::getNPCPrimaryFaction(uint32_t npcFormID) const {
    auto it = npcFactions.find(npcFormID);
    if (it != npcFactions.end() && !it->second.empty()) {
        return it->second[0].factionFormID;
    }
    return 0;
}

int32_t FactionManager::calculateDisposition(uint32_t npc1FormID, uint32_t npc2FormID) const {
    // Use int64_t to prevent overflow during accumulation
    int64_t disposition = 50;

    // Get factions for both NPCs
    auto factions1 = getNPCFactions(npc1FormID);
    auto factions2 = getNPCFactions(npc2FormID);

    // Check faction relationships
    for (const auto& f1 : factions1) {
        for (const auto& f2 : factions2) {
            FactionRelationship rel = getFactionRelationship(f1.factionFormID, f2.factionFormID);

            switch (rel) {
                case FactionRelationship::Allied:
                    disposition += 50;
                    break;
                case FactionRelationship::Friendly:
                    disposition += 25;
                    break;
                case FactionRelationship::Hostile:
                    disposition -= 50;
                    break;
                case FactionRelationship::Neutral:
                default:
                    break;
            }
        }
    }

    // Clamp to 0-100 range after accumulation
    disposition = std::max<int64_t>(0, std::min<int64_t>(100, disposition));
    return static_cast<int32_t>(disposition);
}

std::string FactionManager::getFactionRankName(uint32_t factionFormID, int32_t rank) const {
    const FactionData* faction = getFaction(factionFormID);
    if (!faction) return "Unknown";

    if (rank >= 0 && rank < static_cast<int32_t>(faction->ranks.size())) {
        return faction->ranks[rank].rankName;
    }

    return "Member";
}

bool FactionManager::isNPCInFaction(uint32_t npcFormID, uint32_t factionFormID) const {
    auto it = npcFactions.find(npcFormID);
    if (it == npcFactions.end()) return false;

    for (const auto& membership : it->second) {
        if (membership.factionFormID == factionFormID) {
            return true;
        }
    }

    return false;
}

int32_t FactionManager::getRelationModifier(uint32_t faction1FormID, uint32_t faction2FormID) const {
    const FactionData* faction1 = getFaction(faction1FormID);
    if (!faction1) return 0;

    for (const auto& relation : faction1->relations) {
        if (relation.factionFormID == faction2FormID) {
            return relation.modifier;
        }
    }

    return 0;
}

} // namespace oblivion
