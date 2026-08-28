// Phase 48: Stress Test Implementation
// Memory, concurrent tasks, entities, textures, and event bus stress tests

#include "phase48_stress_test.h"
#include "../engine/memory_pool.h"
#include "../engine/async_task_manager.h"
#include "../engine/imperial_weave.h"
#include "../game/npc.h"
#include "../game/npc_manager.h"
#include <chrono>
#include <cmath>
#include <cstring>
#include <atomic>
#include <vector>
#include <memory>
#include <sstream>
#include <thread>
#include <mutex>

#ifdef __ANDROID__
#include <android/log.h>
#define LOG_TAG "Phase48Stress"
#define TEST_LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define TEST_LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#else
#include <cstdio>
#define TEST_LOGI(...) printf(__VA_ARGS__)
#define TEST_LOGE(...) fprintf(stderr, __VA_ARGS__)
#endif

// ============================================================================
// Timer helper
// ============================================================================

float Phase48StressTest::getTimeMs() {
    static auto start = std::chrono::high_resolution_clock::now();
    auto now = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<float, std::milli>(now - start).count();
}

// ============================================================================
// Constructor / Destructor
// ============================================================================

Phase48StressTest::Phase48StressTest() {}
Phase48StressTest::~Phase48StressTest() {}

// ============================================================================
// Record a test result
// ============================================================================

void Phase48StressTest::record(const std::string& name, bool passed,
                                const std::string& msg, float ms) {
    Phase48StressResult r;
    r.testName = name;
    r.passed = passed;
    r.message = msg;
    r.durationMs = ms;
    results_.push_back(r);
    TEST_LOGI("[%s] %s (%.2f ms) %s",
              passed ? "PASS" : "FAIL", name.c_str(), ms, msg.c_str());
}

int Phase48StressTest::getPassCount() const {
    int count = 0;
    for (const auto& r : results_) {
        if (r.passed) count++;
    }
    return count;
}

int Phase48StressTest::getFailCount() const {
    return static_cast<int>(results_.size()) - getPassCount();
}

std::string Phase48StressTest::getSummary() const {
    std::ostringstream ss;
    ss << "=== Phase 48 Stress Test Results ===\n";
    ss << "Total: " << results_.size()
       << " | Pass: " << getPassCount()
       << " | Fail: " << getFailCount() << "\n\n";

    for (const auto& r : results_) {
        ss << (r.passed ? "[PASS]" : "[FAIL]") << " " << r.testName;
        if (r.durationMs > 0.0f) {
            ss << " (" << r.durationMs << " ms)";
        }
        if (!r.message.empty()) {
            ss << " - " << r.message;
        }
        ss << "\n";
    }
    return ss.str();
}

// ============================================================================
// Run all stress tests
// ============================================================================

bool Phase48StressTest::runAllTests() {
    TEST_LOGI("=== Phase 48 Stress Test START ===");
    results_.clear();

    testMemoryStress();
    testConcurrentTaskStress();
    testEntityStress();
    testTextureStress();
    testEventBusStress();

    int pass = getPassCount();
    int fail = getFailCount();
    TEST_LOGI("=== Phase 48 Stress Test END: %d pass, %d fail ===", pass, fail);
    return fail == 0;
}

// ============================================================================
// Test 1: Memory Stress - Allocate/deallocate 10000 objects
// ============================================================================

void Phase48StressTest::testMemoryStress() {
    float t0 = getTimeMs();

    // Use MemoryPoolManager's ObjectPool with PooledNPC
    const size_t POOL_SIZE = 10000;
    ObjectPool<MemoryPoolManager::PooledNPC> pool(POOL_SIZE);

    // Acquire 10000 objects
    std::vector<MemoryPoolManager::PooledNPC*> acquired;
    acquired.reserve(POOL_SIZE);

    for (size_t i = 0; i < POOL_SIZE; i++) {
        auto* obj = pool.acquire();
        if (obj) {
            obj->position[0] = static_cast<float>(i);
            obj->health = static_cast<int32_t>(i % 100);
            obj->active = true;
            acquired.push_back(obj);
        }
    }

    bool allAcquired = (acquired.size() == POOL_SIZE);
    bool peakOk = (pool.getPeakActive() == POOL_SIZE);

    // Release all
    for (auto* obj : acquired) {
        pool.release(obj);
    }
    bool allReleased = (pool.getActiveCount() == 0);

    // Re-acquire to verify pool recycling
    std::vector<MemoryPoolManager::PooledNPC*> reacquired;
    for (size_t i = 0; i < POOL_SIZE; i++) {
        auto* obj = pool.acquire();
        if (obj) {
            reacquired.push_back(obj);
        }
    }
    bool reacquireOk = (reacquired.size() == POOL_SIZE);

    // Cleanup
    for (auto* obj : reacquired) {
        pool.release(obj);
    }

    float elapsed = getTimeMs() - t0;
    bool allOk = allAcquired && peakOk && allReleased && reacquireOk;

    std::ostringstream msg;
    msg << "acquired=" << acquired.size() << ", peak=" << pool.getPeakActive()
        << ", recycled=" << reacquired.size();

    record("MemoryStress", allOk, msg.str(), elapsed);
}

// ============================================================================
// Test 2: Concurrent Task Stress - Submit 100 async tasks
// ============================================================================

void Phase48StressTest::testConcurrentTaskStress() {
    float t0 = getTimeMs();

    AsyncTaskManager taskMgr;
    bool initOk = taskMgr.initialize(4);
    if (!initOk) {
        record("ConcurrentTaskStress", false, "AsyncTaskManager init failed");
        return;
    }

    const int TASK_COUNT = 100;
    std::atomic<int> completedCount{0};
    std::vector<std::future<int>> futures;

    // Submit 100 tasks with varying priorities
    for (int i = 0; i < TASK_COUNT; i++) {
        AsyncTaskManager::Priority prio;
        if (i % 4 == 0) prio = AsyncTaskManager::Priority::CRITICAL;
        else if (i % 4 == 1) prio = AsyncTaskManager::Priority::HIGH;
        else if (i % 4 == 2) prio = AsyncTaskManager::Priority::NORMAL;
        else prio = AsyncTaskManager::Priority::LOW;

        auto future = taskMgr.submit(
            prio,
            AsyncTaskManager::Category::OTHER,
            "stress_task_" + std::to_string(i),
            [&completedCount, i]() -> int {
                // Simulate work
                volatile int sum = 0;
                for (int j = 0; j < 1000; j++) {
                    sum += j * i;
                }
                completedCount.fetch_add(1);
                return i;
            }
        );
        futures.push_back(std::move(future));
    }

    // Wait for all tasks
    taskMgr.waitForAll(std::chrono::milliseconds(10000));

    // Verify all completed
    int validResults = 0;
    for (auto& f : futures) {
        if (f.valid()) {
            try {
                int val = f.get();
                if (val >= 0 && val < TASK_COUNT) {
                    validResults++;
                }
            } catch (...) {
                // Task threw exception
            }
        }
    }

    auto stats = taskMgr.getStats();
    bool allCompleted = (completedCount.load() == TASK_COUNT);
    bool resultsOk = (validResults == TASK_COUNT);

    float elapsed = getTimeMs() - t0;
    bool allOk = allCompleted && resultsOk;

    std::ostringstream msg;
    msg << "submitted=" << TASK_COUNT << ", completed=" << completedCount.load()
        << ", valid=" << validResults << ", totalSubmitted=" << stats.totalSubmitted;

    record("ConcurrentTaskStress", allOk, msg.str(), elapsed);

    taskMgr.cleanup();
}

// ============================================================================
// Test 3: Entity Stress - Spawn 500 NPCs, update all
// ============================================================================

void Phase48StressTest::testEntityStress() {
    float t0 = getTimeMs();

    NpcManager npcMgr;
    npcMgr.initialize();

    const int NPC_COUNT = 500;

    // Spawn 500 NPCs
    std::vector<std::shared_ptr<NPC>> npcs;
    npcs.reserve(NPC_COUNT);

    for (int i = 0; i < NPC_COUNT; i++) {
        std::string name = "StressNPC_" + std::to_string(i);
        float x = static_cast<float>(i % 50) * 10.0f;
        float z = static_cast<float>(i / 50) * 10.0f;
        auto npc = npcMgr.createNPC(name, glm::vec3(x, 0.0f, z));
        if (npc) {
            npc->status.initialize(100.0f, 50.0f, 1);
            npc->setAIState(AIState::WANDER);
            npcs.push_back(npc);
        }
    }

    bool spawnOk = (static_cast<int>(npcs.size()) == NPC_COUNT);

    // Update all NPCs for 60 frames
    float updateStart = getTimeMs();
    for (int frame = 0; frame < 60; frame++) {
        npcMgr.update(1.0f / 60.0f);
    }
    float updateTime = getTimeMs() - updateStart;

    // Verify all NPCs survived
    int aliveCount = 0;
    for (const auto& npc : npcs) {
        auto n = npcMgr.getNPC(npc->npcId);
        if (n && n->status.isAlive()) {
            aliveCount++;
        }
    }
    bool aliveOk = (aliveCount == NPC_COUNT);

    // Area query test
    auto nearbyNpcs = npcMgr.getNPCsInArea(glm::vec3(0.0f, 0.0f, 0.0f), 50.0f);
    bool areaQueryOk = !nearbyNpcs.empty();

    float elapsed = getTimeMs() - t0;
    bool allOk = spawnOk && aliveOk && areaQueryOk;

    std::ostringstream msg;
    msg << "spawned=" << npcs.size() << ", alive=" << aliveCount
        << ", update60f=" << updateTime << "ms, nearby=" << nearbyNpcs.size();

    record("EntityStress", allOk, msg.str(), elapsed);

    npcMgr.cleanup();
}

// ============================================================================
// Test 4: Texture Stress - Load/unload 100 textures
// ============================================================================

void Phase48StressTest::testTextureStress() {
    float t0 = getTimeMs();

    // Use TextureCachePool for texture stress test
    const size_t MAX_ENTRIES = 128;
    const size_t MAX_MEMORY = 32 * 1024 * 1024;  // 32MB
    TextureCachePool cache(MAX_ENTRIES, MAX_MEMORY);
    cache.initialize();

    const int TEXTURE_COUNT = 100;

    // Load 100 textures (simulate with fake texture IDs)
    for (int i = 0; i < TEXTURE_COUNT; i++) {
        std::string key = "texture_" + std::to_string(i);
        uint32_t texId = static_cast<uint32_t>(i + 1);
        size_t sizeBytes = 256 * 256 * 4;  // 256x256 RGBA
        cache.put(key, texId, sizeBytes);
    }

    bool loadOk = (cache.getEntryCount() == static_cast<size_t>(TEXTURE_COUNT));

    // Verify all textures are accessible
    int hitCount = 0;
    for (int i = 0; i < TEXTURE_COUNT; i++) {
        std::string key = "texture_" + std::to_string(i);
        if (cache.contains(key)) {
            hitCount++;
        }
    }
    bool accessOk = (hitCount == TEXTURE_COUNT);

    // Remove half
    for (int i = 0; i < TEXTURE_COUNT / 2; i++) {
        std::string key = "texture_" + std::to_string(i);
        cache.remove(key);
    }
    bool removeOk = (cache.getEntryCount() == static_cast<size_t>(TEXTURE_COUNT / 2));

    // Reload removed textures
    for (int i = 0; i < TEXTURE_COUNT / 2; i++) {
        std::string key = "texture_" + std::to_string(i);
        uint32_t texId = static_cast<uint32_t>(i + 1);
        cache.put(key, texId, 256 * 256 * 4);
    }
    bool reloadOk = (cache.getEntryCount() == static_cast<size_t>(TEXTURE_COUNT));

    // Clear all
    cache.clear();
    bool clearOk = (cache.getEntryCount() == 0);

    float elapsed = getTimeMs() - t0;
    bool allOk = loadOk && accessOk && removeOk && reloadOk && clearOk;

    std::ostringstream msg;
    msg << "loaded=" << TEXTURE_COUNT << ", hits=" << hitCount
        << ", memory=" << cache.getMemoryUsed() << "B";

    record("TextureStress", allOk, msg.str(), elapsed);

    cache.cleanup();
}

// ============================================================================
// Test 5: Event Bus Stress - Emit 10000 events
// ============================================================================

void Phase48StressTest::testEventBusStress() {
    float t0 = getTimeMs();

    weave::EventBus eventBus;
    const int EVENT_COUNT = 10000;

    // Subscribe to multiple event types
    std::atomic<int> eventACount{0};
    std::atomic<int> eventBCount{0};
    std::atomic<int> eventCCount{0};

    eventBus.subscribe("TYPE_A", [&eventACount](const weave::Event&) {
        eventACount.fetch_add(1);
    });
    eventBus.subscribe("TYPE_B", [&eventBCount](const weave::Event&) {
        eventBCount.fetch_add(1);
    });
    eventBus.subscribe("TYPE_C", [&eventCCount](const weave::Event&) {
        eventCCount.fetch_add(1);
    });

    // Emit 10000 events (immediate dispatch)
    float emitStart = getTimeMs();
    for (int i = 0; i < EVENT_COUNT; i++) {
        weave::Event e;
        if (i % 3 == 0) {
            e.type = "TYPE_A";
        } else if (i % 3 == 1) {
            e.type = "TYPE_B";
        } else {
            e.type = "TYPE_C";
        }
        e.sender = static_cast<uint32_t>(i);
        e.payload = "stress_event_" + std::to_string(i);
        eventBus.emitImmediate(e);
    }
    float emitTime = getTimeMs() - emitStart;

    int totalHandled = eventACount.load() + eventBCount.load() + eventCCount.load();
    bool allHandled = (totalHandled == EVENT_COUNT);

    // Test deferred emit + processQueue
    eventBus.clearQueue();
    std::atomic<int> deferredCount{0};
    eventBus.subscribe("DEFERRED", [&deferredCount](const weave::Event&) {
        deferredCount.fetch_add(1);
    });

    const int DEFERRED_COUNT = 1000;
    for (int i = 0; i < DEFERRED_COUNT; i++) {
        weave::Event e;
        e.type = "DEFERRED";
        e.sender = static_cast<uint32_t>(i);
        eventBus.emit(e);
    }

    bool queueSizeOk = (eventBus.getQueueSize() == static_cast<size_t>(DEFERRED_COUNT));
    eventBus.processQueue();
    bool deferredOk = (deferredCount.load() == DEFERRED_COUNT);
    bool queueEmpty = (eventBus.getQueueSize() == 0);

    float elapsed = getTimeMs() - t0;
    bool allOk = allHandled && queueSizeOk && deferredOk && queueEmpty;

    std::ostringstream msg;
    msg << "immediate=" << EVENT_COUNT << " in " << emitTime << "ms"
        << ", deferred=" << DEFERRED_COUNT
        << ", handled=" << totalHandled;

    record("EventBusStress", allOk, msg.str(), elapsed);
}
