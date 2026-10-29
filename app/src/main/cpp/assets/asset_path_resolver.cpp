#include "asset_path_resolver.h"

#include <android/log.h>

#include <cstddef>
#include <string>
#include <vector>

#include "assets_ffi.h"

#define LOG_TAG "AssetPath"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

// This file is a shim: the expansion rules live in Rust (oblivion-re-tools,
// crates/assets-core) behind the assets_ffi C ABI, and the signatures here are
// unchanged so existing callers keep working.
//
// Both calls degrade towards the caller's own request rather than a wrong
// candidate list. A resolver that only tries the spelling the ESM asked for can
// still hit a loose file; one that tries paths the rules never produced would
// send every lookup after files that cannot exist.

const std::vector<std::string>& assetRootFolders() {
    // Built once and kept for the process lifetime: callers take a const
    // reference and walk it on every asset lookup.
    static const std::vector<std::string> kRoots = [] {
        std::vector<std::string> roots;
        const size_t count = afl_asset_root_folder_count();
        roots.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            // The ABI reports the length including the terminating NUL, so the
            // query and the fill can use the same scratch buffer.
            const int length = afl_asset_root_folder(i, nullptr, 0);
            if (length <= 0) {
                LOGD("afl_asset_root_folder(%zu) length query failed with %d", i, length);
                continue;
            }
            std::string folder(static_cast<size_t>(length), '\0');
            const int status = afl_asset_root_folder(i, folder.data(), folder.size());
            if (status <= 0 || static_cast<size_t>(status) > folder.size()) {
                LOGD("afl_asset_root_folder(%zu) failed with %d", i, status);
                continue;
            }
            folder.resize(static_cast<size_t>(status) - 1);
            roots.push_back(std::move(folder));
        }
        return roots;
    }();
    return kRoots;
}

std::vector<std::string> buildAssetPathCandidates(const std::string& normalized) {
    std::vector<std::string> original;
    original.push_back(normalized);

    // Paths travel as raw bytes, not as a C string: a record is not guaranteed to
    // be valid UTF-8, and an embedded NUL is just another byte of the name.
    const char* data = normalized.empty() ? nullptr : normalized.data();

    size_t count = 0;
    size_t bytes = 0;
    int status = afl_build_asset_path_candidates(data, normalized.size(), nullptr, 0, &count, &bytes);
    if (status != AFL_OK) {
        LOGD("afl_build_asset_path_candidates length query failed with %d, only the original "
             "spelling is tried",
             status);
        return original;
    }
    if (count == 0 || bytes == 0) return original;

    std::string block(bytes, '\0');
    status = afl_build_asset_path_candidates(data, normalized.size(), block.data(), block.size(),
                                             &count, &bytes);
    if (status != AFL_OK) {
        LOGD("afl_build_asset_path_candidates failed with %d, only the original spelling is tried",
             status);
        return original;
    }

    // The block is a packed run of NUL-terminated strings, so the only length that
    // means anything is the one the ABI reported.
    std::vector<std::string> candidates;
    candidates.reserve(count);
    size_t offset = 0;
    for (size_t i = 0; i < count; ++i) {
        const size_t end = block.find('\0', offset);
        if (end == std::string::npos) {
            LOGD("candidate %zu is not terminated inside the block, only the original spelling "
                 "is tried",
                 i);
            return original;
        }
        candidates.emplace_back(block, offset, end - offset);
        offset = end + 1;
    }
    return candidates;
}
