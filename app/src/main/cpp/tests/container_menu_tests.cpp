#include "container_menu_tests.h"
#include "../ui/ui_container_menu.h"
#include "../game/container.h"
#include "../game/player.h"
#include "../game/item.h"

#include <cmath>
#include <memory>

namespace {

constexpr float kEps = 1e-4f;

bool near(float a, float b) {
    return std::fabs(a - b) < kEps;
}

std::shared_ptr<Container> makeTestContainer() {
    auto obj = std::make_shared<WorldObject>();
    auto container = std::make_shared<Container>(obj);
    container->setCapacity(100.0f);

    InventorySlot sword;
    sword.item = Item(1, "Iron Sword", ItemType::WEAPON, 10.0f, 50, 1);
    sword.quantity = 1;
    container->addItem(sword);

    InventorySlot potion;
    potion.item = Item(2, "Health Potion", ItemType::POTION, 0.5f, 25, 10);
    potion.quantity = 3;
    container->addItem(potion);

    InventorySlot book;
    book.item = Item(3, "Scroll of Fire", ItemType::SCROLL, 0.2f, 100, 1);
    book.quantity = 1;
    container->addItem(book);

    InventorySlot ingr;
    ingr.item = Item(4, "Nightshade", ItemType::INGREDIENT, 0.1f, 8, 10);
    ingr.quantity = 5;
    container->addItem(ingr);

    return container;
}

Player* makeTestPlayer() {
    auto player = new Player();
    player->initialize(glm::vec3(0.0f));

    InventorySlot armor;
    armor.item = Item(10, "Iron Cuirass", ItemType::ARMOR, 25.0f, 200, 1);
    armor.quantity = 1;
    player->inventory.push_back(armor);

    InventorySlot gold;
    gold.item = Item(11, "Gold Coin", ItemType::MISC, 0.01f, 1, 100);
    gold.quantity = 50;
    player->inventory.push_back(gold);

    return player;
}

} // namespace

void ContainerMenuTests::record(const std::string& name, bool passed,
                                const std::string& msg) {
    results.push_back({name, passed, msg});
}

bool ContainerMenuTests::near(float a, float b, float eps) {
    return std::fabs(a - b) < eps;
}

int ContainerMenuTests::getPassCount() const {
    int n = 0;
    for (const auto& r : results) if (r.passed) n++;
    return n;
}

int ContainerMenuTests::getFailCount() const {
    int n = 0;
    for (const auto& r : results) if (!r.passed) n++;
    return n;
}

std::string ContainerMenuTests::getSummary() const {
    return "Container Menu Test Results\n"
           "Total: " + std::to_string(results.size()) +
           " | Pass: " + std::to_string(getPassCount()) +
           " | Fail: " + std::to_string(getFailCount());
}

bool ContainerMenuTests::runAllTests() {
    results.clear();

    // --- 1. Item type to category mapping ---
    {
        record("WEAPON maps to kCategoryWeapon",
               UIContainerMenu::itemTypeToCategory(static_cast<int>(ItemType::WEAPON)) == 1);
        record("ARMOR maps to kCategoryApparel",
               UIContainerMenu::itemTypeToCategory(static_cast<int>(ItemType::ARMOR)) == 2);
        record("CLOTHING maps to kCategoryApparel",
               UIContainerMenu::itemTypeToCategory(static_cast<int>(ItemType::CLOTHING)) == 2);
        record("POTION maps to kCategoryPotions",
               UIContainerMenu::itemTypeToCategory(static_cast<int>(ItemType::POTION)) == 4);
        record("SCROLL maps to kCategoryPotions",
               UIContainerMenu::itemTypeToCategory(static_cast<int>(ItemType::SCROLL)) == 4);
        record("INGREDIENT maps to kCategoryIngredients",
               UIContainerMenu::itemTypeToCategory(static_cast<int>(ItemType::INGREDIENT)) == 5);
        record("BOOK maps to kCategoryBooks",
               UIContainerMenu::itemTypeToCategory(static_cast<int>(ItemType::BOOK)) == 6);
        record("MISC maps to kCategoryMisc",
               UIContainerMenu::itemTypeToCategory(static_cast<int>(ItemType::MISC)) == 9);
    }

    // --- 2. Category labels ---
    {
        record("label[1] == Weapons",
               std::string(UIContainerMenu::categoryLabel(1)) == "Weapons");
        record("label[4] == Potions",
               std::string(UIContainerMenu::categoryLabel(4)) == "Potions");
    }

    // --- 3. Container menu lifecycle ---
    {
        UIContainerMenu menu;
        record("starts invisible", !menu.isVisible());
        record("starts with no player", menu.getPlayer() == nullptr);
        record("starts with no container", menu.getContainer() == nullptr);
    }

    // --- 4. Take item from container to player ---
    {
        auto container = makeTestContainer();
        std::unique_ptr<Player> player(makeTestPlayer());

        UIContainerMenu menu;
        menu.open(container, player.get());

        record("menu visible after open", menu.isVisible());
        record("container set after open", menu.getContainer() == container);
        record("player set after open", menu.getPlayer() == player.get());
        record("container has 4 items", container->getItemCount() == 4);
        record("player has 2 items", player->inventory.size() == 2);

        menu.takeItem(0);

        record("container has 3 items after take", container->getItemCount() == 3);
        record("player has 3 items after take", player->inventory.size() == 3);

        // Verify the taken item is in player inventory
        bool foundIronSword = false;
        for (const auto& slot : player->inventory) {
            if (slot.item.name == "Iron Sword") {
                foundIronSword = true;
                break;
            }
        }
        record("Iron Sword found in player inventory after take", foundIronSword);

        menu.close();
        record("menu invisible after close", !menu.isVisible());
    }

    // --- 5. Deposit item from player to container ---
    {
        auto container = makeTestContainer();
        std::unique_ptr<Player> player(makeTestPlayer());
        float initialWeight = container->getTotalWeight();

        UIContainerMenu menu;
        menu.open(container, player.get());

        size_t initialContainerCount = container->getItemCount();
        size_t initialPlayerCount = player->inventory.size();

        menu.depositItem(1); // Gold Coin is at index 1 in playerItems

        record("container count increased after deposit",
               container->getItemCount() == initialContainerCount + 1);
        record("player count decreased after deposit",
               player->inventory.size() == initialPlayerCount - 1);
        record("container weight increased after deposit",
               container->getTotalWeight() > initialWeight);

        bool goldFound = false;
        for (const auto& slot : player->inventory) {
            if (slot.item.name == "Gold Coin") {
                goldFound = true;
                break;
            }
        }
        record("Gold Coin removed from player after deposit", !goldFound);
    }

    // --- 6. Take All ---
    {
        auto container = makeTestContainer();
        std::unique_ptr<Player> player(makeTestPlayer());

        UIContainerMenu menu;
        menu.open(container, player.get());

        menu.takeAll();

        record("container empty after takeAll", container->getItemCount() == 0);
        record("all items moved to player", player->inventory.size() == 6);
    }

    // --- 7. Back button closes menu ---
    {
        auto container = makeTestContainer();
        std::unique_ptr<Player> player(makeTestPlayer());

        UIContainerMenu menu;
        menu.open(container, player.get());
        record("menu open before back", menu.isVisible());

        bool handled = menu.handleBack();
        record("handleBack returns true", handled);
        record("menu closed after back", !menu.isVisible());
    }

    return getFailCount() == 0;
}