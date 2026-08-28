#include "async_task_manager.h"

// ============================================================================
// AsyncTaskManager Implementation
// ============================================================================

AsyncTaskManager::AsyncTaskManager() = default;

AsyncTaskManager::~AsyncTaskManager() {
    cleanup();
}

bool AsyncTaskManager::initialize(uint32_t threadCount) {
    if (!workers_.empty()) {
        LOGW_AT("AsyncTaskManager already initialized");
        return true;
    }

    threadCount_ = threadCount;
    stop_.store(false);

    LOGI_AT("Initializing AsyncTaskManager with %u threads", threadCount);

    for (uint32_t i = 0; i < threadCount; ++i) {
        workers_.emplace_back(&AsyncTaskManager::workerThread, this);
    }

    LOGI_AT("AsyncTaskManager initialized successfully");
    return true;
}

void AsyncTaskManager::cleanup() {
    {
        std::unique_lock<std::mutex> lock(queueMutex_);
        stop_.store(true);
    }

    condition_.notify_all();

    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    workers_.clear();

    // Clear remaining tasks
    while (!taskQueue_.empty()) {
        taskQueue_.pop();
    }

    LOGI_AT("AsyncTaskManager cleaned up (completed=%llu, cancelled=%llu, failed=%llu)",
            static_cast<unsigned long long>(totalCompleted_.load()),
            static_cast<unsigned long long>(totalCancelled_.load()),
            static_cast<unsigned long long>(totalFailed_.load()));
}

void AsyncTaskManager::workerThread() {
    while (true) {
        Task task;
        {
            std::unique_lock<std::mutex> lock(queueMutex_);

            condition_.wait(lock, [this]() {
                return stop_.load() || !taskQueue_.empty();
            });

            if (stop_.load() && taskQueue_.empty()) {
                return;
            }

            task = std::move(const_cast<Task&>(taskQueue_.top()));
            taskQueue_.pop();
        }

        // Execute task
        activeTaskCount_.fetch_add(1);

        // Record start time
        {
            std::lock_guard<std::mutex> histLock(historyMutex_);
            auto it = taskHistory_.find(task.id);
            if (it != taskHistory_.end()) {
                it->second.startTime = std::chrono::steady_clock::now();
            }
        }

        auto startTime = std::chrono::steady_clock::now();

        bool success = true;
        try {
            task.func();
        } catch (const std::exception& e) {
            LOGE_AT("Task '%s' (id=%llu) failed: %s",
                    task.name.c_str(),
                    static_cast<unsigned long long>(task.id),
                    e.what());
            success = false;
            totalFailed_.fetch_add(1);
        } catch (...) {
            LOGE_AT("Task '%s' (id=%llu) failed with unknown error",
                    task.name.c_str(),
                    static_cast<unsigned long long>(task.id));
            success = false;
            totalFailed_.fetch_add(1);
        }

        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            endTime - startTime);
        totalExecutionTimeUs_.fetch_add(duration.count());

        activeTaskCount_.fetch_sub(1);
        recordCompletion(task.id, success);

        // Notify completion waiters
        completionCondition_.notify_all();
    }
}

void AsyncTaskManager::recordCompletion(uint64_t taskId, bool success) {
    std::lock_guard<std::mutex> histLock(historyMutex_);
    auto it = taskHistory_.find(taskId);
    if (it != taskHistory_.end()) {
        it->second.endTime = std::chrono::steady_clock::now();
        it->second.completed = success;
    }
    if (success) {
        totalCompleted_.fetch_add(1);
    }
}

bool AsyncTaskManager::cancelTask(uint64_t taskId) {
    std::lock_guard<std::mutex> histLock(historyMutex_);
    auto it = taskHistory_.find(taskId);
    if (it != taskHistory_.end() && !it->second.completed) {
        it->second.cancelled = true;
        totalCancelled_.fetch_add(1);
        return true;
    }
    return false;
}

void AsyncTaskManager::waitForAll(std::chrono::milliseconds timeout) {
    auto deadline = std::chrono::steady_clock::now() + timeout;

    while (true) {
        {
            std::unique_lock<std::mutex> lock(queueMutex_);
            if (taskQueue_.empty() && activeTaskCount_.load() == 0) {
                return;
            }
        }

        if (std::chrono::steady_clock::now() >= deadline) {
            LOGW_AT("waitForAll timed out after %lld ms",
                    static_cast<long long>(timeout.count()));
            return;
        }

        std::unique_lock<std::mutex> lock(queueMutex_);
        completionCondition_.wait_for(lock, std::chrono::milliseconds(100));
    }
}

bool AsyncTaskManager::isTaskDone(uint64_t taskId) const {
    std::lock_guard<std::mutex> histLock(historyMutex_);
    auto it = taskHistory_.find(taskId);
    if (it != taskHistory_.end()) {
        return it->second.completed || it->second.cancelled;
    }
    return true; // Unknown task treated as done
}

AsyncTaskManager::TaskStats AsyncTaskManager::getStats() const {
    TaskStats stats;
    stats.totalSubmitted = totalSubmitted_.load();
    stats.totalCompleted = totalCompleted_.load();
    stats.totalCancelled = totalCancelled_.load();
    stats.totalFailed = totalFailed_.load();
    stats.activeTasks = activeTaskCount_.load();
    stats.threadCount = threadCount_;

    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        stats.pendingTasks = static_cast<uint32_t>(taskQueue_.size());
    }

    uint64_t completed = totalCompleted_.load();
    if (completed > 0) {
        stats.avgExecutionTimeMs =
            static_cast<float>(totalExecutionTimeUs_.load()) /
            static_cast<float>(completed) / 1000.0f;
    }

    return stats;
}

void AsyncTaskManager::resetStats() {
    totalSubmitted_.store(0);
    totalCompleted_.store(0);
    totalCancelled_.store(0);
    totalFailed_.store(0);
    totalExecutionTimeUs_.store(0);
}

AsyncTaskManager::TaskInfo AsyncTaskManager::getTaskInfo(uint64_t taskId) const {
    std::lock_guard<std::mutex> histLock(historyMutex_);
    auto it = taskHistory_.find(taskId);
    if (it != taskHistory_.end()) {
        return it->second;
    }
    return TaskInfo{};
}

uint32_t AsyncTaskManager::getPendingCount() const {
    std::lock_guard<std::mutex> lock(queueMutex_);
    return static_cast<uint32_t>(taskQueue_.size());
}
