#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>

// Simple BSA (Bethesda Softworks Archive) reader
class BSAParser {
public:
    struct FileEntry {
        std::string name;
        uint32_t size;
        uint32_t offset;
    };

    BSAParser();
    ~BSAParser();

    bool open(const std::string& path);
    void close();

    bool isOpen() const { return isValid; }

    std::vector<FileEntry> listFiles() const;
    bool extractFile(const std::string& filename, std::vector<uint8_t>& outData);
    bool extractFile(const std::string& filename, const std::string& outPath);

    static bool isBSAFile(const std::string& path);

private:
    std::string bsaPath;
    bool isValid;
    std::unordered_map<std::string, FileEntry> fileMap;

    bool readHeader();
    bool readFileList();
};
