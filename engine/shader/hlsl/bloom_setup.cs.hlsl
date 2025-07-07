/// File: bloom_setup.cs.hlsl
#include "sampler.hlsl.h"

Texture2D t_TextureLighting : register(t0);
RWTexture2D<float3> u_BloomOutputImage : register(u0);

float rgb_to_luma(float3 rgb) {
    return sqrt(dot(rgb, float3(0.299, 0.587, 0.114)));
}

[numthreads(16, 16, 1)] void main(uint3 dispatch_thread_id : SV_DISPATCHTHREADID) {
    uint2 output_coord = dispatch_thread_id.xy;

    uint width, height;
    u_BloomOutputImage.GetDimensions(width, height);
    float2 output_image_size = float2(width, height);
    float2 uv = float2(output_coord.x / output_image_size.x,
                       output_coord.y / output_image_size.y);

    float3 color = t_TextureLighting.SampleLevel(s_linearClampSampler, uv, 0).rgb;
    float luma = rgb_to_luma(color);

    const float THRESHOLD = 1.3;
    if (luma < THRESHOLD) {
        color = float3(0.0, 0.0, 0.0);
    }

    // @TODO: fix bloom
    u_BloomOutputImage[output_coord] = float4(color, 1.0);
}