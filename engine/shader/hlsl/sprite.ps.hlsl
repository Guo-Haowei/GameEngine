/// File: sprite.ps.hlsl
#include "hlsl/input_output.hlsl"

float4 main(vsoutput_uv input) : SV_TARGET {
    // return float4(1.0f, 1.0f, 1.0f, 1.0f);
    return float4(1.0f, 0.0f, 1.0f, 1.0f);
}
