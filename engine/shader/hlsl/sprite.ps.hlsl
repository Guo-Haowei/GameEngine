/// File: sprite.ps.hlsl
#include "hlsl/input_output.hlsl"
#include "sampler.hlsl.h"

Texture2D t_Sprite : register(t0);

float4 main(VS_OUTPUT_UV input) : SV_TARGET {
    float4 color = t_Sprite.Sample(s_pointClampSampler, input.uv);

    if (color.a < 0.01f) {
        discard;
    }

    return float4(color.rgb, 1.0f);
}
