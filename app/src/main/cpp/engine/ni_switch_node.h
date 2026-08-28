#pragma once

#include <vector>
#include <memory>
#include <string>
#include <functional>
#include <android/log.h>

#define LOG_TAG "NiSwitchNode"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ============================================================================
// NiSwitchNode - Gamebryo switchable LOD/visibility node
// Used for weapon visibility, destructible objects, toggleable elements
// ============================================================================

namespace gamebryo {

// Switch index constants for common uses
namespace SwitchIndices {
    constexpr int INVISIBLE = -1;        // Nothing visible
    constexpr int DEFAULT = 0;           // Default/primary child
    constexpr int DESTROYED = 1;         // Destroyed state
    constexpr int DAMAGED = 2;           // Damaged state
    constexpr int UPGRADE_1 = 3;         // First upgrade level
    constexpr int UPGRADE_2 = 4;         // Second upgrade level
}

// Switch node callback types
using SwitchCallback = std::function<void(int oldIndex, int newIndex)>;
using VisibilityCallback = std::function<void(bool visible)>;

// Child entry in switch node
struct SwitchChild {
    std::string name;
    int index;
    bool enabled = true;
    float fadeInTime = 0.0f;
    float fadeOutTime = 0.0f;
    
    SwitchChild() = default;
    SwitchChild(const std::string& n, int idx) : name(n), index(idx) {}
};

class NiSwitchNode {
public:
    NiSwitchNode();
    ~NiSwitchNode();
    
    // Initialization
    bool initialize(const std::string& name);
    void shutdown();
    
    // Child management
    void addChild(const SwitchChild& child);
    void removeChild(int index);
    void clearChildren();
    size_t getChildCount() const { return children_.size(); }
    
    // Switch control
    void setIndex(int index);
    int getIndex() const { return currentIndex_; }
    int getPreviousIndex() const { return previousIndex_; }
    
    // Quick visibility toggle
    void setVisible(bool visible);
    bool isVisible() const { return currentIndex_ != SwitchIndices::INVISIBLE; }
    
    // Get current active child name
    std::string getActiveChildName() const;
    
    // Child queries
    bool hasChild(int index) const;
    bool hasChild(const std::string& name) const;
    SwitchChild* getChild(int index);
    SwitchChild* getChild(const std::string& name);
    
    // Enable/disable specific children
    void setChildEnabled(int index, bool enabled);
    bool isChildEnabled(int index) const;
    
    // Get all enabled indices
    std::vector<int> getEnabledIndices() const;
    
    // Callbacks
    void setSwitchCallback(SwitchCallback callback) { switchCallback_ = callback; }
    void setVisibilityCallback(VisibilityCallback callback) { visibilityCallback_ = callback; }
    
    // Update (for fade transitions)
    void update(float deltaTime);
    
    // Fade state
    float getFadeAlpha() const { return fadeAlpha_; }
    bool isFading() const { return isFading_; }
    
    // Name
    const std::string& getName() const { return name_; }
    void setName(const std::string& name) { name_ = name; }
    
    // Utility: Create weapon visibility switch
    static std::shared_ptr<NiSwitchNode> createWeaponSwitch(const std::string& name);
    
    // Utility: Create destructible object switch
    static std::shared_ptr<NiSwitchNode> createDestructibleSwitch(const std::string& name);
    
    // Utility: Create upgrade level switch
    static std::shared_ptr<NiSwitchNode> createUpgradeSwitch(const std::string& name, int maxLevel);

private:
    std::string name_;
    std::vector<SwitchChild> children_;
    int currentIndex_ = SwitchIndices::INVISIBLE;
    int previousIndex_ = SwitchIndices::INVISIBLE;
    
    // Fade transition
    float fadeAlpha_ = 1.0f;
    bool isFading_ = false;
    float fadeTimer_ = 0.0f;
    float fadeDuration_ = 0.0f;
    bool fadingIn_ = true;
    
    // Callbacks
    SwitchCallback switchCallback_;
    VisibilityCallback visibilityCallback_;
    
    void startFade(bool fadeIn, float duration);
};

// NiLODNode - Level of Detail switch node
// Automatically switches based on distance to camera
class NiLODNode {
public:
    struct LODLevel {
        std::string name;
        float minDistance;
        float maxDistance;
        float fadeStart;  // Start fade to next level
        float fadeEnd;    // End fade (fully next level)
        
        LODLevel() = default;
        LODLevel(const std::string& n, float minDist, float maxDist)
            : name(n), minDistance(minDist), maxDistance(maxDist),
              fadeStart(maxDist * 0.8f), fadeEnd(maxDist) {}
    };
    
    NiLODNode();
    ~NiLODNode();
    
    bool initialize(const std::string& name);
    void shutdown();
    
    // LOD level management
    void addLODLevel(const LODLevel& level);
    void removeLODLevel(int index);
    void clearLODLevels();
    size_t getLODLevelCount() const { return lodLevels_.size(); }
    
    // Distance-based LOD update
    void updateLOD(const glm::vec3& objectPosition, const glm::vec3& cameraPosition);
    
    // Get current LOD info
    int getCurrentLODIndex() const { return currentLOD_; }
    const LODLevel* getCurrentLODLevel() const;
    std::string getCurrentLODName() const;
    float getLODFade() const { return lodFade_; }
    
    // Force specific LOD (for debugging)
    void forceLOD(int index);
    void clearForce() { forcedLOD_ = -1; }
    bool isLODForced() const { return forcedLOD_ >= 0; }
    
    // LOD ranges
    void setLODRangeBias(float bias) { lodRangeBias_ = bias; }
    float getLODRangeBias() const { return lodRangeBias_; }
    
    // Callback when LOD changes
    using LODCallback = std::function<void(int oldLOD, int newLOD, float fade)>;
    void setLODCallback(LODCallback callback) { lodCallback_ = callback; }
    
    // Statistics
    int getLODSwitchCount() const { return lodSwitchCount_; }
    void resetStats() { lodSwitchCount_ = 0; }

private:
    std::string name_;
    std::vector<LODLevel> lodLevels_;
    int currentLOD_ = 0;
    int previousLOD_ = -1;
    int forcedLOD_ = -1;
    float lodFade_ = 1.0f;
    float lodRangeBias_ = 1.0f;
    int lodSwitchCount_ = 0;
    
    LODCallback lodCallback_;
};

// ============================================================================
// Implementation
// ============================================================================

inline NiSwitchNode::NiSwitchNode() = default;
inline NiSwitchNode::~NiSwitchNode() { shutdown(); }

inline bool NiSwitchNode::initialize(const std::string& name) {
    name_ = name;
    currentIndex_ = SwitchIndices::INVISIBLE;
    previousIndex_ = SwitchIndices::INVISIBLE;
    fadeAlpha_ = 1.0f;
    isFading_ = false;
    LOGI("NiSwitchNode '%s' initialized", name.c_str());
    return true;
}

inline void NiSwitchNode::shutdown() {
    children_.clear();
    switchCallback_ = nullptr;
    visibilityCallback_ = nullptr;
}

inline void NiSwitchNode::addChild(const SwitchChild& child) {
    children_.push_back(child);
    LOGD("Added child '%s' at index %d", child.name.c_str(), child.index);
}

inline void NiSwitchNode::removeChild(int index) {
    children_.erase(
        std::remove_if(children_.begin(), children_.end(),
            [index](const SwitchChild& child) { return child.index == index; }),
        children_.end());
}

inline void NiSwitchNode::clearChildren() {
    children_.clear();
    currentIndex_ = SwitchIndices::INVISIBLE;
    previousIndex_ = SwitchIndices::INVISIBLE;
}

inline void NiSwitchNode::setIndex(int index) {
    if (index == currentIndex_) return;
    
    previousIndex_ = currentIndex_;
    currentIndex_ = index;
    
    // Check if new index is valid and enabled
    if (index != SwitchIndices::INVISIBLE) {
        SwitchChild* child = getChild(index);
        if (!child || !child->enabled) {
            LOGE("Attempted to switch to invalid or disabled index %d", index);
            currentIndex_ = previousIndex_;
            return;
        }
        
        // Start fade if configured
        if (child->fadeInTime > 0.0f) {
            startFade(true, child->fadeInTime);
        }
    } else {
        // Fading out
        SwitchChild* prevChild = getChild(previousIndex_);
        if (prevChild && prevChild->fadeOutTime > 0.0f) {
            startFade(false, prevChild->fadeOutTime);
        }
    }
    
    // Invoke callback
    if (switchCallback_) {
        switchCallback_(previousIndex_, currentIndex_);
    }
    
    LOGD("Switched from %d to %d", previousIndex_, currentIndex_);
}

inline void NiSwitchNode::setVisible(bool visible) {
    if (visible && currentIndex_ == SwitchIndices::INVISIBLE) {
        setIndex(SwitchIndices::DEFAULT);
        if (visibilityCallback_) visibilityCallback_(true);
    } else if (!visible && currentIndex_ != SwitchIndices::INVISIBLE) {
        setIndex(SwitchIndices::INVISIBLE);
        if (visibilityCallback_) visibilityCallback_(false);
    }
}

inline std::string NiSwitchNode::getActiveChildName() const {
    if (currentIndex_ == SwitchIndices::INVISIBLE) return "";
    
    for (const auto& child : children_) {
        if (child.index == currentIndex_) {
            return child.name;
        }
    }
    return "";
}

inline bool NiSwitchNode::hasChild(int index) const {
    for (const auto& child : children_) {
        if (child.index == index) return true;
    }
    return false;
}

inline bool NiSwitchNode::hasChild(const std::string& name) const {
    for (const auto& child : children_) {
        if (child.name == name) return true;
    }
    return false;
}

inline SwitchChild* NiSwitchNode::getChild(int index) {
    for (auto& child : children_) {
        if (child.index == index) return &child;
    }
    return nullptr;
}

inline SwitchChild* NiSwitchNode::getChild(const std::string& name) {
    for (auto& child : children_) {
        if (child.name == name) return &child;
    }
    return nullptr;
}

inline void NiSwitchNode::setChildEnabled(int index, bool enabled) {
    SwitchChild* child = getChild(index);
    if (child) {
        child->enabled = enabled;
        LOGD("Child %d (%s) %s", index, child->name.c_str(), enabled ? "enabled" : "disabled");
    }
}

inline bool NiSwitchNode::isChildEnabled(int index) const {
    for (const auto& child : children_) {
        if (child.index == index) return child.enabled;
    }
    return false;
}

inline std::vector<int> NiSwitchNode::getEnabledIndices() const {
    std::vector<int> result;
    for (const auto& child : children_) {
        if (child.enabled) {
            result.push_back(child.index);
        }
    }
    return result;
}

inline void NiSwitchNode::update(float deltaTime) {
    if (!isFading_) return;
    
    fadeTimer_ += deltaTime;
    float t = fadeTimer_ / fadeDuration_;
    if (t >= 1.0f) {
        t = 1.0f;
        isFading_ = false;
    }
    
    fadeAlpha_ = fadingIn_ ? t : (1.0f - t);
}

inline void NiSwitchNode::startFade(bool fadeIn, float duration) {
    isFading_ = true;
    fadingIn_ = fadeIn;
    fadeTimer_ = 0.0f;
    fadeDuration_ = duration;
    fadeAlpha_ = fadeIn ? 0.0f : 1.0f;
}

inline std::shared_ptr<NiSwitchNode> NiSwitchNode::createWeaponSwitch(const std::string& name) {
    auto node = std::make_shared<NiSwitchNode>();
    node->initialize(name);
    node->addChild(SwitchChild("visible", SwitchIndices::DEFAULT));
    node->addChild(SwitchChild("hidden", SwitchIndices::INVISIBLE));
    return node;
}

inline std::shared_ptr<NiSwitchNode> NiSwitchNode::createDestructibleSwitch(const std::string& name) {
    auto node = std::make_shared<NiSwitchNode>();
    node->initialize(name);
    node->addChild(SwitchChild("intact", SwitchIndices::DEFAULT));
    node->addChild(SwitchChild("destroyed", SwitchIndices::DESTROYED));
    return node;
}

inline std::shared_ptr<NiSwitchNode> NiSwitchNode::createUpgradeSwitch(const std::string& name, int maxLevel) {
    auto node = std::make_shared<NiSwitchNode>();
    node->initialize(name);
    for (int i = 0; i <= maxLevel; ++i) {
        node->addChild(SwitchChild("level_" + std::to_string(i), i));
    }
    return node;
}

// NiLODNode implementation
inline NiLODNode::NiLODNode() = default;
inline NiLODNode::~NiLODNode() { shutdown(); }

inline bool NiLODNode::initialize(const std::string& name) {
    name_ = name;
    currentLOD_ = 0;
    previousLOD_ = -1;
    lodFade_ = 1.0f;
    forcedLOD_ = -1;
    lodSwitchCount_ = 0;
    LOGI("NiLODNode '%s' initialized", name.c_str());
    return true;
}

inline void NiLODNode::shutdown() {
    lodLevels_.clear();
}

inline void NiLODNode::addLODLevel(const LODLevel& level) {
    lodLevels_.push_back(level);
    LOGD("Added LOD level '%s' (%.1f - %.1f)", 
         level.name.c_str(), level.minDistance, level.maxDistance);
}

inline void NiLODNode::removeLODLevel(int index) {
    if (index >= 0 && index < static_cast<int>(lodLevels_.size())) {
        lodLevels_.erase(lodLevels_.begin() + index);
    }
}

inline void NiLODNode::clearLODLevels() {
    lodLevels_.clear();
    currentLOD_ = 0;
}

inline void NiLODNode::updateLOD(const glm::vec3& objectPosition, 
                                  const glm::vec3& cameraPosition) {
    if (lodLevels_.empty()) return;
    
    if (forcedLOD_ >= 0) {
        if (forcedLOD_ != currentLOD_) {
            previousLOD_ = currentLOD_;
            currentLOD_ = forcedLOD_;
            lodSwitchCount_++;
            if (lodCallback_) lodCallback_(previousLOD_, currentLOD_, 1.0f);
        }
        return;
    }
    
    float distance = glm::length(objectPosition - cameraPosition) * lodRangeBias_;
    int newLOD = currentLOD_;
    
    // Find appropriate LOD based on distance
    for (int i = 0; i < static_cast<int>(lodLevels_.size()); ++i) {
        const auto& level = lodLevels_[i];
        if (distance >= level.minDistance && distance < level.maxDistance) {
            newLOD = i;
            
            // Calculate fade if within fade range
            if (distance >= level.fadeStart && distance < level.fadeEnd) {
                lodFade_ = 1.0f - (distance - level.fadeStart) / (level.fadeEnd - level.fadeStart);
            } else {
                lodFade_ = 1.0f;
            }
            break;
        }
    }
    
    // Clamp to valid range
    if (newLOD < 0) newLOD = 0;
    if (newLOD >= static_cast<int>(lodLevels_.size())) {
        newLOD = static_cast<int>(lodLevels_.size()) - 1;
    }
    
    // Check for LOD change
    if (newLOD != currentLOD_) {
        previousLOD_ = currentLOD_;
        currentLOD_ = newLOD;
        lodSwitchCount_++;
        
        LOGD("LOD switch: %d -> %d (distance: %.1f)", previousLOD_, currentLOD_, distance);
        
        if (lodCallback_) {
            lodCallback_(previousLOD_, currentLOD_, lodFade_);
        }
    }
}

inline const NiLODNode::LODLevel* NiLODNode::getCurrentLODLevel() const {
    if (currentLOD_ >= 0 && currentLOD_ < static_cast<int>(lodLevels_.size())) {
        return &lodLevels_[currentLOD_];
    }
    return nullptr;
}

inline std::string NiLODNode::getCurrentLODName() const {
    const auto* level = getCurrentLODLevel();
    return level ? level->name : "";
}

inline void NiLODNode::forceLOD(int index) {
    if (index >= 0 && index < static_cast<int>(lodLevels_.size())) {
        forcedLOD_ = index;
        LOGI("LOD forced to level %d ('%s')", index, lodLevels_[index].name.c_str());
    }
}

} // namespace gamebryo
