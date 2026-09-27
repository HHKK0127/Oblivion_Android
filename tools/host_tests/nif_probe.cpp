// Host probe: parse one NIF with the real NIFParser and report the block walk.
// Fast iteration loop for block-layout work (no Android build required).
//
// Build (from the repo root, Git Bash):
//   g++ -std=c++17 -O2 -Itools/host_tests/stubs -Iapp/src/main/cpp \
//       -Iapp/src/main/cpp/include -Iapp/src/main/cpp/assets \
//       tools/host_tests/nif_probe.cpp app/src/main/cpp/assets/nif_parser.cpp \
//       app/src/main/cpp/assets/nif_block_type_map.cpp -o /tmp/nif_probe
#include "nif_parser.h"
#include <cstdio>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: nif_probe <file.nif>\n");
        return 2;
    }
    NIFParser parser;
    if (!parser.parseFile(argv[1])) {
        std::fprintf(stderr, "parseFile failed: %s\n", argv[1]);
        return 1;
    }
    const NIFHeader& h = parser.getHeader();
    std::printf("version=0x%08X blocks=%u types=%zu blockDataOffset=%zu fileSize=%zu\n",
                h.version, parser.getBlockCount(), h.blockTypeNames.size(),
                parser.getBlockDataOffset(), parser.getFileSize());
    for (uint32_t i = 0; i < parser.getBlockCount(); ++i) {
        std::printf("  block %2u : %s\n", i, parser.getBlockTypeName(i).c_str());
    }
    std::printf("blocksWalked=%d prefix=%u walkError='%s'\n",
                parser.areBlocksWalked() ? 1 : 0, parser.getBlockPrefix(),
                parser.getWalkError().c_str());
    if (parser.areBlocksWalked()) {
        std::printf("trailingData=%zu trailerValues=%u\n",
                    parser.getTrailingDataSize(), parser.getTrailerValueCount());
    }
    // Printed for partial walks too: the last end is where the failing block
    // starts, which is what a layout fix needs.
    for (uint32_t i = 0; i < parser.getBlockCount(); ++i) {
        const size_t begin = parser.getBlockBodyOffset(i);
        const size_t end = parser.getBlockBodyEnd(i);
        if (begin == 0 && end == 0) {
            break;
        }
        std::printf("  block %2u : [%zu,%zu) size=%zu\n", i, begin, end, end - begin);
    }
    const std::vector<NIFGeometry> geometries = parser.extractAllGeometry();
    std::printf("geometries=%zu\n", geometries.size());
    for (size_t i = 0; i < geometries.size(); ++i) {
        const NIFGeometry& g = geometries[i];
        std::printf("  geom %zu: '%s' verts=%zu normals=%zu uvs=%zu colors=%zu tris=%zu"
                    " material=%u texturing=%u\n",
                    i, g.name.c_str(), g.vertices.size(), g.normals.size(),
                    g.texCoords.size(), g.colors.size(), g.triangles.size(),
                    g.materialPropertyIndex, g.texturingPropertyIndex);
        if (!g.vertices.empty()) {
            const size_t shown = g.vertices.size() < 4 ? g.vertices.size() : 4;
            for (size_t v = 0; v < shown; ++v) {
                std::printf("    v%zu=(%.3f, %.3f, %.3f)\n", v,
                            g.vertices[v].x, g.vertices[v].y, g.vertices[v].z);
            }
        }
        if (!g.texCoords.empty()) {
            const size_t shown = g.texCoords.size() < 4 ? g.texCoords.size() : 4;
            for (size_t t = 0; t < shown; ++t) {
                std::printf("    uv%zu=(%.3f, %.3f)\n", t,
                            g.texCoords[t].x, g.texCoords[t].y);
            }
        }
        if (!g.triangles.empty()) {
            std::printf("    t0=(%u, %u, %u)\n", g.triangles[0].v0,
                        g.triangles[0].v1, g.triangles[0].v2);
        }
    }
    return 0;
}
