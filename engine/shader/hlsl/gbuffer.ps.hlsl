/// File: gbuffer.ps.hlsl
#include "cbuffer.hlsl.h"
#include "hlsl/input_output.hlsl"
#include "sampler.hlsl.h"
#include "shader_resource_defines.hlsl.h"

struct ps_output {
    float4 base_color : SV_TARGET0;
    float4 normal : SV_TARGET1;
    float4 out_emissive_roughness_metallic : SV_TARGET2;
};

ps_output main(VS_OUTPUT_MESH input) {
    float4 color = c_baseColor;

    if (c_hasBaseColorMap) {
        color = TEXTURE_2D(BaseColorMap).Sample(s_linearMipWrapSampler, input.uv);
    }

    if (color.a <= 0.0) {
        discard;
    }

    float metallic;
    float roughness;

    if (c_hasMaterialMap != 0) {
        float3 value = TEXTURE_2D(MaterialMap).Sample(s_linearMipWrapSampler, input.uv).rgb;
        metallic = value.b;
        roughness = value.g;
    } else {
        metallic = c_metallic;
        roughness = c_roughness;
    }

    float3 N;
    if (c_hasNormalMap != 0) {
        float3x3 TBN;
        TBN[0] = normalize(input.T);
        TBN[1] = normalize(input.B);
        TBN[2] = normalize(input.normal);
        float3 value = TEXTURE_2D(NormalMap).Sample(s_linearMipWrapSampler, input.uv).rgb;
        N = normalize(mul((2.0f * value) - 1.0f, TBN));
    } else {
        N = normalize(input.normal);
    }

    ps_output output;
    output.base_color = color;
#if 0
    output.base_color = float4(0.5f * N + 0.5f, 1.0f);
#endif

    output.normal.xyz = 0.5f * N + 0.5f;
    output.normal.a = 1.0f;

    // [SCRUM-29] Generate this code with cross compiler
    output.out_emissive_roughness_metallic.r = c_emissivePower;
    output.out_emissive_roughness_metallic.g = roughness;
    output.out_emissive_roughness_metallic.b = metallic;
    output.out_emissive_roughness_metallic.a = 1.0f;
    return output;
}
