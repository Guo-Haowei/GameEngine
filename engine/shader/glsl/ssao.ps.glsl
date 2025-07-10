#include "../cbuffer.hlsl.h"
#include "../common.hlsl.h"

layout(location = 0) in vec2 pass_uv;
layout(location = 0) out float out_color;

uniform sampler2D u_Texture0;
uniform sampler2D u_Texture1;
uniform sampler2D u_Texture2;

#define t_GbufferNormalMap u_Texture0
#define t_GbufferDepth     u_Texture1
#define t_NoiseTexture     u_Texture2

// @TODO: fix HARD CODE
#define SSAO_KERNEL_BIAS 0.025f

void main() {
    const Vector2f uv = pass_uv;

    Vector2i texture_size = textureSize(t_GbufferNormalMap, 0);
    Vector2f noise_scale = Vector2f(texture_size);
    noise_scale *= (1.0f / SSAO_NOISE_SIZE);

    Vector3f N = texture(t_GbufferNormalMap, uv).rgb;
    N = 2.0f * N - 1.0f;

    // @TODO: ?
    Vector3f rvec = Vector3f(texture(t_NoiseTexture, uv * noise_scale).xy, 0.0f);
    Vector3f tangent = normalize(rvec - N * dot(rvec, N));
    Vector3f bitangent = cross(N, tangent);

    mat3 TBN = mat3(tangent, bitangent, N);

    // Reconstruct view position
    // https://stackoverflow.com/questions/11277501/how-to-recover-view-space-position-given-view-space-depth-value-and-ndc-xy
    const float depth = texture(t_GbufferDepth, uv).r;
    const Vector3f origin = NdcToViewPos(uv, depth);

#if 0
    out_color = (TBN * Vector3f(0, 0, 1)).b;
    return;
#endif

    float occlusion = 0.0;
    for (int i = 0; i < SSAO_KERNEL_SIZE; ++i) {
        // get sample position
        vec3 samplePos = TBN * c_ssaoKernel[i].xyz;  // from tangent to view-space
        samplePos = origin.xyz + samplePos * c_ssaoKernalRadius;

        // project sample position (to sample texture) (to get position on screen/texture)
        vec4 offset = vec4(samplePos, 1.0);
        offset = c_camProj * offset;        // from view to clip-space
        offset /= offset.w;                 // perspective divide
        offset.xy = offset.xy * 0.5 + 0.5;  // transform to range 0.0 - 1.0

        const float depth2 = texture(t_GbufferDepth, offset.xy).r;
        const Vector3f sampleOcclusionPos = NdcToViewPos(offset.xy, depth2);
        const float sample_depth = sampleOcclusionPos.z;

        const float range_check = smoothstep(0.0, 1.0, c_ssaoKernalRadius / abs(origin.z - sample_depth));
        const float increment = sample_depth - samplePos.z >= SSAO_KERNEL_BIAS ? 1.0f : 0.0f;
        occlusion += increment * range_check;
    }

    occlusion = 1.0 - (occlusion / float(SSAO_KERNEL_SIZE));
    out_color = occlusion;
}
