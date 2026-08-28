#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <android/log.h>

#define LOG_TAG "BSXFlags"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// ============================================================================
// BSXFlags - Gamebryo object flags implementation
// Controls visibility, collision, shadow casting, and other object properties
// ============================================================================

namespace gamebryo {

// BSXFlags bit definitions (from Oblivion NIF format)
enum class BSXFlag : uint32_t {
    // Basic visibility
    VISIBLE = 0x00000001,           // Object is visible
    
    // Collision flags
    HAS_COLLISION = 0x00000002,     // Object has collision geometry
    COLLISION_ENABLED = 0x00000004, // Collision is currently enabled
    
    // Shadow flags
    CAST_SHADOW = 0x00000008,       // Object casts shadows
    RECEIVE_SHADOW = 0x00000010,    // Object receives shadows
    
    // Rendering flags
    ALPHA_BLEND = 0x00000020,       // Use alpha blending
    ALPHA_TEST = 0x00000040,        // Use alpha testing
    
    // Animation flags
    ANIMATED = 0x00000080,          // Object has animation data
    ANIMATION_ENABLED = 0x00000100, // Animation is currently playing
    
    // LOD flags
    USE_LOD = 0x00000200,           // Object uses LOD switching
    NEVER_FADE = 0x00000400,        // Object never uses distance fade
    
    // Special flags
    PICKABLE = 0x00000800,          // Object can be picked/activated
    HAS_HAVOK = 0x00001000,         // Object has Havok physics data
    IS_MARKER = 0x00002000,         // Object is a marker/helper
    
    // Shader flags
    USE_SPECULAR = 0x00004000,      // Use specular lighting
    USE_REFLECTION = 0x00008000,    // Use environment reflection
    
    // Instance flags (runtime)
    SELECTED = 0x80000000,          // Object is currently selected (editor)
    HIGHLIGHTED = 0x40000000,       // Object is highlighted (editor)
    
    // Common combinations
    DEFAULT_STATIC = VISIBLE | HAS_COLLISION | CAST_SHADOW | RECEIVE_SHADOW,
    DEFAULT_DYNAMIC = VISIBLE | HAS_COLLISION | CAST_SHADOW | RECEIVE_SHADOW | ANIMATED,
    INVISIBLE_COLLIDER = HAS_COLLISION,  // Invisible wall/trigger
    DECAL = VISIBLE | RECEIVE_SHADOW | ALPHA_BLEND | ALPHA_TEST,
    PARTICLE_EMITTER = VISIBLE | ANIMATED | ANIMATION_ENABLED | USE_LOD
};

// BSXFlags container class
class BSXFlags {
public:
    BSXFlags() = default;
    explicit BSXFlags(uint32_t flags) : flags_(flags) {}
    
    // Flag manipulation
    void set(BSXFlag flag) { flags_ |= static_cast<uint32_t>(flag); }
    void clear(BSXFlag flag) { flags_ &= ~static_cast<uint32_t>(flag); }
    void toggle(BSXFlag flag) { flags_ ^= static_cast<uint32_t>(flag); }
    bool has(BSXFlag flag) const { return (flags_ & static_cast<uint32_t>(flag)) != 0; }
    
    // Bulk operations
    void setFlags(uint32_t flags) { flags_ = flags; }
    uint32_t getFlags() const { return flags_; }
    void clearAll() { flags_ = 0; }
    
    // Common property checks
    bool isVisible() const { return has(BSXFlag::VISIBLE); }
    bool hasCollision() const { return has(BSXFlag::HAS_COLLISION); }
    bool isCollisionEnabled() const { return has(BSXFlag::COLLISION_ENABLED); }
    bool castsShadow() const { return has(BSXFlag::CAST_SHADOW); }
    bool receivesShadow() const { return has(BSXFlag::RECEIVE_SHADOW); }
    bool usesAlphaBlend() const { return has(BSXFlag::ALPHA_BLEND); }
    bool usesAlphaTest() const { return has(BSXFlag::ALPHA_TEST); }
    bool isAnimated() const { return has(BSXFlag::ANIMATED); }
    bool isAnimationEnabled() const { return has(BSXFlag::ANIMATION_ENABLED); }
    bool usesLOD() const { return has(BSXFlag::USE_LOD); }
    bool neverFades() const { return has(BSXFlag::NEVER_FADE); }
    bool isPickable() const { return has(BSXFlag::PICKABLE); }
    bool hasHavok() const { return has(BSXFlag::HAS_HAVOK); }
    bool isMarker() const { return has(BSXFlag::IS_MARKER); }
    bool usesSpecular() const { return has(BSXFlag::USE_SPECULAR); }
    bool usesReflection() const { return has(BSXFlag::USE_REFLECTION); }
    
    // Setters for common properties
    void setVisible(bool visible) { 
        if (visible) set(BSXFlag::VISIBLE); 
        else clear(BSXFlag::VISIBLE); 
    }
    void setCollisionEnabled(bool enabled) {
        if (enabled) set(BSXFlag::COLLISION_ENABLED);
        else clear(BSXFlag::COLLISION_ENABLED);
    }
    void setCastShadow(bool cast) {
        if (cast) set(BSXFlag::CAST_SHADOW);
        else clear(BSXFlag::CAST_SHADOW);
    }
    void setReceiveShadow(bool receive) {
        if (receive) set(BSXFlag::RECEIVE_SHADOW);
        else clear(BSXFlag::RECEIVE_SHADOW);
    }
    void setAnimationEnabled(bool enabled) {
        if (enabled) set(BSXFlag::ANIMATION_ENABLED);
        else clear(BSXFlag::ANIMATION_ENABLED);
    }
    
    // String representation for debugging
    std::string toString() const;
    
    // Parse from string (for configuration files)
    static BSXFlags fromString(const std::string& str);
    
    // Comparison
    bool operator==(const BSXFlags& other) const { return flags_ == other.flags_; }
    bool operator!=(const BSXFlags& other) const { return flags_ != other.flags_; }
    
private:
    uint32_t flags_ = 0;
};

// BSXFlagNode - Attach flags to scene graph nodes
struct BSXFlagNode {
    std::string nodeName;
    BSXFlags flags;
    uint32_t nodeId = 0;
    
    BSXFlagNode() = default;
    BSXFlagNode(const std::string& name, uint32_t flagValue, uint32_t id = 0)
        : nodeName(name), flags(flagValue), nodeId(id) {}
};

// BSXFlagManager - Global flag management for scene
class BSXFlagManager {
public:
    static BSXFlagManager& getInstance();
    
    // Flag registration
    void registerNode(const std::string& nodeName, const BSXFlags& flags, uint32_t nodeId = 0);
    void unregisterNode(const std::string& nodeName);
    
    // Flag queries
    BSXFlags getNodeFlags(const std::string& nodeName) const;
    bool nodeHasFlag(const std::string& nodeName, BSXFlag flag) const;
    
    // Flag modification
    void setNodeFlags(const std::string& nodeName, const BSXFlags& flags);
    void setNodeFlag(const std::string& nodeName, BSXFlag flag);
    void clearNodeFlag(const std::string& nodeName, BSXFlag flag);
    
    // Bulk operations
    std::vector<std::string> getNodesWithFlag(BSXFlag flag) const;
    std::vector<std::string> getVisibleNodes() const;
    std::vector<std::string> getCollidableNodes() const;
    std::vector<std::string> getShadowCasters() const;
    std::vector<std::string> getAnimatedNodes() const;
    
    // Filter nodes by multiple criteria
    std::vector<std::string> filterNodes(uint32_t requiredFlags, uint32_t excludedFlags = 0) const;
    
    // Clear all registrations
    void clear();
    
    // Statistics
    size_t getNodeCount() const { return nodes_.size(); }

private:
    BSXFlagManager() = default;
    ~BSXFlagManager() = default;
    BSXFlagManager(const BSXFlagManager&) = delete;
    BSXFlagManager& operator=(const BSXFlagManager&) = delete;
    
    std::unordered_map<std::string, BSXFlagNode> nodes_;
};

// ============================================================================
// Implementation
// ============================================================================

inline std::string BSXFlags::toString() const {
    std::string result = "[";
    bool first = true;
    
    auto addFlag = [&result, &first](const char* name) {
        if (!first) result += "|";
        result += name;
        first = false;
    };
    
    if (has(BSXFlag::VISIBLE)) addFlag("VISIBLE");
    if (has(BSXFlag::HAS_COLLISION)) addFlag("COLLISION");
    if (has(BSXFlag::COLLISION_ENABLED)) addFlag("COLLISION_ON");
    if (has(BSXFlag::CAST_SHADOW)) addFlag("SHADOW");
    if (has(BSXFlag::RECEIVE_SHADOW)) addFlag("RECV_SHADOW");
    if (has(BSXFlag::ALPHA_BLEND)) addFlag("BLEND");
    if (has(BSXFlag::ALPHA_TEST)) addFlag("TEST");
    if (has(BSXFlag::ANIMATED)) addFlag("ANIM");
    if (has(BSXFlag::ANIMATION_ENABLED)) addFlag("ANIM_ON");
    if (has(BSXFlag::USE_LOD)) addFlag("LOD");
    if (has(BSXFlag::NEVER_FADE)) addFlag("NOFADE");
    if (has(BSXFlag::PICKABLE)) addFlag("PICK");
    if (has(BSXFlag::HAS_HAVOK)) addFlag("HAVOK");
    if (has(BSXFlag::IS_MARKER)) addFlag("MARKER");
    if (has(BSXFlag::USE_SPECULAR)) addFlag("SPEC");
    if (has(BSXFlag::USE_REFLECTION)) addFlag("REFL");
    
    result += "]";
    return result;
}

inline BSXFlags BSXFlags::fromString(const std::string& str) {
    BSXFlags flags;
    // Simple parsing: look for flag names separated by | or space
    if (str.find("VISIBLE") != std::string::npos) flags.set(BSXFlag::VISIBLE);
    if (str.find("COLLISION") != std::string::npos) flags.set(BSXFlag::HAS_COLLISION);
    if (str.find("SHADOW") != std::string::npos) {
        flags.set(BSXFlag::CAST_SHADOW);
        flags.set(BSXFlag::RECEIVE_SHADOW);
    }
    if (str.find("BLEND") != std::string::npos) flags.set(BSXFlag::ALPHA_BLEND);
    if (str.find("TEST") != std::string::npos) flags.set(BSXFlag::ALPHA_TEST);
    if (str.find("ANIM") != std::string::npos) flags.set(BSXFlag::ANIMATED);
    if (str.find("LOD") != std::string::npos) flags.set(BSXFlag::USE_LOD);
    if (str.find("PICK") != std::string::npos) flags.set(BSXFlag::PICKABLE);
    if (str.find("HAVOK") != std::string::npos) flags.set(BSXFlag::HAS_HAVOK);
    if (str.find("SPEC") != std::string::npos) flags.set(BSXFlag::USE_SPECULAR);
    if (str.find("REFL") != std::string::npos) flags.set(BSXFlag::USE_REFLECTION);
    return flags;
}

// BSXFlagManager implementation
inline BSXFlagManager& BSXFlagManager::getInstance() {
    static BSXFlagManager instance;
    return instance;
}

inline void BSXFlagManager::registerNode(const std::string& nodeName, 
                                          const BSXFlags& flags, 
                                          uint32_t nodeId) {
    nodes_[nodeName] = BSXFlagNode(nodeName, flags.getFlags(), nodeId);
    LOGD("Registered node '%s' with flags %s", nodeName.c_str(), flags.toString().c_str());
}

inline void BSXFlagManager::unregisterNode(const std::string& nodeName) {
    nodes_.erase(nodeName);
}

inline BSXFlags BSXFlagManager::getNodeFlags(const std::string& nodeName) const {
    auto it = nodes_.find(nodeName);
    if (it != nodes_.end()) {
        return it->second.flags;
    }
    return BSXFlags();
}

inline bool BSXFlagManager::nodeHasFlag(const std::string& nodeName, BSXFlag flag) const {
    auto it = nodes_.find(nodeName);
    if (it != nodes_.end()) {
        return it->second.flags.has(flag);
    }
    return false;
}

inline void BSXFlagManager::setNodeFlags(const std::string& nodeName, const BSXFlags& flags) {
    auto it = nodes_.find(nodeName);
    if (it != nodes_.end()) {
        it->second.flags = flags;
    }
}

inline void BSXFlagManager::setNodeFlag(const std::string& nodeName, BSXFlag flag) {
    auto it = nodes_.find(nodeName);
    if (it != nodes_.end()) {
        it->second.flags.set(flag);
    }
}

inline void BSXFlagManager::clearNodeFlag(const std::string& nodeName, BSXFlag flag) {
    auto it = nodes_.find(nodeName);
    if (it != nodes_.end()) {
        it->second.flags.clear(flag);
    }
}

inline std::vector<std::string> BSXFlagManager::getNodesWithFlag(BSXFlag flag) const {
    std::vector<std::string> result;
    for (const auto& [name, node] : nodes_) {
        if (node.flags.has(flag)) {
            result.push_back(name);
        }
    }
    return result;
}

inline std::vector<std::string> BSXFlagManager::getVisibleNodes() const {
    return getNodesWithFlag(BSXFlag::VISIBLE);
}

inline std::vector<std::string> BSXFlagManager::getCollidableNodes() const {
    std::vector<std::string> result;
    for (const auto& [name, node] : nodes_) {
        if (node.flags.has(BSXFlag::HAS_COLLISION) && 
            node.flags.has(BSXFlag::COLLISION_ENABLED)) {
            result.push_back(name);
        }
    }
    return result;
}

inline std::vector<std::string> BSXFlagManager::getShadowCasters() const {
    return getNodesWithFlag(BSXFlag::CAST_SHADOW);
}

inline std::vector<std::string> BSXFlagManager::getAnimatedNodes() const {
    std::vector<std::string> result;
    for (const auto& [name, node] : nodes_) {
        if (node.flags.has(BSXFlag::ANIMATED) && 
            node.flags.has(BSXFlag::ANIMATION_ENABLED)) {
            result.push_back(name);
        }
    }
    return result;
}

inline std::vector<std::string> BSXFlagManager::filterNodes(uint32_t requiredFlags, 
                                                             uint32_t excludedFlags) const {
    std::vector<std::string> result;
    for (const auto& [name, node] : nodes_) {
        uint32_t flags = node.flags.getFlags();
        if ((flags & requiredFlags) == requiredFlags && 
            (flags & excludedFlags) == 0) {
            result.push_back(name);
        }
    }
    return result;
}

inline void BSXFlagManager::clear() {
    nodes_.clear();
}

} // namespace gamebryo
