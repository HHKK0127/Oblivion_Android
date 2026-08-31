#include "save_manager.h"
#include "../game/player_controller.h"
#include "../game/inventory_manager.h"
#include "../game/spell_manager.h"
#include "../game/quest_manager.h"
#include "../world/world_manager.h"
#include "../script/script_manager.h"
#include <fstream>
#include <sstream>
#include <cstring>
#include <ctime>
#include <chrono>
#include <filesystem>
#include <android/log.h>

namespace fs = std::filesystem;

// ============================================================================
// Initialization
// ============================================================================

bool SaveManager::initialize() {
    std::string baseDir = getBaseDir();
    if (!slotManager_.initialize(baseDir)) {
        LOGE("Failed to initialize SaveSlotManager");
        return false;
    }

    // Initialize auto-save with callback
    autoSave_.initialize([this](uint32_t slotIndex) -> bool {
        return saveGame(slotIndex, "Auto-Save");
    });

    LOGI("SaveManager initialized (binary format v%u)", save_format::CURRENT_VERSION);
    return true;
}

std::string SaveManager::getBaseDir() const {
    return "/data/data/com.example.oblivion/files/saves/";
}

// ============================================================================
// Binary Save/Load (Phase 41)
// ============================================================================

bool SaveManager::saveGame(uint32_t slotIndex, const std::string& slotName) {
    try {
        BinaryWriter writer;

        // Serialize PlayerController
        writer.writeUint32(0x504C5952);  // "PLYR" marker
        if (playerController_) {
            auto player = playerController_->getPlayer();
            if (player) {
                writer.writeVec3(player->position);
                writer.writeVec3(player->rotation);
                writer.writeVec3(player->velocity);
                writer.writeFloat(player->health);
                writer.writeFloat(player->maxHealth);
                writer.writeFloat(player->stamina);
                writer.writeFloat(player->maxStamina);
                writer.writeUint32(player->playerLevel);
                writer.writeFloat(player->experience);
                writer.writeFloat(player->experiencePerLevelUp);
                writer.writeInt32(player->currentCellX);
                writer.writeInt32(player->currentCellY);
                writer.writeUint8(static_cast<uint8_t>(player->movementState));

                // Skills
                writer.writeInt32(player->skills.Blade);
                writer.writeInt32(player->skills.Blunt);
                writer.writeInt32(player->skills.Block);
                writer.writeInt32(player->skills.Restoration);
                writer.writeInt32(player->skills.Destruction);
                writer.writeInt32(player->skills.Alteration);
                writer.writeInt32(player->skills.Conjuration);
                writer.writeInt32(player->skills.Illusion);
                writer.writeInt32(player->skills.Mysticism);
                writer.writeInt32(player->skills.Marksman);
                writer.writeInt32(player->skills.Athletics);
                writer.writeInt32(player->skills.Acrobatics);

                // Attributes
                writer.writeInt32(player->attributes.Strength);
                writer.writeInt32(player->attributes.Intelligence);
                writer.writeInt32(player->attributes.Willpower);
                writer.writeInt32(player->attributes.Agility);
                writer.writeInt32(player->attributes.Speed);
                writer.writeInt32(player->attributes.Endurance);
                writer.writeInt32(player->attributes.Personality);
                writer.writeInt32(player->attributes.Luck);

                // Physics state
                writer.writeBool(player->isOnGround);
                writer.writeFloat(player->speed);
                writer.writeFloat(player->sprintSpeed);

                // Quick-slot spell IDs
                for (int i = 0; i < Player::QUICK_SLOT_COUNT; ++i) {
                    uint32_t spellId = 0;
                    if (player->quickSlotSpells[i]) {
                        spellId = player->quickSlotSpells[i]->spellId;
                    }
                    writer.writeUint32(spellId);
                }

                // Player controller state
                writer.writeBool(playerController_->isInCombatStance());
                writer.writeUint8(static_cast<uint8_t>(playerController_->getAnimState()));

                LOGD("Serialized player: pos=(%.1f,%.1f,%.1f) lvl=%u hp=%.0f",
                     player->position.x, player->position.y, player->position.z,
                     player->playerLevel, player->health);
            }
        }

        // Serialize InventoryManager
        writer.writeUint32(0x494E5654);  // "INVT" marker
        if (inventoryManager_) {
            auto inv = inventoryManager_->getPlayerInventory();
            if (inv) {
                const auto& slots = inv->getAllSlots();
                writer.writeUint32(static_cast<uint32_t>(slots.size()));
                for (const auto& slot : slots) {
                    writer.writeUint32(slot.item.itemId);
                    writer.writeUint32(slot.quantity);
                    writer.writeString(slot.item.name);
                    writer.writeUint8(static_cast<uint8_t>(slot.item.type));
                    writer.writeFloat(slot.item.weight);
                    writer.writeUint32(slot.item.value);
                    writer.writeUint32(slot.slotIndex);
                }
                writer.writeFloat(inv->getTotalWeight());
                LOGD("Serialized %zu inventory slots", slots.size());
            }
        }

        // Serialize SpellManager
        writer.writeUint32(0x53504C4C);  // "SPLL" marker
        if (spellManager_ && playerController_) {
            auto player = playerController_->getPlayer();
            if (player) {
                // Serialize player's known spells from CharacterStatus
                // We store spell IDs that the player knows
                auto playerSpells = spellManager_->getNpcSpells(1);  // Player ID = 1
                writer.writeUint32(static_cast<uint32_t>(playerSpells.size()));
                for (const auto& spell : playerSpells) {
                    writer.writeUint32(spell->spellId);
                    writer.writeString(spell->name);
                    writer.writeUint8(static_cast<uint8_t>(spell->school));
                    writer.writeFloat(spell->manaCost);
                    writer.writeFloat(spell->baseDamage);
                    writer.writeUint32(spell->targetType);
                    // Effects
                    writer.writeUint32(static_cast<uint32_t>(spell->effects.size()));
                    for (const auto& effect : spell->effects) {
                        writer.writeUint8(static_cast<uint8_t>(effect.type));
                        writer.writeFloat(effect.magnitude);
                        writer.writeFloat(effect.duration);
                        writer.writeString(effect.affectedAttribute);
                    }
                }
                LOGD("Serialized %zu player spells", playerSpells.size());
            }
        }

        // Serialize QuestManager
        writer.writeUint32(0x51535453);  // "QSTS" marker
        if (questManager_) {
            auto activeQuests = questManager_->getActiveQuests();
            auto completedQuests = questManager_->getCompletedQuests();

            writer.writeUint32(static_cast<uint32_t>(activeQuests.size()));
            for (const auto& quest : activeQuests) {
                writer.writeUint32(quest->questId);
                writer.writeUint32(quest->giverNpcId);
                writer.writeString(quest->title);
                writer.writeString(quest->description);
                writer.writeUint8(static_cast<uint8_t>(quest->state));
                writer.writeUint32(quest->timeAccepted);

                // Objectives
                writer.writeUint32(static_cast<uint32_t>(quest->objectives.size()));
                for (const auto& obj : quest->objectives) {
                    writer.writeUint32(obj.objectiveId);
                    writer.writeString(obj.description);
                    writer.writeUint8(static_cast<uint8_t>(obj.state));
                    writer.writeUint32(obj.currentProgress);
                    writer.writeUint32(obj.targetProgress);
                }

                // Reward
                writer.writeUint32(quest->reward.goldAmount);
                writer.writeFloat(quest->reward.experiencePoints);
                save_util::writeStringVector(writer, quest->reward.itemRewards);
            }

            writer.writeUint32(static_cast<uint32_t>(completedQuests.size()));
            for (const auto& quest : completedQuests) {
                writer.writeUint32(quest->questId);
                writer.writeUint32(quest->giverNpcId);
                writer.writeString(quest->title);
                writer.writeUint8(static_cast<uint8_t>(quest->state));
                writer.writeUint32(quest->timeCompleted);
            }

            LOGD("Serialized %zu active, %zu completed quests",
                 activeQuests.size(), completedQuests.size());
        }

        // Serialize WorldManager
        writer.writeUint32(0x574C4453);  // "WLDS" marker
        if (worldManager_) {
            const auto& ws = worldManager_->getWorldState();
            writer.writeVec3(ws.playerPosition);
            writer.writeVec3(ws.playerRotation);
            writer.writeFloat(ws.timeOfDay);
            writer.writeFloat(ws.weatherIntensity);
            writer.writeString(ws.currentWeather);
            writer.writeUint32(ws.dayCount);

            // Loaded cells
            const auto& activeCells = worldManager_->getActiveCells();
            writer.writeUint32(static_cast<uint32_t>(activeCells.size()));
            for (const auto& cell : activeCells) {
                writer.writeUint32(cell->cellId);
                writer.writeInt32(cell->cellX);
                writer.writeInt32(cell->cellY);
                writer.writeString(cell->cellName);
                writer.writeUint8(static_cast<uint8_t>(cell->cellType));
            }

            LOGD("Serialized world: time=%.1f day=%u cells=%zu",
                 ws.timeOfDay, ws.dayCount, activeCells.size());
        }

        // Serialize ScriptManager
        writer.writeUint32(0x53435250);  // "SCRP" marker
        if (scriptManager_) {
            // Global variables
            // We need to serialize the global variable state
            // For now, write a placeholder count
            writer.writeUint32(0);  // Will be populated when script globals are accessible
            LOGD("Serialized script state");
        }

        // Write to file
        std::string name = slotName.empty() ? ("Slot " + std::to_string(slotIndex)) : slotName;
        bool success = writeToFile(slotIndex, name, writer.getBuffer());

        if (success) {
            LOGI("Game saved to slot %u (%zu bytes payload)", slotIndex, writer.getSize());
        }

        return success;
    } catch (const std::exception& e) {
        LOGE("Save error: %s", e.what());
        return false;
    }
}

bool SaveManager::loadGame(uint32_t slotIndex) {
    try {
        std::vector<uint8_t> payload;
        if (!readFromFile(slotIndex, payload)) {
            LOGE("Failed to read save file for slot %u", slotIndex);
            return false;
        }

        BinaryReader reader(payload.data(), payload.size());

        // Deserialize PlayerController
        uint32_t plyrMarker = reader.readUint32();
        if (plyrMarker == 0x504C5952 && playerController_) {
            auto player = playerController_->getPlayer();
            if (player) {
                player->position = reader.readVec3();
                player->rotation = reader.readVec3();
                player->velocity = reader.readVec3();
                player->health = reader.readFloat();
                player->maxHealth = reader.readFloat();
                player->stamina = reader.readFloat();
                player->maxStamina = reader.readFloat();
                player->playerLevel = reader.readUint32();
                player->experience = reader.readFloat();
                player->experiencePerLevelUp = reader.readFloat();
                player->currentCellX = reader.readInt32();
                player->currentCellY = reader.readInt32();
                player->movementState = static_cast<MovementState>(reader.readUint8());

                // Skills
                player->skills.Blade = reader.readInt32();
                player->skills.Blunt = reader.readInt32();
                player->skills.Block = reader.readInt32();
                player->skills.Restoration = reader.readInt32();
                player->skills.Destruction = reader.readInt32();
                player->skills.Alteration = reader.readInt32();
                player->skills.Conjuration = reader.readInt32();
                player->skills.Illusion = reader.readInt32();
                player->skills.Mysticism = reader.readInt32();
                player->skills.Marksman = reader.readInt32();
                player->skills.Athletics = reader.readInt32();
                player->skills.Acrobatics = reader.readInt32();

                // Attributes
                player->attributes.Strength = reader.readInt32();
                player->attributes.Intelligence = reader.readInt32();
                player->attributes.Willpower = reader.readInt32();
                player->attributes.Agility = reader.readInt32();
                player->attributes.Speed = reader.readInt32();
                player->attributes.Endurance = reader.readInt32();
                player->attributes.Personality = reader.readInt32();
                player->attributes.Luck = reader.readInt32();

                // Physics state
                player->isOnGround = reader.readBool();
                player->speed = reader.readFloat();
                player->sprintSpeed = reader.readFloat();

                // Quick-slot spell IDs (restored after spells are loaded)
                uint32_t quickSlotIds[Player::QUICK_SLOT_COUNT];
                for (int i = 0; i < Player::QUICK_SLOT_COUNT; ++i) {
                    quickSlotIds[i] = reader.readUint32();
                }

                // Player controller state
                bool combatStance = reader.readBool();
                uint8_t animState = reader.readUint8();
                (void)combatStance;  // Applied via controller methods
                (void)animState;

                // Update physics character position if available
                playerController_->setPosition(player->position);

                LOGD("Loaded player: pos=(%.1f,%.1f,%.1f) lvl=%u hp=%.0f",
                     player->position.x, player->position.y, player->position.z,
                     player->playerLevel, player->health);
            }
        }

        // Deserialize InventoryManager
        uint32_t invtMarker = reader.readUint32();
        if (invtMarker == 0x494E5654 && inventoryManager_) {
            auto inv = inventoryManager_->getPlayerInventory();
            if (inv) {
                inv->clear();
                uint32_t slotCount = reader.readUint32();
                for (uint32_t i = 0; i < slotCount; ++i) {
                    Item item;
                    item.itemId = reader.readUint32();
                    uint32_t qty = reader.readUint32();
                    item.name = reader.readString();
                    item.type = static_cast<ItemType>(reader.readUint8());
                    item.weight = reader.readFloat();
                    item.value = reader.readUint32();
                    uint32_t slotIdx = reader.readUint32();
                    (void)slotIdx;

                    if (qty > 0 && item.itemId > 0) {
                        inv->addItem(item, qty);
                    }
                }
                float totalWeight = reader.readFloat();
                (void)totalWeight;

                LOGD("Loaded %u inventory slots", slotCount);
            }
        }

        // Deserialize SpellManager
        uint32_t spllMarker = reader.readUint32();
        if (spllMarker == 0x53504C4C && spellManager_) {
            uint32_t spellCount = reader.readUint32();
            for (uint32_t i = 0; i < spellCount; ++i) {
                uint32_t spellId = reader.readUint32();
                std::string name = reader.readString();
                MagicSchool school = static_cast<MagicSchool>(reader.readUint8());
                float manaCost = reader.readFloat();
                float baseDamage = reader.readFloat();
                uint32_t targetType = reader.readUint32();

                // Create spell in manager
                uint32_t newId = spellManager_->createSpell(name, name, school, manaCost, baseDamage);
                (void)spellId;  // Use newId for consistency

                // Effects
                uint32_t effectCount = reader.readUint32();
                for (uint32_t j = 0; j < effectCount; ++j) {
                    SpellEffectType type = static_cast<SpellEffectType>(reader.readUint8());
                    float magnitude = reader.readFloat();
                    float duration = reader.readFloat();
                    std::string attr = reader.readString();
                    SpellEffect effect(type, magnitude, duration);
                    effect.affectedAttribute = attr;
                    spellManager_->addEffectToSpell(newId, effect);
                }

                // Teach to player
                spellManager_->teachSpellToNpc(1, newId);
            }
            LOGD("Loaded %u spells", spellCount);
        }

        // Deserialize QuestManager
        uint32_t qstsMarker = reader.readUint32();
        if (qstsMarker == 0x51535453 && questManager_) {
            uint32_t activeCount = reader.readUint32();
            for (uint32_t i = 0; i < activeCount; ++i) {
                uint32_t questId = reader.readUint32();
                uint32_t giverNpcId = reader.readUint32();
                std::string title = reader.readString();
                std::string description = reader.readString();
                QuestState state = static_cast<QuestState>(reader.readUint8());
                uint32_t timeAccepted = reader.readUint32();

                // Create quest
                uint32_t newQuestId = questManager_->createQuest(giverNpcId, title, description);
                (void)questId;

                // Objectives
                uint32_t objCount = reader.readUint32();
                for (uint32_t j = 0; j < objCount; ++j) {
                    uint32_t objId = reader.readUint32();
                    std::string objDesc = reader.readString();
                    QuestObjectiveState objState = static_cast<QuestObjectiveState>(reader.readUint8());
                    uint32_t progress = reader.readUint32();
                    uint32_t target = reader.readUint32();
                    (void)objId;

                    questManager_->addObjective(newQuestId, objDesc, target);
                    if (progress > 0) {
                        // Update progress after adding
                        auto quest = questManager_->getQuest(newQuestId);
                        if (quest && !quest->objectives.empty()) {
                            quest->objectives.back().currentProgress = progress;
                            quest->objectives.back().state = objState;
                        }
                    }
                }

                // Reward
                QuestReward reward;
                reward.goldAmount = reader.readUint32();
                reward.experiencePoints = reader.readFloat();
                save_util::readStringVector(reader, reward.itemRewards);
                questManager_->setQuestReward(newQuestId, reward);

                // Accept quest if it was active
                if (state == QuestState::ACCEPTED || state == QuestState::IN_PROGRESS) {
                    questManager_->acceptQuest(newQuestId);
                }

                (void)timeAccepted;
            }

            uint32_t completedCount = reader.readUint32();
            for (uint32_t i = 0; i < completedCount; ++i) {
                uint32_t questId = reader.readUint32();
                uint32_t giverNpcId = reader.readUint32();
                std::string title = reader.readString();
                QuestState state = static_cast<QuestState>(reader.readUint8());
                uint32_t timeCompleted = reader.readUint32();
                (void)questId; (void)giverNpcId; (void)title; (void)state; (void)timeCompleted;
                // Completed quests are tracked but not re-created
            }

            LOGD("Loaded %u active, %u completed quests", activeCount, completedCount);
        }

        // Deserialize WorldManager
        uint32_t wldsMarker = reader.readUint32();
        if (wldsMarker == 0x574C4453 && worldManager_) {
            WorldState ws;
            ws.playerPosition = reader.readVec3();
            ws.playerRotation = reader.readVec3();
            ws.timeOfDay = reader.readFloat();
            ws.weatherIntensity = reader.readFloat();
            ws.currentWeather = reader.readString();
            ws.dayCount = reader.readUint32();
            worldManager_->setWorldState(ws);

            // Loaded cells
            uint32_t cellCount = reader.readUint32();
            for (uint32_t i = 0; i < cellCount; ++i) {
                uint32_t cellId = reader.readUint32();
                int32_t cellX = reader.readInt32();
                int32_t cellY = reader.readInt32();
                std::string cellName = reader.readString();
                CellType cellType = static_cast<CellType>(reader.readUint8());
                (void)cellId; (void)cellName; (void)cellType;

                // Request cell load
                worldManager_->loadCell(cellX, cellY);
            }

            LOGD("Loaded world: time=%.1f day=%u cells=%u", ws.timeOfDay, ws.dayCount, cellCount);
        }

        // Deserialize ScriptManager
        uint32_t scrpMarker = reader.readUint32();
        if (scrpMarker == 0x53435250 && scriptManager_) {
            uint32_t globalCount = reader.readUint32();
            (void)globalCount;
            LOGD("Loaded script state");
        }

        LOGI("Game loaded from slot %u", slotIndex);
        return true;
    } catch (const std::exception& e) {
        LOGE("Load error: %s", e.what());
        return false;
    }
}

// ============================================================================
// File I/O with header
// ============================================================================

bool SaveManager::writeToFile(uint32_t slotIndex, const std::string& slotName,
                               const std::vector<uint8_t>& payload) {
    save_format::SaveHeader header = {};  // Zero-initialize to avoid uninitialized padding
    header.formatVersion = save_format::CURRENT_VERSION;
    header.gameVersion = 0x000700;  // v0.7.0
    header.timestamp = static_cast<uint64_t>(
        std::chrono::system_clock::now().time_since_epoch().count());
    header.payloadSize = static_cast<uint32_t>(payload.size());
    header.slotIndex = slotIndex;
    header.checksum = save_format::computeCRC32(payload.data(), payload.size());

    size_t copyLen = std::min(slotName.size(), sizeof(header.slotName) - 1);
    std::memcpy(header.slotName, slotName.c_str(), copyLen);
    header.slotName[copyLen] = '\0';  // Ensure null termination

    std::string path = slotManager_.getSlotPath(slotIndex);
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        LOGE("Failed to open save file: %s", path.c_str());
        return false;
    }

    file.write(reinterpret_cast<const char*>(&header), sizeof(header));
    file.write(reinterpret_cast<const char*>(payload.data()), payload.size());
    file.flush();  // Ensure data is written before close (crash safety)
    file.close();

    return true;
}

bool SaveManager::readFromFile(uint32_t slotIndex, std::vector<uint8_t>& payload) {
    std::string path = slotManager_.getSlotPath(slotIndex);
    if (!fs::exists(path)) {
        LOGE("Save file not found: %s", path.c_str());
        return false;
    }

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        LOGE("Failed to open save file: %s", path.c_str());
        return false;
    }

    save_format::SaveHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));

    auto validation = save_format::validateHeader(header);
    if (validation != save_format::ValidationResult::OK) {
        LOGE("Save file validation failed: %d", static_cast<int>(validation));
        return false;
    }

    payload.resize(header.payloadSize);
    file.read(reinterpret_cast<char*>(payload.data()), header.payloadSize);

    // Verify checksum
    uint32_t computedChecksum = save_format::computeCRC32(payload.data(), payload.size());
    if (computedChecksum != header.checksum) {
        LOGE("Checksum mismatch: computed=0x%08X expected=0x%08X",
             computedChecksum, header.checksum);
        return false;
    }

    file.close();
    return true;
}

// ============================================================================
// Quick Save/Load
// ============================================================================

bool SaveManager::quickSave() {
    return saveGame(SaveSlotManager::QUICK_SAVE_SLOT, "Quick Save");
}

bool SaveManager::quickLoad() {
    if (!hasSave(SaveSlotManager::QUICK_SAVE_SLOT)) {
        LOGW("No quick save found");
        return false;
    }
    return loadGame(SaveSlotManager::QUICK_SAVE_SLOT);
}

// ============================================================================
// Auto-save update
// ============================================================================

void SaveManager::update(float deltaTime) {
    autoSave_.update(deltaTime);
}

// ============================================================================
// Slot management
// ============================================================================

bool SaveManager::deleteSave(uint32_t slotIndex) {
    return slotManager_.deleteSlot(slotIndex);
}

std::vector<SaveSlotInfo> SaveManager::getSaveSlots() const {
    return slotManager_.getAllSlots();
}

bool SaveManager::hasSave(uint32_t slotIndex) const {
    return slotManager_.isSlotUsed(slotIndex);
}

// ============================================================================
// Game state capture/restore (for legacy compatibility)
// ============================================================================

bool SaveManager::captureGameState(GameState& state) {
    state.saveTimestamp = static_cast<uint64_t>(
        std::chrono::system_clock::now().time_since_epoch().count());
    state.version = "0.7.0";

    if (playerController_) {
        auto player = playerController_->getPlayer();
        if (player) {
            state.playerPosition = player->position;
            state.playerRotation = player->rotation;
            state.playerLevel = player->playerLevel;
            state.playerExperience = player->experience;
            state.playerStatus.currentHealth = player->health;
            state.playerStatus.maxHealth = player->maxHealth;
            state.playerStatus.currentMana = player->stamina;  // Using stamina as proxy
            state.playerStatus.maxMana = player->maxStamina;
            state.playerStatus.level = player->playerLevel;
        }
    }

    if (worldManager_) {
        const auto& ws = worldManager_->getWorldState();
        state.timeOfDay = ws.timeOfDay;
        state.dayCount = ws.dayCount;
    }

    return true;
}

bool SaveManager::restoreGameState(const GameState& state) {
    if (playerController_) {
        auto player = playerController_->getPlayer();
        if (player) {
            player->position = state.playerPosition;
            player->rotation = state.playerRotation;
            player->playerLevel = state.playerLevel;
            player->experience = state.playerExperience;
            player->health = state.playerStatus.currentHealth;
            player->maxHealth = state.playerStatus.maxHealth;
            playerController_->setPosition(state.playerPosition);
        }
    }

    if (worldManager_) {
        WorldState ws;
        ws.playerPosition = state.playerPosition;
        ws.playerRotation = state.playerRotation;
        ws.timeOfDay = state.timeOfDay;
        ws.dayCount = state.dayCount;
        worldManager_->setWorldState(ws);
    }

    return true;
}

// ============================================================================
// Legacy JSON save/load (backward compatibility)
// ============================================================================

std::string SaveManager::getSavePathLegacy(const std::string& slotName) const {
    return getBaseDir() + slotName + ".json";
}

bool SaveManager::saveGameLegacy(const std::string& slotName, const GameState& state) {
    try {
        std::string json = serializeGameState(state);
        std::string filePath = getSavePathLegacy(slotName);

        std::ofstream file(filePath, std::ios::out);
        if (!file.is_open()) {
            LOGE("Failed to open save file: %s", filePath.c_str());
            return false;
        }

        file << json;
        file.close();

        LOGI("Game saved (legacy): %s", slotName.c_str());
        return true;
    } catch (const std::exception& e) {
        LOGE("Save error: %s", e.what());
        return false;
    }
}

bool SaveManager::loadGameLegacy(const std::string& slotName, GameState& outState) {
    try {
        std::string filePath = getSavePathLegacy(slotName);

        if (!fs::exists(filePath)) {
            LOGE("Save file not found: %s", filePath.c_str());
            return false;
        }

        std::ifstream file(filePath, std::ios::in);
        if (!file.is_open()) {
            LOGE("Failed to open save file: %s", filePath.c_str());
            return false;
        }

        std::stringstream buffer;
        buffer << file.rdbuf();
        file.close();

        std::string json = buffer.str();
        bool success = deserializeGameState(json, outState);

        if (success) {
            LOGI("Game loaded (legacy): %s", slotName.c_str());
        } else {
            LOGE("Failed to deserialize game state");
        }

        return success;
    } catch (const std::exception& e) {
        LOGE("Load error: %s", e.what());
        return false;
    }
}

std::string SaveManager::serializeGameState(const GameState& state) const {
    // JSON string escape helper
    auto escapeJson = [](const std::string& s) -> std::string {
        std::string out;
        out.reserve(s.size());
        for (char c : s) {
            switch (c) {
                case '"':  out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\b': out += "\\b"; break;
                case '\f': out += "\\f"; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default:   out += c; break;
            }
        }
        return out;
    };

    std::stringstream ss;
    ss << "{\n";
    ss << "  \"version\": \"" << escapeJson(state.version) << "\",\n";
    ss << "  \"saveName\": \"" << escapeJson(state.saveName) << "\",\n";
    ss << "  \"timestamp\": " << state.saveTimestamp << ",\n";
    ss << "  \"playerPos\": [" << state.playerPosition.x << ", "
       << state.playerPosition.y << ", " << state.playerPosition.z << "],\n";
    ss << "  \"playerRot\": [" << state.playerRotation.x << ", "
       << state.playerRotation.y << ", " << state.playerRotation.z << "],\n";
    ss << "  \"playerHealth\": " << state.playerStatus.currentHealth << ",\n";
    ss << "  \"playerMana\": " << state.playerStatus.currentMana << ",\n";
    ss << "  \"playerLevel\": " << state.playerLevel << ",\n";
    ss << "  \"timeOfDay\": " << state.timeOfDay << ",\n";
    ss << "  \"dayCount\": " << state.dayCount << ",\n";

    // NPC states
    ss << "  \"npcStates\": [\n";
    {
        bool first = true;
        for (const auto& pair : state.npcStates) {
            if (!first) ss << ",\n";
            first = false;
            const auto& pos = pair.second.first;
            const auto& status = pair.second.second;
            ss << "    {\"npcId\": " << pair.first
               << ", \"pos\": [" << pos.x << ", " << pos.y << ", " << pos.z << "]"
               << ", \"health\": " << status.currentHealth
               << ", \"mana\": " << status.currentMana
               << ", \"stamina\": " << status.stamina
               << ", \"level\": " << status.level << "}";
        }
    }
    ss << "\n  ],\n";

    // Quest states
    ss << "  \"questStates\": [\n";
    {
        bool first = true;
        for (const auto& pair : state.questStates) {
            if (!first) ss << ",\n";
            first = false;
            ss << "    {\"questId\": " << pair.first
               << ", \"state\": " << pair.second << "}";
        }
    }
    ss << "\n  ]\n";
    ss << "}\n";
    return ss.str();
}

bool SaveManager::deserializeGameState(const std::string& json, GameState& outState) {
    try {
        outState.saveName = "Loaded Save";
        outState.saveTimestamp = std::time(nullptr);

        // Extract position from JSON
        size_t pos = json.find("playerPos");
        if (pos != std::string::npos) {
            size_t arr_start = json.find("[", pos);
            size_t arr_end = json.find("]", arr_start);
            if (arr_start != std::string::npos && arr_end != std::string::npos) {
                std::string arr_str = json.substr(arr_start + 1, arr_end - arr_start - 1);
                std::stringstream ss(arr_str);
                std::string token;
                int coord_idx = 0;
                while (std::getline(ss, token, ',') && coord_idx < 3) {
                    size_t start = token.find_first_not_of(" \t\n\r");
                    size_t end = token.find_last_not_of(" \t\n\r");
                    if (start != std::string::npos) {
                        token = token.substr(start, end - start + 1);
                    }
                    float val = std::stof(token);
                    switch (coord_idx) {
                        case 0: outState.playerPosition.x = val; break;
                        case 1: outState.playerPosition.y = val; break;
                        case 2: outState.playerPosition.z = val; break;
                    }
                    coord_idx++;
                }
            }
        }

        // Extract health
        pos = json.find("playerHealth");
        if (pos != std::string::npos) {
            size_t val_start = json.find(":", pos) + 1;
            size_t val_end = json.find(",", val_start);
            outState.playerStatus.currentHealth = std::stof(json.substr(val_start, val_end - val_start));
        }

        // Extract mana
        pos = json.find("playerMana");
        if (pos != std::string::npos) {
            size_t val_start = json.find(":", pos) + 1;
            size_t val_end = json.find(",", val_start);
            if (val_end == std::string::npos) val_end = json.find("}", val_start);
            outState.playerStatus.currentMana = std::stof(json.substr(val_start, val_end - val_start));
        }

        // Extract NPC states
        pos = json.find("\"npcStates\"");
        if (pos != std::string::npos) {
            size_t arr_start = json.find("[", pos);
            size_t arr_end = json.find("]", arr_start);
            if (arr_start != std::string::npos && arr_end != std::string::npos) {
                std::string arr_str = json.substr(arr_start + 1, arr_end - arr_start - 1);
                size_t search_pos = 0;
                while (search_pos < arr_str.size()) {
                    size_t npc_start = arr_str.find("{", search_pos);
                    if (npc_start == std::string::npos) break;
                    size_t npc_end = arr_str.find("}", npc_start);
                    if (npc_end == std::string::npos) break;
                    std::string npc_str = arr_str.substr(npc_start, npc_end - npc_start + 1);

                    size_t id_pos = npc_str.find("\"npcId\"");
                    if (id_pos != std::string::npos) {
                        size_t id_val_start = npc_str.find(":", id_pos) + 1;
                        size_t id_val_end = npc_str.find(",", id_val_start);
                        uint32_t npcId = static_cast<uint32_t>(std::stoul(
                            npc_str.substr(id_val_start, id_val_end - id_val_start)));

                        glm::vec3 pos_vec(0.0f, 0.0f, 0.0f);
                        size_t pos_arr = npc_str.find("\"pos\"");
                        if (pos_arr != std::string::npos) {
                            size_t p_start = npc_str.find("[", pos_arr);
                            size_t p_end = npc_str.find("]", p_start);
                            if (p_start != std::string::npos && p_end != std::string::npos) {
                                std::string p_str = npc_str.substr(p_start + 1, p_end - p_start - 1);
                                std::stringstream pss(p_str);
                                std::string token;
                                int idx = 0;
                                while (std::getline(pss, token, ',') && idx < 3) {
                                    size_t s = token.find_first_not_of(" \t\n\r");
                                    size_t e = token.find_last_not_of(" \t\n\r");
                                    if (s != std::string::npos) token = token.substr(s, e - s + 1);
                                    float v = std::stof(token);
                                    if (idx == 0) pos_vec.x = v;
                                    else if (idx == 1) pos_vec.y = v;
                                    else if (idx == 2) pos_vec.z = v;
                                    idx++;
                                }
                            }
                        }

                        CharacterStatus status;
                        auto parseFloat = [&](const std::string& key, float& out) {
                            size_t kp = npc_str.find(key);
                            if (kp != std::string::npos) {
                                size_t vs = npc_str.find(":", kp) + 1;
                                size_t ve = npc_str.find(",", vs);
                                if (ve == std::string::npos) ve = npc_str.find("}", vs);
                                out = std::stof(npc_str.substr(vs, ve - vs));
                            }
                        };
                        auto parseUint = [&](const std::string& key, uint32_t& out) {
                            size_t kp = npc_str.find(key);
                            if (kp != std::string::npos) {
                                size_t vs = npc_str.find(":", kp) + 1;
                                size_t ve = npc_str.find(",", vs);
                                if (ve == std::string::npos) ve = npc_str.find("}", vs);
                                out = static_cast<uint32_t>(std::stoul(npc_str.substr(vs, ve - vs)));
                            }
                        };
                        parseFloat("\"health\"", status.currentHealth);
                        parseFloat("\"mana\"", status.currentMana);
                        parseFloat("\"stamina\"", status.stamina);
                        parseUint("\"level\"", status.level);

                        outState.npcStates[npcId] = std::make_pair(pos_vec, status);
                    }
                    search_pos = npc_end + 1;
                }
            }
        }

        // Extract quest states
        pos = json.find("\"questStates\"");
        if (pos != std::string::npos) {
            size_t arr_start = json.find("[", pos);
            size_t arr_end = json.find("]", arr_start);
            if (arr_start != std::string::npos && arr_end != std::string::npos) {
                std::string arr_str = json.substr(arr_start + 1, arr_end - arr_start - 1);
                size_t search_pos = 0;
                while (search_pos < arr_str.size()) {
                    size_t q_start = arr_str.find("{", search_pos);
                    if (q_start == std::string::npos) break;
                    size_t q_end = arr_str.find("}", q_start);
                    if (q_end == std::string::npos) break;
                    std::string q_str = arr_str.substr(q_start, q_end - q_start + 1);

                    size_t id_pos = q_str.find("\"questId\"");
                    size_t st_pos = q_str.find("\"state\"");
                    if (id_pos != std::string::npos && st_pos != std::string::npos) {
                        size_t id_vs = q_str.find(":", id_pos) + 1;
                        size_t id_ve = q_str.find(",", id_vs);
                        uint32_t questId = static_cast<uint32_t>(std::stoul(q_str.substr(id_vs, id_ve - id_vs)));

                        size_t st_vs = q_str.find(":", st_pos) + 1;
                        size_t st_ve = q_str.find(",", st_vs);
                        if (st_ve == std::string::npos) st_ve = q_str.find("}", st_vs);
                        int state = std::stoi(q_str.substr(st_vs, st_ve - st_vs));

                        outState.questStates[questId] = state;
                    }
                    search_pos = q_end + 1;
                }
            }
        }

        return true;
    } catch (const std::exception& e) {
        LOGE("Deserialization failed: %s", e.what());
        return false;
    }
}
