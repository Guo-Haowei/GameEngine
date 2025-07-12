/// File: ssao.ps.hlsl
#include "cbuffer.hlsl.h"
#include "common.hlsl.h"
#include "hlsl/input_output.hlsl"
#include "sampler.hlsl.h"

Texture2D t_GbufferNormalMap : register(t0);
Texture2D t_GbufferDepth : register(t1);
Texture2D t_NoiseTexture : register(t2);

// @TODO: fix HARD CODE
#define SSAO_KERNEL_BIAS 0.025f

float main(VS_OUTPUT_UV input)
    : SV_TARGET {
    const Vector2f uv = input.uv;

    Vector2i texture_size;
    t_GbufferNormalMap.GetDimensions(texture_size.x, texture_size.y);
    Vector2f noise_scale = Vector2f(texture_size);
    noise_scale *= (1.0f / SSAO_NOISE_SIZE);

    Vector3f N = t_GbufferNormalMap.Sample(s_pointClampSampler, uv).rgb;
    N = 2.0f * N - 1.0f;

    // reconstruct view position
    // https://stackoverflow.com/questions/11277501/how-to-recover-view-space-position-given-view-space-depth-value-and-ndc-xy
    const float depth = t_GbufferDepth.Sample(s_pointClampSampler, uv).r;
    const Vector3f origin = NdcToViewPos(uv, depth);

    Vector3f rvec = Vector3f(t_NoiseTexture.SampleLevel(s_pointWrapSampler, uv * noise_scale, 0.0f).xy, 0.0f);
    Vector3f tangent = normalize(rvec - N * dot(rvec, N));
    Vector3f bitangent = cross(N, tangent);

    float3x3 TBN = float3x3(tangent, bitangent, N);

#if 0
    return mul(Vector3f(0, 0, 1), TBN).b;
#endif

    float occlusion = 0.0;

    for (int i = 0; i < SSAO_KERNEL_SIZE; ++i) {
        // get sample position
        Vector3f samplePos = mul(c_ssaoKernel[i].xyz, TBN);  // from tangent to view-space
        samplePos = origin.xyz + samplePos * c_ssaoKernalRadius;

        // project sample position (to sample texture) (to get position on screen/texture)
        Vector4f offset = Vector4f(samplePos, 1.0);
        offset = mul(c_camProj, offset);    // from view to clip-space
        offset /= offset.w;                 // perspective divide
        offset.xy = offset.xy * 0.5 + 0.5;  // transform to range 0.0 - 1.0

        const float depth2 = t_GbufferDepth.Sample(s_pointClampSampler, offset.xy).r;
        const Vector3f sampleOcclusionPos = NdcToViewPos(offset.xy, depth2);
        const float sample_depth = sampleOcclusionPos.z;

        const float range_check = smoothstep(0.0, 1.0, c_ssaoKernalRadius / abs(origin.z - sample_depth));
        const float increment = sample_depth - samplePos.z >= SSAO_KERNEL_BIAS ? 1.0f : 0.0f;
        occlusion += increment;
        // occlusion += increment * range_check;
    }

    occlusion = 1.0 - (occlusion / float(SSAO_KERNEL_SIZE));
    return occlusion;
}
