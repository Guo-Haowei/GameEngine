layout(location = 0) out vec3 out_base_color;
layout(location = 1) out vec3 out_position;
layout(location = 2) out vec3 out_normal;
layout(location = 3) out vec3 out_emissive_roughness_metallic;

in struct PS_INPUT {
    vec3 position;
    vec2 uv;
    vec3 T;
    vec3 B;
    vec3 N;
} ps_in;

#include "../cbuffer.h"

void main() {
    vec4 albedo = u_albedo_color;

    if (u_has_albedo_map != 0) {
        albedo = texture(u_albedo_map, ps_in.uv, 0);
    }
    if (albedo.a < 0.001) {
        discard;
    }

    float metallic = u_metallic;
    float roughness = u_roughness;
    if (u_has_pbr_map != 0) {
        // g roughness, b metallic
        vec3 mr = texture(u_pbr_map, ps_in.uv).rgb;
        metallic = mr.b;
        roughness = mr.g;
    }

    // TODO: get rid of branching
    vec3 N;
    if (u_has_normal_map != 0) {
        mat3 TBN = mat3(ps_in.T, ps_in.B, ps_in.N);
        N = normalize(TBN * (2.0 * texture(u_normal_map, ps_in.uv).xyz - 1.0));
    } else {
        N = normalize(ps_in.N);
    }

    out_base_color.rgb = albedo.rgb;
    out_position = ps_in.position;
    out_normal = N;

    out_emissive_roughness_metallic.r = u_emissive_power;
    out_emissive_roughness_metallic.g = roughness;
    out_emissive_roughness_metallic.b = metallic;
}
