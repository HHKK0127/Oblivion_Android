#include "plugin_list.h"

#include <android/log.h>

#include "plugin_ffi.h"

#define LOG_TAG "PluginList"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

// This file is a shim: the grammar and the load-order rules live in Rust
// (oblivion-re-tools, crates/plugin-parser) behind the plugin_ffi C ABI, and the
// signatures here are unchanged so existing callers keep working.
//
// Every call degrades towards an empty result rather than a wrong one. Parsing
// and active-selection report nothing when the ABI refuses the call,
// serialisation returns "", and a failed sort leaves the entries in the order it
// found them. Writing a half-validated list back over the user's plugins.txt, or
// loading plugins in an order the engine did not ask for, is the worse failure.

namespace oblivion {

namespace {

// The C ABI takes parallel arrays of NUL-terminated names and active flags. The
// pointers refer to the caller's strings, so they stay valid as long as the
// caller's vector does.
struct PluginArrays {
    std::vector<const char*> names;
    std::vector<int> activeFlags;
};

PluginArrays collectPluginArrays(const std::vector<PluginListEntry>& entries) {
    PluginArrays arrays;
    arrays.names.reserve(entries.size());
    arrays.activeFlags.reserve(entries.size());
    for (const PluginListEntry& entry : entries) {
        arrays.names.push_back(entry.name.c_str());
        arrays.activeFlags.push_back(entry.active ? 1 : 0);
    }
    return arrays;
}

// Only reachable when the FFI call itself fails, which a live std::string cannot
// cause; it keeps the classifier total instead of misleadingly reporting false.
bool endsWithEsm(const std::string& name) {
    if (name.size() < 4) return false;
    static const char kExtension[] = ".esm";
    const size_t begin = name.size() - 4;
    for (size_t i = 0; i < 4; ++i) {
        const unsigned char c = static_cast<unsigned char>(name[begin + i]);
        const char lowered =
            (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : static_cast<char>(c);
        if (lowered != kExtension[i]) return false;
    }
    return true;
}

}  // namespace

bool isMasterFileName(const std::string& name) {
    int isMaster = 0;
    const int status = plg_is_master_file_name(name.c_str(), &isMaster);
    if (status != PLG_OK) {
        LOGD("plg_is_master_file_name failed with %d, testing the extension here", status);
        return endsWithEsm(name);
    }
    return isMaster != 0;
}

std::vector<PluginListEntry> parsePluginList(const std::string& contents) {
    std::vector<PluginListEntry> entries;
    // The entries only carry offsets into `contents`, so a NULL buffer with a
    // length of 0 is still a valid empty parse.
    const char* data = contents.empty() ? nullptr : contents.data();

    size_t count = 0;
    int status = plg_parse_plugin_list(data, contents.size(), nullptr, 0, &count);
    if (status != PLG_OK) {
        LOGD("plg_parse_plugin_list length query failed with %d, no plugin is read", status);
        return entries;
    }
    if (count == 0) return entries;

    std::vector<plg_plugin_list_entry> raw(count);
    status = plg_parse_plugin_list(data, contents.size(), raw.data(), raw.size(), &count);
    if (status != PLG_OK) {
        LOGD("plg_parse_plugin_list failed with %d, no plugin is read", status);
        return entries;
    }

    entries.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        // The offsets come from the same buffer, but nothing may make the copy
        // read past the string the caller handed over.
        if (raw[i].name_offset > contents.size() ||
            raw[i].name_length > contents.size() - raw[i].name_offset) {
            LOGD("entry %zu points outside the file, dropping it", i);
            continue;
        }
        PluginListEntry entry;
        entry.name.assign(contents, raw[i].name_offset, raw[i].name_length);
        entry.active = raw[i].active != 0;
        entries.push_back(std::move(entry));
    }
    return entries;
}

std::string serializePluginList(const std::vector<PluginListEntry>& entries) {
    const PluginArrays arrays = collectPluginArrays(entries);
    const char* const* names = arrays.names.empty() ? nullptr : arrays.names.data();
    const int* flags = arrays.activeFlags.empty() ? nullptr : arrays.activeFlags.data();

    size_t length = 0;
    int status = plg_serialize_plugin_list(names, flags, arrays.names.size(), nullptr, 0, &length);
    if (status != PLG_OK) {
        LOGD("plg_serialize_plugin_list length query failed with %d, nothing is written", status);
        return std::string();
    }
    std::string out(length, '\0');
    if (length == 0) return out;

    status = plg_serialize_plugin_list(names, flags, arrays.names.size(), out.data(), out.size(),
                                       &length);
    if (status != PLG_OK) {
        LOGD("plg_serialize_plugin_list failed with %d, nothing is written", status);
        return std::string();
    }
    // The result is raw file text, not a C string: only the reported length counts.
    out.resize(length);
    return out;
}

std::vector<std::string> activePluginNames(const std::vector<PluginListEntry>& entries) {
    const PluginArrays arrays = collectPluginArrays(entries);
    const char* const* names = arrays.names.empty() ? nullptr : arrays.names.data();
    const int* flags = arrays.activeFlags.empty() ? nullptr : arrays.activeFlags.data();

    size_t count = 0;
    int status = plg_active_plugin_indices(names, flags, arrays.names.size(), nullptr, 0, &count);
    if (status != PLG_OK) {
        LOGD("plg_active_plugin_indices length query failed with %d, no plugin is listed", status);
        return std::vector<std::string>();
    }

    std::vector<std::string> activeNames;
    if (count == 0) return activeNames;

    std::vector<size_t> indices(count);
    status = plg_active_plugin_indices(names, flags, arrays.names.size(), indices.data(),
                                       indices.size(), &count);
    if (status != PLG_OK) {
        LOGD("plg_active_plugin_indices failed with %d, no plugin is listed", status);
        return activeNames;
    }

    activeNames.reserve(count);
    for (size_t i = 0; i < count; ++i) {
        if (indices[i] >= entries.size()) {
            LOGD("active index %zu is out of range, dropping it", indices[i]);
            continue;
        }
        activeNames.push_back(entries[indices[i]].name);
    }
    return activeNames;
}

void sortPluginLoadOrder(std::vector<PluginListEntry>& entries) {
    const PluginArrays arrays = collectPluginArrays(entries);
    const size_t count = arrays.names.size();

    std::vector<size_t> order(count);
    size_t written = 0;
    const int status =
        plg_sort_plugin_load_order(arrays.names.data(), count, order.data(), order.size(), &written);
    if (status != PLG_OK) {
        LOGD("plg_sort_plugin_load_order failed with %d, the order is left alone", status);
        return;
    }

    // The permutation is applied to a copy and only swapped in once every index
    // checked out, so a rejected call cannot half-reorder the caller's vector.
    std::vector<PluginListEntry> sorted;
    sorted.reserve(written);
    for (size_t i = 0; i < written; ++i) {
        if (order[i] >= count) {
            LOGD("load order index %zu is out of range, the order is left alone", order[i]);
            return;
        }
        sorted.push_back(entries[order[i]]);
    }
    entries.swap(sorted);
}

}  // namespace oblivion
