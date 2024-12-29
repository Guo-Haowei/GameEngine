/// File: forward.ps.hlsl
#include "cbuffer.hlsl.h"
#include "hlsl/input_output.hlsl"
#include "hlsl/lighting.hlsl"
#include "sampler.hlsl.h"
#include "shader_resource_defines.hlsl.h"

float4 main(vsoutput_mesh input) : SV_TARGET {
    const float2 texcoord = input.uv;
    const float4 world_position = mul(c_invViewMatrix, float4(input.world_position, 1.0f));

    float3 base_color = c_baseColor.rgb;
    float alpha = c_baseColor.a;

    if (c_hasBaseColorMap) {
        float4 tmp = TEXTURE_2D(BaseColorMap).Sample(s_linearMipWrapSampler, input.uv);
        base_color = tmp.rgb;
        alpha = tmp.a;
    }

    float metallic;
    float roughness;

    if (c_hasMaterialMap != 0) {
        float3 value = TEXTURE_2D(MaterialMap).Sample(s_linearMipWrapSampler, input.uv).rgb;
        metallic = value.b;
        roughness = value.g;
    } else {
        metallic = c_metallic;
        roughness = c_roughness;
    }

    float3 N;
    if (c_hasNormalMap != 0) {
        float3x3 TBN;
        TBN[0] = normalize(input.T);
        TBN[1] = normalize(input.B);
        TBN[2] = normalize(input.normal);
        float3 value = TEXTURE_2D(NormalMap).Sample(s_linearMipWrapSampler, input.uv).rgb;
        N = normalize(mul((2.0f * value) - 1.0f, TBN));
    } else {
        N = normalize(input.normal);
    }

    float3 color = compute_lighting(base_color,
                                    world_position.xyz,
                                    N,
                                    metallic,
                                    roughness,
                                    c_emissivePower);
    return float4(color, alpha);
}
