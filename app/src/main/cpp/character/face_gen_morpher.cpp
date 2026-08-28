#include "face_gen_morpher.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdlib>

// GLM constants not available in this minimal GLM build
static constexpr float PI_F = 3.14159265358979323846f;

namespace facegen {

// ============================================================================
// FaceShape::toMorphArray - Convert high-level shape to morph parameter array
// ============================================================================

void FaceShape::toMorphArray(float out[FACEGEN_NUM_SYMMETRIC_PARAMS]) const {
    if (!out) return;

    // Initialize to zero
    std::memset(out, 0, FACEGEN_NUM_SYMMETRIC_PARAMS * sizeof(float));

    // Map shape parameters to morph indices
    // The mapping follows Oblivion's FaceGen parameter layout
    out[static_cast<int>(FaceShapeParam::HeadWidth)]   = headWidth;
    out[static_cast<int>(FaceShapeParam::HeadHeight)]  = headHeight;
    out[static_cast<int>(FaceShapeParam::HeadDepth)]   = headDepth;

    // Nose
    out[static_cast<int>(FaceShapeParam::NoseWidth)]   = noseWidth;
    out[static_cast<int>(FaceShapeParam::NoseHeight)]  = noseHeight;
    out[static_cast<int>(FaceShapeParam::NoseLength)]  = noseLength;
    out[static_cast<int>(FaceShapeParam::NoseProfile)] = noseProfile;
    out[static_cast<int>(FaceShapeParam::NoseTip)]     = noseTip;
    out[static_cast<int>(FaceShapeParam::NoseSeptum)]  = noseSeptum;

    // Eyes
    out[static_cast<int>(FaceShapeParam::EyeWidth)]       = eyeWidth;
    out[static_cast<int>(FaceShapeParam::EyeHeight)]      = eyeHeight;
    out[static_cast<int>(FaceShapeParam::EyeDepth)]       = eyeDepth;
    out[static_cast<int>(FaceShapeParam::EyePosition)]    = eyePosition;
    out[static_cast<int>(FaceShapeParam::EyeSeparation)]  = eyeSeparation;
    out[static_cast<int>(FaceShapeParam::EyelidFold)]     = eyelidFold;

    // Eyebrows
    out[static_cast<int>(FaceShapeParam::EyebrowArch)]       = eyebrowArch;
    out[static_cast<int>(FaceShapeParam::EyebrowThickness)]  = eyebrowThickness;
    out[static_cast<int>(FaceShapeParam::EyebrowPosition)]   = eyebrowPosition;
    out[static_cast<int>(FaceShapeParam::EyebrowAngle)]      = eyebrowAngle;

    // Mouth
    out[static_cast<int>(FaceShapeParam::MouthWidth)]    = mouthWidth;
    out[static_cast<int>(FaceShapeParam::MouthHeight)]   = mouthHeight;
    out[static_cast<int>(FaceShapeParam::MouthDepth)]    = mouthDepth;
    out[static_cast<int>(FaceShapeParam::LipThickness)]  = lipThickness;
    out[static_cast<int>(FaceShapeParam::LipFullness)]   = lipFullness;
    out[static_cast<int>(FaceShapeParam::LipCupid)]      = lipCupid;
    out[static_cast<int>(FaceShapeParam::MouthPosition)] = mouthPosition;

    // Jaw
    out[static_cast<int>(FaceShapeParam::JawWidth)]      = jawWidth;
    out[static_cast<int>(FaceShapeParam::JawHeight)]     = jawHeight;
    out[static_cast<int>(FaceShapeParam::JawDepth)]      = jawDepth;
    out[static_cast<int>(FaceShapeParam::JawAngle)]      = jawAngle;
    out[static_cast<int>(FaceShapeParam::ChinWidth)]     = chinWidth;
    out[static_cast<int>(FaceShapeParam::ChinHeight)]    = chinHeight;
    out[static_cast<int>(FaceShapeParam::ChinDepth)]     = chinDepth;
    out[static_cast<int>(FaceShapeParam::ChinProminence)] = chinProminence;
    out[static_cast<int>(FaceShapeParam::ChinCleft)]     = chinCleft;

    // Cheeks
    out[static_cast<int>(FaceShapeParam::CheekWidth)]      = cheekWidth;
    out[static_cast<int>(FaceShapeParam::CheekHeight)]     = cheekHeight;
    out[static_cast<int>(FaceShapeParam::CheekDepth)]      = cheekDepth;
    out[static_cast<int>(FaceShapeParam::CheekboneWidth)]  = cheekboneWidth;
    out[static_cast<int>(FaceShapeParam::CheekboneHeight)] = cheekboneHeight;

    // Ears
    out[static_cast<int>(FaceShapeParam::EarSize)]      = earSize;
    out[static_cast<int>(FaceShapeParam::EarPosition)]  = earPosition;
    out[static_cast<int>(FaceShapeParam::EarRotation)]  = earRotation;

    // Forehead
    out[static_cast<int>(FaceShapeParam::ForeheadWidth)]  = foreheadWidth;
    out[static_cast<int>(FaceShapeParam::ForeheadHeight)] = foreheadHeight;
    out[static_cast<int>(FaceShapeParam::ForeheadDepth)]  = foreheadDepth;

    // Neck
    out[static_cast<int>(FaceShapeParam::NeckWidth)]  = neckWidth;
    out[static_cast<int>(FaceShapeParam::NeckHeight)] = neckHeight;
    out[static_cast<int>(FaceShapeParam::NeckDepth)]  = neckDepth;

    // Age/Weight
    out[static_cast<int>(FaceShapeParam::AgeLines)]    = age;
    out[static_cast<int>(FaceShapeParam::AgeSag)]      = age;
    out[static_cast<int>(FaceShapeParam::WeightFat)]   = weight;
    out[static_cast<int>(FaceShapeParam::WeightMuscle)] = 1.0f - weight;
    out[static_cast<int>(FaceShapeParam::Complexion)]  = complexion;

    // Gender influence on face shape
    out[static_cast<int>(FaceShapeParam::JawWidth)]     = (out[static_cast<int>(FaceShapeParam::JawWidth)] + gender) * 0.5f;
    out[static_cast<int>(FaceShapeParam::ChinProminence)] = (out[static_cast<int>(FaceShapeParam::ChinProminence)] + gender) * 0.5f;
    out[static_cast<int>(FaceShapeParam::LipFullness)]  = (out[static_cast<int>(FaceShapeParam::LipFullness)] + (1.0f - gender)) * 0.5f;
}

// ============================================================================
// Singleton Implementation
// ============================================================================

FaceGenMorpher& FaceGenMorpher::instance() {
    static FaceGenMorpher inst;
    return inst;
}

FaceGenMorpher::~FaceGenMorpher() {
    cleanup();
}

// ============================================================================
// Lifecycle
// ============================================================================

bool FaceGenMorpher::initialize(MeshLoader* meshLoader, AssetTextureManager* texCache) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (initialized_) {
        LOGW_FG("FaceGenMorpher already initialized");
        return true;
    }

    meshLoader_ = meshLoader;
    texCache_ = texCache;

    // Initialize cache
    if (!cache_.initialize()) {
        LOGE_FG("Failed to initialize FaceGen cache");
        return false;
    }

    // Generate procedural base mesh
    if (!generateBaseMesh()) {
        LOGE_FG("Failed to generate base face mesh");
        return false;
    }

    // Generate procedural morph targets
    if (!generateMorphTargets()) {
        LOGW_FG("Failed to generate all morph targets (continuing with partial set)");
    }

    initialized_ = true;
    LOGI_FG("FaceGenMorpher initialized (base vertices: %lu, morph targets: %lu)",
            static_cast<unsigned long>(baseVertices_.size()),
            static_cast<unsigned long>(morphTargets_.size()));
    return true;
}

void FaceGenMorpher::cleanup() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) return;

    cache_.cleanup();
    baseVertices_.clear();
    baseIndices_.clear();
    morphTargets_.clear();
    faceGenRecords_.clear();
    baseMeshLoaded_ = false;

    meshLoader_ = nullptr;
    texCache_ = nullptr;

    initialized_ = false;
    LOGI_FG("FaceGenMorpher cleaned up");
}

// ============================================================================
// ESM Data Loading
// ============================================================================

bool FaceGenMorpher::loadFaceGenData(uint32_t npcFormID, const facegen::FaceGenRecord& record) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!FaceGenParser::validate(record)) {
        LOGW_FG("Invalid FaceGen record for NPC %u", npcFormID);
        return false;
    }

    faceGenRecords_[npcFormID] = record;
    LOGD_FG("Loaded FaceGen record for NPC %u (sym=%s, asym=%s, tex=%s)",
            npcFormID,
            record.hasGeometryData() ? "yes" : "no",
            record.hasGeometryData() ? "yes" : "no",
            record.hasTextureData() ? "yes" : "no");
    return true;
}

bool FaceGenMorpher::loadFaceGenDataBatch(const std::vector<facegen::FaceGenRecord>& records) {
    bool allSuccess = true;
    for (const auto& record : records) {
        if (!loadFaceGenData(record.npcFormID, record)) {
            allSuccess = false;
        }
    }
    LOGI_FG("Batch loaded %lu FaceGen records (all success: %s)",
            static_cast<unsigned long>(records.size()),
            allSuccess ? "yes" : "no");
    return allSuccess;
}

bool FaceGenMorpher::hasFaceGenData(uint32_t npcFormID) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return faceGenRecords_.find(npcFormID) != faceGenRecords_.end();
}

// ============================================================================
// Face Generation
// ============================================================================

bool FaceGenMorpher::generateMorphedMesh(uint32_t npcId, const FaceShape& shape) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_ || !baseMeshLoaded_) {
        LOGE_FG("FaceGenMorpher not initialized or base mesh not loaded");
        return false;
    }

    // Convert shape to morph parameter array
    float morphParams[FACEGEN_NUM_SYMMETRIC_PARAMS];
    shape.toMorphArray(morphParams);

    // Apply morph targets to base mesh
    std::vector<MeshVertex> morphedVertices;
    if (!applyMorphTargets(morphedVertices, baseVertices_, morphParams, FACEGEN_NUM_SYMMETRIC_PARAMS)) {
        LOGE_FG("Failed to apply morph targets for NPC %u", npcId);
        return false;
    }

    // Cache the morphed mesh
    std::vector<uint32_t> indices = baseIndices_;
    if (!cache_.cacheMesh(npcId, morphedVertices, indices)) {
        LOGE_FG("Failed to cache morphed mesh for NPC %u", npcId);
        return false;
    }

    // Upload to GPU
    cache_.uploadMeshToGPU(npcId);

    LOGD_FG("Generated morphed mesh for NPC %u (%lu vertices)",
            npcId, static_cast<unsigned long>(morphedVertices.size()));
    return true;
}

bool FaceGenMorpher::generateBlendedTexture(uint32_t npcId, const FaceTexture& tex) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!initialized_) {
        LOGE_FG("FaceGenMorpher not initialized");
        return false;
    }

    // Blend textures using CPU (shader-based blending is done at render time)
    std::vector<uint8_t> pixels;
    uint32_t width = 0, height = 0;
    if (!blendTexturesCPU(pixels, width, height, tex)) {
        LOGE_FG("Failed to blend textures for NPC %u", npcId);
        return false;
    }

    // Cache the blended texture
    if (!cache_.cacheTexture(npcId, pixels.data(), width, height)) {
        LOGE_FG("Failed to cache blended texture for NPC %u", npcId);
        return false;
    }

    // Upload to GPU
    cache_.uploadTextureToGPU(npcId);

    LOGD_FG("Generated blended texture for NPC %u (%ux%u)",
            npcId, width, height);
    return true;
}

bool FaceGenMorpher::generateFaceFromESM(uint32_t npcFormID) {
    auto it = faceGenRecords_.find(npcFormID);
    if (it == faceGenRecords_.end()) {
        LOGW_FG("No FaceGen data for NPC %u", npcFormID);
        return false;
    }

    const facegen::FaceGenRecord& record = it->second;

    // Convert record to shape/texture
    FaceShape shape = shapeFromRecord(record);
    FaceTexture tex = textureFromRecord(record);

    // Generate mesh and texture
    bool meshOk = generateMorphedMesh(npcFormID, shape);
    bool texOk = generateBlendedTexture(npcFormID, tex);

    return meshOk && texOk;
}

// ============================================================================
// NPC Face Update
// ============================================================================

bool FaceGenMorpher::updateNpcFace(uint32_t npcId, const FaceShape& shape) {
    return generateMorphedMesh(npcId, shape);
}

bool FaceGenMorpher::updateNpcFaceTexture(uint32_t npcId, const FaceTexture& tex) {
    return generateBlendedTexture(npcId, tex);
}

// ============================================================================
// Accessors
// ============================================================================

const CachedFaceMesh* FaceGenMorpher::getMorphedMesh(uint32_t npcId) {
    return cache_.getCachedMesh(npcId);
}

const CachedFaceTexture* FaceGenMorpher::getBlendedTexture(uint32_t npcId) {
    return cache_.getCachedTexture(npcId);
}

const facegen::FaceGenRecord* FaceGenMorpher::getFaceGenRecord(uint32_t npcFormID) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = faceGenRecords_.find(npcFormID);
    if (it == faceGenRecords_.end()) return nullptr;
    return &it->second;
}

FaceShape FaceGenMorpher::shapeFromRecord(const facegen::FaceGenRecord& record) const {
    FaceShape shape;

    // Map symmetric geometry morphs to FaceShape
    const float* sym = record.symmetricGeometry;

    shape.headWidth    = sym[static_cast<int>(FaceShapeParam::HeadWidth)];
    shape.headHeight   = sym[static_cast<int>(FaceShapeParam::HeadHeight)];
    shape.headDepth    = sym[static_cast<int>(FaceShapeParam::HeadDepth)];

    shape.noseWidth    = sym[static_cast<int>(FaceShapeParam::NoseWidth)];
    shape.noseHeight   = sym[static_cast<int>(FaceShapeParam::NoseHeight)];
    shape.noseLength   = sym[static_cast<int>(FaceShapeParam::NoseLength)];
    shape.noseProfile  = sym[static_cast<int>(FaceShapeParam::NoseProfile)];
    shape.noseTip      = sym[static_cast<int>(FaceShapeParam::NoseTip)];

    shape.eyeWidth      = sym[static_cast<int>(FaceShapeParam::EyeWidth)];
    shape.eyeHeight     = sym[static_cast<int>(FaceShapeParam::EyeHeight)];
    shape.eyeDepth      = sym[static_cast<int>(FaceShapeParam::EyeDepth)];
    shape.eyePosition   = sym[static_cast<int>(FaceShapeParam::EyePosition)];
    shape.eyeSeparation = sym[static_cast<int>(FaceShapeParam::EyeSeparation)];

    shape.jawWidth       = sym[static_cast<int>(FaceShapeParam::JawWidth)];
    shape.jawHeight      = sym[static_cast<int>(FaceShapeParam::JawHeight)];
    shape.jawDepth       = sym[static_cast<int>(FaceShapeParam::JawDepth)];
    shape.chinWidth      = sym[static_cast<int>(FaceShapeParam::ChinWidth)];
    shape.chinHeight     = sym[static_cast<int>(FaceShapeParam::ChinHeight)];
    shape.chinDepth      = sym[static_cast<int>(FaceShapeParam::ChinDepth)];
    shape.chinProminence = sym[static_cast<int>(FaceShapeParam::ChinProminence)];

    shape.cheekWidth     = sym[static_cast<int>(FaceShapeParam::CheekWidth)];
    shape.cheekHeight    = sym[static_cast<int>(FaceShapeParam::CheekHeight)];
    shape.cheekDepth     = sym[static_cast<int>(FaceShapeParam::CheekDepth)];

    shape.mouthWidth    = sym[static_cast<int>(FaceShapeParam::MouthWidth)];
    shape.mouthHeight   = sym[static_cast<int>(FaceShapeParam::MouthHeight)];
    shape.lipThickness  = sym[static_cast<int>(FaceShapeParam::LipThickness)];
    shape.lipFullness   = sym[static_cast<int>(FaceShapeParam::LipFullness)];

    shape.earSize      = sym[static_cast<int>(FaceShapeParam::EarSize)];
    shape.earPosition  = sym[static_cast<int>(FaceShapeParam::EarPosition)];

    shape.foreheadHeight = sym[static_cast<int>(FaceShapeParam::ForeheadHeight)];

    shape.age        = sym[static_cast<int>(FaceShapeParam::AgeLines)];
    shape.weight     = sym[static_cast<int>(FaceShapeParam::WeightFat)];
    shape.complexion = sym[static_cast<int>(FaceShapeParam::Complexion)];

    return shape;
}

FaceShape FaceGenMorpher::buildShapeFromRace(const std::string& raceName, bool isFemale) const {
    FaceShape shape;
    // Default values (0.5 = neutral)
    shape.headWidth = 0.5f;
    shape.headHeight = 0.5f;
    shape.headDepth = 0.5f;
    shape.noseWidth = 0.5f;
    shape.noseHeight = 0.5f;
    shape.noseLength = 0.5f;
    shape.noseProfile = 0.5f;
    shape.noseTip = 0.5f;
    shape.eyeWidth = 0.5f;
    shape.eyeHeight = 0.5f;
    shape.eyeDepth = 0.5f;
    shape.eyePosition = 0.5f;
    shape.eyeSeparation = 0.5f;
    shape.jawWidth = 0.5f;
    shape.jawHeight = 0.5f;
    shape.jawDepth = 0.5f;
    shape.chinWidth = 0.5f;
    shape.chinHeight = 0.5f;
    shape.chinDepth = 0.5f;
    shape.chinProminence = 0.5f;
    shape.cheekWidth = 0.5f;
    shape.cheekHeight = 0.5f;
    shape.cheekDepth = 0.5f;
    shape.mouthWidth = 0.5f;
    shape.mouthHeight = 0.5f;
    shape.mouthDepth = 0.5f;
    shape.lipFullness = 0.5f;
    shape.eyebrowArch = 0.5f;
    shape.eyebrowThickness = 0.5f;
    shape.eyebrowPosition = 0.5f;
    shape.eyebrowAngle = 0.5f;
    shape.foreheadHeight = 0.5f;
    shape.earSize = 0.5f;
    shape.earPosition = 0.5f;
    shape.foreheadWidth = 0.5f;
    shape.neckWidth = 0.5f;
    shape.neckDepth = 0.5f;
    shape.complexion = 0.5f;

    // Race-specific adjustments
    std::string lower = raceName;
    for (auto& c : lower) c = static_cast<char>(std::tolower(c));

    if (lower.find("argonian") != std::string::npos) {
        shape.headWidth = 0.35f;
        shape.headDepth = 0.65f;
        shape.noseLength = 0.7f;
        shape.eyeWidth = 0.6f;
        shape.jawWidth = 0.4f;
        shape.mouthWidth = 0.65f;
        shape.lipFullness = 0.3f;
        shape.earSize = 0.3f;
        shape.neckWidth = 0.6f;
    } else if (lower.find("khajiit") != std::string::npos) {
        shape.headWidth = 0.45f;
        shape.noseWidth = 0.65f;
        shape.eyeWidth = 0.6f;
        shape.eyeSeparation = 0.55f;
        shape.jawWidth = 0.55f;
        shape.mouthWidth = 0.6f;
        shape.earSize = 0.7f;
        shape.earPosition = 0.35f;
    } else if (lower.find("orc") != std::string::npos) {
        shape.headWidth = 0.65f;
        shape.jawWidth = 0.7f;
        shape.jawHeight = 0.6f;
        shape.chinProminence = 0.65f;
        shape.noseWidth = 0.6f;
        shape.eyebrowAngle = 0.7f;
        shape.neckWidth = 0.65f;
    } else if (lower.find("elf") != std::string::npos || lower.find("altmer") != std::string::npos ||
               lower.find("bosmer") != std::string::npos || lower.find("dunmer") != std::string::npos) {
        shape.headHeight = 0.6f;
        shape.earSize = 0.65f;
        shape.earPosition = 0.35f;
        shape.eyeHeight = 0.55f;
        shape.noseLength = 0.55f;
        shape.jawWidth = 0.4f;
    } else if (lower.find("breton") != std::string::npos) {
        shape.headWidth = 0.48f;
        shape.noseProfile = 0.52f;
        shape.cheekWidth = 0.52f;
    } else if (lower.find("redguard") != std::string::npos) {
        shape.noseWidth = 0.58f;
        shape.lipFullness = 0.58f;
        shape.jawWidth = 0.55f;
    } else if (lower.find("nord") != std::string::npos) {
        shape.headWidth = 0.55f;
        shape.jawWidth = 0.58f;
        shape.noseProfile = 0.55f;
        shape.eyebrowAngle = 0.58f;
        shape.neckWidth = 0.58f;
    }
    // Imperial uses defaults

    // Gender adjustments
    if (isFemale) {
        shape.jawWidth *= 0.9f;
        shape.chinProminence *= 0.85f;
        shape.eyebrowAngle *= 0.85f;
        shape.neckWidth *= 0.9f;
        shape.lipFullness *= 1.1f;
        shape.cheekHeight *= 1.05f;
    }

    return shape;
}

FaceTexture FaceGenMorpher::textureFromRecord(const facegen::FaceGenRecord& record) const {
    FaceTexture tex;
    tex.skinTexturePath = record.skinTexturePath;
    tex.ageWeight       = record.textureMorphs[static_cast<int>(FaceShapeParam::AgeLines)];
    tex.makeupWeight    = record.textureMorphs[static_cast<int>(FaceShapeParam::Eyeliner)] +
                          record.textureMorphs[static_cast<int>(FaceShapeParam::Lipstick)];
    tex.detailWeight    = record.textureMorphs[static_cast<int>(FaceShapeParam::SkinScar)];
    return tex;
}

size_t FaceGenMorpher::getGeneratedFaceCount() const {
    return cache_.getCachedMeshCount();
}

// ============================================================================
// Base Mesh Management
// ============================================================================

bool FaceGenMorpher::loadBaseMesh(const std::string& nifPath) {
    if (!meshLoader_) {
        LOGW_FG("MeshLoader not available, falling back to procedural mesh");
        return generateBaseMesh();
    }

    auto loadedMesh = meshLoader_->loadMesh(nifPath);
    if (!loadedMesh) {
        LOGW_FG("Failed to load base mesh from %s, falling back to procedural", nifPath.c_str());
        return generateBaseMesh();
    }

    // Extract vertices and indices from loaded mesh
    baseVertices_.clear();
    baseIndices_.clear();

    for (const auto& subMesh : loadedMesh->subMeshes) {
        // Submesh already has GPU resources, but we need CPU-side data
        // For procedural deformation, we need raw vertex data
        // Since LoadedMesh doesn't expose CPU vertices directly, fall back
        LOGW_FG("CPU vertex data not available from MeshLoader, using procedural");
        return generateBaseMesh();
    }

    return true;
}

bool FaceGenMorpher::generateBaseMesh() {
    // Generate procedural face mesh using sphere topology
    baseVertices_.clear();
    baseIndices_.clear();

    const int rings = 16;      // Latitude divisions
    const int sectors = 24;    // Longitude divisions

    generateFaceSphere(baseVertices_, baseIndices_, rings, sectors);
    deformFaceSphere(baseVertices_);

    baseMeshLoaded_ = true;
    LOGD_FG("Generated procedural base face mesh: %lu vertices, %lu indices",
            static_cast<unsigned long>(baseVertices_.size()),
            static_cast<unsigned long>(baseIndices_.size()));
    return true;
}

size_t FaceGenMorpher::getBaseMeshVertexCount() const {
    return baseVertices_.size();
}

// ============================================================================
// Morph Target Management
// ============================================================================

bool FaceGenMorpher::loadMorphTargets(const std::string& path) {
    // TODO: Implement loading from binary file
    LOGW_FG("Morph target loading from file not yet implemented");
    return false;
}

bool FaceGenMorpher::generateMorphTargets() {
    morphTargets_.clear();
    morphTargets_.reserve(FACEGEN_NUM_SYMMETRIC_PARAMS);

    for (int i = 0; i < FACEGEN_NUM_SYMMETRIC_PARAMS; ++i) {
        FaceMorphTarget target;
        target.name = "Morph_" + std::to_string(i);
        generateProceduralMorphTarget(i, target);
        morphTargets_.push_back(std::move(target));
    }

    LOGD_FG("Generated %lu morph targets",
            static_cast<unsigned long>(morphTargets_.size()));
    return !morphTargets_.empty();
}

// ============================================================================
// Internal Methods
// ============================================================================

bool FaceGenMorpher::applyMorphTargets(std::vector<MeshVertex>& outVertices,
                                        const std::vector<MeshVertex>& baseVertices,
                                        const float morphWeights[FACEGEN_NUM_SYMMETRIC_PARAMS],
                                        int numParams) const {
    outVertices = baseVertices;  // Copy base

    if (numParams > static_cast<int>(morphTargets_.size())) {
        numParams = static_cast<int>(morphTargets_.size());
    }

    // Apply each morph target weighted by its weight
    for (int p = 0; p < numParams; ++p) {
        float weight = morphWeights[p];
        if (weight == 0.0f) continue;

        const FaceMorphTarget& target = morphTargets_[p];
        if (!target.isValid() || target.getVertexCount() != outVertices.size()) {
            continue;
        }

        // Apply weighted offsets to each vertex
        for (size_t i = 0; i < outVertices.size(); ++i) {
            outVertices[i].position[0] += target.positionOffsets[i].x * weight;
            outVertices[i].position[1] += target.positionOffsets[i].y * weight;
            outVertices[i].position[2] += target.positionOffsets[i].z * weight;

            outVertices[i].normal[0] += target.normalOffsets[i].x * weight;
            outVertices[i].normal[1] += target.normalOffsets[i].y * weight;
            outVertices[i].normal[2] += target.normalOffsets[i].z * weight;
        }
    }

    // Normalize normals
    for (auto& vertex : outVertices) {
        glm::vec3 n(vertex.normal[0], vertex.normal[1], vertex.normal[2]);
        float len = glm::length(n);
        if (len > 0.0001f) {
            n /= len;
            vertex.normal[0] = n.x;
            vertex.normal[1] = n.y;
            vertex.normal[2] = n.z;
        }
    }

    return true;
}

bool FaceGenMorpher::blendTexturesCPU(std::vector<uint8_t>& outPixels,
                                       uint32_t& outWidth, uint32_t& outHeight,
                                       const FaceTexture& tex) const {
    // Standard face texture resolution
    const uint32_t width = 256;
    const uint32_t height = 256;

    outWidth = width;
    outHeight = height;
    outPixels.resize(static_cast<size_t>(width) * static_cast<size_t>(height) * 4);

    // Generate base skin texture
    std::vector<uint8_t> basePixels(width * height * 4);
    generateBaseSkinTexture(basePixels, width, height, tex.skinTone);

    // Blend with age overlay
    std::vector<uint8_t> agePixels(width * height * 4);
    generateAgeTexture(agePixels, width, height, tex.ageWeight);

    // Blend with makeup overlay
    std::vector<uint8_t> makeupPixels(width * height * 4);
    generateMakeupTexture(makeupPixels, width, height,
                          glm::vec3(0.8f, 0.2f, 0.3f), tex.makeupWeight);

    // Blend all layers
    for (uint32_t i = 0; i < width * height; ++i) {
        size_t idx = i * 4;

        // Start with base
        outPixels[idx + 0] = basePixels[idx + 0];
        outPixels[idx + 1] = basePixels[idx + 1];
        outPixels[idx + 2] = basePixels[idx + 2];
        outPixels[idx + 3] = basePixels[idx + 3];

        // Blend age
        if (tex.ageWeight > 0.0f) {
            float a = (agePixels[idx + 3] / 255.0f) * tex.ageWeight;
            outPixels[idx + 0] = static_cast<uint8_t>(outPixels[idx + 0] * (1 - a) + agePixels[idx + 0] * a);
            outPixels[idx + 1] = static_cast<uint8_t>(outPixels[idx + 1] * (1 - a) + agePixels[idx + 1] * a);
            outPixels[idx + 2] = static_cast<uint8_t>(outPixels[idx + 2] * (1 - a) + agePixels[idx + 2] * a);
        }

        // Blend makeup
        if (tex.makeupWeight > 0.0f) {
            float a = (makeupPixels[idx + 3] / 255.0f) * tex.makeupWeight;
            outPixels[idx + 0] = static_cast<uint8_t>(outPixels[idx + 0] * (1 - a) + makeupPixels[idx + 0] * a);
            outPixels[idx + 1] = static_cast<uint8_t>(outPixels[idx + 1] * (1 - a) + makeupPixels[idx + 1] * a);
            outPixels[idx + 2] = static_cast<uint8_t>(outPixels[idx + 2] * (1 - a) + makeupPixels[idx + 2] * a);
        }
    }

    return true;
}

void FaceGenMorpher::generateProceduralMorphTarget(int paramIndex, FaceMorphTarget& target) const {
    // Generate a procedural morph target based on parameter index
    target.positionOffsets.clear();
    target.normalOffsets.clear();

    if (baseVertices_.empty()) return;

    target.positionOffsets.resize(baseVertices_.size());
    target.normalOffsets.resize(baseVertices_.size());

    for (size_t i = 0; i < baseVertices_.size(); ++i) {
        glm::vec3 pos(baseVertices_[i].position[0],
                      baseVertices_[i].position[1],
                      baseVertices_[i].position[2]);

        glm::vec3 offset(0.0f, 0.0f, 0.0f);
        glm::vec3 normalOffset(0.0f, 0.0f, 0.0f);

        // Procedural deformation based on parameter index
        // Each morph affects specific facial regions
        switch (paramIndex) {
        case 0: // HeadWidth - scale X
                    offset.x = pos.x * 0.05f;
            break;
        case 1: // HeadHeight - scale Y
                    offset.y = pos.y * 0.05f;
            break;
        case 2: // HeadDepth - scale Z
                    offset.z = pos.z * 0.05f;
            break;
        case 9: // NoseWidth - scale nose region
            if (pos.y > -0.1f && pos.y < 0.2f && std::abs(pos.z) > 0.3f) {
                offset.x = pos.x * 0.1f;
            }
            break;
        case 10: // NoseHeight - scale nose vertically
            if (std::abs(pos.z) > 0.3f && pos.y > -0.2f) {
                offset.y = 0.05f;
            }
            break;
        case 15: // CheekWidth
            if (pos.y > -0.2f && pos.y < 0.1f) {
                offset.x = pos.x * 0.08f;
            }
            break;
        case 20: // EyeWidth
            if (pos.y > 0.0f && pos.y < 0.15f) {
                offset.x = pos.x * 0.06f;
            }
            break;
        case 30: // MouthWidth
            if (pos.y < -0.2f && pos.y > -0.3f) {
                offset.x = pos.x * 0.08f;
            }
            break;
        case 40: // JawWidth
            if (pos.y < -0.2f) {
                offset.x = pos.x * 0.06f;
            }
            break;
        case 45: // ChinWidth
            if (pos.y < -0.4f) {
                offset.x = pos.x * 0.05f;
            }
            break;
        case 46: // ChinHeight
            if (pos.y < -0.4f) {
                offset.y = 0.04f;
            }
            break;
        case 50: // ForeheadWidth
            if (pos.y > 0.2f) {
                offset.x = pos.x * 0.04f;
            }
            break;
        case 60: // AgeLines - add subtle bumps
        {
            float noise = std::sin(pos.x * 10.0f) * std::cos(pos.y * 8.0f) * 0.005f;
            offset = glm::vec3(noise, 0.0f, noise);
            break;
        }
        case 62: // WeightFat - inflate
            offset = pos * 0.03f;
            break;
        default:
            // Generic small deformation
            offset = pos * 0.01f;
            break;
        }

        target.positionOffsets[i] = offset;

        // Normal offset points in same direction as position offset (simplified)
        target.normalOffsets[i] = glm::length(offset) > 0.0001f ?
                                   glm::normalize(offset) : glm::vec3(0.0f, 1.0f, 0.0f);
    }
}

// ============================================================================
// Procedural Base Mesh Helpers
// ============================================================================

void FaceGenMorpher::generateFaceSphere(std::vector<MeshVertex>& vertices,
                                          std::vector<uint32_t>& indices,
                                          int rings, int sectors) const {
    vertices.clear();
    indices.clear();

    const float radius = 0.5f;

    // Generate vertices
    for (int r = 0; r <= rings; ++r) {
        for (int s = 0; s <= sectors; ++s) {
            float v = static_cast<float>(r) / static_cast<float>(rings);
            float u = static_cast<float>(s) / static_cast<float>(sectors);

            float phi = v * PI_F;
            float theta = u * 2.0f * PI_F;

            float x = radius * std::sin(phi) * std::cos(theta);
            float y = radius * std::cos(phi);
            float z = radius * std::sin(phi) * std::sin(theta);

            MeshVertex vert;
            vert.position[0] = x;
            vert.position[1] = y;
            vert.position[2] = z;
            vert.normal[0] = x / radius;
            vert.normal[1] = y / radius;
            vert.normal[2] = z / radius;
            vert.texCoord[0] = u;
            vert.texCoord[1] = v;
            vert.boneWeights[0] = 1.0f;
            vert.boneWeights[1] = 0.0f;
            vert.boneWeights[2] = 0.0f;
            vert.boneWeights[3] = 0.0f;
            vert.boneIndices[0] = 0;
            vert.boneIndices[1] = 0;
            vert.boneIndices[2] = 0;
            vert.boneIndices[3] = 0;

            vertices.push_back(vert);
        }
    }

    // Generate indices
    for (int r = 0; r < rings; ++r) {
        for (int s = 0; s < sectors; ++s) {
            uint32_t a = r * (sectors + 1) + s;
            uint32_t b = a + sectors + 1;

            indices.push_back(a);
            indices.push_back(b);
            indices.push_back(a + 1);

            indices.push_back(b);
            indices.push_back(b + 1);
            indices.push_back(a + 1);
        }
    }
}

void FaceGenMorpher::deformFaceSphere(std::vector<MeshVertex>& vertices) const {
    // Deform sphere into face shape
    for (auto& vertex : vertices) {
        glm::vec3 pos(vertex.position[0], vertex.position[1], vertex.position[2]);

        // Flatten back of head
        if (pos.z < 0.0f) {
            pos.z *= 0.5f;
        }

        // Extend chin
        if (pos.y < -0.3f) {
            pos.y *= 1.2f;
        }

        // Make forehead taller
        if (pos.y > 0.3f) {
            pos.y *= 1.1f;
        }

        // Indent eye sockets
        if (pos.y > 0.05f && pos.y < 0.15f && std::abs(pos.x) > 0.15f) {
            pos.z *= 0.85f;
        }

        // Protrude nose
        if (pos.y > -0.1f && pos.y < 0.15f && std::abs(pos.x) < 0.1f) {
            pos.z += 0.08f;
        }

        // Make jaw wider
        if (pos.y < -0.2f) {
            pos.x *= 1.1f;
        }

        vertex.position[0] = pos.x;
        vertex.position[1] = pos.y;
        vertex.position[2] = pos.z;
    }
}

// ============================================================================
// Texture Generation Helpers
// ============================================================================

void FaceGenMorpher::generateBaseSkinTexture(std::vector<uint8_t>& pixels,
                                              uint32_t width, uint32_t height,
                                              float skinTone) const {
    // Skin color gradient based on tone (0.0 = light, 1.0 = dark)
    uint8_t r = static_cast<uint8_t>(255.0f * (1.0f - skinTone * 0.6f));
    uint8_t g = static_cast<uint8_t>(220.0f * (1.0f - skinTone * 0.5f));
    uint8_t b = static_cast<uint8_t>(180.0f * (1.0f - skinTone * 0.4f));

    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            size_t idx = (static_cast<size_t>(y) * width + x) * 4;

            // Add slight noise for skin texture
            uint32_t noise = (x * 13 + y * 7) % 32;
            pixels[idx + 0] = static_cast<uint8_t>(r + noise - 16);
            pixels[idx + 1] = static_cast<uint8_t>(g + noise - 16);
            pixels[idx + 2] = static_cast<uint8_t>(b + noise - 16);
            pixels[idx + 3] = 255;
        }
    }
}

void FaceGenMorpher::generateAgeTexture(std::vector<uint8_t>& pixels,
                                         uint32_t width, uint32_t height,
                                         float age) const {
    // Age overlay: darker spots and lines
    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            size_t idx = (static_cast<size_t>(y) * width + x) * 4;

            // Create wrinkle pattern
            float wrinkle = 0.0f;
            if (y > height / 2 && y < 3 * height / 4) {
                wrinkle = std::sin(static_cast<float>(x) / 8.0f) * 0.3f;
            }

            float darkness = std::max(0.0f, std::sin(static_cast<float>(x + y) / 10.0f)) * age;

            pixels[idx + 0] = static_cast<uint8_t>(255.0f * (1.0f - darkness));
            pixels[idx + 1] = static_cast<uint8_t>(255.0f * (1.0f - darkness));
            pixels[idx + 2] = static_cast<uint8_t>(255.0f * (1.0f - darkness));
            pixels[idx + 3] = static_cast<uint8_t>((darkness + wrinkle * age) * 255.0f);
        }
    }
}

void FaceGenMorpher::generateMakeupTexture(std::vector<uint8_t>& pixels,
                                            uint32_t width, uint32_t height,
                                            const glm::vec3& color, float intensity) const {
    // Makeup overlay: colored regions around eyes and lips
    for (uint32_t y = 0; y < height; ++y) {
        for (uint32_t x = 0; x < width; ++x) {
            size_t idx = (static_cast<size_t>(y) * width + x) * 4;

            float u = static_cast<float>(x) / static_cast<float>(width);
            float v = static_cast<float>(y) / static_cast<float>(height);

            // Eye region (upper third)
            float eyeMask = (v > 0.3f && v < 0.45f) ? 1.0f : 0.0f;

            // Lip region (lower third)
            float lipMask = (v > 0.7f && v < 0.85f) ? 1.0f : 0.0f;

            float mask = std::max(eyeMask, lipMask) * intensity;

            pixels[idx + 0] = static_cast<uint8_t>(255.0f * color.x * mask);
                        pixels[idx + 1] = static_cast<uint8_t>(255.0f * color.y * mask);
                        pixels[idx + 2] = static_cast<uint8_t>(255.0f * color.z * mask);
            pixels[idx + 3] = static_cast<uint8_t>(mask * 255.0f);
        }
    }
}

} // namespace facegen
