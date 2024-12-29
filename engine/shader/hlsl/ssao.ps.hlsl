/// File: tone.ps.hlsl
#include "cbuffer.hlsl.h"
#include "hlsl/input_output.hlsl"
#include "sampler.hlsl.h"
#include "shader_resource_defines.hlsl.h"

// @TODO: fix HARD CODE
#define SSAO_KERNEL_BIAS 0.025f

float main(vsoutput_uv input) : SV_TARGET {
    const Vector2f uv = input.uv;

    Vector2i texture_size;
    TEXTURE_2D(GbufferNormalMap).GetDimensions(texture_size.x, texture_size.y);
    Vector2f noise_scale = Vector2f(texture_size);
    noise_scale *= (1.0f / SSAO_NOISE_SIZE);

    Vector3f N = TEXTURE_2D(GbufferNormalMap).Sample(s_pointClampSampler, uv).rgb;
    N = 2.0f * N - 1.0f;

    Vector3f rvec = Vector3f(TEXTURE_2D(BaseColorMap).SampleLevel(s_pointWrapSampler, uv * noise_scale, 0.0f).xy, 0.0f);
    Vector3f tangent = normalize(rvec - N * dot(rvec, N));
    Vector3f bitangent = cross(N, tangent);

    const Vector3f origin = TEXTURE_2D(GbufferPositionMap).Sample(s_pointClampSampler, uv).xyz;

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
        offset = mul(c_projectionMatrix, offset);  // from view to clip-space
        offset.xyz /= offset.w;                    // perspective divide
        offset.y = -offset.y;
        offset.xy = offset.xy * 0.5 + 0.5;  // transform to range 0.0 - 1.0

        float sample_depth = TEXTURE_2D(GbufferPositionMap).Sample(s_pointClampSampler, offset.xy).z;
        // return sample_depth;

        // const float range_check = smoothstep(0.0, 1.0, c_ssaoKernalRadius / abs(origin.z - sample_depth));
        const float increment = (+(sample_depth - samplePos.z) <= SSAO_KERNEL_BIAS) ? 1.0f : 0.0f;
        // const float increment = (sample_depth >= (samplePos.z + SSAO_KERNEL_BIAS)) ? 1.0f : 0.0f;
        occlusion += increment;
    }

    return occlusion;
}
