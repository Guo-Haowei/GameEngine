/// File: debug_draw_texture.vs.hlsl
#include "cbuffer.hlsl.h"
#include "hlsl/input_output.hlsl"

vsoutput_uv main(vsinput_position input) {
    float2 position = input.position;
    position *= c_debugDrawSize;
    position += c_debugDrawPos;
    vsoutput_uv output;
    output.position = float4(position, 0.0f, 1.0f);
    output.uv = 0.5 * (input.position + 1.0);
    return output;
}
