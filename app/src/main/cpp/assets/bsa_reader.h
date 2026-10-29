#pragma once

#include <cstdint>
#include <string>
#include <vector>

// ============================================================================
// BSA Archive Reader for Oblivion (TES4) .bsa files
//
// This class is a thin C++ front end over the Rust `bsa-core` crate, reached
// through the `bsa-ffi` C ABI (see bsa_ffi.h). All parsing, hashing, path
// normalization and ZLib inflation live on the Rust side; this file only keeps
// the object-oriented surface the rest of the engine calls, and caches the
// parsed index so the accessors can return references and pointers into stable
// storage.
//
// The on-disk format is documented publicly by UESP ("Oblivion Mod:BSA File
// Format"). BSA header magic: 0x00415342 ("BSA\0"). Oblivion version: 0x67.
// ============================================================================

// Archive flags bit masks (see UESP "Oblivion Mod:BSA File Format")
constexpr uint32_t BSA_FLAG_HAS_FOLDERNAMES     = 0x00000001;  // directory names are stored
constexpr uint32_t BSA_FLAG_HAS_FILENAMES       = 0x00000002;  // file names are stored
constexpr uint32_t BSA_FLAG_COMPRESSED          = 0x00000004;  // files compressed by default

struct BSAHeader {
    uint32_t magic;                 // 0x00415342 ("BSA\0")
    uint32_t version;               // 0x67 for Oblivion
    uint32_t folderRecordOffset;    // Offset to folder records
    uint32_t archiveFlags;          // See BSA_FLAG_*
    uint32_t folderCount;           // Number of folder records
    uint32_t fileCount;             // Total number of files
    uint32_t folderNameLength;      // Total length of folder name strings
    uint32_t fileNameLength;        // Total length of file name strings
    uint32_t fileFlags;             // File flags
};

struct BSAFileEntry {
    std::string fullPath;           // Full path like "meshes\\architecture\\something.nif"
    std::string lookupPath;         // fullPath normalized for lookup (lowercase, '/' separators)
    std::string fileName;           // Just the file name
    std::string extension;          // File extension (lowercase)
    std::string folderPath;         // Folder path

    uint64_t hash = 0;              // File name hash (from BSAFileRecord)
    uint32_t offset;                // Data offset in archive
    uint32_t size;                  // Stored size in archive (compressed size when compressed)
    uint32_t realSize;              // Uncompressed size; 0 for compressed entries until extracted
    bool compressed;                // Whether data is ZLib compressed

    BSAFileEntry()
        : offset(0), size(0), realSize(0), compressed(false) {}
};

class BSArchive {
public:
    BSArchive();
    ~BSArchive();

    // A handle is owned, so copying would free it twice.
    BSArchive(const BSArchive&) = delete;
    BSArchive& operator=(const BSArchive&) = delete;

    // Open a BSA file, parse header and build file index
    bool open(const std::string& filePath);

    // Close the archive
    void close();

    // Check if archive is open and valid
    bool isOpen() const { return m_isOpen; }

    // Read a file entry's raw data (compressed if relevant)
    // Returns true on success, data in output buffer
    bool extractFile(const BSAFileEntry& entry, std::vector<uint8_t>& output);

    // Read and decompress a file entry to output buffer
    bool extractFileDecompressed(const BSAFileEntry& entry, std::vector<uint8_t>& output);

    // Get all file entries (for browsing/listing)
    const std::vector<BSAFileEntry>& getFiles() const { return m_files; }

    // Find files by path prefix (e.g., "meshes\\architecture\\")
    std::vector<const BSAFileEntry*> findFilesByPrefix(const std::string& prefix) const;

    // Find a single file by exact path
    const BSAFileEntry* findFile(const std::string& path) const;

    // Find files by extension
    std::vector<const BSAFileEntry*> findFilesByExtension(const std::string& ext) const;

    // Getter
    const BSAHeader& getHeader() const { return m_header; }
    const std::string& getFilePath() const { return m_filePath; }

    // Info
    uint32_t getFileCount() const { return m_header.fileCount; }
    bool isCompressed() const { return (m_header.archiveFlags & BSA_FLAG_COMPRESSED) != 0; }

private:
    // Copy the header and the full entry list out of the Rust archive, so the
    // accessors above can hand out references and pointers. Called after every
    // open attempt, including a failed one, because a parse that stopped early
    // still leaves a partial index to inspect.
    bool syncCache();

    // Map an entry the caller got from this archive back to its index in the
    // Rust archive, falling back to an identity match if it came from elsewhere.
    size_t indexOf(const BSAFileEntry& entry) const;

    // Shared body of the two extract calls, since only the C ABI function differs.
    bool extract(bool decompress, const BSAFileEntry& entry, std::vector<uint8_t>& output);

    // Opaque bsa-ffi handle (bsa_create() / bsa_destroy()).
    void* m_handle;
    std::string m_filePath;

    // Header
    BSAHeader m_header;

    // Parsed data, refreshed by syncCache()
    std::vector<BSAFileEntry> m_files;

    // Status
    bool m_isOpen;
};