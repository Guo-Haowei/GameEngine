/// File: gbuffer.pixel.hlsl
#include "cbuffer.h"
#include "hlsl/input_output.hlsl"
#include "sampler.hlsl.h"
#include "texture_binding.h"

struct ps_output {
    float4 base_color : SV_TARGET0;
    float3 position : SV_TARGET1;
    float3 normal : SV_TARGET2;
    float3 out_emissive_roughness_metallic : SV_TARGET3;
};

ps_output main(vsoutput_mesh input) {
    float3 N = normalize(input.normal);
    float4 color = c_baseColor;

    if (c_hasBaseColorMap) {
        color = t_baseColorMap.Sample(s_linearMipWrapSampler, input.uv);
    }

    if (color.a <= 0.0) {
        discard;
    }

    float metallic;
    float roughness;

    if (c_hasPbrMap != 0) {
        float3 value = t_materialMap.Sample(s_linearMipWrapSampler, input.uv);
        metallic = value.b;
        roughness = value.g;
    } else {
        metallic = c_metallic;
        roughness = c_roughness;
    }

    ps_output output;
    output.base_color = float4(color.rgb, 1.0);
    output.position = input.world_position;
    output.normal = N;

    // [SCRUM-29] Generate this code with cross compiler
    output.out_emissive_roughness_metallic.r = c_emissivePower;
    output.out_emissive_roughness_metallic.g = roughness;
    output.out_emissive_roughness_metallic.b = metallic;
    return output;
}
