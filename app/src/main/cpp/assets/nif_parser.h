#pragma once

#include "nif_types.h"
#include "nif_block_type_map.h"
#include <memory>
#include <map>
#include <fstream>

class NIFParser {
public:
    NIFParser();
    ~NIFParser();

    // Main parsing functions
    bool parseFile(const std::string& filepath);
    // Parse from an in-memory buffer (e.g. resolved through AssetManager).
    // The buffer is copied into the parser, so the caller may release it.
    bool parseFromData(const std::vector<uint8_t>& data);

    // Data access
    const NIFHeader& getHeader() const { return header; }
    const std::vector<std::shared_ptr<NIFNode>>& getNodes() const { return nodes; }
    const std::vector<int32_t>& getRootNodeIndices() const { return rootNodeIndices; }
    
    // Getters for specific data
    std::shared_ptr<NIFNode> getNodeByName(const std::string& name) const;
    std::vector<NIFGeometry> extractAllGeometry() const;

    // --- Phase 30: Block table ---
    // Block bodies carry no size table, so a caller that needs a specific block
    // must walk the blocks in order, starting at getBlockDataOffset(). parseFile()
    // keeps the whole file resident so that walking can happen after it returns.
    uint32_t getBlockCount() const { return static_cast<uint32_t>(header.blockTypeIndices.size()); }
    std::string getBlockTypeName(uint32_t index) const;
    bool hasBlockType(const std::string& typeName) const;
    size_t getBlockDataOffset() const { return header.blockDataOffset; }
    size_t getFileSize() const { return fileBuffer.size(); }
    size_t getCursor() const { return cursor; }
    void setCursor(size_t offset) { cursor = offset; }

    // --- Phase 30: Block body walker (inert: the Rust crate walks the blocks) ---
    bool walkAllBlocks() { return false; }
    bool areBlocksWalked() const { return false; }
    uint32_t getBlockPrefix() const { return 0; }
    uint32_t blockBodyPrefix(const std::string& /*typeName*/) const { return 0; }
    size_t getBlockBodyOffset(uint32_t /*index*/) const { return 0; }
    size_t getBlockBodyEnd(uint32_t /*index*/) const { return 0; }
    bool locateBlockBody(uint32_t /*index*/, size_t& /*offset*/) const { return false; }
    bool findBlocksOfType(const std::string& /*typeName*/, std::vector<uint32_t>& /*out*/) const { return false; }
    const std::string& getWalkError() const { static const std::string kEmpty; return kEmpty; }
    uint32_t getTrailerValueCount() const { return 0; }
    size_t getTrailingDataSize() const { return 0; }

    // --- Phase 30: Skinning ---
    bool parseNiSkinInstance(NIFSkinInstance& skin);
    bool parseNiSkinData(NIFSkinData& skinData);
    bool parseNiSkinPartition(NIFSkinPartition& partition);

    // --- Phase 30: Animation ---
    bool parseNiControllerManager(NIFControllerManager& manager);
    bool parseNiControllerSequence(NIFControllerSequence& sequence);
    bool parseNiKeyframeController(NIFKeyframeController& controller);
    bool parseNiKeyframeData(NIFAnimationClip& clip);
    bool parseNiTextKeyExtraData(std::vector<NIFTextKey>& textKeys);
    bool parseNiTransformController(NIFTransformController& controller);
    bool resolveControllerReferences(NIFControllerManager& manager,
                                     const std::vector<NIFKeyframeData>& keyframeDataArray);

    // --- Phase 30: Physics (Havok) ---
    bool parseBhkCollisionObject(CollisionObject& obj);
    bool parseBhkRigidBody(RigidBodyInfo& info);
    bool parseBhkShape(CollisionShape& shape);
    bool parseBhkBoxShape(CollisionShape& shape);
    bool parseBhkSphereShape(CollisionShape& shape);
    bool parseBhkCapsuleShape(CollisionShape& shape);
    bool parseBhkConvexVerticesShape(CollisionShape& shape);
    bool parseBhkMeshShape(CollisionShape& shape);
    bool parseBhkNiTriStripsShape(CollisionShape& shape);
    bool parseBhkMoppBvTreeShape(CollisionShape& shape);
    bool parseBhkListShape(CollisionShape& shape);
    bool parseBhkCollisionFilter(uint32_t& group, uint32_t& filter);

private:
    // Copy the header, the node table, the root indices and the resident file
    // image out of the Rust parse so the inline getters above can hand out
    // references into stable storage. Called after every parse attempt, because
    // a parse that stopped early still leaves a partial result to inspect.
    bool syncCache();

    // Opaque nif-ffi handle (nif_create() / nif_destroy()).
    void* m_handle;

    // Cached view of the Rust parse, handed out by the inline getters above.
    NIFHeader header;
    std::vector<std::shared_ptr<NIFNode>> nodes;
    std::vector<int32_t> rootNodeIndices;
    std::vector<uint8_t> fileBuffer;   // Whole file, kept resident (getFileSize)
    size_t cursor = 0;                 // Caller-settable read position (get/setCursor)
};
