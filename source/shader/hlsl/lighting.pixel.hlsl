#include "cbuffer.h"
#include "hlsl/input_output.hlsl"
#include "pbr.hlsl"
#include "shadow.hlsl"

// @TODO: fix sampler
SamplerState u_sampler : register(s0);

// @TODO: refactor shadow
#define NUM_POINT_SHADOW_SAMPLES 20

float3 POINT_LIGHT_SHADOW_SAMPLE_OFFSET[NUM_POINT_SHADOW_SAMPLES] = {
    float3(1, 1, 1),
    float3(1, -1, 1),
    float3(-1, -1, 1),
    float3(-1, 1, 1),
    float3(1, 1, -1),
    float3(1, -1, -1),
    float3(-1, -1, -1),
    float3(-1, 1, -1),
    float3(1, 1, 0),
    float3(1, -1, 0),
    float3(-1, -1, 0),
    float3(-1, 1, 0),
    float3(1, 0, 1),
    float3(-1, 0, 1),
    float3(1, 0, -1),
    float3(-1, 0, -1),
    float3(0, 1, 1),
    float3(0, -1, 1),
    float3(0, -1, -1),
    float3(0, 1, -1),
};

float point_shadow_calculation(Light p_light, float3 p_frag_pos, float3 p_eye) {
    float3 light_position = p_light.position;
    float light_far = p_light.max_distance;

    float3 frag_to_light = p_frag_pos - light_position;
    float current_depth = length(frag_to_light);

    float bias = 0.01;

    float view_distance = length(p_eye - p_frag_pos);
    float disk_radius = (1.0 + (view_distance / light_far)) / 100.0;
    float shadow = 0.0;

    float closest_depth = 0.0f;
    switch (p_light.shadow_map_index) {
        case 0:
            closest_depth = t_point_shadow_0.Sample(g_shadow_sampler, frag_to_light).r;
            break;
        case 1:
            closest_depth = t_point_shadow_1.Sample(g_shadow_sampler, frag_to_light).r;
            break;
        case 2:
            closest_depth = t_point_shadow_2.Sample(g_shadow_sampler, frag_to_light).r;
            break;
        case 3:
            closest_depth = t_point_shadow_3.Sample(g_shadow_sampler, frag_to_light).r;
            break;
        default:
            break;
    }

    closest_depth *= light_far;
    if (current_depth - bias > closest_depth) {
        shadow += 1.0;
    }

#if 0
    for (int i = 0; i < NUM_POINT_SHADOW_SAMPLES; ++i) {
        float closest_depth = t_point_shadow_0.Sample(g_shadow_sampler, frag_to_light + POINT_LIGHT_SHADOW_SAMPLE_OFFSET[i] * disk_radius).r;
        closest_depth *= light_far;
        if (current_depth - bias > closest_depth) {
            shadow += 1.0;
        }
    }
    shadow /= float(NUM_POINT_SHADOW_SAMPLES);
#endif

    return shadow;
}

// @TODO: refactor
vec3 lighting(vec3 N, vec3 L, vec3 V, vec3 radiance, vec3 F0, float roughness, float metallic, vec3 p_base_color) {
    vec3 Lo = vec3(0.0, 0.0, 0.0);
    const vec3 H = normalize(V + L);
    const float NdotL = max(dot(N, L), 0.0);
    const float NdotH = max(dot(N, H), 0.0);
    const float NdotV = max(dot(N, V), 0.0);

    // direct cook-torrance brdf
    const float NDF = distribution_ggx(NdotH, roughness);
    const float G = geometry_smith(NdotV, NdotL, roughness);
    const vec3 F = fresnel_schlick(clamp(dot(H, V), 0.0, 1.0), F0);

    const vec3 nom = NDF * G * F;
    float denom = 4 * NdotV * NdotL;

    vec3 specular = nom / max(denom, 0.001);

    const vec3 kS = F;
    const vec3 kD = (1.0 - metallic) * (vec3(1.0, 1.0, 1.0) - kS);

    vec3 direct_lighting = (kD * p_base_color / MY_PI + specular) * radiance * NdotL;

    return direct_lighting;
}

struct ps_output {
    float4 color : SV_TARGET;
    float depth : SV_DEPTH;
};

ps_output main(vsoutput_uv input) {
    ps_output output;

    float2 texcoord = input.uv;

    output.depth = t_gbufferDepth.Sample(u_sampler, texcoord).r;
    clip(0.9999 - output.depth);

    float3 base_color = u_gbuffer_base_color_map.Sample(u_sampler, texcoord).rgb;
    if (c_noTexture != 0) {
        base_color = float3(0.6, 0.6, 0.6);
    }

    const vec3 world_position = u_gbuffer_position_map.Sample(u_sampler, texcoord).rgb;
    const vec3 emissive_roughness_metallic = u_gbuffer_material_map.Sample(u_sampler, texcoord).rgb;
    float emissive = emissive_roughness_metallic.r;
    float roughness = emissive_roughness_metallic.g;
    float metallic = emissive_roughness_metallic.b;

    if (emissive > 0.0) {
        output.color = float4(emissive * base_color, 1.0);
        return output;
    }

    float3 N = u_gbuffer_normal_map.Sample(u_sampler, texcoord).rgb;
    float3 color = 0.5 * (N + float3(1.0, 1.0, 1.0));

    const vec3 V = normalize(c_cameraPosition - world_position);
    const float NdotV = clamp(dot(N, V), 0.0, 1.0);
    vec3 R = reflect(-V, N);

    vec3 Lo = float3(0.0, 0.0, 0.0);
    vec3 F0 = lerp(float3(0.04, 0.04, 0.04), base_color, metallic);

    for (int light_idx = 0; light_idx < MAX_LIGHT_COUNT; ++light_idx) {
        if (light_idx >= c_lightCount) {
            break;
        }

        Light light = c_lights[light_idx];
        int light_type = c_lights[light_idx].type;
        vec3 direct_lighting = vec3(0.0, 0.0, 0.0);
        float shadow = 0.0;
        const vec3 radiance = light.color;
        switch (light.type) {
            case LIGHT_TYPE_INFINITE: {
                vec3 L = light.position;
                float atten = 1.0;
                const vec3 H = normalize(V + L);
                direct_lighting = atten * lighting(N, L, V, radiance, F0, roughness, metallic, base_color);
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
                    direct_lighting = atten * lighting(N, L, V, radiance, F0, roughness, metallic, base_color);
                    if (light.cast_shadow == 1) {
                        shadow = point_shadow_calculation(light, world_position, c_cameraPosition);
                    }
                }
            } break;
            default:
                break;
        }
        Lo += (1.0 - shadow) * direct_lighting;
    }

    color = Lo;
    output.color = float4(color, 1.0);
    return output;
}
