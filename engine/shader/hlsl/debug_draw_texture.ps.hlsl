/// File: debug_draw_texture.ps.hlsl
#include "cbuffer.hlsl.h"
#include "hlsl/input_output.hlsl"
#include "sampler.hlsl.h"
#include "shader_resource_defines.hlsl.h"

Texture2D t_Texture0 : register(t0);

float4 main(VS_OUTPUT_UV input)
    : SV_TARGET {
    float4 color = t_Texture0.SampleLevel(s_linearClampSampler, input.uv, 0.0f);

    switch (c_displayChannel) {
        case DISPLAY_CHANNEL_RRR:
            return float4(color.rrr, 1.0);
        case DISPLAY_CHANNEL_RGB:
            return float4(color.rgb, 1.0);
        default:
            return float4(color.rgb, 1.0);
    }
}
