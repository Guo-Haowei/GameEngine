#include "cbuffer.h"
#include "hlsl/input_output.hlsl"
#include "texture_binding.h"

SamplerState u_sampler : register(s0);

struct ps_output {
    float4 base_color : SV_TARGET0;
    float3 position : SV_TARGET1;
    float3 normal : SV_TARGET2;
    float3 out_emissive_roughness_metallic : SV_TARGET3;
};

ps_output main(vsoutput_mesh input) {
    float3 N = normalize(input.normal);
    float4 color = u_base_color;

    if (u_has_base_color_map) {
        color = u_base_color_map.Sample(u_sampler, input.uv);
    }

    if (color.a <= 0.0) {
        discard;
    }

    float metallic;
    float roughness;

    if (u_has_pbr_map != 0) {
        float3 value = u_material_map.Sample(u_sampler, input.uv);
        metallic = value.b;
        roughness = value.g;
    } else {
        metallic = u_metallic;
        roughness = u_roughness;
    }

    ps_output output;
    output.base_color = float4(color.rgb, 1.0);
    output.position = input.world_position;
    output.normal = N;

    // [SCRUM-29] Generate this code with cross compiler
    output.out_emissive_roughness_metallic.r = u_emissive_power;
    output.out_emissive_roughness_metallic.g = roughness;
    output.out_emissive_roughness_metallic.b = metallic;
    return output;
}
