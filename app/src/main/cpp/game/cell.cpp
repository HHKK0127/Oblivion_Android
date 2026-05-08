#include "cell.h"
#include <android/log.h>

#define LOG_TAG "Cell"
#ifdef ENABLE_DEBUG_LOGS
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#else
#define LOGD(...) do {} while(0)
#endif
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

Cell::Cell(uint32_t cellId, const std::string& cellName, CellType type)
    : cellId(cellId), cellName(cellName), cellType(type), gridX(0), gridY(0),
      boundsMin(-0.5f, -0.5f, -0.5f), boundsMax(0.5f, 0.5f, 0.5f), loaded(false) {
    logCellInfo();
}

Cell::~Cell() {
    unloadObjects();
    LOGD("Cell destroyed: %s", cellName.c_str());
}

void Cell::addObject(std::shared_ptr<WorldObject> obj) {
    if (obj) {
        objects.push_back(obj);
        LOGD("Object added to cell %s: refId=%u, position=(%.2f, %.2f, %.2f)",
             cellName.c_str(), obj->refId, obj->position.x, obj->position.y, obj->position.z);
    }
}

void Cell::removeObject(uint32_t refId) {
    auto it = std::find_if(objects.begin(), objects.end(),
        [refId](const std::shared_ptr<WorldObject>& obj) { return obj->refId == refId; });

    if (it != objects.end()) {
        objects.erase(it);
        LOGD("Object removed from cell %s: refId=%u", cellName.c_str(), refId);
    }
}

std::shared_ptr<WorldObject> Cell::getObject(uint32_t refId) {
    auto it = std::find_if(objects.begin(), objects.end(),
        [refId](const std::shared_ptr<WorldObject>& obj) { return obj->refId == refId; });

    if (it != objects.end()) {
        return *it;
    }
    return nullptr;
}

void Cell::loadObjects() {
    if (loaded) return;

    LOGD("Loading objects for cell: %s (count=%zu)", cellName.c_str(), objects.size());

    // Mark all objects as visible
    for (auto& obj : objects) {
        obj->visible = true;
        LOGD("  Object loaded: refId=%u, visible=true", obj->refId);
    }

    loaded = true;
    LOGD("Cell loaded: %s (objects=%zu)", cellName.c_str(), objects.size());
}

void Cell::unloadObjects() {
    if (!loaded) return;

    LOGD("Unloading objects for cell: %s", cellName.c_str());

    // Release mesh references
    for (auto& obj : objects) {
        obj->mesh = nullptr;
        // TODO: Unload from asset manager when reference counting is complete
    }

    loaded = false;
    LOGD("Cell unloaded: %s", cellName.c_str());
}

void Cell::logCellInfo() const {
    const char* typeStr = (cellType == CellType::INTERIOR) ? "INTERIOR" : "EXTERIOR";

    if (cellType == CellType::EXTERIOR) {
        LOGD("Cell created: ID=%u, Name=%s, Type=%s, Grid=(%d,%d)",
             cellId, cellName.c_str(), typeStr, gridX, gridY);
    } else {
        LOGD("Cell created: ID=%u, Name=%s, Type=%s",
             cellId, cellName.c_str(), typeStr);
    }
}
