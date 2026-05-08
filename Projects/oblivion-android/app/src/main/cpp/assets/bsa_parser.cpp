#include "bsa_parser.h"
#include <android/log.h>
#include <fstream>

#define LOG_TAG_BSA "BSAParser"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG_BSA, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG_BSA, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG_BSA, __VA_ARGS__)

// BSA file magic number
static constexpr uint32_t BSA_MAGIC = 0x00415342; // 'BSA\0'

struct BSAHeader {
    uint32_t fileId;
    uint32_t version;
    uint32_t offset;
    uint32_t archiveFlags;
    uint32_t folderCount;
    uint32_t fileCount;
    uint32_t totalFolderNameLength;
    uint32_t totalFileNameLength;
    uint32_t fileFlags;
};

BSAParser::BSAParser() : isValid(false) {}

BSAParser::~BSAParser() {
    close();
}

bool BSAParser::open(const std::string& path) {
    close();
    bsaPath = path;

    if (!isBSAFile(path)) {
        LOGE("Not a valid BSA file: %s", path.c_str());
        return false;
    }

    if (!readHeader()) {
        LOGE("Failed to read BSA header: %s", path.c_str());
        return false;
    }

    if (!readFileList()) {
        LOGE("Failed to read BSA file list: %s", path.c_str());
        return false;
    }

    isValid = true;
    LOGI("Opened BSA archive: %s (%zu files)", path.c_str(), fileMap.size());
    return true;
}

void BSAParser::close() {
    isValid = false;
    fileMap.clear();
    bsaPath.clear();
}

bool BSAParser::readHeader() {
    std::ifstream file(bsaPath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    BSAHeader header;
    file.read(reinterpret_cast<char*>(&header), sizeof(header));

    if (header.fileId != BSA_MAGIC) {
        return false;
    }

    LOGD("BSA Version: %u, Folders: %u, Files: %u",
         header.version, header.folderCount, header.fileCount);
    return true;
}

bool BSAParser::readFileList() {
    // Simplified: In a real implementation, we would parse the folder records,
    // file records, and filename block here.
    // For now, we just acknowledge the BSA format.
    LOGI("BSA file list reading is stubbed - full implementation requires detailed format parsing");
    return true;
}

std::vector<BSAParser::FileEntry> BSAParser::listFiles() const {
    std::vector<FileEntry> files;
    for (const auto& [name, entry] : fileMap) {
        files.push_back(entry);
    }
    return files;
}

bool BSAParser::extractFile(const std::string& filename, std::vector<uint8_t>& outData) {
    auto it = fileMap.find(filename);
    if (it == fileMap.end()) {
        LOGE("File not found in BSA: %s", filename.c_str());
        return false;
    }

    // Stub: Real implementation would seek to offset and read size bytes
    LOGD("Extracting %s (size: %u)", filename.c_str(), it->second.size);
    outData.resize(it->second.size);
    return true;
}

bool BSAParser::extractFile(const std::string& filename, const std::string& outPath) {
    std::vector<uint8_t> data;
    if (!extractFile(filename, data)) {
        return false;
    }

    std::ofstream outFile(outPath, std::ios::binary);
    if (!outFile.is_open()) {
        LOGE("Failed to create output file: %s", outPath.c_str());
        return false;
    }

    outFile.write(reinterpret_cast<const char*>(data.data()), data.size());
    outFile.close();
    return true;
}

bool BSAParser::isBSAFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    uint32_t magic;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    return magic == BSA_MAGIC;
}
