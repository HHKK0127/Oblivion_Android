#pragma once

// Merges the geometries of a parsed NIF scene into one drawable buffer.
//
// A NIF scene holds its model as several NiTriShape / NiTriStrips blocks - an
// interior wall piece, for example, carries its wall, its floor and its doorway
// as separate geometries. Callers that own a single Mesh (AssetManager,
// WorldLoader) used to build from geometries[0] alone and silently dropped every
// other block, so the model rendered as one fragment of itself.

#include "nif_types.h"
#include "../geometry/mesh.h"

#include <cstddef>
#include <vector>

namespace oblivion {

struct MergedMeshData {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    // Geometries that contributed triangles to the merged buffers.
    size_t geometryCount = 0;
    // Geometries that contributed nothing: no vertices, no triangles, or only
    // triangles pointing past their own vertex array.
    size_t skippedGeometryCount = 0;
};

// Attribute arrays shorter than the vertex array keep the Vertex default, and
// triangle indices are rebased onto the merged vertex array. A geometry whose
// triangles are all unusable contributes no vertices, so the buffers hold only
// vertices that some index reaches.
MergedMeshData mergeGeometry(const std::vector<NIFGeometry>& geometries);

}  // namespace oblivion
