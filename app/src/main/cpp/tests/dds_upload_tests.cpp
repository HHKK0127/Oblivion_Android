#include "tests/dds_upload_tests.h"

#include "assets/dds_loader.h"

#include <chrono>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <vector>

namespace {

constexpr const char* kSyntheticDdsPath = "tmp/dds_upload_tests_synthetic.dds";

float elapsed_ms(std::chrono::steady_clock::time_point start) {
    const auto now = std::chrono::steady_clock::now();
    return std::chrono::duration<float, std::milli>(now - start).count();
}

void write_u32_le(std::vector<uint8_t>& data, size_t offset, uint32_t value) {
    data[offset + 0] = static_cast<uint8_t>(value & 0xFFu);
    data[offset + 1] = static_cast<uint8_t>((value >> 8) & 0xFFu);
    data[offset + 2] = static_cast<uint8_t>((value >> 16) & 0xFFu);
    data[offset + 3] = static_cast<uint8_t>((value >> 24) & 0xFFu);
}

bool write_synthetic_dxt1() {
    // DDS header (magic + 124-byte header) followed by one 16x16 DXT1 level.
    std::vector<uint8_t> data(128u + 128u, 0u);
    write_u32_le(data, 0, DDS_MAGIC);
    write_u32_le(data, 4, 124u);          // dwSize
    write_u32_le(data, 8, 0x21007u);      // CAPS|HEIGHT|WIDTH|PIXELFORMAT|MIPMAPCOUNT
    write_u32_le(data, 12, 16u);          // dwHeight
    write_u32_le(data, 16, 16u);          // dwWidth
    write_u32_le(data, 20, 128u);         // dwPitchOrLinearSize
    write_u32_le(data, 28, 1u);           // dwMipMapCount

    const size_t pixel_format = 76u;
    write_u32_le(data, pixel_format + 0, 32u);
    write_u32_le(data, pixel_format + 4, DDPF_FOURCC);
    write_u32_le(data, pixel_format + 8, FOURCC_DXT1);
    write_u32_le(data, 108u, 0x1000u);    // DDSCAPS_TEXTURE

    // A deterministic non-zero block payload makes the fixture exercise the
    // Rust parser's compressed-data borrow/copy path.
    for (size_t i = 128u; i < data.size(); ++i) {
        data[i] = static_cast<uint8_t>(i - 128u);
    }

    std::ofstream out(kSyntheticDdsPath, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) {
        return false;
    }
    out.write(reinterpret_cast<const char*>(data.data()),
              static_cast<std::streamsize>(data.size()));
    return out.good();
}

}  // namespace

void DdsUploadTests::record(const std::string& name, bool passed,
                            const std::string& msg, float ms) {
    results.push_back({name, passed, msg, ms});
}

void DdsUploadTests::testLoadSyntheticDxt1() {
    const auto start = std::chrono::steady_clock::now();
    const bool fixture_ok = write_synthetic_dxt1();
    DDSLoader loader;
    const bool loaded = fixture_ok && loader.loadFile(kSyntheticDdsPath);
    const DDSTexture& texture = loader.getTexture();
    const bool ok = loaded &&
                    texture.width == 16u &&
                    texture.height == 16u &&
                    texture.mipmapCount == 1u &&
                    texture.compressionFormat == DDSCompressionFormat::DXT1 &&
                    texture.compressedData.size() == 128u;
    std::ostringstream message;
    if (!ok) {
        message << "fixture=" << (fixture_ok ? "ok" : "write-failed")
                << " loaded=" << (loaded ? "true" : "false")
                << " size=" << texture.compressedData.size();
    }
    record("LoadSyntheticDxt1", ok, message.str(), elapsed_ms(start));
    loader.cleanup();
    std::remove(kSyntheticDdsPath);
}

void DdsUploadTests::testCompressedUploadFallsBackWithoutS3tc() {
    const auto start = std::chrono::steady_clock::now();
    const bool fixture_ok = write_synthetic_dxt1();
    DDSLoader loader;
    const bool loaded = fixture_ok && loader.loadFile(kSyntheticDdsPath);
    const unsigned int texture_id = loaded ? loader.uploadCompressedToGPU() : 0u;
    // The host GL stub returns no extension string, so the production path must
    // decline compressed upload and let the caller use RGBA8 decompression.
    const bool ok = loaded && texture_id == 0u;
    record("CompressedUploadFallsBackWithoutS3tc", ok,
           ok ? "" : "expected host S3TC upload fallback", elapsed_ms(start));
    loader.cleanup();
    std::remove(kSyntheticDdsPath);
}

void DdsUploadTests::testCleanupIsIdempotent() {
    const auto start = std::chrono::steady_clock::now();
    DDSLoader loader;
    loader.cleanup();
    loader.cleanup();
    record("CleanupIsIdempotent", true, "", elapsed_ms(start));
}

bool DdsUploadTests::runAllTests() {
    results.clear();
    testLoadSyntheticDxt1();
    testCompressedUploadFallsBackWithoutS3tc();
    testCleanupIsIdempotent();
    return getFailCount() == 0;
}

int DdsUploadTests::getPassCount() const {
    int count = 0;
    for (const DdsUploadTestResult& result : results) {
        if (result.passed) {
            ++count;
        }
    }
    return count;
}

int DdsUploadTests::getFailCount() const {
    int count = 0;
    for (const DdsUploadTestResult& result : results) {
        if (!result.passed) {
            ++count;
        }
    }
    return count;
}

std::string DdsUploadTests::getSummary() const {
    std::ostringstream summary;
    summary << "DdsUploadTests: " << getPassCount() << " passed, "
            << getFailCount() << " failed";
    for (const DdsUploadTestResult& result : results) {
        if (!result.passed) {
            summary << "\n  FAIL " << result.testName << ": " << result.message;
        }
    }
    return summary.str();
}
