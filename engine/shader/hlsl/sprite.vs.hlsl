/// File: sprite.vs.hlsl
#include "cbuffer.hlsl.h"
#include "hlsl/input_output.hlsl"

VS_OUTPUT_UV main(VS_INPUT_SPRITE input) {
    float4 position = float4(input.position, -1.0f, 1.0f);
    position = mul(c_worldMatrix, position);
    position = mul(c_viewMatrix, position);
    position = mul(c_projectionMatrix, position);
    VS_OUTPUT_UV output;
    output.position = position;
    output.uv = input.uv;
    return output;
}
