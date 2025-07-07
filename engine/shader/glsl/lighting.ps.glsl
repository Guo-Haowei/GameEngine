/// File: lighting.ps.glsl
#include "../cbuffer.hlsl.h"
#include "../common.hlsl.h"

layout(location = 0) out vec4 out_color;
layout(location = 0) in vec2 pass_uv;

#include "lighting.glsl"

uniform sampler2D u_Texture0;
uniform sampler2D u_Texture1;
uniform sampler2D u_Texture2;
uniform sampler2D u_Texture3;
uniform sampler2D u_Texture4;
uniform sampler2D u_Texture5;

#define t_GbufferBaseColorMap u_Texture0
#define t_GbufferNormalMap    u_Texture1
#define t_GbufferMaterialMap  u_Texture2
#define t_GbufferDepth        u_Texture3
#define t_SsaoMap             u_Texture4
#define t_ShadowMap           u_Texture5
// #define t_PointShadowArray

void main() {
    const vec2 uv = pass_uv;
    const vec4 emissive_roughness_metallic = texture(t_GbufferMaterialMap, uv);
    if (emissive_roughness_metallic.a <= 0.0f) {
        discard;
    }

    vec3 N = texture(t_GbufferNormalMap, uv).rgb;
    N = 2.0f * N - 1.0f;

    const float depth = texture(t_GbufferDepth, uv).r;
    const Vector3f view_position = NdcToViewPos(uv, depth);
    const vec3 world_position = (c_invView * vec4(view_position, 1.0f)).xyz;
    float emissive = emissive_roughness_metallic.r;
    float roughness = emissive_roughness_metallic.g;
    float metallic = emissive_roughness_metallic.b;

    vec3 base_color = texture(t_GbufferBaseColorMap, uv).rgb;
    if (emissive > 0.0) {
        out_color = vec4(emissive * base_color, 1.0);
        return;
    }

    vec3 color = compute_lighting(base_color,
                                  world_position,
                                  N,
                                  metallic,
                                  roughness,
                                  emissive);
    if (c_ssaoEnabled != 0) {
        float ao = texture(t_SsaoMap, uv).r;
        color *= ao;
    }

    out_color = vec4(color, 1.0f);
}
