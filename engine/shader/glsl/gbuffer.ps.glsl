/// File: gbuffer.ps.glsl
layout(location = 0) out vec4 out_base_color;
layout(location = 1) out vec4 out_normal;
layout(location = 2) out vec4 out_emissive_roughness_metallic;

in struct PS_INPUT {
    vec3 position;
    vec2 uv;
    vec3 T;
    vec3 B;
    vec3 N;
} ps_in;

#include "../cbuffer.hlsl.h"

void main() {
    vec4 albedo = c_baseColor;

    if (c_hasBaseColorMap != 0) {
        albedo = texture(c_BaseColorMapResidentHandle, ps_in.uv, 0);
    }
    if (albedo.a < 0.001) {
        discard;
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

    out_base_color = albedo;
    out_normal.rgb = 0.5f * N + 0.5f;
    out_normal.a = 1.0f;

    out_emissive_roughness_metallic.r = c_emissivePower;
    out_emissive_roughness_metallic.g = roughness;
    out_emissive_roughness_metallic.b = metallic;
    out_emissive_roughness_metallic.a = 1.0f;
}
