/// File: sky.vs.hlsl
#include "cbuffer.hlsl.h"
#include "hlsl/input_output.hlsl"

vsoutput_position main(vsinput_position input) {
    vsoutput_position output;
    output.position = float4(input.position.xy, 0.0f, 1.0f);
    float4 tmp = mul(c_invProjection, output.position);
    tmp.w = 0.0f;
    tmp = mul(tmp, c_viewMatrix);
    output.world_position = tmp.xyz;
    return output;
}
