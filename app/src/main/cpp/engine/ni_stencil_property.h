#pragma once

#include <cstdint>
#include <array>
#include <functional>
#include <android/log.h>
#include <GLES3/gl3.h>

#define LOG_TAG "NiStencilProperty"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

// ============================================================================
// NiStencilProperty - Gamebryo stencil buffer control
// Used for portal mirrors, magic effects, silhouette rendering, decals
// ============================================================================

namespace gamebryo {

// Stencil test function (matching Gamebryo/GLES3)
enum class StencilFunction : uint32_t {
    NEVER = GL_NEVER,           // Always fail
    LESS = GL_LESS,             // Pass if ref < stencil
    EQUAL = GL_EQUAL,           // Pass if ref == stencil
    LEQUAL = GL_LEQUAL,         // Pass if ref <= stencil
    GREATER = GL_GREATER,       // Pass if ref > stencil
    NOTEQUAL = GL_NOTEQUAL,     // Pass if ref != stencil
    GEQUAL = GL_GEQUAL,         // Pass if ref >= stencil
    ALWAYS = GL_ALWAYS          // Always pass
};

// Stencil operation (matching Gamebryo/GLES3)
enum class StencilOperation : uint32_t {
    KEEP = GL_KEEP,             // Keep current value
    ZERO = GL_ZERO,             // Set to zero
    REPLACE = GL_REPLACE,       // Replace with reference
    INCREMENT = GL_INCR,        // Increment (clamp to max)
    DECREMENT = GL_DECR,        // Decrement (clamp to 0)
    INVERT = GL_INVERT,         // Bitwise invert
    INCREMENT_WRAP = GL_INCR_WRAP,  // Increment with wrap
    DECREMENT_WRAP = GL_DECR_WRAP   // Decrement with wrap
};

// Stencil face mode (GLES3 supports separate front/back)
enum class StencilFace : uint32_t {
    FRONT = GL_FRONT,
    BACK = GL_BACK,
    FRONT_AND_BACK = GL_FRONT_AND_BACK
};

// Stencil state configuration
struct StencilState {
    bool enabled = false;
    uint32_t reference = 0;
    uint32_t readMask = 0xFF;
    uint32_t writeMask = 0xFF;
    
    StencilFunction function = StencilFunction::ALWAYS;
    StencilOperation failOp = StencilOperation::KEEP;
    StencilOperation zFailOp = StencilOperation::KEEP;
    StencilOperation zPassOp = StencilOperation::KEEP;
    
    // Two-sided stencil (GLES3)
    bool twoSided = false;
    StencilState backState;
    
    bool operator==(const StencilState& other) const {
        return enabled == other.enabled &&
               reference == other.reference &&
               readMask == other.readMask &&
               writeMask == other.writeMask &&
               function == other.function &&
               failOp == other.failOp &&
               zFailOp == other.zFailOp &&
               zPassOp == other.zPassOp &&
               twoSided == other.twoSided;
    }
    
    bool operator!=(const StencilState& other) const {
        return !(*this == other);
    }
};

// Preset stencil configurations for common effects
namespace StencilPresets {
    // Portal/mirror: Write stencil mask
    inline StencilState PortalMask(uint32_t mask = 0x01) {
        StencilState state;
        state.enabled = true;
        state.reference = mask;
        state.writeMask = mask;
        state.function = StencilFunction::ALWAYS;
        state.failOp = StencilOperation::KEEP;
        state.zFailOp = StencilOperation::KEEP;
        state.zPassOp = StencilOperation::REPLACE;
        return state;
    }
    
    // Portal/mirror content: Only draw inside mask
    inline StencilState PortalContent(uint32_t mask = 0x01) {
        StencilState state;
        state.enabled = true;
        state.reference = mask;
        state.readMask = mask;
        state.function = StencilFunction::EQUAL;
        state.failOp = StencilOperation::KEEP;
        state.zFailOp = StencilOperation::KEEP;
        state.zPassOp = StencilOperation::KEEP;
        return state;
    }
    
    // Decal: Draw only where stencil is zero, increment to prevent overlap
    inline StencilState DecalApply() {
        StencilState state;
        state.enabled = true;
        state.reference = 0;
        state.readMask = 0xFF;
        state.writeMask = 0xFF;
        state.function = StencilFunction::EQUAL;
        state.failOp = StencilOperation::KEEP;
        state.zFailOp = StencilOperation::KEEP;
        state.zPassOp = StencilOperation::INCREMENT;
        return state;
    }
    
    // Decal clear: Reset stencil
    inline StencilState DecalClear() {
        StencilState state;
        state.enabled = true;
        state.reference = 0;
        state.writeMask = 0xFF;
        state.function = StencilFunction::ALWAYS;
        state.failOp = StencilOperation::ZERO;
        state.zFailOp = StencilOperation::ZERO;
        state.zPassOp = StencilOperation::ZERO;
        return state;
    }
    
    // Silhouette: Draw with inverted stencil
    inline StencilState Silhouette(uint32_t mask = 0x02) {
        StencilState state;
        state.enabled = true;
        state.reference = mask;
        state.writeMask = mask;
        state.function = StencilFunction::ALWAYS;
        state.failOp = StencilOperation::INVERT;
        state.zFailOp = StencilOperation::INVERT;
        state.zPassOp = StencilOperation::INVERT;
        return state;
    }
    
    // Outline: Draw only stencil edges
    inline StencilState Outline(uint32_t mask = 0x02) {
        StencilState state;
        state.enabled = true;
        state.reference = 0;
        state.readMask = mask;
        state.function = StencilFunction::NOTEQUAL;
        state.failOp = StencilOperation::KEEP;
        state.zFailOp = StencilOperation::KEEP;
        state.zPassOp = StencilOperation::KEEP;
        return state;
    }
    
    // Disabled
    inline StencilState Disabled() {
        StencilState state;
        state.enabled = false;
        return state;
    }
}

// NiStencilProperty - Main stencil property class
class NiStencilProperty {
public:
    NiStencilProperty();
    ~NiStencilProperty();
    
    // Initialization
    bool initialize(const StencilState& state);
    void shutdown();
    
    // State management
    void setState(const StencilState& state);
    const StencilState& getState() const { return currentState_; }
    
    // Apply stencil state to OpenGL ES
    void apply() const;
    void revert() const;
    
    // Quick state changes
    void enable(bool enabled);
    void setReference(uint32_t ref);
    void setMasks(uint32_t readMask, uint32_t writeMask);
    void setFunction(StencilFunction func);
    void setOperations(StencilOperation fail, StencilOperation zFail, StencilOperation zPass);
    
    // Preset application
    void applyPreset(const StencilState& preset);
    void applyPortalMask(uint32_t mask = 0x01);
    void applyPortalContent(uint32_t mask = 0x01);
    void applyDecal();
    void applyDecalClear();
    void applySilhouette(uint32_t mask = 0x02);
    void applyOutline(uint32_t mask = 0x02);
    void disable();
    
    // Stencil buffer operations
    void clearStencil(uint32_t value = 0);
    void clearStencilRect(int x, int y, int width, int height, uint32_t value);
    
    // State stack for nested stencil operations
    void pushState();
    void popState();
    
    // Query
    bool isEnabled() const { return currentState_.enabled; }
    uint32_t getReference() const { return currentState_.reference; }
    
    // Validation
    static bool isSupported();
    static int getStencilBits();

private:
    StencilState currentState_;
    StencilState previousState_;
    std::array<StencilState, 8> stateStack_;
    int stackTop_ = -1;
    bool initialized_ = false;
    
    void applyState(const StencilState& state) const;
};

// StencilEffectManager - High-level stencil effect management
class StencilEffectManager {
public:
    static StencilEffectManager& getInstance();
    
    // Effect lifecycle
    void beginPortalMirror(const glm::vec3& portalCenter, uint32_t mask = 0x01);
    void endPortalMirror();
    void renderPortalContent(std::function<void()> renderFunc, uint32_t mask = 0x01);
    
    void beginDecalProjection();
    void endDecalProjection();
    void applyDecal(std::function<void()> decalRender);
    
    void beginSilhouette(uint32_t mask = 0x02);
    void endSilhouette();
    void renderOutline(std::function<void()> outlineRender, uint32_t mask = 0x02);
    
    // Magic effect glow
    void beginMagicGlow(uint32_t mask = 0x04);
    void endMagicGlow();
    
    // Clear all stencil effects
    void clearAll();
    
    // Mask management (allocate/free stencil mask bits)
    uint32_t allocateMask();
    void freeMask(uint32_t mask);
    
private:
    StencilEffectManager() = default;
    ~StencilEffectManager() = default;
    StencilEffectManager(const StencilEffectManager&) = delete;
    StencilEffectManager& operator=(const StencilEffectManager&) = delete;
    
    NiStencilProperty stencilProp_;
    uint32_t usedMasks_ = 0;
    bool inPortal_ = false;
    bool inDecal_ = false;
    bool inSilhouette_ = false;
};

// ============================================================================
// Implementation
// ============================================================================

inline NiStencilProperty::NiStencilProperty() = default;
inline NiStencilProperty::~NiStencilProperty() { shutdown(); }

inline bool NiStencilProperty::initialize(const StencilState& state) {
    currentState_ = state;
    previousState_ = StencilPresets::Disabled();
    stackTop_ = -1;
    initialized_ = true;
    
    if (!isSupported()) {
        LOGW("Stencil buffer not fully supported on this device");
    }
    
    LOGI("NiStencilProperty initialized (stencil bits: %d)", getStencilBits());
    return true;
}

inline void NiStencilProperty::shutdown() {
    if (initialized_ && currentState_.enabled) {
        disable();
    }
    initialized_ = false;
}

inline void NiStencilProperty::setState(const StencilState& state) {
    previousState_ = currentState_;
    currentState_ = state;
}

inline void NiStencilProperty::apply() const {
    if (!initialized_) return;
    applyState(currentState_);
}

inline void NiStencilProperty::revert() const {
    applyState(previousState_);
}

inline void NiStencilProperty::applyState(const StencilState& state) const {
    if (state.enabled) {
        glEnable(GL_STENCIL_TEST);
        glStencilFunc(static_cast<GLenum>(state.function), 
                      state.reference, 
                      state.readMask);
        glStencilOp(static_cast<GLenum>(state.failOp),
                    static_cast<GLenum>(state.zFailOp),
                    static_cast<GLenum>(state.zPassOp));
        glStencilMask(state.writeMask);
        
        if (state.twoSided) {
            // GLES3 supports separate front/back stencil
            glStencilFuncSeparate(GL_BACK,
                                  static_cast<GLenum>(state.backState.function),
                                  state.backState.reference,
                                  state.backState.readMask);
            glStencilOpSeparate(GL_BACK,
                                static_cast<GLenum>(state.backState.failOp),
                                static_cast<GLenum>(state.backState.zFailOp),
                                static_cast<GLenum>(state.backState.zPassOp));
        }
    } else {
        glDisable(GL_STENCIL_TEST);
    }
}

inline void NiStencilProperty::enable(bool enabled) {
    currentState_.enabled = enabled;
}

inline void NiStencilProperty::setReference(uint32_t ref) {
    currentState_.reference = ref;
}

inline void NiStencilProperty::setMasks(uint32_t readMask, uint32_t writeMask) {
    currentState_.readMask = readMask;
    currentState_.writeMask = writeMask;
}

inline void NiStencilProperty::setFunction(StencilFunction func) {
    currentState_.function = func;
}

inline void NiStencilProperty::setOperations(StencilOperation fail, 
                                              StencilOperation zFail, 
                                              StencilOperation zPass) {
    currentState_.failOp = fail;
    currentState_.zFailOp = zFail;
    currentState_.zPassOp = zPass;
}

inline void NiStencilProperty::applyPreset(const StencilState& preset) {
    previousState_ = currentState_;
    currentState_ = preset;
    apply();
}

inline void NiStencilProperty::applyPortalMask(uint32_t mask) {
    applyPreset(StencilPresets::PortalMask(mask));
}

inline void NiStencilProperty::applyPortalContent(uint32_t mask) {
    applyPreset(StencilPresets::PortalContent(mask));
}

inline void NiStencilProperty::applyDecal() {
    applyPreset(StencilPresets::DecalApply());
}

inline void NiStencilProperty::applyDecalClear() {
    applyPreset(StencilPresets::DecalClear());
}

inline void NiStencilProperty::applySilhouette(uint32_t mask) {
    applyPreset(StencilPresets::Silhouette(mask));
}

inline void NiStencilProperty::applyOutline(uint32_t mask) {
    applyPreset(StencilPresets::Outline(mask));
}

inline void NiStencilProperty::disable() {
    previousState_ = currentState_;
    currentState_.enabled = false;
    glDisable(GL_STENCIL_TEST);
}

inline void NiStencilProperty::clearStencil(uint32_t value) {
    glClearStencil(static_cast<GLint>(value));
    glClear(GL_STENCIL_BUFFER_BIT);
}

inline void NiStencilProperty::clearStencilRect(int x, int y, int width, int height, uint32_t value) {
    glEnable(GL_SCISSOR_TEST);
    glScissor(x, y, width, height);
    glClearStencil(static_cast<GLint>(value));
    glClear(GL_STENCIL_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);
}

inline void NiStencilProperty::pushState() {
    if (stackTop_ < static_cast<int>(stateStack_.size()) - 1) {
        stateStack_[++stackTop_] = currentState_;
    }
}

inline void NiStencilProperty::popState() {
    if (stackTop_ >= 0) {
        currentState_ = stateStack_[stackTop_--];
        apply();
    }
}

inline bool NiStencilProperty::isSupported() {
    GLint stencilBits = 0;
    glGetIntegerv(GL_STENCIL_BITS, &stencilBits);
    return stencilBits > 0;
}

inline int NiStencilProperty::getStencilBits() {
    GLint stencilBits = 0;
    glGetIntegerv(GL_STENCIL_BITS, &stencilBits);
    return stencilBits;
}

// StencilEffectManager implementation
inline StencilEffectManager& StencilEffectManager::getInstance() {
    static StencilEffectManager instance;
    return instance;
}

inline void StencilEffectManager::beginPortalMirror(const glm::vec3& portalCenter, uint32_t mask) {
    if (inPortal_) return;
    stencilProp_.pushState();
    stencilProp_.applyPortalMask(mask);
    inPortal_ = true;
    LOGD("Begin portal mirror (mask: 0x%02X)", mask);
}

inline void StencilEffectManager::endPortalMirror() {
    if (!inPortal_) return;
    stencilProp_.popState();
    inPortal_ = false;
    LOGD("End portal mirror");
}

inline void StencilEffectManager::renderPortalContent(std::function<void()> renderFunc, uint32_t mask) {
    stencilProp_.pushState();
    stencilProp_.applyPortalContent(mask);
    renderFunc();
    stencilProp_.popState();
}

inline void StencilEffectManager::beginDecalProjection() {
    if (inDecal_) return;
    stencilProp_.pushState();
    stencilProp_.clearStencil(0);
    inDecal_ = true;
}

inline void StencilEffectManager::endDecalProjection() {
    if (!inDecal_) return;
    stencilProp_.popState();
    inDecal_ = false;
}

inline void StencilEffectManager::applyDecal(std::function<void()> decalRender) {
    stencilProp_.pushState();
    stencilProp_.applyDecal();
    decalRender();
    stencilProp_.popState();
}

inline void StencilEffectManager::beginSilhouette(uint32_t mask) {
    if (inSilhouette_) return;
    stencilProp_.pushState();
    stencilProp_.clearStencil(0);
    stencilProp_.applySilhouette(mask);
    inSilhouette_ = true;
}

inline void StencilEffectManager::endSilhouette() {
    if (!inSilhouette_) return;
    stencilProp_.popState();
    inSilhouette_ = false;
}

inline void StencilEffectManager::renderOutline(std::function<void()> outlineRender, uint32_t mask) {
    stencilProp_.pushState();
    stencilProp_.applyOutline(mask);
    outlineRender();
    stencilProp_.popState();
}

inline void StencilEffectManager::beginMagicGlow(uint32_t mask) {
    stencilProp_.pushState();
    // Magic glow uses stencil to prevent overdraw
    stencilProp_.applyPreset(StencilPresets::PortalMask(mask));
}

inline void StencilEffectManager::endMagicGlow() {
    stencilProp_.popState();
}

inline void StencilEffectManager::clearAll() {
    stencilProp_.disable();
    stencilProp_.clearStencil(0);
    usedMasks_ = 0;
    inPortal_ = false;
    inDecal_ = false;
    inSilhouette_ = false;
}

inline uint32_t StencilEffectManager::allocateMask() {
    for (int i = 0; i < 8; ++i) {
        uint32_t mask = 1 << i;
        if ((usedMasks_ & mask) == 0) {
            usedMasks_ |= mask;
            return mask;
        }
    }
    return 0; // No available mask
}

inline void StencilEffectManager::freeMask(uint32_t mask) {
    usedMasks_ &= ~mask;
}

} // namespace gamebryo
