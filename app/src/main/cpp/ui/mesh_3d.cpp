#include "mesh_3d.h"
#include <cmath>

void Mesh3D::generateCube(std::vector<float>& vertices,
                           std::vector<unsigned short>& indices,
                           float size) {
    float h = size * 0.5f;
    // 6 faces, each with 4 unique vertices (for proper normals)
    // Face order: +X, -X, +Y, -Y, +Z, -Z
    struct Face {
        glm::vec3 normal;
        glm::vec3 v0, v1, v2, v3;  // CCW when viewed from outside
        glm::vec2 uv0, uv1, uv2, uv3;
    };

    Face faces[6] = {
        // +X (right)
        {glm::vec3(1,0,0),
         glm::vec3(h,-h,-h), glm::vec3(h,h,-h), glm::vec3(h,h,h), glm::vec3(h,-h,h),
         glm::vec2(0,0), glm::vec2(1,0), glm::vec2(1,1), glm::vec2(0,1)},
        // -X (left)
        {glm::vec3(-1,0,0),
         glm::vec3(-h,-h,h), glm::vec3(-h,h,h), glm::vec3(-h,h,-h), glm::vec3(-h,-h,-h),
         glm::vec2(0,0), glm::vec2(1,0), glm::vec2(1,1), glm::vec2(0,1)},
        // +Y (top)
        {glm::vec3(0,1,0),
         glm::vec3(-h,h,-h), glm::vec3(-h,h,h), glm::vec3(h,h,h), glm::vec3(h,h,-h),
         glm::vec2(0,0), glm::vec2(1,0), glm::vec2(1,1), glm::vec2(0,1)},
        // -Y (bottom)
        {glm::vec3(0,-1,0),
         glm::vec3(-h,-h,h), glm::vec3(-h,-h,-h), glm::vec3(h,-h,-h), glm::vec3(h,-h,h),
         glm::vec2(0,0), glm::vec2(1,0), glm::vec2(1,1), glm::vec2(0,1)},
        // +Z (front)
        {glm::vec3(0,0,1),
         glm::vec3(-h,-h,h), glm::vec3(h,-h,h), glm::vec3(h,h,h), glm::vec3(-h,h,h),
         glm::vec2(0,0), glm::vec2(1,0), glm::vec2(1,1), glm::vec2(0,1)},
        // -Z (back)
        {glm::vec3(0,0,-1),
         glm::vec3(h,-h,-h), glm::vec3(-h,-h,-h), glm::vec3(-h,h,-h), glm::vec3(h,h,-h),
         glm::vec2(0,0), glm::vec2(1,0), glm::vec2(1,1), glm::vec2(0,1)}
    };

    vertices.clear();
    indices.clear();

    for (int f = 0; f < 6; ++f) {
        const Face& face = faces[f];
        unsigned short baseIdx = static_cast<unsigned short>(vertices.size() / 8);

        // Add 4 vertices (position + normal + uv)
        for (int v = 0; v < 4; ++v) {
            glm::vec3 pos = (&face.v0)[v];
            glm::vec2 uv = (&face.uv0)[v];
            vertices.push_back(pos.x);
            vertices.push_back(pos.y);
            vertices.push_back(pos.z);
            vertices.push_back(face.normal.x);
            vertices.push_back(face.normal.y);
            vertices.push_back(face.normal.z);
            vertices.push_back(uv.x);
            vertices.push_back(uv.y);
        }

        // Two triangles: 0-1-2, 0-2-3
        indices.push_back(baseIdx + 0);
        indices.push_back(baseIdx + 1);
        indices.push_back(baseIdx + 2);
        indices.push_back(baseIdx + 0);
        indices.push_back(baseIdx + 2);
        indices.push_back(baseIdx + 3);
    }
}

void Mesh3D::generateSphere(std::vector<float>& vertices,
                             std::vector<unsigned short>& indices,
                             float radius,
                             int stacks,
                             int slices) {
    vertices.clear();
    indices.clear();

    for (int i = 0; i <= stacks; ++i) {
        float v = static_cast<float>(i) / stacks;
        float phi = v * 3.14159265f;  // 0..PI
        float sinPhi = std::sin(phi);
        float cosPhi = std::cos(phi);

        for (int j = 0; j <= slices; ++j) {
            float u = static_cast<float>(j) / slices;
            float theta = u * 2.0f * 3.14159265f;  // 0..2PI
            float sinTheta = std::sin(theta);
            float cosTheta = std::cos(theta);

            float x = sinPhi * cosTheta;
            float y = cosPhi;
            float z = sinPhi * sinTheta;

            vertices.push_back(x * radius);
            vertices.push_back(y * radius);
            vertices.push_back(z * radius);
            vertices.push_back(x);
            vertices.push_back(y);
            vertices.push_back(z);
            vertices.push_back(u);
            vertices.push_back(v);
        }
    }

    int cols = slices + 1;
    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < slices; ++j) {
            unsigned short a = static_cast<unsigned short>(i * cols + j);
            unsigned short b = static_cast<unsigned short>(i * cols + (j + 1));
            unsigned short c = static_cast<unsigned short>((i + 1) * cols + j);
            unsigned short d = static_cast<unsigned short>((i + 1) * cols + (j + 1));

            indices.push_back(a);
            indices.push_back(c);
            indices.push_back(b);
            indices.push_back(b);
            indices.push_back(c);
            indices.push_back(d);
        }
    }
}

void Mesh3D::generatePlane(std::vector<float>& vertices,
                            std::vector<unsigned short>& indices,
                            float size) {
    float h = size * 0.5f;
    vertices.clear();
    indices.clear();

    // Quad facing +Z, normal pointing +Z
    float v[4 * 8] = {
        // position xyz        normal xyz        uv
        -h, -h, 0.0f,         0.0f, 0.0f, 1.0f,   0.0f, 0.0f,
         h, -h, 0.0f,         0.0f, 0.0f, 1.0f,   1.0f, 0.0f,
         h,  h, 0.0f,         0.0f, 0.0f, 1.0f,   1.0f, 1.0f,
        -h,  h, 0.0f,         0.0f, 0.0f, 1.0f,   0.0f, 1.0f,
    };
    for (int i = 0; i < 32; ++i) vertices.push_back(v[i]);

    indices.push_back(0);
    indices.push_back(1);
    indices.push_back(2);
    indices.push_back(0);
    indices.push_back(2);
    indices.push_back(3);
}
