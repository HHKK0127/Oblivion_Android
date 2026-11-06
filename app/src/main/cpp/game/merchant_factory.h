#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "merchant.h"

namespace oblivion { class ESMManager; }

namespace game {

// Barter view of one NPC_ record. Oblivion keeps a shopkeeper stock in the
// CNTO subrecords of the NPC_ and the shop purse in the ACBS barter gold
// word (v261), so the merchant is rebuilt from those fields on demand
// instead of being stored separately. Returns null when the FormID does
// not resolve to a NPC_ record. The shop name prefers the display name and
// falls back to the editor id.
std::shared_ptr<Merchant> buildMerchantFromNpc(const oblivion::ESMManager& esm,
                                               uint32_t npcFormID);

// True when the NPC carries a non-zero ACBS barter gold (v261: the authored
// purse at NPC+0x30 read by GetBarterGold). The old CNTO-gold heuristic
// mis-classified bandits as merchants; the INFO OfferServices topic gate is
// deferred because DIAL has no parsed NPC-owner FormID.
bool npcIsMerchant(const oblivion::ESMManager& esm, uint32_t npcFormID);

}  // namespace game