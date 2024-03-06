#include "cbuffer.h"
#include "hlsl/input_output.hlsl"
#include "texture_binding.h"

// @TODO sampler
SamplerState u_sampler : register(s0);

float4 main(vsoutput_uv input) : SV_TARGET {
    float2 uv = input.uv;

    float3 base_color = u_gbuffer_base_color_map.Sample(u_sampler, uv).rgb;

    float3 N = u_gbuffer_normal_map.Sample(u_sampler, uv);
    float3 color = 0.5 * (N + float3(1.0, 1.0, 1.0));

    color = base_color;
    return float4(color, 1.0);
}
