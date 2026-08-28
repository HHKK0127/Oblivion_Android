#pragma once

#include <cstdint>
#include <string>

// ============================================================================
// Phase 52: FaceGen Shader Definitions
//
// Vertex shader: morph target blending (up to 8 targets) + skin deformation
// Fragment shader: 4-layer texture blend + normal map + specular highlight
//
// These are GLSL ES 3.0 shaders for Android OpenGL ES 3.0
// ============================================================================

namespace facegen {

// ============================================================================
// Shader Uniform Locations
// ============================================================================

struct FaceGenUniforms {
    // Vertex shader uniforms
    int modelMatrix = -1;
    int viewMatrix = -1;
    int projectionMatrix = -1;
    int normalMatrix = -1;

    // Morph target uniforms (up to 8 targets)
    int morphWeights[8] = {-1, -1, -1, -1, -1, -1, -1, -1};
    int morphCount = -1;

    // Bone matrices for skinning (up to 64 bones)
    int boneMatrices = -1;
    int useSkinning = -1;

    // Fragment shader uniforms
    int baseTexture = -1;       // Layer 0: base skin
    int ageTexture = -1;        // Layer 1: age overlay (wrinkles, spots)
    int makeupTexture = -1;     // Layer 2: makeup overlay
    int detailTexture = -1;     // Layer 3: detail (scars, paint)
    int normalMap = -1;         // Normal map

    // Blend weights for texture layers
    int baseWeight = -1;
    int ageWeight = -1;
    int makeupWeight = -1;
    int detailWeight = -1;

    // Lighting
    int lightDirection = -1;
    int lightColor = -1;
    int ambientColor = -1;
    int specularPower = -1;
    int specularStrength = -1;

    // Skin properties
    int skinSubsurfaceColor = -1;
    int skinSubsurfaceRadius = -1;
};

// ============================================================================
// Vertex Shader Source (GLSL ES 3.0)
// ============================================================================

static const char* FACEGEN_VERTEX_SHADER = R"glsl(#version 300 es

// Vertex attributes
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texCoord;
layout(location = 3) in vec4 a_boneWeights;
layout(location = 4) in ivec4 a_boneIndices;

// Morph target attributes (locations 5-12 for up to 8 morph targets)
layout(location = 5)  in vec3 a_morph0_position;
layout(location = 6)  in vec3 a_morph0_normal;
layout(location = 7)  in vec3 a_morph1_position;
layout(location = 8)  in vec3 a_morph1_normal;
layout(location = 9)  in vec3 a_morph2_position;
layout(location = 10) in vec3 a_morph2_normal;
layout(location = 11) in vec3 a_morph3_position;
layout(location = 12) in vec3 a_morph3_normal;

// Uniforms
uniform mat4 u_modelMatrix;
uniform mat4 u_viewMatrix;
uniform mat4 u_projectionMatrix;
uniform mat3 u_normalMatrix;

// Morph weights
uniform float u_morphWeights[8];
uniform int u_morphCount;

// Skinning
uniform mat4 u_boneMatrices[64];
uniform int u_useSkinning;

// Outputs to fragment shader
out vec3 v_worldPosition;
out vec3 v_worldNormal;
out vec2 v_texCoord;
out vec3 v_viewDir;

void main() {
    // Start with base position/normal
    vec3 pos = a_position;
    vec3 norm = a_normal;

    // Apply morph targets (up to 4 in this batch, more via multi-pass)
    if (u_morphCount > 0) {
        pos += a_morph0_position * u_morphWeights[0];
        norm += a_morph0_normal * u_morphWeights[0];
    }
    if (u_morphCount > 1) {
        pos += a_morph1_position * u_morphWeights[1];
        norm += a_morph1_normal * u_morphWeights[1];
    }
    if (u_morphCount > 2) {
        pos += a_morph2_position * u_morphWeights[2];
        norm += a_morph2_normal * u_morphWeights[2];
    }
    if (u_morphCount > 3) {
        pos += a_morph3_position * u_morphWeights[3];
        norm += a_morph3_normal * u_morphWeights[3];
    }

    // Apply skinning (bone transformation)
    if (u_useSkinning != 0) {
        mat4 skinMatrix =
            u_boneMatrices[a_boneIndices.x] * a_boneWeights.x +
            u_boneMatrices[a_boneIndices.y] * a_boneWeights.y +
            u_boneMatrices[a_boneIndices.z] * a_boneWeights.z +
            u_boneMatrices[a_boneIndices.w] * a_boneWeights.w;

        pos = (skinMatrix * vec4(pos, 1.0)).xyz;
        norm = mat3(skinMatrix) * norm;
    }

    // Transform to world space
    vec4 worldPos = u_modelMatrix * vec4(pos, 1.0);
    v_worldPosition = worldPos.xyz;
    v_worldNormal = normalize(u_normalMatrix * norm);
    v_texCoord = a_texCoord;

    // View direction for specular
    vec3 cameraPos = vec3(inverse(u_viewMatrix)[3]);
    v_viewDir = normalize(cameraPos - worldPos.xyz);

    gl_Position = u_projectionMatrix * u_viewMatrix * worldPos;
}
)glsl";

// ============================================================================
// Fragment Shader Source (GLSL ES 3.0)
// ============================================================================

static const char* FACEGEN_FRAGMENT_SHADER = R"glsl(#version 300 es
precision mediump float;

// Inputs from vertex shader
in vec3 v_worldPosition;
in vec3 v_worldNormal;
in vec2 v_texCoord;
in vec3 v_viewDir;

// Texture samplers (4-layer blend)
uniform sampler2D u_baseTexture;      // Layer 0: base skin
uniform sampler2D u_ageTexture;       // Layer 1: age overlay
uniform sampler2D u_makeupTexture;    // Layer 2: makeup
uniform sampler2D u_detailTexture;    // Layer 3: scars/paint
uniform sampler2D u_normalMap;        // Normal map

// Blend weights (0.0 - 1.0)
uniform float u_baseWeight;
uniform float u_ageWeight;
uniform float u_makeupWeight;
uniform float u_detailWeight;

// Lighting uniforms
uniform vec3 u_lightDirection;
uniform vec3 u_lightColor;
uniform vec3 u_ambientColor;
uniform float u_specularPower;
uniform float u_specularStrength;

// Skin subsurface scattering approximation
uniform vec3 u_skinSubsurfaceColor;
uniform float u_skinSubsurfaceRadius;

// Output
out vec4 fragColor;

// Normal map perturbation
vec3 perturbNormal(vec3 normal, vec2 texCoord) {
    vec3 tangentNormal = texture(u_normalMap, texCoord).rgb * 2.0 - 1.0;

    // Compute TBN matrix (simplified - assumes tangent aligned with UV)
    vec3 Q1 = dFdx(v_worldPosition);
    vec3 Q2 = dFdy(v_worldPosition);
    vec2 st1 = dFdx(texCoord);
    vec2 st2 = dFdy(texCoord);

    vec3 T = normalize(Q1 * st2.y - Q2 * st1.y);
    vec3 B = -normalize(cross(normal, T));
    mat3 TBN = mat3(T, B, normal);

    return normalize(TBN * tangentNormal);
}

void main() {
    // Sample all texture layers
    vec4 baseColor = texture(u_baseTexture, v_texCoord);
    vec4 ageColor = texture(u_ageTexture, v_texCoord);
    vec4 makeupColor = texture(u_makeupTexture, v_texCoord);
    vec4 detailColor = texture(u_detailTexture, v_texCoord);

    // Blend textures (weighted overlay)
    // Layer 0: base skin (always present)
    vec3 blendedColor = baseColor.rgb * u_baseWeight;

    // Layer 1: age overlay (wrinkles, age spots)
    blendedColor = mix(blendedColor, ageColor.rgb, ageColor.a * u_ageWeight);

    // Layer 2: makeup overlay
    blendedColor = mix(blendedColor, makeupColor.rgb, makeupColor.a * u_makeupWeight);

    // Layer 3: detail overlay (scars, tattoos, paint)
    blendedColor = mix(blendedColor, detailColor.rgb, detailColor.a * u_detailWeight);

    // Perturb normal with normal map
    vec3 normal = normalize(v_worldNormal);
    normal = perturbNormal(normal, v_texCoord);

    // Diffuse lighting (Lambert)
    vec3 lightDir = normalize(-u_lightDirection);
    float NdotL = max(dot(normal, lightDir), 0.0);

    // Subsurface scattering approximation (wrap lighting)
    float wrapDiffuse = max(dot(normal, lightDir) + u_skinSubsurfaceRadius, 0.0) /
                        (1.0 + u_skinSubsurfaceRadius);
    vec3 subsurface = u_skinSubsurfaceColor * wrapDiffuse * (1.0 - NdotL);

    // Specular highlight (Blinn-Phong)
    vec3 viewDir = normalize(v_viewDir);
    vec3 halfDir = normalize(lightDir + viewDir);
    float NdotH = max(dot(normal, halfDir), 0.0);
    float spec = pow(NdotH, u_specularPower) * u_specularStrength;

    // Combine lighting
    vec3 diffuse = blendedColor * (u_ambientColor + u_lightColor * NdotL);
    vec3 specular = u_lightColor * spec;

    // Final color with subsurface
    vec3 finalColor = diffuse + specular + subsurface;

    // Alpha from base texture
    float alpha = baseColor.a;

    fragColor = vec4(finalColor, alpha);
}
)glsl";

// ============================================================================
// Simplified Shader (for low-end devices / fallback)
// ============================================================================

static const char* FACEGEN_SIMPLE_VERTEX_SHADER = R"glsl(#version 300 es

layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texCoord;

uniform mat4 u_modelMatrix;
uniform mat4 u_viewMatrix;
uniform mat4 u_projectionMatrix;

out vec3 v_normal;
out vec2 v_texCoord;

void main() {
    v_normal = mat3(u_modelMatrix) * a_normal;
    v_texCoord = a_texCoord;
    gl_Position = u_projectionMatrix * u_viewMatrix * u_modelMatrix * vec4(a_position, 1.0);
}
)glsl";

static const char* FACEGEN_SIMPLE_FRAGMENT_SHADER = R"glsl(#version 300 es
precision mediump float;

in vec3 v_normal;
in vec2 v_texCoord;

uniform sampler2D u_baseTexture;
uniform vec3 u_lightDirection;

out vec4 fragColor;

void main() {
    vec3 normal = normalize(v_normal);
    vec3 lightDir = normalize(-u_lightDirection);
    float NdotL = max(dot(normal, lightDir), 0.0);

    vec4 texColor = texture(u_baseTexture, v_texCoord);
    vec3 color = texColor.rgb * (0.3 + 0.7 * NdotL);

    fragColor = vec4(color, texColor.a);
}
)glsl";

// ============================================================================
// Shader Program IDs (compiled at init time)
// ============================================================================

struct FaceGenShaderProgram {
    uint32_t programId = 0;
    FaceGenUniforms uniforms;
    bool compiled = false;
};

} // namespace facegen
