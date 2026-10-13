// LIP file decode tests - see lip_sync_decode_tests.h for the rationale.
//
// Round-trips a synthetic standard lip payload (header + token stream) through
// DecodeLipFile and asserts frame counts and float fidelity, plus the reject
// cases (0xFF variant, empty, malformed tokens).

#include "lip_sync_decode_tests.h"

#include "../character/lip_sync_decoder.h"

#include <chrono>
#include <cstring>
#include <cstdint>
#include <vector>

namespace {

using facegen::DecodeLipFile;

// Emit a u32 little-endian.
void putU32(std::vector<uint8_t>& v, uint32_t x) {
    v.push_back(static_cast<uint8_t>(x & 0xFF));
    v.push_back(static_cast<uint8_t>((x >> 8) & 0xFF));
    v.push_back(static_cast<uint8_t>((x >> 16) & 0xFF));
    v.push_back(static_cast<uint8_t>((x >> 24) & 0xFF));
}

// Emit a f32 little-endian.
void putF32(std::vector<uint8_t>& v, float f) {
    uint32_t u;
    std::memcpy(&u, &f, sizeof(u));
    putU32(v, u);
}

// Token-encodes the expanded body (8-byte header + count*132 keyframe bytes)
// using the .lip run encoding: nonzero bytes emit literally, zero-runs emit
// 0x00 + u16 length.
std::vector<uint8_t> EncodeTokens(const std::vector<uint8_t>& expanded) {
    std::vector<uint8_t> tok;
    size_t i = 0;
    while (i < expanded.size()) {
        if (expanded[i] != 0) {
            tok.push_back(expanded[i]);
            ++i;
        } else {
            size_t j = i;
            while (j < expanded.size() && expanded[j] == 0 && (j - i) < 65535) {
                ++j;
            }
            uint16_t len = static_cast<uint16_t>(j - i);
            tok.push_back(0x00);
            tok.push_back(static_cast<uint8_t>(len & 0xFF));
            tok.push_back(static_cast<uint8_t>((len >> 8) & 0xFF));
            i = j;
        }
    }
    return tok;
}

// Builds a full standard .lip file with the given count, aux value and a
// caller-provided function that fills each keyframe's A/B arrays.
std::vector<uint8_t> BuildStandardLip(
    uint32_t count, int32_t aux,
    void (*fill)(int k, float A[16], float B[17])) {
    std::vector<uint8_t> expanded;
    putU32(expanded, count);
    putU32(expanded, static_cast<uint32_t>(aux));
    for (uint32_t k = 0; k < count; ++k) {
        float A[16] = {0.0f};
        float B[17] = {0.0f};
        if (fill) {
            fill(static_cast<int>(k), A, B);
        }
        for (int c = 0; c < 16; ++c) {
            putF32(expanded, A[c]);
        }
        for (int c = 0; c < 17; ++c) {
            putF32(expanded, B[c]);
        }
    }

    std::vector<uint8_t> file;
    putU32(file, 1);                          // magic
    putU32(file, 24 + count * 132);           // declared expanded size
    putU32(file, 1);                          // version
    std::vector<uint8_t> tokens = EncodeTokens(expanded);
    file.insert(file.end(), tokens.begin(), tokens.end());
    return file;
}

void FillSimple(int k, float A[16], float B[17]) {
    // Deterministic ramps: A0 tracks k, B0 tracks (count-k), others pattern.
    A[0] = static_cast<float>(k % 100) / 100.0f;
    A[3] = 1.0f - static_cast<float>(k % 100) / 100.0f;
    A[7] = (k % 2 == 0) ? 0.5f : 0.25f;
    B[0] = static_cast<float>((100 - k % 100) % 100) / 100.0f;
    B[8] = (k % 3 == 0) ? 0.9f : 0.1f;
}

void FillZero(int k, float A[16], float B[17]) {
    (void)k;
    for (int c = 0; c < 16; ++c) {
        A[c] = 0.0f;
    }
    for (int c = 0; c < 17; ++c) {
        B[c] = 0.0f;
    }
}

} // namespace

bool LipSyncDecodeTests::runAllTests() {
    bool allPassed = true;

    auto t0 = std::chrono::steady_clock::now();
    auto record = [&](const std::string& name, bool passed, const std::string& msg = "") {
        auto t1 = std::chrono::steady_clock::now();
        float ms = std::chrono::duration<float, std::milli>(t1 - t0).count();
        results.push_back({name, passed, msg, ms});
        t0 = t1;
        if (!passed) {
            allPassed = false;
        }
    };

    // 1. Standard file: count=77, round-trip fidelity of viseme/aux floats.
    {
        const uint32_t count = 77;
        auto file = BuildStandardLip(count, -6, FillSimple);
        facegen::LipTimeline tl;
        bool ok = DecodeLipFile(file.data(), file.size(), &tl);
        bool match = ok && tl.GetFrameCount() == static_cast<int>(count);
        if (match) {
            for (int k = 0; k < static_cast<int>(count); ++k) {
                const auto& f = tl.GetFrame(k);
                float eA0 = static_cast<float>(k % 100) / 100.0f;
                float eB0 = static_cast<float>((100 - k % 100) % 100) / 100.0f;
                if (f.viseme[0] != eA0 || f.aux[0] != eB0) {
                    match = false;
                    break;
                }
            }
        }
        record("standard_roundtrip_77", match);
    }

    // 2. All-zero keyframes decode correctly.
    {
        auto file = BuildStandardLip(64, 0, FillZero);
        facegen::LipTimeline tl;
        bool ok = DecodeLipFile(file.data(), file.size(), &tl);
        bool allZero = ok;
        if (allZero) {
            for (int k = 0; k < tl.GetFrameCount(); ++k) {
                const auto& f = tl.GetFrame(k);
                for (int c = 0; c < 16; ++c) {
                    if (f.viseme[c] != 0.0f) { allZero = false; break; }
                }
                if (allZero) for (int c = 0; c < 17; ++c) {
                    if (f.aux[c] != 0.0f) { allZero = false; break; }
                }
                if (!allZero) break;
            }
        }
        record("all_zero_frames", allZero);
    }

    // 3. 0xFF-prefixed variant -> rejected (lip disabled).
    {
        std::vector<uint8_t> file = {0xFF, 0x00, 0x03, 0x00, 0xFB, 0xFF, 0xFF, 0xFF,
                                     0x00, 0x01, 0x00};
        facegen::LipTimeline tl;
        bool ok = DecodeLipFile(file.data(), file.size(), &tl);
        record("reject_ff_variant", !ok && tl.Empty());
    }

    // 4. Empty / undersized file -> rejected.
    {
        std::vector<uint8_t> file = {}; // 0 bytes
        facegen::LipTimeline tl;
        bool ok = DecodeLipFile(file.data(), file.size(), &tl);
        record("reject_empty", !ok && tl.Empty());
    }

    // 5. Truncated token stream (length field overruns) -> rejected.
    {
        std::vector<uint8_t> file;
        putU32(file, 1);
        putU32(file, 24 + 4 * 132);
        putU32(file, 1);
        file.push_back(0x00); // zero-run flag but only one byte left (needs 2)
        facegen::LipTimeline tl;
        bool ok = DecodeLipFile(file.data(), file.size(), &tl);
        record("reject_truncated_tokens", !ok && tl.Empty());
    }

    // 6. Patterned floats survive interpolation sampling (monotonic A0).
    {
        const uint32_t count = 100;
        auto file = BuildStandardLip(count, -3, FillSimple);
        facegen::LipTimeline tl;
        DecodeLipFile(file.data(), file.size(), &tl);
        // Sample halfway through frame 0->1: A0 should be between the two.
        float at0 = tl.Sample(0.0f).viseme[0];
        float at1 = tl.Sample(1.0f).viseme[0];
        float atMid = tl.Sample(0.5f).viseme[0];
        bool monotonic = (atMid >= at0) && (atMid <= at1) && (at1 >= at0);
        record("sample_interpolation", monotonic);
    }

    return allPassed;
}

int LipSyncDecodeTests::getPassCount() const {
    int n = 0;
    for (const auto& r : results) {
        if (r.passed) ++n;
    }
    return n;
}

int LipSyncDecodeTests::getFailCount() const {
    int n = 0;
    for (const auto& r : results) {
        if (!r.passed) ++n;
    }
    return n;
}

std::string LipSyncDecodeTests::getSummary() const {
    int pass = getPassCount();
    int fail = getFailCount();
    return "LIP sync decode: " + std::to_string(pass) + " passed, " +
           std::to_string(fail) + " failed (" + std::to_string(pass + fail) + " total)";
}