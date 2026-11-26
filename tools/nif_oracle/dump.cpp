// Shared oracle dump driver for the original NIFParser and the Rust FFI shim.
// Prints a deterministic text dump of the parsed NIF to stdout so the two
// builds can be diffed bit-for-bit. Floats use %a (hex) and vector payloads
// are compared through FNV-1a hashes over their raw bytes.
#include "nif_parser.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

uint64_t fnv1a(const void* data, size_t size, uint64_t hash = 1469598103934665603ULL) {
    const auto* p = static_cast<const unsigned char*>(data);
    for (size_t i = 0; i < size; ++i) {
        hash ^= p[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

template <typename T>
uint64_t hash_vec(const std::vector<T>& v) {
    return fnv1a(v.data(), v.size() * sizeof(T));
}

// Floats print as their raw IEEE-754 bits. %a renders denormals and NaN
// differently under glibc and the MSVC runtime, so the text is not portable even
// when the value is (found on the legacy 4.0.0.2 markers, 2026-11-26).
void f32(float v) {
    uint32_t bits;
    memcpy(&bits, &v, sizeof(bits));
    printf("%08x", bits);
}

void dump_geometry(const char* tag, const NIFGeometry& g) {
    printf("%s name=%s\n", tag, g.name.c_str());
    printf("%s diffuse=%s normal=%s\n", tag, g.diffuseTexture.c_str(), g.normalTexture.c_str());
    printf("%s verts=%zu hash=%016llx\n", tag, g.vertices.size(),
           static_cast<unsigned long long>(hash_vec(g.vertices)));
    printf("%s norms=%zu hash=%016llx\n", tag, g.normals.size(),
           static_cast<unsigned long long>(hash_vec(g.normals)));
    printf("%s colors=%zu hash=%016llx\n", tag, g.colors.size(),
           static_cast<unsigned long long>(hash_vec(g.colors)));
    printf("%s texcoords=%zu hash=%016llx\n", tag, g.texCoords.size(),
           static_cast<unsigned long long>(hash_vec(g.texCoords)));
    printf("%s tris=%zu hash=%016llx\n", tag, g.triangles.size(),
           static_cast<unsigned long long>(hash_vec(g.triangles)));
    if (!g.vertices.empty()) {
        float minx = g.vertices[0].x, miny = g.vertices[0].y, minz = g.vertices[0].z;
        float maxx = minx, maxy = miny, maxz = minz;
        for (const auto& v : g.vertices) {
            if (v.x < minx) minx = v.x;
            if (v.y < miny) miny = v.y;
            if (v.z < minz) minz = v.z;
            if (v.x > maxx) maxx = v.x;
            if (v.y > maxy) maxy = v.y;
            if (v.z > maxz) maxz = v.z;
        }
        printf("%s bbox_min=", tag);
        f32(minx); printf(" "); f32(miny); printf(" "); f32(minz);
        printf(" bbox_max=");
        f32(maxx); printf(" "); f32(maxy); printf(" "); f32(maxz);
        printf("\n");
    }
    printf("%s transform rot=[", tag);
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            f32(g.transform.rotation.m[i][j]);
            printf(" ");
        }
    }
    printf("] trans=");
    f32(g.transform.translation.x); printf(" ");
    f32(g.transform.translation.y); printf(" ");
    f32(g.transform.translation.z);
    printf(" scale=");
    f32(g.transform.scale);
    printf("\n");
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <file.nif>\n", argv[0]);
        return 2;
    }

    NIFParser parser;
    bool ok = parser.parseFile(argv[1]);
    printf("parse=%d\n", ok ? 1 : 0);

    const NIFHeader& h = parser.getHeader();
    printf("magic=%s\n", h.magic);
    printf("version=0x%08X endian=%u hasEndianByte=%d legacyNetImmerse=%d hasBlockTypeTable=%d\n",
           h.version, h.endian, h.hasEndianByte ? 1 : 0, h.legacyNetImmerse ? 1 : 0,
           h.hasBlockTypeTable ? 1 : 0);
    printf("userVersion=%u numObjects=%u unknown=%u numGroups=%u\n", h.userVersion, h.numObjects,
           h.unknownField, h.numGroups);
    printf("creator=%s processScript=%s exportScript=%s\n", h.creator.c_str(),
           h.processScript.c_str(), h.exportScript.c_str());
    printf("blockTypes=%zu\n", h.blockTypeNames.size());
    for (size_t i = 0; i < h.blockTypeNames.size(); ++i) {
        printf("  bt[%zu]=%s\n", i, h.blockTypeNames[i].c_str());
    }
    printf("blockTypeIndices=%zu hash=%016llx\n", h.blockTypeIndices.size(),
           static_cast<unsigned long long>(hash_vec(h.blockTypeIndices)));
    printf("blockDataOffset=%zu fileSize=%zu\n", h.blockDataOffset, parser.getFileSize());
    printf("blockCount=%u\n", parser.getBlockCount());

    const auto& nodes = parser.getNodes();
    printf("nodes=%zu\n", nodes.size());
    for (size_t i = 0; i < nodes.size(); ++i) {
        const auto& n = nodes[i];
        printf("node[%zu] name=%s type=%s typeIdx=%u parent=%d children=%zu hasGeom=%d\n", i,
               n->name.c_str(), n->blockTypeName.c_str(), n->blockTypeIndex, n->parentIndex,
               n->childIndices.size(), n->hasGeometry ? 1 : 0);
        for (size_t c = 0; c < n->childIndices.size(); ++c) {
            printf("  child[%zu]=%d\n", c, n->childIndices[c]);
        }
        if (n->hasGeometry) {
            char tag[32];
            snprintf(tag, sizeof(tag), "geom[%zu]", i);
            dump_geometry(tag, n->geometry);
        }
    }

    const auto& roots = parser.getRootNodeIndices();
    printf("roots=%zu\n", roots.size());
    for (size_t r = 0; r < roots.size(); ++r) {
        printf("  root[%zu]=%d\n", r, roots[r]);
    }

    if (!nodes.empty()) {
        auto found = parser.getNodeByName(nodes[0]->name);
        printf("getNodeByName(first)=%d\n", found ? 1 : 0);
    }

    auto all = parser.extractAllGeometry();
    printf("extractAllGeometry=%zu\n", all.size());
    for (size_t i = 0; i < all.size(); ++i) {
        char tag[32];
        snprintf(tag, sizeof(tag), "allgeom[%zu]", i);
        dump_geometry(tag, all[i]);
    }

    printf("hasNiNode=%d hasNiTriShape=%d hasNiTriStrips=%d\n",
           parser.hasBlockType("NiNode") ? 1 : 0, parser.hasBlockType("NiTriShape") ? 1 : 0,
           parser.hasBlockType("NiTriStrips") ? 1 : 0);
    printf("blockTypeName(0)=%s\n", parser.getBlockTypeName(0).c_str());
    return 0;
}
