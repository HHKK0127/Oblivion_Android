#include "merchant_tests.h"

#include "../assets/esm_reader.h"
#include "../game/barter.h"
#include "../game/inventory.h"
#include "../game/item_catalog.h"
#include "../game/item.h"
#include "../game/merchant.h"
#include "../game/merchant_factory.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace {

// ---------------------------------------------------------------------------
// Synthetic ESM image
// ---------------------------------------------------------------------------

constexpr uint32_t kNpcStocked = 0x00001000;
constexpr uint32_t kNpcZeroGold = 0x00001001;
constexpr uint32_t kNpcNoStock = 0x00001002;
constexpr uint32_t kNpcClassed = 0x00001003;
constexpr uint32_t kNpcEdidOnlyClass = 0x00001004;
constexpr uint32_t kNpcLegacyCnam = 0x00001005;
constexpr uint32_t kNpcMissingClass = 0x00001006;
constexpr uint32_t kNpcExternalClass = 0x00005000;
constexpr uint32_t kWarriorClass = 0x00004000;
constexpr uint32_t kEdidOnlyClass = 0x00004001;
constexpr uint32_t kAbsentClass = 0x00007000;
constexpr uint32_t kApple = 0x00002000;
constexpr uint32_t kGem = 0x00002001;
constexpr uint32_t kAbsent = 0x00003000;
constexpr uint32_t kUnknownFormID = 0x0000BEEF;

const std::string kAppleId = "00002000";
const std::string kGemId = "00002001";
const std::string kAbsentId = "00003000";
const std::string kGoldId = "0000000F";

struct CntoEntry {
    uint32_t itemFormID;
    int32_t count;
};

void putU32(std::vector<uint8_t>& out, uint32_t value) {
    out.push_back(static_cast<uint8_t>(value & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
}

void putI32(std::vector<uint8_t>& out, int32_t value) {
    uint32_t raw = 0;
    std::memcpy(&raw, &value, 4);
    putU32(out, raw);
}

void putF32(std::vector<uint8_t>& out, float value) {
    uint32_t raw = 0;
    std::memcpy(&raw, &value, 4);
    putU32(out, raw);
}

void putTag(std::vector<uint8_t>& out, const char* tag) {
    out.insert(out.end(), tag, tag + 4);
}

// Subrecord: 4 byte tag, uint16 payload size, payload.
void putSub(std::vector<uint8_t>& out, const char* tag, const std::vector<uint8_t>& payload) {
    putTag(out, tag);
    const uint16_t size = static_cast<uint16_t>(payload.size());
    out.push_back(static_cast<uint8_t>(size & 0xFF));
    out.push_back(static_cast<uint8_t>((size >> 8) & 0xFF));
    out.insert(out.end(), payload.begin(), payload.end());
}

// Record: 4 byte type, uint32 payload size, flags, formID, version, payload.
void putRecord(std::vector<uint8_t>& out, const char* type, uint32_t formID,
               const std::vector<uint8_t>& payload) {
    putTag(out, type);
    putU32(out, static_cast<uint32_t>(payload.size()));
    putU32(out, 0);  // flags
    putU32(out, formID);
    putU32(out, 0);  // version
    out.insert(out.end(), payload.begin(), payload.end());
}

std::vector<uint8_t> textPayload(const std::string& text) {
    std::vector<uint8_t> payload(text.begin(), text.end());
    payload.push_back(0);
    return payload;
}

std::vector<uint8_t> u32Payload(uint32_t value) {
    std::vector<uint8_t> payload;
    putU32(payload, value);
    return payload;
}

std::vector<uint8_t> cntoPayload(uint32_t itemFormID, int32_t count) {
    std::vector<uint8_t> payload;
    putU32(payload, itemFormID);
    putI32(payload, count);
    return payload;
}

std::vector<uint8_t> miscDataPayload(uint32_t value, float weight) {
    std::vector<uint8_t> payload;
    putU32(payload, value);
    putF32(payload, weight);
    return payload;
}

std::vector<uint8_t> npcPayload(const std::string& editorID, const std::string& fullName,
                                const std::vector<CntoEntry>& stock,
                                const std::vector<uint8_t>* cnam = nullptr) {
    std::vector<uint8_t> payload;
    putSub(payload, "EDID", textPayload(editorID));
    if (!fullName.empty()) putSub(payload, "FULL", textPayload(fullName));
    if (cnam) putSub(payload, "CNAM", *cnam);
    for (const CntoEntry& entry : stock) {
        putSub(payload, "CNTO", cntoPayload(entry.itemFormID, entry.count));
    }
    return payload;
}

// CLAS carries the display name in FULL and falls back to EDID when FULL is
// absent; the resolver mirrors that choice.
std::vector<uint8_t> classPayload(const std::string& editorID, const std::string& fullName) {
    std::vector<uint8_t> payload;
    putSub(payload, "EDID", textPayload(editorID));
    if (!fullName.empty()) putSub(payload, "FULL", textPayload(fullName));
    return payload;
}

std::vector<uint8_t> miscPayload(const std::string& editorID, const std::string& fullName,
                                 uint32_t value, float weight) {
    std::vector<uint8_t> payload;
    putSub(payload, "EDID", textPayload(editorID));
    if (!fullName.empty()) putSub(payload, "FULL", textPayload(fullName));
    putSub(payload, "DATA", miscDataPayload(value, weight));
    return payload;
}

std::vector<uint8_t> buildSyntheticEsm() {
    // Records are emitted in stream order: the TES4 header first, then the bare
    // top level records the parser decodes without a GRUP wrapper.
    std::vector<uint8_t> records;
    putRecord(records, "NPC_", kNpcStocked,
              npcPayload("TestMerchant", "Collected Goods",
                         {{game::kGoldFormID, 250}, {kApple, 3}, {kGem, 1}, {kAbsent, 2}}));
    putRecord(records, "NPC_", kNpcZeroGold,
              npcPayload("EdidOnlyTrader", "", {{game::kGoldFormID, 0}, {kApple, 1}}));
    putRecord(records, "NPC_", kNpcNoStock, npcPayload("EmptyTrader", "", {}));

    // CNAM is the class formID; the CLAS records are emitted after the actors
    // on purpose, because resolution only runs after the whole file is decoded.
    const std::vector<uint8_t> warriorCnam = u32Payload(kWarriorClass);
    const std::vector<uint8_t> edidClassCnam = u32Payload(kEdidOnlyClass);
    const std::vector<uint8_t> legacyCnam = textPayload("AB");
    const std::vector<uint8_t> absentCnam = u32Payload(kAbsentClass);
    putRecord(records, "NPC_", kNpcClassed, npcPayload("ClassedNpc", "", {}, &warriorCnam));
    putRecord(records, "NPC_", kNpcEdidOnlyClass,
              npcPayload("EdidClassNpc", "", {}, &edidClassCnam));
    putRecord(records, "NPC_", kNpcLegacyCnam,
              npcPayload("StringClassNpc", "", {}, &legacyCnam));
    putRecord(records, "NPC_", kNpcMissingClass,
              npcPayload("MissingClassNpc", "", {}, &absentCnam));
    putRecord(records, "CLAS", kWarriorClass, classPayload("TestWarrior", "Warrior"));
    putRecord(records, "CLAS", kEdidOnlyClass, classPayload("TestOnlyEdid", ""));

    putRecord(records, "MISC", kApple, miscPayload("TestApple", "Test Apple", 5, 0.5f));
    putRecord(records, "MISC", kGem, miscPayload("TestGem", "", 250, 1.25f));

    std::vector<uint8_t> image;
    putRecord(image, "TES4", 0, std::vector<uint8_t>());
    image.insert(image.end(), records.begin(), records.end());
    return image;
}

// ---------------------------------------------------------------------------
// Small helpers
// ---------------------------------------------------------------------------

bool nearlyEqual(float a, float b) {
    const float delta = a > b ? a - b : b - a;
    return delta < 0.002f;
}

std::shared_ptr<Merchant> buildShop(const oblivion::ESMManager& esm, uint32_t npcFormID) {
    return game::buildMerchantFromNpc(esm, npcFormID);
}

void seedGold(Inventory& player, int32_t amount) {
    game::addPlayerItem(player, game::goldItemInfo(), amount);
}

// The grid store keeps one row per base item FormID and leaves an emptied row in
// place, so "items carried" means occupied rows, not the raw slot count.
size_t occupiedRows(const Inventory& player) {
    size_t rows = 0;
    for (const InventorySlot& slot : player.getAllSlots()) {
        if (!slot.isEmpty()) ++rows;
    }
    return rows;
}

const InventorySlot* findPlayerSlot(const Inventory& player, uint32_t formID) {
    for (const InventorySlot& slot : player.getAllSlots()) {
        if (slot.item.itemId == formID && slot.quantity > 0) return &slot;
    }
    return nullptr;
}

}  // namespace

void MerchantTests::record(const std::string& name, bool passed, const std::string& msg, float ms) {
    MerchantTestResult result;
    result.testName = name;
    result.passed = passed;
    result.message = msg;
    result.durationMs = ms;
    results.push_back(result);
    std::printf("[MerchantTests] %s %s", passed ? "PASS" : "FAIL", name.c_str());
    if (!msg.empty()) {
        std::printf(" - %s", msg.c_str());
    }
    std::printf(" (%.1f ms)\n", ms);
}

int MerchantTests::getPassCount() const {
    int count = 0;
    for (const MerchantTestResult& result : results) {
        if (result.passed) count++;
    }
    return count;
}

int MerchantTests::getFailCount() const {
    int count = 0;
    for (const MerchantTestResult& result : results) {
        if (!result.passed) count++;
    }
    return count;
}

std::string MerchantTests::getSummary() const {
    return "Merchant Test Results\nTotal: " + std::to_string(results.size()) +
           " | Pass: " + std::to_string(getPassCount()) +
           " | Fail: " + std::to_string(getFailCount());
}

bool MerchantTests::runAllTests() {
    results.clear();

    const std::vector<uint8_t> data = buildSyntheticEsm();

    // ---- ESM image decode ----
    oblivion::ESMFile file;
    const bool parsed = file.parseFromMemory("synthetic.esm", data.data(), data.size());
    record("ParseSyntheticEsm", parsed, parsed ? "ESMFile::parseFromMemory ok" : "parse failed");
    if (!parsed) return getFailCount() == 0;

    record("AllNpcRecordsDecoded", file.getNPCs().size() == 7,
           "npcs=" + std::to_string(file.getNPCs().size()));

    oblivion::ESMManager esm;
    const bool loaded = esm.loadPluginFromMemory("synthetic.esm", data.data(), data.size());
    record("LoadSyntheticEsm", loaded, loaded ? "ESMManager::loadPluginFromMemory ok" : "load failed");
    if (!loaded) return getFailCount() == 0;

    const oblivion::NPCData* stockedNpc = esm.findNPC(kNpcStocked);
    record("NpcFullNameDecoded",
           stockedNpc != nullptr && stockedNpc->fullName == "Collected Goods" &&
               stockedNpc->editorID == "TestMerchant",
           stockedNpc ? stockedNpc->fullName : "npc missing");
    record("NpcCntoStockDecoded", stockedNpc != nullptr && stockedNpc->items.size() == 4,
           stockedNpc ? "cnto=" + std::to_string(stockedNpc->items.size()) : "npc missing");

    const oblivion::NPCData* zeroGoldNpc = esm.findNPC(kNpcZeroGold);
    record("NpcCntoZeroCountDropped", zeroGoldNpc != nullptr && zeroGoldNpc->items.size() == 1,
           zeroGoldNpc ? "cnto=" + std::to_string(zeroGoldNpc->items.size()) : "npc missing");

    const oblivion::NPCData* bareNpc = esm.findNPC(kNpcNoStock);
    record("NpcCntoEmptyList", bareNpc != nullptr && bareNpc->items.empty(),
           bareNpc ? "cnto=" + std::to_string(bareNpc->items.size()) : "npc missing");

    // ---- CNAM (class formID) -> CLAS resolution ----
    const oblivion::NPCData* classedNpc = esm.findNPC(kNpcClassed);
    record("NpcCnamReadAsFormId",
           classedNpc != nullptr && classedNpc->classFormID == kWarriorClass,
           classedNpc ? "classFormID=" + std::to_string(classedNpc->classFormID) : "npc missing");
    record("NpcClassNameResolvedFromClas",
           classedNpc != nullptr && classedNpc->className == "Warrior",
           classedNpc ? classedNpc->className : "npc missing");

    const oblivion::NPCData* edidClassNpc = esm.findNPC(kNpcEdidOnlyClass);
    record("NpcClassNameFallsBackToEditorID",
           edidClassNpc != nullptr && edidClassNpc->className == "TestOnlyEdid",
           edidClassNpc ? edidClassNpc->className : "npc missing");

    const oblivion::NPCData* legacyCnamNpc = esm.findNPC(kNpcLegacyCnam);
    record("NpcShortCnamKeptAsString",
           legacyCnamNpc != nullptr && legacyCnamNpc->classFormID == 0 &&
               legacyCnamNpc->className == "AB",
           legacyCnamNpc ? "class=" + legacyCnamNpc->className : "npc missing");

    const oblivion::NPCData* missingClassNpc = esm.findNPC(kNpcMissingClass);
    record("NpcMissingClassKeepsFormId",
           missingClassNpc != nullptr && missingClassNpc->className.empty() &&
               missingClassNpc->classFormID == kAbsentClass,
           missingClassNpc ? "classFormID=" + std::to_string(missingClassNpc->classFormID)
                           : "npc missing");

    // ---- CNAM class link across plugins ----
    {
        const std::vector<uint8_t> externalCnam = u32Payload(kAbsentClass);
        std::vector<uint8_t> pluginImage;
        putRecord(pluginImage, "TES4", 0, std::vector<uint8_t>());
        putRecord(pluginImage, "NPC_", kNpcExternalClass,
                  npcPayload("ExternalClassNpc", "", {}, &externalCnam));

        oblivion::ESMFile plugin;
        const bool pluginParsed =
            plugin.parseFromMemory("plugin.esp", pluginImage.data(), pluginImage.size());
        record("ParsePluginWithoutClas", pluginParsed,
               pluginParsed ? "plugin parsed" : "parse failed");
        if (pluginParsed) {
            record("NpcClassUnresolvedWithoutMaster",
                   !plugin.getNPCs().empty() && plugin.getNPCs()[0].className.empty() &&
                       plugin.getNPCs()[0].classFormID == kAbsentClass,
                   plugin.getNPCs().empty() ? "npc missing" : plugin.getNPCs()[0].className);

            std::unordered_map<uint32_t, std::string> externalClassNames;
            externalClassNames.emplace(kAbsentClass, "Blade");
            const size_t linked = plugin.resolveClassReferences(&externalClassNames);
            record("NpcClassLinkedFromMasterFile",
                   linked == 1 && !plugin.getNPCs().empty() &&
                       plugin.getNPCs()[0].className == "Blade",
                   "linked=" + std::to_string(linked));
        }
    }

    // ---- FormID <-> itemId bridge ----
    record("ItemIdFromFormID", game::itemIdFromFormID(kAbsent) == kAbsentId,
           game::itemIdFromFormID(kAbsent));
    record("ItemIdRoundTrip", game::formIDFromItemId(game::itemIdFromFormID(kGem)) == kGem,
           game::itemIdFromFormID(kGem));
    record("ItemIdFromLowercaseHex", game::formIDFromItemId("0000200a") == 0x0000200A, "0000200a");
    record("ItemIdEmptyRejected", game::formIDFromItemId(std::string()) == 0, "empty -> 0");
    record("ItemIdTooLongRejected", game::formIDFromItemId("000020003") == 0, "9 digits -> 0");
    record("ItemIdNonHexRejected", game::formIDFromItemId("0000ZZZZ") == 0, "non hex -> 0");
    record("ItemIdShortHexAccepted", game::formIDFromItemId("F") == game::kGoldFormID, "F -> 0x0F");

    // ---- base item resolution ----
    const game::ItemInfo goldInfo = game::resolveItemInfo(esm, game::kGoldFormID);
    record("ResolveGoldRecord",
           goldInfo.name == "Gold" && goldInfo.type == ItemType::MISC &&
               nearlyEqual(goldInfo.weight, 0.0f) && goldInfo.value == 1 && goldInfo.resolved,
           "name=" + goldInfo.name);

    const game::ItemInfo appleInfo = game::resolveItemInfo(esm, kApple);
    record("ResolveNamedMisc",
           appleInfo.name == "Test Apple" && appleInfo.type == ItemType::MISC &&
               nearlyEqual(appleInfo.weight, 0.5f) && appleInfo.value == 5 && appleInfo.resolved,
           "name=" + appleInfo.name + " value=" + std::to_string(appleInfo.value));

    const game::ItemInfo gemInfo = game::resolveItemInfo(esm, kGem);
    record("ResolveFallsBackToEditorId",
           gemInfo.name == "TestGem" && gemInfo.value == 250 && gemInfo.resolved,
           "name=" + gemInfo.name);

    const game::ItemInfo absentInfo = game::resolveItemInfo(esm, kAbsent);
    record("ResolveAbsentRecordIsPlaceholder",
           absentInfo.name == "Item_00003000" && !absentInfo.resolved && absentInfo.value == 1 &&
               nearlyEqual(absentInfo.weight, 0.1f),
           "name=" + absentInfo.name);

    const game::ItemInfo nullInfo = game::resolveItemInfo(esm, 0);
    record("ResolveNullFormIDIsNoItem",
           nullInfo.name.empty() && !nullInfo.resolved && nullInfo.value == 0,
           "formID=0 -> no item");

    // ---- merchant detection and construction ----
    record("NpcIsMerchantWithGoldPurse", game::npcIsMerchant(esm, kNpcStocked), "CNTO Gold 250");
    record("NpcIsNotMerchantWithoutGold", !game::npcIsMerchant(esm, kNpcZeroGold), "CNTO Gold 0");
    record("NpcIsNotMerchantWhenAbsent", !game::npcIsMerchant(esm, kUnknownFormID), "0x0000BEEF");
    record("MissingNpcBuildsNothing", buildShop(esm, kUnknownFormID) == nullptr, "0x0000BEEF -> null");

    std::shared_ptr<Merchant> stockedShop = buildShop(esm, kNpcStocked);
    record("MerchantBuiltFromNpc", stockedShop != nullptr, stockedShop ? "built" : "null");
    if (!stockedShop) return getFailCount() == 0;

    record("MerchantShopNamePrefersFullName", stockedShop->getShopName() == "Collected Goods",
           stockedShop->getShopName());
    record("MerchantPurseFromCnto", nearlyEqual(stockedShop->getGold(), 250.0f),
           std::to_string(stockedShop->getGold()));
    record("MerchantStockIsCntoMinusGold",
           stockedShop->getInventory().size() == 3 && stockedShop->findItem(kGoldId) == nullptr,
           "rows=" + std::to_string(stockedShop->getInventory().size()));

    const MerchantItem* appleRow = stockedShop->findItem(kAppleId);
    record("MerchantStockKeepsRecordData",
           appleRow != nullptr && appleRow->itemName == "Test Apple" &&
               nearlyEqual(appleRow->buyPrice, 5.0f) && nearlyEqual(appleRow->sellPrice, 4.0f) &&
               appleRow->quantity == 3 && appleRow->category == ItemType::MISC &&
               nearlyEqual(appleRow->weight, 0.5f) && appleRow->baseValue == 5,
           appleRow ? "apple row carries EDID/FULL and DATA" : "apple row missing");

    const MerchantItem* absentRow = stockedShop->findItem(kAbsentId);
    record("MerchantUnknownStockRow",
           absentRow != nullptr && absentRow->itemName == "Item_00003000" &&
               absentRow->description == "Unknown record" && nearlyEqual(absentRow->weight, 0.1f) &&
               absentRow->baseValue == 1 && absentRow->quantity == 2,
           absentRow ? "placeholder row flagged" : "placeholder row missing");

    std::shared_ptr<Merchant> zeroGoldShop = buildShop(esm, kNpcZeroGold);
    record("ShopNameFallsBackToEditorId",
           zeroGoldShop != nullptr && zeroGoldShop->getShopName() == "EdidOnlyTrader",
           zeroGoldShop ? zeroGoldShop->getShopName() : "null");
    record("ZeroCountGoldKeepsDefaultPurse",
           zeroGoldShop != nullptr && nearlyEqual(zeroGoldShop->getGold(), 5000.0f),
           zeroGoldShop ? std::to_string(zeroGoldShop->getGold()) : "null");

    std::shared_ptr<Merchant> bareShop = buildShop(esm, kNpcNoStock);
    record("EmptyTraderKeepsDefaultPurse",
           bareShop != nullptr && nearlyEqual(bareShop->getGold(), 5000.0f) &&
               bareShop->getInventory().empty(),
           bareShop ? std::to_string(bareShop->getGold()) : "null");

    // ---- buy group: player purse 1000 against the CNTO stock ----
    std::shared_ptr<Merchant> buyShop = buildShop(esm, kNpcStocked);
    if (buyShop) {
        Inventory buyer;
        seedGold(buyer, 1000);
        record("PlayerGoldStackCreated",
               occupiedRows(buyer) == 1 && game::playerGold(buyer) == 1000,
               "gold=" + std::to_string(game::playerGold(buyer)));

        // apple: value 5, canonical buy coefficient ~1.6 -> ceil(7.9996) = 8 per unit.
        const game::BarterResult buy2 = game::buyFromMerchant(buyer, *buyShop, kAppleId, 2, 0.0f);
        record("BuySucceeds", buy2.ok && buy2.gold == 16, "gold=" + std::to_string(buy2.gold));
        record("BuyDebitsPlayerPurse", game::playerGold(buyer) == 984,
               std::to_string(game::playerGold(buyer)));
        record("BuyCreditsMerchantPurse", nearlyEqual(buyShop->getGold(), 266.0f),
               std::to_string(buyShop->getGold()));

        // The purchased stack must keep the record weight and value, otherwise a
        // later sale rebuilds the row as a zero value item.
        const InventorySlot* bought = findPlayerSlot(buyer, kApple);
        record("BuyAddsItemsToPlayer",
               bought != nullptr && bought->quantity == 2 && bought->item.name == "Test Apple" &&
                   bought->item.value == 5 && nearlyEqual(bought->item.weight, 0.5f),
               bought ? "qty=" + std::to_string(bought->quantity) + " value=" +
                            std::to_string(bought->item.value)
                      : "slot missing");

        const MerchantItem* appleAfter = buyShop->findItem(kAppleId);
        record("BuyDecrementsStock", appleAfter != nullptr && appleAfter->quantity == 1,
               appleAfter ? "qty=" + std::to_string(appleAfter->quantity) : "row gone");

        const game::BarterResult buyLast =
            game::buyFromMerchant(buyer, *buyShop, kAppleId, 1, 0.0f);
        record("BuyLastUnitSucceeds", buyLast.ok && buyLast.gold == 8,
               "gold=" + std::to_string(buyLast.gold));
        record("BuyLastUnitDropsStockRow", buyShop->findItem(kAppleId) == nullptr, "row removed");
        record("BuyLastUnitAddsToPlayer", game::countPlayerItem(buyer, kApple) == 3,
               std::to_string(game::countPlayerItem(buyer, kApple)));

        const game::BarterResult noStock =
            game::buyFromMerchant(buyer, *buyShop, kAppleId, 1, 0.0f);
        record("BuyRejectedWithoutStock", !noStock.ok && noStock.gold == 0, "row sold out");

        const game::BarterResult unknown =
            game::buyFromMerchant(buyer, *buyShop, "0000DEAD", 1, 0.0f);
        record("BuyRejectedForUnknownItemId", !unknown.ok && game::playerGold(buyer) == 976,
               std::to_string(game::playerGold(buyer)));

        const game::BarterResult zeroCount =
            game::buyFromMerchant(buyer, *buyShop, kGemId, 0, 0.0f);
        record("BuyRejectedForZeroCount", !zeroCount.ok && game::playerGold(buyer) == 976,
               std::to_string(game::playerGold(buyer)));

        // gem: value 250; a 100 point haggle offset cannot drop the buy below base.
        const game::BarterResult discounted =
            game::buyFromMerchant(buyer, *buyShop, kGemId, 1, 100.0f);
        record("BuyHaggleOffsetFloorsAtBaseValue", discounted.ok && discounted.gold == 250,
               "gold=" + std::to_string(discounted.gold));
        record("BuyDiscountDebitsPlayerPurse", game::playerGold(buyer) == 726,
               std::to_string(game::playerGold(buyer)));
    }

    // No haggle offset can push a buy below the item's base value.
    std::shared_ptr<Merchant> clampShop = buildShop(esm, kNpcStocked);
    if (clampShop) {
        const float lowMercantile = clampShop->calculateBuyPrice(kGemId, 100.0f);
        const float hugeMercantile = clampShop->calculateBuyPrice(kGemId, 500.0f);
        record("BuyHaggleNeverDropsBelowBaseValue",
               nearlyEqual(lowMercantile, 250.0f) && nearlyEqual(hugeMercantile, lowMercantile),
               "100 -> " + std::to_string(lowMercantile) + ", 500 -> " +
                   std::to_string(hugeMercantile));
    }

    // A player without enough gold is rejected and nothing moves.
    std::shared_ptr<Merchant> poorShop = buildShop(esm, kNpcStocked);
    if (poorShop) {
        Inventory poor;
        seedGold(poor, 4);
        const game::BarterResult poorBuy = game::buyFromMerchant(poor, *poorShop, kGemId, 1, 0.0f);
        record("BuyRejectedWithoutGold", !poorBuy.ok && game::playerGold(poor) == 4,
               std::to_string(game::playerGold(poor)));
        record("BuyRejectionKeepsStock",
               poorShop->findItem(kGemId) != nullptr &&
                   poorShop->findItem(kGemId)->quantity == 1 &&
                   nearlyEqual(poorShop->getGold(), 250.0f),
               "purse=" + std::to_string(poorShop->getGold()));
    }

    // Two gold deposits must land in a single row, and paying from that row has to
    // succeed (a purse spread over two slots used to break the payment path).
    {
        Inventory buyer;
        game::addPlayerItem(buyer, game::goldItemInfo(), 8);
        game::addPlayerItem(buyer, game::goldItemInfo(), 8);
        record("PurseMergesAcrossDeposits",
               occupiedRows(buyer) == 1 && game::playerGold(buyer) == 16,
               "rows=" + std::to_string(occupiedRows(buyer)));

        std::shared_ptr<Merchant> splitShop = buildShop(esm, kNpcStocked);
        const MerchantItem* appleRow = splitShop ? splitShop->findItem(kAppleId) : nullptr;
        if (splitShop && appleRow != nullptr) {
            const game::BarterResult bought =
                game::buyFromMerchant(buyer, *splitShop, kAppleId, 2, 0.0f);
            record("BuyFromSplitPurseSucceeds", bought.ok, "gold=" + std::to_string(bought.gold));
            record("BuyFromSplitPurseLeavesNoGold", game::playerGold(buyer) == 0,
                   std::to_string(game::playerGold(buyer)));
            record("BuyFromSplitPurseAddsGoods", game::countPlayerItem(buyer, kApple) == 2,
                   std::to_string(game::countPlayerItem(buyer, kApple)));
        }
    }

    // The shop UI quotes its total with tradeGold, so the quote must match the charge.
    {
        record("TradeGoldRejectsNonPositive",
               game::tradeGold(0.0f, 3) == 0 && game::tradeGold(5.0f, 0) == 0 &&
                   game::tradeGold(-1.0f, 2) == 0 && game::tradeGold(5.0f, -2) == 0,
               "bad inputs never charge");
        record("TradeGoldNeverQuotesZeroForARealItem",
               game::tradeGold(0.4f, 1) == 1 && game::tradeGold(0.1f, 3) == 1,
               "0.4 x 1 -> " + std::to_string(game::tradeGold(0.4f, 1)));
        record("TradeGoldMultipliesAndRounds",
               game::tradeGold(2.5f, 2) == 5 && game::tradeGold(2.0f, 3) == 6,
               "2.5 x 2 -> " + std::to_string(game::tradeGold(2.5f, 2)));

        std::shared_ptr<Merchant> quoteShop = buildShop(esm, kNpcStocked);
        const MerchantItem* quoteRow = quoteShop ? quoteShop->findItem(kAppleId) : nullptr;
        if (quoteShop && quoteRow != nullptr) {
            Inventory quoted;
            seedGold(quoted, 50);
            const int32_t quotedTotal = game::tradeGold(quoteShop->calculateBuyPrice(kAppleId), 2);
            const game::BarterResult bought =
                game::buyFromMerchant(quoted, *quoteShop, kAppleId, 2, 0.0f);
            record("TradeGoldMatchesBuyCharge", bought.ok && bought.gold == quotedTotal,
                   "quote=" + std::to_string(quotedTotal) + " charge=" +
                       std::to_string(bought.gold));
        }

        std::shared_ptr<Merchant> sellShop = buildShop(esm, kNpcNoStock);
        if (sellShop) {
            Inventory quoted;
            seedGold(quoted, 10);
            game::addPlayerItem(quoted, gemInfo, 1);
            const int32_t sellTotal =
                game::tradeGold(Merchant::sellPriceForValue(static_cast<float>(gemInfo.value)), 1);
            const game::BarterResult sold =
                game::sellToMerchant(quoted, *sellShop, kGem, 1, 0.0f);
            record("TradeGoldMatchesSellCredit", sold.ok && sold.gold == sellTotal,
                   "quote=" + std::to_string(sellTotal) + " credit=" +
                       std::to_string(sold.gold));
        }
    }

    // Every refusal has to explain itself, and a successful trade must clear the reason.
    {
        Inventory trader;
        seedGold(trader, 10);
        game::addPlayerItem(trader, appleInfo, 1);

        std::shared_ptr<Merchant> brokenShop = buildShop(esm, kNpcNoStock);
        if (brokenShop) {
            const game::BarterResult outOfStock =
                game::buyFromMerchant(trader, *brokenShop, kAppleId, 1, 0.0f);
            const game::BarterResult zeroCount =
                game::buyFromMerchant(trader, *brokenShop, kAppleId, 0, 0.0f);
            const game::BarterResult unheld =
                game::sellToMerchant(trader, *brokenShop, kGem, 1, 0.0f);
            const game::BarterResult goldSale =
                game::sellToMerchant(trader, *brokenShop, game::kGoldFormID, 1, 0.0f);
            record("BuyRefusalsExplainThemselves",
                   !outOfStock.ok && !outOfStock.reason.empty() && !zeroCount.ok &&
                       !zeroCount.reason.empty(),
                   "out of stock: " + outOfStock.reason);
            record("SellRefusalsExplainThemselves",
                   !unheld.ok && !unheld.reason.empty() && !goldSale.ok &&
                       !goldSale.reason.empty(),
                   "unheld: " + unheld.reason);
        }

        std::shared_ptr<Merchant> tradingShop = buildShop(esm, kNpcStocked);
        if (tradingShop) {
            Inventory roundTripper;
            seedGold(roundTripper, 10);
            const game::BarterResult bought =
                game::buyFromMerchant(roundTripper, *tradingShop, kAppleId, 1, 0.0f);
            const game::BarterResult soldBack =
                game::sellToMerchant(roundTripper, *tradingShop, kApple, 1, 0.0f);
            record("SuccessfulTradesClearReason",
                   bought.ok && bought.reason.empty() && soldBack.ok &&
                       soldBack.reason.empty(),
                   "buy=" + std::to_string(bought.gold) + " sell=" +
                       std::to_string(soldBack.gold));
        }
    }

    // ---- sell group ----
    std::shared_ptr<Merchant> plainShop = buildShop(esm, kNpcNoStock);
    record("SellShopBuilt",
           plainShop != nullptr && plainShop->getShopName() == "EmptyTrader" &&
               plainShop->getInventory().empty() && nearlyEqual(plainShop->getGold(), 5000.0f),
           plainShop ? plainShop->getShopName() : "null");

    // Whole stack: purse 10 + one apple (value 5) sells for ceil(5 * 0.60710678) = 4.
    {
        Inventory seller;
        seedGold(seller, 10);
        game::addPlayerItem(seller, appleInfo, 1);
        std::shared_ptr<Merchant> shop = buildShop(esm, kNpcNoStock);
        if (shop) {
            const game::BarterResult sold =
                game::sellToMerchant(seller, *shop, kApple, 1, 0.0f);
            record("SellWholeStackSucceeds", sold.ok && sold.gold == 4,
                   "gold=" + std::to_string(sold.gold));
            record("SellCreditsPlayerPurse", game::playerGold(seller) == 14,
                   std::to_string(game::playerGold(seller)));
            record("SellDropsEmptiedSlot",
                   occupiedRows(seller) == 1 && game::countPlayerItem(seller, kApple) == 0,
                   "rows=" + std::to_string(occupiedRows(seller)));
            record("SellDebitsMerchantPurse", nearlyEqual(shop->getGold(), 4996.0f),
                   std::to_string(shop->getGold()));

            // The shelf row has to be rebuilt from the sold stack, not from a
            // reference the sale just invalidated.
            const MerchantItem* row = shop->findItem(kAppleId);
            record("SoldRowBuiltFromRecordValue",
                   row != nullptr && row->itemName == "Test Apple" &&
                       row->category == ItemType::MISC && row->baseValue == 5 &&
                       row->quantity == 1 && nearlyEqual(row->weight, 0.5f),
                   row ? "row rebuilt from the sold stack" : "row missing");
            record("SoldRowCostsBuyerFullValue",
                   row != nullptr && nearlyEqual(row->buyPrice, 5.0f) &&
                   nearlyEqual(row->sellPrice, 4.0f),
                   row ? "buy=" + std::to_string(row->buyPrice) + " sell=" +
                             std::to_string(row->sellPrice)
                       : "row missing");
        }
    }

    // Partial stack: purse 10 + three apples, sell two for 2 * 4 = 8.
    {
        Inventory seller;
        seedGold(seller, 10);
        game::addPlayerItem(seller, appleInfo, 3);
        std::shared_ptr<Merchant> shop = buildShop(esm, kNpcNoStock);
        if (shop) {
            const game::BarterResult sold =
                game::sellToMerchant(seller, *shop, kApple, 2, 0.0f);
            record("SellPartOfStackSucceeds", sold.ok && sold.gold == 8,
                   "gold=" + std::to_string(sold.gold));
            const InventorySlot* left = findPlayerSlot(seller, kApple);
            record("SellPartOfStackKeepsRemainder",
                   left != nullptr && left->quantity == 1 && left->item.stackSize == 1 &&
                       occupiedRows(seller) == 2,
                   left ? "left=" + std::to_string(left->quantity) : "slot missing");
            record("SellPartOfStackCreditsPurse", game::playerGold(seller) == 18,
                   std::to_string(game::playerGold(seller)));
        }
    }

    // A row the merchant already stocks only gains quantity; the price stays put.
    {
        Inventory seller;
        seedGold(seller, 10);
        game::addPlayerItem(seller, appleInfo, 2);
        std::shared_ptr<Merchant> shop = buildShop(esm, kNpcStocked);
        if (shop) {
            const game::BarterResult sold =
                game::sellToMerchant(seller, *shop, kApple, 2, 0.0f);
            const MerchantItem* row = shop->findItem(kAppleId);
            record("SellMergesIntoStockedRow",
                   sold.ok && sold.gold == 8 && row != nullptr && row->quantity == 5 &&
                       nearlyEqual(row->buyPrice, 5.0f) && nearlyEqual(row->weight, 0.5f) &&
                       nearlyEqual(shop->getGold(), 242.0f),
                   row ? "qty=" + std::to_string(row->quantity) + " purse=" +
                             std::to_string(shop->getGold())
                       : "row missing");
        }
    }

    // Rejections must leave both sides untouched.
    {
        Inventory seller;
        seedGold(seller, 10);
        game::addPlayerItem(seller, appleInfo, 1);
        std::shared_ptr<Merchant> shop = buildShop(esm, kNpcNoStock);
        if (shop) {
            record("SellRejectsGold",
                   !game::sellToMerchant(seller, *shop, game::kGoldFormID, 1, 0.0f).ok,
                   "purse is not tradable");
            record("SellRejectsNullFormID",
                   !game::sellToMerchant(seller, *shop, 0, 1, 0.0f).ok, "formID 0");
            record("SellRejectsZeroCount", !game::sellToMerchant(seller, *shop, kApple, 0, 0.0f).ok,
                   "count 0");
            record("SellRejectsShortStack",
                   !game::sellToMerchant(seller, *shop, kApple, 2, 0.0f).ok, "holds 1");
            record("SellRejectsUnheldItem",
                   !game::sellToMerchant(seller, *shop, kGem, 1, 0.0f).ok, "holds no gem");
            record("SellRejectionKeepsPlayerPurse", game::playerGold(seller) == 10,
                   std::to_string(game::playerGold(seller)));
            record("SellRejectionLeavesShopStockEmpty",
                   shop->getInventory().empty() && nearlyEqual(shop->getGold(), 5000.0f),
                   "shop untouched");
        }
    }

    // A merchant that cannot pay must not take the goods.
    {
        Inventory seller;
        seedGold(seller, 10);
        game::addPlayerItem(seller, appleInfo, 1);
        std::shared_ptr<Merchant> shop = buildShop(esm, kNpcNoStock);
        if (shop) {
            shop->setGold(1.0f);
            const game::BarterResult sold =
                game::sellToMerchant(seller, *shop, kApple, 1, 0.0f);
            record("SellRejectedByBrokeMerchant", !sold.ok && sold.gold == 0,
                   "purse 1, price 4");
            record("SellRejectionKeepsMerchantPurse", nearlyEqual(shop->getGold(), 1.0f),
                   std::to_string(shop->getGold()));
            record("SellRejectionKeepsPlayerItem", game::countPlayerItem(seller, kApple) == 1,
                   std::to_string(game::countPlayerItem(seller, kApple)));
        }
    }

    // Canonical sell pricing per Oblivion.exe FUN_00488e50 (see RE_EXE_REPORT.md section 57.5).
    {
        Inventory seller;
        seedGold(seller, 10);
        game::addPlayerItem(seller, gemInfo, 1);
        std::shared_ptr<Merchant> shop = buildShop(esm, kNpcNoStock);
        if (shop) {
            const game::BarterResult sold =
                game::sellToMerchant(seller, *shop, kGem, 1, 0.0f);
            record("SellUsesCanonicalFormula", sold.ok && sold.gold == 152,
                   "gold=" + std::to_string(sold.gold));
        }
    }
    {
        Inventory seller;
        seedGold(seller, 10);
        game::addPlayerItem(seller, gemInfo, 1);
        std::shared_ptr<Merchant> shop = buildShop(esm, kNpcNoStock);
        if (shop) {
            const game::BarterResult sold =
                game::sellToMerchant(seller, *shop, kGem, 1, 100.0f);
            record("SellAppliesMercantileBonus",
                       sold.ok && sold.gold == 250 && game::playerGold(seller) == 260,
                   "gold=" + std::to_string(sold.gold));
        }
    }
    record("SellPriceClampsMercantile",
           nearlyEqual(Merchant::sellPriceForValue(250.0f, 100.0f), 100.0f) &&
               nearlyEqual(Merchant::sellPriceForValue(250.0f, 900.0f), 100.0f),
           "250 @ 100 and @ 900 both 100");

    {
        // ceil(0.6071 * 1) leaves one coin, so the sale must still pay one coin.
        Inventory seller;
        seedGold(seller, 10);
        game::addPlayerItem(seller, absentInfo, 1);
        std::shared_ptr<Merchant> shop = buildShop(esm, kNpcNoStock);
        if (shop) {
            const game::BarterResult sold =
                game::sellToMerchant(seller, *shop, kAbsent, 1, 0.0f);
            record("SellOfCheapItemPaysAtLeastOneGold", sold.ok && sold.gold == 1,
                   "gold=" + std::to_string(sold.gold));
        }
    }

    // ---- round trip: selling then buying back must conserve the item data ----
    {
        Inventory trader;
        seedGold(trader, 100);
        game::addPlayerItem(trader, appleInfo, 1);
        std::shared_ptr<Merchant> shop = buildShop(esm, kNpcNoStock);
        if (shop) {
            const game::BarterResult sold =
                game::sellToMerchant(trader, *shop, kApple, 1, 0.0f);
            const game::BarterResult bought =
                game::buyFromMerchant(trader, *shop, kAppleId, 1, 0.0f);
            record("RoundTripTradesSucceed",
                   sold.ok && sold.gold == 4 && bought.ok && bought.gold == 8,
                   "sold for " + std::to_string(sold.gold) + ", bought for " +
                       std::to_string(bought.gold));
            record("RoundTripPlayerPurseNet", game::playerGold(trader) == 96,
                   std::to_string(game::playerGold(trader)));
            record("RoundTripMerchantPurseNet", nearlyEqual(shop->getGold(), 5004.0f),
                   std::to_string(shop->getGold()));

            const InventorySlot* back = findPlayerSlot(trader, kApple);
            record("RoundTripReturnsItemWithRecordValue",
                   back != nullptr && back->quantity == 1 && back->item.value == 5 &&
                       nearlyEqual(back->item.weight, 0.5f),
                   back ? "value=" + std::to_string(back->item.value) : "slot missing");
            record("RoundTripEmptiesShopRow", shop->findItem(kAppleId) == nullptr,
                   "row removed after the buy back");
        }
    }

    // ---- player inventory helpers ----
    {
        Inventory player;
        record("EmptyPlayerGoldIsZero", occupiedRows(player) == 0 && game::playerGold(player) == 0,
               "no rows");
    }
    {
        Inventory player;
        game::addPlayerItem(player, appleInfo, 2);
        game::addPlayerItem(player, appleInfo, 3);
        record("AddMergesEqualFormIDs",
               occupiedRows(player) == 1 && game::countPlayerItem(player, kApple) == 5,
               "rows=" + std::to_string(occupiedRows(player)));
        const InventorySlot* slot = findPlayerSlot(player, kApple);
        record("AddKeepsStackSizeInSync",
               slot != nullptr && slot->quantity == 5 && slot->item.stackSize == 5 &&
                   slot->slotIndex == 0,
               slot ? "qty=" + std::to_string(slot->quantity) : "slot missing");
    }
    {
        // Two deposits of the same FormID must count as one stack.
        Inventory player;
        game::addPlayerItem(player, appleInfo, 2);
        game::addPlayerItem(player, appleInfo, 3);
        record("CountPlayerItemMergesSlots",
               game::countPlayerItem(player, kApple) == 5 && occupiedRows(player) == 1,
               std::to_string(game::countPlayerItem(player, kApple)));
    }
    {
        Inventory player;
        game::addPlayerItem(player, appleInfo, 0);
        game::addPlayerItem(player, appleInfo, -3);
        record("AddIgnoresNonPositiveCount", occupiedRows(player) == 0, "no rows");
    }
    {
        Inventory player;
        game::addPlayerItem(player, game::goldItemInfo(), 7);
        game::addPlayerItem(player, game::goldItemInfo(), 3);
        game::addPlayerItem(player, appleInfo, 1);
        record("GoldStaysOneStack",
               occupiedRows(player) == 2 && game::playerGold(player) == 10 &&
                   game::countPlayerItem(player, kApple) == 1,
               "rows=" + std::to_string(occupiedRows(player)));
    }
    {
        Inventory player;
        game::addPlayerItem(player, appleInfo, 5);
        record("RemovePartialKeepsSlot",
               game::removePlayerItem(player, kApple, 2) && occupiedRows(player) == 1 &&
                   game::countPlayerItem(player, kApple) == 3,
               std::to_string(game::countPlayerItem(player, kApple)));

        const InventorySlot* slot = findPlayerSlot(player, kApple);
        record("RemoveKeepsStackSizeInSync", slot != nullptr && slot->item.stackSize == 3,
               slot ? "stackSize=" + std::to_string(slot->item.stackSize) : "slot missing");
        record("RemoveMoreThanCarriedFails",
               !game::removePlayerItem(player, kApple, 4) &&
                   game::countPlayerItem(player, kApple) == 3,
               "nothing taken");
        record("RemoveZeroFails", !game::removePlayerItem(player, kApple, 0), "count 0");
        record("RemoveUncarriedFails", !game::removePlayerItem(player, kGem, 1), "holds no gem");
        record("RemoveWholeStackDropsSlot",
               game::removePlayerItem(player, kApple, 3) && occupiedRows(player) == 0,
               "rows=" + std::to_string(occupiedRows(player)));
    }
    {
        Inventory player;
        game::addPlayerItem(player, game::goldItemInfo(), 10);
        game::addPlayerItem(player, appleInfo, 1);
        game::addPlayerItem(player, gemInfo, 1);
        record("SlotsAreAppended",
               occupiedRows(player) == 3 && game::playerGold(player) == 10 &&
                   game::countPlayerItem(player, kApple) == 1 &&
                   game::countPlayerItem(player, kGem) == 1,
               "rows=" + std::to_string(occupiedRows(player)));

        // Dropping a stack has to leave the other stacks intact; the store keeps an
        // emptied row in place instead of renumbering the rest.
        game::removePlayerItem(player, kApple, 1);
        record("RemovingARowLeavesTheOthersInPlace",
               occupiedRows(player) == 2 &&
                   game::countPlayerItem(player, kApple) == 0 &&
                   game::countPlayerItem(player, kGem) == 1 &&
                   game::playerGold(player) == 10,
               "rows=" + std::to_string(occupiedRows(player)));

        game::removePlayerItem(player, game::kGoldFormID, 10);
        record("RemoveWholeGoldStackDropsSlot",
               occupiedRows(player) == 1 && game::countPlayerItem(player, kGem) == 1 &&
                   game::playerGold(player) == 0,
               "rows=" + std::to_string(occupiedRows(player)));
    }
    {
        // Taking items out of a container used to append a second Gold row, so one purse
        // could sit in two slots. Deposits now consolidate by FormID, and removal has to
        // draw across the whole purse.
        Inventory player;
        game::addPlayerItem(player, game::goldItemInfo(), 30);
        game::addPlayerItem(player, game::goldItemInfo(), 70);

        record("SplitPurseCountsAsOneStack", game::playerGold(player) == 100,
               std::to_string(game::playerGold(player)));
        record("RemoveSpansSplitSlots",
               game::removePlayerItem(player, game::kGoldFormID, 50), "takes 50 of 100");

        const InventorySlot* left = findPlayerSlot(player, game::kGoldFormID);
        record("RemoveSpansSplitSlotsResult",
               occupiedRows(player) == 1 && left != nullptr && left->quantity == 50 &&
                   left->item.stackSize == 50 && left->slotIndex == 0 &&
                   game::playerGold(player) == 50,
               "rows=" + std::to_string(occupiedRows(player)));

        record("RemoveMoreThanSplitTotalFails",
               !game::removePlayerItem(player, game::kGoldFormID, 51) &&
                   game::playerGold(player) == 50,
               "nothing taken");
        record("RemoveWholeSplitTotalDropsEverySlot",
               game::removePlayerItem(player, game::kGoldFormID, 50) && occupiedRows(player) == 0,
               "rows=" + std::to_string(occupiedRows(player)));
    }
    {
        const game::ItemInfo info = game::goldItemInfo();
        record("GoldItemInfo",
               info.formID == game::kGoldFormID && info.name == "Gold" &&
                   info.type == ItemType::MISC && info.value == 1 &&
                   nearlyEqual(info.weight, 0.0f) && info.resolved,
               "name=" + info.name);
    }
    {
        // Draining the purse and topping it up again has to stay on a single row, so the
        // shop never lists the same FormID twice.
        Inventory player;
        game::addPlayerItem(player, game::goldItemInfo(), 30);
        record("RefilledPurseStartsOnOneRow", occupiedRows(player) == 1,
               "rows=" + std::to_string(occupiedRows(player)));

        game::removePlayerItem(player, game::kGoldFormID, 30);
        game::addPlayerItem(player, game::goldItemInfo(), 10);

        const InventorySlot* purse = findPlayerSlot(player, game::kGoldFormID);
        record("RefilledPurseKeepsSingleRow",
               occupiedRows(player) == 1 && purse != nullptr && purse->quantity == 10 &&
                   game::playerGold(player) == 10,
               "rows=" + std::to_string(occupiedRows(player)));
    }

    return getFailCount() == 0;
}
