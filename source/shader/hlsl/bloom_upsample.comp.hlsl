/// File: bloom_upsample.comp.hlsl
#include "sampler.hlsl.h"
#include "texture_binding.h"

RWTexture2D<float3> g_output_image : register(u3);

[numthreads(16, 16, 1)] void main(uint3 dispatch_thread_id
                                  : SV_DISPATCHTHREADID) {
    const uint2 output_coord = dispatch_thread_id.xy;

    uint width, height;
    g_output_image.GetDimensions(width, height);
    float2 output_image_size = float2(width, height);

    float2 uv = float2(output_coord.x / output_image_size.x,
                       output_coord.y / output_image_size.y);

    uint input_width, input_height;
    t_bloomInputImage.GetDimensions(input_width, input_height);
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
    float3 a = t_bloomInputImage.SampleLevel(linear_clamp_sampler, float2(uv.x - x, uv.y + y), 0).rgb;
    float3 b = t_bloomInputImage.SampleLevel(linear_clamp_sampler, float2(uv.x, uv.y + y), 0).rgb;
    float3 c = t_bloomInputImage.SampleLevel(linear_clamp_sampler, float2(uv.x + x, uv.y + y), 0).rgb;

    float3 d = t_bloomInputImage.SampleLevel(linear_clamp_sampler, float2(uv.x - x, uv.y), 0).rgb;
    float3 e = t_bloomInputImage.SampleLevel(linear_clamp_sampler, float2(uv.x, uv.y), 0).rgb;
    float3 f = t_bloomInputImage.SampleLevel(linear_clamp_sampler, float2(uv.x + x, uv.y), 0).rgb;

    float3 g = t_bloomInputImage.SampleLevel(linear_clamp_sampler, float2(uv.x - x, uv.y - y), 0).rgb;
    float3 h = t_bloomInputImage.SampleLevel(linear_clamp_sampler, float2(uv.x, uv.y - y), 0).rgb;
    float3 i = t_bloomInputImage.SampleLevel(linear_clamp_sampler, float2(uv.x + x, uv.y - y), 0).rgb;

    // Apply weighted distribution, by using a 3x3 tent filter:
    //  1   | 1 2 1 |
    // -- * | 2 4 2 |
    // 16   | 1 2 1 |
    float3 upsample = e * 4.0;
    upsample += (b + d + f + h) * 2.0;
    upsample += (a + c + g + i);
    upsample *= 1.0 / 16.0;

    float3 final_color = g_output_image.Load(output_coord).rgb;
    g_output_image[output_coord] = lerp(final_color, upsample, 0.6);
}
