/// File: debug_draw_texture.vs.hlsl
#include "cbuffer.hlsl.h"
#include "hlsl/input_output.hlsl"

VS_OUTPUT_UV main(VS_INPUT_POS input) {
    float2 position = input.position.xy;
    position *= c_debugDrawSize;
    position += c_debugDrawPos;
    VS_OUTPUT_UV output;
    output.position = float4(position, 0.0f, 1.0f);
    output.uv = 0.5f * (input.position.xy + 1.0f);
    // flip when hlsl
#if !defined(HLSL_2_GLSL)
    output.uv.y = 1.0f - output.uv.y;
#endif
    return output;
}
