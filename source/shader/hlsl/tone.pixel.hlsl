#include "cbuffer.h"
#include "hlsl/input_output.hlsl"
#include "texture_binding.h"

// @TODO: fix sampler
SamplerState u_sampler : register(s0);

float4 main(vsoutput_uv input) : SV_TARGET {
    const float v = 1.0 / 2.0;
    const float3 gamma = float3(v, v, v);

    float2 uv = input.uv;
    // flip uv
    uv.y = 1 - uv.y;

    float3 color = g_texture_lighting.Sample(u_sampler, uv).rgb;

    color = color / (color + float3(1.0, 1.0, 1.0));
    color = pow(color, gamma);

    return float4(color, 1.0);
}
