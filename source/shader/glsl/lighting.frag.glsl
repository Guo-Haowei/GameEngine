#include "../hlsl/cbuffer.h"

#define ENABLE_VXGI 1

layout(location = 0) out vec4 out_color;
layout(location = 0) in vec2 pass_uv;

#include "lighting.glsl"

#if ENABLE_VXGI
#include "vxgi.glsl"
#endif

vec3 FresnelSchlickRoughness(float cosTheta, in vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness) - F0, vec3(0.0))) * pow(1.0 - cosTheta, 5.0);
}

void main() {
    const vec2 uv = pass_uv;
    float depth = texture(g_gbuffer_depth_map, uv).r;

    if (depth > 0.999) discard;

    gl_FragDepth = depth;

    vec3 N = texture(c_gbuffer_normal_roughness_map, uv).rgb;
    N = (2.0 * N) - vec3(1.0);

    const vec4 position_metallic = texture(c_gbuffer_position_metallic_map, uv);
    const vec3 world_position = position_metallic.xyz;

    const vec3 emissive_roughness_metallic = texture(g_gbuffer_material_map, uv).rgb;
    const float emissive = emissive_roughness_metallic.r;
    const float roughness = emissive_roughness_metallic.g;
    const float metallic = emissive_roughness_metallic.b;

    vec3 base_color = texture(g_gbuffer_base_color_map, uv).rgb;

    if (c_no_texture != 0) {
        base_color = vec3(0.6);
    }

    const int cascade_level = find_cascade(world_position);

    const vec3 V = normalize(c_camera_position - world_position);
    const float NdotV = max(dot(N, V), 0.0);
    vec3 R = reflect(-V, N);

    vec3 Lo = vec3(0.0);
    vec3 F0 = mix(vec3(0.04), base_color, metallic);

    for (int light_idx = 0; light_idx < c_light_count; ++light_idx) {
        Light light = c_lights[light_idx];
        int light_type = c_lights[light_idx].type;
        vec3 direct_lighting = vec3(0.0);
        float shadow = 0.0;
        switch (light.type) {
            case LIGHT_TYPE_OMNI: {
                vec3 L = light.position;
                float atten = 1.0;

                const vec3 H = normalize(V + L);
                const vec3 radiance = light.color;
                direct_lighting = atten * lighting(N, L, V, radiance, F0, roughness, metallic, base_color);
                if (light.cast_shadow == 1) {
                    const float NdotL = max(dot(N, L), 0.0);
                    shadow = cascade_shadow(c_shadow_map, world_position, NdotL, cascade_level);
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
                    const vec3 radiance = c_lights[light_idx].color;
                    direct_lighting = atten * lighting(N, L, V, radiance, F0, roughness, metallic, base_color);
                    if (light.cast_shadow == 1) {
                        shadow = point_shadow_calculation(world_position, light_idx, c_camera_position);
                    }
                }
            } break;
            default:
                break;
        }
        Lo += (1.0 - shadow) * direct_lighting;
    }

    // ambient

    vec3 F = FresnelSchlickRoughness(NdotV, F0, roughness);
    vec3 kS = F;
    vec3 kD = 1.0 - kS;
    kD *= 1.0 - metallic;
    vec3 irradiance = texture(c_diffuse_irradiance_map, N).rgb;
    vec3 diffuse = irradiance * base_color.rgb;

    const float MAX_REFLECTION_LOD = 4.0;
    vec3 prefilteredColor = textureLod(c_prefiltered_map, R, roughness * MAX_REFLECTION_LOD).rgb;
    vec2 envBRDF = texture(c_brdf_map, vec2(NdotV, roughness)).rg;
    vec3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);

    const float ao = c_enable_ssao == 0 ? 1.0 : texture(c_ssao_map, uv).r;
    vec3 ambient = (kD * diffuse + specular) * ao;

#if ENABLE_VXGI
    if (c_enable_vxgi == 1) {
        const vec3 F = FresnelSchlickRoughness(NdotV, F0, roughness);
        const vec3 kS = F;
        const vec3 kD = (1.0 - kS) * (1.0 - metallic);

        // indirect diffuse
        vec3 diffuse = base_color.rgb * cone_diffuse(world_position, N);

        // specular cone
        vec3 coneDirection = reflect(-V, N);
        vec3 specular = vec3(0);
        specular = metallic * cone_specular(world_position, coneDirection, roughness);
        Lo += (kD * diffuse + specular) * ao;
    }
#endif

    vec3 color = Lo + ambient;

    const float gamma = 1.0 / 2.2;

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

#if ENABLE_CSM
    if (c_debug_csm != 0) {
        vec3 mask = vec3(0.1);
        for (int light_idx = 0; light_idx < MAX_CASCADE_COUNT; ++light_idx) {
            if (clipSpaceZ <= c_cascade_clip_z[light_idx + 1]) {
                mask[light_idx] = 0.7;
                break;
            }
        }
        color = mix(color.rgb, mask, 0.1);
    }
#endif

    out_color = vec4(color, 1.0);
}
