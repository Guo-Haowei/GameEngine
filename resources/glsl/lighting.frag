#include "cbuffer.glsl.h"

#define ENABLE_VXGI 1

layout(location = 0) out vec4 out_color;
layout(location = 0) in vec2 pass_uv;

#include "common.glsl"
#include "common/lighting.glsl"

#if ENABLE_VXGI
#include "vxgi/vxgi.glsl"
#endif

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

void main() {
    const vec2 uv = pass_uv;
    float depth = texture(c_gbuffer_depth_map, uv).r;

    if (depth > 0.999) discard;

    gl_FragDepth = depth;

    const vec4 normal_roughness = texture(c_gbuffer_normal_roughness_map, uv);
    const vec4 position_metallic = texture(c_gbuffer_position_metallic_map, uv);
    const vec3 world_position = position_metallic.xyz;
    float roughness = normal_roughness.w;
    float metallic = position_metallic.w;

    vec4 albedo = texture(c_gbuffer_albedo_map, uv);

    if (c_no_texture != 0) {
        albedo.rgb = vec3(0.6);
    }

    const int cascade_level = find_cascade(world_position);

    const vec3 N = normal_roughness.xyz;
    const vec3 V = normalize(c_camera_position - world_position);
    const float NdotV = max(dot(N, V), 0.0);
    vec3 Lo = vec3(0.0);
    vec3 F0 = mix(vec3(0.04), albedo.rgb, metallic);
    for (int idx = 0; idx < c_light_count; ++idx) {
        int light_type = c_lights[idx].type;
        vec3 L = vec3(0.0);
        float atten = 1.0;
        if (light_type == LIGHT_TYPE_OMNI) {
            L = c_lights[idx].position;
        } else if (light_type == LIGHT_TYPE_POINT) {
            vec3 delta = -world_position + c_lights[idx].position;
            L = normalize(delta);
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
            float shadow = cascade_shadow(c_shadow_maps[cascade_level], world_position, NdotL, cascade_level);
            direct_lighting = (1.0 - shadow) * direct_lighting;
        }
        Lo += direct_lighting;
    }
    Lo += 0.2 * albedo.rgb;

    const float ao = c_enable_ssao == 0 ? 1.0 : texture(c_ssao_map, uv).r;

#if ENABLE_VXGI
    if (c_enable_vxgi == 1) {
        const vec3 F = fresnelSchlickRoughness(NdotV, F0, roughness);
        const vec3 kS = F;
        const vec3 kD = (1.0 - kS) * (1.0 - metallic);

        // indirect diffuse
        vec3 diffuse = albedo.rgb * cone_diffuse(world_position, N);

        // specular cone
        vec3 coneDirection = reflect(-V, N);
        vec3 specular = metallic * cone_specular(world_position, coneDirection, roughness);
        Lo += (kD * diffuse + specular) * ao;
    }
#endif

    vec3 color = Lo;

    const float gamma = 2.2;

    color = color / (color + 1.0);
    color = pow(color, vec3(gamma));

    // debug CSM
    if (c_debug_csm == 1) {

        float alpha = 0.2;
        switch (cascade_level) {
            case 0:
                color = mix(color, vec3(1, 0, 0), alpha);
                break;
            case 1:
                color = mix(color, vec3(0, 1, 0), alpha);
                break;
            case 2:
                color = mix(color, vec3(0, 0, 1), alpha);
                break;
            default:
                color = mix(color, vec3(1, 1, 0), alpha);
                break;
        }
    }

    out_color = vec4(color, 1.0);

#if ENABLE_CSM
    if (c_debug_csm != 0) {
        vec3 mask = vec3(0.1);
        for (int idx = 0; idx < NUM_CASCADE_MAX; ++idx) {
            if (clipSpaceZ <= c_cascade_clip_z[idx + 1]) {
                mask[idx] = 0.7;
                break;
            }
        }
        out_color.rgb = mix(out_color.rgb, mask, 0.1);
    }
#endif
}
