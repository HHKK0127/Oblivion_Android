#include "mesh_builder.h"

namespace oblivion {

MergedMeshData mergeGeometry(const std::vector<NIFGeometry>& geometries) {
    MergedMeshData merged;

    for (const auto& geometry : geometries) {
        const size_t vertexCount = geometry.vertices.size();
        if (vertexCount == 0) {
            ++merged.skippedGeometryCount;
            continue;
        }

        // Collect this geometry's triangles first: a shape can decode its vertex
        // array and still have no usable triangle, and those vertices would be
        // unreachable once merged.
        std::vector<uint16_t> localIndices;
        localIndices.reserve(geometry.triangles.size() * 3);
        for (const auto& triangle : geometry.triangles) {
            if (triangle.v0 >= vertexCount || triangle.v1 >= vertexCount ||
                triangle.v2 >= vertexCount) {
                continue;
            }
            localIndices.push_back(triangle.v0);
            localIndices.push_back(triangle.v1);
            localIndices.push_back(triangle.v2);
        }
        if (localIndices.empty()) {
            ++merged.skippedGeometryCount;
            continue;
        }

        const unsigned int baseIndex = static_cast<unsigned int>(merged.vertices.size());

        for (size_t i = 0; i < vertexCount; i++) {
            Vertex vertex;
            vertex.position = geometry.vertices[i].toGLM();
            if (i < geometry.normals.size()) {
                vertex.normal = geometry.normals[i].toGLM();
            }
            if (i < geometry.texCoords.size()) {
                vertex.texCoord = geometry.texCoords[i];
            }
            if (i < geometry.colors.size()) {
                vertex.color = glm::vec3(geometry.colors[i].x,
                                         geometry.colors[i].y,
                                         geometry.colors[i].z);
            }
            merged.vertices.push_back(vertex);
        }

        for (const uint16_t index : localIndices) {
            merged.indices.push_back(baseIndex + index);
        }

        ++merged.geometryCount;
    }

    return merged;
}

}  // namespace oblivion
