/// File: debug_draw.ps.hlsl
#include "hlsl/input_output.hlsl"

float4 main(VS_OUTPUT_COLOR input) : SV_TARGET {
    return input.color;
}
