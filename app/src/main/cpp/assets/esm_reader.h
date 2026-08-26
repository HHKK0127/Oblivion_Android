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

/// Creature (CREA) record
struct CreatureData {
    uint32_t formID = 0;
    std::string editorID;
    std::string fullName;
    std::string modelPath;
    uint8_t level = 1;
    uint32_t health = 50;
    uint32_t combat = 0;      // Combat skill
    uint32_t magic = 0;       // Magic skill
    uint32_t stealth = 0;     // Stealth skill
    uint32_t attackDamage = 5;
    uint32_t soulLevel = 0;   // Soul gem level
    uint32_t factionID = 0;
    uint32_t templateFormID = 0;  // Template creature
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

/// Dialogue response (INFO record) — a single line in a dialogue topic
struct InfoData {
    uint32_t formID = 0;
    std::string editorID;
    uint32_t dialFormID = 0;         // Parent DIAL formID
    std::string responseText;        // NAM1 — NPC response text
    std::string promptText;          // NAM2 — player prompt (optional)
    uint8_t responseType = 0;        // TES4: 0=neutral, 1=positive, 2=negative, 3=command
    uint32_t flags = 0;              // INFO flags
    uint32_t speakerFormID = 0;      // TRDT speaker reference
    uint32_t factionFormID = 0;      // Faction condition (ANAM)
    int32_t factionRank = -1;        // Required faction rank (CNAM)
    uint32_t questFormID = 0;        // Linked quest (QSTI)
    int32_t questStage = -1;         // Required quest stage (QSTN)
    std::string conditionFunction;   // Condition function name (CTDA)
};

struct DialogData {
    uint32_t formID = 0;
    std::string editorID;
    std::string fullName;
    std::string questName;
    uint8_t flags = 0;
    uint8_t dialogType = 0;          // 0=Topic, 1=Conversation, 2=Combat, 3=Persuasion, 4=Detection, 5=Service, 6=Misc
    std::vector<InfoData> infos;     // Child INFO records
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

/// Enchantment (ENCH) record
struct EnchantmentData {
    uint32_t formID = 0;
    std::string editorID;
    std::string fullName;
    uint32_t enchantType = 0;    // 0=scroll, 1=staff, 2=weapon, 3=apparel
    uint32_t chargeAmount = 0;
    uint32_t enchantCost = 0;
    uint32_t flags = 0;
    // Effects (EFID + EFIT pairs)
    std::vector<uint32_t> effectFormIDs;
    std::vector<float> effectMagnitudes;
    std::vector<uint32_t> effectAreas;
    std::vector<uint32_t> effectDurations;
};

/// Magic Effect (MGEF) record
struct MagicEffectData {
    uint32_t formID = 0;
    std::string editorID;
    std::string fullName;
    std::string description;
    uint32_t school = 0;         // 0=alteration, 1=conjuration, 2=destruction, 3=illusion, 4=mysticism, 5=restoration
    uint32_t baseCost = 0;
    uint32_t flags = 0;
    float baseMagnitude = 0.0f;
    float baseDuration = 0.0f;
    float range = 0.0f;
    uint32_t effectType = 0;     // 0=other, 1=fire, 2=frost, 3=shock, 4=drain, 5=absorb, 6=disintegrate
    uint32_t actorValue = 0;     // Which attribute/skill is affected
};

/// Skill (SKIL) record
struct SkillData {
    uint32_t formID = 0;
    std::string editorID;
    std::string fullName;
    std::string description;
    uint32_t skillID = 0;        // Skill enum value (0-21)
    uint32_t specialization = 0; // 0=Combat, 1=Magic, 2=Stealth
    float useMult = 0.0f;        // Experience multiplier
    float offsetMult = 0.0f;     // Offset multiplier
    uint32_t improveMult = 0;    // Improvement multiplier
    std::vector<uint32_t> governingAttribute; // Which attribute governs this skill
};

/// Birthsign (BSGN) record
struct BirthsignData {
    uint32_t formID = 0;
    std::string editorID;
    std::string fullName;
    std::string description;
    std::string texturePath;     // Star texture path
    std::vector<uint32_t> spellFormIDs; // Powers granted by this birthsign
};

/// Container (CONT) record
struct ContainerData {
    uint32_t formID = 0;
    std::string editorID;
    std::string fullName;
    std::string modelPath;
    float weight = 0.0f;         // Container weight capacity
    uint32_t flags = 0;          // 0x0001 = respawns, 0x0002 = show name
    // Items in container (CNTO subrecords)
    struct ContainerItem {
        uint32_t itemFormID = 0;
        uint32_t count = 1;
    };
    std::vector<ContainerItem> items;
};

/// Light (LIGH) data
struct LightData {
    uint32_t formID = 0;
    std::string editorID;
    std::string fullName;
    std::string modelPath;
    std::string iconPath;
    float duration = 0;          // Time in seconds (0 = infinite)
    float radius = 0;
    uint32_t color = 0;          // RGBA color
    uint32_t flags = 0;          // 0x0001 = dynamic, 0x0002 = can carry, 0x0004 = negative, 0x0008 = flicker, etc.
    float falloff = 0;
    float fov = 0;
    float weight = 0;
    int32_t value = 0;
};

/// Sound (SOUN) data
struct SoundData {
    uint32_t formID = 0;
    std::string editorID;
    std::string soundPath;       // FNAM - sound file path
    uint8_t minDistance = 0;
    uint8_t maxDistance = 0;
    uint8_t freqAdjust = 0;
    uint8_t flags = 0;
};

/// Tree (TREE) data
struct TreeData {
    uint32_t formID = 0;
    std::string editorID;
    std::string modelPath;
    std::string iconPath;
    uint32_t ingredientFormID = 0;  // INGR - harvested ingredient
    float harvestChance = 0;
};

/// Flora (FLOR) data
struct FloraData {
    uint32_t formID = 0;
    std::string editorID;
    std::string fullName;
    std::string modelPath;
    uint32_t ingredientFormID = 0;  // INGR - harvested ingredient
    float harvestChance = 0;
};

/// Activator (ACTI) data
struct ActivatorData {
    uint32_t formID = 0;
    std::string editorID;
    std::string fullName;
    std::string modelPath;
    uint32_t scriptFormID = 0;
};

/// Apparatus (APPA) data
struct ApparatusData {
    uint32_t formID = 0;
    std::string editorID;
    std::string fullName;
    std::string modelPath;
    std::string iconPath;
    uint32_t quality = 0;       // 0-3 (rough, average, fine, excellent)
    float weight = 0;
    int32_t value = 0;
};

/// Eyes (EYES) data
struct EyesData {
    uint32_t formID = 0;
    std::string editorID;
    std::string fullName;
    std::string iconPath;
    uint32_t flags = 0;         // 0x01 = playable
};

/// Hair (HAIR) data
struct HairData {
    uint32_t formID = 0;
    std::string editorID;
    std::string fullName;
    std::string modelPath;
    std::string iconPath;
    uint32_t flags = 0;         // 0x01 = playable, 0x02 = not male, 0x04 = not female
};

/// Climate (CLMT) data
struct ClimateData {
    uint32_t formID = 0;
    std::string editorID;
    // Weather types (4 sunrises, 4 day, 4 sunset, 4 night)
    uint32_t weatherTypes[16] = {};
    uint8_t sunriseBegin = 0;   // Hour (0-23)
    uint8_t sunriseEnd = 0;
    uint8_t sunsetBegin = 0;
    uint8_t sunsetEnd = 0;
    uint8_t volatility = 0;    // 0-100
    uint8_t moons = 0;         // Bit flags: 0x01 = masser, 0x02 = secunda
};

/// Region (REGN) data
struct RegionData {
    uint32_t formID = 0;
    std::string editorID;
    std::string fullName;
    uint32_t mapColor = 0;
    uint32_t worldSpaceFormID = 0;
    // Weather types for this region
    struct RegionWeather {
        uint32_t weatherFormID = 0;
        uint32_t chance = 0;
    };
    std::vector<RegionWeather> weathers;
};

/// Static (STAT) data
struct StaticData {
    uint32_t formID = 0;
    std::string editorID;
    std::string modelPath;
};

/// Leveled Item/Creature entry
struct LeveledListEntry {
    uint32_t referencedFormID = 0;   // Reference to NPC_, WEAP, ARMO, etc.
    uint16_t level = 0;              // Minimum level for this entry
    uint16_t count = 1;              // Count/number
};

/// Leveled List base (LVLI = items, LVLC = creatures, LVSP = spells)
struct LeveledListData {
    uint32_t formID = 0;
    std::string editorID;
    uint8_t chanceNone = 0;          // Chance that nothing is selected (0-100)
    uint8_t flags = 0;               // Bit 0: calc from all levels <= PC, Bit 1: calc for each item in count
    std::vector<LeveledListEntry> entries;
};

/// NavMesh triangle (3 vertex indices + adjacency flags)
struct NavMeshTriangle {
    uint16_t vertex[3] = {0, 0, 0};  // Indices into vertex array
    int16_t adjacentEdge[3] = {-1, -1, -1};  // Adjacent triangle index per edge (-1=none)
};

/// NavMesh (NAVM) record — AI pathfinding data
struct NavMeshData {
    uint32_t formID = 0;
    std::string editorID;
    uint32_t cellFormID = 0;          // Owning cell FormID (from NVER reference)
    glm::vec3 location{0.0f, 0.0f, 0.0f};  // Origin offset
    int32_t numVertices = 0;
    int32_t numTriangles = 0;
    std::vector<glm::vec3> vertices;
    std::vector<NavMeshTriangle> triangles;
};

/// Armor (ARMO) record — equipment definition
struct ArmorData {
    uint32_t formID = 0;
    std::string editorID;
    std::string fullName;
    uint32_t value = 0;
    float weight = 0.0f;
    uint32_t armorRating = 0;
    int32_t health = 0;                // Condition/durability
    uint32_t enchantmentID = 0;        // ENAM FormID
    uint32_t bipedModelID = 0;
    uint32_t flags = 0;
    uint8_t armorType = 0;             // 0=light, 1=heavy, 2=cloth
    std::string modelPath;            // MODL
};

/// Book (BOOK) record — skill books and regular books
struct BookData {
    uint32_t formID = 0;
    std::string editorID;
    std::string fullName;
    std::string description;           // DESC subrecord
    uint32_t teachesSkillID = 0;       // Skill FormID (from DATA: flags bit 0)
    uint8_t teachesSkillLevel = 0;     // Skill increase level
    uint32_t value = 0;
    float weight = 0.0f;
    std::string modelPath;
};

/// Faction (FACT) record
struct FactionRank {
    std::string rankName;
    uint32_t rankData = 0;  // rank int, perks, etc.
};

struct FactionRelation {
    uint32_t factionFormID = 0;
    int32_t modifier = 0;
    uint32_t groupFlags = 0;
};

struct FactionData {
    uint32_t formID = 0;
    std::string editorID;
    std::string fullName;              // FULL name
    uint32_t flags = 0;
    int32_t crimeGoldMultiplier = 0;   // CRIM subrecord
    std::vector<FactionRank> ranks;    // RNAM + MNAM pairs
    std::vector<FactionRelation> relations;  // XNAM subrecords
};

/// Race (RACE) record
struct RaceData {
    uint32_t formID = 0;
    std::string editorID;
    std::string fullName;              // FULL name
    std::string description;           // DESC subrecord
    // Skill bonuses (8 skills: 2 major bonus skills)
    std::vector<std::pair<uint32_t, int8_t>> skillBonuses;  // skillFormID, bonus
    // Attribute bonuses (7 attributes)
    uint8_t attrStrength = 0;
    uint8_t attrIntelligence = 0;
    uint8_t attrWillpower = 0;
    uint8_t attrAgility = 0;
    uint8_t attrSpeed = 0;
    uint8_t attrEndurance = 0;
    uint8_t attrPersonality = 0;
    uint32_t maleVoiceFormID = 0;
    uint32_t femaleVoiceFormID = 0;
    // Body data
    std::string maleModelPath;
    std::string femaleModelPath;
    std::vector<uint32_t> spellFormIDs;  // Racial spells (SPLO references)
    uint32_t startingHealth = 0;
};

/// Class (CLAS) record
struct ClassData {
    uint32_t formID = 0;
    std::string editorID;
    std::string fullName;              // FULL name
    std::string description;           // DESC subrecord
    uint8_t specialization = 0;        // 0=Combat, 1=Mysticism, 2=Stealth
    uint32_t primaryAttribute1 = 0;     // PRAT subrecord (2 uint32_t)
    uint32_t primaryAttribute2 = 0;
    uint8_t majorSkills[7] = {0};       // 7 skill FormIDs (lower bits)
    uint8_t flags = 0;
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

/// Clothing (CLOT) record
struct ClothingData {
        uint32_t formID = 0;
        std::string editorID;
        std::string fullName;
        uint32_t value = 0;
        float weight = 0.0f;
        uint32_t enchantmentID = 0;
        std::string modelPath;
};

/// Ingredient (INGR) record — alchemy reagent
struct IngredientData {
        uint32_t formID = 0;
        std::string editorID;
        std::string fullName;
        uint32_t value = 0;
        float weight = 0.0f;
        std::string modelPath;
        // Up to 4 magic effects
        std::vector<uint32_t> effectFormIDs;    // IRQD subrecords
        std::vector<float> effectMagnitudes;    // IRQF
        std::vector<uint32_t> effectAreas;      // IRQA
        std::vector<uint32_t> effectDurations;  // IRQT
};

/// Potion/Alchemy (ALCH) record
struct AlchemyData {
        uint32_t formID = 0;
        std::string editorID;
        std::string fullName;
        uint32_t value = 0;
        float weight = 0.0f;
        std::string modelPath;
        // Magic effects (same structure as INGR but with ENIT subrecord header)
        std::vector<uint32_t> effectFormIDs;
        std::vector<float> effectMagnitudes;
        std::vector<uint32_t> effectAreas;
        std::vector<uint32_t> effectDurations;
};

/// Miscellaneous item (MISC) record
struct MiscItemData {
        uint32_t formID = 0;
        std::string editorID;
        std::string fullName;
        uint32_t value = 0;
        float weight = 0.0f;
        std::string modelPath;
};

/// Road/PathGrid (ROAD) record
struct RoadData {
        uint32_t formID = 0;                  // Cell formID this road belongs to
        uint32_t cellFormID = 0;
        std::vector<glm::vec3> nodes;         // Path grid nodes
        std::vector<std::pair<uint16_t, uint16_t>> edges;  // Node index pairs
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
        const std::vector<CreatureData>& getCreatures() const { return m_creatures; }
        const std::vector<WeaponData>& getWeapons() const { return m_weapons; }
        const std::vector<QuestData>& getQuests() const { return m_quests; }
        const std::vector<DialogData>& getDialogs() const { return m_dialogs; }
        const std::vector<ReferenceData>& getReferences() const { return m_references; }
        const std::vector<TerrainData>& getTerrains() const { return m_terrains; }
        const std::vector<WorldData>& getWorlds() const { return m_worlds; }
        const std::vector<SpellData>& getSpells() const { return m_spells; }
        const std::vector<EnchantmentData>& getEnchantments() const { return m_enchantments; }
        const std::vector<MagicEffectData>& getMagicEffects() const { return m_magicEffects; }
        const std::vector<SkillData>& getSkills() const { return m_skills; }
        const std::vector<BirthsignData>& getBirthsigns() const { return m_birthsigns; }
        const std::vector<ContainerData>& getContainers() const { return m_containers; }
        const std::vector<LightData>& getLights() const { return m_lights; }
        const std::vector<StaticData>& getStatics() const { return m_statics; }
        const std::vector<SoundData>& getSounds() const { return m_sounds; }
        const std::vector<TreeData>& getTrees() const { return m_trees; }
        const std::vector<FloraData>& getFloras() const { return m_floras; }
        const std::vector<ActivatorData>& getActivators() const { return m_activators; }
        const std::vector<ApparatusData>& getApparatuses() const { return m_apparatuses; }
        const std::vector<EyesData>& getEyes() const { return m_eyes; }
        const std::vector<HairData>& getHairs() const { return m_hairs; }
        const std::vector<ClimateData>& getClimates() const { return m_climates; }
        const std::vector<RegionData>& getRegions() const { return m_regions; }
    const std::vector<LeveledListData>& getLeveledLists() const { return m_leveledLists; }
    const std::vector<NavMeshData>& getNavMeshes() const { return m_navMeshes; }
    const std::vector<BookData>& getBooks() const { return m_books; }
    const std::vector<FactionData>& getFactions() const { return m_factions; }
    const std::vector<RaceData>& getRaces() const { return m_races; }
    const std::vector<ClassData>& getClasses() const { return m_classes; }
    const std::vector<ClothingData>& getClothing() const { return m_clothing; }
    const std::vector<IngredientData>& getIngredients() const { return m_ingredients; }
    const std::vector<AlchemyData>& getAlchemy() const { return m_alchemy; }
    const std::vector<MiscItemData>& getMiscItems() const { return m_miscItems; }
    const std::vector<RoadData>& getRoads() const { return m_roads; }
    const std::vector<ArmorData>& getArmors() const { return m_armors; }

        // Get file metadata
        const std::string& getFileName() const { return m_fileName; }
        bool isMaster() const { return m_isMaster; }

private:
    std::string m_fileName;
    bool m_isMaster = false;

    std::vector<CellData> m_cells;
    std::vector<NPCData> m_npcs;
    std::vector<CreatureData> m_creatures;
    std::vector<WeaponData> m_weapons;
    std::vector<QuestData> m_quests;
    std::vector<DialogData> m_dialogs;
    std::vector<ReferenceData> m_references;
    std::vector<TerrainData> m_terrains;
    std::vector<WorldData> m_worlds;
    std::vector<SpellData> m_spells;
    std::vector<EnchantmentData> m_enchantments;
    std::vector<MagicEffectData> m_magicEffects;
    std::vector<SkillData> m_skills;
    std::vector<BirthsignData> m_birthsigns;
    std::vector<ContainerData> m_containers;
    std::vector<LightData> m_lights;
    std::vector<StaticData> m_statics;
    std::vector<SoundData> m_sounds;
    std::vector<TreeData> m_trees;
    std::vector<FloraData> m_floras;
    std::vector<ActivatorData> m_activators;
    std::vector<ApparatusData> m_apparatuses;
    std::vector<EyesData> m_eyes;
    std::vector<HairData> m_hairs;
    std::vector<ClimateData> m_climates;
    std::vector<RegionData> m_regions;
        std::vector<LeveledListData> m_leveledLists;
    std::vector<NavMeshData> m_navMeshes;
    std::vector<ArmorData> m_armors;
    std::vector<BookData> m_books;
    std::vector<FactionData> m_factions;
    std::vector<RaceData> m_races;
    std::vector<ClassData> m_classes;
    std::vector<ClothingData> m_clothing;
    std::vector<IngredientData> m_ingredients;
    std::vector<AlchemyData> m_alchemy;
    std::vector<MiscItemData> m_miscItems;
    std::vector<RoadData> m_roads;

    // DIAL/INFO tracking — last DIAL formID for child INFO association
    uint32_t m_lastDialFormID = 0;

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
        void decodeCreature(const ESMRecord& rec);
        void decodeWeapon(const ESMRecord& rec);
        void decodeQuest(const ESMRecord& rec);
        void decodeDialog(const ESMRecord& rec);
        void decodeInfo(const ESMRecord& rec);
        void decodeReference(const ESMRecord& rec);
        void decodeTerrain(const ESMRecord& rec);
        void decodeWorld(const ESMRecord& rec);
                void decodeSpell(const ESMRecord& rec);
                void decodeEnchantment(const ESMRecord& rec);
                void decodeMagicEffect(const ESMRecord& rec);
                void decodeSkill(const ESMRecord& rec);
                void decodeBirthsign(const ESMRecord& rec);
                void decodeContainer(const ESMRecord& rec);
                void decodeLight(const ESMRecord& rec);
                void decodeStatic(const ESMRecord& rec);
                void decodeSound(const ESMRecord& rec);
                void decodeTree(const ESMRecord& rec);
                void decodeFlora(const ESMRecord& rec);
                void decodeActivator(const ESMRecord& rec);
                void decodeApparatus(const ESMRecord& rec);
                void decodeEyes(const ESMRecord& rec);
                void decodeHair(const ESMRecord& rec);
                void decodeClimate(const ESMRecord& rec);
                void decodeRegion(const ESMRecord& rec);
                void decodeLeveledList(const ESMRecord& rec);
                void decodeNavMesh(const ESMRecord& rec);
                void decodeArmor(const ESMRecord& rec);
                void decodeBook(const ESMRecord& rec);
                void decodeFaction(const ESMRecord& rec);
                void decodeRace(const ESMRecord& rec);
                void decodeClass(const ESMRecord& rec);
                void decodeClothing(const ESMRecord& rec);
                void decodeIngredient(const ESMRecord& rec);
                void decodeAlchemy(const ESMRecord& rec);
                void decodeMiscItem(const ESMRecord& rec);
                void decodeRoad(const ESMRecord& rec);
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
                                 file->getSpells().size() + file->getLeveledLists().size() +
                                     file->getNavMeshes().size() + file->getArmors().size() +
                                     file->getBooks().size() + file->getFactions().size() +
                                     file->getRaces().size() + file->getClasses().size() +
                                     file->getClothing().size() + file->getIngredients().size() +
                                     file->getAlchemy().size() + file->getMiscItems().size() +
                                     file->getRoads().size();
                        return count;
                    }
        size_t findRecordsByType(const std::string& type) const;

    // Look up data by FormID
    const NPCData* findNPC(uint32_t formID) const;
    const CreatureData* findCreature(uint32_t formID) const;
    const CellData* findCell(uint32_t formID) const;
    const WeaponData* findWeapon(uint32_t formID) const;
    const ArmorData* findArmor(uint32_t formID) const;
    const SpellData* findSpell(uint32_t formID) const;
    const EnchantmentData* findEnchantment(uint32_t formID) const;
    const MagicEffectData* findMagicEffect(uint32_t formID) const;
    const SkillData* findSkill(uint32_t formID) const;
    const BirthsignData* findBirthsign(uint32_t formID) const;
    const ContainerData* findContainer(uint32_t formID) const;
    const LightData* findLight(uint32_t formID) const;
    const StaticData* findStatic(uint32_t formID) const;
    const SoundData* findSound(uint32_t formID) const;
    const TreeData* findTree(uint32_t formID) const;
    const FloraData* findFlora(uint32_t formID) const;
    const ActivatorData* findActivator(uint32_t formID) const;
    const ApparatusData* findApparatus(uint32_t formID) const;
    const EyesData* findEyes(uint32_t formID) const;
    const HairData* findHair(uint32_t formID) const;
    const ClimateData* findClimate(uint32_t formID) const;
    const RegionData* findRegion(uint32_t formID) const;
    const QuestData* findQuest(uint32_t formID) const;
    const DialogData* findDialog(uint32_t formID) const;
    const LeveledListData* findLeveledList(uint32_t formID) const;
    const NavMeshData* findNavMesh(uint32_t formID) const;
    const WorldData* findWorld(uint32_t formID) const;
    const RaceData* findRace(uint32_t formID) const;
    const ClassData* findClass(uint32_t formID) const;
    const BookData* findBook(uint32_t formID) const;
    const ClothingData* findClothing(uint32_t formID) const;
    const IngredientData* findIngredient(uint32_t formID) const;
    const AlchemyData* findAlchemy(uint32_t formID) const;
    const MiscItemData* findMiscItem(uint32_t formID) const;
    const FactionData* findFaction(uint32_t formID) const;

    // Resolve a leveled list: pick entries appropriate for the given player level
    // Returns a list of (referencedFormID, count) pairs
    std::vector<std::pair<uint32_t, uint16_t>> resolveLeveledList(uint32_t listFormID, uint32_t playerLevel) const;

    // Iteration
    const std::vector<NPCData>& getAllNPCs() const;
    const std::vector<CreatureData>& getAllCreatures() const;
    const std::vector<CellData>& getAllCells() const;
    const std::vector<WeaponData>& getAllWeapons() const;
    const std::vector<QuestData>& getAllQuests() const;
    const std::vector<DialogData>& getAllDialogs() const;
    const std::vector<ReferenceData>& getAllReferences() const;
    const std::vector<TerrainData>& getAllTerrains() const;
    const std::vector<WorldData>& getAllWorlds() const;
    const std::vector<SpellData>& getAllSpells() const;
    const std::vector<EnchantmentData>& getAllEnchantments() const;
    const std::vector<MagicEffectData>& getAllMagicEffects() const;
    const std::vector<SkillData>& getAllSkills() const;
    const std::vector<BirthsignData>& getAllBirthsigns() const;
    const std::vector<ContainerData>& getAllContainers() const;
    const std::vector<LightData>& getAllLights() const;
    const std::vector<StaticData>& getAllStatics() const;
    const std::vector<SoundData>& getAllSounds() const;
    const std::vector<TreeData>& getAllTrees() const;
    const std::vector<FloraData>& getAllFloras() const;
    const std::vector<ActivatorData>& getAllActivators() const;
    const std::vector<ApparatusData>& getAllApparatuses() const;
    const std::vector<EyesData>& getAllEyes() const;
    const std::vector<HairData>& getAllHairs() const;
    const std::vector<ClimateData>& getAllClimates() const;
    const std::vector<RegionData>& getAllRegions() const;
    const std::vector<LeveledListData>& getAllLeveledLists() const;
    const std::vector<NavMeshData>& getAllNavMeshes() const;
    const std::vector<ArmorData>& getAllArmors() const;
    const std::vector<BookData>& getAllBooks() const;
    const std::vector<FactionData>& getAllFactions() const;
    const std::vector<RaceData>& getAllRaces() const;
    const std::vector<ClassData>& getAllClasses() const;
    const std::vector<ClothingData>& getAllClothing() const;
    const std::vector<IngredientData>& getAllIngredients() const;
    const std::vector<AlchemyData>& getAllAlchemy() const;
    const std::vector<MiscItemData>& getAllMiscItems() const;
    const std::vector<RoadData>& getAllRoads() const;

    size_t getPluginCount() const { return m_files.size(); }

private:
    std::vector<std::unique_ptr<ESMFile>> m_files;
    std::unordered_map<uint32_t, size_t> m_npcIndex;  // formID -> file index
    std::unordered_map<uint32_t, size_t> m_creatureIndex;
    std::unordered_map<uint32_t, size_t> m_cellIndex;
    std::unordered_map<uint32_t, size_t> m_weaponIndex;
        std::unordered_map<uint32_t, size_t> m_armorIndex;
        std::unordered_map<uint32_t, size_t> m_spellIndex;
        std::unordered_map<uint32_t, size_t> m_enchantmentIndex;
        std::unordered_map<uint32_t, size_t> m_magicEffectIndex;
        std::unordered_map<uint32_t, size_t> m_skillIndex;
        std::unordered_map<uint32_t, size_t> m_birthsignIndex;
        std::unordered_map<uint32_t, size_t> m_containerIndex;
        std::unordered_map<uint32_t, size_t> m_lightIndex;
        std::unordered_map<uint32_t, size_t> m_staticIndex;
        std::unordered_map<uint32_t, size_t> m_soundIndex;
        std::unordered_map<uint32_t, size_t> m_treeIndex;
        std::unordered_map<uint32_t, size_t> m_floraIndex;
        std::unordered_map<uint32_t, size_t> m_activatorIndex;
        std::unordered_map<uint32_t, size_t> m_apparatusIndex;
        std::unordered_map<uint32_t, size_t> m_eyesIndex;
        std::unordered_map<uint32_t, size_t> m_hairIndex;
        std::unordered_map<uint32_t, size_t> m_climateIndex;
        std::unordered_map<uint32_t, size_t> m_regionIndex;
        std::unordered_map<uint32_t, size_t> m_questIndex;
        std::unordered_map<uint32_t, size_t> m_dialogIndex;
        std::unordered_map<uint32_t, size_t> m_leveledListIndex;
        std::unordered_map<uint32_t, size_t> m_navMeshIndex;
        std::unordered_map<uint32_t, size_t> m_worldIndex;
        std::unordered_map<uint32_t, size_t> m_raceIndex;
        std::unordered_map<uint32_t, size_t> m_classIndex;
        std::unordered_map<uint32_t, size_t> m_bookIndex;
        std::unordered_map<uint32_t, size_t> m_clothingIndex;
        std::unordered_map<uint32_t, size_t> m_ingredientIndex;
        std::unordered_map<uint32_t, size_t> m_alchemyIndex;
        std::unordered_map<uint32_t, size_t> m_miscItemIndex;
        std::unordered_map<uint32_t, size_t> m_factionIndex;

    void rebuildIndices();
};

} // namespace oblivion
