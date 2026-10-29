// Merchant persistence tests - see merchant_persistence_tests.h for the rationale.

#include "merchant_persistence_tests.h"

#include "../game/merchant.h"
#include "../save_system/save_format_version.h"
#include "../save_system/save_manager.h"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace fs = std::filesystem;

namespace {

// Distinct slots so a developer's real saves in host_saves/ are never touched.
constexpr uint32_t kSlotA = 92;
constexpr uint32_t kSlotB = 93;

constexpr uint32_t kMerchantFormID = 0x0001B001;
constexpr uint32_t kUnknownFormID = 0x0001B002;

void removeSlot(SaveManager& manager, uint32_t slotIndex) {
    std::error_code ec;
    fs::remove(manager.getSlotManager().getSlotPath(slotIndex), ec);
}

// Rewrites only formatVersion in the file header. The CRC covers the payload,
// so a header-only edit leaves the file loading cleanly while making the reader
// take the older code path - the cheapest way to prove a v4 save (which ends
// after CNTR) ignores a trailing MERC section instead of misreading it.
bool forceHeaderVersion(const std::string& path, uint32_t version) {
    std::fstream file(path, std::ios::in | std::ios::out | std::ios::binary);
    if (!file.is_open()) return false;

    file.seekp(offsetof(save_format::SaveHeader, formatVersion), std::ios::beg);
    file.write(reinterpret_cast<const char*>(&version), sizeof(version));
    return file.good();
}

std::shared_ptr<Merchant> makeMerchant(uint32_t formID,
                                       const std::string& name = "Test Goods") {
    return std::make_shared<Merchant>(formID, name, "General Goods");
}

void seedStarterStock(const std::shared_ptr<Merchant>& merchant) {
    // Stands in for the stock a merchant is built with from its plugin records.
    merchant->addItem(MerchantItem("plugin_row", "Plugin Row", 9.0f, 4.0f, 2,
                                   ItemType::INGREDIENT, "From the plugin.",
                                   0.1f, 9));
}

void seedSavedStock(const std::shared_ptr<Merchant>& merchant) {
    merchant->setGold(1234.5f);
    merchant->addItem(MerchantItem("test_apple", "Test Apple", 5.0f, 2.0f, 3,
                                   ItemType::POTION, "A crunchy apple.", 0.5f, 5));
    merchant->addItem(MerchantItem("test_gem", "Test Gem", 250.0f, 100.0f, 1,
                                   ItemType::WEAPON, "Shiny.", 1.25f, 250));
}

}  // namespace

void MerchantPersistenceTests::record(const std::string& name, bool passed,
                                      const std::string& msg, float ms) {
    results.push_back({name, passed, msg, ms});
}

int MerchantPersistenceTests::getPassCount() const {
    int n = 0;
    for (const auto& r : results) if (r.passed) n++;
    return n;
}

int MerchantPersistenceTests::getFailCount() const {
    int n = 0;
    for (const auto& r : results) if (!r.passed) n++;
    return n;
}

std::string MerchantPersistenceTests::getSummary() const {
    std::string summary =
        "Merchant Persistence Test Results\n"
        "Total: " + std::to_string(results.size()) +
        " | Pass: " + std::to_string(getPassCount()) +
        " | Fail: " + std::to_string(getFailCount());
    for (const auto& r : results) {
        if (!r.passed) {
            summary += "\n  [FAIL] " + r.testName;
            if (!r.message.empty()) summary += " - " + r.message;
        }
    }
    return summary;
}

bool MerchantPersistenceTests::runAllTests() {
    results.clear();
    const auto t0 = std::chrono::high_resolution_clock::now();

    SaveManager saver;
    record("SaveManager initializes", saver.initialize());
    removeSlot(saver, kSlotA);
    removeSlot(saver, kSlotB);

    record("save format is v5 or newer", save_format::CURRENT_VERSION >= 5,
           "CURRENT_VERSION=" + std::to_string(save_format::CURRENT_VERSION));

    // --- Materialised merchant round trip ------------------------------------
    auto shop = makeMerchant(kMerchantFormID);
    seedSavedStock(shop);

    std::unordered_map<uint32_t, std::shared_ptr<Merchant>> registry;
    registry.emplace(kMerchantFormID, shop);
    saver.setMerchantRegistry(&registry);

    const bool saved = saver.initialize() && saver.saveGame(kSlotA, "Merchant Test A");
    record("saveGame writes the MERC section", saved);

    // A fresh manager models a process restart: its registry is empty because
    // merchants are built lazily from the NPC record.
    SaveManager loader;
    const bool loaded = loader.initialize() && loader.loadGame(kSlotA);
    record("loadGame accepts a v5 save with merchants", loaded);

    auto restored = makeMerchant(kMerchantFormID, "ESM Goods");
    seedStarterStock(restored);
    const bool applied = loader.applyPendingMerchantState(restored);
    record("pending state applies to the freshly materialised merchant", applied);

    record("purse round trips", restored->getGold() == 1234.5f,
           "got " + std::to_string(restored->getGold()));

    const auto& items = restored->getInventory();
    record("stock replaces the plugin rows instead of merging",
           items.size() == 2 && restored->findItem("plugin_row") == nullptr,
           "rows=" + std::to_string(items.size()));

    if (items.size() == 2) {
        record("first item id", items[0].itemId == "test_apple");
        record("first item name", items[0].itemName == "Test Apple");
        record("first item prices",
               items[0].buyPrice == 5.0f && items[0].sellPrice == 2.0f);
        record("first item quantity", items[0].quantity == 3,
               "got " + std::to_string(items[0].quantity));
        record("first item category", items[0].category == ItemType::POTION);
        record("first item description", items[0].description == "A crunchy apple.");
        record("first item weight", items[0].weight == 0.5f);
        record("first item base value", items[0].baseValue == 5);
        record("second item id", items[1].itemId == "test_gem");
        record("second item category", items[1].category == ItemType::WEAPON);
        record("second item weight", items[1].weight == 1.25f);
        record("second item base value", items[1].baseValue == 250);
    } else {
        record("merchant stock round trips", false,
               "expected 2 items, got " + std::to_string(items.size()));
    }

    // The state is handed over exactly once; a second apply must fall back to
    // the plugin stock rather than replay the save.
    record("pending state is consumed by the first apply",
           !loader.applyPendingMerchantState(restored));
    record("unknown FormID reports no pending state",
           !loader.applyPendingMerchantState(makeMerchant(kUnknownFormID, "Other")));
    record("null merchant reports no pending state",
           !loader.applyPendingMerchantState(nullptr));

    // --- Explicit re-load into a live registry -------------------------------
    SaveManager reloader;
    auto live = makeMerchant(kMerchantFormID, "Live Goods");
    live->setGold(1.0f);
    seedStarterStock(live);
    std::unordered_map<uint32_t, std::shared_ptr<Merchant>> liveRegistry;
    liveRegistry.emplace(kMerchantFormID, live);
    reloader.setMerchantRegistry(&liveRegistry);

    const bool reloaded = reloader.initialize() && reloader.loadGame(kSlotA);
    record("reload into a live registry succeeds", reloaded);
    record("live merchant receives the saved purse", live->getGold() == 1234.5f,
           "got " + std::to_string(live->getGold()));
    record("live merchant stock is replaced",
           live->getInventory().size() == 2 && live->findItem("plugin_row") == nullptr,
           "rows=" + std::to_string(live->getInventory().size()));
    record("state is consumed during the load",
           !reloader.applyPendingMerchantState(live));

    // --- Sold-out rows are not persisted -------------------------------------
    SaveManager soldOut;
    auto depleted = makeMerchant(kMerchantFormID, "Depleted Goods");
    depleted->setGold(10.0f);
    depleted->addItem(MerchantItem("sold_out", "Sold Out", 1.0f, 1.0f, 0,
                                   ItemType::MISC, "", 0.0f, 1));
    depleted->addItem(MerchantItem("in_stock", "In Stock", 1.0f, 1.0f, 7,
                                   ItemType::MISC, "", 0.0f, 1));
    std::unordered_map<uint32_t, std::shared_ptr<Merchant>> depletedRegistry;
    depletedRegistry.emplace(kMerchantFormID, depleted);
    soldOut.setMerchantRegistry(&depletedRegistry);
    record("sold-out merchant saves",
           soldOut.initialize() && soldOut.saveGame(kSlotB, "Merchant Test B"));

    SaveManager depletedLoader;
    record("sold-out save reloads", depletedLoader.initialize() &&
                                    depletedLoader.loadGame(kSlotB));
    auto depletedRestored = makeMerchant(kMerchantFormID, "ESM Goods");
    record("sold-out merchant applies",
           depletedLoader.applyPendingMerchantState(depletedRestored));
    record("quantity 0 rows are dropped, live rows kept",
           depletedRestored->getInventory().size() == 1 &&
               depletedRestored->findItem("sold_out") == nullptr &&
               depletedRestored->findItem("in_stock") != nullptr,
           "rows=" + std::to_string(depletedRestored->getInventory().size()));

    // --- Carry-forward: saved before the merchant was ever materialised ------
    SaveManager carry;
    // No registry is set: the player never opened this shop, so the purse and
    // stock only exist in the pending state restored from slot A.
    const bool carryLoaded = carry.initialize() && carry.loadGame(kSlotA);
    record("carry-forward source loads", carryLoaded);

    // Saving now must write the still-unmaterialised merchant back out.
    const bool carrySaved = carry.saveGame(kSlotB, "Merchant Test B");
    record("save before materialisation succeeds", carrySaved);

    SaveManager finalLoader;
    const bool finalLoaded = finalLoader.initialize() && finalLoader.loadGame(kSlotB);
    record("carry-forward save reloads", finalLoaded);

    auto carried = makeMerchant(kMerchantFormID, "ESM Goods");
    const bool carriedApplied = finalLoader.applyPendingMerchantState(carried);
    record("carry-forward keeps the unmaterialised merchant", carriedApplied);
    record("carry-forward keeps the purse", carried->getGold() == 1234.5f,
           "got " + std::to_string(carried->getGold()));
    record("carry-forward keeps the stock", carried->getInventory().size() == 2,
           "rows=" + std::to_string(carried->getInventory().size()));

    // --- v4 backward compatibility -------------------------------------------
    const std::string slotAPath = saver.getSlotManager().getSlotPath(kSlotA);
    record("v5 header downgrades to v4 for the compatibility check",
           forceHeaderVersion(slotAPath, 4));

    SaveManager v4Loader;
    const bool v4Loaded = v4Loader.initialize() && v4Loader.loadGame(kSlotA);
    record("a v4 save with a trailing MERC section still loads", v4Loaded);
    auto v4Merchant = makeMerchant(kMerchantFormID, "ESM Goods");
    record("a v4 save reports no merchant state",
           !v4Loader.applyPendingMerchantState(v4Merchant));

    removeSlot(saver, kSlotA);
    removeSlot(saver, kSlotB);

    const auto t1 = std::chrono::high_resolution_clock::now();
    const float totalMs =
        std::chrono::duration<float, std::milli>(t1 - t0).count();
    if (!results.empty()) results.back().durationMs = totalMs;

    return getFailCount() == 0;
}
