#include "bsa_reader.h"

#include <android/log.h>
#include <cstring>
#include <algorithm>
#include <sstream>
#include <zlib.h>

#undef LOG_TAG
#undef LOGD
#undef LOGE
#define LOG_TAG "BSAReader"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// ============================================================================
// BSA Magic Constants
// ============================================================================
static constexpr uint32_t BSA_MAGIC = 0x00415342; // "BSA\0"
static constexpr uint32_t OB_HEADER_VERSION = 0x67;

// ============================================================================
// Constructor / Destructor
// ============================================================================

BSArchive::BSArchive()
    : m_isOpen(false)
    , m_parsed(false) {
    std::memset(&m_header, 0, sizeof(m_header));
}

BSArchive::~BSArchive() {
    close();
}

// ============================================================================
// Open / Close
// ============================================================================

bool BSArchive::open(const std::string& filePath) {
    close();

    m_filePath = filePath;

    // Open file
    m_stream.open(filePath, std::ios::binary);
    if (!m_stream.is_open()) {
        LOGE("Failed to open BSA file: %s", filePath.c_str());
        return false;
    }

    // Read header
    m_stream.read(reinterpret_cast<char*>(&m_header.magic), sizeof(m_header.magic));
    m_stream.read(reinterpret_cast<char*>(&m_header.version), sizeof(m_header.version));
    m_stream.read(reinterpret_cast<char*>(&m_header.folderRecordOffset), sizeof(m_header.folderRecordOffset));
    m_stream.read(reinterpret_cast<char*>(&m_header.archiveFlags), sizeof(m_header.archiveFlags));
    m_stream.read(reinterpret_cast<char*>(&m_header.folderCount), sizeof(m_header.folderCount));
    m_stream.read(reinterpret_cast<char*>(&m_header.fileCount), sizeof(m_header.fileCount));
    m_stream.read(reinterpret_cast<char*>(&m_header.folderNameLength), sizeof(m_header.folderNameLength));
    m_stream.read(reinterpret_cast<char*>(&m_header.fileNameLength), sizeof(m_header.fileNameLength));
    m_stream.read(reinterpret_cast<char*>(&m_header.fileFlags), sizeof(m_header.fileFlags));

    // Validate magic
    if (m_header.magic != BSA_MAGIC) {
        LOGE("Invalid BSA magic: 0x%08X (expected 0x%08X)", m_header.magic, BSA_MAGIC);
        close();
        return false;
    }

    // Validate version
    if (m_header.version != OB_HEADER_VERSION) {
        LOGD("BSA version 0x%02X (Oblivion expects 0x%02X)", m_header.version, OB_HEADER_VERSION);
        // Non-Oblivion version but still try to parse
    }

    LOGD("BSA opened: %s", filePath.c_str());
    LOGD("  Version: 0x%02X, Folders: %u, Files: %u",
         m_header.version, m_header.folderCount, m_header.fileCount);
    LOGD("  ArchiveFlags: 0x%08X, Compressed: %s",
         m_header.archiveFlags,
         (m_header.archiveFlags & BSA_FLAG_COMPRESSED) ? "YES" : "NO");
    LOGD("  FolderNameLength: %u, FileNameLength: %u",
         m_header.folderNameLength, m_header.fileNameLength);

    m_isOpen = true;

    // Parse the archive structure
    return parseArchive();
}

void BSArchive::close() {
    if (m_stream.is_open()) {
        m_stream.close();
    }
    m_isOpen = false;
    m_parsed = false;
    m_folders.clear();
    m_files.clear();
    m_folderNames.clear();
    m_nameTable.clear();
    std::memset(&m_header, 0, sizeof(m_header));
    m_filePath.clear();
}

// ============================================================================
// Archive Parsing
// ============================================================================

bool BSArchive::parseArchive() {
    if (!m_isOpen) {
        LOGE("Archive not open");
        return false;
    }

    // Step 1: Parse folder records
    if (!parseFolderRecords()) {
        LOGE("Failed to parse folder records");
        return false;
    }

    // Step 2: Read name table at the end of metadata
    if (!readNameTable()) {
        LOGE("Failed to read name table");
        return false;
    }

    m_parsed = true;
    LOGD("BSA parsed successfully: %zu files indexed", m_files.size());
    return true;
}

bool BSArchive::parseFolderRecords() {
    // Seek to folder record offset
    m_stream.seekg(m_header.folderRecordOffset, std::ios::beg);
    if (!m_stream.good()) {
        LOGE("Failed to seek to folder records at offset %u", m_header.folderRecordOffset);
        return false;
    }

    // Read folder records
    m_folders.reserve(m_header.folderCount);
    for (uint32_t i = 0; i < m_header.folderCount; i++) {
        BSAFolderRecord folder;
        m_stream.read(reinterpret_cast<char*>(&folder.hash), sizeof(folder.hash));
        m_stream.read(reinterpret_cast<char*>(&folder.fileCount), sizeof(folder.fileCount));
        m_stream.read(reinterpret_cast<char*>(&folder.offset), sizeof(folder.offset));

        if (!m_stream.good()) {
            LOGE("Failed to read folder record %u", i);
            return false;
        }

        m_folders.push_back(folder);
    }

    LOGD("Read %zu folder records", m_folders.size());

    // Read folder name strings (each name is length-prefixed with byte len - 1, null-terminated)
    // The offset field of each folder record points into this string pool
    // The string pool starts right after the folder records
    // We read all folder names sequentially
    for (uint32_t i = 0; i < m_header.folderCount; i++) {
        uint8_t nameLenByte;
        m_stream.read(reinterpret_cast<char*>(&nameLenByte), sizeof(nameLenByte));
        if (!m_stream.good()) {
            LOGE("Failed to read folder name length for folder %u", i);
            return false;
        }

        // The length byte stores (actualLength - 1), or offset into 0x10000 pool
        // For Oblivion BSA, it's typically (length - 1)
        uint8_t actualLen = nameLenByte + 1;

        std::string folderName;
        if (actualLen > 0) {
            std::vector<char> nameBuf(actualLen + 1, 0);
            m_stream.read(nameBuf.data(), actualLen);
            if (!m_stream.good()) {
                LOGE("Failed to read folder name for folder %u", i);
                return false;
            }
            folderName = std::string(nameBuf.data(), actualLen);
        }

        // Skip null terminator
        char nullTerm;
        m_stream.read(&nullTerm, 1);

        m_folderNames[m_folders[i].offset] = folderName;
    }

    LOGD("Read %zu folder names", m_folderNames.size());

    // Pre-allocate file entries
    m_files.reserve(m_header.fileCount);

    // Read file records for each folder
    // After folder names, we have the file records.
    // Each folder has a list of file records (hash + size + offset).
    // The file records immediately follow the folder name strings.
    for (uint32_t i = 0; i < m_header.folderCount; i++) {
        const auto& folder = m_folders[i];
        auto folderIt = m_folderNames.find(folder.offset);
        std::string folderPath = (folderIt != m_folderNames.end()) ? folderIt->second : "";

        for (uint32_t j = 0; j < folder.fileCount; j++) {
            BSAFileRecord fileRec;
            m_stream.read(reinterpret_cast<char*>(&fileRec.hash), sizeof(fileRec.hash));
            m_stream.read(reinterpret_cast<char*>(&fileRec.size), sizeof(fileRec.size));
            m_stream.read(reinterpret_cast<char*>(&fileRec.offset), sizeof(fileRec.offset));

            if (!m_stream.good()) {
                LOGE("Failed to read file record %u in folder %u", j, i);
                return false;
            }

            // Determine compression state
            bool compressed = (m_header.archiveFlags & BSA_FLAG_COMPRESSED) != 0;
            if (fileRec.size & BSA_SIZE_COMPRESS_TOGGLE) {
                compressed = !compressed;
            }
            uint32_t actualSize = fileRec.size & ~BSA_SIZE_COMPRESS_TOGGLE;

            BSAFileEntry entry;
            entry.hash = fileRec.hash;
            entry.offset = fileRec.offset;
            entry.size = actualSize;
            entry.compressed = compressed;
            entry.folderPath = folderPath;
            // FullPath will be set from name table
            // Temporary path using folder
            if (!folderPath.empty()) {
                entry.fullPath = folderPath + "\\";
            }
            // We'll store the raw name when we read the name table
            // For now, store index so name table can fill it in

            m_files.push_back(entry);
        }
    }

    LOGD("Read %zu file records", m_files.size());
    return true;
}

bool BSArchive::readNameTable() {
    // The name table contains null-terminated strings in the format:
    // "folder\filename.ext" for each file entry, in the same order as file records.
    // It's located at the current stream position (after all file records).

    // If the archive has no name table flag, we can't get full paths
    bool hasNameTable = (m_header.archiveFlags & BSA_FLAG_HAS_NAMETABLE) != 0 &&
                        (m_header.archiveFlags & BSA_FLAG_HAS_FOLDERNAMES) != 0;

    if (!hasNameTable) {
        LOGD("Archive has no name table, using folder-based paths");
        // Build paths from folder + hash-based names
        uint32_t fileIdx = 0;
        for (uint32_t i = 0; i < m_header.folderCount && fileIdx < m_files.size(); i++) {
            for (uint32_t j = 0; j < m_folders[i].fileCount && fileIdx < m_files.size(); j++) {
                auto& entry = m_files[fileIdx];
                // Use folder path + hash as name
                std::string path = entry.folderPath;
                if (!path.empty()) {
                    path += "\\";
                }
                path += std::to_string(m_folders[i].hash ^ entry.hash) + ".dat";

                entry.fullPath = path;
                entry.fileName = entry.fullPath.substr(entry.fullPath.find_last_of("\\/") + 1);
                size_t dotPos = entry.fileName.find_last_of('.');
                entry.extension = (dotPos != std::string::npos)
                    ? entry.fileName.substr(dotPos + 1) : "";

                // Convert extension to lowercase
                std::transform(entry.extension.begin(), entry.extension.end(),
                               entry.extension.begin(), ::tolower);

                fileIdx++;
            }
        }
        return true;
    }

    // Read name table - one null-terminated string per file
    m_nameTable.reserve(m_header.fileCount);
    for (uint32_t i = 0; i < m_header.fileCount && i < m_files.size(); i++) {
        std::string name;
        char c;
        while (m_stream.get(c)) {
            if (c == '\0') break;
            name += c;
        }

        if (!m_stream.good() && name.empty()) {
            LOGE("Failed to read name table entry %u", i);
            return false;
        }

        m_nameTable.push_back(name);

        // Assign to file entry
        auto& entry = m_files[i];
        entry.fullPath = name;

        // Extract file name (last component after \ or /)
        size_t lastSep = entry.fullPath.find_last_of("\\/");
        if (lastSep != std::string::npos) {
            entry.fileName = entry.fullPath.substr(lastSep + 1);
        } else {
            entry.fileName = entry.fullPath;
        }

        // Extract extension
        size_t dotPos = entry.fileName.find_last_of('.');
        if (dotPos != std::string::npos) {
            entry.extension = entry.fileName.substr(dotPos + 1);
            std::transform(entry.extension.begin(), entry.extension.end(),
                           entry.extension.begin(), ::tolower);
        }
    }

    LOGD("Read %zu name table entries", m_nameTable.size());
    return true;
}

// ============================================================================
// File Extraction
// ============================================================================

bool BSArchive::extractFile(const BSAFileEntry& entry, std::vector<uint8_t>& output) {
    if (!m_isOpen) {
        LOGE("Archive not open");
        return false;
    }

    // Seek to file data offset
    m_stream.seekg(entry.offset, std::ios::beg);
    if (!m_stream.good()) {
        LOGE("Failed to seek to file data at offset %u", entry.offset);
        return false;
    }

    // Read raw data
    output.resize(entry.size);
    m_stream.read(reinterpret_cast<char*>(output.data()), entry.size);

    if (!m_stream.good()) {
        LOGE("Failed to read file data at offset %u (size %u)", entry.offset, entry.size);
        return false;
    }

    return true;
}

bool BSArchive::extractFileDecompressed(const BSAFileEntry& entry, std::vector<uint8_t>& output) {
    if (!m_isOpen) {
        LOGE("Archive not open");
        return false;
    }

    // Seek to file data offset
    m_stream.seekg(entry.offset, std::ios::beg);
    if (!m_stream.good()) {
        LOGE("Failed to seek to file data at offset %u", entry.offset);
        return false;
    }

    if (!entry.compressed) {
        // Not compressed, just read raw data
        output.resize(entry.size);
        m_stream.read(reinterpret_cast<char*>(output.data()), entry.size);
        return m_stream.good();
    }

    // Compressed: first 4 bytes are the uncompressed size
    uint32_t uncompressedSize;
    m_stream.read(reinterpret_cast<char*>(&uncompressedSize), sizeof(uncompressedSize));
    if (!m_stream.good()) {
        LOGE("Failed to read uncompressed size");
        return false;
    }

    // Remaining data is ZLib compressed
    uint32_t compressedDataSize = entry.size - 4;
    std::vector<uint8_t> compressedData(compressedDataSize);
    m_stream.read(reinterpret_cast<char*>(compressedData.data()), compressedDataSize);
    if (!m_stream.good()) {
        LOGE("Failed to read compressed data");
        return false;
    }

    // Decompress with ZLib
    return decompressZLib(compressedData.data(), compressedDataSize,
                          output, uncompressedSize);
}

bool BSArchive::decompressZLib(const uint8_t* input, size_t inputSize,
                                std::vector<uint8_t>& output, size_t uncompressedSize) const {
    z_stream strm;
    std::memset(&strm, 0, sizeof(strm));

    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = static_cast<uInt>(inputSize);
    strm.next_in = const_cast<uint8_t*>(input);

    // Try raw inflate first (no zlib header), fall back to window-bits + auto header
    int windowBits = -MAX_WBITS;  // Raw inflate
    int ret = inflateInit2(&strm, windowBits);
    if (ret != Z_OK) {
        inflateEnd(&strm);
        std::memset(&strm, 0, sizeof(strm));
        strm.zalloc = Z_NULL;
        strm.zfree = Z_NULL;
        strm.opaque = Z_NULL;
        strm.avail_in = static_cast<uInt>(inputSize);
        strm.next_in = const_cast<uint8_t*>(input);

        // Retry with auto-detect zlib/gzip header
        ret = inflateInit2(&strm, 15 + 32);
        if (ret != Z_OK) {
            LOGE("inflateInit2 failed: %d", ret);
            return false;
        }
    }

    output.clear();
    output.reserve(uncompressedSize > 0 ? uncompressedSize : ZLIB_CHUNK_SIZE);

    std::vector<uint8_t> buffer(ZLIB_CHUNK_SIZE);

    do {
        strm.avail_out = static_cast<uInt>(buffer.size());
        strm.next_out = buffer.data();

        ret = inflate(&strm, Z_NO_FLUSH);

        if (ret == Z_STREAM_ERROR || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
            LOGE("inflate error: %d", ret);
            inflateEnd(&strm);
            return false;
        }

        size_t bytesProduced = buffer.size() - strm.avail_out;
        output.insert(output.end(), buffer.data(), buffer.data() + bytesProduced);
    } while (ret != Z_STREAM_END);

    inflateEnd(&strm);
    return true;
}

// ============================================================================
// File Lookup
// ============================================================================

std::vector<const BSAFileEntry*> BSArchive::findFilesByPrefix(const std::string& prefix) const {
    std::vector<const BSAFileEntry*> results;
    for (const auto& entry : m_files) {
        // Case-insensitive prefix comparison
        if (entry.fullPath.size() >= prefix.size()) {
            std::string entryPrefix = entry.fullPath.substr(0, prefix.size());
            std::string lowerPrefix = prefix;
            std::transform(entryPrefix.begin(), entryPrefix.end(), entryPrefix.begin(), ::tolower);
            std::transform(lowerPrefix.begin(), lowerPrefix.end(), lowerPrefix.begin(), ::tolower);
            if (entryPrefix == lowerPrefix) {
                results.push_back(&entry);
            }
        }
    }
    return results;
}

const BSAFileEntry* BSArchive::findFile(const std::string& path) const {
    // Case-insensitive search
    std::string lowerPath = path;
    std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), ::tolower);

    for (const auto& entry : m_files) {
        std::string lowerEntry = entry.fullPath;
        std::transform(lowerEntry.begin(), lowerEntry.end(), lowerEntry.begin(), ::tolower);
        if (lowerEntry == lowerPath) {
            return &entry;
        }
    }
    return nullptr;
}

std::vector<const BSAFileEntry*> BSArchive::findFilesByExtension(const std::string& ext) const {
    std::vector<const BSAFileEntry*> results;
    std::string lowerExt = ext;
    std::transform(lowerExt.begin(), lowerExt.end(), lowerExt.begin(), ::tolower);

    for (const auto& entry : m_files) {
        if (entry.extension == lowerExt) {
            results.push_back(&entry);
        }
    }
    return results;
}
