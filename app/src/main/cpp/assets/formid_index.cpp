#include "formid_index.h"

#include <android/log.h>

#include "plugin_ffi.h"

#define LOG_TAG "FormIDIndex"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

// This file is a shim: the load-order translation itself lives in Rust
// (oblivion-re-tools, crates/plugin-parser) behind the plugin_ffi C ABI, and the
// signatures here are unchanged so existing callers keep working.
//
// Every call degrades towards leaving FormIDs alone. A failed rebase returns the
// FormID it was given, and a failed map build keeps the plugin's own index in
// the last entry while leaving its masters unresolved. Pointing a FormID at the
// wrong plugin is worse than not translating it, because the wrong plugin may
// well have a record at that ID.

namespace oblivion {

namespace {

// The C ABI takes an array of NUL-terminated names. The returned pointers point
// into `names`, so they stay valid as long as the caller's vector does.
std::vector<const char*> namePointers(const std::vector<std::string>& names) {
    std::vector<const char*> pointers;
    pointers.reserve(names.size());
    for (const std::string& name : names) {
        pointers.push_back(name.c_str());
    }
    return pointers;
}

}  // namespace

bool pluginNameEquals(const std::string& lhs, const std::string& rhs) {
    int equal = 0;
    const int status = plg_plugin_name_equals(lhs.c_str(), rhs.c_str(), &equal);
    if (status != PLG_OK) {
        LOGD("plg_plugin_name_equals failed with %d, falling back to an exact comparison", status);
        return lhs == rhs;
    }
    return equal != 0;
}

std::vector<uint32_t> buildFormIDIndexMap(const std::vector<std::string>& masters,
                                          const std::vector<std::string>& loadedFileNames,
                                          uint32_t selfIndex) {
    // Built before the call so that a failure leaves the plugin able to resolve
    // its own records: the last entry is always its own load-order index.
    std::vector<uint32_t> indexMap(plg_formid_index_map_len(masters.size()),
                                   kUnresolvedFormIDIndex);
    indexMap.back() = selfIndex;

    const std::vector<const char*> masterNames = namePointers(masters);
    const std::vector<const char*> loadedNames = namePointers(loadedFileNames);
    size_t written = 0;
    const int status = plg_build_formid_index_map(
        masterNames.empty() ? nullptr : masterNames.data(), masterNames.size(),
        loadedNames.empty() ? nullptr : loadedNames.data(), loadedNames.size(), selfIndex,
        indexMap.data(), indexMap.size(), &written);
    if (status != PLG_OK) {
        LOGD("plg_build_formid_index_map failed with %d, %zu master(s) stay unresolved", status,
             masters.size());
    }
    return indexMap;
}

uint32_t rebaseFormID(uint32_t formID, const std::vector<uint32_t>& indexMap) {
    uint32_t rebased = formID;
    const int status = plg_rebase_formid(formID, indexMap.empty() ? nullptr : indexMap.data(),
                                         indexMap.size(), &rebased);
    if (status != PLG_OK) {
        LOGD("plg_rebase_formid failed with %d, FormID %08X is left as it is", status, formID);
        return formID;
    }
    return rebased;
}

}  // namespace oblivion
