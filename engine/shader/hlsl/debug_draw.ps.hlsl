/// File: debug_draw.ps.hlsl
#include "hlsl/input_output.hlsl"

float4 main(vsoutput_color input) : SV_TARGET {
    return input.color;
}
