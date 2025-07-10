/// File: sprite.vs.hlsl
#include "cbuffer.hlsl.h"
#include "hlsl/input_output.hlsl"

vsoutput_uv main(VS_INPUT_SPRITE input) {
    float4 position = float4(input.position, -1.0f, 1.0f);
    position = mul(c_viewMatrix, position);
    position = mul(c_projectionMatrix, position);
    // position = mul(c_camView, position);
    // position = mul(c_camProj, position);
    vsoutput_uv output;
    output.position = position;
    output.uv = input.uv;
    return output;
}
