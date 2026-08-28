#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <cstring>

// ============================================================================
// Phase 52: FaceGen Data Structures
//
// ESM record types and binary parser for Oblivion FaceGen data.
// FaceGen data is stored in NPC_ records as subrecords:
//   FGGS - FaceGen Geometry Symmetric (morph offsets)
//   FGGA - FaceGen Geometry Asymmetric
//   FGTS - FaceGen Texture Symmetric
//   SNAM - FaceGen texture swap (skin/hair)
// ============================================================================

namespace facegen {

// ============================================================================
// ESM Record Type Constants (4CC tags)
// ============================================================================

static constexpr uint32_t RECORD_NPC_ = 0x5F43504E;  // 'NPC_' in little-endian
static constexpr uint32_t RECORD_RACE = 0x45434152;  // 'RACE'

// Subrecord tags for FaceGen data
static constexpr uint32_t TAG_FGGS = 0x53474746;  // 'FGGS' - symmetric geometry
static constexpr uint32_t TAG_FGGA = 0x41474746;  // 'FGGA' - asymmetric geometry
static constexpr uint32_t TAG_FGTS = 0x53544746;  // 'FGTS' - symmetric texture
static constexpr uint32_t TAG_SNAM = 0x4D414E53;  // 'SNAM' - skin texture swap
static constexpr uint32_t TAG_HCLF = 0x464C4348;  // 'HCLF' - hair color
static constexpr uint32_t TAG_INAM = 0x4D414E49;  // 'INAM' - hair/eye asset
static constexpr uint32_t TAG_RNAM = 0x4D414E52;  // 'RNAM' - race FormID
static constexpr uint32_t TAG_CNAM = 0x4D414E43;  // 'CNAM' - class
static constexpr uint32_t TAG_ENAM = 0x4D414E45;  // 'ENAM' - eyes
static constexpr uint32_t TAG_HAIR = 0x52494148;  // 'HAIR' - hair model

// ============================================================================
// FaceGen Geometry Constants
// ============================================================================

// Oblivion FaceGen uses 100 symmetric + 100 asymmetric morph parameters
// Each parameter is a float controlling a blend shape
static constexpr int FACEGEN_NUM_SYMMETRIC_PARAMS  = 100;
static constexpr int FACEGEN_NUM_ASYMMETRIC_PARAMS = 100;
static constexpr int FACEGEN_NUM_TEXTURE_PARAMS    = 100;
static constexpr int FACEGEN_TOTAL_MORPH_PARAMS    = FACEGEN_NUM_SYMMETRIC_PARAMS + FACEGEN_NUM_ASYMMETRIC_PARAMS;

// FGGS/FGGA data size: 100 floats (400 bytes)
static constexpr size_t FACEGEN_GEOMETRY_DATA_SIZE = FACEGEN_NUM_SYMMETRIC_PARAMS * sizeof(float);
// FGTS data size: 100 floats (400 bytes)
static constexpr size_t FACEGEN_TEXTURE_DATA_SIZE  = FACEGEN_NUM_TEXTURE_PARAMS * sizeof(float);

// ============================================================================
// Face Shape Parameter Indices (Oblivion FaceGen standard)
// ============================================================================
// These map to the 100 symmetric morph parameters in FGGS

enum class FaceShapeParam : uint8_t {
    // Head shape (0-14)
    HeadWidth = 0,
    HeadHeight = 1,
    HeadDepth = 2,
    BrowWidth = 3,
    BrowHeight = 4,
    BrowDepth = 5,
    EarSize = 6,
    EarPosition = 7,
    EarRotation = 8,
    NoseWidth = 9,
    NoseHeight = 10,
    NoseLength = 11,
    NoseProfile = 12,
    NoseTip = 13,
    NoseSeptum = 14,

    // Cheeks (15-19)
    CheekWidth = 15,
    CheekHeight = 16,
    CheekDepth = 17,
    CheekboneWidth = 18,
    CheekboneHeight = 19,

    // Eyes (20-29)
    EyeWidth = 20,
    EyeHeight = 21,
    EyeDepth = 22,
    EyePosition = 23,
    EyeSeparation = 24,
    EyelidFold = 25,
    EyebrowArch = 26,
    EyebrowThickness = 27,
    EyebrowPosition = 28,
    EyebrowAngle = 29,

    // Mouth (30-39)
    MouthWidth = 30,
    MouthHeight = 31,
    MouthDepth = 32,
    LipThickness = 33,
    LipFullness = 34,
    LipCupid = 35,
    MouthPosition = 36,
    MouthExpression = 37,
    TeethWidth = 38,
    TeethDepth = 39,

    // Jaw (40-49)
    JawWidth = 40,
    JawHeight = 41,
    JawDepth = 42,
    JawAngle = 43,
    JawPosition = 44,
    ChinWidth = 45,
    ChinHeight = 46,
    ChinDepth = 47,
    ChinProminence = 48,
    ChinCleft = 49,

    // Forehead (50-54)
    ForeheadWidth = 50,
    ForeheadHeight = 51,
    ForeheadDepth = 52,
    ForeheadAngle = 53,
    ForeheadProminence = 54,

    // Neck (55-59)
    NeckWidth = 55,
    NeckHeight = 56,
    NeckDepth = 57,
    NeckMuscle = 58,
    NeckFat = 59,

    // Age/Weight/Complexion (60-69)
    AgeLines = 60,
    AgeSag = 61,
    WeightFat = 62,
    WeightMuscle = 63,
    Complexion = 64,
    SkinBlemish = 65,
    SkinWrinkle = 66,
    SkinPores = 67,
    SkinFreckle = 68,
    SkinScar = 69,

    // Hair (70-79)
    HairVolume = 70,
    HairLength = 71,
    HairCurl = 72,
    HairPart = 73,
    HairWidowsPeak = 74,
    HairReceding = 75,
    HairBangs = 76,
    HairSideburns = 77,
    HairBack = 78,
    HairTexture = 79,

    // Facial Hair (80-84)
    BeardFullness = 80,
    BeardLength = 81,
    MustacheFullness = 82,
    MustacheCurl = 83,
    Goatee = 84,

    // Makeup/Tattoo (85-89)
    Eyeliner = 85,
    Eyeshadow = 86,
    Blush = 87,
    Lipstick = 88,
    Tattoo = 89,

    // Extra (90-99)
    Extra0 = 90,
    Extra1 = 91,
    Extra2 = 92,
    Extra3 = 93,
    Extra4 = 94,
    Extra5 = 95,
    Extra6 = 96,
    Extra7 = 97,
    Extra8 = 98,
    Extra9 = 99,

    COUNT = 100
};

// ============================================================================
// FaceGen Record (parsed from ESM NPC_ record)
// ============================================================================

struct FaceGenRecord {
    uint32_t npcFormID = 0;

    // Symmetric geometry morphs (100 floats)
    // Controls bilateral-symmetric face shape changes
    float symmetricGeometry[FACEGEN_NUM_SYMMETRIC_PARAMS];

    // Asymmetric geometry morphs (100 floats)
    // Controls left-right asymmetric face shape changes
    float asymmetricGeometry[FACEGEN_NUM_ASYMMETRIC_PARAMS];

    // Texture morphs (100 floats)
    // Controls texture blending (skin tone, age, makeup, etc.)
    float textureMorphs[FACEGEN_NUM_TEXTURE_PARAMS];

    // Texture swap references
    uint32_t hairFormID = 0;
    uint32_t eyeFormID = 0;
    uint32_t hairColorFormID = 0;
    uint32_t raceFormID = 0;

    // Skin texture path (from SNAM)
    std::string skinTexturePath;

    FaceGenRecord() {
        std::memset(symmetricGeometry, 0, sizeof(symmetricGeometry));
        std::memset(asymmetricGeometry, 0, sizeof(asymmetricGeometry));
        std::memset(textureMorphs, 0, sizeof(textureMorphs));
    }

    bool hasGeometryData() const {
        // Check if any geometry data is non-zero
        for (int i = 0; i < FACEGEN_NUM_SYMMETRIC_PARAMS; ++i) {
            if (symmetricGeometry[i] != 0.0f) return true;
        }
        for (int i = 0; i < FACEGEN_NUM_ASYMMETRIC_PARAMS; ++i) {
            if (asymmetricGeometry[i] != 0.0f) return true;
        }
        return false;
    }

    bool hasTextureData() const {
        for (int i = 0; i < FACEGEN_NUM_TEXTURE_PARAMS; ++i) {
            if (textureMorphs[i] != 0.0f) return true;
        }
        return false;
    }
};

// ============================================================================
// Binary Parser for FaceGen Subrecords
// ============================================================================

class FaceGenParser {
public:
    // Parse FaceGen data from raw subrecord bytes
    // Returns true if valid FaceGen data was found
    static bool parseFGGS(FaceGenRecord& record, const uint8_t* data, size_t dataSize) {
        if (!data || dataSize < FACEGEN_GEOMETRY_DATA_SIZE) return false;
        std::memcpy(record.symmetricGeometry, data, FACEGEN_GEOMETRY_DATA_SIZE);
        return true;
    }

    static bool parseFGGA(FaceGenRecord& record, const uint8_t* data, size_t dataSize) {
        if (!data || dataSize < FACEGEN_GEOMETRY_DATA_SIZE) return false;
        std::memcpy(record.asymmetricGeometry, data, FACEGEN_GEOMETRY_DATA_SIZE);
        return true;
    }

    static bool parseFGTS(FaceGenRecord& record, const uint8_t* data, size_t dataSize) {
        if (!data || dataSize < FACEGEN_TEXTURE_DATA_SIZE) return false;
        std::memcpy(record.textureMorphs, data, FACEGEN_TEXTURE_DATA_SIZE);
        return true;
    }

    // Parse a complete FaceGen record from ESM subrecords
    // Caller provides subrecord data for each tag
    static FaceGenRecord parseFromSubrecords(
        uint32_t npcFormID,
        const uint8_t* fggsData, size_t fggsSize,
        const uint8_t* fggaData, size_t fggaSize,
        const uint8_t* fgtsData, size_t fgtsSize,
        const std::string& skinTexture = "",
        uint32_t hairID = 0, uint32_t eyeID = 0,
        uint32_t hairColorID = 0, uint32_t raceID = 0)
    {
        FaceGenRecord record;
        record.npcFormID = npcFormID;
        record.skinTexturePath = skinTexture;
        record.hairFormID = hairID;
        record.eyeFormID = eyeID;
        record.hairColorFormID = hairColorID;
        record.raceFormID = raceID;

        if (fggsData) parseFGGS(record, fggsData, fggsSize);
        if (fggaData) parseFGGA(record, fggaData, fggaSize);
        if (fgtsData) parseFGTS(record, fgtsData, fgtsSize);

        return record;
    }

    // Validate data integrity
    static bool validate(const FaceGenRecord& record) {
        // Check for NaN/Inf in morph values
        for (int i = 0; i < FACEGEN_NUM_SYMMETRIC_PARAMS; ++i) {
            if (record.symmetricGeometry[i] != record.symmetricGeometry[i]) return false; // NaN check
        }
        for (int i = 0; i < FACEGEN_NUM_ASYMMETRIC_PARAMS; ++i) {
            if (record.asymmetricGeometry[i] != record.asymmetricGeometry[i]) return false;
        }
        for (int i = 0; i < FACEGEN_NUM_TEXTURE_PARAMS; ++i) {
            if (record.textureMorphs[i] != record.textureMorphs[i]) return false;
        }
        return true;
    }
};

} // namespace facegen
