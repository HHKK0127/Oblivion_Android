// NIF geometry merge tests - see mesh_builder_tests.h for the rationale.

#include "mesh_builder_tests.h"

#include "../assets/mesh_builder.h"

#include <chrono>

namespace {

bool sameVec3(const glm::vec3& a, const glm::vec3& b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

bool sameVec2(const glm::vec2& a, const glm::vec2& b) {
    return a.x == b.x && a.y == b.y;
}

NIFGeometry makeGeometry(const char* name, const std::vector<glm::vec3>& positions,
                         const std::vector<NIFTriangle>& triangles) {
    NIFGeometry geometry;
    geometry.name = name;
    for (const auto& p : positions) {
        geometry.vertices.push_back(NIFVector3(p.x, p.y, p.z));
    }
    geometry.triangles = triangles;
    return geometry;
}

// A quad: 4 vertices, 2 triangles, indices 0..3.
NIFGeometry makeQuad(const char* name, float xOffset) {
    std::vector<NIFTriangle> triangles = {
        {0, 1, 2},
        {0, 2, 3},
    };
    return makeGeometry(name,
                        {{xOffset, 0.0f, 0.0f},
                         {xOffset + 1.0f, 0.0f, 0.0f},
                         {xOffset + 1.0f, 1.0f, 0.0f},
                         {xOffset, 1.0f, 0.0f}},
                        triangles);
}

}  // namespace

void MeshBuilderTests::record(const std::string& name, bool passed,
                              const std::string& msg, float ms) {
    results.push_back({name, passed, msg, ms});
}

int MeshBuilderTests::getPassCount() const {
    int n = 0;
    for (const auto& r : results) if (r.passed) n++;
    return n;
}

int MeshBuilderTests::getFailCount() const {
    int n = 0;
    for (const auto& r : results) if (!r.passed) n++;
    return n;
}

std::string MeshBuilderTests::getSummary() const {
    return "Mesh Builder Test Results\n"
           "Total: " + std::to_string(results.size()) +
           " | Pass: " + std::to_string(getPassCount()) +
           " | Fail: " + std::to_string(getFailCount());
}

bool MeshBuilderTests::runAllTests() {
    results.clear();
    const auto t0 = std::chrono::high_resolution_clock::now();

    // 1. The regression: a scene of several shapes must merge into one buffer,
    //    not collapse to its first shape.
    {
        const std::vector<NIFGeometry> geometries = {
            makeQuad("wall", 0.0f),
            makeQuad("floor", 10.0f),
            makeGeometry("doorway",
                         {{20.0f, 0.0f, 0.0f}, {21.0f, 0.0f, 0.0f}, {20.0f, 2.0f, 0.0f}},
                         {{0, 1, 2}}),
        };
        const oblivion::MergedMeshData merged = oblivion::mergeGeometry(geometries);

        record("every geometry contributes vertices", merged.vertices.size() == 11,
               "vertices=" + std::to_string(merged.vertices.size()));
        record("every geometry contributes indices", merged.indices.size() == 15,
               "indices=" + std::to_string(merged.indices.size()));
        record("all geometries are counted", merged.geometryCount == 3,
               "geometryCount=" + std::to_string(merged.geometryCount));
        record("nothing is skipped", merged.skippedGeometryCount == 0);

        // The first block keeps its original numbering, so a scene with one
        // shape is byte-identical to the old single-geometry path.
        record("the first geometry keeps its indices",
               merged.indices[0] == 0 && merged.indices[1] == 1 && merged.indices[2] == 2);

        // The second and third blocks are rebased by 4 and 8.
        record("the second geometry is rebased by its vertex count",
               merged.indices[6] == 4 && merged.indices[7] == 5 && merged.indices[8] == 6,
               "indices[6..8]=" + std::to_string(merged.indices[6]) + "," +
               std::to_string(merged.indices[7]) + "," + std::to_string(merged.indices[8]));
        record("the third geometry is rebased by the running vertex count",
               merged.indices[12] == 8 && merged.indices[13] == 9 && merged.indices[14] == 10,
               "indices[12..14]=" + std::to_string(merged.indices[12]) + "," +
               std::to_string(merged.indices[13]) + "," + std::to_string(merged.indices[14]));

        // The dropped-block bug in one assertion: the doorway's own position is
        // present at its rebased offset, which the old code never reached.
        record("a later geometry's vertices survive at the rebased offset",
               sameVec3(merged.vertices[8].position, glm::vec3(20.0f, 0.0f, 0.0f)),
               "vertex8=(" + std::to_string(merged.vertices[8].position.x) + "," +
               std::to_string(merged.vertices[8].position.y) + "," +
               std::to_string(merged.vertices[8].position.z) + ")");
    }

    // 2. Every index must address the merged vertex array: an out-of-range index
    //    is an out-of-bounds GPU read, not a visual glitch.
    {
        const std::vector<NIFGeometry> geometries = {
            makeQuad("a", 0.0f),
            makeQuad("b", 10.0f),
            makeGeometry("c", {{20.0f, 0.0f, 0.0f}, {21.0f, 0.0f, 0.0f}}, {{0, 1, 1}}),
        };
        const oblivion::MergedMeshData merged = oblivion::mergeGeometry(geometries);

        bool inRange = true;
        for (const unsigned int index : merged.indices) {
            if (index >= merged.vertices.size()) inRange = false;
        }
        record("every merged index is inside the merged vertex array", inRange,
               "vertices=" + std::to_string(merged.vertices.size()) +
               " indices=" + std::to_string(merged.indices.size()));
        record("the index buffer is whole triangles", merged.indices.size() % 3 == 0);
    }

    // 3. A geometry that carries only positions keeps the Vertex defaults, so a
    //    shape with no normals does not inherit the previous shape's data.
    {
        const std::vector<NIFGeometry> geometries = {
            makeGeometry("bare", {{1.0f, 2.0f, 3.0f}, {4.0f, 5.0f, 6.0f}, {7.0f, 8.0f, 9.0f}},
                         {{0, 1, 2}}),
        };
        const oblivion::MergedMeshData merged = oblivion::mergeGeometry(geometries);
        const Vertex expected;

        record("a position-only geometry keeps the default normal",
               sameVec3(merged.vertices[0].normal, expected.normal));
        record("a position-only geometry keeps the default texcoord",
               sameVec2(merged.vertices[0].texCoord, expected.texCoord));
        record("a position-only geometry keeps the default colour",
               sameVec3(merged.vertices[0].color, expected.color));
        record("a position-only geometry keeps its positions",
               sameVec3(merged.vertices[2].position, glm::vec3(7.0f, 8.0f, 9.0f)));
    }

    // 4. Supplied attributes are copied through.
    {
        NIFGeometry geometry = makeGeometry("textured",
                                            {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
                                            {{0, 1, 2}});
        geometry.normals.push_back(NIFVector3(0.0f, 0.0f, -1.0f));
        geometry.normals.push_back(NIFVector3(0.0f, 1.0f, 0.0f));
        geometry.normals.push_back(NIFVector3(1.0f, 0.0f, 0.0f));
        geometry.texCoords.push_back(glm::vec2(0.25f, 0.75f));
        geometry.texCoords.push_back(glm::vec2(0.5f, 0.5f));
        geometry.colors.push_back(NIFVector4(0.1f, 0.2f, 0.3f, 1.0f));

        const oblivion::MergedMeshData merged = oblivion::mergeGeometry({geometry});

        record("normals are copied when present",
               sameVec3(merged.vertices[2].normal, glm::vec3(1.0f, 0.0f, 0.0f)));
        record("texcoords are copied when present",
               sameVec2(merged.vertices[0].texCoord, glm::vec2(0.25f, 0.75f)));
        record("colours are copied when present",
               sameVec3(merged.vertices[0].color, glm::vec3(0.1f, 0.2f, 0.3f)));
        // A short attribute array falls back per vertex, not per geometry.
        record("a short attribute array falls back per vertex",
               sameVec2(merged.vertices[2].texCoord, Vertex().texCoord) &&
               sameVec3(merged.vertices[2].color, Vertex().color));
    }

    // 5. Unusable geometry is skipped and leaves no orphan vertices behind: a
    //    vertex no index reaches is memory the GPU still has to hold.
    {
        const std::vector<NIFGeometry> geometries = {
            makeGeometry("empty", {}, {}),
            makeGeometry("vertices only", {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}}, {}),
            makeGeometry("out of range", {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
                         {{0, 1, 2}}),
            makeQuad("good", 0.0f),
        };
        const oblivion::MergedMeshData merged = oblivion::mergeGeometry(geometries);

        record("geometry with no vertices is skipped", merged.skippedGeometryCount == 3,
               "skipped=" + std::to_string(merged.skippedGeometryCount));
        record("only the drawable geometry is counted", merged.geometryCount == 1);
        record("skipped geometry leaves no orphan vertices", merged.vertices.size() == 4,
               "vertices=" + std::to_string(merged.vertices.size()));
        record("a triangle past its own vertex array is dropped",
               merged.indices.size() == 6, "indices=" + std::to_string(merged.indices.size()));
    }

    // 6. A triangle that is usable next to an unusable one still merges.
    {
        const std::vector<NIFGeometry> geometries = {
            makeGeometry("mixed", {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
                         {{0, 1, 2}, {0, 1, 9}}),
        };
        const oblivion::MergedMeshData merged = oblivion::mergeGeometry(geometries);

        record("the usable triangle of a mixed geometry survives",
               merged.indices.size() == 3 && merged.geometryCount == 1);
        record("the unusable triangle of a mixed geometry is dropped",
               merged.indices[0] == 0 && merged.indices[1] == 1 && merged.indices[2] == 2);
    }

    // 7. An empty scene is reported as empty rather than as one blank shape.
    {
        const oblivion::MergedMeshData merged = oblivion::mergeGeometry({});

        record("an empty scene yields no vertices", merged.vertices.empty());
        record("an empty scene yields no indices", merged.indices.empty());
        record("an empty scene counts nothing", merged.geometryCount == 0 &&
               merged.skippedGeometryCount == 0);
    }

    const auto t1 = std::chrono::high_resolution_clock::now();
    const float totalMs = std::chrono::duration<float, std::milli>(t1 - t0).count();
    for (auto& r : results) r.durationMs = totalMs / static_cast<float>(results.size());

    return getFailCount() == 0;
}
