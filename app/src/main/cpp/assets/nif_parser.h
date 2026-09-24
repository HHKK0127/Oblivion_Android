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

    // Main parsing function
    bool parseFile(const std::string& filepath);

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

    // --- Phase 30: Block body walker ---
    // Retail NIF files store no per-block size table, so the only way to reach a
    // block body is to walk every preceding block in order. walkAllBlocks()
    // decodes each body just far enough to measure it and records the byte range
    // of every block, which lets the readers below seek straight to a block.
    bool walkAllBlocks();
    bool areBlocksWalked() const { return blocksWalked; }
    uint32_t getBlockPrefix() const { return blockPrefix; }
    size_t getBlockBodyOffset(uint32_t index) const;
    size_t getBlockBodyEnd(uint32_t index) const;
    bool locateBlockBody(uint32_t index, size_t& offset) const;
    bool findBlocksOfType(const std::string& typeName, std::vector<uint32_t>& out) const;
    const std::string& getWalkError() const { return walkError; }

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
    bool parseBhkPackedNiTriStripsShape(CollisionShape& shape);
    bool parseBhkMoppBvTreeShape(CollisionShape& shape);
    bool parseBhkCollisionFilter(uint32_t& group, uint32_t& filter);

private:
    // File data
    NIFHeader header;
    std::vector<std::shared_ptr<NIFNode>> nodes;
    std::vector<int32_t> rootNodeIndices;
    std::vector<uint8_t> fileBuffer;   // Whole file, kept resident for block parsing
    size_t cursor = 0;                 // Read position inside fileBuffer
    bool readError = false;            // Set when a read runs past the end of fileBuffer

    // Block walker state (see walkAllBlocks)
    std::vector<size_t> blockBodyOffsets;   // Body start of every block, prefix included
    std::vector<size_t> blockBodyEnds;      // One past the last body byte of every block
    uint32_t blockPrefix = 0;               // Per-block leading uint32 on 10.1.0.x meshes
    bool blocksWalked = false;              // True once walkAllBlocks() has succeeded
    std::string walkError;                  // Human-readable reason the walk stopped

    // Parsing helpers
    bool readHeader();
    bool parseObjectArray();
    bool buildNodeHierarchy();
    static bool isNamedBlockType(const std::string& typeName);

    // Block walker
    bool walkBlockBody(const std::string& typeName);
    bool skipBytes(size_t count);
    void skipFixed(size_t count);
    void skipRefArray(uint32_t count);
    void skipPtrArray(uint32_t count);
    void skipVector3Array(uint32_t count);
    void skipVector4Array(uint32_t count);
    void skipColor4Array(uint32_t count);
    void skipFloatArray(uint32_t count);
    void skipU16Array(uint32_t count);
    void skipStringArray(uint32_t count);
    void skipNiObjectNET();
    void skipNiAVObject();
    void skipNiGeometry();
    void skipNiTimeController();
    void skipNiInterpController();
    void skipNiPSysModifier();
    void skipNiGeometryData(const std::string& typeName);
    void skipMaterialData();
    void skipTexDesc();
    void skipShaderTexDescs(uint32_t count);
    void skipKeyGroup(const char* valueType);
    void skipKeys(uint32_t count, const char* valueType, int arg);
    void skipQuatKeys(uint32_t count, int rotationType);
    void skipKeyframeData();
    void skipBlendInterpolator(bool boolValue);
    void skipInterpBlendItems(uint32_t count);
    void skipNodeSet();
    void skipAVObjectArray(uint32_t count);
    void skipMatchGroups(uint32_t count);
    void skipFurniturePositions(uint32_t count);
    void skipSkinPartition();
    void skipControlledBlock();
    void skipMorph(uint32_t numVertices);
    void skipNiPSysEmitterBase(bool hasEmitterObject);
    static int keyValueSize(const char* valueType);

    // Binary reading helpers
    bool readBytes(char* buffer, size_t count);
    uint8_t readUInt8();
    bool readString(std::string& str);
    bool readByteString(std::string& str);  // bzstring: u8 length (NUL included) + body
    bool readStringRef(std::string& str);  // String reference from string table
    uint32_t readUInt32();
    uint16_t readUInt16();
    float readFloat();
    NIFVector3 readVector3();
    NIFVector4 readVector4();
    NIFMatrix3x3 readMatrix3x3();
    NIFTransform readTransform();
    
    // Block type identification (string-based via NIFBlockTypeMap)
    NIFBlockType getBlockType(const std::string& blockName);
    
    // Node-specific parsers
    bool parseNiNode(std::shared_ptr<NIFNode>& node);
    bool parseNiTriShape(std::shared_ptr<NIFNode>& node);
    bool parseNiTriStrips(std::shared_ptr<NIFNode>& node);
    bool parseMaterialProperty();
    bool parseTexturingProperty();
};
