/// File: skybox.vs.hlsl
#include "cbuffer.hlsl.h"
#include "hlsl/input_output.hlsl"

VS_OUTPUT_POSITION main(VS_INPUT_MESH input) {
    float3x3 rotate = (float3x3)c_camView;
    float3 position3 = mul(rotate, input.position);
    float4 position = mul(c_camProj, float4(position3, 1.0));

    VS_OUTPUT_POSITION output;
    output.position = position.xyww;
    output.world_position = input.position;
    return output;
}
