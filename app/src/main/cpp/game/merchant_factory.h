#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "merchant.h"

namespace oblivion { class ESMManager; }

namespace game {

// Barter view of one NPC_ record. Oblivion keeps a shopkeeper stock and purse in
// the CNTO subrecords of the NPC_, so the merchant is rebuilt from that list on
// demand instead of being stored separately. Returns null when the FormID does
// not resolve to a NPC_ record. The shop name prefers the display name and
// falls back to the editor id.
std::shared_ptr<Merchant> buildMerchantFromNpc(const oblivion::ESMManager& esm,
                                               uint32_t npcFormID);

// True when the NPC carries Gold in its CNTO stock. Gold in the stock list is
// the best available stand-in for the vanilla merchant flag until that flag is
// reverse engineered.
bool npcIsMerchant(const oblivion::ESMManager& esm, uint32_t npcFormID);

}  // namespace game