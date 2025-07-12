/// File: particle_draw.ps.hlsl
#include "cbuffer.hlsl.h"
#include "hlsl/input_output.hlsl"
#include "sampler.hlsl.h"
#include "shader_resource_defines.hlsl.h"

float4 main(VS_OUTPUT_UV input)
    : SV_TARGET {
    float2 texcoord = input.uv;
    if (c_emitterUseTexture == 1) {
        float4 base_color = TEXTURE_2D(BaseColorMap).Sample(s_linearMipWrapSampler, texcoord);
        clip(base_color.a - 0.1);
        return float4(3, 3, 3, 1) * base_color * c_particleColor;
    } else {
        return c_particleColor;
    }
    // clip(0.1 - base_color.a);
}
