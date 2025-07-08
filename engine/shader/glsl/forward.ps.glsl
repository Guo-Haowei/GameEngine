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

uniform sampler2D u_Texture0;
uniform samplerCube u_Texture1;
uniform samplerCube u_Texture2;
uniform sampler2D u_Texture3;
uniform sampler2D u_Texture4;
uniform sampler2D u_Texture5;
uniform sampler3D u_Texture6;

#define t_ShadowMap         u_Texture0
#define t_DiffuseIrradiance u_Texture1
#define t_Prefiltered       u_Texture2
#define t_BrdfLut           u_Texture3
#define t_LTC1              u_Texture4
#define t_LTC2              u_Texture5
#define t_VoxelLighting     u_Texture6

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
    const vec3 world_position = (c_invView * vec4(view_position, 1.0f)).xyz;

    out_color.rgb = compute_lighting(t_ShadowMap,
                                     base_color,
                                     world_position,
                                     N,
                                     metallic,
                                     roughness,
                                     emissive);
}
