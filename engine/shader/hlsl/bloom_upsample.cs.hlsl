/// File: bloom_upsample.cs.hlsl
#include "sampler.hlsl.h"

Texture2D t_BloomInputTexture : register(t0);
RWTexture2D<float3> u_BloomOutputImage : register(u0);

[numthreads(16, 16, 1)] void main(uint3 dispatch_thread_id : SV_DISPATCHTHREADID) {
    const uint2 output_coord = dispatch_thread_id.xy;

    uint width, height;
    u_BloomOutputImage.GetDimensions(width, height);
    float2 output_image_size = float2(width, height);

    float2 uv = float2(output_coord.x / output_image_size.x,
                       output_coord.y / output_image_size.y);

    uint input_width, input_height;
    t_BloomInputTexture.GetDimensions(input_width, input_height);
    float x = 1.0f / input_width;
    float y = 1.0f / input_height;
    uv.x += 0.5f * x;
    uv.y += 0.5f * y;

    float filterRadius = 0.005;
    x = filterRadius;
    y = filterRadius;

    // Take 9 samples around current texel:
    // a - b - c
    // d - e - f
    // g - h - i
    // === ('e' is the current texel) ===
    float3 a = t_BloomInputTexture.SampleLevel(s_linearClampSampler, float2(uv.x - x, uv.y + y), 0).rgb;
    float3 b = t_BloomInputTexture.SampleLevel(s_linearClampSampler, float2(uv.x, uv.y + y), 0).rgb;
    float3 c = t_BloomInputTexture.SampleLevel(s_linearClampSampler, float2(uv.x + x, uv.y + y), 0).rgb;

    float3 d = t_BloomInputTexture.SampleLevel(s_linearClampSampler, float2(uv.x - x, uv.y), 0).rgb;
    float3 e = t_BloomInputTexture.SampleLevel(s_linearClampSampler, float2(uv.x, uv.y), 0).rgb;
    float3 f = t_BloomInputTexture.SampleLevel(s_linearClampSampler, float2(uv.x + x, uv.y), 0).rgb;

    float3 g = t_BloomInputTexture.SampleLevel(s_linearClampSampler, float2(uv.x - x, uv.y - y), 0).rgb;
    float3 h = t_BloomInputTexture.SampleLevel(s_linearClampSampler, float2(uv.x, uv.y - y), 0).rgb;
    float3 i = t_BloomInputTexture.SampleLevel(s_linearClampSampler, float2(uv.x + x, uv.y - y), 0).rgb;

    // Apply weighted distribution, by using a 3x3 tent filter:
    //  1   | 1 2 1 |
    // -- * | 2 4 2 |
    // 16   | 1 2 1 |
    float3 upsample = e * 4.0;
    upsample += (b + d + f + h) * 2.0;
    upsample += (a + c + g + i);
    upsample *= 1.0 / 16.0;

    float3 final_color = u_BloomOutputImage.Load(output_coord).rgb;
    final_color = lerp(final_color, upsample, 0.6);
    u_BloomOutputImage[output_coord] = float4(final_color, 1.0);
}
