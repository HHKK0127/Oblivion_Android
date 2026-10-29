#include "mesh_builder.h"

#include <android/log.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include "assets_ffi.h"

#define LOG_TAG "MeshBuilder"
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

// This file is a shim: the merge rules live in Rust (oblivion-re-tools,
// crates/assets-core) behind the assets_ffi C ABI, and the signature here is
// unchanged so existing callers keep working.
//
// A failed call degrades to an empty MergedMeshData rather than a partial one: the
// caller uploads whatever it is handed straight to the GPU, so a half-filled
// buffer would draw a fragment of the model instead of nothing.

namespace oblivion {

namespace {

static_assert(sizeof(Vertex) == AFL_VERTEX_FLOATS * sizeof(float),
              "Vertex must stay a dense run of 11 floats: the merge writes the interleaved "
              "vertex array straight into it");

// One geometry flattened into the plain runs the C ABI reads. NIFVector3 and
// NIFVector4 are 3- and 4-float records, so this copy keeps the call independent
// of their exact layout and drops the unused w channel of the colours.
//
// The ABI counts elements, not floats (a position is one vertex, not three
// floats), so each run keeps its element count next to its flattened storage.
struct GeometryBuffers {
    std::vector<float> positions;
    std::vector<float> normals;
    std::vector<float> texCoords;
    std::vector<float> colors;
    std::vector<uint16_t> triangles;
    size_t vertexCount = 0;
    size_t normalCount = 0;
    size_t texCoordCount = 0;
    size_t colorCount = 0;
    size_t triangleCount = 0;
};

GeometryBuffers flattenGeometry(const NIFGeometry& geometry) {
    GeometryBuffers buffers;

    buffers.vertexCount = geometry.vertices.size();
    buffers.positions.reserve(buffers.vertexCount * 3);
    for (const NIFVector3& vertex : geometry.vertices) {
        buffers.positions.push_back(vertex.x);
        buffers.positions.push_back(vertex.y);
        buffers.positions.push_back(vertex.z);
    }

    buffers.normalCount = geometry.normals.size();
    buffers.normals.reserve(buffers.normalCount * 3);
    for (const NIFVector3& normal : geometry.normals) {
        buffers.normals.push_back(normal.x);
        buffers.normals.push_back(normal.y);
        buffers.normals.push_back(normal.z);
    }

    buffers.texCoordCount = geometry.texCoords.size();
    buffers.texCoords.reserve(buffers.texCoordCount * 2);
    for (const glm::vec2& texCoord : geometry.texCoords) {
        buffers.texCoords.push_back(texCoord.x);
        buffers.texCoords.push_back(texCoord.y);
    }

    buffers.colorCount = geometry.colors.size();
    buffers.colors.reserve(buffers.colorCount * 3);
    for (const NIFVector4& color : geometry.colors) {
        buffers.colors.push_back(color.x);
        buffers.colors.push_back(color.y);
        buffers.colors.push_back(color.z);
    }

    buffers.triangleCount = geometry.triangles.size();
    buffers.triangles.reserve(buffers.triangleCount * 3);
    for (const NIFTriangle& triangle : geometry.triangles) {
        buffers.triangles.push_back(triangle.v0);
        buffers.triangles.push_back(triangle.v1);
        buffers.triangles.push_back(triangle.v2);
    }

    return buffers;
}

afl_geometry_input describe(const GeometryBuffers& buffers) {
    afl_geometry_input input{};
    // An empty attribute array is passed as NULL with a zero count, which the ABI
    // reads as "attribute missing" rather than as a zero-length array at a null
    // address.
    if (!buffers.positions.empty()) {
        input.positions = buffers.positions.data();
        input.position_count = buffers.vertexCount;
    }
    if (!buffers.normals.empty()) {
        input.normals = buffers.normals.data();
        input.normal_count = buffers.normalCount;
    }
    if (!buffers.texCoords.empty()) {
        input.tex_coords = buffers.texCoords.data();
        input.tex_coord_count = buffers.texCoordCount;
    }
    if (!buffers.colors.empty()) {
        input.colors = buffers.colors.data();
        input.color_count = buffers.colorCount;
    }
    if (!buffers.triangles.empty()) {
        input.triangles = buffers.triangles.data();
        input.triangle_count = buffers.triangleCount;
    }
    return input;
}

}  // namespace

MergedMeshData mergeGeometry(const std::vector<NIFGeometry>& geometries) {
    MergedMeshData merged;

    // The C ABI reads parallel attribute runs per geometry, so the decoded blocks
    // are flattened once and referenced by both passes.
    std::vector<GeometryBuffers> buffers;
    buffers.reserve(geometries.size());
    for (const NIFGeometry& geometry : geometries) {
        buffers.push_back(flattenGeometry(geometry));
    }

    std::vector<afl_geometry_input> inputs;
    inputs.reserve(buffers.size());
    for (const GeometryBuffers& geometry : buffers) {
        inputs.push_back(describe(geometry));
    }
    const afl_geometry_input* rawInputs = inputs.empty() ? nullptr : inputs.data();

    // Size query first: the merged counts cannot be derived from the input,
    // because a geometry whose triangles all point past its own vertex array
    // contributes nothing.
    afl_merge_counts counts{};
    int status = afl_merge_geometry(rawInputs, inputs.size(), nullptr, 0, nullptr, 0, &counts);
    if (status != AFL_OK) {
        LOGD("afl_merge_geometry length query failed with %d, no geometry is merged", status);
        return merged;
    }
    if (counts.vertex_count == 0) {
        // Nothing usable, but the skip tally still describes what the input held.
        merged.geometryCount = counts.geometry_count;
        merged.skippedGeometryCount = counts.skipped_geometry_count;
        return merged;
    }

    std::vector<Vertex> vertices(counts.vertex_count);
    std::vector<unsigned int> indices(counts.index_count);
    status = afl_merge_geometry(rawInputs, inputs.size(),
                                reinterpret_cast<float*>(vertices.data()),
                                counts.vertex_count * AFL_VERTEX_FLOATS, indices.data(),
                                indices.size(), &counts);
    if (status != AFL_OK) {
        LOGD("afl_merge_geometry failed with %d, no geometry is merged", status);
        return merged;
    }

    // The buffers are only handed over once the call has filled them completely,
    // so a rejected call cannot leave a partial model behind.
    merged.vertices.swap(vertices);
    merged.indices.swap(indices);
    merged.geometryCount = counts.geometry_count;
    merged.skippedGeometryCount = counts.skipped_geometry_count;
    return merged;
}

}  // namespace oblivion
