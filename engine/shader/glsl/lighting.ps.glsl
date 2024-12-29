/// File: lighting.ps.glsl
#include "../cbuffer.hlsl.h"

layout(location = 0) out vec4 out_color;
layout(location = 0) in vec2 pass_uv;

#include "lighting.glsl"

void main() {
    const vec2 texcoord = pass_uv;
    const vec4 emissive_roughness_metallic = texture(t_GbufferMaterialMap, texcoord);
    if (emissive_roughness_metallic.a <= 0.0f) {
        discard;
    }

    //gl_FragDepth = depth;

    vec3 N = texture(t_GbufferNormalMap, texcoord).rgb;
    N = 2.0f * N - 1.0f;

    const vec3 world_position = texture(t_GbufferPositionMap, texcoord).rgb;
    float emissive = emissive_roughness_metallic.r;
    float roughness = emissive_roughness_metallic.g;
    float metallic = emissive_roughness_metallic.b;

    vec3 base_color = texture(t_GbufferBaseColorMap, texcoord).rgb;
    if (c_noTexture != 0) {
        base_color = vec3(0.6);
    }

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
    out_color = vec4(color, 1.0f);
}
