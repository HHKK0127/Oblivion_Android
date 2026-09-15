#pragma once

#include <vector>
#include <glm/glm.hpp>

/**
 * @brief Generate 3D primitives (cube, sphere) with UV mapping.
 * Vertices are interleaved: position(3) + normal(3) + uv(2) = 8 floats per vertex.
 * Index buffer is provided for triangle list rendering.
 */
class Mesh3D {
public:
    /**
     * Generate a unit cube centered at origin with side length 1.
     * Each face has its own normal for proper lighting.
     */
    static void generateCube(std::vector<float>& vertices,
                              std::vector<unsigned short>& indices,
                              float size = 1.0f);

    /**
     * Generate a UV sphere centered at origin with given radius.
     * @param radius Sphere radius
     * @param stacks Latitude divisions
     * @param slices Longitude divisions
     */
    static void generateSphere(std::vector<float>& vertices,
                                std::vector<unsigned short>& indices,
                                float radius = 1.0f,
                                int stacks = 16,
                                int slices = 24);

    /**
     * Generate a flat plane (single quad) facing +Z.
     */
    static void generatePlane(std::vector<float>& vertices,
                               std::vector<unsigned short>& indices,
                               float size = 1.0f);
};
