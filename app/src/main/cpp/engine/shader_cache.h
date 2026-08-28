#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <list>
#include <mutex>
#include <functional>
#include <chrono>
#include <sstream>
#include <android/log.h>

#define LOG_TAG_SC "ShaderCache"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD_SC(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_SC, __VA_ARGS__)
#else
#define LOGD_SC(...) do {} while(0)
#endif
#define LOGI_SC(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_SC, __VA_ARGS__)
#define LOGW_SC(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG_SC, __VA_ARGS__)

// ============================================================================
// Shader Cache
// Phase 55: LRU shader program caching with permutation support
// ============================================================================

namespace engine {

// Shader type enum
enum class ShaderType : uint8_t {
    VERTEX = 0,
    FRAGMENT = 1,
    COMPUTE = 2
};

// Compiled shader program handle
struct ShaderProgram {
    uint32_t programId = 0;
    uint32_t vertexShaderId = 0;
    uint32_t fragmentShaderId = 0;
    std::string vertexSource;
    std::string fragmentSource;
    uint64_t sourceHash = 0;
    float compileTimeMs = 0.0f;
    uint32_t useCount = 0;
    bool isValid = false;
};

// Shader permutation key
struct ShaderPermutationKey {
    std::string baseName;
    std::vector<std::string> defines;

    bool operator==(const ShaderPermutationKey& o) const {
        return baseName == o.baseName && defines == o.defines;
    }
};

struct ShaderPermutationKeyHash {
    size_t operator()(const ShaderPermutationKey& k) const {
        size_t h = std::hash<std::string>{}(k.baseName);
        for (const auto& d : k.defines) {
            h ^= std::hash<std::string>{}(d) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }
        return h;
    }
};

// Uniform location cache entry
struct UniformEntry {
    int location = -1;
    // Cached values to avoid redundant GL calls
    float floatVal[4] = {};
    int intVal[4] = {};
    uint32_t lastUpdateFrame = 0;
};

// ============================================================================
// ShaderCache - singleton LRU shader program cache
// ============================================================================

class ShaderCache {
public:
    static constexpr size_t DEFAULT_MAX_CACHED = 64;

    static ShaderCache& instance() {
        static ShaderCache inst;
        return inst;
    }

    void init(size_t maxCached = DEFAULT_MAX_CACHED) {
        std::lock_guard<std::mutex> lock(mutex_);
        maxCached_ = maxCached;
        hitCount_ = 0;
        missCount_ = 0;
        totalCompileTimeMs_ = 0.0f;
        initialized_ = true;
        LOGI_SC("Initialized with max cache size: %zu", maxCached);
    }

    void shutdown() {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.clear();
        lruList_.clear();
        lruMap_.clear();
        permutations_.clear();
        uniformCache_.clear();
        initialized_ = false;
    }

    // --- Program cache ---

    // Get or compile a shader program (LRU cached)
    ShaderProgram* getOrCreate(
        const std::string& vertexSource,
        const std::string& fragmentSource,
        const std::string& debugName = ""
    ) {
        std::lock_guard<std::mutex> lock(mutex_);

        uint64_t hash = hashSources(vertexSource, fragmentSource);

        // Check cache
        auto it = cache_.find(hash);
        if (it != cache_.end()) {
            // Cache hit: move to front of LRU
            touchLRU(hash);
            hitCount_++;
            it->second.useCount++;
            return &it->second;
        }

        // Cache miss: compile new shader
        missCount_++;

        auto compileStart = std::chrono::high_resolution_clock::now();

        ShaderProgram program;
        program.vertexSource = vertexSource;
        program.fragmentSource = fragmentSource;
        program.sourceHash = hash;

        // In a real implementation, this would call glCreateShader/glCompileShader/glLinkProgram
        // For now, mark as valid stub
        program.programId = nextProgramId_++;
        program.vertexShaderId = nextProgramId_++;
        program.fragmentShaderId = nextProgramId_++;
        program.isValid = true;

        auto compileEnd = std::chrono::high_resolution_clock::now();
        program.compileTimeMs = std::chrono::duration<float, std::milli>(
            compileEnd - compileStart).count();
        totalCompileTimeMs_ += program.compileTimeMs;

        // Evict LRU if at capacity
        if (cache_.size() >= maxCached_) {
            evictLRU();
        }

        // Insert into cache
        cache_[hash] = program;
        lruList_.push_front(hash);
        lruMap_[hash] = lruList_.begin();

        if (!debugName.empty()) {
            LOGD_SC("Compiled shader '%s' in %.2f ms (hash: %llu)",
                    debugName.c_str(), program.compileTimeMs,
                    static_cast<unsigned long long>(hash));
        }

        return &cache_[hash];
    }

    // --- Permutation system ---

    // Register a shader base with possible permutations
    void registerPermutation(
        const std::string& baseName,
        const std::string& vertexTemplate,
        const std::string& fragmentTemplate
    ) {
        std::lock_guard<std::mutex> lock(mutex_);
        permutationTemplates_[baseName] = {vertexTemplate, fragmentTemplate};
    }

    // Get a specific permutation (compiles defines into shader source)
    ShaderProgram* getPermutation(
        const std::string& baseName,
        const std::vector<std::string>& defines
    ) {
        std::lock_guard<std::mutex> lock(mutex_);

        ShaderPermutationKey key{baseName, defines};
        auto it = permutations_.find(key);
        if (it != permutations_.end()) {
            return it->second;
        }

        // Build source with defines
        auto tmplIt = permutationTemplates_.find(baseName);
        if (tmplIt == permutationTemplates_.end()) {
            LOGW_SC("Unknown shader base: %s", baseName.c_str());
            return nullptr;
        }

        std::string defineBlock;
        for (const auto& d : defines) {
            defineBlock += "#define " + d + "\n";
        }

        std::string vertSrc = defineBlock + tmplIt->second.first;
        std::string fragSrc = defineBlock + tmplIt->second.second;

        // Unlock before calling getOrCreate (which also locks)
        mutex_.unlock();
        ShaderProgram* prog = getOrCreate(vertSrc, fragSrc, baseName);
        mutex_.lock();

        permutations_[key] = prog;
        return prog;
    }

    // --- Uniform cache ---

    void setUniformFloat(uint32_t programId, const std::string& name,
                         const float* values, int count) {
        std::lock_guard<std::mutex> lock(mutex_);
        uint64_t key = uniformKey(programId, name);
        UniformEntry& entry = uniformCache_[key];
        bool changed = false;
        for (int i = 0; i < count && i < 4; i++) {
            if (entry.floatVal[i] != values[i]) {
                entry.floatVal[i] = values[i];
                changed = true;
            }
        }
        if (changed) {
            // In real impl: glUniform1f/2f/3f/4f
            entry.lastUpdateFrame = currentFrame_;
        }
    }

    void setUniformInt(uint32_t programId, const std::string& name,
                       const int* values, int count) {
        std::lock_guard<std::mutex> lock(mutex_);
        uint64_t key = uniformKey(programId, name);
        UniformEntry& entry = uniformCache_[key];
        bool changed = false;
        for (int i = 0; i < count && i < 4; i++) {
            if (entry.intVal[i] != values[i]) {
                entry.intVal[i] = values[i];
                changed = true;
            }
        }
        if (changed) {
            entry.lastUpdateFrame = currentFrame_;
        }
    }

    // --- Hot reload ---

    // Invalidate a specific shader (for hot reload)
    void invalidate(const std::string& debugName) {
        std::lock_guard<std::mutex> lock(mutex_);
        // In real impl, would look up by name and remove from cache
        LOGI_SC("Shader invalidation requested: %s", debugName.c_str());
    }

    // Invalidate all shaders
    void invalidateAll() {
        std::lock_guard<std::mutex> lock(mutex_);
        cache_.clear();
        lruList_.clear();
        lruMap_.clear();
        permutations_.clear();
        LOGI_SC("All shaders invalidated");
    }

    // --- Frame tracking ---

    void advanceFrame() { currentFrame_++; }

    // --- Statistics ---

    struct CacheStats {
        size_t cachedCount;
        size_t maxCached;
        uint32_t hitCount;
        uint32_t missCount;
        float hitRate;
        float totalCompileTimeMs;
        float avgCompileTimeMs;
    };

    CacheStats getStats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        CacheStats s{};
        s.cachedCount = cache_.size();
        s.maxCached = maxCached_;
        s.hitCount = hitCount_;
        s.missCount = missCount_;
        uint32_t total = hitCount_ + missCount_;
        s.hitRate = total > 0 ? static_cast<float>(hitCount_) / total : 0.0f;
        s.totalCompileTimeMs = totalCompileTimeMs_;
        s.avgCompileTimeMs = missCount_ > 0 ? totalCompileTimeMs_ / missCount_ : 0.0f;
        return s;
    }

private:
    ShaderCache() = default;

    bool initialized_ = false;
    size_t maxCached_ = DEFAULT_MAX_CACHED;
    uint32_t nextProgramId_ = 1;
    uint32_t currentFrame_ = 0;

    // LRU cache
    std::unordered_map<uint64_t, ShaderProgram> cache_;
    std::list<uint64_t> lruList_;
    std::unordered_map<uint64_t, std::list<uint64_t>::iterator> lruMap_;

    // Permutation system
    std::unordered_map<std::string, std::pair<std::string, std::string>> permutationTemplates_;
    std::unordered_map<ShaderPermutationKey, ShaderProgram*, ShaderPermutationKeyHash> permutations_;

    // Uniform cache
    std::unordered_map<uint64_t, UniformEntry> uniformCache_;

    // Stats
    uint32_t hitCount_ = 0;
    uint32_t missCount_ = 0;
    float totalCompileTimeMs_ = 0.0f;

    mutable std::mutex mutex_;

    uint64_t hashSources(const std::string& vert, const std::string& frag) const {
        // FNV-1a hash
        uint64_t hash = 0xcbf29ce484222325ULL;
        for (char c : vert) {
            hash ^= static_cast<uint64_t>(c);
            hash *= 0x100000001b3ULL;
        }
        for (char c : frag) {
            hash ^= static_cast<uint64_t>(c);
            hash *= 0x100000001b3ULL;
        }
        return hash;
    }

    void touchLRU(uint64_t hash) {
        auto it = lruMap_.find(hash);
        if (it != lruMap_.end()) {
            lruList_.erase(it->second);
        }
        lruList_.push_front(hash);
        lruMap_[hash] = lruList_.begin();
    }

    void evictLRU() {
        if (lruList_.empty()) return;
        uint64_t victim = lruList_.back();
        lruList_.pop_back();
        lruMap_.erase(victim);
        cache_.erase(victim);
        LOGD_SC("Evicted shader (hash: %llu)", static_cast<unsigned long long>(victim));
    }

    static uint64_t uniformKey(uint32_t programId, const std::string& name) {
        uint64_t h = static_cast<uint64_t>(programId) << 32;
        for (char c : name) {
            h ^= static_cast<uint64_t>(c);
            h *= 0x100000001b3ULL;
        }
        return h;
    }
};

} // namespace engine
