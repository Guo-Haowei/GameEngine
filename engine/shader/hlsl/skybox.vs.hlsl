/// File: skybox.vs.hlsl
#include "cbuffer.hlsl.h"
#include "hlsl/input_output.hlsl"

vsoutput_position main(vsinput_mesh input) {
    float3x3 rotate = (float3x3)c_viewMatrix;
    float3 position3 = mul(rotate, input.position);
    float4 position = mul(c_projectionMatrix, float4(position3, 1.0));

    vsoutput_position output;
    output.position = position.xyww;
    output.world_position = input.position;
    return output;
}
