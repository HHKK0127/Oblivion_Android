#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>
#include <future>
#include <string>
#include <chrono>
#include <android/log.h>

#define LOG_TAG_ASYNCTASK "AsyncTaskManager"
#define LOGD_AT(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_ASYNCTASK, __VA_ARGS__)
#define LOGI_AT(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_ASYNCTASK, __VA_ARGS__)
#define LOGW_AT(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG_ASYNCTASK, __VA_ARGS__)
#define LOGE_AT(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_ASYNCTASK, __VA_ARGS__)

// ============================================================================
// AsyncTask Manager - Thread pool based async task execution
// ============================================================================

class AsyncTaskManager {
public:
    // ========================================================================
    // Task priority levels
    // ========================================================================

    enum class Priority : uint8_t {
        CRITICAL = 0,   // Must complete ASAP (e.g., player cell load)
        HIGH = 1,       // Important (e.g., nearby cell load)
        NORMAL = 2,     // Standard (e.g., texture prefetch)
        LOW = 3,        // Background (e.g., distant cell load)
        IDLE = 4        // Only when idle (e.g., cache cleanup)
    };

    // ========================================================================
    // Task category for profiling
    // ========================================================================

    enum class Category : uint8_t {
        CELL_LOAD,
        TEXTURE_LOAD,
        ESM_PARSE,
        MESH_LOAD,
        AUDIO_LOAD,
        CACHE_OPERATION,
        OTHER
    };

    // ========================================================================
    // Task handle for tracking
    // ========================================================================

    struct TaskInfo {
        uint64_t taskId = 0;
        std::string name;
        Priority priority = Priority::NORMAL;
        Category category = Category::OTHER;
        std::chrono::steady_clock::time_point submitTime;
        std::chrono::steady_clock::time_point startTime;
        std::chrono::steady_clock::time_point endTime;
        bool completed = false;
        bool cancelled = false;
    };

    // ========================================================================
    // Statistics
    // ========================================================================

    struct TaskStats {
        uint64_t totalSubmitted = 0;
        uint64_t totalCompleted = 0;
        uint64_t totalCancelled = 0;
        uint64_t totalFailed = 0;
        uint32_t pendingTasks = 0;
        uint32_t activeTasks = 0;
        float avgExecutionTimeMs = 0.0f;
        uint32_t threadCount = 0;
    };

    AsyncTaskManager();
    ~AsyncTaskManager();

    // ========================================================================
    // Lifecycle
    // ========================================================================

    bool initialize(uint32_t threadCount = 4);
    void cleanup();

    // ========================================================================
    // Task submission
    // ========================================================================

    // Submit a task with priority and category
    template <typename Func, typename... Args>
    auto submit(Priority priority, Category category, const std::string& name,
                Func&& func, Args&&... args)
        -> std::future<typename std::invoke_result<Func, Args...>::type>;

    // Submit a simple task with default priority
    template <typename Func, typename... Args>
    auto submit(Func&& func, Args&&... args)
        -> std::future<typename std::invoke_result<Func, Args...>::type>;

    // Submit a cell load task (high priority)
    template <typename Func, typename... Args>
    auto submitCellLoad(Func&& func, Args&&... args)
        -> std::future<typename std::invoke_result<Func, Args...>::type>;

    // Submit a texture load task (normal priority)
    template <typename Func, typename... Args>
    auto submitTextureLoad(Func&& func, Args&&... args)
        -> std::future<typename std::invoke_result<Func, Args...>::type>;

    // Submit an ESM parse task (normal priority)
    template <typename Func, typename... Args>
    auto submitESMParse(Func&& func, Args&&... args)
        -> std::future<typename std::invoke_result<Func, Args...>::type>;

    // ========================================================================
    // Task management
    // ========================================================================

    // Cancel a pending task
    bool cancelTask(uint64_t taskId);

    // Wait for all pending tasks to complete
    void waitForAll(std::chrono::milliseconds timeout = std::chrono::milliseconds(5000));

    // Check if a specific task is done
    bool isTaskDone(uint64_t taskId) const;

    // ========================================================================
    // Statistics
    // ========================================================================

    TaskStats getStats() const;
    void resetStats();

    // Get task info by ID
    TaskInfo getTaskInfo(uint64_t taskId) const;

    // Get pending task count
    uint32_t getPendingCount() const;

private:
    // ========================================================================
    // Internal task wrapper
    // ========================================================================

    struct Task {
        uint64_t id;
        std::string name;
        Priority priority;
        Category category;
        std::function<void()> func;
        std::chrono::steady_clock::time_point submitTime;

        // For priority queue ordering
        bool operator<(const Task& other) const {
            return static_cast<int>(priority) > static_cast<int>(other.priority);
        }
    };

    // ========================================================================
    // Member variables
    // ========================================================================

    std::vector<std::thread> workers_;
    std::priority_queue<Task> taskQueue_;
    mutable std::mutex queueMutex_;
    std::condition_variable condition_;
    std::condition_variable completionCondition_;
    std::atomic<bool> stop_{false};
    std::atomic<uint64_t> nextTaskId_{1};
    std::atomic<uint32_t> activeTaskCount_{0};

    // Task tracking
    std::unordered_map<uint64_t, TaskInfo> taskHistory_;
    mutable std::mutex historyMutex_;

    // Statistics
    std::atomic<uint64_t> totalSubmitted_{0};
    std::atomic<uint64_t> totalCompleted_{0};
    std::atomic<uint64_t> totalCancelled_{0};
    std::atomic<uint64_t> totalFailed_{0};
    std::atomic<uint64_t> totalExecutionTimeUs_{0};

    uint32_t threadCount_ = 4;

    // ========================================================================
    // Worker thread function
    // ========================================================================

    void workerThread();

    // Record task completion
    void recordCompletion(uint64_t taskId, bool success);
};

// ============================================================================
// Template implementations
// ============================================================================

template <typename Func, typename... Args>
auto AsyncTaskManager::submit(Priority priority, Category category,
                               const std::string& name,
                               Func&& func, Args&&... args)
    -> std::future<typename std::invoke_result<Func, Args...>::type>
{
    using ReturnType = typename std::invoke_result<Func, Args...>::type;

    auto taskPtr = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind(std::forward<Func>(func), std::forward<Args>(args)...));

    std::future<ReturnType> result = taskPtr->get_future();

    uint64_t taskId = nextTaskId_.fetch_add(1);

    {
        std::unique_lock<std::mutex> lock(queueMutex_);

        if (stop_.load()) {
            LOGW_AT("Cannot submit task: AsyncTaskManager is stopped");
            return std::future<ReturnType>();
        }

        Task task;
        task.id = taskId;
        task.name = name;
        task.priority = priority;
        task.category = category;
        task.func = [taskPtr]() { (*taskPtr)(); };
        task.submitTime = std::chrono::steady_clock::now();

        // Record in history
        {
            std::lock_guard<std::mutex> histLock(historyMutex_);
            TaskInfo info;
            info.taskId = taskId;
            info.name = name;
            info.priority = priority;
            info.category = category;
            info.submitTime = task.submitTime;
            taskHistory_[taskId] = info;
        }

        taskQueue_.push(std::move(task));
        totalSubmitted_.fetch_add(1);
    }

    condition_.notify_one();
    return result;
}

template <typename Func, typename... Args>
auto AsyncTaskManager::submit(Func&& func, Args&&... args)
    -> std::future<typename std::invoke_result<Func, Args...>::type>
{
    return submit(Priority::NORMAL, Category::OTHER, "unnamed",
                  std::forward<Func>(func), std::forward<Args>(args)...);
}

template <typename Func, typename... Args>
auto AsyncTaskManager::submitCellLoad(Func&& func, Args&&... args)
    -> std::future<typename std::invoke_result<Func, Args...>::type>
{
    return submit(Priority::HIGH, Category::CELL_LOAD, "cell_load",
                  std::forward<Func>(func), std::forward<Args>(args)...);
}

template <typename Func, typename... Args>
auto AsyncTaskManager::submitTextureLoad(Func&& func, Args&&... args)
    -> std::future<typename std::invoke_result<Func, Args...>::type>
{
    return submit(Priority::NORMAL, Category::TEXTURE_LOAD, "texture_load",
                  std::forward<Func>(func), std::forward<Args>(args)...);
}

template <typename Func, typename... Args>
auto AsyncTaskManager::submitESMParse(Func&& func, Args&&... args)
    -> std::future<typename std::invoke_result<Func, Args...>::type>
{
    return submit(Priority::NORMAL, Category::ESM_PARSE, "esm_parse",
                  std::forward<Func>(func), std::forward<Args>(args)...);
}
