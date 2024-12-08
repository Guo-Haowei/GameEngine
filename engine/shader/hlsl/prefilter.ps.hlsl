/// File: prefilter.ps.hlsl
#include "cbuffer.hlsl.h"
#include "hlsl/input_output.hlsl"
#include "pbr.hlsl.h"
#include "sampler.hlsl.h"
#include "shader_resource_defines.hlsl.h"

#define SAMPLE_COUNT 1024u

float4 main(vsoutput_position input) : SV_TARGET {
    float3 N = normalize(input.world_position);

    // make the simplyfying assumption that V equals R equals the normal
    float3 R = N;
    float3 V = R;

    float roughness = c_envPassRoughness;

    float3 prefilteredColor = float3(0.0, 0.0, 0.0);
    float totalWeight = 0.0;

    for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
        float2 Xi = Hammersley(i, SAMPLE_COUNT);
        float3 H = ImportanceSampleGGX(Xi, N, roughness);
        float3 L = reflect(-V, H);

        float NdotL = dot(N, L);
        if (NdotL > 0.0) {
            // sample from the environment's mip level based on roughness/pdf
            float NdotH = max(dot(N, H), 0.0);
            float D = DistributionGGX(NdotH, roughness);
            float HdotV = max(dot(H, V), 0.0);
            float pdf = D * NdotH / (4.0 * HdotV) + 0.0001;

            // @TODO: fix hard code
            float resolution = 512.0;  // resolution of source cubemap (per face)
            float saTexel = 4.0 * MY_PI / (6.0 * resolution * resolution);
            float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);

            float mipLevel = roughness == 0.0 ? 0.0 : 0.5 * log2(saSample / saTexel);

            prefilteredColor += TEXTURE_CUBE(Skybox).SampleLevel(s_cubemapClampSampler, L, mipLevel).rgb * NdotL;
            totalWeight += NdotL;
        }
    }

    prefilteredColor = prefilteredColor / totalWeight;
    return float4(prefilteredColor, 1.0);
}