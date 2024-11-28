/// File: screenspace_quad.vs.hlsl
#include "hlsl/input_output.hlsl"

vsoutput_uv main(vsinput_position input) {
    vsoutput_uv output;
    output.position = float4(input.position, 1.0);
    output.uv = 0.5 * (input.position.xy + 1.0);
    output.uv.y = 1.0 - output.uv.y;
    return output;
}
