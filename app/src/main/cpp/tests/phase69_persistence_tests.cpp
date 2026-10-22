// Phase 69 persistence tests - see phase69_persistence_tests.h for the rationale.

#include "phase69_persistence_tests.h"

#include "../game/container.h"
#include "../save_system/save_format_version.h"
#include "../save_system/save_manager.h"
#include "../world/world_manager.h"

#include <chrono>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

// Distinct slots so a developer's real saves in host_saves/ are never touched.
constexpr uint32_t kSlotA = 90;
constexpr uint32_t kSlotB = 91;

constexpr uint32_t kChestFormID = 0x0001A2B3;
constexpr uint32_t kBarrelFormID = 0x0001A2B4;

InventoryItem makeItem(uint32_t id, const std::string& name, ItemType type,
                       uint32_t quantity, float weight, uint32_t value) {
    InventoryItem slot;
    slot.item.itemId = id;
    slot.item.name = name;
    slot.item.type = type;
    slot.item.weight = weight;
    slot.item.value = value;
    slot.item.stackSize = quantity;
    slot.quantity = quantity;
    return slot;
}

std::shared_ptr<Container> makeContainer(uint32_t formID) {
    auto worldObject = std::make_shared<WorldObject>();
    worldObject->objectId = formID;
    worldObject->objectName = "Test Container";
    worldObject->isStatic = false;
    worldObject->isInteractable = true;

    auto container = std::make_shared<Container>(worldObject);
    container->setFormID(formID);
    return container;
}

void removeSlot(SaveManager& manager, uint32_t slotIndex) {
    std::error_code ec;
    fs::remove(manager.getSlotManager().getSlotPath(slotIndex), ec);
}

}  // namespace

void Phase69PersistenceTests::record(const std::string& name, bool passed,
                                     const std::string& msg, float ms) {
    results.push_back({name, passed, msg, ms});
}

int Phase69PersistenceTests::getPassCount() const {
    int n = 0;
    for (const auto& r : results) if (r.passed) n++;
    return n;
}

int Phase69PersistenceTests::getFailCount() const {
    int n = 0;
    for (const auto& r : results) if (!r.passed) n++;
    return n;
}

std::string Phase69PersistenceTests::getSummary() const {
    std::string summary =
        "Phase 69 Persistence Test Results\n"
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

bool Phase69PersistenceTests::runAllTests() {
    results.clear();
    const auto t0 = std::chrono::high_resolution_clock::now();

    SaveManager saver;
    const bool saverReady = saver.initialize();
    record("SaveManager initializes", saverReady);
    removeSlot(saver, kSlotA);
    removeSlot(saver, kSlotB);

    record("save format is v4 or newer", save_format::CURRENT_VERSION >= 4,
           "CURRENT_VERSION=" + std::to_string(save_format::CURRENT_VERSION));

    // --- Materialised container round trip -----------------------------------
    auto chest = makeContainer(kChestFormID);
    chest->addItem(makeItem(0x0001E0A1, "Iron Dagger", ItemType::WEAPON, 1, 3.0f, 10));
    chest->addItem(makeItem(0x0001E0B2, "Minor Healing Potion", ItemType::POTION, 4, 0.5f, 25));
    chest->setLocked(true);

    WorldManager worldA;
    worldA.registerContainer(chest);
    saver.setWorldManager(&worldA);

    record("registry keeps container by FormID",
           worldA.getContainerByFormID(kChestFormID) == chest);
    record("registry rejects runtime-only container (FormID 0)",
           worldA.getContainerByFormID(0) == nullptr);

    const bool saved = saver.initialize() && saver.saveGame(kSlotA, "Phase69 Test A");
    record("saveGame writes the CNTR section", saved);

    // A fresh manager models a process restart: no registry entries at all.
    SaveManager loader;
    WorldManager worldB;  // deliberately empty - the container does not exist yet
    loader.setWorldManager(&worldB);

    const bool loaded = loader.initialize() && loader.loadGame(kSlotA);
    record("loadGame accepts a v4 save with containers", loaded);

    auto restored = makeContainer(kChestFormID);
    const bool applied = loader.applyPendingContainerState(restored);
    record("pending contents apply to the freshly materialised container", applied);

    record("restored item count", restored->getItemCount() == 2,
           "got " + std::to_string(restored->getItemCount()));

    const auto& items = restored->getItems();
    if (items.size() == 2) {
        record("first item id", items[0].item.itemId == 0x0001E0A1);
        record("first item name", items[0].item.name == "Iron Dagger");
        record("first item type", items[0].item.type == ItemType::WEAPON);
        record("first item weight", items[0].item.weight == 3.0f);
        record("second item id", items[1].item.itemId == 0x0001E0B2);
        record("second item quantity", items[1].quantity == 4,
               "got " + std::to_string(items[1].quantity));
        record("second item value", items[1].item.value == 25);
        record("second item type", items[1].item.type == ItemType::POTION);
    } else {
        record("container contents round trip", false,
               "expected 2 items, got " + std::to_string(items.size()));
    }
    record("locked flag round trips", restored->isLocked());

    // The state is handed over exactly once; the caller falls back to the ESM
    // for anything the save did not know about.
    record("pending state is consumed by the first apply",
           !loader.applyPendingContainerState(restored));
    record("unknown FormID reports no pending state",
           !loader.applyPendingContainerState(makeContainer(kBarrelFormID)));

    // --- Empty container round trip ------------------------------------------
    SaveManager emptySaver;
    auto barrel = makeContainer(kBarrelFormID);
    WorldManager worldC;
    worldC.registerContainer(barrel);
    emptySaver.setWorldManager(&worldC);
    const bool emptySaved = emptySaver.initialize() &&
                            emptySaver.saveGame(kSlotB, "Phase69 Test B");
    record("empty container is still written", emptySaved);

    SaveManager emptyLoader;
    WorldManager worldD;
    emptyLoader.setWorldManager(&worldD);
    const bool emptyLoaded = emptyLoader.initialize() && emptyLoader.loadGame(kSlotB);
    record("empty container reloads", emptyLoaded);

    auto emptyRestored = makeContainer(kBarrelFormID);
    record("empty container applies", emptyLoader.applyPendingContainerState(emptyRestored));
    record("empty container stays empty", emptyRestored->getItemCount() == 0);
    record("empty container stays unlocked", !emptyRestored->isLocked());

    // --- Carry-forward: saved before the container was ever materialised -----
    SaveManager carry;
    WorldManager worldE;  // no containers: the player never opened the chest
    carry.setWorldManager(&worldE);
    const bool carryLoaded = carry.initialize() && carry.loadGame(kSlotA);
    record("carry-forward source loads", carryLoaded);

    // Saving now must not drop the buffered contents.
    const bool carrySaved = carry.saveGame(kSlotB, "Phase69 Test B");
    record("save before materialisation succeeds", carrySaved);

    SaveManager finalLoader;
    WorldManager worldF;
    finalLoader.setWorldManager(&worldF);
    const bool finalLoaded = finalLoader.initialize() && finalLoader.loadGame(kSlotB);
    record("carry-forward save reloads", finalLoaded);

    auto carried = makeContainer(kChestFormID);
    const bool carriedApplied = finalLoader.applyPendingContainerState(carried);
    record("carry-forward keeps the unmaterialised container", carriedApplied);
    record("carry-forward keeps its contents", carried->getItemCount() == 2,
           "got " + std::to_string(carried->getItemCount()));
    record("carry-forward keeps the locked flag", carried->isLocked());

    removeSlot(saver, kSlotA);
    removeSlot(saver, kSlotB);

    const auto t1 = std::chrono::high_resolution_clock::now();
    const float totalMs =
        std::chrono::duration<float, std::milli>(t1 - t0).count();
    if (!results.empty()) results.back().durationMs = totalMs;

    return getFailCount() == 0;
}
