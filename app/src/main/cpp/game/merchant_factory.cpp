#include "merchant_factory.h"

#include "item_catalog.h"
#include "../assets/esm_reader.h"

namespace game {

namespace {

// Prices derive from the record value through the reverse-engineered Merchant
// formula (docs/RE_EXE_REPORT.md §57.5, FUN_00488e50).
void addStock(Merchant& merchant, const oblivion::ESMManager& esm,
              uint32_t itemFormID, int32_t count) {
    if (itemFormID == 0 || itemFormID == kGoldFormID || count <= 0) return;

    const ItemInfo info = resolveItemInfo(esm, itemFormID);
    const float value = static_cast<float>(info.value);
    // weight and baseValue ride along with the shelf row so that a purchase can
    // be rebuilt in the player inventory without re-resolving the ESM record.
    merchant.addItem(MerchantItem(itemIdFromFormID(info.formID), info.name,
                                  value, Merchant::sellPriceForValue(value),
                                  count, info.type,
                                  info.resolved ? std::string()
                                                : std::string("Unknown record"),
                                  info.weight, info.value));
}

}  // namespace

bool npcIsMerchant(const oblivion::ESMManager& esm, uint32_t npcFormID) {
    const oblivion::NPCData* npc = esm.findNPC(npcFormID);
    if (npc == nullptr) return false;

    // Trade-capable NPCs carry a non-zero ACBS barter gold (v261). The
    // original also gates topics through INFO OfferServices, but DIAL has no
    // parsed NPC-owner FormID and the INFO bit writer is unallocated, so the
    // topic gate stays deferred and barterGold is the primary merchant test.
    // The old CNTO-gold-purse heuristic mis-classified bandits as merchants.
    return npc->barterGold > 0;
}

std::shared_ptr<Merchant> buildMerchantFromNpc(const oblivion::ESMManager& esm,
                                               uint32_t npcFormID) {
    const oblivion::NPCData* npc = esm.findNPC(npcFormID);
    if (npc == nullptr) return nullptr;

    auto merchant = std::make_shared<Merchant>(
        npcFormID, npc->fullName.empty() ? npc->editorID : npc->fullName);

    int32_t gold = 0;
    for (const oblivion::NPCData::NPCItem& entry : npc->items) {
        if (entry.itemFormID == kGoldFormID) {
            if (entry.count > 0) gold += entry.count;
            continue;
        }
        addStock(*merchant, esm, entry.itemFormID, entry.count);
    }

    // The constructor starts every shop at 5000 gold, so the authored barter
    // gold replaces that default instead of adding to it. Fall back to the
    // CNTO purse sum when barterGold is zero: the two pools diverge in the
    // original (v261), but a record with only a purse still needs funds to
    // trade with.
    if (npc->barterGold > 0) {
        merchant->setGold(static_cast<float>(npc->barterGold));
    } else if (gold > 0) {
        merchant->setGold(static_cast<float>(gold));
    }
    return merchant;
}

}  // namespace game