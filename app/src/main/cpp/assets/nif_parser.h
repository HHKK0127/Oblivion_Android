#pragma once

#include "nif_types.h"
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

private:
    // File data
    NIFHeader header;
    std::vector<std::shared_ptr<NIFNode>> nodes;
    std::vector<int32_t> rootNodeIndices;
    std::ifstream fileStream;
    
    // Parsing helpers
    bool readHeader();
    bool parseBlockTypeStrings();
    bool parseObjectArray();
    bool buildNodeHierarchy();
    
    // Binary reading helpers
    bool readBytes(char* buffer, size_t count);
    bool readString(std::string& str);
    bool readStringRef(std::string& str);  // String reference from string table
    uint32_t readUInt32();
    uint16_t readUInt16();
    float readFloat();
    NIFVector3 readVector3();
    NIFVector4 readVector4();
    NIFMatrix3x3 readMatrix3x3();
    NIFTransform readTransform();
    
    // Block type identification
    NIFBlockType getBlockType(const std::string& blockName);
    
    // Node-specific parsers
    bool parseNiNode(std::shared_ptr<NIFNode>& node);
    bool parseNiTriShape(std::shared_ptr<NIFNode>& node);
    bool parseNiTriStrips(std::shared_ptr<NIFNode>& node);
    bool parseMaterialProperty();
    bool parseTexturingProperty();
};
