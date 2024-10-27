layout(rgba16f, binding = 0) uniform image3D u_albedo_texture;
layout(rgba16f, binding = 1) uniform image3D u_normal_texture;

#include "../cbuffer.h"

in vec3 pass_position;
in vec3 pass_normal;
in vec2 pass_uv;

#include "lighting.glsl"

void main() {
    vec4 albedo = u_base_color;
    if (u_has_base_color_map != 0) {
        albedo = texture(u_base_color_map, pass_uv);
    }
    if (albedo.a < 0.001) {
        discard;
    }

    float metallic = u_metallic;
    float roughness = u_roughness;
    if (u_has_pbr_map != 0) {
        // g roughness, b metallic
        vec3 mr = texture(u_material_map, pass_uv).rgb;
        metallic = mr.b;
        roughness = mr.g;
    }

    vec3 world_position = pass_position;

    const vec3 N = normalize(pass_normal);
    const vec3 V = normalize(u_camera_position - world_position);
    const float NdotV = max(dot(N, V), 0.0);
    vec3 Lo = vec3(0.0);
    vec3 F0 = mix(vec3(0.04), albedo.rgb, metallic);
    for (int light_idx = 0; light_idx < u_light_count; ++light_idx) {
        Light light = u_lights[light_idx];
        int light_type = u_lights[light_idx].type;
        vec3 direct_lighting = vec3(0.0);
        float shadow = 0.0;
        switch (light.type) {
            case LIGHT_TYPE_INFINITE: {
                vec3 L = light.position;
                float atten = 1.0;

                const vec3 H = normalize(V + L);
                const vec3 radiance = light.color;
                direct_lighting = atten * lighting(N, L, V, radiance, F0, roughness, metallic, albedo.rgb);
                if (light.cast_shadow == 1) {
                    const float NdotL = max(dot(N, L), 0.0);
                    shadow = shadowTest(light, t_shadow_map, world_position, NdotL);
                    direct_lighting *= (1.0 - shadow);
                }
            } break;
            case LIGHT_TYPE_POINT: {
                vec3 delta = -world_position + light.position;
                float dist = length(delta);
                float atten = (light.atten_constant + light.atten_linear * dist +
                               light.atten_quadratic * (dist * dist));
                atten = 1.0 / atten;
                if (atten > 0.01) {
                    vec3 L = normalize(delta);
                    const vec3 H = normalize(V + L);
                    const vec3 radiance = u_lights[light_idx].color;
                    direct_lighting = atten * lighting(N, L, V, radiance, F0, roughness, metallic, albedo.rgb);
                    if (light.cast_shadow == 1) {
                        shadow = point_shadow_calculation(light, world_position, u_camera_position);
                    }
                }
            } break;
            default:
                break;
        }
        Lo += (1.0 - shadow) * direct_lighting;
    }
    // dummy ambient
    // Lo += 0.0 * albedo.rgb;

    vec3 color = Lo;

    ///////////////////////////////////////////////////////////////////////////

    // write lighting information to texel
    vec3 voxel = (pass_position - u_world_center) / u_world_size_half;  // normalize it to [-1, 1]
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
