#include "cbuffer.h"
#include "hlsl/input_output.hlsl"
#include "pbr.hlsl"
#include "texture_binding.h"

// @TODO: fix sampler
SamplerState u_sampler : register(s0);
SamplerState g_shadow_sampler : register(s1);

int find_cascade(const in vec3 p_pos_world) {
    return 0;
}

// @TODO: move this to shadow file
float cascade_shadow(Texture2D p_shadow_map,
                     const in vec3 p_pos_world,
                     float p_NdotL,
                     int p_level) {
    vec4 pos_light = mul(u_main_light_matrices[p_level], vec4(p_pos_world, 1.0));
    vec3 coords = pos_light.xyz / pos_light.w;
    coords = 0.5 * coords + 0.5;  // [0, 1]
    float current_depth = coords.z;
    coords.x += p_level;
    coords.x /= MAX_CASCADE_COUNT;

    if (current_depth > 1.0) {
        return 0.0;
    }

    float shadow = 0.0;
    int shadow_width, shadow_height;
    p_shadow_map.GetDimensions(shadow_width, shadow_height);

    vec2 texel_size = 1.0 / vec2(shadow_width, shadow_height);

    // @TODO: better bias
    float bias = max(0.005 * (1.0 - p_NdotL), 0.0005);

    const int SAMPLE_STEP = 1;
    for (int x = -SAMPLE_STEP; x <= SAMPLE_STEP; ++x) {
        for (int y = -SAMPLE_STEP; y <= SAMPLE_STEP; ++y) {
            vec2 offset = vec2(x, y) * texel_size;
            float closest_depth = p_shadow_map.Sample(g_shadow_sampler, coords.xy + offset).r;
            shadow += current_depth - bias > closest_depth ? 1.0 : 0.0;
        }
    }

    const float samples = float(2 * SAMPLE_STEP + 1);
    shadow /= samples * samples;
    return shadow;
}
///////////////

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

float4 main(vsoutput_uv input) : SV_TARGET {
    float2 texcoord = input.uv;

    float3 base_color = u_gbuffer_base_color_map.Sample(u_sampler, texcoord).rgb;
    if (u_no_texture != 0) {
        base_color = float3(0.6, 0.6, 0.6);
    }

    const vec3 world_position = u_gbuffer_position_map.Sample(u_sampler, texcoord).rgb;
    const vec3 emissive_roughness_metallic = u_gbuffer_material_map.Sample(u_sampler, texcoord).rgb;
    float emissive = emissive_roughness_metallic.r;
    float roughness = emissive_roughness_metallic.g;
    float metallic = emissive_roughness_metallic.b;

    if (emissive > 0.0) {
        return float4(emissive * base_color, 1.0);
    }

    // @TODO: fix this
    const int cascade_level = find_cascade(world_position);

    float3 N = u_gbuffer_normal_map.Sample(u_sampler, texcoord).rgb;
    float3 color = 0.5 * (N + float3(1.0, 1.0, 1.0));

    const vec3 V = normalize(u_camera_position - world_position);
    const float NdotV = clamp(dot(N, V), 0.0, 1.0);
    vec3 R = reflect(-V, N);

    vec3 Lo = float3(0.0, 0.0, 0.0);
    vec3 F0 = lerp(float3(0.04, 0.04, 0.04), base_color, metallic);

    for (int light_idx = 0; light_idx < u_light_count; ++light_idx) {
        Light light = u_lights[light_idx];
        int light_type = u_lights[light_idx].type;
        vec3 direct_lighting = vec3(0.0, 0.0, 0.0);
        float shadow = 0.0;
        const vec3 radiance = light.color;
        switch (light.type) {
            case LIGHT_TYPE_OMNI: {
                vec3 L = light.position;
                float atten = 1.0;
                const vec3 H = normalize(V + L);
                direct_lighting = atten * lighting(N, L, V, radiance, F0, roughness, metallic, base_color);
                if (light.cast_shadow == 1) {
                    const float NdotL = max(dot(N, L), 0.0);
                    shadow = cascade_shadow(u_shadow_map, world_position, NdotL, cascade_level);
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
                }
            } break;
            default:
                break;
        }
        Lo += (1.0 - shadow) * direct_lighting;
    }

    color = Lo;
    return float4(color, 1.0);
}
