#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <glm/glm.hpp>

namespace oblivion {

// ============================================================================
// TES4 ESM/ESP Record Format
// ============================================================================

// Record flags (bits in header.flags)
static constexpr uint32_t REC_FLAG_COMPRESSED = 0x00040000;
static constexpr uint32_t REC_FLAG_DELETED   = 0x00000020;

// Group types (used in GRUP records)
enum class GroupType : uint32_t {
    Top      = 0,  // Top-level records
    World    = 1,  // World children
    Int      = 2,  // Internal cell children
    Ext      = 3,  // External cell children
    Cell     = 4,  // Cell persistent/temporary
    Topic   = 5,   // Dialogue topics
    CellChild = 6, // Cell child records
    TopicChild = 7,// Topic child records
    Dial = 8,
    DialChild = 9,
    Quest = 10,
    QuestChild = 11,
    NavMesh = 12,
    Misc = 13,
};

// Field types in subrecords
enum class FieldType : uint8_t {
    Unknown = 0,
    String = 1,    // ZString (null-terminated)
    Integer = 2,   // 32-bit int
    Float = 3,     // 32-bit float
    Binary = 4,    // Raw bytes
    FormID = 5,    // Object reference
};

// A single subrecord within a record
struct SubRecord {
    char tag[4];          // 4-char tag (e.g., "FULL", "NAME", "DATA")
    std::vector<uint8_t> data;
    size_t size() const { return data.size(); }
};

// A record (CELL, NPC_, WEAP, etc.)
struct ESMRecord {
    char recType[4] = {0,0,0,0};
    uint32_t dataSize = 0;
    uint32_t flags = 0;
    uint32_t formID = 0;
    std::vector<SubRecord> subRecords;

    // Helpers
    const SubRecord* findSubRecord(const char* tag) const;
    std::string getString(const char* tag) const;
    uint32_t getUint(const char* tag) const;
    int32_t getInt(const char* tag) const;
    float getFloat(const char* tag) const;
    uint32_t getFormID(const char* tag) const;
};

// GRUP header (24 bytes)
struct GroupHeader {
    char recType[4];        // "GRUP"
    uint32_t groupSize;     // Total group size (including this header)
    int32_t groupLabel;     // World/cell ID or just label
    uint32_t groupType;     // GroupType enum
    int16_t stamp;          // Block index (used for compressed data)
};

// High-level record data (decoded from a parsed record)
struct CellData {
    uint32_t formID = 0;
    std::string editorID;
    std::string fullName;
    uint32_t worldspaceID = 0;  // 0 = interior
    int32_t gridX = 0;
    int32_t gridY = 0;
    // Lighting / climate data omitted for simplicity
};

struct NPCData {
    uint32_t formID = 0;
    std::string editorID;
    std::string fullName;
    std::string race;
    uint32_t raceID = 0;
    std::string className;
    uint8_t level = 1;
    uint32_t factionID = 0;
    uint32_t health = 50;
    uint32_t stamina = 50;
    uint32_t magicka = 50;
};

struct WeaponData {
    uint32_t formID = 0;
    std::string editorID;
    std::string fullName;
    uint32_t damage = 10;
    uint32_t value = 50;
    uint32_t weight = 5;
    uint32_t health = 100;
    uint32_t enchantID = 0;
};

struct QuestData {
    uint32_t formID = 0;
    std::string editorID;
    std::string fullName;
    uint8_t flags = 0;
    uint8_t priority = 0;
};

struct DialogData {
    uint32_t formID = 0;
    std::string editorID;
    std::string fullName;
    std::string questName;
    uint8_t flags = 0;
};

/// Worldspace definition
struct WorldData {
    uint32_t formID = 0;
    std::string editorID;
    std::string fullName;
    glm::vec2 worldOffset{0.0f, 0.0f};   // Cell grid offset (from DATA)
    int32_t minX = 0, minY = 0;           // Bounds
    int32_t maxX = 0, maxY = 0;
};

/// Spell definition (SPEL record)
struct SpellData {
    uint32_t formID = 0;
    std::string editorID;
    std::string fullName;
    uint8_t spellType = 0;       // 0=spell, 1=disease, 2=power, 3=lesser, 4=ability, 5=poison
    uint32_t cost = 0;
    uint8_t level = 0;           // 0=novice .. 4=master
    uint32_t flags = 0;
    // Effects (EFID + EFIT pairs)
    std::vector<uint32_t> effectFormIDs;    // Reference to MGEF records
    std::vector<float> effectMagnitudes;
    std::vector<uint32_t> effectAreas;
    std::vector<uint32_t> effectDurations;
    uint32_t effectType = 0;  // from SPIT: fire/frost/shock etc.
};

/// Placed object/NPC reference in the world
struct ReferenceData {
        uint32_t formID = 0;           // This reference's formID
        uint32_t baseFormID = 0;       // FormID of the base object (NPC_, WEAP, etc.)
        uint32_t cellFormID = 0;       // Parent cell formID
        glm::vec3 position{0.0f, 0.0f, 0.0f};
        glm::vec3 rotation{0.0f, 0.0f, 0.0f};
        float scale = 1.0f;
        uint16_t flags = 0;
};

/// Terrain data from LAND record (65x65 heightmap)
struct TerrainData {
        uint32_t formID = 0;           // Cell formID this terrain belongs to
        std::vector<float> heights;    // 65x65 = 4225 float heights
    
        bool hasHeights() const { return !heights.empty(); }
};

// ============================================================================
// ESMFile - parses a single .esm/.esp file
// ============================================================================

class ESMFile {
public:
        ESMFile() = default;
        ~ESMFile() = default;

        // Open and parse an ESM/ESP file
        bool open(const std::string& filePath);
        bool parseFromMemory(const std::string& name, const uint8_t* data, size_t dataSize);

        // Get parsed data
        const std::vector<CellData>& getCells() const { return m_cells; }
        const std::vector<NPCData>& getNPCs() const { return m_npcs; }
        const std::vector<WeaponData>& getWeapons() const { return m_weapons; }
        const std::vector<QuestData>& getQuests() const { return m_quests; }
        const std::vector<DialogData>& getDialogs() const { return m_dialogs; }
        const std::vector<ReferenceData>& getReferences() const { return m_references; }
        const std::vector<TerrainData>& getTerrains() const { return m_terrains; }
        const std::vector<WorldData>& getWorlds() const { return m_worlds; }
        const std::vector<SpellData>& getSpells() const { return m_spells; }

        // Get file metadata
        const std::string& getFileName() const { return m_fileName; }
        bool isMaster() const { return m_isMaster; }

private:
    std::string m_fileName;
    bool m_isMaster = false;

    std::vector<CellData> m_cells;
    std::vector<NPCData> m_npcs;
    std::vector<WeaponData> m_weapons;
    std::vector<QuestData> m_quests;
    std::vector<DialogData> m_dialogs;
    std::vector<ReferenceData> m_references;
    std::vector<TerrainData> m_terrains;
    std::vector<WorldData> m_worlds;
    std::vector<SpellData> m_spells;

    // Parsing helpers
        bool readRecordHeader(std::ifstream& file, ESMRecord& rec);
        bool readSubRecord(std::ifstream& file, SubRecord& sub);
        bool readGroup(std::ifstream& file, GroupType groupType, uint32_t groupSize);
    
        // Memory-based parsing helpers
        bool readRecordHeaderMem(const uint8_t*& pos, const uint8_t* end, ESMRecord& rec);
        bool readSubRecordMem(const uint8_t*& pos, const uint8_t* end, SubRecord& sub);
        bool readGroupMem(const uint8_t*& pos, const uint8_t* end, GroupType groupType, uint32_t groupSize);

        // Record decoders
        void decodeRecord(const ESMRecord& rec);
        void decodeCell(const ESMRecord& rec);
        void decodeNPC(const ESMRecord& rec);
        void decodeWeapon(const ESMRecord& rec);
        void decodeQuest(const ESMRecord& rec);
        void decodeDialog(const ESMRecord& rec);
        void decodeReference(const ESMRecord& rec);
        void decodeTerrain(const ESMRecord& rec);
        void decodeWorld(const ESMRecord& rec);
                void decodeSpell(const ESMRecord& rec);
        };

// ============================================================================
// ESMManager - aggregates multiple ESM files
// ============================================================================

class ESMManager {
public:
    ESMManager() = default;
    ~ESMManager() { cleanup(); }

    // Load order: masters first, then the addon file
    bool loadPlugin(const std::string& esmPath);
        bool loadPluginFromMemory(const std::string& name, const uint8_t* data, size_t dataSize);
        void cleanup();

        // Query helpers
        size_t getRecordCount() const {
            size_t count = 0;
            for (const auto& file : m_files)
                count += file->getNPCs().size() + file->getCells().size() +
                         file->getWeapons().size() + file->getQuests().size() +
                         file->getDialogs().size() + file->getReferences().size() +
                             file->getTerrains().size() + file->getWorlds().size() +
                             file->getSpells().size();
                return count;
            }
        size_t findRecordsByType(const std::string& type) const;

    // Look up data by FormID
    const NPCData* findNPC(uint32_t formID) const;
    const CellData* findCell(uint32_t formID) const;
    const WeaponData* findWeapon(uint32_t formID) const;
    const QuestData* findQuest(uint32_t formID) const;
    const DialogData* findDialog(uint32_t formID) const;

    // Iteration
    const std::vector<NPCData>& getAllNPCs() const;
    const std::vector<CellData>& getAllCells() const;
    const std::vector<WeaponData>& getAllWeapons() const;
    const std::vector<QuestData>& getAllQuests() const;
    const std::vector<DialogData>& getAllDialogs() const;
    const std::vector<ReferenceData>& getAllReferences() const;
    const std::vector<TerrainData>& getAllTerrains() const;
    const std::vector<WorldData>& getAllWorlds() const;

    size_t getPluginCount() const { return m_files.size(); }

private:
    std::vector<std::unique_ptr<ESMFile>> m_files;
    std::unordered_map<uint32_t, size_t> m_npcIndex;  // formID -> file index
    std::unordered_map<uint32_t, size_t> m_cellIndex;
    std::unordered_map<uint32_t, size_t> m_weaponIndex;
    std::unordered_map<uint32_t, size_t> m_questIndex;
    std::unordered_map<uint32_t, size_t> m_dialogIndex;

    void rebuildIndices();
};

} // namespace oblivion
