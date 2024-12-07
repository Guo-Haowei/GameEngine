/// File: cube_map.vs.hlsl
#include "cbuffer.hlsl.h"
#include "hlsl/input_output.hlsl"

vsoutput_position main(vsinput_position input) {
    vsoutput_position output;
    output.position = mul(c_cubeProjectionViewMatrix, float4(input.position, 1.0));
    output.world_position = input.position;
    return output;
}
