#pragma once

#include "../assets/esm_reader.h"
#include <vector>
#include <unordered_map>

namespace oblivion {

/// Faction relationship types
enum class FactionRelationship : uint32_t {
    Neutral   = 0,
    Friendly  = 1,
    Allied    = 2,
    Hostile   = 3
};

/// NPC faction membership
struct FactionMembership {
    uint32_t factionFormID = 0;
    int32_t rank = 0;
    int32_t reputation = 0;
};

/// Faction manager for NPC relationships and disposition
class FactionManager {
public:
    FactionManager();
    ~FactionManager();

    void initialize(const ESMManager* esmMgr);

    /// Get faction by formID
    const FactionData* getFaction(uint32_t formID) const;

    /// Get all factions
    const std::vector<FactionData>& getAllFactions() const;

    /// Get faction relationship between two factions
    FactionRelationship getFactionRelationship(uint32_t faction1FormID, uint32_t faction2FormID) const;

    /// Set NPC faction membership
    void setNPCFaction(uint32_t npcFormID, const FactionMembership& membership);

    /// Get NPC faction memberships
    std::vector<FactionMembership> getNPCFactions(uint32_t npcFormID) const;

    /// Get NPC's primary faction
    uint32_t getNPCPrimaryFaction(uint32_t npcFormID) const;

    /// Calculate disposition between two NPCs based on faction relationships
    int32_t calculateDisposition(uint32_t npc1FormID, uint32_t npc2FormID) const;

    /// Get faction rank name
    std::string getFactionRankName(uint32_t factionFormID, int32_t rank) const;

    /// Check if NPC is in faction
    bool isNPCInFaction(uint32_t npcFormID, uint32_t factionFormID) const;

private:
    const ESMManager* esmManager = nullptr;

    /// NPC faction memberships: npcFormID -> list of memberships
    std::unordered_map<uint32_t, std::vector<FactionMembership>> npcFactions;

    /// Get faction relation modifier between two factions
    int32_t getRelationModifier(uint32_t faction1FormID, uint32_t faction2FormID) const;
};

} // namespace oblivion
