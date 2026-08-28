// ============================================================================
// WindField - Perlin noise-based spatial wind field for vegetation animation
// Phase 51: Provides spatially varying wind for natural tree sway
// ============================================================================

#include "tree_wind_field.h"

#include <cmath>
#include <cstring>
#include <algorithm>

namespace vegetation {

// ============================================================================
// Permutation table (Ken Perlin's original)
// ============================================================================

static const int PERM_INIT[256] = {
    151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,
    140,36,103,30,69,142,8,99,37,240,21,10,23,190,6,148,
    247,120,234,75,0,26,197,62,94,252,219,203,117,35,11,32,
    57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,
    74,165,71,134,139,48,27,166,77,146,158,231,83,111,229,122,
    60,211,133,230,220,105,92,41,55,46,245,40,244,102,143,54,
    65,25,63,161,1,216,80,73,209,76,132,187,208,89,18,169,
    200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,
    52,217,226,250,124,123,5,202,38,147,118,126,255,82,85,212,
    207,206,59,227,47,16,58,17,182,189,28,42,223,183,170,213,
    119,248,152,2,44,154,163,70,221,153,101,155,167,43,172,9,
    129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,
    218,246,97,228,251,34,242,193,238,210,144,12,191,179,162,241,
    81,51,145,235,249,14,239,107,49,192,214,31,181,199,106,157,
    184,84,204,176,115,121,50,45,127,4,150,254,138,236,205,93,
    222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
};

WindField::WindField() {
    std::memset(perm_, 0, sizeof(perm_));
}

WindField::~WindField() = default;

void WindField::initialize(uint32_t seed) {
    seed_ = seed;

    // Initialize permutation table with seed-based shuffle
    for (int i = 0; i < 256; i++) {
        perm_[i] = PERM_INIT[i];
    }

    // Fisher-Yates shuffle with seed
    uint32_t s = seed;
    for (int i = 255; i > 0; i--) {
        s = s * 1103515245 + 12345;  // LCG
        int j = static_cast<int>((s >> 16) % static_cast<uint32_t>(i + 1));
        if (j < 0) j = -j;
        std::swap(perm_[i], perm_[j]);
    }

    // Duplicate for wrapping
    for (int i = 0; i < 256; i++) {
        perm_[256 + i] = perm_[i];
    }
}

// ============================================================================
// Perlin noise core
// ============================================================================

float WindField::fade(float t) const {
    return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

float WindField::lerp(float a, float b, float t) const {
    return a + t * (b - a);
}

float WindField::grad(int hash, float x, float y, float z) const {
    int h = hash & 15;
    float u = (h < 8) ? x : y;
    float v = (h < 4) ? y : ((h == 12 || h == 14) ? x : z);
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

float WindField::perlinNoise3D(float x, float y, float z) const {
    int xi = static_cast<int>(std::floor(x)) & 255;
    int yi = static_cast<int>(std::floor(y)) & 255;
    int zi = static_cast<int>(std::floor(z)) & 255;

    float xf = x - std::floor(x);
    float yf = y - std::floor(y);
    float zf = z - std::floor(z);

    float u = fade(xf);
    float v = fade(yf);
    float w = fade(zf);

    int a  = perm_[xi] + yi;
    int aa = perm_[a] + zi;
    int ab = perm_[a + 1] + zi;
    int b  = perm_[xi + 1] + yi;
    int ba = perm_[b] + zi;
    int bb = perm_[b + 1] + zi;

    return lerp(
        lerp(
            lerp(grad(perm_[aa], xf, yf, zf),
                 grad(perm_[ba], xf - 1, yf, zf), u),
            lerp(grad(perm_[ab], xf, yf - 1, zf),
                 grad(perm_[bb], xf - 1, yf - 1, zf), u), v),
        lerp(
            lerp(grad(perm_[aa + 1], xf, yf, zf - 1),
                 grad(perm_[ba + 1], xf - 1, yf, zf - 1), u),
            lerp(grad(perm_[ab + 1], xf, yf - 1, zf - 1),
                 grad(perm_[bb + 1], xf - 1, yf - 1, zf - 1), u), v), w);
}

float WindField::perlinNoise2D(float x, float y) const {
    return perlinNoise3D(x, y, 0.0f);
}

float WindField::fractalNoise(float x, float y, float z, int octaves) const {
    float value = 0.0f;
    float amplitude = 1.0f;
    float frequency = 1.0f;
    float maxValue = 0.0f;

    for (int i = 0; i < octaves; i++) {
        value += perlinNoise3D(x * frequency, y * frequency, z * frequency) * amplitude;
        maxValue += amplitude;
        amplitude *= 0.5f;
        frequency *= 2.0f;
    }

    return value / maxValue;
}

// ============================================================================
// Wind sampling
// ============================================================================

glm::vec3 WindField::sampleWind(const glm::vec3& worldPos, float time) const {
    // Spatial coordinates scaled for noise
    float sx = worldPos.x * spatialScale_;
    float sy = worldPos.y * spatialScale_ * 0.5f;
    float sz = worldPos.z * spatialScale_;

    // Time component
    float t = time * timeScale_;

    // Multi-octave noise for spatial variation
    float noiseX = fractalNoise(sx + t * 0.3f, sy, sz, 3);
    float noiseZ = fractalNoise(sx, sy, sz + t * 0.3f + 100.0f, 3);

    // Gust noise (higher frequency, lower amplitude)
    float gustNoise = fractalNoise(sx * 3.0f + t * 2.0f, sy, sz * 3.0f, 2);
    float gust = gustNoise * gustStrength_;

    // Combine base wind with noise variation
    float totalStrength = baseStrength_ + gust;
    totalStrength = std::max(0.0f, totalStrength);

    // Wind direction with noise perturbation
    glm::vec3 windDir = baseDirection_;
    windDir.x += noiseX * 0.3f;
    windDir.z += noiseZ * 0.3f;
    windDir = glm::normalize(windDir);

    return windDir * totalStrength;
}

void WindField::update(float deltaTime) {
    time_ += deltaTime;
}

} // namespace vegetation
