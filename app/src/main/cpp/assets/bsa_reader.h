#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <fstream>

// ============================================================================
// BSA Archive Reader for Oblivion (TES4) .bsa files
//
// Implements the Oblivion BSA format based on SharpBSABA2 source analysis.
// BSA Header Magic: 0x00415342 ("BSA\0")
// Oblivion Version: 0x67
// ============================================================================

// Archive flags bit masks
constexpr uint32_t BSA_FLAG_HAS_NAMETABLE      = 0x00000001;
constexpr uint32_t BSA_FLAG_HAS_FOLDERNAMES     = 0x00000002;
constexpr uint32_t BSA_FLAG_COMPRESSED          = 0x00000004;

// Bit 30 of file size field toggles per-entry compression
constexpr uint32_t BSA_SIZE_COMPRESS_TOGGLE     = 0x40000000;

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

struct BSAFolderRecord {
    uint64_t hash;                  // Folder name hash
    uint32_t fileCount;             // Number of files in this folder
    uint32_t offset;                // Offset to folder name string (in string pool)
};

struct BSAFileRecord {
    uint64_t hash;                  // File name hash
    uint32_t size;                  // File size (bit 30 = compression toggle)
    uint32_t offset;                // Offset to file data in archive
};

struct BSAFileEntry {
    std::string fullPath;           // Full path like "meshes\\architecture\\something.nif"
    std::string fileName;           // Just the file name
    std::string extension;          // File extension (lowercase)
    std::string folderPath;         // Folder path

    uint64_t hash = 0;              // File name hash (from BSAFileRecord)
    uint32_t offset;                // Data offset in archive
    uint32_t size;                  // Raw size in archive
    uint32_t realSize;              // Uncompressed size (0 if unknown)
    bool compressed;                // Whether data is ZLib compressed

    BSAFileEntry()
        : offset(0), size(0), realSize(0), compressed(false) {}
};

class BSArchive {
public:
    BSArchive();
    ~BSArchive();

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
    // Parse the BSA structure after header is read
    bool parseArchive();

    // Read folder records and their file entries
    bool parseFolderRecords();

    // Read the name table at end of header region
    bool readNameTable();

    // Decompress ZLib data
    bool decompressZLib(const uint8_t* input, size_t inputSize,
                        std::vector<uint8_t>& output, size_t uncompressedSize) const;

    // File stream
    std::ifstream m_stream;
    std::string m_filePath;

    // Header
    BSAHeader m_header;

    // Parsed data
    std::vector<BSAFolderRecord> m_folders;
    std::vector<BSAFileEntry> m_files;

    // Folder name strings (indexed by folder record offset)
    std::unordered_map<uint32_t, std::string> m_folderNames;

    // Name table strings (for building full paths)
    std::vector<std::string> m_nameTable;

    // Status
    bool m_isOpen;
    bool m_parsed;

    // Path prefix for folder/file lookups
    static constexpr size_t ZLIB_CHUNK_SIZE = 16384;
};
