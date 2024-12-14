/// File: lighting.ps.hlsl
#include "cbuffer.hlsl.h"
#include "hlsl/input_output.hlsl"
#include "pbr.hlsl.h"
#include "sampler.hlsl.h"
#include "shader_resource_defines.hlsl.h"
#include "shadow.hlsl"

float3 FresnelSchlickRoughness(float cosTheta, in float3 F0, float roughness) {
    float3 zero = float3(0.0, 0.0, 0.0);
    float3 tmp = float3(1.0, 1.0, 1.0) - roughness;
    return F0 + (max(tmp, zero)) * pow(1.0 - cosTheta, 5.0);
}

// @TODO: refactor shadow
#define NUM_POINT_SHADOW_SAMPLES 20

static float3 POINT_LIGHT_SHADOW_SAMPLE_OFFSET[NUM_POINT_SHADOW_SAMPLES] = {
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
    float disk_radius = (1.0 + (view_distance / light_far)) / 300.0;

    float shadow = 0.0;

#if 1
    float4 uv = float4(frag_to_light, (float)p_light.shadow_map_index);
    float closest_depth = TEXTURE_CUBE_ARRAY(PointShadowArray).SampleLevel(s_shadowSampler, uv, 0).r;
    closest_depth *= light_far;
    if (current_depth - bias > closest_depth) {
        shadow += 1.0;
    }
#else
    for (int i = 0; i < NUM_POINT_SHADOW_SAMPLES; ++i) {
        float4 uv = float4(frag_to_light + POINT_LIGHT_SHADOW_SAMPLE_OFFSET[i] * disk_radius, (float)p_light.shadow_map_index);
        float closest_depth = TEXTURE_CUBE_ARRAY(pointShadowArray).SampleLevel(s_shadowSampler, uv, 0).r;
        closest_depth *= light_far;
        if (current_depth - bias > closest_depth) {
            shadow += 1.0;
        }
    }
    shadow /= float(NUM_POINT_SHADOW_SAMPLES);
#endif

    return min(shadow, 0.8f);
}

// @TODO: refactor
float3 lighting(float3 N, float3 L, float3 V, float3 radiance, float3 F0, float roughness, float metallic, float3 p_base_color) {
    float3 Lo = float3(0.0, 0.0, 0.0);
    const float3 H = normalize(V + L);
    const float NdotL = max(dot(N, L), 0.0);
    const float NdotH = max(dot(N, H), 0.0);
    const float NdotV = max(dot(N, V), 0.0);

    // direct cook-torrance brdf
    const float NDF = DistributionGGX(NdotH, roughness);
    const float G = GeometrySmith(NdotV, NdotL, roughness);
    const float3 F = FresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

    const float3 nom = NDF * G * F;
    float denom = 4 * NdotV * NdotL;

    float3 specular = nom / max(denom, 0.001);

    const float3 kS = F;
    const float3 kD = (1.0 - metallic) * (float3(1.0, 1.0, 1.0) - kS);

    float3 direct_lighting = (kD * p_base_color / MY_PI + specular) * radiance * NdotL;

    return direct_lighting;
}

float4 main(vsoutput_uv input) : SV_TARGET {
    const float2 texcoord = input.uv;
    const float4 emissive_roughness_metallic = TEXTURE_2D(GbufferMaterialMap).Sample(s_linearMipWrapSampler, texcoord);
    clip(emissive_roughness_metallic.a - 0.01f);

    float3 base_color = TEXTURE_2D(GbufferBaseColorMap).Sample(s_linearMipWrapSampler, texcoord).rgb;
    if (c_noTexture != 0) {
        base_color = float3(0.6, 0.6, 0.6);
    }

    const float3 world_position = TEXTURE_2D(GbufferPositionMap).Sample(s_linearMipWrapSampler, texcoord).rgb;

    float emissive = emissive_roughness_metallic.r;
    float roughness = emissive_roughness_metallic.g;
    float metallic = emissive_roughness_metallic.b;

    if (emissive > 0.0) {
        return float4(emissive * base_color, 1.0);
    }

    float3 N = TEXTURE_2D(GbufferNormalMap).Sample(s_linearMipWrapSampler, texcoord).rgb;

    const float3 V = normalize(c_cameraPosition - world_position);
    const float NdotV = clamp(dot(N, V), 0.0, 1.0);
    float3 R = reflect(-V, N);

    float3 Lo = float3(0.0, 0.0, 0.0);
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), base_color, metallic);

    for (int light_idx = 0; light_idx < MAX_LIGHT_COUNT; ++light_idx) {
        if (light_idx >= c_lightCount) {
            break;
        }

        Light light = c_lights[light_idx];
        int light_type = c_lights[light_idx].type;
        float3 direct_lighting = float3(0.0, 0.0, 0.0);
        float shadow = 0.0;
        const float3 radiance = light.color;
        switch (light.type) {
            case LIGHT_TYPE_INFINITE: {
                float3 L = light.position;
                float atten = 1.0;
                const float3 H = normalize(V + L);
                direct_lighting = atten * lighting(N, L, V, radiance, F0, roughness, metallic, base_color);
                if (light.cast_shadow == 1) {
                    const float NdotL = max(dot(N, L), 0.0);
                    shadow = shadowTest(light, world_position, NdotL);
                    direct_lighting *= (1.0 - shadow);
                }
            } break;
            case LIGHT_TYPE_POINT: {
                float3 delta = -world_position + light.position;
                float dist = length(delta);
                float atten = (light.atten_constant + light.atten_linear * dist +
                               light.atten_quadratic * (dist * dist));
                atten = 1.0 / atten;

                if (atten > 0.01) {
                    float3 L = normalize(delta);
                    const float3 H = normalize(V + L);
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

    // IBL
    float3 F = FresnelSchlickRoughness(NdotV, F0, roughness);
    float3 kS = F;
    float3 kD = 1.0 - kS;
    // kD *= 1.0 - metallic;
    float3 irradiance = TEXTURE_CUBE(DiffuseIrradiance).SampleLevel(s_cubemapClampSampler, N, 0.0f).rgb;
    float3 diffuse = irradiance * base_color.rgb;

    const float MAX_REFLECTION_LOD = 4.0f;
    float3 prefilteredColor = TEXTURE_CUBE(Prefiltered).SampleLevel(s_cubemapClampSampler, R, roughness * MAX_REFLECTION_LOD).rgb;
    float2 lut_uv = float2(NdotV, roughness);
    lut_uv.y = 1.0 - lut_uv.y;
    float2 envBRDF = TEXTURE_2D(BrdfLut).SampleLevel(s_linearClampSampler, lut_uv, 0.0f).rg;
    float3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);

    const float ao = 1.0;
    float3 ambient = (kD * diffuse + specular) * ao;

    return float4(Lo + ambient, 1.0);
}
