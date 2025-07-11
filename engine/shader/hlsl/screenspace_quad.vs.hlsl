/// File: screenspace_quad.vs.hlsl
#include "hlsl/input_output.hlsl"

VS_OUTPUT_UV main(VS_INPUT_POS input) {
    VS_OUTPUT_UV output;
    output.position = float4(input.position, 1.0);
    output.uv = 0.5 * (input.position.xy + 1.0);
    output.uv.y = 1.0 - output.uv.y;
    return output;
}
