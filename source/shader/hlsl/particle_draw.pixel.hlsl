/// File: particle_draw.pixel.hlsl
#include "hlsl/input_output.hlsl"

float4 main(vsoutput_color input) : SV_TARGET {
    return float4(input.color.rgb, 1.0);
}
