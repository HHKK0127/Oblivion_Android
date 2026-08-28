#pragma once

// ============================================================================
// Tree Billboard Shader - GLSL shader sources for vegetation rendering
// Phase 51: Billboard + wind sway shader for SpeedTree alternative
// ============================================================================

#include <string>

namespace vegetation {
namespace shaders {

// ============================================================================
// Billboard Vertex Shader
// - Billboard rotation (camera-facing)
// - Wind sway (leaf vertex deformation based on time)
// - Distance fade
// ============================================================================
static const std::string BILLBOARD_VERTEX = R"glsl(#version 300 es
precision highp float;

// Per-vertex attributes (billboard quad)
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;

// Per-instance attributes
layout(location = 2) in vec3 aInstancePos;
layout(location = 3) in float aInstanceScale;
layout(location = 4) in float aInstanceRotation;
layout(location = 5) in float aInstanceWindOffset;

// Uniforms
uniform mat4 uViewProj;
uniform vec3 uCameraPos;
uniform vec3 uWindDirection;
uniform float uWindStrength;
uniform float uWindTime;
uniform float uSwayPeriod;
uniform float uVertexAmplitude;
uniform float uMaxDrawDistance;
uniform float uFadeStartDist;

// Outputs to fragment shader
out vec2 vTexCoord;
out float vFadeFactor;
out float vFogFactor;

// Simple hash for per-tree variation
float hash(float n) {
    return fract(sin(n) * 43758.5453123);
}

void main() {
    // Billboard: compute camera-facing orientation
    vec3 worldPos = aInstancePos;
    vec3 toCamera = normalize(uCameraPos - worldPos);

    // Build billboard basis (always face camera on Y axis)
    vec3 right = normalize(cross(vec3(0.0, 1.0, 0.0), toCamera));
    vec3 up = vec3(0.0, 1.0, 0.0);
    vec3 forward = toCamera;

    // Apply per-instance rotation around Y axis
    float cosR = cos(aInstanceRotation);
    float sinR = sin(aInstanceRotation);
    vec3 rotatedRight = right * cosR + forward * sinR;

    // Wind sway - affects upper vertices more
    float heightFactor = max(aPosition.y, 0.0);
    float windPhase = uWindTime * uSwayPeriod + aInstanceWindOffset;
    float sway = sin(windPhase) * uVertexAmplitude * uWindStrength;
    sway += sin(windPhase * 2.3) * uVertexAmplitude * 0.3 * uWindStrength;

    // Apply wind displacement (stronger at top)
    vec3 windOffset = uWindDirection * sway * heightFactor;

    // Scale and position
    vec3 scaledPos = aPosition * aInstanceScale;
    vec3 worldOffset = rotatedRight * scaledPos.x + up * scaledPos.y + windOffset;

    vec4 finalPos = vec4(worldPos + worldOffset, 1.0);
    gl_Position = uViewProj * finalPos;

    vTexCoord = aTexCoord;

    // Distance fade
    float dist = length(uCameraPos - worldPos);
    float fadeRange = uMaxDrawDistance - uFadeStartDist;
    vFadeFactor = 1.0 - clamp((dist - uFadeStartDist) / max(fadeRange, 1.0), 0.0, 1.0);

    // Fog factor (exponential)
    float fogDensity = 0.005;
    vFogFactor = exp(-dist * fogDensity);
}
)glsl";

// ============================================================================
// Billboard Fragment Shader
// - Alpha test (leaf cutout)
// - Distance fog
// ============================================================================
static const std::string BILLBOARD_FRAGMENT = R"glsl(#version 300 es
precision highp float;

in vec2 vTexCoord;
in float vFadeFactor;
in float vFogFactor;

uniform sampler2D uTexture;
uniform vec4 uFogColor;
uniform float uAlphaThreshold;

out vec4 fragColor;

void main() {
    vec4 texColor = texture(uTexture, vTexCoord);

    // Alpha test for leaf cutout
    if (texColor.a < uAlphaThreshold) {
        discard;
    }

    // Apply distance fade
    texColor.a *= vFadeFactor;

    // Apply fog
    vec3 finalColor = mix(uFogColor.rgb, texColor.rgb, vFogFactor);
    fragColor = vec4(finalColor, texColor.a);
}
)glsl";

// ============================================================================
// Mesh Tree Vertex Shader (NEAR/MID/FAR LOD)
// - Standard model-view-projection
// - Wind sway (stronger at top vertices based on UV.y or vertex height)
// - Per-instance transform
// ============================================================================
static const std::string MESH_VERTEX = R"glsl(#version 300 es
precision highp float;

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec3 aNormal;

// Per-instance
layout(location = 3) in vec3 aInstancePos;
layout(location = 4) in float aInstanceScale;
layout(location = 5) in float aInstanceRotation;
layout(location = 6) in float aInstanceWindOffset;

uniform mat4 uViewProj;
uniform vec3 uCameraPos;
uniform vec3 uWindDirection;
uniform float uWindStrength;
uniform float uWindTime;
uniform float uSwayPeriod;
uniform float uVertexAmplitude;
uniform vec3 uLightDir;

out vec2 vTexCoord;
out vec3 vNormal;
out float vDiffuse;
out float vFogFactor;
out float vFadeFactor;

float hash(float n) {
    return fract(sin(n) * 43758.5453123);
}

void main() {
    // Per-instance rotation
    float cosR = cos(aInstanceRotation);
    float sinR = sin(aInstanceRotation);
    vec3 localPos = aPosition;
    float rx = localPos.x * cosR - localPos.z * sinR;
    float rz = localPos.x * sinR + localPos.z * cosR;
    localPos.x = rx;
    localPos.z = rz;

    // Wind sway (height-based)
    float heightFactor = clamp(aPosition.y / max(aInstanceScale, 0.1), 0.0, 1.0);
    float windPhase = uWindTime * uSwayPeriod + aInstanceWindOffset;
    float sway = sin(windPhase) * uVertexAmplitude * uWindStrength * heightFactor;
    sway += sin(windPhase * 1.7) * uVertexAmplitude * 0.2 * uWindStrength * heightFactor;

    vec3 windOffset = uWindDirection * sway;

    // Scale and translate
    vec3 worldPos = localPos * aInstanceScale + aInstancePos + windOffset;

    gl_Position = uViewProj * vec4(worldPos, 1.0);

    vTexCoord = aTexCoord;

    // Rotate normal
    vec3 n = aNormal;
    float nx = n.x * cosR - n.z * sinR;
    float nz = n.x * sinR + n.z * cosR;
    n.x = nx;
    n.z = nz;
    vNormal = normalize(n);

    // Simple diffuse lighting
    vDiffuse = max(dot(vNormal, normalize(uLightDir)), 0.3);

    // Distance fog
    float dist = length(uCameraPos - worldPos);
    float fogDensity = 0.005;
    vFogFactor = exp(-dist * fogDensity);

    // Fade
    float maxDist = 200.0;
    float fadeStart = 150.0;
    float fadeRange = maxDist - fadeStart;
    vFadeFactor = 1.0 - clamp((dist - fadeStart) / max(fadeRange, 1.0), 0.0, 1.0);
}
)glsl";

// ============================================================================
// Mesh Tree Fragment Shader
// - Texture sampling with alpha test
// - Diffuse lighting
// - Fog
// ============================================================================
static const std::string MESH_FRAGMENT = R"glsl(#version 300 es
precision highp float;

in vec2 vTexCoord;
in vec3 vNormal;
in float vDiffuse;
in float vFogFactor;
in float vFadeFactor;

uniform sampler2D uTexture;
uniform vec4 uFogColor;
uniform float uAlphaThreshold;

out vec4 fragColor;

void main() {
    vec4 texColor = texture(uTexture, vTexCoord);

    // Alpha test
    if (texColor.a < uAlphaThreshold) {
        discard;
    }

    // Apply diffuse lighting
    vec3 litColor = texColor.rgb * vDiffuse;

    // Apply fog
    vec3 finalColor = mix(uFogColor.rgb, litColor, vFogFactor);

    // Apply fade
    float alpha = texColor.a * vFadeFactor;

    fragColor = vec4(finalColor, alpha);
}
)glsl";

} // namespace shaders
} // namespace vegetation
