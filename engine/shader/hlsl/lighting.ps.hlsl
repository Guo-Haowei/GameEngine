/// File: lighting.ps.hlsl
#include "cbuffer.hlsl.h"
#include "common.hlsl.h"
#include "hlsl/input_output.hlsl"
#include "hlsl/lighting.hlsl"
#include "pbr.hlsl.h"
#include "sampler.hlsl.h"
#include "shader_resource_defines.hlsl.h"

Texture2D t_GbufferBaseColorMap : register(t0);
Texture2D t_GbufferNormalMap : register(t1);
Texture2D t_GbufferMaterialMap : register(t2);
Texture2D t_GbufferDepth : register(t3);
Texture2D t_SsaoMap : register(t4);
Texture2D t_ShadowMap : register(t5);
// Texture2D t_PointShadowArray : register(t6);

float4 main(vsoutput_uv input) : SV_TARGET {
    const float2 uv = input.uv;
    const float4 emissive_roughness_metallic = t_GbufferMaterialMap.Sample(s_linearMipWrapSampler, uv);
    clip(emissive_roughness_metallic.a - 0.01f);

    float3 base_color = t_GbufferBaseColorMap.Sample(s_linearMipWrapSampler, uv).rgb;

    const float depth = t_GbufferDepth.Sample(s_pointClampSampler, uv).r;
    const Vector3f view_position = NdcToViewPos(float2(uv.x, 1.0f - uv.y), depth);
    const float4 world_position = mul(c_invView, float4(view_position, 1.0f));

    float emissive = emissive_roughness_metallic.r;
    float roughness = emissive_roughness_metallic.g;
    float metallic = emissive_roughness_metallic.b;

    if (emissive > 0.0) {
        return float4(emissive * base_color, 1.0);
    }

    float3 N = t_GbufferNormalMap.Sample(s_linearMipWrapSampler, uv).rgb;
    N = 2.0f * N - 1.0f;

    float3 color = compute_lighting(t_ShadowMap,
                                    base_color,
                                    world_position.xyz,
                                    N,
                                    metallic,
                                    roughness,
                                    emissive);
    if (c_ssaoEnabled != 0) {
        float ao = t_SsaoMap.Sample(s_pointClampSampler, uv).r;
        color *= ao;
    }

    return float4(color, 1.0f);
}
