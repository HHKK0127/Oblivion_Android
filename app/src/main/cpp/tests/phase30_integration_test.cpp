// Phase 30 Step 13: Integration Test Implementation
// Tests the full NIF pipeline: parse -> skeleton -> skinning -> animation -> collision
// Uses real Oblivion NIF data from assets

#include "phase30_integration_test.h"
#include "../assets/nif_parser.h"
#include "../assets/nif_types.h"
#include "../animation/skeleton.h"
#include "../animation/animation_player.h"
#include "../geometry/skin_partition_packer.h"
#include "../collision/collision_world.h"
#include "../collision/character_controller.h"
#include <chrono>
#include <cstring>
#include <algorithm>
#include <cmath>

#ifdef __ANDROID__
#include <android/log.h>
#include <android/asset_manager.h>
#define LOG_TAG "Phase30Test"
#define TEST_LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define TEST_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#else
#include <cstdio>
#define TEST_LOGI(...) printf(__VA_ARGS__)
#define TEST_LOGE(...) fprintf(stderr, __VA_ARGS__)
#endif

// ============================================
// Test NIF file paths (Oblivion asset paths)
// ============================================

// Humanoid skeleton (most common, has NiSkinInstance + NiControllerManager)
static const char* TEST_NIF_HUMAN = "meshes/characters/_1stperson/skeleton.nif";
static const char* TEST_NIF_HUMAN_MESH = "meshes/characters/_1stperson/male/malebody.nif";

// Architecture (has collision objects)
static const char* TEST_NIF_DOOR = "meshes/architecture/buildingparts/impdoor01.nif";
static const char* TEST_NIF_FURNITURE = "meshes/architecture/furniture/chair01.nif";

// Weapons (small meshes, often have collision)
static const char* TEST_NIF_SWORD = "meshes/weapons/iron/ironsword.nif";
static const char* TEST_NIF_SHIELD = "meshes/weapons/iron/ironshield.nif";

// Creatures (skeleton + animation)
static const char* TEST_NIF_CREATURE = "meshes/creatures/horse/horse.nif";

// ============================================
// Helper: High-resolution timer
// ============================================
static float getTimeMs() {
    static auto start = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<float, std::milli>(now - start).count();
}

// ============================================
// Helper: Matrices approximately equal
// ============================================
static bool matricesApproxEqual(const glm::mat4& a, const glm::mat4& b, float epsilon = 0.001f) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (std::fabs(a[i][j] - b[i][j]) > epsilon) {
                return false;
            }
        }
    }
    return true;
}

// ============================================
// Helper: Dereference shared_ptr<NIFNode> array
// ============================================
static std::vector<NIFNode> derefNodes(const std::vector<std::shared_ptr<NIFNode>>& src) {
    std::vector<NIFNode> out;
    out.reserve(src.size());
    for (const auto& p : src) {
        if (p) out.push_back(*p);
    }
    return out;
}

// ============================================
// Helper: Check if a file can be opened
// ============================================
bool Phase30IntegrationTest::fileExists(const std::string& path) const {
    std::ifstream f(path);
    return f.good();
}

// ============================================
// Constructor / Destructor
// ============================================
Phase30IntegrationTest::Phase30IntegrationTest() {}
Phase30IntegrationTest::~Phase30IntegrationTest() {}

// ============================================
// Record a test result
// ============================================
void Phase30IntegrationTest::record(const std::string& name, bool passed,
                                     const std::string& msg, float ms) {
    TestResult r;
    r.testName = name;
    r.passed = passed;
    r.message = msg;
    r.durationMs = ms;
    results.push_back(r);

    if (passed) {
        TEST_LOGI("[PASS] %s (%.2f ms) %s", name.c_str(), ms, msg.c_str());
    } else {
        TEST_LOGE("[FAIL] %s (%.2f ms) %s", name.c_str(), ms, msg.c_str());
    }
}

// ============================================
// Run all tests
// ============================================
bool Phase30IntegrationTest::runAllTests(const std::string& assetBasePath) {
    basePath = assetBasePath;
    results.clear();

    TEST_LOGI("========================================");
    TEST_LOGI("Phase 30 Integration Test Suite");
    TEST_LOGI("Asset base: %s", basePath.c_str());
    TEST_LOGI("========================================");

    testNIFParsing();
    testSkeletonBuilding();
    testSkinPartitionPacking();
    testAnimationParsing();
    testAnimationPlayback();
    testCollisionParsing();
    testCollisionWorld();
    testCharacterController();
    testFullPipeline();

    TEST_LOGI("========================================");
    TEST_LOGI("Results: %d passed, %d failed, %d total",
              getPassCount(), getFailCount(), (int)results.size());
    TEST_LOGI("========================================");

    return getFailCount() == 0;
}

int Phase30IntegrationTest::getPassCount() const {
    int count = 0;
    for (const auto& r : results) if (r.passed) count++;
    return count;
}

int Phase30IntegrationTest::getFailCount() const {
    int count = 0;
    for (const auto& r : results) if (!r.passed) count++;
    return count;
}

std::string Phase30IntegrationTest::getSummary() const {
    std::string summary;
    summary += "Phase 30 Integration Test Results\n";
    summary += "================================\n";
    for (const auto& r : results) {
        summary += r.passed ? "[PASS] " : "[FAIL] ";
        summary += r.testName;
        if (!r.message.empty()) {
            summary += " - " + r.message;
        }
        summary += "\n";
    }
    char buf[128];
    snprintf(buf, sizeof(buf), "\n%d/%d passed (%d failed)\n",
             getPassCount(), (int)results.size(), getFailCount());
    summary += buf;
    return summary;
}

// ============================================
// Test Group 1: NIF Parsing
// ============================================
void Phase30IntegrationTest::testNIFParsing() {
    TEST_LOGI("--- Test Group: NIF Parsing ---");

    // Test 1.1: Parse a humanoid NIF file
    {
        std::string path = basePath + "/" + TEST_NIF_HUMAN;
        if (!fileExists(path)) {
            // Try alternate path
            path = basePath + "/" + TEST_NIF_HUMAN_MESH;
        }

        float t0 = getTimeMs();
        NIFParser parser;
        bool ok = parser.parseFile(path);
        float dt = getTimeMs() - t0;

        if (!fileExists(path)) {
            record("NIF_Parse_Humanoid", false, "Test file not found: " + path, 0.0f);
        } else {
            record("NIF_Parse_Humanoid", ok,
                   ok ? "nodes=" + std::to_string(parser.getNodes().size()) : "parse failed", dt);

            // Test 1.2: Header validation
            if (ok) {
                const auto& header = parser.getHeader();
                bool headerOk = (header.numObjects > 0);
                record("NIF_Header_Validation", headerOk,
                       "objects=" + std::to_string(header.numObjects), 0.0f);
            }

            // Test 1.3: Node hierarchy
            if (ok) {
                const auto& nodes = parser.getNodes();
                bool hasNodes = !nodes.empty();
                bool hasRoots = !parser.getRootNodeIndices().empty();
                record("NIF_Node_Hierarchy", hasNodes && hasRoots,
                       "nodes=" + std::to_string(nodes.size()) +
                       " roots=" + std::to_string(parser.getRootNodeIndices().size()), 0.0f);
            }
        }
    }

    // Test 1.4: Parse architecture NIF (different structure)
    {
        std::string path = basePath + "/" + TEST_NIF_DOOR;
        if (fileExists(path)) {
            float t0 = getTimeMs();
            NIFParser parser;
            bool ok = parser.parseFile(path);
            float dt = getTimeMs() - t0;
            record("NIF_Parse_Architecture", ok,
                   ok ? "nodes=" + std::to_string(parser.getNodes().size()) : "parse failed", dt);
        } else {
            record("NIF_Parse_Architecture", false, "Test file not found", 0.0f);
        }
    }

    // Test 1.5: Parse weapon NIF
    {
        std::string path = basePath + "/" + TEST_NIF_SWORD;
        if (fileExists(path)) {
            float t0 = getTimeMs();
            NIFParser parser;
            bool ok = parser.parseFile(path);
            float dt = getTimeMs() - t0;
            record("NIF_Parse_Weapon", ok,
                   ok ? "nodes=" + std::to_string(parser.getNodes().size()) : "parse failed", dt);
        } else {
            record("NIF_Parse_Weapon", false, "Test file not found", 0.0f);
        }
    }
}

// ============================================
// Test Group 2: Skeleton Building
// ============================================
void Phase30IntegrationTest::testSkeletonBuilding() {
    TEST_LOGI("--- Test Group: Skeleton Building ---");

    std::string path = basePath + "/" + TEST_NIF_HUMAN;
    if (!fileExists(path)) {
        path = basePath + "/" + TEST_NIF_HUMAN_MESH;
    }
    if (!fileExists(path)) {
        record("Skeleton_Build", false, "No test NIF with skeleton found");
        return;
    }

    NIFParser parser;
    if (!parser.parseFile(path)) {
        record("Skeleton_Build", false, "NIF parse failed");
        return;
    }

    // Try to parse skin instance
    NIFSkinInstance skinInstance;
    NIFSkinData skinData;

    float t0 = getTimeMs();
    bool hasSkin = parser.parseNiSkinInstance(skinInstance);
    float dt = getTimeMs() - t0;

    if (!hasSkin) {
        record("Skeleton_Build", false, "No NiSkinInstance found in test NIF");
        return;
    }

    record("Skeleton_ParseSkinInstance", true, "skinDataIndex=" + std::to_string(skinInstance.skinDataIndex), dt);

    // Parse skin data
    t0 = getTimeMs();
    bool hasSkinData = parser.parseNiSkinData(skinData);
    dt = getTimeMs() - t0;
    record("Skeleton_ParseSkinData", hasSkinData,
           hasSkinData ? "bones=" + std::to_string(skinData.numBones) : "parse failed", dt);

    if (!hasSkinData) return;

    // Build skeleton
    Skeleton skeleton;
    t0 = getTimeMs();
    bool built = skeleton.buildFromNIF(derefNodes(parser.getNodes()), skinInstance, skinData);
    dt = getTimeMs() - t0;
    record("Skeleton_Build", built,
           built ? "bones=" + std::to_string(skeleton.getBoneCount()) : "build failed", dt);

    if (!built) return;

    // Test 2.1: Bone count
    bool boneCountOk = skeleton.getBoneCount() > 0;
    record("Skeleton_BoneCount", boneCountOk,
           "count=" + std::to_string(skeleton.getBoneCount()));

    // Test 2.2: Bone name lookup
    const auto& bones = skeleton.getBones();
    if (!bones.empty()) {
        int idx = skeleton.getBoneIndex(bones[0].name);
        bool lookupOk = (idx == 0);
        record("Skeleton_BoneLookup", lookupOk, "name=" + bones[0].name);
    }

    // Test 2.3: Update (iterative, no stack overflow)
    t0 = getTimeMs();
    skeleton.setBindPose();
    skeleton.update();
    dt = getTimeMs() - t0;
    record("Skeleton_Update", true, "iterative update completed", dt);

    // Test 2.4: Skinning matrices
    const auto& skinMatrices = skeleton.getSkinningMatrices();
    bool matricesOk = (skinMatrices.size() == static_cast<size_t>(skeleton.getBoneCount()));
    record("Skeleton_SkinningMatrices", matricesOk,
           "count=" + std::to_string(skinMatrices.size()));

    // Test 2.5: Identity check at bind pose
    if (!skinMatrices.empty()) {
        bool identityOk = matricesApproxEqual(skinMatrices[0], glm::mat4());
        record("Skeleton_BindPoseIdentity", identityOk,
               identityOk ? "root skinning matrix is identity" : "root skinning matrix is NOT identity");
    }
}

// ============================================
// Test Group 3: Skin Partition Packing
// ============================================
void Phase30IntegrationTest::testSkinPartitionPacking() {
    TEST_LOGI("--- Test Group: Skin Partition Packing ---");

    std::string path = basePath + "/" + TEST_NIF_HUMAN;
    if (!fileExists(path)) {
        path = basePath + "/" + TEST_NIF_HUMAN_MESH;
    }
    if (!fileExists(path)) {
        record("SkinPartition_Pack", false, "No test NIF found");
        return;
    }

    NIFParser parser;
    if (!parser.parseFile(path)) {
        record("SkinPartition_Pack", false, "NIF parse failed");
        return;
    }

    // Try to parse NiSkinPartition
    NIFSkinPartition partition;
    float t0 = getTimeMs();
    bool hasPartition = parser.parseNiSkinPartition(partition);
    float dt = getTimeMs() - t0;

    if (!hasPartition) {
        // If no partition in file, test the packer with synthetic data
        record("SkinPartition_Parse", false, "No NiSkinPartition in file (expected for some NIFs)");

        // Test packer with synthetic skin data
        NIFSkinData syntheticSkin;
        syntheticSkin.numBones = 6;
        syntheticSkin.boneData.resize(6);
        for (int b = 0; b < 6; b++) {
            syntheticSkin.boneData[b].skinTransform = NIFTransform();
        }
        // Create 8 vertices with weights to bones 0-5
        for (int i = 0; i < 8; i++) {
            NIFVertexWeight w0;
            w0.vertexIndex = i;
            w0.weight = 0.6f;
            syntheticSkin.boneData[i % 6].vertexWeights.push_back(w0);
            NIFVertexWeight w1;
            w1.vertexIndex = i;
            w1.weight = 0.4f;
            syntheticSkin.boneData[(i + 1) % 6].vertexWeights.push_back(w1);
        }

        PackOptions opts;
        opts.maxBonesPerVertex = 4;
        opts.maxBonesPerPartition = 4;

        NIFSkinPartition result;
        t0 = getTimeMs();
        bool packOk = SkinPartitionPacker::pack(syntheticSkin, 8, opts, result);
        dt = getTimeMs() - t0;

        record("SkinPartition_Pack_Synthetic", packOk,
               "partitions=" + std::to_string(result.partitions.size()), dt);
        return;
    }

    record("SkinPartition_Parse", true,
           "partitions=" + std::to_string(partition.partitions.size()), dt);

    // Validate partition data
    bool valid = true;
    for (const auto& p : partition.partitions) {
        if (p.numVertices == 0 || p.numTriangles == 0) {
            valid = false;
            break;
        }
    }
    record("SkinPartition_Validate", valid,
           valid ? "all partitions have vertices and triangles" : "empty partition found");

    // Test max bones per partition
    uint32_t maxBones = 0;
    for (const auto& p : partition.partitions) {
        uint32_t count = static_cast<uint32_t>(p.bonePalette.bones.size());
        if (count > maxBones) maxBones = count;
    }
    bool maxBonesOk = (maxBones <= partition.maxBonesPerPartition);
    record("SkinPartition_MaxBones", maxBonesOk,
           "maxBones=" + std::to_string(maxBones) +
           " limit=" + std::to_string(partition.maxBonesPerPartition));
}

// ============================================
// Test Group 4: Animation Parsing
// ============================================
void Phase30IntegrationTest::testAnimationParsing() {
    TEST_LOGI("--- Test Group: Animation Parsing ---");

    // Try multiple NIFs that might have animation data
    std::vector<std::string> candidates = {
        TEST_NIF_HUMAN,
        TEST_NIF_HUMAN_MESH,
        TEST_NIF_CREATURE
    };

    bool found = false;
    for (const auto& candidate : candidates) {
        std::string path = basePath + "/" + candidate;
        if (!fileExists(path)) continue;

        NIFParser parser;
        if (!parser.parseFile(path)) continue;

        // Try to parse controller manager
        NIFControllerManager manager;
        float t0 = getTimeMs();
        bool hasManager = parser.parseNiControllerManager(manager);
        float dt = getTimeMs() - t0;

        if (!hasManager) {
            record("Animation_ParseManager", false, "No NiControllerManager in " + candidate);
            continue;
        }

        found = true;
        record("Animation_ParseManager", true,
               "sequences=" + std::to_string(manager.sequences.size()) +
               " file=" + candidate, dt);

        // Test 4.1: Sequence count
        bool hasSequences = !manager.sequences.empty();
        record("Animation_HasSequences", hasSequences,
               "count=" + std::to_string(manager.sequences.size()));

        // Test 4.2: Parse each sequence
        for (size_t i = 0; i < manager.sequences.size() && i < 5; i++) {
            const auto& seq = manager.sequences[i];
            bool seqOk = !seq.name.empty() && seq.duration > 0.0f;
            record("Animation_Sequence_" + std::to_string(i), seqOk,
                   "name=" + seq.name +
                   " duration=" + std::to_string(seq.duration) +
                   " blocks=" + std::to_string(seq.controlledBlocks.size()));
        }

        // Test 4.3: Text keys
        for (size_t i = 0; i < manager.sequences.size() && i < 3; i++) {
            const auto& seq = manager.sequences[i];
            if (!seq.textKeys.empty()) {
                record("Animation_TextKeys_" + std::to_string(i), true,
                       "count=" + std::to_string(seq.textKeys.size()));
            }
        }

        break;  // Found animation data, stop searching
    }

    if (!found) {
        record("Animation_ParseManager", false, "No animation data found in any test NIF");
    }
}

// ============================================
// Test Group 5: Animation Playback
// ============================================
void Phase30IntegrationTest::testAnimationPlayback() {
    TEST_LOGI("--- Test Group: Animation Playback ---");

    // Build skeleton from NIF
    std::string path = basePath + "/" + TEST_NIF_HUMAN;
    if (!fileExists(path)) {
        path = basePath + "/" + TEST_NIF_HUMAN_MESH;
    }
    if (!fileExists(path)) {
        record("Animation_Playback", false, "No test NIF found");
        return;
    }

    NIFParser parser;
    if (!parser.parseFile(path)) {
        record("Animation_Playback", false, "NIF parse failed");
        return;
    }

    // Build skeleton
    NIFSkinInstance skinInstance;
    NIFSkinData skinData;
    if (!parser.parseNiSkinInstance(skinInstance) || !parser.parseNiSkinData(skinData)) {
        record("Animation_Playback", false, "No skin data for skeleton");
        return;
    }

    Skeleton skeleton;
    if (!skeleton.buildFromNIF(derefNodes(parser.getNodes()), skinInstance, skinData)) {
        record("Animation_Playback", false, "Skeleton build failed");
        return;
    }

    // Parse animation
    NIFControllerManager manager;
    if (!parser.parseNiControllerManager(manager)) {
        record("Animation_Playback", false, "No animation data");
        return;
    }

    // Create animation player
    animation::AnimationPlayer player;
    player.initialize(&skeleton, &manager.sequences);

    // Test 5.1: Play animation
    if (!manager.sequences.empty()) {
        float t0 = getTimeMs();
        player.play(0, true, 1.0f);
        float dt = getTimeMs() - t0;
        record("Animation_Play", true, "sequence=0 loop=true", dt);

        // Test 5.2: Update (advance time)
        t0 = getTimeMs();
        player.update(0.016f);  // ~60fps frame
        dt = getTimeMs() - t0;
        record("Animation_Update", true, "dt=16ms", dt);

        // Test 5.3: Get bone matrices
        std::vector<glm::mat4> boneMatrices;
        t0 = getTimeMs();
        player.getBoneMatrices(boneMatrices);
        dt = getTimeMs() - t0;
        bool matricesOk = (boneMatrices.size() == static_cast<size_t>(skeleton.getBoneCount()));
        record("Animation_GetBoneMatrices", matricesOk,
               "count=" + std::to_string(boneMatrices.size()), dt);

        // Test 5.4: Multiple frames (no crash, no NaN)
        bool noNaN = true;
        for (int frame = 0; frame < 60; frame++) {
            player.update(0.016f);
            player.getBoneMatrices(boneMatrices);
            for (const auto& m : boneMatrices) {
                for (int r = 0; r < 4; r++) {
                    for (int c = 0; c < 4; c++) {
                        if (std::isnan(m[r][c]) || std::isinf(m[r][c])) {
                            noNaN = false;
                            break;
                        }
                    }
                    if (!noNaN) break;
                }
                if (!noNaN) break;
            }
            if (!noNaN) break;
        }
        record("Animation_60Frames_NoNaN", noNaN,
               noNaN ? "60 frames without NaN/Inf" : "NaN or Inf detected in bone matrices");

        // Test 5.5: Stop
        player.stop(0);
        bool stopped = !player.isPlaying(0);
        record("Animation_Stop", stopped, stopped ? "stopped cleanly" : "still playing after stop");

        // Test 5.6: Crossfade (if multiple sequences)
        if (manager.sequences.size() >= 2) {
            player.play(0, true, 1.0f);
            player.update(0.1f);
            player.crossfade(0, 1, 0.5f);
            player.update(0.016f);
            record("Animation_Crossfade", true, "crossfade 0->1 completed");
        }
    }
}

// ============================================
// Test Group 6: Collision Parsing
// ============================================
void Phase30IntegrationTest::testCollisionParsing() {
    TEST_LOGI("--- Test Group: Collision Parsing ---");

    // Try multiple NIFs that might have collision data
    std::vector<std::string> candidates = {
        TEST_NIF_DOOR,
        TEST_NIF_FURNITURE,
        TEST_NIF_SWORD,
        TEST_NIF_SHIELD,
        TEST_NIF_HUMAN
    };

    bool found = false;
    for (const auto& candidate : candidates) {
        std::string path = basePath + "/" + candidate;
        if (!fileExists(path)) continue;

        NIFParser parser;
        if (!parser.parseFile(path)) continue;

        CollisionObject colObj;
        float t0 = getTimeMs();
        bool hasCollision = parser.parseBhkCollisionObject(colObj);
        float dt = getTimeMs() - t0;

        if (!hasCollision) {
            continue;
        }

        found = true;
        record("Collision_ParseObject", true,
               "target=" + colObj.targetName +
               " shapeType=" + std::to_string(static_cast<int>(colObj.shape.type)) +
               " file=" + candidate, dt);

        // Test 6.1: Rigid body info
        bool bodyOk = (colObj.bodyInfo.mass >= 0.0f);
        record("Collision_RigidBody", bodyOk,
               "mass=" + std::to_string(colObj.bodyInfo.mass) +
               " friction=" + std::to_string(colObj.bodyInfo.friction) +
               " restitution=" + std::to_string(colObj.bodyInfo.restitution));

        // Test 6.2: Shape type
        bool shapeOk = (colObj.shape.type != CollisionShapeType::None);
        record("Collision_Shape", shapeOk,
               "type=" + std::to_string(static_cast<int>(colObj.shape.type)));

        // Test 6.3: Shape dimensions
        if (colObj.shape.type == CollisionShapeType::Box) {
            bool dimOk = (colObj.shape.halfExtents.x > 0.0f);
            record("Collision_BoxDimensions", dimOk,
                   "halfExtents=" + std::to_string(colObj.shape.halfExtents.x) + "," +
                   std::to_string(colObj.shape.halfExtents.y) + "," +
                   std::to_string(colObj.shape.halfExtents.z));
        } else if (colObj.shape.type == CollisionShapeType::Sphere) {
            bool radOk = (colObj.shape.radius > 0.0f);
            record("Collision_SphereRadius", radOk,
                   "radius=" + std::to_string(colObj.shape.radius));
        } else if (colObj.shape.type == CollisionShapeType::Capsule) {
            bool capOk = (colObj.shape.radius > 0.0f && colObj.shape.height > 0.0f);
            record("Collision_CapsuleDimensions", capOk,
                   "radius=" + std::to_string(colObj.shape.radius) +
                   " height=" + std::to_string(colObj.shape.height));
        }

        break;  // Found collision data
    }

    if (!found) {
        record("Collision_ParseObject", false, "No collision data found in any test NIF");
    }
}

// ============================================
// Test Group 7: CollisionWorld
// ============================================
void Phase30IntegrationTest::testCollisionWorld() {
    TEST_LOGI("--- Test Group: CollisionWorld ---");

    CollisionWorld world;
    world.setGravity(glm::vec3(0.0f, -9.81f, 0.0f));

    // Test 7.1: Add bodies
    CollisionBody floor;
    floor.shapeType = ShapeType::BOX;
    floor.position = glm::vec3(0.0f, -1.0f, 0.0f);
    floor.halfExtents = glm::vec3(50.0f, 1.0f, 50.0f);
    floor.isStatic = true;
    int32_t floorId = world.addBody(floor);
    record("CollisionWorld_AddFloor", floorId >= 0, "id=" + std::to_string(floorId));

    CollisionBody sphere;
    sphere.shapeType = ShapeType::SPHERE;
    sphere.position = glm::vec3(0.0f, 5.0f, 0.0f);
    sphere.radius = 0.5f;
    sphere.mass = 1.0f;
    sphere.isStatic = false;
    int32_t sphereId = world.addBody(sphere);
    record("CollisionWorld_AddSphere", sphereId >= 0, "id=" + std::to_string(sphereId));

    CollisionBody box;
    box.shapeType = ShapeType::BOX;
    box.position = glm::vec3(2.0f, 0.0f, 0.0f);
    box.halfExtents = glm::vec3(1.0f, 1.0f, 1.0f);
    box.isStatic = true;
    int32_t boxId = world.addBody(box);
    record("CollisionWorld_AddBox", boxId >= 0, "id=" + std::to_string(boxId));

    // Test 7.2: Body count
    record("CollisionWorld_BodyCount", world.getBodyCount() == 3,
           "count=" + std::to_string(world.getBodyCount()));

    // Test 7.3: Step simulation
    float t0 = getTimeMs();
    world.step(1.0f / 60.0f);
    float dt = getTimeMs() - t0;
    record("CollisionWorld_Step", true, "step completed", dt);

    // Test 7.4: AABB query
    std::vector<int32_t> queryResults;
    AABB queryBounds;
    queryBounds.min = glm::vec3(-1.0f, -2.0f, -1.0f);
    queryBounds.max = glm::vec3(1.0f, 2.0f, 1.0f);
    world.queryAABB(queryBounds, queryResults);
    bool queryOk = !queryResults.empty();
    record("CollisionWorld_QueryAABB", queryOk,
           "results=" + std::to_string(queryResults.size()));

    // Test 7.5: Sphere query
    queryResults.clear();
    world.querySphere(glm::vec3(0.0f, 0.0f, 0.0f), 10.0f, queryResults);
    record("CollisionWorld_QuerySphere", !queryResults.empty(),
           "results=" + std::to_string(queryResults.size()));

    // Test 7.6: Remove body
    world.removeBody(sphereId);
    record("CollisionWorld_RemoveBody", world.getBodyCount() == 2,
           "count=" + std::to_string(world.getBodyCount()));

    // Test 7.7: Multiple steps (stability)
    bool stable = true;
    for (int i = 0; i < 100; i++) {
        world.step(1.0f / 60.0f);
        const auto& contacts = world.getContacts();
        // Check for NaN in contacts
        for (size_t c = 0; c < contacts.size(); c++) {
            const auto& cp = contacts.data()[c];
            if (std::isnan(cp.penetration) || std::isinf(cp.penetration)) {
                stable = false;
                break;
            }
        }
        if (!stable) break;
    }
    record("CollisionWorld_100Steps_Stable", stable,
           stable ? "100 steps without NaN" : "NaN detected in contacts");

    // Test 7.8: Dispatch table initialization
    CollisionWorld::initDispatchTable();
    record("CollisionWorld_InitDispatch", true, "dispatch table initialized");
}

// ============================================
// Test Group 8: CharacterController
// ============================================
void Phase30IntegrationTest::testCharacterController() {
    TEST_LOGI("--- Test Group: CharacterController ---");

    // Setup world with floor
    CollisionWorld world;
    world.setGravity(glm::vec3(0.0f, -9.81f, 0.0f));
    CollisionWorld::initDispatchTable();

    CollisionBody floor;
    floor.shapeType = ShapeType::BOX;
    floor.position = glm::vec3(0.0f, -1.0f, 0.0f);
    floor.halfExtents = glm::vec3(50.0f, 1.0f, 50.0f);
    floor.isStatic = true;
    world.addBody(floor);

    // Add some walls
    CollisionBody wall;
    wall.shapeType = ShapeType::BOX;
    wall.position = glm::vec3(5.0f, 0.0f, 0.0f);
    wall.halfExtents = glm::vec3(0.5f, 2.0f, 5.0f);
    wall.isStatic = true;
    world.addBody(wall);

    // Create character controller
    CharacterController controller;
    float t0 = getTimeMs();
    controller.init(&world, glm::vec3(0.0f, 2.0f, 0.0f), 0.4f, 1.8f);
    float dt = getTimeMs() - t0;
    record("CharController_Init", true, "position=(0,2,0) radius=0.4 height=1.8", dt);

    // Test 8.1: Initial position
    glm::vec3 pos = controller.getPosition();
    bool posOk = (std::fabs(pos.y - 2.0f) < 0.1f);
    record("CharController_InitialPosition", posOk,
           "pos=" + std::to_string(pos.x) + "," + std::to_string(pos.y) + "," + std::to_string(pos.z));

    // Test 8.2: Gravity (fall to ground)
    t0 = getTimeMs();
    for (int i = 0; i < 60; i++) {
        controller.update(1.0f / 60.0f);
    }
    dt = getTimeMs() - t0;
    pos = controller.getPosition();
    bool fellToGround = (pos.y < 2.0f);
    record("CharController_Gravity", fellToGround,
           "pos.y=" + std::to_string(pos.y) + " after 60 frames", dt);

    // Test 8.3: Grounded state
    bool grounded = controller.isGrounded();
    record("CharController_Grounded", grounded,
           grounded ? "on ground" : "not grounded");

    // Test 8.4: Horizontal movement
    glm::vec3 beforePos = controller.getPosition();
    controller.move(glm::vec3(1.0f, 0.0f, 0.0f));
    controller.update(1.0f / 60.0f);
    glm::vec3 afterPos = controller.getPosition();
    bool moved = (afterPos.x > beforePos.x);
    record("CharController_Move", moved,
           "dx=" + std::to_string(afterPos.x - beforePos.x));

    // Test 8.5: Wall collision (move into wall)
    controller.setPosition(glm::vec3(3.0f, 0.0f, 0.0f));
    for (int i = 0; i < 30; i++) {
        controller.move(glm::vec3(1.0f, 0.0f, 0.0f));
        controller.update(1.0f / 60.0f);
    }
    pos = controller.getPosition();
    bool wallBlocked = (pos.x < 5.0f);  // Should not pass through wall
    record("CharController_WallCollision", wallBlocked,
           "pos.x=" + std::to_string(pos.x) + " (wall at x=5)");

    // Test 8.6: Ground info
    const GroundInfo& gi = controller.getGroundInfo();
    record("CharController_GroundInfo", true,
           "grounded=" + std::string(gi.isGrounded ? "true" : "false") +
           " slope=" + std::to_string(gi.groundSlope));
}

// ============================================
// Test Group 9: Full Pipeline
// ============================================
void Phase30IntegrationTest::testFullPipeline() {
    TEST_LOGI("--- Test Group: Full Pipeline ---");

    // This test exercises the entire pipeline:
    // NIF Parse -> Skeleton -> Skinning -> Animation -> Collision -> CharacterController

    std::string path = basePath + "/" + TEST_NIF_HUMAN;
    if (!fileExists(path)) {
        path = basePath + "/" + TEST_NIF_HUMAN_MESH;
    }
    if (!fileExists(path)) {
        record("FullPipeline", false, "No test NIF found");
        return;
    }

    float totalStart = getTimeMs();

    // Step 1: Parse NIF
    NIFParser parser;
    if (!parser.parseFile(path)) {
        record("FullPipeline", false, "NIF parse failed");
        return;
    }
    record("FullPipeline_Parse", true, "nodes=" + std::to_string(parser.getNodes().size()));

    // Step 2: Build skeleton
    NIFSkinInstance skinInstance;
    NIFSkinData skinData;
    parser.parseNiSkinInstance(skinInstance);
    parser.parseNiSkinData(skinData);

    Skeleton skeleton;
    bool skelOk = skeleton.buildFromNIF(derefNodes(parser.getNodes()), skinInstance, skinData);
    record("FullPipeline_Skeleton", skelOk,
           skelOk ? "bones=" + std::to_string(skeleton.getBoneCount()) : "failed");

    // Step 3: Parse skin partition
    NIFSkinPartition partition;
    parser.parseNiSkinPartition(partition);
    record("FullPipeline_SkinPartition", true,
           "partitions=" + std::to_string(partition.partitions.size()));

    // Step 4: Parse animation
    NIFControllerManager manager;
    bool hasAnim = parser.parseNiControllerManager(manager);
    record("FullPipeline_Animation", hasAnim,
           hasAnim ? "sequences=" + std::to_string(manager.sequences.size()) : "no animation");

    // Step 5: Setup collision world
    CollisionWorld world;
    CollisionWorld::initDispatchTable();
    world.setGravity(glm::vec3(0.0f, -9.81f, 0.0f));

    CollisionBody floor;
    floor.shapeType = ShapeType::BOX;
    floor.position = glm::vec3(0.0f, -1.0f, 0.0f);
    floor.halfExtents = glm::vec3(100.0f, 1.0f, 100.0f);
    floor.isStatic = true;
    world.addBody(floor);

    // Try to add collision from NIF
    CollisionObject colObj;
    if (parser.parseBhkCollisionObject(colObj)) {
        CollisionBody nifBody;
        nifBody.shapeType = ShapeType::BOX;  // Simplified
        nifBody.position = glm::vec3(colObj.bodyInfo.transform.translation.x,
                                      colObj.bodyInfo.transform.translation.y,
                                      colObj.bodyInfo.transform.translation.z);
        if (colObj.shape.type == CollisionShapeType::Box) {
            nifBody.halfExtents = glm::vec3(colObj.shape.halfExtents.x,
                                             colObj.shape.halfExtents.y,
                                             colObj.shape.halfExtents.z);
        } else if (colObj.shape.type == CollisionShapeType::Sphere) {
            nifBody.shapeType = ShapeType::SPHERE;
            nifBody.radius = colObj.shape.radius;
        }
        nifBody.isStatic = true;
        world.addBody(nifBody);
        record("FullPipeline_NIFCollision", true, "added NIF collision body");
    } else {
        record("FullPipeline_NIFCollision", false, "no collision in NIF (non-fatal)");
    }

    // Step 6: Setup character controller
    CharacterController controller;
    controller.init(&world, glm::vec3(0.0f, 5.0f, 0.0f), 0.4f, 1.8f);

    // Step 7: Setup animation player
    animation::AnimationPlayer animPlayer;
    if (hasAnim && skelOk) {
        animPlayer.initialize(&skeleton, &manager.sequences);
        if (!manager.sequences.empty()) {
            animPlayer.play(0, true, 1.0f);
        }
    }

    // Step 8: Simulate 10 frames of the full pipeline
    bool pipelineOk = true;
    for (int frame = 0; frame < 10; frame++) {
        float dt = 1.0f / 60.0f;

        // Update animation
        if (hasAnim && skelOk) {
            animPlayer.update(dt);
            std::vector<glm::mat4> boneMatrices;
            animPlayer.getBoneMatrices(boneMatrices);

            // Check for NaN
            for (const auto& m : boneMatrices) {
                for (int r = 0; r < 4; r++) {
                    for (int c = 0; c < 4; c++) {
                        if (std::isnan(m[r][c]) || std::isinf(m[r][c])) {
                            pipelineOk = false;
                        }
                    }
                }
            }
        }

        // Update collision
        world.step(dt);

        // Update character
        controller.move(glm::vec3(0.1f, 0.0f, 0.0f));
        controller.update(dt);

        if (!pipelineOk) break;
    }

    float totalDt = getTimeMs() - totalStart;
    record("FullPipeline_10Frames", pipelineOk,
           pipelineOk ? "10 frames completed without errors" : "NaN/Inf detected", totalDt);

    // Final state
    glm::vec3 finalPos = controller.getPosition();
    record("FullPipeline_FinalState", true,
           "playerPos=" + std::to_string(finalPos.x) + "," +
           std::to_string(finalPos.y) + "," + std::to_string(finalPos.z) +
           " bodies=" + std::to_string(world.getBodyCount()) +
           " bones=" + std::to_string(skeleton.getBoneCount()));
}
