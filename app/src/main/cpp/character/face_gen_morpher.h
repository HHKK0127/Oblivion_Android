#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <android/log.h>

#include "face_gen_data.h"
#include "face_gen_shader.h"
#include "face_gen_cache.h"

// Forward declarations
class MeshLoader;
class AssetTextureManager;

// ============================================================================
// Phase 52: FaceGen Morpher
//
// FaceGen replacement system for NPC face generation/deformation.
// Uses metaball-style deformation + texture blending approach.
//
// Architecture:
//   1. Base face mesh (shared across all NPCs)
//   2. 200+ morph targets (100 symmetric + 100 asymmetric)
//   3. Shape parameter interpolation (0.0 - 1.0)
//   4. 4-layer texture blending (skin, age, makeup, detail)
//   5. LRU cache for generated faces
// ============================================================================

#define LOG_TAG_FG "FaceGenMorpher"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD_FG(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_FG, __VA_ARGS__)
#else
#define LOGD_FG(...) do {} while(0)
#endif
#define LOGI_FG(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_FG, __VA_ARGS__)
#define LOGW_FG(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG_FG, __VA_ARGS__)
#define LOGE_FG(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_FG, __VA_ARGS__)

namespace facegen {

// ============================================================================
// Face Shape - 50+ parameters controlling face deformation
// ============================================================================

struct FaceShape {
    // Head proportions
    float headWidth = 0.5f;
    float headHeight = 0.5f;
    float headDepth = 0.5f;

    // Nose
    float noseWidth = 0.5f;
    float noseHeight = 0.5f;
    float noseLength = 0.5f;
    float noseProfile = 0.5f;
    float noseTip = 0.5f;
    float noseSeptum = 0.5f;

    // Eyes
    float eyeWidth = 0.5f;
    float eyeHeight = 0.5f;
    float eyeDepth = 0.5f;
    float eyePosition = 0.5f;
    float eyeSeparation = 0.5f;
    float eyelidFold = 0.5f;

    // Eyebrows
    float eyebrowArch = 0.5f;
    float eyebrowThickness = 0.5f;
    float eyebrowPosition = 0.5f;
    float eyebrowAngle = 0.5f;

    // Mouth
    float mouthWidth = 0.5f;
    float mouthHeight = 0.5f;
    float mouthDepth = 0.5f;
    float lipThickness = 0.5f;
    float lipFullness = 0.5f;
    float lipCupid = 0.5f;
    float mouthPosition = 0.5f;

    // Jaw
    float jawWidth = 0.5f;
    float jawHeight = 0.5f;
    float jawDepth = 0.5f;
    float jawAngle = 0.5f;
    float chinWidth = 0.5f;
    float chinHeight = 0.5f;
    float chinDepth = 0.5f;
    float chinProminence = 0.5f;
    float chinCleft = 0.0f;

    // Cheeks
    float cheekWidth = 0.5f;
    float cheekHeight = 0.5f;
    float cheekDepth = 0.5f;
    float cheekboneWidth = 0.5f;
    float cheekboneHeight = 0.5f;

    // Ears
    float earSize = 0.5f;
    float earPosition = 0.5f;
    float earRotation = 0.5f;

    // Forehead
    float foreheadWidth = 0.5f;
    float foreheadHeight = 0.5f;
    float foreheadDepth = 0.5f;

    // Neck
    float neckWidth = 0.5f;
    float neckHeight = 0.5f;
    float neckDepth = 0.5f;

    // Age/Weight
    float age = 0.5f;         // 0.0 = young, 1.0 = old
    float weight = 0.5f;      // 0.0 = thin, 1.0 = heavy
    float complexion = 0.5f;  // 0.0 = clear, 1.0 = blemished

    // Gender (0.0 = feminine, 1.0 = masculine)
    float gender = 0.5f;

    // Race (index into race table)
    uint32_t raceIndex = 0;

    // Convert to raw morph parameter array (100 floats)
    void toMorphArray(float out[FACEGEN_NUM_SYMMETRIC_PARAMS]) const;
};

// ============================================================================
// Face Texture - multi-layer texture blend configuration
// ============================================================================

struct FaceTexture {
    // Base skin texture path
    std::string skinTexturePath;

    // Age texture (wrinkles, age spots)
    std::string ageTexturePath;
    float ageWeight = 0.0f;

    // Makeup texture
    std::string makeupTexturePath;
    float makeupWeight = 0.0f;

    // Detail texture (scars, tattoos, paint)
    std::string detailTexturePath;
    float detailWeight = 0.0f;

    // Normal map
    std::string normalMapPath;

    // Hair color (RGB)
    glm::vec3 hairColor = glm::vec3(0.3f, 0.2f, 0.1f);

    // Eye color (RGB)
    glm::vec3 eyeColor = glm::vec3(0.3f, 0.5f, 0.7f);

    // Skin tone modifier
    float skinTone = 0.5f;  // 0.0 = light, 1.0 = dark
};

// ============================================================================
// Face Morph Target - vertex offset arrays for blend shapes
// ============================================================================

struct FaceMorphTarget {
    std::string name;
    std::vector<glm::vec3> positionOffsets;  // Per-vertex position deltas
    std::vector<glm::vec3> normalOffsets;    // Per-vertex normal deltas

    bool isValid() const {
        return !positionOffsets.empty() && positionOffsets.size() == normalOffsets.size();
    }

    size_t getVertexCount() const { return positionOffsets.size(); }
};

// ============================================================================
// FaceGenMorpher - singleton face generation system
// ============================================================================

class FaceGenMorpher {
public:
    static FaceGenMorpher& instance();

    // Lifecycle
    bool initialize(MeshLoader* meshLoader, AssetTextureManager* texCache);
    void cleanup();
    bool isInitialized() const { return initialized_; }

    // ========================================================================
    // ESM Data Loading
    // ========================================================================

    // Load FaceGen data from ESM record
    bool loadFaceGenData(uint32_t npcFormID, const facegen::FaceGenRecord& record);

    // Load FaceGen data for multiple NPCs (batch)
    bool loadFaceGenDataBatch(const std::vector<facegen::FaceGenRecord>& records);

    // Check if FaceGen data is loaded for an NPC
    bool hasFaceGenData(uint32_t npcFormID) const;

    // ========================================================================
    // Face Generation
    // ========================================================================

    // Generate morphed mesh from shape parameters
    // Returns true if mesh was generated successfully
    bool generateMorphedMesh(uint32_t npcId, const FaceShape& shape);

    // Generate blended texture from texture parameters
    // Returns true if texture was generated successfully
    bool generateBlendedTexture(uint32_t npcId, const FaceTexture& tex);

    // Generate complete face (mesh + texture) from ESM data
    bool generateFaceFromESM(uint32_t npcFormID);

    // ========================================================================
    // NPC Face Update
    // ========================================================================

    // Update NPC face with new shape (regenerates mesh)
    bool updateNpcFace(uint32_t npcId, const FaceShape& shape);

    // Update NPC face texture
    bool updateNpcFaceTexture(uint32_t npcId, const FaceTexture& tex);

    // ========================================================================
    // Accessors
    // ========================================================================

    // Get morphed mesh for NPC (returns nullptr if not generated)
    const CachedFaceMesh* getMorphedMesh(uint32_t npcId);

    // Get blended texture for NPC (returns nullptr if not generated)
    const CachedFaceTexture* getBlendedTexture(uint32_t npcId);

    // Get FaceGen record for NPC
    const facegen::FaceGenRecord* getFaceGenRecord(uint32_t npcFormID) const;

    // Get FaceShape from FaceGen record
    FaceShape shapeFromRecord(const facegen::FaceGenRecord& record) const;

    // Build default FaceShape from race name and gender
    FaceShape buildShapeFromRace(const std::string& raceName, bool isFemale) const;

    // Get FaceTexture from FaceGen record
    FaceTexture textureFromRecord(const facegen::FaceGenRecord& record) const;

    // ========================================================================
    // Base Mesh Management
    // ========================================================================

    // Load base face mesh from NIF file
    bool loadBaseMesh(const std::string& nifPath);

    // Generate procedural base face mesh
    bool generateBaseMesh();

    // Get base mesh vertex count
    size_t getBaseMeshVertexCount() const;

    // ========================================================================
    // Morph Target Management
    // ========================================================================

    // Load morph targets from file
    bool loadMorphTargets(const std::string& path);

    // Generate procedural morph targets for all 100 parameters
    bool generateMorphTargets();

    // Get morph target count
    size_t getMorphTargetCount() const { return morphTargets_.size(); }

    // ========================================================================
    // Cache Access
    // ========================================================================

    FaceGenCache& getCache() { return cache_; }
    const FaceGenCache& getCache() const { return cache_; }

    // ========================================================================
    // Statistics
    // ========================================================================

    size_t getLoadedRecordCount() const { return faceGenRecords_.size(); }
    size_t getGeneratedFaceCount() const;

private:
    FaceGenMorpher() = default;
    ~FaceGenMorpher();
    FaceGenMorpher(const FaceGenMorpher&) = delete;
    FaceGenMorpher& operator=(const FaceGenMorpher&) = delete;

    bool initialized_ = false;

    // External dependencies
    MeshLoader* meshLoader_ = nullptr;
    AssetTextureManager* texCache_ = nullptr;

    // Base face mesh data
    std::vector<MeshVertex> baseVertices_;
    std::vector<uint32_t> baseIndices_;
    bool baseMeshLoaded_ = false;

    // Morph targets (indexed by parameter index)
    std::vector<FaceMorphTarget> morphTargets_;

    // ESM FaceGen records (indexed by NPC FormID)
    std::unordered_map<uint32_t, facegen::FaceGenRecord> faceGenRecords_;

    // Generated face cache
    FaceGenCache cache_;

    // Thread safety
    mutable std::mutex mutex_;

    // Internal methods
    bool applyMorphTargets(std::vector<MeshVertex>& outVertices,
                           const std::vector<MeshVertex>& baseVertices,
                           const float morphWeights[FACEGEN_NUM_SYMMETRIC_PARAMS],
                           int numParams) const;

    bool blendTexturesCPU(std::vector<uint8_t>& outPixels,
                          uint32_t& outWidth, uint32_t& outHeight,
                          const FaceTexture& tex) const;

    void generateProceduralMorphTarget(int paramIndex, FaceMorphTarget& target) const;

    // Procedural base mesh generation helpers
    void generateFaceSphere(std::vector<MeshVertex>& vertices,
                            std::vector<uint32_t>& indices,
                            int rings, int sectors) const;

    void deformFaceSphere(std::vector<MeshVertex>& vertices) const;

    // Texture generation helpers
    void generateBaseSkinTexture(std::vector<uint8_t>& pixels,
                                  uint32_t width, uint32_t height,
                                  float skinTone) const;

    void generateAgeTexture(std::vector<uint8_t>& pixels,
                            uint32_t width, uint32_t height,
                            float age) const;

    void generateMakeupTexture(std::vector<uint8_t>& pixels,
                               uint32_t width, uint32_t height,
                               const glm::vec3& color, float intensity) const;
};

} // namespace facegen
