#include "bsa_reader.h"

#include <android/log.h>

#include <cstring>
#include <functional>
#include <utility>

#include "bsa_ffi.h"

#undef LOG_TAG
#undef LOGD
#undef LOGW
#undef LOGE
#define LOG_TAG "BSAReader"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ============================================================================
// Helpers around the bsa-ffi C ABI
// ============================================================================
namespace {

static constexpr uint32_t OB_HEADER_VERSION = 0x67;

// Grab a NUL terminated message from the Rust side. The size is queried first so
// nothing is truncated.
std::string lastErrorMessage() {
    const int needed = bsa_last_error_message(nullptr, 0);
    if (needed <= 1) {
        return std::string();
    }
    std::string message(static_cast<size_t>(needed) - 1, '\0');
    if (bsa_last_error_message(&message[0], message.size() + 1) != needed) {
        return std::string();
    }
    return message;
}

// One of an entry's strings, e.g. BSA_FIELD_LOOKUP_PATH. Returns an empty string
// when the index is out of range, which cannot happen for an index that came
// from bsa_get_files().
std::string entryText(void* handle, size_t index, int field) {
    const int needed = bsa_entry_text(handle, index, field, nullptr, 0);
    if (needed <= 1) {
        return std::string();
    }
    std::string text(static_cast<size_t>(needed) - 1, '\0');
    if (bsa_entry_text(handle, index, field, &text[0], text.size() + 1) != needed) {
        return std::string();
    }
    return text;
}

}  // namespace

// ============================================================================
// Constructor / Destructor
// ============================================================================

BSArchive::BSArchive()
    : m_handle(bsa_create())
    , m_isOpen(false) {
    std::memset(&m_header, 0, sizeof(m_header));
    if (m_handle == nullptr) {
        LOGE("Failed to allocate the BSA reader: %s", lastErrorMessage().c_str());
    }
}

BSArchive::~BSArchive() {
    if (m_handle != nullptr) {
        bsa_destroy(m_handle);
        m_handle = nullptr;
    }
}

// ============================================================================
// Open / Close
// ============================================================================

bool BSArchive::open(const std::string& filePath) {
    close();

    m_filePath = filePath;

    if (m_handle == nullptr) {
        m_filePath.clear();
        return false;
    }

    const int status = bsa_open(m_handle, filePath.c_str());
    const std::string error = lastErrorMessage();

    // The previous reader kept the path for a file it could not open, and cleared
    // it when the header never validated. Mirror that.
    if (status == BSA_ERROR_BAD_MAGIC || status == BSA_ERROR_TRUNCATED) {
        m_filePath.clear();
    }

    // Even a failed open can leave a partial index, so cache whatever the archive
    // holds before deciding what to report.
    syncCache();
    m_isOpen = bsa_is_open(m_handle) != 0;

    if (status != BSA_OK) {
        LOGE("Failed to open BSA '%s': %s", filePath.c_str(), error.c_str());
        return false;
    }

    if (m_header.version != OB_HEADER_VERSION) {
        LOGD("BSA version 0x%02X (Oblivion expects 0x%02X)", m_header.version, OB_HEADER_VERSION);
    }

    LOGD("BSA opened: %s", filePath.c_str());
    LOGD("  Version: 0x%02X, Folders: %u, Files: %u",
         m_header.version, m_header.folderCount, m_header.fileCount);
    LOGD("  ArchiveFlags: 0x%08X, Compressed: %s",
         m_header.archiveFlags,
         (m_header.archiveFlags & BSA_FLAG_COMPRESSED) ? "YES" : "NO");
    LOGD("  FolderNameLength: %u, FileNameLength: %u",
         m_header.folderNameLength, m_header.fileNameLength);

    return true;
}

void BSArchive::close() {
    if (m_handle != nullptr) {
        bsa_close(m_handle);
    }
    m_isOpen = false;
    m_files.clear();
    m_files.shrink_to_fit();
    std::memset(&m_header, 0, sizeof(m_header));
    m_filePath.clear();
}

// ============================================================================
// Cache synchronisation
// ============================================================================

bool BSArchive::syncCache() {
    m_files.clear();
    std::memset(&m_header, 0, sizeof(m_header));

    if (m_handle == nullptr) {
        return false;
    }

    bsa_header header;
    std::memset(&header, 0, sizeof(header));
    if (bsa_get_header(m_handle, &header) != BSA_OK) {
        return false;
    }
    m_header.magic = header.magic;
    m_header.version = header.version;
    m_header.folderRecordOffset = header.folder_record_offset;
    m_header.archiveFlags = header.archive_flags;
    m_header.folderCount = header.folder_count;
    m_header.fileCount = header.file_count;
    m_header.folderNameLength = header.folder_name_length;
    m_header.fileNameLength = header.file_name_length;
    m_header.fileFlags = header.file_flags;

    const size_t total = bsa_get_files(m_handle, nullptr, 0);
    if (total == 0) {
        return true;
    }

    std::vector<bsa_entry_info> info(total);
    const size_t readCount = bsa_get_files(m_handle, info.data(), info.size());
    if (readCount != total) {
        LOGW("BSA entry count changed while copying the index (%u entries, %u read)",
             static_cast<unsigned>(total), static_cast<unsigned>(readCount));
        return false;
    }

    m_files.resize(total);
    for (size_t i = 0; i < total; ++i) {
        BSAFileEntry& entry = m_files[i];
        entry.fullPath = entryText(m_handle, i, BSA_FIELD_FULL_PATH);
        entry.lookupPath = entryText(m_handle, i, BSA_FIELD_LOOKUP_PATH);
        entry.fileName = entryText(m_handle, i, BSA_FIELD_FILE_NAME);
        entry.extension = entryText(m_handle, i, BSA_FIELD_EXTENSION);
        entry.folderPath = entryText(m_handle, i, BSA_FIELD_FOLDER_PATH);
        entry.hash = info[i].hash;
        entry.offset = info[i].offset;
        entry.size = info[i].size;
        entry.realSize = info[i].real_size;
        entry.compressed = info[i].compressed != 0;
    }
    return true;
}

size_t BSArchive::indexOf(const BSAFileEntry& entry) const {
    // The accessors hand out pointers into m_files, so the common case is a
    // subtraction. std::less makes comparing against the range well defined even
    // for an entry that came from another archive.
    const BSAFileEntry* base = m_files.data();
    if (!m_files.empty()) {
        const BSAFileEntry* last = base + m_files.size();
        if (!std::less<const BSAFileEntry*>()(&entry, base) &&
            std::less<const BSAFileEntry*>()(&entry, last)) {
            return static_cast<size_t>(&entry - base);
        }
    }

    // Fall back to identity, so a copy of an entry still extracts.
    for (size_t i = 0; i < m_files.size(); ++i) {
        const BSAFileEntry& candidate = m_files[i];
        if (candidate.hash == entry.hash && candidate.offset == entry.offset &&
            candidate.size == entry.size) {
            return i;
        }
    }

    // Past the end: bsa_extract_* reports BSA_ERROR_INDEX_OUT_OF_RANGE.
    return m_files.size();
}

// ============================================================================
// Extraction
// ============================================================================

bool BSArchive::extract(bool decompress, const BSAFileEntry& entry, std::vector<uint8_t>& output) {
    if (!m_isOpen || m_handle == nullptr) {
        return false;
    }

    const size_t index = indexOf(entry);
    const uint8_t* data = nullptr;
    size_t length = 0;
    const int status = decompress
        ? bsa_extract_decompressed(m_handle, index, &data, &length)
        : bsa_extract_raw(m_handle, index, &data, &length);

    if (status != BSA_OK) {
        LOGW("BSA extract failed for entry %zu: %s", index, lastErrorMessage().c_str());
        return false;
    }

    if (length == 0 || data == nullptr) {
        output.clear();
        return true;
    }

    output.assign(data, data + length);
    return true;
}

bool BSArchive::extractFile(const BSAFileEntry& entry, std::vector<uint8_t>& output) {
    return extract(false, entry, output);
}

bool BSArchive::extractFileDecompressed(const BSAFileEntry& entry, std::vector<uint8_t>& output) {
    return extract(true, entry, output);
}

// ============================================================================
// Lookup
// ============================================================================

const BSAFileEntry* BSArchive::findFile(const std::string& path) const {
    if (!m_isOpen || m_handle == nullptr) {
        return nullptr;
    }

    size_t index = 0;
    if (bsa_find_file(m_handle, path.c_str(), path.size(), &index) != BSA_FOUND) {
        return nullptr;
    }
    if (index >= m_files.size()) {
        return nullptr;
    }
    return &m_files[index];
}

std::vector<const BSAFileEntry*> BSArchive::findFilesByPrefix(const std::string& prefix) const {
    std::vector<const BSAFileEntry*> result;
    if (!m_isOpen || m_handle == nullptr) {
        return result;
    }

    const size_t* indices = nullptr;
    size_t count = 0;
    const int found = bsa_find_files_by_prefix(
        m_handle, prefix.c_str(), prefix.size(), &indices, &count);
    if (found <= 0 || indices == nullptr) {
        return result;
    }

    result.reserve(static_cast<size_t>(found));
    for (size_t i = 0; i < count && i < static_cast<size_t>(found); ++i) {
        if (indices[i] < m_files.size()) {
            result.push_back(&m_files[indices[i]]);
        }
    }
    return result;
}

std::vector<const BSAFileEntry*> BSArchive::findFilesByExtension(const std::string& ext) const {
    std::vector<const BSAFileEntry*> result;
    if (!m_isOpen || m_handle == nullptr) {
        return result;
    }

    const size_t* indices = nullptr;
    size_t count = 0;
    const int found = bsa_find_files_by_extension(
        m_handle, ext.c_str(), ext.size(), &indices, &count);
    if (found <= 0 || indices == nullptr) {
        return result;
    }

    result.reserve(static_cast<size_t>(found));
    for (size_t i = 0; i < count && i < static_cast<size_t>(found); ++i) {
        if (indices[i] < m_files.size()) {
            result.push_back(&m_files[indices[i]]);
        }
    }
    return result;
}