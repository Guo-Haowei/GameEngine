/// File: post_process.ps.hlsl
#include "cbuffer.hlsl.h"
#include "hlsl/input_output.hlsl"
#include "sampler.hlsl.h"
#include "shader_resource_defines.hlsl.h"

Texture2D t_TextureLighting : register(t0);
Texture2D t_TextureHighlightSelect : register(t1);
Texture2D t_BloomInputTexture : register(t2);

static const float3x3 sx = float3x3(
    1.0, 2.0, 1.0,
    0.0, 0.0, 0.0,
    -1.0, -2.0, -1.0);
static const float3x3 sy = float3x3(
    1.0, 0.0, -1.0,
    2.0, 0.0, -2.0,
    1.0, 0.0, -1.0);

float4 main(vsoutput_uv input) : SV_TARGET {
    float2 uv = input.uv;
    // flip uv
    // uv.y = 1 - uv.y;
    // Edge detection
    int width, height;
    t_TextureHighlightSelect.GetDimensions(width, height);
    float2 texel_size = float2(width, height);
    texel_size = 1.0 / texel_size;

    float3x3 I;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            float2 offset = uv + texel_size * float2(i - 1, j - 1);
            I[i][j] = t_TextureHighlightSelect.Sample(s_linearClampSampler, offset).r;
        }
    }

    float gx = dot(sx[0], I[0]) + dot(sx[1], I[1]) + dot(sx[2], I[2]);
    float gy = dot(sy[0], I[0]) + dot(sy[1], I[1]) + dot(sy[2], I[2]);

    float g = sqrt(pow(gx, 2.0) + pow(gy, 2.0));
    if (g > 0.0) {
        return float4(0.98, 0.64, 0.0, 1.0);
    }

    float3 color = t_TextureLighting.Sample(s_linearClampSampler, uv).rgb;

    // Bloom

    if (c_enableBloom == 1) {
        float3 bloom = t_BloomInputTexture.Sample(s_linearClampSampler, uv).rgb;
        color += bloom;
    }

    // Gamma correction
    // HDR tonemapping
    color = color / (color + float3(1.0f, 1.0f, 1.0f));
    const float gamma = 2.2f;
    color = pow(color, 1.0f / float3(gamma, gamma, gamma));

    return float4(color, 1.0);
}
