/// File: gbuffer.pixel.hlsl
#include "cbuffer.hlsl.h"
#include "hlsl/input_output.hlsl"
#include "sampler.hlsl.h"
#include "texture_binding.hlsl.h"

struct ps_output {
    float3 base_color : SV_TARGET0;
    float4 position : SV_TARGET1;
    float4 normal : SV_TARGET2;
    float3 out_emissive_roughness_metallic : SV_TARGET3;
};

ps_output main(vsoutput_mesh input) {
    float4 color = c_baseColor;

    if (c_hasBaseColorMap) {
#ifdef HLSL_LANG_D3D12
        color = t_texture2DArray[c_baseColorMapIndex].Sample(s_linearMipWrapSampler, input.uv);
#else
        color = t_baseColorMap.Sample(s_linearMipWrapSampler, input.uv);
#endif
    }

    if (color.a <= 0.0) {
        discard;
    }

    float metallic;
    float roughness;

    if (c_hasMaterialMap != 0) {
#ifdef HLSL_LANG_D3D12
        float3 value = t_texture2DArray[c_materialMapIndex].Sample(s_linearMipWrapSampler, input.uv).rgb;
#else
        float3 value = t_materialMap.Sample(s_linearMipWrapSampler, input.uv).rgb;
#endif
        metallic = value.b;
        roughness = value.g;
    } else {
        metallic = c_metallic;
        roughness = c_roughness;
    }

    float3 N;
    if (c_hasNormalMap != 0) {
        float3x3 TBN;
        TBN[0] = input.T;
        TBN[1] = input.B;
        TBN[2] = input.normal;
#ifdef HLSL_LANG_D3D12
        float3 value = t_texture2DArray[c_normalMapIndex].Sample(s_linearMipWrapSampler, input.uv).rgb;
#else
        float3 value = t_normalMap.Sample(s_linearMipWrapSampler, input.uv).rgb;
#endif
        N = normalize(mul((2.0f * value) - 1.0f, TBN));
    } else {
        N = normalize(input.normal);
    }

    ps_output output;
    output.base_color = color.rgb;
    output.position.xyz = input.world_position;
    output.normal.xyz = N;

    // [SCRUM-29] Generate this code with cross compiler
    output.out_emissive_roughness_metallic.r = c_emissivePower;
    output.out_emissive_roughness_metallic.g = roughness;
    output.out_emissive_roughness_metallic.b = metallic;
    return output;
}
