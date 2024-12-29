#include "../cbuffer.hlsl.h"
#include "../shader_resource_defines.hlsl.h"

layout(location = 0) in vec2 pass_uv;
layout(location = 0) out float out_color;

// @TODO: fix HARD CODE
#define SSAO_KERNEL_BIAS 0.025f

void main() {
    const Vector2f uv = pass_uv;

    Vector2i texture_size = textureSize(t_GbufferNormalMap, 0);
    Vector2f noise_scale = Vector2f(texture_size);
    noise_scale *= (1.0f / SSAO_NOISE_SIZE);

    noise_scale = vec2(800 / 4, 600 / 4);

    Vector3f N = texture(t_GbufferNormalMap, uv).rgb;
    N = 2.0f * N - 1.0f;

    Vector3f rvec = texture(t_BaseColorMap, uv * noise_scale).xyz;
    Vector3f tangent = normalize(rvec - N * dot(rvec, N));
    Vector3f bitangent = cross(N, tangent);

    float D = texture(t_GbufferDepth, uv).r;
    mat3 TBN = mat3(tangent, bitangent, N);
    TBN = mat3(c_viewMatrix) * TBN;

    // @TODO: reconstruct depth from matrix
    Vector4f origin = Vector4f(texture(t_GbufferPositionMap, uv).xyz, 1.0f);

    float occlusion = 0.0;
    for (int i = 0; i < SSAO_KERNEL_SIZE; ++i) {
        // get sample position
        vec3 samplePos = TBN * c_ssaoKernel[i].xyz;  // from tangent to view-space
        samplePos = origin.xyz + samplePos * c_ssaoKernalRadius;

        // project sample position (to sample texture) (to get position on screen/texture)
        vec4 offset = vec4(samplePos, 1.0);
        offset = c_projectionMatrix * offset;  // from view to clip-space
        offset.xyz /= offset.w;                // perspective divide
        offset.xy = offset.xy * 0.5 + 0.5;     // transform to range 0.0 - 1.0

        float sample_depth = texture(t_GbufferPositionMap, offset.xy).z;

        // range check & accumulate
        float range_check = smoothstep(0.0, 1.0, c_ssaoKernalRadius / abs(origin.z - sample_depth));
        float increment = (sample_depth >= (samplePos.z + SSAO_KERNEL_BIAS)) ? 1.0f : 0.0f;
        occlusion += increment * range_check;
    }

    occlusion = 1.0 - (occlusion / float(SSAO_KERNEL_SIZE));
    out_color = occlusion;
}
