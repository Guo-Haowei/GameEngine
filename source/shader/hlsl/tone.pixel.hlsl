/// File: tone.pixel.hlsl
#include "cbuffer.hlsl.h"
#include "hlsl/input_output.hlsl"
#include "sampler.hlsl.h"
#include "texture_binding.hlsl.h"

float4 main(vsoutput_uv input) : SV_TARGET {
    const float v = 1.0 / 2.0;
    const float3 gamma = float3(v, v, v);

    float2 uv = input.uv;
    // flip uv
    uv.y = 1 - uv.y;

    float3 color = t_textureLighting.Sample(s_linearClampSampler, uv).rgb;

    if (c_enableBloom == 1) {
        float3 bloom = t_bloomInputImage.Sample(s_linearClampSampler, uv).rgb;
        color += bloom;
    }

    color = color / (color + float3(1.0, 1.0, 1.0));
    color = pow(color, gamma);

    return float4(color, 1.0);
}
