#include "lip_sync_decoder.h"

namespace facegen {

namespace {

inline uint32_t ReadU32(const uint8_t* p, size_t offset) {
    uint32_t v;
    std::memcpy(&v, p + offset, sizeof(v));
    return v;
}

inline float ReadF32(const uint8_t* p, size_t offset) {
    uint32_t u = ReadU32(p, offset);
    float f;
    std::memcpy(&f, &u, sizeof(f));
    return f;
}

// Expands the run-length token stream (byte!=0 literal, 0x00+u16 zeros) into
// the decompressed payload. Returns false if the stream is malformed
// (overruns the source or produces more than kMaxExpanded bytes).
bool ExpandStream(const uint8_t* body, size_t bodySize, std::vector<uint8_t>& out) {
    // Worst case expansion <= 8 + count*132; count is bounded by size. Guard
    // against pathological all-zero runs by capping total output size.
    const size_t kMaxExpanded = 8 + static_cast<size_t>(kLipMaxFrames) * 132;
    out.clear();
    out.reserve(std::min(bodySize * 2, kMaxExpanded));
    size_t i = 0;
    while (i < bodySize) {
        uint8_t b = body[i++];
        if (b != 0) {
            out.push_back(b);
            if (out.size() > kMaxExpanded) {
                return false;
            }
        } else {
            if (i + 2 > bodySize) {
                return false; // truncated length field
            }
            uint16_t len = static_cast<uint16_t>(body[i]) |
                           (static_cast<uint16_t>(body[i + 1]) << 8);
            i += 2;
            if (out.size() + len > kMaxExpanded) {
                return false;
            }
            out.insert(out.end(), len, 0x00);
        }
    }
    return true;
}

} // namespace

bool DecodeLipFile(const uint8_t* data, size_t size, LipTimeline* out) {
    if (out == nullptr) {
        return false;
    }
    out->Clear();

    // Reject undersized / variant / empty inputs.
    if (size < 12) {
        return false; // empty (or tiny) -- lip disabled
    }

    // Standard header begins with u32 = 1. 0xFF prefix marks the variant
    // files (7 shipped), treated as lip disabled.
    const uint8_t* h = data;
    if (h[0] != 0x01) {
        return false; // variant / non-standard
    }

    std::vector<uint8_t> expanded;
        if (!ExpandStream(data + 12, size - 12, expanded)) {
            return false;
        }
        if (expanded.size() < 8) {
        return false;
    }

        const uint32_t count = ReadU32(expanded.data(), 0);
        // count validated (< 0xF4241) by the source; sanity-clamp here too.
        if (count > static_cast<uint32_t>(kLipMaxFrames)) {
            return false;
        }
        const size_t expected = 8 + static_cast<size_t>(count) * 132;
                if (expanded.size() < expected) {
                    return false; // truncated keyframe region
                }

            // NOTE: header[1] is NOT validated. Across the shipped voice set it is
            // inconsistent: most files store 24 + count*132, but some store the
            // expanded body size + 1 (= 9 + count*132). A strict match would reject
            // legitimate files, so the field is treated as opaque.

            for (uint32_t k = 0; k < count; ++k) {
        const uint8_t* base = expanded.data() + 8 + static_cast<size_t>(k) * 132;
        LipFrame frame;
        for (int c = 0; c < kLipVariantCount; ++c) {
            frame.viseme[c] = ReadF32(base, static_cast<size_t>(c) * 4);
        }
        for (int c = 0; c < kLipAuxCount; ++c) {
            frame.aux[c] = ReadF32(base, 64 + static_cast<size_t>(c) * 4);
        }
        out->AddFrame(frame);
    }

    return !out->Empty();
}

} // namespace facegen