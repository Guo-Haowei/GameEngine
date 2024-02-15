layout(rgba16f, binding = 0) uniform image3D u_albedo_texture;
layout(rgba16f, binding = 1) uniform image3D u_normal_texture;

#include "cbuffer.glsl.h"

in vec3 pass_position;
in vec3 pass_normal;
in vec2 pass_uv;
in vec4 pass_light_space_positions[NUM_CASCADES];

#include "common.glsl"
#include "common/lighting.glsl"

void main() {
    vec4 albedo = c_albedo_color;
    if (c_has_albedo_map != 0) {
        albedo = texture(c_albedo_maps[c_texture_map_idx], pass_uv);
    }
    if (albedo.a < 0.001) {
        discard;
    }

    float metallic = c_metallic;
    float roughness = c_roughness;
    if (c_has_pbr_map != 0) {
        // g roughness, b metallic
        vec3 mr = texture(c_pbr_maps[c_texture_map_idx], pass_uv).rgb;
        metallic = mr.b;
        roughness = mr.g;
    }

    vec3 world_position = pass_position;

    const vec3 N = normalize(pass_normal);
    const vec3 L = c_sun_direction;
    const vec3 V = normalize(c_camera_position - world_position);
    const vec3 H = normalize(V + L);
    const float NdotV = max(dot(N, V), 0.0);
    vec3 F0 = mix(vec3(0.04), albedo.rgb, metallic);
    vec3 radiance = c_light_color;
    vec3 Lo = lighting(N, L, V, radiance, F0, roughness, metallic, albedo, world_position);

    vec3 color = lighting(N, L, V, radiance, F0, roughness, metallic, albedo, world_position);

    ///////////////////////////////////////////////////////////////////////////

    // write lighting information to texel
    vec3 voxel = (pass_position - c_world_center) / c_world_size_half;  // normalize it to [-1, 1]
    voxel = 0.5 * voxel + vec3(0.5);                                    // normalize to [0, 1]
    ivec3 dim = imageSize(u_albedo_texture);
    ivec3 coord = ivec3(dim * voxel);

    f16vec4 final_color = f16vec4(color.r, color.g, color.b, 1.0);
    // imageAtomicAdd(u_albedo_texture, coord, final_color);
    imageAtomicMax(u_albedo_texture, coord, final_color);

    // TODO: average normal
    f16vec4 normal_color = f16vec4(N.r, N.g, N.b, 1.0);
    imageAtomicAdd(u_normal_texture, coord, normal_color);
}
