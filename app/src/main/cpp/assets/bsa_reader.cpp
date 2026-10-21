#include "bsa_reader.h"

#include <android/log.h>
#include <cstring>
#include <algorithm>
#include <sstream>
#include <sys/stat.h>
#include <zlib.h>

#undef LOG_TAG
#undef LOGD
#undef LOGW
#undef LOGE
#define LOG_TAG "BSAReader"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
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
    : m_nameBlockOffset(0)
    , m_isOpen(false)
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
        struct stat st;
        if (stat(filePath.c_str(), &st) != 0) {
            LOGW("BSA file not found: %s", filePath.c_str());
        } else {
            LOGE("Failed to open BSA file (exists but unreadable): %s", filePath.c_str());
        }
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
    m_fileHashIndex.clear();
    m_nameBlockOffset = 0;
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

    // Build hash-based file lookup index for O(1) findFile().
        m_fileHashIndex.clear();
        m_fileHashIndex.reserve(m_files.size());
        for (const auto& entry : m_files) {
            m_fileHashIndex[entry.hash] = &entry;
        }

        m_parsed = true;
        LOGD("BSA parsed successfully: %zu files indexed, %zu in hash index",
             m_files.size(), m_fileHashIndex.size());
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

    // In the Oblivion layout each folder's name and file records form a single
    // contiguous block, and the blocks appear in folder-record order. The
    // offset stored in a folder record is biased by the total file name length,
    // so the real file position is (offset - fileNameLength).
    m_folderNames.assign(m_header.folderCount, std::string());
    m_files.reserve(m_header.fileCount);

    uint32_t blockEnd = m_header.folderRecordOffset + m_header.folderCount * 16;

    for (uint32_t i = 0; i < m_header.folderCount; i++) {
        const auto& folder = m_folders[i];

        uint32_t blockOffset = folder.offset;
        if (blockOffset >= m_header.fileNameLength) {
            blockOffset -= m_header.fileNameLength;
        }

        m_stream.seekg(blockOffset, std::ios::beg);
        if (!m_stream.good()) {
            LOGE("Failed to seek to folder block %u at offset %u", i, blockOffset);
            return false;
        }

        // Folder name is a bzstring: the length byte includes the trailing null.
        std::string folderPath;
        if (m_header.archiveFlags & BSA_FLAG_HAS_FOLDERNAMES) {
            uint8_t lenByte = 0;
            m_stream.read(reinterpret_cast<char*>(&lenByte), 1);
            if (!m_stream.good()) {
                LOGE("Failed to read folder name length for folder %u", i);
                return false;
            }

            if (lenByte > 1) {
                std::vector<char> nameBuf(lenByte, 0);
                m_stream.read(nameBuf.data(), lenByte);
                if (!m_stream.good()) {
                    LOGE("Failed to read folder name for folder %u", i);
                    return false;
                }
                folderPath.assign(nameBuf.data());
            }
        }
        m_folderNames[i] = folderPath;

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
            uint32_t actualSize = fileRec.size & BSA_SIZE_MASK;

            BSAFileEntry entry;
            entry.hash = fileRec.hash;
            entry.offset = fileRec.offset;
            entry.size = actualSize;
            // Uncompressed entries store their real size directly; compressed
            // entries carry the uncompressed size in a 4-byte prefix that is
            // only read when the entry is extracted.
            entry.realSize = compressed ? 0 : actualSize;
            entry.compressed = compressed;
            entry.folderPath = folderPath;

            m_files.push_back(entry);
        }

        blockEnd = std::max(blockEnd, blockOffset + 1 +
                            (folderPath.empty() ? 0 : static_cast<uint32_t>(folderPath.size()) + 1) +
                            folder.fileCount * 16);
    }

    // The file name block starts right after the last folder block.
    m_nameBlockOffset = blockEnd;

    LOGD("Read %zu file records, name block at %u", m_files.size(), m_nameBlockOffset);
    return true;
}

// Normalize a path for archive lookup: separators are unified and case is
// ignored, because callers pass '/' while BSA folder names are stored with '\'.
static std::string normalizeLookupPath(const std::string& path) {
    std::string result = path;
    for (char& ch : result) {
        if (ch == '\\') {
            ch = '/';
        } else if (ch >= 'A' && ch <= 'Z') {
            ch = static_cast<char>(ch - 'A' + 'a');
        }
    }
    return result;
}

bool BSArchive::readNameTable() {
    // The name block holds one null-terminated file name per file, in the same
    // order as the file records. It is exactly fileNameLength bytes long.
    if (!(m_header.archiveFlags & BSA_FLAG_HAS_FILENAMES) || m_header.fileNameLength == 0) {
        LOGD("Archive has no file name block, using folder-based paths");
        for (auto& entry : m_files) {
            entry.fullPath = entry.folderPath;
            if (!entry.fullPath.empty()) {
                entry.fullPath += "\\";
            }
            entry.fullPath += std::to_string(entry.hash) + ".dat";
            entry.lookupPath = normalizeLookupPath(entry.fullPath);
            entry.fileName = entry.fullPath.substr(entry.fullPath.find_last_of("\\/") + 1);
            size_t dotPos = entry.fileName.find_last_of('.');
            entry.extension = (dotPos != std::string::npos)
                ? entry.fileName.substr(dotPos + 1) : "";
            std::transform(entry.extension.begin(), entry.extension.end(),
                           entry.extension.begin(), ::tolower);
        }
        return true;
    }

    m_stream.seekg(m_nameBlockOffset, std::ios::beg);
    if (!m_stream.good()) {
        LOGE("Failed to seek to name block at offset %u", m_nameBlockOffset);
        return false;
    }

    std::vector<char> nameBlock(m_header.fileNameLength, 0);
    m_stream.read(nameBlock.data(), m_header.fileNameLength);
    if (!m_stream.good()) {
        LOGE("Failed to read name block (%u bytes)", m_header.fileNameLength);
        return false;
    }

    m_nameTable.reserve(m_header.fileCount);

    const char* cursor = nameBlock.data();
    const char* end = nameBlock.data() + nameBlock.size();

    for (uint32_t i = 0; i < m_header.fileCount && i < m_files.size(); i++) {
        if (cursor >= end) {
            LOGE("Name block exhausted at entry %u", i);
            return false;
        }

        std::string name(cursor);
        cursor += name.size() + 1;

        m_nameTable.push_back(name);

        auto& entry = m_files[i];
        if (entry.folderPath.empty()) {
            entry.fullPath = name;
        } else {
            entry.fullPath = entry.folderPath + "\\" + name;
        }
        entry.lookupPath = normalizeLookupPath(entry.fullPath);

        entry.fileName = name;

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
    if (entry.size < 4) {
        LOGE("Compressed entry size too small: %u", entry.size);
        return false;
    }

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
    // Oblivion stores compressed entries as zlib streams. Some third-party
    // archives use headerless deflate, so try zlib first and fall back to raw.
    const int windowBits[2] = { 15 + 32, -MAX_WBITS };

    for (int attempt = 0; attempt < 2; attempt++) {
        z_stream strm;
        std::memset(&strm, 0, sizeof(strm));
        strm.zalloc = Z_NULL;
        strm.zfree = Z_NULL;
        strm.opaque = Z_NULL;
        strm.avail_in = static_cast<uInt>(inputSize);
        strm.next_in = const_cast<uint8_t*>(input);

        if (inflateInit2(&strm, windowBits[attempt]) != Z_OK) {
            continue;
        }

        output.clear();
        output.reserve(uncompressedSize > 0 ? uncompressedSize : ZLIB_CHUNK_SIZE);

        std::vector<uint8_t> buffer(ZLIB_CHUNK_SIZE);
        int ret = Z_OK;
        bool ok = false;

        for (;;) {
            strm.avail_out = static_cast<uInt>(buffer.size());
            strm.next_out = buffer.data();

            ret = inflate(&strm, Z_NO_FLUSH);

            if (ret == Z_STREAM_ERROR || ret == Z_DATA_ERROR ||
                ret == Z_MEM_ERROR || ret == Z_BUF_ERROR) {
                break;
            }

            size_t bytesProduced = buffer.size() - strm.avail_out;
            output.insert(output.end(), buffer.data(), buffer.data() + bytesProduced);

            if (ret == Z_STREAM_END) {
                ok = true;
                break;
            }
        }

        inflateEnd(&strm);

        if (ok) {
            return true;
        }

        LOGD("inflate attempt %d failed (windowBits %d, ret %d)",
             attempt, windowBits[attempt], ret);
    }

    LOGE("Failed to decompress entry");
    return false;
}

// ============================================================================
// BSA Hash Functions (RE from Oblivion.exe 0x6FA040 / 0x6FA080)
//
// These implement the exact hash algorithm the game uses to index files
// inside BSA archives. See docs/BSA_HASH_ALGORITHM.md for full details.
// ============================================================================

static uint32_t hash32(const char* s, uint8_t n) {
    uint32_t h = 0;
    for (uint8_t i = 0; i < n; ++i) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        h = h * 0x1003F + (c >= 'A' && c <= 'Z' ? c + 0x20 : c);
    }
    return h;
}

// Build a 64-bit BSA hash from a name and optional extension.
// When extra is nullptr, no extension tweak is applied (folder hash mode).
static uint64_t buildHash(const char* name, const char* extra) {
    size_t n = strlen(name);
    uint8_t out[32] = {};

    out[0] = (n > 0) ? static_cast<uint8_t>(name[n - 1] | 0x20) : 0;
    out[1] = (n > 2) ? static_cast<uint8_t>(name[n - 2] | 0x20) : 0;
    out[2] = n & 0xFF;
    out[3] = (n > 0) ? static_cast<uint8_t>(name[0] | 0x20) : 0;

    uint32_t h = 0;
    if (n > 3) {
        h = hash32(name + 1, static_cast<uint8_t>((n - 3) & 0xFF));
    }
    if (extra) {
        h += hash32(extra, static_cast<uint8_t>(strlen(extra) & 0xFF));
    }

    // Extension tweak table
    int k = 0;
    if (extra) {
        // Case-insensitive extension comparison
        const char* e = extra;
        if (e[0] == '.') {
            if ((e[1] == 'n' || e[1] == 'N') && (e[2] == 'i' || e[2] == 'I') && (e[3] == 'f' || e[3] == 'F') && e[4] == 0) k = 1;
            else if ((e[1] == 'k' || e[1] == 'K') && (e[2] == 'f' || e[2] == 'F') && e[3] == 0) k = 2;
            else if ((e[1] == 'd' || e[1] == 'D') && (e[2] == 'd' || e[2] == 'D') && (e[3] == 's' || e[3] == 'S') && e[4] == 0) k = 3;
            else if ((e[1] == 'w' || e[1] == 'W') && (e[2] == 'a' || e[2] == 'A') && (e[3] == 'v' || e[3] == 'V') && e[4] == 0) k = 4;
        }
    }

    out[3] = static_cast<uint8_t>(out[3] + ((k & 0xFC) << 5));
    out[0] = static_cast<uint8_t>(out[0] + ((k & 0xFE) << 6));
    out[1] = static_cast<uint8_t>(out[1] + ((k << 7) & 0xFF));

    return static_cast<uint64_t>(out[0])
         | (static_cast<uint64_t>(out[1]) << 8)
         | (static_cast<uint64_t>(out[2]) << 16)
         | (static_cast<uint64_t>(out[3]) << 24)
         | (static_cast<uint64_t>(h) << 32);
}

// Compute the BSA file hash for a path like "meshes\\armor\\daedric_helm.nif".
// The hash is computed from the base name and extension only (not the folder).
static uint64_t computeFileHash(const std::string& path) {
    // Find the last separator to isolate the file name
    size_t sep = path.find_last_of("\\/");
    size_t dot = path.find_last_of('.');
    std::string fname, ext;

    if (sep != std::string::npos) {
        fname = path.substr(sep + 1);
    } else {
        fname = path;
    }

    if (dot != std::string::npos && dot > sep) {
            // dot is relative to the full path; adjust to be relative to fname
            size_t localDot = dot - sep - 1;
            ext = fname.substr(localDot);
            fname = fname.substr(0, localDot);
        }

    return buildHash(fname.c_str(), ext.empty() ? nullptr : ext.c_str());
}

// ============================================================================
// File Lookup
// ============================================================================

std::vector<const BSAFileEntry*> BSArchive::findFilesByPrefix(const std::string& prefix) const {
    std::vector<const BSAFileEntry*> results;
    const std::string normalizedPrefix = normalizeLookupPath(prefix);
    for (const auto& entry : m_files) {
        if (entry.lookupPath.size() >= normalizedPrefix.size() &&
            entry.lookupPath.compare(0, normalizedPrefix.size(), normalizedPrefix) == 0) {
            results.push_back(&entry);
        }
    }
    return results;
}

const BSAFileEntry* BSArchive::findFile(const std::string& path) const {
    // Fast path: compute the BSA hash and look up in the hash index.
    const std::string normalizedPath = normalizeLookupPath(path);
    const uint64_t hash = computeFileHash(normalizedPath);
    auto it = m_fileHashIndex.find(hash);
    if (it != m_fileHashIndex.end()) {
        // Verify the match by comparing the full lookup path to avoid
        // false positives from hash collisions (extremely unlikely in practice).
        if (it->second->lookupPath == normalizedPath) {
            return it->second;
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
