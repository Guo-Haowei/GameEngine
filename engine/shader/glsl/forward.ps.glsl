/// File: forward.ps.glsl
layout(location = 0) out vec4 out_color;

in struct PS_INPUT {
    vec3 position;
    vec2 uv;
    vec3 T;
    vec3 B;
    vec3 N;
} ps_in;

#include "../cbuffer.hlsl.h"
#include "lighting.glsl"

void main() {
    vec3 base_color = c_baseColor.rgb;
    out_color.a = c_baseColor.a;

    if (c_hasBaseColorMap != 0) {
        vec4 tmp = texture(c_BaseColorMapResidentHandle, ps_in.uv, 0);
        base_color = tmp.rgb;
        out_color.a = tmp.a;
    }

    float metallic = c_metallic;
    float roughness = c_roughness;
    if (c_hasMaterialMap != 0) {
        vec3 mr = texture(c_MaterialMapResidentHandle, ps_in.uv).rgb;
        metallic = mr.b;
        roughness = mr.g;
    }

    // TODO: get rid of branching
    vec3 N;
    if (c_hasNormalMap != 0) {
        mat3 TBN = mat3(ps_in.T, ps_in.B, ps_in.N);
        N = normalize(TBN * (2.0 * texture(c_NormalMapResidentHandle, ps_in.uv).xyz - 1.0));
    } else {
        N = normalize(ps_in.N);
    }

    const float emissive = c_emissivePower;
    const vec3 view_position = ps_in.position;
    const vec3 world_position = (c_invViewMatrix * vec4(view_position, 1.0f)).xyz;

    out_color.rgb = compute_lighting(base_color,
                                     world_position,
                                     N,
                                     metallic,
                                     roughness,
                                     emissive);
}
