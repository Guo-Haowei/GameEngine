/// File: voxelization.ps.glsl
#include "../cbuffer.hlsl.h"

in vec3 pass_position;
in vec3 pass_normal;
in vec2 pass_uv;

uniform sampler2D u_Texture0;
uniform sampler2D u_Texture1;
uniform sampler2D u_Texture2;

#define t_ShadowMap u_Texture0
#define t_LTC1      u_Texture1
#define t_LTC2      u_Texture2

#define DISABLE_IBL
#define DISABLE_VXGI
#include "lighting.glsl"

layout(rgba16f, binding = 0) uniform image3D u_albedo_texture;
layout(rgba16f, binding = 1) uniform image3D u_normal_texture;

void main() {
    vec4 base_color = c_baseColor;
    if (c_hasBaseColorMap != 0) {
        base_color = texture(c_BaseColorMapResidentHandle, pass_uv);
    }
    if (base_color.a < 0.001) {
        discard;
    }

    float metallic = c_metallic;
    float roughness = c_roughness;
    if (c_hasMaterialMap != 0) {
        // g roughness, b metallic
        vec3 mr = texture(c_MaterialMapResidentHandle, pass_uv).rgb;
        metallic = mr.b;
        roughness = mr.g;
    }

    vec3 world_position = pass_position;

    const float emissive = c_emissivePower;
    const vec3 N = normalize(pass_normal);

    vec3 color = compute_lighting(t_ShadowMap,
                                  base_color.rgb,
                                  world_position,
                                  N,
                                  metallic,
                                  roughness,
                                  emissive);

    // write lighting information to texel
    vec3 voxel = (pass_position - c_voxelWorldCenter) / c_voxelWorldSizeHalf;  // normalize it to [-1, 1]
    voxel = 0.5 * voxel + vec3(0.5);                                           // normalize to [0, 1]
    ivec3 dim = imageSize(u_albedo_texture);
    ivec3 coord = ivec3(dim * voxel);

    f16vec4 final_color = f16vec4(color.r, color.g, color.b, 1.0);
    imageAtomicMax(u_albedo_texture, coord, final_color);

    // TODO: average normal
    f16vec4 normal_color = f16vec4(N.r, N.g, N.b, 1.0);
    imageAtomicAdd(u_normal_texture, coord, normal_color);
}
