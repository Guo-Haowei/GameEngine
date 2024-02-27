#include "vsoutput.hlsl"

float4 main(vsoutput_mesh input) : SV_TARGET {
    float3 N = normalize(input.normal);
    float3 color = N + float3(1.0, 1.0, 1.0);
    color = 0.5 * color;

    return float4(color, 1.0);
}