#pragma once

// Phase 30: Skinned mesh shader sources
// Bone-weighted vertex transformation for NIF skeletal animation

namespace SkinningShader {

// Vertex shader: bone-weighted transformation
// Attributes: position(0), normal(1), texcoord(2), color(3), boneWeights(4), boneIndices(5)
// UBO binding 0: BoneMatrices (mat4[64])
inline const char* vertexSource = R"(#version 300 es
precision highp float;

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aColor;
layout(location = 4) in vec4 aBoneWeights;
layout(location = 5) in ivec4 aBoneIndices;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

layout(std140) uniform BoneMatrices {
    mat4 uBones[64];
};

out vec3 vNormal;
out vec2 vTexCoord;
out vec3 vColor;
out vec3 vWorldPos;

void main() {
    mat4 boneTransform =
        uBones[aBoneIndices.x] * aBoneWeights.x +
        uBones[aBoneIndices.y] * aBoneWeights.y +
        uBones[aBoneIndices.z] * aBoneWeights.z +
        uBones[aBoneIndices.w] * aBoneWeights.w;

    vec4 skinnedPos = boneTransform * vec4(aPosition, 1.0);
    vec3 skinnedNormal = mat3(boneTransform) * aNormal;

    vec4 worldPos = uModel * skinnedPos;
    vWorldPos = worldPos.xyz;
    vNormal = normalize(mat3(uModel) * skinnedNormal);
    vTexCoord = aTexCoord;
    vColor = aColor;

    gl_Position = uProjection * uView * worldPos;
}
)";

// Fragment shader: basic textured lighting (same as static mesh)
inline const char* fragmentSource = R"(#version 300 es
precision mediump float;

in vec3 vNormal;
in vec2 vTexCoord;
in vec3 vColor;
in vec3 vWorldPos;

uniform sampler2D uTexture;
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uAmbientColor;

out vec4 fragColor;

void main() {
    vec3 normal = normalize(vNormal);
    float NdotL = max(dot(normal, normalize(uLightDir)), 0.0);

    vec3 diffuse = uLightColor * NdotL;
    vec3 ambient = uAmbientColor;

    vec4 texColor = texture(uTexture, vTexCoord);
    vec3 litColor = texColor.rgb * (ambient + diffuse) * vColor;

    fragColor = vec4(litColor, texColor.a);
}
)";

} // namespace SkinningShader
