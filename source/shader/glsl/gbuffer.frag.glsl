layout(location = 0) out vec3 out_base_color;
layout(location = 1) out vec4 out_position_metallic;
layout(location = 2) out vec3 out_normal;
layout(location = 3) out vec3 out_emissive_roughness_metallic;

in struct PS_INPUT {
    vec3 position;
    vec2 uv;
    vec3 T;
    vec3 B;
    vec3 N;
} ps_in;

#include "../hlsl/cbuffer.h"

void main() {
    vec4 albedo = c_albedo_color;

    if (c_has_albedo_map != 0) {
        albedo = texture(c_albedo_map, ps_in.uv, 0);
    }
    if (albedo.a < 0.001) {
        discard;
    }

    float metallic = c_metallic;
    float roughness = c_roughness;
    if (c_has_pbr_map != 0) {
        // g roughness, b metallic
        vec3 mr = texture(c_pbr_map, ps_in.uv).rgb;
        metallic = mr.b;
        roughness = mr.g;
    }

    out_position_metallic.xyz = ps_in.position;
    out_position_metallic.w = metallic;

    // TODO: get rid of branching
    vec3 N;
    if (c_has_normal_map != 0) {
        mat3 TBN = mat3(ps_in.T, ps_in.B, ps_in.N);
        N = normalize(TBN * (2.0 * texture(c_normal_map, ps_in.uv).xyz - 1.0));
    } else {
        N = normalize(ps_in.N);
    }

    out_normal = 0.5 * (N + vec3(1.0));

    out_base_color.rgb = albedo.rgb;

    out_emissive_roughness_metallic.r = c_emissive_power;
    out_emissive_roughness_metallic.g = roughness;
    out_emissive_roughness_metallic.b = metallic;
}
