layout(rgba16f, binding = 0) uniform image3D u_albedo_texture;
layout(rgba16f, binding = 1) uniform image3D u_normal_texture;

#include "cbuffer.glsl.h"

in vec3 pass_position;
in vec3 pass_normal;
in vec2 pass_uv;

#include "common.glsl"
#include "common/lighting.glsl"
#include "common/shadow.glsl"

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

    const int cascade_level = find_cascade(world_position);

    const vec3 N = normalize(pass_normal);
    const vec3 V = normalize(c_camera_position - world_position);
    const float NdotV = max(dot(N, V), 0.0);
    vec3 Lo = vec3(0.0);
    vec3 F0 = mix(vec3(0.04), albedo.rgb, metallic);
    for (int idx = 0; idx < c_light_count; ++idx) {
        int light_type = c_lights[idx].type;
        vec3 L = vec3(0.0);
        float atten = 1.0;
        if (light_type == 0) {
            L = c_lights[idx].position;
        } else if (light_type == 1) {
            vec3 delta = world_position - c_lights[idx].position;
            L = -normalize(delta);
            float dist = length(delta);
            atten = (c_lights[idx].atten_constant + c_lights[idx].atten_linear * dist +
                     c_lights[idx].atten_quadratic * (dist * dist));
            atten = 1.0 / atten;
        }

        const vec3 H = normalize(V + L);
        const vec3 radiance = c_lights[idx].color;
        vec3 direct_lighting = atten * lighting(N, L, V, radiance, F0, roughness, metallic, albedo);

        // @TODO: shadow
        if (c_lights[idx].cast_shadow == 1) {
            const float NdotL = max(dot(N, L), 0.0);
            float shadow = cascade_shadow(c_shadow_map, world_position, NdotL, SC_NUM_CASCADES - 1);
            direct_lighting = (1.0 - shadow) * direct_lighting;
        }
        Lo += direct_lighting;
    }

    vec3 color = Lo;

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
