/// File: tone.ps.hlsl
#include "cbuffer.hlsl.h"
#include "hlsl/input_output.hlsl"
#include "sampler.hlsl.h"
#include "shader_resource_defines.hlsl.h"

#if 1
static const float3x3 sx = float3x3(
    1.0, 2.0, 1.0,
    0.0, 0.0, 0.0,
    -1.0, -2.0, -1.0);
static const float3x3 sy = float3x3(
    1.0, 0.0, -1.0,
    2.0, 0.0, -2.0,
    1.0, 0.0, -1.0);
#else
static const float3x3 sx = float3x3(
    1.0, 0.0, -1.0,
    2.0, 0.0, -2.0,
    1.0, 0.0, -1.0);
static const float3x3 sy = float3x3(
    1.0, 2.0, 1.0,
    0.0, 0.0, 0.0,
    -1.0, -2.0, -1.0);
#endif

float4 main(vsoutput_uv input) : SV_TARGET {
    float2 uv = input.uv;
    // flip uv
    // uv.y = 1 - uv.y;
    // Edge detection
    int width, height;
    TEXTURE_2D(TextureHighlightSelect).GetDimensions(width, height);
    float2 texel_size = float2(width, height);
    texel_size = 1.0 / texel_size;

    float3x3 I;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            float2 offset = uv + texel_size * float2(i - 1, j - 1);
            I[i][j] = TEXTURE_2D(TextureHighlightSelect).Sample(s_linearClampSampler, offset).r;
        }
    }

    float gx = dot(sx[0], I[0]) + dot(sx[1], I[1]) + dot(sx[2], I[2]);
    float gy = dot(sy[0], I[0]) + dot(sy[1], I[1]) + dot(sy[2], I[2]);

    float g = sqrt(pow(gx, 2.0) + pow(gy, 2.0));
    if (g > 0.0) {
        return float4(0.98, 0.64, 0.0, 0.0);
    }

    float3 color = TEXTURE_2D(TextureLighting).Sample(s_linearClampSampler, uv).rgb;

    // Bloom

#ifndef HLSL_LANG_D3D12
    if (c_enableBloom == 1) {
        float3 bloom = TEXTURE_2D(BloomInputTexture).Sample(s_linearClampSampler, uv).rgb;
        color += bloom;
    }
#endif

    // Gamma correction
    const float v = 1.0 / 2.0;
    const float3 gamma = float3(v, v, v);
    color = color / (color + float3(1.0, 1.0, 1.0));
    color = pow(color, gamma);

    return float4(color, 1.0);
}
