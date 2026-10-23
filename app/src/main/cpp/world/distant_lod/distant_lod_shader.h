#pragma once

// ============================================================================
// Distant LOD Shader - GLSL ES 3.00 shader sources for LOD rendering
// Phase 50: Distant LOD System
// ============================================================================

// Vertex shader: transforms vertices, passes UV and vertex color to fragment
static const char* DISTANT_LOD_VERTEX_SHADER = R"(#version 300 es
precision highp float;

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec4 aColor;

uniform mat4 uViewProj;
uniform mat4 uModel;
uniform vec3 uCameraPos;

out vec2 vTexCoord;
out vec4 vColor;
out float vDistance;
out vec3 vWorldPos;

void main() {
    vec4 worldPos = uModel * vec4(aPosition, 1.0);
    vWorldPos = worldPos.xyz;
    gl_Position = uViewProj * worldPos;
    vTexCoord = aTexCoord;
    vColor = aColor;
    vDistance = length(worldPos.xyz - uCameraPos);
}
)";

// Fragment shader: texture sampling with distance fade and fog
static const char* DISTANT_LOD_FRAGMENT_SHADER = R"(#version 300 es
precision mediump float;

in vec2 vTexCoord;
in vec4 vColor;
in float vDistance;
in vec3 vWorldPos;

uniform sampler2D uTexture;
uniform vec4 uFadeParams;    // x=far fade start, y=far fade end, z=near fade start, w=near fade end
uniform vec4 uFogParams;     // x=density, y=startDist, z=fogR, w=fogG
uniform float uAlpha;        // Global alpha multiplier
uniform float uUseTexture;   // 1.0 to sample uTexture, 0.0 to use the vertex colour only

out vec4 fragColor;

void main() {
    // Sample texture. Meshes without a texture (generated terrain LOD, horizon
    // ring) use their vertex colour; sampling a texture unit with no texture
    // bound is undefined and would leak the previously bound texture.
    vec4 baseColor = (uUseTexture > 0.5) ? texture(uTexture, vTexCoord) * vColor : vColor;

    // Far fade: linear alpha falloff with distance. Disabled when the range is
    // empty (end <= start), which is how whole-landscape block LOD meshes opt
    // out - their extent reaches far beyond any useful fade end.
    float fadeFactor = 1.0;
    if (uFadeParams.y > uFadeParams.x) {
        float fadeStart = uFadeParams.x;
        float fadeEnd = uFadeParams.y;
        fadeFactor = 1.0 - clamp((vDistance - fadeStart) / max(fadeEnd - fadeStart, 1.0), 0.0, 1.0);
    }

    // Near fade: ramps a mesh in once the camera is far enough, so a coarse
    // mesh overlapping high-detail geometry does not show through as a shell.
    // Disabled when the range is empty (w <= z).
    if (uFadeParams.w > uFadeParams.z) {
        fadeFactor *= clamp((vDistance - uFadeParams.z) / max(uFadeParams.w - uFadeParams.z, 1.0), 0.0, 1.0);
    }

    // Fog: exponential distance fog
    float fogDensity = uFogParams.x;
    float fogStart = uFogParams.y;
    float fogFactor = 1.0 - clamp(exp(-fogDensity * max(vDistance - fogStart, 0.0)), 0.0, 1.0);
    vec3 fogColor = vec3(uFogParams.z, uFogParams.w, 0.85);

    // Apply fog to base color
    vec3 finalColor = mix(baseColor.rgb, fogColor, fogFactor);

    // Apply distance fade and global alpha
    float finalAlpha = baseColor.a * fadeFactor * uAlpha;

    fragColor = vec4(finalColor, finalAlpha);
}
)";

// Horizon ring vertex shader (simpler, no texture)
static const char* HORIZON_VERTEX_SHADER = R"(#version 300 es
precision highp float;

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec4 aColor;

uniform mat4 uViewProj;
uniform vec3 uCameraPos;

out vec4 vColor;
out float vHeight;

void main() {
    // Offset ring to follow camera XZ position
    vec3 pos = aPosition;
    pos.x += uCameraPos.x;
    pos.z += uCameraPos.z;

    gl_Position = uViewProj * vec4(pos, 1.0);
    vColor = aColor;
    vHeight = aPosition.y;
}
)";

// Horizon ring fragment shader
static const char* HORIZON_FRAGMENT_SHADER = R"(#version 300 es
precision mediump float;

in vec4 vColor;
in float vHeight;

out vec4 fragColor;

void main() {
    // Gradient from base (dark) to peak (sky-tinted)
    float heightFactor = clamp(vHeight / 200.0, 0.0, 1.0);
    vec3 baseColor = vColor.rgb;

    // Blend toward sky color at peaks
    vec3 skyColor = vec3(0.55, 0.65, 0.85);
    vec3 finalColor = mix(baseColor, skyColor, heightFactor * 0.6);

    fragColor = vec4(finalColor, vColor.a);
}
)";
