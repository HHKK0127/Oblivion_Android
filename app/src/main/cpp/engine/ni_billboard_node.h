#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <memory>
#include <string>
#include <android/log.h>

#define LOG_TAG "NiBillboardNode"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// ============================================================================
// NiBillboardNode - Camera-facing billboard implementation
// Gamebryo engine component for Oblivion vegetation, particles, and effects
// ============================================================================

namespace gamebryo {

// Billboard alignment modes (matching Gamebryo)
enum class BillboardMode : uint32_t {
    ALWAYS_FACE_CAMERA = 0,     // Always face camera (full rotation)
    ROTATE_ABOUT_UP = 1,        // Rotate only around world up axis
    RIGID_FACE_CAMERA = 2,      // Face camera but maintain up direction
    ALWAYS_FACE_CENTER = 3,     // Face camera center point
    RIGID_FACE_CENTER = 4       // Rigid face center with up constraint
};

// Billboard node configuration
struct BillboardConfig {
    BillboardMode mode = BillboardMode::ALWAYS_FACE_CAMERA;
    glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    float scale = 1.0f;
    bool useViewOffset = false;
    glm::vec2 viewOffset = glm::vec2(0.0f, 0.0f);
    
    // Distance fade for LOD transitions
    float fadeStartDistance = 50.0f;
    float fadeEndDistance = 100.0f;
    bool useDistanceFade = false;
};

// Individual billboard instance
struct BillboardInstance {
    glm::vec3 position;
    glm::vec2 size;
    float rotation = 0.0f;  // Optional rotation around view axis
    glm::vec4 color = glm::vec4(1.0f);
    uint32_t textureIndex = 0;
    float distanceFade = 1.0f;
    bool visible = true;
};

// NiBillboardNode - Gamebryo-compatible billboard system
class NiBillboardNode {
public:
    NiBillboardNode();
    ~NiBillboardNode();
    
    // Initialization
    bool initialize(const BillboardConfig& config);
    void shutdown();
    
    // Configuration
    void setConfig(const BillboardConfig& config);
    const BillboardConfig& getConfig() const { return config_; }
    
    // Billboard management
    void addBillboard(const BillboardInstance& instance);
    void removeBillboard(size_t index);
    void clearBillboards();
    size_t getBillboardCount() const { return instances_.size(); }
    
    // Update all billboards to face camera
    void update(const glm::vec3& cameraPosition, 
                const glm::vec3& cameraForward,
                const glm::vec3& cameraUp,
                const glm::mat4& viewMatrix);
    
    // Get model matrices for rendering (after update)
    const std::vector<glm::mat4>& getModelMatrices() const { return modelMatrices_; }
    const std::vector<BillboardInstance>& getInstances() const { return instances_; }
    
    // Utility: Calculate billboard matrix for a single instance
    static glm::mat4 calculateBillboardMatrix(
        const glm::vec3& position,
        const glm::vec3& cameraPosition,
        const glm::vec3& cameraForward,
        const glm::vec3& cameraUp,
        const glm::vec3& worldUp,
        BillboardMode mode,
        float rotation = 0.0f,
        const glm::vec2& size = glm::vec2(1.0f));
    
    // Distance fade calculation
    static float calculateDistanceFade(
        const glm::vec3& position,
        const glm::vec3& cameraPosition,
        float fadeStart,
        float fadeEnd);
    
    // Culling
    void setCullingEnabled(bool enabled) { cullingEnabled_ = enabled; }
    bool isCullingEnabled() const { return cullingEnabled_; }
    
    // Visibility check (frustum culling)
    bool isVisible(const glm::vec3& position, float radius, 
                   const glm::mat4& viewProjMatrix) const;

private:
    BillboardConfig config_;
    std::vector<BillboardInstance> instances_;
    std::vector<glm::mat4> modelMatrices_;
    bool initialized_ = false;
    bool cullingEnabled_ = true;
    
    // Internal update methods
    void updateAlwaysFaceCamera(const glm::vec3& cameraPosition,
                                const glm::vec3& cameraForward,
                                const glm::vec3& cameraUp);
    void updateRotateAboutUp(const glm::vec3& cameraPosition,
                             const glm::vec3& worldUp);
    void updateRigidFaceCamera(const glm::vec3& cameraPosition,
                               const glm::vec3& cameraForward,
                               const glm::vec3& worldUp);
    void updateAlwaysFaceCenter(const glm::vec3& cameraPosition);
    void updateRigidFaceCenter(const glm::vec3& cameraPosition,
                               const glm::vec3& worldUp);
};

// ============================================================================
// Implementation
// ============================================================================

inline NiBillboardNode::NiBillboardNode() = default;
inline NiBillboardNode::~NiBillboardNode() { shutdown(); }

inline bool NiBillboardNode::initialize(const BillboardConfig& config) {
    config_ = config;
    instances_.clear();
    modelMatrices_.clear();
    initialized_ = true;
    LOGI("NiBillboardNode initialized (mode: %d)", static_cast<int>(config_.mode));
    return true;
}

inline void NiBillboardNode::shutdown() {
    instances_.clear();
    modelMatrices_.clear();
    initialized_ = false;
}

inline void NiBillboardNode::setConfig(const BillboardConfig& config) {
    config_ = config;
}

inline void NiBillboardNode::addBillboard(const BillboardInstance& instance) {
    instances_.push_back(instance);
    modelMatrices_.push_back(glm::mat4());
}

inline void NiBillboardNode::removeBillboard(size_t index) {
    if (index < instances_.size()) {
        instances_.erase(instances_.begin() + index);
        modelMatrices_.erase(modelMatrices_.begin() + index);
    }
}

inline void NiBillboardNode::clearBillboards() {
    instances_.clear();
    modelMatrices_.clear();
}

inline void NiBillboardNode::update(const glm::vec3& cameraPosition,
                                     const glm::vec3& cameraForward,
                                     const glm::vec3& cameraUp,
                                     const glm::mat4& viewMatrix) {
    if (!initialized_ || instances_.empty()) return;
    
    // Ensure model matrices vector matches instance count
    if (modelMatrices_.size() != instances_.size()) {
        modelMatrices_.resize(instances_.size());
    }
    
    // Update based on billboard mode
    switch (config_.mode) {
        case BillboardMode::ALWAYS_FACE_CAMERA:
            updateAlwaysFaceCamera(cameraPosition, cameraForward, cameraUp);
            break;
        case BillboardMode::ROTATE_ABOUT_UP:
            updateRotateAboutUp(cameraPosition, config_.worldUp);
            break;
        case BillboardMode::RIGID_FACE_CAMERA:
            updateRigidFaceCamera(cameraPosition, cameraForward, config_.worldUp);
            break;
        case BillboardMode::ALWAYS_FACE_CENTER:
            updateAlwaysFaceCenter(cameraPosition);
            break;
        case BillboardMode::RIGID_FACE_CENTER:
            updateRigidFaceCenter(cameraPosition, config_.worldUp);
            break;
    }
}

inline void NiBillboardNode::updateAlwaysFaceCamera(const glm::vec3& cameraPosition,
                                                     const glm::vec3& cameraForward,
                                                     const glm::vec3& cameraUp) {
    glm::vec3 right = glm::normalize(glm::cross(cameraForward, cameraUp));
    glm::vec3 up = glm::normalize(glm::cross(right, cameraForward));
    
    for (size_t i = 0; i < instances_.size(); ++i) {
        auto& inst = instances_[i];
        if (!inst.visible) continue;
        
        // Calculate distance fade
        if (config_.useDistanceFade) {
            inst.distanceFade = calculateDistanceFade(
                inst.position, cameraPosition, 
                config_.fadeStartDistance, config_.fadeEndDistance);
        }
        
        // Build rotation matrix from camera basis vectors
        glm::mat4 rotation(1.0f);
        rotation[0] = glm::vec4(right, 0.0f);
        rotation[1] = glm::vec4(up, 0.0f);
        rotation[2] = glm::vec4(-cameraForward, 0.0f);
        
        // Apply rotation around view axis if specified
        if (inst.rotation != 0.0f) {
            glm::mat4 rotMatrix = glm::rotate(glm::mat4(), inst.rotation, -cameraForward);
            rotation = rotation * rotMatrix;
        }
        
        // Build model matrix
        glm::mat4 model(1.0f);
        model = glm::translate(model, inst.position);
        model = model * rotation;
        model = glm::scale(model, glm::vec3(inst.size.x, inst.size.y, 1.0f) * config_.scale);
        
        modelMatrices_[i] = model;
    }
}

inline void NiBillboardNode::updateRotateAboutUp(const glm::vec3& cameraPosition,
                                                  const glm::vec3& worldUp) {
    for (size_t i = 0; i < instances_.size(); ++i) {
        auto& inst = instances_[i];
        if (!inst.visible) continue;
        
        // Calculate direction to camera in XZ plane
        glm::vec3 toCamera = cameraPosition - inst.position;
        toCamera.y = 0.0f;
        
        if (glm::length(toCamera) > 0.001f) {
            toCamera = glm::normalize(toCamera);
            
            // Calculate rotation angle around world up
            float angle = atan2(toCamera.x, toCamera.z);
            
            // Build model matrix
            glm::mat4 model(1.0f);
            model = glm::translate(model, inst.position);
            model = glm::rotate(model, angle, worldUp);
            model = glm::scale(model, glm::vec3(inst.size.x, inst.size.y, 1.0f) * config_.scale);
            
            modelMatrices_[i] = model;
        }
    }
}

inline void NiBillboardNode::updateRigidFaceCamera(const glm::vec3& cameraPosition,
                                                    const glm::vec3& cameraForward,
                                                    const glm::vec3& worldUp) {
    for (size_t i = 0; i < instances_.size(); ++i) {
        auto& inst = instances_[i];
        if (!inst.visible) continue;
        
        glm::vec3 toCamera = glm::normalize(cameraPosition - inst.position);
        
        // Calculate right vector constrained to world up
        glm::vec3 right = glm::normalize(glm::cross(worldUp, toCamera));
        glm::vec3 up = glm::normalize(glm::cross(toCamera, right));
        
        glm::mat4 rotation(1.0f);
        rotation[0] = glm::vec4(right, 0.0f);
        rotation[1] = glm::vec4(up, 0.0f);
        rotation[2] = glm::vec4(toCamera, 0.0f);
        
        glm::mat4 model(1.0f);
        model = glm::translate(model, inst.position);
        model = model * rotation;
        model = glm::scale(model, glm::vec3(inst.size.x, inst.size.y, 1.0f) * config_.scale);
        
        modelMatrices_[i] = model;
    }
}

inline void NiBillboardNode::updateAlwaysFaceCenter(const glm::vec3& cameraPosition) {
    // Simplified: all billboards face camera center
    updateAlwaysFaceCamera(cameraPosition, 
                           glm::normalize(-cameraPosition),
                           glm::vec3(0.0f, 1.0f, 0.0f));
}

inline void NiBillboardNode::updateRigidFaceCenter(const glm::vec3& cameraPosition,
                                                    const glm::vec3& worldUp) {
    updateRigidFaceCamera(cameraPosition,
                          glm::normalize(-cameraPosition),
                          worldUp);
}

inline glm::mat4 NiBillboardNode::calculateBillboardMatrix(
    const glm::vec3& position,
    const glm::vec3& cameraPosition,
    const glm::vec3& cameraForward,
    const glm::vec3& cameraUp,
    const glm::vec3& worldUp,
    BillboardMode mode,
    float rotation,
    const glm::vec2& size) {
    
    glm::mat4 model(1.0f);
    
    switch (mode) {
        case BillboardMode::ALWAYS_FACE_CAMERA: {
            glm::vec3 right = glm::normalize(glm::cross(cameraForward, cameraUp));
            glm::vec3 up = glm::normalize(glm::cross(right, cameraForward));
            
            glm::mat4 rot(1.0f);
            rot[0] = glm::vec4(right, 0.0f);
            rot[1] = glm::vec4(up, 0.0f);
            rot[2] = glm::vec4(-cameraForward, 0.0f);
            
            if (rotation != 0.0f) {
                glm::mat4 rotZ = glm::rotate(glm::mat4(), rotation, -cameraForward);
                rot = rot * rotZ;
            }
            
            model = glm::translate(model, position);
            model = model * rot;
            model = glm::scale(model, glm::vec3(size.x, size.y, 1.0f));
            break;
        }
        case BillboardMode::ROTATE_ABOUT_UP: {
            glm::vec3 toCamera = cameraPosition - position;
            toCamera.y = 0.0f;
            if (glm::length(toCamera) > 0.001f) {
                float angle = atan2(toCamera.x, toCamera.z);
                model = glm::translate(model, position);
                model = glm::rotate(model, angle, worldUp);
                model = glm::scale(model, glm::vec3(size.x, size.y, 1.0f));
            }
            break;
        }
        default:
            model = glm::translate(model, position);
            model = glm::scale(model, glm::vec3(size.x, size.y, 1.0f));
            break;
    }
    
    return model;
}

inline float NiBillboardNode::calculateDistanceFade(
    const glm::vec3& position,
    const glm::vec3& cameraPosition,
    float fadeStart,
    float fadeEnd) {
    
    float distance = glm::length(position - cameraPosition);
    if (distance >= fadeEnd) return 0.0f;
    if (distance <= fadeStart) return 1.0f;
    
    return 1.0f - (distance - fadeStart) / (fadeEnd - fadeStart);
}

inline bool NiBillboardNode::isVisible(const glm::vec3& position, float radius,
                                        const glm::mat4& viewProjMatrix) const {
    if (!cullingEnabled_) return true;
    
    glm::vec4 clipPos = viewProjMatrix * glm::vec4(position, 1.0f);
    if (clipPos.w <= 0.0f) return false;
    
    glm::vec3 ndc = glm::vec3(clipPos) / clipPos.w;
    
    // Simple frustum check with margin for billboard radius
    float margin = radius / clipPos.w;
    return (ndc.x >= -1.0f - margin && ndc.x <= 1.0f + margin &&
            ndc.y >= -1.0f - margin && ndc.y <= 1.0f + margin &&
            ndc.z >= -1.0f && ndc.z <= 1.0f);
}

} // namespace gamebryo
