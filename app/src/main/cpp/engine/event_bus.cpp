// EventBus out-of-line definitions.
//
// Split out of imperial_weave.cpp: the event system is used by gameplay systems that
// must be buildable/linkable without the renderer, video and Jolt subsystems that
// imperial_weave.cpp pulls in (for example the host test harness).
#include "imperial_weave.h"

namespace weave {

// ============================================================================
// EventBus implementation (thread-safe)
// ============================================================================

void EventBus::subscribe(const std::string& type, Handler handler) {
    std::lock_guard<std::mutex> lock(handlersMutex_);
    auto it = handlers_.find(type);
    if (it == handlers_.end()) {
        it = handlers_.emplace(type, std::make_shared<std::vector<Handler>>()).first;
    }
    it->second->push_back(std::move(handler));
}

void EventBus::unsubscribe(const std::string& type) {
    std::lock_guard<std::mutex> lock(handlersMutex_);
    handlers_.erase(type);
}

void EventBus::emitImmediate(const Event& event) {
    // v3: Copy shared_ptr only (avoid vector copy)
    std::shared_ptr<std::vector<Handler>> localHandlers;
    {
        std::lock_guard<std::mutex> lock(handlersMutex_);
        auto it = handlers_.find(event.type);
        if (it == handlers_.end()) return;
        localHandlers = it->second;
    }
    for (const auto& handler : *localHandlers) {
        if (handler) handler(event);
    }
}

void EventBus::emit(const Event& event) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    queue_.push_back(event);
}

void EventBus::processQueue() {
    std::vector<Event> localQueue;
    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        if (queue_.empty()) return;
        localQueue.swap(queue_);
    }

    for (const auto& event : localQueue) {
        // v3: shared_ptr copy only
        std::shared_ptr<std::vector<Handler>> localHandlers;
        {
            std::lock_guard<std::mutex> lock(handlersMutex_);
            auto it = handlers_.find(event.type);
            if (it == handlers_.end()) continue;
            localHandlers = it->second;
        }
        for (const auto& handler : *localHandlers) {
            if (handler) handler(event);
        }
    }
}

void EventBus::clearQueue() {
    std::lock_guard<std::mutex> lock(queueMutex_);
    queue_.clear();
}

size_t EventBus::getQueueSize() const {
    std::lock_guard<std::mutex> lock(queueMutex_);
    return queue_.size();
}

}  // namespace weave
