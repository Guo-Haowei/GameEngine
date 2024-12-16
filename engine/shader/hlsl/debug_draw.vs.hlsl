/// File: debug_draw.vs.hlsl
#include "cbuffer.hlsl.h"
#include "hlsl/input_output.hlsl"

vsoutput_color main(vsinput_color input) {
    float4 position = float4(input.position, 1.0);
    position = mul(c_viewMatrix, position);
    position = mul(c_projectionMatrix, position);
    vsoutput_color output;
    output.color = input.color;
    output.position = position;
    return output;
}