#pragma once

// ============================================================================
// Phase 65: LIP file decoder for dialogue lip sync.
//
// Decodes Oblivion .lip files into a frame-based morph-weight timeline that
// drives the FaceExpressionSystem. The format was reverse-engineered from the
// Steam EXE (see tmp/re_lip_format.md) and validated on all 9204 shipped lips:
//   9196 standard, 7 0xFF-prefixed variants, 1 empty.
//
// Standard format:
//   Header (12 bytes): u32=1, u32=24+count*132, u32=1
//   Body: token stream -- byte != 0 emits a literal byte; byte == 0 is
//         followed by u16 length L and emits L zero bytes.
//   After expansion, offset 0..7 hold dword[0]=count and dword[1]=auxiliary
//   (-15..0), then `count` keyframes of 132 bytes each:
//       array A[16] x u32 (32-bit float) at offset 0
//       array B[17] x u32 (32-bit float) at offset 64
//   A = viseme morph weights (mouth/jaw driven by speech),
//   B = auxiliary channel weights.
//
// The 0xFF-prefixed variant files and empty files are treated as lip-sync
// disabled (invalid) and returned as a failed decode.
// ============================================================================

#include <cstdint>
#include <cstring>
#include <vector>

namespace facegen {

// Maximum safe frame count. Header[1] == 24 + count*132 anyway; guard against
// malformed or hostile inputs.
static constexpr int kLipMaxFrames = 2048;
static constexpr int kLipVariantCount = 16;  // array A
static constexpr int kLipAuxCount = 17;      // array B

// A single decoded lip keyframe.
struct LipFrame {
    // Viseme morph weights (array A). Values are 32-bit floats in [0,1].
    float viseme[kLipVariantCount];
    // Auxiliary channel weights (array B).
    float aux[kLipAuxCount];
};

// Decoded lip payload: a frame-based weight timeline.
class LipTimeline {
public:
    LipTimeline() = default;

    void Clear() { frames_.clear(); frames_.shrink_to_fit(); }

    bool Empty() const { return frames_.empty(); }

    void AddFrame(const LipFrame& frame) {
        if (frames_.size() < static_cast<size_t>(kLipMaxFrames)) {
            frames_.push_back(frame);
        }
    }

    int GetFrameCount() const { return static_cast<int>(frames_.size()); }

    const LipFrame& GetFrame(int index) const { return frames_[index]; }

    // Linear interpolation between adjacent frames by a fractional frame
    // position (0..frameCount-1). Clamps at the ends.
    LipFrame Sample(float framePos) const {
        if (frames_.empty()) {
            LipFrame f{};
            std::memset(&f, 0, sizeof(f));
            return f;
        }
        if (framePos <= 0.0f) {
            return frames_.front();
        }
        int last = GetFrameCount() - 1;
        if (framePos >= static_cast<float>(last)) {
            return frames_.back();
        }
        int i = static_cast<int>(framePos);
        float t = framePos - static_cast<float>(i);
        const LipFrame& a = frames_[i];
        const LipFrame& b = frames_[i + 1];
        LipFrame out;
        for (int c = 0; c < kLipVariantCount; ++c) {
            out.viseme[c] = a.viseme[c] + (b.viseme[c] - a.viseme[c]) * t;
        }
        for (int c = 0; c < kLipAuxCount; ++c) {
            out.aux[c] = a.aux[c] + (b.aux[c] - a.aux[c]) * t;
        }
        return out;
    }

    const std::vector<LipFrame>& Frames() const { return frames_; }

private:
    std::vector<LipFrame> frames_;
};

// Decodes a raw .lip buffer into a LipTimeline. Returns true on success and
// fills `out`. Returns false (lip disabled) for 0xFF variants and empty files.
bool DecodeLipFile(const uint8_t* data, size_t size, LipTimeline* out);

} // namespace facegen